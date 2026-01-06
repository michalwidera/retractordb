RetractorDB
-----------------------------------------------------------------------

CI Build Status: [![CircleCI](https://circleci.com/gh/michalwidera/retractordb.svg?style=svg)](https://circleci.com/gh/michalwidera/retractordb)

[comment]: # (VSCode view: Ctrl+k,v)

These are sources of RetractorDB Time Series Database System prototype.

__This is work in progress.__

Project Web Page
-----------------------------------------------------------------------
[Retractordb.com](https://retractordb.com) is the official web page of RetractorDB system.

Commands
-----------------------------------------------------------------------

* _xqry_: database client
* _xretractor_: database main process
* _xtrdb_: data accessor and testing tool

How to install
-----------------------------------------------------------------------

```
sudo apt-get -y install gcc cmake make build-essential python3 python3-pip python3-venv valgrind clang-format mold
pip install conan
pip install cmakelang
conan profile detect
git clone https://github.com/michalwidera/retractordb.git
cd retractordb
conan install . -s build_type=Debug --build missing
conan build . -s build_type=Debug --build missing
cd build/Debug
make install
```

optional in case of Externaly Managed PYTHON
```
python3 -m venv .venv
source .venv/bin/activate
```
and add this activate into .bashrc or simillar startup script.

optional in build/Debug folder:
```
make test
cmake .
make
make install
make descgrammar
make rqlgrammar
make cformat
```

additional:

```
https://github.com/cheshirekow/cmake_format
```

Work with antl4 and .g4 files requires java - it will install with conan install.

If you want see graphic response, use gnuplot - install it with:
```
sudo apt install gnuplot
```

After installation _xretractor_, _xqry_ ... etc will be installed in ~/.local/bin on path.
Please check proper installation by typing in command prompt - for instance: _xqry -h_

You should see:
```
xqry - xretractor communication tool.
Allowed options:
  -s [ --select ] arg         show this stream
  -t [ --detail ] arg         show details of this stream
  ...
```

To get full functionality additional packages may be required like:
```
sudo apt install graphviz feh tmux gnuplot
```
How to Ninja (instead of make)
-----------------------------------------------------------------------
[Ninja](https://ninja-build.org/manual.html) is build system.
If you want use this instead of makefile please follow this [path](https://docs.conan.io/2/examples/tools/cmake/cmake_toolchain/use_different_toolchain_generator.html):

0. Install ninja

```
sudo apt install ninja-build
```

1. Modify conan2 profile (for instance):

```
more ~/.conan2/profiles/default
[settings]
arch=x86_64
build_type=Release
compiler=gcc
compiler.cppstd=gnu23
compiler.libcxx=libstdc++11
compiler.version=13
os=Linux

[conf]
tools.cmake.cmaketoolchain:generator=Ninja
```

2. If previous build/Debug exist - remove this.
3. Then you can issue:
```
conan install . -s build_type=Debug --build missing
conan build . -s build_type=Debug --build missing
cd build/Debug
ninja install
ninja test
ninja descgrammar
ninja rqlgrammar
ninja cformat
ninja
```

4. Have fun.

Query Language
-----------------------------------------------------------------------
Example queries are located in examples directory as *.rql files.
Two types of queries are supported.
First starts from _declare_ keyword that declares time series.
This could be a data file, binary file or even device file from _/dev_ directory.
Second starts from _select_ keyword and have following formal form:

```
select column_expression [as ColumnName], [column_expression [as ColumnName]]
stream output_stream_name
from stream_junction_expression
```

Example:
```
select str2[0]/2+1,str2[1]
stream str2
from core1+core0
```

Author
-----------------------------------------------------------------------

Project created in 2003-2026 by Michal Widera
(michal@widera.com.pl)


Additional materials
------------------------------------------------------------------------
* [Slides in PL](https://retractordb.com/assets/presentation/Prezentacja_RetractorDB_PL.pdf)
* [Interlace example online](https://retractordb.com/assets/interlace.html)
* [Sum example online](https://retractordb.com/assets/sum.html)
