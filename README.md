RetractorDB
-----------------------------------------------------------------------

This branch: [![CircleCI](https://circleci.com/gh/michalwidera/retractordb.svg?style=svg)](https://circleci.com/gh/michalwidera/retractordb)

[comment]: # (VSCode view: Ctrl+k,v)

These are sources of RetractorDB Time Series Database System prototype. This is work in progress.

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
sudo apt-get -y install gcc cmake make build-essential python3 python3-pip valgrind clang-format
pip install conan
conan profile detect
git clone https://github.com/michalwidera/retractordb.git
cd retractordb
conan install . -s build_type=Debug --build missing
conan build . -s build_type=Debug --build missing
cd build/Debug
make install
```

optional in build/Debug folder:
```
make test
cmake .
make
make install
make grammar
make cformat
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
select column_names
stream output_stream
from junction_of_input_streams
```

Example:
```
select core1[0]/2+1,a
stream str2
from core1+core0
```

Author
-----------------------------------------------------------------------

Project created in 2003-2023 by Michal Widera
(michal@widera.com.pl)
