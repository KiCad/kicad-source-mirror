/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2019 KiCad Developers, see AUTHORS.txt for contributors.
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

#include <pcbnew_utils/board_file_utils.h>

// For PCB parsing
#include <kicad_plugin.h>
#include <pcb_parser.h>
#include <richio.h>

#include <class_board.h>

namespace KI_TEST
{

void DumpBoardToFile( BOARD& board, const std::string& aFilename )
{
    PCB_IO io;
    io.Save( aFilename, &board );
}


std::unique_ptr<BOARD_ITEM> ReadBoardItemFromFile( const std::string& aFilename )
{
    FILE_LINE_READER reader( aFilename );

    PCB_PARSER parser;
    parser.SetLineReader( &reader );

    return std::unique_ptr<BOARD_ITEM>( parser.Parse() );
}

} // namespace KI_TEST