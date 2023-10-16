/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2004-2020 KiCad Developers.
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

#ifndef MENUS_HELPERS_H_
#define MENUS_HELPERS_H_

/**
 * @file menus_helpers.h
 * @brief Macros and inline functions to create menus items in menubars or popup menus.
 */


#include <wx/menu.h>
#include <wx/menuitem.h>

class ACTION_MENU;
class TOOL_INTERACTIVE;

void AddMenuLanguageList( ACTION_MENU* aMasterMenu, TOOL_INTERACTIVE* aControlTool );

#endif // MENUS_HELPERS_H_
