/**********************************************/
/* Footprints selection and loading functions */
/**********************************************/

#include "fctsys.h"
#include "gr_basic.h"
#include "common.h"
#include "class_drawpanel.h"
#include "confirm.h"
#include "eda_doc.h"
#include "kicad_string.h"
#include "gestfich.h"
#include "dialog_get_component.h"
#include "appl_wxstruct.h"

#include "pcbnew.h"
#include "wxPcbStruct.h"
#include "module_editor_frame.h"
#include "dialog_helpers.h"
#include "richio.h"
#include "filter_reader.h"
#include "footprint_info.h"


static void DisplayCmpDoc( wxString& Name );

static FOOTPRINT_LIST MList;

/**
 * Function Load_Module_From_BOARD
 * load in Modedit a footfrint from the main board
 * @param Module = the module to load. If NULL, a module reference will we asked to user
 * @return true if a module isloaded, false otherwise.
 */
bool WinEDA_ModuleEditFrame::Load_Module_From_BOARD( MODULE* Module )
{
    MODULE* NewModule;
    PCB_BASE_FRAME* parent = (PCB_BASE_FRAME*) GetParent();

    if( Module == NULL )
    {
        if( ! parent->GetBoard() || ! parent->GetBoard()->m_Modules )
            return false;

        Module = Select_1_Module_From_BOARD( parent->GetBoard() );
    }

    if( Module == NULL )
        return false;

    SetCurItem( NULL );

    Clear_Pcb( false );

    GetBoard()->m_Status_Pcb = 0;
    NewModule = new MODULE( GetBoard() );
    NewModule->Copy( Module );
    NewModule->m_Link = Module->m_TimeStamp;

    Module = NewModule;

    GetBoard()->Add( Module );

    Module->m_Flags = 0;

    GetBoard()->m_NetInfo->BuildListOfNets();

    GetScreen()->SetCrossHairPosition( wxPoint( 0, 0 ) );
    Place_Module( Module, NULL );

    if( Module->GetLayer() != LAYER_N_FRONT )
        Module->Flip( Module->m_Pos );

    Rotate_Module( NULL, Module, 0, false );
    GetScreen()->ClrModify();
    Zoom_Automatique( false );

    return true;
}


MODULE* PCB_BASE_FRAME::Load_Module_From_Library( const wxString& library, wxDC* DC )
{
    MODULE* module;
    wxPoint curspos = GetScreen()->GetCrossHairPosition();
    wxString             ModuleName, keys;
    static wxArrayString HistoryList;
    static wxString      lastCommponentName;
    bool             AllowWildSeach = TRUE;

    /* Ask for a component name or key words */
    DIALOG_GET_COMPONENT dlg( this, GetComponentDialogPosition(), HistoryList,
                          _( "Place Module" ), false );

    dlg.SetComponentName( lastCommponentName );

    if( dlg.ShowModal() == wxID_CANCEL )
        return NULL;

    ModuleName = dlg.GetComponentName();

    if( ModuleName.IsEmpty() )  /* Cancel command */
    {
        DrawPanel->MoveCursorToCrossHair();
        return NULL;
    }

    ModuleName.MakeUpper();

    if( ModuleName[0] == '=' )   // Selection by keywords
    {
        AllowWildSeach = false;
        keys = ModuleName.AfterFirst( '=' );
        ModuleName = Select_1_Module_From_List( this, library, wxEmptyString,
                                                keys );
        if( ModuleName.IsEmpty() )  /* Cancel command */
        {
            DrawPanel->MoveCursorToCrossHair();
            return NULL;
        }
    }
    else if( ( ModuleName.Contains( wxT( "?" ) ) )
            || ( ModuleName.Contains( wxT( "*" ) ) ) )  // Selection wild card
    {
        AllowWildSeach = false;
        ModuleName     = Select_1_Module_From_List( this, library, ModuleName,
                                                    wxEmptyString );
        if( ModuleName.IsEmpty() )
        {
            DrawPanel->MoveCursorToCrossHair();
            return NULL;    /* Cancel command. */
        }
    }

    module = Get_Librairie_Module( library, ModuleName, false );

    if( ( module == NULL ) && AllowWildSeach )    /* Search with wildcard */
    {
        AllowWildSeach = false;
        wxString wildname = wxChar( '*' ) + ModuleName + wxChar( '*' );
        ModuleName = wildname;
        ModuleName = Select_1_Module_From_List( this, library, ModuleName,
                                                wxEmptyString );
        if( ModuleName.IsEmpty() )
        {
            DrawPanel->MoveCursorToCrossHair();
            return NULL;    /* Cancel command. */
        }
        else
            module = Get_Librairie_Module( library, ModuleName, TRUE );
    }

    GetScreen()->SetCrossHairPosition( curspos );
    DrawPanel->MoveCursorToCrossHair();

    if( module )
    {
        lastCommponentName = ModuleName;
        AddHistoryComponentName( HistoryList, ModuleName );

        module->m_Flags     = IS_NEW;
        module->m_Link      = 0;
        module->m_TimeStamp = GetTimeStamp();
        GetBoard()->m_Status_Pcb = 0;
        module->SetPosition( curspos );

        /* TODO: call RecalculateAllTracksNetcode() only if some pads pads have
         * a netname.
         * If all pads are not connected (usually the case in module libraries,
         * rebuild only the pad and list of nets ( faster)
         */

//        GetBoard()->m_Pcb->m_NetInfo->BuildListOfNets();
        RecalculateAllTracksNetcode();

        if( DC )
            module->Draw( DrawPanel, DC, GR_OR );
    }

    return module;
}


/**
 * Function Get_Librairie_Module
 *
 *  Read active libraries or one library to find and load a given module
 *  If found the module is linked to the tail of linked list of modules
 *  @param aLibraryFullFilename: the full filename of the library to read. If empty,
 *                    all active libraries are read
 *  @param aModuleName = module name to load
 *  @param aDisplayMessageError = true to display an error message if any.
 *  @return a MODULE * pointer to the new module, or NULL
 *
 */
MODULE* PCB_BASE_FRAME::Get_Librairie_Module( const wxString& aLibraryFullFilename,
                                              const wxString& aModuleName,
                                              bool            aDisplayMessageError )
{
    int        Found = 0;
    wxFileName fn;
    char*      Line;
    wxString   Name;
    wxString   msg, tmp;
    MODULE*    NewModule;
    FILE*      file = NULL;
    unsigned   ii;
    bool       one_lib = aLibraryFullFilename.IsEmpty() ? false : true;

    for( ii = 0; ii < g_LibName_List.GetCount(); ii++ )
    {
        if( one_lib )
            fn = aLibraryFullFilename;
        else
            fn = wxFileName( wxEmptyString, g_LibName_List[ii],
                             ModuleFileExtension );

        tmp = wxGetApp().FindLibraryPath( fn );

        if( !tmp )
        {
            if( aDisplayMessageError )
            {
                msg.Printf( _( "PCB footprint library file <%s> not found in search paths." ),
                            GetChars( fn.GetFullName() ) );
                wxMessageBox( msg, _( "Library Load Error" ),
                              wxOK | wxICON_ERROR, this );
            }
            continue;
        }

        file = wxFopen( tmp, wxT( "rt" ) );

        if( file == NULL )
        {
            msg.Printf( _( "Could not open PCB footprint library file <%s>." ),
                        GetChars( tmp ) );
            wxMessageBox( msg, _( "Library Load Error" ),
                          wxOK | wxICON_ERROR, this );
            continue;
        }

        FILE_LINE_READER fileReader( file, tmp );

        FILTER_READER reader( fileReader );

        msg.Printf( _( "Scan Lib: %s" ), GetChars( tmp ) );
        SetStatusText( msg );

        /* Reading header ENTETE_LIBRAIRIE */
        reader.ReadLine();
        Line = reader.Line();
        StrPurge( Line );
        if( strnicmp( Line, ENTETE_LIBRAIRIE, L_ENTETE_LIB ) != 0 )
        {
            msg.Printf( _( "<%s> is not a valid Kicad PCB footprint library file." ),
                        GetChars( tmp ) );
            wxMessageBox( msg, _( "Library Load Error" ),
                          wxOK | wxICON_ERROR, this );
            return NULL;
        }

        /* Reading the list of modules in the library. */
        Found = 0;
        while( !Found && reader.ReadLine() )
        {
            Line = reader.Line();
            if( strnicmp( Line, "$MODULE", 6 ) == 0 )
                break;
            if( strnicmp( Line, "$INDEX", 6 ) == 0 )
            {
                while( reader.ReadLine() )
                {
                    Line = reader.Line();
                    if( strnicmp( Line, "$EndINDEX", 9 ) == 0 )
                        break;
                    StrPurge( Line );
                    msg = FROM_UTF8( Line );
                    if( msg.CmpNoCase( aModuleName ) == 0 )
                    {
                        Found = 1;
                        break; /* Found! */
                    }
                }
            }
        }

        /* Read library. */
        while( Found && reader.ReadLine() )
        {
            Line = reader.Line();
            if( Line[0] != '$' )
                continue;

            if( Line[1] != 'M' )
                continue;

            if( strnicmp( Line, "$MODULE", 7 ) != 0 )
                continue;

            StrPurge( Line + 8 );

            // Read module name.
            Name = FROM_UTF8( Line + 8 );

            if( Name.CmpNoCase( aModuleName ) == 0 )
            {
                NewModule = new MODULE( GetBoard() );

                // Switch the locale to standard C (needed to print
                // floating point numbers like 1.3)
                SetLocaleTo_C_standard();
                NewModule->ReadDescr( &reader );
                SetLocaleTo_Default();         // revert to the current locale
                GetBoard()->Add( NewModule, ADD_APPEND );
                SetStatusText( wxEmptyString );
                return NewModule;
            }
        }

        if( one_lib )
            break;
    }

    if( aDisplayMessageError )
    {
        msg.Printf( _( "Module <%s> not found" ), GetChars( aModuleName ) );
        DisplayError( NULL, msg );
    }

    return NULL;
}


/**
 * Function Select_1_Module_From_List
 * Display a list of modules found in active libraries or a given library
 *
 * @param aWindow - The active window.
 * @param aLibraryFullFilename = library to list (if aLibraryFullFilename ==
 *                                void, list all modules)
 * @param aMask = Display filter (wildcard)( Mask = wxEmptyString if not used )
 * @param aKeyWord = keyword list, to display a filtered list of module having
 *                    one (or more) of these keyworks in their keywork list
 *                    ( aKeyWord = wxEmptyString if not used )
 *
 * @return wxEmptyString if abort or fails, or the selected module name if Ok
 */
wxString PCB_BASE_FRAME::Select_1_Module_From_List( EDA_DRAW_FRAME* aWindow,
                                                    const wxString& aLibraryFullFilename,
                                                    const wxString& aMask,
                                                    const wxString& aKeyWord )
{
    static wxString OldName;    /* Save the name of the last module loaded. */
    wxString        CmpName;
    wxString        msg;
    wxArrayString   libnames_list;

    if( aLibraryFullFilename.IsEmpty() )
        libnames_list = g_LibName_List;
    else
        libnames_list.Add( aLibraryFullFilename );

    /* Find modules in libraries. */
    MList.ReadFootprintFiles( libnames_list );

    wxArrayString footprint_names_list;
    /* Create list of modules if search by keyword. */
    if( !aKeyWord.IsEmpty() )
    {
        for( unsigned ii = 0; ii < MList.GetCount(); ii++ )
        {
            if( KeyWordOk( aKeyWord, MList.GetItem(ii).m_KeyWord ) )
                footprint_names_list.Add( MList.GetItem(ii).m_Module );
        }
    }
    else
        for( unsigned ii = 0; ii < MList.GetCount(); ii++ )
            footprint_names_list.Add( MList.GetItem(ii).m_Module );

    msg.Printf( _( "Modules [%d items]" ), footprint_names_list.GetCount() );
    WinEDAListBox dlg( aWindow, msg, footprint_names_list, OldName,
                       DisplayCmpDoc, GetComponentDialogPosition() );

    if( dlg.ShowModal() == wxID_OK )
        CmpName = dlg.GetTextSelection();
    else
        CmpName.Empty();

    if( CmpName != wxEmptyString )
        OldName = CmpName;

    return CmpName;
}


/* Find and display the doc Component Name
 * The list of doc is pointed to by mlist.
 */
static void DisplayCmpDoc( wxString& Name )
{
    FOOTPRINT_INFO* module_info = MList.GetModuleInfo( Name );

    if( !module_info )
    {
        Name.Empty();
        return;
    }

    Name  = module_info->m_Doc.IsEmpty() ? wxT( "No Doc" ) : module_info->m_Doc;
    Name += wxT( "\nKeyW: " );
    Name += module_info->m_KeyWord.IsEmpty() ? wxT( "No Keyword" ) : module_info->m_KeyWord;
}


/**
 * Function Select_1_Module_From_BOARD
 * Display the list of modules currently existing on the BOARD
 * @return a pointer to a module if this module is selected or NULL otherwise
 * @param aPcb = the board from modules can be loaded
 */
MODULE* WinEDA_ModuleEditFrame::Select_1_Module_From_BOARD( BOARD* aPcb )
{
    MODULE*         Module;
    static wxString OldName;       /* Save name of last module selectec. */
    wxString        CmpName, msg;

    wxArrayString listnames;

    Module = aPcb->m_Modules;
    for( ; Module != NULL; Module = (MODULE*) Module->Next() )
        listnames.Add( Module->m_Reference->m_Text );

    msg.Printf( _( "Modules [%d items]" ), listnames.GetCount() );

    WinEDAListBox dlg( this, msg, listnames, wxEmptyString );
    dlg.SortList();

    if( dlg.ShowModal() == wxID_OK )
        CmpName = dlg.GetTextSelection();
    else
        return NULL;

    OldName = CmpName;

    Module = aPcb->m_Modules;
    for( ; Module != NULL; Module = (MODULE*) Module->Next() )
    {
        if( CmpName == Module->m_Reference->m_Text )
            break;
    }

    return Module;
}
