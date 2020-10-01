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
 * @brief Pcbnew PLUGIN for CADSTAR PCB Archive (*.cpa) format: an ASCII format
 *        based on S-expressions.
 */

#ifndef CADSTAR_ARCHIVE_PLUGIN_H_
#define CADSTAR_ARCHIVE_PLUGIN_H_


#include <io_mgr.h>
#include <layers_id_colors_and_visibility.h> // PCB_LAYER_ID


/**
 * @brief Describes an imported layer and how it could be mapped to KiCad Layers
 */
struct INPUT_LAYER_DESC
{
    wxString     Name;             ///< Imported layer name as displayed in original application.
    LSET         PermittedLayers;  ///< KiCad layers that the imported layer can be mapped onto.
    PCB_LAYER_ID AutoMapLayer;     ///< Best guess as to what the equivalent KiCad layer might be.

    INPUT_LAYER_DESC()
    {
        Name            = wxEmptyString;
        PermittedLayers = LSET();
        AutoMapLayer    = PCB_LAYER_ID::UNDEFINED_LAYER;
    }
};

/**
 * A CADSTAR layer name.
 */
typedef wxString INPUT_LAYER_NAME;

/**
 * @brief Map of CADSTAR (INPUT_LAYER_NAME) to KiCad Layers.
 * If the mapped KiCad layer is UNDEFINED_LAYER, then the CADSTAR layer will not
 * be imported
 */
typedef std::map<INPUT_LAYER_NAME, PCB_LAYER_ID> LAYER_MAP;

/**
 * @brief Pointer to a function that takes a map of Cadstar and KiCad layers
 * and returns a re-mapped version. If the re-mapped layer
 */
typedef std::function<LAYER_MAP( const std::vector<INPUT_LAYER_DESC>& )> LAYER_MAPPING_HANDLER;


class CADSTAR_PCB_ARCHIVE_PLUGIN : public PLUGIN
{
public:
    // -----<PUBLIC PLUGIN API>--------------------------------------------------

    const wxString PluginName() const override;

    BOARD* Load( const wxString& aFileName, BOARD* aAppendToMe,
            const PROPERTIES* aProperties = NULL ) override;

    const wxString GetFileExtension() const override;

    long long GetLibraryTimestamp( const wxString& aLibraryPath ) const override
    {
        // No support for libraries....
        return 0;
    }

    // -----</PUBLIC PLUGIN API>-------------------------------------------------

    /**
     * @brief Default callback - just returns the automapped layers
     * @param aInputLayerDescriptionVector 
     * @return Auto-mapped layers
     */
    static LAYER_MAP DefaultLayerMappingCallback(
            const std::vector<INPUT_LAYER_DESC>& aInputLayerDescriptionVector );

    /**
     * @brief Register a different handler to be called when mapping of Cadstar to KiCad
     * layers occurs
     * @param aLayerMappingHandler 
     */
    void RegisterLayerMappingCallback( LAYER_MAPPING_HANDLER aLayerMappingHandler );

    CADSTAR_PCB_ARCHIVE_PLUGIN();
    ~CADSTAR_PCB_ARCHIVE_PLUGIN();

private:
    const PROPERTIES*     m_props;
    BOARD*                m_board;
    LAYER_MAPPING_HANDLER m_layer_mapping_handler;
    bool                  m_show_layer_mapping_warnings;
};

#endif // CADSTAR_ARCHIVE_PLUGIN_H_
