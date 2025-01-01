/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2012-2016 Jean-Pierre Charras, jp.charras at wanadoo.fr
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

#pragma once

// List of predefined zooms used in zoom in/out from hotkeys, context menu and toolbar
// Zooming using mouse wheel can have different limits
#define ZOOM_LIST_GERBVIEW 0.022, 0.035, 0.05, 0.08, 0.13, 0.22, 0.35, 0.6, 1.0,\
                           2.2, 3.5, 5.0, 8.0, 13.0, 22.0, 35.0, 50.0, 80.0, 130.0, 220.0

#define ZOOM_LIST_PCBNEW 0.13, 0.22, 0.35, 0.6, 1.0, 1.5, 2.2, 3.5, 5.0, 8.0, 13.0,\
                         20.0, 35.0, 50.0, 80.0, 130.0, 220.0, 300.0

#define ZOOM_LIST_PCBNEW_HYPER 0.6, 1.0, 1.5, 2.2, 3.5, 5.0, 8.0, 13.0, 20.0, 35.0, 50.0,\
                               80.0, 130.0, 220.0, 350.0, 600.0, 900.0, 1500.0

#define ZOOM_LIST_PL_EDITOR 0.022, 0.035, 0.05, 0.08, 0.13, 0.22, 0.35, 0.6, 1.0, 2.2,\
                            3.5, 5.0, 8.0, 13.0, 22.0, 35.0, 50.0, 80.0, 130.0, 220.0

#define ZOOM_LIST_EESCHEMA 0.05, 0.07, 0.1, 0.15, 0.2, 0.3, 0.5, 0.7, 1.0, 1.5, 2.0,\
                           3.0, 4.5, 6.5, 10.0, 15.0, 20.0, 30.0, 45.0, 65.0, 100.0

// Zoom scale limits for zoom (especially mouse wheel)
// the limits can differ from zoom list because the zoom list cannot be as long as
// we want because the zoom list is displayed in menus.
// But zoom by mouse wheel is limited mainly by the usability

// Scale limits for zoom  for Eeschema
#define ZOOM_MAX_LIMIT_EESCHEMA 100
#define ZOOM_MIN_LIMIT_EESCHEMA 0.01

#define ZOOM_MAX_LIMIT_EESCHEMA_PREVIEW 20
#define ZOOM_MIN_LIMIT_EESCHEMA_PREVIEW 0.5

// Scale limits for zoom for pl_editor
#define ZOOM_MAX_LIMIT_PLEDITOR 20
#define ZOOM_MIN_LIMIT_PLEDITOR 0.05

// Scale limits for zoom for gerbview
#define ZOOM_MAX_LIMIT_GERBVIEW 5000
#define ZOOM_MIN_LIMIT_GERBVIEW 0.02

// Scale limits for zoom (especially mouse wheel) for Pcbnew
#define ZOOM_MAX_LIMIT_PCBNEW 50000
#define ZOOM_MIN_LIMIT_PCBNEW 0.1
