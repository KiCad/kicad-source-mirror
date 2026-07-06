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

#ifndef PCB_IO_EASYEDAPRO_V3_H_
#define PCB_IO_EASYEDAPRO_V3_H_

#include <io/common/plugin_common_choose_project.h>
#include <pcb_io/pcb_io.h>

#include <memory>

namespace EASYEDAPRO
{
class V3_DOC_PARSER;
}


class PCB_IO_EASYEDAPRO_V3 : public PCB_IO, public PROJECT_CHOOSER_PLUGIN
{
public:
    PCB_IO_EASYEDAPRO_V3();
    ~PCB_IO_EASYEDAPRO_V3() override;

    const IO_BASE::IO_FILE_DESC GetBoardFileDesc() const override
    {
        return IO_BASE::IO_FILE_DESC( _HKI( "EasyEDA (JLCEDA) Pro v3 project" ), { "epro2" } );
    }

    const IO_BASE::IO_FILE_DESC GetLibraryDesc() const override
    {
        return IO_BASE::IO_FILE_DESC( _HKI( "EasyEDA (JLCEDA) Pro v3 files" ), { "elibz2" } );
    }

    bool CanReadBoard( const wxString& aFileName ) const override;

    bool CanReadLibrary( const wxString& aFileName ) const override;

    BOARD* LoadBoard( const wxString& aFileName, BOARD* aAppendToMe,
                      const std::map<std::string, UTF8>* aProperties = nullptr, PROJECT* aProject = nullptr ) override;

    long long GetLibraryTimestamp( const wxString& aLibraryPath ) const override;

    void FootprintEnumerate( wxArrayString& aFootprintNames, const wxString& aLibraryPath, bool aBestEfforts,
                             const std::map<std::string, UTF8>* aProperties = nullptr ) override;

    FOOTPRINT* FootprintLoad( const wxString& aLibraryPath, const wxString& aFootprintName, bool aKeepUUID = false,
                              const std::map<std::string, UTF8>* aProperties = nullptr ) override;

    bool IsLibraryWritable( const wxString& aLibraryPath ) override { return false; }

private:
    const EASYEDAPRO::V3_DOC_PARSER& getCachedLibraryParser( const wxString& aLibraryPath ) const;

    mutable wxString                                   m_cachedLibraryPath;
    mutable long long                                  m_cachedLibraryTimestamp = -1;
    mutable std::unique_ptr<EASYEDAPRO::V3_DOC_PARSER> m_cachedLibraryParser;
};


#endif // PCB_IO_EASYEDAPRO_V3_H_
