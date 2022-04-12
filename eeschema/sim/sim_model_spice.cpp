/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2022 Mikolaj Wielgus
 * Copyright (C) 2022 KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 3
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, you may find one here:
 * https://www.gnu.org/licenses/gpl-3.0.html
 * or you may search the http://www.gnu.org website for the version 3 license,
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

#include <sim/sim_model_spice.h>
#include <pegtl.hpp>
#include <pegtl/contrib/parse_tree.hpp>


SIM_MODEL_RAWSPICE::SIM_MODEL_RAWSPICE( TYPE aType )
    : SIM_MODEL( aType )
{
}


bool SIM_MODEL_RAWSPICE::setParamFromSpiceCode( const wxString& aParamName,
                                                const wxString& aParamValue,
                                                SIM_VALUE_GRAMMAR::NOTATION aNotation )
{
    int i = 0;

    for(; i < GetParamCount(); ++i )
    {
        if( GetParam( i ).info.name == aParamName.Lower() )
            break;
    }


    if( i == GetParamCount() )
    {
        // No parameter with this name found. Create a new one.
        std::unique_ptr<PARAM::INFO> paramInfo = std::make_unique<PARAM::INFO>();

        paramInfo->name = aParamName.Lower();
        paramInfo->type = SIM_VALUE::TYPE::STRING;
        m_paramInfos.push_back( std::move( paramInfo ) );

        AddParam( *m_paramInfos.back() );
    }

    try
    {
        GetParam( i ).value->FromString( wxString( aParamValue ), aNotation );
    }
    catch( const KI_PARAM_ERROR& e )
    {
        // Shouldn't happen since it's TYPE::STRING.
        return false;
    }

    return true;
}
