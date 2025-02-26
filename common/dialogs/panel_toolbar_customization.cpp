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

#include <wx/listctrl.h>
#include <wx/menu.h>

// Simple IDs for the split button menu
enum
{
    ID_SEPARATOR_MENU = ( wxID_HIGHEST + 5 ),
    ID_SPACER_MENU,
    ID_GROUP_MENU
};


enum class TOOLBAR_TREE_ITEM_TYPE
{
    TOOL,
    GROUP,
    SPACER,
    SEPARATOR
};

class TOOLBAR_TREE_ITEM_DATA : public wxTreeItemData
{
public:
    TOOLBAR_TREE_ITEM_DATA( TOOLBAR_TREE_ITEM_TYPE aType ) :
        wxTreeItemData(),
        m_name( "" ),
        m_action( nullptr ),
        m_type( aType )
    {
        // Hard-code the name for the separator
        if( aType == TOOLBAR_TREE_ITEM_TYPE::SEPARATOR )
            m_name = "separator";
    }

    void SetAction( TOOL_ACTION* aAction ) { m_action = aAction; }
    TOOL_ACTION* GetAction() const         { return m_action; }

    void SetName( std::string aName )  { m_name = aName; }
    std::string GetName() const        { return m_name; }

    TOOLBAR_TREE_ITEM_TYPE GetType() const { return m_type; }

private:
    std::string  m_name;
    TOOL_ACTION* m_action;

    TOOLBAR_TREE_ITEM_TYPE m_type;
};


PANEL_TOOLBAR_CUSTOMIZATION::PANEL_TOOLBAR_CUSTOMIZATION( wxWindow* aParent, APP_SETTINGS_BASE* aCfg,
                                                          TOOLBAR_SETTINGS* aTbSettings,
                                                          std::vector<TOOL_ACTION*> aTools,
                                                          std::vector<ACTION_TOOLBAR_CONTROL*> aControls ) :
        PANEL_TOOLBAR_CUSTOMIZATION_BASE( aParent ),
        m_actionImageList( nullptr ),
        m_appSettings( aCfg ),
        m_tbSettings( aTbSettings )
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
}


PANEL_TOOLBAR_CUSTOMIZATION::~PANEL_TOOLBAR_CUSTOMIZATION()
{
    delete m_actionImageList;
}

void PANEL_TOOLBAR_CUSTOMIZATION::ResetPanel()
{

}


bool PANEL_TOOLBAR_CUSTOMIZATION::TransferDataToWindow()
{
    auto tb = m_tbSettings->GetToolbarConfig( TOOLBAR_LOC::RIGHT, false );

    // Always populate the actions before the toolbars, that way the icons are available
    populateActions( m_availableTools, m_availableControls );

    // Populate the choicebox to select the toolbar to edit


    if( tb.has_value() )
        populateToolbarTree( tb.value() );

    // Sync the enable/disable control
    enableCustomControls( m_appSettings->m_CustomToolbars );
    m_customToolbars->SetValue( m_appSettings->m_CustomToolbars );

    return true;
}


bool PANEL_TOOLBAR_CUSTOMIZATION::TransferDataFromWindow()
{
    return true;
}


void PANEL_TOOLBAR_CUSTOMIZATION::populateToolbarTree( const TOOLBAR_CONFIGURATION& aToolbar )
{
    m_toolbarTree->DeleteAllItems();
    m_toolbarTree->SetImageList( m_actionImageList );

    wxTreeItemId root = m_toolbarTree->AddRoot( "Toolbar" );

    for( auto& item : aToolbar.GetToolbarItems() )
    {
        if( item == "separator" )
        {
            // Add a separator
            TOOLBAR_TREE_ITEM_DATA* sepTreeItem = new TOOLBAR_TREE_ITEM_DATA( TOOLBAR_TREE_ITEM_TYPE::SEPARATOR );
            m_toolbarTree->AppendItem( root, "Separator", -1, -1, sepTreeItem );
        }
        else if( item.starts_with( "group" ) )
        {
            // Add a group of items to the toolbar
            const TOOLBAR_GROUP_CONFIG* groupConfig = aToolbar.GetGroup( item );

            if( !groupConfig )
            {
                wxASSERT_MSG( false, wxString::Format( "Unable to find group %s", item ) );
                continue;
            }

            TOOLBAR_TREE_ITEM_DATA* groupTreeItem = new TOOLBAR_TREE_ITEM_DATA( TOOLBAR_TREE_ITEM_TYPE::GROUP );
            groupTreeItem->SetName( item );

            wxTreeItemId groupId = m_toolbarTree->AppendItem( root, item, -1, -1, groupTreeItem );

            // Add the elements below the group
            for( auto& groupItem : groupConfig->GetGroupItems() )
            {
                auto toolMap = m_availableTools.find( groupItem );

                if( toolMap == m_availableTools.end() )
                {
                    wxASSERT_MSG( false, wxString::Format( "Unable to find group tool %s", groupItem ) );
                    continue;
                }

                TOOLBAR_TREE_ITEM_DATA* toolTreeItem = new TOOLBAR_TREE_ITEM_DATA( TOOLBAR_TREE_ITEM_TYPE::TOOL );
                toolTreeItem->SetName( groupItem );
                toolTreeItem->SetAction( toolMap->second );

                int  imgIdx = -1;
                auto imgMap = m_actionImageListMap.find( groupItem );

                if( imgMap != m_actionImageListMap.end() )
                    imgIdx = imgMap->second;

                m_toolbarTree->AppendItem( groupId, toolMap->second->GetFriendlyName(),
                                           imgIdx, -1, toolTreeItem );
            }
        }
        else if( item.starts_with( "control" ) )
        {
            // Add a custom control to the toolbar

        }
        else if( item.starts_with( "spacer" ) )
        {

        }
        else
        {
            // Add a tool
            auto toolMap = m_availableTools.find( item );

            if( toolMap == m_availableTools.end() )
            {
                wxASSERT_MSG( false, wxString::Format( "Unable to find tool %s", item ) );
                continue;
            }

            TOOLBAR_TREE_ITEM_DATA* toolTreeItem = new TOOLBAR_TREE_ITEM_DATA( TOOLBAR_TREE_ITEM_TYPE::TOOL );
            toolTreeItem->SetName( item );
            toolTreeItem->SetAction( toolMap->second );

            int  imgIdx = -1;
            auto imgMap = m_actionImageListMap.find( item );

            if( imgMap != m_actionImageListMap.end() )
                imgIdx = imgMap->second;

            m_toolbarTree->AppendItem( root, toolMap->second->GetFriendlyName(),
                                       imgIdx, -1, toolTreeItem );
        }
    }

    m_toolbarTree->ExpandAll();
}


void PANEL_TOOLBAR_CUSTOMIZATION::populateActions( const std::map<std::string, TOOL_ACTION*>& aTools,
                                                   const std::map<std::string, ACTION_TOOLBAR_CONTROL*>& aControls )
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

    auto toBitmap = [&]( BITMAPS aBmps )
    {
        wxBitmap bmp = KiBitmap( aBmps, physSize );
        bmp.SetScaleFactor( bmpsf );
        wxASSERT(bmp.IsOk());
        return bmp;
    };

    m_actionImageList = new wxImageList( logicSize, logicSize, true,
                                         static_cast<int>( aTools.size() ) );

    // Populate the various image lists for the action icons, and the actual control
    int itemIdx = 0;

    for( auto [k, tool] : aTools )
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

}


void PANEL_TOOLBAR_CUSTOMIZATION::onSpacerPress( wxCommandEvent& aEvent )
{

}


void PANEL_TOOLBAR_CUSTOMIZATION::onSeparatorPress( wxCommandEvent& aEvent )
{

}


void PANEL_TOOLBAR_CUSTOMIZATION::onCustomizeTbCb( wxCommandEvent& event )
{
    enableCustomControls( event.IsChecked() );
}


void PANEL_TOOLBAR_CUSTOMIZATION::enableCustomControls( bool enable )
{
    m_tbChoice->Enable( enable );
    m_toolbarTree->Enable( enable );
    m_btnAddTool->Enable( enable );
    m_btnToolDelete->Enable( enable );
    m_btnToolMoveDown->Enable( enable );
    m_btnToolMoveUp->Enable( enable );
    m_actionsList->Enable( enable );
    m_insertButton->Enable( enable );
}


void PANEL_TOOLBAR_CUSTOMIZATION::OnToolDelete( wxCommandEvent& event )
{

}


void PANEL_TOOLBAR_CUSTOMIZATION::OnToolMoveUp( wxCommandEvent& event )
{

}


void PANEL_TOOLBAR_CUSTOMIZATION::OnToolMoveDown( wxCommandEvent& event )
{

}
