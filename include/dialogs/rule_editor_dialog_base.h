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

#ifndef RULE_EDITOR_DIALOG_BASE_H
#define RULE_EDITOR_DIALOG_BASE_H

#include <wx/treectrl.h>
#include <wx/srchctrl.h>
#include <wx/bmpbuttn.h>
#include <wx/splitter.h>

#include <variant>
#include <optional>
#include <unordered_map>

#include <dialog_shim.h>
#include <dialogs/rule_editor_data_base.h>

class WX_INFOBAR;
class wxDragImage;

/**
 * Enumeration representing the available context menu options for the rule editor tree.
 */
enum RULE_EDITOR_TREE_CONTEXT_OPT
{
    ADD_RULE = 0,
    DUPLICATE_RULE,
    DELETE_RULE,
    MOVE_UP,
    MOVE_DOWN,
};

/**
 * Structure representing a node in a rule tree, collection of this used for building the rule tree.
 */
struct RULE_TREE_NODE
{
    int                                    m_nodeId;
    wxString                               m_nodeName;
    int                                    m_nodeType;
    int                                    m_nodeLevel;
    std::optional<int>                     m_nodeTypeMap;
    std::vector<RULE_TREE_NODE>            m_childNodes;
    std::shared_ptr<RULE_EDITOR_DATA_BASE> m_nodeData;
};


/**
 * A class representing additional data associated with a wxTree item.
 * This class is used to store and manage metadata for individual items in a wxTree,
 * linking each tree item to its corresponding node ID and parent tree item.
 */
class RULE_TREE_ITEM_DATA : public wxTreeItemData
{
public:
    explicit RULE_TREE_ITEM_DATA( int aNodeId, wxTreeItemId aParentTreeItemId,
                                  wxTreeItemId aTreeItemId ) :
            m_nodeId( aNodeId ),
            m_treeItemId( aTreeItemId ),
            m_parentTreeItemId( aParentTreeItemId )
    {
    }

    int GetNodeId() const { return m_nodeId; }

    wxTreeItemId GetParentTreeItemId() const { return m_parentTreeItemId; }

    void SetParentTreeItemId( wxTreeItemId aParentTreeItemId )
    {
        m_parentTreeItemId = aParentTreeItemId;
    }

    wxTreeItemId GetTreeItemId() const { return m_treeItemId; }

    void SetTreeItemId( wxTreeItemId aTreeItemId ) { m_treeItemId = aTreeItemId; }

private:
    int          m_nodeId;
    wxTreeItemId m_treeItemId;
    wxTreeItemId m_parentTreeItemId;
};


class RULE_EDITOR_DIALOG_BASE : public DIALOG_SHIM
{
    friend class DIALOG_DRC_RULE_EDITOR;

public:
    RULE_EDITOR_DIALOG_BASE( wxWindow* aParent, const wxString& aTitle,
                             const wxSize& aInitialSize = wxDefaultSize );

    ~RULE_EDITOR_DIALOG_BASE() override;

    /**
     * Enumeration representing the available context menu options for the rule editor tree.
     */
    enum RULE_EDITOR_TREE_CONTEXT_OPT_ID
    {
        ID_NEWRULE = wxID_HIGHEST + 1,
        ID_COPYRULE,
        ID_DELETERULE,
        ID_MOVEUP,
        ID_MOVEDOWN
    };

    /**
     * Gets the tree control used for displaying and managing rules.
     *
     * @return A pointer to the wxTreeCtrl instance.
     */
    wxTreeCtrl* GetRuleTreeCtrl() { return m_ruleTreeCtrl; }

    /**
     * Marks the dialog as modified, typically used to indicate unsaved changes.
     */
    void SetModified() { m_modified = true; }

    /**
     * Static method to retrieve the rule editor dialog instance associated with a given window.
     *
     * @param aWindow The window for which the dialog is being retrieved.
     *
     * @return A pointer to the RULE_EDITOR_DIALOG_BASE instance, or nullptr if not found.
     */
    static RULE_EDITOR_DIALOG_BASE* GetDialog( wxWindow* aWindow );

    /**
     * Pure virtual method to get the default rule tree items.
     * Must be implemented by derived classes.
     *
     * @return A vector of default RULE_TREE_NODE items.
     */
    virtual std::vector<RULE_TREE_NODE> GetDefaultRuleTreeItems() = 0;

    /**
     * Pure virtual method to add a new rule to the tree.
     * Must be implemented by derived classes.
     *
     * @param aRuleTreeItemData The data associated with the new rule.
     */
    virtual void AddNewRule( RULE_TREE_ITEM_DATA* aRuleTreeItemData ) = 0;

    /**
     * Pure virtual method to duplicate an existing rule in the tree.
     * Must be implemented by derived classes.
     *
     * @param aRuleTreeItemData The data of the rule to duplicate.
     */
    virtual void DuplicateRule( RULE_TREE_ITEM_DATA* aRuleTreeItemData ) = 0;

    /**
     * Pure virtual method to remove a rule from the tree.
     * Must be implemented by derived classes.
     *
     * @param aNodeId The ID of the rule node to remove.
     */
    virtual void RemoveRule( int aNodeId ) = 0;

    /**
     * Pure virtual method to handle tree item selection changes.
     * Must be implemented by derived classes.
     *
     * @param aCurrentRuleTreeItemData The data of the newly selected rule tree item.
     */
    virtual void RuleTreeItemSelectionChanged( RULE_TREE_ITEM_DATA* aCurrentRuleTreeItemData ) = 0;

    /**
     * Pure virtual method to update the rule tree item data.
     * Must be implemented by derived classes.
     *
     * @param aRuleTreeItemData The data of the rule tree item to be updated.
     */
    virtual void UpdateRuleTypeTreeItemData( RULE_TREE_ITEM_DATA* aRuleTreeItemData ) = 0;

    /**
     * Pure virtual method to verify if a context menu option for a rule tree item should be enabled.
     * Must be implemented by derived classes.
     *
     * @param aRuleTreeItemData The data of the rule tree item to check.
     * @param aOption The context menu option to verify.
     *
     * @return true if the option should be enabled, false otherwise.
     */
    virtual bool isEnabled( RULE_TREE_ITEM_DATA* aRuleTreeItemData,
                                                          RULE_EDITOR_TREE_CONTEXT_OPT aOption ) = 0;

    /**
     * Initializes the rule tree by adding nodes, setting up the structure, and saving its state.
     *
     * @param aRuleTreeNodes A vector of rule tree nodes with their IDs, names, and child nodes.
     */
    void InitRuleTreeItems( const std::vector<RULE_TREE_NODE>& aRuleTreeNodes );

    /**
     * Retrieves the current content panel.
     *
     * @return A pointer to the current content panel.
     */
    wxPanel* GetContentPanel() { return m_contentPanel; }

    /**
     * Replaces the current content panel with a new one based on the selected constraint type.
     *
     * @param aContentPanel The new content panel to replace the existing one.
     */
    void SetContentPanel( wxPanel* aContentPanel );

    /**
     * Adds a new rule tree item under the specified parent and updates the tree history.
     *
     * @param aRuleTreeNode The node data to add.
     * @param aParentTreeItemId The parent item's ID.
     */
    void AppendNewRuleTreeItem( const RULE_TREE_NODE& aRuleTreeNode,
                                wxTreeItemId aParentTreeItemId );

    /**
     * Retrieves the currently selected rule tree item data.
     *
     * @return A pointer to the currently selected rule tree item data.
     */
    RULE_TREE_ITEM_DATA* GetCurrentlySelectedRuleTreeItemData() { return m_selectedData; }

    /**
     * Retrieves the previously selected rule tree item ID.
     *
     * @return The ID of the previously selected rule tree item.
     */
    wxTreeItemId GetPreviouslySelectedRuleTreeItemId() { return m_previousId; }

    /**
     * Updates the text of a specified rule tree item.
     *
     * @param aItemId The ID of the tree item to update.
     * @param aItemText The new text to set for the tree item.
     */
    void UpdateRuleTreeItemText( wxTreeItemId aItemId, wxString aItemText );

    /**
     * Enables or disables controls within the rule editor dialog.
     *
     * @param aEnable true to enable the controls, false to disable them.
     */
    void SetControlsEnabled( bool aEnable );

    /**
     * Deletes a tree item and removes its corresponding node from history.
     *
     * @param aItemId The tree item ID to delete.
     * @param aNodeId The node ID to remove from history.
     */
    void DeleteRuleTreeItem( wxTreeItemId aItemId, const int& aNodeId );

    virtual void OnSave( wxCommandEvent& aEvent ) = 0;

    virtual void OnCancel( wxCommandEvent& aEvent ) = 0;

protected:
    void finishInitialization();

    bool TransferDataToWindow() override;

    bool TransferDataFromWindow() override;

private:
    /**
     * Populates the rule tree with nodes and their children.
     *
     * @param aRuleTreeNodes All rule tree nodes.
     * @param aRuleTreeNode Current node to add.
     * @param aParentTreeItemId Parent item ID for the current node.
     */
    void populateRuleTreeCtrl( const std::vector<RULE_TREE_NODE>& aRuleTreeNodes,
                               const RULE_TREE_NODE& aRuleTreeNode,
                               wxTreeItemId aParentTreeItemId );

    /**
     * Handles right-click on a rule tree item to create a context menu.
     *
     * @param aEvent The right-click event.
     */
    void onRuleTreeItemRightClick( wxTreeEvent& aEvent );

    /**
     * Updates action buttons based on the selected tree item.
     *
     * @param aEvent The selection change event.
     */
    void onRuleTreeItemSelectionChanged( wxTreeEvent& aEvent );

    /**
     * Handles double-click activation of a tree item.
     * Creates a new rule of the activated type when appropriate.
     */
    void onRuleTreeItemActivated( wxTreeEvent& aEvent );

    /**
     * Creates a new rule when the "New Rule" option is clicked.
     *
     * @param aEvent The command event triggered by the click.
     */
    void onNewRuleOptionClick( wxCommandEvent& aEvent );

    /**
     * Duplicates the selected rule when "Duplicate Rule" is clicked.
     *
     * @param aEvent The command event triggered by the click.
     */
    void onDuplicateRuleOptionClick( wxCommandEvent& aEvent );

    /**
     * Deletes the selected rule when "Delete Rule" is clicked.
     *
     * @param aEvent The command event triggered by the click.
     */
    void onDeleteRuleOptionClick( wxCommandEvent& aEvent );

    /**
     * Moves a rule item up in the tree when "Move Up" is clicked.
     *
     * @param aEvent The command event triggered by the click.
     */
    void onMoveUpRuleOptionClick( wxCommandEvent& aEvent );

    /**
     * Moves a rule item down in the tree when "Move Down" is clicked.
     *
     * @param aEvent The command event triggered by the click.
     */
    void onMoveDownRuleOptionClick( wxCommandEvent& aEvent );

    /**
     * Initiates drag operation for a tree item on mouse down.
     *
     * @param aEvent The mouse down event.
     */
    void onRuleTreeItemLeftDown( wxMouseEvent& aEvent );

   /**
     * Handles drag motion to move the item along with the cursor.
     *
     * @param aEvent The mouse motion event during drag.
     */
    void onRuleTreeItemMouseMotion( wxMouseEvent& aEvent );

    /**
     * Completes the drag operation on mouse release.
     *
     * @param aEvent The mouse release event.
     */
    void onRuleTreeItemLeftUp( wxMouseEvent& aEvent );

    /**
     * Applies filter to the rule tree based on the search string.
     *
     * @param aEvent The command event containing the filter string.
     */
    void onFilterSearch( wxCommandEvent& aEvent );

    /**
     * Recursively filters tree items to show only those matching the filter.
     *
     * @param aItem The tree item to check.
     * @param aFilter The filter string.
     * @return True if the item matches the filter or has visible children.
     */
    bool filterRuleTree( const wxTreeItemId& aItem, const wxString& aFilter );

    /**
     * Saves the state of a tree item to history.
     *
     * @param aItem The item to save.
     * @param aNodeId The node ID (optional).
     */
    void saveRuleTreeState( const wxTreeItemId& aItem, const int& aNodeId = 0 );

    /**
     * Restores a tree item from history and appends it under a parent.
     *
     * @param aParent The parent item to append to.
     * @param aNodeId The node ID to restore.
     */
    void restoreRuleTree( const wxTreeItemId& aParent, const int& aNodeId );

    /**
     * Appends a new rule item to the tree.
     *
     * @param aRuleTreeNode The rule tree node for the new item.
     * @param aParentTreeItemId Parent item ID.
     * @return The newly created tree item ID.
     */
    wxTreeItemId appendRuleTreeItem( const RULE_TREE_NODE& aRuleTreeNode,
                                     wxTreeItemId aParentTreeItemId );

    /**
     * Retrieves child nodes of a given parent node.
     *
     * @param aNodes List of all nodes.
     * @param aParentId The parent node ID.
     * @param aResult A vector to store child nodes.
     */
    void getRuleTreeChildNodes( const std::vector<RULE_TREE_NODE>& aNodes, int aParentId,
                                std::vector<RULE_TREE_NODE>& aResult );

    /**
     * Recursively moves all child nodes of a source item to a destination during drag.
     *
     * @param aSrcTreeItemId Source item ID.
     * @param aDestTreeItemId Destination item ID.
     */
    void moveRuleTreeItemChildrensTooOnDrag( wxTreeItemId aSrcTreeItemId,
                                             wxTreeItemId aDestTreeItemId );

    /**
     * Updates the state of move options (up/down) for the selected item.
     */
    void updateRuleTreeItemMoveOptionState();

    /**
     * Updates the action buttons based on the current selection.
     */
    void updateRuleTreeActionButtonsState( RULE_TREE_ITEM_DATA* aRuleTreeItemData );

    void onResize( wxSizeEvent& event );

    void onClose( wxCloseEvent& aEvt );

protected:
    wxTreeCtrl*       m_ruleTreeCtrl;
    WX_INFOBAR*       m_infoBar;
    wxPanel*          m_contentPanel;
    wxSplitterWindow* m_splitter;
    wxSearchCtrl*     m_filterSearch;
    wxTextCtrl*       m_filterText;
    wxBoxSizer*       m_buttonsSizer;
    wxBitmapButton*   m_addRuleButton;
    wxBitmapButton*   m_copyRuleButton;
    wxBitmapButton*   m_moveTreeItemUpButton;
    wxBitmapButton*   m_moveTreeItemDownButton;
    wxBitmapButton*   m_deleteRuleButton;
    wxButton*         m_saveRuleButton;
    wxButton*         m_cancelRuleButton;

private:
    bool m_isDragging;
    bool m_enableMoveUp;
    bool m_enableMoveDown;
    bool m_enableAddRule;
    bool m_enableDuplicateRule;
    bool m_enableDeleteRule;
    int  m_defaultSashPosition;

    wxString                    m_title;
    std::vector<RULE_TREE_NODE> m_defaultTreeItems;
    RULE_TREE_ITEM_DATA*        m_selectedData;
    wxTreeItemId                m_previousId;
    wxTreeItemId                m_draggedItem;
    wxDragImage*                m_dragImage;

    std::unordered_map<int, std::tuple<wxString, std::vector<int>, wxTreeItemId>> m_treeHistoryData;
};

#endif //RULE_EDITOR_DIALOG_BASE_H
