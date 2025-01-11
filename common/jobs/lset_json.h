/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2024 Mark Roszko <mark.roszko@gmail.com>
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

#include <lset.h>
#include <nlohmann/json_fwd.hpp>


void to_json( nlohmann::json& aJson, const LSET& aLset )
{
    nlohmann::json layers = nlohmann::json::array();

    for( PCB_LAYER_ID layer : aLset.Seq() )
        layers.push_back( LSET::Name( layer ) );

    aJson = layers;
}


void from_json( const nlohmann::json& aJson, LSET& aLset )
{
    if( aJson.is_array() )
    {
        aLset.clear();

        for( const nlohmann::json& layer : aJson )
        {
            if( layer.is_string() )
            {
                wxString name = layer.get<wxString>();
                int      layerId = LSET::NameToLayer( name );
                if( layerId != UNDEFINED_LAYER )
                    aLset.set( layerId );
            }
        }
    }
    else if( aJson.is_string() )
    {
        // Allow hex strings to be parsed into LSETs
        aLset.ParseHex( aJson.get<std::string>() );
    }
}