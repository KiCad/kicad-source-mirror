/*! \file kbool/src/instonly.cpp
    \author Probably Klaas Holwerda
 
    Copyright: 2001-2004 (C) Probably Klaas Holwerda
 
    Licence: see kboollicense.txt 
 
    RCS-ID: $Id: instonly.cpp,v 1.3 2009/02/06 21:33:03 titato Exp $
*/

#ifdef __GNUG__
#pragma option -Jgd

#include "kbool/_dl_itr.h"
#include "kbool/node.h"
#include "kbool/record.h"
#include "kbool/link.h"
#include "kbool/_lnk_itr.h"
#include "kbool/scanbeam.h"
#include "kbool/graph.h"
#include "kbool/graphlst.h"
//#include "kbool/misc.h"

template class DL_Node<void *>;
template class DL_Iter<void *>;
template class DL_List<void *>;

template class DL_Node<int>;
template class DL_Iter<int>;
template class DL_List<int>;

template class TDLI<Node>;
template class TDLI<LPoint>;
template class TDLI<Record>;
template class TDLI<KBoolLink>;
template class TDLI<Graph>;

#endif
