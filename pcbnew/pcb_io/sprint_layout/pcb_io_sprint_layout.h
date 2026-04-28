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
 *
 * This file contains file format knowledge derived from the xlay project:
 *   https://github.com/sergey-raevskiy/xlay
 * and the OpenBoardView project:
 *   https://github.com/OpenBoardView/OpenBoardView
 */

#ifndef PCB_IO_SPRINT_LAYOUT_H_
#define PCB_IO_SPRINT_LAYOUT_H_

#include <map>
#include <memory>
#include <string>
#include <vector>

#include <io/common/plugin_common_choose_project.h>
#include <pcb_io/pcb_io.h>
#include <pcb_io/pcb_io_mgr.h>

class BOARD;
class FOOTPRINT;

class PCB_IO_SPRINT_LAYOUT : public PCB_IO, public PROJECT_CHOOSER_PLUGIN
{
public:
    const IO_BASE::IO_FILE_DESC GetBoardFileDesc() const override
    {
        return IO_BASE::IO_FILE_DESC( _HKI( "Sprint Layout board file" ), { "lay6", "lay" } );
    }

    const IO_BASE::IO_FILE_DESC GetLibraryFileDesc() const override
    {
        return IO_BASE::IO_FILE_DESC( _HKI( "Sprint Layout macro file" ), { "lmk" } );
    }

    const IO_BASE::IO_FILE_DESC GetLibraryDesc() const override
    {
        return IO_BASE::IO_FILE_DESC( _HKI( "Sprint Layout macro library" ), {}, { "lmk" }, false );
    }

    void FootprintEnumerate( wxArrayString& aFootprintNames, const wxString& aLibraryPath, bool aBestEfforts,
                             const std::map<std::string, UTF8>* aProperties = nullptr ) override;

    FOOTPRINT* FootprintLoad( const wxString& aLibraryPath, const wxString& aFootprintName, bool aKeepUUID = false,
                              const std::map<std::string, UTF8>* aProperties = nullptr ) override;

    bool CanReadBoard( const wxString& aFileName ) const override;

    BOARD* LoadBoard( const wxString& aFileName, BOARD* aAppendToMe,
                      const std::map<std::string, UTF8>* aProperties = nullptr, PROJECT* aProject = nullptr ) override;

    std::vector<FOOTPRINT*> GetImportedCachedLibraryFootprints() override;

    long long GetLibraryTimestamp( const wxString& aLibraryPath ) const override { return 0; }

    bool IsLibraryWritable( const wxString& aLibraryPath ) override { return false; }

    PCB_IO_SPRINT_LAYOUT();
    ~PCB_IO_SPRINT_LAYOUT() override;

private:
    std::map<wxString, std::unique_ptr<FOOTPRINT>> m_loadedFootprints;
};

#endif // PCB_IO_SPRINT_LAYOUT_H_
