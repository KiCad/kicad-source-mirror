/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 *
 * Based on the dsn2kicad reference implementation and on OrCAD file format
 * documentation from the OpenOrCadParser project (MIT licensed).
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
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef SCH_IO_ORCAD_H_
#define SCH_IO_ORCAD_H_

#include <sch_io/sch_io.h>
#include <sch_io/sch_io_mgr.h>
#include <reporter.h>

#include <map>
#include <memory>
#include <vector>


class SCH_SHEET;
class LIB_SYMBOL;
class SCH_SCREEN;
class SCHEMATIC;

std::string OrcadNormalizeCfbName( const std::string& aName );


class SCH_IO_ORCAD : public SCH_IO
{
public:
    SCH_IO_ORCAD() :
            SCH_IO( wxS( "OrCAD Schematic" ) )
    {
        m_reporter = &WXLOG_REPORTER::GetInstance();
    }

    ~SCH_IO_ORCAD() {}

    const IO_BASE::IO_FILE_DESC GetSchematicFileDesc() const override
    {
        return IO_BASE::IO_FILE_DESC( _HKI( "OrCAD Capture schematic files" ), { "dsn" } );
    }

    const IO_BASE::IO_FILE_DESC GetLibraryDesc() const override
    {
        return IO_BASE::IO_FILE_DESC( _HKI( "OrCAD Capture symbol library files" ), { "olb" } );
    }

    /** The .dsn extension also identifies SPECCTRA files. Require an OrCAD compound document. */
    bool CanReadSchematicFile( const wxString& aFileName ) const override;

    bool CanReadLibrary( const wxString& aFileName ) const override;

    int GetModifyHash() const override { return 0; }


    SCH_SHEET* LoadSchematicFile( const wxString& aFileName, SCHEMATIC* aSchematic, SCH_SHEET* aAppendToMe = nullptr,
                                  const std::map<std::string, UTF8>* aProperties = nullptr ) override;


    void EnumerateSymbolLib( wxArrayString& aSymbolNameList, const wxString& aLibraryPath,
                             const std::map<std::string, UTF8>* aProperties = nullptr ) override;

    void EnumerateSymbolLib( std::vector<LIB_SYMBOL*>& aSymbolList, const wxString& aLibraryPath,
                             const std::map<std::string, UTF8>* aProperties = nullptr ) override;

    LIB_SYMBOL* LoadSymbol( const wxString& aLibraryPath, const wxString& aAliasName,
                            const std::map<std::string, UTF8>* aProperties = nullptr ) override;

    bool IsLibraryWritable( const wxString& aLibraryPath ) override { return false; }

private:
    /** Parse an .OLB and build its KiCad library symbols, cached by library path.
     * The returned symbols are owned by m_libCache. */
    const std::vector<std::unique_ptr<LIB_SYMBOL>>& loadOlbSymbols( const wxString& aLibraryPath );

    std::map<wxString, std::vector<std::unique_ptr<LIB_SYMBOL>>> m_libCache;
};

#endif // SCH_IO_ORCAD_H_
