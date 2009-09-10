/*! \file include/record.h
    \author Klaas Holwerda
 
    Copyright: 2001-2004 (C) Klaas Holwerda
 
    Licence: see kboollicense.txt 
 
    RCS-ID: $Id: record.h,v 1.4 2009/09/07 19:23:28 titato Exp $
*/

#ifndef RECORD_H
#define RECORD_H

class kbNode;
#include "kbool/booleng.h"

#include "kbool/link.h"
#include "kbool/line.h"

enum BEAM_TYPE { NORMAL, FLAT};

enum DIRECTION  {GO_LEFT, GO_RIGHT};

//extern void DeleteRecordPool();
class A2DKBOOLDLLEXP Bool_Engine;

class A2DKBOOLDLLEXP kbRecord
{
protected:
    Bool_Engine* _GC;
public:
//     void deletepool();

    kbRecord( kbLink* link, Bool_Engine* GC );

    ~kbRecord();

//     void* operator new(size_t size);

//     void operator delete(void* recordptr);

    void SetNewLink( kbLink* link );

    void Set_Flags();

    void Calc_Ysp( kbNode* low );

    kbLink* GetLink();

    kbLine* GetLine();

    B_INT Ysp();

    void SetYsp( B_INT ysp );

    DIRECTION Direction();

    bool Calc_Left_Right( kbRecord* record_above_me );

    bool Equal( kbRecord* );

private:
    kbLine   _line;

    B_INT   _ysp;

    //! going left are right in beam
    DIRECTION _dir;

    //! how far in group_a
    int         _a;

    //! how far in group_b
    int         _b;

};

#endif
