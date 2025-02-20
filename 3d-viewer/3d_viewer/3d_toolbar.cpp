/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2016 Mario Luzeiro <mrluzeiro@ua.pt>
 * Copyright (C) 2015 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 2013 Wayne Stambaugh <stambaughw@gmail.com>
 * Copyright (C) 2023 CERN
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

#include <wx/wupdlock.h>
#include <wx/choice.h>

#include <bitmaps.h>
#include <dialogs/eda_view_switcher.h>
#include <eda_3d_viewer_frame.h>
#include <tool/action_toolbar.h>
#include <tools/eda_3d_actions.h>
#include <3d_viewer_id.h>

std::optional<TOOLBAR_CONFIGURATION> EDA_3D_VIEWER_FRAME::DefaultTopMainToolbarConfig()
{
    TOOLBAR_CONFIGURATION config;

    // clang-format off
    // Set up toolbar
    /* TODO (ISM): Move to action
    m_tbTopMain->AddTool( ID_RELOAD3D_BOARD, wxEmptyString,
        KiBitmapBundle( BITMAPS::import3d ),
        _( "Reload board" ) );
    */

    config.AppendSeparator();
    /* TODO (ISM): Move to action
    m_tbTopMain->AddTool( ID_TOOL_SCREENCOPY_TOCLIBBOARD, wxEmptyString,
        KiBitmapBundle( BITMAPS::copy ),
        _( "Copy 3D image to clipboard" ) );
*/

    config.AppendSeparator();
    /* TODO (ISM): Move to action
    m_tbTopMain->AddTool( ID_RENDER_CURRENT_VIEW, wxEmptyString,
        KiBitmapBundle( BITMAPS::ray_tracing ),
        _( "Render current view using Raytracing" ), wxITEM_CHECK );
*/

    config.AppendSeparator()
          .AppendAction( ACTIONS::zoomRedraw )
          .AppendAction( ACTIONS::zoomInCenter )
          .AppendAction( ACTIONS::zoomOutCenter )
          .AppendAction( ACTIONS::zoomFitScreen );

    config.AppendSeparator()
          .AppendAction( EDA_3D_ACTIONS::rotateXCW )
          .AppendAction( EDA_3D_ACTIONS::rotateXCCW );

    config.AppendSeparator()
          .AppendAction( EDA_3D_ACTIONS::rotateYCW )
          .AppendAction( EDA_3D_ACTIONS::rotateYCCW );

    config.AppendSeparator()
          .AppendAction( EDA_3D_ACTIONS::rotateZCW )
          .AppendAction( EDA_3D_ACTIONS::rotateZCCW );

    config.AppendSeparator()
          .AppendAction( EDA_3D_ACTIONS::flipView );

    config.AppendSeparator()
          .AppendAction( EDA_3D_ACTIONS::moveLeft )
          .AppendAction( EDA_3D_ACTIONS::moveRight )
          .AppendAction( EDA_3D_ACTIONS::moveUp )
          .AppendAction( EDA_3D_ACTIONS::moveDown );

    config.AppendSeparator()
          .AppendAction( EDA_3D_ACTIONS::toggleOrtho );

    config.AppendSeparator()
          .AppendAction( EDA_3D_ACTIONS::showLayersManager );

    // clang-format on

    return config;
}
