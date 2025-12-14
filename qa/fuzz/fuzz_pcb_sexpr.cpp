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

#include <cstdint>
#include <string>
#include <memory>

#include <pcb_io/kicad_sexpr/pcb_io_kicad_sexpr_parser.h>
#include <richio.h>
#include <board_item.h>

#include "fuzz_init.h"
#include <pcbnew_settings.h>
#include <mock_pgm_base.h>
#include <settings/settings_manager.h>

extern "C" int LLVMFuzzerInitialize( int* argc, char*** argv )
{
    fuzz_init( argc, argv );
    Pgm().GetSettingsManager().RegisterSettings( new PCBNEW_SETTINGS, false );

    Pgm().GetSettingsManager().Load();
    Pgm().GetSettingsManager().LoadProject( "" );

    return 0;
}

extern "C" int LLVMFuzzerTestOneInput(const uint8_t *Data, size_t Size) {
    std::string input(reinterpret_cast<const char*>(Data), Size);
    STRING_LINE_READER reader(input, "fuzz");
    
    auto callback = [](wxString, int, wxString, wxString) { return true; };

    // We need to handle exceptions as the parser throws on invalid input
    try {
        PCB_IO_KICAD_SEXPR_PARSER parser(&reader, nullptr, callback);
        std::unique_ptr<BOARD_ITEM> item(parser.Parse());
    } catch (...) {
    }
    
    return 0;
}