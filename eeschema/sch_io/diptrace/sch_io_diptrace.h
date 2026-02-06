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

#ifndef SCH_IO_DIPTRACE_H_
#define SCH_IO_DIPTRACE_H_

#include <sch_io/sch_io.h>
#include <sch_io/sch_io_mgr.h>
#include <reporter.h>


class SCH_SHEET;
class SCH_SCREEN;
class SCHEMATIC;


/**
 * A SCH_IO derivation for loading DipTrace schematic files (.dch).
 *
 * DipTrace stores schematics as proprietary binary files using big-endian
 * biased integer encoding.  This plugin reads the binary format and creates
 * KiCad schematic objects (symbols, wires, labels, sheets).
 *
 * Supported format versions: 31 through 49.
 */
class SCH_IO_DIPTRACE : public SCH_IO
{
public:
    SCH_IO_DIPTRACE() : SCH_IO( wxS( "DipTrace Schematic" ) )
    {
        m_reporter = &WXLOG_REPORTER::GetInstance();
    }

    ~SCH_IO_DIPTRACE() {}

    const IO_BASE::IO_FILE_DESC GetSchematicFileDesc() const override
    {
        return IO_BASE::IO_FILE_DESC( _HKI( "DipTrace schematic files" ), { "dch" } );
    }

    const IO_BASE::IO_FILE_DESC GetLibraryDesc() const override
    {
        return IO_BASE::IO_FILE_DESC( wxEmptyString, {} );
    }

    bool CanReadSchematicFile( const wxString& aFileName ) const override;

    int GetModifyHash() const override { return 0; }

    SCH_SHEET* LoadSchematicFile( const wxString& aFileName, SCHEMATIC* aSchematic,
                                  SCH_SHEET*             aAppendToMe = nullptr,
                                  const std::map<std::string, UTF8>* aProperties = nullptr ) override;

    bool IsLibraryWritable( const wxString& aLibraryPath ) override { return false; }
};

#endif // SCH_IO_DIPTRACE_H_
