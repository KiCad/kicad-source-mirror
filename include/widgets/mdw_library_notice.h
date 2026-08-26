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

#ifndef MDW_LIBRARY_NOTICE_H
#define MDW_LIBRARY_NOTICE_H

#include <mdw/client/mdw_subscription.h>

class EDA_BASE_FRAME;
class PROJECT;

/**
 * Show in aFrame's infobar what aProject's team libraries still need, such as turning the workspace preference
 * on, setting up a source or fetching a copy. Clears the notice once nothing is needed.
 */
void ShowTeamLibraryNotice( EDA_BASE_FRAME* aFrame, PROJECT& aProject );

/**
 * While the subscription lives, show "Library updates available" in aFrame's infobar, with Get Library Updates,
 * whenever a backend announces a change to a team library the frame's project uses. Nothing is updated until the
 * user asks.
 */
[[nodiscard]] MDW_SUBSCRIPTION WatchTeamLibraryUpdates( EDA_BASE_FRAME* aFrame );

#endif // MDW_LIBRARY_NOTICE_H
