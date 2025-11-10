/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
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

#include <board_commit.h>
#include <confirm.h>
#include <dialog_footprint_properties_fp_editor.h>
#include <footprint_edit_frame.h>
#include <footprint_tree_pane.h>
#include <footprint_library_adapter.h>
#include <functional>
#include <kiway_express.h>
#include <pcb_group.h>
#include <pcb_marker.h>
#include <pcb_textbox.h>
#include <pcb_barcode.h>
#include <pcb_table.h>
#include <pcb_shape.h>
#include <pad.h>
#include <zone.h>
#include <settings/color_settings.h>
#include <tool/tool_manager.h>
#include <tools/pcb_actions.h>
#include <tools/footprint_editor_control.h>
#include <widgets/appearance_controls.h>
#include <widgets/lib_tree.h>
#include <pcb_layer_box_selector.h>
#include <pcb_dimension.h>
#include <project_pcb.h>
#include <view/view_controls.h>
#include <dialogs/dialog_dimension_properties.h>
#include <dialogs/dialog_table_properties.h>

using namespace std::placeholders;


void FOOTPRINT_EDIT_FRAME::LoadFootprintFromLibrary( LIB_ID aFPID )
{
    bool is_last_fp_from_brd = IsCurrentFPFromBoard();

    FOOTPRINT* footprint = LoadFootprint( aFPID );

    if( !footprint )
        return;

    if( !Clear_Pcb( true ) )
        return;

    GetCanvas()->GetViewControls()->SetCrossHairCursorPosition( VECTOR2D( 0, 0 ), false );
    AddFootprintToBoard( footprint );

    footprint->ClearFlags();

    // if either reference or value are missing, reinstall them -
    // otherwise you cannot see what you are doing on board
    if( footprint->Reference().GetText().IsEmpty() )
        footprint->SetReference( wxT( "Ref**" ) );

    if( footprint->Value().GetText().IsEmpty() )
        footprint->SetValue( wxT( "Val**" ) );

    Zoom_Automatique( false );

    Update3DView( true, true );

    GetScreen()->SetContentModified( false );

    UpdateView();
    GetCanvas()->Refresh();

    // Update the save items if needed.
    if( is_last_fp_from_brd )
    {
        ReCreateMenuBar();
        ReCreateHToolbar();
    }

    m_treePane->GetLibTree()->ExpandLibId( aFPID );

    m_centerItemOnIdle = aFPID;
    Bind( wxEVT_IDLE, &FOOTPRINT_EDIT_FRAME::centerItemIdleHandler, this );

    m_treePane->GetLibTree()->RefreshLibTree();        // update highlighting
}


void FOOTPRINT_EDIT_FRAME::centerItemIdleHandler( wxIdleEvent& aEvent )
{
    m_treePane->GetLibTree()->CenterLibId( m_centerItemOnIdle );
    Unbind( wxEVT_IDLE, &FOOTPRINT_EDIT_FRAME::centerItemIdleHandler, this );
}


class BASIC_FOOTPRINT_INFO : public FOOTPRINT_INFO
{
public:
    BASIC_FOOTPRINT_INFO( FOOTPRINT* aFootprint )
    {
        wxASSERT( aFootprint );

        m_nickname = aFootprint->GetFPID().GetLibNickname().wx_str();
        m_fpname = aFootprint->GetFPID().GetLibItemName().wx_str();
        m_pad_count = aFootprint->GetPadCount( DO_NOT_INCLUDE_NPTH );
        m_unique_pad_count = aFootprint->GetUniquePadCount( DO_NOT_INCLUDE_NPTH );
        m_keywords = aFootprint->GetKeywords();
        m_doc = aFootprint->GetLibDescription();
        m_loaded = true;
    }
};


void FOOTPRINT_EDIT_FRAME::UpdateLibraryTree( const wxDataViewItem& aTreeItem,
                                              FOOTPRINT* aFootprint )
{
    wxCHECK( aFootprint, /* void */ );

    BASIC_FOOTPRINT_INFO footprintInfo( aFootprint );

    if( aTreeItem.IsOk() )   // Can be not found in tree if the current footprint is imported
                             // from file therefore not yet in tree.
    {
        static_cast<LIB_TREE_NODE_ITEM*>( aTreeItem.GetID() )->Update( &footprintInfo );
        m_treePane->GetLibTree()->RefreshLibTree();
    }
}


void FOOTPRINT_EDIT_FRAME::editFootprintProperties( FOOTPRINT* aFootprint )
{
    LIB_ID oldFPID = aFootprint->GetFPID();

    DIALOG_FOOTPRINT_PROPERTIES_FP_EDITOR dialog( this, aFootprint );
    dialog.ShowQuasiModal();

    // Update design settings for footprint layers
    updateEnabledLayers();

    // Update library tree and title in case of a name change
    wxDataViewItem treeItem = m_adapter->FindItem( oldFPID );
    UpdateLibraryTree( treeItem, aFootprint );
    UpdateTitle();

    UpdateMsgPanel();
    UpdateUserInterface();
}


void FOOTPRINT_EDIT_FRAME::OnEditItemRequest( BOARD_ITEM* aItem )
{
    switch( aItem->Type() )
    {
    case PCB_REFERENCE_IMAGE_T:
        ShowReferenceImagePropertiesDialog( aItem );
        break;

    case PCB_BARCODE_T:
        ShowBarcodePropertiesDialog( static_cast<PCB_BARCODE*>( aItem ) );
        break;

    case PCB_PAD_T:
        ShowPadPropertiesDialog( static_cast<PAD*>( aItem ) );
        break;

    case PCB_FOOTPRINT_T:
        editFootprintProperties( static_cast<FOOTPRINT*>( aItem ) );
        GetCanvas()->Refresh();
        break;

    case PCB_FIELD_T:
    case PCB_TEXT_T:
        ShowTextPropertiesDialog( static_cast<PCB_TEXT*>( aItem ) );
        break;

    case PCB_TEXTBOX_T:
        ShowTextBoxPropertiesDialog( static_cast<PCB_TEXTBOX*>( aItem ) );
        break;

    case PCB_TABLE_T:
    {
        DIALOG_TABLE_PROPERTIES dlg( this, static_cast<PCB_TABLE*>( aItem ) );

        //QuasiModal required for Scintilla auto-complete
        dlg.ShowQuasiModal();
        break;
    }

    case PCB_SHAPE_T :
        ShowGraphicItemPropertiesDialog( static_cast<PCB_SHAPE*>( aItem ) );
        break;

    case PCB_DIM_ALIGNED_T:
    case PCB_DIM_CENTER_T:
    case PCB_DIM_RADIAL_T:
    case PCB_DIM_ORTHOGONAL_T:
    case PCB_DIM_LEADER_T:
    {
        DIALOG_DIMENSION_PROPERTIES dlg( this, static_cast<PCB_DIMENSION_BASE*>( aItem ) );

        // TODO: why is this QuasiModal?
        dlg.ShowQuasiModal();
        break;
    }

    case PCB_ZONE_T:
    {
        ZONE*         zone = static_cast<ZONE*>( aItem );
        bool          success = false;
        ZONE_SETTINGS zoneSettings;

        zoneSettings << *static_cast<ZONE*>( aItem );

        if( zone->GetIsRuleArea() )
            success = InvokeRuleAreaEditor( this, &zoneSettings ) == wxID_OK;
        else if( zone->IsOnCopperLayer() )
            success = InvokeCopperZonesEditor( this, zone, &zoneSettings ) == wxID_OK;
        else
            success = InvokeNonCopperZonesEditor( this, &zoneSettings ) == wxID_OK;

        if( success )
        {
            BOARD_COMMIT commit( this );
            commit.Modify( zone );
            commit.Push( _( "Edit Zone" ) );
            zoneSettings.ExportSetting( *static_cast<ZONE*>( aItem ) );
        }

        break;
    }

    case PCB_GROUP_T:
        m_toolManager->RunAction( ACTIONS::groupProperties,
                                  static_cast<EDA_GROUP*>( static_cast<PCB_GROUP*>( aItem ) ) );
        break;

    case PCB_MARKER_T:
        m_toolManager->GetTool<FOOTPRINT_EDITOR_CONTROL>()->CrossProbe( static_cast<PCB_MARKER*>( aItem ) );
        break;

    case PCB_POINT_T:
        break;

    default:
        wxFAIL_MSG( wxT( "FOOTPRINT_EDIT_FRAME::OnEditItemRequest: unsupported item type " )
                    + aItem->GetClass() );
        break;
    }
}


COLOR4D FOOTPRINT_EDIT_FRAME::GetGridColor()
{
    return GetColorSettings()->GetColor( LAYER_GRID );
}


void FOOTPRINT_EDIT_FRAME::SetActiveLayer( PCB_LAYER_ID aLayer )
{
    const PCB_LAYER_ID oldLayer = GetActiveLayer();

    if( oldLayer == aLayer )
        return;

    PCB_BASE_FRAME::SetActiveLayer( aLayer );

    /*
     * Follow the PCB editor logic for showing/hiding clearance layers: show only for
     * the active copper layer or a front/back non-copper layer.
     */
    const auto getClearanceLayerForActive = []( PCB_LAYER_ID aActiveLayer ) -> std::optional<int>
    {
        if( IsCopperLayer( aActiveLayer ) )
            return CLEARANCE_LAYER_FOR( aActiveLayer );

        return std::nullopt;
    };

    if( std::optional<int> oldClearanceLayer = getClearanceLayerForActive( oldLayer ) )
        GetCanvas()->GetView()->SetLayerVisible( *oldClearanceLayer, false );

    if( std::optional<int> newClearanceLayer = getClearanceLayerForActive( aLayer ) )
        GetCanvas()->GetView()->SetLayerVisible( *newClearanceLayer, true );

    m_appearancePanel->OnLayerChanged();

    m_toolManager->RunAction( PCB_ACTIONS::layerChanged );  // notify other tools
    GetCanvas()->SetFocus();                             // allow capture of hotkeys
    GetCanvas()->SetHighContrastLayer( aLayer );
    GetCanvas()->Refresh();
}


bool FOOTPRINT_EDIT_FRAME::OpenProjectFiles( const std::vector<wxString>& aFileSet, int aCtl )
{
    if( !Clear_Pcb( true ) )
        return false;                  // this command is aborted

    GetCanvas()->GetViewControls()->SetCrossHairCursorPosition( VECTOR2D( 0, 0 ), false );
    ImportFootprint( aFileSet[ 0 ] );

    if( GetBoard()->GetFirstFootprint() )
        GetBoard()->GetFirstFootprint()->ClearFlags();

    GetScreen()->SetContentModified( false );
    Zoom_Automatique( false );
    GetCanvas()->Refresh();

    return true;
}


void FOOTPRINT_EDIT_FRAME::KiwayMailIn( KIWAY_EXPRESS& mail )
{
    const std::string& payload = mail.GetPayload();

    switch( mail.Command() )
    {
    case MAIL_FP_EDIT:
        if( !payload.empty() )
        {
            wxFileName fpFileName( payload );
            wxString   libNickname;
            wxString   msg;

            FOOTPRINT_LIBRARY_ADAPTER* adapter = PROJECT_PCB::FootprintLibAdapter( &Prj() );
            std::optional<LIBRARY_TABLE_ROW*> optRow = adapter->FindRowByURI( fpFileName.GetPath() );

            if( !optRow )
            {
                msg.Printf( _( "The current configuration does not include the footprint library '%s'." ),
                            fpFileName.GetPath() );
                msg += wxS( "\n" ) + _( "Use Manage Footprint Libraries to edit the configuration." );
                DisplayErrorMessage( this, _( "Library not found in footprint library table." ),
                                     msg );
                break;
            }

            libNickname = ( *optRow )->Nickname();

            if( !adapter->HasLibrary( libNickname, true ) )
            {
                msg.Printf( _( "The footprint library '%s' is not enabled in the current configuration." ),
                            libNickname );
                msg += wxS( "\n" ) + _( "Use Manage Footprint Libraries to edit the configuration." );
                DisplayErrorMessage( this, _( "Footprint library not enabled." ), msg );
                break;
            }

            LIB_ID  fpId( libNickname, fpFileName.GetName() );

            if( m_treePane )
            {
                m_treePane->GetLibTree()->SelectLibId( fpId );
                wxCommandEvent event( EVT_LIBITEM_CHOSEN );
                wxPostEvent( m_treePane, event );
            }
        }

        break;

    case MAIL_RELOAD_LIB:
        SyncLibraryTree( true );
        RefreshLibraryTree();
        break;

    default:
        break;
    }
}
