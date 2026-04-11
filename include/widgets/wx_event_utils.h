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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#pragma once

#include <wx/evtloop.h>

/**
 * Drain pending events of the given categories from the active event loop.
 *
 * During long-running operations that call progress-dialog Update() frequently,
 * wxWidgets can accumulate a backlog of timer events that makes the UI sluggish
 * after the operation completes.  Calling this after each Update() prevents the
 * backlog from building up.
 *
 * See https://github.com/wxWidgets/wxWidgets/issues/26192
 *
 * @param aCategories  Event categories to yield for (default wxEVT_CATEGORY_TIMER).
 */
inline void DrainPendingEvents( long aCategories = wxEVT_CATEGORY_TIMER )
{
    if( wxEventLoopBase* loop = wxEventLoopBase::GetActive() )
        loop->YieldFor( aCategories );
}
