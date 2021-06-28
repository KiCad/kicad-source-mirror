/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2018 Jean-Pierre Charras, jean-pierre.charras@ujf-grenoble.fr
 * Copyright (C) 2012 SoftPLC Corporation, Dick Hollenbeck <dick@softplc.com>
 * Copyright (C) 1992-2020 KiCad Developers, see AUTHORS.txt for contributors.
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

#include <board.h>
#include <footprint.h>
#include <confirm.h>
#include <connectivity/connectivity_data.h>
#include <dialog_choose_footprint.h>
#include <dialog_get_footprint_by_name.h>
#include <dialog_helpers.h>
#include <footprint_edit_frame.h>
#include <footprint_info_impl.h>
#include <footprint_tree_pane.h>
#include <footprint_viewer_frame.h>
#include <fp_lib_table.h>
#include <io_mgr.h>
#include <kicad_string.h>
#include <kiway.h>
#include <lib_id.h>
#include <macros.h>
#include <pcb_edit_frame.h>
#include <pcbnew_settings.h>
#include <view/view_controls.h>
#include <widgets/lib_tree.h>
#include <widgets/progress_reporter.h>
#include <dialog_pad_properties.h>

#include "fp_tree_model_adapter.h"


static wxArrayString s_FootprintHistoryList;
static unsigned      s_FootprintHistoryMaxCount = 8;

static void AddFootprintToHistory( const wxString& aName )
{
    // Remove duplicates
    for( int ii = s_FootprintHistoryList.GetCount() - 1; ii >= 0; --ii )
    {
        if( s_FootprintHistoryList[ ii ] == aName )
            s_FootprintHistoryList.RemoveAt((size_t) ii );
    }

    // Add the new name at the beginning of the history list
    s_FootprintHistoryList.Insert( aName, 0 );

    // Remove extra names
    while( s_FootprintHistoryList.GetCount() >= s_FootprintHistoryMaxCount )
        s_FootprintHistoryList.RemoveAt( s_FootprintHistoryList.GetCount() - 1 );
}


#include <bitmaps.h>
bool FOOTPRINT_EDIT_FRAME::LoadFootprintFromBoard( FOOTPRINT* aFootprint )
{
    bool is_last_fp_from_brd = IsCurrentFPFromBoard();

    FOOTPRINT*      newFootprint = nullptr;
    PCB_EDIT_FRAME* frame = (PCB_EDIT_FRAME*) Kiway().Player( FRAME_PCB_EDITOR, false );

    if( frame == nullptr )     // happens if no board editor opened
        return false;

    if( aFootprint == nullptr )
    {
        if( !frame->GetBoard() || !frame->GetBoard()->GetFirstFootprint() )
            return false;

        aFootprint = SelectFootprintFromBoard( frame->GetBoard() );
    }

    if( aFootprint == nullptr )
        return false;

    // Ensure we do not have the pad editor open (that is apseudo modal dlg).
    // LoadFootprintFromBoard() can be called from the board editor, and we must ensure
    // no footprint item is currently in edit
    if( wxWindow::FindWindowByName( PAD_PROPERTIES_DLG_NAME ) )
        wxWindow::FindWindowByName( PAD_PROPERTIES_DLG_NAME )->Close();

    if( !Clear_Pcb( true ) )
        return false;

    m_boardFootprintUuids.clear();

    auto recordAndUpdateUuid =
            [&]( BOARD_ITEM* aItem )
            {
                KIID newId;
                m_boardFootprintUuids[ newId ] = aItem->m_Uuid;
                const_cast<KIID&>( aItem->m_Uuid ) = newId;
            };

    newFootprint = (FOOTPRINT*) aFootprint->Clone();    // Keep existing uuids
    newFootprint->SetParent( GetBoard() );
    newFootprint->SetLink( aFootprint->m_Uuid );

    newFootprint->ClearFlags();
    recordAndUpdateUuid( newFootprint );
    newFootprint->RunOnChildren(
            [&]( BOARD_ITEM* aItem )
            {
                if( aItem->Type() == PCB_PAD_T )
                    aItem->SetLocked( false );

                aItem->ClearFlags();
                recordAndUpdateUuid( aItem );
            } );

    AddFootprintToBoard( newFootprint );

    // Clear references to any net info, because the footprint editor does know any thing about
    // nets handled by the current edited board.
    // Moreover we do not want to save any reference to an unknown net when saving the footprint
    // in lib cache so we force the ORPHANED dummy net info for all pads.
    newFootprint->ClearAllNets();

    GetCanvas()->GetViewControls()->SetCrossHairCursorPosition( VECTOR2D( 0, 0 ), false );
    PlaceFootprint( newFootprint );
    newFootprint->SetPosition( wxPoint( 0, 0 ) ); // cursor in GAL may not yet be initialized

    // Put it on FRONT layer,
    // because this is the default in Footprint Editor, and in libs
    if( newFootprint->GetLayer() != F_Cu )
        newFootprint->Flip( newFootprint->GetPosition(), frame->Settings().m_FlipLeftRight );

    // Put it in orientation 0,
    // because this is the default orientation in Footprint Editor, and in libs
    newFootprint->SetOrientation( 0 );

    Zoom_Automatique( false );

    m_adapter->SetPreselectNode( newFootprint->GetFPID(), 0 );

    ClearUndoRedoList();
    GetScreen()->SetContentModified( false );

    // Update the save items if needed.
    if( !is_last_fp_from_brd )
    {
        ReCreateMenuBar();
        ReCreateHToolbar();

        if( IsSearchTreeShown() )
            ToggleSearchTree();
    }

    Update3DView( true, true );
    UpdateView();
    GetCanvas()->Refresh();
    m_treePane->GetLibTree()->RefreshLibTree();    // update any previously-highlighted items

    return true;
}


wxString PCB_BASE_FRAME::SelectFootprintFromLibBrowser()
{
    // Close the current non-modal Lib browser if opened, and open a new one, in "modal" mode:
    FOOTPRINT_VIEWER_FRAME* viewer;
    viewer = (FOOTPRINT_VIEWER_FRAME*) Kiway().Player( FRAME_FOOTPRINT_VIEWER, false );

    if( viewer )
    {
        viewer->Destroy();
        // Destroy() does not immediately delete the viewer, if some events are pending.
        // (for this reason delete operator cannot be used blindly with "top level" windows)
        // so gives a slice of time to delete the viewer frame.
        // This is especially important in OpenGL mode to avoid recreating context before
        // the old one is deleted.
        wxSafeYield();
    }

    SetFocus();

    // Creates the modal Lib browser:
    viewer = (FOOTPRINT_VIEWER_FRAME*) Kiway().Player( FRAME_FOOTPRINT_VIEWER_MODAL, true, this );

    wxString    fpid;
    int ret = viewer->ShowModal( &fpid, this );
    (void) ret;     // make static analyser quiet

    viewer->Destroy();

    return fpid;
}


FOOTPRINT* PCB_BASE_FRAME::SelectFootprintFromLibTree( LIB_ID aPreselect )
{
    FP_LIB_TABLE*   fpTable = Prj().PcbFootprintLibs();
    wxString        footprintName;
    LIB_ID          fpid;
    FOOTPRINT*      footprint = nullptr;

    static wxString lastComponentName;

    // Load footprint files:
    WX_PROGRESS_REPORTER* progressReporter = new WX_PROGRESS_REPORTER( this,
                                                    _( "Loading Footprint Libraries" ), 3 );
    GFootprintList.ReadFootprintFiles( fpTable, nullptr, progressReporter );
    bool cancel = progressReporter->WasCancelled();

    // Force immediate deletion of the WX_PROGRESS_REPORTER.  Do not use Destroy(), or use
    // Destroy() followed by wxSafeYield() because on Windows, APP_PROGRESS_DIALOG and
    // WX_PROGRESS_REPORTER have some side effects on the event loop manager.  For instance, a
    // subsequent call to ShowModal() or ShowQuasiModal() for a dialog following the use of a
    // WX_PROGRESS_REPORTER results in incorrect modal or quasi modal behavior.
    delete progressReporter;

    if( cancel )
        return nullptr;

    if( GFootprintList.GetErrorCount() )
        GFootprintList.DisplayErrors( this );

    wxObjectDataPtr<LIB_TREE_MODEL_ADAPTER> ptr = FP_TREE_MODEL_ADAPTER::Create( this, fpTable );
    FP_TREE_MODEL_ADAPTER* adapter = static_cast<FP_TREE_MODEL_ADAPTER*>( ptr.get() );

    std::vector<LIB_TREE_ITEM*> historyInfos;

    for( const wxString& item : s_FootprintHistoryList )
    {
        LIB_TREE_ITEM* fp_info = GFootprintList.GetFootprintInfo( item );

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

    DIALOG_CHOOSE_FOOTPRINT dialog( this, title, ptr );

    if( dialog.ShowQuasiModal() == wxID_CANCEL )
        return nullptr;

    if( dialog.IsExternalBrowserSelected() )
    {
        // SelectFootprintFromLibBrowser() returns the "full" footprint name, i.e.
        // <lib_name>/<footprint name> or LIB_ID format "lib_name:fp_name:rev#"
        footprintName = SelectFootprintFromLibBrowser();

        if( footprintName.IsEmpty() )  // Cancel command
            return nullptr;
        else
            fpid.Parse( footprintName );
    }
    else
    {
        fpid = dialog.GetSelectedLibId();

        if( !fpid.IsValid() )
            return nullptr;
        else
            footprintName = fpid.Format();
    }

    try
    {
        footprint = loadFootprint( fpid );
    }
    catch( const IO_ERROR& )
    {
    }

    if( footprint )
    {
        lastComponentName = footprintName;
        AddFootprintToHistory( footprintName );
    }

    return footprint;
}


FOOTPRINT* PCB_BASE_FRAME::LoadFootprint( const LIB_ID& aFootprintId )
{
    FOOTPRINT* footprint = nullptr;

    try
    {
        footprint = loadFootprint( aFootprintId );
    }
    catch( const IO_ERROR& )
    {
    }

    return footprint;
}


FOOTPRINT* PCB_BASE_FRAME::loadFootprint( const LIB_ID& aFootprintId )
{
    FP_LIB_TABLE*   fptbl = Prj().PcbFootprintLibs();

    wxCHECK_MSG( fptbl, nullptr, wxT( "Cannot look up LIB_ID in NULL FP_LIB_TABLE." ) );

    FOOTPRINT *footprint = nullptr;

    // When loading a footprint from a library in the footprint editor
    // the items UUIDs must be keep and not reinitialized
    bool keepUUID = IsType( FRAME_FOOTPRINT_EDITOR );

    try
    {
        footprint = fptbl->FootprintLoadWithOptionalNickname( aFootprintId, keepUUID );
    }
    catch( const IO_ERROR& )
    {
    }

    // If the footprint is found, clear all net info to be sure there are no broken links to
    // any netinfo list (should be not needed, but it can be edited from the footprint editor )
    if( footprint )
        footprint->ClearAllNets();

    return footprint;
}


FOOTPRINT* FOOTPRINT_EDIT_FRAME::SelectFootprintFromBoard( BOARD* aPcb )
{
    static wxString oldName;       // Save name of last footprint selected.

    wxString        fpname;
    wxString        msg;
    wxArrayString   listnames;

    for( FOOTPRINT* footprint : aPcb->Footprints() )
        listnames.Add( footprint->GetReference() );

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

    EDA_LIST_DIALOG dlg( this, msg, headers, itemsToDisplay, wxEmptyString );

    if( dlg.ShowModal() == wxID_OK )
        fpname = dlg.GetTextSelection();
    else
        return nullptr;

    oldName = fpname;

    for( auto mod : aPcb->Footprints() )
    {
        if( fpname == mod->GetReference() )
            return mod;
    }

    return nullptr;
}


bool FOOTPRINT_EDIT_FRAME::SaveLibraryAs( const wxString& aLibraryPath )
{
    const wxString&    curLibPath = aLibraryPath;
    wxString    dstLibPath = CreateNewLibrary( wxEmptyString, aLibraryPath );

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

        cur->FootprintEnumerate( footprints, curLibPath, false );

        for( unsigned i = 0;  i < footprints.size();  ++i )
        {
            const FOOTPRINT* footprint = cur->GetEnumeratedFootprint( curLibPath, footprints[i] );
            dst->FootprintSave( dstLibPath, footprint );

            msg = wxString::Format( _( "Footprint '%s' saved." ), footprints[i] );
            SetStatusText( msg );
        }
    }
    catch( const IO_ERROR& ioe )
    {
        DisplayError( this, ioe.What() );
        return false;
    }

    msg = wxString::Format( _( "Footprint library '%s' saved as '%s'." ),
                            curLibPath,
                            dstLibPath );

    DisplayInfoMessage( this, msg );

    SetStatusText( wxEmptyString );
    return true;
}


static FOOTPRINT* s_FootprintInitialCopy = nullptr;    // Copy of footprint for abort/undo command

static PICKED_ITEMS_LIST s_PickedList;              // A pick-list to save initial footprint
                                                    //   and dragged tracks


FOOTPRINT* PCB_BASE_FRAME::GetFootprintFromBoardByReference()
{
    wxString        footprintName;
    wxArrayString   fplist;

    // Build list of available fp references, to display them in dialog
    for( auto fp : GetBoard()->Footprints() )
        fplist.Add( fp->GetReference() + wxT("    ( ") + fp->GetValue() + wxT(" )") );

    fplist.Sort();

    DIALOG_GET_FOOTPRINT_BY_NAME dlg( this, fplist );

    if( dlg.ShowModal() != wxID_OK )    //Aborted by user
        return nullptr;

    footprintName = dlg.GetValue();
    footprintName.Trim( true );
    footprintName.Trim( false );

    if( !footprintName.IsEmpty() )
    {
        for( auto mod : GetBoard()->Footprints() )
        {
            if( mod->GetReference().CmpNoCase( footprintName ) == 0 )
                return mod;
        }
    }

    return nullptr;
}


void PCB_BASE_FRAME::PlaceFootprint( FOOTPRINT* aFootprint, bool aRecreateRatsnest )
{
    if( aFootprint == nullptr )
        return;

    OnModify();

    if( aFootprint->IsNew() )
    {
        SaveCopyInUndoList( aFootprint, UNDO_REDO::NEWITEM );
    }
    else if( aFootprint->IsMoving() )
    {
        ITEM_PICKER picker( nullptr, aFootprint, UNDO_REDO::CHANGED );
        picker.SetLink( s_FootprintInitialCopy );
        s_PickedList.PushItem( picker );
        s_FootprintInitialCopy = nullptr;     // the picker is now owner of s_ModuleInitialCopy.
    }

    if( s_PickedList.GetCount() )
    {
        SaveCopyInUndoList( s_PickedList, UNDO_REDO::UNSPECIFIED );

        // Clear list, but DO NOT delete items, because they are owned by the saved undo
        // list and they therefore in use
        s_PickedList.ClearItemsList();
    }

    aFootprint->SetPosition((wxPoint) GetCanvas()->GetViewControls()->GetCursorPosition() );
    aFootprint->ClearFlags();

    delete s_FootprintInitialCopy;
    s_FootprintInitialCopy = nullptr;

    if( aRecreateRatsnest )
        m_pcb->GetConnectivity()->Update( aFootprint );

    if( aRecreateRatsnest )
        Compile_Ratsnest( true );

    SetMsgPanel( aFootprint );
}


