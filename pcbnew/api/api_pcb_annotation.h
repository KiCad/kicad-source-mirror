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

#ifndef KICAD_API_PCB_ANNOTATION_H
#define KICAD_API_PCB_ANNOTATION_H

#include <string>

#include <api/annotation.h>
#include <api/common/commands/annotation_commands.pb.h>

class BOARD;
class NETLIST;
class PCB_CONTEXT;
class REPORTER;


bool GenerateBoardNetlist( BOARD& aBoard, std::string& aNetlist );

bool ParseSchematicNetlist( const std::string& aNetlistText, NETLIST& aNetlist,
                            REPORTER& aReporter );

bool ApplyNetlist( PCB_CONTEXT& aContext, NETLIST& aNetlist,
                   const kiapi::common::commands::ForwardAnnotationSettings& aSettings,
                   ANNOTATION_RESULT& aResult, REPORTER& aReporter );

bool ApplySchematicNetlist( PCB_CONTEXT& aContext, const std::string& aNetlistText,
                            const kiapi::common::commands::ForwardAnnotationSettings& aSettings,
                            ANNOTATION_RESULT& aResult, REPORTER& aReporter );

bool RelinkFootprints( PCB_CONTEXT& aContext, const std::string& aNetlistText,
                       ANNOTATION_RESULT& aResult, REPORTER& aReporter );

#endif // KICAD_API_PCB_ANNOTATION_H
