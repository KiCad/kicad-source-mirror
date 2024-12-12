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


#ifndef DIALOG_DRC_RULE_EDITOR_H
#define DIALOG_DRC_RULE_EDITOR_H


#include <drc/drc_rule_parser.h>
#include <thread_pool.h>
#include <drc/drc_item.h>
#include "tools/drc_tool.h"
#include <tool/tool_manager.h>
#include <drc/drc_rtree.h>

#include <widgets/progress_reporter_base.h>

#include <dialogs/rule_editor_dialog_base.h>
#include "panel_drc_rule_editor.h"
#include "drc_rule_editor_utils.h"
#include "panel_drc_group_header.h"


class PCB_EDIT_FRAME;
class DIALOG_DRC_RULE_EDITOR;
class TOOL_MANAGER;
class DRC_RTREE;

class DIALOG_DRC_RULE_EDITOR : public RULE_EDITOR_DIALOG_BASE, PROGRESS_REPORTER_BASE
{
public:
    DIALOG_DRC_RULE_EDITOR( PCB_EDIT_FRAME* aEditorFrame, wxWindow* aParent );

    ~DIALOG_DRC_RULE_EDITOR();

    bool TransferDataToWindow() override;

    bool TransferDataFromWindow() override;

    std::vector<RULE_TREE_NODE> GetDefaultRuleTreeItems() override;

    /**
     * Adds a new rule to the rule tree, either as a child or under the parent,
     * based on the node type (CONSTRAINT or not).
     *
     * @param aRuleTreeItemData The data for the rule tree item to be added.
     */
    void AddNewRule( RULE_TREE_ITEM_DATA* aRuleTreeItemData ) override;

    /**
     * Duplicates a rule from the source tree node and appends it as a new item under the same parent.
     *
     * @param aRuleTreeItemData The data of the rule to be duplicated.
     */
    void DuplicateRule( RULE_TREE_ITEM_DATA* aRuleTreeItemData ) override;

    /**
     * Handles rule tree item selection changes, updating the content panel with appropriate editor or header panel.
     *
     * @param aCurrentRuleTreeItemData The data of the currently selected rule tree item.
     */
    void RuleTreeItemSelectionChanged( RULE_TREE_ITEM_DATA* aCurrentRuleTreeItemData ) override;

    /**
     * Updates the rule tree item data by transferring data from the rule editor panel and updating the item text.
     *
     * @param aRuleTreeItemData The data of the rule tree item to be updated.
     */
    void UpdateRuleTypeTreeItemData( RULE_TREE_ITEM_DATA* aCurrentRuleTreeItemData ) override;

    /**
     * Verifies if a context menu option should be enabled based on the rule tree item type.
     *
     * @param aRuleTreeItemData The data of the rule tree item.
     * @param aOption The context menu option to verify.
     *
     * @return true if the option should be enabled, false otherwise.
     */
    bool isEnabled( RULE_TREE_ITEM_DATA*         aRuleTreeItemData,
                                                  RULE_EDITOR_TREE_CONTEXT_OPT aOption ) override;

    /**
     * Removes a rule from the rule tree after confirmation, deleting the item and associated data.
     *
     * @param aNodeId The ID of the node to be removed.
     */
    void RemoveRule( int aNodeId ) override;

    void UpdateData();

    void OnSave( wxCommandEvent& aEvent ) override;

    void OnCancel( wxCommandEvent& aEvent ) override;

private:
    std::vector<RULE_TREE_NODE> buildElectricalRuleTreeNodes( int& aParentId );

    std::vector<RULE_TREE_NODE> buildManufacturabilityRuleTreeNodes( int& aParentId );

    std::vector<RULE_TREE_NODE> buildHighspeedDesignRuleTreeNodes( int& aParentId );

    std::vector<RULE_TREE_NODE> buildFootprintsRuleTreeNodes( int& aParentId );

    void LoadExistingRules();

    void SaveRulesToFile();

    /**
     * Creates a new rule tree node with a unique name and assigns the appropriate constraint data.
     *
     * @param aRuleTreeItemData The rule tree item data for the node.
     *
     * @return The newly created rule tree node.
     */
    RULE_TREE_NODE buildRuleTreeNode( RULE_TREE_ITEM_DATA* aRuleTreeItemData );

    /**
     * Creates a new rule tree node with the specified parameters, generating a new ID if not provided.
     *
     * @param aName The name of the node.
     * @param aNodeType The type of the node (e.g., RULE, CONSTRAINT).
     * @param aParentId The ID of the parent node, if any.
     * @param aConstraintType The constraint type, if any.
     * @param aChildNodes List of child nodes.
     * @param id Optional ID for the node.
     *
     * @return The newly created RULE_TREE_NODE.
     */
    RULE_TREE_NODE
    buildRuleTreeNodeData( const std::string& aName, const DRC_RULE_EDITOR_ITEM_TYPE& aNodeType,
                           const std::optional<int>&                             aParentId = std::nullopt,
                           const std::optional<DRC_RULE_EDITOR_CONSTRAINT_NAME>& aConstraintType = std::nullopt,
                           const std::vector<RULE_TREE_NODE>&                    aChildNodes = {},
                           const std::optional<int>&                             aId = std::nullopt );

    /**
     * Build a rule tree node from a constraint keyword loaded from a
     * .kicad_drc file.
     */
    RULE_TREE_NODE buildRuleNodeFromKicadDrc( const wxString& aName, const wxString& aCode,
                                              const std::optional<int>& aParentId = std::nullopt );

    /**
     * Retrieves the rule tree node for a given ID.
     * Returns a pointer to the node if found, or nullptr if not.
     *
     * @param aNodeId The node ID to search for.
     *
     * @return A pointer to the node, or nullptr if not found.
     */
    RULE_TREE_NODE* getRuleTreeNodeInfo( const int& aNodeId );

    /**
     * Saves the rule after validating the rule editor panel.
     * Displays an error message if validation fails, otherwise updates the rule tree data.
     *
     * @param aNodeId The ID of the rule node to save.
     */
    void saveRule( int aNodeId );

    /**
     * Closes the rule entry view and re-enables controls.
     *
     * @param aNodeId The ID of the rule node being closed.
     */
    void closeRuleEntryView( int aNodeId );

    /**
     * Highlights board items matching the current rule condition.
     *
     * @param aNodeId The ID of the rule node (currently unused).
     * @return The number of matching items, or -1 if the condition has a syntax error.
     */
    int highlightMatchingItems( int aNodeId );

    /**
     * Validates if the rule name is unique for the given node ID.
     *
     * @param aNodeId The ID of the node to exclude from the check.
     * @param aRuleName The rule name to validate.
     *
     * @return True if the rule name is unique, false otherwise.
     */
    bool validateRuleName( int aNodeId, wxString aRuleName );

    /**
     * Deletes a rule tree node by its ID.
     *
     * @param aNodeId The ID of the node to delete.
     *
     * @return True if the node was found and deleted, false otherwise.
     */
    bool deleteTreeNodeData( const int& aNodeId );

    /**
     * Collects all child rule nodes for a given parent node ID.
     *
     * @param aParentId The ID of the parent node.
     * @param aResult The vector to store the child nodes.
     */
    void collectChildRuleNodes( int aParentId, std::vector<RULE_TREE_NODE*>& aResult );

    // PROGRESS_REPORTER calls
    bool updateUI() override;

    void AdvancePhase( const wxString& aMessage ) override;


protected:
    BOARD_DESIGN_SETTINGS& bds() { return m_currentBoard->GetDesignSettings(); }

    BOARD*          m_currentBoard; // the board currently on test
    PCB_EDIT_FRAME* m_frame;
    REPORTER*       m_reporter;
    DRC_TOOL*       m_drcTool;
    wxDataViewCtrl* m_markerDataView;

private:
    int                         m_nodeId;
    std::vector<RULE_TREE_NODE> m_ruleTreeNodeDatas;
    PANEL_DRC_RULE_EDITOR*      m_ruleEditorPanel;
    PANEL_DRC_GROUP_HEADER*     m_groupHeaderPanel;

    std::shared_ptr<RC_ITEMS_PROVIDER> m_markersProvider;
    RC_TREE_MODEL*                     m_markersTreeModel;
    int                                m_severities;
};

#endif //DIALOG_DRC_RULE_EDITOR_H
