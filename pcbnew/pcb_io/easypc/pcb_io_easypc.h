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

/**
 * @file pcb_io_easypc.h
 * @brief PCB_IO for Number One Systems Easy-PC and RS DesignSpark PCB boards and symbol libraries (.psl).
 */

#ifndef PCB_IO_EASYPC_H_
#define PCB_IO_EASYPC_H_

#include <map>
#include <memory>

#include <pcb_io/pcb_io.h>
#include <pcb_io/pcb_io_mgr.h>
#include <pcb_io/common/plugin_common_layer_mapping.h>

namespace EASYPC
{
class LIBRARY_FILE;
}


class PCB_IO_EASYPC : public PCB_IO, public LAYER_MAPPABLE_PLUGIN
{
public:
    PCB_IO_EASYPC();
    ~PCB_IO_EASYPC() override;

    const IO_BASE::IO_FILE_DESC GetBoardFileDesc() const override
    {
        return IO_BASE::IO_FILE_DESC( _HKI( "Easy-PC and DesignSpark PCB files" ), { "pcb" } );
    }

    const IO_BASE::IO_FILE_DESC GetLibraryDesc() const override
    {
        return IO_BASE::IO_FILE_DESC( _HKI( "Easy-PC and DesignSpark PCB symbol libraries" ), { "psl" } );
    }

    bool CanReadBoard( const wxString& aFileName ) const override;
    bool CanReadLibrary( const wxString& aFileName ) const override;
    bool CanReadFootprint( const wxString& aFileName ) const override { return CanReadLibrary( aFileName ); }

    long long GetLibraryTimestamp( const wxString& aLibraryPath ) const override;

    void FootprintEnumerate( wxArrayString& aFootprintNames, const wxString& aLibraryPath, bool aBestEfforts,
                             const std::map<std::string, UTF8>* aProperties = nullptr ) override;

    std::unique_ptr<FOOTPRINT> FootprintLoad( const wxString& aLibraryPath, const wxString& aFootprintName,
                                              bool                               aKeepUUID = false,
                                              const std::map<std::string, UTF8>* aProperties = nullptr ) override;

    const FOOTPRINT* GetEnumeratedFootprint( const wxString& aLibraryPath, const wxString& aFootprintName,
                                             const std::map<std::string, UTF8>* aProperties = nullptr ) override;

    bool FootprintExists( const wxString& aLibraryPath, const wxString& aFootprintName,
                          const std::map<std::string, UTF8>* aProperties = nullptr ) override;

    bool CachesEnumeratedFootprints() const override { return true; }

    bool IsLibraryWritable( const wxString& aLibraryPath ) override { return false; }

    wxString GetImportedDesignRules() const override { return m_importedDesignRules; }

protected:
    void loadBoard( const wxString& aFileName, BOARD& aBoard, bool aIsNewLoad,
                    const std::map<std::string, UTF8>* aProperties = nullptr, PROJECT* aProject = nullptr ) override;

private:
    struct LIBRARY
    {
        long long                                      Timestamp = 0;
        std::unique_ptr<EASYPC::LIBRARY_FILE>          File;
        std::map<wxString, std::unique_ptr<FOOTPRINT>> Footprints;
    };

    /// The parsed library, reread when its modification time changes
    LIBRARY& library( const wxString& aLibraryPath );

    wxString                    m_importedDesignRules;
    std::map<wxString, LIBRARY> m_libraries;
};

#endif // PCB_IO_EASYPC_H_
