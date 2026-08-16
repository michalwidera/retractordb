// Kanoniczny serializer metryki pierwotnej K26 — strona Flinka.
//
// Port `rdb::probe::canonicalRecordBytes` z retractordb (src/rdb/lib/probe.cc), ktore jest
// JEDYNYM miejscem definicji tej metryki. Zmiana odwzorowania wolna wylacznie tam; ten plik
// za nia idzie, a bramka zgodnosci to CanonTest wobec oracle/canonical_oracle.
//
// Odwzorowanie (probe.cc, canonicalFieldWidth):
//   BYTE = 1 B, INTEGER/UINT/DOUBLE = 8 B, FLOAT = 4 B, RATIONAL/INTPAIR = 16 B,
//   IDXPAIR = rlen*rarray + 8 B, STRING = rlen zadeklarowany (bez mnoznika rarray),
//   pola konfiguracyjne (TYPE, REF, RETENTION, RETMEMORY) i NULLTYPE = 0 B.
// Do sumy szerokosci dochodzi kanoniczna mapa NULL/luk: jeden bit na wartosc splaszczonego
// widoku pol, zaokraglony w gore do bajtu.
//
// Splaszczony widok pol (Descriptor::rebuildFieldMappings, descriptor.cc:42):
//   pole konfiguracyjne  -> 0 wartosci,
//   STRING               -> 1 wartosc niezaleznie od rarray,
//   pozostale (w tym NULLTYPE) -> rarray wartosci.
// NULLTYPE nie wnosi szerokosci, ale WNOSI wartosc do mapy — to nie jest pomylka, tak liczy
// silnik i tak musi liczyc Flink.
//
// Wynik zalezy wylacznie od deskryptora, nie od zawartosci rekordu: metryka bajtowa ma byc
// deterministycznym wynikiem mechanizmu.
public final class Canon {

  /** Typy pol — nazwy identyczne z rdb::descFld (src/include/fldType.hpp). */
  public enum Type {
    BYTE,
    INTEGER,
    UINT,
    RATIONAL,
    FLOAT,
    DOUBLE,
    INTPAIR,
    IDXPAIR,
    STRING,
    NULLTYPE,
    TYPE,
    REF,
    RETENTION,
    RETMEMORY
  }

  /** Pole deskryptora — odpowiednik rdb::rField. */
  public static final class Field {
    final String name;
    final int rlen;
    final int rarray;
    final Type rtype;

    public Field(String name, int rlen, int rarray, Type rtype) {
      this.name = name;
      this.rlen = rlen;
      this.rarray = rarray;
      this.rtype = rtype;
    }
  }

  /** Deskryptor — odpowiednik rdb::Descriptor (uporzadkowana lista pol). */
  public static final class Descriptor {
    final Field[] fields;

    public Descriptor(Field... fields) {
      this.fields = fields;
    }
  }

  private Canon() {}

  private static boolean isConfigurationField(Type type) {
    return type == Type.TYPE || type == Type.REF || type == Type.RETENTION || type == Type.RETMEMORY;
  }

  private static long fieldWidth(Field field) {
    long count = field.rarray;
    switch (field.rtype) {
      case BYTE:
        return count;
      case INTEGER:
      case UINT:
      case DOUBLE:
        return 8L * count;
      case FLOAT:
        return 4L * count;
      case RATIONAL:
      case INTPAIR:
        return 16L * count;
      case IDXPAIR:
        return (long) field.rlen * count + 8L;
      case STRING:
        return (long) field.rlen * count;
      default:
        return 0L; // NULLTYPE oraz pola konfiguracyjne
    }
  }

  private static long flatElementCount(Descriptor descriptor) {
    long values = 0;
    for (Field field : descriptor.fields) {
      if (isConfigurationField(field.rtype)) {
        continue;
      }
      values += (field.rtype == Type.STRING) ? 1 : field.rarray;
    }
    return values;
  }

  /** Kanoniczna szerokosc rekordu wraz z kanoniczna mapa NULL/luk. */
  public static long recordBytes(Descriptor descriptor) {
    long bytes = 0;
    for (Field field : descriptor.fields) {
      bytes += fieldWidth(field);
    }
    return bytes + (flatElementCount(descriptor) + 7L) / 8L;
  }

  /** Deskryptor jednopolowy INTEGER — rekord wszystkich strumieni rodzin K26 (9 B). */
  public static Descriptor singleInteger(String name) {
    return new Descriptor(new Field(name, 4, 1, Type.INTEGER));
  }

  /** Deskryptor dwupolowy INTEGER — rekord wyniku `+` dwoch strumieni jednopolowych (17 B). */
  public static Descriptor pairOfIntegers(String first, String second) {
    return new Descriptor(new Field(first, 4, 1, Type.INTEGER), new Field(second, 4, 1, Type.INTEGER));
  }

  //
  // ─── Liczniki logicznych zapisow (odpowiednik probe::logicalWriteCounters) ─────────────
  //
  // Rozdzial rol jest ten sam, co po stronie RetractorDB: zapisy do badanego podplanu ida do
  // licznika, publiczne rekordy monitorow do MIANOWNIKA. Pomylka tutaj przenosi bajty
  // z licznika do mianownika i zmienia to, co metryka mierzy (RAPORT_PILOTA.md §6a).
  //
  // Rownoleglosc jobow K26 jest 1, wiec statyczne liczniki w jednej JVM sa poprawne.
  // Odczyt nastepuje w P6, po zamrozonej liczbie rekordow — nie w tej sesji.

  private static long substrateWrites = 0;
  private static long substrateBytes = 0;
  private static long publicAppends = 0;
  private static long evalCalls = 0;
  private static long evalTokens = 0;
  private static long hashPicks = 0;
  private static long addMerges = 0;

  /** Zapis rekordu przez operator NALEZACY do badanego podplanu (licznik metryki). */
  public static synchronized void onSubstrateWrite(long canonicalBytes) {
    substrateWrites++;
    substrateBytes += canonicalBytes;
  }

  /** Rekord wystawiony na per-monitorowy sink (mianownik metryki). */
  public static synchronized void onPublicAppend() {
    publicAppends++;
  }

  public static synchronized String logicalReport() {
    return "LOGICAL substrat: zapisy=" + substrateWrites + " bajty=" + substrateBytes + "  publiczne: rekordy="
        + publicAppends;
  }

  //
  // ─── Liczniki pracy (odpowiednik probe::workCounters) ──────────────────────────────────
  //
  // Powod, dla ktorego te liczniki istnieja OBOK bajtowych: metryka bajtowa mierzy
  // MATERIALIZACJE, nie prace. Autor Flinka, ktory scali caly monitor w jeden operator,
  // nie materializuje ani jednego rekordu posredniego — i licznik bajtow pokazalby zero,
  // mimo Q-krotnie zduplikowanego obliczenia. Liczba WYKONAN programu pol na slot jest
  // odporna na dowolne ciecie obliczenia na operatory, wiec to ona niesie twierdzenie
  // o zduplikowanej pracy. §10 wymienia ja wsrod metryk mechanizmu
  // („wykonan kosztownego programu na slot").
  //
  // Semantyka przeniesiona 1:1 z silnika (`expressionEvaluator::eval`, probe.cc):
  //   * JEDNO wywolanie na wykonanie programu, nie na wezel planu — ten sam program
  //     wykonuje sie raz na slot na KAZDY strumien, ktory go uzywa;
  //   * `tokens` = dlugosc programu (`program.size()`), odczytana z zrzutu planu pilota,
  //     nie oszacowana (patrz K26Ops.TOKENS_*).
  // Licznikow okna agregatu (agse*) nie ma, bo zadna z trzech rodzin K26 nie ma agregatu.

  /** Wykonanie programu pola: liczba WYKONANYCH tokenow, nie rozmiar programu w planie. */
  public static synchronized void onEval(long tokens) {
    evalCalls++;
    evalTokens += tokens;
  }

  /** Wybor skladowej przeplotu w tym slocie (odpowiednik STREAM_HASH). */
  public static synchronized void onHashPick() {
    hashPicks++;
  }

  /** Scalenie payloadow sumy strumieni (odpowiednik STREAM_ADD). */
  public static synchronized void onAddMerge() {
    addMerges++;
  }

  public static synchronized String workReport() {
    return "WORK eval: wywolania=" + evalCalls + " tokeny=" + evalTokens + "  hash: wybory=" + hashPicks
        + "  add: scalenia=" + addMerges;
  }
}
