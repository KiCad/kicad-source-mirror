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

#include <sim/sim_model_subckt.h>
#include <pegtl.hpp>
#include <pegtl/contrib/parse_tree.hpp>


namespace SIM_MODEL_SUBCKT_SPICE_PARSER
{
    using namespace SPICE_GRAMMAR;

    template <typename Rule> struct spiceUnitSelector : std::false_type {};

    template <> struct spiceUnitSelector<dotSubckt> : std::true_type {};
    template <> struct spiceUnitSelector<modelName> : std::true_type {};
    template <> struct spiceUnitSelector<dotSubcktPinName> : std::true_type {};
    template <> struct spiceUnitSelector<param> : std::true_type {};
    template <> struct spiceUnitSelector<number<SIM_VALUE::TYPE::INT, NOTATION::SPICE>>
        : std::true_type {};
    template <> struct spiceUnitSelector<number<SIM_VALUE::TYPE::FLOAT, NOTATION::SPICE>>
        : std::true_type {};
}


SIM_MODEL_SUBCKT::SIM_MODEL_SUBCKT( TYPE aType )
    : SIM_MODEL( aType )
{
}


bool SIM_MODEL_SUBCKT::ReadSpiceCode( const std::string& aSpiceCode )
{
    tao::pegtl::string_input<> in( aSpiceCode, "from_content" );
    std::unique_ptr<tao::pegtl::parse_tree::node> root;

    try
    {
        root = tao::pegtl::parse_tree::parse<SIM_MODEL_SUBCKT_SPICE_PARSER::spiceUnitGrammar,
                                             SIM_MODEL_SUBCKT_SPICE_PARSER::spiceUnitSelector>
            ( in );
    }
    catch( const tao::pegtl::parse_error& e )
    {
        return false;
    }

    wxASSERT( root );

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
                    AddPin( { subnode->string(), GetPinCount() + 1 } );
                }
                else if( subnode->is_type<SIM_MODEL_SUBCKT_SPICE_PARSER::param>() )
                {
                    m_paramInfos.push_back( std::make_unique<PARAM::INFO>() );
                    m_paramInfos.back()->name = subnode->string();
                    m_paramInfos.back()->isInstanceParam = true;
                    m_paramInfos.back()->isSpiceInstanceParam = true;

                    AddParam( *m_paramInfos.back() );
                }
                else if( subnode->is_type<
                        SIM_MODEL_SUBCKT_SPICE_PARSER::number<SIM_VALUE::TYPE::INT,
                                                        SIM_MODEL_SUBCKT_SPICE_PARSER::NOTATION::SPICE>>()
                    || subnode->is_type<
                        SIM_MODEL_SUBCKT_SPICE_PARSER::number<SIM_VALUE::TYPE::FLOAT,
                                                        SIM_MODEL_SUBCKT_SPICE_PARSER::NOTATION::SPICE>>() )
                {
                    wxASSERT( m_paramInfos.size() > 0 );
                    m_paramInfos.back()->defaultValue = subnode->string();
                }
            }
        }
        else
        {
            wxFAIL_MSG( "Unhandled parse tree node" );
            return false;
        }
    }

    m_spiceCode = aSpiceCode;
    return true;
}


wxString SIM_MODEL_SUBCKT::GenerateSpiceModelLine( const wxString& aModelName ) const
{
    return "";
}


std::vector<wxString> SIM_MODEL_SUBCKT::GenerateSpiceCurrentNames( const wxString& aRefName ) const
{
    std::vector<wxString> currentNames;

    for( unsigned i = 0; i < GetPinCount(); ++i )
        currentNames.push_back( wxString::Format( "I(%s:%s)",
                                                  GenerateSpiceItemName( aRefName ),
                                                  GetPin( i ).name ) );

    return currentNames;
}


void SIM_MODEL_SUBCKT::SetBaseModel( const SIM_MODEL& aBaseModel )
{
    SIM_MODEL::SetBaseModel( aBaseModel );

    // Pins aren't constant for subcircuits, so they need to be copied from the base model.
    for( unsigned i = 0; i < GetBaseModel()->GetPinCount(); ++i )
        AddPin( GetBaseModel()->GetPin( i ) );

    // Same for parameters.
    for( unsigned i = 0; i < GetBaseModel()->GetParamCount(); ++i )
        AddParam( GetBaseModel()->GetParam( i ).info );
}
