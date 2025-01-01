/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.TXT for contributors.
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

#ifndef WIDGETS_TAB_TRAVERSAL__H
#define WIDGETS_TAB_TRAVERSAL__H

/**
 * @file
 * Functions for manipulating tab traversal in forms and dialogs.
 */

#include <wx/window.h>

#include <vector>

namespace KIUI
{

/**
 * Set a list of controls to have a defined sequential tab order.
 *
 * Each control in the list will come after the previous one. The first control will
 * keep its current position. The end result will be that the given control
 * will be sequential when tabbed though.
 *
 * This can be slightly clearer than manually calling MoveAfterInTabOrder
 * on each control in turn.
 *
 * @param aControlsInTabOrder list of controls (wxWindows) in desired tab order
 */
void SetControlsTabOrder( const std::vector<wxWindow*>& aControlsInTabOrder );

} // namespace KIUI

#endif // WIDGETS_TAB_TRAVERSAL__H