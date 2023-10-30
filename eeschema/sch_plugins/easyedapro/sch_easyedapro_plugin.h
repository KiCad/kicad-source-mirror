/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2023 Alex Shvartzkop <dudesuchamazing@gmail.com>
 * Copyright (C) 2023 KiCad Developers, see AUTHORS.txt for contributors.
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

#ifndef SCH_EASYEDAPRO_PLUGIN_H_
#define SCH_EASYEDAPRO_PLUGIN_H_

#include <sch_io_mgr.h>
#include <reporter.h>
#include <plugins/common/plugin_common_choose_project.h>


class SCH_SHEET;
class SCH_SCREEN;


class SCH_EASYEDAPRO_PLUGIN : public SCH_PLUGIN, public PROJECT_CHOOSER_PLUGIN
{
public:
    SCH_EASYEDAPRO_PLUGIN();
    ~SCH_EASYEDAPRO_PLUGIN();

    const wxString GetName() const override;

    void SetReporter( REPORTER* aReporter ) override { m_reporter = aReporter; }

    void SetProgressReporter( PROGRESS_REPORTER* aReporter ) override
    {
        m_progressReporter = aReporter;
    }

    const PLUGIN_FILE_DESC GetSchematicFileDesc() const override
    {
        return PLUGIN_FILE_DESC( _HKI( "EasyEDA (JLCEDA) Pro files" ), { "epro", "zip" } );
    }

    const PLUGIN_FILE_DESC GetLibraryFileDesc() const override { return GetSchematicFileDesc(); }

    bool CanReadSchematicFile( const wxString& aFileName ) const override;

    int GetModifyHash() const override;

    SCH_SHEET* LoadSchematicFile( const wxString& aFileName, SCHEMATIC* aSchematic,
                                  SCH_SHEET*             aAppendToMe = nullptr,
                                  const STRING_UTF8_MAP* aProperties = nullptr ) override;

    void EnumerateSymbolLib( wxArrayString& aSymbolNameList, const wxString& aLibraryPath,
                             const STRING_UTF8_MAP* aProperties = nullptr ) override;

    void EnumerateSymbolLib( std::vector<LIB_SYMBOL*>& aSymbolList, const wxString& aLibraryPath,
                             const STRING_UTF8_MAP* aProperties = nullptr ) override;

    void LoadAllDataFromProject( const wxString& aLibraryPath );

    LIB_SYMBOL* LoadSymbol( const wxString& aLibraryPath, const wxString& aAliasName,
                            const STRING_UTF8_MAP* aProperties = nullptr ) override;

    bool IsSymbolLibWritable( const wxString& aLibraryPath ) override { return false; }

private:
    struct PRJ_DATA; // Opaque data structure
    PRJ_DATA* m_projectData = nullptr;

    REPORTER*          m_reporter;         // current reporter for warnings/errors
    PROGRESS_REPORTER* m_progressReporter; // optional; may be nullptr
};


#endif // SCH_EASYEDAPRO_PLUGIN_H_
