/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2015 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 2015 KiCad Developers, see CHANGELOG.TXT for contributors.
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

/* Set up color Layers for Eeschema
 */

#include <fctsys.h>
#include <draw_frame.h>
#include <class_drawpanel.h>

#include <general.h>

#include <dialog_color_config.h>

#define ID_COLOR_SETUP  1800

DIALOG_COLOR_CONFIG::DIALOG_COLOR_CONFIG( EDA_DRAW_FRAME* aParent ) :
    DIALOG_COLOR_CONFIG_BASE( aParent )
{
    m_parent = aParent;

    m_colorConfig = new WIDGET_COLOR_CONFIG( this, aParent );
    m_colorConfig->InstallOnPanel( m_pnlColors );

    GetSizer()->SetSizeHints( this );
}


bool DIALOG_COLOR_CONFIG::TransferDataFromWindow()
{
    return m_colorConfig->TransferDataFromControl();
}
