/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2018 KiCad Developers, see AUTHORS.txt for contributors.
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


#ifndef EDA_DOCKART_H
#define EDA_DOCKART_H

#include <wx/aui/aui.h>
#include <wx/aui/dockart.h>
#include <wx/aui/framemanager.h>

class EDA_DRAW_FRAME;


class EDA_DOCKART : public wxAuiDefaultDockArt
{
private:
    EDA_DRAW_FRAME* m_frame;

public:
    EDA_DOCKART( EDA_DRAW_FRAME* aParent ) :
        m_frame( aParent )
    {
        SetMetric( wxAUI_DOCKART_PANE_BORDER_SIZE, 1 );
    }

    /**
     * Draw borders for Kicad AUI panes.
     *
     * The principal specialization over wxWidgets' default borders is the absence of a
     * white single-pixel frame (which looks particularly poor on canvasses with dark
     * backgrounds).
     */
    void DrawBorder( wxDC& aDC, wxWindow* aWindow, const wxRect& r,
                     wxAuiPaneInfo& aPane ) override;
};


#endif //EDA_DOCKART_H
