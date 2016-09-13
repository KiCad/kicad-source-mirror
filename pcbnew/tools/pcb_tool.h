/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2016 CERN
 * @author Tomasz Wlostowski <tomasz.wlostowski@cern.ch>
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

#ifndef __PCB_TOOL_H
#define __PCB_TOOL_H

#include <string>

#include <tool/tool_event.h>
#include <tool/tool_interactive.h>
#include <wxPcbStruct.h>
#include <class_board.h>

/**
 * Class PCB_TOOL
 *
 * A tool operating on a BOARD object
**/

class PCB_TOOL : public TOOL_INTERACTIVE
{
public:
    /**
     * Constructor
     *
     * Creates a tool with given id & name. The name must be unique. */
    PCB_TOOL( TOOL_ID aId, const std::string& aName ) :
        TOOL_INTERACTIVE ( aId, aName ),
        m_editModules( false ) {};

    /**
     * Constructor
     *
     * Creates a tool with given name. The name must be unique. */
    PCB_TOOL( const std::string& aName ) :
        TOOL_INTERACTIVE ( aName ),
        m_editModules( false ) {};

    virtual ~PCB_TOOL() {};

    /**
     * Function SetEditModules()
     *
     * Toggles edit module mode. When enabled, one may select parts of modules individually
     * (graphics, pads, etc.), so they can be modified.
     * @param aEnabled decides if the mode should be enabled.
     */
    void SetEditModules( bool aEnabled )
    {
        m_editModules = aEnabled;
    }

    bool EditingModules() const
    {
        return m_editModules;
    }

protected:
    KIGFX::VIEW* view() const { return getView(); }
    PCB_EDIT_FRAME* frame() const { return getEditFrame<PCB_EDIT_FRAME>(); }
    BOARD* board() const { return getModel<BOARD>(); }

    bool m_editModules;
};

#endif
