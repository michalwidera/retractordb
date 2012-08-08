/*Copyright 2010-2012 Streams Lab Michal Widera

  RESTRICTED RIGHTS LEGEND
  Use, duplication or disclosure by any commercial industry (public or
  private), private individual or by any Government Agency without an
  expressed written consent of release from Streams Lab Michal Widera
  is subject to restriction of the Rights.

  Manufacturer is:
      Streams Lab Michal Widera
      Reymonta str. 56/1
      Poland, Zabrze, 41-800
      (+48) 606 995 918

$Rev:: 2             $:  Revision of last commit
$Author:: developer  $:  Author of last commit
$Date:: 2012-07-15 0#$:  Date of last commit
$Id:: RandomFile.h 2#$:  Id of last commit
*/
#ifndef RANDOMFILEH
#define RANDOMFILEH

#include "stdafx.h"

#ifdef WIN32
#include <windows.h>
#endif
#include <fstream>
#include <string>

using std::fstream ;
using std::string ;
using std::ios ;

/** CRandomFile
*   Klasa tworz¹ca strumien na dysku o losowej nazwie. 
*	Nazwa pliku jest tworzona losowo w 
*	konstruktorze. Utworzone pliki maja charakter tymczasowy i sa
*	automatycznie usuwane w destruktorze.
*   @short Klasa do tworzenia strumienia na dysku.
*
*/
class CRandomFile : public fstream
{
	
	/** Zmienna przechowujaca informacje czy plik powinien zostac zniszczony razem z obiektem */
	bool destroyOnDestructor ;

public:

	string filename;
	
	CRandomFile( string murExt = "mur"  , const bool destroyOnDestructor = true );
	virtual ~CRandomFile();
	const CRandomFile &operator << ( string& text );
};


#endif

