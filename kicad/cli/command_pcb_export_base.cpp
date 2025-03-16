/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2022 Mark Roszko <mark.roszko@gmail.com>
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
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "command_pcb_export_base.h"
#include <bitset>
#include <string_utils.h>


CLI::PCB_EXPORT_BASE_COMMAND::PCB_EXPORT_BASE_COMMAND( const std::string& aName,
                                                       bool aInputCanBeDir, bool aOutputIsDir ) :
        COMMAND( aName )
{
    addCommonArgs( true, true, aInputCanBeDir, aOutputIsDir );
}


void CLI::PCB_EXPORT_BASE_COMMAND::addLayerArg()
{
    m_argParser.add_argument( "-l", ARG_LAYERS )
            .default_value( std::string() )
            .help( UTF8STDSTR( _( "Comma separated list of untranslated layer names to include "
                                  "such as F.Cu,B.Cu" ) ) )
            .metavar( "LAYER_LIST" );
}


void CLI::PCB_EXPORT_BASE_COMMAND::addCommonLayersArg()
{
    m_argParser.add_argument( "--cl", ARG_COMMON_LAYERS )
            .default_value( std::string() )
            .help( UTF8STDSTR( _( "Layers to include on each plot, comma separated list of "
                                  "untranslated layer names to include such as F.Cu,B.Cu" ) ) )
            .metavar( "COMMON_LAYER_LIST" );
}