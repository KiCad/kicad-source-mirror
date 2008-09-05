/*! \file ../include/../scanbeam.h
    \author Probably Klaas Holwerda

    Copyright: 2001-2004 (C) Probably Klaas Holwerda

    Licence: wxWidgets Licence

    RCS-ID: $Id: scanbeam.h,v 1.2 2005/06/11 19:25:12 frm Exp $
*/

#ifndef SCANBEAM_H
#define SCANBEAM_H

#if defined(__GNUG__) && !defined(NO_GCC_PRAGMA)
#pragma interface
#endif

#include "../include/booleng.h"
#include "../include/_lnk_itr.h"

#include "../include/record.h"
#include "../include/link.h"

enum SCANTYPE{NODELINK,LINKLINK,GENLR,LINKHOLES,INOUT};

#if 0   // Kicad does dot use wxWidgets lib when building the kbool library
        // but uses wxWidgets. So WXUSINGDLL could be defined in makefiles
        // but must not be used when building kbool
#if defined(WXUSINGDLL)
template class A2DKBOOLDLLEXP DL_Iter<Record*>;
#endif
#endif

class A2DKBOOLDLLEXP ScanBeam : public DL_List<Record*>
{
   protected:
      Bool_Engine* _GC;

	public:
			ScanBeam(Bool_Engine* GC);
			~ScanBeam();
         void SetType(Node* low,Node* high);

			bool  FindNew(SCANTYPE scantype,TDLI<KBoolLink>* _I, bool& holes );
			bool  RemoveOld(SCANTYPE scantype,TDLI<KBoolLink>* _I, bool& holes );

   private:

			bool  ProcessHoles(bool atinsert,TDLI<KBoolLink>* _LI);
         int	Process_LinkToLink_Crossings();			// find crossings
			int 	Process_PointToLink_Crossings();
			int 	Process_LinkToLink_Flat(KBoolLine* flatline);
         void	SortTheBeam( bool backangle );
			bool 	checksort();
			bool 	writebeam();
			void	Calc_Ysp();
			//int 	FindCloseLinksAndCross(TDLI<KBoolLink>* _I,Node* _lowf);
         void 	Generate_INOUT(int graphnumber);

         Node*				  _low;
         DL_Iter<Record*> _BI;
         int 				  lastinserted;
         BEAM_TYPE		  _type;
};

#endif 
