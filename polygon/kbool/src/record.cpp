/*! \file src/record.cpp
    \author Klaas Holwerda or Julian Smart
 
    Copyright: 2001-2004 (C) Klaas Holwerda
 
    Licence: see kboollicense.txt 
 
    RCS-ID: $Id: record.cpp,v 1.4 2009/09/07 19:23:28 titato Exp $
*/

#include "kbool/booleng.h"
#include "kbool/record.h"
#include "kbool/node.h"

#include <stdlib.h>
#include <math.h>

#define LNK _line.GetLink()

//int r_index=-1;
//void* _Record_Pool[30];

//void DeleteRecordPool()
//{
//   while (r_index!=-1)
//   {
//     free( _Record_Pool[r_index--]);
//   }
//}

kbRecord::~kbRecord()
{}


//void* kbRecord::operator new(size_t size)
//{
//
//   if (r_index!=-1)
//   {
//     return _Record_Pool[r_index--];
//   }
//
//   return malloc(size);
//}

//void kbRecord::operator delete(void* recordptr)
//{
//
//   if (r_index < 28)
//   {
//     _Record_Pool[++r_index]= recordptr;
//     return;
//   }
//
// free (recordptr);
//}

//void kbRecord::deletepool()
//{
//
//   while (r_index!=-1)
//   {
//     free( _Record_Pool[r_index--]);
//   }
//}

kbRecord::kbRecord( kbLink* link, Bool_Engine* GC )
        : _line( GC )
{
    _GC = GC;
    _dir = GO_RIGHT;
    _a = 0;
    _b = 0;
    _line.Set( link );
    _line.CalculateLineParameters();
}


//when the dimensions of a link for a record changes, its line parameters need to be recalculated
void kbRecord::SetNewLink( kbLink* link )
{
    _line.Set( link );
    _line.CalculateLineParameters();
}

//for beams calculate the ysp on the low scanline
void kbRecord::Calc_Ysp( kbNode* low )
{
    if ( ( LNK->GetEndNode() == low ) || ( LNK->GetBeginNode() == low ) )
    {
        _ysp = low->GetY();
        return;
    }

    if ( LNK->GetEndNode()->GetX() == LNK->GetBeginNode()->GetX() )
        _ysp = low->GetY(); //flatlink only in flatbeams
    else if ( LNK->GetEndNode()->GetX() == low->GetX() )
        _ysp = LNK->GetEndNode()->GetY();
    else if ( LNK->GetBeginNode()->GetX() == low->GetX() )
        _ysp = LNK->GetBeginNode()->GetY();
    else
        _ysp = _line.Calculate_Y_from_X( low->GetX() );
}

//to set the _dir for new links in the beam
void kbRecord::Set_Flags()
{
    if ( LNK->GetEndNode()->GetX() == LNK->GetBeginNode()->GetX() ) //flatlink ?
    {  //only happens in flat beams
        if ( LNK->GetEndNode()->GetY() < LNK->GetBeginNode()->GetY() )
            _dir = GO_RIGHT;
        else
            _dir = GO_LEFT;
    }
    else
    {
        if ( LNK->GetEndNode()->GetX() > LNK->GetBeginNode()->GetX() )
            _dir = GO_RIGHT;
        else
            _dir = GO_LEFT;
    }
}

kbLink* kbRecord::GetLink()
{
    return LNK;
}

B_INT kbRecord::Ysp()
{
    return _ysp;
}

void kbRecord::SetYsp( B_INT ysp )
{
    _ysp = ysp;
}

DIRECTION kbRecord::Direction()
{
    return DIRECTION( _dir );
}

bool kbRecord::Calc_Left_Right( kbRecord* record_above_me )
{
    bool par = false;

    if ( !record_above_me )   //null if no record above
    {  _a = 0;_b = 0;  }
    else
    {
        _a = record_above_me->_a;
        _b = record_above_me->_b;
    }

    switch ( _dir & 1 )
    {
        case GO_LEFT : if ( LNK->Group() == GROUP_A )
            {
                LNK->SetRightA( ( bool )( _a > 0 ) );

                if ( _GC->GetWindingRule() )
                    LNK->GetInc() ? _a++ : _a--;
                else
                {  //ALTERNATE
                    if ( _a )
                        _a = 0;
                    else
                        _a = 1;
                }

                LNK->SetLeftA( ( bool )( _a > 0 ) );
                LNK->SetLeftB( ( bool )( _b > 0 ) );
                LNK->SetRightB( ( bool )( _b > 0 ) );
            }
            else
            {
                LNK->SetRightA( ( bool )( _a > 0 ) );
                LNK->SetLeftA( ( bool )( _a > 0 ) );
                LNK->SetRightB( ( bool )( _b > 0 ) );

                if ( _GC->GetWindingRule() )
                    LNK->GetInc() ? _b++ : _b--;
                else //ALTERNATE
                {
                    if ( _b )
                        _b = 0;
                    else
                        _b = 1;
                }

                LNK->SetLeftB( ( bool )( _b > 0 ) );
            }
            break;
        case GO_RIGHT : if ( LNK->Group() == GROUP_A )
            {
                LNK->SetLeftA( ( bool )( _a > 0 ) );

                if ( _GC->GetWindingRule() )
                    LNK->GetInc() ? _a++ : _a--;
                else
                {  //ALTERNATE
                    if ( _a )
                        _a = 0;
                    else
                        _a = 1;
                }

                LNK->SetRightA( ( bool )( _a > 0 ) );
                LNK->SetLeftB( ( bool )( _b > 0 ) );
                LNK->SetRightB( ( bool )( _b > 0 ) );
            }
            else
            {
                LNK->SetRightA( ( bool )( _a > 0 ) );
                LNK->SetLeftA( ( bool )( _a > 0 ) );
                LNK->SetLeftB( ( bool )( _b > 0 ) );

                if ( _GC->GetWindingRule() )
                    LNK->GetInc() ? _b++ : _b--;
                else
                {  //ALTERNATE
                    if ( _b )
                        _b = 0;
                    else
                        _b = 1;
                }

                LNK->SetRightB( ( bool )( _b > 0 ) );
            }
            break;
        default   :  _GC->error( "Undefined Direction of link", "function IScanBeam::Calc_Set_Left_Right()" );
            break;
    }

//THE NEXT WILL WORK for MOST windingrule polygons,
//even when not taking into acount windingrule
// not all
    /*
       switch (_dir&1)
       {
          case GO_LEFT : if (LNK->Group() == GROUP_A)
                            {
                               LNK->SetRightA((bool)(_a>0));
     
                               if (booleng->Get_WindingRule())
                                  LNK->GetInc() ? _a++ : _a--;
                               else
                                  _a--;
     
                               LNK->SetLeftA((bool)(_a>0));
                               LNK->SetLeftB((bool)(_b>0));
                               LNK->SetRightB((bool)(_b>0));
                            }
                            else
                            {
                               LNK->SetRightA((bool)(_a > 0));
                               LNK->SetLeftA((bool)(_a>0));
                               LNK->SetRightB((bool)(_b>0));
     
                               if (booleng->Get_WindingRule())
                                  LNK->GetInc() ? _b++ : _b--;
                               else
                                  _b--;
     
                               LNK->SetLeftB((bool)(_b>0));
                            }
                               break;
          case GO_RIGHT : if (LNK->Group() == GROUP_A)
                               {
                                  LNK->SetLeftA((bool)(_a>0));
     
                                  if (booleng->Get_WindingRule())
                                     LNK->GetInc() ? _a++ : _a--;
                                  else
                                     _a++;
     
                                  LNK->SetRightA((bool)(_a>0));
                                  LNK->SetLeftB((bool)(_b>0));
                                  LNK->SetRightB((bool)(_b>0));
                               }
                            else
                               {
                                  LNK->SetRightA((bool)(_a>0));
                                  LNK->SetLeftA((bool)(_a>0));
                                  LNK->SetLeftB((bool)(_b>0));
     
                                  if (booleng->Get_WindingRule())
                                     LNK->GetInc() ? _b++ : _b--;
                                  else
                                     _b++;
     
                                  LNK->SetRightB((bool)(_b>0));
                               }
                            break;
          default   :  _messagehandler->error("Undefined Direction of link","function IScanBeam::Calc_Set_Left_Right()");
                            break;
       }
    */
    //if the records are parallel (same begin/endnodes)
    //the above link a/b flag are adjusted to the current a/b depth
    if ( record_above_me && Equal( record_above_me ) )
    {
        par = true;
        LNK->Mark();
        record_above_me->_a = _a;
        record_above_me->_b = _b;
        if ( Direction() == GO_LEFT )
        {
            //set the bottom side  of the above link
            if ( record_above_me->Direction() == GO_LEFT )
            {
                record_above_me->LNK->SetLeftA( LNK->GetLeftA() );
                record_above_me->LNK->SetLeftB( LNK->GetLeftB() );
            }
            else
            {
                record_above_me->LNK->SetRightA( LNK->GetLeftA() );
                record_above_me->LNK->SetRightB( LNK->GetLeftB() );
            }
        }
        else
        {
            //set the bottom side  of the above link
            if ( record_above_me->Direction() == GO_LEFT )
            {
                record_above_me->LNK->SetLeftA( LNK->GetRightA() );
                record_above_me->LNK->SetLeftB( LNK->GetRightB() );
            }
            else
            {
                record_above_me->LNK->SetRightA( LNK->GetRightA() );
                record_above_me->LNK->SetRightB( LNK->GetRightB() );
            }
        }
    }
    return par;
}

bool kbRecord::Equal( kbRecord *a )
{
    return( ( bool )( ( LNK->GetOther( a->LNK->GetBeginNode() ) == a->LNK->GetEndNode() )  &&
                      ( LNK->GetOther( a->LNK->GetEndNode() )   == a->LNK->GetBeginNode() )  ) );
}


kbLine* kbRecord::GetLine()
{
    return & _line;
}


