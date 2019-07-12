/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2018 Jean-Pierre Charras, jean-pierre.charras@ujf-grenoble.fr
 * Copyright (C) 2012 SoftPLC Corporation, Dick Hollenbeck <dick@softplc.com>
 * Copyright (C) 1992-2018 KiCad Developers, see AUTHORS.txt for contributors.
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

#include <functional>
using namespace std::placeholders;

#include <fctsys.h>
#include <pcb_draw_panel_gal.h>
#include <confirm.h>
#include <eda_doc.h>
#include <kicad_string.h>
#include <pgm_base.h>
#include <kiway.h>
#include <view/view_controls.h>
#include <pcb_edit_frame.h>
#include <dialog_helpers.h>
#include <filter_reader.h>
#include <gr_basic.h>
#include <macros.h>
#include <fp_lib_table.h>
#include <lib_id.h>
#include <footprint_tree_pane.h>
#include <class_board.h>
#include <class_module.h>
#include <io_mgr.h>
#include <connectivity/connectivity_data.h>
#include <pcbnew.h>
#include <footprint_edit_frame.h>
#include <footprint_info.h>
#include <footprint_info_impl.h>
#include <dialog_choose_footprint.h>
#include <dialog_get_footprint_by_name.h>
#include <footprint_viewer_frame.h>
#include <wildcards_and_files_ext.h>
#include <widgets/progress_reporter.h>
#include <widgets/lib_tree.h>
#include "fp_tree_model_adapter.h"


static wxArrayString s_ModuleHistoryList;
static unsigned      s_ModuleHistoryMaxCount = 8;

static void AddModuleToHistory( const wxString& aName )
{
    // Remove duplicates
    for( int ii = s_ModuleHistoryList.GetCount() - 1; ii >= 0; --ii )
    {
        if( s_ModuleHistoryList[ ii ] == aName )
            s_ModuleHistoryList.RemoveAt( (size_t) ii );
    }

    // Add the new name at the beginning of the history list
    s_ModuleHistoryList.Insert( aName, 0 );

    // Remove extra names
    while( s_ModuleHistoryList.GetCount() >= s_ModuleHistoryMaxCount )
        s_ModuleHistoryList.RemoveAt( s_ModuleHistoryList.GetCount() - 1 );
}


static void clearModuleItemFlags( BOARD_ITEM* aItem )
{
    aItem->ClearFlags();
}

#include "pcbnew_id.h"
#include <bitmaps.h>
bool FOOTPRINT_EDIT_FRAME::Load_Module_From_BOARD( MODULE* aModule )
{
    bool is_last_fp_from_brd = IsCurrentFPFromBoard();

    MODULE* newModule;
    PCB_EDIT_FRAME* frame = (PCB_EDIT_FRAME*) Kiway().Player( FRAME_PCB, false );

    if( frame == NULL )     // happens if no board editor opened
        return false;

    if( aModule == NULL )
    {
        if( !frame->GetBoard() || !frame->GetBoard()->GetFirstModule() )
            return false;

        aModule = SelectFootprintFromBoard( frame->GetBoard() );
    }

    if( aModule == NULL )
        return false;

    if( !Clear_Pcb( true ) )
        return false;

    newModule = new MODULE( *aModule );
    newModule->SetParent( GetBoard() );
    newModule->SetLink( aModule->GetTimeStamp() );

    newModule->ClearFlags();
    newModule->RunOnChildren( std::bind( &clearModuleItemFlags, _1 ) );

    AddModuleToBoard( newModule );

    // Clear references to any net info, because the footprint editor
    // does know any thing about nets handled by the current edited board.
    // Morever we do not want to save any reference to an unknown net when
    // saving the footprint in lib cache
    // so we force the ORPHANED dummy net info for all pads
    newModule->ClearAllNets();

    GetCanvas()->GetViewControls()->SetCrossHairCursorPosition( VECTOR2D( 0, 0 ), false );
    PlaceModule( newModule );
    newModule->SetPosition( wxPoint( 0, 0 ) ); // cursor in GAL may not be initialized at the moment

    // Put it on FRONT layer,
    // because this is the default in ModEdit, and in libs
    if( newModule->GetLayer() != F_Cu )
        newModule->Flip( newModule->GetPosition(), frame->Settings().m_FlipLeftRight );

    // Put it in orientation 0,
    // because this is the default orientation in ModEdit, and in libs
    newModule->SetOrientation( 0 );

    Zoom_Automatique( false );

    m_adapter->SetPreselectNode( newModule->GetFPID(), 0 );

    GetScreen()->ClearUndoRedoList();
    GetScreen()->ClrModify();

    // Update the save items if needed.
    if( !is_last_fp_from_brd )
    {
        ReCreateMenuBar();
        ReCreateHToolbar();
    }

    Update3DView( true );
    updateView();
    GetCanvas()->Refresh();
    m_treePane->GetLibTree()->Refresh();    // update any previously-highlighted items

    return true;
}


wxString PCB_BASE_FRAME::SelectFootprintFromLibBrowser()
{
    // Close the current non-modal Lib browser if opened, and open a new one, in "modal" mode:
    FOOTPRINT_VIEWER_FRAME* viewer;
    viewer = (FOOTPRINT_VIEWER_FRAME*) Kiway().Player( FRAME_PCB_MODULE_VIEWER, false );

    if( viewer )
    {
        viewer->Destroy();
        // Destroy() does not immediately delete the viewer, if some events are pending.
        // (for this reason delete operator cannot be used blindly with "top level" windows)
        // so gives a slice of time to delete the viewer frame.
        // This is especially important in OpenGL mode to avoid recreating context before
        // the old one is deleted
        wxSafeYield();
    }

    SetFocus();

    // Creates the modal Lib browser:
    viewer = (FOOTPRINT_VIEWER_FRAME*) Kiway().Player( FRAME_PCB_MODULE_VIEWER_MODAL, true, this );

    wxString    fpid;
    int ret = viewer->ShowModal( &fpid, this );
    (void) ret;     // make static analyser quiet

    viewer->Destroy();

    return fpid;
}


MODULE* PCB_BASE_FRAME::SelectFootprintFromLibTree( LIB_ID aPreselect, bool aAllowBrowser )
{
    FP_LIB_TABLE*   fpTable = Prj().PcbFootprintLibs();
    wxString        moduleName;
    LIB_ID          fpid;
    MODULE*         module = NULL;

    static wxString lastComponentName;

    WX_PROGRESS_REPORTER progressReporter( this, _( "Loading Footprint Libraries" ), 3 );
    GFootprintList.ReadFootprintFiles( fpTable, nullptr, &progressReporter );
    progressReporter.Show( false );

    if( progressReporter.WasCancelled() )
        return NULL;

    if( GFootprintList.GetErrorCount() )
        GFootprintList.DisplayErrors( this );

    auto adapterPtr( FP_TREE_MODEL_ADAPTER::Create( fpTable ) );
    auto adapter = static_cast<FP_TREE_MODEL_ADAPTER*>( adapterPtr.get() );

    std::vector<LIB_TREE_ITEM*> historyInfos;

    for( auto const& item : s_ModuleHistoryList )
    {
        LIB_TREE_ITEM* fp_info = GFootprintList.GetModuleInfo( item );

        // this can be null, for example, if the footprint has been deleted from a library.
        if( fp_info != nullptr )
            historyInfos.push_back( fp_info );
    }

    adapter->DoAddLibrary( "-- " + _( "Recently Used" ) + " --", wxEmptyString, historyInfos, true );

    if( aPreselect.IsValid() )
        adapter->SetPreselectNode( aPreselect, 0 );
    else if( historyInfos.size() )
        adapter->SetPreselectNode( historyInfos[0]->GetLibId(), 0 );

    adapter->AddLibraries();

    wxString title;
    title.Printf( _( "Choose Footprint (%d items loaded)" ), adapter->GetItemCount() );

    DIALOG_CHOOSE_FOOTPRINT dialog( this, title, adapterPtr, aAllowBrowser );

    if( dialog.ShowQuasiModal() == wxID_CANCEL )
        return NULL;

    if( dialog.IsExternalBrowserSelected() )
    {
        // SelectFootprintFromLibBrowser() returns the "full" footprint name, i.e.
        // <lib_name>/<footprint name> or LIB_ID format "lib_name:fp_name:rev#"
        moduleName = SelectFootprintFromLibBrowser();

        if( moduleName.IsEmpty() )  // Cancel command
            return NULL;
        else
            fpid.Parse( moduleName, LIB_ID::ID_PCB );
    }
    else
    {
        fpid = dialog.GetSelectedLibId();

        if( !fpid.IsValid() )
            return NULL;
        else
            moduleName = fpid.Format();
    }

    try
    {
        module = loadFootprint( fpid );
    }
    catch( const IO_ERROR& ioe )
    {
        wxLogDebug( wxT( "Error loading footprint '%s'.\n\nError: %s" ),
                    fpid.Format().c_str(),
                    ioe.What() );
    }

    if( module )
    {
        lastComponentName = moduleName;
        AddModuleToHistory( moduleName );
    }

    return module;
}


MODULE* PCB_BASE_FRAME::LoadFootprint( const LIB_ID& aFootprintId )
{
    MODULE* module = NULL;

    try
    {
        module = loadFootprint( aFootprintId );
    }
    catch( const IO_ERROR& ioe )
    {
        wxLogDebug( wxT( "An error occurred attemping to load footprint '%s'.\n\nError: %s" ),
                    aFootprintId.Format().c_str(), GetChars( ioe.What() ) );
    }

    return module;
}


MODULE* PCB_BASE_FRAME::loadFootprint( const LIB_ID& aFootprintId )
{
    FP_LIB_TABLE*   fptbl = Prj().PcbFootprintLibs();

    wxCHECK_MSG( fptbl, NULL, wxT( "Cannot look up LIB_ID in NULL FP_LIB_TABLE." ) );

    MODULE *module = nullptr;
    try
    {
        module = fptbl->FootprintLoadWithOptionalNickname( aFootprintId );
    }
    catch( const IO_ERROR& ioe )
    {
        wxLogDebug( wxT( "An error occurred attemping to load footprint '%s'.\n\nError: %s" ),
                    aFootprintId.Format().c_str(), GetChars( ioe.What() ) );
    }

    // If the module is found, clear all net info,
    // to be sure there is no broken links
    // to any netinfo list (should be not needed, but it can be edited from
    // the footprint editor )
    if( module )
        module->ClearAllNets();

    return module;
}


MODULE* FOOTPRINT_EDIT_FRAME::SelectFootprintFromBoard( BOARD* aPcb )
{
    static wxString oldName;       // Save name of last module selected.

    wxString        fpname;
    wxString        msg;
    wxArrayString   listnames;

    for( auto module : aPcb->Modules() )
        listnames.Add( module->GetReference() );

    msg.Printf( _( "Footprints [%u items]" ), (unsigned) listnames.GetCount() );

    wxArrayString headers;

    headers.Add( _( "Footprint" ) );

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

    for( auto mod : aPcb->Modules() )
    {
        if( fpname == mod->GetReference() )
            return mod;
    }

    return nullptr;
}


bool FOOTPRINT_EDIT_FRAME::SaveLibraryAs( const wxString& aLibraryPath )
{
    wxString    curLibPath = aLibraryPath;
    wxString    dstLibPath = CreateNewLibrary();

    if( !dstLibPath )
        return false;             // user aborted in CreateNewLibrary()

    wxBusyCursor dummy;
    wxString msg;

    IO_MGR::PCB_FILE_T  dstType = IO_MGR::GuessPluginTypeFromLibPath( dstLibPath );
    IO_MGR::PCB_FILE_T  curType = IO_MGR::GuessPluginTypeFromLibPath( curLibPath );

    try
    {
        PLUGIN::RELEASER cur( IO_MGR::PluginFind( curType ) );
        PLUGIN::RELEASER dst( IO_MGR::PluginFind( dstType ) );

        wxArrayString footprints;

        cur->FootprintEnumerate( footprints, curLibPath );

        for( unsigned i = 0;  i < footprints.size();  ++i )
        {
            const MODULE* footprint = cur->GetEnumeratedFootprint( curLibPath, footprints[i] );
            dst->FootprintSave( dstLibPath, footprint );

            msg = wxString::Format( _( "Footprint \"%s\" saved" ), footprints[i] );
            SetStatusText( msg );
        }
    }
    catch( const IO_ERROR& ioe )
    {
        DisplayError( this, ioe.What() );
        return false;
    }

    msg = wxString::Format( _( "Footprint library \"%s\" saved as \"%s\"." ),
                            curLibPath,
                            dstLibPath );

    DisplayInfoMessage( this, msg );

    SetStatusText( wxEmptyString );
    return true;
}


static MODULE*           s_ModuleInitialCopy = NULL;   // Copy of module for abort/undo command

static PICKED_ITEMS_LIST s_PickedList;                 // a pick-list to save initial module
//   and dragged tracks


MODULE* PCB_BASE_FRAME::GetFootprintFromBoardByReference()
{
    wxString        moduleName;
    wxArrayString   fplist;

    // Build list of available fp references, to display them in dialog
    for( auto fp : GetBoard()->Modules() )
        fplist.Add( fp->GetReference() + wxT("    ( ") + fp->GetValue() + wxT(" )") );

    fplist.Sort();

    DIALOG_GET_FOOTPRINT_BY_NAME dlg( this, fplist );

    if( dlg.ShowModal() != wxID_OK )    //Aborted by user
        return NULL;

    moduleName = dlg.GetValue();
    moduleName.Trim( true );
    moduleName.Trim( false );

    if( !moduleName.IsEmpty() )
    {
        for( auto mod : GetBoard()->Modules() )
        {
            if( mod->GetReference().CmpNoCase( moduleName ) == 0 )
                return mod;
        }
    }

    return nullptr;
}


void PCB_BASE_FRAME::PlaceModule( MODULE* aModule, bool aRecreateRatsnest )
{
    if( aModule == 0 )
        return;

    OnModify();

    if( aModule->IsNew() )
    {
        SaveCopyInUndoList( aModule, UR_NEW );
    }
    else if( aModule->IsMoving() )
    {
        ITEM_PICKER picker( aModule, UR_CHANGED );
        picker.SetLink( s_ModuleInitialCopy );
        s_PickedList.PushItem( picker );
        s_ModuleInitialCopy = NULL;     // the picker is now owner of s_ModuleInitialCopy.
    }

    if( s_PickedList.GetCount() )
    {
        SaveCopyInUndoList( s_PickedList, UR_UNSPECIFIED );

        // Clear list, but DO NOT delete items, because they are owned by the saved undo
        // list and they therefore in use
        s_PickedList.ClearItemsList();
    }

    aModule->SetPosition( (wxPoint) GetCanvas()->GetViewControls()->GetCursorPosition() );
    aModule->ClearFlags();

    delete s_ModuleInitialCopy;
    s_ModuleInitialCopy = NULL;

    if( aRecreateRatsnest )
        m_Pcb->GetConnectivity()->Update( aModule );

    if( aRecreateRatsnest )
        Compile_Ratsnest( true );

    SetMsgPanel( aModule );
}


