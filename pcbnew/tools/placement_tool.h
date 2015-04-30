/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2014 CERN
 * @author Maciej Suminski <maciej.suminski@cern.ch>
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

#ifndef PLACEMENT_TOOL_H_
#define PLACEMENT_TOOL_H_

#include <tool/tool_interactive.h>

class SELECTION_TOOL;

class PLACEMENT_TOOL : public TOOL_INTERACTIVE
{
public:
    PLACEMENT_TOOL();
    virtual ~PLACEMENT_TOOL();

    /// @copydoc TOOL_INTERACTIVE::Reset()
    void Reset( RESET_REASON aReason ) {};

    /// @copydoc TOOL_INTERACTIVE::Init()
    bool Init();

    /**
     * Function AlignTop()
     * Sets Y coordinate of the selected items to the value of the top-most selected item Y coordinate.
     */
    int AlignTop( const TOOL_EVENT& aEvent );

    /**
     * Function AlignBottom()
     * Sets Y coordinate of the selected items to the value of the bottom-most selected item Y coordinate.
     */
    int AlignBottom( const TOOL_EVENT& aEvent );

    /**
     * Function AlignLeft()
     * Sets X coordinate of the selected items to the value of the left-most selected item X coordinate.
     */
    int AlignLeft( const TOOL_EVENT& aEvent );

    /**
     * Function AlignRight()
     * Sets X coordinate of the selected items to the value of the right-most selected item X coordinate.
     */
    int AlignRight( const TOOL_EVENT& aEvent );

    /**
     * Function DistributeHorizontally()
     * Distributes the selected items along the X axis.
     */
    int DistributeHorizontally( const TOOL_EVENT& aEvent );

    /**
     * Function DistributeVertically()
     * Distributes the selected items along the Y axis.
     */
    int DistributeVertically( const TOOL_EVENT& aEvent );

    ///> Sets up handlers for various events.
    void SetTransitions();

private:
    SELECTION_TOOL* m_selectionTool;
};

#endif /* PLACEMENT_TOOL_H_ */
