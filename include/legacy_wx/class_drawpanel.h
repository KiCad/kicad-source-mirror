/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2004 Jean-Pierre Charras, jaen-pierre.charras@gipsa-lab.inpg.com
 * Copyright (C) 2011 Wayne Stambaugh <stambaughw@gmail.com>
 * Copyright (C) 1992-2018 KiCad Developers, see AUTHORS.txt for contributors.
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

#ifndef  PANEL_WXSTRUCT_H
#define  PANEL_WXSTRUCT_H

#include <base_struct.h>
#include <gr_basic.h>
#include <eda_rect.h>


class BASE_SCREEN;
class PCB_SCREEN;


class EDA_DRAW_PANEL : public wxScrolledWindow
{
public:
    EDA_DRAW_PANEL( EDA_DRAW_FRAME* parent, int id, const wxPoint& pos, const wxSize& size );
    ~EDA_DRAW_PANEL();

    BASE_SCREEN* GetScreen();

    EDA_DRAW_FRAME* GetParent() const;

    // Only used for printing, so no clipping
    virtual EDA_RECT* GetClipBox() { return nullptr; }

    /// @copydoc wxWindow::Refresh()
    void Refresh( bool eraseBackground = true, const wxRect* rect = NULL ) override;
};


#endif  /* #ifndef PANEL_WXSTRUCT_H */
