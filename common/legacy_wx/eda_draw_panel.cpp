/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2019 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 2007 Wayne Stambaugh <stambaughw@gmail.com>
 * Copyright (C) 1992-2019 KiCad Developers, see AUTHORS.txt for contributors.
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

#include <fctsys.h>
#include <kiface_i.h>
#include <gr_basic.h>
#include <common.h>
#include <macros.h>
#include <id.h>
#include <class_drawpanel.h>
#include <base_screen.h>
#include <trace_helpers.h>


#ifdef __WXMAC__
const int drawPanelStyle = wxHSCROLL | wxVSCROLL | wxALWAYS_SHOW_SB;
#else
const int drawPanelStyle = wxHSCROLL | wxVSCROLL;
#endif

EDA_DRAW_PANEL::EDA_DRAW_PANEL( EDA_DRAW_FRAME* parent, int id, const wxPoint& pos,
                                const wxSize& size ) :
    wxScrolledWindow( parent, id, pos, size, drawPanelStyle )
{
    wxASSERT( parent );

    ShowScrollbars( wxSHOW_SB_ALWAYS, wxSHOW_SB_ALWAYS );
    DisableKeyboardScrolling();

    SetLayoutDirection( wxLayout_LeftToRight );

    SetBackgroundColour( parent->GetDrawBgColor().ToColour() );
}


EDA_DRAW_PANEL::~EDA_DRAW_PANEL()
{
}


EDA_DRAW_FRAME* EDA_DRAW_PANEL::GetParent() const
{
    wxWindow* mom = wxScrolledWindow::GetParent();
    return (EDA_DRAW_FRAME*) mom;
}


BASE_SCREEN* EDA_DRAW_PANEL::GetScreen()
{
    EDA_DRAW_FRAME* parentFrame = GetParent();

    return parentFrame->GetScreen();
}


void EDA_DRAW_PANEL::Refresh( bool eraseBackground, const wxRect* rect )
{
    GetParent()->GetGalCanvas()->Refresh();
}

