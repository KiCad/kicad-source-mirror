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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef SCH_IO_EASYPC_H_
#define SCH_IO_EASYPC_H_

#include <sch_io/sch_io.h>
#include <sch_io/sch_io_mgr.h>
#include <reporter.h>

#include <map>
#include <memory>

namespace EASYPC
{
class LIBRARY_FILE;
}


/**
 * Imports Number One Systems Easy-PC and RS DesignSpark PCB schematics (.sch), projects (.prj) and symbol (.ssl)
 * and component (.cml) libraries.
 *
 * A project becomes one top-level sheet per schematic; nets join across sheets by user-defined name only.  A .cml
 * component reads its gate symbols from the .ssl each gate names, looked for beside the .cml ignoring case, and
 * fails with an error naming the gate when one is missing.
 */
class SCH_IO_EASYPC : public SCH_IO
{
public:
    SCH_IO_EASYPC();
    ~SCH_IO_EASYPC();

    const IO_BASE::IO_FILE_DESC GetSchematicFileDesc() const override
    {
        return IO_BASE::IO_FILE_DESC( _HKI( "Easy-PC / DesignSpark schematic and project files" ), { "sch", "prj" } );
    }

    const IO_BASE::IO_FILE_DESC GetLibraryDesc() const override
    {
        return IO_BASE::IO_FILE_DESC( _HKI( "Easy-PC / DesignSpark symbol and component libraries" ),
                                      { "ssl", "cml" } );
    }

    bool CanReadLibrary( const wxString& aFileName ) const override;

    void EnumerateSymbolLib( wxArrayString& aSymbolNameList, const wxString& aLibraryPath,
                             const std::map<std::string, UTF8>* aProperties = nullptr ) override;

    void EnumerateSymbolLib( std::vector<LIB_SYMBOL*>& aSymbolList, const wxString& aLibraryPath,
                             const std::map<std::string, UTF8>* aProperties = nullptr ) override;

    /// Converted on first use and owned by the plugin until the file changes
    LIB_SYMBOL* LoadSymbol( const wxString& aLibraryPath, const wxString& aPartName,
                            const std::map<std::string, UTF8>* aProperties = nullptr ) override;

    bool CanReadSchematicFile( const wxString& aFileName ) const override;

    int GetModifyHash() const override { return 0; }

    SCH_SHEET* LoadSchematicFile( const wxString& aFileName, SCHEMATIC* aSchematic, SCH_SHEET* aAppendToMe = nullptr,
                                  const std::map<std::string, UTF8>* aProperties = nullptr ) override;

    bool IsLibraryWritable( const wxString& aLibraryPath ) override { return false; }

private:
    struct LIBRARY
    {
        long long                                       Timestamp = 0;
        std::unique_ptr<EASYPC::LIBRARY_FILE>           File;
        std::map<wxString, std::unique_ptr<LIB_SYMBOL>> Symbols;
    };

    LIBRARY& library( const wxString& aLibraryPath );

    std::unique_ptr<LIB_SYMBOL> convertComponent( const LIBRARY& aLib, const wxString& aLibraryPath,
                                                  const wxString& aName );

    std::map<wxString, LIBRARY> m_libraries;
};

#endif // SCH_IO_EASYPC_H_
