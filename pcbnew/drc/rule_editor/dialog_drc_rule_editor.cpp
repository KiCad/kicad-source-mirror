/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2024 KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */


#include <widgets/wx_progress_reporters.h>
#include <widgets/appearance_controls.h>

#include <confirm.h>
#include <pcb_edit_frame.h>
#include <kiface_base.h>

#include "dialog_drc_rule_editor.h"
#include "panel_drc_rule_editor.h"
#include "drc_rule_editor_enums.h"
#include "drc_re_base_constraint_data.h"
#include "drc_re_numeric_input_constraint_data.h"
#include "drc_re_bool_input_constraint_data.h"
#include "drc_re_via_style_constraint_data.h"
#include "drc_re_abs_length_two_constraint_data.h"
#include "drc_re_min_txt_ht_th_constraint_data.h"
#include "drc_re_rtg_diff_pair_constraint_data.h"
#include "drc_re_routing_width_constraint_data.h"
#include "drc_re_permitted_layers_constraint_data.h"
#include "drc_re_allowed_orientation_constraint_data.h"
#include "drc_re_custom_rule_constraint_data.h"
#include "drc_re_rule_loader.h"
#include "drc_re_rule_saver.h"
#include <drc/drc_engine.h>
#include <drc/drc_rule_condition.h>
#include <tool/tool_manager.h>
#include <tool/actions.h>
#include <wx/ffile.h>
#include <memory>


const RULE_TREE_NODE* FindNodeById( const std::vector<RULE_TREE_NODE>& aNodes, int aTargetId )
{
    auto it = std::find_if( aNodes.begin(), aNodes.end(),
                            [aTargetId]( const RULE_TREE_NODE& node )
                            {
                                return node.m_nodeId == aTargetId;
                            } );

    if( it != aNodes.end() )
    {
        return &( *it );
    }

    return nullptr;
}


DIALOG_DRC_RULE_EDITOR::DIALOG_DRC_RULE_EDITOR( PCB_EDIT_FRAME* aEditorFrame, wxWindow* aParent ) :
        RULE_EDITOR_DIALOG_BASE( aParent, _( "Design Rule Editor" ), wxSize( 980, 680 ) ),
        PROGRESS_REPORTER_BASE( 1 ),
        m_reporter( nullptr ),
        m_nodeId( 0 )
{
    m_frame = aEditorFrame;
    m_currentBoard = m_frame->GetBoard();
    m_ruleEditorPanel = nullptr;

    m_ruleTreeCtrl->DeleteAllItems();

    m_ruleTreeNodeDatas = GetDefaultRuleTreeItems();

    InitRuleTreeItems( m_ruleTreeNodeDatas );

    LoadExistingRules();

    if( Prj().IsReadOnly() )
    {
        m_infoBar->ShowMessage( _( "Project is missing or read-only. Settings will not be "
                                   "editable." ),
                                wxICON_WARNING );
    }

    m_severities = 0;

    m_markersProvider = std::make_shared<DRC_ITEMS_PROVIDER>( m_currentBoard, MARKER_BASE::MARKER_DRC,
                                                              MARKER_BASE::MARKER_DRAWING_SHEET );

    m_markerDataView =
            new wxDataViewCtrl( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxDV_ROW_LINES | wxDV_SINGLE );

    m_markersTreeModel = new RC_TREE_MODEL( m_frame, m_markerDataView );
    m_markerDataView->AssociateModel( m_markersTreeModel );
    m_markersTreeModel->Update( m_markersProvider, m_severities );

    m_markerDataView->Hide();
}


DIALOG_DRC_RULE_EDITOR::~DIALOG_DRC_RULE_EDITOR()
{
}


bool DIALOG_DRC_RULE_EDITOR::TransferDataToWindow()
{
    bool ok = RULE_EDITOR_DIALOG_BASE::TransferDataToWindow();

    // Ensure minimum size is our intended small value after base sizing logic
    Layout();
    SetMinSize( wxSize( 400, 300 ) );

    // If SetSizeHints inflated the height (e.g., to full display height), reset to the ctor's
    // intended initial size so that any saved geometry can apply without being trumped by the
    // current oversized height (Show() uses max(current, saved)).
    SetSize( wxSize( 980, 680 ) );

    return ok;
}


bool DIALOG_DRC_RULE_EDITOR::TransferDataFromWindow()
{
    if( !RULE_EDITOR_DIALOG_BASE::TransferDataFromWindow() )
        return false;

    SaveRulesToFile();

    return true;
}


std::vector<RULE_TREE_NODE> DIALOG_DRC_RULE_EDITOR::GetDefaultRuleTreeItems()
{
    std::vector<RULE_TREE_NODE> result;

    int lastParentId;
    int electricalItemId;
    int manufacturabilityItemId;
    int highSpeedDesignId;
    int footprintItemId;

    result.push_back( buildRuleTreeNodeData( "Design Rules", DRC_RULE_EDITOR_ITEM_TYPE::ROOT ) );
    lastParentId = m_nodeId;

    result.push_back( buildRuleTreeNodeData( "Electrical", DRC_RULE_EDITOR_ITEM_TYPE::CATEGORY, lastParentId ) );
    electricalItemId = m_nodeId;

    result.push_back( buildRuleTreeNodeData( "Manufacturability", DRC_RULE_EDITOR_ITEM_TYPE::CATEGORY, lastParentId ) );
    manufacturabilityItemId = m_nodeId;

    result.push_back( buildRuleTreeNodeData( "Highspeed design", DRC_RULE_EDITOR_ITEM_TYPE::CATEGORY, lastParentId ) );
    highSpeedDesignId = m_nodeId;

    result.push_back( buildRuleTreeNodeData( "Footprints", DRC_RULE_EDITOR_ITEM_TYPE::CATEGORY, lastParentId ) );
    footprintItemId = m_nodeId;

    std::vector<RULE_TREE_NODE> subItemNodes = buildElectricalRuleTreeNodes( electricalItemId );
    result.insert( result.end(), subItemNodes.begin(), subItemNodes.end() );

    subItemNodes = buildManufacturabilityRuleTreeNodes( manufacturabilityItemId );
    result.insert( result.end(), subItemNodes.begin(), subItemNodes.end() );

    subItemNodes = buildHighspeedDesignRuleTreeNodes( highSpeedDesignId );
    result.insert( result.end(), subItemNodes.begin(), subItemNodes.end() );

    subItemNodes = buildFootprintsRuleTreeNodes( footprintItemId );
    result.insert( result.end(), subItemNodes.begin(), subItemNodes.end() );

    // Custom rules category
    result.push_back( buildRuleTreeNodeData( "Custom", DRC_RULE_EDITOR_ITEM_TYPE::CATEGORY, lastParentId ) );
    int customItemId = m_nodeId;
    result.push_back(
            buildRuleTreeNodeData( "Custom Rule", DRC_RULE_EDITOR_ITEM_TYPE::CONSTRAINT, customItemId, CUSTOM_RULE ) );

    return result;
}

void DIALOG_DRC_RULE_EDITOR::LoadExistingRules()
{
    wxFileName rulesFile( m_frame->GetDesignRulesPath() );

    if( !rulesFile.FileExists() )
        return;

    DRC_RULE_LOADER loader;
    std::vector<DRC_RE_LOADED_PANEL_ENTRY> entries = loader.LoadFile( rulesFile.GetFullPath() );

    wxLogTrace( wxS( "KI_TRACE_DRC_RULE_EDITOR" ),
                wxS( "[LoadExistingRules] Loaded %zu entries from %s" ),
                entries.size(), rulesFile.GetFullPath() );

    // Debug: Log all constraint nodes available for parent lookup
    wxLogTrace( wxS( "KI_TRACE_DRC_RULE_EDITOR" ),
                wxS( "[LoadExistingRules] Available constraint nodes in m_ruleTreeNodeDatas:" ) );

    for( const RULE_TREE_NODE& node : m_ruleTreeNodeDatas )
    {
        if( node.m_nodeType == CONSTRAINT )
        {
            wxLogTrace( wxS( "KI_TRACE_DRC_RULE_EDITOR" ),
                        wxS( "[LoadExistingRules]   Node '%s': nodeId=%d, m_nodeTypeMap=%d" ),
                        wxString( node.m_nodeName ), node.m_nodeId,
                        node.m_nodeTypeMap ? static_cast<int>( *node.m_nodeTypeMap ) : -1 );
        }
    }

    // Helper to find tree item by node ID
    std::function<wxTreeItemId( wxTreeItemId, int )> findItem =
            [&]( wxTreeItemId aItem, int aTargetId ) -> wxTreeItemId
    {
        RULE_TREE_ITEM_DATA* data =
                dynamic_cast<RULE_TREE_ITEM_DATA*>( m_ruleTreeCtrl->GetItemData( aItem ) );

        if( data && data->GetNodeId() == aTargetId )
            return aItem;

        wxTreeItemIdValue cookie;
        wxTreeItemId      child = m_ruleTreeCtrl->GetFirstChild( aItem, cookie );

        while( child.IsOk() )
        {
            wxTreeItemId res = findItem( child, aTargetId );

            if( res.IsOk() )
                return res;

            child = m_ruleTreeCtrl->GetNextChild( aItem, cookie );
        }

        return wxTreeItemId();
    };

    for( DRC_RE_LOADED_PANEL_ENTRY& entry : entries )
    {
        DRC_RULE_EDITOR_CONSTRAINT_NAME type = entry.panelType;

        wxLogTrace( wxS( "KI_TRACE_DRC_RULE_EDITOR" ),
                    wxS( "[LoadExistingRules] Processing entry: rule='%s', panelType=%d" ),
                    entry.ruleName, static_cast<int>( type ) );

        // Find parent node by constraint type
        int parentId = -1;

        for( const RULE_TREE_NODE& node : m_ruleTreeNodeDatas )
        {
            if( node.m_nodeType == CONSTRAINT && node.m_nodeTypeMap && *node.m_nodeTypeMap == type )
            {
                parentId = node.m_nodeId;
                break;
            }
        }

        if( parentId == -1 )
        {
            wxLogTrace( wxS( "KI_TRACE_DRC_RULE_EDITOR" ),
                        wxS( "[LoadExistingRules] No parent found for panelType=%d, skipping" ),
                        static_cast<int>( type ) );
            continue;
        }

        wxTreeItemId parentItem = findItem( m_ruleTreeCtrl->GetRootItem(), parentId );

        if( !parentItem.IsOk() )
        {
            wxLogTrace( wxS( "KI_TRACE_DRC_RULE_EDITOR" ),
                        wxS( "[LoadExistingRules] Tree item not found for parentId=%d, skipping" ),
                        parentId );
            continue;
        }

        wxLogTrace( wxS( "KI_TRACE_DRC_RULE_EDITOR" ),
                    wxS( "[LoadExistingRules] Found parent node: parentId=%d" ), parentId );

        RULE_TREE_NODE node =
                buildRuleTreeNodeData( entry.ruleName.ToStdString(), RULE, parentId, type );

        // Transfer loaded entry data to the constraint data
        auto ruleData = std::dynamic_pointer_cast<DRC_RE_BASE_CONSTRAINT_DATA>( entry.constraintData );

        if( ruleData )
        {
            ruleData->SetId( node.m_nodeData->GetId() );
            ruleData->SetParentId( parentId );
            ruleData->SetOriginalRuleText( entry.originalRuleText );
            ruleData->SetWasEdited( entry.wasEdited );
            node.m_nodeData = ruleData;
        }

        m_ruleTreeNodeDatas.push_back( node );
        AppendNewRuleTreeItem( node, parentItem );

        wxLogTrace( wxS( "KI_TRACE_DRC_RULE_EDITOR" ),
                    wxS( "[LoadExistingRules] Appended rule '%s' to tree under parentId=%d" ),
                    entry.ruleName, parentId );
    }
}


void DIALOG_DRC_RULE_EDITOR::AddNewRule( RULE_TREE_ITEM_DATA* aRuleTreeItemData )
{
    wxTreeItemId    treeItemId;
    RULE_TREE_NODE* nodeDetail = getRuleTreeNodeInfo( aRuleTreeItemData->GetNodeId() );

    if( nodeDetail->m_nodeType == CONSTRAINT )
    {
        treeItemId = aRuleTreeItemData->GetTreeItemId();
    }
    else
    {
        treeItemId = aRuleTreeItemData->GetParentTreeItemId();
    }

    AppendNewRuleTreeItem( buildRuleTreeNode( aRuleTreeItemData ), treeItemId );
}


void DIALOG_DRC_RULE_EDITOR::DuplicateRule( RULE_TREE_ITEM_DATA* aRuleTreeItemData )
{
    RULE_TREE_NODE* sourceTreeNode = getRuleTreeNodeInfo( aRuleTreeItemData->GetNodeId() );
    RULE_TREE_NODE  targetTreeNode = buildRuleTreeNode( aRuleTreeItemData );

    auto sourceDataPtr = dynamic_pointer_cast<RULE_EDITOR_DATA_BASE>( sourceTreeNode->m_nodeData );

    if( sourceDataPtr )
    {
        targetTreeNode.m_nodeData->CopyFrom( *sourceDataPtr );
    }

    wxTreeItemId treeItemId = aRuleTreeItemData->GetParentTreeItemId();
    AppendNewRuleTreeItem( targetTreeNode, treeItemId );
}


void DIALOG_DRC_RULE_EDITOR::RuleTreeItemSelectionChanged( RULE_TREE_ITEM_DATA* aCurrentRuleTreeItemData )
{
    RULE_TREE_NODE* nodeDetail = getRuleTreeNodeInfo( aCurrentRuleTreeItemData->GetNodeId() );

    if( nodeDetail->m_nodeType == ROOT || nodeDetail->m_nodeType == CATEGORY || nodeDetail->m_nodeType == CONSTRAINT )
    {
        std::vector<RULE_TREE_NODE*> ruleNodes;
        collectChildRuleNodes( nodeDetail->m_nodeId, ruleNodes );

        std::vector<DRC_RULE_ROW> rows;
        rows.reserve( ruleNodes.size() );

        for( RULE_TREE_NODE* ruleNode : ruleNodes )
        {
            RULE_TREE_NODE* parentNode = getRuleTreeNodeInfo( ruleNode->m_nodeData->GetParentId() );
            wxString        type = parentNode ? parentNode->m_nodeName : wxString{};
            rows.push_back( { type, ruleNode->m_nodeData->GetRuleName(), ruleNode->m_nodeData->GetComment() } );
        }

        m_groupHeaderPanel = new PANEL_DRC_GROUP_HEADER( m_splitter, rows );
        SetContentPanel( m_groupHeaderPanel );
        m_ruleEditorPanel = nullptr;
    }
    else if( nodeDetail->m_nodeType == RULE )
    {
        RULE_TREE_ITEM_DATA* parentItemData = dynamic_cast<RULE_TREE_ITEM_DATA*>(
                m_ruleTreeCtrl->GetItemData( aCurrentRuleTreeItemData->GetParentTreeItemId() ) );
        RULE_TREE_NODE* paretNodeDetail = getRuleTreeNodeInfo( parentItemData->GetNodeId() );
        wxString        constraintName = paretNodeDetail->m_nodeName;

        m_ruleEditorPanel = new PANEL_DRC_RULE_EDITOR(
                m_splitter, m_frame->GetBoard(),
                static_cast<DRC_RULE_EDITOR_CONSTRAINT_NAME>( nodeDetail->m_nodeTypeMap.value_or( -1 ) ),
                &constraintName, dynamic_pointer_cast<DRC_RE_BASE_CONSTRAINT_DATA>( nodeDetail->m_nodeData ) );

        SetContentPanel( m_ruleEditorPanel );

        m_ruleEditorPanel->TransferDataToWindow();

        m_ruleEditorPanel->SetSaveCallback(
                [this]( int aNodeId )
                {
                    this->saveRule( aNodeId );
                } );

        m_ruleEditorPanel->SetRemoveCallback(
                [this]( int aNodeId )
                {
                    this->RemoveRule( aNodeId );
                } );

        m_ruleEditorPanel->SetCloseCallback(
                [this]( int aNodeId )
                {
                    this->closeRuleEntryView( aNodeId );
                } );

        m_ruleEditorPanel->SetRuleNameValidationCallback(
                [this]( int aNodeId, wxString aRuleName )
                {
                    return this->validateRuleName( aNodeId, aRuleName );
                } );

        m_ruleEditorPanel->SetShowMatchesCallBack(
                [this]( int aNodeId ) -> int
                {
                    return this->highlightMatchingItems( aNodeId );
                } );

        m_groupHeaderPanel = nullptr;
    }
}


void DIALOG_DRC_RULE_EDITOR::OnSave( wxCommandEvent& aEvent )
{
    if( m_ruleEditorPanel )
        m_ruleEditorPanel->Save( aEvent );
}


void DIALOG_DRC_RULE_EDITOR::OnCancel( wxCommandEvent& aEvent )
{
    if( m_ruleEditorPanel )
    {
        auto data = m_ruleEditorPanel->GetConstraintData();
        bool isNew = data && data->IsNew();

        m_ruleEditorPanel->Cancel( aEvent );

        if( isNew )
            return;
    }

    aEvent.Skip();
}


void DIALOG_DRC_RULE_EDITOR::UpdateRuleTypeTreeItemData( RULE_TREE_ITEM_DATA* aRuleTreeItemData )
{
    RULE_TREE_NODE* nodeDetail = getRuleTreeNodeInfo( aRuleTreeItemData->GetNodeId() );

    if( nodeDetail->m_nodeType == DRC_RULE_EDITOR_ITEM_TYPE::RULE && m_ruleEditorPanel )
    {
        m_ruleEditorPanel->TransferDataFromWindow();

        nodeDetail->m_nodeName = nodeDetail->m_nodeData->GetRuleName();
        nodeDetail->m_nodeData->SetIsNew( false );

        // Mark as edited so the rule gets regenerated instead of using original text
        auto constraintData =
                std::dynamic_pointer_cast<DRC_RE_BASE_CONSTRAINT_DATA>( nodeDetail->m_nodeData );

        if( constraintData )
            constraintData->SetWasEdited( true );

        UpdateRuleTreeItemText( aRuleTreeItemData->GetTreeItemId(), nodeDetail->m_nodeName );
    }
}


bool DIALOG_DRC_RULE_EDITOR::isEnabled( RULE_TREE_ITEM_DATA*         aRuleTreeItemData,
                                                                      RULE_EDITOR_TREE_CONTEXT_OPT aOption )
{
    RULE_TREE_NODE* nodeDetail = getRuleTreeNodeInfo( aRuleTreeItemData->GetNodeId() );

    switch( aOption )
    {
    case RULE_EDITOR_TREE_CONTEXT_OPT::ADD_RULE:
        return nodeDetail->m_nodeType == DRC_RULE_EDITOR_ITEM_TYPE::CONSTRAINT
               || nodeDetail->m_nodeType == DRC_RULE_EDITOR_ITEM_TYPE::RULE;
    case RULE_EDITOR_TREE_CONTEXT_OPT::DUPLICATE_RULE:
    case RULE_EDITOR_TREE_CONTEXT_OPT::DELETE_RULE: return nodeDetail->m_nodeType == DRC_RULE_EDITOR_ITEM_TYPE::RULE;
    case RULE_EDITOR_TREE_CONTEXT_OPT::MOVE_UP:
    case RULE_EDITOR_TREE_CONTEXT_OPT::MOVE_DOWN:
        return nodeDetail->m_nodeType == DRC_RULE_EDITOR_ITEM_TYPE::RULE;
    default: return true;
    }
}


void DIALOG_DRC_RULE_EDITOR::RemoveRule( int aNodeId )
{
    RULE_TREE_ITEM_DATA* itemData = dynamic_cast<RULE_TREE_ITEM_DATA*>(
            m_ruleTreeCtrl->GetItemData( GetCurrentlySelectedRuleTreeItemData()->GetTreeItemId() ) );
    RULE_TREE_NODE* nodeDetail = getRuleTreeNodeInfo( itemData->GetNodeId() );

    if( !nodeDetail->m_nodeData->IsNew() )
    {
        if( OKOrCancelDialog( this, _( "Confirmation" ), "", _( "Are you sure you want to delete?" ), _( "Delete" ) )
            != wxID_OK )
        {
            return;
        }
    }

    if( itemData )
    {
        int nodeId = itemData->GetNodeId();

        DeleteRuleTreeItem( GetCurrentlySelectedRuleTreeItemData()->GetTreeItemId(), nodeId );
        deleteTreeNodeData( nodeId );
    }

    SetControlsEnabled( true );
}


std::vector<RULE_TREE_NODE> DIALOG_DRC_RULE_EDITOR::buildElectricalRuleTreeNodes( int& aParentId )
{
    std::vector<RULE_TREE_NODE> result;
    int                         lastParentId;

    result.push_back( buildRuleTreeNodeData( "Clearance", DRC_RULE_EDITOR_ITEM_TYPE::CATEGORY, aParentId ) );
    lastParentId = m_nodeId;

    result.push_back( buildRuleTreeNodeData( "Basic clearance", DRC_RULE_EDITOR_ITEM_TYPE::CONSTRAINT, lastParentId,
                                             BASIC_CLEARANCE ) );
    result.push_back( buildRuleTreeNodeData( "Board outline clearance", DRC_RULE_EDITOR_ITEM_TYPE::CONSTRAINT,
                                             lastParentId, BOARD_OUTLINE_CLEARANCE ) );
    result.push_back( buildRuleTreeNodeData( "Minimum clearance", DRC_RULE_EDITOR_ITEM_TYPE::CONSTRAINT, lastParentId,
                                             MINIMUM_CLEARANCE ) );
    result.push_back( buildRuleTreeNodeData( "Minimum item clearance", DRC_RULE_EDITOR_ITEM_TYPE::CONSTRAINT,
                                             lastParentId, MINIMUM_ITEM_CLEARANCE ) );
    result.push_back( buildRuleTreeNodeData( "Copper to edge clearance", DRC_RULE_EDITOR_ITEM_TYPE::CONSTRAINT,
                                             lastParentId, COPPER_TO_EDGE_CLEARANCE ) );
    result.push_back( buildRuleTreeNodeData( "Courtyard Clearance", DRC_RULE_EDITOR_ITEM_TYPE::CONSTRAINT, lastParentId,
                                             COURTYARD_CLEARANCE ) );
    result.push_back( buildRuleTreeNodeData( "Physical Clearance", DRC_RULE_EDITOR_ITEM_TYPE::CONSTRAINT, lastParentId,
                                             PHYSICAL_CLEARANCE ) );
    result.push_back( buildRuleTreeNodeData( "Creepage distance", DRC_RULE_EDITOR_ITEM_TYPE::CONSTRAINT, lastParentId,
                                             CREEPAGE_DISTANCE ) );

    result.push_back( buildRuleTreeNodeData( "Connection Width", DRC_RULE_EDITOR_ITEM_TYPE::CATEGORY, aParentId ) );
    lastParentId = m_nodeId;

    result.push_back( buildRuleTreeNodeData( "Minimum connection width", DRC_RULE_EDITOR_ITEM_TYPE::CONSTRAINT,
                                             lastParentId, MINIMUM_CONNECTION_WIDTH ) );
    result.push_back( buildRuleTreeNodeData( "Minimum track width", DRC_RULE_EDITOR_ITEM_TYPE::CONSTRAINT, lastParentId,
                                             MINIMUM_TRACK_WIDTH ) );

    result.push_back( buildRuleTreeNodeData( "Hole Clearance", DRC_RULE_EDITOR_ITEM_TYPE::CATEGORY, aParentId ) );
    lastParentId = m_nodeId;

    result.push_back( buildRuleTreeNodeData( "Copper to hole clearance", DRC_RULE_EDITOR_ITEM_TYPE::CONSTRAINT,
                                             lastParentId, COPPER_TO_HOLE_CLEARANCE ) );
    result.push_back( buildRuleTreeNodeData( "Hole to hole clearance", DRC_RULE_EDITOR_ITEM_TYPE::CONSTRAINT,
                                             lastParentId, HOLE_TO_HOLE_CLEARANCE ) );

    result.push_back( buildRuleTreeNodeData( "Spoke Count", DRC_RULE_EDITOR_ITEM_TYPE::CATEGORY, aParentId ) );
    lastParentId = m_nodeId;

    result.push_back( buildRuleTreeNodeData( "Minimum thermal relief spoke count",
                                             DRC_RULE_EDITOR_ITEM_TYPE::CONSTRAINT, lastParentId,
                                             MINIMUM_THERMAL_RELIEF_SPOKE_COUNT ) );

    return result;
}


std::vector<RULE_TREE_NODE> DIALOG_DRC_RULE_EDITOR::buildManufacturabilityRuleTreeNodes( int& aParentId )
{
    std::vector<RULE_TREE_NODE> result;
    int                         lastParentId;

    result.push_back( buildRuleTreeNodeData( "Annular Width", DRC_RULE_EDITOR_ITEM_TYPE::CATEGORY, aParentId ) );
    lastParentId = m_nodeId;

    result.push_back( buildRuleTreeNodeData( "Minimum annular width", DRC_RULE_EDITOR_ITEM_TYPE::CONSTRAINT,
                                             lastParentId, MINIMUM_ANNULAR_WIDTH ) );
    result.push_back( buildRuleTreeNodeData( "Hole", DRC_RULE_EDITOR_ITEM_TYPE::CATEGORY, aParentId ) );
    lastParentId = m_nodeId;
    result.push_back( buildRuleTreeNodeData( "Minimum through hole", DRC_RULE_EDITOR_ITEM_TYPE::CONSTRAINT,
                                             lastParentId, MINIMUM_THROUGH_HOLE ) );
    result.push_back( buildRuleTreeNodeData( "Hole to hole distance", DRC_RULE_EDITOR_ITEM_TYPE::CONSTRAINT,
                                             lastParentId, HOLE_TO_HOLE_DISTANCE ) );
    result.push_back( buildRuleTreeNodeData( "Minimum uvia hole", DRC_RULE_EDITOR_ITEM_TYPE::CONSTRAINT, lastParentId,
                                             MINIMUM_UVIA_HOLE ) );
    result.push_back( buildRuleTreeNodeData( "Minimum uvia diameter", DRC_RULE_EDITOR_ITEM_TYPE::CONSTRAINT,
                                             lastParentId, MINIMUM_UVIA_DIAMETER ) );
    result.push_back(
            buildRuleTreeNodeData( "Via style", DRC_RULE_EDITOR_ITEM_TYPE::CONSTRAINT, lastParentId, VIA_STYLE ) );

    result.push_back( buildRuleTreeNodeData( "Text Geometry", DRC_RULE_EDITOR_ITEM_TYPE::CATEGORY, aParentId ) );
    lastParentId = m_nodeId;

    result.push_back( buildRuleTreeNodeData( "Minimum text height and thickness", DRC_RULE_EDITOR_ITEM_TYPE::CONSTRAINT,
                                             lastParentId, MINIMUM_TEXT_HEIGHT_AND_THICKNESS ) );

    result.push_back( buildRuleTreeNodeData( "Silkscreen Clearance", DRC_RULE_EDITOR_ITEM_TYPE::CATEGORY, aParentId ) );
    lastParentId = m_nodeId;

    result.push_back( buildRuleTreeNodeData( "Silk to silk clearance", DRC_RULE_EDITOR_ITEM_TYPE::CONSTRAINT,
                                             lastParentId, SILK_TO_SILK_CLEARANCE ) );
    result.push_back( buildRuleTreeNodeData( "Silk to soldermask clearance", DRC_RULE_EDITOR_ITEM_TYPE::CONSTRAINT,
                                             lastParentId, SILK_TO_SOLDERMASK_CLEARANCE ) );

    result.push_back( buildRuleTreeNodeData( "Soldermask", DRC_RULE_EDITOR_ITEM_TYPE::CATEGORY, aParentId ) );
    lastParentId = m_nodeId;

    result.push_back( buildRuleTreeNodeData( "Minimum soldermask silver", DRC_RULE_EDITOR_ITEM_TYPE::CONSTRAINT,
                                             lastParentId, MINIMUM_SOLDERMASK_SILVER ) );
    result.push_back( buildRuleTreeNodeData( "Soldermask expansion", DRC_RULE_EDITOR_ITEM_TYPE::CONSTRAINT,
                                             lastParentId, SOLDERMASK_EXPANSION ) );

    result.push_back( buildRuleTreeNodeData( "Solderpaste", DRC_RULE_EDITOR_ITEM_TYPE::CATEGORY, aParentId ) );
    lastParentId = m_nodeId;

    result.push_back( buildRuleTreeNodeData( "Solderpaste expansion", DRC_RULE_EDITOR_ITEM_TYPE::CONSTRAINT,
                                             lastParentId, SOLDERPASTE_EXPANSION ) );

    result.push_back( buildRuleTreeNodeData( "Deviation", DRC_RULE_EDITOR_ITEM_TYPE::CATEGORY, aParentId ) );
    lastParentId = m_nodeId;

    result.push_back( buildRuleTreeNodeData( "Maximum allowed deviation", DRC_RULE_EDITOR_ITEM_TYPE::CONSTRAINT,
                                             lastParentId, MAXIMUM_ALLOWED_DEVIATION ) );

    result.push_back( buildRuleTreeNodeData( "Annular Ring", DRC_RULE_EDITOR_ITEM_TYPE::CATEGORY, aParentId ) );
    lastParentId = m_nodeId;

    result.push_back( buildRuleTreeNodeData( "Minimum annular ring", DRC_RULE_EDITOR_ITEM_TYPE::CONSTRAINT,
                                             lastParentId, MINIMUM_ANGULAR_RING ) );

    return result;
}


std::vector<RULE_TREE_NODE> DIALOG_DRC_RULE_EDITOR::buildHighspeedDesignRuleTreeNodes( int& aParentId )
{
    std::vector<RULE_TREE_NODE> result;
    int                         lastParentId;

    result.push_back( buildRuleTreeNodeData( "Diff Pair (width, gap, uncoupled length)",
                                             DRC_RULE_EDITOR_ITEM_TYPE::CATEGORY, aParentId ) );
    lastParentId = m_nodeId;

    result.push_back( buildRuleTreeNodeData( "Routing diff pair", DRC_RULE_EDITOR_ITEM_TYPE::CONSTRAINT, lastParentId,
                                             ROUTING_DIFF_PAIR ) );
    result.push_back( buildRuleTreeNodeData( "Routing width", DRC_RULE_EDITOR_ITEM_TYPE::CONSTRAINT, lastParentId,
                                             ROUTING_WIDTH ) );
    result.push_back( buildRuleTreeNodeData( "Maximum via count", DRC_RULE_EDITOR_ITEM_TYPE::CONSTRAINT, lastParentId,
                                             MAXIMUM_VIA_COUNT ) );

    result.push_back( buildRuleTreeNodeData( "Skew", DRC_RULE_EDITOR_ITEM_TYPE::CATEGORY, aParentId ) );
    lastParentId = m_nodeId;

    result.push_back( buildRuleTreeNodeData( "Length Matching", DRC_RULE_EDITOR_ITEM_TYPE::CATEGORY, aParentId ) );
    lastParentId = m_nodeId;

    result.push_back( buildRuleTreeNodeData( "Matched length diff pair", DRC_RULE_EDITOR_ITEM_TYPE::CONSTRAINT,
                                             lastParentId, MATCHED_LENGTH_DIFF_PAIR ) );
    result.push_back( buildRuleTreeNodeData( "Absolute length", DRC_RULE_EDITOR_ITEM_TYPE::CONSTRAINT, lastParentId,
                                             ABSOLUTE_LENGTH ) );

    return result;
}


std::vector<RULE_TREE_NODE> DIALOG_DRC_RULE_EDITOR::buildFootprintsRuleTreeNodes( int& aParentId )
{
    std::vector<RULE_TREE_NODE> result;
    int                         lastParentId;

    result.push_back( buildRuleTreeNodeData( "Allowed Layers", DRC_RULE_EDITOR_ITEM_TYPE::CATEGORY, aParentId ) );
    lastParentId = m_nodeId;

    result.push_back( buildRuleTreeNodeData( "Permitted layers", DRC_RULE_EDITOR_ITEM_TYPE::CONSTRAINT, lastParentId,
                                             PERMITTED_LAYERS ) );

    result.push_back( buildRuleTreeNodeData( "Orientation", DRC_RULE_EDITOR_ITEM_TYPE::CATEGORY, aParentId ) );
    lastParentId = m_nodeId;

    result.push_back( buildRuleTreeNodeData( "Allowed orientation", DRC_RULE_EDITOR_ITEM_TYPE::CONSTRAINT, lastParentId,
                                             ALLOWED_ORIENTATION ) );

    result.push_back( buildRuleTreeNodeData( "Via Placement", DRC_RULE_EDITOR_ITEM_TYPE::CATEGORY, aParentId ) );
    lastParentId = m_nodeId;

    result.push_back( buildRuleTreeNodeData( "Vias under SMD", DRC_RULE_EDITOR_ITEM_TYPE::CONSTRAINT, lastParentId,
                                             VIAS_UNDER_SMD ) );

    return result;
}


/**
 * Checks if a node with the given name exists in the rule tree or its child nodes.
 *
 * @param aRuleTreeNode The node to check.
 * @param aTargetName The name of the target node to search for.
 * @return true if the node exists, false otherwise.
 */
bool nodeExists( const RULE_TREE_NODE& aRuleTreeNode, const wxString& aTargetName )
{
    if( aRuleTreeNode.m_nodeName == aTargetName )
    {
        return true;
    }

    for( const auto& child : aRuleTreeNode.m_childNodes )
    {
        if( nodeExists( child, aTargetName ) )
        {
            return true;
        }
    }

    return false;
}


/**
 * Checks if a node with the given name exists in a list of rule tree nodes.
 *
 * @param aRuleTreeNodes The list of rule tree nodes to check.
 * @param aTargetName The name of the target node to search for.
 * @return true if the node exists, false otherwise.
 */
bool nodeExists( const std::vector<RULE_TREE_NODE>& aRuleTreeNodes, const wxString& aTargetName )
{
    for( const auto& node : aRuleTreeNodes )
    {
        if( nodeExists( node, aTargetName ) )
        {
            return true;
        }
    }

    return false;
}


RULE_TREE_NODE DIALOG_DRC_RULE_EDITOR::buildRuleTreeNode( RULE_TREE_ITEM_DATA* aRuleTreeItemData )
{
    // Factory function type for creating constraint data objects
    using ConstraintDataFactory =
            std::function<std::shared_ptr<DRC_RE_BASE_CONSTRAINT_DATA>( const DRC_RE_BASE_CONSTRAINT_DATA& )>;

    // Factory map for constraint data creation
    static const std::unordered_map<DRC_RULE_EDITOR_CONSTRAINT_NAME, ConstraintDataFactory> s_constraintFactories = {
        { DRC_RULE_EDITOR_CONSTRAINT_NAME::VIA_STYLE,
          []( const DRC_RE_BASE_CONSTRAINT_DATA& data )
          {
              return std::make_shared<DRC_RE_VIA_STYLE_CONSTRAINT_DATA>( data );
          } },
        { DRC_RULE_EDITOR_CONSTRAINT_NAME::MINIMUM_TEXT_HEIGHT_AND_THICKNESS,
          []( const DRC_RE_BASE_CONSTRAINT_DATA& data )
          {
              return std::make_shared<DRC_RE_MINIMUM_TEXT_HEIGHT_THICKNESS_CONSTRAINT_DATA>( data );
          } },
        { DRC_RULE_EDITOR_CONSTRAINT_NAME::ROUTING_DIFF_PAIR,
          []( const DRC_RE_BASE_CONSTRAINT_DATA& data )
          {
              return std::make_shared<DRC_RE_ROUTING_DIFF_PAIR_CONSTRAINT_DATA>( data );
          } },
        { DRC_RULE_EDITOR_CONSTRAINT_NAME::ROUTING_WIDTH,
          []( const DRC_RE_BASE_CONSTRAINT_DATA& data )
          {
              return std::make_shared<DRC_RE_ROUTING_WIDTH_CONSTRAINT_DATA>( data );
          } },
        { DRC_RULE_EDITOR_CONSTRAINT_NAME::PERMITTED_LAYERS,
          []( const DRC_RE_BASE_CONSTRAINT_DATA& data )
          {
              return std::make_shared<DRC_RE_PERMITTED_LAYERS_CONSTRAINT_DATA>( data );
          } },
        { DRC_RULE_EDITOR_CONSTRAINT_NAME::ALLOWED_ORIENTATION,
          []( const DRC_RE_BASE_CONSTRAINT_DATA& data )
          {
              return std::make_shared<DRC_RE_ALLOWED_ORIENTATION_CONSTRAINT_DATA>( data );
          } },
        { DRC_RULE_EDITOR_CONSTRAINT_NAME::CUSTOM_RULE,
          []( const DRC_RE_BASE_CONSTRAINT_DATA& data )
          {
              return std::make_shared<DRC_RE_CUSTOM_RULE_CONSTRAINT_DATA>( data );
          } }
    };

    RULE_TREE_ITEM_DATA* treeItemData;
    RULE_TREE_NODE*      nodeDetail = getRuleTreeNodeInfo( aRuleTreeItemData->GetNodeId() );

    if( nodeDetail->m_nodeType == CONSTRAINT )
    {
        treeItemData = aRuleTreeItemData;
    }
    else
    {
        treeItemData = dynamic_cast<RULE_TREE_ITEM_DATA*>(
                m_ruleTreeCtrl->GetItemData( aRuleTreeItemData->GetParentTreeItemId() ) );
        nodeDetail = getRuleTreeNodeInfo( treeItemData->GetNodeId() );
    }

    m_nodeId++;

    wxString nodeName = nodeDetail->m_nodeName + " 1";

    int  loop = 2;
    bool check = false;

    do
    {
        check = false;

        if( nodeExists( m_ruleTreeNodeDatas, nodeName ) )
        {
            nodeName = nodeDetail->m_nodeName + wxString::Format( " %d", loop );
            loop++;
            check = true;
        }
    } while( check );

    RULE_TREE_NODE newRuleNode = buildRuleTreeNodeData(
            nodeName.ToStdString(), RULE, nodeDetail->m_nodeId,
            static_cast<DRC_RULE_EDITOR_CONSTRAINT_NAME>( nodeDetail->m_nodeTypeMap.value_or( 0 ) ), {}, m_nodeId );

    auto nodeType = static_cast<DRC_RULE_EDITOR_CONSTRAINT_NAME>( newRuleNode.m_nodeTypeMap.value_or( -1 ) );

    DRC_RE_BASE_CONSTRAINT_DATA clearanceData( m_nodeId, nodeDetail->m_nodeData->GetId(), newRuleNode.m_nodeName );

    if( s_constraintFactories.find( nodeType ) != s_constraintFactories.end() )
    {
        newRuleNode.m_nodeData = s_constraintFactories.at( nodeType )( clearanceData );
    }
    else if( DRC_RULE_EDITOR_UTILS::IsNumericInputType( nodeType ) )
    {
        newRuleNode.m_nodeData = std::make_shared<DRC_RE_NUMERIC_INPUT_CONSTRAINT_DATA>( clearanceData );
    }
    else if( DRC_RULE_EDITOR_UTILS::IsBoolInputType( nodeType ) )
    {
        newRuleNode.m_nodeData = std::make_shared<DRC_RE_BOOL_INPUT_CONSTRAINT_DATA>( clearanceData );
    }
    else
    {
        wxLogWarning( "No factory found for constraint type: %d", nodeType );
        newRuleNode.m_nodeData = std::make_shared<DRC_RE_BASE_CONSTRAINT_DATA>( clearanceData );
    }

    std::static_pointer_cast<DRC_RE_BASE_CONSTRAINT_DATA>( newRuleNode.m_nodeData )
            ->SetConstraintCode( DRC_RULE_EDITOR_UTILS::ConstraintToKicadDrc( nodeType ) );
    newRuleNode.m_nodeData->SetIsNew( true );

    m_ruleTreeNodeDatas.push_back( newRuleNode );

    return newRuleNode;
}


RULE_TREE_NODE* DIALOG_DRC_RULE_EDITOR::getRuleTreeNodeInfo( const int& aNodeId )
{
    auto it = std::find_if( m_ruleTreeNodeDatas.begin(), m_ruleTreeNodeDatas.end(),
                            [aNodeId]( const RULE_TREE_NODE& node )
                            {
                                return node.m_nodeId == aNodeId;
                            } );

    if( it != m_ruleTreeNodeDatas.end() )
    {
        return &( *it ); // Return pointer to the found node
    }
    else
        return nullptr;
}


void DIALOG_DRC_RULE_EDITOR::saveRule( int aNodeId )
{
    if( !m_ruleEditorPanel->GetIsValidationSucceeded() )
    {
        std::string validationMessage = m_ruleEditorPanel->GetValidationMessage();

        DisplayErrorMessage( this, validationMessage );
    }
    else
    {
        RULE_TREE_ITEM_DATA* itemData = dynamic_cast<RULE_TREE_ITEM_DATA*>(
                m_ruleTreeCtrl->GetItemData( GetCurrentlySelectedRuleTreeItemData()->GetTreeItemId() ) );

        if( itemData )
        {
            UpdateRuleTypeTreeItemData( itemData );
        }

        SaveRulesToFile();

        SetControlsEnabled( true );
    }
}


void DIALOG_DRC_RULE_EDITOR::closeRuleEntryView( int aNodeId )
{
    SetControlsEnabled( true );
}


int DIALOG_DRC_RULE_EDITOR::highlightMatchingItems( int aNodeId )
{
    (void) aNodeId;

    if( !m_ruleEditorPanel )
        return -1;

    // Ensure we use the latest text from the condition editor
    m_ruleEditorPanel->TransferDataFromWindow();
    wxString condition = m_ruleEditorPanel->GetConstraintData()->GetRuleCondition();

    wxLogTrace( wxS( "KI_TRACE_DRC_RULE_EDITOR" ),
                wxS( "[ShowMatches] nodeId=%d, condition='%s'" ), aNodeId, condition );

    // Empty condition matches nothing
    if( condition.IsEmpty() )
    {
        m_frame->FocusOnItems( {} );
        Raise();
        return 0;
    }

    // Pre-compile condition to detect syntax errors
    DRC_RULE_CONDITION testCondition( condition );

    if( !testCondition.Compile( nullptr ) )
    {
        wxLogTrace( wxS( "KI_TRACE_DRC_RULE_EDITOR" ), wxS( "[ShowMatches] compile failed" ) );
        return -1;
    }

    m_drcTool = m_frame->GetToolManager()->GetTool<DRC_TOOL>();

    std::vector<BOARD_ITEM*> allMatches =
            m_drcTool->GetDRCEngine()->GetItemsMatchingCondition( condition, ASSERTION_CONSTRAINT, m_reporter );

    // Filter out items without visible geometry
    std::vector<BOARD_ITEM*> matches;

    for( BOARD_ITEM* item : allMatches )
    {
        switch( item->Type() )
        {
        case PCB_NETINFO_T:
        case PCB_GENERATOR_T:
        case PCB_GROUP_T:
            continue;

        default:
            matches.push_back( item );
            break;
        }
    }

    int matchCount = static_cast<int>( matches.size() );

    wxLogTrace( wxS( "KI_TRACE_DRC_RULE_EDITOR" ), wxS( "[ShowMatches] matched_count=%d (filtered from %zu)" ),
                matchCount, allMatches.size() );

    // Clear any existing selection and select matched items
    m_frame->GetToolManager()->RunAction( ACTIONS::selectionClear );

    if( matches.size() > 0 )
    {
        std::vector<EDA_ITEM*> selectItems;

        for( BOARD_ITEM* item : matches )
            selectItems.push_back( item );

        m_frame->GetToolManager()->RunAction( ACTIONS::selectItems, &selectItems );
        m_frame->GetToolManager()->RunAction( ACTIONS::zoomFitSelection );
    }

    // Also brighten items to provide additional visual feedback
    m_frame->FocusOnItems( matches );
    Raise();

    return matchCount;
}


bool DIALOG_DRC_RULE_EDITOR::validateRuleName( int aNodeId, wxString aRuleName )
{
    auto it = std::find_if( m_ruleTreeNodeDatas.begin(), m_ruleTreeNodeDatas.end(),
                            [aNodeId, aRuleName]( const RULE_TREE_NODE& node )
                            {
                                return node.m_nodeName == aRuleName && node.m_nodeId != aNodeId
                                       && node.m_nodeType == RULE;
                            } );

    if( it != m_ruleTreeNodeDatas.end() )
    {
        return false;
    }

    return true;
}


bool DIALOG_DRC_RULE_EDITOR::deleteTreeNodeData( const int& aNodeId )
{
    size_t initial_size = m_ruleTreeNodeDatas.size();

    m_ruleTreeNodeDatas.erase( std::remove_if( m_ruleTreeNodeDatas.begin(), m_ruleTreeNodeDatas.end(),
                                               [aNodeId]( const RULE_TREE_NODE& node )
                                               {
                                                   return node.m_nodeId == aNodeId;
                                               } ),
                               m_ruleTreeNodeDatas.end() );

    if( m_ruleTreeNodeDatas.size() < initial_size )
        return true;
    else
        return false;
}


void DIALOG_DRC_RULE_EDITOR::collectChildRuleNodes( int aParentId, std::vector<RULE_TREE_NODE*>& aResult )
{
    std::vector<RULE_TREE_NODE> children;
    getRuleTreeChildNodes( m_ruleTreeNodeDatas, aParentId, children );

    for( const auto& child : children )
    {
        RULE_TREE_NODE* childNode = getRuleTreeNodeInfo( child.m_nodeId );

        if( childNode->m_nodeType == RULE )
            aResult.push_back( childNode );

        collectChildRuleNodes( childNode->m_nodeId, aResult );
    }
}


RULE_TREE_NODE DIALOG_DRC_RULE_EDITOR::buildRuleTreeNodeData(
        const std::string& aName, const DRC_RULE_EDITOR_ITEM_TYPE& aNodeType, const std::optional<int>& aParentId,
        const std::optional<DRC_RULE_EDITOR_CONSTRAINT_NAME>& aConstraintType,
        const std::vector<RULE_TREE_NODE>& aChildNodes, const std::optional<int>& id )
{
    unsigned int newId;

    if( id )
    {
        newId = *id; // Use provided ID
    }
    else
    {
        newId = 1;

        if( m_nodeId )
            newId = m_nodeId + 1;
    }

    m_nodeId = newId;

    RULE_EDITOR_DATA_BASE baseData;
    baseData.SetId( newId );

    if( aParentId )
    {
        baseData.SetParentId( *aParentId );
    }

    return { .m_nodeId = m_nodeId,
             .m_nodeName = aName,
             .m_nodeType = aNodeType,
             .m_nodeLevel = -1,
             .m_nodeTypeMap = aConstraintType,
             .m_childNodes = aChildNodes,
             .m_nodeData = std::make_shared<RULE_EDITOR_DATA_BASE>( baseData ) };
}


RULE_TREE_NODE DIALOG_DRC_RULE_EDITOR::buildRuleNodeFromKicadDrc( const wxString& aName, const wxString& aCode,
                                                                  const std::optional<int>& aParentId )
{
    auto           typeOpt = DRC_RULE_EDITOR_UTILS::GetConstraintTypeFromCode( aCode );
    RULE_TREE_NODE node =
            buildRuleTreeNodeData( aName.ToStdString(), DRC_RULE_EDITOR_ITEM_TYPE::CONSTRAINT, aParentId, typeOpt );

    auto baseData = std::dynamic_pointer_cast<DRC_RE_BASE_CONSTRAINT_DATA>( node.m_nodeData );
    DRC_RULE_EDITOR_UTILS::ConstraintFromKicadDrc( aCode, baseData.get() );
    node.m_nodeData = baseData;
    return node;
}


bool DIALOG_DRC_RULE_EDITOR::updateUI()
{
    return !m_cancelled;
}


void DIALOG_DRC_RULE_EDITOR::AdvancePhase( const wxString& aMessage )
{
    PROGRESS_REPORTER_BASE::AdvancePhase( aMessage );
    SetCurrentProgress( 0.0 );
}


void DIALOG_DRC_RULE_EDITOR::UpdateData()
{
    m_markersTreeModel->Update( m_markersProvider, m_severities );
}

void DIALOG_DRC_RULE_EDITOR::SaveRulesToFile()
{
    std::vector<DRC_RE_LOADED_PANEL_ENTRY> entries;

    for( const RULE_TREE_NODE& node : m_ruleTreeNodeDatas )
    {
        if( node.m_nodeType != RULE )
            continue;

        auto data = std::dynamic_pointer_cast<DRC_RE_BASE_CONSTRAINT_DATA>( node.m_nodeData );

        if( !data )
            continue;

        DRC_RE_LOADED_PANEL_ENTRY entry;

        if( node.m_nodeTypeMap )
            entry.panelType = static_cast<DRC_RULE_EDITOR_CONSTRAINT_NAME>( *node.m_nodeTypeMap );
        else
            entry.panelType = CUSTOM_RULE;

        entry.constraintData = data;
        entry.ruleName = data->GetRuleName();
        entry.condition = data->GetRuleCondition();
        entry.originalRuleText = data->GetOriginalRuleText();
        entry.wasEdited = data->WasEdited();

        entries.push_back( entry );
    }

    DRC_RULE_SAVER saver;
    saver.SaveFile( m_frame->GetDesignRulesPath(), entries, m_currentBoard );
}
