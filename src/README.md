# RetractorDB:system

[comment]: # (VSCode view: Ctrl+k,v)

## RetractorDB - system

* [retractor](retractor/README.md)
* [xtrdb](rdb/README.md)
* [xqry](qry/README.md)

## UML System Perspective - Use Case Diagram

![Use Case Diagram](http://www.plantuml.com/plantuml/proxy?cache=no&src=https://raw.githubusercontent.com/michalwidera/retractordb/master/src/UML/system-use-case.puml)

```plantuml

@startuml

skin rose

title RetractorDB - Use Case Diagram


(Create Query File) as QF<<Create>>
(Compile Query) as CF<<Compile>>
(Execute Query) as CX<<Execute>>

(Query Plan) as QP<<Show>>
(Current Data From Query) as CD<<Read>>
(Read Dump) as RD<<Dump>>
(System Status) as SS<<Status>>

(Show Query Diagram) as QD
(Display Current Signal) as SI


actor :Operator: as O<<Operator>>
actor :User: as U<<User>> 

O  -down-> U

O - QF
O - CF
CF .down.> CX : <<include>>
QF .> CF : <<include>>

QP -up- U
CD -up- U
RD -up- U
SS -up- U

QD .up.> QP : <<extend>>
SI .up.> CD : <<extend>>

@enduml

```