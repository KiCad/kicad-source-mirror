/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 3
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef SETTINGS_SNAP_SETTINGS_PARAMS_H
#define SETTINGS_SNAP_SETTINGS_PARAMS_H

#include <vector>

#include <settings/parameters.h>
#include <settings/snap_settings.h>

/**
 * Register the snap inference parameters.
 *
 * Kept here so the schematic and board editors cannot drift apart on config keys or defaults;
 * the defaults come from the struct itself rather than being restated per editor.
 */
inline void AddSnapInferenceParams( std::vector<PARAM_BASE*>& aParams, SNAP_INFERENCE_SETTINGS& aSettings )
{
    const SNAP_INFERENCE_SETTINGS defaults;

    aParams.emplace_back( new PARAM<bool>( "editing.snap_object_geometry", &aSettings.objectGeometry,
                                           defaults.objectGeometry ) );

    aParams.emplace_back( new PARAM<bool>( "editing.snap_construction_extensions",
                                           &aSettings.constructionExtensions, defaults.constructionExtensions ) );

    aParams.emplace_back( new PARAM<bool>( "editing.snap_tangent_normal", &aSettings.tangentNormal,
                                           defaults.tangentNormal ) );

    aParams.emplace_back( new PARAM<bool>( "editing.snap_alignment_distribution",
                                           &aSettings.alignmentDistribution, defaults.alignmentDistribution ) );
}

#endif
