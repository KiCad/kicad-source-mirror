/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2024 KiCad Developers, see AUTHORS.txt for contributors.
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

#ifndef DRC_RE_OVERLAY_PANEL_FACTORY_H
#define DRC_RE_OVERLAY_PANEL_FACTORY_H

#include <eda_units.h>

#include "drc_rule_editor_enums.h"

class wxWindow;
class DRC_RE_BITMAP_OVERLAY_PANEL;
class DRC_RE_BASE_CONSTRAINT_DATA;


/**
 * Factory for creating overlay panels for DRC constraint editing.
 *
 * Maps constraint types to their corresponding bitmap overlay panel classes. The overlay
 * panels display constraint diagrams with input fields positioned over the artwork.
 */
class DRC_RE_OVERLAY_PANEL_FACTORY
{
public:
    /**
     * Create an overlay panel for the given constraint type.
     *
     * Returns the appropriate overlay panel subclass based on the constraint type. For
     * constraints that don't have a specialized overlay panel (e.g., custom rules),
     * returns nullptr.
     *
     * @param aParent The parent window for the overlay panel.
     * @param aType The constraint type determining which panel class to create.
     * @param aData The constraint data to bind to the panel. The factory will cast this
     *              to the appropriate derived type based on aType.
     * @param aUnits The unit system for displaying values. Ignored for constraint types
     *               that don't display dimensional values.
     * @return The created overlay panel, or nullptr if the constraint type doesn't support
     *         bitmap overlays.
     */
    static DRC_RE_BITMAP_OVERLAY_PANEL* CreateOverlayPanel( wxWindow* aParent,
                                                             DRC_RULE_EDITOR_CONSTRAINT_NAME aType,
                                                             DRC_RE_BASE_CONSTRAINT_DATA* aData,
                                                             EDA_UNITS aUnits );
};

#endif // DRC_RE_OVERLAY_PANEL_FACTORY_H
