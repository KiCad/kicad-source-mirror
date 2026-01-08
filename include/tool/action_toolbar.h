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

#pragma once

#include <map>
#include <memory>
#include <vector>
#include <wx/bitmap.h>          // Needed for the auibar include
#include <wx/aui/auibar.h>
#include <wx/aui/framemanager.h>
#include <wx/popupwin.h>
#include <wx/panel.h>
#include <tool/action_manager.h>
#include <frame_type.h>

class ACTION_MENU;
class BITMAP_BUTTON;
class EDA_BASE_FRAME;
class TOOL_ACTION;
class TOOL_MANAGER;
class TOOLBAR_CONFIGURATION;

/**
 * A group of actions that will be displayed together on a toolbar palette.
 */
class ACTION_GROUP
{
public:
    // Make the toolbar a friend so it can easily access everything inside here
    friend class ACTION_TOOLBAR;

    ACTION_GROUP( const std::string_view& aName );

    ACTION_GROUP( const std::string_view& aName, const std::vector<const TOOL_ACTION*>& aActions );

    /**
     * Set the default action to use when first creating the toolbar palette icon.
     *
     * If no default action is provided, the default will be the first action in the
     * vector.
     *
     * @param aDefault is the default action.
     */
    void SetDefaultAction( const TOOL_ACTION& aDefault );

    /**
     * Get the default action to use when first creating this group's toolbar palette icon.
     */
    const TOOL_ACTION* GetDefaultAction() const { return m_defaultAction; }

    /**
     * Get the name of the group.
     */
    std::string GetName() const { return m_name; }

    /**
     * Get the ID used in the UI to reference this group
     */
    int GetUIId() const;

    /**
     * Set the actions contained in this group.
     *
     * The first action in the list will be the new default action.
     *
     * @param aActions is the new set of actions.
     */
    void SetActions( const std::vector<const TOOL_ACTION*>& aActions );

    /**
     * Get a vector of all the actions contained inside this group.
     */
    const std::vector<const TOOL_ACTION*>& GetActions() const { return m_actions; }

protected:
    ///< The action ID for this action group
    int m_id;

    ///< The name of this action group
    std::string m_name;

    ///< The default action to display on the toolbar item
    const TOOL_ACTION* m_defaultAction;

    ///< The actions that compose the group.  Non-owning.
    std::vector<const TOOL_ACTION*> m_actions;
};


/**
 * A popup window that contains a row of toolbar-like buttons for the user to choose from.
 */
class ACTION_TOOLBAR_PALETTE : public wxPopupTransientWindow
{
public:
    /**
     * Create the palette.
     *
     * @param aParent is the parent window
     * @param aVertical is true if the palette should make the buttons a vertical line,
     *                  false for a horizontal line.
     */
    ACTION_TOOLBAR_PALETTE( wxWindow* aParent, bool aVertical );

    /**
     * Add an action to the palette.
     *
     * @param aAction is the action to add
     */
    void AddAction( const TOOL_ACTION& aAction );

    /**
     * Enable the button for an action on the palette.
     *
     * @param aAction is the action who's button should be enabled
     * @param aEnable is true to enable the button, false to disable
     */
    void EnableAction( const TOOL_ACTION& aAction, bool aEnable = true );

    /**
     * Check/Toggle the button for an action on the palette.
     *
     * @param aAction is the action who's button should be checked
     * @param aCheck is true to check the button, false to uncheck
     */
    void CheckAction( const TOOL_ACTION& aAction, bool aCheck = true );

    /**
     * Set the size all the buttons on this palette should be.
     * This function will automatically pad all button bitmaps to ensure this
     * size is met.
     *
     * @param aSize is the requested size of the buttons
     */
    void SetButtonSize( wxRect& aSize ) { m_buttonSize = aSize; }

    /**
     * Popup this window
     *
     * @param aFocus is the window to keep focus on (if supported)
     */
    void Popup( wxWindow* aFocus = nullptr ) override;

    /**
     * Set the action group that this palette contains the actions for
     */
    void SetGroup( ACTION_GROUP* aGroup ) { m_group = aGroup; }
    ACTION_GROUP* GetGroup() { return m_group; }

protected:
    void onCharHook( wxKeyEvent& aEvent );

    // The group that the buttons in the palette are part of
     ACTION_GROUP* m_group;

    ///< The size each button on the toolbar should be
    wxRect         m_buttonSize;

    ///< True if the palette uses vertical buttons, false for horizontal buttons
    bool           m_isVertical;

    wxPanel*       m_panel;
    wxBoxSizer*    m_mainSizer;
    wxBoxSizer*    m_buttonSizer;

    ///< The buttons that act as the toolbar on the palette
    std::map<int, BITMAP_BUTTON*> m_buttons;
};

// Forward declare this because the toolbar wants it
class ACTION_TOOLBAR_CONTROL;

/**
 * Define the structure of a toolbar with buttons that invoke ACTIONs.
 */
class ACTION_TOOLBAR : public wxAuiToolBar
{
public:
    ACTION_TOOLBAR( EDA_BASE_FRAME* parent, wxWindowID id = wxID_ANY,
                    const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxDefaultSize,
                    long style = wxAUI_TB_DEFAULT_STYLE );

    virtual ~ACTION_TOOLBAR();

    /**
     * Set the AUI manager that this toolbar belongs to.
     *
     * @param aManager is the AUI manager
     */
    void SetAuiManager( wxAuiManager* aManager ) { m_auiManager = aManager; }

    void SetToolManager( TOOL_MANAGER* aManager ) { m_toolManager = aManager; }

    /**
     * Add a TOOL_ACTION-based button to the toolbar.
     *
     * The toggle/cancel attributes are set using the attributes in the action.
     *
     * After selecting the entry, a #TOOL_EVENT command containing name of the action is sent.
     *
     * @param aAction is the action to add.
     */
    void Add( const TOOL_ACTION& aAction );

    /**
     * Add a TOOL_ACTION-based button to the toolbar.
     *
     * After selecting the entry, a #TOOL_EVENT command containing name of the action is sent.
     *
     * @param aAction is the action to add.
     * @param aIsToggleEntry makes the toolbar item a toggle entry when true.
     * @param aIsCancellable when true, cancels the tool if clicked when tool is active.
     */
    void Add( const TOOL_ACTION& aAction, bool aIsToggleEntry,
              bool aIsCancellable = false );

    /**
     * Add a large button such as used in the KiCad Manager Frame's launch bar.
     *
     * @param aAction
     */
    void AddButton( const TOOL_ACTION& aAction );

    /**
     * Add a separator that introduces space on either side to not squash the tools
     * when scaled.
     *
     * @param aWindow is the window to get the scaling factor of
     */
    void AddScaledSeparator( wxWindow* aWindow );

    /**
     * Add a control to the toolbar.
     */
    void Add( wxControl* aControl, const wxString& aLabel = wxEmptyString );

    /**
     * Add a context menu to a specific tool item on the toolbar.
     *
     * This toolbar gets ownership of the menu object, and will delete it when the
     * ClearToolbar() function is called.
     *
     * @param aAction is the action to get the menu
     * @param aMenu is the context menu
     */
    void AddToolContextMenu( const TOOL_ACTION& aAction, std::unique_ptr<ACTION_MENU> aMenu );

    /**
     * Add a set of actions to a toolbar as a group. One action from the group will be displayed
     * at a time.
     *
     * This toolbar gets ownership of the group object, and will delete it when the
     * ClearToolbar() function is called.
     *
     * @param aGroup is the group to add. The first action in the group will be the first shown
     *               on the toolbar.
     */
    void AddGroup( std::unique_ptr<ACTION_GROUP> aGroup );

    /**
     * Select an action inside a group
     *
     * @param aGroup is the group that contains the action
     * @param aAction is the action inside the group
     */
    void SelectAction( ACTION_GROUP* aGroup, const TOOL_ACTION& aAction );

    /**
     * Select the given action in whatever group contains it and update that group's icon.
     * If the action is not part of any group on this toolbar, this is a no-op.
     */
    void SelectAction( const TOOL_ACTION& aAction );

    /**
     * Replace the contents of this toolbar with the configuration given in
     * @c aConfig.
     *
     * @param aConfig is the configuration to apply to the toolbar
     */
    void ApplyConfiguration( const TOOLBAR_CONFIGURATION& aConfig );

    /**
     * Update the width of all wxControl tools on thsi toolbar
     */
    void UpdateControlWidths();

    /**
     * Update the toolbar item width of a control using its best size.
     *
     * @param aID is the ID of the toolbar item to update the width for
     */
    void UpdateControlWidth( int aID );

    /**
     * Clear the toolbar and remove all associated menus.
     */
    void ClearToolbar();

    /**
     * Updates the bitmap of a particular tool.
     *
     * Not icon-based because we use it for the custom-drawn layer pair bitmap.
     */
    void SetToolBitmap( const TOOL_ACTION& aAction, const wxBitmapBundle& aBitmap );

    /**
     * Apply the default toggle action.
     *
     * For checked items this is check/uncheck; for non-checked items it's enable/disable.
     */
    void Toggle( const TOOL_ACTION& aAction, bool aState );

    void Toggle( const TOOL_ACTION& aAction, bool aEnabled, bool aChecked );

    /**
     * Use this over Realize() to avoid a rendering glitch with fixed orientation toolbars
     *
     * The standard Realize() draws both horizontal and vertical to determine sizing
     * However with many icons, potato PCs, etc, you can actually see that double draw
     * This custom function avoids the double draw if the HORIZONTAL or VERTICAL toolbar
     * properties are set.
     */
    bool KiRealize();

    /**
     * Reload all the bitmaps for the tools (e.g. when switching icon themes)
     */
    void RefreshBitmaps();

    /**
     * Get the list of custom controls that could be used on toolbars.
     */
    static std::list<ACTION_TOOLBAR_CONTROL*>& GetAllCustomControls()
    {
        static std::list<ACTION_TOOLBAR_CONTROL*> m_controls;
        return m_controls;
    }

    /**
     * Get the list of custom controls that could be used on a particular frame type.
     */
    static std::list<ACTION_TOOLBAR_CONTROL*> GetCustomControlList( FRAME_T aContext );

    static constexpr bool TOGGLE = true;
    static constexpr bool CANCEL = true;

protected:
    /**
     * Update a group toolbar item to look like a specific action.
     *
     * Note: This function does not verify that the action is inside the group.
     */
    void doSelectAction( ACTION_GROUP* aGroup, const TOOL_ACTION& aAction );

    /**
     * Popup the #ACTION_TOOLBAR_PALETTE associated with the ACTION_GROUP of the
     * given toolbar item.
     */
    void popupPalette( wxAuiToolBarItem* aItem );

    ///< Handler for a mouse up/down event
    void onMouseClick( wxMouseEvent& aEvent );

    ///< Handler for when a drag event occurs on an item
    void onItemDrag( wxAuiToolBarEvent& aEvent );

    ///< The default tool event handler
    void onToolEvent( wxAuiToolBarEvent& aEvent );

    ///< Handle a right-click on a menu item
    void onRightClick( wxAuiToolBarEvent& aEvent );

    ///< Handle the button select inside the palette
    void onPaletteEvent( wxCommandEvent& aEvent );

    ///< Handle the palette timer triggering
    void onTimerDone( wxTimerEvent& aEvent );

    void onThemeChanged( wxSysColourChangedEvent &aEvent );

    ///< Render the triangle in the lower-right corner that represents that an action palette
    ///< is available for an item
    void OnCustomRender( wxDC& aDc, const wxAuiToolBarItem& aItem, const wxRect& aRect ) override;

    void DoSetToolTipText( const wxString& aTip ) override;

protected:
    // Timer used to determine when the palette should be opened after a group item is pressed
    EDA_BASE_FRAME*         m_parent;
    wxTimer*                m_paletteTimer;

    wxAuiManager*           m_auiManager;
    TOOL_MANAGER*           m_toolManager;
    ACTION_TOOLBAR_PALETTE* m_palette;

    std::map<int, bool>                m_toolKinds;
    std::map<int, bool>                m_toolCancellable;
    std::map<int, const TOOL_ACTION*>  m_toolActions;

    /// IDs for all the control items in this toolbar
    std::vector<int> m_controlIDs;

    std::map<int, std::unique_ptr<ACTION_GROUP>> m_actionGroups;
    std::map<int, std::unique_ptr<ACTION_MENU>>  m_toolMenus;
};

/**
 * Type for the function signature that is used to add custom controls to the toolbar.
 *
 * Note, these functions SHOULD NOT use the wxWidgets-provided `AddControl` function to
 * add the controls to the toolbar, instead they should use the `ACTION_TOOLBAR::Add` functions
 * to ensure proper registration of the control.
 */
typedef std::function<void ( ACTION_TOOLBAR* )> ACTION_TOOLBAR_CONTROL_FACTORY;


/**
 * Class to hold basic information about controls that can be added to the toolbars.
 */
class ACTION_TOOLBAR_CONTROL
{
public:
    ACTION_TOOLBAR_CONTROL( const std::string& aName, const wxString& aUiName,
                            const wxString& aDescription, std::vector<FRAME_T> aSupportedContexts ) :
        m_name( aName ),
        m_uiname( aUiName ),
        m_description( aDescription ),
        m_supportedContexts( aSupportedContexts )
    {
        wxASSERT_MSG( aName.starts_with( "control" ),
                      wxString::Format( "Control name \"%s\" must start with \"control\"", aName ) );

        ACTION_TOOLBAR::GetAllCustomControls().push_back( this );
    }

    const std::string& GetName() const { return m_name; }
    const wxString& GetUiName() const { return m_uiname; }
    const wxString& GetDescription() const { return m_description; }

    bool SupportedFor( FRAME_T aFrame ) const
    {
        for( FRAME_T candidate : m_supportedContexts )
        {
            if( aFrame == candidate )
                return true;
        }

        return false;
    }

protected:
    /**
     * Name of the control - must start with "control."
     */
    std::string m_name;

    /**
     * Short description to show for the control
     */
    wxString m_uiname;

    /**
     * User-visible tooltip for the control
     */
    wxString m_description;

    /**
     * List of frame types that support the control.
     */
    std::vector<FRAME_T> m_supportedContexts;
};

class ACTION_TOOLBAR_CONTROLS
{
public:
    static ACTION_TOOLBAR_CONTROL gridSelect;
    static ACTION_TOOLBAR_CONTROL zoomSelect;
    static ACTION_TOOLBAR_CONTROL ipcScripting;
    static ACTION_TOOLBAR_CONTROL unitSelector;
    static ACTION_TOOLBAR_CONTROL bodyStyleSelector;
    static ACTION_TOOLBAR_CONTROL layerSelector;
    static ACTION_TOOLBAR_CONTROL overrideLocks;
};
