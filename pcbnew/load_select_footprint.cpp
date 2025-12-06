/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2018 Jean-Pierre Charras, jean-pierre.charras@ujf-grenoble.fr
 * Copyright (C) 2012 SoftPLC Corporation, Dick Hollenbeck <dick@softplc.com>
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
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
#include <eda_list_dialog.h>
#include <footprint_edit_frame.h>
#include <footprint_chooser_frame.h>
#include <footprint_viewer_frame.h>
#include <footprint_library_adapter.h>
#include <pcb_io/pcb_io_mgr.h>
#include <string_utils.h>
#include <kiway.h>
#include <lib_id.h>
#include <macros.h>
#include <pcb_edit_frame.h>
#include <pcbnew_settings.h>
#include <board_design_settings.h>
#include <drc/drc_item.h>
#include <view/view_controls.h>
#include <widgets/lib_tree.h>
#include <widgets/wx_progress_reporters.h>
#include <dialog_pad_properties.h>
#include <project_pcb.h>
#include <locale_io.h>


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

    std::map<int, SEVERITY>& severities = GetBoard()->GetDesignSettings().m_DRCSeverities;
    severities[ DRCE_MISSING_COURTYARD ] = RPT_SEVERITY_WARNING;

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
    newFootprint->SetParentGroup( nullptr );
    newFootprint->SetLink( aFootprint->m_Uuid );

    // Clear flags not used in Fp editor
    newFootprint->ClearFlags();
    newFootprint->SetLocked( false );

    recordAndUpdateUuid( newFootprint );
    newFootprint->RunOnChildren(
            [&]( BOARD_ITEM* aItem )
            {
                if( aItem->Type() == PCB_PAD_T )
                    aItem->SetLocked( false );

                aItem->ClearFlags();
                recordAndUpdateUuid( aItem );
            },
            RECURSE_MODE::RECURSE );

    AddFootprintToBoard( newFootprint );

    // Clear references to any net info, because the footprint editor does know any thing about
    // nets handled by the current edited board.
    // Moreover we do not want to save any reference to an unknown net when saving the footprint
    // in lib cache so we force the ORPHANED dummy net info for all pads.
    newFootprint->ClearAllNets();

    GetCanvas()->GetViewControls()->SetCrossHairCursorPosition( VECTOR2D( 0, 0 ), false );
    PlaceFootprint( newFootprint );
    newFootprint->SetPosition( VECTOR2I( 0, 0 ) ); // cursor in GAL may not yet be initialized

    // Put it on FRONT layer,
    // because this is the default in Footprint Editor, and in libs
    if( newFootprint->GetLayer() != F_Cu )
    {
        newFootprint->Flip( newFootprint->GetPosition(),
                            frame->GetPcbNewSettings()->m_FlipDirection );
    }

    // Put it in orientation 0,
    // because this is the default orientation in Footprint Editor, and in libs
    newFootprint->SetOrientation( ANGLE_0 );

    Zoom_Automatique( false );

    m_adapter->SetPreselectNode( newFootprint->GetFPID(), 0 );

    ClearUndoRedoList();
    GetScreen()->SetContentModified( false );

    // Update the save items if needed.
    if( !is_last_fp_from_brd )
    {
        ReCreateMenuBar();
        ReCreateHToolbar();

        if( IsLibraryTreeShown() )
            ToggleLibraryTree();
    }

    Update3DView( true, true );
    UpdateView();
    GetCanvas()->Refresh();
    m_treePane->GetLibTree()->RefreshLibTree();    // update any previously-highlighted items

    return true;
}


FOOTPRINT* PCB_BASE_FRAME::SelectFootprintFromLibrary( LIB_ID aPreselect )
{
    wxString        footprintName = aPreselect.Format().wx_str();
    LIB_ID          fpid;
    FOOTPRINT*      footprint = nullptr;

    static wxString lastComponentName;

    if( KIWAY_PLAYER* frame = Kiway().Player( FRAME_FOOTPRINT_CHOOSER, true, this ) )
    {
        if( frame->ShowModal( &footprintName, this ) )
            fpid.Parse( UTF8( footprintName ) );

        frame->Destroy();
    }

    if( !fpid.IsValid() )
        return nullptr;

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
    FOOTPRINT_LIBRARY_ADAPTER* adapter = PROJECT_PCB::FootprintLibAdapter( &Prj() );
    FOOTPRINT *footprint = nullptr;

    // When loading a footprint from a library in the footprint editor
    // the items UUIDs must be keep and not reinitialized
    bool keepUUID = IsType( FRAME_FOOTPRINT_EDITOR );

    try
    {
        footprint = adapter->LoadFootprintWithOptionalNickname( aFootprintId, keepUUID );
    }
    catch( const IO_ERROR& )
    {
    }

    if( footprint )
    {
        // If the footprint is found, clear all net info to be sure there are no broken links to
        // any netinfo list (should be not needed, but it can be edited from the footprint editor )
        footprint->ClearAllNets();

        if( m_pcb && !m_pcb->IsFootprintHolder() )
        {
            BOARD_DESIGN_SETTINGS& bds = m_pcb->GetDesignSettings();

            footprint->ApplyDefaultSettings( *m_pcb, bds.m_StyleFPFields, bds.m_StyleFPText,
                                             bds.m_StyleFPShapes, bds.m_StyleFPDimensions,
                                             bds.m_StyleFPBarcodes );
        }
    }

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

    for( FOOTPRINT* fp : aPcb->Footprints() )
    {
        if( fpname == fp->GetReference() )
            return fp;
    }

    return nullptr;
}


bool FOOTPRINT_EDIT_FRAME::SaveLibraryAs( const wxString& aLibraryPath )
{
    const wxString& curLibPath = aLibraryPath;
    wxString        dstLibPath = CreateNewLibrary( _( "Save Footprint Library As" ), aLibraryPath );

    if( !dstLibPath )
        return false;             // user aborted in CreateNewLibrary()

    wxBusyCursor dummy;
    wxString msg;

    LOCALE_IO              toggle_locale;
    PCB_IO_MGR::PCB_FILE_T dstType = PCB_IO_MGR::GuessPluginTypeFromLibPath( dstLibPath );
    PCB_IO_MGR::PCB_FILE_T curType = PCB_IO_MGR::GuessPluginTypeFromLibPath( curLibPath );

    if( dstType == PCB_IO_MGR::FILE_TYPE_NONE )
        dstType = PCB_IO_MGR::KICAD_SEXP;

    try
    {
        IO_RELEASER<PCB_IO> cur( PCB_IO_MGR::FindPlugin( curType ) );
        IO_RELEASER<PCB_IO> dst( PCB_IO_MGR::FindPlugin( dstType ) );

        if( !cur )
        {
            msg = wxString::Format( _( "Unable to find a reader for '%s'." ), curLibPath );
            DisplayError( this, msg );
            return false;
        }

        if( !dst )
        {
            msg = wxString::Format( _( "Unable to find a writer for '%s'." ), dstLibPath );
            DisplayError( this, msg );
            return false;
        }

        wxArrayString footprints;

        cur->FootprintEnumerate( footprints, curLibPath, false );

        for( const wxString& fp : footprints )
        {
            const FOOTPRINT* footprint = cur->GetEnumeratedFootprint( curLibPath, fp );
            dst->FootprintSave( dstLibPath, footprint );

            msg = wxString::Format( _( "Footprint '%s' saved." ), fp );
            SetStatusText( msg );
        }
    }
    catch( const IO_ERROR& ioe )
    {
        DisplayErrorMessage( this, _( "Error saving footprint library" ), ioe.What() );
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

void PCB_BASE_FRAME::PlaceFootprint( FOOTPRINT* aFootprint, bool aRecreateRatsnest, std::optional<VECTOR2I> aPosition )
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

    if( aPosition.has_value() )
        aFootprint->SetPosition( aPosition.value() );
    else
        aFootprint->SetPosition( GetCanvas()->GetViewControls()->GetCursorPosition() );

    aFootprint->ClearFlags();

    delete s_FootprintInitialCopy;
    s_FootprintInitialCopy = nullptr;

    if( aRecreateRatsnest )
        m_pcb->GetConnectivity()->Update( aFootprint );

    if( aRecreateRatsnest )
        Compile_Ratsnest( true );

    SetMsgPanel( aFootprint );
}


