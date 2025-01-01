/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2022 Mike Williams <mike at mikebwilliams.com>
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


#include <pgm_base.h>
#include <settings/settings_manager.h>
#include <eeschema_settings.h>
#include <tool/selection.h>
#include <tools/sch_editor_conditions.h>

#include <functional>
#include <wx/debug.h>

using namespace std::placeholders;


SELECTION_CONDITION SCH_EDITOR_CONDITIONS::LineMode( LINE_MODE aMode )
{
    SCH_BASE_FRAME* schFrame = dynamic_cast<SCH_BASE_FRAME*>( m_frame );

    wxASSERT( schFrame );

    return std::bind( &SCH_EDITOR_CONDITIONS::lineModeFunc, _1, schFrame, aMode );
}


bool SCH_EDITOR_CONDITIONS::lineModeFunc( const SELECTION& aSelection, SCH_BASE_FRAME* aFrame,
                                          LINE_MODE aMode )
{
    wxCHECK( aFrame && aFrame->eeconfig(), false );

    return aFrame->eeconfig()->m_Drawing.line_mode == aMode;
}
