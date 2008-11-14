/*! \file include/graphlst.h
    \author Klaas Holwerda
 
    Copyright: 2001-2004 (C) Klaas Holwerda
 
    Licence: see kboollicense.txt 
 
    RCS-ID: $Id: graphlst.h,v 1.3 2008/06/04 21:23:21 titato Exp $
*/

/* @@(#) $Source: /cvsroot/wxart2d/wxArt2D/thirdparty/kbool/include/kbool/graphlst.h,v $ $Revision: 1.3 $ $Date: 2008/06/04 21:23:21 $ */

/*
Program GRAPHLST.H
Purpose Implements a list of graphs (header)
Last Update 11-03-1996
*/

#ifndef GRAPHLIST_H
#define GRAPHLIST_H

#include "kbool/booleng.h"

#include "kbool/_lnk_itr.h"

#include "kbool/graph.h"

class Debug_driver;


class A2DKBOOLDLLEXP GraphList: public DL_List<void*>
{
protected:
    Bool_Engine* _GC;
public:

    GraphList( Bool_Engine* GC );

    GraphList( GraphList* other );

    ~GraphList();

    void    MakeOneGraph( Graph *total );

    void    Prepare( Graph *total );
    void     MakeRings();
    void     Correction();

    void    Simplify( double marge );
    void     Smoothen( double marge );
    void     Merge();
    void     Boolean( BOOL_OP operation, int intersectionRunsMax );

    void           WriteGraphs();
    void           WriteGraphsKEY( Bool_Engine* GC );

protected:
    void    Renumber();
    void    UnMarkAll();

};


#endif

