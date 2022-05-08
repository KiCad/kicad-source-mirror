/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 1992-2013 jp.charras at wanadoo.fr
 * Copyright (C) 2013 SoftPLC Corporation, Dick Hollenbeck <dick@softplc.com>
 * Copyright (C) 1992-2022 KiCad Developers, see AUTHORS.TXT for contributors.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, you may find one here:
 * http://www.gnu.org/licenses/old-licenses/gpl-2.0.html
 * or you may search the http://www.gnu.org website for the version 2 license,
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

#include "netlist_exporter_spice.h"
#include <pegtl.hpp>
#include <pegtl/contrib/parse_tree.hpp>
#include <sim/sim_model_spice.h>
#include <common.h>
#include <confirm.h>
#include <pgm_base.h>
#include <env_paths.h>
#include <sim/sim_library.h>
#include <sch_screen.h>
#include <sch_text.h>
#include <sch_textbox.h>
#include <string_utils.h>


namespace NETLIST_EXPORTER_SPICE_PARSER
{
    using namespace SPICE_GRAMMAR;

    struct textGrammar : spiceSourceGrammar {};

    template <typename Rule> struct textSelector : std::false_type {};
    template <> struct textSelector<modelUnit> : std::true_type {};

    template <> struct textSelector<dotTitle> : std::true_type {};
    template <> struct textSelector<dotTitleTitle> : std::true_type {};

    template <> struct textSelector<dotInclude> : std::true_type {};
    template <> struct textSelector<dotIncludePathWithoutQuotes> : std::true_type {};
    template <> struct textSelector<dotIncludePathWithoutApostrophes> : std::true_type {};
    template <> struct textSelector<dotIncludePath> : std::true_type {};

    template <> struct textSelector<dotLine> : std::true_type {};
}


bool NETLIST_EXPORTER_SPICE::WriteNetlist( const wxString& aOutFileName, unsigned aNetlistOptions )
{
    FILE_OUTPUTFORMATTER formatter( aOutFileName, wxT( "wt" ), '\'' );
    return GenerateNetlist( formatter, aNetlistOptions );
}


bool NETLIST_EXPORTER_SPICE::GenerateNetlist( OUTPUTFORMATTER& aFormatter, unsigned aNetlistOptions )
{
    // Default title.
    m_title = "KiCad schematic";

    if( !ReadSchematicAndLibraries( aNetlistOptions ) )
        return false;

    aFormatter.Print( 0, ".title %s\n", TO_UTF8( m_title ) );

    writeIncludes( aFormatter, aNetlistOptions );
    writeModels( aFormatter );
    writeDirectives( aFormatter, aNetlistOptions );
    writeItems( aFormatter );

    aFormatter.Print( 0, ".end\n" );

    return true;
}


bool NETLIST_EXPORTER_SPICE::ReadSchematicAndLibraries( unsigned aNetlistOptions )
{
    std::set<wxString> refNames; // Set of reference names to check for duplication.
    int notConnectedCounter = 1;

    ReadDirectives();

    m_nets.clear();
    m_items.clear();

    for( unsigned int sheetIndex = 0; sheetIndex < m_schematic->GetSheets().size(); ++sheetIndex )
    {
        SCH_SHEET_PATH sheet = m_schematic->GetSheets().at( sheetIndex );

        for( SCH_ITEM* schItem : sheet.LastScreen()->Items().OfType( SCH_SYMBOL_T ) )
        {
            SCH_SYMBOL* symbol = findNextSymbol( schItem, &sheet );

            if( !symbol )
                continue;

            CreatePinList( symbol, &sheet, true );

            SPICE_ITEM item;

            item.model = SIM_MODEL::Create(
                    static_cast<int>( m_sortedSymbolPinList.size() ), symbol->GetFields() );

            readLibraryField( *symbol, item );
            readNameField( *symbol, item );
            readEnabledField( *symbol, item );

            if( !readRefName( sheet, *symbol, item, refNames ) )
                return false;

            readPins( *symbol, item, notConnectedCounter );

            // TODO: transmission line handling?

            m_items.push_back( std::move( item ) );
        }
    }

    return true;
}


void NETLIST_EXPORTER_SPICE::ReplaceForbiddenChars( wxString& aNetName )
{
    aNetName.Replace( "(", "_" );
    aNetName.Replace( ")", "_" );
    aNetName.Replace( " ", "_" );
}


wxString NETLIST_EXPORTER_SPICE::GetItemName( const wxString& aRefName ) const
{
    const std::list<SPICE_ITEM>& spiceItems = GetItems();

    auto it = std::find_if( spiceItems.begin(), spiceItems.end(),
                            [aRefName]( const SPICE_ITEM& aItem )
                            {
                                return aItem.refName == aRefName;
                            } );

    if( it == spiceItems.end() )
        return "";

    return it->model->GenerateSpiceItemName( aRefName );
}


void NETLIST_EXPORTER_SPICE::ReadDirectives()
{
    m_directives.clear();

    for( unsigned int sheetIndex = 0; sheetIndex < m_schematic->GetSheets().size(); ++sheetIndex )
    {
        for( SCH_ITEM* item : m_schematic->GetSheets().at( sheetIndex ).LastScreen()->Items() )
        {
            wxString text;

            if( item->Type() == SCH_TEXT_T )
                text = static_cast<SCH_TEXT*>( item )->GetShownText();
            else if( item->Type() == SCH_TEXTBOX_T )
                text = static_cast<SCH_TEXTBOX*>( item )->GetShownText();
            else
                continue;

            tao::pegtl::string_input<> in( text.ToStdString() + "\n", "from_content" );
            std::unique_ptr<tao::pegtl::parse_tree::node> root;

            try
            {
                root = tao::pegtl::parse_tree::parse<NETLIST_EXPORTER_SPICE_PARSER::textGrammar,
                                                     NETLIST_EXPORTER_SPICE_PARSER::textSelector>
                    ( in );
            }
            catch( const tao::pegtl::parse_error& e )
            {
                continue;
            }

            wxASSERT( root );

            for( const auto& node : root->children )
            {
                if( node->is_type<NETLIST_EXPORTER_SPICE_PARSER::dotTitle>() )
                    m_title = node->children.at( 0 )->string();
                else if( node->is_type<NETLIST_EXPORTER_SPICE_PARSER::dotInclude>() )
                    m_libraries[node->children.at( 0 )->string()] =
                        SIM_LIBRARY::Create( node->children.at( 0 )->string() );
                else
                    m_directives.emplace_back( node->string() );
            }
        }
    }
}


void NETLIST_EXPORTER_SPICE::readLibraryField( SCH_SYMBOL& aSymbol, SPICE_ITEM& aItem )
{
    SCH_FIELD* field = aSymbol.FindField( SIM_LIBRARY::LIBRARY_FIELD );
    wxString path;

    if( field )
        path = field->GetShownText();
    else if( auto model = dynamic_cast<const SIM_MODEL_SPICE*>( aItem.model.get() ) )
    {
        // Special case for legacy models.
        int libParamIndex = static_cast<int>( SIM_MODEL_SPICE::SPICE_PARAM::LIB );
        path = model->GetParam( libParamIndex ).value->ToString();
    }
    
    if( path.IsEmpty() )
        return;

    if( auto library = SIM_LIBRARY::Create( m_schematic->Prj().AbsolutePath( path ) ) )
        m_libraries.try_emplace( path, std::move( library ) );

    aItem.libraryPath = path;
}


void NETLIST_EXPORTER_SPICE::readNameField( SCH_SYMBOL& aSymbol, SPICE_ITEM& aItem )
{
    SCH_FIELD* field = aSymbol.FindField( SIM_LIBRARY::NAME_FIELD );

    if( !field )
        return;

    wxString modelName = field->GetShownText();

    if( m_libraries.count( aItem.libraryPath ) )
    {
        const SIM_LIBRARY& library = *m_libraries.at( aItem.libraryPath );
        const SIM_MODEL* baseModel = library.FindModel( modelName );

        if( baseModel )
        {
            aItem.model = SIM_MODEL::Create( *baseModel,
                                             m_sortedSymbolPinList.size(),
                                             aSymbol.GetFields() );
            aItem.modelName = modelName;
            return;
        }
    }
}


void NETLIST_EXPORTER_SPICE::readEnabledField( SCH_SYMBOL& aSymbol, SPICE_ITEM& aItem )
{
    SCH_FIELD* field = aSymbol.FindField( ENABLED_FIELD );

    if( !field )
        return;

    aItem.enabled = StringToBool( field->GetShownText() );
}


bool NETLIST_EXPORTER_SPICE::readRefName( SCH_SHEET_PATH& aSheet, SCH_SYMBOL& aSymbol,
                                          SPICE_ITEM& aItem,
                                          std::set<wxString>& aRefNames )
{
    aItem.refName = aSymbol.GetRef( &aSheet );

    if( aRefNames.count( aItem.refName ) )
    {
        DisplayErrorMessage( nullptr,
                _( "Multiple symbols have the same reference designator.\n"
                   "Annotation must be corrected before simulating." ) );
        return false;
    }

    aRefNames.insert( aItem.refName );
    return true;
}


void NETLIST_EXPORTER_SPICE::readPins( SCH_SYMBOL& aSymbol, SPICE_ITEM& aItem,
                                       int& notConnectedCounter )
{
    for( const PIN_INFO& pin : m_sortedSymbolPinList )
    {
        wxString netName = pin.netName;
        ReplaceForbiddenChars( netName );
        netName = UnescapeString( netName );

        if( netName.IsEmpty() )
            netName = wxString::Format( wxT( "NC_%.2u" ), notConnectedCounter++ );

        aItem.pins.push_back( netName );
        m_nets.insert( netName );
    }
}


void NETLIST_EXPORTER_SPICE::writeIncludes( OUTPUTFORMATTER& aFormatter,
                                            unsigned aNetlistOptions )
{
    for( auto&& [libraryPath, library] : m_libraries )
    {
        // First, expand env vars, if any.
        wxString libName = ExpandEnvVarSubstitutions( libraryPath, &m_schematic->Prj() );
        wxString fullPath;

        if( aNetlistOptions & OPTION_ADJUST_INCLUDE_PATHS )
        {
            // Look for the library in known search locations.
            fullPath = ResolveFile( libName, &Pgm().GetLocalEnvVariables(), &m_schematic->Prj() );
            
            if( fullPath.IsEmpty() )
            {
                DisplayErrorMessage( nullptr,
                        wxString::Format( _( "Could not find library file '%s'" ), libName ) );
                fullPath = libName;
            }
        }
        else
            fullPath = libName;

        aFormatter.Print( 0, ".include \"%s\"\n", TO_UTF8( fullPath ) );
    }
}


void NETLIST_EXPORTER_SPICE::writeModels( OUTPUTFORMATTER& aFormatter )
{
    for( const SPICE_ITEM& item : m_items )
    {
        if( !item.enabled )
            continue;

        aFormatter.Print( 0, "%s", TO_UTF8( item.model->GenerateSpiceModelLine( item.modelName ) ) );
    }
}


void NETLIST_EXPORTER_SPICE::writeItems( OUTPUTFORMATTER& aFormatter )
{
    for( const SPICE_ITEM& item : m_items )
    {
        if( !item.enabled )
            continue;

        aFormatter.Print( 0, "%s", TO_UTF8( item.model->GenerateSpiceItemLine( item.refName,
                                                                               item.modelName,
                                                                               item.pins ) ) );
    }
}


void NETLIST_EXPORTER_SPICE::writeDirectives( OUTPUTFORMATTER& aFormatter, unsigned aNetlistOptions )
{
    if( aNetlistOptions & OPTION_SAVE_ALL_VOLTAGES )
        aFormatter.Print( 0, ".save all\n" );

    if( aNetlistOptions & OPTION_SAVE_ALL_CURRENTS )
        aFormatter.Print( 0, ".probe alli\n" );

    for( const wxString& directive : m_directives )
        aFormatter.Print( 0, "%s\n", TO_UTF8( directive ) );
}
