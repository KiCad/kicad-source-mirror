/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2025 KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 3
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef SCH_IO_PADS_H_
#define SCH_IO_PADS_H_

#include <sch_io/sch_io.h>
#include <sch_io/sch_io_mgr.h>

#include <unordered_map>

class SCH_SCREEN;
class SCH_SHEET;
class SCHEMATIC;
class LIB_SYMBOL;

/**
 * A #SCH_IO derivation for loading PADS Logic schematic files.
 *
 * PADS Logic exports schematic designs as ASCII text files that can be parsed
 * and converted to KiCad schematic format.
 */
class SCH_IO_PADS : public SCH_IO
{
public:
    SCH_IO_PADS();
    ~SCH_IO_PADS();

    const IO_BASE::IO_FILE_DESC GetSchematicFileDesc() const override
    {
        return IO_BASE::IO_FILE_DESC( _HKI( "PADS Logic schematic files" ), { "asc", "txt" } );
    }

    const IO_BASE::IO_FILE_DESC GetLibraryDesc() const override
    {
        return IO_BASE::IO_FILE_DESC( _HKI( "PADS Logic library files" ), { "asc", "txt" } );
    }

    bool CanReadSchematicFile( const wxString& aFileName ) const override;
    bool CanReadLibrary( const wxString& aFileName ) const override;

    int GetModifyHash() const override { return 0; }

    SCH_SHEET* LoadSchematicFile( const wxString& aFileName, SCHEMATIC* aSchematic,
                                  SCH_SHEET*             aAppendToMe = nullptr,
                                  const std::map<std::string, UTF8>* aProperties = nullptr ) override;

private:
    /**
     * Check if the file header indicates a PADS Logic schematic file.
     *
     * @param aFileName Path to the file to check.
     * @return True if file appears to be a PADS Logic schematic.
     */
    bool checkFileHeader( const wxString& aFileName ) const;

    std::unordered_map<wxString, SEVERITY> m_errorMessages;
};

#endif // SCH_IO_PADS_H_
