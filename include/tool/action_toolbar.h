/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
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

#ifndef ACTION_TOOLBAR_H
#define ACTION_TOOLBAR_H

#include <map>
#include <wx/bitmap.h>          // Needed for the auibar include
#include <wx/aui/auibar.h>
#include <tool/tool_event.h>

class EDA_DRAW_FRAME;
class TOOL_MANAGER;
class TOOL_ACTION;

/**
 * Class ACTION_TOOLBAR
 *
 * Defines the structure of a toolbar with buttons that invoke ACTIONs.
 */
class ACTION_TOOLBAR : public wxAuiToolBar
{
public:
    ACTION_TOOLBAR( EDA_BASE_FRAME* parent, wxWindowID id = wxID_ANY,
                    const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxDefaultSize,
                    long style = wxAUI_TB_DEFAULT_STYLE );

    virtual ~ACTION_TOOLBAR() {}

    /**
     * Function Add()
     * Adds a TOOL_ACTION-based button to the toolbar. After selecting the entry,
     * a TOOL_EVENT command containing name of the action is sent.
     */
    void Add( const TOOL_ACTION& aAction, bool aIsToggleEntry = false );

    /**
     * Function AddButton()
     * Adds a large button such as used in the Kicad Manager Frame's launch bar.
     * @param aAction
     */
    void AddButton( const TOOL_ACTION& aAction );

    /**
     * Function SetToolBitmap()
     * Updates the bitmap of a particular tool.  Not icon-based because we use it
     * for the custom-drawn layer pair bitmap.
     */
    void SetToolBitmap( const TOOL_ACTION& aAction, const wxBitmap& aBitmap );

    /**
     * Applies the default toggle action.  For checked items this is check/uncheck; for
     * non-checked items it's enable/disable.
     */
    void Toggle( const TOOL_ACTION& aAction, bool aState );

    void Toggle( const TOOL_ACTION& aAction, bool aEnabled, bool aChecked );

    static const bool TOGGLE = true;

protected:
    ///> The default tool event handler.
    void onToolEvent( wxAuiToolBarEvent& aEvent );

protected:
    ///> Tool items with ID higher than that are considered TOOL_ACTIONs
    static const int ACTION_ID = 10000;

    TOOL_MANAGER* m_toolManager;
    std::map<int, bool>               m_toolKinds;
    std::map<int, const TOOL_ACTION*> m_toolActions;
};

#endif
