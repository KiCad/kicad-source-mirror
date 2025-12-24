/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2022 Mikolaj Wielgus
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

#include "sim/sim_model_subckt.h"

#include <ki_exception.h>
#include <fmt/core.h>
#include <pegtl.hpp>
#include <pegtl/contrib/parse_tree.hpp>
#include <sim/spice_grammar.h>


namespace SIM_MODEL_SUBCKT_SPICE_PARSER
{
    using namespace SPICE_GRAMMAR;

    template <typename Rule> struct spiceUnitSelector : std::false_type {};

    template <> struct spiceUnitSelector<dotSubckt> : std::true_type {};
    template <> struct spiceUnitSelector<modelName> : std::true_type {};
    template <> struct spiceUnitSelector<dotSubcktPinName> : std::true_type {};
    template <> struct spiceUnitSelector<dotSubcktParams> : std::true_type {};
    template <> struct spiceUnitSelector<param> : std::true_type {};
    template <> struct spiceUnitSelector<paramValue> : std::true_type {};
    template <> struct spiceUnitSelector<number<SIM_VALUE::TYPE_INT, NOTATION::SPICE>>
        : std::true_type {};
    template <> struct spiceUnitSelector<number<SIM_VALUE::TYPE_FLOAT, NOTATION::SPICE>>
        : std::true_type {};
}


std::string SPICE_GENERATOR_SUBCKT::ModelLine( const SPICE_ITEM& aItem ) const
{
    return "";
}


std::vector<std::string> SPICE_GENERATOR_SUBCKT::CurrentNames( const SPICE_ITEM& aItem ) const
{
    std::vector<std::string> currentNames;

    if( GetPins().size() == 2 )
    {
        currentNames.push_back( fmt::format( "I({})", ItemName( aItem ) ) );
    }
    else
    {
        for( const SIM_MODEL_PIN& pin : GetPins() )
            currentNames.push_back( fmt::format( "I({}:{})", ItemName( aItem ), pin.modelPinName ) );
    }

    return currentNames;
}


void SPICE_MODEL_PARSER_SUBCKT::ReadModel( const SIM_LIBRARY_SPICE& aLibrary,
                                           const std::string& aSpiceCode )
{
    tao::pegtl::string_input<> in( aSpiceCode, "from_content" );
    std::unique_ptr<tao::pegtl::parse_tree::node> root;

    try
    {
        root = tao::pegtl::parse_tree::parse<SIM_MODEL_SUBCKT_SPICE_PARSER::spiceUnitGrammar,
                                             SIM_MODEL_SUBCKT_SPICE_PARSER::spiceUnitSelector,
                                             tao::pegtl::nothing,
                                             SIM_MODEL_SUBCKT_SPICE_PARSER::control>( in );
    }
    catch( const tao::pegtl::parse_error& e )
    {
        THROW_IO_ERROR( e.what() );
    }

    SIM_MODEL_SUBCKT& model = static_cast<SIM_MODEL_SUBCKT&>( m_model );

    for( const auto& node : root->children )
    {
        if( node->is_type<SIM_MODEL_SUBCKT_SPICE_PARSER::dotSubckt>() )
        {
            for( const auto& subnode : node->children )
            {
                if( subnode->is_type<SIM_MODEL_SUBCKT_SPICE_PARSER::modelName>() )
                {
                }
                else if( subnode->is_type<SIM_MODEL_SUBCKT_SPICE_PARSER::dotSubcktPinName>() )
                {
                    model.AddPin( { subnode->string(), fmt::format( "{}", model.GetPinCount() + 1 ) } );
                }
                else if( subnode->is_type<SIM_MODEL_SUBCKT_SPICE_PARSER::dotSubcktParams>() )
                {
                    for( const auto& subsubnode : subnode->children )
                    {
                        if( subsubnode->is_type<SIM_MODEL_SUBCKT_SPICE_PARSER::param>() )
                        {
                            model.m_paramInfos.push_back( std::make_unique<SIM_MODEL::PARAM::INFO>() );
                            model.m_paramInfos.back()->name = subsubnode->string();
                            model.m_paramInfos.back()->isInstanceParam = true;
                            model.m_paramInfos.back()->isSpiceInstanceParam = true;

                            model.AddParam( *model.m_paramInfos.back() );
                        }
                        else if( subsubnode->is_type<SIM_MODEL_SUBCKT_SPICE_PARSER::paramValue>() )
                        {
                            wxASSERT( model.m_paramInfos.size() > 0 );
                            model.m_paramInfos.back()->defaultValue = subsubnode->string();
                        }
                        else
                        {
                            wxFAIL_MSG( "Unhandled parse tree subsubnode" );
                        }
                    }
                }
            }
        }
        else
        {
            wxFAIL_MSG( "Unhandled parse tree node" );
        }
    }

    model.m_spiceCode = aSpiceCode;
}


SIM_MODEL_SUBCKT::SIM_MODEL_SUBCKT() :
    SIM_MODEL_SPICE( TYPE::SUBCKT, std::make_unique<SPICE_GENERATOR_SUBCKT>( *this ),
                     std::make_unique<SPICE_MODEL_PARSER_SUBCKT>( *this ) )
{
}


void SIM_MODEL_SUBCKT::SetBaseModel( const SIM_MODEL& aBaseModel )
{
    SIM_MODEL::SetBaseModel( aBaseModel );

    // Pins aren't constant for subcircuits, so they need to be copied from the base model.
    for( const SIM_MODEL_PIN& pin : GetBaseModel()->GetPins() )
        AddPin( pin );

    // Same for parameters.
    for( int ii = 0; ii < GetBaseModel()->GetParamCount(); ++ii )
        AddParam( GetBaseModel()->GetParam( ii ).info );
}


std::string SIM_MODEL_SUBCKT::GetSpiceCode() const
{
    if( !m_spiceCode.empty() )
        return m_spiceCode;

    if( const SIM_MODEL_SUBCKT* baseModel = dynamic_cast<const SIM_MODEL_SUBCKT*>( m_baseModel ) )
        return baseModel->GetSpiceCode();

    return "";
}