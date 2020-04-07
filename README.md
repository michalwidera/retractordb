RetractorDB
-----------------------------------------------------------------------
![CircleCI](https://circleci.com/gh/michalwidera/retractordb.svg?style=shield&circle-token=b1aed4cd0fd95f82927fce06972f5bdb4456a5a3)

Sources of RetractorDB Time Series Database System prototype.

Contents
-----------------------------------------------------------------------

xqry: database client

xdumper: query plan dumper

xcompiler: query compiler

xretractor: database main process

xdisplay: query plan presentation script

Installation procedure
-----------------------------------------------------------------------

```
sudo apt-get -y install gcc cmake libboost-all-dev make build-essential iwyu
git clone https://github.com/michalwidera/retractordb.git
cd retractordb
cmake CMakeLists.txt; make
sudo make install
```
After installation xretractor, xqry ... etc will be installed in /usr/local/bin on path.
Please check proper installation by typing in command prompt - for instance: xqry -h

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

Author
-----------------------------------------------------------------------

Project created in 2003-2020 by Michal Widera
(michal@widera.com.pl)
