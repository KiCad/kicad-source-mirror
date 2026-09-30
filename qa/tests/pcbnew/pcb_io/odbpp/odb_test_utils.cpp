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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "odb_test_utils.h"

#include <map>

#include <board.h>
#include <core/utf8.h>
#include <reporter.h>
#include <pcbnew/pcb_io/odbpp/pcb_io_odbpp.h>

namespace fs = std::filesystem;


fs::path ExportOdb( const BOARD& aBoard, const fs::path& aDir, const std::string& aUnits,
                    const std::string& aSigfig, REPORTER* aReporter )
{
    fs::create_directories( aDir );

    PCB_IO_ODBPP odbExporter;

    if( aReporter )
        odbExporter.SetReporter( aReporter );

    std::map<std::string, UTF8> props;
    props["units"] = aUnits;
    props["sigfig"] = aSigfig;

    odbExporter.SaveBoard( wxString::FromUTF8( aDir.string() ), const_cast<BOARD&>( aBoard ), &props );

    return aDir;
}
