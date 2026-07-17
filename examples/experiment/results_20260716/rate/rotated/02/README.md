# Kampania: rate

Wygenerowano automatycznie przez `start_supervisor.sh` przed rozpoczeciem
pierwszego badania tej kampanii. Nie edytowac recznie -- kolejne uruchomienia
kampanii nadpisuja ten plik ta sama trescia.

- branch eksperymentu: `experiment/20260716`
- data przygotowania kampanii: 2026-07-16T14:57:39+02:00
- plik konfiguracji badan: `examples/experiment/config/campaign_rate.csv`

## Cel

Ustalenie maksymalnej czestosci naplywu danych (Hz), przy ktorej algorytm detekcji QRS (Pan-Tompkins, examples/ecg/rec205/rec205-detect.rql) na tym sprzecie workera dotrzymuje budzetu czasu rzeczywistego -- REQUIREMENTS.md pkt 12-15. Celem NIE jest poprawnosc detekcji, tylko przepustowosc (pkt 13).

## Struktura

Kazdy podkatalog `study_NN/` odpowiada jednemu wierszowi pliku konfiguracji
(jednemu badaniu) i zawiera `results.md`, `state_before.md`,
`state_after.md`, `e1_probe.csv`, `metrics.csv` -- patrz glowny
`examples/experiment/README.md` po opis kazdego z tych plikow.
