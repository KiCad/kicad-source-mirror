/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2020 Roberto Fernandez Bautista <roberto.fer.bau@gmail.com>
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
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

#ifndef SCH_IO_CADSTAR_ARCHIVE_H_
#define SCH_IO_CADSTAR_ARCHIVE_H_

#include <map>
#include <sch_io/sch_io.h>
#include <sch_io/sch_io_mgr.h>
#include <reporter.h>
#include <lib_symbol.h>
#include <wildcards_and_files_ext.h>


class SCH_SHEET;
class SCH_SCREEN;

class SCH_IO_CADSTAR_ARCHIVE : public SCH_IO
{
public:
    SCH_IO_CADSTAR_ARCHIVE() : SCH_IO( wxS( "CADSTAR Schematic Archive" ) ),
        m_cacheTimestamp( 0 )
    {
        m_reporter = &WXLOG_REPORTER::GetInstance();
    }

    virtual ~SCH_IO_CADSTAR_ARCHIVE() {}

    void SetReporter( REPORTER* aReporter ) override { m_reporter = aReporter; }

    void SetProgressReporter( PROGRESS_REPORTER* aReporter ) override
    {
        m_progressReporter = aReporter;
    }

    const IO_BASE::IO_FILE_DESC GetSchematicFileDesc() const override
    {
        return IO_BASE::IO_FILE_DESC( _HKI( "CADSTAR Schematic Archive files" ),
                                      { FILEEXT::CadstarSchematicFileExtension } );
    }

    const IO_BASE::IO_FILE_DESC GetLibraryDesc() const override
    {
        return IO_BASE::IO_FILE_DESC( _HKI( "CADSTAR Parts Library files" ),
                                      { FILEEXT::CadstarPartsLibraryFileExtension } );
    }

    bool CanReadLibrary( const wxString& aFileName ) const override;

    int GetModifyHash() const override;

    SCH_SHEET* LoadSchematicFile( const wxString& aFileName, SCHEMATIC* aSchematic,
                                  SCH_SHEET*             aAppendToMe = nullptr,
                                  const std::map<std::string, UTF8>* aProperties = nullptr ) override;

    void EnumerateSymbolLib( wxArrayString&    aSymbolNameList,
                             const wxString&   aLibraryPath,
                             const std::map<std::string, UTF8>* aProperties = nullptr ) override;

    void EnumerateSymbolLib( std::vector<LIB_SYMBOL*>& aSymbolList,
                             const wxString&           aLibraryPath,
                             const std::map<std::string, UTF8>*         aProperties = nullptr ) override;

    LIB_SYMBOL* LoadSymbol( const wxString& aLibraryPath, const wxString& aAliasName,
                            const std::map<std::string, UTF8>* aProperties = nullptr ) override;

    void GetAvailableSymbolFields( std::vector<wxString>& aNames ) override;


    // Writing to CADSTAR libraries is not supported
    bool IsLibraryWritable( const wxString& aLibraryPath ) override { return false; }

    void GetLibraryOptions( std::map<std::string, UTF8>* aListToAppendTo ) const override;

private:
    // Symbol caching
    void ensureLoadedLibrary( const wxString& aLibraryPath, const std::map<std::string, UTF8>* aProperties );

    typedef std::map<const wxString, std::unique_ptr<LIB_SYMBOL>> NAME_TO_SYMBOL_MAP;

    NAME_TO_SYMBOL_MAP m_libCache;
    long long          m_cacheTimestamp;
    wxString           m_cachePath;
    wxFileName         m_cachecsafn;
    wxString           m_cachefplibname;
};

#endif // SCH_IO_CADSTAR_ARCHIVE_H_
