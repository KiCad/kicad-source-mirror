/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2020 Ian McInerney <ian.s.mcinerney at ieee.org>
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


#include <view/view.h>
#include <gal/painter.h>
#include <class_draw_panel_gal.h>
#include <eda_base_frame.h>
#include <eda_draw_frame.h>
#include <gal/gal_display_options.h>
#include <tool/editor_conditions.h>
#include <tool/selection.h>

#include <functional>
#include <wx/debug.h>

using namespace std::placeholders;


SELECTION_CONDITION EDITOR_CONDITIONS::ContentModified()
{
    return std::bind( &EDITOR_CONDITIONS::contentModifiedFunc, _1, m_frame );
}


SELECTION_CONDITION EDITOR_CONDITIONS::UndoAvailable()
{
    return std::bind( &EDITOR_CONDITIONS::undoFunc, _1, m_frame );
}


SELECTION_CONDITION EDITOR_CONDITIONS::RedoAvailable()
{
    return std::bind( &EDITOR_CONDITIONS::redoFunc, _1, m_frame );
}


SELECTION_CONDITION EDITOR_CONDITIONS::Units( EDA_UNITS aUnit )
{
    return std::bind( &EDITOR_CONDITIONS::unitsFunc, _1, m_frame, aUnit );
}


SELECTION_CONDITION EDITOR_CONDITIONS::CurrentTool( const TOOL_ACTION& aTool )
{
    return std::bind( &EDITOR_CONDITIONS::toolFunc, _1, m_frame, std::cref( aTool ) );
}


SELECTION_CONDITION EDITOR_CONDITIONS::NoActiveTool()
{
    return std::bind( &EDITOR_CONDITIONS::noToolFunc, _1, m_frame );
}


SELECTION_CONDITION EDITOR_CONDITIONS::GridVisible()
{
    // The grid visibility check requires a draw frame
    EDA_DRAW_FRAME* drwFrame = dynamic_cast<EDA_DRAW_FRAME*>( m_frame );

    wxASSERT( drwFrame );

    return std::bind( &EDITOR_CONDITIONS::gridFunc, _1, drwFrame );
}


SELECTION_CONDITION EDITOR_CONDITIONS::GridOverrides()
{
    // The grid lock check requires a draw frame
    EDA_DRAW_FRAME* drwFrame = dynamic_cast<EDA_DRAW_FRAME*>( m_frame );

    wxASSERT( drwFrame );

    return std::bind( &EDITOR_CONDITIONS::gridOverridesFunc, _1, drwFrame );
}


SELECTION_CONDITION EDITOR_CONDITIONS::PolarCoordinates()
{
    // The polar coordinates require a draw frame
    EDA_DRAW_FRAME* drwFrame = dynamic_cast<EDA_DRAW_FRAME*>( m_frame );

    wxASSERT( drwFrame );

    return std::bind( &EDITOR_CONDITIONS::polarCoordFunc, _1, drwFrame );
}


SELECTION_CONDITION EDITOR_CONDITIONS::CursorSmallCrosshairs()
{
    EDA_DRAW_FRAME* drwFrame = dynamic_cast<EDA_DRAW_FRAME*>( m_frame );

    wxASSERT( drwFrame );

    return std::bind( &EDITOR_CONDITIONS::cursorFunc, _1, drwFrame,
                      KIGFX::CROSS_HAIR_MODE::SMALL_CROSS );
}


SELECTION_CONDITION EDITOR_CONDITIONS::CursorFullCrosshairs()
{
    EDA_DRAW_FRAME* drwFrame = dynamic_cast<EDA_DRAW_FRAME*>( m_frame );

    wxASSERT( drwFrame );

    return std::bind( &EDITOR_CONDITIONS::cursorFunc, _1, drwFrame,
                      KIGFX::CROSS_HAIR_MODE::FULLSCREEN_CROSS );
}


SELECTION_CONDITION EDITOR_CONDITIONS::Cursor45Crosshairs()
{
    EDA_DRAW_FRAME* drwFrame = dynamic_cast<EDA_DRAW_FRAME*>( m_frame );

    wxASSERT( drwFrame );

    return std::bind( &EDITOR_CONDITIONS::cursorFunc, _1, drwFrame,
                      KIGFX::CROSS_HAIR_MODE::FULLSCREEN_DIAGONAL );
}


SELECTION_CONDITION EDITOR_CONDITIONS::BoundingBoxes()
{
    EDA_DRAW_FRAME* drwFrame = dynamic_cast<EDA_DRAW_FRAME*>( m_frame );

    wxASSERT( drwFrame );

    return std::bind( &EDITOR_CONDITIONS::bboxesFunc, _1, drwFrame );
}


SELECTION_CONDITION EDITOR_CONDITIONS::ScriptingConsoleVisible()
{
    EDA_DRAW_FRAME* drwFrame = dynamic_cast<EDA_DRAW_FRAME*>( m_frame );

    wxASSERT( drwFrame );

    return std::bind( &EDITOR_CONDITIONS::consoleVisibleFunc, _1, drwFrame );
}


bool EDITOR_CONDITIONS::contentModifiedFunc( const SELECTION& aSelection, EDA_BASE_FRAME* aFrame )
{
    return aFrame->IsContentModified();
}


bool EDITOR_CONDITIONS::undoFunc( const SELECTION& aSelection, EDA_BASE_FRAME* aFrame )
{
    return aFrame->GetUndoCommandCount() > 0;
}


bool EDITOR_CONDITIONS::redoFunc( const SELECTION& aSelection, EDA_BASE_FRAME* aFrame )
{
    return aFrame->GetRedoCommandCount() > 0;
}


bool EDITOR_CONDITIONS::unitsFunc( const SELECTION& aSelection, EDA_BASE_FRAME* aFrame,
                                   EDA_UNITS aUnits )
{
    return aFrame->GetUserUnits() == aUnits;
}


bool EDITOR_CONDITIONS::toolFunc( const SELECTION& aSelection, EDA_BASE_FRAME* aFrame,
                                  const TOOL_ACTION& aTool )
{
    return aFrame->IsCurrentTool( aTool );
}


bool EDITOR_CONDITIONS::noToolFunc( const SELECTION& aSelection, EDA_BASE_FRAME* aFrame )
{
    return aFrame->ToolStackIsEmpty();
}


bool EDITOR_CONDITIONS::gridFunc( const SELECTION& aSelection, EDA_DRAW_FRAME* aFrame )
{
    return aFrame->IsGridVisible();
}


bool EDITOR_CONDITIONS::gridOverridesFunc( const SELECTION& aSelection, EDA_DRAW_FRAME* aFrame )
{
    return aFrame->IsGridOverridden();
}


bool EDITOR_CONDITIONS::polarCoordFunc( const SELECTION& aSelection, EDA_DRAW_FRAME* aFrame )
{
    return aFrame->GetShowPolarCoords();
}


bool EDITOR_CONDITIONS::cursorFunc( const SELECTION& aSelection, EDA_DRAW_FRAME* aFrame,
                                    KIGFX::CROSS_HAIR_MODE aMode )
{
    return aFrame->GetGalDisplayOptions().GetCursorMode() == aMode;
}


bool EDITOR_CONDITIONS::bboxesFunc( const SELECTION& aSelection, EDA_DRAW_FRAME* aFrame )
{
    return aFrame->GetCanvas()->GetView()->GetPainter()->GetSettings()->GetDrawBoundingBoxes();
}


bool EDITOR_CONDITIONS::consoleVisibleFunc( const SELECTION& aSelection, EDA_DRAW_FRAME* aFrame )
{
    return aFrame->IsScriptingConsoleVisible();
}
