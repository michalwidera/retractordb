# xtrdb

[RetractorDB](../../README.md) | [Manual (PL)](https://dokumentacja.retractordb.com/zalaczniki/opcje-wywolania/xtrdb.html) | [Manual (EN)](https://documentation.retractordb.com/appendices/command-line-options/xtrdb.html)

`xtrdb` inspects and edits RetractorDB storage offline. Its interactive terminal can display descriptors, read records and null/gap metadata, append or update records, and create binary test fixtures. The storage family can include data, `.desc`, `.meta`, shadow files and retention segments.

## Basic use

```bash
xtrdb --help
xtrdb --storagemap path/to/data
xtrdb
```

`--storagemap` prints a storage-family report and exits. In the interactive terminal, inspect an existing artifact by its data-file path:

```text
open path/to/data
desc
size
list 5
quit
```

The displayed `.` is a prompt, not part of the command. Use `help` for the command list. Supply a schema to `open` only when creating storage that has no descriptor; existing storage loads its `.desc`.

## Offline access and scripting

Stop engines that own the target files before opening them. Interactive and batch startup check held locks in the configured `paths.lock_dir` and live instance slots on the current namespace's bus. This also detects a live instance whose lock file disappeared, but it is not a proof that no engine in another namespace or lock directory owns the files. `--help` and `--storagemap` return before these startup guards.

`xtrdb --noprompt` accepts commands from standard input without prompts. A refused command can still be followed by a normal process exit: inspect diagnostics and `status`, rather than treating exit code 0 as proof that every operation succeeded. `set` changes the payload buffer; `append` or `write` persists it. `quit` does not save unwritten payload edits.

For live results use [`xqry`](../qry/README.md). For programmatic offline reads and schema inspection use Python [`Storage` and `Descriptor`](../../api/python/README.md).

## Documentation

| Topic | PL | EN |
| --- | --- | --- |
| CLI, terminal commands and storage access | [xtrdb](https://dokumentacja.retractordb.com/zalaczniki/opcje-wywolania/xtrdb.html) | [xtrdb](https://documentation.retractordb.com/appendices/command-line-options/xtrdb.html) |
| Storage-map output and examples | [Inspekcja magazynu](https://dokumentacja.retractordb.com/architektura-systemu-przetwarzania-danych/format-zapisu-danych/narzedzie-inspekcji.html) | [Storage inspection](https://documentation.retractordb.com/data-processing-system-architecture/data-storage-format/inspection-tool.html) |
| Files, descriptors and metadata | [Pliki](https://dokumentacja.retractordb.com/architektura-systemu-przetwarzania-danych/format-zapisu-danych/pliki.html) | [Files](https://documentation.retractordb.com/data-processing-system-architecture/data-storage-format/files.html) |
| Rotation and retention | [Rotacja](https://dokumentacja.retractordb.com/architektura-systemu-przetwarzania-danych/format-zapisu-danych/rotacja.html) | [Rotation](https://documentation.retractordb.com/data-processing-system-architecture/data-storage-format/rotation.html) |

The underlying C++ storage library is in [lib/](lib/); repository build and test rules are in [CLAUDE.md](../../CLAUDE.md).
