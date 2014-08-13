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
#include <pcb_draw_panel_gal.h>
#include <confirm.h>
#include <eda_doc.h>
#include <kicad_string.h>
#include <pgm_base.h>
#include <kiway.h>
//#include <frame_type.h>
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
#include <class_pcb_layer_widget.h>


static void DisplayCmpDoc( wxString& aName, void* aData );

static FOOTPRINT_LIST MList;


bool FOOTPRINT_EDIT_FRAME::Load_Module_From_BOARD( MODULE* aModule )
{
    MODULE* newModule;
    PCB_EDIT_FRAME* frame = (PCB_EDIT_FRAME*) Kiway().Player( FRAME_PCB, false );

    if( frame == NULL )     // happens if no board editor opened
        return false;

    if( aModule == NULL )
    {
        if( ! frame->GetBoard() || ! frame->GetBoard()->m_Modules )
            return false;

        aModule = SelectFootprint( frame->GetBoard() );
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

    GetBoard()->Add( newModule );

    newModule->ClearFlags();

    // Clear references to net info, because the footprint editor
    // does know any thing about nets handled by the current edited board.
    // Morever the main board can change or the net info relative to this main board
    // can change while editing this footprint in the footprint editor
    for( D_PAD* pad = newModule->Pads(); pad; pad = pad->Next() )
        pad->SetNetCode( NETINFO_LIST::UNCONNECTED );

    SetCrossHairPosition( wxPoint( 0, 0 ) );
    PlaceModule( newModule, NULL );
    newModule->SetPosition( wxPoint( 0, 0 ) ); // cursor in GAL may not be initialized at the moment

    // Put it on FRONT layer,
    // because this is the default in ModEdit, and in libs
    if( newModule->GetLayer() != F_Cu )
        newModule->Flip( newModule->GetPosition() );

    // Put it in orientation 0,
    // because this is the default orientation in ModEdit, and in libs
    Rotate_Module( NULL, newModule, 0, false );
    GetScreen()->ClrModify();
    Zoom_Automatique( false );

    if( IsGalCanvasActive() )
        updateView();

    return true;
}


wxString PCB_BASE_FRAME::SelectFootprintFromLibBrowser()
{
    // Close the current non-modal Lib browser if opened, and open a new one, in "modal" mode:
    FOOTPRINT_VIEWER_FRAME* viewer;

    viewer = (FOOTPRINT_VIEWER_FRAME*) Kiway().Player( FRAME_PCB_MODULE_VIEWER, false );

    if( viewer )
        viewer->Destroy();

    viewer = (FOOTPRINT_VIEWER_FRAME*) Kiway().Player( FRAME_PCB_MODULE_VIEWER_MODAL, true );

    wxString    fpid;

    viewer->ShowModal( &fpid, this );

    //DBG(printf("%s: fpid:'%s'\n", __func__, TO_UTF8( fpid ) );)

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
    catch( const IO_ERROR& ioe )
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
            catch( const IO_ERROR& ioe )
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

        if( IsGalCanvasActive() )
            module->SetPosition( wxPoint( 0, 0 ) ); // cursor in GAL may not be initialized at the moment
        else
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


MODULE* PCB_BASE_FRAME::LoadFootprint( const FPID& aFootprintId )
{
    MODULE* module = NULL;

    try
    {
        module = loadFootprint( aFootprintId );
    }
    catch( const IO_ERROR& ioe )
    {
        wxLogDebug( wxT( "An error occurred attemping to load footprint '%s'.\n\nError: %s" ),
                    aFootprintId.Format().c_str(), GetChars( ioe.errorText ) );
    }

    return module;
}


MODULE* PCB_BASE_FRAME::loadFootprint( const FPID& aFootprintId )
    throw( IO_ERROR, PARSE_ERROR )
{
    FP_LIB_TABLE*   fptbl = Prj().PcbFootprintLibs();

    wxCHECK_MSG( fptbl, NULL, wxT( "Cannot look up FPID in NULL FP_LIB_TABLE." ) );

    return fptbl->FootprintLoadWithOptionalNickname( aFootprintId );
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


static void DisplayCmpDoc( wxString& aName, void* aData )
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

    EDA_LIST_DIALOG dlg( this, msg, headers, itemsToDisplay, wxEmptyString, NULL, NULL, SORT_LIST );

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
    catch( const IO_ERROR& ioe )
    {
        DisplayError( this, ioe.errorText );
        return;
    }

    wxString msg = wxString::Format(
                    _( "Footprint library '%s' saved as '%s'." ),
                    GetChars( curLibPath ), GetChars( dstLibPath ) );

    DisplayInfoMessage( this, msg );
}
