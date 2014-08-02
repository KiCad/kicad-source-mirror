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

/**
 * TODO description
 */

class PLACEMENT_TOOL : public TOOL_INTERACTIVE
{
public:
    PLACEMENT_TOOL();
    virtual ~PLACEMENT_TOOL();

    /// @copydoc TOOL_INTERACTIVE::Reset()
    void Reset( RESET_REASON aReason ) {};

    /// @copydoc TOOL_INTERACTIVE::Init()
    bool Init();

    /// TODO
    int AlignTop( TOOL_EVENT& aEvent );

    /// TODO
    int AlignBottom( TOOL_EVENT& aEvent );

    /// TODO
    int AlignLeft( TOOL_EVENT& aEvent );

    /// TODO
    int AlignRight( TOOL_EVENT& aEvent );

    /// TODO
    int DistributeHorizontally( TOOL_EVENT& aEvent );

    /// TODO
    int DistributeVertically( TOOL_EVENT& aEvent );

private:
    /// TODO
    void setTransitions();

    SELECTION_TOOL* m_selectionTool;
};

#endif /* PLACEMENT_TOOL_H_ */
