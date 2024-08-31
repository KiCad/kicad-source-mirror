/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2022 Mikolaj Wielgus
 * Copyright (C) 2022-2023 KiCad Developers, see AUTHORS.txt for contributors.
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
#include <sim/sim_model_spice_fallback.h>
#include <ki_exception.h>

#include <pegtl.hpp>
#include <pegtl/contrib/parse_tree.hpp>
#include <utility>
#include <core/thread_pool.h>


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


static SIM_MODEL::TYPE getFallbackType( const wxString& aToken, const wxString& aLine )
{
    for( SIM_MODEL::TYPE candidate : SIM_MODEL::TYPE_ITERATOR() )
    {
        wxString candidate_type = SIM_MODEL::SpiceInfo( candidate ).modelType;

        if( candidate_type.IsEmpty() )
            continue;

        if( SIM_MODEL::SpiceInfo( candidate ).level != ""
                && !SIM_MODEL::SpiceInfo( candidate ).isDefaultLevel )
        {
            continue;
        }

        if( candidate_type.StartsWith( wxS( "VDMOS" ) ) && aToken == wxS( "VDMOS" ) )
        {
            if( candidate_type.EndsWith( wxS( "PCHAN" ) ) )
            {
                if( aLine.Upper().Contains( wxS( "PCHAN" ) ) )
                    return candidate;
            }
            else
            {
                if( !aLine.Upper().Contains( wxS( "PCHAN" ) ) )
                    return candidate;
            }
        }
        else if( aToken.StartsWith( candidate_type ) )
        {
            return candidate;
        }
    }

    return SIM_MODEL::TYPE::NONE;
}


void SPICE_LIBRARY_PARSER::readFallbacks( const wxString& aFilePath, REPORTER& aReporter )
{
    try
    {
        wxArrayString lines = wxSplit( SafeReadFile( aFilePath, wxS( "r" ) ), '\n' );

        for( const wxString& line : lines )
        {
            wxStringTokenizer tokenizer( line, wxS( " ()\t\r\n" ), wxTOKEN_STRTOK );
            wxString          token = tokenizer.GetNextToken().Lower();

            if( token == wxS( ".model" ) )
            {
                wxString        name = tokenizer.GetNextToken();
                wxString        typeToken = tokenizer.GetNextToken().Upper();
                SIM_MODEL::TYPE type = getFallbackType( typeToken, line );

                m_library.m_models.push_back( std::make_unique<SIM_MODEL_SPICE_FALLBACK>( type ) );
                m_library.m_modelNames.emplace_back( name );
            }
            else if( token == wxS( ".inc" ) )
            {
                wxString lib = m_library.m_pathResolver( tokenizer.GetNextToken(), aFilePath );

                readFallbacks( lib, aReporter );
            }
        }
    }
    catch( IO_ERROR& e )
    {
        aReporter.Report( e.What(), RPT_SEVERITY_ERROR );
    }
}


void SPICE_LIBRARY_PARSER::parseFile( const wxString &aFilePath,
                                      std::vector<std::pair<std::string, std::string>>* aModelQueue,
                                      REPORTER& aReporter )
{
    try
    {
        tao::pegtl::string_input<> in( SafeReadFile( aFilePath, wxS( "r" ) ).ToStdString(),
                                       aFilePath.ToStdString() );
        auto root = tao::pegtl::parse_tree::parse<SIM_LIBRARY_SPICE_PARSER::libraryGrammar,
                                                  SIM_LIBRARY_SPICE_PARSER::librarySelector,
                                                  tao::pegtl::nothing,
                                                  SIM_LIBRARY_SPICE_PARSER::control> ( in );

        for( const auto& node : root->children )
        {
            if( node->is_type<SIM_LIBRARY_SPICE_PARSER::modelUnit>() )
            {
                aModelQueue->emplace_back( node->children.at( 0 )->string(), node->string() );
            }
            else if( node->is_type<SIM_LIBRARY_SPICE_PARSER::dotInclude>() )
            {
                wxString lib = m_library.m_pathResolver( node->children.at( 0 )->string(), aFilePath );

                try
                {
                    parseFile( lib, aModelQueue, aReporter );
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
}


void SPICE_LIBRARY_PARSER::ReadFile( const wxString& aFilePath, REPORTER& aReporter )
{
    m_library.m_models.clear();
    m_library.m_modelNames.clear();

    std::vector<std::pair<std::string, std::string>> modelQueue;

    // Aside from the simulation model editor dialog, about the only data we use from the
    // complete models are the pin definitions for SUBCKTs.  The standard LTSpice "cmp" libraries
    // (cmp/standard.bjt, cmp/standard.mos, etc.) have copious error which trip up our parser,
    // and our parser is *really* slow on such large files (nearly 5 seconds on my dev machine).
    if( !m_forceFullParse && aFilePath.Contains( wxS( "/LTspiceXVII/lib/cmp/standard" ) ) )
        readFallbacks( aFilePath, aReporter );
    else
        parseFile( aFilePath, &modelQueue, aReporter );

    m_library.m_models.reserve( modelQueue.size() );
    m_library.m_modelNames.reserve( modelQueue.size() );

    for( int ii = 0; ii < (int) modelQueue.size(); ++ii )
    {
        m_library.m_models.emplace_back( nullptr );
        m_library.m_modelNames.emplace_back( "" );
    }

    thread_pool& tp = GetKiCadThreadPool();

    tp.push_loop( modelQueue.size(),
            [&]( const int a, const int b )
            {
                for( int ii = a; ii < b; ++ii )
                {
                    std::unique_ptr<SIM_MODEL> model;

                    try
                    {
                        model = SIM_MODEL_SPICE::Create( m_library, modelQueue[ii].second );
                    }
                    catch( const IO_ERROR& e )
                    {
                       aReporter.Report( e.What(), RPT_SEVERITY_ERROR );
                    }
                    catch( ... )
                    {
                       aReporter.Report( wxString::Format( _( "Cannot create sim model from %s" ),
                                                           modelQueue[ii].second ),
                                         RPT_SEVERITY_ERROR );
                    }

                    if( model )
                    {
                        m_library.m_models[ii] = std::move( model );
                        m_library.m_modelNames[ii] = modelQueue[ii].first;
                    }
                }
            } );
    tp.wait_for_tasks();
}
