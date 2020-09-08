/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2020 Roberto Fernandez Bautista <roberto.fer.bau@gmail.com>
 * Copyright (C) 2020 KiCad Developers, see AUTHORS.txt for contributors.
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

/**
 * @file cadstar_pcb_archive_parser.cpp
 * @brief Reads in a CADSTAR Schematic Archive (*.csa) file
 */

#ifndef CADSTAR_SCH_ARCHIVE_PARSER_H_
#define CADSTAR_SCH_ARCHIVE_PARSER_H_

#include <plugins/cadstar/cadstar_archive_parser.h>


/**
 * @brief Represents a CADSTAR Schematic Archive (*.csa) file
 */
class CADSTAR_SCH_ARCHIVE_PARSER : public CADSTAR_ARCHIVE_PARSER
{
public:
    explicit CADSTAR_SCH_ARCHIVE_PARSER( wxString aFilename )
            : Filename( aFilename ), CADSTAR_ARCHIVE_PARSER()
    {
       // KiCadUnitMultiplier = 10; // assume hundredth micron
    }

    /**
     * @brief Parses the file
     * @throw IO_ERROR if file could not be opened or there was
     * an error while parsing
     */
    void Parse();

    wxString Filename;

}; //CADSTAR_SCH_ARCHIVE_PARSER

#endif // CADSTAR_SCH_ARCHIVE_PARSER_H_
