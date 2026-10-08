/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#pragma once

#include <nlohmann/json_fwd.hpp>

#include <compatibility_report.h>
#include <downgrade_target.h>
#include <kicommon.h>

/// Migrate a .kicad_pro document down to what the target release expects. The downgrade twin
/// of the settings upgrade migrations: newer keys are removed, nested schema versions are
/// rewound, and behavior-changing losses are added to the report. Keys the target simply
/// ignores are retained when inert. Unknown future schemas block without modifying the document.
KICOMMON_API void DowngradeProjectFileJson( nlohmann::json& aDoc, const DOWNGRADE_TARGET& aTarget,
                                            COMPATIBILITY_REPORT& aReport );
