/*! \file kbool/samples/boolonly/boolonly.h
    \author Probably Klaas Holwerda

    Copyright: 2001-2004 (C) Probably Klaas Holwerda

    Licence: wxWidgets Licence

    RCS-ID: $Id: boolonly.h,v 1.5 2005/05/24 19:13:38 titato Exp $
*/

#ifdef __GNUG__
#pragma implementation
#endif

#include "kbool/include/_lnk_itr.h"
#include "kbool/include/booleng.h"

class KBoolPoint
{
	public:

		KBoolPoint();
		KBoolPoint(double const ,double const);
		double	GetX();
		double	GetY();

	private:
		double _x;
		double _y;

};

