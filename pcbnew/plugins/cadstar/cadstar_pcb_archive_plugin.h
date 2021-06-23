/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2020 Roberto Fernandez Bautista <roberto.fer.bau@gmail.com>
 * Copyright (C) 2020 KiCad Developers, see AUTHORS.txt for contributors.
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
 * @brief Pcbnew #PLUGIN for CADSTAR PCB Archive (*.cpa) format: an ASCII format
 *        based on S-expressions.
 */

#ifndef CADSTAR_ARCHIVE_PLUGIN_H_
#define CADSTAR_ARCHIVE_PLUGIN_H_


#include <io_mgr.h>
#include <layers_id_colors_and_visibility.h> // PCB_LAYER_ID
#include <plugins/common/plugin_common_layer_mapping.h>


class CADSTAR_PCB_ARCHIVE_PLUGIN : public PLUGIN, public LAYER_REMAPPABLE_PLUGIN
{
public:
    const wxString PluginName() const override;

    BOARD* Load( const wxString& aFileName, BOARD* aAppendToMe,
                 const PROPERTIES* aProperties = nullptr, PROJECT* aProject = nullptr,
                 PROGRESS_REPORTER* aProgressReporter = nullptr ) override;

    const wxString GetFileExtension() const override;

    long long GetLibraryTimestamp( const wxString& aLibraryPath ) const override
    {
        // No support for libraries....
        return 0;
    }

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
    void RegisterLayerMappingCallback( LAYER_MAPPING_HANDLER aLayerMappingHandler ) override;

    CADSTAR_PCB_ARCHIVE_PLUGIN();
    ~CADSTAR_PCB_ARCHIVE_PLUGIN();

private:
    void clearLoadedFootprints();

    const PROPERTIES*       m_props;
    BOARD*                  m_board;
    std::vector<FOOTPRINT*> m_loaded_footprints;
    bool                    m_show_layer_mapping_warnings;
};

#endif // CADSTAR_ARCHIVE_PLUGIN_H_
