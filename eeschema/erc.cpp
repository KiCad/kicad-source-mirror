/**************************************************/
/* Module de tst "ERC" ( Electrical Rules Check ) */
/**************************************************/

#include "fctsys.h"
#include "common.h"
#include "class_drawpanel.h"
#include "kicad_string.h"
#include "gestfich.h"
#include "appl_wxstruct.h"

#include "program.h"
#include "libcmp.h"
#include "general.h"
#include "netlist.h"
#include "bitmaps.h"
#include "class_marker_sch.h"

#include "dialog_erc.h"
#include "erc.h"

#include "protos.h"

/* ERC tests :
 *  1 - conflicts between connected pins ( example: 2 connected outputs )
 *  2 - minimal connections requirements ( 1 input *must* be connected to an output, or a passive pin )
 */


/* fonctions locales */
static bool WriteDiagnosticERC( const wxString& FullFileName );
static void Diagnose( WinEDA_DrawPanel* panel,
                      ObjetNetListStruct* NetItemRef,
                      ObjetNetListStruct* NetItemTst, int MinConnexion, int Diag );
static void TestOthersItems( WinEDA_DrawPanel* panel,
                             ObjetNetListStruct* NetItemRef,
                             ObjetNetListStruct* NetStart,
                             int* NetNbItems, int* MinConnexion );
static void TestLabel( WinEDA_DrawPanel*   panel,
                       ObjetNetListStruct* NetItemRef,
                       ObjetNetListStruct* StartNet );

/* Local variables */
int WriteFichierERC = FALSE;

/*
 *  Electrical type of pins:
 *  PIN_INPUT = usual pin input: must be connected
 *  PIN_OUTPUT = usual output
 *  PIN_BIDI = input or output (like port for a microprocessor)
 *  PIN_TRISTATE = tris state bus pin
 *  PIN_PASSIVE = pin for passive components: must be connected, and can be connected to any pin
 *  PIN_UNSPECIFIED = unkown electrical properties: creates alway a warning when connected
 *  PIN_POWER_IN = power input (GND, VCC for ICs). Must be connected to a power output.
 *  PIN_POWER_OUT = output of a regulator: intended to be connected to power input pins
 *  PIN_OPENCOLLECTOR = pin type open collector
 *  PIN_OPENEMITTER = pin type open emitter
 *  PIN_NC = not connected (must be left open)
 *
 *  Minimal requirements:
 *  All pins *must* be connected (except PIN_NC).
 *  When a pin is not connected in schematic, the user must place a "non connected" symbol to this pin.
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
 */
int DefaultDiagErc[PIN_NMAX][PIN_NMAX] =
{
    /*       I,   O,   Bi,  3S, Pas, UnS,PwrI,PwrO,  OC,  OE,  NC */
/* I */  { OK,  OK,  OK,  OK,  OK,  WAR, OK,  OK,  OK,  OK,  WAR       },
/* O */  { OK,  ERR, OK,  WAR, OK,  WAR, OK,  ERR, ERR, ERR, WAR       },
/* Bi*/  { OK,  OK,  OK,  OK,  OK,  WAR, OK,  WAR, OK,  WAR, WAR       },
/* 3S*/  { OK,  WAR, OK,  OK,  OK,  WAR, WAR, ERR, WAR, WAR, WAR       },
/*Pas*/  { OK,  OK,  OK,  OK,  OK,  WAR, OK,  OK,  OK,  OK,  WAR       },
/*UnS */ { WAR, WAR, WAR, WAR, WAR, WAR, WAR, WAR, WAR, WAR, WAR       },
/*PwrI*/ { OK,  OK,  OK,  WAR, OK,  WAR, OK,  OK,  OK,  OK,  ERR       },
/*PwrO*/ { OK,  ERR, WAR, ERR, OK,  WAR, OK,  ERR, ERR, ERR, WAR       },
/* OC */ { OK,  ERR, OK,  WAR, OK,  WAR, OK,  ERR, OK,  OK,  WAR       },
/* OE */ { OK,  ERR, WAR, WAR, OK,  WAR, OK,  ERR, OK,  OK,  WAR       },
/* NC */ { WAR, WAR, WAR, WAR, WAR, WAR, WAR, WAR, WAR, WAR, WAR       }
};


/* Minimal connection table */
#define DRV    3    /* Net driven by a signal (a pin output for instance) */
#define NET_NC 2    /* Net "connected" to a "NoConnect symbol" */
#define NOD    1    /* Net not driven ( Such as 2 or more connected inputs )*/
#define NOC    0    /* Pin isolee, non connectee */

/* Look up table which gives the minimal drive for a pair of connected pins on a net
 *  Initial state of a net is NOC (No Connection)
 *  Can be updated to NET_NC, or NOD (Not Driven) or DRV (DRIven)
 *
 *  Can be updated to NET_NC only if the previous state is NOC
 *
 *  Nets are OK when their final state is NET_NC or DRV
 *  Nets with the state NOD have no source signal
 */
static int MinimalReq[PIN_NMAX][PIN_NMAX] =
{
    /* In, Out,  Bi,  3S, Pas, UnS,PwrI,PwrO,  OC,  OE,  NC */
/* In*/  {   NOD, DRV, DRV, DRV, DRV, DRV, NOD, DRV, DRV, DRV, NOC },
/*Out*/  {   DRV, DRV, DRV, DRV, DRV, DRV, DRV, DRV, DRV, DRV, NOC },
/* Bi*/  {   DRV, DRV, DRV, DRV, DRV, DRV, NOD, DRV, DRV, DRV, NOC },
/* 3S*/  {   DRV, DRV, DRV, DRV, DRV, DRV, NOD, DRV, DRV, DRV, NOC },
/*Pas*/  {   DRV, DRV, DRV, DRV, DRV, DRV, NOD, DRV, DRV, DRV, NOC },
/*UnS*/  {   DRV, DRV, DRV, DRV, DRV, DRV, NOD, DRV, DRV, DRV, NOC },
/*PwrI*/ {   NOD, DRV, NOD, NOD, NOD, NOD, NOD, DRV, NOD, NOD, NOC },
/*PwrO*/ {   DRV, DRV, DRV, DRV, DRV, DRV, DRV, DRV, DRV, DRV, NOC },
/* OC*/  {   DRV, DRV, DRV, DRV, DRV, DRV, NOD, DRV, DRV, DRV, NOC },
/* OE*/  {   DRV, DRV, DRV, DRV, DRV, DRV, NOD, DRV, DRV, DRV, NOC },
/* NC*/  {   NOC, NOC, NOC, NOC, NOC, NOC, NOC, NOC, NOC, NOC, NOC }
};



/**Function TestDuplicateSheetNames( )
 * inside a given sheet, one cannot have sheets with duplicate names (file names can be duplicated).
 */
int TestDuplicateSheetNames( )
{
    int err_count = 0;
    EDA_ScreenList ScreenList;      // Created the list of screen
    for( SCH_SCREEN* Screen = ScreenList.GetFirst(); Screen != NULL; Screen = ScreenList.GetNext() )
    {
        for( SCH_ITEM* ref_item = Screen->EEDrawList; ref_item != NULL; ref_item = ref_item->Next() )
        {
            // search for a scheet;
            if( ref_item->Type() != DRAW_SHEET_STRUCT_TYPE )
                continue;
            for( SCH_ITEM* item_to_test = ref_item->Next();
                item_to_test != NULL;
                item_to_test = item_to_test->Next() )
            {
                if( item_to_test->Type() != DRAW_SHEET_STRUCT_TYPE )
                    continue;

                // We have found a second sheet: compare names
                if( ( (DrawSheetStruct*) ref_item )->m_SheetName.CmpNoCase( ( (DrawSheetStruct*)
                                                                             item_to_test )->
                                                                           m_SheetName ) == 0 )
                {
                    /* Create a new marker type ERC error*/
                    MARKER_SCH* Marker = new MARKER_SCH();
                    Marker->m_TimeStamp = GetTimeStamp();
                    Marker->SetData( ERCE_DUPLICATE_SHEET_NAME,
                                     ( (DrawSheetStruct*) item_to_test )->m_Pos,
                                     _( "Duplicate Sheet name" ),
                                     ( (DrawSheetStruct*) item_to_test )->m_Pos );
                    Marker->SetMarkerType( MARK_ERC );
                    Marker->SetErrorLevel( ERR );
                    Marker->SetNext( Screen->EEDrawList );
                    Screen->EEDrawList = Marker;
                    err_count++;
                }
            }
        }
    }

    return err_count;
}

/**************************************************/
void DIALOG_ERC::TestErc( wxArrayString* aMessagesList )
/**************************************************/
{
    wxFileName          fn;
    ObjetNetListStruct* NetItemRef;
    ObjetNetListStruct* OldItem;
    ObjetNetListStruct* StartNet;
    ObjetNetListStruct* Lim;

    int NetNbItems, MinConn;

    if( !DiagErcTableInit )
    {
        memcpy( DiagErc, DefaultDiagErc, sizeof(DefaultDiagErc) );
        DiagErcTableInit = TRUE;
    }

    WriteFichierERC = m_WriteResultOpt->GetValue();

    ReAnnotatePowerSymbolsOnly();
    if( m_Parent->CheckAnnotate( aMessagesList, false ) )
    {
        if( aMessagesList )
        {
            wxString msg = _( "Annotation Required!" );
            msg += wxT( "\n" );
            aMessagesList->Add( msg );
        }
        return;
    }

    /* Erase all DRC markers */
    DeleteAllMarkers( MARK_ERC );

    g_EESchemaVar.NbErrorErc   = 0;
    g_EESchemaVar.NbWarningErc = 0;

    /* Cleanup the entire hierarchy */
    EDA_ScreenList ScreenList;

    for( SCH_SCREEN* Screen = ScreenList.GetFirst(); Screen != NULL; Screen = ScreenList.GetNext() )
    {
        bool ModifyWires;
        ModifyWires = Screen->SchematicCleanUp( NULL );

        /* if wire list has changed, delete Undo Redo list to avoid
         *  pointers on deleted data problems */
        if( ModifyWires )
            Screen->ClearUndoRedoList();
    }

    /* Test duplicate sheet names
     * inside a given sheet, one cannot have sheets with duplicate names (file names can be duplicated).
     */
    int errcnt = TestDuplicateSheetNames( );
    g_EESchemaVar.NbErrorErc += errcnt;
    g_EESchemaVar.NbWarningErc += errcnt;

    m_Parent->BuildNetListBase();

    /* Analyse de la table des connexions : */
    Lim = g_TabObjNet + g_NbrObjNet;

    /* Reset the flag m_FlagOfConnection, that will be used next, in calculations */
    for( NetItemRef = g_TabObjNet;  NetItemRef < Lim;   NetItemRef++ )
        NetItemRef->m_FlagOfConnection = UNCONNECTED;

    NetNbItems = 0;
    MinConn    = NOC;

    StartNet = OldItem = NetItemRef = g_TabObjNet;

    for( ; NetItemRef < Lim; NetItemRef++ )
    {
        /* Tst changement de net */
        if( OldItem->GetNet() != NetItemRef->GetNet() )
        {
            MinConn    = NOC;
            NetNbItems = 0;
            StartNet   = NetItemRef;
        }

        switch( NetItemRef->m_Type )
        {
        case NET_SEGMENT:
        case NET_BUS:
        case NET_JONCTION:
        case NET_LABEL:
        case NET_BUSLABELMEMBER:
        case NET_PINLABEL:
        case NET_GLOBLABEL:
        case NET_GLOBBUSLABELMEMBER:

            // These items do not create erc problems
            break;

        case NET_HIERLABEL:
        case NET_HIERBUSLABELMEMBER:
        case NET_SHEETLABEL:
        case NET_SHEETBUSLABELMEMBER:

            // ERC problems when pin sheets do not match hierachical labels.
            // Each pin sheet must match a hierachical label
            // Each hierachicallabel must match a pin sheet
            TestLabel( m_Parent->DrawPanel, NetItemRef, StartNet );
            break;

        case NET_NOCONNECT:

            // ERC problems when a noconnect symbol is connected to more than one pin.
            MinConn = NET_NC;
            if( NetNbItems != 0 )
                Diagnose( m_Parent->DrawPanel, NetItemRef, NULL, MinConn, UNC );
            break;

        case NET_PIN:

            // Look for ERC problems between pins:
            TestOthersItems( m_Parent->DrawPanel,
                             NetItemRef, StartNet, &NetNbItems, &MinConn );
            break;
        }

        OldItem = NetItemRef;
    }

    FreeTabNetList( g_TabObjNet, g_NbrObjNet );

    // Displays global results:
    wxString num;
    num.Printf( wxT( "%d" ), g_EESchemaVar.NbErrorErc );
    m_TotalErrCount->SetLabel( num );

    num.Printf( wxT( "%d" ), g_EESchemaVar.NbErrorErc - g_EESchemaVar.NbWarningErc );
    m_LastErrCount->SetLabel( num );

    num.Printf( wxT( "%d" ), g_EESchemaVar.NbWarningErc );
    m_LastWarningCount->SetLabel( num );

    // Display diags:
    DisplayERC_MarkersList();

    // Display new markers:
    m_Parent->DrawPanel->Refresh();

    /* Generation ouverture fichier diag */
    if( WriteFichierERC == TRUE )
    {
        fn = g_RootSheet->m_AssociatedScreen->m_FileName;
        fn.SetExt( wxT( "erc" ) );

        wxFileDialog dlg( this, _( "ERC File" ), fn.GetPath(), fn.GetFullName(),
                          _( "Electronic rule check file (.erc)|*.erc" ),
                          wxFD_SAVE | wxFD_OVERWRITE_PROMPT );

        if( dlg.ShowModal() == wxID_CANCEL )
            return;

        if( WriteDiagnosticERC( dlg.GetPath() ) )
        {
            Close( TRUE );
            ExecuteFile( this, wxGetApp().GetEditorName(),
                        QuoteFullPath( fn ) );
        }
    }
}


/********************************************************/
static void Diagnose( WinEDA_DrawPanel* aPanel,
                      ObjetNetListStruct* aNetItemRef,
                      ObjetNetListStruct* aNetItemTst,
                      int aMinConn, int aDiag )
/********************************************************/

/* Creates an ERC marker to show the ERC problem about aNetItemRef
 * or between aNetItemRef and aNetItemTst
 *  if MinConn < 0: this is an error on labels
 */
{
    MARKER_SCH* Marker = NULL;
    SCH_SCREEN* screen;
    int         ii, jj;

    if( aDiag == OK )
        return;

    /* Creation du nouveau marqueur type Erreur ERC */
    Marker = new MARKER_SCH();
    Marker->m_TimeStamp = GetTimeStamp();

    Marker->SetMarkerType( MARK_ERC );
    Marker->SetErrorLevel( WAR );
    screen = aNetItemRef->m_SheetList.LastScreen();
    Marker->SetNext( screen->EEDrawList );
    screen->EEDrawList = Marker;
    g_EESchemaVar.NbErrorErc++;
    g_EESchemaVar.NbWarningErc++;

    wxString msg;
    if( aMinConn < 0 )   // Traitement des erreurs sur labels
    {
        if( (aNetItemRef->m_Type == NET_HIERLABEL)
           || (aNetItemRef->m_Type == NET_HIERBUSLABELMEMBER) )
        {
            msg.Printf( _( "HLabel %s not connected to SheetLabel" ),
                       aNetItemRef->m_Label->GetData() );
        }
        else
            msg.Printf( _( "SheetLabel %s not connected to HLabel" ),
                       aNetItemRef->m_Label->GetData() );

        Marker->SetData( ERCE_HIERACHICAL_LABEL, aNetItemRef->m_Start, msg, aNetItemRef->m_Start );
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
        if( aMinConn == NOC )    /* 1 seul element dans le net */
        {
            msg.Printf( _( "Cmp %s, Pin %s (%s) Unconnected" ),
                        cmp_ref.GetData(), string_pinnum.GetData(),
                        MsgPinElectricType[ii] );
            Marker->SetData( ERCE_PIN_NOT_CONNECTED,
                             aNetItemRef->m_Start,
                             msg,
                             aNetItemRef->m_Start );
            return;
        }

        if( aMinConn == NOD )    /* pas de pilotage du net */
        {
            if( aNetItemRef->m_Type == NET_PIN && aNetItemRef->m_Link )
                cmp_ref = ( (SCH_COMPONENT*) aNetItemRef->m_Link )->GetRef(
                    &aNetItemRef->m_SheetList );
            msg.Printf(
                _( "Cmp %s, Pin %s (%s) not driven (Net %d)" ),
                cmp_ref.GetData(), string_pinnum.GetData(),
                MsgPinElectricType[ii], aNetItemRef->GetNet() );
            Marker->SetData( ERCE_PIN_NOT_DRIVEN,
                             aNetItemRef->m_Start,
                             msg,
                             aNetItemRef->m_Start );
            return;
        }

        if( aDiag == UNC )
        {
            msg.Printf(
                _( "More than 1 Pin connected to UnConnect symbol" ) );
            Marker->SetData( ERCE_NOCONNECT_CONNECTED,
                             aNetItemRef->m_Start,
                             msg,
                             aNetItemRef->m_Start );
            return;
        }
    }

    if( aNetItemTst )         /* Erreur entre 2 pins */
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
            alt_cmp = ( (SCH_COMPONENT*) aNetItemTst->m_Link )->GetRef( &aNetItemTst->m_SheetList );
        msg.Printf( _("Cmp %s, Pin %s (%s) connected to " ),
                   cmp_ref.GetData(), string_pinnum.GetData(), MsgPinElectricType[ii] );
        Marker->SetData( errortype,
                         aNetItemRef->m_Start,
                         msg,
                         aNetItemRef->m_Start );
        msg.Printf( _("Cmp %s, Pin %s (%s) (net %d)" ),
                   alt_cmp.GetData(), alt_string_pinnum.GetData(), MsgPinElectricType[jj],
                   aNetItemRef->GetNet() );
        Marker->SetAuxiliaryData( msg, aNetItemTst->m_Start );
    }
}


/********************************************************************/
static void TestOthersItems( WinEDA_DrawPanel* panel,
                             ObjetNetListStruct* NetItemRef,
                             ObjetNetListStruct* netstart,
                             int* NetNbItems, int* MinConnexion )
/********************************************************************/

/* Routine testant les conflits electriques entre
 *  NetItemRef
 *  et les autres items du meme net
 */
{
    ObjetNetListStruct* NetItemTst;
    ObjetNetListStruct* Lim;

    int ref_elect_type, jj, erc = OK, local_minconn;

    /* Analyse de la table des connexions : */
    Lim = g_TabObjNet + g_NbrObjNet;    // pointe la fin de la liste

    ref_elect_type = NetItemRef->m_ElectricalType;

    NetItemTst    = netstart;
    local_minconn = NOC;

    /* Test pins Pins connected to NetItemRef */
    for( ; ; NetItemTst++ )
    {
        if( NetItemRef == NetItemTst )
            continue;

        /* We examine only a given net. We stop the search if the net changes */
        if( (NetItemTst >= Lim)                                     // End of list
           || ( NetItemRef->GetNet() != NetItemTst->GetNet() ) )    // End of net
        {
            /* Fin de netcode trouve: Tst connexion minimum */
            if( (*MinConnexion < NET_NC ) && (local_minconn < NET_NC ) )  /* Not connected or not driven pin */
            {
                bool seterr = true;
                if( local_minconn == NOC && NetItemRef->m_Type == NET_PIN)
                {
                    /* This pin is not connected: for multiple part per package, and duplicated pin,
                    * search for an other instance of this pin
                    * this will be flagged only is all instances of this pin are not connected
                    * TODO test also if instances connected are connected to the same net
                    */
                    for ( ObjetNetListStruct *duppin = g_TabObjNet; duppin < Lim; duppin ++ )
                    {
                        if ( duppin->m_Type != NET_PIN )
                            continue;
                        if( duppin == NetItemRef )
                            continue;
                        if ( NetItemRef->m_PinNum != duppin->m_PinNum )
                            continue;

                        if( ( (SCH_COMPONENT*) NetItemRef->m_Link )->GetRef(&NetItemRef->m_SheetList) !=
                                ((SCH_COMPONENT*) duppin->m_Link )->GetRef(&duppin->m_SheetList) )
                            continue;
                        // Same component and same pin. Do dot create error for this pin
                        // if the other pin is connected (i.e. if duppin net has an other item)
                        if ( (duppin > g_TabObjNet) && (duppin->GetNet() == (duppin-1)->GetNet()))
                            seterr = false;
                        if ( (duppin < Lim-1) && (duppin->GetNet() == (duppin+1)->GetNet()) )
                            seterr = false;
                    }
                }
                if ( seterr )
                    Diagnose( panel, NetItemRef, NULL, local_minconn, WAR );
                *MinConnexion = DRV;   // inhibition autres messages de ce type pour ce net
            }
            return;
        }

        switch( NetItemTst->m_Type )
        {
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
            jj = NetItemTst->m_ElectricalType;
            local_minconn = MAX( MinimalReq[ref_elect_type][jj], local_minconn );

            if( NetItemTst <= NetItemRef )
                break;

            *NetNbItems += 1;
            if( erc == OK )         // 1 marqueur par pin maxi
            {
                erc = DiagErc[ref_elect_type][jj];
                if( erc != OK )
                {
                    if( NetItemTst->m_FlagOfConnection == 0 )
                    {
                        Diagnose( panel, NetItemRef, NetItemTst, 0, erc );
                        NetItemTst->m_FlagOfConnection = NOCONNECT;
                    }
                }
            }
            break;
        }
    }
}


/********************************************************/
static bool WriteDiagnosticERC( const wxString& FullFileName )
/*********************************************************/

/* Create the Diagnostic file (<xxx>.erc file)
 */
{
    SCH_ITEM*      DrawStruct;
    MARKER_SCH*    Marker;
    char           Line[1024];
    static FILE*   OutErc;
    DrawSheetPath* Sheet;
    wxString       msg;

    if( ( OutErc = wxFopen( FullFileName, wxT( "wt" ) ) ) == NULL )
        return FALSE;

    DateAndTime( Line );
    msg = _( "ERC report" );

    fprintf( OutErc, "%s (%s)\n", CONV_TO_UTF8( msg ), Line );

    EDA_SheetList SheetList;

    for( Sheet = SheetList.GetFirst(); Sheet != NULL; Sheet = SheetList.GetNext() )
    {
        if( Sheet->Last() == g_RootSheet )
        {
            msg.Printf( _( "\n***** Sheet / (Root) \n" ) );
        }
        else
        {
            wxString str = Sheet->PathHumanReadable();
            msg.Printf( _( "\n***** Sheet %s\n" ), str.GetData() );
        }

        fprintf( OutErc, "%s", CONV_TO_UTF8( msg ) );

        DrawStruct = Sheet->LastDrawList();
        for( ; DrawStruct != NULL; DrawStruct = DrawStruct->Next() )
        {
            if( DrawStruct->Type() != DRAW_MARKER_STRUCT_TYPE )
                continue;

            Marker = (MARKER_SCH*) DrawStruct;
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


static bool IsLabelsConnected( ObjetNetListStruct* a, ObjetNetListStruct* b )
{
    int at = a->m_Type;
    int bt = b->m_Type;

    if( (at == NET_HIERLABEL || at == NET_HIERBUSLABELMEMBER)
       &&(bt == NET_SHEETLABEL || bt == NET_SHEETBUSLABELMEMBER) )
    {
        if( a->m_SheetList == b->m_SheetListInclude )
        {
            return true; //connected!
        }
    }
    return false; //these two are unconnected
}


/***********************************************************************/
void TestLabel( WinEDA_DrawPanel*   panel,
                ObjetNetListStruct* NetItemRef,
                ObjetNetListStruct* StartNet )
/***********************************************************************/

/* Routine controlant qu'un sheetLabel est bien connecte a un Glabel de la
 *  sous-feuille correspondante
 */
{
    ObjetNetListStruct* NetItemTst, * Lim;
    int erc = 1;

    /* Analyse de la table des connexions : */
    Lim = g_TabObjNet + g_NbrObjNet;

    NetItemTst = StartNet;

    /* Examen de la liste des Labels connectees a NetItemRef */
    for( ; ; NetItemTst++ )
    {
        if( NetItemTst == NetItemRef )
            continue;

        /* Est - on toujours dans le meme net ? */
        if( ( NetItemTst ==  Lim )
           || ( NetItemRef->GetNet() != NetItemTst->GetNet() ) )
        {
            /* Fin de netcode trouve */
            if( erc )
            {
                /* GLabel ou SheetLabel orphelin */
                Diagnose( panel, NetItemRef, NULL, -1, WAR );
            }
            return;
        }
        if( IsLabelsConnected( NetItemRef, NetItemTst ) )
            erc = 0;

        //same thing, different order.
        if( IsLabelsConnected( NetItemTst, NetItemRef ) )
            erc = 0;
    }
}
