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

#include "sim/spice_library_parser.h"

#include <stdexcept>
#include <utility>

#include <thread_pool.h>
#include <ki_exception.h>
#include <sim/sim_library_spice.h>
#include <sim/spice_grammar.h>
#include <sim/sim_model_spice.h>
#include <richio.h>
#include <wx/strconv.h>

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


void SPICE_LIBRARY_PARSER::parseFile( const wxString &aFilePath, REPORTER& aReporter,
                                      std::vector<std::pair<std::string, std::string>>* aQueue )
{
    try
    {
        std::string fileContents = SafeReadFile( aFilePath, wxS( "r" ) ).ToStdString( wxConvUTF8 );
        std::string filePath = aFilePath.ToStdString( wxConvUTF8 );

        tao::pegtl::string_input<> in( fileContents, filePath );
        auto root = tao::pegtl::parse_tree::parse<SIM_LIBRARY_SPICE_PARSER::libraryGrammar,
                                                  SIM_LIBRARY_SPICE_PARSER::librarySelector,
                                                  tao::pegtl::nothing,
                                                  SIM_LIBRARY_SPICE_PARSER::control> ( in );

        for( const auto& node : root->children )
        {
            if( node->is_type<SIM_LIBRARY_SPICE_PARSER::modelUnit>() )
            {
                aQueue->emplace_back( node->children.at( 0 )->string(), node->string() );
            }
            else if( node->is_type<SIM_LIBRARY_SPICE_PARSER::dotInclude>() )
            {
                wxString lib = m_library.m_pathResolver( node->children.at( 0 )->string(), aFilePath );

                try
                {
                    parseFile( lib, aReporter, aQueue );
                }
                catch( const IO_ERROR& e )
                {
                    aReporter.Report( e.What(), RPT_SEVERITY_ERROR );
                }
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
    catch( const IO_ERROR& e )
    {
        aReporter.Report( e.What(), RPT_SEVERITY_ERROR );
    }
    catch( const tao::pegtl::parse_error& e )
    {
        aReporter.Report( e.what(), RPT_SEVERITY_ERROR );
    }
    catch( const std::out_of_range& e )
    {
        aReporter.Report( wxString::Format( _( "Error parsing SPICE library '%s': %s" ),
                                            aFilePath, e.what() ),
                          RPT_SEVERITY_ERROR );
    }
}


void SPICE_LIBRARY_PARSER::ReadFile( const wxString& aFilePath, REPORTER& aReporter )
{
    m_library.m_models.clear();
    m_library.m_modelNames.clear();

    std::vector<std::pair<std::string, std::string>> modelQueue;

    parseFile( aFilePath, aReporter, &modelQueue );

    m_library.m_models.reserve( modelQueue.size() );
    m_library.m_modelNames.reserve( modelQueue.size() );

    for( const auto& [name, source] : modelQueue )
    {
        m_library.m_models.emplace_back( nullptr );
        m_library.m_modelNames.emplace_back( name );
    }

    auto createModel =
            [&]( int ii, bool firstPass )
            {
                m_library.m_models[ii] = SIM_MODEL_SPICE::Create( m_library, modelQueue[ii].second,
                                                                  firstPass, aReporter );
            };

    // Read all self-contained models in parallel
    thread_pool& tp = GetKiCadThreadPool();

    auto results = tp.submit_loop( 0, modelQueue.size(),
                            [&]( const int ii )
                            {
                                createModel( ii, true );
                            } );
    results.wait();

    // Now read all models that might refer to other models in order.
    for( int ii = 0; ii < (int) modelQueue.size(); ++ii )
    {
        if( !m_library.m_models[ii] )
            createModel( ii, false );
    }
}
