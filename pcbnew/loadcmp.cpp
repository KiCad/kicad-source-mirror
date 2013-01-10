/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2012 Jean-Pierre Charras, jean-pierre.charras@ujf-grenoble.fr
 * Copyright (C) 2012 SoftPLC Corporation, Dick Hollenbeck <dick@softplc.com>
 * Copyright (C) 1992-2012 KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, you may find one here:
 * http://www.gnu.org/licenses/old-licenses/gpl-2.0.html
 * or you may search the http://www.gnu.org website for the version 2 license,
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

/**
 * @file pcbnew/loadcmp.cpp
 * @brief Footprints selection and loading functions.
 */

#include <fctsys.h>
#include <class_drawpanel.h>
#include <confirm.h>
#include <eda_doc.h>
#include <kicad_string.h>
#include <appl_wxstruct.h>
#include <wxPcbStruct.h>
#include <dialog_helpers.h>
#include <filter_reader.h>
#include <gr_basic.h>
#include <macros.h>
#include <pcbcommon.h>

#include <class_board.h>
#include <class_module.h>
#include <io_mgr.h>

#include <pcbnew.h>
#include <module_editor_frame.h>
#include <footprint_info.h>
#include <dialog_get_component.h>
#include <modview_frame.h>
#include <wildcards_and_files_ext.h>


static void DisplayCmpDoc( wxString& Name );

static FOOTPRINT_LIST MList;


bool FOOTPRINT_EDIT_FRAME::Load_Module_From_BOARD( MODULE* aModule )
{
    MODULE* newModule;
    PCB_BASE_FRAME* parent = (PCB_BASE_FRAME*) GetParent();

    if( aModule == NULL )
    {
        if( ! parent->GetBoard() || ! parent->GetBoard()->m_Modules )
            return false;

        aModule = Select_1_Module_From_BOARD( parent->GetBoard() );
    }

    if( aModule == NULL )
        return false;

    SetCurItem( NULL );

    Clear_Pcb( false );

    GetBoard()->m_Status_Pcb = 0;
    newModule = new MODULE( *aModule );
    newModule->SetParent( GetBoard() );
    newModule->m_Link = aModule->GetTimeStamp();

    aModule = newModule;

    GetBoard()->Add( aModule );

    aModule->ClearFlags();

    GetBoard()->BuildListOfNets();

    GetScreen()->SetCrossHairPosition( wxPoint( 0, 0 ) );
    PlaceModule( aModule, NULL );

    // Put it on FRONT layer,
    // because this is the default in ModEdit, and in libs
    if( aModule->GetLayer() != LAYER_N_FRONT )
        aModule->Flip( aModule->m_Pos );

    // Put it in orientation 0,
    // because this is the default orientation in ModEdit, and in libs
    Rotate_Module( NULL, aModule, 0, false );
    GetScreen()->ClrModify();
    Zoom_Automatique( false );

    return true;
}

/*
 * Launch the footprint viewer to select the name of a footprint to load.
 * return the selected footprint name
 */
wxString PCB_BASE_FRAME::SelectFootprintFromLibBrowser( void )
{
    wxSemaphore semaphore( 0, 1 );

    // Close the current Lib browser, if opened, and open a new one, in "modal" mode:
    FOOTPRINT_VIEWER_FRAME * viewer = FOOTPRINT_VIEWER_FRAME::GetActiveFootprintViewer();

    if( viewer )
        viewer->Destroy();

    viewer = new FOOTPRINT_VIEWER_FRAME( this, &semaphore,
                 KICAD_DEFAULT_DRAWFRAME_STYLE | wxFRAME_FLOAT_ON_PARENT );

    // Show the library viewer frame until it is closed
    while( semaphore.TryWait() == wxSEMA_BUSY ) // Wait for viewer closing event
    {
        wxYield();
        wxMilliSleep( 50 );
    }

    wxString fpname = viewer->GetSelectedFootprint();
    viewer->Destroy();

    return fpname;
}


MODULE* PCB_BASE_FRAME::Load_Module_From_Library( const wxString& aLibrary,
                                                  bool aUseFootprintViewer,
                                                  wxDC* aDC )
{
    MODULE*     module;
    wxPoint     curspos = GetScreen()->GetCrossHairPosition();
    wxString    moduleName, keys;
    bool        allowWildSeach = true;

    static wxArrayString HistoryList;
    static wxString      lastComponentName;

    // Ask for a component name or key words
    DIALOG_GET_COMPONENT dlg( this, HistoryList,
                          _( "Load Module" ), aUseFootprintViewer );

    dlg.SetComponentName( lastComponentName );

    if( dlg.ShowModal() == wxID_CANCEL )
        return NULL;

    if( dlg.m_GetExtraFunction )
    {
        moduleName = SelectFootprintFromLibBrowser();
    }
    else
    {
        moduleName = dlg.GetComponentName();
    }

    if( moduleName.IsEmpty() )  // Cancel command
    {
        m_canvas->MoveCursorToCrossHair();
        return NULL;
    }

    if( dlg.IsKeyword() )   // Selection by keywords
    {
        allowWildSeach = false;
        keys = moduleName;
        moduleName = Select_1_Module_From_List( this, aLibrary, wxEmptyString, keys );

        if( moduleName.IsEmpty() )  // Cancel command
        {
            m_canvas->MoveCursorToCrossHair();
            return NULL;
        }
    }
    else if( ( moduleName.Contains( wxT( "?" ) ) )
            || ( moduleName.Contains( wxT( "*" ) ) ) )  // Selection wild card
    {
        allowWildSeach = false;
        moduleName     = Select_1_Module_From_List( this, aLibrary, moduleName, wxEmptyString );

        if( moduleName.IsEmpty() )
        {
            m_canvas->MoveCursorToCrossHair();
            return NULL;    // Cancel command.
        }
    }

    module = GetModuleLibrary( aLibrary, moduleName, false );

    if( !module && allowWildSeach )    // Search with wild card
    {
        allowWildSeach = false;

        wxString wildname = wxChar( '*' ) + moduleName + wxChar( '*' );
        moduleName = wildname;

        moduleName = Select_1_Module_From_List( this, aLibrary, moduleName, wxEmptyString );

        if( moduleName.IsEmpty() )
        {
            m_canvas->MoveCursorToCrossHair();
            return NULL;    // Cancel command.
        }
        else
        {
            module = GetModuleLibrary( aLibrary, moduleName, true );
        }
    }

    GetScreen()->SetCrossHairPosition( curspos );
    m_canvas->MoveCursorToCrossHair();

    if( module )
    {
        lastComponentName = moduleName;
        AddHistoryComponentName( HistoryList, moduleName );

        module->SetFlags( IS_NEW );
        module->m_Link = 0;

        module->SetTimeStamp( GetNewTimeStamp() );
        GetBoard()->m_Status_Pcb = 0;

        module->SetPosition( curspos );

        // Put it on FRONT layer,
        // (Can be stored flipped if the lib is an archive built from a board)
        if( module->IsFlipped() )
            module->Flip( module->m_Pos );

        // Place it in orientation 0,
        // even if it is not saved with orientation 0 in lib
        // (Can happen if the lib is an archive built from a board)
        Rotate_Module( NULL, module, 0, false );

        RecalculateAllTracksNetcode();

        if( aDC )
            module->Draw( m_canvas, aDC, GR_OR );
    }

    return module;
}


/* scans active libraries to find and load aFootprintName.
 * If found  the module is added to the BOARD, just for good measure.
 *  aLibraryPath is the full/short name of the library.
 *                      if empty, search in all libraries
 *  aFootprintName is the footprint to load
 *  aDisplayError = true to display an error message if any.
 *
 *  return a pointer to the new module, or NULL
 */
MODULE* PCB_BASE_FRAME::GetModuleLibrary( const wxString& aLibraryPath,
                                          const wxString& aFootprintName,
                                          bool aDisplayError )
{
    if( aLibraryPath.IsEmpty() )
        return loadFootprintFromLibraries( aFootprintName, aDisplayError );
    else
        return loadFootprintFromLibrary( aLibraryPath, aFootprintName, aDisplayError );
}


/* loads aFootprintName from aLibraryPath.
 * If found the module is added to the BOARD, just for good measure.
 *
 * aLibraryPath - the full filename or the short name of the library to read.
 * if it is a short name, the file is searched in all library valid paths
 */
MODULE* PCB_BASE_FRAME::loadFootprintFromLibrary( const wxString& aLibraryPath,
                                                  const wxString& aFootprintName,
                                                  bool            aDisplayError,
                                                  bool            aAddToBoard )
{
    try
    {
        PLUGIN::RELEASER pi( IO_MGR::PluginFind( IO_MGR::LEGACY ) );

        wxString libPath = wxGetApp().FindLibraryPath( aLibraryPath );

        MODULE* footprint = pi->FootprintLoad( libPath, aFootprintName );

        if( !footprint )
        {
            if( aDisplayError )
            {
                wxString msg = wxString::Format(
                    _( "Footprint '%s' not found in library '%s'" ),
                    aFootprintName.GetData(),
                    libPath.GetData() );

                DisplayError( NULL, msg );
            }

            return NULL;
        }

        if( aAddToBoard )
            GetBoard()->Add( footprint, ADD_APPEND );

        SetStatusText( wxEmptyString );
        return footprint;
    }
    catch( IO_ERROR ioe )
    {
        DisplayError( this, ioe.errorText );
        return NULL;
    }
}


/* Explore the libraries list and
 * loads aFootprintName from the first library it is found
 * If found add the module is also added to the BOARD, just for good measure.
 */
MODULE* PCB_BASE_FRAME::loadFootprintFromLibraries(
        const wxString& aFootprintName, bool aDisplayError )
{
    bool    showed_error = false;
    MODULE* footprint = NULL;

    try
    {
        PLUGIN::RELEASER pi( IO_MGR::PluginFind( IO_MGR::LEGACY ) );

        for( unsigned ii = 0; ii < g_LibraryNames.GetCount(); ii++ )
        {
            wxFileName fn = wxFileName( wxEmptyString, g_LibraryNames[ii], LegacyFootprintLibPathExtension );

            wxString libPath = wxGetApp().FindLibraryPath( fn );

            if( !libPath )
            {
                if( aDisplayError && !showed_error )
                {
                    wxString msg = wxString::Format(
                        _( "PCB footprint library file <%s> not found in search paths." ),
                        fn.GetFullName().GetData() );

                    DisplayError( this, msg );
                    showed_error = true;
                }
                continue;
            }

            footprint = pi->FootprintLoad( libPath, aFootprintName );

            if( footprint )
            {
                GetBoard()->Add( footprint, ADD_APPEND );
                SetStatusText( wxEmptyString );
                return footprint;
            }
        }

        if( !footprint )
        {
            if( aDisplayError )
            {
                wxString msg = wxString::Format(
                    _( "Footprint '%s' not found in any library" ),
                    aFootprintName.GetData() );

                DisplayError( NULL, msg );
            }

            return NULL;
        }
    }
    catch( IO_ERROR ioe )
    {
        DisplayError( this, ioe.errorText );
    }
    return NULL;
}


wxString PCB_BASE_FRAME::Select_1_Module_From_List( EDA_DRAW_FRAME* aWindow,
                                                    const wxString& aLibraryFullFilename,
                                                    const wxString& aMask,
                                                    const wxString& aKeyWord )
{
    static wxString OldName;    // Save the name of the last module loaded.
    wxString        CmpName;
    wxString        msg;
    wxArrayString   libnames_list;

    if( aLibraryFullFilename.IsEmpty() )
        libnames_list = g_LibraryNames;
    else
        libnames_list.Add( aLibraryFullFilename );

    // Find modules in libraries.
    MList.ReadFootprintFiles( libnames_list );

    wxArrayString footprint_names_list;

    if( !aKeyWord.IsEmpty() ) // Create a list of modules found by keyword.
    {
        for( unsigned ii = 0; ii < MList.GetCount(); ii++ )
        {
            if( KeyWordOk( aKeyWord, MList.GetItem(ii).m_KeyWord ) )
                footprint_names_list.Add( MList.GetItem(ii).m_Module );
        }
    }
    else if( !aMask.IsEmpty() ) // Create a list of modules found by pattern
    {
        for( unsigned ii = 0; ii < MList.GetCount(); ii++ )
        {
            wxString& candidate = MList.GetItem(ii).m_Module;

            if( WildCompareString( aMask, candidate, false ) )
                footprint_names_list.Add( candidate );
        }
    }
    else        // Create the full list of modules
    {
        for( unsigned ii = 0; ii < MList.GetCount(); ii++ )
            footprint_names_list.Add( MList.GetItem(ii).m_Module );
    }

    if( footprint_names_list.GetCount() )
    {
        msg.Printf( _( "Modules [%d items]" ), (int) footprint_names_list.GetCount() );
        EDA_LIST_DIALOG dlg( aWindow, msg, footprint_names_list, OldName,
                             DisplayCmpDoc );

        if( dlg.ShowModal() == wxID_OK )
        {
            CmpName = dlg.GetTextSelection();
            SkipNextLeftButtonReleaseEvent();
        }
        else
            CmpName.Empty();
    }
    else
    {
        DisplayError( aWindow, _( "No footprint found" ) );
        CmpName.Empty();
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


MODULE* FOOTPRINT_EDIT_FRAME::Select_1_Module_From_BOARD( BOARD* aPcb )
{
    MODULE*         module;
    static wxString OldName;       // Save name of last module selected.
    wxString        CmpName, msg;

    wxArrayString listnames;

    module = aPcb->m_Modules;

    for( ; module != NULL; module = (MODULE*) module->Next() )
        listnames.Add( module->m_Reference->m_Text );

    msg.Printf( _( "Modules [%d items]" ), listnames.GetCount() );

    EDA_LIST_DIALOG dlg( this, msg, listnames, wxEmptyString, NULL, SORT_LIST );

    if( dlg.ShowModal() == wxID_OK )
        CmpName = dlg.GetTextSelection();
    else
        return NULL;

    OldName = CmpName;

    module = aPcb->m_Modules;

    for( ; module != NULL; module = (MODULE*) module->Next() )
    {
        if( CmpName == module->m_Reference->m_Text )
            break;
    }

    return module;
}


void FOOTPRINT_EDIT_FRAME::OnSaveLibraryAs( wxCommandEvent& aEvent )
{
    wxString    curLibPath = getLibPath();
    wxString    dstLibPath = CreateNewLibrary();

    if( !dstLibPath )
        return;             // user aborted in CreateNewLibrary()

    IO_MGR::PCB_FILE_T  dstType = IO_MGR::GuessPluginTypeFromLibPath( dstLibPath );
    IO_MGR::PCB_FILE_T  curType = IO_MGR::GuessPluginTypeFromLibPath( curLibPath );

    try
    {
        PLUGIN::RELEASER cur( IO_MGR::PluginFind( curType ) );
        PLUGIN::RELEASER dst( IO_MGR::PluginFind( dstType ) );

        wxArrayString mods = cur->FootprintEnumerate( curLibPath );

        for( unsigned i = 0;  i < mods.size();  ++i )
        {
            std::auto_ptr<MODULE> m( cur->FootprintLoad( curLibPath, mods[i] ) );
            dst->FootprintSave( dstLibPath, m.get() );

            // m is deleted here by auto_ptr.
        }
    }
    catch( IO_ERROR ioe )
    {
        DisplayError( this, ioe.errorText );
        return;
    }

    wxString msg = wxString::Format(
                    _( "Footprint library\n'%s' saved as\n'%s'" ),
                    GetChars( curLibPath ), GetChars( dstLibPath ) );

    DisplayInfoMessage( this, msg );
}
