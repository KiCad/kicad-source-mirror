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

#ifndef KICAD_API_SCH_ANNOTATION_H
#define KICAD_API_SCH_ANNOTATION_H

#include <string>

#include <api/annotation.h>
#include <api/common/commands/annotation_commands.pb.h>

class KIWAY;
class REPORTER;
class SCHEMATIC;
class SCH_CONTEXT;
class SCH_EDIT_FRAME;

bool CheckSchematicReadyForNetlist( SCHEMATIC& aSchematic, REPORTER& aReporter );

void FormatSchematicNetlist( SCHEMATIC& aSchematic, KIWAY* aKiway, std::string& aNetlist );

bool GenerateSchematicNetlist( SCHEMATIC& aSchematic, KIWAY* aKiway, std::string& aNetlist,
                               REPORTER& aReporter );

bool ApplyBoardNetlistToSchematic( SCH_CONTEXT* aContext, SCH_EDIT_FRAME* aFrame,
                                   const std::string& aNetlist,
                                   const kiapi::common::commands::BackAnnotationSettings& aSettings,
                                   ANNOTATION_RESULT& aResult, REPORTER& aReporter );

#endif // KICAD_API_SCH_ANNOTATION_H
