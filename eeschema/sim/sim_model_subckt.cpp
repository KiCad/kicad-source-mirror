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
#include <sim/spice_grammar.h>
#include <pegtl.hpp>
#include <pegtl/contrib/parse_tree.hpp>


namespace SIM_MODEL_SUBCKT_SPICE_PARSER
{
    using namespace SPICE_GRAMMAR;

    template <typename Rule> struct spiceUnitSelector : std::false_type {};

    template <> struct spiceUnitSelector<dotSubckt> : std::true_type {};
    template <> struct spiceUnitSelector<modelName> : std::true_type {};
    template <> struct spiceUnitSelector<dotSubcktPinName> : std::true_type {};
    template <> struct spiceUnitSelector<paramValuePairs> : std::true_type {};
    template <> struct spiceUnitSelector<param> : std::true_type {};
    template <> struct spiceUnitSelector<number<SIM_VALUE::TYPE_INT, NOTATION::SPICE>>
        : std::true_type {};
    template <> struct spiceUnitSelector<number<SIM_VALUE::TYPE_FLOAT, NOTATION::SPICE>>
        : std::true_type {};
}


SIM_MODEL_SUBCKT::SIM_MODEL_SUBCKT( TYPE aType )
    : SIM_MODEL( aType )
{
}


void SIM_MODEL_SUBCKT::ReadSpiceCode( const wxString& aSpiceCode )
{
    tao::pegtl::string_input<> in( aSpiceCode.ToUTF8(), "from_content" );
    std::unique_ptr<tao::pegtl::parse_tree::node> root;

    try
    {
        root = tao::pegtl::parse_tree::parse<SIM_MODEL_SUBCKT_SPICE_PARSER::spiceUnitGrammar,
                                             SIM_MODEL_SUBCKT_SPICE_PARSER::spiceUnitSelector>
            ( in );
    }
    catch( const tao::pegtl::parse_error& e )
    {
        THROW_IO_ERROR( e.what() );
    }

    for( const auto& node : root->children )
    {
        if( node->is_type<SIM_MODEL_SUBCKT_SPICE_PARSER::dotSubckt>() )
        {
            bool hadParamValuePairs = false;

            for( const auto& subnode : node->children )
            {
                if( subnode->is_type<SIM_MODEL_SUBCKT_SPICE_PARSER::modelName>() )
                {
                }
                else if( subnode->is_type<SIM_MODEL_SUBCKT_SPICE_PARSER::dotSubcktPinName>() )
                {
                    AddPin( { subnode->string(), wxString::FromCDouble( GetPinCount() + 1 ) } );
                }
                else if( !hadParamValuePairs
                    && subnode->is_type<SIM_MODEL_SUBCKT_SPICE_PARSER::paramValuePairs>() )
                {
                    for( const auto& subsubnode : subnode->children )
                    {
                        if( subsubnode->is_type<SIM_MODEL_SUBCKT_SPICE_PARSER::param>() )
                        {
                            m_paramInfos.push_back( std::make_unique<PARAM::INFO>() );
                            m_paramInfos.back()->name = subsubnode->string();
                            m_paramInfos.back()->isInstanceParam = true;
                            m_paramInfos.back()->isSpiceInstanceParam = true;

                            AddParam( *m_paramInfos.back() );
                        }
                        else
                        {
                            wxFAIL_MSG( "Unhandled parse tree subsubnode" );
                        }
                    }

                    hadParamValuePairs = true;
                }
                else if( subnode->is_type<
                        SIM_MODEL_SUBCKT_SPICE_PARSER::number<SIM_VALUE::TYPE_INT,
                            SIM_MODEL_SUBCKT_SPICE_PARSER::NOTATION::SPICE>>()
                    || subnode->is_type<
                        SIM_MODEL_SUBCKT_SPICE_PARSER::number<SIM_VALUE::TYPE_FLOAT,
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
        }
    }

    m_spiceCode = aSpiceCode;
}


wxString SIM_MODEL_SUBCKT::GenerateSpiceModelLine( const wxString& aModelName ) const
{
    return "";
}


std::vector<wxString> SIM_MODEL_SUBCKT::GenerateSpiceCurrentNames( const wxString& aRefName ) const
{
    std::vector<wxString> currentNames;

    for( const PIN& pin : GetPins() )
    {
        currentNames.push_back( wxString::Format( "I(%s:%s)",
                                                  GenerateSpiceItemName( aRefName ),
                                                  pin.name ) );
    }

    return currentNames;
}


void SIM_MODEL_SUBCKT::SetBaseModel( const SIM_MODEL& aBaseModel )
{
    SIM_MODEL::SetBaseModel( aBaseModel );

    // Pins aren't constant for subcircuits, so they need to be copied from the base model.
    for( const PIN& pin : GetBaseModel()->GetPins() )
        AddPin( pin );

    // Same for parameters.
    for( const PARAM& param : GetBaseModel()->GetParams() )
        AddParam( param.info );
}


void SIM_MODEL_SUBCKT::CreatePins( unsigned aSymbolPinCount )
{
    SIM_MODEL::CreatePins( aSymbolPinCount );

    // Reset the pins to Not Connected. Linear order is not as common, and reordering the pins is
    // more effort in the GUI than assigning them from scratch.
    for( int pinIndex = 0; pinIndex < GetPinCount(); ++pinIndex )
        SetPinSymbolPinNumber( pinIndex, "" );
}
