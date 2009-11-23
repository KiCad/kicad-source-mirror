/*****************/
/* genorcad.cpp  */
/*****************/

/*
 * Create the netlist (* NET) by placing the *.lib ref FORMAT ORCADPCB
 * The value (share value) is truncated to 16 letters.
 */

#include "fctsys.h"

#include "common.h"
#include "cvpcb.h"

#include "protos.h"

#define MAX_LEN_NETNAME 16


static void TriPinsModule( COMPONENT* CurrentCmp );
static void ChangePinNet(  wxString& PinNet, bool rightJustify );


int NetNumCode;         /* Number of used for NetNames created during
                         * reallocation of NetNames. */

int genorcad( bool rightJustify )
{
    char       Line[1024];
    PIN*       Pin;
    COMPONENT* CurrentCmp;
    wxString   Title = wxGetApp().GetAppName() + wxT( " " ) + GetBuildVersion();

    NetNumCode = 1; DateAndTime( Line );
    fprintf( dest, "( { Netlist by %s, date = %s }\n",
             CONV_TO_UTF8( Title ), Line );

    CurrentCmp = BaseListeCmp;
    for( ; CurrentCmp != NULL; CurrentCmp = CurrentCmp->Pnext )
    {
        fprintf( dest, " ( %s ", CONV_TO_UTF8( CurrentCmp->m_TimeStamp ) );

        if( !CurrentCmp->m_Module.IsEmpty() )
            fprintf( dest, CONV_TO_UTF8( CurrentCmp->m_Module ) );

        else
            fprintf( dest, "$noname$" );

        fprintf( dest, " %s ", CONV_TO_UTF8( CurrentCmp->m_Reference ) );

        fprintf( dest, "%s\n", CONV_TO_UTF8( CurrentCmp->m_Value ) );

        /* Sort pins. */
        TriPinsModule( CurrentCmp );

        Pin = CurrentCmp->m_Pins;
        for( ; Pin != NULL; Pin = Pin->Pnext )
        {
            if( Pin->m_PinNet.Len() > MAX_LEN_NETNAME )
                ChangePinNet( Pin->m_PinNet, rightJustify );

            if( !Pin->m_PinNet.IsEmpty() )
                fprintf( dest, "  ( %s %s )\n",
                         CONV_TO_UTF8( Pin->m_PinNum ),
                         CONV_TO_UTF8( Pin->m_PinNet ) );
            else
                fprintf( dest, "  ( %s ? )\n", CONV_TO_UTF8( Pin->m_PinNum ) );
        }

        fprintf( dest, " )\n" );
    }

    fprintf( dest, ")\n*\n" );
    fclose( dest );
    return 0;
}


/* Sort pins */
static void TriPinsModule( COMPONENT* CurrentCmp )
{
    PIN* Pin, * NextPin, ** BasePin;
    int  nbpins = 0, ii;

    Pin = CurrentCmp->m_Pins;
    if( Pin == NULL )
        return;

    for( ; Pin != NULL; Pin = Pin->Pnext )
        nbpins++;

    BasePin = (PIN**) MyZMalloc( nbpins * sizeof(PIN*) );

    Pin = CurrentCmp->m_Pins;
    for( ii = 0; ii < nbpins; ii++, Pin = Pin->Pnext )
    {
        BasePin[ii] = Pin;
    }

    qsort( BasePin, nbpins, sizeof( COMPONENT*), PinCompare );

    for( ii = 0; ii < nbpins - 1; ii++ )
    {
        BasePin[ii]->Pnext = BasePin[ii + 1];
    }

    BasePin[ii]->Pnext = NULL;
    CurrentCmp->m_Pins = BasePin[0];

    MyFree( BasePin );

    /* Remove duplicate pins. */
    Pin = CurrentCmp->m_Pins;
    while( Pin != NULL )
    {
        NextPin = Pin->Pnext;
        if( NextPin == NULL )
            break;
        if( Pin->m_PinNum != NextPin->m_PinNum )
        {
            Pin = Pin->Pnext;  continue;
        }
        /* Successive 2 pins have the same number. */
        if( Pin->m_PinNet != NextPin->m_PinNet )
        {
            wxString msg;
            msg.Printf( _( "%s %s pin %s : Different Nets" ),
                        CurrentCmp->m_Reference.GetData(),
                        CurrentCmp->m_Value.GetData(),
                        Pin->m_PinNum.GetData() );
            DisplayError( NULL, msg, 60 );
        }
        Pin->Pnext = NextPin->Pnext;
        delete NextPin;
    }
}


/* ???
 *
 * Change le NetName PinNet par un nom compose des 8 derniers codes de PinNet
 * suivi de _Xnnnnn ou nnnnn est un nom de 0 a 99999
 */
static void ChangePinNet( wxString& PinNet, bool rightJustify )
{
    PIN*       Pin;
    COMPONENT* CurrentCmp;
    int        ii;
    wxString   OldName;
    wxString   NewName;

    OldName = PinNet;
    ii = PinNet.Len();
    if( rightJustify )  /* Retain the last 8 letters of the name. */
    {
        NewName = OldName.Right( 8 ); NewName << NetNumCode;
    }
    else                /* Retains the 8 first letters of the name. */
    {
        NewName = OldName.Left( 8 ); NewName << NetNumCode;
    }
    NetNumCode++;

    CurrentCmp = BaseListeCmp;
    for( ; CurrentCmp != NULL; CurrentCmp = CurrentCmp->Pnext )
    {
        Pin = CurrentCmp->m_Pins;
        for( ; Pin != NULL; Pin = Pin->Pnext )
        {
            if( Pin->m_PinNet != OldName )
                continue;
            Pin->m_PinNet = NewName;
        }
    }
}
