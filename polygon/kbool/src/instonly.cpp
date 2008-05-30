/*! \file ../src/instonly.cpp
    \author Probably Klaas Holwerda

    Copyright: 2001-2004 (C) Probably Klaas Holwerda

    Licence: wxWidgets Licence

    RCS-ID: $Id: instonly.cpp,v 1.5 2005/05/24 19:13:38 titato Exp $
*/

#ifdef __GNUG__
#pragma option -Jgd

#include "../include/_dl_itr.h"
#include "../include/node.h"
#include "../include/record.h"
#include "../include/link.h"
#include "../include/_lnk_itr.h"
#include "../include/scanbeam.h"
#include "../include/graph.h"
#include "../include/graphlst.h"
//#include "../include/misc.h"

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
