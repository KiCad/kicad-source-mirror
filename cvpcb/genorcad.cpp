/***************/
/* genorcad()  */
/***************/

/*
 *  ComplŠte la netliste (*.NET) en y placant les ref *.lib FORMAT ORCADPCB
 *  La valeur (Part Value) est tronquee a 16 lettres
 */

#include "fctsys.h"

#include "common.h"
#include "cvpcb.h"

#include "protos.h"

#define MAX_LEN_NETNAME 16

/* Routines locales */
static void TriPinsModule( COMPONENT* CurrentCmp );
static void ChangePinNet(  wxString& PinNet, bool rightJustify );

/* Variables Locales */
int NetNumCode;         /* Nombre utilise pour cree des NetNames lors de
                         *  reaffectation de NetNames */

int genorcad( bool rightJustify )
{
    char       Line[1024];
    PIN*       Pin;
    COMPONENT* CurrentCmp;
    wxString   Title = wxGetApp().GetAppName() + wxT( " " ) + GetBuildVersion();

    NetNumCode = 1; DateAndTime( Line );
    fprintf( dest, "( { Netlist by %s, date = %s }\n",
             CONV_TO_UTF8( Title ), Line );

    /***********************/
    /* Lecture de la liste */
    /***********************/

    CurrentCmp = BaseListeCmp;
    for( ; CurrentCmp != NULL; CurrentCmp = CurrentCmp->Pnext )
    {
        fprintf( dest, " ( %s ", CONV_TO_UTF8( CurrentCmp->m_TimeStamp ) );

        if( !CurrentCmp->m_Module.IsEmpty() )
            fprintf( dest, CONV_TO_UTF8( CurrentCmp->m_Module ) );

        else
            fprintf( dest, "$noname$" );

        fprintf( dest, " %s ", CONV_TO_UTF8( CurrentCmp->m_Reference ) );

        /* placement de la valeur */
        fprintf( dest, "%s\n", CONV_TO_UTF8( CurrentCmp->m_Valeur ) );

        /* Tri des pins */
        TriPinsModule( CurrentCmp );

        /* Placement de la liste des pins */
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


/***********************************************/
static void TriPinsModule( COMPONENT* CurrentCmp )
/***********************************************/

/* Tri et controle des pins du module CurrentCmp
 */
{
    PIN* Pin, * NextPin, ** BasePin;
    int  nbpins = 0, ii;

    Pin = CurrentCmp->m_Pins;
    if( Pin == NULL )
        return;

    /* comptage des pins */
    for( ; Pin != NULL; Pin = Pin->Pnext )
        nbpins++;

    /* Tri des pins: etablissement de la liste des pointeurs */
    BasePin = (PIN**) MyZMalloc( nbpins * sizeof(PIN*) );

    Pin = CurrentCmp->m_Pins;
    for( ii = 0; ii < nbpins; ii++, Pin = Pin->Pnext )
    {
        BasePin[ii] = Pin;
    }

    /* Tri des Pins */
    qsort( BasePin, nbpins, sizeof( COMPONENT*), PinCompare );

    /* Remise a jour des pointeurs chaines */
    for( ii = 0; ii < nbpins - 1; ii++ )
    {
        BasePin[ii]->Pnext = BasePin[ii + 1];
    }

    BasePin[ii]->Pnext = NULL;
    CurrentCmp->m_Pins = BasePin[0];

    MyFree( BasePin );

    /* Elimination des redondances */
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
        /* 2 pins successives ont le meme numero */
        if( Pin->m_PinNet != NextPin->m_PinNet )
        {
            wxString msg;
            msg.Printf( _( "%s %s pin %s : Different Nets" ),
                        CurrentCmp->m_Reference.GetData(),
                        CurrentCmp->m_Valeur.GetData(),
                        Pin->m_PinNum.GetData() );
            DisplayError( NULL, msg, 60 );
        }
        Pin->Pnext = NextPin->Pnext;
        delete NextPin;
    }
}


/*******************************************/
static void ChangePinNet( wxString& PinNet, bool rightJustify )
/*******************************************/

/* Change le NetName PinNet par un nom compose des 8 derniers codes de PinNet
 *   suivi de _Xnnnnn ou nnnnn est un nom de 0 a 99999
 */
{
    PIN*       Pin;
    COMPONENT* CurrentCmp;
    int        ii;
    wxString   OldName;
    wxString   NewName;

    OldName = PinNet;
    ii = PinNet.Len();
    if( rightJustify )  /* On conserve les 8 dernieres lettres du nom */
    {
        NewName = OldName.Right( 8 ); NewName << NetNumCode;
    }
    else             /* On conserve les 8 premieres lettres du nom */
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
