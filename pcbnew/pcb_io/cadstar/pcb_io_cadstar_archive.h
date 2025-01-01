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

#ifndef CADSTAR_ARCHIVE_PLUGIN_H_
#define CADSTAR_ARCHIVE_PLUGIN_H_

#include <pcb_io/pcb_io.h>
#include <pcb_io/pcb_io_mgr.h>
#include <pcb_io/common/plugin_common_layer_mapping.h>
#include <layer_ids.h> // PCB_LAYER_ID

#include <memory>


class PCB_IO_CADSTAR_ARCHIVE : public PCB_IO, public LAYER_MAPPABLE_PLUGIN
{
public:
    const IO_BASE::IO_FILE_DESC GetBoardFileDesc() const override
    {
        return IO_BASE::IO_FILE_DESC( _HKI( "CADSTAR PCB Archive files" ), { "cpa" } );
    }

    const IO_BASE::IO_FILE_DESC GetLibraryDesc() const override { return GetBoardFileDesc(); }

    bool CanReadBoard( const wxString& aFileName ) const override;
    bool CanReadLibrary( const wxString& aFileName ) const override;
    bool CanReadFootprint( const wxString& aFileName ) const override;

    BOARD* LoadBoard( const wxString& aFileName, BOARD* aAppendToMe,
                      const std::map<std::string, UTF8>* aProperties = nullptr, PROJECT* aProject = nullptr ) override;

    std::vector<FOOTPRINT*> GetImportedCachedLibraryFootprints() override;

    /**
     * Return the automapped layers.
     *
     * @param aInputLayerDescriptionVector
     * @return Auto-mapped layers
     */
    static std::map<wxString, PCB_LAYER_ID> DefaultLayerMappingCallback(
            const std::vector<INPUT_LAYER_DESC>& aInputLayerDescriptionVector );

    /**
     * Register a different handler to be called when mapping of Cadstar to KiCad layers occurs.
     *
     * @param aLayerMappingHandler
     */
    void RegisterCallback( LAYER_MAPPING_HANDLER aLayerMappingHandler ) override;

    void FootprintEnumerate( wxArrayString& aFootprintNames, const wxString& aLibraryPath,
                             bool aBestEfforts, const std::map<std::string, UTF8>* aProperties = nullptr ) override;

    // default implementation of GetEnumeratedFootprint is fine (call FootprintLoad)

    bool FootprintExists( const wxString& aLibraryPath, const wxString& aFootprintName,
                          const std::map<std::string, UTF8>* aProperties = nullptr ) override;

    FOOTPRINT* FootprintLoad( const wxString& aLibraryPath, const wxString& aFootprintName,
                              bool  aKeepUUID = false,
                              const std::map<std::string, UTF8>* aProperties = nullptr ) override;

    long long GetLibraryTimestamp( const wxString& aLibraryPath ) const override;

    /**
     * CADSTAR Plugin is read-only
     * @return Always false
     */
    bool IsLibraryWritable( const wxString& aLibraryPath ) override { return false; }

    PCB_IO_CADSTAR_ARCHIVE();
    ~PCB_IO_CADSTAR_ARCHIVE();

private:
    void clearLoadedFootprints();
    void ensureLoadedLibrary( const wxString& aLibraryPath );

    typedef std::map<const wxString, std::unique_ptr<FOOTPRINT>> NAME_TO_FOOTPRINT_MAP;

    std::map<wxString, NAME_TO_FOOTPRINT_MAP> m_cache;
    std::map<wxString, long long> m_timestamps;

    std::vector<FOOTPRINT*> m_loaded_footprints;
    bool                    m_show_layer_mapping_warnings;

    bool checkBoardHeader( const wxString& aFileName ) const;
};

#endif // CADSTAR_ARCHIVE_PLUGIN_H_
