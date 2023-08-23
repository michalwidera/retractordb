# [RetractorDB](../README.md):xretractor

[comment]: # (VSCode view: Ctrl+k,v)

## RetractorDB - query compiler & executor process

Compilation and execution are gathered in one program called xretractor.
The build phase precedes the query execution process.
The tool also provides methods to dump compiled query execution plans in text form.

> :warning: **This is work in progress**: This readme can be outdated.

This process is called from command prompt with:
```
$xretractor -h
xretractor - compiler & data processing tool.

Usage: xretractor queryfile [option]

Available options:
  -h [ --help ]               Show program options
  -q [ --queryfile ] arg      query set file
  -v [ --verbose ]            verbose mode (show stream params)
  -m [ --tlimitqry ] arg (=0) query limit, 0 - no limit
  -c [ --onlycompile ]        compile only mode
Branch: master:b38fb68, Code compiler: GNU Ver. 13.2.0, Build time: 2308192116, Type: Debug
This is free software; see the source for copying conditions.
There is NO warranty; not even for MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
```

Please notice that this tool has second face when you call it with "only compile" option. This face is required for _Show Diagram_ or _Show query Plan_ actions.

```
$xretractor -c -h
xretractor - compiler & data processing tool.

Usage: xretractor queryfile [option]

Available options:
  -h [ --help ]          show help options
  -q [ --queryfile ] arg query set file
  -r [ --quiet ]         no output on screen, skip dumper
  -d [ --dot ]           create dot output
  -m [ --csv ]           create csv output
  -f [ --fields ]        show fields in dot file
  -t [ --tags ]          show tags in dot file
  -s [ --streamprogs ]   show stream programs in dot file
  -c [ --onlycompile ]   compile only mode
Branch: master:b38fb68, Code compiler: GNU Ver. 13.2.0, Build time: 2308192116, Type: Debug
This is free software; see the source for copying conditions.
There is NO warranty; not even for MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
```