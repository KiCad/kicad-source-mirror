/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2020 Roberto Fernandez Bautista <roberto.fer.bau@gmail.com>
 * Copyright (C) 2020-2023 KiCad Developers, see AUTHORS.txt for contributors.
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
 * @file cadstar_pcb_archive_plugin.h
 * @brief Pcbnew PLUGIN for CADSTAR PCB Archive (*.cpa) format: an ASCII format
 *        based on S-expressions.
 */

#ifndef CADSTAR_SCH_ARCHIVE_PLUGIN_H_
#define CADSTAR_SCH_ARCHIVE_PLUGIN_H_

#include <map>
#include <sch_io_mgr.h>
#include <reporter.h>
#include <lib_symbol.h>


class SCH_SHEET;
class SCH_SCREEN;

class CADSTAR_SCH_ARCHIVE_PLUGIN : public SCH_PLUGIN
{
public:
    CADSTAR_SCH_ARCHIVE_PLUGIN() :
        m_cacheTimestamp( 0 )
    {
        m_reporter = &WXLOG_REPORTER::GetInstance();
        m_progressReporter = nullptr;
    }

    virtual ~CADSTAR_SCH_ARCHIVE_PLUGIN() {}

    const wxString GetName() const override;

    void SetReporter( REPORTER* aReporter ) override { m_reporter = aReporter; }

    void SetProgressReporter( PROGRESS_REPORTER* aReporter ) override
    {
        m_progressReporter = aReporter;
    }

    const wxString GetFileExtension() const override;

    const wxString GetLibraryFileExtension() const override;

    int GetModifyHash() const override;

    SCH_SHEET* Load( const wxString& aFileName, SCHEMATIC* aSchematic,
                     SCH_SHEET* aAppendToMe = nullptr,
                     const STRING_UTF8_MAP* aProperties = nullptr ) override;

    bool CheckHeader( const wxString& aFileName ) override;

    void EnumerateSymbolLib( wxArrayString&    aSymbolNameList,
                             const wxString&   aLibraryPath,
                             const STRING_UTF8_MAP* aProperties = nullptr ) override;

    void EnumerateSymbolLib( std::vector<LIB_SYMBOL*>& aSymbolList,
                             const wxString&           aLibraryPath,
                             const STRING_UTF8_MAP*         aProperties = nullptr ) override;

    LIB_SYMBOL* LoadSymbol( const wxString& aLibraryPath, const wxString& aAliasName,
                            const STRING_UTF8_MAP* aProperties = nullptr ) override;

    void GetAvailableSymbolFields( std::vector<wxString>& aNames ) override;


    // Writing to CADSTAR libraries is not supported
    bool IsSymbolLibWritable( const wxString& aLibraryPath ) override
    {
        return false;
    }

    void SymbolLibOptions( STRING_UTF8_MAP* aListToAppendTo ) const override;

private:
    // Symbol caching
    void ensureLoadedLibrary( const wxString& aLibraryPath, const STRING_UTF8_MAP* aProperties );

    typedef std::map<const wxString, std::unique_ptr<LIB_SYMBOL>> NAME_TO_SYMBOL_MAP;

    NAME_TO_SYMBOL_MAP m_libCache;
    long long          m_cacheTimestamp;
    wxString           m_cachePath;
    wxFileName         m_cachecsafn;
    wxString           m_cachefplibname;

    REPORTER* m_reporter; // current reporter for warnings/errors
    PROGRESS_REPORTER* m_progressReporter;  // optional; may be nullptr
};

#endif // CADSTAR_SCH_ARCHIVE_PLUGIN_H_
