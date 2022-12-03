# RetractorDB:Rdb

## Retractor DB - inner database structure

Retractor DB is using inner database format. This format supports read,update, append commands interface. This database is called _STORAGE_ here.

> :warning: **This is work in progress**: This readme can be outdated.

## Supported data types

Storage supports data of following types:

| type | size(bytes) | range | meaning |
| ---  | ---: | --- | --- |
|BYTE  | 1 | 0..255|8-bit byte|
|INTEGER   | 4 |-2147483648 to 2147483647|C++ Integer|
|Unit  | 4 |0 to 4294967295|Unsigned int|
|STRING|[declared]|Array of Bytes|Array of bytes dedicated for strings|
|BYTEARRAY|[declared]|Array of Bytes|Array of bytes dedicated for binary payloads|
|INTARRAY|[declared]|Array of Integers|Array of integers dedicated for integer array payloads|
|FLOAT| 4 | ... | Floating point value|
|DOUBLE| 8 | ... | Floating point value (double size)|

Table 1. Supported types by :Rdb

## Storage terminal

Terminal is called xtrdb.
After call, you will see . (dot prompt)
type h for help
```
.help
exit|quit|q                      exit
quitdrop|qd                      exit & drop artifacts
open|ropen [file]                open database - create connection (r-reverse iterator)
create|rcreate [file][schema]    create database with schema (r-reverse iterator)
                                 example: .create test_db { INTEGER dane STRING name[3] }
desc                             show schema
read [n]                         read record from database into payload
write [n]                        send record to database from payload
append                           append payload to database
set [field][value]               set payload field value
setpos [position][number value]                  set payload field number value
status                           show status of payload
flip                             flip reverse iterator
rox                              remove on exit flip
print                            show payload
hex|dec                          type of input/output of byte/number fields
size                             show database size in records
dump                             show payload memory
ok
.
```

## Create file with data

```
$ xtrdb
.create testfile { STRING name[5] BYTEARRAY data[3] BYTE control INTEGER intdata FLOAT measurment }
ok
.desc
{       STRING name[5]
        BYTEARRAY data[3]
        BYTE control
        INTEGER intdata
        FLOAT measurment
}
```

That will create two files: testfile and testfile.desc.
File testfile will contain only binary data - testfile.desc will contain data descriptor (in text format)

### Add data to storage

```
.open testfile
ok
.desc
{       STRING name[5]
        BYTEARRAY data[3]
        BYTE control
        INTEGER intdata
        FLOAT measurment
}
.set name test1
.set data 5 7 8
.set control 233
.set intdata 345678
.set measurment 3.1415926
.print
{       name:test1
        data:5 7 8
        control:233
        intdata:345678
        measurment:3.14159
}
.append
ok
```

## Modify data in storage
```
.open testfile
ok
.read 0
ok
.print
{       name:test1
        data:5 7 8
        control:233
        intdata:345678
        measurment:3.14159
}
.set name test2
.print
{       name:test2
        data:5 7 8
        control:233
        intdata:345678
        measurment:3.14159
}
.append
ok
.read 0
ok
.print
{       name:test1
        data:5 7 8
        control:233
        intdata:345678
        measurment:3.14159
}
.
```

## UML Sequence Diagram of :Rdb communication process

![Class Diagram](http://www.plantuml.com/plantuml/proxy?src=https://raw.githubusercontent.com/michalwidera/retractordb/issue_17/src/rdb/UML/rdb-comunication.puml)
