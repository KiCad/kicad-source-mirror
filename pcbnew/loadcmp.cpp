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
#include <fp_lib_table.h>
#include <fpid.h>

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

        aModule = SelectFootprint( parent->GetBoard() );
    }

    if( aModule == NULL )
        return false;

    SetCurItem( NULL );

    Clear_Pcb( false );

    GetBoard()->m_Status_Pcb = 0;
    newModule = new MODULE( *aModule );
    newModule->SetParent( GetBoard() );
    newModule->SetLink( aModule->GetTimeStamp() );

    aModule = newModule;

    GetBoard()->Add( aModule );

    aModule->ClearFlags();

    GetBoard()->BuildListOfNets();

    SetCrossHairPosition( wxPoint( 0, 0 ) );
    PlaceModule( aModule, NULL );

    // Put it on FRONT layer,
    // because this is the default in ModEdit, and in libs
    if( aModule->GetLayer() != LAYER_N_FRONT )
        aModule->Flip( aModule->GetPosition() );

    // Put it in orientation 0,
    // because this is the default orientation in ModEdit, and in libs
    Rotate_Module( NULL, aModule, 0, false );
    GetScreen()->ClrModify();
    Zoom_Automatique( false );

    return true;
}


wxString PCB_BASE_FRAME::SelectFootprintFromLibBrowser()
{
    wxString    fpname;
    wxString    fpid;

    wxSemaphore semaphore( 0, 1 );

    // Close the current Lib browser, if opened, and open a new one, in "modal" mode:
    FOOTPRINT_VIEWER_FRAME * viewer = FOOTPRINT_VIEWER_FRAME::GetActiveFootprintViewer();

    if( viewer )
        viewer->Destroy();

    viewer = new FOOTPRINT_VIEWER_FRAME( this, m_footprintLibTable, &semaphore,
                                         KICAD_DEFAULT_DRAWFRAME_STYLE | wxFRAME_FLOAT_ON_PARENT );

    // Show the library viewer frame until it is closed
    while( semaphore.TryWait() == wxSEMA_BUSY ) // Wait for viewer closing event
    {
        wxYield();
        wxMilliSleep( 50 );
    }

    fpname = viewer->GetSelectedFootprint();

    if( !!fpname )
    {
        fpid = viewer->GetSelectedLibrary() + wxT( ":" ) + fpname;
    }

    viewer->Destroy();

    return fpid;
}


MODULE* PCB_BASE_FRAME::LoadModuleFromLibrary( const wxString& aLibrary,
                                               FP_LIB_TABLE*   aTable,
                                               bool            aUseFootprintViewer,
                                               wxDC*           aDC )
{
    MODULE*     module = NULL;
    wxPoint     curspos = GetCrossHairPosition();
    wxString    moduleName, keys;
    wxString    libName = aLibrary;
    bool        allowWildSeach = true;

    static wxArrayString HistoryList;
    static wxString      lastComponentName;

    // Ask for a component name or key words
    DIALOG_GET_COMPONENT dlg( this, HistoryList, _( "Load Module" ), aUseFootprintViewer );

    dlg.SetComponentName( lastComponentName );

    if( dlg.ShowModal() == wxID_CANCEL )
        return NULL;

    if( dlg.m_GetExtraFunction )
    {
        // SelectFootprintFromLibBrowser() returns the "full" footprint name, i.e.
        // <lib_name>/<footprint name> or FPID format "lib_name:fp_name:rev#"
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

    if( dlg.IsKeyword() )                          // Selection by keywords
    {
        allowWildSeach = false;
        keys = moduleName;
        moduleName = SelectFootprint( this, libName, wxEmptyString, keys, aTable );

        if( moduleName.IsEmpty() )                 // Cancel command
        {
            m_canvas->MoveCursorToCrossHair();
            return NULL;
        }
    }
    else if( moduleName.Contains( wxT( "?" ) )
           || moduleName.Contains( wxT( "*" ) ) )  // Selection wild card
    {
        allowWildSeach = false;
        moduleName     = SelectFootprint( this, libName, moduleName, wxEmptyString, aTable );

        if( moduleName.IsEmpty() )
        {
            m_canvas->MoveCursorToCrossHair();
            return NULL;                           // Cancel command.
        }
    }

    FPID fpid;

    wxCHECK_MSG( fpid.Parse( moduleName ) < 0, NULL,
                 wxString::Format( wxT( "Could not parse FPID string '%s'." ),
                                   GetChars( moduleName ) ) );

    try
    {
        module = loadFootprint( fpid );
    }
    catch( IO_ERROR ioe )
    {
        wxLogDebug( wxT( "An error occurred attemping to load footprint '%s'.\n\nError: %s" ),
                    fpid.Format().c_str(), GetChars( ioe.errorText ) );
    }

    if( !module && allowWildSeach )                // Search with wild card
    {
        allowWildSeach = false;

        wxString wildname = wxChar( '*' ) + moduleName + wxChar( '*' );
        moduleName = wildname;

        moduleName = SelectFootprint( this, libName, moduleName, wxEmptyString, aTable );

        if( moduleName.IsEmpty() )
        {
            m_canvas->MoveCursorToCrossHair();
            return NULL;    // Cancel command.
        }
        else
        {
            FPID fpid;

            wxCHECK_MSG( fpid.Parse( moduleName ) < 0, NULL,
                         wxString::Format( wxT( "Could not parse FPID string '%s'." ),
                                           GetChars( moduleName ) ) );

            try
            {
                module = loadFootprint( fpid );
            }
            catch( IO_ERROR ioe )
            {
                wxLogDebug( wxT( "An error occurred attemping to load footprint '%s'.\n\nError: %s" ),
                            fpid.Format().c_str(), GetChars( ioe.errorText ) );
            }
        }
    }

    SetCrossHairPosition( curspos );
    m_canvas->MoveCursorToCrossHair();

    if( module )
    {
        GetBoard()->Add( module, ADD_APPEND );
        lastComponentName = moduleName;
        AddHistoryComponentName( HistoryList, moduleName );

        module->SetFlags( IS_NEW );
        module->SetLink( 0 );
        module->SetPosition( curspos );
        module->SetTimeStamp( GetNewTimeStamp() );
        GetBoard()->m_Status_Pcb = 0;


        // Put it on FRONT layer,
        // (Can be stored flipped if the lib is an archive built from a board)
        if( module->IsFlipped() )
            module->Flip( module->GetPosition() );

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


MODULE* PCB_BASE_FRAME::GetModuleLibrary( const wxString& aLibraryPath,
                                          const wxString& aFootprintName,
                                          bool            aDisplayError )
{
    if( aLibraryPath.IsEmpty() )
        return loadFootprintFromLibraries( aFootprintName, aDisplayError );
    else
        return loadFootprintFromLibrary( aLibraryPath, aFootprintName, aDisplayError );
}


MODULE* PCB_BASE_FRAME::loadFootprintFromLibrary( const wxString& aLibraryPath,
                                                  const wxString& aFootprintName,
                                                  bool            aDisplayError )
{
    try
    {
        PLUGIN::RELEASER pi( IO_MGR::PluginFind( IO_MGR::LEGACY ) );

        // Ensure the library name has the right extension
        // (sometimes the name is given without ext)
        wxString libname = aLibraryPath;

        if( !libname.EndsWith( wxT(".") + LegacyFootprintLibPathExtension) )
            libname <<  wxT(".") << LegacyFootprintLibPathExtension;

        wxString libPath = wxGetApp().FindLibraryPath( libname );

        if( libPath.IsEmpty() )
        {
            wxString msg = wxString::Format( _( "Library '%s' not found." ),
                                             libname.GetData() );
            DisplayError( NULL, msg );
            return NULL;
        }

        MODULE* footprint = pi->FootprintLoad( libPath, aFootprintName );

        if( !footprint )
        {
            if( aDisplayError )
            {
                wxString msg = wxString::Format(
                    _( "Footprint '%s' not found in library '%s'." ),
                    aFootprintName.GetData(),
                    libPath.GetData() );

                DisplayError( NULL, msg );
            }

            return NULL;
        }

        SetStatusText( wxEmptyString );
        return footprint;
    }
    catch( IO_ERROR ioe )
    {
        DisplayError( this, ioe.errorText );
        return NULL;
    }
}


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
            wxFileName fn = wxFileName( wxEmptyString, g_LibraryNames[ii],
                                        LegacyFootprintLibPathExtension );

            wxString libPath = wxGetApp().FindLibraryPath( fn );

            if( !libPath )
            {
                if( aDisplayError && !showed_error )
                {
                    wxString msg = wxString::Format(
                        _( "PCB footprint library file '%s' not found in search paths." ),
                        fn.GetFullName().GetData() );

                    DisplayError( this, msg );
                    showed_error = true;
                }

                continue;
            }

            footprint = pi->FootprintLoad( libPath, aFootprintName );

            if( footprint )
            {
                SetStatusText( wxEmptyString );
                return footprint;
            }
        }

        if( !footprint )
        {
            if( aDisplayError )
            {
                wxString msg = wxString::Format(
                    _( "Footprint %s not found in any library." ),
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

/* attempts to load aFootprintId from the footprint library table.
 * return the #MODULE if found or NULL if not found or error.
 */
MODULE* PCB_BASE_FRAME::LoadFootprint( const FPID& aFootprintId )
{
    MODULE* module = NULL;

    try
    {
        module = loadFootprint( aFootprintId );
    }
    catch( IO_ERROR ioe )
    {
        wxLogDebug( wxT( "An error occurred attemping to load footprint '%s'.\n\nError: %s" ),
                    aFootprintId.Format().c_str(), GetChars( ioe.errorText ) );
    }

    return module;
}


MODULE* PCB_BASE_FRAME::loadFootprint( const FPID& aFootprintId )
    throw( IO_ERROR, PARSE_ERROR )
{
    wxCHECK_MSG( m_footprintLibTable != NULL, NULL,
                 wxT( "Cannot look up FPID in NULL FP_LIB_TABLE." ) );

    wxString   nickname = aFootprintId.GetLibNickname();
    wxString   fpname   = aFootprintId.GetFootprintName();

    if( nickname.size() )
    {
        return m_footprintLibTable->FootprintLoad( nickname, fpname );
    }

    // user did not enter a nickname, just a footprint name, help him out a little:
    else
    {
        std::vector<wxString> nicks = m_footprintLibTable->GetLogicalLibs();

        // Search each library going through libraries alphabetically.
        for( unsigned i = 0;  i<nicks.size();  ++i )
        {
            // FootprintLoad() returns NULL on not found, does not throw exception
            // unless there's an IO_ERROR.
            MODULE* ret = m_footprintLibTable->FootprintLoad( nicks[i], fpname );
            if( ret )
                return ret;
        }

        return NULL;
    }
}


wxString PCB_BASE_FRAME::SelectFootprint( EDA_DRAW_FRAME* aWindow,
                                          const wxString& aLibraryName,
                                          const wxString& aMask,
                                          const wxString& aKeyWord,
                                          FP_LIB_TABLE*   aTable )
{
    static wxString oldName;    // Save the name of the last module loaded.

    wxString        fpname;
    wxString        msg;
    wxArrayString   libraries;
    FP_LIB_TABLE    libTable;

    std::vector< wxArrayString > rows;

    wxASSERT( aTable != NULL );

    MList.ReadFootprintFiles( aTable, !aLibraryName ? NULL : &aLibraryName );

    if( MList.GetErrorCount() )
    {
        MList.DisplayErrors( this );
        return wxEmptyString;
    }

    if( MList.GetCount() == 0 )
    {
        wxString tmp;

        for( unsigned i = 0;  i < libraries.GetCount();  i++ )
        {
            tmp += libraries[i] + wxT( "\n" );
        }

        msg.Printf( _( "No footprints could be read from library file(s):\n\n%s\nin any of "
                       "the library search paths.  Verify your system is configured properly "
                       "so the footprint libraries can be found." ), GetChars( tmp ) );
        DisplayError( aWindow, msg );
        return wxEmptyString;
    }

    if( !aKeyWord.IsEmpty() )       // Create a list of modules found by keyword.
    {
        for( unsigned ii = 0; ii < MList.GetCount(); ii++ )
        {
            if( KeyWordOk( aKeyWord, MList.GetItem( ii ).GetKeywords() ) )
            {
                wxArrayString   cols;
                cols.Add( MList.GetItem( ii ).GetFootprintName() );
                cols.Add( MList.GetItem( ii ).GetNickname() );
                rows.push_back( cols );
            }
        }
    }
    else if( !aMask.IsEmpty() )     // Create a list of modules found by pattern
    {
        for( unsigned ii = 0; ii < MList.GetCount(); ii++ )
        {
            const wxString& candidate = MList.GetItem( ii ).GetFootprintName();

            if( WildCompareString( aMask, candidate, false ) )
            {
                wxArrayString   cols;
                cols.Add( MList.GetItem( ii ).GetFootprintName() );
                cols.Add( MList.GetItem( ii ).GetNickname() );
                rows.push_back( cols );
            }
        }
    }
    else                            // Create the full list of modules
    {
        for( unsigned ii = 0; ii < MList.GetCount(); ii++ )
        {
            wxArrayString   cols;
            cols.Add( MList.GetItem( ii ).GetFootprintName() );
            cols.Add( MList.GetItem( ii ).GetNickname() );
            rows.push_back( cols );
        }
    }

    if( !rows.empty() )
    {
        wxArrayString headers;

        headers.Add( _( "Module" ) );
        headers.Add( _( "Library" ) );

        msg.Printf( _( "Modules [%d items]" ), (int) rows.size() );

        EDA_LIST_DIALOG dlg( aWindow, msg, headers, rows, oldName, DisplayCmpDoc );

        if( dlg.ShowModal() == wxID_OK )
        {
            fpname = dlg.GetTextSelection();

            fpname = dlg.GetTextSelection( 1 ) + wxT( ":" ) + fpname;

            SkipNextLeftButtonReleaseEvent();
        }
        else
            fpname.Empty();
    }
    else
    {
        DisplayError( aWindow, _( "No footprint found." ) );
        fpname.Empty();
    }

    if( fpname != wxEmptyString )
        oldName = fpname;

    wxLogDebug( wxT( "Footprint '%s' was selected." ), GetChars( fpname ) );

    return fpname;
}


static void DisplayCmpDoc( wxString& aName )
{
    FOOTPRINT_INFO* module_info = MList.GetModuleInfo( aName );

    if( !module_info )
    {
        aName.Empty();
        return;
    }

    aName  = _( "Description: " ) + module_info->GetDoc();
    aName += _( "\nKey words: " ) + module_info->GetKeywords();
}


MODULE* FOOTPRINT_EDIT_FRAME::SelectFootprint( BOARD* aPcb )
{
    static wxString oldName;       // Save name of last module selected.

    wxString        fpname;
    wxString        msg;
    wxArrayString   listnames;
    MODULE*         module = aPcb->m_Modules;

    for(  ; module;  module = module->Next() )
        listnames.Add( module->GetReference() );

    msg.Printf( _( "Modules [%d items]" ), listnames.GetCount() );

    wxArrayString headers;

    headers.Add( _( "Module" ) );

    std::vector<wxArrayString> itemsToDisplay;

    // Conversion from wxArrayString to vector of ArrayString
    for( unsigned i = 0; i < listnames.GetCount(); i++ )
    {
        wxArrayString item;

        item.Add( listnames[i] );
        itemsToDisplay.push_back( item );
    }

    EDA_LIST_DIALOG dlg( this, msg, headers, itemsToDisplay, wxEmptyString, NULL, SORT_LIST );

    if( dlg.ShowModal() == wxID_OK )
        fpname = dlg.GetTextSelection();
    else
        return NULL;

    oldName = fpname;

    module = aPcb->m_Modules;

    for(  ;  module;  module = module->Next() )
    {
        if( fpname == module->GetReference() )
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
                    _( "Footprint library '%s' saved as '%s'." ),
                    GetChars( curLibPath ), GetChars( dstLibPath ) );

    DisplayInfoMessage( this, msg );
}
