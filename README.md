RetractorDB
-----------------------------------------------------------------------

CI Build Status: [![CircleCI](https://circleci.com/gh/michalwidera/retractordb.svg?style=svg)](https://circleci.com/gh/michalwidera/retractordb)

[comment]: # (VSCode view: Ctrl+k,v)

These are sources of RetractorDB Time Series Database System prototype.

__This is work in progress.__

Project Web Page
-----------------------------------------------------------------------
[https://retractordb.com](https://retractordb.com)

Commands
-----------------------------------------------------------------------

* _xqry_: database client
* _xretractor_: database main process
* _xdisplay_: query plan presentation script

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

Work with antl4 and .g4 files requires java - install it with:
```
sudo apt install default-jre
```

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
select core1[0]/2+1,core0
stream str2
from core1+core0
```

Author
-----------------------------------------------------------------------

Project created in 2003-2025 by Michal Widera
(michal@widera.com.pl)


Additional materials
------------------------------------------------------------------------
* [Slides in PL](https://retractordb.com/assets/presentation/Prezentacja_RetractorDB_PL.pdf)
* [Interlace example online](https://retractordb.com/assets/interlace.html)
* [Sum example online](https://retractordb.com/assets/sum.html)
