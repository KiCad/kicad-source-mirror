/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2019 CERN
 * Copyright (C) 2019 KiCad Developers, see CHANGELOG.txt for contributors.
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

#include <algorithm>
#include <advanced_config.h>
#include <bitmaps.h>
#include <eda_draw_frame.h>
#include <functional>
#include <kiplatform/ui.h>
#include <math/util.h>
#include <memory>
#include <pgm_base.h>
#include <settings/common_settings.h>
#include <tool/action_toolbar.h>
#include <tool/actions.h>
#include <tool/tool_event.h>
#include <tool/tool_interactive.h>
#include <tool/tool_manager.h>
#include <widgets/bitmap_button.h>
#include <widgets/wx_aui_art_providers.h>
#include <wx/popupwin.h>
#include <wx/renderer.h>
#include <wx/sizer.h>


wxBitmap MakeDisabledBitmap( const wxBitmap& aSource )
{
    return aSource.ConvertToDisabled( KIPLATFORM::UI::IsDarkTheme() ? 70 : 255 );
}


ACTION_GROUP::ACTION_GROUP( std::string aName, const std::vector<const TOOL_ACTION*>& aActions )
{
    wxASSERT_MSG( aActions.size() > 0, "Action groups must have at least one action" );

    // The default action is just the first action in the vector
    m_actions       = aActions;
    m_defaultAction = m_actions[0];

    m_name = aName;
    m_id   = ACTION_MANAGER::MakeActionId( m_name );
}


void ACTION_GROUP::SetDefaultAction( const TOOL_ACTION& aDefault )
{
    bool valid = std::any_of( m_actions.begin(), m_actions.end(),
                              [&]( const TOOL_ACTION* aAction ) -> bool
                              {
                                  // For some reason, we can't compare the actions directly
                                  return aAction->GetId() == aDefault.GetId();
                              } );

    wxASSERT_MSG( valid, "Action must be present in a group to be the default" );

    m_defaultAction = &aDefault;
}


#define PALETTE_BORDER 4    // The border around the palette buttons on all sides
#define BUTTON_BORDER  1    // The border on the sides of the buttons that touch other buttons


ACTION_TOOLBAR_PALETTE::ACTION_TOOLBAR_PALETTE( wxWindow* aParent, bool aVertical ) :
        wxPopupTransientWindow( aParent, wxBORDER_NONE ),
        m_group( nullptr ),
        m_isVertical( aVertical ),
        m_panel( nullptr ),
        m_mainSizer( nullptr ),
        m_buttonSizer( nullptr )
{
    m_panel = new wxPanel( this, wxID_ANY );
    m_panel->SetBackgroundColour( wxSystemSettings::GetColour( wxSYS_COLOUR_WINDOW ) );

    // This sizer holds the buttons for the actions
    m_buttonSizer = new wxBoxSizer( aVertical ? wxVERTICAL : wxHORIZONTAL );

    // This sizer holds the other sizer, so that a consistent border is present on all sides
    m_mainSizer = new wxBoxSizer( aVertical ? wxVERTICAL : wxHORIZONTAL );
    m_mainSizer->Add( m_buttonSizer, wxSizerFlags().Border( wxALL, PALETTE_BORDER ) );

    m_panel->SetSizer( m_mainSizer );

    Connect( wxEVT_CHAR_HOOK, wxCharEventHandler( ACTION_TOOLBAR_PALETTE::onCharHook ),
             NULL, this );
}


void ACTION_TOOLBAR_PALETTE::AddAction( const TOOL_ACTION& aAction )
{
    wxBitmap normalBmp   = KiScaledBitmap( aAction.GetIcon(), this );
    wxBitmap disabledBmp = MakeDisabledBitmap( normalBmp );

    int padding = ( m_buttonSize.GetWidth() - normalBmp.GetWidth() ) / 2;

    BITMAP_BUTTON* button = new BITMAP_BUTTON( m_panel, aAction.GetUIId() );

    button->SetBitmap( normalBmp );
    button->SetDisabledBitmap( disabledBmp );
    button->SetPadding( padding );
    button->SetToolTip( aAction.GetDescription() );

    m_buttons[aAction.GetUIId()] = button;

    if( m_isVertical )
        m_buttonSizer->Add( button, wxSizerFlags().Border( wxTOP | wxBOTTOM, BUTTON_BORDER ) );
    else
        m_buttonSizer->Add( button, wxSizerFlags().Border( wxLEFT | wxRIGHT, BUTTON_BORDER ) );

    m_buttonSizer->Layout();
}


void ACTION_TOOLBAR_PALETTE::EnableAction( const TOOL_ACTION& aAction, bool aEnable )
{
    auto it = m_buttons.find( aAction.GetUIId() );

    if( it != m_buttons.end() )
        it->second->Enable( aEnable );
}


void ACTION_TOOLBAR_PALETTE::CheckAction( const TOOL_ACTION& aAction, bool aCheck )
{
    auto it = m_buttons.find( aAction.GetUIId() );

    if( it != m_buttons.end() )
        it->second->Check( aCheck );
}


void ACTION_TOOLBAR_PALETTE::Popup( wxWindow* aFocus )
{
    m_mainSizer->Fit( m_panel );
    SetClientSize( m_panel->GetSize() );

    wxPopupTransientWindow::Popup( aFocus );
}


void ACTION_TOOLBAR_PALETTE::onCharHook( wxKeyEvent& aEvent )
{
    // Allow the escape key to dismiss this popup
    if( aEvent.GetKeyCode() == WXK_ESCAPE )
        Dismiss();
    else
        aEvent.Skip();
}


ACTION_TOOLBAR::ACTION_TOOLBAR( EDA_BASE_FRAME* parent, wxWindowID id, const wxPoint& pos,
                                const wxSize& size, long style ) :
    wxAuiToolBar( parent, id, pos, size, style ),
    m_paletteTimer( nullptr ),
    m_auiManager( nullptr ),
    m_toolManager( parent->GetToolManager() ),
    m_palette( nullptr )
{
    m_paletteTimer = new wxTimer( this );

#if !wxCHECK_VERSION( 3, 1, 0 )
    // Custom art provider makes dark mode work on wx < 3.1
    WX_AUI_TOOLBAR_ART* newArt = new WX_AUI_TOOLBAR_ART();
    SetArtProvider( newArt );
#endif

    Connect( wxEVT_COMMAND_TOOL_CLICKED, wxAuiToolBarEventHandler( ACTION_TOOLBAR::onToolEvent ),
             NULL, this );
    Connect( wxEVT_AUITOOLBAR_RIGHT_CLICK, wxAuiToolBarEventHandler( ACTION_TOOLBAR::onToolRightClick ),
             NULL, this );
    Connect( wxEVT_AUITOOLBAR_BEGIN_DRAG, wxAuiToolBarEventHandler( ACTION_TOOLBAR::onItemDrag ),
             NULL, this );
    Connect( wxEVT_LEFT_DOWN, wxMouseEventHandler( ACTION_TOOLBAR::onMouseClick ),
             NULL, this );
    Connect( wxEVT_LEFT_UP, wxMouseEventHandler( ACTION_TOOLBAR::onMouseClick ),
             NULL, this );
    Connect( m_paletteTimer->GetId(), wxEVT_TIMER, wxTimerEventHandler( ACTION_TOOLBAR::onTimerDone ),
             NULL, this );
}


ACTION_TOOLBAR::~ACTION_TOOLBAR()
{
    delete m_paletteTimer;

    // Clear all the maps keeping track of our items on the toolbar
    m_toolMenus.clear();
    m_actionGroups.clear();
    m_toolCancellable.clear();
    m_toolKinds.clear();
    m_toolActions.clear();
}


void ACTION_TOOLBAR::Add( const TOOL_ACTION& aAction, bool aIsToggleEntry, bool aIsCancellable )
{
    wxASSERT( GetParent() );
    wxASSERT_MSG( !( aIsCancellable && !aIsToggleEntry ), "aIsCancellable requires aIsToggleEntry" );

    int      toolId = aAction.GetUIId();
    wxBitmap bmp    = KiScaledBitmap( aAction.GetIcon(), GetParent() );

    AddTool( toolId, wxEmptyString, bmp, MakeDisabledBitmap( bmp ),
             aIsToggleEntry ? wxITEM_CHECK : wxITEM_NORMAL,
             aAction.GetDescription(), wxEmptyString, nullptr );

    m_toolKinds[ toolId ]       = aIsToggleEntry;
    m_toolActions[ toolId ]     = &aAction;
    m_toolCancellable[ toolId ] = aIsCancellable;
}


void ACTION_TOOLBAR::AddButton( const TOOL_ACTION& aAction )
{
    int      toolId = aAction.GetUIId();
    wxBitmap bmp    = KiScaledBitmap( aAction.GetIcon(), GetParent() );

    AddTool( toolId, wxEmptyString, bmp, MakeDisabledBitmap( bmp ),
             wxITEM_NORMAL, aAction.GetDescription(), wxEmptyString, nullptr );

    m_toolKinds[ toolId ] = false;
    m_toolActions[ toolId ] = &aAction;
}


void ACTION_TOOLBAR::AddScaledSeparator( wxWindow* aWindow )
{
    int scale = Pgm().GetCommonSettings()->m_Appearance.icon_scale;

    if( scale == 0 )
        scale = KiIconScale( aWindow );

    if( scale > 4 )
        AddSpacer( 16 * ( scale - 4 ) / 4 );

    AddSeparator();

    if( scale > 4 )
        AddSpacer( 16 * ( scale - 4 ) / 4 );
}


void ACTION_TOOLBAR::AddToolContextMenu( const TOOL_ACTION& aAction,
                                         std::unique_ptr<ACTION_MENU> aMenu )
{
    int toolId = aAction.GetUIId();

    m_toolMenus[toolId] = std::move( aMenu );
}


void ACTION_TOOLBAR::AddGroup( ACTION_GROUP* aGroup, bool aIsToggleEntry )
{
    int                groupId       = aGroup->GetUIId();
    const TOOL_ACTION* defaultAction = aGroup->GetDefaultAction();
    wxBitmap           bmp           = KiScaledBitmap( defaultAction->GetIcon(), GetParent() );

    wxASSERT( GetParent() );
    wxASSERT( defaultAction );

    m_toolKinds[ groupId ]    = aIsToggleEntry;
    m_toolActions[ groupId ]  = defaultAction;
    m_actionGroups[ groupId ] = aGroup;

    // Add the main toolbar item representing the group
    AddTool( groupId, wxEmptyString, bmp, MakeDisabledBitmap( bmp ),
             aIsToggleEntry ? wxITEM_CHECK : wxITEM_NORMAL,
             wxEmptyString, wxEmptyString, nullptr );

    // Select the default action
    doSelectAction( aGroup, *defaultAction );
}


void ACTION_TOOLBAR::SelectAction( ACTION_GROUP* aGroup, const TOOL_ACTION& aAction )
{
    bool valid = std::any_of( aGroup->m_actions.begin(), aGroup->m_actions.end(),
                              [&]( const TOOL_ACTION* action2 ) -> bool
                              {
                                  // For some reason, we can't compare the actions directly
                                  return aAction.GetId() == action2->GetId();
                              } );

    if( valid )
        doSelectAction( aGroup, aAction );
}


void ACTION_TOOLBAR::doSelectAction( ACTION_GROUP* aGroup, const TOOL_ACTION& aAction )
{
    wxASSERT( GetParent() );

    int groupId = aGroup->GetUIId();

    wxAuiToolBarItem* item   = FindTool( groupId );

    if( !item )
        return;

    // Update the item information
    item->SetShortHelp( aAction.GetDescription() );
    item->SetBitmap( KiScaledBitmap( aAction.GetIcon(), GetParent() ) );
    item->SetDisabledBitmap( MakeDisabledBitmap( item->GetBitmap() ) );

    // Register a new handler with the new UI conditions
    if( m_toolManager )
    {
        const ACTION_CONDITIONS* cond = m_toolManager->GetActionManager()->GetCondition( aAction );

        wxASSERT_MSG( cond, wxString::Format( "Missing UI condition for action %s",
                                              aAction.GetName() ) );

        m_toolManager->GetToolHolder()->UnregisterUIUpdateHandler( groupId );
        m_toolManager->GetToolHolder()->RegisterUIUpdateHandler( groupId, *cond );
    }

    // Update the currently selected action
    m_toolActions[ groupId ] = &aAction;

    Refresh();
}


void ACTION_TOOLBAR::UpdateControlWidth( int aID )
{
    wxAuiToolBarItem* item = FindTool( aID );
    wxASSERT_MSG( item, wxString::Format( "No toolbar item found for ID %d", aID ) );

    // The control on the toolbar is stored inside the window field of the item
    wxControl* control = dynamic_cast<wxControl*>( item->GetWindow() );
    wxASSERT_MSG( control, wxString::Format( "No control located in toolbar item with ID %d", aID ) );

    // Update the size the item has stored using the best size of the control
    wxSize bestSize = control->GetBestSize();
    item->SetMinSize( bestSize );

    // Update the sizer item sizes
    // This is a bit convoluted because there are actually 2 sizers that need to be updated:
    // 1. The main sizer that is used for the entire toolbar (this sizer item can be found in the
    // toolbar item)
    if( wxSizerItem* szrItem = item->GetSizerItem() )
        szrItem->SetMinSize( bestSize );

    // 2. The controls have a second sizer that allows for padding above/below the control with stretch
    // space, so we also need to update the sizer item for the control in that sizer with the new size.
    // We let wx do the search for us, since SetItemMinSize is recursive and will locate the control
    // on that sizer.
    if( m_sizer )
    {
        m_sizer->SetItemMinSize( control, bestSize );

        // Now actually update the toolbar with the new sizes
        m_sizer->Layout();
    }
}


void ACTION_TOOLBAR::ClearToolbar()
{
    // Clear all the maps keeping track of our items on the toolbar
    m_toolMenus.clear();
    m_actionGroups.clear();
    m_toolCancellable.clear();
    m_toolKinds.clear();
    m_toolActions.clear();

    // Remove the actual tools from the toolbar
    Clear();
}


void ACTION_TOOLBAR::SetToolBitmap( const TOOL_ACTION& aAction, const wxBitmap& aBitmap )
{
    int toolId = aAction.GetUIId();
    wxAuiToolBar::SetToolBitmap( toolId, aBitmap );

    // Set the disabled bitmap: we use the disabled bitmap version
    // of aBitmap.
    wxAuiToolBarItem* tb_item = wxAuiToolBar::FindTool( toolId );

    if( tb_item )
        tb_item->SetDisabledBitmap( MakeDisabledBitmap( aBitmap ) );
}


void ACTION_TOOLBAR::Toggle( const TOOL_ACTION& aAction, bool aState )
{
    int toolId = aAction.GetUIId();

    if( m_toolKinds[ toolId ] )
        ToggleTool( toolId, aState );
    else
        EnableTool( toolId, aState );
}


void ACTION_TOOLBAR::Toggle( const TOOL_ACTION& aAction, bool aEnabled, bool aChecked )
{
    int toolId = aAction.GetUIId();

    EnableTool( toolId, aEnabled );
    ToggleTool( toolId, aEnabled && aChecked );
}


void ACTION_TOOLBAR::onToolEvent( wxAuiToolBarEvent& aEvent )
{
    int            id   = aEvent.GetId();
    wxEventType    type = aEvent.GetEventType();
    OPT_TOOL_EVENT evt;

    bool handled = false;

    if( m_toolManager && type == wxEVT_COMMAND_TOOL_CLICKED  && id >= TOOL_ACTION::GetBaseUIId() )
    {
        const auto actionIt = m_toolActions.find( id );

        // The toolbar item is toggled before the event is sent, so we check for it not being
        // toggled to see if it was toggled originally
        if( m_toolCancellable[id] && !GetToolToggled( id ) )
        {
            // Send a cancel event
            m_toolManager->CancelTool();
            handled = true;
        }
        else if( actionIt != m_toolActions.end() )
        {
            // Dispatch a tool event
            evt = actionIt->second->MakeEvent();
            evt->SetHasPosition( false );
            m_toolManager->ProcessEvent( *evt );
            handled = true;
        }
    }

    // Skip the event if we don't handle it
    if( !handled )
        aEvent.Skip();
}


void ACTION_TOOLBAR::onToolRightClick( wxAuiToolBarEvent& aEvent )
{
    int toolId = aEvent.GetToolId();

    // This means the event was not on a button
    if( toolId == -1 )
        return;

    // Ensure that the ID used maps to a proper tool ID.
    // If right-clicked on a group item, this is needed to get the ID of the currently selected
    // action, since the event's ID is that of the group.
    const auto actionIt = m_toolActions.find( toolId );

    if( actionIt != m_toolActions.end() )
        toolId = actionIt->second->GetUIId();

    // Find the menu for the action
    const auto menuIt = m_toolMenus.find( toolId );

    if( menuIt == m_toolMenus.end() )
        return;

    // Update and show the menu
    std::unique_ptr<ACTION_MENU>& owningMenu = menuIt->second;

    // Get the actual menu pointer to show it
    ACTION_MENU* menu = owningMenu.get();
    SELECTION    dummySel;

    if( CONDITIONAL_MENU* condMenu = dynamic_cast<CONDITIONAL_MENU*>( menu ) )
        condMenu->Evaluate( dummySel );

    menu->UpdateAll();
    PopupMenu( menu );

    // Remove hovered item when the menu closes, otherwise it remains hovered even if the
    // mouse is not on the toolbar
    SetHoverItem( nullptr );
}

// The time (in milliseconds) between pressing the left mouse button and opening the palette
#define PALETTE_OPEN_DELAY 500


void ACTION_TOOLBAR::onMouseClick( wxMouseEvent& aEvent )
{
    wxAuiToolBarItem* item = FindToolByPosition( aEvent.GetX(), aEvent.GetY() );

    if( item )
    {
        // Ensure there is no active palette
        if( m_palette )
        {
            m_palette->Hide();
            m_palette->Destroy();
            m_palette = nullptr;
        }

        // Start the popup conditions if it is a left mouse click and the tool clicked is a group
        if( aEvent.LeftDown() && ( m_actionGroups.find( item->GetId() ) != m_actionGroups.end() ) )
            m_paletteTimer->StartOnce( PALETTE_OPEN_DELAY );

        // Clear the popup conditions if it is a left up, because that implies a click happened
        if( aEvent.LeftUp() )
            m_paletteTimer->Stop();
    }

    // Skip the event so wx can continue processing the mouse event
    aEvent.Skip();
}


void ACTION_TOOLBAR::onItemDrag( wxAuiToolBarEvent& aEvent )
{
    int toolId = aEvent.GetToolId();

    if( m_actionGroups.find( toolId ) != m_actionGroups.end() )
    {
        wxAuiToolBarItem* item = FindTool( toolId );

        // Use call after because opening the palette from a mouse handler
        // creates a weird mouse state that causes problems on OSX.
        CallAfter( &ACTION_TOOLBAR::popupPalette, item );

        // Don't skip this event since we are handling it
        return;
    }

    // Skip since we don't care about it
    aEvent.Skip();
}


void ACTION_TOOLBAR::onTimerDone( wxTimerEvent& aEvent )
{
    // We need to search for the tool using the client coordinates
    wxPoint mousePos = ScreenToClient( wxGetMousePosition() );

    wxAuiToolBarItem* item = FindToolByPosition( mousePos.x, mousePos.y );

    if( item )
        popupPalette( item );
}


void ACTION_TOOLBAR::onPaletteEvent( wxCommandEvent& aEvent )
{
    if( !m_palette )
        return;

    OPT_TOOL_EVENT evt;
    ACTION_GROUP*  group = m_palette->GetGroup();

    // Find the action corresponding to the button press
    auto actionIt = std::find_if( group->GetActions().begin(), group->GetActions().end(),
                                  [=]( const TOOL_ACTION* aAction )
                                  {
                                      return aAction->GetUIId() == aEvent.GetId();
                                  } );

    if( actionIt != group->GetActions().end() )
    {
        const TOOL_ACTION* action = *actionIt;

        // Dispatch a tool event
        evt = action->MakeEvent();
        evt->SetHasPosition( false );
        m_toolManager->ProcessEvent( *evt );

        // Update the main toolbar item with the selected action
        doSelectAction( group, *action );
    }

    // Hide the palette
    m_palette->Hide();
    m_palette->Destroy();
    m_palette = nullptr;
}


void ACTION_TOOLBAR::popupPalette( wxAuiToolBarItem* aItem )
{
    // Clear all popup conditions
    m_paletteTimer->Stop();

    wxWindow* toolParent = dynamic_cast<wxWindow*>( m_toolManager->GetToolHolder() );

    wxASSERT( GetParent() );
    wxASSERT( m_auiManager );
    wxASSERT( toolParent );

    // Ensure the item we are using for the palette has a group associated with it.
    const auto it = m_actionGroups.find( aItem->GetId() );

    if( it == m_actionGroups.end() )
        return;

    ACTION_GROUP* group = it->second;

    wxAuiPaneInfo& pane = m_auiManager->GetPane( this );

    // We use the size of the toolbar items for our palette buttons
    wxRect toolRect = GetToolRect( aItem->GetId() );

    // The position for the palette window must be in screen coordinates
    wxPoint pos( ClientToScreen( toolRect.GetPosition() ) );

    // True for vertical buttons, false for horizontal
    bool    dir        = true;
    size_t  numActions = group->m_actions.size();

    // The size of the palette in the long dimension
    int paletteLongDim =   ( 2 * PALETTE_BORDER )                  // The border on all sides of the buttons
                         + ( BUTTON_BORDER )                       // The border on the start of the buttons
                         + ( numActions * BUTTON_BORDER )          // The other button borders
                         + ( numActions * toolRect.GetHeight() );  // The size of the buttons

    // Determine the position of the top left corner of the palette window
    switch( pane.dock_direction )
    {
        case wxAUI_DOCK_TOP:
            // Top toolbars need to shift the palette window down by the toolbar padding
            dir = true;                                            // Buttons are vertical in the palette
            pos = ClientToScreen( toolRect.GetBottomLeft() );
            pos += wxPoint( -PALETTE_BORDER,                       // Shift left to align the button edges
                            m_bottomPadding );                     // Shift down to move away from the toolbar
            break;

        case wxAUI_DOCK_BOTTOM:
            // Bottom toolbars need to shift the palette window up by its height (all buttons + border + toolbar padding)
            dir = true;                                            // Buttons are vertical in the palette
            pos = ClientToScreen( toolRect.GetTopLeft() );
            pos += wxPoint( -PALETTE_BORDER,                       // Shift left to align the button
                            -( paletteLongDim + m_topPadding ) );  // Shift up by the entire length of the palette
            break;

        case wxAUI_DOCK_LEFT:
            // Left toolbars need to shift the palette window up by the toolbar padding
            dir = false;                                           // Buttons are horizontal in the palette
            pos = ClientToScreen( toolRect.GetTopRight() );
            pos += wxPoint( m_rightPadding,                        // Shift right to move away from the toolbar
                            -( PALETTE_BORDER ) );                 // Shift up to align the button tops
            break;

        case wxAUI_DOCK_RIGHT:
            // Right toolbars need to shift the palette window left by its width (all buttons + border + toolbar padding)
            dir = false;                                           // Buttons are horizontal in the palette
            pos = ClientToScreen( toolRect.GetTopLeft() );
            pos += wxPoint( -( paletteLongDim + m_leftPadding ),   // Shift left by the entire length of the palette
                            -( PALETTE_BORDER  ) );                // Shift up to align the button
            break;
    }

    m_palette = new ACTION_TOOLBAR_PALETTE( GetParent(), dir );

    // We handle the button events in the toolbar class, so connect the right handler
    m_palette->SetGroup( group );
    m_palette->SetButtonSize( toolRect );
    m_palette->Connect( wxEVT_BUTTON, wxCommandEventHandler( ACTION_TOOLBAR::onPaletteEvent ),
                        NULL, this );


    // Add the actions in the group to the palette and update their enabled state
    // We purposely don't check items in the palette
    for( const TOOL_ACTION* action : group->m_actions )
    {
        wxUpdateUIEvent evt( action->GetUIId() );

        toolParent->ProcessWindowEvent( evt );

        m_palette->AddAction( *action );

        if( evt.GetSetEnabled() )
            m_palette->EnableAction( *action, evt.GetEnabled() );
    }

    // Release the mouse to ensure the first click will be recognized in the palette
    ReleaseMouse();

    m_palette->SetPosition( pos );
    m_palette->Popup();

    // Clear the mouse state on the toolbar because otherwise wxWidgets gets confused
    // and won't properly display any highlighted items after the palette is closed.
    // (This is the equivalent of calling the DoResetMouseState() private function)
    RefreshOverflowState();
    SetHoverItem( nullptr );
    SetPressedItem( nullptr );

    m_dragging   = false;
    m_tipItem    = nullptr;
    m_actionPos  = wxPoint( -1, -1 );
    m_actionItem = nullptr;
}


void ACTION_TOOLBAR::OnCustomRender(wxDC& aDc, const wxAuiToolBarItem& aItem,
                                    const wxRect& aRect )
{
    auto it = m_actionGroups.find( aItem.GetId() );

    if( it == m_actionGroups.end() )
        return;

    // Choose the color to draw the triangle
    wxColour clr;

    if( aItem.GetState() & wxAUI_BUTTON_STATE_DISABLED )
        clr = wxSystemSettings::GetColour( wxSYS_COLOUR_GRAYTEXT );
    else
        clr = wxSystemSettings::GetColour( wxSYS_COLOUR_BTNTEXT );

    // Must set both the pen (for the outline) and the brush (for the polygon fill)
    aDc.SetPen( wxPen( clr ) );
    aDc.SetBrush( wxBrush( clr ) );

    // Make the side length of the triangle approximately 1/5th of the bitmap
    int sideLength = KiROUND( aRect.height / 5.0 );

    // This will create a triangle with its point at the bottom right corner,
    // and its other two corners along the right and bottom sides
    wxPoint btmRight = aRect.GetBottomRight();
    wxPoint topCorner( btmRight.x,              btmRight.y - sideLength );
    wxPoint btmCorner( btmRight.x - sideLength, btmRight.y  );

    wxPointList points;
    points.Append( &btmRight );
    points.Append( &topCorner );
    points.Append( &btmCorner );

    aDc.DrawPolygon( &points );
}


bool ACTION_TOOLBAR::KiRealize()
{
    wxClientDC dc( this );
    if( !dc.IsOk() )
        return false;

    // calculate hint sizes for both horizontal and vertical
    // in the order that leaves toolbar in correct final state

    // however, skip calculating alternate orientations if we dont need them due to window style
    bool retval = true;
    if( m_orientation == wxHORIZONTAL )
    {
        if( !( GetWindowStyle() & wxAUI_TB_HORIZONTAL ) )
        {
            m_vertHintSize = GetSize();
            retval         = RealizeHelper( dc, false );
        }

        if( retval && RealizeHelper( dc, true ) )
        {
            m_horzHintSize = GetSize();
        }
        else
        {
            retval = false;
        }
    }
    else
    {
        if( !( GetWindowStyle() & wxAUI_TB_VERTICAL ) )
        {
            m_horzHintSize = GetSize();
            retval         = RealizeHelper( dc, true );
        }

        if( retval && RealizeHelper( dc, false ) )
        {
            m_vertHintSize = GetSize();
        }
        else
        {
            retval = false;
        }
    }

    Refresh( false );
    return retval;
}


void ACTION_TOOLBAR::RefreshBitmaps()
{
    for( const std::pair<int, const TOOL_ACTION*> pair : m_toolActions )
    {
        wxAuiToolBarItem* tool = FindTool( pair.first );

        wxBitmap bmp = KiScaledBitmap( pair.second->GetIcon(), GetParent() );

        tool->SetBitmap( bmp );
        tool->SetDisabledBitmap( MakeDisabledBitmap( bmp ) );
    }

    Refresh();
}
