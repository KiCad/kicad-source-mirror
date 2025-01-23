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


#ifndef QA_PCBNEW_UTILS_BOARD_FILE_UTILS__H
#define QA_PCBNEW_UTILS_BOARD_FILE_UTILS__H

#include <iostream>
#include <memory>
#include <string>

class BOARD;
class BOARD_ITEM;
class FOOTPRINT;

/**
 * @file board_file_utils.h
 * General utilities for PCB file IO for QA programs
 */
namespace KI_TEST
{
/**
 * Utility which returns a path to the data directory where the test board files are stored.
 */
std::string GetPcbnewTestDataDir();

/**
 * Utility function to simply write a Board out to a file.
 *
 * Helps debug tests and utility programs by making it easy to quickly
 * write to disk without directly using the PCB_IO_KICAD_SEXPR API.
 *
 * Note: The aBoard param is non-const because PCB_IO_KICAD_SEXPR::Save demands it
 * and I am not confident a const_cast will be a true assurance.
 *
 * @param aBoard    the board to write out
 * @param aFilename the filename to write to
 */
void DumpBoardToFile( BOARD& aBoard, const std::string& aFilename );

/**
 * Same as DumpBoardToFile, but for footprints
 */
void DumpFootprintToFile( const FOOTPRINT& aFootprint, const std::string& aLibraryPath );

/**
 * Utility function to read a #BOARD_ITEM (probably a #FOOTPRINT or a #BOARD)
 * from a file.
 *
 * Helps when writing tests or utilities that can be fed an external file.
 *
 * @param aFilename   the file to read in
 * @returns           a new #BOARD_ITEM, which is nullptr if the read or parse failed.
 */
std::unique_ptr<BOARD_ITEM> ReadBoardItemFromStream( std::istream& aStream );

/**
 * Read a specific kind of #BOARD_ITEM from a stream
 *
 * @tparam ITEM the item type to return (probably a #FOOTPRINT or #BOARD)
 * @param aStream the stream to read from.
 */
template <typename ITEM> std::unique_ptr<ITEM> ReadItemFromStream( std::istream& aStream )
{
    auto                  bi_ptr = ReadBoardItemFromStream( aStream );
    std::unique_ptr<ITEM> downcast_ptr;

    // if it's the right type, downcast and "steal" (and we'll return ownership)
    ITEM* const tmp = dynamic_cast<ITEM*>( bi_ptr.get() );
    if( tmp != nullptr )
    {
        bi_ptr.release();
        downcast_ptr.reset( tmp );
    }

    return downcast_ptr;
}

/**
 * Read a board from a file, or another stream, as appropriate
 *
 * @param aFilename The file to read, or the fallback if empty
 * @param aFallback: the fallback stream
 * @return a #BOARD, if successful
 */
std::unique_ptr<BOARD> ReadBoardFromFileOrStream( const std::string& aFilename,
                                                  std::istream& aFallback = std::cin );

std::unique_ptr<FOOTPRINT> ReadFootprintFromFileOrStream( const std::string& aFilename,
                                                          std::istream& aFallback = std::cin );

} // namespace KI_TEST

#endif // QA_PCBNEW_UTILS_BOARD_FILE_UTILS__H
