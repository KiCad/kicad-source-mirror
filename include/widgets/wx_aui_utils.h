/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */


#ifndef KICAD_WX_AUI_UTILS_H
#define KICAD_WX_AUI_UTILS_H

class wxAuiMangager;
class wxAuiPaneInfo;

/**
 * Sets the size of an AUI pane, working around http://trac.wxwidgets.org/ticket/13180
 * @param aManager is an AUI manager
 * @param aPane is a wxAuiPaneInfo containing pane info managed by aManager
 * @param aWidth is the width to set (-1 for automatic)
 * @param aHeight is the height to set (-1 for automatic)
 */
void SetAuiPaneSize( wxAuiManager& aManager, wxAuiPaneInfo& aPane, int aWidth, int aHeight );

#endif // KICAD_WX_AUI_UTILS_H
