/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
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

#include <symbol.h>
#include <sch_group.h>


std::vector<int> SYMBOL::ViewGetLayers() const
{
    // Pins and op point currents are drawn by their parent symbol,
    // so the parent must draw to LAYER_DANGLING and LAYER_OP_CURRENTS
    if( Type() == SCH_SYMBOL_T )
        return { LAYER_DANGLING,          LAYER_OP_CURRENTS,       LAYER_DEVICE,
                 LAYER_REFERENCEPART,     LAYER_VALUEPART,         LAYER_FIELDS,
                 LAYER_DEVICE_BACKGROUND, LAYER_SHAPES_BACKGROUND, LAYER_NOTES_BACKGROUND,
                 LAYER_SELECTION_SHADOWS };

    // Library symbols must include LAYER_PRIVATE_NOTES
    if( Type() == LIB_SYMBOL_T )
        return { LAYER_DEVICE,
                 LAYER_REFERENCEPART,
                 LAYER_VALUEPART,
                 LAYER_FIELDS,
                 LAYER_PRIVATE_NOTES,
                 LAYER_DEVICE_BACKGROUND,
                 LAYER_SHAPES_BACKGROUND,
                 LAYER_NOTES_BACKGROUND,
                 LAYER_SELECTION_SHADOWS };

    // This should never happen but if it does, return a reasonable default
    return { LAYER_DEVICE,           LAYER_REFERENCEPART,     LAYER_VALUEPART,
             LAYER_FIELDS,           LAYER_DEVICE_BACKGROUND, LAYER_SHAPES_BACKGROUND,
             LAYER_NOTES_BACKGROUND, LAYER_SELECTION_SHADOWS };
}
