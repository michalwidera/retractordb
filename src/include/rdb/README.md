# RetractorDB:Rdb Headers description

Following headers are in include/rdb directory are:

file name|Type name|description|temporary
:--|---|---|--:
descriptor.h|Descriptor|Description - this object creates ability to create descriptions of binary frames using types and arrays|no
faccfs.h|genericBinaryFileAccessor|File Accessor as File system - type. This is underlying type that supports access to binary fields. std::fstream is used as interface. :Rdb user does not need to use this object directly|?
faccposix.h|posixBinaryFileAccessor|File Accessor as Posix - type. This is underlying type that supports access to binary fields. Posix functions are used as interface. :Rdb user does not need to use this object directly|?
faccposixprm.h|posixPrmBinaryFileAccessor|File Accessor as Posix Permanent - type. This is underlying type that supports access to binary fields. Posix functions are used as interface. Difference between faccposix is that file descriptor remains open during entire live of object :Rdb user does not need to use this object directly. This is development area.|no
fainterface.h|FileAccessorInterface|File Accessor Interface. This is used as pattern for: faccposix, facposixprm and faccfs types. All these types need to support this interface|yes
payload.h|payload|This is accessor for payload memory area that supports applying descriptor type over memory area.|yes

Temporary field means that this file most probably will disappear during next development.

## UML Class schema of :Rdb

[https://www.visual-paradigm.com/guide/uml-unified-modeling-language/uml-aggregation-vs-composition/]: <>


![Class Diagram](http://www.plantuml.com/plantuml/proxy?src=https://raw.githubusercontent.com/michalwidera/retractordb/issue_17/src/include/rdb/UML/rdb.puml)
