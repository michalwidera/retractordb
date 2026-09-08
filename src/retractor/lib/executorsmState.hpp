#pragma once

#include <atomic>
#include <condition_variable>
#include <cstdint>
#include <memory>
#include <mutex>
#include <string>
#include <utility>
#include <vector>

/// Stan wspolny wykonawcy planu, dzielony przez jednostki, na ktore rozpada sie implementacja
/// `executorsm`: petle epok (executorsm.cpp), dyspozytor komend (executorsmCommands.cpp), kanal
/// ad-hoc (executorsmAdHoc.cpp) i kanal przeladowania planu (executorsmPlanReload.cpp).
///
/// Obiekty dziela sie na dwie grupy i granica miedzy nimi jest istotna:
///  * grupa PIERWSZA to zmienne, po ktore siegaja takze inne jednostki biblioteki
///    (dataModel, dumpManager, presenter, streamInstance, launcher). Kazda z nich deklarowala
///    je dawniej u siebie wlasnym `extern` — piec niezaleznych deklaracji tego samego obiektu,
///    ktorych rozjazd typu jest naruszeniem ODR niewidocznym dla kompilatora. Teraz wszystkie
///    biora je stad i to jest jedyne zrodlo. Zostaja w przestrzeni globalnej: `esm::` kazalaby
///    kwalifikowac kazde uzycie w tamtych jednostkach, nic w zamian nie dajac;
///  * grupa DRUGA to stan widziany wylacznie przez jednostki wykonawcy. Siedzi w przestrzeni
///    `esm`, zeby nazwy pokroju `cv` czy `busPtr` nie stawaly sie symbolami globalnymi
///    biblioteki. Jednostki wykonawcy wchodza w nia przez `using namespace esm;`.

class dataModel;
class FlockServiceGuard;
class IpcServer;
class PersistentCounter;
namespace bus {
class Bus;
}

//
// Grupa pierwsza: stan dzielony takze poza wykonawca.
//

/// Licznik rotacji planu; pusty, gdy plan nie niesie `:ROTATION`. Czyta go streamInstance.
extern std::unique_ptr<PersistentCounter> pCounterPtr;

/// Muteks stanu planu; definiowany w dataModel.cpp, ktory takze wlacza ten naglowek — dzieki
/// temu definicja jest sprawdzana wzgledem deklaracji. Kolejnosc zagniezdzenia wzgledem
/// plan_epoch_mutex opisana nizej.
extern std::mutex core_mutex;

/// Wiersze zrodlowe planu (id strumienia -> tekst zapytania). Czyta je presenter i launcher.
extern std::vector<std::pair<std::string, std::string>> processedLines;

/// Model danych BIEZACEJ epoki; null w trybie bezczynnym i miedzy epokami — patrz plan_epoch_mutex.
extern dataModel *pProc;

// variable connected with llimitqry (-m) parameter
// counts remaining loop iterations; 0 = stop, inifitie_loop = run forever
extern std::atomic<int> iLoopLimitCnt;

//
// Grupa druga: stan wewnetrzny wykonawcy.
//

namespace esm {
/// Zycie EPOKI planu. Obiekt `dataModel` z petli epok i tresc `*coreInstancePtr` istnieja
/// tylko miedzy opublikowaniem pProc a jego zgaszeniem. core_mutex tego nie pilnuje: chroni
/// pojedyncza zmiane stanu, a handler komendy zwalnia go, ZANIM siegnie po model, i czyta
/// globalny pProc na nowo przy kazdym uzyciu. Watek przetwarzania zdazyl w tym oknie zgasic
/// wskaznik i rozebrac model -- `xqry -d` rownolegle z `xqry --reset` konczylo sie SIGSEGV
/// w dataModel::streamStoredSize (this=0x0), w polowie petli po strumieniach.
/// Ten muteks trzyma epoke w miejscu przez CALY czas obslugi komendy, a wymiana epoki czeka
/// na jego zwolnienie. Wystarcza muteks zwykly, bo handlery i tak sa szeregowane -- IpcServer
/// prowadzi dokladnie jedna petle odbioru komend. Kolejnosc zagniezdzenia jest zawsze
/// plan_epoch_mutex -> core_mutex.
extern std::mutex plan_epoch_mutex;

extern std::condition_variable cv;  // multithreading condition variable

/// Czy biezaca epoka W OGOLE zbuduje model: predykat czekania handlera komendy. Zdejmowane
/// pod core_mutex takze przy rozbiorce epoki, zeby komenda nie czekala na model, ktory juz
/// nie powstanie.
extern std::atomic<bool> dataModelExpected;

/// Zatrzask bramki --xqrywait: czy watek komunikacyjny odebral juz JAKAKOLWIEK komende.
/// Osobny od iLoopLimitCnt swiadomie -- patrz komentarz przy bramce w run().
extern std::atomic<bool> firstQueryReceived;

/// Rewizja planu zmieniona importem ad-hoc; petla slotow przebudowuje po niej os czasu.
extern std::atomic<std::uint64_t> adHocPlanRevision;

/// Tryb --until-eof calego przebiegu.
extern bool untilEofMode;

/// Zadanie przeladowania planu przyjete przez kanal IPC. Podnosi je resetCommit() po pelnej
/// walidacji, zdejmuje applyPendingPlan(). Petla epok traktuje je jak warunek konca epoki —
/// dokladnie tak samo jak `stop_now`, tyle ze po niej zaczyna sie epoka nastepna, nie koniec
/// procesu.
extern std::atomic<bool> planResetRequested;
/// Tresc przyjetego zestawu RQL. Chroniona przez core_mutex.
extern std::string pendingPlanText;

/// Czy trwa wymiana planu: od PRZYJECIA zestawu az do aktywacji jego rezerwacji na magistrali.
/// Osobna od planResetRequested, bo tamta gasnie na POCZATKU wymiany, a rezerwacja zyje jeszcze
/// przez cale budowanie planu. Roznica byla dziura: gniazdo magistrali trzyma DOKLADNIE JEDNA
/// rezerwacje, wiec reset przyjety w oknie miedzy zabraniem tekstu a activateReservedPlan()
/// nadpisywal rezerwacje planu wlasnie wchodzacego. Odchodzacy plan aktywowal wtedy cudza
/// rezerwacje (oglaszajac na magistrali nazwy, ktorych nie liczy), a nastepna epoka nie miala
/// juz czego aktywowac i konczyla sie FatalError -- w trybie --service takze wyczyszczeniem
/// pliku zapytan, czyli restartem uslugi BEZ planu. Odtworzone dwoma rownoleglymi `xqry -q`.
///
/// Flaga ma dokladnie jednego pisarza z kazdej strony: podnosi ja watek komunikacyjny
/// (IpcServer prowadzi jedna petle komend), zdejmuje watek glowny i dopiero PO aktywacji.
/// Odczyt "false" znaczy wiec, ze ani rezerwacja nie wisi, ani wymiana nie trwa -- sprawdzenie
/// w resetCommit() nie potrzebuje muteksu.
extern std::atomic<bool> planSwapInFlight;

/// Plik zapytan uslugi, do ktorego trafia przyjety plan i ktory jest oprozniany po bledzie
/// krytycznym. PUSTY dla instancji, ktora usluga nie jest — plik operatora uruchamiajacego
/// xretractor z terminala nie jest stanem uslugi i nie wolno go nadpisywac.
extern std::string serviceQueryFilePath;

// Transport IPC serwera. Obiekt o statycznym czasie zycia, bo sprzatanie musi byc
// osiagalne z handlera atexit (cleanup w executorsm.cpp): std::exit nie uruchamia destruktorow
// obiektow automatycznych, a destruktory obiektow statycznych wykonuja sie PO
// handlerach zarejestrowanych pozniej niz ich konstrukcja.
extern IpcServer ipcServer;

/// Straznik blokady uslugi — wskaznik wazny WYLACZNIE na czas trwania executorsm::run().
///
/// std::exit — przez ktory konczy sie FatalError — nie uruchamia destruktorow obiektow
/// AUTOMATYCZNYCH. Przy bledzie krytycznym cleanup() jest jedynym miejscem, ktore jeszcze
/// dziala, wiec to on musi zwolnic flock. Stabilny plik blokady pozostaje na dysku celowo.
///
/// Zerowany przed powrotem z run() (patrz lockGuardScope), i to jest wymog poprawnosci:
/// handlery atexit wykonuja sie PO zakonczeniu main, a straznik jest tam obiektem
/// automatycznym — po normalnym wyjsciu wskaznik wskazywalby na obiekt juz zniszczony.
extern FlockServiceGuard *serviceGuardPtr;

/// Magistrala xrdbbus — wskaznik wazny na tych samych zasadach co serviceGuardPtr powyzej.
/// Slot instancji musi zniknac takze na sciezce FatalError, inaczej martwy wpis blokowalby
/// nazwy strumieni az do chwili, gdy ktos go zauwazy i sprzatnie.
extern bus::Bus *busPtr;
}  // namespace esm
