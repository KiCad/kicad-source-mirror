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


/**
 * A SCH_IO derivation for loading OrCAD Capture schematic designs (.dsn).
 *
 * An OrCAD .dsn is an OLE2/CFB compound document.  The plugin reads the root
 * 'Library' stream (string table, fonts, page settings), the 'Cache' and
 * 'Packages/<name>' streams (symbol definitions, package pin maps) and the
 * 'Views/<folder>/Pages/<page>' streams (page content), then hands the parsed
 * design to ORCAD_CONVERTER.
 *
 * Supported: the modern stream framing (Library version 3 and later, i.e.
 * OrCAD 10.x/2003 onward) and the pre-2003 v2.0 framing (parsed with the v2 page
 * reader; its legacy symbol cache is not decoded, so v2 symbol graphics are
 * synthesized placeholders).  Unambiguous hierarchical block designs are recreated
 * as KiCad hierarchical sheets.  Designs with incomplete or ambiguous block
 * mappings fall back to separate top-level sheets, materialized once per block
 * occurrence with that occurrence's reference designators.  OLE-embedded pictures
 * are skipped with a warning.
 */
class SCH_IO_ORCAD : public SCH_IO
{
public:
    SCH_IO_ORCAD() : SCH_IO( wxS( "OrCAD Schematic" ) )
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

    /**
     * The .dsn extension collides with SPECCTRA PCB session files (plain text),
     * so beyond the extension gate the file must be an OLE2/CFB compound
     * document (magic D0 CF 11 E0 A1 B1 1A E1) whose root storage contains a
     * "Library" stream AND a "Views" or "Schematics" storage.
     */
    bool CanReadSchematicFile( const wxString& aFileName ) const override;

    int GetModifyHash() const override { return 0; }

    /**
     * Orchestration: root sheet/screen boilerplate (honoring aAppendToMe), CFB
     * open, Library parse + version gate, Cache/Packages parse and merge, root
     * schematic folder selection (folder matching the Library's schematic name
     * case-insensitively, else the first folder; other folders are recorded as
     * skipped), page-order resolution, per-page parse, ORCAD_CONVERTER::Convert,
     * then CurrentSheet().UpdateAllScreenReferences() and
     * aSchematic->FixupJunctionsAfterImport().
     */
    SCH_SHEET* LoadSchematicFile( const wxString& aFileName, SCHEMATIC* aSchematic,
                                  SCH_SHEET*             aAppendToMe = nullptr,
                                  const std::map<std::string, UTF8>* aProperties = nullptr ) override;

    /**
     * Enumerate the symbols of an OrCAD .OLB library (an OLE2/CFB compound document
     * with the same Library/Cache/Packages streams as a design, but no schematic
     * pages).  The cache symbol definitions are converted to KiCad library symbols.
     */
    void EnumerateSymbolLib( wxArrayString& aSymbolNameList, const wxString& aLibraryPath,
                             const std::map<std::string, UTF8>* aProperties = nullptr ) override;

    void EnumerateSymbolLib( std::vector<LIB_SYMBOL*>& aSymbolList, const wxString& aLibraryPath,
                             const std::map<std::string, UTF8>* aProperties = nullptr ) override;

    LIB_SYMBOL* LoadSymbol( const wxString& aLibraryPath, const wxString& aAliasName,
                            const std::map<std::string, UTF8>* aProperties = nullptr ) override;

    bool IsLibraryWritable( const wxString& aLibraryPath ) override { return false; }

private:
    /// Parse an .OLB and build its KiCad library symbols, cached by library path.
    /// The returned symbols are owned by m_libCache.
    const std::vector<std::unique_ptr<LIB_SYMBOL>>& loadOlbSymbols( const wxString& aLibraryPath );

    std::map<wxString, std::vector<std::unique_ptr<LIB_SYMBOL>>> m_libCache;
};

#endif // SCH_IO_ORCAD_H_
