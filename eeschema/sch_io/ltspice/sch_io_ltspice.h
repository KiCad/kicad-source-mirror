/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2022 Chetan Subhash Shinde<chetanshinde2001@gmail.com>
 * Copyright (C) 2023 CERN
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or (at your
 * option) any later version
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef SCH_IO_LTSPICE_H_
#define SCH_IO_LTSPICE_H_

#include <sch_io/sch_io.h>
#include <sch_io/sch_io_mgr.h>
#include <reporter.h>
#include <wx/log.h>


class SCH_SHEET;
class SCH_SCREEN;


class SCH_IO_LTSPICE : public SCH_IO
{
public:
    SCH_IO_LTSPICE() : SCH_IO( wxS( "LTspice Schematic" ) )
    {
        m_reporter = &WXLOG_REPORTER::GetInstance();
    }

    ~SCH_IO_LTSPICE()
    {
    }

    const IO_BASE::IO_FILE_DESC GetSchematicFileDesc() const override
    {
        return IO_BASE::IO_FILE_DESC( _HKI( "LTspice schematic files" ), { "asc" } );
    }

    const IO_BASE::IO_FILE_DESC GetLibraryDesc() const override
    {
        // This was originally commented out, so keep it commented and just return an empty library description
        //return IO_BASE::IO_FILE_DESC( _HKI( "LTspice library files" ), { "lib" } );
        return IO_BASE::IO_FILE_DESC( wxEmptyString, { } );
    }

    int GetModifyHash() const override;

    SCH_SHEET* LoadSchematicFile( const wxString& aFileName, SCHEMATIC* aSchematic,
                                  SCH_SHEET*             aAppendToMe = nullptr,
                                  const std::map<std::string, UTF8>* aProperties = nullptr ) override;

};
#endif // SCH_IO_LTSPICE_H_
