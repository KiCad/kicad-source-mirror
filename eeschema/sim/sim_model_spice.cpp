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
#include <sim/sim_model_raw_spice.h>
#include <sim/spice_model_parser.h>
#include <confirm.h>

#include <boost/algorithm/string/trim.hpp>


std::string SPICE_GENERATOR_SPICE::Preview( const std::string& aModelName ) const
{
    std::string spiceCode = ModelLine( aModelName );

    if( spiceCode == "" )
        spiceCode = static_cast<const SIM_MODEL_SPICE&>( m_model ).m_spiceCode;

    if( spiceCode == "" && m_model.GetBaseModel() )
        spiceCode = static_cast<const SIM_MODEL_SPICE*>( m_model.GetBaseModel() )->m_spiceCode;

    std::string itemLine = ItemLine( "", aModelName );

    if( spiceCode != "" )
        spiceCode.append( "\n" );

    spiceCode.append( itemLine );
    return boost::trim_copy( spiceCode );
}


std::unique_ptr<SIM_MODEL_SPICE> SIM_MODEL_SPICE::Create( const std::string& aSpiceCode )
{
    auto model = static_cast<SIM_MODEL_SPICE*>(
            SIM_MODEL::Create( SPICE_MODEL_PARSER::ReadType( aSpiceCode ) ).release() );

    try
    {
        model->m_spiceModelParser->ReadModel( aSpiceCode );
    }
    catch( const IO_ERROR& e )
    {
        DisplayErrorMessage( nullptr, e.What() );
    }

    return std::unique_ptr<SIM_MODEL_SPICE>( model );
}


SIM_MODEL_SPICE::SIM_MODEL_SPICE( TYPE aType,
                                  std::unique_ptr<SPICE_GENERATOR> aSpiceGenerator ) :
    SIM_MODEL( aType, std::move( aSpiceGenerator ) ),
    m_spiceModelParser( std::make_unique<SPICE_MODEL_PARSER>( *this ) )
{
}


SIM_MODEL_SPICE::SIM_MODEL_SPICE( TYPE aType,
                                  std::unique_ptr<SPICE_GENERATOR> aSpiceGenerator,
                                  std::unique_ptr<SPICE_MODEL_PARSER> aSpiceModelParser ) :
    SIM_MODEL( aType, std::move( aSpiceGenerator ) ),
    m_spiceModelParser( std::move( aSpiceModelParser ) )
{
}


bool SIM_MODEL_SPICE::SetParamValue( unsigned aParamIndex, const std::string& aParamValue,
                                     SIM_VALUE_GRAMMAR::NOTATION aNotation )
{
    // Models sourced from a library are immutable.
    if( m_spiceCode != "" )
        return false;

    return SIM_MODEL::SetParamValue( aParamIndex, aParamValue, aNotation );
}


bool SIM_MODEL_SPICE::SetParamFromSpiceCode( const std::string& aParamName,
                                             const std::string& aParamValue,
                                             SIM_VALUE_GRAMMAR::NOTATION aNotation )
{
    return SIM_MODEL::SetParamValue( aParamName, aParamValue, aNotation );
}
