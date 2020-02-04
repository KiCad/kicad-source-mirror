/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2020 CERN
 * @author Maciej Suminski <maciej.suminski@cern.ch>
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
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef PROPERTIES_TOOL_H
#define PROPERTIES_TOOL_H

#include <tools/pcb_tool_base.h>

/**
 * Action handler for the Properties panel
 */
class PROPERTIES_TOOL : public PCB_TOOL_BASE
{
public:
    PROPERTIES_TOOL()
        : PCB_TOOL_BASE( "pcbnew.Properties" )
    {
    }

    int UpdateProperties( const TOOL_EVENT& aEvent );

    void setTransitions() override;
};

#endif
