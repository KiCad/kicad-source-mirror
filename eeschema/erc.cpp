/**************************************/
/*  erc.cpp - Electrical Rules Check  */
/**************************************/

#include "fctsys.h"
#include "common.h"
#include "class_drawpanel.h"
#include "kicad_string.h"
#include "bitmaps.h"
#include "class_sch_screen.h"
#include "wxEeschemaStruct.h"

#include "general.h"
#include "netlist.h"
#include "lib_pin.h"
#include "protos.h"
#include "erc.h"
#include "sch_marker.h"
#include "sch_component.h"
#include "sch_sheet.h"


/* ERC tests :
 *  1 - conflicts between connected pins ( example: 2 connected outputs )
 *  2 - minimal connections requirements ( 1 input *must* be connected to an
 * output, or a passive pin )
 */


/*
 *  Electrical type of pins:
 *  PIN_INPUT = usual pin input: must be connected
 *  PIN_OUTPUT = usual output
 *  PIN_BIDI = input or output (like port for a microprocessor)
 *  PIN_TRISTATE = tris state bus pin
 *  PIN_PASSIVE = pin for passive components: must be connected, and can be
 * connected to any pin
 *  PIN_UNSPECIFIED = unknown electrical properties: creates always a warning
 * when connected
 *  PIN_POWER_IN = power input (GND, VCC for ICs). Must be connected to a power
 * output.
 *  PIN_POWER_OUT = output of a regulator: intended to be connected to power
 * input pins
 *  PIN_OPENCOLLECTOR = pin type open collector
 *  PIN_OPENEMITTER = pin type open emitter
 *  PIN_NC = not connected (must be left open)
 *
 *  Minimal requirements:
 *  All pins *must* be connected (except PIN_NC).
 *  When a pin is not connected in schematic, the user must place a "non
 * connected" symbol to this pin.
 *  This ensures a forgotten connection will be detected.
 */

/* Messages for conflicts :
 *  PIN_INPUT, PIN_OUTPUT, PIN_BIDI, PIN_TRISTATE, PIN_PASSIVE,
 *  PIN_UNSPECIFIED, PIN_POWER_IN, PIN_POWER_OUT, PIN_OPENCOLLECTOR,
 *  PIN_OPENEMITTER, PIN_NC
 *  These messages are used to show the ERC matrix in ERC dialog
 */

// Messages for matrix rows:
const wxChar* CommentERC_H[] =
{
    wxT( "Input Pin...." ),
    wxT( "Output Pin..." ),
    wxT( "BiDi Pin....." ),
    wxT( "3 State Pin.." ),
    wxT( "Passive Pin.." ),
    wxT( "Unspec Pin..." ),
    wxT( "Power IN Pin." ),
    wxT( "PowerOUT Pin." ),
    wxT( "Open Coll...." ),
    wxT( "Open Emit...." ),
    wxT( "No Conn......" ),
    NULL
};

// Messages for matrix columns
const wxChar* CommentERC_V[] =
{
    wxT( "Input Pin" ),
    wxT( "Output Pin" ),
    wxT( "BiDi Pin" ),
    wxT( "3 State Pin" ),
    wxT( "Passive Pin" ),
    wxT( "Unspec Pin" ),
    wxT( "Power IN Pin" ),
    wxT( "PowerOUT Pin" ),
    wxT( "Open Coll" ),
    wxT( "Open Emit" ),
    wxT( "No Conn" ),
    NULL
};


/* Look up table which gives the diag for a pair of connected pins
 *  Can be modified by ERC options.
 *  at start up: must be loaded by DefaultDiagErc
 */
int  DiagErc[PIN_NMAX][PIN_NMAX];
bool DiagErcTableInit;       // go to TRUE after DiagErc init

/* Default Look up table which gives the diag for a pair of connected pins
 *  Same as DiagErc, but cannot be modified
 *  Used to init or reset DiagErc
 *  note also, to avoid inconsistancy:
 *    DefaultDiagErc[i][j] = DefaultDiagErc[j][i]
 */
int DefaultDiagErc[PIN_NMAX][PIN_NMAX] =
{
/*         I,   O,    Bi,   3S,   Pas,  UnS,  PwrI, PwrO, OC,   OE,   NC */
/* I */  { OK,  OK,   OK,   OK,   OK,   WAR,  OK,   OK,   OK,   OK,   ERR },
/* O */  { OK,  ERR,  OK,   WAR,  OK,   WAR,  OK,   ERR,  ERR,  ERR,  ERR },
/* Bi*/  { OK,  OK,   OK,   OK,   OK,   WAR,  OK,   WAR,  OK,   WAR,  ERR },
/* 3S*/  { OK,  WAR,  OK,   OK,   OK,   WAR,  WAR,  ERR,  WAR,  WAR,  ERR },
/*Pas*/  { OK,  OK,   OK,   OK,   OK,   WAR,  OK,   OK,   OK,   OK,   ERR },
/*UnS */ { WAR, WAR,  WAR,  WAR,  WAR,  WAR,  WAR,  WAR,  WAR,  WAR,  ERR },
/*PwrI*/ { OK,  OK,   OK,   WAR,  OK,   WAR,  OK,   OK,   OK,   OK,   ERR },
/*PwrO*/ { OK,  ERR,  WAR,  ERR,  OK,   WAR,  OK,   ERR,  ERR,  ERR,  ERR },
/* OC */ { OK,  ERR,  OK,   WAR,  OK,   WAR,  OK,   ERR,  OK,   OK,   ERR },
/* OE */ { OK,  ERR,  WAR,  WAR,  OK,   WAR,  OK,   ERR,  OK,   OK,   ERR },
/* NC */ { ERR, ERR,  ERR,  ERR,  ERR,  ERR,  ERR,  ERR,  ERR,  ERR,  ERR }
};


/* Look up table which gives the minimal drive for a pair of connected pins on
 * a net
 *  Initial state of a net is NOC (Net with No Connection)
 *  Can be updated to NPI (Pin Isolated), NET_NC (Net with a no connect symbol),
 *                  NOD (Not Driven) or DRV (DRIven)
 *
 *  Can be updated to NET_NC with no error only if there is only one pin in net
 *
 *  Nets are OK when their final state is NET_NC or DRV
 *  Nets with the state NOD have no source signal
 */
static int MinimalReq[PIN_NMAX][PIN_NMAX] =
{
/*         In   Out, Bi,  3S,  Pas, UnS, PwrI,PwrO,OC,  OE,  NC */
/* In*/  { NOD, DRV, DRV, DRV, DRV, DRV, NOD, DRV, DRV, DRV, NPI },
/*Out*/  { DRV, DRV, DRV, DRV, DRV, DRV, DRV, DRV, DRV, DRV, NPI },
/* Bi*/  { DRV, DRV, DRV, DRV, DRV, DRV, NOD, DRV, DRV, DRV, NPI },
/* 3S*/  { DRV, DRV, DRV, DRV, DRV, DRV, NOD, DRV, DRV, DRV, NPI },
/*Pas*/  { DRV, DRV, DRV, DRV, DRV, DRV, NOD, DRV, DRV, DRV, NPI },
/*UnS*/  { DRV, DRV, DRV, DRV, DRV, DRV, NOD, DRV, DRV, DRV, NPI },
/*PwrI*/ { NOD, DRV, NOD, NOD, NOD, NOD, NOD, DRV, NOD, NOD, NPI },
/*PwrO*/ { DRV, DRV, DRV, DRV, DRV, DRV, DRV, DRV, DRV, DRV, NPI },
/* OC*/  { DRV, DRV, DRV, DRV, DRV, DRV, NOD, DRV, DRV, DRV, NPI },
/* OE*/  { DRV, DRV, DRV, DRV, DRV, DRV, NOD, DRV, DRV, DRV, NPI },
/* NC*/  { NPI, NPI, NPI, NPI, NPI, NPI, NPI, NPI, NPI, NPI, NPI }
};


/** Function TestDuplicateSheetNames( )
 * inside a given sheet, one cannot have sheets with duplicate names (file
 * names can be duplicated).
 * @return the error count
 * @param aCreateMarker: true = create error markers in schematic,
 *                       false = calculate error count only
 */
int TestDuplicateSheetNames( bool aCreateMarker )
{
    int         err_count = 0;
    SCH_SCREENS ScreenList;      // Created the list of screen

    for( SCH_SCREEN* Screen = ScreenList.GetFirst();
         Screen != NULL;
         Screen = ScreenList.GetNext() )
    {
        for( SCH_ITEM* ref_item = Screen->EEDrawList;
             ref_item != NULL;
             ref_item = ref_item->Next() )
        {
            // search for a sheet;
            if( ref_item->Type() != DRAW_SHEET_STRUCT_TYPE )
                continue;
            for( SCH_ITEM* item_to_test = ref_item->Next();
                 item_to_test != NULL;
                 item_to_test = item_to_test->Next() )
            {
                if( item_to_test->Type() != DRAW_SHEET_STRUCT_TYPE )
                    continue;

                // We have found a second sheet: compare names
                if( ( (SCH_SHEET*) ref_item )->m_SheetName.CmpNoCase(
                        ( ( SCH_SHEET* ) item_to_test )-> m_SheetName )
                    == 0 )
                {
                    if( aCreateMarker )
                    {
                        /* Create a new marker type ERC error*/
                        SCH_MARKER* Marker = new SCH_MARKER();
                        Marker->m_TimeStamp = GetTimeStamp();
                        Marker->SetData( ERCE_DUPLICATE_SHEET_NAME,
                                         ( (SCH_SHEET*) item_to_test )->m_Pos,
                                         _( "Duplicate Sheet name" ),
                                         ( (SCH_SHEET*) item_to_test )->m_Pos );
                        Marker->SetMarkerType( MARK_ERC );
                        Marker->SetErrorLevel( ERR );
                        Marker->SetNext( Screen->EEDrawList );
                        Screen->EEDrawList = Marker;
                    }
                    err_count++;
                }
            }
        }
    }

    return err_count;
}


/* Creates an ERC marker to show the ERC problem about aNetItemRef
 * or between aNetItemRef and aNetItemTst
 *  if MinConn < 0: this is an error on labels
 */
void Diagnose( WinEDA_DrawPanel* aPanel,
               NETLIST_OBJECT* aNetItemRef,
               NETLIST_OBJECT* aNetItemTst,
               int aMinConn, int aDiag )
{
    SCH_MARKER* Marker = NULL;
    SCH_SCREEN* screen;
    int         ii, jj;

    if( aDiag == OK )
        return;

    /* Create new marker for ERC error. */
    Marker = new SCH_MARKER();
    Marker->m_TimeStamp = GetTimeStamp();

    Marker->SetMarkerType( MARK_ERC );
    Marker->SetErrorLevel( WAR );
    screen = aNetItemRef->m_SheetList.LastScreen();
    Marker->SetNext( screen->EEDrawList );
    screen->EEDrawList = Marker;
    g_EESchemaVar.NbErrorErc++;
    g_EESchemaVar.NbWarningErc++;

    wxString msg;
    if( aMinConn < 0 )
    {
        if( (aNetItemRef->m_Type == NET_HIERLABEL)
            || (aNetItemRef->m_Type == NET_HIERBUSLABELMEMBER) )
        {
            msg.Printf( _( "HLabel %s not connected to SheetLabel" ),
                        GetChars( aNetItemRef->m_Label ) );
        }
        else
        {
            msg.Printf( _( "SheetLabel %s not connected to HLabel" ),
                        GetChars( aNetItemRef->m_Label ) );
        }


        Marker->SetData( ERCE_HIERACHICAL_LABEL,
                         aNetItemRef->m_Start,
                         msg,
                         aNetItemRef->m_Start );
        return;
    }

    ii = aNetItemRef->m_ElectricalType;

    wxString string_pinnum, cmp_ref;
    char     ascii_buf[5];
    ascii_buf[4] = 0;
    memcpy( ascii_buf, &aNetItemRef->m_PinNum, 4 );
    string_pinnum = CONV_FROM_UTF8( ascii_buf );
    cmp_ref = wxT( "?" );
    if( aNetItemRef->m_Type == NET_PIN && aNetItemRef->m_Link )
        cmp_ref = ( (SCH_COMPONENT*) aNetItemRef->m_Link )->GetRef( &aNetItemRef->m_SheetList );

    if( aNetItemTst == NULL )
    {
        if( aMinConn == NOC )    /* Only 1 element in the net. */
        {
            msg.Printf( _( "Cmp %s, Pin %s (%s) Unconnected" ),
                        GetChars( cmp_ref ), GetChars( string_pinnum ),
                        MsgPinElectricType[ii] );
            Marker->SetData( ERCE_PIN_NOT_CONNECTED,
                             aNetItemRef->m_Start,
                             msg,
                             aNetItemRef->m_Start );
            return;
        }

        if( aMinConn == NOD )    /* Nothing driving the net. */
        {
            if( aNetItemRef->m_Type == NET_PIN && aNetItemRef->m_Link )
                cmp_ref = ( (SCH_COMPONENT*) aNetItemRef->m_Link )->GetRef(
                    &aNetItemRef->m_SheetList );
            msg.Printf( _( "Cmp %s, Pin %s (%s) not driven (Net %d)" ),
                        GetChars( cmp_ref ), GetChars( string_pinnum ),
                        MsgPinElectricType[ii], aNetItemRef->GetNet() );
            Marker->SetData( ERCE_PIN_NOT_DRIVEN,
                             aNetItemRef->m_Start,
                             msg,
                             aNetItemRef->m_Start );
            return;
        }

        if( aDiag == UNC )
        {
            msg.Printf( _( "More than 1 Pin connected to UnConnect symbol" ) );
            Marker->SetData( ERCE_NOCONNECT_CONNECTED,
                             aNetItemRef->m_Start,
                             msg,
                             aNetItemRef->m_Start );
            return;
        }
    }

    if( aNetItemTst )         /* Error between 2 pins */
    {
        jj = aNetItemTst->m_ElectricalType;
        int errortype = ERCE_PIN_TO_PIN_WARNING;
        if( aDiag == ERR )
        {
            Marker->SetErrorLevel( ERR );
            g_EESchemaVar.NbWarningErc--;
            errortype = ERCE_PIN_TO_PIN_ERROR;
        }

        wxString alt_string_pinnum, alt_cmp;
        memcpy( ascii_buf, &aNetItemTst->m_PinNum, 4 );
        alt_string_pinnum = CONV_FROM_UTF8( ascii_buf );
        alt_cmp = wxT( "?" );
        if( aNetItemTst->m_Type == NET_PIN && aNetItemTst->m_Link )
            alt_cmp = ( (SCH_COMPONENT*) aNetItemTst->m_Link )->GetRef(
                &aNetItemTst->m_SheetList );
        msg.Printf( _( "Cmp %s, Pin %s (%s) connected to " ),
                    GetChars( cmp_ref ), GetChars( string_pinnum ), MsgPinElectricType[ii] );
        Marker->SetData( errortype,
                         aNetItemRef->m_Start,
                         msg,
                         aNetItemRef->m_Start );
        msg.Printf( _( "Cmp %s, Pin %s (%s) (net %d)" ),
                    GetChars( alt_cmp ), GetChars( alt_string_pinnum ), MsgPinElectricType[jj],
                    aNetItemRef->GetNet() );
        Marker->SetAuxiliaryData( msg, aNetItemTst->m_Start );
    }
}


/* Routine testing electrical conflicts between NetItemRef and other items
 * of the same net
 */
void TestOthersItems( WinEDA_DrawPanel* panel,
                      unsigned NetItemRef,
                      unsigned netstart,
                      int* NetNbItems, int* MinConnexion )
{
    unsigned NetItemTst;

    int      ref_elect_type, jj, erc = OK, local_minconn;

    /* Analysis of the table of connections. */
    ref_elect_type = g_NetObjectslist[NetItemRef]->m_ElectricalType;

    NetItemTst    = netstart;
    local_minconn = NOC;
    if( ref_elect_type == PIN_NC )
        local_minconn = NPI;

    /* Test pins connected to NetItemRef */
    for( ; ; NetItemTst++ )
    {
        if( NetItemRef == NetItemTst )
            continue;

        /* We examine only a given net. We stop the search if the net changes
         **/
        if( ( NetItemTst >= g_NetObjectslist.size() ) // End of list
            || ( g_NetObjectslist[NetItemRef]->GetNet() !=
                 g_NetObjectslist[NetItemTst]->GetNet() ) ) // End of net
        {
            /* End net code found: minimum connection test. */
            if( (*MinConnexion < NET_NC ) && (local_minconn < NET_NC ) )
            {
                /* Not connected or not driven pin. */
                bool seterr = true;
                if( local_minconn == NOC
                    && g_NetObjectslist[NetItemRef]->m_Type == NET_PIN )
                {
                    /* This pin is not connected: for multiple part per
                     * package, and duplicated pin,
                     * search for an other instance of this pin
                     * this will be flagged only is all instances of this pin
                     * are not connected
                     * TODO test also if instances connected are connected to
                     * the same net
                     */
                    for( unsigned duppin = 0;
                         duppin < g_NetObjectslist.size();
                         duppin++ )
                    {
                        if( g_NetObjectslist[duppin]->m_Type != NET_PIN )
                            continue;
                        if( duppin == NetItemRef )
                            continue;
                        if( g_NetObjectslist[NetItemRef]->m_PinNum !=
                            g_NetObjectslist[duppin]->m_PinNum )
                            continue;

                        if( ( (SCH_COMPONENT*) g_NetObjectslist[NetItemRef]->
                             m_Link )->GetRef( &g_NetObjectslist[NetItemRef]->
                                               m_SheetList ) !=
                           ( (SCH_COMPONENT*) g_NetObjectslist[duppin]->m_Link )
                           ->GetRef( &g_NetObjectslist[duppin]->m_SheetList ) )
                            continue;

                        // Same component and same pin. Do dot create error for
                        // this pin
                        // if the other pin is connected (i.e. if duppin net
                        // has an other item)
                        if( (duppin > 0)
                           && ( g_NetObjectslist[duppin]->GetNet() ==
                               g_NetObjectslist[duppin - 1]->GetNet() ) )
                            seterr = false;
                        if( (duppin < g_NetObjectslist.size() - 1)
                           && ( g_NetObjectslist[duppin]->GetNet() ==
                               g_NetObjectslist[duppin + 1]->GetNet() ) )
                            seterr = false;
                    }
                }
                if( seterr )
                    Diagnose( panel,
                              g_NetObjectslist[NetItemRef],
                              NULL,
                              local_minconn,
                              WAR );
                *MinConnexion = DRV;   // inhibiting other messages of this
                                       // type for the net.
            }
            return;
        }

        switch( g_NetObjectslist[NetItemTst]->m_Type )
        {
        case NET_ITEM_UNSPECIFIED:
        case NET_SEGMENT:
        case NET_BUS:
        case NET_JONCTION:
        case NET_LABEL:
        case NET_HIERLABEL:
        case NET_BUSLABELMEMBER:
        case NET_HIERBUSLABELMEMBER:
        case NET_SHEETBUSLABELMEMBER:
        case NET_SHEETLABEL:
        case NET_GLOBLABEL:
        case NET_GLOBBUSLABELMEMBER:
        case NET_PINLABEL:
            break;

        case NET_NOCONNECT:
            local_minconn = MAX( NET_NC, local_minconn );
            break;

        case NET_PIN:
            jj = g_NetObjectslist[NetItemTst]->m_ElectricalType;
            local_minconn = MAX( MinimalReq[ref_elect_type][jj], local_minconn );

            if( NetItemTst <= NetItemRef )
                break;

            *NetNbItems += 1;
            if( erc == OK )
            {
                erc = DiagErc[ref_elect_type][jj];
                if( erc != OK )
                {
                    if( g_NetObjectslist[NetItemTst]->m_FlagOfConnection == 0 )
                    {
                        Diagnose( panel,
                                  g_NetObjectslist[NetItemRef],
                                  g_NetObjectslist[NetItemTst],
                                  0,
                                  erc );
                        g_NetObjectslist[NetItemTst]->m_FlagOfConnection =
                            NOCONNECT_SYMBOL_PRESENT;
                    }
                }
            }
            break;
        }
    }
}


/* Create the Diagnostic file (<xxx>.erc file)
 */
bool WriteDiagnosticERC( const wxString& FullFileName )
{
    SCH_ITEM*       DrawStruct;
    SCH_MARKER*     Marker;
    char            Line[1024];
    static FILE*    OutErc;
    SCH_SHEET_PATH* Sheet;
    wxString        msg;

    if( ( OutErc = wxFopen( FullFileName, wxT( "wt" ) ) ) == NULL )
        return FALSE;

    DateAndTime( Line );
    msg = _( "ERC report" );

    fprintf( OutErc, "%s (%s)\n", CONV_TO_UTF8( msg ), Line );

    SCH_SHEET_LIST SheetList;

    for( Sheet = SheetList.GetFirst(); Sheet != NULL; Sheet = SheetList.GetNext() )
    {
        if( Sheet->Last() == g_RootSheet )
        {
            msg.Printf( _( "\n***** Sheet / (Root) \n" ) );
        }
        else
        {
            wxString str = Sheet->PathHumanReadable();
            msg.Printf( _( "\n***** Sheet %s\n" ), GetChars( str ) );
        }

        fprintf( OutErc, "%s", CONV_TO_UTF8( msg ) );

        DrawStruct = Sheet->LastDrawList();
        for( ; DrawStruct != NULL; DrawStruct = DrawStruct->Next() )
        {
            if( DrawStruct->Type() != TYPE_SCH_MARKER )
                continue;

            Marker = (SCH_MARKER*) DrawStruct;
            if( Marker->GetMarkerType() != MARK_ERC )
                continue;
            msg = Marker->GetReporter().ShowReport();
            fprintf( OutErc, "%s", CONV_TO_UTF8( msg ) );
        }
    }

    msg.Printf( _( "\n >> Errors ERC: %d\n" ), g_EESchemaVar.NbErrorErc );
    fprintf( OutErc, "%s", CONV_TO_UTF8( msg ) );
    fclose( OutErc );

    return TRUE;
}


static bool IsLabelsConnected( NETLIST_OBJECT* a, NETLIST_OBJECT* b )
{
    int at = a->m_Type;
    int bt = b->m_Type;

    if( ( at == NET_HIERLABEL || at == NET_HIERBUSLABELMEMBER )
       && ( bt == NET_SHEETLABEL || bt == NET_SHEETBUSLABELMEMBER ) )
    {
        if( a->m_SheetList == b->m_SheetListInclude )
        {
            return true; //connected!
        }
    }
    return false; //these two are unconnected
}


/* Routine to perform erc on a sheetLabel that is connected to a corresponding
 * sub sheet Glabel
 */
void TestLabel( WinEDA_DrawPanel* panel, unsigned NetItemRef, unsigned StartNet )
{
    unsigned NetItemTst;
    int      erc = 1;


    NetItemTst = StartNet;

    /* Review the list of labels connected to NetItemRef. */
    for( ; ; NetItemTst++ )
    {
        if( NetItemTst == NetItemRef )
            continue;

        /* Is always in the same net? */
        if( ( NetItemTst ==  g_NetObjectslist.size() )
           || ( g_NetObjectslist[NetItemRef]->GetNet() !=
                g_NetObjectslist[NetItemTst]->GetNet() ) )
        {
            /* End Netcode found. */
            if( erc )
            {
                /* Glabel or SheetLabel orphaned. */
                Diagnose( panel, g_NetObjectslist[NetItemRef], NULL, -1, WAR );
            }
            return;
        }
        if( IsLabelsConnected( g_NetObjectslist[NetItemRef],
                               g_NetObjectslist[NetItemTst] ) )
            erc = 0;

        //same thing, different order.
        if( IsLabelsConnected( g_NetObjectslist[NetItemTst],
                               g_NetObjectslist[NetItemRef] ) )
            erc = 0;
    }
}
