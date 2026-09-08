// Boost.JSON jest biblioteka kompilowana osobno; naglowki daje Conan, a
// implementacje trzeba wciagnac raz, w jednej jednostce translacji. Silnik ma
// wlasna kopie tego TU, zeby xqry nie linkowal niczego z api/ — API zalezy od
// silnika, nigdy odwrotnie.
#include <boost/json/src.hpp>
