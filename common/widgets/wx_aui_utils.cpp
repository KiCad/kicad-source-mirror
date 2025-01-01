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

#include <wx/aui/aui.h>
#include <widgets/wx_aui_utils.h>


void SetAuiPaneSize( wxAuiManager& aManager, wxAuiPaneInfo& aPane, int aWidth, int aHeight )
{
    wxSize minSize = aPane.min_size;

    // wxAUI hack: force width by setting MinSize() and then Fixed()
    // thanks to ZenJu http://trac.wxwidgets.org/ticket/13180
    aPane.MinSize( aWidth, aHeight );
    aPane.Fixed();
    aManager.Update();

    // now make it resizable again
    aPane.Resizable();
    aManager.Update();

    // Note: DO NOT call m_auimgr.Update() anywhere after this; it will nuke the size
    // back to minimum.
    aPane.MinSize( minSize.x, minSize.y );
}
