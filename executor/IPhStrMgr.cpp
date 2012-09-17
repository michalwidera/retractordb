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
#include "stdafx.h"

#include "IPhStrMgr.h"

#include "dbstream.h"
#include "Processor.h"

extern QS::Processor proc ;

class CPhStrMgr {
public:
    CPhStrMgr() {} ;

};


IPhStrMgr::IPhStrMgr() :
    pImpl( new CPhStrMgr() )
{
}




