/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.TXT for contributors.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 3
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, you may find one here:
 * http://www.gnu.org/licenses/gpl-3.0.html
 * or you may search the http://www.gnu.org website for the version 3 license,
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

#pragma once

#include <wx/timer.h>

#include <math/vector2d.h>
#include <tool/tool_interactive.h>
#include <eda_item.h>

class COLLECTOR;
class KIID;


enum class SELECTION_MODE
{
    INSIDE_RECTANGLE,
    TOUCHING_RECTANGLE,
    INSIDE_LASSO,
    TOUCHING_LASSO
};


class SELECTION_TOOL : public TOOL_INTERACTIVE, public wxEvtHandler
{
public:
    SELECTION_TOOL( const std::string& aName );
    ~SELECTION_TOOL(){};

    /**
     * Update a menu's state based on the current selection.  The menu is passed in aEvent's
     * parameter.
     */
    int UpdateMenu( const TOOL_EVENT& aEvent );

    int AddItemToSel( const TOOL_EVENT& aEvent );
    void AddItemToSel( EDA_ITEM* aItem, bool aQuietMode = false );
    int AddItemsToSel( const TOOL_EVENT& aEvent );
    void AddItemsToSel( EDA_ITEMS* aList, bool aQuietMode = false );

    int RemoveItemFromSel( const TOOL_EVENT& aEvent );
    void RemoveItemFromSel( EDA_ITEM* aItem, bool aQuietMode = false );
    int RemoveItemsFromSel( const TOOL_EVENT& aEvent );
    void RemoveItemsFromSel( EDA_ITEMS* aList, bool aQuietMode = false );

    int ReselectItem( const TOOL_EVENT& aEvent );

    /**
     * A safer version of RemoveItemsFromSel( EDA_ITEMS ) which doesn't require the items to
     * still exist.
     */
    void RemoveItemsFromSel( std::vector<KIID>* aList, bool aQuietMode = false );

    void BrightenItem( EDA_ITEM* aItem );
    void UnbrightenItem( EDA_ITEM* aItem );

    /**
     * Show a popup menu to trim the COLLECTOR passed as aEvent's parameter down to a single
     * item.
     *
     * @note This routine **does not** modify the selection.
     */
    int SelectionMenu( const TOOL_EVENT& aEvent );

    SELECTION& GetSelection() { return selection(); }

    virtual void EnterGroup() {}
    virtual void ExitGroup( bool aSelectGroup = false ) {}

protected:
    /**
     * Return a reference to the selection.
     */
    virtual SELECTION& selection() = 0;

    /**
     * Start the process to show our disambiguation menu once the user has kept the mouse down
     * for the minimum time.
     * @param aEvent
     */
   void onDisambiguationExpire( wxTimerEvent& aEvent );

    /**
     * Take necessary action mark an item as selected.
     */
    virtual void select( EDA_ITEM* aItem ) = 0;

    /**
     * Take necessary action mark an item as unselected.
     */
    virtual void unselect( EDA_ITEM* aItem ) = 0;

    /**
     * Highlight the item visually.
     *
     * @param aHighlightMode should be either SELECTED or BRIGHTENED
     * @param aGroup is the group to add the item to in the BRIGHTENED mode.
     */
    virtual void highlight( EDA_ITEM* aItem, int aHighlightMode, SELECTION* aGroup = nullptr ) = 0;

    /**
     * Unhighlight the item visually.
     *
     * @param aHighlightMode should be either SELECTED or BRIGHTENED
     * @param aGroup is the group to remove the item from.
     */
    virtual void unhighlight( EDA_ITEM* aItem, int aHighlightMode,
                              SELECTION* aGroup = nullptr ) = 0;

    /**
     * Set the configuration of m_additive, m_subtractive, m_exclusive_or, m_skip_heuristics
     * from the state of modifier keys SHIFT, CTRL, ALT and depending on the OS
     */
    void setModifiersState( bool aShiftState, bool aCtrlState, bool aAltState );

    /**
     * True if a selection modifier is enabled, false otherwise.
     */
    bool hasModifier();

    /**
     * Determine if ctrl-click is highlight net or XOR selection.
     */
    virtual bool ctrlClickHighlights() { return false; }

    bool doSelectionMenu( COLLECTOR* aCollector );

protected:
    bool            m_additive;          ///< Items should be added to sel (instead of replacing).
    bool            m_subtractive;       ///< Items should be removed from selection.
    bool            m_exclusive_or;      ///< Items' selection state should be toggled.
    bool            m_multiple;          ///< Multiple selection mode is active.

    /// Show disambiguation menu for all items under the cursor rather than trying to narrow
    /// them down first using heuristics.
    bool            m_skip_heuristics;
    bool            m_highlight_modifier;///< Select highlight net on left click.
    bool            m_drag_additive;     ///< Add multiple items to selection.
    bool            m_drag_subtractive;  ///< Remove multiple from selection.

    bool            m_canceledMenu;      ///< Sets to true if the disambiguation menu was canceled.

    wxTimer         m_disambiguateTimer; ///< Timer to show the disambiguate menu.

    VECTOR2I        m_originalCursor;    ///< Location of original cursor when starting click.
};
