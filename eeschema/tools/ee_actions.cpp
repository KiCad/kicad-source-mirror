/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2019 KiCad Developers, see AUTHORS.txt for contributors.
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

#include <common.h>
#include <eeschema_id.h>
#include <tools/ee_actions.h>


char g_lastBusEntryShape = '/';


OPT<TOOL_EVENT> EE_ACTIONS::TranslateLegacyId( int aId )
{
    switch( aId )
    {
    case ID_CANCEL_CURRENT_COMMAND:
        return ACTIONS::cancelInteractive.MakeEvent();

    case ID_SIM_PROBE:
        return EE_ACTIONS::simProbe.MakeEvent();

    case ID_SIM_TUNE:
        return EE_ACTIONS::simTune.MakeEvent();

    case ID_SCH_UNFOLD_BUS:
        return EE_ACTIONS::unfoldBus.MakeEvent();

    case ID_MOUSE_CLICK:
        return ACTIONS::cursorClick.MakeEvent();

    case ID_MOUSE_DOUBLECLICK:
        return ACTIONS::cursorDblClick.MakeEvent();
    }

    return OPT<TOOL_EVENT>();
}
