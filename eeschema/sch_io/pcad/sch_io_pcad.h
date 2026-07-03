/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2025 KiCad Developers, see AUTHORS.txt for contributors.
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
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef SCH_IO_PCAD_H
#define SCH_IO_PCAD_H

#include <sch_io/sch_io.h>
#include <sch_io/sch_io_mgr.h>

#include <map>
#include <memory>

class SCH_SCREEN;
class SCH_SHEET;
class SCHEMATIC;
class LIB_SYMBOL;

namespace PCAD_SCH
{
struct SHEET;
struct SCHEMATIC;
}

struct LIB_SYMBOL_STORE;

/**
 * A SCH_IO derivation for loading P-CAD 2006 ASCII schematic files (.SCH).
 *
 * P-CAD files begin with "ACCEL_ASCII" and use a parenthesised keyword format
 * identical to the format used by the existing pcbnew P-CAD PCB importer.
 */
class SCH_IO_PCAD : public SCH_IO
{
public:
    SCH_IO_PCAD();
    ~SCH_IO_PCAD();

    const IO_BASE::IO_FILE_DESC GetSchematicFileDesc() const override
    {
        return IO_BASE::IO_FILE_DESC( _HKI( "P-CAD schematic files" ), { "SCH", "sch" } );
    }

    const IO_BASE::IO_FILE_DESC GetLibraryDesc() const override
    {
        return IO_BASE::IO_FILE_DESC( _HKI( "P-CAD schematic and library files" ),
                                      { "SCH", "sch", "LIA", "lia" } );
    }

    bool CanReadSchematicFile( const wxString& aFileName ) const override;

    bool CanReadLibrary( const wxString& aFileName ) const override;

    int GetModifyHash() const override { return m_modifyHash; }

    SCH_SHEET* LoadSchematicFile( const wxString& aFileName, SCHEMATIC* aSchematic,
                                  SCH_SHEET*             aAppendToMe  = nullptr,
                                  const std::map<std::string, UTF8>* aProperties = nullptr ) override;

    // Library interface — P-CAD .SCH files embed symbol definitions, so we
    // can expose them as a read-only symbol library.
    void EnumerateSymbolLib( wxArrayString& aSymbolNameList, const wxString& aLibraryPath,
                             const std::map<std::string, UTF8>* aProperties = nullptr ) override;

    void EnumerateSymbolLib( std::vector<LIB_SYMBOL*>& aSymbolList, const wxString& aLibraryPath,
                             const std::map<std::string, UTF8>* aProperties = nullptr ) override;

    LIB_SYMBOL* LoadSymbol( const wxString& aLibraryPath, const wxString& aPartName,
                            const std::map<std::string, UTF8>* aProperties = nullptr ) override;

    bool IsLibraryWritable( const wxString& aLibraryPath ) override { return false; }

private:
    static wxString getLibName( const ::SCHEMATIC* aSchematic, const wxString& aFileName );

    /// Parse the library file once and serve cache-owned symbols, invalidated
    /// by path or file timestamp changes.
    void ensureLoadedLibrary( const wxString& aLibraryPath );

    void populateScreen( SCH_SCREEN* aScreen, const PCAD_SCH::SHEET& aSheet,
                         const PCAD_SCH::SCHEMATIC& aPcad, double aPageH,
                         const LIB_SYMBOL_STORE& aLibSymbols, const wxString& aLibName );

    wxString  m_cachePath;
    long long m_cacheTimestamp = 0;
    int       m_modifyHash = 0;

    std::map<wxString, std::unique_ptr<LIB_SYMBOL>> m_libCache;
};

#endif // SCH_IO_PCAD_H
