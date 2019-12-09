#/bin/sh
#skrypt do testu regresyjnego pod liuxem
# -f [ --fields ]                  pokaz pola w dot
# -t [ --tags ]                    pokaz tagi
# -s [ --streamprogs ]             pokaz programy strumieniowe w dot
# -w [ --showtypes ]               pokaz typy pol w pliku dot

#jeśli pokazujemy -t tagi to muszą być pola

if ! xcompiler -q ./test/query-thesis.txt ; then exit 1 ; fi 

if ! xdumper -d  -i query.qry.lg1       > out1_1.dot ; then exit 1 ; fi 
if ! xdumper -d  -i query.qry.lg2       > out1_2.dot ; then exit 1 ; fi 
if ! xdumper -d                         > out1_3.dot   ; then exit 1 ; fi 

if ! xdumper -d -s -f -i query.qry.lg1 > out2_1.dot ; then exit 1 ; fi 
if ! xdumper -d -s -f -i query.qry.lg2 > out2_2.dot ; then exit 1 ; fi 
if ! xdumper -d -s -f -i query.qry     > out2_3.dot ; then exit 1 ; fi 

if ! xdumper -d -f -t -i query.qry.lg1 > out3_1.dot ; then exit 1 ; fi 
if ! xdumper -d -f -t -i query.qry.lg2 > out3_2.dot ; then exit 1 ; fi 
if ! xdumper -d -f -t -i query.qry     > out3_3.dot ; then exit 1 ; fi 

if ! xdumper -d -s -f -t -i query.qry.lg1 > out4_1.dot ; then exit 1 ; fi 
if ! xdumper -d -s -f -t -i query.qry.lg2 > out4_2.dot ; then exit 1 ; fi 
if ! xdumper -d -s -f -t -i query.qry     > out4_3.dot ; then exit 1 ; fi 

echo "1"; dot -Teps out1_1.dot -o eoutput1_1.eps
echo "1"; dot -Teps out1_2.dot -o eoutput1_2.eps
echo "1"; dot -Teps out1_3.dot -o eoutput1_3.eps

echo "2"; dot -Teps out2_1.dot -o eoutput2_1.eps
echo "2"; dot -Teps out2_2.dot -o eoutput2_2.eps
echo "2"; dot -Teps out2_3.dot -o eoutput2_3.eps

echo "3"; dot -Teps out3_1.dot -o eoutput3_1.eps
echo "3"; dot -Teps out3_2.dot -o eoutput3_2.eps
echo "3"; dot -Teps out3_3.dot -o eoutput3_3.eps

echo "4"; dot -Teps out4_1.dot -o eoutput4_1.eps
echo "4"; dot -Teps out4_2.dot -o eoutput4_2.eps
echo "4"; dot -Teps out4_3.dot -o eoutput4_3.eps

echo "1"; dot -Tpng out1_1.dot -o output1_1.png
echo "1"; dot -Tpng out1_2.dot -o output1_2.png
echo "1"; dot -Tpng out1_3.dot -o output1_3.png

echo "2"; dot -Tpng out2_1.dot -o output2_1.png
echo "2"; dot -Tpng out2_2.dot -o output2_2.png
echo "2"; dot -Tpng out2_3.dot -o output2_3.png

echo "3"; dot -Tpng out3_1.dot -o output3_1.png
echo "3"; dot -Tpng out3_2.dot -o output3_2.png
echo "3"; dot -Tpng out3_3.dot -o output3_3.png

echo "4"; dot -Tpng out4_1.dot -o output4_1.png
echo "4"; dot -Tpng out4_2.dot -o output4_2.png
echo "4"; dot -Tpng out4_3.dot -o output4_3.png

exit 1 

gpicview output1.png
gpicview output2.png
gpicview output3.png
gpicview output4.png
gpicview output5.png
gpicview output6.png