/*! \file include/scanbeam.h
    \author Klaas Holwerda
 
    Copyright: 2001-2004 (C) Klaas Holwerda
 
    Licence: see kboollicense.txt 
 
    RCS-ID: $Id: scanbeam.h,v 1.5 2009/09/07 19:23:28 titato Exp $
*/

#ifndef SCANBEAM_H
#define SCANBEAM_H

#include "kbool/booleng.h"
#include "kbool/_lnk_itr.h"

#include "kbool/record.h"
#include "kbool/link.h"

enum SCANTYPE{NODELINK, LINKLINK, GENLR, LINKHOLES, INOUT};

#if defined(WXART2D_USINGDLL)
template class A2DKBOOLDLLEXP DL_Iter<kbRecord*>;
#endif

class A2DKBOOLDLLEXP ScanBeam : public DL_List<kbRecord*>
{
protected:
    Bool_Engine* _GC;

public:
    ScanBeam( Bool_Engine* GC );
    ~ScanBeam();
    void SetType( kbNode* low, kbNode* high );

    bool  FindNew( SCANTYPE scantype, TDLI<kbLink>* _I, bool& holes );
    bool  RemoveOld( SCANTYPE scantype, TDLI<kbLink>* _I, bool& holes );

private:

    bool  ProcessHoles( bool atinsert, TDLI<kbLink>* _LI );
    int Process_LinkToLink_Crossings();   // find crossings
    int  Process_PointToLink_Crossings();
    int  Process_LinkToLink_Flat( kbLine* flatline );
    void SortTheBeam( bool backangle );
    bool  checksort();
    bool  writebeam();
    void Calc_Ysp();
    //int  FindCloseLinksAndCross(TDLI<kbLink>* _I,kbNode* _lowf);
    void  Generate_INOUT( int graphnumber );

    kbNode*      _low;
    DL_Iter<kbRecord*> _BI;
    int       lastinserted;
    BEAM_TYPE    _type;
};

#endif
