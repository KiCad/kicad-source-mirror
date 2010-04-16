/***********************/
/* PCBNEW: netlist.cpp */
/***********************/

/*
 *  Function to read a netlist. While reading a netlist:
 *  - Load new footprints
 *  - Initialize net info
 *  - Test for missing or extra footprints
 *  - Recalculate ratsnest
 *
 *  Important remark:
 *  When reading a netlist Pcbnew must identify existing footprints (link
 * between existing footprints an components in netlist)
 *  This identification can be from 2 fields:
 *      - The reference (U2, R5 ..): this is the normal mode
 *      - The Time Stamp (Signature Temporelle), useful after a full schematic
 * reannotation
 *          because references can be changed for the same schematic.
 *  So when reading a netlist this identification ReadPcbNetlist() has
 * selection of the way to identify footprints.
 *  If we want to fully reannotate a schematic this sequence must be used
 *   SAVE your board !!!
 *   Create and read the netlist (to ensure all info is correct, mainly
 * references and time stamp)
 *   Reannotate the schematic (references will be changed, but not time stamp)
 *   Recreate and read the new netlist using the Time Stamp identification
 * (that reinit the new references)
 */
#include "fctsys.h"
#include "common.h"
#include "class_drawpanel.h"
#include "confirm.h"
#include "kicad_string.h"
#include "gestfich.h"
#include "pcbnew.h"
#include "wxPcbStruct.h"
#include "richio.h"

#include "dialog_netlist.h"

#include "protos.h"

// constants used by ReadPcbNetlist():
#define TESTONLY   1
#define READMODULE 0


class MODULEtoLOAD
{
public:
    wxString      m_LibName;
    wxString      m_CmpName;
    wxString      m_TimeStampPath;
    MODULEtoLOAD* m_Next;

public: MODULEtoLOAD( const wxString& libname,
                      const wxString& cmpname,
                      const wxString& timestamp_path )
    {
        m_LibName = libname;
        m_CmpName = cmpname;
        m_TimeStampPath = timestamp_path;
        m_Next = NULL;
    }


    ~MODULEtoLOAD() { };

    MODULEtoLOAD* Next() const { return (MODULEtoLOAD*) m_Next; }
    void SetNext( MODULEtoLOAD* next ) { m_Next = next; }
};


static void    SortListModulesToLoadByLibname( int NbModules );
static int     BuildFootprintsListFromNetlistFile(
    const wxString& aNetlistFullFilename,
    wxArrayString&  aBufName );
static FILE *  OpenNetlistFile( const wxString& aFullFileName );
static void    AddToList( const wxString& NameLibCmp,
                          const wxString& NameCmp,
                          const wxString& TimeStampPath );
static int     SetPadNetName( wxWindow*   aFrame,
                              char*       Text,
                              MODULE*     Module,
                              wxTextCtrl* aMessageWindow );
static int     ReadListeModules( const wxString& CmpFullFileName,
                                 const wxString* RefCmp,
                                 const wxString* TimeStampPath,
                                 wxString&       NameModule );
static MODULE* ReadNetModule( WinEDA_PcbFrame* aFrame,
                              wxTextCtrl*      aMessageWindow,
                              const wxString&  CmpFullFileName,
                              char*            Text,
                              int*             UseFichCmp,
                              int              TstOnly,
                              bool             Select_By_Timestamp,
                              bool             aChangeFootprint );
static void LoadListeModules( WinEDA_PcbFrame* aPcbFrame );


static int           s_NbNewModules;
static MODULEtoLOAD* s_ModuleToLoad_List;

#define BUFFER_CHAR_SIZE 2048

/** function OpenNetlistFile
 *  used to open a netlist file
 */
FILE * OpenNetlistFile( const wxString& aFullFileName )
{
    if( aFullFileName.IsEmpty() )
        return FALSE;  /* No filename: exit */

    FILE * netfile = wxFopen( aFullFileName, wxT( "rt" ) );
    if( netfile == NULL )
    {
        wxString msg;
        msg.Printf( _( "Netlist file %s not found" ),
                    GetChars( aFullFileName ) );
        DisplayError( NULL, msg );
    }

    return netfile;
}


/** Function ReadPcbNetlist
 * Update footprints (load missing footprints and delete on request extra
 * footprints)
 * Update connectivity info ( Net Name list )
 * Update Reference, value and "TIME STAMP"
 * @param aNetlistFullFilename = netlist file name (*.net)
 * @param aCmpFullFileName = cmp/footprint list file name (*.cmp) if not found,
 * @return true if Ok
 * only the netlist will be used
 *
 *  the format of the netlist is something like:
 # EESchema Netlist Version 1.0 generee le  18/5/2005-12:30:22
 *  (
 *  ( 40C08647 $noname R20 4,7K {Lib=R}
 *  (    1 VCC )
 *  (    2 MODB_1 )
 *  )
 *  ( 40C0863F $noname R18 4,7_k {Lib=R}
 *  (    1 VCC )
 *  (    2 MODA_1 )
 *  )
 *  }
 * #End
 */
bool WinEDA_PcbFrame::ReadPcbNetlist(
                     const wxString&  aNetlistFullFilename,
                     const wxString&  aCmpFullFileName,
                     wxTextCtrl*      aMessageWindow,
                     bool             aChangeFootprint,
                     bool             aDeleteBadTracks,
                     bool             aDeleteExtraFootprints,
                     bool             aSelect_By_Timestamp )
{
    int     State, Comment;
    MODULE* Module = NULL;
    D_PAD*  PtPad;
    char*   Text;
    int     UseFichCmp = 1;

    FILE * netfile = OpenNetlistFile( aNetlistFullFilename );
    if( !netfile )
        return false;

    if( aMessageWindow )
    {
        wxString msg;
        msg.Printf( _( "Reading Netlist \"%s\"" ),
                    GetChars( aNetlistFullFilename ) );
        aMessageWindow->AppendText( msg + wxT( "\n" ) );
    }

    // Clear undo and redo lists to avoid inconsistencies between lists
    GetScreen()->ClearUndoRedoList();

    OnModify();
    GetBoard()->m_Status_Pcb = 0;
    State = 0; Comment = 0;
    s_NbNewModules = 0;

    wxBusyCursor dummy;        // Shows an hourglass while calculating

    FILE_LINE_READER netlistReader( netfile,  BUFFER_CHAR_SIZE );
    char* Line = netlistReader;
    /* First, read the netlist: Build the list of footprints to load (new
     * footprints)
     */
    while( netlistReader.ReadLine( ) )
    {
        Text = StrPurge( Line );

        if( Comment ) /* Comments in progress */
        {
            if( ( Text = strchr( Text, '}' ) ) == NULL )
                continue;
            Comment = 0;
        }
        if( *Text == '{' ) /* Start Comment */
        {
            Comment = 1;
            if( ( Text = strchr( Text, '}' ) ) == NULL )
                continue;
        }

        if( *Text == '(' )
            State++;

        if( *Text == ')' )
            State--;

        if( State == 2 )
        {
            Module = ReadNetModule( this,
                                    aMessageWindow,
                                    aCmpFullFileName,
                                    Text,
                                    &UseFichCmp,
                                    TESTONLY,
                                    aSelect_By_Timestamp,
                                    aChangeFootprint );
            continue;
        }

        if( State >= 3 ) /* Do not analyzed pad description here. */
        {
            State--;
        }
    }

    /* Load new footprints */
    if( s_NbNewModules )
    {
        LoadListeModules( this );

        // Free module list:
        MODULEtoLOAD* item, * next_item;
        for( item = s_ModuleToLoad_List; item != NULL; item = next_item )
        {
            next_item = item->Next();
            delete item;
        }

        s_ModuleToLoad_List = NULL;
    }

    /* Second read , All footprints are on board, one must update the schematic
     * info (pad netnames) */
    netlistReader.Rewind( );
    while( netlistReader.ReadLine( ) )
    {
        Text = StrPurge( Line );

        if( Comment )                                       /* we are reading a
                                                             * comment */
        {
            if( ( Text = strchr( Text, '}' ) ) == NULL )    /* this is the end
                                                             * of a comment */
                continue;
            Comment = 0;
        }
        if( *Text == '{' ) /* this is the beginning of a comment */
        {
            Comment = 1;
            if( ( Text = strchr( Text, '}' ) ) == NULL )
                continue;
        }

        if( *Text == '(' )
            State++;
        if( *Text == ')' )
            State--;

        if( State == 2 )
        {
            Module = ReadNetModule( this,
                                    aMessageWindow,
                                    aCmpFullFileName,
                                    Text,
                                    &UseFichCmp,
                                    READMODULE,
                                    aSelect_By_Timestamp,
                                    aChangeFootprint );
            if( Module == NULL ) // the module could not be created (perhaps
                                 // footprint not found in library)
            {
                continue;
            }
            else /* clear pads netnames */
            {
                PtPad = Module->m_Pads;
                for( ; PtPad != NULL; PtPad = (D_PAD*) PtPad->Next() )
                {
                    PtPad->SetNetname( wxEmptyString );
                }
            }
            continue;
        }

        if( State >= 3 )
        {
            if( Module )
            {
                SetPadNetName( NULL, Text, Module, aMessageWindow );
            }
            State--;
        }
    }

    fclose( netfile );

    // Delete footprints not found in netlist:
    if( aDeleteExtraFootprints )
    {
        wxArrayString ModuleListFromNetlist;
        /* Build list of modules in the netlist */
        int NbModulesNetListe =
            BuildFootprintsListFromNetlistFile( aNetlistFullFilename,
                                                ModuleListFromNetlist );
        if( NbModulesNetListe  )
        {
            MODULE* NextModule;
            Module = GetBoard()->m_Modules;
            bool    ask_for_confirmation = true;
            for( ; Module != NULL; Module = NextModule )
            {
                int ii;
                NextModule = Module->Next();
                if( Module->m_ModuleStatus & MODULE_is_LOCKED )
                    continue;
                for( ii = 0; ii < NbModulesNetListe; ii++ )
                {
                    if( Module->m_Reference->m_Text.CmpNoCase(
                            ModuleListFromNetlist[ii] ) == 0 )
                    {
                        break; /* Module is already in net list. */
                    }
                }

                if( ii == NbModulesNetListe )   /* Module not found in
                                                 * net list. */
                {
                    if( ask_for_confirmation )
                    {
                        ask_for_confirmation = false;
                        if( !IsOK( NULL,
                                   _( "Ok to delete footprints not in netlist?" ) ) )
                            break;
                    }
                    Module->DeleteStructure();
                }
            }
        }
    }

    /* Rebuild the connectivity */
    Compile_Ratsnest( NULL, true );

    if( GetBoard()->m_Track )
    {
        if( aDeleteBadTracks )    // Remove erroneous tracks
        {
            RemoveMisConnectedTracks( NULL, true );
            Compile_Ratsnest( NULL, true );
        }
    }

    GetBoard()->DisplayInfo( this );
    DrawPanel->Refresh();

    return true;
}


/* Load the description of a footprint, net list type Pcbnew
 * and update the corresponding module.
 *
 * If TstOnly == 0 if the module does not exist, it is responsible
 * If TstOnly! = 0 if the module does not exist, it is added to the list
 * Load modules has
 * Text contains the first line of description
 * UseFichCmp is a flag
 * If! = 0, file components. CMP will be used
 * Is reset to 0 if the file does not exist
 *
 * Analyze lines like:
 * ($ 40C08647 noname R20 4.7 K Lib = (R)
 * (1 VCC)
 * (2 MODB_1)
 */
MODULE* ReadNetModule( WinEDA_PcbFrame* aFrame,
                       wxTextCtrl*      aMessageWindow,
                       const wxString&  aCmpFullFileName,
                       char*            Text,
                       int*             UseFichCmp,
                       int              TstOnly,
                       bool             aSelect_By_Timestamp,
                       bool             aChangeFootprint )
{
    MODULE*  Module;
    char*    text;
    wxString TimeStampPath;
    wxString TextNameLibMod;
    wxString TextValeur;
    wxString TextCmpName;
    wxString NameLibCmp;
    int      Error = 0;
    char     Line[1024];
    bool     Found;

    strcpy( Line, Text );

    TextValeur = wxT( "~" );

    if( ( text = strtok( Line, " ()\t\n" ) ) == NULL )
        Error = 1;
    else
        TimeStampPath = CONV_FROM_UTF8( text );

    if( ( text = strtok( NULL, " ()\t\n" ) ) == NULL )
        Error = 1;
    else
        TextNameLibMod = CONV_FROM_UTF8( text );

    if( ( text = strtok( NULL, " ()\t\n" ) ) == NULL )
        Error = 1;
    else
        TextCmpName = CONV_FROM_UTF8( text );

    if( ( text = strtok( NULL, " ()\t\n" ) ) == NULL )
        Error = -1;
    else
        TextValeur = CONV_FROM_UTF8( text );

    if( Error > 0 )
        return NULL;

    /* Test if module is already loaded. */
    Module = aFrame->GetBoard()->m_Modules;
    MODULE* NextModule;

    for( Found = false; Module != NULL; Module = NextModule )
    {
        NextModule = Module->Next();
        if( aSelect_By_Timestamp ) /* identification by time stamp */
        {
            if( TimeStampPath.CmpNoCase( Module->m_Path ) == 0 )
                Found = true;
        }
        else    /* identification by Reference */
        {
            if( TextCmpName.CmpNoCase( Module->m_Reference->m_Text ) == 0 )
                Found = true;
        }
        if( Found ) // test footprint matching for existing modules:  current
                    // m_LibRef and module name in netlist must match
        {
            if( TstOnly != TESTONLY )
            {
                NameLibCmp = TextNameLibMod;    // Use footprint name from netlist
                if( *UseFichCmp )               // Try to get footprint name from .cmp file
                {
                    if( aSelect_By_Timestamp )
                    {
                        *UseFichCmp = ReadListeModules( aCmpFullFileName,
                                                        NULL,
                                                        &TimeStampPath,
                                                        NameLibCmp );
                    }
                    else
                    {
                        *UseFichCmp = ReadListeModules( aCmpFullFileName,
                                                        &TextCmpName,
                                                        NULL,
                                                        NameLibCmp );
                    }
                }

                /* Module mismatch: current module and module specified in
                 * net list are  different.
                 */
                if( Module->m_LibRef.CmpNoCase( NameLibCmp ) != 0 )
                {
                    if( aChangeFootprint )   // footprint exchange allowed.
                    {
                        MODULE* NewModule =
                            aFrame->Get_Librairie_Module( wxEmptyString,
                                                          NameLibCmp,
                                                          true );
                        if( NewModule )  /* Change old module to the new module
                                          * (and delete the old one) */
                        {
                            aFrame->Exchange_Module( Module, NewModule, NULL );
                            Module = NewModule;
                        }
                    }
                    else
                    {
                        wxString msg;
                        msg.Printf( _( "Component \"%s\": Mismatch! module \
is [%s] and netlist said [%s]\n" ),
                                    GetChars( TextCmpName ),
                                    GetChars( Module->m_LibRef ),
                                    GetChars( NameLibCmp ) );

                        if( aMessageWindow )
                            aMessageWindow->AppendText( msg );
                    }
                }
            }

            break;
        }
    }

    if( Module == NULL )    /* a new module must be loaded from libs */
    {
        NameLibCmp = TextNameLibMod;    // Use footprint name from netlist
        if( *UseFichCmp )               // Try to get footprint name from .cmp file
        {
            if( aSelect_By_Timestamp == 1 )
            {
                *UseFichCmp = ReadListeModules( aCmpFullFileName,
                                                NULL,
                                                &TimeStampPath,
                                                NameLibCmp );
            }
            else
            {
                *UseFichCmp = ReadListeModules( aCmpFullFileName,
                                                &TextCmpName,
                                                NULL,
                                                NameLibCmp );
            }
        }


        if( TstOnly == TESTONLY )
            AddToList( NameLibCmp, TextCmpName, TimeStampPath );
        else
        {
            if( aMessageWindow )
            {
                wxString msg;
                msg.Printf( _( "Component [%s] not found" ),
                            GetChars( TextCmpName ) );
                aMessageWindow->AppendText( msg + wxT( "\n" ) );
            }
        }
        return NULL;    /* The module could not be loaded. */
    }

    /* Fields update ( reference, value and "Time Stamp") */
    Module->m_Reference->m_Text = TextCmpName;
    Module->m_Value->m_Text     = TextValeur;
    Module->m_Path = TimeStampPath;

    return Module;
}


/** Function SetPadNetName
 *  Update a pad netname in a given footprint
 *  @param Text = Text from netlist (format: (pad = net) )
 *  @param Module = the given footprint
 *  @param aMessageWindow = a wxTextCtrl to print error and warning message
 * (can be NULL)
 */
int SetPadNetName( wxWindow*   frame,
                   char*       Text,
                   MODULE*     Module,
                   wxTextCtrl* aMessageWindow )
{
    D_PAD*   pad;
    char*    TextPinName, * TextNetName;
    int      Error = 0;
    bool     trouve;
    char     Line[256];
    wxString Msg;

    if( Module == NULL )
        return 0;

    strcpy( Line, Text );

    if( ( TextPinName = strtok( Line, " ()\t\n" ) ) == NULL )
        Error = 1;
    if( ( TextNetName = strtok( NULL, " ()\t\n" ) ) == NULL )
        Error = 1;
    if( Error )
        return 0;

    pad = Module->m_Pads; trouve = FALSE;
    for( ; pad != NULL; pad = (D_PAD*) pad->Next() )
    {
        if( strnicmp( TextPinName, pad->m_Padname, 4 ) == 0 )
        {
            trouve = true;
            if( *TextNetName != '?' )
                pad->SetNetname( CONV_FROM_UTF8( TextNetName ) );
            else
                pad->SetNetname( wxEmptyString );
        }
    }

    if( !trouve )
    {
        if( aMessageWindow )
        {
            wxString pin_name = CONV_FROM_UTF8( TextPinName );
            Msg.Printf( _( "Module [%s]: Pad [%s] not found" ),
                        GetChars( Module->m_Reference->m_Text ),
                        GetChars( pin_name ) );
            aMessageWindow->AppendText( Msg + wxT( "\n" ) );
        }
    }

    return trouve;
}


/**
 * build and shows a list of existing modules on board
 * The user can select a module from this list
 * @return a pointer to the selected module or NULL
 */
MODULE* WinEDA_PcbFrame::ListAndSelectModuleName( void )
{
    int     ii, jj;
    MODULE* Module;

    if( GetBoard()->m_Modules == NULL )
    {
        DisplayError( this, _( "No Modules" ) ); return 0;
    }

    WinEDAListBox listbox( this, _( "Components" ), NULL, wxEmptyString );
    Module = (MODULE*) GetBoard()->m_Modules;
    for( ; Module != NULL; Module = (MODULE*) Module->Next() )
    {
        listbox.Append( Module->m_Reference->m_Text );
    }

    ii = listbox.ShowModal();

    if( ii < 0 )
    {
        Module = NULL;
    }
    else /* Search for the selected footprint */
    {
        wxString ref = listbox.GetTextSelection();
        Module = (MODULE*) GetBoard()->m_Modules;
        for( jj = 0; Module != NULL; Module = (MODULE*) Module->Next(), jj++ )
        {
            if( Module->m_Reference->m_Text.Cmp( ref ) == 0 )
                break;
        }
    }

    return Module;
}


/** Function TestFor_Duplicate_Missing_And_Extra_Footprints
 * Build a list from the given board and netlist :
 *  1 - for duplicate footprints on board
 *  2 - for missing footprints
 *  3 - for footprints not in netlist
 * @param aFrame = current active frame
 * @param aNetlistFullFilename = the given netlist
 * @param aPcb = the given board
 */
void TestFor_Duplicate_Missing_And_Extra_Footprints( wxWindow*       aFrame,
                                                     const wxString& aNetlistFullFilename,
                                                     BOARD*          aPcb )
#define MAX_LEN_TXT 32
{
    int ii;
    MODULE*           Module, * pt_aux;
    int               NbModulesNetListe, nberr = 0;
    WinEDA_TextFrame* List;
    wxArrayString     ModuleListFromNetlist;

    if( aPcb->m_Modules == NULL )
    {
        DisplayInfoMessage( aFrame, _( "No modules" ), 10 );
        return;
    }

    /* Build the list of references of the net list modules. */

    NbModulesNetListe =
        BuildFootprintsListFromNetlistFile( aNetlistFullFilename,
                                            ModuleListFromNetlist );
    if( NbModulesNetListe < 0 )
        return;  /* File not found */

    if( NbModulesNetListe == 0 )
    {
        DisplayError( aFrame, _( "No modules in NetList" ), 10 );
        return;
    }

    List = new WinEDA_TextFrame( aFrame, _( "Check Modules" ) );

    /* Search for duplicate footprints. */
    List->Append( _( "Duplicates" ) );

    Module = aPcb->m_Modules;

    for( ; Module != NULL; Module = Module->Next() )
    {
        pt_aux = Module->Next();
        for( ; pt_aux != NULL; pt_aux = pt_aux->Next() )
        {
            if( Module->m_Reference->m_Text.CmpNoCase( pt_aux->m_Reference->
                                                       m_Text ) == 0 )
            {
                List->Append( Module->m_Reference->m_Text );
                nberr++;
                break;
            }
        }
    }

    /* Search for the missing module by the net list. */
    List->Append( _( "Lack:" ) );

    for( ii = 0; ii < NbModulesNetListe; ii++ )
    {
        Module = (MODULE*) aPcb->m_Modules;
        for( ; Module != NULL; Module = Module->Next() )
        {
            if( Module->m_Reference->m_Text.CmpNoCase(
                    ModuleListFromNetlist[ii] ) == 0 )
            {
                break;
            }
        }

        if( Module == NULL )    // Module missing, not found in board
        {
            List->Append( ModuleListFromNetlist[ii] );
            nberr++;
        }
    }

    /* Search for modules not in net list. */
    List->Append( _( "Not in Netlist:" ) );

    Module = (MODULE*) aPcb->m_Modules;
    for( ; Module != NULL; Module = Module->Next() )
    {
        for( ii = 0; ii < NbModulesNetListe; ii++ )
        {
            if( Module->m_Reference->m_Text.CmpNoCase(
                    ModuleListFromNetlist[ii] ) == 0 )
            {
                break; /* Module is in net list. */
            }
        }

        if( ii == NbModulesNetListe )   /* Module not found in netlist */
        {
            List->Append( Module->m_Reference->m_Text );
            nberr++;
        }
    }

    List->ShowModal(); List->Destroy();
}


/** Function BuildFootprintsListFromNetlistFile
 *  Fill BufName with footprints names read from the netlist.
 * @param aNetlistFullFilename = netlist file name
 * @param BufName = wxArrayString to fill with footprint names
 * @return Footprint count, or -1 if netlist file cannot opened
 */
int BuildFootprintsListFromNetlistFile( const wxString& aNetlistFullFilename,
                                        wxArrayString&  aBufName )
{
    int  nb_modules_lus;
    int  State, Comment;
    char * Text, * LibModName;

    FILE * netfile = OpenNetlistFile( aNetlistFullFilename );
    if( !netfile )
        return -1;

    FILE_LINE_READER netlistReader( netfile,  BUFFER_CHAR_SIZE );
    char* Line = netlistReader;

    State = 0; Comment = 0;
    nb_modules_lus = 0;

    while( netlistReader.ReadLine( ) )
    {
        Text = StrPurge( Line );
        if( Comment )
        {
            if( ( Text = strchr( Text, '}' ) ) == NULL )
                continue;
            Comment = 0;
        }
        if( *Text == '{' ) /* Comments. */
        {
            Comment = 1;
            if( ( Text = strchr( Text, '}' ) ) == NULL )
                continue;
        }

        if( *Text == '(' )
            State++;
        if( *Text == ')' )
            State--;

        if( State == 2 )
        {
            int Error = 0;
            if( strtok( Line, " ()\t\n" ) == NULL )
                Error = 1;  /* TimeStamp */
            if( ( LibModName = strtok( NULL, " ()\t\n" ) ) == NULL )
                Error = 1;
            /* Load the name of the component. */
            if( ( Text = strtok( NULL, " ()\t\n" ) ) == NULL )
                Error = 1;
            nb_modules_lus++;
            aBufName.Add( CONV_FROM_UTF8( Text ) );
            continue;
        }

        if( State >= 3 )
        {
            State--;
        }
    }

    fclose( netfile );

    return nb_modules_lus;
}


/*
 * Get the file CMP giving the equivalence modules / components
 * Returns:
 * If this file exists returns:
 * 1 and the module name in NameModule
 * -1 If not found in module file
 * Otherwise 0;
 *
 * Call settings:
 * RefCmp (NULL if selection by TimeStamp)
 * TimeStamp (time signature if it exists, NULL otherwise)
 * Pointer to the buffer receiving the name of the module
 *
 * Example file:
 *
 * Cmp-Mod V01 Genere by Pcbnew 29/10/2003-13: 11:6 *
 *  BeginCmp
 *  TimeStamp = /322D3011;
 *  Reference = BUS1;
 *  ValeurCmp = BUSPC;
 *  IdModule  = BUS_PC;
 *  EndCmp
 *
 *  BeginCmp
 *  TimeStamp = /32307DE2/AA450F67;
 *  Reference = C1;
 *  ValeurCmp = 47uF;
 *  IdModule  = CP6;
 *  EndCmp
 *
 */
int ReadListeModules( const wxString& CmpFullFileName, const wxString* RefCmp,
                      const wxString* TimeStamp, wxString& NameModule )
{
    wxString refcurrcmp, timestamp, idmod;
    char*    ptcar;
    FILE*    FichCmp;

    if( (RefCmp == NULL) && (TimeStamp == 0) )
        return 0;

    FichCmp = wxFopen( CmpFullFileName, wxT( "rt" ) );
    if( FichCmp == NULL )
    {
        wxString msg;
        msg.Printf( _( "File <%s> not found, use Netlist for lib module selection" ),
                    GetChars( CmpFullFileName ) );
        DisplayError( NULL, msg, 20 );
        return 0;
    }

    FILE_LINE_READER netlistReader( FichCmp,  BUFFER_CHAR_SIZE );
    char* Line = netlistReader;

    while( netlistReader.ReadLine() )
    {
        if( strnicmp( Line, "BeginCmp", 8 ) != 0 )
            continue;

        /* Begin component description. */
        refcurrcmp.Empty();
        idmod.Empty();
        timestamp.Empty();
        while( netlistReader.ReadLine() )
        {
            if( strnicmp( Line, "EndCmp", 6 ) == 0 )
                break;

            if( strnicmp( Line, "Reference =", 11 ) == 0 )
            {
                ptcar = Line + 11;
                ptcar = strtok( ptcar, " =;\t\n" );
                if( ptcar )
                    refcurrcmp = CONV_FROM_UTF8( ptcar );
                continue;
            }

            if( strnicmp( Line, "IdModule  =", 11 ) == 0 )
            {
                ptcar = Line + 11;
                ptcar = strtok( ptcar, " =;\t\n" );
                if( ptcar )
                    idmod = CONV_FROM_UTF8( ptcar );
                continue;
            }
            if( strnicmp( Line, "TimeStamp =", 11 ) == 0 )
            {
                ptcar = Line + 11;
                ptcar = strtok( ptcar, " =;\t\n" );
                if( ptcar )
                    timestamp = CONV_FROM_UTF8( ptcar );
            }
        }

        /* Check if component read is valid. */
        if( RefCmp )
        {
            if( RefCmp->CmpNoCase( refcurrcmp ) == 0 )  // Found!
            {
                fclose( FichCmp );
                NameModule = idmod;
                return 1;
            }
        }
        else if( TimeStamp )
        {
            if( TimeStamp->CmpNoCase( timestamp ) == 0
                && !timestamp.IsEmpty() )                // Found
            {
                fclose( FichCmp );
                NameModule = idmod;
                return 1;
            }
        }
    }

    fclose( FichCmp );
    return -1;
}


/* This function add to the current list of footprints found in netlist
 *  a new MODULEtoLOAD item (a descriptor of footprints)
 */
void AddToList( const wxString& NameLibCmp, const wxString& CmpName, const wxString& path )
{
    MODULEtoLOAD* NewMod;

    NewMod = new MODULEtoLOAD( NameLibCmp, CmpName, path );
    NewMod->SetNext( s_ModuleToLoad_List );
    s_ModuleToLoad_List = NewMod;
    s_NbNewModules++;
}


/* Load new modules from library.
 * If a module is being loaded it is duplicated, which avoids reading
 * unnecessary library.
 */
void LoadListeModules( WinEDA_PcbFrame* aPcbFrame )
{
    MODULEtoLOAD* ref, * cmp;
    int           ii;
    MODULE*       Module = NULL;
    wxPoint       ModuleBestPosition;

    if( s_NbNewModules == 0 )
        return;

    SortListModulesToLoadByLibname( s_NbNewModules );
    ref = cmp = s_ModuleToLoad_List;

    // Calculate the footprint "best" position:
    if( aPcbFrame->SetBoardBoundaryBoxFromEdgesOnly() )
    {
        ModuleBestPosition.x =
            aPcbFrame->GetBoard()->m_BoundaryBox.GetRight() + 5000;
        ModuleBestPosition.y =
            aPcbFrame->GetBoard()->m_BoundaryBox.GetBottom() + 10000;
    }
    else
        ModuleBestPosition = wxPoint( 0, 0 );

    for( ii = 0; ii < s_NbNewModules; ii++, cmp = cmp->Next() )
    {
        if( (ii == 0) || ( ref->m_LibName != cmp->m_LibName) )
        {
            /* New footprint : must be loaded from a library */
            Module = aPcbFrame->Get_Librairie_Module( wxEmptyString,
                                                      cmp->m_LibName,
                                                      FALSE );
            ref    = cmp;
            if( Module == NULL )
            {
                wxString msg;
                msg.Printf( _( "Component [%s]: footprint <%s> not found" ),
                            GetChars( cmp->m_CmpName ),
                            GetChars( cmp->m_LibName ) );
                DisplayError( NULL, msg );
                continue;
            }
            Module->SetPosition( ModuleBestPosition );

            /* Update schematic links : reference "Time Stamp" and schematic
             *hierarchical path */
            Module->m_Reference->m_Text = cmp->m_CmpName;
            Module->m_TimeStamp = GetTimeStamp();
            Module->m_Path = cmp->m_TimeStampPath;
        }
        else
        {
            /* Footprint already loaded from a library, duplicate it (faster)
             */
            MODULE* newmodule;
            if( Module == NULL )
                continue;            /* Module does not exist in library. */

            newmodule = new MODULE( aPcbFrame->GetBoard() );
            newmodule->Copy( Module );

            aPcbFrame->GetBoard()->Add( newmodule, ADD_APPEND );

            Module = newmodule;
            Module->m_Reference->m_Text = cmp->m_CmpName;
            Module->m_TimeStamp = GetTimeStamp();
            Module->m_Path = cmp->m_TimeStampPath;
        }
    }
}


/* Routines used by qsort to sort a load module. */
static int SortByLibName( MODULEtoLOAD** ref, MODULEtoLOAD** cmp )
{
    int ii = (*ref)->m_LibName.CmpNoCase( (*cmp)->m_LibName );

    return ii;
}


/* Sort the module list in alphabetical order by module name.
 */
void SortListModulesToLoadByLibname( int NbModules )
{
    MODULEtoLOAD** base_list, * item;
    int            ii;

    base_list = (MODULEtoLOAD**) MyMalloc( NbModules * sizeof(MODULEtoLOAD*) );

    for( ii = 0, item = s_ModuleToLoad_List; ii < NbModules; ii++ )
    {
        base_list[ii] = item;
        item = item->Next();
    }

    qsort( base_list, NbModules, sizeof(MODULEtoLOAD*),
           ( int ( * )( const void*, const void* ) )SortByLibName );

    s_ModuleToLoad_List = *base_list;

    for( ii = 0; ii < NbModules - 1; ii++ )
    {
        item = base_list[ii];
        item->SetNext( base_list[ii + 1] );
    }

    item = base_list[ii];
    item->SetNext( NULL );

    free( base_list );
}
