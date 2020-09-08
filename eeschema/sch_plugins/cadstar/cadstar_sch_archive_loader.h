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
 * @file cadstar_sch_archive_loader.h
 * @brief Loads a csa file into a KiCad SCHEMATIC object
 */

#ifndef CADSTAR_SCH_ARCHIVE_LOADER_H_
#define CADSTAR_SCH_ARCHIVE_LOADER_H_

#include <sch_plugins/cadstar/cadstar_sch_archive_parser.h>
#include <schematic.h>


class CADSTAR_SCH_ARCHIVE_LOADER : public CADSTAR_SCH_ARCHIVE_PARSER
{
public:
    explicit CADSTAR_SCH_ARCHIVE_LOADER( wxString aFilename )
            : CADSTAR_SCH_ARCHIVE_PARSER( aFilename )
    {
        mSchematic = nullptr;
    }


    ~CADSTAR_SCH_ARCHIVE_LOADER()
    {
    }

    /**
     * @brief Loads a CADSTAR PCB Archive file into the KiCad BOARD object given
     * @param aBoard 
     */
    void Load( ::SCHEMATIC* aSchematic );


private:
    ::SCHEMATIC* mSchematic;

}; // CADSTAR_SCH_ARCHIVE_LOADER


#endif // CADSTAR_SCH_ARCHIVE_LOADER_H_
