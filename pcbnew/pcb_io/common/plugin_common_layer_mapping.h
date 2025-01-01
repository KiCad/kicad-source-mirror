/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2020 Jan Mrázek <email@honzamrazek.cz>
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

#ifndef PLUGIN_COMMON_LAYER_MAPPING_H
#define PLUGIN_COMMON_LAYER_MAPPING_H

#include <functional>
#include <map>

#include <pcb_io/pcb_io_mgr.h>
#include <layer_ids.h> // PCB_LAYER_ID
#include <lset.h>

/**
 * @brief Describes an imported layer and how it could be mapped to KiCad Layers
 */
struct INPUT_LAYER_DESC
{
    wxString     Name;             ///< Imported layer name as displayed in original application.
    LSET         PermittedLayers;  ///< KiCad layers that the imported layer can be mapped onto.
    PCB_LAYER_ID AutoMapLayer;     ///< Best guess as to what the equivalent KiCad layer might be.
    bool         Required;         ///< Should we require the layer to be assigned?

    INPUT_LAYER_DESC()
            : Name( wxEmptyString ),
              PermittedLayers(),
              AutoMapLayer( PCB_LAYER_ID::UNDEFINED_LAYER ),
              Required( true )
    {
    }
};

/**
 * @brief Pointer to a function that takes a map of source and KiCad layers
 * and returns a re-mapped version. If the re-mapped layer is UNDEFINED_LAYER,
 * then the source layer will not be imported
 */
using LAYER_MAPPING_HANDLER = std::function<std::map<wxString, PCB_LAYER_ID>( const std::vector<INPUT_LAYER_DESC>& )>;


/**
 * @brief Plugin class for import plugins that support remappable layers
 */
class LAYER_MAPPABLE_PLUGIN
{
public:
    /**
     * @brief Register a different handler to be called when mapping of input
     * layers to KiCad layers occurs
     *
     * The function is marked as virtual, so the plugins can implement extra
     * logic (e.g., enable warnings or checks)
     *
     * @param aLayerMappingHandler
     */
    virtual void RegisterCallback( LAYER_MAPPING_HANDLER aLayerMappingHandler )
    {
        m_layer_mapping_handler = aLayerMappingHandler;
    }

    virtual ~LAYER_MAPPABLE_PLUGIN() = default;
protected:
    LAYER_MAPPING_HANDLER m_layer_mapping_handler; ///< Callback to get layer mapping
};

#endif // PLUGIN_COMMON_LAYER_MAPPING_H
