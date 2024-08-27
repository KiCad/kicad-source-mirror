/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 1992-2013 jp.charras at wanadoo.fr
 * Copyright (C) 2013 SoftPLC Corporation, Dick Hollenbeck <dick@softplc.com>
 * Copyright (C) 1992-2024 KiCad Developers, see AUTHORS.TXT for contributors.
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
#include <sim/spice_circuit_model.h>
#include <sim/sim_library_spice.h>
#include <sim/sim_model_raw_spice.h>
#include <common.h>
#include <confirm.h>
#include <pgm_base.h>
#include <env_paths.h>
#include <sim/sim_library_ibis.h>
#include <sim/sim_xspice_parser.h>
#include <sch_screen.h>
#include <sch_textbox.h>
#include <string_utils.h>
#include <ki_exception.h>

#include <dialogs/html_message_box.h>
#include <fmt/core.h>
#include <paths.h>
#include <wx/dir.h>
#include <wx/log.h>
#include <locale_io.h>
#include "markup_parser.h"


std::string NAME_GENERATOR::Generate( const std::string& aProposedName )
{
    std::string name = aProposedName;
    int         ii = 1;

    while( m_names.contains( name ) )
        name = fmt::format( "{}#{}", aProposedName, ii++ );

    return name;
}


NETLIST_EXPORTER_SPICE::NETLIST_EXPORTER_SPICE( SCHEMATIC_IFACE* aSchematic ) :
    NETLIST_EXPORTER_BASE( aSchematic ),
    m_libMgr( &aSchematic->Prj() )
{
}


bool NETLIST_EXPORTER_SPICE::WriteNetlist( const wxString& aOutFileName, unsigned aNetlistOptions,
                                           REPORTER& aReporter )
{
    FILE_OUTPUTFORMATTER formatter( aOutFileName, wxT( "wt" ), '\'' );
    return DoWriteNetlist( wxEmptyString, aNetlistOptions, formatter, aReporter );
}


bool NETLIST_EXPORTER_SPICE::DoWriteNetlist( const wxString& aSimCommand, unsigned aSimOptions,
                                             OUTPUTFORMATTER& aFormatter, REPORTER& aReporter )
{
    LOCALE_IO dummy;

   // Cleanup list to avoid duplicate if the netlist exporter is run more than once.
    m_rawIncludes.clear();

    bool result = ReadSchematicAndLibraries( aSimOptions, aReporter );

    WriteHead( aFormatter, aSimOptions );

    writeIncludes( aFormatter, aSimOptions );
    writeModels( aFormatter );

    // Skip this if there is no netlist to avoid an ngspice segfault
    if( !m_items.empty() )
        WriteDirectives( aSimCommand, aSimOptions, aFormatter );

    writeItems( aFormatter );

    WriteTail( aFormatter, aSimOptions );

    return result;
}


void NETLIST_EXPORTER_SPICE::WriteHead( OUTPUTFORMATTER& aFormatter, unsigned aNetlistOptions )
{
    aFormatter.Print( 0, ".title KiCad schematic\n" );
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

    for( SCH_SHEET_PATH& sheet : BuildSheetList( aNetlistOptions ) )
    {
        for( SCH_ITEM* item : sheet.LastScreen()->Items().OfType( SCH_SYMBOL_T ) )
        {
            SCH_SYMBOL* symbol = findNextSymbol( item, sheet );

            if( !symbol || symbol->GetExcludedFromSim() )
                continue;

            try
            {
                SPICE_ITEM            spiceItem;
                std::vector<PIN_INFO> pins = CreatePinList( symbol, sheet, true );

                for( const SCH_FIELD& field : symbol->GetFields() )
                {
                    spiceItem.fields.emplace_back( VECTOR2I(), -1, symbol, field.GetName() );

                    if( field.GetId() == REFERENCE_FIELD )
                        spiceItem.fields.back().SetText( symbol->GetRef( &sheet ) );
                    else
                        spiceItem.fields.back().SetText( field.GetShownText( &sheet, false ) );
                }

                readRefName( sheet, *symbol, spiceItem, refNames );
                readModel( sheet, *symbol, spiceItem, aReporter );
                readPinNumbers( *symbol, spiceItem, pins );
                readPinNetNames( *symbol, spiceItem, pins, ncCounter );
                readNodePattern( spiceItem );
                // TODO: transmission line handling?

                m_items.push_back( std::move( spiceItem ) );
            }
            catch( IO_ERROR& e )
            {
                aReporter.Report( e.What(), RPT_SEVERITY_ERROR );
            }
        }
    }

    return !aReporter.HasMessageOfSeverity( RPT_SEVERITY_UNDEFINED | RPT_SEVERITY_ERROR );
}


void NETLIST_EXPORTER_SPICE::ConvertToSpiceMarkup( wxString* aNetName )
{
    MARKUP::MARKUP_PARSER         markupParser( aNetName->ToStdString() );
    std::unique_ptr<MARKUP::NODE> root = markupParser.Parse();

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
                            *aNetName += '~';
                        }
                        else if( aNode->isSubscript() || aNode->isSuperscript() )
                        {
                            // V_{OUT} is just a pretty-printed version of VOUT
                        }

                        if( aNode->has_content() )
                            *aNetName += aNode->string();
                    }

                    for( const std::unique_ptr<MARKUP::NODE>& child : aNode->children )
                        convertMarkup( child );
                }
            };

    *aNetName = wxEmptyString;
    convertMarkup( root );

    // Replace all ngspice-disallowed chars in netnames by a '_'
    aNetName->Replace( '%', '_' );
    aNetName->Replace( '(', '_' );
    aNetName->Replace( ')', '_' );
    aNetName->Replace( ',', '_' );
    aNetName->Replace( '[', '_' );
    aNetName->Replace( ']', '_' );
    aNetName->Replace( '<', '_' );
    aNetName->Replace( '>', '_' );
    aNetName->Replace( '~', '_' );
    aNetName->Replace( ' ', '_' );

    // A net name on the root sheet with a label '/foo' is going to get titled "//foo".  This
    // will trip up ngspice as "//" opens a line comment.
    if( aNetName->StartsWith( wxS( "//" ) ) )
        aNetName->Replace( wxS( "//" ), wxS( "/root/" ), false /* replace all */ );
}


wxString NETLIST_EXPORTER_SPICE::GetItemName( const wxString& aRefName ) const
{
    if( const SPICE_ITEM* item = FindItem( aRefName ) )
        return item->model->SpiceGenerator().ItemName( *item );

    return wxEmptyString;
}


const SPICE_ITEM* NETLIST_EXPORTER_SPICE::FindItem( const wxString& aRefName ) const
{
    const std::string            refName = aRefName.ToStdString();
    const std::list<SPICE_ITEM>& spiceItems = GetItems();

    auto it = std::find_if( spiceItems.begin(), spiceItems.end(),
                            [refName]( const SPICE_ITEM& item )
                            {
                                return item.refName == refName;
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

    for( const SCH_SHEET_PATH& sheet : BuildSheetList( aNetlistOptions ) )
    {
        for( SCH_ITEM* item : sheet.LastScreen()->Items() )
        {
            if( item->GetExcludedFromSim() )
                continue;

            if( item->Type() == SCH_TEXT_T )
                text = static_cast<SCH_TEXT*>( item )->GetShownText( &sheet, false );
            else if( item->Type() == SCH_TEXTBOX_T )
                text = static_cast<SCH_TEXTBOX*>( item )->GetShownText( &sheet, false );
            else
                continue;

            // Send anything that contains directives to SPICE
            wxStringTokenizer tokenizer( text, wxT( "\r\n" ), wxTOKEN_STRTOK );
            bool              foundDirective = false;

            auto isDirective =
                    []( const wxString& line, const wxString& dir )
                    {
                        return line == dir || line.StartsWith( dir + wxS( " " ) );
                    };

            while( tokenizer.HasMoreTokens() )
            {
                wxString line = tokenizer.GetNextToken().Upper();

                if( line.StartsWith( wxT( "." ) ) )
                {
                    if(    isDirective( line, wxS( ".AC" ) )
                        || isDirective( line, wxS( ".CONTROL" ) )
                        || isDirective( line, wxS( ".CSPARAM" ) )
                        || isDirective( line, wxS( ".DISTO" ) )
                        || isDirective( line, wxS( ".DC" ) )
                        || isDirective( line, wxS( ".ELSE" ) )
                        || isDirective( line, wxS( ".ELSEIF" ) )
                        || isDirective( line, wxS( ".END" ) )
                        || isDirective( line, wxS( ".ENDC" ) )
                        || isDirective( line, wxS( ".ENDIF" ) )
                        || isDirective( line, wxS( ".ENDS" ) )
                        || isDirective( line, wxS( ".FOUR" ) )
                        || isDirective( line, wxS( ".FUNC" ) )
                        || isDirective( line, wxS( ".GLOBAL" ) )
                        || isDirective( line, wxS( ".IC" ) )
                        || isDirective( line, wxS( ".IF" ) )
                        || isDirective( line, wxS( ".INCLUDE" ) )
                        || isDirective( line, wxS( ".LIB" ) )
                        || isDirective( line, wxS( ".MEAS" ) )
                        || isDirective( line, wxS( ".MODEL" ) )
                        || isDirective( line, wxS( ".NODESET" ) )
                        || isDirective( line, wxS( ".NOISE" ) )
                        || isDirective( line, wxS( ".OP" ) )
                        || isDirective( line, wxS( ".OPTIONS" ) )
                        || isDirective( line, wxS( ".PARAM" ) )
                        || isDirective( line, wxS( ".PLOT" ) )
                        || isDirective( line, wxS( ".PRINT" ) )
                        || isDirective( line, wxS( ".PROBE" ) )
                        || isDirective( line, wxS( ".PZ" ) )
                        || isDirective( line, wxS( ".SAVE" ) )
                        || isDirective( line, wxS( ".SENS" ) )
                        || isDirective( line, wxS( ".SP" ) )
                        || isDirective( line, wxS( ".SUBCKT" ) )
                        || isDirective( line, wxS( ".TEMP" ) )
                        || isDirective( line, wxS( ".TF" ) )
                        || isDirective( line, wxS( ".TITLE" ) )
                        || isDirective( line, wxS( ".TRAN" ) )
                        || isDirective( line, wxS( ".WIDTH" ) ) )
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
                                        SPICE_ITEM& aItem, REPORTER& aReporter )
{
    const SIM_LIBRARY::MODEL& libModel = m_libMgr.CreateModel( &aSheet, aSymbol, aReporter );

    aItem.baseModelName = libModel.name;
    aItem.model = &libModel.model;

    std::string modelName = aItem.model->SpiceGenerator().ModelName( aItem );
    // Resolve model name collisions.
    aItem.modelName = m_modelNameGenerator.Generate( modelName );

    // FIXME: Don't have special cases for raw Spice models and KIBIS.
    if( auto rawSpiceModel = dynamic_cast<const SIM_MODEL_RAW_SPICE*>( aItem.model ) )
    {
        int      libParamIndex = static_cast<int>( SIM_MODEL_RAW_SPICE::SPICE_PARAM::LIB );
        wxString path = rawSpiceModel->GetParam( libParamIndex ).value;

        if( !path.IsEmpty() )
            m_rawIncludes.insert( path );
    }
    else if( auto ibisModel = dynamic_cast<const SIM_MODEL_IBIS*>( aItem.model ) )
    {
        wxFileName cacheFn;
        cacheFn.AssignDir( PATHS::GetUserCachePath() );
        cacheFn.AppendDir( wxT( "ibis" ) );
        cacheFn.SetFullName( aSymbol.GetRef( &aSheet ) + wxT( ".cache" ) );

        wxFile cacheFile( cacheFn.GetFullPath(), wxFile::write );

        if( !cacheFile.IsOpened() )
        {
            wxLogError( _( "Could not open file '%s' to write IBIS model" ),
                        cacheFn.GetFullPath() );
        }

        auto spiceGenerator = static_cast<const SPICE_GENERATOR_IBIS&>( ibisModel->SpiceGenerator() );

        wxString    cacheFilepath = cacheFn.GetPath( wxPATH_GET_VOLUME | wxPATH_GET_SEPARATOR );
        std::string modelData = spiceGenerator.IbisDevice( aItem, m_schematic->Prj(),
                                                           cacheFilepath, aReporter );

        cacheFile.Write( wxString( modelData ) );
        m_rawIncludes.insert( cacheFn.GetFullPath() );
    }
}


void NETLIST_EXPORTER_SPICE::readPinNumbers( SCH_SYMBOL& aSymbol, SPICE_ITEM& aItem,
                                             const std::vector<PIN_INFO>& aPins  )
{
    for( const PIN_INFO& pin : aPins )
        aItem.pinNumbers.emplace_back( pin.num.ToStdString() );
}


void NETLIST_EXPORTER_SPICE::readPinNetNames( SCH_SYMBOL& aSymbol, SPICE_ITEM& aItem,
                                              const std::vector<PIN_INFO>& aPins, int& aNcCounter )
{
    for( const PIN_INFO& pinInfo : aPins )
    {
        wxString netName = GenerateItemPinNetName( pinInfo.netName, aNcCounter );

        aItem.pinNetNames.push_back( netName.ToStdString() );
        m_nets.insert( netName );
    }
}


void NETLIST_EXPORTER_SPICE::getNodePattern( SPICE_ITEM&               aItem,
                                             std::vector<std::string>& aModifiers )
{
    std::string input = SIM_MODEL::GetFieldValue( &aItem.fields, SIM_NODES_FORMAT_FIELD, true );
    
    if( input == "" )
        return;

    tao::pegtl::string_input<>                    in( input, "Sim.NodesFormat field" );
    std::unique_ptr<tao::pegtl::parse_tree::node> root;
    std::string                                   singleNodeModifier;

    try
    {
        root = tao::pegtl::parse_tree::parse<SIM_XSPICE_PARSER_GRAMMAR::nodeSequenceGrammar,
                                             SIM_XSPICE_PARSER_GRAMMAR::spiceUnitSelector,
                                             tao::pegtl::nothing,
                                             SIM_XSPICE_PARSER_GRAMMAR::control>( in );
        for( const auto& node : root->children )
        {
            if( node->is_type<SIM_XSPICE_PARSER_GRAMMAR::squareBracketC>() )
            {
                //we want ']' to close previous ?
                aModifiers.back().append( node->string() );
            }
            else
            { //rest goes to the new singleNodeModifier
                singleNodeModifier.append( node->string() );
            }

            if( node->is_type<SIM_XSPICE_PARSER_GRAMMAR::nodeName>() )
            {
                aModifiers.push_back( singleNodeModifier );
                singleNodeModifier.erase( singleNodeModifier.begin(), singleNodeModifier.end() );
            }
        }
    }
    catch( const tao::pegtl::parse_error& e )
    {
        THROW_IO_ERROR( wxString::Format( _( "Error in parsing model '%s', error: '%s'" ),
                                          aItem.refName, e.what() ) );
    }
}
void NETLIST_EXPORTER_SPICE::readNodePattern( SPICE_ITEM& aItem )
{
    std::vector<std::string> xspicePattern;
    NETLIST_EXPORTER_SPICE::getNodePattern( aItem, xspicePattern );

    if( xspicePattern.empty() )
        return;

    if( xspicePattern.size() != aItem.pinNetNames.size() )
    {
        THROW_IO_ERROR( wxString::Format( _( "Error in parsing model '%s', wrong number of nodes "
                                             "'?' in Sim.NodesFormat compared to connections" ),
                                          aItem.refName ) );
        return;
    }

    auto itNetNames = aItem.pinNetNames.begin();

    for( std::string& pattern : xspicePattern )
    {
        // ngspice does not care about aditional spaces, and we make sure that "%d?" is separated
        const std::string netName = " " + *itNetNames + " ";
        pattern.replace( pattern.find( "?" ), 1, netName );
        *itNetNames = pattern;
        ++itNetNames;
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
            wxLogError( _( "Could not find library file '%s'" ), expandedPath );
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
    for( const auto& [path, library] : m_libMgr.GetLibraries() )
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


void NETLIST_EXPORTER_SPICE::WriteDirectives( const wxString& aSimCommand, unsigned aSimOptions,
                                              OUTPUTFORMATTER& aFormatter ) const
{
    if( aSimOptions & OPTION_SAVE_ALL_VOLTAGES )
        aFormatter.Print( 0, ".save all\n" );

    if( aSimOptions & OPTION_SAVE_ALL_CURRENTS )
        aFormatter.Print( 0, ".probe alli\n" );

    if( aSimOptions & OPTION_SAVE_ALL_DISSIPATIONS )
    {
        for( const SPICE_ITEM& item : m_items )
        {
            // ngspice (v39) does not support power measurement for XSPICE devices
            // XPSICE devices are marked with 'A'
            std::string itemName = item.model->SpiceGenerator().ItemName( item );

            if( ( item.model->GetPinCount() >= 2 ) && ( itemName.size() > 0 )
                && ( itemName.c_str()[0] != 'A' ) )
            {
                aFormatter.Print( 0, ".probe p(%s)\n", itemName.c_str() );
            }
        }
    }

    auto isSimCommand =
            []( const wxString& candidate, const wxString& dir )
            {
                return candidate == dir || candidate.StartsWith( dir + wxS( " " ) );
            };

    for( const wxString& directive : m_directives )
    {
        bool simCommand = false;

        if( directive.StartsWith( "." ) )
        {
            wxString candidate = directive.Upper();

            simCommand = ( isSimCommand( candidate, wxS( ".AC" ) )
                        || isSimCommand( candidate, wxS( ".DC" ) )
                        || isSimCommand( candidate, wxS( ".TRAN" ) )
                        || isSimCommand( candidate, wxS( ".OP" ) )
                        || isSimCommand( candidate, wxS( ".DISTO" ) )
                        || isSimCommand( candidate, wxS( ".NOISE" ) )
                        || isSimCommand( candidate, wxS( ".PZ" ) )
                        || isSimCommand( candidate, wxS( ".SENS" ) )
                        || isSimCommand( candidate, wxS( ".TF" ) ) );
        }

        if( !simCommand || ( aSimOptions & OPTION_SIM_COMMAND ) )
            aFormatter.Print( 0, "%s\n", UTF8( directive ).c_str() );
    }
}


wxString NETLIST_EXPORTER_SPICE::GenerateItemPinNetName( const wxString& aNetName,
                                                         int& aNcCounter ) const
{
    wxString netName = UnescapeString( aNetName );

    ConvertToSpiceMarkup( &netName );

    if( netName.IsEmpty() )
        netName.Printf( wxS( "NC-%d" ), aNcCounter++ );

    return netName;
}


SCH_SHEET_LIST NETLIST_EXPORTER_SPICE::BuildSheetList( unsigned aNetlistOptions ) const
{
    SCH_SHEET_LIST sheets;

    if( aNetlistOptions & OPTION_CUR_SHEET_AS_ROOT )
        sheets = SCH_SHEET_LIST( m_schematic->CurrentSheet().Last() );
    else
        sheets = m_schematic->BuildSheetListSortedByPageNumbers();

    alg::delete_if( sheets,
                    [&]( const SCH_SHEET_PATH& sheet )
                    {
                        return sheet.GetExcludedFromSim();
                    } );

    return sheets;
}

