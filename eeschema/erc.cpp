/**************************************************/
/* Module de tst "ERC" ( Electrical Rules Check ) */
/**************************************************/

#include "fctsys.h"
#include "gr_basic.h"
#include "common.h"
#include "class_drawpanel.h"
#include "confirm.h"
#include "kicad_string.h"
#include "gestfich.h"
#include "appl_wxstruct.h"

#include "program.h"
#include "libcmp.h"
#include "general.h"
#include "netlist.h"
#include "protos.h"
#include "bitmaps.h"

#include "dialog_erc.h"

/* On teste
 *  1 - conflits entre pins connectees ( ex: 2 sorties connectees )
 *  2 - les imperatifs minimaux ( 1 entree doit etre connectee a une sortie )
 */


/* fonctions exportees */

/* fonctions importees */

/* fonctions locales */
static bool WriteDiagnosticERC( const wxString& FullFileName );
static void Diagnose( WinEDA_DrawPanel* panel,
                      ObjetNetListStruct* NetItemRef,
                      ObjetNetListStruct* NetItemTst, int MinConnexion, int Diag );
static void TestOthersItems( WinEDA_DrawPanel* panel,
                             ObjetNetListStruct* NetItemRef,
                             ObjetNetListStruct* NetStart,
                             int* NetNbItems, int* MinConnexion );
static void TestLabel( WinEDA_DrawPanel* panel,
                       ObjetNetListStruct* NetItemRef,
                       ObjetNetListStruct* StartNet );

/* Variable locales */
int WriteFichierERC = FALSE;

/* Tableau des types de conflit :
 *  PIN_INPUT, PIN_OUTPUT, PIN_BIDI, PIN_TRISTATE, PIN_PASSIVE,
 *  PIN_UNSPECIFIED, PIN_POWER_IN, PIN_POWER_OUT, PIN_OPENCOLLECTOR,
 *  PIN_OPENEMITTER, PIN_NC
 */

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
int DiagErc[PIN_NMAX][PIN_NMAX];
bool       DiagErcTableInit; // go to TRUE after DiagErc init

/* Default Look up table which gives the diag for a pair of connected pins
 *  Same as DiagErc, but cannot be modified
 *  Used to init or reset DiagErc
 */
int DefaultDiagErc[PIN_NMAX][PIN_NMAX] =
{   /*       I,   O,   Bi,  3S, Pas, UnS,PwrI,PwrO,  OC,  OE,  NC */
/* I */  { OK,  OK,   OK,   OK,   OK,   WAR,  OK,   OK,   OK,   OK,   WAR    },
/* O */  { OK,  ERR,  OK,   WAR,  OK,   WAR,  OK,   ERR,  ERR,  ERR,  WAR    },
/* Bi*/  { OK,  OK,   OK,   OK,   OK,   WAR,  OK,   WAR,  OK,   WAR,  WAR    },
/* 3S*/  { OK,  WAR,  OK,   OK,   OK,   WAR,  WAR,  ERR,  WAR,  WAR,  WAR    },
/*Pas*/  { OK,  OK,   OK,   OK,   OK,   WAR,  OK,   OK,   OK,   OK,   WAR    },
/*UnS */ { WAR, WAR,  WAR,  WAR,  WAR,  WAR,  WAR,  WAR,  WAR,  WAR,  WAR    },
/*PwrI*/ { OK,  OK,   OK,   WAR,  OK,   WAR,  OK,   OK,   OK,   OK,   ERR    },
/*PwrO*/ { OK,  ERR,  WAR,  ERR,  OK,   WAR,  OK,   ERR,  ERR,  ERR,  WAR    },
/* OC */ { OK,  ERR,  OK,   WAR,  OK,   WAR,  OK,   ERR,  OK,   OK,   WAR    },
/* OE */ { OK,  ERR,  WAR,  WAR,  OK,   WAR,  OK,   ERR,  OK,   OK,   WAR    },
/* NC */ { WAR, WAR,  WAR,  WAR,  WAR,  WAR,  WAR,  WAR,  WAR,  WAR,  WAR    }
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
{         /* In, Out,  Bi,  3S, Pas, UnS,PwrI,PwrO,  OC,  OE,  NC */
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


/**************************************************/
void DIALOG_ERC::TestErc( wxTextCtrl* aMessagesList )
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
    if( CheckAnnotate( m_Parent, 0 ) )
    {
        if ( aMessagesList )
        {
            aMessagesList->AppendText( _( "Annotation Required!" )  );
            aMessagesList->AppendText( wxT("\n") );
        }
        return;
    }

    /* Effacement des anciens marqueurs DRC */
    DeleteAllMarkers( MARQ_ERC );

    g_EESchemaVar.NbErrorErc   = 0;
    g_EESchemaVar.NbWarningErc = 0;

    /* Cleanup the entire hierarchy */
    EDA_ScreenList ScreenList;

    for( SCH_SCREEN* Screen = ScreenList.GetFirst(); Screen != NULL; Screen = ScreenList.GetNext() )
    {
        bool ModifyWires;
        ModifyWires = Screen->SchematicCleanUp( NULL );

        /* if wire list has changed, delete Udo Redo list to avoid
         *  pointers on deleted data problems */
        if( ModifyWires )
            Screen->ClearUndoRedoList();
    }

    /* Test duplicate sheet names
     * inside a given sheet, one cannot have sheets with duplicate names (file names can be duplicated).
     * Test screens is enought
     */
    for( SCH_SCREEN* Screen = ScreenList.GetFirst(); Screen != NULL; Screen = ScreenList.GetNext() )
    {
        for( SCH_ITEM* ref_item = Screen->EEDrawList; ref_item != NULL; ref_item = ref_item->Next() )
        {
            // serach for a scheet;
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
                    DrawMarkerStruct* Marker =
                        new DrawMarkerStruct( ( (DrawSheetStruct*) item_to_test )->m_Pos,
                                             _( "Duplicate Sheet name" ) );

                    Marker->m_Type = MARQ_ERC;
                    Marker->m_MarkFlags = ERR;
                    Marker->SetNext( Screen->EEDrawList );
                    Screen->EEDrawList = Marker;
                    g_EESchemaVar.NbErrorErc++;
                    g_EESchemaVar.NbWarningErc++;
                }
            }
        }
    }

    m_Parent->BuildNetListBase();

    /* Analyse de la table des connexions : */
    Lim = g_TabObjNet + g_NbrObjNet;

    /* Reset du flag m_FlagOfConnection, utilise par la suite */
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
        case NET_GLOBLABEL: //not sure how to handle global labels -- they should be treated like other nets (just global!0
        case NET_GLOBBUSLABELMEMBER:
            break;

        case NET_HIERLABEL:
        case NET_HIERBUSLABELMEMBER:
        case NET_SHEETLABEL:
        case NET_SHEETBUSLABELMEMBER:
            TestLabel( m_Parent->DrawPanel, NetItemRef, StartNet );
            break;

        case NET_NOCONNECT:
            MinConn = NET_NC;
            if( NetNbItems != 0 )
                Diagnose( m_Parent->DrawPanel,NetItemRef, NULL, MinConn, UNC );
            break;

        case NET_PIN:
            TestOthersItems( m_Parent->DrawPanel,
                             NetItemRef, StartNet, &NetNbItems, &MinConn );
            break;
        }

        OldItem = NetItemRef;
    }

    FreeTabNetList( g_TabObjNet, g_NbrObjNet );

    wxString num;
    num.Printf( wxT( "%d" ), g_EESchemaVar.NbErrorErc );
    m_TotalErrCount->SetLabel( num );

    num.Printf( wxT( "%d" ), g_EESchemaVar.NbErrorErc - g_EESchemaVar.NbWarningErc );
    m_LastErrCount->SetLabel( num );

    num.Printf( wxT( "%d" ), g_EESchemaVar.NbWarningErc );
    m_LastWarningCount->SetLabel( num );

    // Display diags:
    EDA_SheetList SheetList;
    for( DrawSheetPath * Sheet = SheetList.GetFirst(); Sheet != NULL; Sheet = SheetList.GetNext() )
    {
        SCH_ITEM * DrawStruct = Sheet->LastDrawList();
        for( ; DrawStruct != NULL; DrawStruct = DrawStruct->Next() )
        {
            if( DrawStruct->Type() != DRAW_MARKER_STRUCT_TYPE )
                continue;

            /* Marqueur trouve */
            DrawMarkerStruct* Marker = (DrawMarkerStruct*) DrawStruct;
            if( Marker->m_Type != MARQ_ERC )
                continue;

            /* Display diag */
            wxString msg;
            msg.Printf(_("sheet %s: %s\n"),
                    Sheet->PathHumanReadable().GetData(),
                    Marker->GetComment().GetData() );
            m_MessagesList->AppendText( msg );
        }
    }


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



/**************************************************************/
void DIALOG_ERC::ResetDefaultERCDiag( wxCommandEvent& event )
/**************************************************************/

/* Remet aux valeurs par defaut la matrice de diagnostic
 */
{
    memcpy( DiagErc, DefaultDiagErc, sizeof(DiagErc) );
    ReBuildMatrixPanel();
}


/************************************************************/
void DIALOG_ERC::ChangeErrorLevel( wxCommandEvent& event )
/************************************************************/

/* Change the error level for the pressed button, on the matrix table
 */
{
    int             id, level, ii, x, y;
    wxBitmapButton* Butt;
    const char**    new_bitmap_xpm = NULL;
    wxPoint         pos;

    id   = event.GetId();
    ii   = id - ID_MATRIX_0;
    Butt = (wxBitmapButton*) event.GetEventObject();
    pos  = Butt->GetPosition();

    x = ii / PIN_NMAX; y = ii % PIN_NMAX;

    level = DiagErc[y][x];

    switch( level )
    {
    case OK:
        level = WAR;
        new_bitmap_xpm = warning_xpm;
        break;

    case WAR:
        level = ERR;
        new_bitmap_xpm = error_xpm;
        break;

    case ERR:
        level = OK;
        new_bitmap_xpm = erc_green_xpm;
        break;
    }

    if( new_bitmap_xpm )
    {
        delete Butt;
        Butt = new wxBitmapButton( m_PanelERCOptions, id,
                                   wxBitmap( new_bitmap_xpm ), pos );

        m_ButtonList[y][x] = Butt;
        DiagErc[y][x] = DiagErc[x][y] = level;
    }
}


/********************************************************/
static void Diagnose( WinEDA_DrawPanel* panel,
                      ObjetNetListStruct* NetItemRef,
                      ObjetNetListStruct* NetItemTst,
                      int MinConn, int Diag )
/********************************************************/

/* Creates an ERC marker to show the ERC problem between NetItemRef and NetItemTst
 *  if MinConn < 0: this is an error on labels
 */
{
    DrawMarkerStruct* Marker = NULL;
    wxString          DiagLevel;
    SCH_SCREEN*       screen;
    int ii, jj;

    if( Diag == OK )
        return;

    /* Creation du nouveau marqueur type Erreur ERC */
    Marker = new DrawMarkerStruct( NetItemRef->m_Start, wxEmptyString );

    Marker->m_Type = MARQ_ERC;
    Marker->m_MarkFlags = WAR;
    screen = NetItemRef->m_SheetList.LastScreen();
    Marker->SetNext( screen->EEDrawList );
    screen->EEDrawList = Marker;
    g_EESchemaVar.NbErrorErc++;
    g_EESchemaVar.NbWarningErc++;

    if( MinConn < 0 )   // Traitement des erreurs sur labels
    {
        if( (NetItemRef->m_Type == NET_HIERLABEL)
           || (NetItemRef->m_Type == NET_HIERBUSLABELMEMBER) )
        {
            Marker->m_Comment.Printf( _( "Warning HLabel %s not connected to SheetLabel" ),
                                     NetItemRef->m_Label->GetData() );
        }
        else
            Marker->m_Comment.Printf( _( "Warning SheetLabel %s not connected to HLabel" ),
                                     NetItemRef->m_Label->GetData() );
        return;
    }

    ii = NetItemRef->m_ElectricalType;

    wxString string_pinnum, cmp_ref;
    char ascii_buf[5];
    ascii_buf[4] = 0;
    memcpy( ascii_buf, &NetItemRef->m_PinNum, 4 );
    string_pinnum = CONV_FROM_UTF8( ascii_buf );
    cmp_ref = wxT("?");

    if( NetItemTst == NULL )
    {
        if( MinConn == NOC )    /* 1 seul element dans le net */
        {
            if ( NetItemRef->m_Type == NET_PIN && NetItemRef->m_Link )
                cmp_ref = ((SCH_COMPONENT*)NetItemRef->m_Link)->GetRef( &NetItemRef->m_SheetList );
            Marker->m_Comment.Printf( _( "Warning Cmp %s, Pin %s (%s) Unconnected" ),
                cmp_ref.GetData(), string_pinnum.GetData(), MsgPinElectricType[ii] );
            return;
        }

        if( MinConn == NOD )    /* pas de pilotage du net */
        {
           if ( NetItemRef->m_Type == NET_PIN && NetItemRef->m_Link )
                cmp_ref = ((SCH_COMPONENT*)NetItemRef->m_Link)->GetRef( &NetItemRef->m_SheetList );
            Marker->m_Comment.Printf(
                _( "Warning Cmp %s, Pin %s (%s) not driven (Net %d)" ),
                cmp_ref.GetData(), string_pinnum.GetData(),
                MsgPinElectricType[ii], NetItemRef->GetNet() );
            return;
        }

        if( Diag == UNC )
        {
            Marker->m_Comment.Printf(
                _( "Warning More than 1 Pin connected to UnConnect symbol" ) );
            return;
        }
    }

    if( NetItemTst )         /* Erreur entre 2 pins */
    {
        jj = NetItemTst->m_ElectricalType;
        DiagLevel = _( "Warning" );
        if( Diag == ERR )
        {
            DiagLevel = _( "Error" );
            Marker->m_MarkFlags = ERR;
            g_EESchemaVar.NbWarningErc--;
        }

        wxString alt_string_pinnum, alt_cmp;
        memcpy( ascii_buf, &NetItemTst->m_PinNum, 4 );
        alt_string_pinnum = CONV_FROM_UTF8( ascii_buf );
        alt_cmp = wxT("?");
        if ( NetItemTst->m_Type == NET_PIN && NetItemTst->m_Link )
            alt_cmp = ((SCH_COMPONENT*)NetItemTst->m_Link)->GetRef( &NetItemTst->m_SheetList );
        Marker->m_Comment.Printf( _("%s: Cmp %s, Pin %s (%s) connected to Cmp %s, Pin %s (%s) (net %d)" ),
                                 DiagLevel.GetData(),
                                 cmp_ref.GetData(), string_pinnum.GetData(), MsgPinElectricType[ii],
                                 alt_cmp.GetData(), alt_string_pinnum.GetData(),MsgPinElectricType[jj], NetItemRef->GetNet() );
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

    /* Examen de la liste des Pins connectees a NetItemRef */
    for( ; ; NetItemTst++ )
    {
        if( NetItemRef == NetItemTst )
            continue;

        /* Est - on toujours dans le meme net ? */
        if( (NetItemTst >= Lim)                                     // fin de liste (donc fin de net)
           || ( NetItemRef->GetNet() != NetItemTst->GetNet() ) )    // fin de net
        {                                                           /* Fin de netcode trouve: Tst connexion minimum */
            if( (*MinConnexion < NET_NC )
               && (local_minconn < NET_NC ) )                       /* Not connected or not driven pin */
            {
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
    EDA_BaseStruct*   DrawStruct;
    DrawMarkerStruct* Marker;
    char Line[256];
    static FILE*      OutErc;
    DrawSheetPath*    Sheet;
    wxString          msg;

    if( ( OutErc = wxFopen( FullFileName, wxT( "wt" ) ) ) == NULL )
        return FALSE;

    DateAndTime( Line );
    msg = _( "ERC control" );

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

            /* Marqueur trouve */
            Marker = (DrawMarkerStruct*) DrawStruct;
            if( Marker->m_Type != MARQ_ERC )
                continue;

            /* Write diag marqueur */
            msg.Printf( _( "ERC: %s (X= %2.3f inches, Y= %2.3f inches\n" ),
                        Marker->GetComment().GetData(),
                        (float) Marker->m_Pos.x / 1000,
                        (float) Marker->m_Pos.y / 1000 );

            fprintf( OutErc, "%s", CONV_TO_UTF8( msg ) );
        }
    }

    msg.Printf( _( "\n >> Errors ERC: %d\n" ), g_EESchemaVar.NbErrorErc );
    fprintf( OutErc, "%s", CONV_TO_UTF8( msg ) );
    fclose( OutErc );

    return TRUE;
}


bool TestLabel_( ObjetNetListStruct* a, ObjetNetListStruct* b )
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
void TestLabel( WinEDA_DrawPanel* panel, ObjetNetListStruct* NetItemRef, ObjetNetListStruct* StartNet )
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
        if( TestLabel_( NetItemRef, NetItemTst ) )
            erc = 0;

        //same thing, different order.
        if( TestLabel_( NetItemTst, NetItemRef ) )
            erc = 0;
    }
}
