/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
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

#include "sim/sim_model_spice_fallback.h"

#include <ki_exception.h>
#include <fmt/format.h>


SIM_MODEL_SPICE_FALLBACK::SIM_MODEL_SPICE_FALLBACK( TYPE aType, const std::string& aRawSpiceCode ) :
        SIM_MODEL_SPICE( aType, std::make_unique<SPICE_GENERATOR_SPICE>( *this ) )
{
    // Create the model we *should* have had to copy its parameter list
    std::unique_ptr<SIM_MODEL> model = SIM_MODEL::Create( aType );

    for( int ii = 0; ii < GetParamCount(); ++ii )
        AddParam( GetParam( ii ).info );

    m_spiceCode = aRawSpiceCode;
}


void SIM_MODEL_SPICE_FALLBACK::AssignSymbolPinNumberToModelPin( const std::string& aModelPinName,
                                                                const wxString& aSymbolPinNumber )
{
    try
    {
        SIM_MODEL::AssignSymbolPinNumberToModelPin( aModelPinName, aSymbolPinNumber );
    }
    catch( IO_ERROR& )
    {
        // This is a fall-back, so we won't necessarily know the pin names.  If we didn't find
        // it, then just create a new pin.
        m_modelPins.push_back( { aModelPinName, aSymbolPinNumber } );
    }
}


std::vector<std::string> SIM_MODEL_SPICE_FALLBACK::GetPinNames() const
{
    // If we're a fall-back for a paticular model type, then return its pin names
    std::unique_ptr<SIM_MODEL> model = SIM_MODEL::Create( GetType() );
    return model->GetPinNames();
}


int SIM_MODEL_SPICE_FALLBACK::doFindParam( const std::string& aParamName ) const
{
    for( int ii = 0; ii < GetParamCount(); ++ii )
    {
        const SIM_MODEL::PARAM& param = GetParam( ii );

        if( param.Matches( aParamName ) )
            return ii;
    }

    // Look for escaped param names as a second pass (as they're less common)
    for( int ii = 0; ii < GetParamCount(); ++ii )
    {
        const SIM_MODEL::PARAM& param = GetParam( ii );

        if( !param.info.name.ends_with( '_' ) )
            continue;

        if( param.Matches( aParamName + "_" ) )
            return ii;
    }

    return -1;
}


