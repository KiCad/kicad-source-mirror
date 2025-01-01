/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2023 Alex Shvartzkop <dudesuchamazing@gmail.com>
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

#ifndef PCB_EASYEDAPRO_PLUGIN_H_
#define PCB_EASYEDAPRO_PLUGIN_H_

#include <nlohmann/json_fwd.hpp>

#include <io/common/plugin_common_choose_project.h>
#include <pcb_io/pcb_io.h>
#include <pcb_io/pcb_io_mgr.h>


class PCB_IO_EASYEDAPRO : public PCB_IO, public PROJECT_CHOOSER_PLUGIN
{
public:
    const IO_BASE::IO_FILE_DESC GetBoardFileDesc() const override
    {
        return IO_BASE::IO_FILE_DESC( _HKI( "EasyEDA (JLCEDA) Pro project" ), { "epro", "zip" } );
    }

    const IO_BASE::IO_FILE_DESC GetLibraryDesc() const override
    {
        return IO_BASE::IO_FILE_DESC( _HKI( "EasyEDA (JLCEDA) Pro project" ),
                                      { "elibz", "epro", "zip" } );
    }

    const IO_BASE::IO_FILE_DESC GetLibraryFileDesc() const override
    {
        return IO_BASE::IO_FILE_DESC( _HKI( "EasyEDA (JLCEDA) Pro files" ),
                                      { "elibz", "efoo", "epro", "zip" } );
    }

    bool CanReadBoard( const wxString& aFileName ) const override;

    BOARD* LoadBoard( const wxString& aFileName, BOARD* aAppendToMe,
                      const std::map<std::string, UTF8>* aProperties = nullptr, PROJECT* aProject = nullptr ) override;

    long long GetLibraryTimestamp( const wxString& aLibraryPath ) const override;

    void FootprintEnumerate( wxArrayString& aFootprintNames, const wxString& aLibraryPath,
                             bool                   aBestEfforts,
                             const std::map<std::string, UTF8>* aProperties = nullptr ) override;

    std::vector<FOOTPRINT*> GetImportedCachedLibraryFootprints() override;

    FOOTPRINT* FootprintLoad( const wxString& aLibraryPath, const wxString& aFootprintName,
                              bool                   aKeepUUID = false,
                              const std::map<std::string, UTF8>* aProperties = nullptr ) override;

    bool IsLibraryWritable( const wxString& aLibraryPath ) override { return false; }

    PCB_IO_EASYEDAPRO();
    ~PCB_IO_EASYEDAPRO();

private:
    struct PRJ_DATA; // Opaque data structure
    PRJ_DATA* m_projectData = nullptr;

    void LoadAllDataFromProject( const wxString& aLibraryPath, const nlohmann::json& aProject );
};


#endif // PCB_EASYEDAPRO_PLUGIN_H_
