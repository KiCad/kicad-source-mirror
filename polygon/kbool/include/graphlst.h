/*! \file ../include/../graphlst.h
    \author Probably Klaas Holwerda

    Copyright: 2001-2004 (C) Probably Klaas Holwerda

    Licence: wxWidgets Licence

    RCS-ID: $Id: graphlst.h,v 1.1 2005/05/24 19:13:37 titato Exp $
*/

/* @@(#) $Source: /cvsroot/wxart2d/wxArt2D/modules/../include/graphlst.h,v $ $Revision: 1.1 $ $Date: 2005/05/24 19:13:37 $ */

/*
Program	GRAPHLST.H
Purpose	Implements a list of graphs (header)
Last Update	11-03-1996
*/

#ifndef GRAPHLIST_H
#define GRAPHLIST_H

#if defined(__GNUG__) && !defined(NO_GCC_PRAGMA)
#pragma interface
#endif

#include "../include/booleng.h"

#include "../include/_lnk_itr.h"

#include "../include/graph.h"

class Debug_driver;


class A2DKBOOLDLLEXP GraphList: public DL_List<void*>
{
   protected:
      Bool_Engine* _GC;
	public:

		GraphList(Bool_Engine* GC);

      GraphList( GraphList* other ); 

		~GraphList();

		void				MakeOneGraph(Graph *total);

		void				Prepare(Graph *total);
		void 				MakeRings();
		void 				Correction();

		void				Simplify( double marge);
		void 				Smoothen( double marge);
		void 				Merge();
		void 				Boolean(BOOL_OP operation, int intersectionRunsMax );

      void           WriteGraphs();
      void           WriteGraphsKEY( Bool_Engine* GC );
   
	protected:
		void				Renumber();
		void				UnMarkAll();

};


#endif

