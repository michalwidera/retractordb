# Kampania: clients

Wygenerowano automatycznie przez `start_supervisor.sh` przed rozpoczeciem
pierwszego badania tej kampanii. Nie edytowac recznie -- kolejne uruchomienia
kampanii nadpisuja ten plik ta sama trescia.

- branch eksperymentu: `experiment/20260718_40ms`
- data przygotowania kampanii: 2026-07-18T17:16:13+02:00
- plik konfiguracji badan: `examples/experiment/config/campaign_clients.csv`

## Cel

Ustalenie wplywu liczby dolaczonych klientow xqry (1..10) na proces xretractor i system, przy ustalonej czestosci naplywu -- REQUIREMENTS.md pkt 16.

## Struktura

Kazdy podkatalog `study_NN/` odpowiada jednemu wierszowi pliku konfiguracji
(jednemu badaniu) i zawiera `results.md`, `state_before.md`,
`state_after.md`, `e1_probe.csv`, `metrics.csv` -- patrz glowny
`examples/experiment/README.md` po opis kazdego z tych plikow.
