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

    GetScreen()->SetCrossHairPosition( wxPoint( 0, 0 ) );
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

/*
 * Launch the footprint viewer to select the name of a footprint to load.
 * return the selected footprint name
 */
wxString PCB_BASE_FRAME::SelectFootprintFromLibBrowser( void )
{
    wxString    fpname;
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

#if !defined( USE_FP_LIB_TABLE )
    // Returns the full fp name, i.e. the lib name and th fp name,
    // separated by a '/' (/ is now an illegal char in fp names)
    fpname = viewer->GetSelectedLibraryFullName() + wxT( "/" ) + viewer->GetSelectedFootprint();
#else
    fpname = viewer->GetSelectedLibrary() + wxT( ":" ) + viewer->GetSelectedFootprint();
#endif

    viewer->Destroy();

    return fpname;
}


MODULE* PCB_BASE_FRAME::LoadModuleFromLibrary( const wxString& aLibrary,
                                               FP_LIB_TABLE*   aTable,
                                               bool            aUseFootprintViewer,
                                               wxDC*           aDC )
{
    MODULE*     module;
    wxPoint     curspos = GetScreen()->GetCrossHairPosition();
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
#if !defined( USE_FP_LIB_TABLE )
        wxString full_fpname = SelectFootprintFromLibBrowser();
        moduleName = full_fpname.AfterLast( '/' );
        libName = full_fpname.BeforeLast( '/' );
#else
        libName = SelectFootprintFromLibBrowser();
#endif
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

    module = GetModuleLibrary( libName, moduleName, false );

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
            module = GetModuleLibrary( libName, moduleName, true );
        }
    }

    GetScreen()->SetCrossHairPosition( curspos );
    m_canvas->MoveCursorToCrossHair();

    if( module )
    {
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
                    _( "Footprint %s not found in library <%s>." ),
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


MODULE* PCB_BASE_FRAME::loadFootprint( const FPID& aFootprintId )
    throw( IO_ERROR, PARSE_ERROR )
{
    wxCHECK_MSG( m_footprintLibTable != NULL, NULL,
                 wxT( "Cannot look up FPID in NULL FP_LIB_TABLE." ) );

    wxString   libName = FROM_UTF8( aFootprintId.GetLibNickname().c_str() );

    const FP_LIB_TABLE::ROW* row = m_footprintLibTable->FindRow( libName );

    if( row == NULL )
    {
        wxString msg;
        msg.Printf( _( "No library named <%s> was found in the footprint library table." ),
                    aFootprintId.GetLibNickname().c_str() );
        THROW_IO_ERROR( msg );
    }

    wxString   footprintName = FROM_UTF8( aFootprintId.GetFootprintName().c_str() );
    wxString   libPath = row->GetFullURI();

    libPath = FP_LIB_TABLE::ExpandSubstitutions( libPath );

    PLUGIN::RELEASER pi( IO_MGR::PluginFind( IO_MGR::EnumFromStr( row->GetType() ) ) );

    return pi->FootprintLoad( libPath, footprintName );
}


wxString PCB_BASE_FRAME::SelectFootprint( EDA_DRAW_FRAME* aWindow,
                                          const wxString& aLibraryFullFilename,
                                          const wxString& aMask,
                                          const wxString& aKeyWord,
                                          FP_LIB_TABLE*   aTable )
{
    static wxString              OldName;    // Save the name of the last module loaded.
    wxString                     CmpName;
    wxString                     msg;
    wxArrayString                libraries;
    std::vector< wxArrayString > rows;


    if( aLibraryFullFilename.IsEmpty() )
    {
#if !defined( USE_FP_LIB_TABLE )
        libraries = g_LibraryNames;
#else
        wxASSERT( aTable != NULL );

        std::vector< wxString > libNames = aTable->GetLogicalLibs();

        for( unsigned i = 0; i < libNames.size(); i++ )
        {
            wxString uri = aTable->FindRow( libNames[i] )->GetFullURI();
            uri = FP_LIB_TABLE::ExpandSubstitutions( uri );
            libraries.Add( uri );
        }
#endif
    }
    else
    {
        libraries.Add( aLibraryFullFilename );
    }

    if( libraries.IsEmpty() )
    {
        DisplayError( aWindow, _( "No footprint libraries were specified." ) );
        return wxEmptyString;
    }

    // Find modules in libraries.
    MList.ReadFootprintFiles( libraries );

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
            if( KeyWordOk( aKeyWord, MList.GetItem( ii ).m_KeyWord ) )
            {
                wxArrayString   cols;
                cols.Add( MList.GetItem( ii ).GetFootprintName() );
                cols.Add( MList.GetItem( ii ).GetLibraryName() );
                rows.push_back( cols );
            }
        }
    }
    else if( !aMask.IsEmpty() )     // Create a list of modules found by pattern
    {
        for( unsigned ii = 0; ii < MList.GetCount(); ii++ )
        {
            wxString& candidate = MList.GetItem( ii ).m_Module;

            if( WildCompareString( aMask, candidate, false ) )
            {
                wxArrayString   cols;
                cols.Add( MList.GetItem( ii ).GetFootprintName() );
                cols.Add( MList.GetItem( ii ).GetLibraryName() );
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
            cols.Add( MList.GetItem( ii ).GetLibraryName() );
            rows.push_back( cols );
        }
    }

    if( !rows.empty() )
    {
        wxArrayString headers;

        headers.Add( _( "Module" ) );
        headers.Add( _( "Library" ) );

        msg.Printf( _( "Modules [%d items]" ), (int) rows.size() );
        EDA_LIST_DIALOG dlg( aWindow, msg, headers, rows, OldName, DisplayCmpDoc );

        if( dlg.ShowModal() == wxID_OK )
        {
            CmpName = dlg.GetTextSelection();

#if defined( USE_FP_LIB_TABLE )
            CmpName += wxT( ":" ) + dlg.GetTextSelection( 1 );
#endif

            SkipNextLeftButtonReleaseEvent();
        }
        else
            CmpName.Empty();
    }
    else
    {
        DisplayError( aWindow, _( "No footprint found." ) );
        CmpName.Empty();
    }

    if( CmpName != wxEmptyString )
        OldName = CmpName;

    wxLogDebug( wxT( "Footprint <%s> was selected." ), GetChars( CmpName ) );

    return CmpName;
}


static void DisplayCmpDoc( wxString& Name )
{
    FOOTPRINT_INFO* module_info = MList.GetModuleInfo( Name );

    if( !module_info )
    {
        Name.Empty();
        return;
    }

    Name  = _( "Description: " ) + module_info->m_Doc;
    Name += _( "\nKey words: " ) + module_info->m_KeyWord;
}


MODULE* FOOTPRINT_EDIT_FRAME::SelectFootprint( BOARD* aPcb )
{
    MODULE*         module;
    static wxString OldName;       // Save name of last module selected.
    wxString        CmpName, msg;

    wxArrayString listnames;

    module = aPcb->m_Modules;

    for( ; module != NULL; module = (MODULE*) module->Next() )
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
        CmpName = dlg.GetTextSelection();
    else
        return NULL;

    OldName = CmpName;

    module = aPcb->m_Modules;

    for( ; module != NULL; module = (MODULE*) module->Next() )
    {
        if( CmpName == module->GetReference() )
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
                    _( "Footprint library <%s> saved as <%s>." ),
                    GetChars( curLibPath ), GetChars( dstLibPath ) );

    DisplayInfoMessage( this, msg );
}
