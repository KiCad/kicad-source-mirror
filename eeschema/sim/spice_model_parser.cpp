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

#include <wx/log.h>

#include <ki_exception.h>
#include <sim/spice_model_parser.h>
#include <sim/spice_grammar.h>
#include <sim/sim_model_spice.h>
#include <sim/sim_library_spice.h>

#include <boost/algorithm/string/case_conv.hpp>
#include <boost/algorithm/string/predicate.hpp>
#include <pegtl.hpp>
#include <pegtl/contrib/parse_tree.hpp>


/**
 * Flag to enable SPICE model parser debugging output.
 *
 * @ingroup trace_env_vars
 */
static const wxChar traceSpiceModelParser[] = wxT( "KICAD_SPICE_MODEL_PARSER" );


namespace SIM_MODEL_SPICE_PARSER
{
    using namespace SPICE_GRAMMAR;

    template <typename Rule> struct spiceUnitSelector : std::false_type {};

    template <> struct spiceUnitSelector<dotModelAko> : std::true_type {};
    template <> struct spiceUnitSelector<dotModel> : std::true_type {};
    template <> struct spiceUnitSelector<modelName> : std::true_type {};
    template <> struct spiceUnitSelector<dotModelType> : std::true_type {};
    template <> struct spiceUnitSelector<param> : std::true_type {};
    template <> struct spiceUnitSelector<paramValue> : std::true_type {};

    template <> struct spiceUnitSelector<dotSubckt> : std::true_type {};
}


bool SPICE_MODEL_PARSER::ReadType( const SIM_LIBRARY_SPICE& aLibrary, const std::string& aSpiceCode,
                                   SIM_MODEL::TYPE* aType, bool aFirstPass )
{
    *aType = SIM_MODEL::TYPE::NONE;

    tao::pegtl::string_input<> in( aSpiceCode, "Spice_Code" );
    std::unique_ptr<tao::pegtl::parse_tree::node> root;

    try
    {
        root = tao::pegtl::parse_tree::parse<SIM_MODEL_SPICE_PARSER::spiceUnitGrammar,
                                             SIM_MODEL_SPICE_PARSER::spiceUnitSelector,
                                             tao::pegtl::nothing,
                                             SIM_MODEL_SPICE_PARSER::control>
            ( in );
    }
    catch( const tao::pegtl::parse_error& e )
    {
        wxLogTrace( traceSpiceModelParser, wxS( "%s" ), e.what() );
        return true;
    }

    for( const auto& node : root->children )
    {
        if( node->is_type<SIM_MODEL_SPICE_PARSER::dotModelAko>() )
        {
            // The first pass of model loading is done in parallel, so we can't (yet) look
            // up our reference model.
            if( aFirstPass )
                return false;

            std::string modelName = node->children.at( 0 )->string();
            std::string akoName = node->children.at( 1 )->string();
            const SIM_MODEL* sourceModel = aLibrary.FindModel( akoName );

            if( !sourceModel )
            {
                THROW_IO_ERROR( wxString::Format( _( "Could not find parent model '%s' for \"A Kind Of\" model '%s'" ),
                                                  akoName,
                                                  modelName ) );
            }

            *aType = sourceModel->GetType();
            return true;
        }
        else if( node->is_type<SIM_MODEL_SPICE_PARSER::dotModel>() )
        {
            std::string paramName;
            std::string typeString;
            std::string level;
            std::string version;

            for( const auto& subnode : node->children )
            {
                if( subnode->is_type<SIM_MODEL_SPICE_PARSER::modelName>() )
                {
                    // Do nothing.
                }
                else if( subnode->is_type<SIM_MODEL_SPICE_PARSER::dotModelType>() )
                {
                    typeString = subnode->string();
                    SIM_MODEL::TYPE type = ReadTypeFromSpiceStrings( typeString );

                    if( type != SIM_MODEL::TYPE::RAWSPICE )
                    {
                        *aType = type;
                        return true;
                    }
                }
                else if( subnode->is_type<SIM_MODEL_SPICE_PARSER::param>() )
                {
                    paramName = subnode->string();
                }
                else if( subnode->is_type<SIM_MODEL_SPICE_PARSER::paramValue>() )
                {
                    wxASSERT( paramName != "" );

                    if( paramName == "level" )
                        level = subnode->string();
                    else if( paramName == "version" )
                        version = subnode->string();
                }
                else
                {
                    wxFAIL_MSG( "Unhandled parse tree subnode" );
                    return true;
                }
            }

            // Type was not determined from Spice type string alone, so now we take `level` and
            // `version` variables into account too. This is suboptimal since we read the model
            // twice this way, and moreover the code is now somewhat duplicated.

            *aType = ReadTypeFromSpiceStrings( typeString, level, version, false );
            return true;
        }
        else if( node->is_type<SIM_MODEL_SPICE_PARSER::dotSubckt>() )
        {
            // The first pass of model loading is done in parallel, so we can't (yet) refer to
            // any other models in a subcircuit.
            if( aFirstPass )
                return false;

            *aType = SIM_MODEL::TYPE::SUBCKT;
            return true;
        }
        else
        {
            wxFAIL_MSG( "Unhandled parse tree node" );
            return true;
        }
    }

    wxFAIL_MSG( "Could not derive type from SPICE code" );
    return true;
}


void SPICE_MODEL_PARSER::ReadModel( const SIM_LIBRARY_SPICE& aLibrary,
                                    const std::string& aSpiceCode )
{
    // The default behavior is to treat the Spice param=value pairs as the model parameters and
    // values (for many models the correspondence is not exact, so this function is overridden).

    tao::pegtl::string_input<> in( aSpiceCode, "Spice_Code" );
    std::unique_ptr<tao::pegtl::parse_tree::node> root;

    try
    {
        root = tao::pegtl::parse_tree::parse<SIM_MODEL_SPICE_PARSER::spiceUnitGrammar,
                                             SIM_MODEL_SPICE_PARSER::spiceUnitSelector,
                                             tao::pegtl::nothing,
                                             SIM_MODEL_SPICE_PARSER::control>
            ( in );
    }
    catch( tao::pegtl::parse_error& e )
    {
        THROW_IO_ERROR( e.what() );
    }

    for( const auto& node : root->children )
    {
        if( node->is_type<SIM_MODEL_SPICE_PARSER::dotModelAko>() )
        {
            std::string modelName = node->children.at( 0 )->string();
            std::string akoName = node->children.at( 1 )->string();

            const SIM_MODEL* sourceModel = aLibrary.FindModel( akoName );

            if( !sourceModel )
            {
                THROW_IO_ERROR( wxString::Format( _( "Could not find parent model '%s' for \"A Kind Of\" model '%s'" ),
                                                  akoName,
                                                  modelName ) );
            }

            for( int i = 0; i < static_cast<int>( sourceModel->GetParamCount() ); ++i )
                m_model.SetParamValue( i, sourceModel->GetParam( i ).value );

            std::string paramName;

            for( const auto& subnode : node->children )
            {
                if( subnode->is_type<SIM_MODEL_SPICE_PARSER::modelName>() )
                {
                    // Do nothing.
                }
                else if( subnode->is_type<SIM_MODEL_SPICE_PARSER::dotModelType>() )
                {
                    // Do nothing.
                }
                else if( subnode->is_type<SIM_MODEL_SPICE_PARSER::param>() )
                {
                    paramName = subnode->string();
                }
                else if( subnode->is_type<SIM_MODEL_SPICE_PARSER::paramValue>() )
                {
                    wxASSERT( paramName != "" );

                    m_model.SetParamFromSpiceCode( paramName, subnode->string() );
                }
                else
                {
                    wxFAIL_MSG( "Unhandled parse tree subnode" );
                }
            }
        }
        else if( node->is_type<SIM_MODEL_SPICE_PARSER::dotModel>() )
        {
            std::string modelName;
            std::string paramName;

            for( const auto& subnode : node->children )
            {
                if( subnode->is_type<SIM_MODEL_SPICE_PARSER::modelName>() )
                {
                    modelName = subnode->string();
                }
                else if( subnode->is_type<SIM_MODEL_SPICE_PARSER::dotModelType>() )
                {
                    // Do nothing.
                }
                else if( subnode->is_type<SIM_MODEL_SPICE_PARSER::param>() )
                {
                    paramName = subnode->string();
                }
                else if( subnode->is_type<SIM_MODEL_SPICE_PARSER::paramValue>() )
                {
                    wxASSERT( paramName != "" );

                    m_model.SetParamFromSpiceCode( paramName, subnode->string() );
                }
                else
                {
                    wxFAIL_MSG( "Unhandled parse tree subnode" );
                }
            }
        }
        else
        {
            wxFAIL_MSG( "Unhandled parse tree node" );
        }
    }

    m_model.m_spiceCode = aSpiceCode;
}


SIM_MODEL::TYPE SPICE_MODEL_PARSER::ReadTypeFromSpiceStrings( const std::string& aTypeString,
                                                              const std::string& aLevel,
                                                              const std::string& aVersion,
                                                              bool aSkipDefaultLevel )
{
    wxString input_level = wxString( aLevel ).BeforeFirst( '.' );
    wxString input_type( aTypeString );
    bool     vdmos = false;
    bool     pchan = false;

    input_type.UpperCase();

    if( input_type.StartsWith( wxS( "VDMOS" ) ) )
    {
        vdmos = true;
        pchan = input_type.Contains( wxS( "PCHAN" ) );
    }

    for( SIM_MODEL::TYPE candidate : SIM_MODEL::TYPE_ITERATOR() )
    {
        wxString candidate_type = SIM_MODEL::SpiceInfo( candidate ).modelType;

        if( candidate_type.IsEmpty() )
            continue;

        if( candidate_type.StartsWith( wxS( "VDMOS" ) ) && vdmos )
        {
            if( vdmos && pchan && candidate_type.EndsWith( wxS( "PCHAN" ) ) )
                return candidate;
            else if( vdmos && !pchan && candidate_type.EndsWith( wxS( "NCHAN" ) ) )
                return candidate;
        }
        else if( input_type.StartsWith( candidate_type ) )
        {
            if( SIM_MODEL::SpiceInfo( candidate ).version != aVersion )
                continue;

            if( SIM_MODEL::SpiceInfo( candidate ).level == input_level )
                return candidate;

            if( aSkipDefaultLevel )
                continue;

            if( SIM_MODEL::SpiceInfo( candidate ).isDefaultLevel && aLevel == "" )
                return candidate;
        }
    }

    // If the type string is not recognized, demote to a raw Spice element. This way the user won't
    // have an error if there is a type KiCad does not recognize.
    return SIM_MODEL::TYPE::RAWSPICE;
}

