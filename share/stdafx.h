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
*/
#pragma once
// stdafx.h : include file for standard system include files,
// or project specific include files that are used frequently, but
// are changed infrequently
//

//#pragma message ("- stdafx compiler")

#ifndef STDAFXH
#define STDAFXH

#ifdef _MSC_VER
#pragma warning(disable: 4267)
#pragma warning(disable: 4800)
#endif

#include <iostream>

//#include <tchar.h>
#include <stdlib.h>	//min

//Additional c++ libraries
#ifdef WIN32
#include <conio.h>
#else
#include <stdio.h>
#endif 

#include <fstream>
#include <string>
#include <map>
#include <vector>
#include <deque>
#include <stdexcept>
#include <cassert>
#include <exception>
#include <algorithm>
#include <cstddef>		//tize_t
#include <sstream>
#include <fstream>
#include <sstream>
#include <climits>      //Required maximum and minimum int
#include <limits.h>

//Boost libs
#include <boost/program_options.hpp>
#include <boost/rational.hpp>
#include <boost/foreach.hpp>
#include <boost/scoped_ptr.hpp>
#include <boost/crc.hpp>
//#include <boost/locale.hpp>
//#include <boost/lexical_cast.hpp>

typedef boost::rational<int> R ;

#ifdef _MSC_VER
#define COMPILER "MS"
#else
#ifdef __GNUC__
#define COMPILER "GNU"
#else
#define COMPILER "Unknown"
#endif
#endif

#ifdef _MSC_VER
#define WIN32_LEAN_AND_MEAN
#define _CRT_SECURE_NO_WARNINGS
#include <windows.h>
#define usleep Sleep
#else
#include <unistd.h>
#endif

//Ten kod jest odpowiedzialny za poprawna kompilacje na vs2003
#if _MSC_VER >= 1400
// this is Visual C++ 2005
#elif _MSC_VER >= 1310
// this is Visual c++ .NET 2003
#define sprintf sprintf_s
#define sscanf	sscanf_s
#ifdef assert
#undef assert
//#define assert(test) (void)0
#define assert(test)  if ( ! (test) ) throw std::out_of_range( #test );  
#endif
#elif _MSC_VER > 1300
#error Visual C++ .NET 2002 Compiler not supported
#endif 

#endif
