/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
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

#include <board_commit.h>
#include <confirm.h>
#include <dialog_footprint_properties_fp_editor.h>
#include <footprint_edit_frame.h>
#include <footprint_tree_pane.h>
#include <fp_lib_table.h>
#include <functional>
#include <kiway_express.h>
#include <pcb_layer_box_selector.h>
#include <pcbnew_id.h>
#include <ratsnest/ratsnest_data.h>
#include <settings/color_settings.h>
#include <tool/tool_manager.h>
#include <tools/pcb_actions.h>
#include <widgets/appearance_controls.h>
#include <widgets/lib_tree.h>

using namespace std::placeholders;


void FOOTPRINT_EDIT_FRAME::LoadFootprintFromBoard( wxCommandEvent& event )
{
    LoadFootprintFromBoard( NULL );
}


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

    // if either m_Reference or m_Value are gone, reinstall them -
    // otherwise you cannot see what you are doing on board
    FP_TEXT* ref = &footprint->Reference();
    FP_TEXT* val = &footprint->Value();

    if( val && ref )
    {
        ref->SetType( FP_TEXT::TEXT_is_REFERENCE );    // just in case ...

        if( ref->GetLength() == 0 )
            ref->SetText( wxT( "Ref**" ) );

        val->SetType( FP_TEXT::TEXT_is_VALUE );        // just in case ...

        if( val->GetLength() == 0 )
            val->SetText( wxT( "Val**" ) );
    }

    if( m_zoomSelectBox->GetSelection() == 0 )
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
    m_treePane->GetLibTree()->CenterLibId( aFPID );
    m_treePane->GetLibTree()->RefreshLibTree();        // update highlighting
}


void FOOTPRINT_EDIT_FRAME::SelectLayer( wxCommandEvent& event )
{
    SetActiveLayer( ToLAYER_ID( m_selLayerBox->GetLayerSelection() ) );

    if( GetDisplayOptions().m_ContrastModeDisplay != HIGH_CONTRAST_MODE::NORMAL )
        GetCanvas()->Refresh();
}


void FOOTPRINT_EDIT_FRAME::SaveFootprintToBoard( wxCommandEvent& event )
{
    SaveFootprintToBoard( true );
}


class BASIC_FOOTPRINT_INFO : public FOOTPRINT_INFO
{
public:
    BASIC_FOOTPRINT_INFO( FOOTPRINT* aFootprint )
    {
        m_nickname = aFootprint->GetFPID().GetLibNickname().wx_str();
        m_fpname = aFootprint->GetFPID().GetLibItemName().wx_str();
        m_pad_count = aFootprint->GetPadCount( DO_NOT_INCLUDE_NPTH );
        m_unique_pad_count = aFootprint->GetUniquePadCount( DO_NOT_INCLUDE_NPTH );
        m_keywords = aFootprint->GetKeywords();
        m_doc = aFootprint->GetDescription();
        m_loaded = true;
    }
};


void FOOTPRINT_EDIT_FRAME::editFootprintProperties( FOOTPRINT* aFootprint )
{
    LIB_ID oldFPID = aFootprint->GetFPID();

    DIALOG_FOOTPRINT_PROPERTIES_FP_EDITOR dialog( this, aFootprint );
    dialog.ShowModal();

    // Update library tree
    BASIC_FOOTPRINT_INFO footprintInfo( aFootprint );
    wxDataViewItem       treeItem = m_adapter->FindItem( oldFPID );

    if( treeItem.IsOk() )   // Can be not found in tree if the current footprint is imported
                            // from file therefore not yet in tree.
    {
        static_cast<LIB_TREE_NODE_LIB_ID*>( treeItem.GetID() )->Update( &footprintInfo );
        m_treePane->GetLibTree()->RefreshLibTree();
    }

    UpdateTitle();      // in case of a name change...

    UpdateMsgPanel();
}


void FOOTPRINT_EDIT_FRAME::OnEditItemRequest( BOARD_ITEM* aItem )
{
    switch( aItem->Type() )
    {
    case PCB_PAD_T:
        ShowPadPropertiesDialog( static_cast<PAD*>( aItem ) );
        break;

    case PCB_FOOTPRINT_T:
        editFootprintProperties( static_cast<FOOTPRINT*>( aItem ) );
        GetCanvas()->Refresh();
        break;

    case PCB_FP_TEXT_T:
        ShowTextPropertiesDialog( aItem );
        break;

    case PCB_FP_SHAPE_T :
        ShowGraphicItemPropertiesDialog( aItem );
        break;

    case PCB_FP_ZONE_T:
    {
        ZONE*         zone = static_cast<ZONE*>( aItem );
        bool          success = false;
        ZONE_SETTINGS zoneSettings;

        zoneSettings << *static_cast<ZONE*>( aItem );

        if( zone->GetIsRuleArea() )
        {
            success = InvokeRuleAreaEditor( this, &zoneSettings ) == wxID_OK;
        }
        else if( zone->IsOnCopperLayer() )
        {
            success = InvokeCopperZonesEditor( this, &zoneSettings ) == wxID_OK;
        }
        else
        {
            success = InvokeNonCopperZonesEditor( this, &zoneSettings ) == wxID_OK;
        }

        if( success )
        {
            BOARD_COMMIT commit( this );
            commit.Modify( zone );
            commit.Push( _( "Edit Zone" ) );
            zoneSettings.ExportSetting( *static_cast<ZONE*>( aItem ) );
        }
    }
    break;

    case PCB_GROUP_T:
        m_toolManager->RunAction( PCB_ACTIONS::groupProperties, true, aItem );
        break;

    default:
        wxFAIL_MSG( "FOOTPRINT_EDIT_FRAME::OnEditItemRequest: unsupported item type "
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
    PCB_BASE_FRAME::SetActiveLayer( aLayer );

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

            FP_LIB_TABLE*        libTable = Prj().PcbFootprintLibs();
            const LIB_TABLE_ROW* libTableRow = libTable->FindRowByURI( fpFileName.GetPath() );

            if( !libTableRow )
            {
                msg.Printf( _( "The current configuration does not include a library named '%s'.\n"
                               "Use Manage Footprint Libraries to edit the configuration." ),
                            fpFileName.GetPath() );
                DisplayErrorMessage( this, _( "Library not found in footprint library table." ), msg );
                break;
            }

            libNickname = libTableRow->GetNickName();

            if( !libTable->HasLibrary( libNickname, true ) )
            {
                msg.Printf( _( "The library '%s' is not enabled in the current configuration.\n"
                               "Use Manage Footprint Libraries to edit the configuration." ),
                            libNickname );
                DisplayErrorMessage( this, _( "Footprint library not enabled." ), msg );
                break;
            }

            LIB_ID  fpId( libNickname, fpFileName.GetName() );

            if( m_treePane )
            {
                m_treePane->GetLibTree()->SelectLibId( fpId );
                wxCommandEvent event( COMPONENT_SELECTED );
                wxPostEvent( m_treePane, event );
            }
        }

        break;

    default:
        ;
    }
}
