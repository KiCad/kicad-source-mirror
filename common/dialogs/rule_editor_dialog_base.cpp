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


#include <widgets/resettable_panel.h>
#include <widgets/wx_infobar.h>
#include <widgets/wx_panel.h>
#include <widgets/ui_common.h>

#include <wx/button.h>
#include <wx/grid.h>
#include <wx/sizer.h>
#include <wx/treectrl.h>
#include <wx/listctrl.h>
#include <wx/stc/stc.h>
#include <wx/menu.h>
#include <wx/log.h>
#include <wx/dragimag.h> // For wxDragImage
#include <wx/wx.h>
#include <wx/dnd.h>
#include <wx/dcmemory.h> // Include for wxMemoryDC
#include <wx/dcclient.h> // Include for wxClientDC if needed
#include <wx/dcbuffer.h> // Include for double-buffered drawing
#include <wx/bitmap.h>

#include <confirm.h>
#include <paths.h>
#include <bitmaps.h>
#include <launch_ext.h>
#include <algorithm>
#include <dialogs/rule_editor_dialog_base.h>


static wxSearchCtrl* CreateTextFilterBox( wxWindow* aParent, const wxString& aDescriptiveText )
{
    wxSearchCtrl* search_widget = new wxSearchCtrl( aParent, wxID_ANY );

    search_widget->ShowSearchButton( false );
    search_widget->ShowCancelButton( true );

    search_widget->SetDescriptiveText( aDescriptiveText );

#ifdef __WXGTK__
    // wxSearchCtrl vertical height is not calculated correctly on some GTK setups
    // See https://gitlab.com/kicad/code/kicad/-/issues/9019
    search_widget->SetMinSize( wxSize( -1, aParent->GetTextExtent( wxT( "qb" ) ).y + 10 ) );
#endif

    return search_widget;
}


RULE_EDITOR_DIALOG_BASE::RULE_EDITOR_DIALOG_BASE( wxWindow* aParent, const wxString& aTitle,
                                                  const wxSize& aInitialSize ) :
        DIALOG_SHIM( aParent, wxID_ANY, aTitle, wxDefaultPosition, aInitialSize,
                     wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER ),
        m_isDragging( false ),
        m_enableMoveUp( false ),
        m_enableMoveDown( false ),
        m_enableAddRule( false ),
        m_enableDuplicateRule( false ),
        m_enableDeleteRule( false ),
        m_defaultSashPosition( 200 ),
        m_title( aTitle ),
        m_selectedData( nullptr ),
        m_previousId( nullptr ),
        m_draggedItem( nullptr ),
        m_dragImage( nullptr )
{
    wxBoxSizer* mainSizer = new wxBoxSizer( wxVERTICAL );
    SetSizer( mainSizer );

    wxBoxSizer* infoBarSizer = new wxBoxSizer( wxHORIZONTAL );

    m_infoBar = new WX_INFOBAR( this );
    infoBarSizer->Add( m_infoBar, 0, wxEXPAND, 0 );

    // Add the tree panel to the content sizer
    m_splitter = new wxSplitterWindow( this, wxID_ANY, wxDefaultPosition, wxDefaultSize,
                                        wxSP_3D | wxSP_LIVE_UPDATE | wxSP_3DSASH );
    m_splitter->SetMinimumPaneSize( 50 ); // Set a minimum pane size

    // Create the tree control panel
    WX_PANEL* treeCtrlPanel =
            new WX_PANEL( m_splitter, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxBORDER_NONE | wxTAB_TRAVERSAL );
    treeCtrlPanel->SetBorders( true, true, true, true );
    wxBoxSizer* treeCtrlSizer = new wxBoxSizer( wxVERTICAL );
    treeCtrlPanel->SetSizer( treeCtrlSizer );

    // Add a search text box above the tree control
    m_filterSearch = CreateTextFilterBox( treeCtrlPanel, _( "Type filter text" ) );
    treeCtrlSizer->Add( m_filterSearch, 0, wxEXPAND | wxBOTTOM, 5 );

    // Create the tree control and set its font
    m_ruleTreeCtrl = new wxTreeCtrl( treeCtrlPanel, wxID_ANY, wxDefaultPosition, wxDefaultSize,
                                     wxTR_DEFAULT_STYLE | wxTR_HIDE_ROOT | wxTR_HAS_BUTTONS );
    m_ruleTreeCtrl->SetFont( KIUI::GetControlFont( this ) );

    // Adjust the tree control's window style to remove the border
    long treeCtrlFlags = m_ruleTreeCtrl->GetWindowStyleFlag();
    treeCtrlFlags = ( treeCtrlFlags & ~wxBORDER_MASK ) | wxBORDER_NONE;
    m_ruleTreeCtrl->SetWindowStyleFlag( treeCtrlFlags );

    // Add the tree control to its sizer
    treeCtrlSizer->Add( m_ruleTreeCtrl, 1, wxEXPAND | wxBOTTOM, 5 );

    // Create a sizer for the action buttons
    wxBoxSizer* actionButtonsSizer = new wxBoxSizer( wxHORIZONTAL );

    // Create the action buttons
    m_addRuleButton =
            new wxBitmapButton( treeCtrlPanel, wxID_ANY, KiBitmapBundle( BITMAPS::small_plus ),
                                wxDefaultPosition, wxDefaultSize, 0 );
    m_copyRuleButton = new wxBitmapButton( treeCtrlPanel, wxID_ANY, KiBitmapBundle( BITMAPS::copy ),
                                           wxDefaultPosition, wxDefaultSize, 0 );
    m_moveTreeItemUpButton =
            new wxBitmapButton( treeCtrlPanel, wxID_ANY, KiBitmapBundle( BITMAPS::small_up ),
                                wxDefaultPosition, wxDefaultSize, 0 );
    m_moveTreeItemDownButton =
            new wxBitmapButton( treeCtrlPanel, wxID_ANY, KiBitmapBundle( BITMAPS::small_down ),
                                wxDefaultPosition, wxDefaultSize, 0 );
    m_deleteRuleButton =
            new wxBitmapButton( treeCtrlPanel, wxID_ANY, KiBitmapBundle( BITMAPS::small_trash ),
                                wxDefaultPosition, wxDefaultSize, 0 );

    actionButtonsSizer->Add( m_addRuleButton, 0, wxLEFT | wxRIGHT, 5 );
    actionButtonsSizer->Add( m_copyRuleButton, 0, wxLEFT | wxRIGHT, 5 );
    actionButtonsSizer->Add( m_moveTreeItemUpButton, 0, wxLEFT | wxRIGHT, 5 );
    actionButtonsSizer->Add( m_moveTreeItemDownButton, 0, wxLEFT | wxRIGHT, 5 );

    // Add a spacer between the "Move" buttons and the "Delete" button
    actionButtonsSizer->AddStretchSpacer( 1 ); // Spacer between the Move and Delete buttons

    // Add the "Delete" button at the end
    actionButtonsSizer->Add( m_deleteRuleButton, 0, wxRIGHT, 5 );

    // Add the action buttons sizer to the tree control sizer
    treeCtrlSizer->Add( actionButtonsSizer, 0, wxBOTTOM | wxEXPAND, 5 );

    treeCtrlPanel->Layout();

    // Create the dynamic content panel
    m_contentPanel = new wxPanel( m_splitter, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxBORDER_NONE );
    m_contentPanel->SetBackgroundColour( *wxLIGHT_GREY );

    // Add the tree control panel to the splitter.
    // Calculate the minimum tree panel width based on the tree control's best size.
    treeCtrlPanel->Layout();
    int minTreeWidth = treeCtrlPanel->GetBestSize().GetWidth();

    // Ensure a reasonable minimum width for the tree panel
    if( minTreeWidth < 200 )
        minTreeWidth = 200;

    m_defaultSashPosition = minTreeWidth;

    m_splitter->SplitVertically( treeCtrlPanel, m_contentPanel, m_defaultSashPosition );

    // Set gravity to 0 so the tree panel maintains its size and the content panel resizes
    m_splitter->SetSashGravity( 0.0 );

    wxBoxSizer* m_sizerButtons = new wxBoxSizer( wxHORIZONTAL );

    wxStdDialogButtonSizer* m_sdbSizer = new wxStdDialogButtonSizer();
    m_cancelRuleButton = new wxButton( this, wxID_CANCEL );
    m_cancelRuleButton->SetLabelText( _( "Close" ) );
    m_sdbSizer->AddButton( m_cancelRuleButton );
    m_saveRuleButton = new wxButton( this, wxID_OK );
    m_saveRuleButton->SetLabelText( _( "Save" ) );
    m_sdbSizer->AddButton( m_saveRuleButton );
    m_sdbSizer->Realize();

    m_sizerButtons->Add( m_sdbSizer, 1, wxEXPAND, 5 );


    // Add the info bar sizer to the main sizer
    mainSizer->Add( infoBarSizer, 0, wxEXPAND, 0 );
    mainSizer->Add( m_splitter, 1, wxEXPAND | wxBOTTOM, 5 );
    mainSizer->Add( m_sizerButtons, 0, wxALL | wxEXPAND, 5 );
    SetSize( wxSize( 980, 680 ) );
    Layout();

    // Bind the context menu event
    m_filterSearch->Bind( wxEVT_COMMAND_TEXT_UPDATED, &RULE_EDITOR_DIALOG_BASE::onFilterSearch,
                          this );
    m_ruleTreeCtrl->Bind( wxEVT_TREE_ITEM_RIGHT_CLICK,
                          &RULE_EDITOR_DIALOG_BASE::onRuleTreeItemRightClick, this );
    m_ruleTreeCtrl->Bind( wxEVT_TREE_SEL_CHANGED,
                          &RULE_EDITOR_DIALOG_BASE::onRuleTreeItemSelectionChanged, this );
    m_ruleTreeCtrl->Bind( wxEVT_TREE_ITEM_ACTIVATED,
                          &RULE_EDITOR_DIALOG_BASE::onRuleTreeItemActivated, this );
    m_ruleTreeCtrl->Bind( wxEVT_LEFT_DOWN, &RULE_EDITOR_DIALOG_BASE::onRuleTreeItemLeftDown, this );
    m_ruleTreeCtrl->Bind( wxEVT_MOTION, &RULE_EDITOR_DIALOG_BASE::onRuleTreeItemMouseMotion, this );
    m_ruleTreeCtrl->Bind( wxEVT_LEFT_UP, &RULE_EDITOR_DIALOG_BASE::onRuleTreeItemLeftUp, this );

    m_addRuleButton->Bind( wxEVT_BUTTON, &RULE_EDITOR_DIALOG_BASE::onNewRuleOptionClick, this );
    m_copyRuleButton->Bind( wxEVT_BUTTON, &RULE_EDITOR_DIALOG_BASE::onDuplicateRuleOptionClick,
                            this );
    m_moveTreeItemUpButton->Bind( wxEVT_BUTTON, &RULE_EDITOR_DIALOG_BASE::onMoveUpRuleOptionClick,
                                  this );
    m_moveTreeItemDownButton->Bind( wxEVT_BUTTON,
                                    &RULE_EDITOR_DIALOG_BASE::onMoveDownRuleOptionClick, this );
    m_deleteRuleButton->Bind( wxEVT_BUTTON, &RULE_EDITOR_DIALOG_BASE::onDeleteRuleOptionClick,
                              this );
    m_saveRuleButton->Bind( wxEVT_BUTTON, &RULE_EDITOR_DIALOG_BASE::OnSave, this );
    m_cancelRuleButton->Bind( wxEVT_BUTTON, &RULE_EDITOR_DIALOG_BASE::OnCancel, this );

    this->Bind( wxEVT_SIZE, &RULE_EDITOR_DIALOG_BASE::onResize, this );

    this->Bind( wxEVT_CLOSE_WINDOW, &RULE_EDITOR_DIALOG_BASE::onClose, this );
}


void RULE_EDITOR_DIALOG_BASE::finishInitialization()
{
    wxSize curSz = GetSize();
    wxSize minSz = GetMinSize();

    finishDialogSettings();

    curSz = GetSize();
    minSz = GetMinSize();
}


RULE_EDITOR_DIALOG_BASE::~RULE_EDITOR_DIALOG_BASE()
{
    m_filterSearch->Unbind( wxEVT_COMMAND_TEXT_UPDATED, &RULE_EDITOR_DIALOG_BASE::onFilterSearch, this );
    m_ruleTreeCtrl->Unbind( wxEVT_TREE_ITEM_RIGHT_CLICK, &RULE_EDITOR_DIALOG_BASE::onRuleTreeItemRightClick, this );
    m_ruleTreeCtrl->Unbind( wxEVT_TREE_SEL_CHANGED, &RULE_EDITOR_DIALOG_BASE::onRuleTreeItemSelectionChanged, this );
    m_ruleTreeCtrl->Unbind( wxEVT_TREE_ITEM_ACTIVATED, &RULE_EDITOR_DIALOG_BASE::onRuleTreeItemActivated, this );
    m_ruleTreeCtrl->Unbind( wxEVT_LEFT_DOWN, &RULE_EDITOR_DIALOG_BASE::onRuleTreeItemLeftDown, this );
    m_ruleTreeCtrl->Unbind( wxEVT_MOTION, &RULE_EDITOR_DIALOG_BASE::onRuleTreeItemMouseMotion, this );
    m_ruleTreeCtrl->Unbind( wxEVT_LEFT_UP, &RULE_EDITOR_DIALOG_BASE::onRuleTreeItemLeftUp, this );

    m_addRuleButton->Unbind( wxEVT_BUTTON, &RULE_EDITOR_DIALOG_BASE::onNewRuleOptionClick, this );
    m_copyRuleButton->Unbind( wxEVT_BUTTON, &RULE_EDITOR_DIALOG_BASE::onDuplicateRuleOptionClick, this );
    m_moveTreeItemUpButton->Unbind( wxEVT_BUTTON, &RULE_EDITOR_DIALOG_BASE::onMoveUpRuleOptionClick, this );
    m_moveTreeItemDownButton->Unbind( wxEVT_BUTTON, &RULE_EDITOR_DIALOG_BASE::onMoveDownRuleOptionClick, this );
    m_deleteRuleButton->Unbind( wxEVT_BUTTON, &RULE_EDITOR_DIALOG_BASE::onDeleteRuleOptionClick, this );
    m_saveRuleButton->Unbind( wxEVT_BUTTON, &RULE_EDITOR_DIALOG_BASE::OnSave, this );
    m_cancelRuleButton->Unbind( wxEVT_BUTTON, &RULE_EDITOR_DIALOG_BASE::OnCancel, this );

    this->Unbind( wxEVT_SIZE, &RULE_EDITOR_DIALOG_BASE::onResize, this );
    this->Unbind( wxEVT_CLOSE_WINDOW, &RULE_EDITOR_DIALOG_BASE::onClose, this );

    m_selectedData = nullptr;
    m_previousId = nullptr;

    if( m_dragImage )
    {
        delete m_dragImage;
        m_dragImage = nullptr;
    }
}


bool RULE_EDITOR_DIALOG_BASE::TransferDataToWindow()
{
    finishInitialization();

    // Call TransferDataToWindow() only once:
    // this is enough on wxWidgets 3.1
    if( !DIALOG_SHIM::TransferDataToWindow() )
        return false;

    return true;
}


bool RULE_EDITOR_DIALOG_BASE::TransferDataFromWindow()
{
    bool ret = true;

    // Call TransferDataFromWindow() only once:
    // this is enough on wxWidgets 3.1
    if( !DIALOG_SHIM::TransferDataFromWindow() )
        ret = false;

    return ret;
}


RULE_EDITOR_DIALOG_BASE* RULE_EDITOR_DIALOG_BASE::GetDialog( wxWindow* aParent )
{
    while( aParent )
    {
        if( RULE_EDITOR_DIALOG_BASE* currentDialog =
                    dynamic_cast<RULE_EDITOR_DIALOG_BASE*>( aParent ) )
            return currentDialog;

        aParent = aParent->GetParent();
    }

    return nullptr;
}


void RULE_EDITOR_DIALOG_BASE::InitRuleTreeItems( const std::vector<RULE_TREE_NODE>& aNodes )
{
    if( aNodes.empty() )
        return;

    m_defaultTreeItems = aNodes;

    wxTreeItemId rootId = m_ruleTreeCtrl->AddRoot( aNodes[0].m_nodeName );

    RULE_TREE_ITEM_DATA* itemData =
            new RULE_TREE_ITEM_DATA( aNodes[0].m_nodeId, nullptr, rootId );
    m_ruleTreeCtrl->SetItemData( rootId, itemData );

    std::vector<RULE_TREE_NODE> childNodes;
    getRuleTreeChildNodes( aNodes, aNodes[0].m_nodeId, childNodes );

    for( const auto& child : childNodes )
    {
        populateRuleTreeCtrl( aNodes, child, rootId );
    }

    m_treeHistoryData.clear();
    saveRuleTreeState( m_ruleTreeCtrl->GetRootItem(), m_defaultTreeItems[0].m_nodeId );

    m_ruleTreeCtrl->SelectItem( m_ruleTreeCtrl->GetFirstVisibleItem() );
    m_ruleTreeCtrl->CollapseAll();
}


void RULE_EDITOR_DIALOG_BASE::SetContentPanel( wxPanel* aContentPanel )
{
    int sash_position = m_defaultSashPosition;

    if( m_contentPanel )
    {
        sash_position = m_splitter->GetSashPosition();
        m_splitter->Unsplit( m_contentPanel );
        m_contentPanel->Destroy();
    }

    m_contentPanel = aContentPanel;
    auto treeCtrlPanel = m_splitter->GetWindow1();
    m_splitter->SplitVertically( treeCtrlPanel, m_contentPanel, sash_position );

    Layout();
    Refresh();
}


void RULE_EDITOR_DIALOG_BASE::AppendNewRuleTreeItem( const RULE_TREE_NODE& aNode,
                                                     wxTreeItemId aParent )
{
    wxTreeItemId currentTreeItemId = appendRuleTreeItem( aNode, aParent );

    auto it = m_treeHistoryData.find( aNode.m_nodeData->GetParentId() );

    if( it != m_treeHistoryData.end() )
    {
        std::vector<int>& existingChildren = std::get<1>( it->second );
        existingChildren.push_back( aNode.m_nodeId );

        m_treeHistoryData[aNode.m_nodeId] = { aNode.m_nodeName,
                                                      {},
                                                      currentTreeItemId };
    }

    m_ruleTreeCtrl->SelectItem( currentTreeItemId );
}


void RULE_EDITOR_DIALOG_BASE::UpdateRuleTreeItemText( wxTreeItemId aItemId, wxString aItemText )
{
    m_ruleTreeCtrl->SetItemText( aItemId, aItemText );
}


void RULE_EDITOR_DIALOG_BASE::SetControlsEnabled( bool aEnable )
{
    m_ruleTreeCtrl->Enable( aEnable );
    m_filterSearch->Enable( aEnable );

    updateRuleTreeActionButtonsState( m_selectedData );

    if( m_cancelRuleButton )
        m_cancelRuleButton->SetLabelText( aEnable ? _( "Close" ) : _( "Cancel" ) );
}


void RULE_EDITOR_DIALOG_BASE::DeleteRuleTreeItem( wxTreeItemId aItemId, const int& aNodeId )
{
    m_ruleTreeCtrl->Delete( aItemId );
    m_treeHistoryData.erase( aNodeId );
}


void RULE_EDITOR_DIALOG_BASE::populateRuleTreeCtrl(
        const std::vector<RULE_TREE_NODE>& aNodes, const RULE_TREE_NODE& aNode,
        wxTreeItemId aParent )
{
    wxTreeItemId currentId = appendRuleTreeItem( aNode, aParent );

    std::vector<RULE_TREE_NODE> childNodes;
    getRuleTreeChildNodes( aNodes, aNode.m_nodeId, childNodes );

    for( const auto& child : childNodes )
    {
        populateRuleTreeCtrl( aNodes, child, currentId );
    }

    m_ruleTreeCtrl->Expand( currentId );
}


void RULE_EDITOR_DIALOG_BASE::onRuleTreeItemRightClick( wxTreeEvent& aEvent )
{
    wxTreeItemId item = aEvent.GetItem();

    if( !item.IsOk() )
        return;

    m_ruleTreeCtrl->SelectItem( item );

    RULE_TREE_ITEM_DATA* ruleTreeItemData =
            dynamic_cast<RULE_TREE_ITEM_DATA*>( m_ruleTreeCtrl->GetItemData( item ) );

    if( !ruleTreeItemData )
        return;

    updateRuleTreeActionButtonsState( ruleTreeItemData );

    wxMenu menu;

    if( m_enableAddRule )
        KIUI::AddMenuItem( &menu, ID_NEWRULE, _( "New Rule" ), KiBitmap( BITMAPS::small_plus ) );

    if( m_enableAddRule )
        KIUI::AddMenuItem( &menu, ID_COPYRULE, _( "Copy Rule" ), KiBitmap( BITMAPS::copy ) );

    if( m_enableDeleteRule )
        KIUI::AddMenuItem( &menu, ID_DELETERULE, _( "Delete Rule" ),
                           KiBitmap( BITMAPS::small_trash ) );

    if( isEnabled( ruleTreeItemData,
                                                 RULE_EDITOR_TREE_CONTEXT_OPT::MOVE_UP )
        && m_enableMoveUp )
        KIUI::AddMenuItem( &menu, ID_MOVEUP, _( "Move Up" ), KiBitmap( BITMAPS::small_up ) );

    if( isEnabled( ruleTreeItemData,
                                                 RULE_EDITOR_TREE_CONTEXT_OPT::MOVE_DOWN )
        && m_enableMoveDown )
        KIUI::AddMenuItem( &menu, ID_MOVEDOWN, _( "Move Down" ), KiBitmap( BITMAPS::small_down ) );

    menu.Bind( wxEVT_COMMAND_MENU_SELECTED,
               [&]( wxCommandEvent& aCmd )
               {
                   switch( aCmd.GetId() )
                   {
                   case ID_NEWRULE: onNewRuleOptionClick( aCmd ); break;

                   case ID_COPYRULE: onDuplicateRuleOptionClick( aCmd ); break;

                   case ID_DELETERULE: onDeleteRuleOptionClick( aCmd ); break;

                   case ID_MOVEUP: onMoveUpRuleOptionClick( aCmd ); break;

                   case ID_MOVEDOWN: onMoveDownRuleOptionClick( aCmd ); break;
                   default: aCmd.Skip();
                   }
               } );

    PopupMenu( &menu );
}


void RULE_EDITOR_DIALOG_BASE::onRuleTreeItemSelectionChanged( wxTreeEvent& aEvent )
{
    wxTreeItemId selectedItem = aEvent.GetItem();

    RULE_TREE_ITEM_DATA* selectedItemData =
            dynamic_cast<RULE_TREE_ITEM_DATA*>( m_ruleTreeCtrl->GetItemData( selectedItem ) );

    if( !selectedItemData )
        return;

    if( m_selectedData )
    {
        if( m_selectedData->GetNodeId() == selectedItemData->GetNodeId() )
        {
            m_selectedData = selectedItemData;
            updateRuleTreeActionButtonsState( selectedItemData );
            return;
        }

        m_previousId = m_selectedData->GetTreeItemId();
    }

    m_selectedData = selectedItemData;

    RuleTreeItemSelectionChanged( m_selectedData );

    updateRuleTreeActionButtonsState( selectedItemData );
}

void RULE_EDITOR_DIALOG_BASE::onRuleTreeItemActivated( wxTreeEvent& aEvent )
{
    RULE_TREE_ITEM_DATA* itemData =
            dynamic_cast<RULE_TREE_ITEM_DATA*>( m_ruleTreeCtrl->GetItemData( aEvent.GetItem() ) );

    if( itemData &&
        isEnabled( itemData, RULE_EDITOR_TREE_CONTEXT_OPT::ADD_RULE ) )
    {
        AddNewRule( itemData );
    }
}


void RULE_EDITOR_DIALOG_BASE::onNewRuleOptionClick( wxCommandEvent& aEvent )
{
    AddNewRule( m_selectedData );
}


void RULE_EDITOR_DIALOG_BASE::onDuplicateRuleOptionClick( wxCommandEvent& aEvent )
{
    DuplicateRule( m_selectedData );
}


void RULE_EDITOR_DIALOG_BASE::onDeleteRuleOptionClick( wxCommandEvent& aEvent )
{
    RemoveRule( m_selectedData->GetNodeId() );
}


void RULE_EDITOR_DIALOG_BASE::onMoveUpRuleOptionClick( wxCommandEvent& aEvent )
{
    if( !m_enableMoveUp )
        return;

    wxTreeItemId selectedItem = m_ruleTreeCtrl->GetSelection();
    if( !selectedItem.IsOk() )
        return;

    wxTreeItemId parent = m_ruleTreeCtrl->GetItemParent( selectedItem );

    if( !parent.IsOk() )
        return;

    wxTreeItemIdValue cookie;
    wxTreeItemId      current = m_ruleTreeCtrl->GetFirstChild( parent, cookie );
    wxTreeItemId      previous;
    wxTreeItemId      beforePrevious;

    while( current.IsOk() && current != selectedItem )
    {
        beforePrevious = previous;
        previous = current;
        current = m_ruleTreeCtrl->GetNextChild( parent, cookie );
    }

    if( !previous.IsOk() )
    {
        return;
    }

    wxTreeItemId insertPosition = beforePrevious.IsOk() ? beforePrevious : wxTreeItemId();

    wxTreeItemId newItem = m_ruleTreeCtrl->InsertItem(
            parent, insertPosition, m_ruleTreeCtrl->GetItemText( selectedItem ) );

    moveRuleTreeItemChildrensTooOnDrag( selectedItem, newItem );

    wxTreeItemData* itemData = m_ruleTreeCtrl->GetItemData( selectedItem );

    if( itemData )
    {
        RULE_TREE_ITEM_DATA* ruleTreeItemData = dynamic_cast<RULE_TREE_ITEM_DATA*>( itemData );
        ruleTreeItemData->SetTreeItemId( newItem );

        m_ruleTreeCtrl->SetItemData( newItem, itemData );
        m_ruleTreeCtrl->SetItemData( selectedItem, nullptr ); // Detach data from the old item

        saveRuleTreeState( newItem, ruleTreeItemData->GetNodeId() );

        m_ruleTreeCtrl->SelectItem( newItem );

        m_ruleTreeCtrl->Expand( newItem );
        m_ruleTreeCtrl->ExpandAllChildren( newItem );
    }

    m_ruleTreeCtrl->DeleteChildren( selectedItem );
    m_ruleTreeCtrl->Delete( selectedItem );
}


void RULE_EDITOR_DIALOG_BASE::onMoveDownRuleOptionClick( wxCommandEvent& aEvent )
{
    if( !m_enableMoveDown )
        return;

    wxTreeItemId selectedItem = m_ruleTreeCtrl->GetSelection();

    if( !selectedItem.IsOk() )
        return;

    wxTreeItemId      parent = m_ruleTreeCtrl->GetItemParent( selectedItem );
    wxTreeItemIdValue cookie;
    wxTreeItemId      current = m_ruleTreeCtrl->GetFirstChild( parent, cookie );

    while( current.IsOk() && current != selectedItem )
    {
        current = m_ruleTreeCtrl->GetNextChild( parent, cookie );
    }

    wxTreeItemId next = m_ruleTreeCtrl->GetNextChild( parent, cookie );

    if( !next.IsOk() )
    {
        return;
    }

    wxString        itemText = m_ruleTreeCtrl->GetItemText( selectedItem );
    wxTreeItemData* itemData = m_ruleTreeCtrl->GetItemData( selectedItem );

    wxTreeItemId newItem = m_ruleTreeCtrl->InsertItem( parent, next, itemText );

    moveRuleTreeItemChildrensTooOnDrag( selectedItem, newItem );

    if( itemData )
    {
        RULE_TREE_ITEM_DATA* ruleTreeItemData = dynamic_cast<RULE_TREE_ITEM_DATA*>( itemData );
        ruleTreeItemData->SetTreeItemId( newItem );

        m_ruleTreeCtrl->SetItemData( newItem, itemData );
        m_ruleTreeCtrl->SetItemData( selectedItem, nullptr ); // Detach data

        saveRuleTreeState( newItem, ruleTreeItemData->GetNodeId() );

        m_ruleTreeCtrl->SelectItem( newItem );

        m_ruleTreeCtrl->Expand( newItem );
    }

    m_ruleTreeCtrl->DeleteChildren( selectedItem );
    m_ruleTreeCtrl->Delete( selectedItem );
}


void RULE_EDITOR_DIALOG_BASE::onRuleTreeItemLeftDown( wxMouseEvent& aEvent )
{
    wxPoint      pt = aEvent.GetPosition();
    int          flags;
    wxTreeItemId item = m_ruleTreeCtrl->HitTest( pt, flags );

    // Check if click is on the expand/collapse button
    if( flags & wxTREE_HITTEST_ONITEMBUTTON )
    {
        // Prevent wxEVT_LEFT_DOWN from propagating further
        aEvent.Skip();
        return;
    }

    if( item.IsOk() )
    {
        m_draggedItem = item;

        wxString   text = m_ruleTreeCtrl->GetItemText( item );
        wxMemoryDC dc;
        wxBitmap   bitmap( 200, 30 );
        dc.SelectObject( bitmap );
        dc.SetBackground( *wxWHITE_BRUSH );
        dc.Clear();
        dc.SetFont( *wxNORMAL_FONT );
        dc.DrawText( text, 5, 5 );
        dc.SelectObject( wxNullBitmap );

        m_dragImage = new wxDragImage( bitmap );
        m_isDragging = false;
    }

    aEvent.Skip();
}


void RULE_EDITOR_DIALOG_BASE::onRuleTreeItemMouseMotion( wxMouseEvent& aEvent )
{
    if( aEvent.Dragging() && m_draggedItem.IsOk() && m_dragImage )
    {
        wxPoint currentPos = aEvent.GetPosition();

        if( !m_isDragging )
        {
            if( !m_dragImage->BeginDrag( wxPoint( 0, 0 ), m_ruleTreeCtrl, true ) )
            {
                delete m_dragImage;
                m_dragImage = nullptr;
                return;
            }

            m_dragImage->Show();
            m_isDragging = true;
        }
        else
        {
            m_dragImage->Move( currentPos );
        }
    }

    aEvent.Skip();
}


void RULE_EDITOR_DIALOG_BASE::onRuleTreeItemLeftUp( wxMouseEvent& aEvent )
{
    if( m_draggedItem.IsOk() && m_isDragging )
    {
        if( m_dragImage )
        {
            m_dragImage->Hide();
            m_dragImage->EndDrag();
            delete m_dragImage;
            m_dragImage = nullptr;
            m_isDragging = false;
        }

        wxPoint      pos = aEvent.GetPosition();
        int          flags;
        wxTreeItemId targetItem = m_ruleTreeCtrl->HitTest( pos, flags );

        if( !targetItem.IsOk() )
            return;

        wxTreeItemId draggedParent = m_ruleTreeCtrl->GetItemParent( m_draggedItem );
        wxTreeItemId targetParent = m_ruleTreeCtrl->GetItemParent( targetItem );

        if( draggedParent != targetParent )
            return;

        if( draggedParent == targetParent )
        {
            wxTreeItemId      newItem;
            wxTreeItemIdValue cookie;

            if( flags & wxTREE_HITTEST_ONITEMLABEL
                && targetItem == m_ruleTreeCtrl->GetFirstChild( targetParent, cookie ) )
            {
                // Prepend item as the first child of target parent
                newItem = m_ruleTreeCtrl->PrependItem(
                        targetParent, m_ruleTreeCtrl->GetItemText( m_draggedItem ) );
            }
            else
            {
                wxTreeItemId current = m_ruleTreeCtrl->GetFirstChild( targetParent, cookie );
                wxTreeItemId previous;

                // Traverse through siblings to find the position of the target item
                while( current.IsOk() && current != targetItem )
                {
                    previous = current;
                    current = m_ruleTreeCtrl->GetNextChild( targetParent, cookie );
                }

                if( !previous.IsOk() )
                {
                    // If no previous item, insert as the first child
                    newItem = m_ruleTreeCtrl->PrependItem(
                            targetParent, m_ruleTreeCtrl->GetItemText( m_draggedItem ) );
                }
                else
                {
                    wxTreeItemId nextSibling = m_ruleTreeCtrl->GetNextChild( targetParent, cookie );

                    if( nextSibling.IsOk() )
                    {
                        // If there is a next sibling, insert after the previous item
                        newItem = m_ruleTreeCtrl->InsertItem(
                                targetParent, previous,
                                m_ruleTreeCtrl->GetItemText( m_draggedItem ) );
                    }
                    else
                    {
                        // If no next sibling, append the item as the last child
                        newItem = m_ruleTreeCtrl->AppendItem(
                                targetParent, m_ruleTreeCtrl->GetItemText( m_draggedItem ) );
                    }
                }
            }

            moveRuleTreeItemChildrensTooOnDrag( m_draggedItem, newItem );

            wxTreeItemData* itemData = m_ruleTreeCtrl->GetItemData( m_draggedItem );

            if( itemData )
            {
                RULE_TREE_ITEM_DATA* ruleTreeItemData =
                        dynamic_cast<RULE_TREE_ITEM_DATA*>( itemData );
                ruleTreeItemData->SetTreeItemId( newItem );

                m_ruleTreeCtrl->SetItemData( newItem, ruleTreeItemData );
                m_ruleTreeCtrl->SetItemData( m_draggedItem, nullptr );

                saveRuleTreeState( newItem, ruleTreeItemData->GetNodeId() );

                m_ruleTreeCtrl->SelectItem( newItem );

                m_ruleTreeCtrl->Expand( newItem );
            }

            m_ruleTreeCtrl->DeleteChildren( m_draggedItem );
            m_ruleTreeCtrl->Delete( m_draggedItem );
        }

        m_draggedItem = nullptr;
    }

    aEvent.Skip();
}


void RULE_EDITOR_DIALOG_BASE::onFilterSearch( wxCommandEvent& aEvent )
{
    const auto searchStr = aEvent.GetString().Lower();

    m_ruleTreeCtrl->DeleteAllItems();

    restoreRuleTree( nullptr, m_defaultTreeItems[0].m_nodeId );

    wxTreeItemId root = m_ruleTreeCtrl->GetRootItem();

    if( root.IsOk() )
    {
        filterRuleTree( root, searchStr );
    }
}


bool RULE_EDITOR_DIALOG_BASE::filterRuleTree( const wxTreeItemId& aItem, const wxString& aFilter )
{
    bool matches = m_ruleTreeCtrl->GetItemText( aItem ).Lower().Contains( aFilter );

    wxTreeItemIdValue cookie;
    wxTreeItemId      child = m_ruleTreeCtrl->GetFirstChild( aItem, cookie );
    bool              hasVisibleChildren = false;

    std::vector<wxTreeItemId> itemsToDelete;

    while( child.IsOk() )
    {
        if( filterRuleTree( child, aFilter ) )
        {
            hasVisibleChildren = true;
        }
        else
        {
            itemsToDelete.push_back( child );
        }

        child = m_ruleTreeCtrl->GetNextChild( aItem, cookie );
    }

    for( const auto& id : itemsToDelete )
    {
        m_ruleTreeCtrl->Delete( id );
    }

    return matches || hasVisibleChildren;
}


void RULE_EDITOR_DIALOG_BASE::saveRuleTreeState( const wxTreeItemId& aItem, const int& aNodeId )
{
    wxString         itemText = m_ruleTreeCtrl->GetItemText( aItem );
    std::vector<int> children;

    wxTreeItemIdValue cookie;
    wxTreeItemId      child = m_ruleTreeCtrl->GetFirstChild( aItem, cookie );

    while( child.IsOk() )
    {
        RULE_TREE_ITEM_DATA* childItemData =
                dynamic_cast<RULE_TREE_ITEM_DATA*>( m_ruleTreeCtrl->GetItemData( child ) );
        int childId = childItemData->GetNodeId();

        children.push_back( childId );

        saveRuleTreeState( child, childId );

        child = m_ruleTreeCtrl->GetNextChild( aItem, cookie );
    }

    m_treeHistoryData[aNodeId] = { itemText, children, aItem };
}


void RULE_EDITOR_DIALOG_BASE::restoreRuleTree( const wxTreeItemId& aParent, const int& aNodeId )
{
    auto it = m_treeHistoryData.find( aNodeId );

    if( it == m_treeHistoryData.end() )
        return;

    const auto& [itemText, children, itemId] = it->second;

    wxTreeItemId newItem;

    if( aParent )
        newItem = m_ruleTreeCtrl->AppendItem( aParent, itemText );
    else
        newItem = m_ruleTreeCtrl->AddRoot( itemText );

    if( newItem )
    {
        wxTreeItemId& treeItemId = std::get<2>( it->second );
        treeItemId = newItem;
    }

    RULE_TREE_ITEM_DATA* itemData = new RULE_TREE_ITEM_DATA( aNodeId, aParent, newItem );
    m_ruleTreeCtrl->SetItemData( newItem, itemData );

    for( const auto& childId : children )
    {
        restoreRuleTree( newItem, childId );
    }

    // Don't expand the root item because it is hidden
    if( aParent )
        m_ruleTreeCtrl->Expand( newItem );
}


wxTreeItemId RULE_EDITOR_DIALOG_BASE::appendRuleTreeItem( const RULE_TREE_NODE& aNode, wxTreeItemId aParent )
{
    wxTreeItemId currentTreeItemId = m_ruleTreeCtrl->AppendItem( aParent, aNode.m_nodeName );

    RULE_TREE_ITEM_DATA* itemData = new RULE_TREE_ITEM_DATA( aNode.m_nodeId, aParent, currentTreeItemId );
    m_ruleTreeCtrl->SetItemData( currentTreeItemId, itemData );

    m_ruleTreeCtrl->Expand( currentTreeItemId );

    return currentTreeItemId;
}


void RULE_EDITOR_DIALOG_BASE::getRuleTreeChildNodes( const std::vector<RULE_TREE_NODE>& aNodes,
                                                     int aParentId,
                                                     std::vector<RULE_TREE_NODE>& aResult )
{
    std::vector<RULE_TREE_NODE> filteredNodes;

    std::copy_if( aNodes.begin(), aNodes.end(), std::back_inserter( filteredNodes ),
                  [&aParentId]( const RULE_TREE_NODE& node )
                  {
                      return node.m_nodeData->GetParentId() == aParentId;
                  } );

    if( filteredNodes.size() > 0 )
    {
        aResult.insert( aResult.end(), filteredNodes.begin(), filteredNodes.end() );
    }
}

void RULE_EDITOR_DIALOG_BASE::moveRuleTreeItemChildrensTooOnDrag( wxTreeItemId aSrc,
                                                                  wxTreeItemId aDest )
{
    wxTreeItemIdValue cookie;
    wxTreeItemId      child = m_ruleTreeCtrl->GetFirstChild( aSrc, cookie );

    while( child.IsOk() )
    {
        wxTreeItemId newChild =
                m_ruleTreeCtrl->AppendItem( aDest, m_ruleTreeCtrl->GetItemText( child ) );

        RULE_TREE_ITEM_DATA* ruleTreeItemData =
                dynamic_cast<RULE_TREE_ITEM_DATA*>( m_ruleTreeCtrl->GetItemData( child ) );
        ruleTreeItemData->SetParentTreeItemId( aDest );
        ruleTreeItemData->SetTreeItemId( newChild );

        m_ruleTreeCtrl->SetItemData( newChild, ruleTreeItemData );
        m_ruleTreeCtrl->SetItemData( child, nullptr );

        moveRuleTreeItemChildrensTooOnDrag( child, newChild );

        child = m_ruleTreeCtrl->GetNextChild( aSrc, cookie );
    }
}


void RULE_EDITOR_DIALOG_BASE::updateRuleTreeItemMoveOptionState()
{
    wxTreeItemId selectedItem = m_ruleTreeCtrl->GetSelection();

    m_enableMoveUp = false;
    m_enableMoveDown = false;

    if( !selectedItem.IsOk() )
        return;

    if( selectedItem == m_ruleTreeCtrl->GetRootItem() )
        return;

    RULE_TREE_ITEM_DATA* itemData =
            dynamic_cast<RULE_TREE_ITEM_DATA*>( m_ruleTreeCtrl->GetItemData( selectedItem ) );

    if( !itemData ||
        !isEnabled( itemData, RULE_EDITOR_TREE_CONTEXT_OPT::MOVE_UP ) ||
        !isEnabled( itemData, RULE_EDITOR_TREE_CONTEXT_OPT::MOVE_DOWN ) )
    {
        return;
    }

    wxTreeItemId      parent = m_ruleTreeCtrl->GetItemParent( selectedItem );
    wxTreeItemIdValue cookie;
    wxTreeItemId      firstChild = m_ruleTreeCtrl->GetFirstChild( parent, cookie );
    wxTreeItemId      lastChild = firstChild;

    while( lastChild.IsOk() )
    {
        wxTreeItemId next = m_ruleTreeCtrl->GetNextChild( parent, cookie );

        if( !next.IsOk() )
            break;

        lastChild = next;
    }

    m_enableMoveUp = ( selectedItem != firstChild );
    m_enableMoveDown = ( selectedItem != lastChild );
}


void RULE_EDITOR_DIALOG_BASE::updateRuleTreeActionButtonsState( RULE_TREE_ITEM_DATA* aData )
{
    wxTreeItemId selectedItem = m_ruleTreeCtrl->GetSelection();

    if( !selectedItem.IsOk() || !m_ruleTreeCtrl->IsEnabled() )
    {
        m_addRuleButton->Enable( false );
        m_copyRuleButton->Enable( false );
        m_moveTreeItemUpButton->Enable( false );
        m_moveTreeItemDownButton->Enable( false );
        m_deleteRuleButton->Enable( false );

        return;
    }

    m_enableAddRule = false;
    m_enableDuplicateRule = false;
    m_enableDeleteRule = false;

    if( isEnabled( aData, RULE_EDITOR_TREE_CONTEXT_OPT::ADD_RULE ) )
        m_enableAddRule = true;

    if( isEnabled( aData, RULE_EDITOR_TREE_CONTEXT_OPT::DUPLICATE_RULE ) )
        m_enableDuplicateRule = true;

    if( isEnabled( aData, RULE_EDITOR_TREE_CONTEXT_OPT::DELETE_RULE ) )
        m_enableDeleteRule = true;

    updateRuleTreeItemMoveOptionState();

    m_addRuleButton->Enable( m_enableAddRule );
    m_copyRuleButton->Enable( m_enableDuplicateRule );
    m_moveTreeItemUpButton->Enable( m_enableMoveUp );
    m_moveTreeItemDownButton->Enable( m_enableMoveDown );
    m_deleteRuleButton->Enable( m_enableDeleteRule );
}


void RULE_EDITOR_DIALOG_BASE::onResize( wxSizeEvent& event )
{
    Layout();

    event.Skip();
}


void RULE_EDITOR_DIALOG_BASE::onClose( wxCloseEvent& aEvt )
{
    wxPostEvent( this, wxCommandEvent( wxEVT_COMMAND_BUTTON_CLICKED, wxID_CANCEL ) );
}
