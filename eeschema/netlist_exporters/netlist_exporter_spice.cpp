/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 1992-2013 jp.charras at wanadoo.fr
 * Copyright (C) 2013 SoftPLC Corporation, Dick Hollenbeck <dick@softplc.com>
 * Copyright (C) 1992-2023 KiCad Developers, see AUTHORS.TXT for contributors.
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

#include <sim/kibis/kibis.h>
#include <netlist_exporter_spice.h>
#include <sim/ngspice_circuit_model.h>
#include <sim/sim_library_spice.h>
#include <sim/sim_model_raw_spice.h>
#include <sim/sim_model_ideal.h>
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

#include <dialogs/html_message_box.h>
#include <fmt/core.h>
#include <paths.h>
#include <wx/dir.h>
#include <locale_io.h>
#include "markup_parser.h"

#if 0

#include <pegtl.hpp>
#include <pegtl/contrib/parse_tree.hpp>

namespace NETLIST_EXPORTER_SPICE_PARSER
{
    using namespace SPICE_GRAMMAR;

    struct textGrammar : must<spiceSource> {};

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
#endif


std::string NAME_GENERATOR::Generate( const std::string& aProposedName )
{
    if( !m_names.count( aProposedName ) )
        return aProposedName;

    for( uint64_t i = 1; i < UINT64_MAX; ++i )
    {
        std::string name = fmt::format( "{}#{}", aProposedName, i );

        if( !m_names.count( name ) )
            return name;
    }

    // Should never happen.
    THROW_IO_ERROR( wxString::Format( _( "Failed to generate a name for '%s': exceeded UINT64_MAX" ),
                                      aProposedName ) );
}


NETLIST_EXPORTER_SPICE::NETLIST_EXPORTER_SPICE( SCHEMATIC_IFACE* aSchematic,
                                                wxWindow* aDialogParent ) :
    NETLIST_EXPORTER_BASE( aSchematic ),
    m_libMgr( &aSchematic->Prj() ),
    m_dialogParent( aDialogParent )
{
}


bool NETLIST_EXPORTER_SPICE::WriteNetlist( const wxString& aOutFileName, unsigned aNetlistOptions,
                                           REPORTER& aReporter )
{
    m_libMgr.SetReporter( &aReporter );
    FILE_OUTPUTFORMATTER formatter( aOutFileName, wxT( "wt" ), '\'' );
    return DoWriteNetlist( formatter, aNetlistOptions, aReporter );
}


bool NETLIST_EXPORTER_SPICE::DoWriteNetlist( OUTPUTFORMATTER& aFormatter, unsigned aNetlistOptions,
                                             REPORTER& aReporter )
{
    LOCALE_IO dummy;

   // Cleanup list to avoid duplicate if the netlist exporter is run more than once.
    m_rawIncludes.clear();

    if( !ReadSchematicAndLibraries( aNetlistOptions, aReporter ) )
        return false;

    WriteHead( aFormatter, aNetlistOptions );

    writeIncludes( aFormatter, aNetlistOptions );
    writeModels( aFormatter );

    // Skip this if there is no netlist to avoid an ngspice segfault
    if( !m_items.empty() )
        WriteDirectives( aFormatter, aNetlistOptions );

    writeItems( aFormatter );

    WriteTail( aFormatter, aNetlistOptions );

    return true;
}


void NETLIST_EXPORTER_SPICE::WriteHead( OUTPUTFORMATTER& aFormatter, unsigned aNetlistOptions )
{
    aFormatter.Print( 0, "KiCad schematic\n" );
}


void NETLIST_EXPORTER_SPICE::WriteTail( OUTPUTFORMATTER& aFormatter, unsigned aNetlistOptions )
{
    aFormatter.Print( 0, ".end\n" );
}


bool NETLIST_EXPORTER_SPICE::ReadSchematicAndLibraries( unsigned aNetlistOptions,
                                                        REPORTER& aReporter )
{
    wxString              msg;
    std::set<std::string> refNames; // Set of reference names to check for duplication.
    int                   ncCounter = 1;

    ReadDirectives( aNetlistOptions );

    m_nets.clear();
    m_items.clear();
    m_referencesAlreadyFound.Clear();
    m_libParts.clear();

    wxFileName cacheDir;
    cacheDir.AssignDir( PATHS::GetUserCachePath() );
    cacheDir.AppendDir( wxT( "ibis" ) );

    if( !cacheDir.DirExists() )
    {
        cacheDir.Mkdir( wxS_DIR_DEFAULT, wxPATH_MKDIR_FULL );

        if( !cacheDir.DirExists() )
        {
            wxLogTrace( wxT( "IBIS_CACHE:" ),
                        wxT( "%s:%s:%d\n * failed to create ibis cache directory '%s'" ),
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

    size_t numFilesFound = wxDir::GetAllFiles( dirName, &fileList, fileSpec );

    for( size_t ii = 0; ii < numFilesFound; ii++ )
    {
        // Completes path to specific file so we can get its "last access" date
        thisFile.SetFullName( fileList[ii] );
        wxRemoveFile( thisFile.GetFullPath() );
    }

    for( SCH_SHEET_PATH& sheet : GetSheets( aNetlistOptions ) )
    {
        for( SCH_ITEM* item : sheet.LastScreen()->Items().OfType( SCH_SYMBOL_T ) )
        {
            SCH_SYMBOL* symbol = findNextSymbol( item, &sheet );

            if( !symbol || symbol->GetFieldText( SIM_ENABLE_FIELD ) == wxT( "0" ) )
                continue;

            CreatePinList( symbol, &sheet, true );

            SPICE_ITEM spiceItem;

            for( int i = 0; i < symbol->GetFieldCount(); ++i )
            {
                spiceItem.fields.emplace_back( VECTOR2I(), i, symbol,
                                               symbol->GetFields()[ i ].GetName() );

                if( i == REFERENCE_FIELD )
                    spiceItem.fields.back().SetText( symbol->GetRef( &sheet ) );
                else
                    spiceItem.fields.back().SetText( symbol->GetFields()[i].GetShownText( 0, false ) );
            }

            wxString deviceType;
            wxString modelType;
            wxString modelParams;
            wxString pinMap;

            // JEY TODO: readModel() below will also do the inference, so I don't think this
            // accomplishes anything....
            // Infer RLC and VI models if they aren't specified
            if( SIM_MODEL::InferSimModel( *symbol, &spiceItem.fields, true,
                                          SIM_VALUE_GRAMMAR::NOTATION::SPICE, &deviceType,
                                          &modelType, &modelParams, &pinMap ) )
            {
                spiceItem.fields.emplace_back( symbol, -1, SIM_DEVICE_TYPE_FIELD );
                spiceItem.fields.back().SetText( deviceType );

                if( !modelType.IsEmpty() )
                {
                    spiceItem.fields.emplace_back( symbol, -1, SIM_TYPE_FIELD );
                    spiceItem.fields.back().SetText( modelType );
                }

                spiceItem.fields.emplace_back( symbol, -1, SIM_PARAMS_FIELD );
                spiceItem.fields.back().SetText( modelParams );

                spiceItem.fields.emplace_back( symbol, -1, SIM_PINS_FIELD );
                spiceItem.fields.back().SetText( pinMap );
            }

            try
            {
                readRefName( sheet, *symbol, spiceItem, refNames );
                readModel( sheet, *symbol, spiceItem );
                readPinNumbers( *symbol, spiceItem );
                readPinNetNames( *symbol, spiceItem, ncCounter );

                // TODO: transmission line handling?

                m_items.push_back( std::move( spiceItem ) );
            }
            catch( const IO_ERROR& e )
            {
                msg.Printf( _( "Error reading simulation model from symbol '%s':\n%s" ),
                            symbol->GetRef( &sheet ),
                            e.Problem() );
                aReporter.Report( msg, RPT_SEVERITY_ERROR );
            }
        }
    }

    return !aReporter.HasMessage();
}


void NETLIST_EXPORTER_SPICE::ConvertToSpiceMarkup( std::string& aNetName )
{
    MARKUP::MARKUP_PARSER         markupParser( aNetName );
    std::unique_ptr<MARKUP::NODE> root = markupParser.Parse();
    std::string                   converted;

    std::function<void( const std::unique_ptr<MARKUP::NODE>&)> convertMarkup =
            [&]( const std::unique_ptr<MARKUP::NODE>& aNode )
            {
                if( aNode )
                {
                    if( !aNode->is_root() )
                    {
                        if( aNode->isOverbar() )
                        {
                            // ~{CLK} is a different signal than CLK
                            converted += '~';
                        }
                        else if( aNode->isSubscript() || aNode->isSuperscript() )
                        {
                            // V_{OUT} is just a pretty-printed version of VOUT
                        }

                        if( aNode->has_content() )
                            converted += aNode->string();
                    }

                    for( const std::unique_ptr<MARKUP::NODE>& child : aNode->children )
                        convertMarkup( child );
                }
            };

    convertMarkup( root );

    // Remove ngspice-disallowed chars
    std::replace( converted.begin(), converted.end(), '%', '_' );
    std::replace( converted.begin(), converted.end(), '(', '_' );
    std::replace( converted.begin(), converted.end(), ')', '_' );
    std::replace( converted.begin(), converted.end(), ',', '_' );
    std::replace( converted.begin(), converted.end(), '[', '_' );
    std::replace( converted.begin(), converted.end(), ']', '_' );
    std::replace( converted.begin(), converted.end(), '<', '_' );
    std::replace( converted.begin(), converted.end(), '>', '_' );
    std::replace( converted.begin(), converted.end(), '~', '_' );

    aNetName = converted;
}


std::string NETLIST_EXPORTER_SPICE::GetItemName( const std::string& aRefName ) const
{
    const SPICE_ITEM* item = FindItem( aRefName );

    if( !item )
        return "";

    return item->model->SpiceGenerator().ItemName( *item );
}


const SPICE_ITEM* NETLIST_EXPORTER_SPICE::FindItem( const std::string& aRefName ) const
{
    const std::list<SPICE_ITEM>& spiceItems = GetItems();

    auto it = std::find_if( spiceItems.begin(), spiceItems.end(),
                            [aRefName]( const SPICE_ITEM& item )
                            {
                                return item.refName == aRefName;
                            } );

    if( it != spiceItems.end() )
        return &*it;

    return nullptr;
}


void NETLIST_EXPORTER_SPICE::ReadDirectives( unsigned aNetlistOptions )
{
    wxString msg;
    wxString text;

    m_directives.clear();

    for( const SCH_SHEET_PATH& sheet : GetSheets( aNetlistOptions ) )
    {
        for( SCH_ITEM* item : sheet.LastScreen()->Items() )
        {
            if( item->Type() == SCH_TEXT_T )
                text = static_cast<SCH_TEXT*>( item )->GetShownText();
            else if( item->Type() == SCH_TEXTBOX_T )
                text = static_cast<SCH_TEXTBOX*>( item )->GetShownText();
            else
                continue;

            // Send anything that contains directives to SPICE
            wxStringTokenizer tokenizer( text, wxT( "\r\n" ), wxTOKEN_STRTOK );
            bool              foundDirective = false;

            while( tokenizer.HasMoreTokens() )
            {
                wxString line = tokenizer.GetNextToken().Upper();

                if( line.StartsWith( wxT( "." ) ) )
                {
                    if(    line.StartsWith( wxT( ".AC" ) )
                        || line.StartsWith( wxT( ".CONTROL" ) )
                        || line.StartsWith( wxT( ".CSPARAM" ) )
                        || line.StartsWith( wxT( ".DISTO" ) )
                        || line.StartsWith( wxT( ".ELSE" ) )
                        || line.StartsWith( wxT( ".ELSEIF" ) )
                        || line.StartsWith( wxT( ".END" ) )
                        || line.StartsWith( wxT( ".ENDC" ) )
                        || line.StartsWith( wxT( ".ENDIF" ) )
                        || line.StartsWith( wxT( ".ENDS" ) )
                        || line.StartsWith( wxT( ".FOUR" ) )
                        || line.StartsWith( wxT( ".FUNC" ) )
                        || line.StartsWith( wxT( ".GLOBAL" ) )
                        || line.StartsWith( wxT( ".IC" ) )
                        || line.StartsWith( wxT( ".IF" ) )
                        || line.StartsWith( wxT( ".INCLUDE" ) )
                        || line.StartsWith( wxT( ".LIB" ) )
                        || line.StartsWith( wxT( ".MEAS" ) )
                        || line.StartsWith( wxT( ".MODEL" ) )
                        || line.StartsWith( wxT( ".NODESET" ) )
                        || line.StartsWith( wxT( ".NOISE" ) )
                        || line.StartsWith( wxT( ".OP" ) )
                        || line.StartsWith( wxT( ".OPTIONS" ) )
                        || line.StartsWith( wxT( ".PARAM" ) )
                        || line.StartsWith( wxT( ".PLOT" ) )
                        || line.StartsWith( wxT( ".PRINT" ) )
                        || line.StartsWith( wxT( ".PROBE" ) )
                        || line.StartsWith( wxT( ".PZ" ) )
                        || line.StartsWith( wxT( ".SAVE" ) )
                        || line.StartsWith( wxT( ".SENS" ) )
                        || line.StartsWith( wxT( ".SP" ) )
                        || line.StartsWith( wxT( ".SUBCKT" ) )
                        || line.StartsWith( wxT( ".TEMP" ) )
                        || line.StartsWith( wxT( ".TF" ) )
                        || line.StartsWith( wxT( ".TITLE" ) )
                        || line.StartsWith( wxT( ".TRAN" ) )
                        || line.StartsWith( wxT( ".WIDTH" ) ) )
                    {
                        foundDirective = true;
                        break;
                    }
                }
                else if( line.StartsWith( wxT( "K" ) ) )
                {
                    // Check for mutual inductor declaration
                    wxStringTokenizer line_t( line, wxT( " \t" ), wxTOKEN_STRTOK );

                    // Coupling ID
                    if( !line_t.HasMoreTokens() || !line_t.GetNextToken().StartsWith( wxT( "K" ) ) )
                        continue;

                    // Inductor 1 ID
                    if( !line_t.HasMoreTokens() || !line_t.GetNextToken().StartsWith( wxT( "L" ) ) )
                        continue;

                    // Inductor 2 ID
                    if( !line_t.HasMoreTokens() || !line_t.GetNextToken().StartsWith( wxT( "L" ) ) )
                        continue;

                    // That's probably distinctive enough not to bother trying to parse the
                    // coupling value.  If there's anything else, assume it's the value.
                    if( line_t.HasMoreTokens() )
                    {
                        foundDirective = true;
                        break;
                    }
                }
            }

            if( foundDirective )
                m_directives.emplace_back( text );
        }
    }
}


void NETLIST_EXPORTER_SPICE::readRefName( SCH_SHEET_PATH& aSheet, SCH_SYMBOL& aSymbol,
                                          SPICE_ITEM& aItem, std::set<std::string>& aRefNames )
{
    aItem.refName = aSymbol.GetRef( &aSheet );

    if( !aRefNames.insert( aItem.refName ).second )
        wxASSERT( wxT( "Duplicate refdes encountered; what happened to ReadyToNetlist()?" ) );
}


void NETLIST_EXPORTER_SPICE::readModel( SCH_SHEET_PATH& aSheet, SCH_SYMBOL& aSymbol,
                                        SPICE_ITEM& aItem )
{
    SIM_LIBRARY::MODEL libModel = m_libMgr.CreateModel( &aSheet, aSymbol );

    aItem.baseModelName = libModel.name;
    aItem.model = &libModel.model;

    std::string modelName = aItem.model->SpiceGenerator().ModelName( aItem );
    // Resolve model name collisions.
    aItem.modelName = m_modelNameGenerator.Generate( modelName );

    // FIXME: Don't have special cases for raw Spice models and KIBIS.
    if( auto rawSpiceModel = dynamic_cast<const SIM_MODEL_RAW_SPICE*>( aItem.model ) )
    {
        int      libParamIndex = static_cast<int>( SIM_MODEL_RAW_SPICE::SPICE_PARAM::LIB );
        wxString path = rawSpiceModel->GetParam( libParamIndex ).value->ToString();

        if( !path.IsEmpty() )
            m_rawIncludes.insert( path );
    }
    else if( auto kibisModel = dynamic_cast<const SIM_MODEL_KIBIS*>( aItem.model ) )
    {
        wxFileName cacheFn;
        cacheFn.AssignDir( PATHS::GetUserCachePath() );
        cacheFn.AppendDir( wxT( "ibis" ) );
        cacheFn.SetFullName( aSymbol.GetRef( &aSheet ) + wxT( ".cache" ) );

        wxFile cacheFile( cacheFn.GetFullPath(), wxFile::write );

        if( !cacheFile.IsOpened() )
        {
            DisplayErrorMessage( m_dialogParent,
                                 wxString::Format( _( "Could not open file '%s' to write "
                                                      "IBIS model" ),
                                                   cacheFn.GetFullPath() ) );
        }

        auto spiceGenerator = static_cast<const SPICE_GENERATOR_KIBIS&>( kibisModel->SpiceGenerator() );
        std::string modelData = spiceGenerator.IbisDevice( aItem, m_schematic->Prj(),
                                                           cacheFn.GetPath( wxPATH_GET_SEPARATOR ) );

        cacheFile.Write( wxString( modelData ) );
        m_rawIncludes.insert( cacheFn.GetFullPath() );
    }
}


void NETLIST_EXPORTER_SPICE::readPinNumbers( SCH_SYMBOL& aSymbol, SPICE_ITEM& aItem )
{
    for( const PIN_INFO& pin : m_sortedSymbolPinList )
        aItem.pinNumbers.emplace_back( std::string( pin.num.ToUTF8() ) );
}


void NETLIST_EXPORTER_SPICE::readPinNetNames( SCH_SYMBOL& aSymbol, SPICE_ITEM& aItem,
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
                                           const wxString& aPath )
{
    // First, expand env vars, if any.
    wxString expandedPath = ExpandEnvVarSubstitutions( aPath, &m_schematic->Prj() );

    // Path may have been authored by someone on a Windows box; convert it to UNIX format
    expandedPath.Replace( '\\', '/' );

    wxString fullPath;

    if( aNetlistOptions & OPTION_ADJUST_INCLUDE_PATHS )
    {
        // Look for the library in known search locations.
        fullPath = ResolveFile( expandedPath, &Pgm().GetLocalEnvVariables(), &m_schematic->Prj() );

        if( fullPath.IsEmpty() )
        {
            DisplayErrorMessage( m_dialogParent,
                                 wxString::Format( _( "Could not find library file '%s'" ),
                                                   expandedPath ) );
            fullPath = expandedPath;
        }
        else if( wxFileName::GetPathSeparator() == '\\' )
        {
            // Convert it to UNIX format (again) if ResolveFile() returned a Windows style path
            fullPath.Replace( '\\', '/' );
        }
    }
    else
    {
        fullPath = expandedPath;
    }

    aFormatter.Print( 0, ".include \"%s\"\n", TO_UTF8( fullPath ) );
}


void NETLIST_EXPORTER_SPICE::writeIncludes( OUTPUTFORMATTER& aFormatter, unsigned aNetlistOptions )
{
    for( auto& [path, library] : m_libMgr.GetLibraries() )
    {
        if( dynamic_cast<const SIM_LIBRARY_SPICE*>( &library.get() ) )
            writeInclude( aFormatter, aNetlistOptions, path );
    }

    for( const wxString& path : m_rawIncludes )
        writeInclude( aFormatter, aNetlistOptions, path );
}


void NETLIST_EXPORTER_SPICE::writeModels( OUTPUTFORMATTER& aFormatter )
{
    for( const SPICE_ITEM& item : m_items )
    {
        if( !item.model->IsEnabled() )
            continue;

        aFormatter.Print( 0, "%s", item.model->SpiceGenerator().ModelLine( item ).c_str() );
    }
}


void NETLIST_EXPORTER_SPICE::writeItems( OUTPUTFORMATTER& aFormatter )
{
    for( const SPICE_ITEM& item : m_items )
    {
        if( !item.model->IsEnabled() )
            continue;

        aFormatter.Print( 0, "%s", item.model->SpiceGenerator().ItemLine( item ).c_str() );
    }
}


void NETLIST_EXPORTER_SPICE::WriteDirectives( OUTPUTFORMATTER& aFormatter,
                                              unsigned aNetlistOptions ) const
{
    if( aNetlistOptions & OPTION_SAVE_ALL_VOLTAGES )
        aFormatter.Print( 0, ".save all\n" );

    if( aNetlistOptions & OPTION_SAVE_ALL_CURRENTS )
        aFormatter.Print( 0, ".probe alli\n" );

    for( const wxString& directive : m_directives )
    {
        bool simCommand = false;

        if( directive.StartsWith( "." ) )
        {
            wxString candidate = directive.Upper();

            simCommand = ( candidate.StartsWith( wxT( ".AC" ) )
                        || candidate.StartsWith( wxT( ".DC" ) )
                        || candidate.StartsWith( wxT( ".TRAN" ) )
                        || candidate.StartsWith( wxT( ".OP" ) )
                        || candidate.StartsWith( wxT( ".DISTO" ) )
                        || candidate.StartsWith( wxT( ".NOISE" ) )
                        || candidate.StartsWith( wxT( ".PZ" ) )
                        || candidate.StartsWith( wxT( ".SENS" ) )
                        || candidate.StartsWith( wxT( ".TF" ) ) );
        }

        if( !simCommand || ( aNetlistOptions & OPTION_SIM_COMMAND ) )
            aFormatter.Print( 0, "%s\n", UTF8( directive ).c_str() );
    }
}


std::string NETLIST_EXPORTER_SPICE::GenerateItemPinNetName( const std::string& aNetName,
                                                            int& aNcCounter ) const
{
    std::string netName = aNetName;

    ConvertToSpiceMarkup( netName );
    netName = std::string( UnescapeString( netName ).ToUTF8() );

    if( netName == "" )
        netName = fmt::format( "NC-{}", aNcCounter++ );

    return netName;
}


SCH_SHEET_LIST NETLIST_EXPORTER_SPICE::GetSheets( unsigned aNetlistOptions ) const
{
    if( aNetlistOptions & OPTION_CUR_SHEET_AS_ROOT )
        return SCH_SHEET_LIST( m_schematic->CurrentSheet().Last() );
    else
        return m_schematic->GetSheets();
}

