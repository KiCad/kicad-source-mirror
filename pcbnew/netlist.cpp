/**
 * @file pcbnew/netlist.cpp
 */

/*
 *  Functions to read a netlist. They are:
 *  - Load new footprints and initialize net info
 *  - Test for missing or extra footprints
 *  - Recalculate full connectivity info
 *
 *  Important remark:
 *  When reading a netlist Pcbnew must identify existing footprints (link
 * between existing footprints an components in netlist)
 *  This identification can be from 2 fields:
 *      - The reference (U2, R5 ..): this is the normal mode
 *      - The Time Stamp (Signature Temporarily), useful after a full schematic
 * reannotation because references can be changed for the same schematic.
 * So when reading a netlist ReadPcbNetlist() can use references or time stamps
 * to identify footprints on board and the corresponding component in schematic.
 *  If we want to fully reannotate a schematic this sequence must be used
 *   1 - SAVE your board !!!
 *   2 - Create and read the netlist (to ensure all info is correct, mainly
 * references and time stamp)
 *   3 - Reannotate the schematic (references will be changed, but not time stamps )
 *   4 - Recreate and read the new netlist using the Time Stamp identification
 * (that reinit the new references)
 */

#include "algorithm"

#include "fctsys.h"
#include "class_drawpanel.h"
#include "confirm.h"
#include "kicad_string.h"
#include "gestfich.h"
#include "wxPcbStruct.h"
#include "richio.h"
#include "dialog_helpers.h"
#include "macros.h"

#include "class_board.h"
#include "class_module.h"

#include "pcbnew.h"
#include "dialog_netlist.h"


// constants used by ReadNetlistModuleDescr():
#define TESTONLY   true
#define READMODULE false


/*
 * Helper class, to store new footprints info found in netlist.
 * New footprints are footprints relative to new components found in netlist
 */
class MODULE_INFO
{
public:
    wxString m_LibName;
    wxString m_CmpName;
    wxString m_TimeStampPath;

public: MODULE_INFO( const wxString& libname,
                     const wxString& cmpname,
                     const wxString& timestamp_path )
    {
        m_LibName = libname;
        m_CmpName = cmpname;
        m_TimeStampPath = timestamp_path;
    }

    ~MODULE_INFO() { };
};


/*
 * Helper class, to read a netlist.
 */
class NETLIST_READER
{
private:
    PCB_EDIT_FRAME*            m_pcbframe;          // the main Pcbnew frame
    wxTextCtrl*                m_messageWindow;     // a textctrl to show messages (can be NULL)
    wxString                   m_netlistFullName;   // The full netlist filename
    wxString                   m_cmplistFullName;   // The full component/footprint association filename
    MODULE*                    m_currModule;        // The footprint currently being read in netlist
    std::vector <MODULE_INFO*> m_newModulesList;    // The list of new footprints,
                                                    // found in netlist, but not on board
                                                    // (must be loaded from libraries)
    bool m_useCmpFile;                              // true to use .cmp files as component/footprint file link
                                                    // false to use netlist only to know component/footprint link

public:
    bool m_UseTimeStamp;            // Set to true to identify footprints by time stamp
                                    // false to use schematic reference
    bool m_ChangeFootprints;        // Set to true to change existing footprints to new ones
                                    // when netlist gives a different footprint name

public:
    NETLIST_READER( PCB_EDIT_FRAME* aFrame, wxTextCtrl* aMessageWindow = NULL )
    {
        m_pcbframe = aFrame;
        m_messageWindow    = aMessageWindow;
        m_UseTimeStamp     = false;
        m_ChangeFootprints = false;
        m_useCmpFile = true;
    }

    ~NETLIST_READER()
    {
        // Free new modules list:
        for( unsigned ii = 0; ii < m_newModulesList.size(); ii++ )
            delete m_newModulesList[ii];

        m_newModulesList.clear();
    }

    /**
     * Function ReadNetList
     * The main function to read a netlist, and update the board
     * @param aFile = the already opened file (will be closed by ReadNetList)
     * @param aNetlistFileName = the netlist full file name (*.net file)
     * @param aCmplistFileName = the auxiliary component full file name (*.cmp file)
     * If the aCmplistFileName file is not given or not found,
     * the netlist is used to know the component/footprint link.
     */
    bool ReadNetList( FILE* aFile, const wxString& aNetlistFileName,
                      const wxString& aCmplistFileName );

    /**
     * Function BuildComponentsListFromNetlist
     * Fill aBufName with component references read from the netlist.
     * @param aNetlistFilename = netlist full file name
     * @param aBufName = wxArrayString to fill with component references
     * @return the component count, or -1 if netlist file cannot opened
     */
    int  BuildComponentsListFromNetlist( const wxString& aNetlistFilename,
                                         wxArrayString&  aBufName );

    /**
     * function RemoveExtraFootprints
     * Remove (delete) not locked footprints found on board, but not in netlist
     * @param aNetlistFileName = the netlist full file name (*.net file)
     */
    void    RemoveExtraFootprints( const wxString& aNetlistFileName );

private:

    /**
     * Function SetPadNetName
     *  Update a pad netname using the current footprint
     *  from the netlist (line format: ( \<pad number\> \<net name\> ) )
     *  @param aText = current line read from netlist
     */
    bool    SetPadNetName( char* aText );

    /**
     * Function ReadNetlistModuleDescr
     * Read the full description of a footprint, from the netlist
     * and update the corresponding module.
     * @param aTstOnly bool to switch between 2 modes:
     *      aTstOnly = false:
     *          if the module does not exist, it is added to m_newModulesList
     *      aTstOnly = true:
     *          if the module does not exist, it is loaded and added to the board module list
     * @param  aText contains the first line of description
     * This function uses m_useFichCmp as a flag to know the footprint name:
     *      If true: component file *.cmp is used
     *      If false: the netlist only is used
     *      This flag is reset to false if the .cmp file is not found
     */
    MODULE* ReadNetlistModuleDescr( char* aText, bool aTstOnly );

    /**
     * Function loadNewModules
     * Load from libraries new modules found in netlist and add them to the current Board.
     * @return false if a footprint is not found, true if all footprints are loaded
     */
    bool    loadNewModules();

    /**
     * function readModuleComponentLinkfile
     * read the *.cmp file ( filename in m_cmplistFullName )
     * giving the equivalence between footprint names and components
     * to find the footprint name corresponding to aCmpIdent
     * @return true and the module name in aFootprintName, false if not found
     *
     * @param aCmpIdent = component identification: schematic reference or time stamp
     * @param aFootprintName the footprint name corresponding to the component identification
     */
    bool    readModuleComponentLinkfile( const wxString* aCmpIdent, wxString& aFootprintName );
};


/**
 * Function OpenNetlistFile
 *  used to open a netlist file
 */
static FILE* OpenNetlistFile( const wxString& aFullFileName )
{
    if( aFullFileName.IsEmpty() )
        return false;  // No filename: exit

    FILE* file = wxFopen( aFullFileName, wxT( "rt" ) );

    if( file == NULL )
    {
        wxString msg;
        msg.Printf( _( "Netlist file %s not found" ), GetChars( aFullFileName ) );
        wxMessageBox( msg );
    }

    return file;
}


/* Function to sort the footprint list.
 * the given list is sorted by name
 */
static bool SortByLibName( MODULE_INFO* ref, MODULE_INFO* cmp )
{
    int ii = ref->m_LibName.CmpNoCase( cmp->m_LibName );

    return ii > 0;
}


/**
 * Function ReadPcbNetlist
 * Update footprints (load missing footprints and delete on request extra
 * footprints)
 * Update connectivity info ( Net Name list )
 * Update Reference, value and "TIME STAMP"
 * @param aNetlistFullFilename = netlist file name (*.net)
 * @param aCmpFullFileName = cmp/footprint list file name (*.cmp) if not found,
 * @param aMessageWindow  = a wxTextCtrl to print messages (can be NULL).
 * @param aChangeFootprint = true to change existing footprints
 *                              when the netlist gives a different footprint.
 *                           false to keep existing footprints
 * @param aDeleteBadTracks - true to erase erroneous tracks after updating connectivity info.
 * @param aDeleteExtraFootprints - true to remove unlocked footprints found on board but not
 *                                 in netlist.
 * @param aSelect_By_Timestamp - true to use schematic timestamps instead of schematic references
 *                              to identify footprints on board
 *                              (Must be used after a full reannotation in schematic).
 * @return true if Ok
 */
bool PCB_EDIT_FRAME::ReadPcbNetlist( const wxString& aNetlistFullFilename,
                                     const wxString& aCmpFullFileName,
                                     wxTextCtrl*     aMessageWindow,
                                     bool            aChangeFootprint,
                                     bool            aDeleteBadTracks,
                                     bool            aDeleteExtraFootprints,
                                     bool            aSelect_By_Timestamp )
{
    FILE*   netfile = OpenNetlistFile( aNetlistFullFilename );

    if( !netfile )
        return false;

    SetLastNetListRead( aNetlistFullFilename );

    if( aMessageWindow )
    {
        wxString msg;
        msg.Printf( _( "Reading Netlist \"%s\"" ), GetChars( aNetlistFullFilename ) );
        aMessageWindow->AppendText( msg + wxT( "\n" ) );

        if( ! aCmpFullFileName.IsEmpty() )
        {
            msg.Printf( _( "Using component/footprint link file \"%s\"" ),
                        GetChars( aCmpFullFileName ) );
            aMessageWindow->AppendText( msg + wxT( "\n" ) );
        }
     }

    // Clear undo and redo lists to avoid inconsistencies between lists
    GetScreen()->ClearUndoRedoList();

    OnModify();

    // Clear flags and pointers to avoid inconsistencies
    GetBoard()->m_Status_Pcb = 0;
    SetCurItem( NULL );

    wxBusyCursor   dummy;      // Shows an hourglass while calculating

    NETLIST_READER netList_Reader( this, aMessageWindow );
    netList_Reader.m_UseTimeStamp     = aSelect_By_Timestamp;
    netList_Reader.m_ChangeFootprints = aChangeFootprint;
    netList_Reader.ReadNetList( netfile, aNetlistFullFilename, aCmpFullFileName );

    // Delete footprints not found in netlist:
    if( aDeleteExtraFootprints )
    {
        netList_Reader.RemoveExtraFootprints( aNetlistFullFilename );
    }

    // Rebuild the board connectivity:
    Compile_Ratsnest( NULL, true );

    if( aDeleteBadTracks && GetBoard()->m_Track )
    {
        // Remove erroneous tracks
        RemoveMisConnectedTracks( NULL );
        Compile_Ratsnest( NULL, true );
    }

    GetBoard()->DisplayInfo( this );
    m_canvas->Refresh();

    return true;
}


/* function RemoveExtraFootprints
 * Remove (delete) not locked footprints found on board, but not in netlist
 */
void NETLIST_READER::RemoveExtraFootprints( const wxString& aNetlistFileName )
{
    wxArrayString componentsInNetlist;

    // Build list of modules in the netlist
    int modulesCount = BuildComponentsListFromNetlist( aNetlistFileName, componentsInNetlist );

    if( modulesCount == 0 )
        return;

    MODULE* nextModule;
    MODULE* module = m_pcbframe->GetBoard()->m_Modules;
    bool ask_user = true;

    for( ; module != NULL; module = nextModule )
    {
        int ii;
        nextModule = module->Next();

        if( module->m_ModuleStatus & MODULE_is_LOCKED )
            continue;

        for( ii = 0; ii < modulesCount; ii++ )
        {
            if( module->m_Reference->m_Text.CmpNoCase( componentsInNetlist[ii] ) == 0 )
                break; // Module is found in net list.
        }

        if( ii == modulesCount )   // Module not found in netlist.
        {
            if( ask_user )
            {
                ask_user = false;

                if( !IsOK( NULL,
                    _( "Ok to delete not locked footprints not found in netlist?" ) ) )
                    break;
            }

            module->DeleteStructure();
        }
    }
}

/*
 * Function ReadNetlist
 * Update footprints (load missing footprints and delete on request extra
 * footprints)
 * Update References, values, "TIME STAMP" and connectivity data
 * return true if Ok
 *
 *  the format of the netlist is something like:
 * # EESchema Netlist Version 1.0 generee le  18/5/2005-12:30:22
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
bool NETLIST_READER::ReadNetList( FILE*           aFile,
                                  const wxString& aNetlistFileName,
                                  const wxString& aCmplistFileName )
{
    int state   = 0;
    bool is_comment = false;

    m_netlistFullName = aNetlistFileName;
    m_cmplistFullName = aCmplistFileName;

    m_useCmpFile = true;

    /* First, read the netlist: Build the list of footprints to load (new
     * footprints)
     */

    // netlineReader dtor will close aFile
    FILE_LINE_READER netlineReader( aFile, m_netlistFullName );

    while( netlineReader.ReadLine() )
    {
        char* line = StrPurge( netlineReader.Line() );

        if( is_comment ) // Comments in progress
        {
            // Test for end of the current comment
            if( ( line = strchr( line, '}' ) ) == NULL )
                continue;

            is_comment = false;
        }
        if( *line == '{' ) // Start Comment
        {
            is_comment = true;

            if( ( line = strchr( line, '}' ) ) == NULL )
                continue;
        }

        if( *line == '(' )
            state++;

        if( *line == ')' )
            state--;

        if( state == 2 )
        {
            ReadNetlistModuleDescr( line, TESTONLY );
            continue;
        }

        if( state >= 3 ) // First pass: pad descriptions are not read here.
        {
            state--;
        }
    }

    // Load new footprints
    bool success = loadNewModules();

    if( ! success )
        wxMessageBox( _("Some footprints are not found in libraries") );

    /* Second read , All footprints are on board.
     * One must update the schematic info (pad netnames)
     */
    netlineReader.Rewind();
    m_currModule = NULL;
    state = 0;
    is_comment = false;

    while( netlineReader.ReadLine() )
    {
        char* line = StrPurge( netlineReader.Line() );

        if( is_comment )   // we are reading a comment
        {
            // Test for end of the current comment
            if( ( line = strchr( line, '}' ) ) == NULL )
                continue;
            is_comment = false;
        }

        if( *line == '{' ) // this is the beginning of a comment
        {
            is_comment = true;

            if( ( line = strchr( line, '}' ) ) == NULL )
                continue;
        }

        if( *line == '(' )
            state++;

        if( *line == ')' )
            state--;

        if( state == 2 )
        {
            m_currModule = ReadNetlistModuleDescr( line, READMODULE );

            if( m_currModule == NULL ) // the module could not be created (perhaps
            {
                // footprint not found in library)
                continue;
            }
            else // clear pads netnames
            {
                for( D_PAD* pad = m_currModule->m_Pads;  pad;  pad = pad->Next() )
                {
                    pad->SetNetname( wxEmptyString );
                }
            }

            continue;
        }

        if( state >= 3 )
        {
            if( m_currModule )
                SetPadNetName( line );
            state--;
        }
    }

    return true;
}


/* Function ReadNetlistModuleDescr
 * Read the full description of a footprint, from the netlist
 * and update the corresponding module.
 * param aTstOnly bool to switch between 2 modes:
 *      If aTstOnly == false:
 *          if the module does not exist, it is added to m_newModulesList
 *      If aTstOnly = true:
 *          if the module does not exist, it is loaded and added to the board module list
 * param  aText contains the first line of description
 * This function uses m_useFichCmp as a flag to know the footprint name:
 *      If true: component file *.cmp is used
 *      If false: the netlist only is used
 *      This flag is reset to false if the .cmp file is not found
 * Analyze lines like:
 * ( /40C08647 $noname R20 4.7K {Lib=R}
 * (1 VCC)
 * (2 MODB_1)
 * )
 */
MODULE* NETLIST_READER::ReadNetlistModuleDescr( char* aText, bool aTstOnly )
{
    char*    text;
    wxString timeStampPath;         // the full time stamp read from netlist
    wxString textFootprintName;     // the footprint name read from netlist
    wxString textValue;             // the component value read from netlist
    wxString textCmpReference;      // the component schematic reference read from netlist
    wxString cmpFootprintName;      // the footprint name read from the *.cmp file
                                    // giving the equivalence between footprint names and components
    bool     error = false;
    char     line[1024];

    strcpy( line, aText );

    textValue = wxT( "~" );

    // Read descr line like  /40C08647 $noname R20 4.7K {Lib=R}

    // Read time stamp (first word)
    if( ( text = strtok( line, " ()\t\n" ) ) == NULL )
        error = true;
    else
        timeStampPath = FROM_UTF8( text );

    // Read footprint name (second word)
    if( ( text = strtok( NULL, " ()\t\n" ) ) == NULL )
        error = true;
    else
        textFootprintName = FROM_UTF8( text );

    // Read schematic reference (third word)
    if( ( text = strtok( NULL, " ()\t\n" ) ) == NULL )
        error = true;
    else
        textCmpReference = FROM_UTF8( text );

    // Read schematic value (forth word)
    if( ( text = strtok( NULL, " ()\t\n" ) ) == NULL )
        error = true;
    else
        textValue = FROM_UTF8( text );

    if( error )
        return NULL;

    // Test if module is already loaded.
    wxString * identMod = &textCmpReference;

    if( m_UseTimeStamp )
        identMod = &timeStampPath;

    MODULE* module = m_pcbframe->GetBoard()->m_Modules;
    MODULE* nextModule;
    bool    found = false;

    for( ; module != NULL; module = nextModule )
    {
        nextModule = module->Next();

        if( m_UseTimeStamp ) // identification by time stamp
        {
            if( timeStampPath.CmpNoCase( module->m_Path ) == 0 )
                found = true;
        }
        else    // identification by Reference
        {
            if( textCmpReference.CmpNoCase( module->m_Reference->m_Text ) == 0 )
                found = true;
        }

        if( found ) // The footprint corresponding to the component is already on board
        {
            // This footprint is already on board
            // but if m_LibRef (existing footprint name) and the footprint name from netlist
            // do not match, change this footprint on demand.
            if( ! aTstOnly )
            {
                cmpFootprintName = textFootprintName;     // Use footprint name from netlist

                if( m_useCmpFile )                        // Try to get footprint name from .cmp file
                {
                    m_useCmpFile = readModuleComponentLinkfile( identMod, cmpFootprintName );
                }

                /* Module mismatch: current footprint and footprint specified in
                 * net list are different.
                 */
                if( module->m_LibRef.CmpNoCase( cmpFootprintName ) != 0 )
                {
                    if( m_ChangeFootprints )   // footprint exchange allowed.
                    {
                        MODULE* newModule = m_pcbframe->GetModuleLibrary( wxEmptyString,
                                                                          cmpFootprintName,
                                                                          true );

                        if( newModule )
                        {
                            // Change old module to the new module (and delete the old one)
                            m_pcbframe->Exchange_Module( module, newModule, NULL );
                            module = newModule;
                        }
                    }
                    else
                    {
                        wxString msg;
                        msg.Printf( _( "Component \"%s\": Mismatch! module is [%s] and netlist said [%s]\n" ),
                                    GetChars( textCmpReference ),
                                    GetChars( module->m_LibRef ),
                                    GetChars( cmpFootprintName ) );

                        if( m_messageWindow )
                            m_messageWindow->AppendText( msg );
                    }
                }
            }

            break;
        }
    }

    if( module == NULL )                    // a new module must be loaded from libs
    {
        cmpFootprintName = textFootprintName;   // Use footprint name from netlist

        if( m_useCmpFile )                      // Try to get footprint name from .cmp file
        {
            m_useCmpFile = readModuleComponentLinkfile( identMod, cmpFootprintName );
        }

        if( aTstOnly )
        {
            MODULE_INFO* newMod;
            newMod = new MODULE_INFO( cmpFootprintName, textCmpReference, timeStampPath );
            m_newModulesList.push_back( newMod );
        }
        else
        {
            if( m_messageWindow )
            {
                wxString msg;
                msg.Printf( _( "Component [%s] not found" ), GetChars( textCmpReference ) );
                m_messageWindow->AppendText( msg + wxT( "\n" ) );
            }
        }

        return NULL;    // The module could not be loaded.
    }

    // Update current module ( reference, value and "Time Stamp")
    module->m_Reference->m_Text = textCmpReference;
    module->m_Value->m_Text     = textValue;
    module->m_Path = timeStampPath;

    return module;
}


/*
 * Function SetPadNetName
 *  Update a pad netname using the current footprint
 *  Line format: ( <pad number> <net name> )
 *  Param aText = current line read from netlist
 */
bool NETLIST_READER::SetPadNetName( char* aText )
{
    char*       p;
    char        line[256];

    if( m_currModule == NULL )
        return false;

    strncpy( line, aText, sizeof(line) );

    if( ( p = strtok( line, " ()\t\n" ) ) == NULL )
        return false;

    wxString pinName = FROM_UTF8( p );

    if( ( p = strtok( NULL, " ()\t\n" ) ) == NULL )
        return false;

    wxString netName = FROM_UTF8( p );

    bool found = false;
    for( D_PAD* pad = m_currModule->m_Pads;  pad;  pad = pad->Next() )
    {
        wxString padName = pad->GetPadName();

        if( padName == pinName )
        {
            found = true;
            if( (char) netName[0] != '?' )
                pad->SetNetname( netName );
            else
                pad->SetNetname( wxEmptyString );
        }
    }

    if( !found )
    {
        if( m_messageWindow )
        {
            wxString msg;
            msg.Printf( _( "Module [%s]: Pad [%s] not found" ),
                        GetChars( m_currModule->m_Reference->m_Text ),
                        GetChars( pinName ) );
            m_messageWindow->AppendText( msg + wxT( "\n" ) );
        }
    }

    return found;
}


/**
 * build and shows a list of existing modules on board
 * The user can select a module from this list
 * @return a pointer to the selected module or NULL
 */
MODULE* PCB_EDIT_FRAME::ListAndSelectModuleName( void )
{
    MODULE* Module;

    if( GetBoard()->m_Modules == NULL )
    {
        DisplayError( this, _( "No Modules" ) );
        return 0;
    }

    wxArrayString listnames;
    Module = (MODULE*) GetBoard()->m_Modules;

    for( ; Module != NULL; Module = (MODULE*) Module->Next() )
        listnames.Add( Module->m_Reference->m_Text );

    EDA_LIST_DIALOG dlg( this, _( "Components" ), listnames, wxEmptyString );

    if( dlg.ShowModal() != wxID_OK )
        return NULL;

    wxString ref = dlg.GetTextSelection();
    Module = (MODULE*) GetBoard()->m_Modules;

    for( ; Module != NULL; Module = Module->Next() )
    {
        if( Module->m_Reference->m_Text == ref )
            break;
    }

    return Module;
}


/*
 * Function Test_Duplicate_Missing_And_Extra_Footprints
 * Build a list of duplicate, missing and extra footprints
 * from the current board and a netlist netlist :
 * Shows 3 lists:
 *  1 - duplicate footprints on board
 *  2 - missing footprints (found in netlist but not on board)
 *  3 - footprints not in netlist but on board
 * @param aNetlistFullFilename = the full filename netlist
 */
void PCB_EDIT_FRAME::Test_Duplicate_Missing_And_Extra_Footprints(
    const wxString& aNetlistFullFilename )
{
#define MAX_LEN_TXT 32
    int           ii;
    int           NbModulesNetListe, nberr = 0;
    wxArrayString tmp;
    wxArrayString list;

    if( GetBoard()->m_Modules == NULL )
    {
        DisplayInfoMessage( this, _( "No modules" ) );
        return;
    }

    // Build the list of references of the net list modules.
    NETLIST_READER netList_Reader( this );
    NbModulesNetListe = netList_Reader.BuildComponentsListFromNetlist( aNetlistFullFilename, tmp );

    if( NbModulesNetListe < 0 )
        return;  // File not found

    if( NbModulesNetListe == 0 )
    {
        wxMessageBox( _( "No modules in NetList" ) );
        return;
    }

    // Search for duplicate footprints.
    list.Add( _( "Duplicates" ) );

    MODULE* module = GetBoard()->m_Modules;

    for( ; module != NULL; module = module->Next() )
    {
        MODULE* altmodule = module->Next();

        for( ; altmodule != NULL; altmodule = altmodule->Next() )
        {
            if( module->m_Reference->m_Text.CmpNoCase( altmodule->m_Reference->m_Text ) == 0 )
            {
                if( module->m_Reference->m_Text.IsEmpty() )
                    list.Add( wxT("<noref>") );
                else
                    list.Add( module->m_Reference->m_Text );

                nberr++;
                break;
            }
        }
    }

    // Search for the missing module by the net list.
    list.Add( _( "Missing:" ) );

    for( ii = 0; ii < NbModulesNetListe; ii++ )
    {
        module = (MODULE*) GetBoard()->m_Modules;

        for( ; module != NULL; module = module->Next() )
        {
            if( module->m_Reference->m_Text.CmpNoCase( tmp[ii] ) == 0 )
            {
                break;
            }
        }

        if( module == NULL )    // Module missing, not found in board
        {
            list.Add( tmp[ii] );
            nberr++;
        }
    }

    // Search for modules not in net list.
    list.Add( _( "Not in Netlist:" ) );

    module = GetBoard()->m_Modules;
    for( ; module != NULL; module = module->Next() )
    {
        for( ii = 0; ii < NbModulesNetListe; ii++ )
        {
            if( module->m_Reference->m_Text.CmpNoCase( tmp[ii] ) == 0 )
            {
                break; // Module is in net list.
            }
        }

        if( ii == NbModulesNetListe )   // Module not found in netlist
        {
            list.Add( module->m_Reference->m_Text );
            nberr++;
        }
    }

    wxSingleChoiceDialog dlg( this, wxEmptyString, _( "Check Modules" ), list, NULL,
                              wxCHOICEDLG_STYLE & ~wxCANCEL );

    dlg.ShowModal();
}


/**
 * Function BuildComponentsListFromNetlist
 * Fill aBufName with component references read from the netlist.
 * @param aNetlistFilename = netlist full file name
 * @param aBufName = wxArrayString to fill with component references
 * @return component count, or -1 if netlist file cannot opened
 */
int NETLIST_READER::BuildComponentsListFromNetlist( const wxString& aNetlistFilename,
                                                    wxArrayString&  aBufName )
{
    int   component_count;
    int   state;
    bool  is_comment;
    char* text;

    FILE* netfile = OpenNetlistFile( aNetlistFilename );

    if( netfile == NULL )
        return -1;

    FILE_LINE_READER netlineReader( netfile, aNetlistFilename );    // ctor will close netfile
    char*            Line = netlineReader;

    state = 0;
    is_comment = false;
    component_count = 0;

    while( netlineReader.ReadLine() )
    {
        text = StrPurge( Line );

        if( is_comment )
        {
            if( ( text = strchr( text, '}' ) ) == NULL )
                continue;

            is_comment = false;
        }

        if( *text == '{' ) // Comments.
        {
            is_comment = true;

            if( ( text = strchr( text, '}' ) ) == NULL )
                continue;
        }

        if( *text == '(' )
            state++;

        if( *text == ')' )
            state--;

        if( state == 2 )
        {
            // skip TimeStamp:
            strtok( Line, " ()\t\n" );

            // skip footprint name:
            strtok( NULL, " ()\t\n" );

            // Load the reference of the component:
            text = strtok( NULL, " ()\t\n" );
            component_count++;
            aBufName.Add( FROM_UTF8( text ) );
            continue;
        }

        if( state >= 3 )
        {
            state--;
        }
    }

    return component_count;
}


/*
 * function readModuleComponentLinkfile
 * read the *.cmp file ( filename in m_cmplistFullName )
 * giving the equivalence Footprint_names / components
 * to find the footprint name corresponding to aCmpIdent
 * return true and the module name in aFootprintName, false if not found
 *
 * param aCmpIdent = component identification: schematic reference or time stamp
 * param aFootprintName the footprint name corresponding to the component identification
 *
 * Sample file:
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
bool NETLIST_READER::readModuleComponentLinkfile( const wxString* aCmpIdent,
                                                  wxString& aFootprintName )
{
    wxString refcurrcmp;    // Stores value read from line like Reference = BUS1;
    wxString timestamp;     // Stores value read from line like TimeStamp = /32307DE2/AA450F67;
    wxString idmod;         // Stores value read from line like IdModule  = CP6;

    FILE*    cmpFile = wxFopen( m_cmplistFullName, wxT( "rt" ) );

    if( cmpFile == NULL )
    {
        wxString msg;
        msg.Printf( _( "File <%s> not found, use Netlist for lib module selection" ),
                   GetChars( m_cmplistFullName ) );
        DisplayError( NULL, msg, 20 );
        return false;
    }

    // netlineReader dtor will close cmpFile
    FILE_LINE_READER netlineReader( cmpFile, m_cmplistFullName );
    wxString buffer;
    wxString value;

    while( netlineReader.ReadLine() )
    {
        buffer = FROM_UTF8( netlineReader.Line() );

        if( ! buffer.StartsWith( wxT("BeginCmp") ) )
            continue;

        // Begin component description.
        refcurrcmp.Empty();
        idmod.Empty();
        timestamp.Empty();

        while( netlineReader.ReadLine() )
        {
            buffer = FROM_UTF8( netlineReader.Line() );

            if( buffer.StartsWith( wxT("EndCmp") ) )
                break;

            // store string value, stored between '=' and ';' delimiters.
            value = buffer.AfterFirst( '=' );
            value = value.BeforeLast( ';');
            value.Trim(true);
            value.Trim(false);

            if( buffer.StartsWith( wxT("Reference") ) )
            {
                refcurrcmp = value;
                continue;
            }

            if( buffer.StartsWith( wxT("IdModule  =" ) ) )
            {
                idmod = value;
                continue;
            }

            if( buffer.StartsWith( wxT("TimeStamp =" ) ) )
            {
                timestamp = value;
                continue;
            }
        }

        // Check if this component is the right component:
        if( m_UseTimeStamp )    // Use schematic timestamp to locate the footprint
        {
            if( aCmpIdent->CmpNoCase( timestamp ) == 0  && !timestamp.IsEmpty() )
            {    // Found
                aFootprintName = idmod;
                return true;
            }
        }
        else                   // Use schematic reference to locate the footprint
        {
            if( aCmpIdent->CmpNoCase( refcurrcmp ) == 0 )   // Found!
            {
                aFootprintName = idmod;
                return true;
            }
        }
    }

    return true;
}


/* Load new modules from library.
 * If a new module is already loaded it is duplicated, which avoids multiple
 * unnecessary disk or net access to read libraries.
 * return false if a footprint is not found, true if OK
 */
bool NETLIST_READER::loadNewModules()
{
    MODULE_INFO* ref, * cmp;
    MODULE*      Module = NULL;
    wxPoint      ModuleBestPosition;
    BOARD*       pcb = m_pcbframe->GetBoard();
    bool         success = true;

    if( m_newModulesList.size() == 0 )
        return true;

    sort( m_newModulesList.begin(), m_newModulesList.end(), SortByLibName );

    // Calculate the footprint "best" position:
    EDA_RECT bbbox = pcb->ComputeBoundingBox( true );

    if( bbbox.GetWidth() || bbbox.GetHeight() )
    {
        ModuleBestPosition = bbbox.GetEnd();
        ModuleBestPosition.y += 5000;
    }

    ref = cmp = m_newModulesList[0];

    for( unsigned ii = 0; ii < m_newModulesList.size(); ii++ )
    {
        cmp = m_newModulesList[ii];

        if( (ii == 0) || ( ref->m_LibName != cmp->m_LibName) )
        {
            // New footprint : must be loaded from a library
            Module = m_pcbframe->GetModuleLibrary( wxEmptyString, cmp->m_LibName, false );
            ref = cmp;

            if( Module == NULL )
            {
                success = false;
                wxString msg;
                msg.Printf( _( "Component [%s]: footprint <%s> not found" ),
                            GetChars( cmp->m_CmpName ),
                            GetChars( cmp->m_LibName ) );

                if( m_messageWindow )
                {
                    msg += wxT("\n");
                    m_messageWindow->AppendText( msg );
                }
                else
                {
                    DisplayError( NULL, msg );
                }

                continue;
            }

            Module->SetPosition( ModuleBestPosition );

            /* Update schematic links : reference "Time Stamp" and schematic
             * hierarchical path */
            Module->m_Reference->m_Text = cmp->m_CmpName;
            Module->SetTimeStamp( GetNewTimeStamp() );
            Module->m_Path = cmp->m_TimeStampPath;
        }
        else
        {
            // Footprint already loaded from a library, duplicate it (faster)
            if( Module == NULL )
                continue;            // Module does not exist in library.

            MODULE* newmodule = new MODULE( *Module );
            newmodule->SetParent( pcb );

            pcb->Add( newmodule, ADD_APPEND );

            Module = newmodule;
            Module->m_Reference->m_Text = cmp->m_CmpName;
            Module->SetTimeStamp( GetNewTimeStamp() );
            Module->m_Path = cmp->m_TimeStampPath;
        }
    }

    return success;
}
