/*
 * This program source code file is part of KiCad, a free EDA CAD application.

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

#include <tools/tool_event_utils.h>
#include <tools/pcb_actions.h>
#include <pcb_base_edit_frame.h>


bool TOOL_EVT_UTILS::IsRotateToolEvt( const TOOL_EVENT& aEvt )
{
    return aEvt.IsAction( &PCB_ACTIONS::rotateCw )
            || aEvt.IsAction( &PCB_ACTIONS::rotateCcw );
}


EDA_ANGLE TOOL_EVT_UTILS::GetEventRotationAngle( const PCB_BASE_EDIT_FRAME& aFrame,
                                                 const TOOL_EVENT& aEvent )
{
    wxASSERT_MSG( IsRotateToolEvt( aEvent ), wxT( "Expected rotation event" ) );

    EDA_ANGLE rotAngle = aFrame.GetRotationAngle();
    const int angleMultiplier = aEvent.Parameter<int>();

    wxASSERT_MSG( angleMultiplier == 1 || angleMultiplier == -1, "Expected 1 or -1" );

    return angleMultiplier > 0 ? rotAngle : -rotAngle;
}
