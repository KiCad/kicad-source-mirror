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

#ifndef SCH_EDITOR_CONDITIONS_H_
#define SCH_EDITOR_CONDITIONS_H_

#include <eeschema_settings.h>
#include <tool/editor_conditions.h>

#include <sch_base_frame.h>

class EDA_BASE_FRAME;
class EDA_DRAW_FRAME;

/**
 * Group generic conditions for PCB editor states.
 */
class SCH_EDITOR_CONDITIONS : public EDITOR_CONDITIONS
{
public:
    SCH_EDITOR_CONDITIONS( SCH_BASE_FRAME* aFrame ) : EDITOR_CONDITIONS( aFrame ) {}

    /**
     * Create a functor that tests if the frame is in the specified line drawing mode.
     *
     * @return Functor testing the line drawing mode of a frame.
     */
    SELECTION_CONDITION LineMode( LINE_MODE aMode );

protected:
    ///< Helper function used by LineMode().
    static bool lineModeFunc( const SELECTION& aSelection, SCH_BASE_FRAME* aFrame,
                              LINE_MODE aMode );
};

#endif /* SCH_EDITOR_CONDITIONS_H_ */
