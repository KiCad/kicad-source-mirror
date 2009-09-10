/*! \file kbool/samples/boolonly/boolonly.h
    \author Probably Klaas Holwerda
 
    Copyright: 2001-2004 (C) Probably Klaas Holwerda
 
    Licence: see kboollicense.txt 
 
    RCS-ID: $Id: boolonly.h,v 1.5 2009/02/06 21:33:03 titato Exp $
*/

#include "kbool/booleng.h"
#include "kbool/_lnk_itr.h"

class KBoolPoint
{
public:

    KBoolPoint();
    KBoolPoint( double const , double const );
    double GetX();
    double GetY();

private:
    double _x;
    double _y;

};

