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

#include <confirm.h>

#include <sim/spice_library_parser.h>
#include <sim/sim_library_spice.h>
#include <sim/spice_grammar.h>
#include <sim/sim_model_spice.h>
#include <ki_exception.h>

#include <pegtl.hpp>
#include <pegtl/contrib/parse_tree.hpp>


namespace SIM_LIBRARY_SPICE_PARSER
{
    using namespace SPICE_GRAMMAR;

    // TODO: unknownLine is already handled in spiceUnit.
    struct libraryGrammar : spiceSourceGrammar {};


    template <typename Rule> struct librarySelector : std::false_type {};

    template <> struct librarySelector<modelUnit> : std::true_type {};
    template <> struct librarySelector<modelName> : std::true_type {};

    template <> struct librarySelector<dotInclude> : std::true_type {};
    template <> struct librarySelector<dotIncludePathWithoutQuotes> : std::true_type {};
    template <> struct librarySelector<dotIncludePathWithoutApostrophes> : std::true_type {};
    template <> struct librarySelector<dotIncludePath> : std::true_type {};

    // For debugging.
    template <> struct librarySelector<unknownLine> : std::true_type {};
};


void SPICE_LIBRARY_PARSER::readElement( const std::string &aFilePath )
{
    tao::pegtl::file_input in( aFilePath );
    std::unique_ptr<tao::pegtl::parse_tree::node> root =
                tao::pegtl::parse_tree::parse<SIM_LIBRARY_SPICE_PARSER::libraryGrammar,
                                              SIM_LIBRARY_SPICE_PARSER::librarySelector,
                                              tao::pegtl::nothing,
                                              SIM_LIBRARY_SPICE_PARSER::control>( in );

    for( const auto& node : root->children )
    {
        if( node->is_type<SIM_LIBRARY_SPICE_PARSER::modelUnit>() )
        {
            try
            {
                m_library.m_models.push_back( SIM_MODEL_SPICE::Create( m_library, node->string() ) );
                m_library.m_modelNames.emplace_back( node->children.at( 0 )->string() );
            }
            catch( const IO_ERROR& e )
            {
                DisplayErrorMessage( nullptr, e.What() );
            }
        }
        else if( node->is_type<SIM_LIBRARY_SPICE_PARSER::dotInclude>() )
        {
            std::string lib = node->children.at( 0 )->string();

            if( m_library.m_pathResolver )
                lib = ( *m_library.m_pathResolver )( lib, aFilePath );

            readElement( lib );
        }
        else if( node->is_type<SIM_LIBRARY_SPICE_PARSER::unknownLine>() )
        {
            // Do nothing.
        }
        else
        {
            wxFAIL_MSG( "Unhandled parse tree node" );
        }
    }
}


void SPICE_LIBRARY_PARSER::ReadFile( const std::string& aFilePath )
{
    m_library.m_models.clear();
    m_library.m_modelNames.clear();

    try
    {
        readElement( aFilePath );
    }
    catch( const std::filesystem::filesystem_error& e )
    {
        THROW_IO_ERROR( e.what() );
    }
    catch( const tao::pegtl::parse_error& e )
    {
        THROW_IO_ERROR( e.what() );
    }
}
