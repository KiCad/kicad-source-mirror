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

#ifndef SCH_IO_EASYEDAPRO_V3_H_
#define SCH_IO_EASYEDAPRO_V3_H_

#include <io/common/plugin_common_choose_project.h>
#include <sch_io/sch_io.h>

#include <memory>


class SCH_SHEET;
class SCH_SCREEN;

namespace EASYEDAPRO
{
class V3_DOC_PARSER;
}


class SCH_IO_EASYEDAPRO_V3 : public SCH_IO, public PROJECT_CHOOSER_PLUGIN
{
public:
    SCH_IO_EASYEDAPRO_V3();
    ~SCH_IO_EASYEDAPRO_V3() override;

    const IO_BASE::IO_FILE_DESC GetSchematicFileDesc() const override
    {
        return IO_BASE::IO_FILE_DESC( _HKI( "EasyEDA (JLCEDA) Pro v3 files" ), { "epro2" } );
    }

    const IO_BASE::IO_FILE_DESC GetLibraryDesc() const override
    {
        return IO_BASE::IO_FILE_DESC( _HKI( "EasyEDA (JLCEDA) Pro v3 files" ), { "elibz2" } );
    }

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
    const EASYEDAPRO::V3_DOC_PARSER& getCachedLibraryParser( const wxString& aLibraryPath ) const;

    mutable wxString                                   m_cachedLibraryPath;
    mutable long long                                  m_cachedLibraryTimestamp = -1;
    mutable std::unique_ptr<EASYEDAPRO::V3_DOC_PARSER> m_cachedLibraryParser;
};


#endif // SCH_IO_EASYEDAPRO_V3_H_
