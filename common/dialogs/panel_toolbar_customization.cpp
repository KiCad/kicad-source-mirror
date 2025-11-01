/*
 * This program source code file is part of KICAD, a free EDA CAD application.
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

#include <dialogs/panel_toolbar_customization.h>

#include <bitmaps.h>
#include <settings/app_settings.h>
#include <tool/ui/toolbar_configuration.h>
#include <widgets/split_button.h>
#include <widgets/std_bitmap_button.h>

#include <magic_enum.hpp>
#include <wx/listctrl.h>
#include <wx/menu.h>

// Simple IDs for the split button menu
enum
{
    ID_SEPARATOR_MENU = ( wxID_HIGHEST + 5 ),
    ID_SPACER_MENU,
    ID_GROUP_MENU
};


static std::map<TOOLBAR_LOC, wxString> s_toolbarNameMap = {
    { TOOLBAR_LOC::LEFT,     _( "Left" ) },
    { TOOLBAR_LOC::RIGHT,    _( "Right" ) },
    { TOOLBAR_LOC::TOP_MAIN, _( "Top main" ) },
    { TOOLBAR_LOC::TOP_AUX,  _( "Top auxiliary" ) }
};


class TOOLBAR_TREE_ITEM_DATA : public wxTreeItemData
{
public:
    TOOLBAR_TREE_ITEM_DATA() :
        m_type( TOOLBAR_ITEM_TYPE::SEPARATOR ),     // Init m_type to something
        m_action( nullptr ),
        m_size( 0 )
    { }

    TOOLBAR_TREE_ITEM_DATA( TOOLBAR_ITEM_TYPE aType ) :
        m_type( aType ),
        m_action( nullptr ),
        m_size( 0 )
    { }

    TOOLBAR_TREE_ITEM_DATA( TOOLBAR_ITEM_TYPE aType, int aSize ) :
        m_type( aType ),
        m_action( nullptr ),
        m_size( aSize )
    {
        wxASSERT( aType == TOOLBAR_ITEM_TYPE::SPACER );
    }

    TOOLBAR_TREE_ITEM_DATA( TOOLBAR_ITEM_TYPE aType, wxString aName ) :
        m_type( aType ),
        m_action( nullptr ),
        m_size( 0 ),
        m_name( aName )
    {
        wxASSERT( aType == TOOLBAR_ITEM_TYPE::CONTROL
                  || aType == TOOLBAR_ITEM_TYPE::TB_GROUP );
    }

    TOOLBAR_TREE_ITEM_DATA( TOOLBAR_ITEM_TYPE aType, TOOL_ACTION* aAction ) :
        m_type( aType ),
        m_action( aAction ),
        m_size( 0 )
    {
        wxASSERT( aType == TOOLBAR_ITEM_TYPE::TOOL );
    }

    void SetAction( TOOL_ACTION* aAction ) { m_action = aAction; }
    TOOL_ACTION* GetAction() const         { return m_action; }

    void SetName( const wxString& aName ) { m_name = aName; }
    const wxString& GetName() const       { return m_name; }

    void SetSize( int aSize )  { m_size = aSize; }
    int GetSize() const        { return m_size; }

    TOOLBAR_ITEM_TYPE GetType() const { return m_type; }

private:
    // Item type
    TOOLBAR_ITEM_TYPE m_type;

    // Tool properties
    TOOL_ACTION* m_action;

    // Spacer properties
    int m_size;

    // Group/control properties
    wxString m_name;
};


PANEL_TOOLBAR_CUSTOMIZATION::PANEL_TOOLBAR_CUSTOMIZATION( wxWindow* aParent, APP_SETTINGS_BASE* aCfg,
                                                          TOOLBAR_SETTINGS* aTbSettings,
                                                          const std::vector<TOOL_ACTION*>& aTools,
                                                          const std::vector<ACTION_TOOLBAR_CONTROL*>& aControls ) :
        PANEL_TOOLBAR_CUSTOMIZATION_BASE( aParent ),
        m_actionImageList( nullptr ),
        m_appSettings( aCfg ),
        m_appTbSettings( aTbSettings ),
        m_currentToolbar( TOOLBAR_LOC::TOP_MAIN )
{
    // Copy the tools and controls into the internal maps
    for( auto& tool : aTools )
        m_availableTools.emplace( tool->GetName(), tool );

    for( auto& control : aControls )
        m_availableControls.emplace( control->GetName(), control );

    // Configure the Ui elements
    m_btnToolDelete->SetBitmap( KiBitmapBundle( BITMAPS::small_trash ) );
    m_btnToolMoveUp->SetBitmap( KiBitmapBundle( BITMAPS::small_up ) );
    m_btnToolMoveDown->SetBitmap( KiBitmapBundle( BITMAPS::small_down ) );
    m_btnAddTool->SetBitmap( KiBitmapBundle( BITMAPS::left ) );

    m_insertButton->SetLabel( _( "Insert separator" ) );
    //m_insertButton->SetWidthPadding( 4 );

    // Populate the browse library options
    wxMenu* insertMenu = m_insertButton->GetSplitButtonMenu();

    insertMenu->Append( ID_SPACER_MENU, _( "Insert spacer" ) );
    insertMenu->Append( ID_GROUP_MENU, _( "Insert group" ) );

    insertMenu->Bind( wxEVT_COMMAND_MENU_SELECTED, &PANEL_TOOLBAR_CUSTOMIZATION::onSpacerPress,
                      this, ID_SPACER_MENU );
    insertMenu->Bind( wxEVT_COMMAND_MENU_SELECTED, &PANEL_TOOLBAR_CUSTOMIZATION::onGroupPress,
                      this, ID_GROUP_MENU );

    // This is the button only press for the browse button instead of the menu
    m_insertButton->Bind( wxEVT_BUTTON, &PANEL_TOOLBAR_CUSTOMIZATION::onSeparatorPress, this );

    // TODO (ISM): Enable moving up/down and draging
    m_btnToolMoveDown->Enable( false );
    m_btnToolMoveUp->Enable( false );
}


PANEL_TOOLBAR_CUSTOMIZATION::~PANEL_TOOLBAR_CUSTOMIZATION()
{
    delete m_actionImageList;
}


void PANEL_TOOLBAR_CUSTOMIZATION::ResetPanel()
{
    // Go over every toolbar and initialize things
    for( auto& tb : magic_enum::enum_values<TOOLBAR_LOC>() )
    {
        // Create a shadow toolbar
        auto tbConfig = m_appTbSettings->DefaultToolbarConfig( tb );

        if( tbConfig.has_value() )
            m_toolbars[tb] = tbConfig.value();
    }

    // Populate the toolbar view with the default toolbar
    m_tbChoice->SetSelection( 0 );

    auto firstTb = magic_enum::enum_cast<TOOLBAR_LOC>( 0 );

    if( firstTb.has_value() )
        m_currentToolbar = firstTb.value();

    populateToolbarTree();

}


bool PANEL_TOOLBAR_CUSTOMIZATION::TransferDataToWindow()
{
    wxArrayString tbChoices;

    // Go over every toolbar and initialize things
    for( auto& tb : magic_enum::enum_values<TOOLBAR_LOC>() )
    {
        // Create a shadow toolbar
        auto tbConfig = m_appTbSettings->GetToolbarConfig( tb );

        if( tbConfig.has_value() )
            m_toolbars.emplace( tb, tbConfig.value() );

        // Setup the UI name
        const auto& tbName = s_toolbarNameMap.find( tb );

        wxASSERT_MSG( tbName != s_toolbarNameMap.end(),
                      wxString::Format( "Unknown toolbar: %s", magic_enum::enum_name( tb ) ) );

        tbChoices.Add( tbName->second );
    }

    m_tbChoice->Set( tbChoices );

    // Always populate the actions before the toolbars, that way the icons are available
    populateActions();

    // Populate the toolbar view
    m_tbChoice->SetSelection( 0 );

    auto firstTb = magic_enum::enum_cast<TOOLBAR_LOC>( 0 );

    if( firstTb.has_value() )
        m_currentToolbar = firstTb.value();

    populateToolbarTree();

    // Sync the enable/disable control
    enableCustomControls( m_appSettings->m_CustomToolbars );
    m_customToolbars->SetValue( m_appSettings->m_CustomToolbars );

    return true;
}


bool PANEL_TOOLBAR_CUSTOMIZATION::TransferDataFromWindow()
{
    m_appSettings->m_CustomToolbars = m_customToolbars->GetValue();

    // Store the current toolbar
    std::optional<TOOLBAR_CONFIGURATION> currentTb = parseToolbarTree();

    if( currentTb.has_value() )
        m_toolbars[m_currentToolbar] = currentTb.value();

    // Write the shadow toolbars with changes back to the app toolbar settings
    for( const auto& [loc, config] : m_toolbars )
        m_appTbSettings->SetStoredToolbarConfig( loc, config );

    return true;
}


std::optional<TOOLBAR_CONFIGURATION> PANEL_TOOLBAR_CUSTOMIZATION::parseToolbarTree()
{
    TOOLBAR_CONFIGURATION config;

    wxTreeItemId      mainId;
    wxTreeItemId      rootId = m_toolbarTree->GetRootItem();
    wxTreeItemIdValue mainCookie;

    if( !rootId.IsOk() )
        return std::nullopt;

    mainId = m_toolbarTree->GetFirstChild( rootId, mainCookie );

    while( mainId.IsOk() )
    {
        wxTreeItemData* treeData = m_toolbarTree->GetItemData( mainId );

        TOOLBAR_TREE_ITEM_DATA* tbData = dynamic_cast<TOOLBAR_TREE_ITEM_DATA*>( treeData );

        wxCHECK2( tbData, continue );

        switch( tbData->GetType() )
        {
        case TOOLBAR_ITEM_TYPE::SPACER:
            config.AppendSpacer( tbData->GetSize() );
            break;

        case TOOLBAR_ITEM_TYPE::SEPARATOR:
            config.AppendSeparator();
            break;

        case TOOLBAR_ITEM_TYPE::CONTROL:
            config.AppendControl( tbData->GetName().ToStdString() );
            break;

        case TOOLBAR_ITEM_TYPE::TOOL:
            config.AppendAction( *( tbData->GetAction() ) );
            break;

        case TOOLBAR_ITEM_TYPE::TB_GROUP:
            TOOLBAR_GROUP_CONFIG grpConfig( tbData->GetName() );

            if( m_toolbarTree->ItemHasChildren( mainId ) )
            {
                wxTreeItemIdValue childCookie;
                wxTreeItemId      childId = m_toolbarTree->GetFirstChild( mainId, childCookie );

                while( childId.IsOk() )
                {
                    wxTreeItemData* childTreeData = m_toolbarTree->GetItemData( childId );

                    TOOLBAR_TREE_ITEM_DATA* childTbData = dynamic_cast<TOOLBAR_TREE_ITEM_DATA*>( childTreeData );

                    wxCHECK2( childTbData, break );

                    switch( childTbData->GetType() )
                    {
                    case TOOLBAR_ITEM_TYPE::TB_GROUP:
                    case TOOLBAR_ITEM_TYPE::SPACER:
                    case TOOLBAR_ITEM_TYPE::SEPARATOR:
                    case TOOLBAR_ITEM_TYPE::CONTROL:
                        wxASSERT_MSG( false, "Invalid entry in a group" );
                        break;

                    case TOOLBAR_ITEM_TYPE::TOOL:
                        grpConfig.AddAction( *( childTbData->GetAction() ) );
                        break;
                    }

                    childId = m_toolbarTree->GetNextChild( mainId, childCookie );
                }
            }

            config.AppendGroup( grpConfig );
        }

        mainId = m_toolbarTree->GetNextChild( rootId, mainCookie );
    }

    return config;
}


void PANEL_TOOLBAR_CUSTOMIZATION::populateToolbarTree()
{
    m_toolbarTree->DeleteAllItems();
    m_toolbarTree->SetImageList( m_actionImageList );

    const auto& it = m_toolbars.find( m_currentToolbar );

    if( it == m_toolbars.end() )
    {
        // Disable the controls and bail out - no toolbar here
        enableToolbarControls( false );
        return;
    }

    // Ensure the controls are enabled
    enableToolbarControls( true );

    TOOLBAR_CONFIGURATION toolbar = it->second;

    wxTreeItemId root = m_toolbarTree->AddRoot( "Toolbar" );

    for( const TOOLBAR_ITEM& item : toolbar.GetToolbarItems() )
    {
        switch( item.m_Type )
        {
        case TOOLBAR_ITEM_TYPE::SEPARATOR:
        {
            // Add a separator
            TOOLBAR_TREE_ITEM_DATA* sepTreeItem = new TOOLBAR_TREE_ITEM_DATA( TOOLBAR_ITEM_TYPE::SEPARATOR );
            m_toolbarTree->AppendItem( root, "Separator", -1, -1, sepTreeItem );
            break;
        }

        case TOOLBAR_ITEM_TYPE::SPACER:
        {
            // Add a spacer
            TOOLBAR_TREE_ITEM_DATA* spacerTreeItem = new TOOLBAR_TREE_ITEM_DATA( TOOLBAR_ITEM_TYPE::SPACER );
            spacerTreeItem->SetSize( item.m_Size );
            m_toolbarTree->AppendItem( root, wxString::Format( "Spacer: %i", item.m_Size ), -1, -1,
                                       spacerTreeItem );
            break;
        }

        case TOOLBAR_ITEM_TYPE::CONTROL:
        {
            // Add a control
            TOOLBAR_TREE_ITEM_DATA* controlTreeItem = new TOOLBAR_TREE_ITEM_DATA( TOOLBAR_ITEM_TYPE::CONTROL );
            controlTreeItem->SetName( item.m_ControlName );
            m_toolbarTree->AppendItem( root, item.m_ControlName, -1, -1, controlTreeItem );
            break;
        }

        case TOOLBAR_ITEM_TYPE::TOOL:
        {
            // Add a tool
            auto toolMap = m_availableTools.find( item.m_ActionName );

            if( toolMap == m_availableTools.end() )
            {
                wxASSERT_MSG( false, wxString::Format( "Unable to find tool %s", item.m_ActionName ) );
                continue;
            }

            TOOLBAR_TREE_ITEM_DATA* toolTreeItem = new TOOLBAR_TREE_ITEM_DATA( TOOLBAR_ITEM_TYPE::TOOL );
            toolTreeItem->SetAction( toolMap->second );

            int  imgIdx = -1;
            auto imgMap = m_actionImageListMap.find( item.m_ActionName );

            if( imgMap != m_actionImageListMap.end() )
                imgIdx = imgMap->second;

            m_toolbarTree->AppendItem( root, toolMap->second->GetFriendlyName(), imgIdx, -1, toolTreeItem );
            break;
        }

        case TOOLBAR_ITEM_TYPE::TB_GROUP:
        {
            // Add a group of items to the toolbar
            TOOLBAR_TREE_ITEM_DATA* groupTreeItem = new TOOLBAR_TREE_ITEM_DATA( TOOLBAR_ITEM_TYPE::TB_GROUP );
            groupTreeItem->SetName( item.m_GroupName );

            wxTreeItemId groupId = m_toolbarTree->AppendItem( root, item.m_GroupName, -1, -1,
                                                              groupTreeItem );

            // Add the elements below the group
            for( const TOOLBAR_ITEM& groupItem : item.m_GroupItems )
            {
                auto toolMap = m_availableTools.find( groupItem.m_ActionName );

                if( toolMap == m_availableTools.end() )
                {
                    wxASSERT_MSG( false, wxString::Format( "Unable to find group tool %s", groupItem.m_ActionName ) );
                    continue;
                }

                TOOLBAR_TREE_ITEM_DATA* toolTreeItem = new TOOLBAR_TREE_ITEM_DATA( TOOLBAR_ITEM_TYPE::TOOL );
                toolTreeItem->SetAction( toolMap->second );

                int  imgIdx = -1;
                auto imgMap = m_actionImageListMap.find( groupItem.m_ActionName );

                if( imgMap != m_actionImageListMap.end() )
                    imgIdx = imgMap->second;

                m_toolbarTree->AppendItem( groupId, toolMap->second->GetFriendlyName(),
                                           imgIdx, -1, toolTreeItem );
            }

            break;
        }
        }
    }

    m_toolbarTree->ExpandAll();

    wxTreeItemIdValue temp;
    wxTreeItemId firstItem = m_toolbarTree->GetFirstChild( root, temp );

    if( firstItem.IsOk()  )
    {
        m_toolbarTree->SelectItem( firstItem );
        m_toolbarTree->EnsureVisible( firstItem );
    }
}


void PANEL_TOOLBAR_CUSTOMIZATION::populateActions()
{
    // Clear all existing information for the actions
    delete m_actionImageList;
    m_actionImageListMap.clear();
    m_actionImageBundleVector.clear();

    // Prep the control
    m_actionsList->DeleteAllItems();
    m_actionsList->DeleteAllColumns();
    m_actionsList->InsertColumn( 0, "", wxLIST_FORMAT_LEFT, wxLIST_AUTOSIZE );

    // Prepare the image list (taken from project_tree.cpp)
    int logicSize = 24 * GetDPIScaleFactor() / GetContentScaleFactor(); // Cross-platform way
    int physSize = ToPhys( logicSize ); // aka *GetContentScaleFactor()

    if( physSize >= 64 )
        physSize = 64;
    else if( physSize >= 48 )
        physSize = 48;
    else if( physSize >= 32 )
        physSize = 32;
    else
        physSize = 24;

    logicSize = std::min( logicSize, physSize );
    int bmpsf = std::max( 1, physSize / logicSize );

    logicSize = physSize / bmpsf;

    auto toBitmap =
            [&]( BITMAPS aBmps )
            {
                wxBitmap bmp = KiBitmap( aBmps, physSize );
                bmp.SetScaleFactor( bmpsf );
                wxASSERT(bmp.IsOk());
                return bmp;
            };

    m_actionImageList = new wxImageList( logicSize, logicSize, true,
                                         static_cast<int>( m_availableTools.size() ) );

    // Populate the various image lists for the action icons, and the actual control
    int itemIdx = 0;

    for( const auto& [k, tool] : m_availableTools )
    {
        if( tool->CheckToolbarState( TOOLBAR_STATE::HIDDEN ) )
            continue;

        wxListItem item;
        item.SetText( tool->GetFriendlyName() );
        item.SetData( static_cast<void*>( tool ) );
        item.SetId( itemIdx++ );

        if( tool->GetIcon() != BITMAPS::INVALID_BITMAP )
        {
            int idx = m_actionImageList->Add( toBitmap( tool->GetIcon() ) );

            // If the image list throws away the image, then we shouldn't show the image anywhere.
            // TODO: Make sure all images have all possible sizes so the image list doesn't get grumpy.
            if( idx != -1 )
            {
                m_actionImageBundleVector.push_back( KiBitmapBundle( tool->GetIcon() ) );
                m_actionImageListMap.emplace( tool->GetName(), idx );

                item.SetImage( idx );
            }
        }

        m_actionsList->InsertItem( item );
    }

    m_actionsList->SetSmallImages( m_actionImageBundleVector );

    // This must be done after adding everything to the list to make the columns wide enough
    m_actionsList->SetColumnWidth( 0, wxLIST_AUTOSIZE );
}


void PANEL_TOOLBAR_CUSTOMIZATION::onGroupPress( wxCommandEvent& aEvent )
{
    TOOLBAR_TREE_ITEM_DATA* treeItem = new TOOLBAR_TREE_ITEM_DATA( TOOLBAR_ITEM_TYPE::TB_GROUP,
                                                                   _( "Group" ) );

    wxTreeItemId newItem;
    wxTreeItemId selItem = m_toolbarTree->GetSelection();

    if( selItem.IsOk() )
    {
        // Can't add a group onto a group
        wxTreeItemId parent = m_toolbarTree->GetItemParent( selItem );

        if( parent.IsOk() )
        {
            wxTreeItemId secondParent = m_toolbarTree->GetItemParent( parent );

            if( secondParent.IsOk() )
            {
                delete treeItem;
                return;
            }
        }

        newItem = m_toolbarTree->InsertItem( m_toolbarTree->GetRootItem(), selItem, treeItem->GetName(),
                                             -1, -1, treeItem );
    }
    else
    {
        newItem = m_toolbarTree->AppendItem( m_toolbarTree->GetRootItem(), treeItem->GetName(), -1, -1,
                                             treeItem );
    }

    if( newItem.IsOk() )
    {
        m_toolbarTree->SelectItem( newItem );
        m_toolbarTree->EnsureVisible( newItem );
    }
}


void PANEL_TOOLBAR_CUSTOMIZATION::onSpacerPress( wxCommandEvent& aEvent )
{
    TOOLBAR_TREE_ITEM_DATA* treeItem = new TOOLBAR_TREE_ITEM_DATA( TOOLBAR_ITEM_TYPE::SPACER, 5 );

    wxString label = wxString::Format( "Spacer: %i", treeItem->GetSize() );

    wxTreeItemId newItem;
    wxTreeItemId selItem = m_toolbarTree->GetSelection();

    if( selItem.IsOk() )
    {
        // Insert after the current selection at the same level
        wxTreeItemId parent = m_toolbarTree->GetItemParent( selItem );

        // Can't insert a spacer in a group yet
        if( parent.IsOk() )
        {
            wxTreeItemId secondParent = m_toolbarTree->GetItemParent( parent );

            if( secondParent.IsOk() )
            {
                delete treeItem;
                return;
            }
        }

        newItem = m_toolbarTree->InsertItem( parent, selItem, label, -1, -1, treeItem );
    }
    else
        newItem = m_toolbarTree->AppendItem( m_toolbarTree->GetRootItem(), label, -1, -1, treeItem );

    if( newItem.IsOk() )
    {
        m_toolbarTree->SelectItem( newItem );
        m_toolbarTree->EnsureVisible( newItem );
    }
}


void PANEL_TOOLBAR_CUSTOMIZATION::onSeparatorPress( wxCommandEvent& aEvent )
{
    TOOLBAR_TREE_ITEM_DATA* treeItem = new TOOLBAR_TREE_ITEM_DATA( TOOLBAR_ITEM_TYPE::SEPARATOR );

    wxTreeItemId newItem;
    wxTreeItemId selItem = m_toolbarTree->GetSelection();

    if( selItem.IsOk() )
    {
        // Insert after the current selection at the same level
        wxTreeItemId parent = m_toolbarTree->GetItemParent( selItem );

        // Can't insert a separator in a group yet
        if( parent.IsOk() )
        {
            wxTreeItemId secondParent = m_toolbarTree->GetItemParent( parent );

            if( secondParent.IsOk() )
            {
                delete treeItem;
                return;
            }
        }

        newItem = m_toolbarTree->InsertItem( parent, selItem, "Separator", -1, -1, treeItem );
    }
    else
    {
        newItem = m_toolbarTree->AppendItem( m_toolbarTree->GetRootItem(), "Separator", -1, -1,
                                             treeItem );
    }

    if( newItem.IsOk() )
    {
        m_toolbarTree->SelectItem( newItem );
        m_toolbarTree->EnsureVisible( newItem );
    }
}


void PANEL_TOOLBAR_CUSTOMIZATION::onCustomizeTbCb( wxCommandEvent& event )
{
    enableCustomControls( event.IsChecked() );
}


void PANEL_TOOLBAR_CUSTOMIZATION::enableCustomControls( bool enable )
{
    m_tbChoice->Enable( enable );
    enableToolbarControls( enable );
}


void PANEL_TOOLBAR_CUSTOMIZATION::enableToolbarControls( bool enable )
{
    m_toolbarTree->Enable( enable );
    m_btnAddTool->Enable( enable );
    m_btnToolDelete->Enable( enable );

    // TODO (ISM): Enable moving up/down
    //m_btnToolMoveDown->Enable( enable );
    //m_btnToolMoveUp->Enable( enable );
    m_actionsList->Enable( enable );
    m_insertButton->Enable( enable );
}


void PANEL_TOOLBAR_CUSTOMIZATION::onToolDelete( wxCommandEvent& event )
{
    wxTreeItemId item = m_toolbarTree->GetSelection();

    if( !item.IsOk() )
        return;

    // The tree control defaults to nothing selected if you delete the item
    // at the last place, so we have to manually get the itme immediately before
    // the one we will delete, and then select it if nothing is selected.
    wxTreeItemId prev = m_toolbarTree->GetPrevSibling( item );

    m_toolbarTree->Delete( item );

    item = m_toolbarTree->GetSelection();

    if( !item.IsOk() && prev.IsOk() )
    {
        m_toolbarTree->SelectItem( prev );
        m_toolbarTree->EnsureVisible( prev );
    }
}


void PANEL_TOOLBAR_CUSTOMIZATION::onToolMoveUp( wxCommandEvent& event )
{

}


void PANEL_TOOLBAR_CUSTOMIZATION::onToolMoveDown( wxCommandEvent& event )
{

}


void PANEL_TOOLBAR_CUSTOMIZATION::onBtnAddAction( wxCommandEvent& event )
{
    // Get the selected item
    long actionIdx = m_actionsList->GetNextItem( -1, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED );

    // Nothing is selected, bail out
    if( actionIdx < 0 )
        return;

    // This is needed because GetItemData returns a wxUIntPtr, which is actually size_t...
    void*        v      = ( void* ) m_actionsList->GetItemData( actionIdx );
    TOOL_ACTION* action = static_cast<TOOL_ACTION*>( v );

    // Build the item to add
    TOOLBAR_TREE_ITEM_DATA* toolTreeItem = new TOOLBAR_TREE_ITEM_DATA( TOOLBAR_ITEM_TYPE::TOOL );
    toolTreeItem->SetAction( action );

    int  imgIdx = -1;
    auto imgMap = m_actionImageListMap.find( action->GetName() );

    if( imgMap != m_actionImageListMap.end() )
        imgIdx = imgMap->second;

    // Actually add the item
    wxString     label   = action->GetFriendlyName();
    wxTreeItemId selItem = m_toolbarTree->GetSelection();
    wxTreeItemId newItem;

    if( selItem.IsOk() )
    {
        TOOLBAR_TREE_ITEM_DATA* data =
                dynamic_cast<TOOLBAR_TREE_ITEM_DATA*>( m_toolbarTree->GetItemData( selItem ) );

        if( data && data->GetType() == TOOLBAR_ITEM_TYPE::TB_GROUP )
        {
            // Insert into the end of the group
            newItem = m_toolbarTree->AppendItem( selItem, label, imgIdx, -1, toolTreeItem );
        }
        else
        {
            // Insert after the current selection at the same level
            wxTreeItemId parent = m_toolbarTree->GetItemParent( selItem );
            newItem = m_toolbarTree->InsertItem( parent, selItem, label, imgIdx, -1, toolTreeItem );
        }
    }
    else
    {
        // Insert at the root level if there is no selection
        newItem = m_toolbarTree->AppendItem( m_toolbarTree->GetRootItem(), label, imgIdx, -1,
                                             toolTreeItem );
    }

    if( newItem.IsOk() )
    {
        m_toolbarTree->SelectItem( newItem );
        m_toolbarTree->EnsureVisible( newItem );

        // Move the action to the next available one, to be nice
        if( ++actionIdx < m_actionsList->GetItemCount() )
            m_actionsList->SetItemState( actionIdx, wxLIST_STATE_SELECTED, wxLIST_STATE_SELECTED );
    }
}


void PANEL_TOOLBAR_CUSTOMIZATION::onTreeBeginLabelEdit( wxTreeEvent& event )
{
    wxTreeItemId id = event.GetItem();

    if( id.IsOk() )
    {
        wxTreeItemData* treeData = m_toolbarTree->GetItemData( id );

        if( TOOLBAR_TREE_ITEM_DATA* tbData = dynamic_cast<TOOLBAR_TREE_ITEM_DATA*>( treeData ) )
        {
            switch( tbData->GetType() )
            {
            case TOOLBAR_ITEM_TYPE::TOOL:
            case TOOLBAR_ITEM_TYPE::CONTROL:
            case TOOLBAR_ITEM_TYPE::SEPARATOR:
                // Don't let these be edited
                event.Veto();
                break;

            case TOOLBAR_ITEM_TYPE::TB_GROUP:
            case TOOLBAR_ITEM_TYPE::SPACER:
                // Do nothing here
                break;
            }
        }
    }
}


void PANEL_TOOLBAR_CUSTOMIZATION::onTreeEndLabelEdit( wxTreeEvent& event )
{

}


void PANEL_TOOLBAR_CUSTOMIZATION::onTbChoiceSelect( wxCommandEvent& event )
{
    // Store the current toolbar
    std::optional<TOOLBAR_CONFIGURATION> currentTb = parseToolbarTree();

    if( currentTb.has_value() )
        m_toolbars[m_currentToolbar] = currentTb.value();

    // Populate the new one
    auto newTb = magic_enum::enum_cast<TOOLBAR_LOC>( event.GetInt() );

    if( newTb.has_value() )
    {
        m_currentToolbar = newTb.value();
        populateToolbarTree();
    }
}
