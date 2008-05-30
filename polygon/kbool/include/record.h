/*! \file ../include/../record.h
    \author Probably Klaas Holwerda

    Copyright: 2001-2004 (C) Probably Klaas Holwerda

    Licence: wxWidgets Licence

    RCS-ID: $Id: record.h,v 1.1 2005/05/24 19:13:37 titato Exp $
*/

#ifndef RECORD_H
#define RECORD_H

#if defined(__GNUG__) && !defined(NO_GCC_PRAGMA)
#pragma interface
#endif

class Node;
#include "../include/booleng.h"

#include "../include/link.h"
#include "../include/line.h"

enum BEAM_TYPE { NORMAL,FLAT};

enum DIRECTION  {GO_LEFT,GO_RIGHT};

//extern void DeleteRecordPool();
class A2DKBOOLDLLEXP Bool_Engine;

class A2DKBOOLDLLEXP Record
{
        protected:
                                        Bool_Engine* _GC;
	public:
//					void deletepool();

					Record(KBoolLink* link,Bool_Engine* GC);

					~Record();

//					void* operator new(size_t size);

//					void operator delete(void* recordptr);

					void SetNewLink(KBoolLink* link);

					void Set_Flags();

					void Calc_Ysp(Node* low);

					KBoolLink* GetLink();

					KBoolLine* GetLine();

					B_INT	Ysp();

					void SetYsp(B_INT ysp);

					DIRECTION Direction();

					bool Calc_Left_Right(Record* record_above_me);

					bool Equal(Record*);

	private:
					KBoolLine   _line;

					B_INT 		_ysp;

               //! going left are right in beam
					DIRECTION	_dir;    

               //! how far in group_a
               int         _a;               

               //! how far in group_b
               int         _b;               

};

#endif 
