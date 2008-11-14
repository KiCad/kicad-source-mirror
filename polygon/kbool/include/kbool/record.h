/*! \file include/record.h
    \author Klaas Holwerda
 
    Copyright: 2001-2004 (C) Klaas Holwerda
 
    Licence: see kboollicense.txt 
 
    RCS-ID: $Id: record.h,v 1.3 2008/06/04 21:23:22 titato Exp $
*/

#ifndef RECORD_H
#define RECORD_H

class Node;
#include "kbool/booleng.h"

#include "kbool/link.h"
#include "kbool/line.h"

enum BEAM_TYPE { NORMAL, FLAT};

enum DIRECTION  {GO_LEFT, GO_RIGHT};

//extern void DeleteRecordPool();
class A2DKBOOLDLLEXP Bool_Engine;

class A2DKBOOLDLLEXP Record
{
protected:
    Bool_Engine* _GC;
public:
//     void deletepool();

    Record( KBoolLink* link, Bool_Engine* GC );

    ~Record();

//     void* operator new(size_t size);

//     void operator delete(void* recordptr);

    void SetNewLink( KBoolLink* link );

    void Set_Flags();

    void Calc_Ysp( Node* low );

    KBoolLink* GetLink();

    KBoolLine* GetLine();

    B_INT Ysp();

    void SetYsp( B_INT ysp );

    DIRECTION Direction();

    bool Calc_Left_Right( Record* record_above_me );

    bool Equal( Record* );

private:
    KBoolLine   _line;

    B_INT   _ysp;

    //! going left are right in beam
    DIRECTION _dir;

    //! how far in group_a
    int         _a;

    //! how far in group_b
    int         _b;

};

#endif
