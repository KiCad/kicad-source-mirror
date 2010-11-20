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

class ModList
{
public:
    ModList* Next;
    wxString m_Name, m_Doc, m_KeyWord;

public:
    ModList()
    {
        Next = NULL;
    }

    ~ModList()
    {
    }
};


static void DisplayCmpDoc( wxString& Name );
static void ReadDocLib( const wxString& ModLibName );


static ModList* MList;

/**
 * Function Load_Module_From_BOARD
 * load in Modedit a footfrint from the main board
 * @param Module = the module to load. If NULL, a module reference will we asked to user
 * @return true if a module isloaded, false otherwise.
 */
bool WinEDA_ModuleEditFrame::Load_Module_From_BOARD( MODULE* Module )
{
    MODULE* NewModule;
    WinEDA_BasePcbFrame* parent = (WinEDA_BasePcbFrame*) GetParent();

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

    GetScreen()->m_Curseur.x = GetScreen()->m_Curseur.y = 0;
    Place_Module( Module, NULL );
    if( Module->GetLayer() != LAYER_N_FRONT )
        Module->Flip( Module->m_Pos );
    Rotate_Module( NULL, Module, 0, false );
    GetScreen()->ClrModify();
    Zoom_Automatique( TRUE );

    return true;
}


MODULE* WinEDA_BasePcbFrame::Load_Module_From_Library( const wxString& library,
                                                       wxDC*           DC )
{
    MODULE* module;
    wxPoint curspos = GetScreen()->m_Curseur;
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
        DrawPanel->MouseToCursorSchema();
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
            DrawPanel->MouseToCursorSchema();
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
            DrawPanel->MouseToCursorSchema();
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
            DrawPanel->MouseToCursorSchema();
            return NULL;    /* Cancel command. */
        }
        else
            module = Get_Librairie_Module( library, ModuleName, TRUE );
    }

    GetScreen()->m_Curseur = curspos;
    DrawPanel->MouseToCursorSchema();

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
 *  @param aLibrary: the full filename of the library to read. If empty,
 *                    all active libraries are read
 *  @param aModuleName = module name to load
 *  @param aDisplayMessageError = true to display an error message if any.
 *  @return a MODULE * pointer to the new module, or NULL
 *
 */
MODULE* WinEDA_BasePcbFrame::Get_Librairie_Module(
    const wxString& aLibraryFullFilename,
    const wxString& aModuleName,
    bool            aDisplayMessageError )
{
    int        LineNum, Found = 0;
    wxFileName fn;
    char       Line[512];
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

        msg.Printf( _( "Scan Lib: %s" ), GetChars( tmp ) );
        Affiche_Message( msg );

        /* Reading header ENTETE_LIBRAIRIE */
        LineNum = 0;
        GetLine( file, Line, &LineNum );
        StrPurge( Line );
        if( strnicmp( Line, ENTETE_LIBRAIRIE, L_ENTETE_LIB ) != 0 )
        {
            msg.Printf( _( "<%s> is not a valid Kicad PCB footprint library file." ),
                        GetChars( tmp ) );
            wxMessageBox( msg, _( "Library Load Error" ),
                          wxOK | wxICON_ERROR, this );
            fclose( file );
            return NULL;
        }

        /* Reading the list of modules in the library. */
        Found = 0;
        while( !Found && GetLine( file, Line, &LineNum ) )
        {
            if( strnicmp( Line, "$MODULE", 6 ) == 0 )
                break;
            if( strnicmp( Line, "$INDEX", 6 ) == 0 )
            {
                while( GetLine( file, Line, &LineNum ) )
                {
                    if( strnicmp( Line, "$EndINDEX", 9 ) == 0 )
                        break;
                    StrPurge( Line );
                    msg = CONV_FROM_UTF8( Line );
                    if( msg.CmpNoCase( aModuleName ) == 0 )
                    {
                        Found = 1;
                        break; /* Found! */
                    }
                }
            }
        }

        /* Read library. */
        while( Found && GetLine( file, Line, &LineNum ) )
        {
            if( Line[0] != '$' )
                continue;
            if( Line[1] != 'M' )
                continue;
            if( strnicmp( Line, "$MODULE", 7 ) != 0 )
                continue;
            /* Read module name. */
            Name = CONV_FROM_UTF8( Line + 8 );

            if( Name.CmpNoCase( aModuleName ) == 0 )
            {
                NewModule = new MODULE( GetBoard() );

                // Switch the locale to standard C (needed to print
                // floating point numbers like 1.3)
                SetLocaleTo_C_standard();
                NewModule->ReadDescr( file, &LineNum );
                SetLocaleTo_Default();         // revert to the current locale
                GetBoard()->Add( NewModule, ADD_APPEND );
                fclose( file );
                Affiche_Message( wxEmptyString );
                return NewModule;
            }
        }

        fclose( file );
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
 *  Display a list of modules found in active libraries or a given library
 *  @param aLibraryFullFilename = library to list (if aLibraryFullFilename ==
 *                                void, list all modules)
 *  @param aMask = Display filter (wildcard)( Mask = wxEmptyString if not used
 * )
 *  @param aKeyWord = keyword list, to display a filtered list of module having
 *                    one (or more) of these keyworks in their keywork list
 *                    ( aKeyWord = wxEmptyString if not used )
 *
 *  @return wxEmptyString if abort or fails, or the selected module name if Ok
 */
wxString WinEDA_BasePcbFrame::Select_1_Module_From_List(
    WinEDA_DrawFrame* active_window,
    const wxString& aLibraryFullFilename,
    const wxString& aMask, const wxString& aKeyWord )
{
    int             LineNum;
    unsigned        ii;
    char            Line[1024];
    wxFileName      fn;
    static wxString OldName;    /* Save the name of the last module loaded. */
    wxString        CmpName, tmp;
    FILE*           file;
    wxString        msg;
    wxArrayString   itemslist;

    wxBeginBusyCursor();

    /* Find modules in libraries. */
    for( ii = 0; ii < g_LibName_List.GetCount(); ii++ )
    {
        /* Calculate the full file name of the library. */
        if( aLibraryFullFilename.IsEmpty() )
        {
            fn = wxFileName( wxEmptyString, g_LibName_List[ii],
                             ModuleFileExtension );
        }
        else
            fn = aLibraryFullFilename;

        tmp = wxGetApp().FindLibraryPath( fn );

        if( !tmp )
        {
            msg.Printf( _( "PCB footprint library file <%s> not found in search paths." ),
                        GetChars( fn.GetFullName() ) );
            wxMessageBox( msg, _( "Library Load Error" ),
                          wxOK | wxICON_ERROR, this );
            continue;
        }

        ReadDocLib( tmp );

        if( !aKeyWord.IsEmpty() ) /* Don't read the library if selection
                                   * by keywords, already read. */
        {
            if( !aLibraryFullFilename.IsEmpty() )
                break;
            continue;
        }

        file = wxFopen( tmp, wxT( "rt" ) );

        if( file  == NULL )
        {
            if( !aLibraryFullFilename.IsEmpty() )
                break;
            continue;
        }

        // Statusbar library loaded message
        msg = _( "Library " ) + fn.GetFullPath() + _( " loaded" );
        Affiche_Message( msg );

        /* Read header. */
        LineNum = 0;
        GetLine( file, Line, &LineNum, sizeof(Line) - 1 );

        if( strnicmp( Line, ENTETE_LIBRAIRIE, L_ENTETE_LIB ) != 0 )
        {
            msg.Printf( _( "<%s> is not a valid Kicad PCB footprint library file." ),
                        GetChars( tmp ) );
            wxMessageBox( msg, _( "Library Load Error" ),
                          wxOK | wxICON_ERROR, this );
            fclose( file );
            continue;
        }

        /* Read library. */
        while( GetLine( file, Line, &LineNum, sizeof(Line) - 1 ) )
        {
            if( Line[0] != '$' )
                continue;
            if( strnicmp( Line, "$MODULE", 6 ) == 0 )
                break;
            if( strnicmp( Line, "$INDEX", 6 ) == 0 )
            {
                while( GetLine( file, Line, &LineNum ) )
                {
                    if( strnicmp( Line, "$EndINDEX", 9 ) == 0 )
                        break;
                    strupper( Line );
                    msg = CONV_FROM_UTF8( StrPurge( Line ) );
                    if( aMask.IsEmpty() )
                        itemslist.Add( msg );
                    else if( WildCompareString( aMask, msg, false ) )
                        itemslist.Add( msg );
                }
            } /* End read INDEX */
        }

        /* End read library. */
        fclose( file );
        file = NULL;

        if( !aLibraryFullFilename.IsEmpty() )
            break;
    }

    /* Create list of modules if search by keyword. */
    if( !aKeyWord.IsEmpty() )
    {
        ModList* ItemMod = MList;
        while( ItemMod != NULL )
        {
            if( KeyWordOk( aKeyWord, ItemMod->m_KeyWord ) )
                itemslist.Add( ItemMod->m_Name );
            ItemMod = ItemMod->Next;
        }
    }

    wxEndBusyCursor();

    msg.Printf( _( "Modules [%d items]" ), itemslist.GetCount() );
    WinEDAListBox dlg( active_window, msg, itemslist, OldName,
                        DisplayCmpDoc, GetComponentDialogPosition() );

    dlg.SortList();

    if( dlg.ShowModal() == wxID_OK )
        CmpName = dlg.GetTextSelection();
    else
        CmpName.Empty();

    while( MList != NULL )
    {
        ModList* NewMod = MList->Next;
        delete MList;
        MList = NewMod;
    }

    if( CmpName != wxEmptyString )
        OldName = CmpName;

    return CmpName;
}


/* Find and display the doc Component Name
 * The list of doc is pointed to by mlist.
 */
static void DisplayCmpDoc( wxString& Name )
{
    ModList* Mod = MList;

    if( !Mod )
    {
        Name.Empty();
        return;
    }

    while( Mod )
    {
        if( !Mod->m_Name.IsEmpty() && ( Mod->m_Name.CmpNoCase( Name ) == 0 ) )
            break;
        Mod = Mod->Next;
    }

    if( Mod )
    {
        Name  = !Mod->m_Doc.IsEmpty() ? Mod->m_Doc  : wxT( "No Doc" );
        Name += wxT( "\nKeyW: " );
        Name += !Mod->m_KeyWord.IsEmpty() ? Mod->m_KeyWord : wxT( "No Keyword" );
    }
    else
        Name = wxEmptyString;
}


/* Read the doc file and combine with a library ModLibName.
 * Load in memory the list of docs string pointed to by mlist
 * ModLibName = full file name of the library modules
 */
static void ReadDocLib( const wxString& ModLibName )
{
    ModList*   NewMod;
    char       Line[1024];
    FILE*      LibDoc;
    wxFileName fn = ModLibName;

    fn.SetExt( EXT_DOC );

    if( ( LibDoc = wxFopen( fn.GetFullPath(), wxT( "rt" ) ) ) == NULL )
        return;

    GetLine( LibDoc, Line, NULL, sizeof(Line) - 1 );
    if( strnicmp( Line, ENTETE_LIBDOC, L_ENTETE_LIB ) != 0 )
        return;

    while( GetLine( LibDoc, Line, NULL, sizeof(Line) - 1 ) )
    {
        if( Line[0] != '$' )
            continue;
        if( Line[1] == 'E' )
            break; ;
        if( Line[1] == 'M' ) /* Debut decription 1 module */
        {
            NewMod = new ModList();
            NewMod->Next = MList;
            MList = NewMod;
            while( GetLine( LibDoc, Line, NULL, sizeof(Line) - 1 ) )
            {
                if( Line[0] ==  '$' ) /* $EndMODULE */
                    break;

                switch( Line[0] )
                {
                case 'L':       /* LibName */
                    NewMod->m_Name = CONV_FROM_UTF8( StrPurge( Line + 3 ) );
                    break;

                case 'K':       /* KeyWords */
                    NewMod->m_KeyWord = CONV_FROM_UTF8( StrPurge( Line + 3 ) );
                    break;

                case 'C':       /* Doc */
                    NewMod->m_Doc = CONV_FROM_UTF8( StrPurge( Line + 3 ) );
                    break;
                }
            }
        } /* End read 1 module. */
    }

    fclose( LibDoc );
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
