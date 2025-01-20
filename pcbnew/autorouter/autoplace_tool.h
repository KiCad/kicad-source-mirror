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

#ifndef TOOLS_AUTOPLACE_TOOL_H
#define TOOLS_AUTOPLACE_TOOL_H

#include <tools/pcb_tool_base.h>


/**
 * Tool responsible for automagic placement of components.
 */
class AUTOPLACE_TOOL : public PCB_TOOL_BASE
{
public:
    AUTOPLACE_TOOL();
    ~AUTOPLACE_TOOL();

    ///< Bind handlers to corresponding TOOL_ACTIONs.
    void setTransitions() override;

private:
    int autoplace( std::vector<FOOTPRINT*>& aFootprints );

    int autoplaceSelected( const TOOL_EVENT& aEvent );
    int autoplaceOffboard( const TOOL_EVENT& aEvent );
};


#endif // TOOLS_AUTOPLACE_TOOL_H
