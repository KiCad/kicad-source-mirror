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

#include <../../pcbnew/ibis/kibis.h>
#include "netlist_exporter_spice.h"
#include <sim/sim_library_spice.h>
#include <sim/sim_model_raw_spice.h>
#include <sim/spice_grammar.h>
#include <common.h>
#include <confirm.h>
#include <pgm_base.h>
#include <env_paths.h>
#include <sim/sim_library.h>
#include <sim/sim_library_kibis.h>
#include <sim/sim_model_kibis.h>
#include <sim/sim_model.h>
#include <sch_screen.h>
#include <sch_text.h>
#include <sch_textbox.h>
#include <string_utils.h>


#include <boost/algorithm/string/replace.hpp>
#include <fmt/core.h>
#include <paths.h>
#include <pegtl.hpp>
#include <pegtl/contrib/parse_tree.hpp>
#include <wx/dir.h>


namespace NETLIST_EXPORTER_SPICE_PARSER
{
    using namespace SPICE_GRAMMAR;

    struct textGrammar : must<spiceSourceNothrow> {};

    template <typename Rule> struct textSelector : std::false_type {};
    template <> struct textSelector<modelUnit> : std::true_type {};

    template <> struct textSelector<dotControl> : std::true_type {};

    template <> struct textSelector<dotTitle> : std::true_type {};
    template <> struct textSelector<dotTitleTitle> : std::true_type {};

    template <> struct textSelector<dotInclude> : std::true_type {};
    template <> struct textSelector<dotIncludePathWithoutQuotes> : std::true_type {};
    template <> struct textSelector<dotIncludePathWithoutApostrophes> : std::true_type {};
    template <> struct textSelector<dotIncludePath> : std::true_type {};

    template <> struct textSelector<kLine> : std::true_type {};

    template <> struct textSelector<dotLine> : std::true_type {};
}


NETLIST_EXPORTER_SPICE::NETLIST_EXPORTER_SPICE( SCHEMATIC_IFACE* aSchematic ) :
    NETLIST_EXPORTER_BASE( aSchematic ),
    m_libMgr( aSchematic->Prj() )
{
}


bool NETLIST_EXPORTER_SPICE::WriteNetlist( const wxString& aOutFileName, unsigned aNetlistOptions )
{
    FILE_OUTPUTFORMATTER formatter( aOutFileName, wxT( "wt" ), '\'' );
    return DoWriteNetlist( formatter, aNetlistOptions );
}


bool NETLIST_EXPORTER_SPICE::DoWriteNetlist( OUTPUTFORMATTER& aFormatter, unsigned aNetlistOptions )
{
   // Cleanup list to avoid duplicate if the netlist exporter is run more than once.
    m_rawIncludes.clear();

    // Default title.
    m_title = "KiCad schematic";

    if( !ReadSchematicAndLibraries( aNetlistOptions ) )
        return false;

    WriteHead( aFormatter, aNetlistOptions );

    writeIncludes( aFormatter, aNetlistOptions );
    writeModels( aFormatter );
    WriteDirectives( aFormatter, aNetlistOptions );
    writeItems( aFormatter );

    WriteTail( aFormatter, aNetlistOptions );

    return true;
}


void NETLIST_EXPORTER_SPICE::WriteHead( OUTPUTFORMATTER& aFormatter, unsigned aNetlistOptions )
{
    aFormatter.Print( 0, ".title %s\n", m_title.c_str() );
}


void NETLIST_EXPORTER_SPICE::WriteTail( OUTPUTFORMATTER& aFormatter, unsigned aNetlistOptions )
{
    aFormatter.Print( 0, ".end\n" );
}


bool NETLIST_EXPORTER_SPICE::ReadSchematicAndLibraries( unsigned aNetlistOptions )
{
    std::set<std::string> refNames; // Set of reference names to check for duplication.
    int ncCounter = 1;

    ReadDirectives( aNetlistOptions );

    m_nets.clear();
    m_items.clear();
    m_libParts.clear();

    wxFileName cacheDir;
    cacheDir.AssignDir( PATHS::GetUserCachePath() );
    cacheDir.AppendDir( wxT( "ibis" ) );

    if( !cacheDir.DirExists() )
    {
        cacheDir.Mkdir( wxS_DIR_DEFAULT, wxPATH_MKDIR_FULL );

        if( !cacheDir.DirExists() )
        {
            wxLogTrace(
                    "IBIS_CACHE:", wxT( "%s:%s:%d\n * failed to create ibis cache directory '%s'" ),
                    __FILE__, __FUNCTION__, __LINE__, cacheDir.GetPath() );

            return false;
        }
    }

    wxDir    dir;
    wxString dirName = cacheDir.GetFullPath();

    if( !dir.Open( dirName ) )
        return false;

    wxFileName    thisFile;
    wxArrayString fileList;
    wxString      fileSpec = wxT( "*.cache" );

    thisFile.SetPath( dirName ); // Set the base path to the cache folder

    int numFilesFound = dir.GetAllFiles( dirName, &fileList, fileSpec );

    for( int i = 0; i < numFilesFound; i++ )
    {
        // Completes path to specific file so we can get its "last access" date
        thisFile.SetFullName( fileList[i] );
        wxRemoveFile( thisFile.GetFullPath() );
    }

    for( SCH_SHEET_PATH& sheet : GetSheets( aNetlistOptions ) )
    {
        for( SCH_ITEM* item : sheet.LastScreen()->Items().OfType( SCH_SYMBOL_T ) )
        {
            SCH_SYMBOL* symbol = findNextSymbol( item, &sheet );

            if( !symbol )
                continue;

            CreatePinList( symbol, &sheet, true );

            ITEM spiceItem;

            try
            {
                if( !readRefName( sheet, *symbol, spiceItem, refNames ) )
                    return false;

                readModel( sheet, *symbol, spiceItem );

                readPinNumbers( *symbol, spiceItem );
                readPinNetNames( *symbol, spiceItem, ncCounter );

                // TODO: transmission line handling?

                m_items.push_back( std::move( spiceItem ) );
            }
            catch( const IO_ERROR& e )
            {
                DisplayErrorMessage( nullptr, e.What() );
            }
        }
    }

    return true;
}


void NETLIST_EXPORTER_SPICE::ReplaceForbiddenChars( std::string& aNetName )
{
    boost::replace_all( aNetName, "(", "_" );
    boost::replace_all( aNetName, ")", "_" );
    boost::replace_all( aNetName, " ", "_" );
}


std::string NETLIST_EXPORTER_SPICE::GetItemName( const std::string& aRefName ) const
{
    const std::list<ITEM>& spiceItems = GetItems();

    auto it = std::find_if( spiceItems.begin(), spiceItems.end(),
                            [aRefName]( const ITEM& aItem )
                            {
                                return aItem.refName == aRefName;
                            } );

    if( it == spiceItems.end() )
        return "";

    return it->model->SpiceGenerator().ItemName( aRefName );
}


void NETLIST_EXPORTER_SPICE::ReadDirectives( unsigned aNetlistOptions )
{
    m_directives.clear();

    for( const SCH_SHEET_PATH& sheet : GetSheets( aNetlistOptions ) )
    {
        for( SCH_ITEM* item : sheet.LastScreen()->Items() )
        {
            wxString text;

            if( item->Type() == SCH_TEXT_T )
                text = static_cast<SCH_TEXT*>( item )->GetShownText();
            else if( item->Type() == SCH_TEXTBOX_T )
                text = static_cast<SCH_TEXTBOX*>( item )->GetShownText();
            else
                continue;

            tao::pegtl::string_input<> in( text.ToUTF8(), "from_content" );
            std::unique_ptr<tao::pegtl::parse_tree::node> root;

            try
            {
                root = tao::pegtl::parse_tree::parse<NETLIST_EXPORTER_SPICE_PARSER::textGrammar,
                                                     NETLIST_EXPORTER_SPICE_PARSER::textSelector,
                                                     NETLIST_EXPORTER_SPICE_PARSER::control>
                    ( in );
            }
            catch( const tao::pegtl::parse_error& )
            {
                continue;
            }

            wxASSERT( root );

            for( const auto& node : root->children )
            {
                if( node->is_type<NETLIST_EXPORTER_SPICE_PARSER::dotTitle>() )
                {
                    m_title = node->children.at( 0 )->string();
                }
                else if( node->is_type<NETLIST_EXPORTER_SPICE_PARSER::dotInclude>() )
                {
                    std::string path = node->children.at( 0 )->string();

                    try
                    {
                        m_libMgr.CreateLibrary( path );
                    }
                    catch( const IO_ERROR& e )
                    {
                        DisplayErrorMessage( nullptr,
                                wxString::Format( "Failed reading model library '%s'.", path ),
                                e.What() );
                    }
                }
                else
                    m_directives.emplace_back( node->string() );
            }
        }
    }
}


bool NETLIST_EXPORTER_SPICE::readRefName( SCH_SHEET_PATH& aSheet, SCH_SYMBOL& aSymbol,
                                          ITEM& aItem,
                                          std::set<std::string>& aRefNames )
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


void NETLIST_EXPORTER_SPICE::readModel( SCH_SHEET_PATH& aSheet, SCH_SYMBOL& aSymbol, ITEM& aItem )
{
    auto [modelName, model] = m_libMgr.CreateModel( aSymbol );
    aItem.modelName = ( modelName != "" ) ? modelName : ( "__" + aItem.refName );
    aItem.model = &model;

    // FIXME: Special cases for raw Spice models and KIBIS.
    if( auto rawSpiceModel = dynamic_cast<const SIM_MODEL_RAW_SPICE*>( aItem.model ) )
    {
        int libParamIndex = static_cast<int>( SIM_MODEL_RAW_SPICE::SPICE_PARAM::LIB );
        std::string path = rawSpiceModel->GetParam( libParamIndex ).value->ToString();

        if( path != "" )
            m_rawIncludes.insert( path );
    }
    else if( auto kibisModel = dynamic_cast<const SIM_MODEL_KIBIS*>( aItem.model ) )
    {
        aItem.modelName = aItem.model->GetFieldValue( &aSymbol.GetFields(),
                                                      SIM_LIBRARY_KIBIS::MODEL_FIELD );

        wxFileName cacheDir;
        cacheDir.AssignDir( PATHS::GetUserCachePath() );
        cacheDir.AppendDir( wxT( "ibis" ) );

        std::string libraryPath = fmt::format( "{}/{}.cache",
                                               std::string( cacheDir.GetPath() ),
                                               std::string( aSymbol.GetRef( &aSheet ) ) );
        wxFile cacheFile( libraryPath, wxFile::write );

        if( !cacheFile.IsOpened() )
        {
            DisplayErrorMessage( nullptr,
                    wxString::Format( _( "Could not open file '%s' to write IBIS model" ),
                                      libraryPath ) );
        }

        auto spiceGenerator = static_cast<const SPICE_GENERATOR_KIBIS&>( kibisModel->SpiceGenerator() );
        std::string modelData = spiceGenerator.IbisDevice( aSymbol.GetFields() );

        cacheFile.Write( wxString( modelData ) );
        m_rawIncludes.insert( libraryPath );
    }
}


void NETLIST_EXPORTER_SPICE::readPinNumbers( SCH_SYMBOL& aSymbol, ITEM& aItem )
{
    for( const PIN_INFO& pin : m_sortedSymbolPinList )
        aItem.pinNumbers.emplace_back( std::string( pin.num.ToUTF8() ) );
}


void NETLIST_EXPORTER_SPICE::readPinNetNames( SCH_SYMBOL& aSymbol, ITEM& aItem,
                                              int& aNcCounter )
{
    for( const PIN_INFO& pinInfo : m_sortedSymbolPinList )
    {
        std::string netName = GenerateItemPinNetName( std::string( pinInfo.netName.ToUTF8() ),
                                                      aNcCounter );

        aItem.pinNetNames.push_back( netName );
        m_nets.insert( netName );
    }
}


void NETLIST_EXPORTER_SPICE::writeInclude( OUTPUTFORMATTER& aFormatter, unsigned aNetlistOptions,
                                           const std::string& aPath )
{
    // First, expand env vars, if any.
    wxString expandedPath = ExpandEnvVarSubstitutions( aPath, &m_schematic->Prj() );
    wxString fullPath;

    if( aNetlistOptions & OPTION_ADJUST_INCLUDE_PATHS )
    {
        // Look for the library in known search locations.
        fullPath = ResolveFile( expandedPath, &Pgm().GetLocalEnvVariables(),
                                &m_schematic->Prj() );

        if( fullPath.IsEmpty() )
        {
            DisplayErrorMessage( nullptr,
                                 wxString::Format( _( "Could not find library file '%s'" ),
                                                   expandedPath ) );
            fullPath = expandedPath;
        }
    }
    else
        fullPath = expandedPath;

    aFormatter.Print( 0, ".include \"%s\"\n", TO_UTF8( fullPath ) );
}


void NETLIST_EXPORTER_SPICE::writeIncludes( OUTPUTFORMATTER& aFormatter, unsigned aNetlistOptions )
{
    for( auto&& [path, library] : m_libMgr.GetLibraries() )
    {
        if( dynamic_cast<const SIM_LIBRARY_SPICE*>( &library.get() ) )
            writeInclude( aFormatter, aNetlistOptions, path );
    }

    for( const std::string& path : m_rawIncludes )
        writeInclude( aFormatter, aNetlistOptions, path );
}


void NETLIST_EXPORTER_SPICE::writeModels( OUTPUTFORMATTER& aFormatter )
{
    for( const ITEM& item : m_items )
    {
        if( !item.model->IsEnabled() )
            continue;

        aFormatter.Print( 0, "%s",
                          item.model->SpiceGenerator().ModelLine( item.modelName ).c_str() );
    }
}


void NETLIST_EXPORTER_SPICE::writeItems( OUTPUTFORMATTER& aFormatter )
{
    for( const ITEM& item : m_items )
    {
        if( !item.model->IsEnabled() )
            continue;

        aFormatter.Print( 0, "%s",
                          item.model->SpiceGenerator().ItemLine( item.refName,
                                                                 item.modelName,
                                                                 item.pinNumbers,
                                                                 item.pinNetNames ).c_str() );
    }
}


void NETLIST_EXPORTER_SPICE::WriteDirectives( OUTPUTFORMATTER& aFormatter,
                                              unsigned         aNetlistOptions ) const
{
    if( aNetlistOptions & OPTION_SAVE_ALL_VOLTAGES )
        aFormatter.Print( 0, ".save all\n" );

    if( aNetlistOptions & OPTION_SAVE_ALL_CURRENTS )
        aFormatter.Print( 0, ".probe alli\n" );

    for( const std::string& directive : m_directives )
        aFormatter.Print( 0, "%s\n", directive.c_str() );
}


std::string NETLIST_EXPORTER_SPICE::GenerateItemPinNetName( const std::string& aNetName,
                                                            int& aNcCounter ) const
{
    std::string netName = aNetName;

    ReplaceForbiddenChars( netName );
    netName = std::string( UnescapeString( netName ).ToUTF8() );

    if( netName == "" )
        netName = fmt::format( "NC-{}", aNcCounter++ );

    return netName;
}


SCH_SHEET_LIST NETLIST_EXPORTER_SPICE::GetSheets( unsigned aNetlistOptions ) const
{
    if( aNetlistOptions & OPTION_CUR_SHEET_AS_ROOT )
        return SCH_SHEET_LIST( m_schematic->CurrentSheet().at( 0 ) );
    else
        return m_schematic->GetSheets();
}

