Abracadabra DB
-----------------------------------------------------------------------
![CircleCI](https://circleci.com/gh/michalwidera/abracadabradb.svg?style=shield&circle-token=b1aed4cd0fd95f82927fce06972f5bdb4456a5a3)

Sources of AbracadabraDB Time Series Database System prototype.

Contents
-----------------------------------------------------------------------

xqry: database client

xdumper: query plan dumper

xcompiler: query compiler

xabracadabra: database main process

xdisplay: query plan presentation script

Installation procedure
-----------------------------------------------------------------------

```
sudo apt-get -y install gcc cmake libboost-all-dev make build-essential
git clone https://github.com/michalwidera/abracadabradb.git
cd abracadabradb
cmake CMakeLists.txt; make
sudo make install
```
After installation xabracadabra, xqry ... etc will be installed in /usr/local/bin on path.
Please check proper installation by typing in command prompt - for instance: xqry -h

You should see:
```
xqry - xabracadabra communication tool.
Allowed options:
  -s [ --select ] arg         show this stream
  -t [ --detail ] arg         show details of this stream
  ...
```
  

To get full functionality additional packages may be required like:
```
sudo apt install graphviz, feh, tmux
```

Author
-----------------------------------------------------------------------

Project created in 2003-2020 by Michal Widera
(michal@widera.com.pl)
