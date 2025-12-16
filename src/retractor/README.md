# [RetractorDB](../README.md):xretractor

[comment]: # (VSCode view: Ctrl+k,v)

## RetractorDB - query compiler & executor process

Compilation and execution are gathered in one program called xretractor.
The build phase precedes the query execution process.
The tool also provides methods to dump compiled query execution plans in text form.

> :warning: **This is work in progress**: This readme can be outdated.

This process is called from command prompt with:
```
$xretractor - compiler & data processing tool.

Usage: xretractor queryfile [option]

Available options:
  -h [ --help ]               Show program options
  -s [ --status ]             check service status
  -q [ --queryfile ] arg      query set file
  -v [ --verbose ]            verbose mode (show stream params)
  -x [ --xqrywait ]           wait with processing for first query
  -k [ --noanykey ]           do not wait for any key to terminate
  -m [ --tlimitqry ] arg (=0) query limit, 0 - no limit
  -c [ --onlycompile ]        compile only mode
Branch: issue_31-doc:eb1aba1, Code compiler: GNU Ver. 13.3.0, Build time: 2512170028, Type: Debug
Log: /tmp/xretractor.log
This software is licensed under the MIT License and is provided ‘as is’,
without warranty of any kind. For more information, see the LICENSE file.
```

Please notice that this tool has second face when you call it with "only compile" option. This face is required for _Show Diagram_ or _Show query Plan_ actions.

```
$xretractor -c -h
xretractor - compiler & data processing tool.

Usage: xretractor queryfile [option]

Available options:
  -h [ --help ]          show help options
  -q [ --queryfile ] arg query set file
  -r [ --quiet ]         no output on screen, skip presenter
  -d [ --dot ]           create dot output
  -m [ --csv ]           create csv output
  -f [ --fields ]        show fields in dot file
  -t [ --tags ]          show tags in dot file
  -s [ --streamprogs ]   show stream programs in dot file
  -u [ --rules ]         show rules in dot file
  -p [ --transparent ]   make dot background transparent
  -w [ --diagram ] arg   create diagram output
  -c [ --onlycompile ]   compile only mode
Branch: issue_31-doc:eb1aba1, Code compiler: GNU Ver. 13.3.0, Build time: 2512170028, Type: Debug
Log: /tmp/xretractor.log
This software is licensed under the MIT License and is provided ‘as is’,
without warranty of any kind. For more information, see the LICENSE file.
```

## Storage state machine

![Use Case Diagram](http://www.plantuml.com/plantuml/proxy?cache=no&src=https://raw.githubusercontent.com/michalwidera/retractordb/master/src/retractor/UML/storage-access-state.puml)