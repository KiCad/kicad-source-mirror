/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2017 CERN
 * Copyright (C) 2017-2024 Kicad Developers, see AUTHORS.txt for contributors.
 *
 * @author Alejandro García Montoro <alejandro.garciamontoro@gmail.com>
 * @author Maciej Suminski <maciej.suminski@cern.ch>
 * @author Russell Oliver <roliver8143@gmail.com>
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
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <sch_io/eagle/sch_io_eagle.h>

#include <locale_io.h>

#include <algorithm>
#include <memory>
#include <wx/filename.h>
#include <wx/string.h>
#include <wx/tokenzr.h>
#include <wx/wfstream.h>
#include <wx/txtstrm.h>
#include <wx/xml/xml.h>

#include <font/fontconfig.h>
#include <io/eagle/eagle_parser.h>
#include <lib_id.h>
#include <progress_reporter.h>
#include <project.h>
#include <project/net_settings.h>
#include <project_sch.h>
#include <sch_bus_entry.h>
#include <sch_edit_frame.h>
#include <sch_io/kicad_legacy/sch_io_kicad_legacy.h>
#include <sch_junction.h>
#include <sch_label.h>
#include <sch_marker.h>
#include <sch_pin.h>
#include <sch_screen.h>
#include <sch_shape.h>
#include <sch_sheet.h>
#include <sch_sheet_path.h>
#include <sch_sheet_pin.h>
#include <sch_symbol.h>
#include <schematic.h>
#include <string_utils.h>
#include <symbol_lib_table.h>
#include <wildcards_and_files_ext.h>


// Eagle schematic axes are aligned with x increasing left to right and Y increasing bottom to top
// KiCad schematic axes are aligned with x increasing left to right and Y increasing top to bottom.

using namespace std;

/**
 * Map of EAGLE pin type values to KiCad pin type values
 */
static const std::map<wxString, ELECTRICAL_PINTYPE> pinDirectionsMap = {
    { wxT( "sup" ),    ELECTRICAL_PINTYPE::PT_POWER_IN },
    { wxT( "pas" ),    ELECTRICAL_PINTYPE::PT_PASSIVE },
    { wxT( "out" ),    ELECTRICAL_PINTYPE::PT_OUTPUT },
    { wxT( "in" ),     ELECTRICAL_PINTYPE::PT_INPUT },
    { wxT( "nc" ),     ELECTRICAL_PINTYPE::PT_NC },
    { wxT( "io" ),     ELECTRICAL_PINTYPE::PT_BIDI },
    { wxT( "oc" ),     ELECTRICAL_PINTYPE::PT_OPENCOLLECTOR },
    { wxT( "hiz" ),    ELECTRICAL_PINTYPE::PT_TRISTATE },
    { wxT( "pwr" ),    ELECTRICAL_PINTYPE::PT_POWER_IN },
};


///< Compute a bounding box for all items in a schematic sheet
static BOX2I getSheetBbox( SCH_SHEET* aSheet )
{
    BOX2I bbox;

    for( SCH_ITEM* item : aSheet->GetScreen()->Items() )
        bbox.Merge( item->GetBoundingBox() );

    return bbox;
}


///< Extract the net name part from a pin name (e.g. return 'GND' for pin named 'GND@2')
static inline wxString extractNetName( const wxString& aPinName )
{
    return aPinName.BeforeFirst( '@' );
}


SCH_SHEET* SCH_IO_EAGLE::getCurrentSheet()
{
    return m_sheetPath.Last();
}


SCH_SCREEN* SCH_IO_EAGLE::getCurrentScreen()
{
    SCH_SHEET* currentSheet = m_sheetPath.Last();
    wxCHECK( currentSheet, nullptr );
    return currentSheet->GetScreen();
}


wxString SCH_IO_EAGLE::getLibName()
{
    if( m_libName.IsEmpty() )
    {
        // Try to come up with a meaningful name
        m_libName = m_schematic->Prj().GetProjectName();

        if( m_libName.IsEmpty() )
        {
            wxFileName fn( m_rootSheet->GetFileName() );
            m_libName = fn.GetName();
        }

        if( m_libName.IsEmpty() )
            m_libName = wxT( "noname" );

        m_libName += wxT( "-eagle-import" );
        m_libName = LIB_ID::FixIllegalChars( m_libName, true ).wx_str();
    }

    return m_libName;
}


wxFileName SCH_IO_EAGLE::getLibFileName()
{
    wxFileName fn;

    wxCHECK( m_schematic, fn );

    fn.Assign( m_schematic->Prj().GetProjectPath(), getLibName(),
               FILEEXT::KiCadSymbolLibFileExtension );

    return fn;
}


void SCH_IO_EAGLE::loadLayerDefs( const std::vector<std::unique_ptr<ELAYER>>& aLayers )
{
    // match layers based on their names
    for( const std::unique_ptr<ELAYER>& elayer : aLayers )
    {
        /**
         * Layers in KiCad schematics are not actually layers, but abstract groups mainly used to
         * decide item colors.
         *
         * <layers>
         *     <layer number="90" name="Modules" color="5" fill="1" visible="yes" active="yes"/>
         *     <layer number="91" name="Nets" color="2" fill="1" visible="yes" active="yes"/>
         *     <layer number="92" name="Busses" color="1" fill="1" visible="yes" active="yes"/>
         *     <layer number="93" name="Pins" color="2" fill="1" visible="no" active="yes"/>
         *     <layer number="94" name="Symbols" color="4" fill="1" visible="yes" active="yes"/>
         *     <layer number="95" name="Names" color="7" fill="1" visible="yes" active="yes"/>
         *     <layer number="96" name="Values" color="7" fill="1" visible="yes" active="yes"/>
         *     <layer number="97" name="Info" color="7" fill="1" visible="yes" active="yes"/>
         *     <layer number="98" name="Guide" color="6" fill="1" visible="yes" active="yes"/>
         * </layers>
         */

        if( elayer->name == wxT( "Nets" ) )
            m_layerMap[elayer->number] = LAYER_WIRE;
        else if( elayer->name == wxT( "Info" ) || elayer->name == wxT( "Guide" ) )
            m_layerMap[elayer->number] = LAYER_NOTES;
        else if( elayer->name == wxT( "Busses" ) )
            m_layerMap[elayer->number] = LAYER_BUS;
    }
}


SCH_LAYER_ID SCH_IO_EAGLE::kiCadLayer( int aEagleLayer )
{
    auto it = m_layerMap.find( aEagleLayer );
    return it == m_layerMap.end() ? LAYER_NOTES : it->second;
}


// Return the KiCad symbol orientation based on eagle rotation degrees.
static SYMBOL_ORIENTATION_T kiCadComponentRotation( float eagleDegrees )
{
    int roti = int( eagleDegrees );

    switch( roti )
    {
    case 0:   return SYM_ORIENT_0;
    case 90:  return SYM_ORIENT_90;
    case 180: return SYM_ORIENT_180;
    case 270: return SYM_ORIENT_270;

    default:
        wxASSERT_MSG( false, wxString::Format( wxT( "Unhandled orientation (%d degrees)" ),
                                               roti ) );
        return SYM_ORIENT_0;
    }
}


// Calculate text alignment based on the given Eagle text alignment parameters.
static void eagleToKicadAlignment( EDA_TEXT* aText, int aEagleAlignment, int aRelDegress,
                                   bool aMirror, bool aSpin, int aAbsDegress )
{
    int align = aEagleAlignment;

    if( aRelDegress == 90 )
    {
        aText->SetTextAngle( ANGLE_VERTICAL );
    }
    else if( aRelDegress == 180 )
    {
        align = -align;
    }
    else if( aRelDegress == 270 )
    {
        aText->SetTextAngle( ANGLE_VERTICAL );
        align = -align;
    }

    if( aMirror == true )
    {
        if( aAbsDegress == 90 || aAbsDegress == 270 )
        {
            if( align == ETEXT::BOTTOM_RIGHT )
                align = ETEXT::TOP_RIGHT;
            else if( align == ETEXT::BOTTOM_LEFT )
                align = ETEXT::TOP_LEFT;
            else if( align == ETEXT::TOP_LEFT )
                align = ETEXT::BOTTOM_LEFT;
            else if( align == ETEXT::TOP_RIGHT )
                align = ETEXT::BOTTOM_RIGHT;
        }
        else if( aAbsDegress == 0 || aAbsDegress == 180 )
        {
            if( align == ETEXT::BOTTOM_RIGHT )
                align = ETEXT::BOTTOM_LEFT;
            else if( align == ETEXT::BOTTOM_LEFT )
                align = ETEXT::BOTTOM_RIGHT;
            else if( align == ETEXT::TOP_LEFT )
                align = ETEXT::TOP_RIGHT;
            else if( align == ETEXT::TOP_RIGHT )
                align = ETEXT::TOP_LEFT;
            else if( align == ETEXT::CENTER_LEFT )
                align = ETEXT::CENTER_RIGHT;
            else if( align == ETEXT::CENTER_RIGHT )
                align = ETEXT::CENTER_LEFT;
        }
    }

    switch( align )
    {
    case ETEXT::CENTER:
        aText->SetHorizJustify( GR_TEXT_H_ALIGN_CENTER );
        aText->SetVertJustify( GR_TEXT_V_ALIGN_CENTER );
        break;

    case ETEXT::CENTER_LEFT:
        aText->SetHorizJustify( GR_TEXT_H_ALIGN_LEFT );
        aText->SetVertJustify( GR_TEXT_V_ALIGN_CENTER );
        break;

    case ETEXT::CENTER_RIGHT:
        aText->SetHorizJustify( GR_TEXT_H_ALIGN_RIGHT );
        aText->SetVertJustify( GR_TEXT_V_ALIGN_CENTER );
        break;

    case ETEXT::TOP_CENTER:
        aText->SetHorizJustify( GR_TEXT_H_ALIGN_CENTER );
        aText->SetVertJustify( GR_TEXT_V_ALIGN_TOP );
        break;

    case ETEXT::TOP_LEFT:
        aText->SetHorizJustify( GR_TEXT_H_ALIGN_LEFT );
        aText->SetVertJustify( GR_TEXT_V_ALIGN_TOP );
        break;

    case ETEXT::TOP_RIGHT:
        aText->SetHorizJustify( GR_TEXT_H_ALIGN_RIGHT );
        aText->SetVertJustify( GR_TEXT_V_ALIGN_TOP );
        break;

    case ETEXT::BOTTOM_CENTER:
        aText->SetHorizJustify( GR_TEXT_H_ALIGN_CENTER );
        aText->SetVertJustify( GR_TEXT_V_ALIGN_BOTTOM );
        break;

    case ETEXT::BOTTOM_LEFT:
        aText->SetHorizJustify( GR_TEXT_H_ALIGN_LEFT );
        aText->SetVertJustify( GR_TEXT_V_ALIGN_BOTTOM );
        break;

    case ETEXT::BOTTOM_RIGHT:
        aText->SetHorizJustify( GR_TEXT_H_ALIGN_RIGHT );
        aText->SetVertJustify( GR_TEXT_V_ALIGN_BOTTOM );
        break;

    default:
        aText->SetHorizJustify( GR_TEXT_H_ALIGN_RIGHT );
        aText->SetVertJustify( GR_TEXT_V_ALIGN_BOTTOM );
        break;
    }
}


SCH_IO_EAGLE::SCH_IO_EAGLE() : SCH_IO( wxS( "EAGLE" ) ),
    m_rootSheet( nullptr ),
    m_schematic( nullptr ),
    m_module( nullptr ),
    m_sheetIndex( 1 )
{
    m_reporter = &WXLOG_REPORTER::GetInstance();
}


SCH_IO_EAGLE::~SCH_IO_EAGLE()
{
}


int SCH_IO_EAGLE::GetModifyHash() const
{
    return 0;
}


SCH_SHEET* SCH_IO_EAGLE::LoadSchematicFile( const wxString& aFileName, SCHEMATIC* aSchematic,
                                            SCH_SHEET*             aAppendToMe,
                                            const std::map<std::string, UTF8>* aProperties )
{
    wxASSERT( !aFileName || aSchematic != nullptr );
    LOCALE_IO toggle; // toggles on, then off, the C locale.

    // Show the font substitution warnings
    fontconfig::FONTCONFIG::SetReporter( &WXLOG_REPORTER::GetInstance() );

    m_filename = aFileName;
    m_schematic = aSchematic;

    if( m_progressReporter )
    {
        m_progressReporter->Report( wxString::Format( _( "Loading %s..." ), aFileName ) );

        if( !m_progressReporter->KeepRefreshing() )
            THROW_IO_ERROR( ( "Open canceled by user." ) );
    }

    // Load the document
    wxXmlDocument xmlDocument = loadXmlDocument( m_filename.GetFullPath() );

    // Retrieve the root as current node
    wxXmlNode* currentNode = xmlDocument.GetRoot();

    if( m_progressReporter )
        m_progressReporter->SetNumPhases( static_cast<int>( GetNodeCount( currentNode ) ) );

    // Delete on exception, if I own m_rootSheet, according to aAppendToMe
    unique_ptr<SCH_SHEET> deleter( aAppendToMe ? nullptr : m_rootSheet );

    wxFileName newFilename( m_filename );
    newFilename.SetExt( FILEEXT::KiCadSchematicFileExtension );

    if( aAppendToMe )
    {
        wxCHECK_MSG( aSchematic->IsValid(), nullptr,
                     wxT( "Can't append to a schematic with no root!" ) );

        m_rootSheet = &aSchematic->Root();
    }
    else
    {
        m_rootSheet = new SCH_SHEET( aSchematic );
        m_rootSheet->SetFileName( newFilename.GetFullPath() );
        aSchematic->SetRoot( m_rootSheet );
    }

    if( !m_rootSheet->GetScreen() )
    {
        SCH_SCREEN* screen = new SCH_SCREEN( m_schematic );
        screen->SetFileName( newFilename.GetFullPath() );
        m_rootSheet->SetScreen( screen );

        // Virtual root sheet UUID must be the same as the schematic file UUID.
        const_cast<KIID&>( m_rootSheet->m_Uuid ) = screen->GetUuid();
    }

    SYMBOL_LIB_TABLE* libTable = PROJECT_SCH::SchSymbolLibTable( &m_schematic->Prj() );

    wxCHECK_MSG( libTable, nullptr, wxT( "Could not load symbol lib table." ) );

    m_pi.reset( SCH_IO_MGR::FindPlugin( SCH_IO_MGR::SCH_KICAD ) );
    m_properties = std::make_unique<std::map<std::string, UTF8>>();
    ( *m_properties )[SCH_IO_KICAD_LEGACY::PropBuffering] = "";

    /// @note No check is being done here to see if the existing symbol library exists so this
    ///       will overwrite the existing one.
    if( !libTable->HasLibrary( getLibName() ) )
    {
        // Create a new empty symbol library.
        m_pi->CreateLibrary( getLibFileName().GetFullPath() );
        wxString libTableUri = wxT( "${KIPRJMOD}/" ) + getLibFileName().GetFullName();

        // Add the new library to the project symbol library table.
        libTable->InsertRow( new SYMBOL_LIB_TABLE_ROW( getLibName(), libTableUri,
                                                       wxT( "KiCad" ) ) );

        // Save project symbol library table.
        wxFileName fn( m_schematic->Prj().GetProjectPath(),
                       SYMBOL_LIB_TABLE::GetSymbolLibTableFileName() );

        // So output formatter goes out of scope and closes the file before reloading.
        {
            FILE_OUTPUTFORMATTER formatter( fn.GetFullPath() );
            libTable->Format( &formatter, 0 );
        }

        // Reload the symbol library table.
        m_schematic->Prj().SetElem( PROJECT::ELEM::SYMBOL_LIB_TABLE, nullptr );
        PROJECT_SCH::SchSymbolLibTable( &m_schematic->Prj() );
    }

    m_eagleDoc = std::make_unique<EAGLE_DOC>( currentNode, this );

    // If the attribute is found, store the Eagle version;
    // otherwise, store the dummy "0.0" version.
    m_version = ( m_eagleDoc->version.IsEmpty() ) ? wxString( wxS( "0.0" ) ) : m_eagleDoc->version;

    // Load drawing
    loadDrawing( m_eagleDoc->drawing );

    m_pi->SaveLibrary( getLibFileName().GetFullPath() );

    SCH_SCREENS allSheets( m_rootSheet );
    allSheets.UpdateSymbolLinks(); // Update all symbol library links for all sheets.

    return m_rootSheet;
}


void SCH_IO_EAGLE::EnumerateSymbolLib( wxArrayString&         aSymbolNameList,
                                       const wxString&        aLibraryPath,
                                       const std::map<std::string, UTF8>* aProperties )
{
    m_filename = aLibraryPath;
    m_libName = m_filename.GetName();

    ensureLoadedLibrary( aLibraryPath );

    auto it = m_eagleLibs.find( m_libName );

    if( it != m_eagleLibs.end() )
    {
        for( const auto& [symName, libSymbol] : it->second.KiCadSymbols )
            aSymbolNameList.push_back( symName );
    }
}


void SCH_IO_EAGLE::EnumerateSymbolLib( std::vector<LIB_SYMBOL*>& aSymbolList,
                                       const wxString&           aLibraryPath,
                                       const std::map<std::string, UTF8>*    aProperties )
{
    m_filename = aLibraryPath;
    m_libName = m_filename.GetName();

    ensureLoadedLibrary( aLibraryPath );

    auto it = m_eagleLibs.find( m_libName );

    if( it != m_eagleLibs.end() )
    {
        for( const auto& [symName, libSymbol] : it->second.KiCadSymbols )
            aSymbolList.push_back( libSymbol.get() );
    }
}


LIB_SYMBOL* SCH_IO_EAGLE::LoadSymbol( const wxString& aLibraryPath, const wxString& aAliasName,
                                      const std::map<std::string, UTF8>* aProperties )
{
    m_filename = aLibraryPath;
    m_libName = m_filename.GetName();

    ensureLoadedLibrary( aLibraryPath );

    auto it = m_eagleLibs.find( m_libName );

    if( it != m_eagleLibs.end() )
    {
        auto it2 = it->second.KiCadSymbols.find( aAliasName );

        if( it2 != it->second.KiCadSymbols.end() )
            return it2->second.get();
    }

    return nullptr;
}


long long SCH_IO_EAGLE::getLibraryTimestamp( const wxString& aLibraryPath ) const
{
    wxFileName fn( aLibraryPath );

    if( fn.IsFileReadable() && fn.GetModificationTime().IsValid() )
        return fn.GetModificationTime().GetValue().GetValue();
    else
        return wxDateTime( 0.0 ).GetValue().GetValue();
}


void SCH_IO_EAGLE::ensureLoadedLibrary( const wxString& aLibraryPath )
{
    // Suppress font substitution warnings
    fontconfig::FONTCONFIG::SetReporter( nullptr );

    if( m_eagleLibs.find( m_libName ) != m_eagleLibs.end() )
    {
        wxCHECK( m_timestamps.count( m_libName ), /*void*/ );

        if( m_timestamps.at( m_libName ) == getLibraryTimestamp( aLibraryPath ) )
            return;
    }

    LOCALE_IO toggle; // toggles on, then off, the C locale.

    if( m_progressReporter )
    {
        m_progressReporter->Report( wxString::Format( _( "Loading %s..." ), aLibraryPath ) );

        if( !m_progressReporter->KeepRefreshing() )
            THROW_IO_ERROR( ( "Open canceled by user." ) );
    }

    // Load the document
    wxXmlDocument xmlDocument = loadXmlDocument( m_filename.GetFullPath() );

    // Retrieve the root as current node
    std::unique_ptr<EAGLE_DOC> doc = std::make_unique<EAGLE_DOC>( xmlDocument.GetRoot(), this );

    // If the attribute is found, store the Eagle version;
    // otherwise, store the dummy "0.0" version.
    m_version = ( doc->version.IsEmpty() ) ? wxString( wxS( "0.0" ) ) : doc->version;

    // Load drawing
    loadDrawing( doc->drawing );

    // Remember timestamp
    m_timestamps[m_libName] = getLibraryTimestamp( aLibraryPath );
}


wxXmlDocument SCH_IO_EAGLE::loadXmlDocument( const wxString& aFileName )
{
    wxXmlDocument      xmlDocument;
    wxFFileInputStream stream( m_filename.GetFullPath() );

    if( !stream.IsOk() )
    {
        THROW_IO_ERROR(
                wxString::Format( _( "Unable to read file '%s'." ), m_filename.GetFullPath() ) );
    }

    // read first line to check for Eagle XML format file
    wxTextInputStream text( stream );
    wxString          line = text.ReadLine();

    if( !line.StartsWith( wxT( "<?xml" ) ) && !line.StartsWith( wxT( "<!--" ) ) )
    {
        THROW_IO_ERROR( wxString::Format( _( "'%s' is an Eagle binary-format file; "
                                             "only Eagle XML-format files can be imported." ),
                                          m_filename.GetFullPath() ) );
    }

    if( !xmlDocument.Load( stream ) )
    {
        THROW_IO_ERROR(
                wxString::Format( _( "Unable to read file '%s'." ), m_filename.GetFullPath() ) );
    }

    return xmlDocument;
}


void SCH_IO_EAGLE::loadDrawing( const std::unique_ptr<EDRAWING>& aDrawing )
{
    wxCHECK( aDrawing, /* void */ );

    loadLayerDefs( aDrawing->layers );

    if( aDrawing->library )
    {
        EAGLE_LIBRARY& elib = m_eagleLibs[m_libName];
        elib.name = m_libName;

        loadLibrary( &aDrawing->library.value(), &elib );
    }

    if( aDrawing->schematic )
        loadSchematic( *aDrawing->schematic );
}


void SCH_IO_EAGLE::countNets( const ESCHEMATIC& aSchematic )
{
    for( const std::unique_ptr<ESHEET>& esheet : aSchematic.sheets )
    {
        for( const std::unique_ptr<ENET>& enet : esheet->nets )
        {
            wxString netName = enet->netname;

            if( m_netCounts.count( netName ) )
                m_netCounts[netName] = m_netCounts[netName] + 1;
            else
                m_netCounts[netName] = 1;
        }
    }

    for( const auto& [modname, emodule] : aSchematic.modules )
    {
        for( const std::unique_ptr<ESHEET>& esheet : emodule->sheets )
        {
            for( const std::unique_ptr<ENET>& enet : esheet->nets )
            {
                wxString netName = enet->netname;

                if( m_netCounts.count( netName ) )
                    m_netCounts[netName] = m_netCounts[netName] + 1;
                else
                    m_netCounts[netName] = 1;
            }
        }
    }
}


void SCH_IO_EAGLE::loadSchematic( const ESCHEMATIC& aSchematic )
{
    // Map all children into a readable dictionary
    if( aSchematic.sheets.empty() )
        return;

    // N.B. Eagle parts are case-insensitive in matching but we keep the display case
    for( const auto& [name, epart] : aSchematic.parts )
        m_partlist[name.Upper()] = epart.get();

    for( const auto& [modname, emodule] : aSchematic.modules )
    {
        for( const auto& [name, epart] : emodule->parts )
            m_partlist[name.Upper()] = epart.get();
    }

    if( !aSchematic.libraries.empty() )
    {
        for( const auto& [name, elibrary] : aSchematic.libraries )
        {
            wxString libName = elibrary->name;

            if( elibrary->urn )
            {
                wxString tmp = *elibrary->urn;

                libName += tmp.AfterLast( '/' );
            }

            EAGLE_LIBRARY* elib = &m_eagleLibs[libName];
            elib->name          = libName;

            loadLibrary( elibrary.get(), &m_eagleLibs[libName] );
        }

        m_pi->SaveLibrary( getLibFileName().GetFullPath() );
    }

    // find all nets and count how many sheets they appear on.
    // local labels will be used for nets found only on that sheet.
    countNets( aSchematic );

    // There is always at least a root sheet.
    m_sheetPath.push_back( m_rootSheet );
    m_sheetPath.SetPageNumber( wxT( "1" ) );

    size_t sheetCount = aSchematic.sheets.size();

    if( sheetCount > 1 )
    {
        int x, y;
        x = 1;
        y = 1;

        for( const std::unique_ptr<ESHEET>& esheet : aSchematic.sheets )
        {
            VECTOR2I pos    = VECTOR2I( x * schIUScale.MilsToIU( 1000 ),
                                        y * schIUScale.MilsToIU( 1000 ) );

            // Eagle schematics are never more than one sheet deep so the parent sheet is
            // always the root sheet.
            std::unique_ptr<SCH_SHEET> sheet = std::make_unique<SCH_SHEET>( m_rootSheet, pos );
            SCH_SCREEN* screen = new SCH_SCREEN( m_schematic );
            sheet->SetScreen( screen );
            screen->SetFileName( sheet->GetFileName() );

            wxCHECK2( sheet && screen, continue );

            wxString pageNo = wxString::Format( wxT( "%d" ), m_sheetIndex );

            m_sheetPath.push_back( sheet.get() );
            loadSheet( esheet );

            m_sheetPath.SetPageNumber( pageNo );
            m_sheetPath.pop_back();

            SCH_SCREEN* currentScreen = m_rootSheet->GetScreen();

            wxCHECK2( currentScreen, continue );

            currentScreen->Append( sheet.release() );

            x += 2;

            if( x > 10 ) // Start next row of sheets.
            {
                x = 1;
                y += 2;
            }

            m_sheetIndex++;
        }
    }
    else
    {
        for( const std::unique_ptr<ESHEET>& esheet : aSchematic.sheets )
            loadSheet( esheet );
    }

    // Handle the missing symbol units that need to be instantiated
    // to create the missing implicit connections

    // Calculate the already placed items bounding box and the page size to determine
    // placement for the new symbols
    VECTOR2I pageSizeIU = m_rootSheet->GetScreen()->GetPageSettings().GetSizeIU( schIUScale.IU_PER_MILS );
    BOX2I    sheetBbox  = getSheetBbox( m_rootSheet );
    VECTOR2I newCmpPosition( sheetBbox.GetLeft(), sheetBbox.GetBottom() );
    int      maxY = sheetBbox.GetY();

    SCH_SHEET_PATH sheetpath;
    m_rootSheet->LocatePathOfScreen( m_rootSheet->GetScreen(), &sheetpath );

    for( auto& cmp : m_missingCmps )
    {
        const SCH_SYMBOL* origSymbol = cmp.second.cmp;

        for( auto& unitEntry : cmp.second.units )
        {
            if( unitEntry.second == false )
                continue; // unit has been already processed

            // Instantiate the missing symbol unit
            int            unit      = unitEntry.first;
            const wxString reference = origSymbol->GetField( REFERENCE_FIELD )->GetText();
            std::unique_ptr<SCH_SYMBOL> symbol( (SCH_SYMBOL*) origSymbol->Duplicate() );

            symbol->SetUnitSelection( &sheetpath, unit );
            symbol->SetUnit( unit );
            symbol->SetOrientation( 0 );
            symbol->AddHierarchicalReference( sheetpath.Path(), reference, unit );

            // Calculate the placement position
            BOX2I cmpBbox = symbol->GetBoundingBox();
            int   posY    = newCmpPosition.y + cmpBbox.GetHeight();
            symbol->SetPosition( VECTOR2I( newCmpPosition.x, posY ) );
            newCmpPosition.x += cmpBbox.GetWidth();
            maxY = std::max( maxY, posY );

            if( newCmpPosition.x >= pageSizeIU.x )            // reached the page boundary?
                newCmpPosition = VECTOR2I( sheetBbox.GetLeft(), maxY ); // then start a new row

            // Add the global net labels to recreate the implicit connections
            addImplicitConnections( symbol.get(), m_rootSheet->GetScreen(), false );
            m_rootSheet->GetScreen()->Append( symbol.release() );
        }
    }

    m_missingCmps.clear();
}


void SCH_IO_EAGLE::loadSheet( const std::unique_ptr<ESHEET>& aSheet )
{
    SCH_SHEET* sheet = getCurrentSheet();
    SCH_SCREEN* screen = getCurrentScreen();

    wxCHECK( sheet && screen, /* void */ );

    if( !m_module )
    {
        std::string filename;
        wxFileName  fn = m_filename;

        fn.SetExt( FILEEXT::KiCadSchematicFileExtension );

        filename = wxString::Format( wxT( "%s_%d" ), m_filename.GetName(), m_sheetIndex );
        sheet->SetName( filename );

        ReplaceIllegalFileNameChars( &filename );
        replace( filename.begin(), filename.end(), ' ', '_' );

        fn.SetName( filename );

        sheet->SetFileName( fn.GetFullName() );
        screen->SetFileName( fn.GetFullPath() );
    }

    sheet->AutoplaceFields( screen, true );

    if( aSheet->plain )
    {
        for( const std::unique_ptr<EPOLYGON>& epoly : aSheet->plain->polygons )
            screen->Append( loadPolyLine( epoly ) );

        for( const std::unique_ptr<EWIRE>& ewire : aSheet->plain->wires )
        {
            SEG endpoints;
            screen->Append( loadWire( ewire, endpoints ) );
        }

        for( const std::unique_ptr<ETEXT>& etext : aSheet->plain->texts )
            screen->Append( loadPlainText( etext ) );

        for( const std::unique_ptr<ECIRCLE>& ecircle : aSheet->plain->circles )
            screen->Append( loadCircle( ecircle ) );

        for( const std::unique_ptr<ERECT>& erectangle : aSheet->plain->rectangles )
            screen->Append( loadRectangle( erectangle ) );

        for( const std::unique_ptr<EFRAME>& eframe : aSheet->plain->frames )
        {
            std::vector<SCH_ITEM*> frameItems;

            loadFrame( eframe, frameItems );

            for( SCH_ITEM* item : frameItems )
                screen->Append( item );
        }

        // Holes and splines currently not handled.  Not sure hole has any meaning in scheamtics.
    }

    for( const auto& [name, moduleinst] : aSheet->moduleinsts )
    {
        SCH_SHEET* modSheet = loadModuleInstance( moduleinst );
        screen->Append( modSheet );
    }

    for( const std::unique_ptr<EINSTANCE>& einstance : aSheet->instances )
    {
        if( m_module )
            loadInstance( einstance, m_module->parts );
        else
            loadInstance( einstance, m_eagleDoc->drawing->schematic->parts );
    }

    // Loop through all buses
    // From the DTD: "Buses receive names which determine which signals they include.
    // A bus is a drawing object. It does not create any electrical connections.
    // These are always created by means of the nets and their names."
    for( const std::unique_ptr<EBUS>& ebus : aSheet->busses )
    {
        // Get the bus name
        wxString busName = translateEagleBusName( ebus->name );

        // Load segments of this bus
        loadSegments( ebus->segments, busName, wxString() );
    }

    for( const std::unique_ptr<ENET>& enet : aSheet->nets )
    {
        // Get the net name and class
        wxString netName  = enet->netname;
        wxString netClass = wxString::Format( wxS( "%i" ), enet->netcode );

        // Load segments of this net
        loadSegments( enet->segments, netName, netClass );
    }

    adjustNetLabels(); // needs to be called before addBusEntries()
    addBusEntries();

    // Calculate the new sheet size.
    BOX2I    sheetBoundingBox = getSheetBbox( sheet );
    VECTOR2I targetSheetSize = sheetBoundingBox.GetSize();
    targetSheetSize += VECTOR2I( schIUScale.MilsToIU( 1500 ), schIUScale.MilsToIU( 1500 ) );

    // Get current Eeschema sheet size.
    VECTOR2I  pageSizeIU = screen->GetPageSettings().GetSizeIU( schIUScale.IU_PER_MILS );
    PAGE_INFO pageInfo   = screen->GetPageSettings();

    // Increase if necessary
    if( pageSizeIU.x < targetSheetSize.x )
        pageInfo.SetWidthMils( schIUScale.IUToMils( targetSheetSize.x ) );

    if( pageSizeIU.y < targetSheetSize.y )
        pageInfo.SetHeightMils( schIUScale.IUToMils( targetSheetSize.y ) );

    // Set the new sheet size.
    screen->SetPageSettings( pageInfo );

    pageSizeIU = screen->GetPageSettings().GetSizeIU( schIUScale.IU_PER_MILS );
    VECTOR2I sheetcentre( pageSizeIU.x / 2, pageSizeIU.y / 2 );
    VECTOR2I itemsCentre = sheetBoundingBox.Centre();

    // round the translation to nearest 100mil to place it on the grid.
    VECTOR2I translation = sheetcentre - itemsCentre;
    translation.x       = translation.x - translation.x % schIUScale.MilsToIU( 100 );
    translation.y       = translation.y - translation.y % schIUScale.MilsToIU( 100 );

    // Add global net labels for the named power input pins in this sheet
    for( SCH_ITEM* item : screen->Items().OfType( SCH_SYMBOL_T ) )
    {
        SCH_SYMBOL* symbol = static_cast<SCH_SYMBOL*>( item );
        addImplicitConnections( symbol, screen, true );
    }

    m_connPoints.clear();

    // Translate the items.
    std::vector<SCH_ITEM*> allItems;

    std::copy( screen->Items().begin(), screen->Items().end(), std::back_inserter( allItems ) );

    for( SCH_ITEM* item : allItems )
    {
        item->SetPosition( item->GetPosition() + translation );

        // We don't read positions of Eagle label fields (primarily intersheet refs), so we
        // need to autoplace them after applying the translation.
        if( SCH_LABEL_BASE* label = dynamic_cast<SCH_LABEL_BASE*>( item ) )
            label->AutoplaceFields( screen, false );

        item->ClearFlags();
        screen->Update( item );
    }
}


SCH_SHEET* SCH_IO_EAGLE::loadModuleInstance( const std::unique_ptr<EMODULEINST>& aModuleInstance )
{
    SCH_SHEET* currentSheet = getCurrentSheet();
    SCH_SCREEN* currentScreen = getCurrentScreen();

    wxCHECK( currentSheet &&currentScreen, nullptr );

    m_sheetIndex++;

    // Eagle document has already be checked for drawing and schematic nodes so this
    // should not segfault.
    auto it = m_eagleDoc->drawing->schematic->modules.find( aModuleInstance->moduleinst );

    // Find the module referenced by the module instance.
    if( it == m_eagleDoc->drawing->schematic->modules.end() )
    {
        THROW_IO_ERROR( wxString::Format( _( "No module instance '%s' found in schematic "
                                             "file:\n%s" ),
                                          aModuleInstance->name, m_filename.GetFullPath() ) );
    }


    wxFileName fn = m_filename;
    fn.SetName( aModuleInstance->name );
    fn.SetExt( FILEEXT::KiCadSchematicFileExtension );

    VECTOR2I portExtWireEndpoint;
    VECTOR2I size( it->second->dx.ToSchUnits(), it->second->dy.ToSchUnits() );

    int halfX = KiROUND( size.x / 2.0 );
    int halfY = KiROUND( size.y / 2.0 );
    int portExtWireLength = schIUScale.mmToIU( 5.08 );
    VECTOR2I pos( aModuleInstance->x.ToSchUnits() - halfX,
                  -aModuleInstance->y.ToSchUnits() - halfY );

    std::unique_ptr<SCH_SHEET> newSheet = std::make_unique<SCH_SHEET>( currentSheet, pos, size );

    SCH_SCREEN* newScreen = new SCH_SCREEN( m_schematic );

    wxCHECK( newSheet && newScreen, nullptr );

    newSheet->SetScreen( newScreen );
    newSheet->SetFileName( fn.GetFullName() );
    newSheet->SetName( aModuleInstance->name );
    newScreen->SetFileName( fn.GetFullPath() );

    for( const auto& [portName, port] : it->second->ports )
    {
        VECTOR2I pinPos( 0, 0 );
        int pinOffset = port->coord.ToSchUnits();
        SHEET_SIDE side = SHEET_SIDE::LEFT;

        if( port->side == "left" )
        {
            side = SHEET_SIDE::LEFT;
            pinPos.x = pos.x;
            pinPos.y = pos.y + halfY - pinOffset;
            portExtWireEndpoint = pinPos;
            portExtWireEndpoint.x -= portExtWireLength;
        }
        else if( port->side == "right" )
        {
            side = SHEET_SIDE::RIGHT;
            pinPos.x = pos.x + size.x;
            pinPos.y = pos.y + halfY - pinOffset;
            portExtWireEndpoint = pinPos;
            portExtWireEndpoint.x += portExtWireLength;
        }
        else if( port->side == "top" )
        {
            side = SHEET_SIDE::TOP;
            pinPos.x = pos.x + halfX + pinOffset;
            pinPos.y = pos.y;
            portExtWireEndpoint = pinPos;
            portExtWireEndpoint.y -= portExtWireLength;
        }
        else if( port->side == "bottom" )
        {
            side = SHEET_SIDE::BOTTOM;
            pinPos.x = pos.x + halfX + pinOffset;
            pinPos.y = pos.y + size.y;
            portExtWireEndpoint = pinPos;
            portExtWireEndpoint.y += portExtWireLength;
        }

        SCH_LINE* portExtWire =  new SCH_LINE( pinPos, LAYER_WIRE );
        portExtWire->SetEndPoint( portExtWireEndpoint );
        currentScreen->Append( portExtWire );

        LABEL_FLAG_SHAPE pinType = LABEL_FLAG_SHAPE::L_UNSPECIFIED;

        if( port->direction )
        {
            if( *port->direction == "in" )
                pinType = LABEL_FLAG_SHAPE::L_INPUT;
            else if( *port->direction == "out" )
                pinType = LABEL_FLAG_SHAPE::L_OUTPUT;
            else if( *port->direction == "io" )
                pinType = LABEL_FLAG_SHAPE::L_BIDI;
            else if( *port->direction == "hiz" )
                pinType = LABEL_FLAG_SHAPE::L_TRISTATE;
            else
                pinType = LABEL_FLAG_SHAPE::L_UNSPECIFIED;

            // KiCad does not support passive, power, open collector, or no-connect sheet
            // pins that Eagle ports support.  They are set to unspecified to minimize
            // ERC issues.
        }

        SCH_SHEET_PIN* sheetPin = new SCH_SHEET_PIN( newSheet.get(), VECTOR2I( 0, 0 ), portName );

        sheetPin->SetShape( pinType );
        sheetPin->SetPosition( pinPos );
        sheetPin->SetSide( side );
        newSheet->AddPin( sheetPin );
    }

    wxString pageNo = wxString::Format( wxT( "%d" ), m_sheetIndex );

    m_sheetPath.push_back( newSheet.get() );

    m_module = it->second.get();

    // It's not clear if Eagle modules can have more than one sheet so this may not be correct.
    if( it->second->sheets.size() == 1 )
    {
        loadSheet( it->second->sheets.at( 0 ) );
    }
    else
    {
        int i = 0;
        int x = 1;
        int y = 1;

        for( const std::unique_ptr<ESHEET>& esheet : it->second->sheets )
        {
            pos = VECTOR2I( x * schIUScale.MilsToIU( 1000 ),
                            y * schIUScale.MilsToIU( 1000 ) );

            std::unique_ptr<SCH_SHEET> sheet = std::make_unique<SCH_SHEET>( newSheet.get(), pos );
            SCH_SCREEN* screen = new SCH_SCREEN( m_schematic );
            sheet->SetScreen( screen );

            wxString newFileName = fn.GetName();

            newFileName += wxString::Format( wxS( "_%d" ), i + 1 );
            fn.SetName( newFileName );
            screen->SetFileName( fn.GetFullPath() );
            sheet->SetFileName( fn.GetFullName() );

            wxCHECK2( sheet && screen, continue );

            wxString subSheetPageNo = wxString::Format( wxT( "%d" ), m_sheetIndex );

            m_sheetPath.push_back( sheet.get() );
            loadSheet( esheet );

            m_sheetPath.SetPageNumber( subSheetPageNo );
            m_sheetPath.pop_back();

            newScreen->Append( sheet.release() );

            x += 2;

            if( x > 10 ) // Start next row of sheets.
            {
                x = 1;
                y += 2;
            }

            m_sheetIndex++;
        }
    }

    m_sheetPath.SetPageNumber( pageNo );
    m_sheetPath.pop_back();

    m_module = nullptr;

    return newSheet.release();
}


void SCH_IO_EAGLE::loadFrame( const std::unique_ptr<EFRAME>& aFrame,
                              std::vector<SCH_ITEM*>& aItems )
{
    int xMin = aFrame->x1.ToSchUnits();
    int xMax = aFrame->x2.ToSchUnits();
    int yMin = -aFrame->y1.ToSchUnits();
    int yMax = -aFrame->y2.ToSchUnits();

    if( xMin > xMax )
        std::swap( xMin, xMax );

    if( yMin > yMax )
        std::swap( yMin, yMax );

    SCH_SHAPE* lines = new SCH_SHAPE( SHAPE_T::POLY );
    lines->AddPoint( VECTOR2I( xMin, yMin ) );
    lines->AddPoint( VECTOR2I( xMax, yMin ) );
    lines->AddPoint( VECTOR2I( xMax, yMax ) );
    lines->AddPoint( VECTOR2I( xMin, yMax ) );
    lines->AddPoint( VECTOR2I( xMin, yMin ) );
    aItems.push_back( lines );

    if( !( aFrame->border_left == false ) )
    {
        lines = new SCH_SHAPE( SHAPE_T::POLY );
        lines->AddPoint( VECTOR2I( xMin + schIUScale.MilsToIU( 150 ),
                                   yMin + schIUScale.MilsToIU( 150 ) ) );
        lines->AddPoint( VECTOR2I( xMin + schIUScale.MilsToIU( 150 ),
                                   yMax - schIUScale.MilsToIU( 150 ) ) );
        aItems.push_back( lines );

        int i;
        int height = yMax - yMin;
        int x1 = xMin;
        int x2 = x1 + schIUScale.MilsToIU( 150 );
        int legendPosX = xMin + schIUScale.MilsToIU( 75 );
        double rowSpacing = height / double( aFrame->rows );
        double legendPosY = yMin + ( rowSpacing / 2 );

        for( i = 1; i < aFrame->rows; i++ )
        {
            int newY = KiROUND( yMin + ( rowSpacing * (double) i ) );
            lines = new SCH_SHAPE( SHAPE_T::POLY );
            lines->AddPoint( VECTOR2I( x1, newY ) );
            lines->AddPoint( VECTOR2I( x2, newY ) );
            aItems.push_back( lines );
        }

        char legendChar = 'A';

        for( i = 0; i < aFrame->rows; i++ )
        {
            SCH_TEXT* legendText = new SCH_TEXT();
            legendText->SetPosition( VECTOR2I( legendPosX, KiROUND( legendPosY ) ) );
            legendText->SetHorizJustify( GR_TEXT_H_ALIGN_CENTER );
            legendText->SetVertJustify( GR_TEXT_V_ALIGN_CENTER );
            legendText->SetText( wxString( legendChar ) );
            legendText->SetTextSize( VECTOR2I( schIUScale.MilsToIU( 90 ),
                                               schIUScale.MilsToIU( 100 ) ) );
            aItems.push_back( legendText );
            legendChar++;
            legendPosY += rowSpacing;
        }
    }

    if( !( aFrame->border_right == false ) )
    {
        lines = new SCH_SHAPE( SHAPE_T::POLY );
        lines->AddPoint( VECTOR2I( xMax - schIUScale.MilsToIU( 150 ),
                                   yMin + schIUScale.MilsToIU( 150 ) ) );
        lines->AddPoint( VECTOR2I( xMax - schIUScale.MilsToIU( 150 ),
                                   yMax - schIUScale.MilsToIU( 150 ) ) );
        aItems.push_back( lines );

        int i;
        int height = yMax - yMin;
        int x1 = xMax - schIUScale.MilsToIU( 150 );
        int x2 = xMax;
        int legendPosX = xMax - schIUScale.MilsToIU( 75 );
        double rowSpacing = height / double( aFrame->rows );
        double legendPosY = yMin + ( rowSpacing / 2 );

        for( i = 1; i < aFrame->rows; i++ )
        {
            int newY = KiROUND( yMin + ( rowSpacing * (double) i ) );
            lines = new SCH_SHAPE( SHAPE_T::POLY );
            lines->AddPoint( VECTOR2I( x1, newY ) );
            lines->AddPoint( VECTOR2I( x2, newY ) );
            aItems.push_back( lines );
        }

        char legendChar = 'A';

        for( i = 0; i < aFrame->rows; i++ )
        {
            SCH_TEXT* legendText = new SCH_TEXT();
            legendText->SetPosition( VECTOR2I( legendPosX, KiROUND( legendPosY ) ) );
            legendText->SetHorizJustify( GR_TEXT_H_ALIGN_CENTER );
            legendText->SetVertJustify( GR_TEXT_V_ALIGN_CENTER );
            legendText->SetText( wxString( legendChar ) );
            legendText->SetTextSize( VECTOR2I( schIUScale.MilsToIU( 90 ),
                                               schIUScale.MilsToIU( 100 ) ) );
            aItems.push_back( legendText );
            legendChar++;
            legendPosY += rowSpacing;
        }
    }

    if( !( aFrame->border_top == false ) )
    {
        lines = new SCH_SHAPE( SHAPE_T::POLY );
        lines->AddPoint( VECTOR2I( xMax - schIUScale.MilsToIU( 150 ),
                                   yMin + schIUScale.MilsToIU( 150 ) ) );
        lines->AddPoint( VECTOR2I( xMin + schIUScale.MilsToIU( 150 ),
                                   yMin + schIUScale.MilsToIU( 150 ) ) );
        aItems.push_back( lines );

        int i;
        int width = xMax - xMin;
        int y1 = yMin;
        int y2 = yMin + schIUScale.MilsToIU( 150 );
        int legendPosY = yMin + schIUScale.MilsToIU( 75 );
        double columnSpacing = width / double( aFrame->columns );
        double legendPosX = xMin + ( columnSpacing / 2 );

        for( i = 1; i < aFrame->columns; i++ )
        {
            int newX = KiROUND( xMin + ( columnSpacing * (double) i ) );
            lines = new SCH_SHAPE( SHAPE_T::POLY );
            lines->AddPoint( VECTOR2I( newX, y1 ) );
            lines->AddPoint( VECTOR2I( newX, y2 ) );
            aItems.push_back( lines );
        }

        char legendChar = '1';

        for( i = 0; i < aFrame->columns; i++ )
        {
            SCH_TEXT* legendText = new SCH_TEXT();
            legendText->SetPosition( VECTOR2I( KiROUND( legendPosX ), legendPosY ) );
            legendText->SetHorizJustify( GR_TEXT_H_ALIGN_CENTER );
            legendText->SetVertJustify( GR_TEXT_V_ALIGN_CENTER );
            legendText->SetText( wxString( legendChar ) );
            legendText->SetTextSize( VECTOR2I( schIUScale.MilsToIU( 90 ),
                                               schIUScale.MilsToIU( 100 ) ) );
            aItems.push_back( legendText );
            legendChar++;
            legendPosX += columnSpacing;
        }
    }

    if( !( aFrame->border_bottom == false ) )
    {
        lines = new SCH_SHAPE( SHAPE_T::POLY );
        lines->AddPoint( VECTOR2I( xMax - schIUScale.MilsToIU( 150 ),
                                   yMax - schIUScale.MilsToIU( 150 ) ) );
        lines->AddPoint( VECTOR2I( xMin + schIUScale.MilsToIU( 150 ),
                                   yMax - schIUScale.MilsToIU( 150 ) ) );
        aItems.push_back( lines );

        int i;
        int width = xMax - xMin;
        int y1 = yMax - schIUScale.MilsToIU( 150 );
        int y2 = yMax;
        int legendPosY = yMax - schIUScale.MilsToIU( 75 );
        double columnSpacing = width / double( aFrame->columns );
        double legendPosX = xMin + ( columnSpacing / 2 );

        for( i = 1; i < aFrame->columns; i++ )
        {
            int newX = KiROUND( xMin + ( columnSpacing * (double) i ) );
            lines = new SCH_SHAPE( SHAPE_T::POLY );
            lines->AddPoint( VECTOR2I( newX, y1 ) );
            lines->AddPoint( VECTOR2I( newX, y2 ) );
            aItems.push_back( lines );
        }

        char legendChar = '1';

        for( i = 0; i < aFrame->columns; i++ )
        {
            SCH_TEXT* legendText = new SCH_TEXT();
            legendText->SetPosition( VECTOR2I( KiROUND( legendPosX ), legendPosY ) );
            legendText->SetHorizJustify( GR_TEXT_H_ALIGN_CENTER );
            legendText->SetVertJustify( GR_TEXT_V_ALIGN_CENTER );
            legendText->SetText( wxString( legendChar ) );
            legendText->SetTextSize( VECTOR2I( schIUScale.MilsToIU( 90 ),
                                               schIUScale.MilsToIU( 100 ) ) );
            aItems.push_back( legendText );
            legendChar++;
            legendPosX += columnSpacing;
        }
    }
}


void SCH_IO_EAGLE::loadSegments( const std::vector<std::unique_ptr<ESEGMENT>>& aSegments,
                                 const wxString& netName,
                                 const wxString& aNetClass )
{
    // Loop through all segments
    SCH_SCREEN* screen         = getCurrentScreen();

    wxCHECK( screen, /* void */ );

    size_t segmentCount = aSegments.size();

    for( const std::unique_ptr<ESEGMENT>& esegment : aSegments )
    {
        bool      labelled = false; // has a label been added to this continuously connected segment
        bool      firstWireFound  = false;
        SEG       firstWire;

        m_segments.emplace_back();
        SEG_DESC& segDesc = m_segments.back();

        for( const std::unique_ptr<EWIRE>& ewire : esegment->wires )
        {
            // TODO: Check how intersections used in adjustNetLabels should be
            // calculated - for now we pretend that all wires are line segments.
            SEG thisWire;
            SCH_ITEM* wire = loadWire( ewire, thisWire );
            m_connPoints[thisWire.A].emplace( wire );
            m_connPoints[thisWire.B].emplace( wire );

            if( !firstWireFound )
            {
                firstWire = thisWire;
                firstWireFound = true;
            }

            // Test for intersections with other wires
            for( SEG_DESC& desc : m_segments )
            {
                if( !desc.labels.empty() && desc.labels.front()->GetText() == netName )
                    continue; // no point in saving intersections of the same net

                for( const SEG& seg : desc.segs )
                {
                    OPT_VECTOR2I intersection = thisWire.Intersect( seg, true );

                    if( intersection )
                        m_wireIntersections.push_back( *intersection );
                }
            }

            segDesc.segs.push_back( thisWire );
            screen->Append( wire );
        }

        for( const std::unique_ptr<EJUNCTION>& ejunction : esegment->junctions )
            screen->Append( loadJunction( ejunction ) );

        for( const std::unique_ptr<ELABEL>& elabel : esegment->labels )
        {
            SCH_TEXT* label = loadLabel( elabel, netName );
            screen->Append( label );

            wxASSERT( segDesc.labels.empty()
                    || segDesc.labels.front()->GetText() == label->GetText() );

            segDesc.labels.push_back( label );
            labelled = true;
        }

        for( const std::unique_ptr<EPINREF>& epinref : esegment->pinRefs )
        {
            wxString part = epinref->part;
            wxString pin = epinref->pin;

            auto powerPort = m_powerPorts.find( wxT( "#" ) + part );

            if( powerPort != m_powerPorts.end()
              && powerPort->second == EscapeString( pin, CTX_NETNAME ) )
            {
                labelled = true;
            }
        }

        // Add a small label to the net segment if it hasn't been labeled already or is not
        // connect to a power symbol with a pin on the same net.  This preserves the named net
        // feature of Eagle schematics.
        if( !labelled && firstWireFound )
        {
            std::unique_ptr<SCH_LABEL_BASE> label;

            // Add a global label if the net appears on more than one Eagle sheet
            if( m_netCounts[netName.ToStdString()] > 1 )
                label.reset( new SCH_GLOBALLABEL );
            else if( segmentCount > 1 )
                label.reset( new SCH_LABEL );

            if( label )
            {
                label->SetPosition( firstWire.A );
                label->SetText( escapeName( netName ) );
                label->SetTextSize( VECTOR2I( schIUScale.MilsToIU( 40 ),
                                              schIUScale.MilsToIU( 40 ) ) );

                if( firstWire.B.x > firstWire.A.x )
                    label->SetSpinStyle( SPIN_STYLE::LEFT );
                else
                    label->SetSpinStyle( SPIN_STYLE::RIGHT );

                screen->Append( label.release() );
            }
        }
    }
}


SCH_SHAPE* SCH_IO_EAGLE::loadPolyLine( const std::unique_ptr<EPOLYGON>& aPolygon )
{
    std::unique_ptr<SCH_SHAPE> poly = std::make_unique<SCH_SHAPE>( SHAPE_T::POLY );
    VECTOR2I   pt, prev_pt;
    opt_double prev_curve;

    for( const std::unique_ptr<EVERTEX>& evertex : aPolygon->vertices )
    {
        pt = VECTOR2I( evertex->x.ToSchUnits(), -evertex->y.ToSchUnits() );

        if( prev_curve )
        {
            SHAPE_ARC arc;
            arc.ConstructFromStartEndAngle( prev_pt, pt, -EDA_ANGLE( *prev_curve, DEGREES_T ) );
            poly->GetPolyShape().Append( arc, -1, -1, ARC_ACCURACY );
        }
        else
        {
            poly->AddPoint( pt );
        }

        prev_pt = pt;
        prev_curve = evertex->curve;
    }

    poly->SetLayer( kiCadLayer( aPolygon->layer ) );
    poly->SetStroke( STROKE_PARAMS( aPolygon->width.ToSchUnits(), LINE_STYLE::SOLID ) );
    poly->SetFillMode( FILL_T::FILLED_SHAPE );

    return poly.release();
}


SCH_ITEM* SCH_IO_EAGLE::loadWire( const std::unique_ptr<EWIRE>& aWire, SEG& endpoints )
{
    VECTOR2I start, end;

    start.x = aWire->x1.ToSchUnits();
    start.y = -aWire->y1.ToSchUnits();
    end.x   = aWire->x2.ToSchUnits();
    end.y   = -aWire->y2.ToSchUnits();

    // For segment wires.
    endpoints = SEG( start, end );

    if( aWire->curve )
    {
        std::unique_ptr<SCH_SHAPE> arc = std::make_unique<SCH_SHAPE>( SHAPE_T::ARC );

        VECTOR2I center = ConvertArcCenter( start, end, *aWire->curve );
        arc->SetCenter( center );
        arc->SetStart( start );

        // KiCad rotates the other way.
        arc->SetArcAngleAndEnd( -EDA_ANGLE( *aWire->curve, DEGREES_T ), true );
        arc->SetLayer( kiCadLayer( aWire->layer ) );
        arc->SetStroke( STROKE_PARAMS( aWire->width.ToSchUnits(), LINE_STYLE::SOLID ) );

        return arc.release();
    }
    else
    {
        std::unique_ptr<SCH_LINE> line = std::make_unique<SCH_LINE>();

        line->SetStartPoint( start );
        line->SetEndPoint( end );
        line->SetLayer( kiCadLayer( aWire->layer ) );
        line->SetStroke( STROKE_PARAMS( aWire->width.ToSchUnits(), LINE_STYLE::SOLID ) );

        return line.release();
    }
}


SCH_SHAPE* SCH_IO_EAGLE::loadCircle( const std::unique_ptr<ECIRCLE>& aCircle )
{
    std::unique_ptr<SCH_SHAPE> circle = std::make_unique<SCH_SHAPE>( SHAPE_T::CIRCLE );
    VECTOR2I center( aCircle->x.ToSchUnits(), -aCircle->y.ToSchUnits() );

    circle->SetLayer( kiCadLayer( aCircle->layer ) );
    circle->SetPosition( center );
    circle->SetEnd( VECTOR2I( center.x + aCircle->radius.ToSchUnits(), center.y ) );
    circle->SetStroke( STROKE_PARAMS( aCircle->width.ToSchUnits(), LINE_STYLE::SOLID ) );

    return circle.release();
}


SCH_SHAPE* SCH_IO_EAGLE::loadRectangle( const std::unique_ptr<ERECT>& aRectangle )
{
    std::unique_ptr<SCH_SHAPE> rectangle = std::make_unique<SCH_SHAPE>( SHAPE_T::RECTANGLE );

    rectangle->SetLayer( kiCadLayer( aRectangle->layer ) );
    rectangle->SetPosition( VECTOR2I( aRectangle->x1.ToSchUnits(), -aRectangle->y1.ToSchUnits() ) );
    rectangle->SetEnd( VECTOR2I( aRectangle->x2.ToSchUnits(), -aRectangle->y2.ToSchUnits() ) );

    if( aRectangle->rot )
    {
        VECTOR2I pos( rectangle->GetPosition() );
        VECTOR2I end( rectangle->GetEnd() );
        VECTOR2I center( rectangle->GetCenter() );

        RotatePoint( pos, center, EDA_ANGLE( aRectangle->rot->degrees, DEGREES_T ) );
        RotatePoint( end,  center, EDA_ANGLE( aRectangle->rot->degrees, DEGREES_T ) );

        rectangle->SetPosition( pos );
        rectangle->SetEnd( end );
    }

    // Eagle rectangles are filled by definition.
    rectangle->SetFillMode( FILL_T::FILLED_SHAPE );

    return rectangle.release();
}


SCH_JUNCTION* SCH_IO_EAGLE::loadJunction( const std::unique_ptr<EJUNCTION>&  aJunction )
{
    std::unique_ptr<SCH_JUNCTION> junction = std::make_unique<SCH_JUNCTION>();

    VECTOR2I pos( aJunction->x.ToSchUnits(), -aJunction->y.ToSchUnits() );

    junction->SetPosition( pos );

    return junction.release();
}


SCH_TEXT* SCH_IO_EAGLE::loadLabel( const std::unique_ptr<ELABEL>& aLabel,
                                   const wxString& aNetName )
{
    VECTOR2I elabelpos( aLabel->x.ToSchUnits(), -aLabel->y.ToSchUnits() );

    // Determine if the label is local or global depending on
    // the number of sheets the net appears in
    bool                            global = m_netCounts[aNetName] > 1;
    std::unique_ptr<SCH_LABEL_BASE> label;

    VECTOR2I textSize = VECTOR2I( KiROUND( aLabel->size.ToSchUnits() * 0.7 ),
                                  KiROUND( aLabel->size.ToSchUnits() * 0.7 ) );

    if( m_module )
    {
        if(  m_module->ports.find( aNetName ) != m_module->ports.end() )
        {
            label = std::make_unique<SCH_HIERLABEL>();
            label->SetText( escapeName( aNetName ) );

            const auto it = m_module->ports.find( aNetName );

            LABEL_SHAPE type;

            if( it->second->direction )
            {
                wxString direction = *it->second->direction;

                if( direction == "in" )
                    type = LABEL_SHAPE::LABEL_INPUT;
                else if( direction == "out" )
                    type = LABEL_SHAPE::LABEL_OUTPUT;
                else if( direction == "io" )
                    type = LABEL_SHAPE::LABEL_BIDI;
                else if( direction == "hiz" )
                    type = LABEL_SHAPE::LABEL_TRISTATE;
                else
                    type = LABEL_SHAPE::LABEL_PASSIVE;

                // KiCad does not support passive, power, open collector, or no-connect sheet
                // pins that Eagle ports support.  They are set to unspecified to minimize
                // ERC issues.
                label->SetLabelShape( type );
            }
        }
        else
        {
            label = std::make_unique<SCH_LABEL>();
            label->SetText( escapeName( aNetName ) );
        }
    }
    else if( global )
    {
        label = std::make_unique<SCH_GLOBALLABEL>();
        label->SetText( escapeName( aNetName ) );
    }
    else
    {
        label = std::make_unique<SCH_LABEL>();
        label->SetText( escapeName( aNetName ) );
    }

    label->SetPosition( elabelpos );
    label->SetTextSize( textSize );
    label->SetSpinStyle( SPIN_STYLE::RIGHT );

    if( aLabel->rot )
    {
        for( int i = 0; i < KiROUND( aLabel->rot->degrees / 90 ) %4; ++i )
            label->Rotate90( false );

        if( aLabel->rot->mirror )
            label->MirrorSpinStyle( false );
    }

    return label.release();
}


std::pair<VECTOR2I, const SEG*>
SCH_IO_EAGLE::findNearestLinePoint( const VECTOR2I&         aPoint,
                                    const std::vector<SEG>& aLines ) const
{
    VECTOR2I   nearestPoint;
    const SEG* nearestLine = nullptr;

    float d, mindistance = std::numeric_limits<float>::max();

    // Find the nearest start, middle or end of a line from the list of lines.
    for( const SEG& line : aLines )
    {
        VECTOR2I testpoint = line.A;
        d = sqrt( abs( ( ( aPoint.x - testpoint.x ) ^ 2 ) + ( ( aPoint.y - testpoint.y ) ^ 2 ) ) );

        if( d < mindistance )
        {
            mindistance  = d;
            nearestPoint = testpoint;
            nearestLine  = &line;
        }

        testpoint = line.Center();
        d = sqrt( abs( ( ( aPoint.x - testpoint.x ) ^ 2 ) + ( ( aPoint.y - testpoint.y ) ^ 2 ) ) );

        if( d < mindistance )
        {
            mindistance  = d;
            nearestPoint = testpoint;
            nearestLine  = &line;
        }

        testpoint = line.B;
        d = sqrt( abs( ( ( aPoint.x - testpoint.x ) ^ 2 ) + ( ( aPoint.y - testpoint.y ) ^ 2 ) ) );

        if( d < mindistance )
        {
            mindistance  = d;
            nearestPoint = testpoint;
            nearestLine  = &line;
        }
    }

    return std::make_pair( nearestPoint, nearestLine );
}


void SCH_IO_EAGLE::loadInstance( const std::unique_ptr<EINSTANCE>& aInstance,
                                 const std::map<wxString, std::unique_ptr<EPART>>& aParts )
{
    wxCHECK( aInstance, /* void */ );

    SCH_SCREEN* screen = getCurrentScreen();

    wxCHECK( screen, /* void */ );

    const auto partIt = aParts.find( aInstance->part );

    if( partIt == aParts.end() )
    {
        Report( wxString::Format( _( "Error parsing Eagle file. Could not find '%s' "
                                     "instance but it is referenced in the schematic." ),
                                  aInstance->part ),
                RPT_SEVERITY_ERROR );

        return;
    }

    const std::unique_ptr<EPART>& epart = partIt->second;

    wxString libName = epart->library;

    // Correctly handle versioned libraries.
    if( epart->libraryUrn )
    {
        wxString tmp = *epart->libraryUrn;

        libName += tmp.AfterLast( '/' );
    }

    wxString gatename    = epart->deviceset + wxS( "_" ) + epart->device + wxS( "_" ) +
                           aInstance->gate;
    wxString symbolname  = wxString( epart->deviceset + epart->device );
    symbolname.Replace( wxT( "*" ), wxEmptyString );
    wxString kisymbolname = EscapeString( symbolname, CTX_LIBID );

    // Eagle schematics can have multiple libraries containing symbols with duplicate symbol
    // names.  Because this parser stores all of the symbols in a single library, the
    // loadSymbol() function, prefixed the original Eagle library name to the symbol name
    // in case of a name clash.  Check for the prefixed symbol first.  This ensures that
    // the correct library symbol gets mapped on load.
    wxString altSymbolName = libName + wxT( "_" ) + symbolname;
    altSymbolName = EscapeString( altSymbolName, CTX_LIBID );

    wxString libIdSymbolName = altSymbolName;

    const auto libIt = m_eagleLibs.find( libName );

    if( libIt == m_eagleLibs.end() )
    {
        Report( wxString::Format( wxS( "Eagle library '%s' not found while looking up symbol for"
                                       "deviceset '%s', device '%s', and gate '%s." ),
                                  libName, epart->deviceset, epart->device, aInstance->gate ) );
        return;
    }

    const auto gateIt = libIt->second.GateToUnitMap.find( gatename );

    if( gateIt == libIt->second.GateToUnitMap.end() )
    {
        Report( wxString::Format( wxS( "Symbol not found for deviceset '%s', device '%s', and "
                                       "gate '%s in library '%s'." ),
                                  epart->deviceset, epart->device, aInstance->gate, libName ) );
        return;
    }

    int unit = gateIt->second;

    wxString       package;
    EAGLE_LIBRARY* elib = &m_eagleLibs[libName];

    auto p = elib->package.find( kisymbolname );

    if( p != elib->package.end() )
        package = p->second;

    LIB_SYMBOL* part = m_pi->LoadSymbol( getLibFileName().GetFullPath(), altSymbolName,
                                         m_properties.get() );

    if( !part )
    {
        part = m_pi->LoadSymbol( getLibFileName().GetFullPath(), kisymbolname,
                                 m_properties.get() );
        libIdSymbolName = kisymbolname;
    }

    if( !part )
    {
        Report( wxString::Format( _( "Could not find '%s' in the imported library." ),
                                  UnescapeString( kisymbolname ) ),
                RPT_SEVERITY_ERROR );
        return;
    }

    LIB_ID                      libId( getLibName(), libIdSymbolName );
    std::unique_ptr<SCH_SYMBOL> symbol = std::make_unique<SCH_SYMBOL>();
    symbol->SetLibId( libId );
    symbol->SetUnit( unit );
    symbol->SetPosition( VECTOR2I( aInstance->x.ToSchUnits(), -aInstance->y.ToSchUnits() ) );

    // assume that footprint library is identical to project name
    if( !package.IsEmpty() )
    {
        wxString footprint = m_schematic->Prj().GetProjectName() + wxT( ":" ) + package;
        symbol->GetField( FOOTPRINT_FIELD )->SetText( footprint );
    }

    if( aInstance->rot )
    {
        symbol->SetOrientation( kiCadComponentRotation( aInstance->rot->degrees ) );

        if( aInstance->rot->mirror )
            symbol->MirrorHorizontally( aInstance->x.ToSchUnits() );
    }

    std::vector<SCH_FIELD*> partFields;
    part->GetFields( partFields );

    for( const SCH_FIELD* field : partFields )
    {
        symbol->GetFieldById( field->GetId() )->ImportValues( *field );
        symbol->GetFieldById( field->GetId() )->SetTextPos( (VECTOR2I)symbol->GetPosition()
                                                            + field->GetTextPos() );
    }

    // If there is no footprint assigned, then prepend the reference value
    // with a hash character to mute netlist updater complaints
    wxString reference = package.IsEmpty() ? '#' + aInstance->part : aInstance->part;

    // reference must end with a number but EAGLE does not enforce this
    if( reference.find_last_not_of( wxT( "0123456789" ) ) == ( reference.Length()-1 ) )
        reference.Append( wxT( "0" ) );

    // EAGLE allows references to be single digits.  This breaks KiCad netlisting, which requires
    // parts to have non-digit + digit annotation.  If the reference begins with a number,
    // we prepend 'UNK' (unknown) for the symbol designator
    if( reference.find_first_not_of( wxT( "0123456789" ) ) != 0 )
        reference.Prepend( wxT( "UNK" ) );

    // EAGLE allows designator to start with # but that is used in KiCad
    // for symbols which do not have a footprint
    if( aInstance->part.find_first_not_of( wxT( "#" ) ) != 0 )
        reference.Prepend( wxT( "UNK" ) );

    SCH_FIELD* referenceField = symbol->GetField( REFERENCE_FIELD );
    referenceField->SetText( reference );

    SCH_FIELD* valueField = symbol->GetField( VALUE_FIELD );
    bool       userValue = m_userValue.at( libIdSymbolName );

    if( part->GetUnitCount() > 1 )
    {
        getEagleSymbolFieldAttributes( aInstance, wxS( ">NAME" ), referenceField );
        getEagleSymbolFieldAttributes( aInstance, wxS( ">VALUE" ), valueField );
    }

    if( epart->value && !epart->value.CGet().IsEmpty() )
    {
        valueField->SetText( *epart->value );
    }
    else
    {
        valueField->SetText( kisymbolname );

        if( userValue )
            valueField->SetVisible( false );
    }

    for( const auto& [ attrName, attr ] : epart->attributes )
    {
        VECTOR2I   newFieldPosition( 0, 0 );
        SCH_FIELD* lastField = symbol->GetFieldById( symbol->GetFieldCount() - 1 );

        if( lastField )
            newFieldPosition = lastField->GetPosition();

        SCH_FIELD newField( newFieldPosition, symbol->GetFieldCount(), symbol.get() );

        newField.SetName( attrName );

        if( attr->value )
            newField.SetText( *attr->value );

        newField.SetVisible( ( attr->display == EATTR::Off ) ? false : true );

        symbol->AddField( newField );
    }

    for( const auto& [variantName, variant] : epart->variants )
    {
        SCH_FIELD* field = symbol->AddField( *symbol->GetField( VALUE_FIELD ) );
        field->SetName( wxT( "VARIANT_" ) + variant->name );

        if( variant->value )
            field->SetText( *variant->value );

        field->SetVisible( false );
    }

    bool valueAttributeFound = false;
    bool nameAttributeFound  = false;

    // Parse attributes for the instance
    for( auto& [name, eattr] : aInstance->attributes )
    {
        SCH_FIELD* field = nullptr;

        if( eattr->name.Lower() == wxT( "name" ) )
        {
            field              = symbol->GetField( REFERENCE_FIELD );
            nameAttributeFound = true;
        }
        else if( eattr->name.Lower() == wxT( "value" ) )
        {
            field               = symbol->GetField( VALUE_FIELD );
            valueAttributeFound = true;
        }
        else
        {
            field = symbol->FindField( eattr->name );

            if( field )
                field->SetVisible( false );
        }

        if( field )
        {
            field->SetPosition( VECTOR2I( eattr->x->ToSchUnits(), -eattr->y->ToSchUnits() ) );
            int  align      = eattr->align ? *eattr->align : ETEXT::BOTTOM_LEFT;
            int  absdegrees = eattr->rot ? eattr->rot->degrees : 0;
            bool mirror     = eattr->rot ? eattr->rot->mirror : false;

            if( aInstance->rot && aInstance->rot->mirror )
                mirror = !mirror;

            bool spin = eattr->rot ? eattr->rot->spin : false;

            if( eattr->display == EATTR::Off || eattr->display == EATTR::NAME )
                field->SetVisible( false );

            int rotation   = aInstance->rot ? aInstance->rot->degrees : 0;
            int reldegrees = ( absdegrees - rotation + 360.0 );
            reldegrees %= 360;

            eagleToKicadAlignment( field, align, reldegrees, mirror, spin, absdegrees );
        }
    }

    // Use the instance attribute to determine the reference and value field visibility.
    if( aInstance->smashed && aInstance->smashed.Get() )
    {
        symbol->GetField( VALUE_FIELD )->SetVisible( valueAttributeFound );
        symbol->GetField( REFERENCE_FIELD )->SetVisible( nameAttributeFound );
    }

    symbol->AddHierarchicalReference( m_sheetPath.Path(), reference, unit );

    // Save the pin positions
    SYMBOL_LIB_TABLE& schLibTable = *PROJECT_SCH::SchSymbolLibTable( &m_schematic->Prj() );
    LIB_SYMBOL* libSymbol = schLibTable.LoadSymbol( symbol->GetLibId() );

    wxCHECK( libSymbol, /*void*/ );

    symbol->SetLibSymbol( new LIB_SYMBOL( *libSymbol ) );

    for( const SCH_PIN* pin : symbol->GetLibPins() )
        m_connPoints[symbol->GetPinPhysicalPosition( pin )].emplace( pin );

    if( part->IsPower() )
        m_powerPorts[ reference ] = symbol->GetField( VALUE_FIELD )->GetText();

    symbol->ClearFlags();

    screen->Append( symbol.release() );
}


EAGLE_LIBRARY* SCH_IO_EAGLE::loadLibrary( const ELIBRARY* aLibrary, EAGLE_LIBRARY* aEagleLibrary )
{
    wxCHECK( aLibrary && aEagleLibrary, nullptr );

    // Loop through the device sets and load each of them
    for( const auto& [name, edeviceset] : aLibrary->devicesets )
    {
        // Get Device set information
        wxString prefix = edeviceset->prefix ? edeviceset->prefix.Get() : wxString( wxT( "" ) );
        wxString deviceSetDescr;

        if( edeviceset->description )
            deviceSetDescr = convertDescription( UnescapeHTML( edeviceset->description->text ) );

        // For each device in the device set:
        for( const std::unique_ptr<EDEVICE>& edevice : edeviceset->devices )
        {
            // Create symbol name from deviceset and device names.
            wxString symbolName = edeviceset->name + edevice->name;
            symbolName.Replace( wxT( "*" ), wxEmptyString );
            wxASSERT( !symbolName.IsEmpty() );
            symbolName = EscapeString( symbolName, CTX_LIBID );

            if( edevice->package )
                aEagleLibrary->package[symbolName] = edevice->package.Get();

            // Create KiCad symbol.
            std::unique_ptr<LIB_SYMBOL> libSymbol = std::make_unique<LIB_SYMBOL>( symbolName );

            // Process each gate in the deviceset for this device.
            int        gate_count = static_cast<int>( edeviceset->gates.size() );
            libSymbol->SetUnitCount( gate_count );
            libSymbol->LockUnits( true );

            SCH_FIELD* reference = libSymbol->GetFieldById( REFERENCE_FIELD );

            if( prefix.length() == 0 )
            {
                reference->SetVisible( false );
            }
            else
            {
                // If there is no footprint assigned, then prepend the reference value
                // with a hash character to mute netlist updater complaints
                reference->SetText( edevice->package ? prefix : '#' + prefix );
            }

            libSymbol->GetFieldById( VALUE_FIELD )->SetVisible( true );

            int  gateindex = 1;
            bool ispower   = false;

            for( const auto& [gateName, egate] : edeviceset->gates )
            {
                const auto it = aLibrary->symbols.find( egate->symbol );

                if( it == aLibrary->symbols.end() )
                {
                    Report( wxString::Format( wxS( "Eagle symbol '%s' not found in library '%s'." ),
                                              egate->symbol, aLibrary->name ) );
                    continue;
                }

                wxString gateMapName = edeviceset->name + wxS( "_" ) + edevice->name +
                                       wxS( "_" ) + egate->name;
                aEagleLibrary->GateToUnitMap[gateMapName] = gateindex;
                ispower = loadSymbol( it->second, libSymbol, edevice, gateindex, egate->name );

                gateindex++;
            }

            libSymbol->SetUnitCount( gate_count );

            if( gate_count == 1 && ispower )
                libSymbol->SetPower();

            // Don't set the footprint field if no package is defined in the Eagle schematic.
            if( edevice->package )
            {
                wxString libName;

                if( m_schematic )
                {
                    // assume that footprint library is identical to project name
                    libName = m_schematic->Prj().GetProjectName();
                }
                else
                {
                    libName = m_libName;
                }

                wxString packageString = libName + wxT( ":" ) + aEagleLibrary->package[symbolName];

                libSymbol->GetFootprintField().SetText( packageString );
            }

            wxString libName = libSymbol->GetName();
            libSymbol->SetName( libName );
            libSymbol->SetDescription( deviceSetDescr );

            if( m_pi )
            {
                // If duplicate symbol names exist in multiple Eagle symbol libraries, prefix the
                // Eagle symbol library name to the symbol which should ensure that it is unique.
                if( m_pi->LoadSymbol( getLibFileName().GetFullPath(), libName ) )
                {
                    libName = aEagleLibrary->name + wxT( "_" ) + libName;
                    libName = EscapeString( libName, CTX_LIBID );
                    libSymbol->SetName( libName );
                }

                m_pi->SaveSymbol( getLibFileName().GetFullPath(),
                                  new LIB_SYMBOL( *libSymbol.get() ), m_properties.get() );
            }

            aEagleLibrary->KiCadSymbols[ libName ] = std::move( libSymbol );

            // Store information on whether the value of VALUE_FIELD for a part should be
            // part/@value or part/@deviceset + part/@device.
            m_userValue.emplace( std::make_pair( libName, edeviceset->uservalue == true ) );
        }
    }

    return aEagleLibrary;
}


bool SCH_IO_EAGLE::loadSymbol( const std::unique_ptr<ESYMBOL>& aEsymbol,
                               std::unique_ptr<LIB_SYMBOL>& aSymbol,
                               const std::unique_ptr<EDEVICE>& aDevice, int aGateNumber,
                               const wxString& aGateName )
{
    wxCHECK( aEsymbol && aSymbol && aDevice, false );

    wxString               symbolName = aEsymbol->name;
    std::vector<SCH_ITEM*> items;

    bool showRefDes = false;
    bool showValue  = false;
    bool ispower    = false;
    int  pincount   = 0;

    for( const std::unique_ptr<ECIRCLE>& ecircle : aEsymbol->circles )
        aSymbol->AddDrawItem( loadSymbolCircle( aSymbol, ecircle, aGateNumber ) );

    for( const std::unique_ptr<EPIN>& epin : aEsymbol->pins )
    {
        std::unique_ptr<SCH_PIN> pin( loadPin( aSymbol, epin, aGateNumber ) );
        pincount++;

        pin->SetType( ELECTRICAL_PINTYPE::PT_BIDI );

        if( epin->direction )
        {
            for( const auto& pinDir : pinDirectionsMap )
            {
                if( epin->direction->Lower() == pinDir.first )
                {
                    pin->SetType( pinDir.second );

                    if( pinDir.first == wxT( "sup" ) ) // power supply symbol
                        ispower = true;

                    break;
                }
            }

        }

        if( aDevice->connects.size() != 0 )
        {
            for( const std::unique_ptr<ECONNECT>& connect : aDevice->connects )
            {
                if( connect->gate == aGateName && pin->GetName() == connect->pin )
                {
                    wxArrayString pads = wxSplit( wxString( connect->pad ), ' ' );

                    pin->SetUnit( aGateNumber );
                    pin->SetName( escapeName( pin->GetName() ) );

                    if( pads.GetCount() > 1 )
                    {
                        pin->SetNumberTextSize( 0 );
                    }

                    for( unsigned i = 0; i < pads.GetCount(); i++ )
                    {
                        SCH_PIN* apin = new SCH_PIN( *pin );

                        wxString padname( pads[i] );
                        apin->SetNumber( padname );
                        aSymbol->AddDrawItem( apin );
                    }

                    break;
                }
            }
        }
        else
        {
            pin->SetUnit( aGateNumber );
            pin->SetNumber( wxString::Format( wxT( "%i" ), pincount ) );
            aSymbol->AddDrawItem( pin.release() );
        }
    }

    for( const std::unique_ptr<EPOLYGON>& epolygon : aEsymbol->polygons )
        aSymbol->AddDrawItem( loadSymbolPolyLine( aSymbol, epolygon, aGateNumber ) );

    for( const std::unique_ptr<ERECT>& erectangle : aEsymbol->rectangles )
        aSymbol->AddDrawItem( loadSymbolRectangle( aSymbol, erectangle, aGateNumber ) );

    for( const std::unique_ptr<ETEXT>& etext : aEsymbol->texts )
    {
        std::unique_ptr<SCH_TEXT> libtext( loadSymbolText( aSymbol, etext, aGateNumber ) );

        if( libtext->GetText() == wxT( "${REFERENCE}" ) )
        {
            // Move text & attributes to Reference field and discard LIB_TEXT item
            SCH_FIELD* field = aSymbol->GetFieldById( REFERENCE_FIELD );
            loadFieldAttributes( field, libtext.get() );

            // Show Reference field if Eagle reference was uppercase
            showRefDes = etext->text == wxT( ">NAME" );
        }
        else if( libtext->GetText() == wxT( "${VALUE}" ) )
        {
            // Move text & attributes to Value field and discard LIB_TEXT item
            SCH_FIELD* field = aSymbol->GetFieldById( VALUE_FIELD );
            loadFieldAttributes( field, libtext.get() );

            // Show Value field if Eagle reference was uppercase
            showValue = etext->text == wxT( ">VALUE" );
        }
        else
        {
            aSymbol->AddDrawItem( libtext.release() );
        }
    }

    for( const std::unique_ptr<EWIRE>& ewire : aEsymbol->wires )
        aSymbol->AddDrawItem( loadSymbolWire( aSymbol, ewire, aGateNumber ) );

    for( const std::unique_ptr<EFRAME>& eframe : aEsymbol->frames )
    {
        std::vector<SCH_ITEM*> frameItems;

        loadFrame( eframe, frameItems );

        for( SCH_ITEM* item : frameItems )
        {
            item->SetParent( aSymbol.get() );
            item->SetUnit( aGateNumber );
            aSymbol->AddDrawItem( item );
        }
    }

    aSymbol->GetFieldById( REFERENCE_FIELD )->SetVisible( showRefDes );
    aSymbol->GetFieldById( VALUE_FIELD )->SetVisible( showValue );

    return pincount == 1 ? ispower : false;
}


SCH_SHAPE* SCH_IO_EAGLE::loadSymbolCircle( std::unique_ptr<LIB_SYMBOL>& aSymbol,
                                           const std::unique_ptr<ECIRCLE>& aCircle,
                                           int aGateNumber )
{
    wxCHECK( aSymbol && aCircle, nullptr );

    // Parse the circle properties
    SCH_SHAPE* circle = new SCH_SHAPE( SHAPE_T::CIRCLE );
    VECTOR2I   center( aCircle->x.ToSchUnits(), aCircle->y.ToSchUnits() );

    circle->SetParent( aSymbol.get() );
    circle->SetPosition( center );
    circle->SetEnd( VECTOR2I( center.x + aCircle->radius.ToSchUnits(), center.y ) );
    circle->SetStroke( STROKE_PARAMS( aCircle->width.ToSchUnits(), LINE_STYLE::SOLID ) );
    circle->SetUnit( aGateNumber );

    return circle;
}


SCH_SHAPE* SCH_IO_EAGLE::loadSymbolRectangle( std::unique_ptr<LIB_SYMBOL>& aSymbol,
                                              const std::unique_ptr<ERECT>& aRectangle,
                                              int aGateNumber )
{
    wxCHECK( aSymbol && aRectangle, nullptr );

    SCH_SHAPE* rectangle = new SCH_SHAPE( SHAPE_T::RECTANGLE );

    rectangle->SetParent( aSymbol.get() );
    rectangle->SetPosition( VECTOR2I( aRectangle->x1.ToSchUnits(), -aRectangle->y1.ToSchUnits() ) );
    rectangle->SetEnd( VECTOR2I( aRectangle->x2.ToSchUnits(), -aRectangle->y2.ToSchUnits() ) );

    if( aRectangle->rot )
    {
        VECTOR2I pos( rectangle->GetPosition() );
        VECTOR2I end( rectangle->GetEnd() );
        VECTOR2I center( rectangle->GetCenter() );

        RotatePoint( pos, center, EDA_ANGLE( aRectangle->rot->degrees, DEGREES_T ) );
        RotatePoint( end,  center, EDA_ANGLE( aRectangle->rot->degrees, DEGREES_T ) );

        rectangle->SetPosition( pos );
        rectangle->SetEnd( end );
    }

    rectangle->SetUnit( aGateNumber );

    // Eagle rectangles are filled by definition.
    rectangle->SetFillMode( FILL_T::FILLED_SHAPE );

    return rectangle;
}


SCH_ITEM* SCH_IO_EAGLE::loadSymbolWire( std::unique_ptr<LIB_SYMBOL>& aSymbol,
                                        const std::unique_ptr<EWIRE>& aWire, int aGateNumber )
{
    wxCHECK( aSymbol && aWire, nullptr );

    VECTOR2I begin, end;

    begin.x = aWire->x1.ToSchUnits();
    begin.y = -aWire->y1.ToSchUnits();
    end.x   = aWire->x2.ToSchUnits();
    end.y   = -aWire->y2.ToSchUnits();

    if( begin == end )
        return nullptr;

    // if the wire is an arc
    if( aWire->curve )
    {
        SCH_SHAPE* arc = new SCH_SHAPE( SHAPE_T::ARC, LAYER_DEVICE );
        VECTOR2I   center = ConvertArcCenter( begin, end, *aWire->curve * -1 );
        double     radius = sqrt( ( ( center.x - begin.x ) * ( center.x - begin.x ) ) +
                                  ( ( center.y - begin.y ) * ( center.y - begin.y ) ) );

        arc->SetParent( aSymbol.get() );

        // this emulates the filled semicircles created by a thick arc with flat ends caps.
        if( aWire->cap == EWIRE::FLAT && aWire->width.ToSchUnits() >= 2 * radius )
        {
            VECTOR2I centerStartVector = ( begin - center ) *
                                         ( aWire->width.ToSchUnits() / radius );
            begin = center + centerStartVector;

            arc->SetStroke( STROKE_PARAMS( 1, LINE_STYLE::SOLID ) );
            arc->SetFillMode( FILL_T::FILLED_SHAPE );
        }
        else
        {
            arc->SetStroke( STROKE_PARAMS( aWire->width.ToSchUnits(), LINE_STYLE::SOLID ) );
        }

        arc->SetCenter( center );
        arc->SetStart( begin );
        arc->SetArcAngleAndEnd( EDA_ANGLE( *aWire->curve * -1, DEGREES_T ), true );
        arc->SetUnit( aGateNumber );

        return arc;
    }
    else
    {
        SCH_SHAPE* poly = new SCH_SHAPE( SHAPE_T::POLY, LAYER_DEVICE );

        poly->AddPoint( begin );
        poly->AddPoint( end );
        poly->SetUnit( aGateNumber );
        poly->SetStroke( STROKE_PARAMS( aWire->width.ToSchUnits(), LINE_STYLE::SOLID ) );

        return poly;
    }
}


SCH_SHAPE* SCH_IO_EAGLE::loadSymbolPolyLine( std::unique_ptr<LIB_SYMBOL>& aSymbol,
                                             const std::unique_ptr<EPOLYGON>& aPolygon,
                                             int aGateNumber )
{
    wxCHECK( aSymbol && aPolygon, nullptr );

    SCH_SHAPE* poly = new SCH_SHAPE( SHAPE_T::POLY );
    VECTOR2I   pt, prev_pt;
    opt_double prev_curve;

    poly->SetParent( aSymbol.get() );

    for( const std::unique_ptr<EVERTEX>& evertex : aPolygon->vertices )
    {
        pt = VECTOR2I( evertex->x.ToSchUnits(), evertex->y.ToSchUnits() );

        if( prev_curve )
        {
            SHAPE_ARC arc;
            arc.ConstructFromStartEndAngle( prev_pt, pt, -EDA_ANGLE( *prev_curve, DEGREES_T ) );
            poly->GetPolyShape().Append( arc, -1, -1, ARC_ACCURACY );
        }
        else
        {
            poly->AddPoint( pt );
        }

        prev_pt = pt;
        prev_curve = evertex->curve;
    }

    poly->SetStroke( STROKE_PARAMS( aPolygon->width.ToSchUnits(), LINE_STYLE::SOLID ) );
    poly->SetFillMode( FILL_T::FILLED_SHAPE );
    poly->SetUnit( aGateNumber );

    return poly;
}


SCH_PIN* SCH_IO_EAGLE::loadPin( std::unique_ptr<LIB_SYMBOL>& aSymbol,
                                const std::unique_ptr<EPIN>& aPin, int aGateNumber )
{
    wxCHECK( aSymbol && aPin, nullptr );

    std::unique_ptr<SCH_PIN> pin = std::make_unique<SCH_PIN>( aSymbol.get() );
    pin->SetPosition( VECTOR2I( aPin->x.ToSchUnits(), -aPin->y.ToSchUnits() ) );
    pin->SetName( aPin->name );
    pin->SetUnit( aGateNumber );

    int roti = aPin->rot ? aPin->rot->degrees : 0;

    switch( roti )
    {
    case 0:   pin->SetOrientation( PIN_ORIENTATION::PIN_RIGHT ); break;
    case 90:  pin->SetOrientation( PIN_ORIENTATION::PIN_UP ); break;
    case 180: pin->SetOrientation( PIN_ORIENTATION::PIN_LEFT ); break;
    case 270: pin->SetOrientation( PIN_ORIENTATION::PIN_DOWN ); break;
    default:  wxFAIL_MSG( wxString::Format( wxT( "Unhandled orientation (%d degrees)." ), roti ) );
    }

    pin->SetLength( schIUScale.MilsToIU( 300 ) );  // Default pin length when not defined.

    if( aPin->length )
    {
        wxString length = aPin->length.Get();

        if( length == wxT( "short" ) )
            pin->SetLength( schIUScale.MilsToIU( 100 ) );
        else if( length == wxT( "middle" ) )
            pin->SetLength( schIUScale.MilsToIU( 200 ) );
        else if( length == wxT( "long" ) )
            pin->SetLength( schIUScale.MilsToIU( 300 ) );
        else if( length == wxT( "point" ) )
            pin->SetLength( schIUScale.MilsToIU( 0 ) );
    }

    // emulate the visibility of pin elements
    if( aPin->visible )
    {
        wxString visible = aPin->visible.Get();

        if( visible == wxT( "off" ) )
        {
            pin->SetNameTextSize( 0 );
            pin->SetNumberTextSize( 0 );
        }
        else if( visible == wxT( "pad" ) )
        {
            pin->SetNameTextSize( 0 );
        }
        else if( visible == wxT( "pin" ) )
        {
            pin->SetNumberTextSize( 0 );
        }

        /*
         *  else if( visible == wxT( "both" ) )
         *  {
         *  }
         */
    }

    if( aPin->function )
    {
        wxString function = aPin->function.Get();

        if( function == wxT( "dot" ) )
            pin->SetShape( GRAPHIC_PINSHAPE::INVERTED );
        else if( function == wxT( "clk" ) )
            pin->SetShape( GRAPHIC_PINSHAPE::CLOCK );
        else if( function == wxT( "dotclk" ) )
            pin->SetShape( GRAPHIC_PINSHAPE::INVERTED_CLOCK );
    }

    return pin.release();
}


SCH_TEXT* SCH_IO_EAGLE::loadSymbolText( std::unique_ptr<LIB_SYMBOL>& aSymbol,
                                        const std::unique_ptr<ETEXT>& aText, int aGateNumber )
{
    wxCHECK( aSymbol && aText, nullptr );

    std::unique_ptr<SCH_TEXT> libtext = std::make_unique<SCH_TEXT>();

    libtext->SetParent( aSymbol.get() );
    libtext->SetUnit( aGateNumber );
    libtext->SetPosition( VECTOR2I( aText->x.ToSchUnits(), -aText->y.ToSchUnits() ) );

    const wxString&   eagleText = aText->text;
    wxString          adjustedText;
    wxStringTokenizer tokenizer( eagleText, "\r\n" );

    // Strip the whitespace from both ends of each line.
    while( tokenizer.HasMoreTokens() )
    {
        wxString tmp = interpretText( tokenizer.GetNextToken().Trim( true ).Trim( false ) );

        if( tokenizer.HasMoreTokens() )
            tmp += wxT( "\n" );

        adjustedText += tmp;
    }

    libtext->SetText( adjustedText.IsEmpty() ? wxString( wxS( "~" ) ) : adjustedText );

    loadTextAttributes( libtext.get(), aText );

    return libtext.release();
}


SCH_TEXT* SCH_IO_EAGLE::loadPlainText( const std::unique_ptr<ETEXT>& aText )
{
    wxCHECK( aText, nullptr );

    std::unique_ptr<SCH_TEXT> schtext = std::make_unique<SCH_TEXT>();

    const wxString&   eagleText = aText->text;
    wxString          adjustedText;
    wxStringTokenizer tokenizer( eagleText, "\r\n" );

    // Strip the whitespace from both ends of each line.
    while( tokenizer.HasMoreTokens() )
    {
        wxString tmp = interpretText( tokenizer.GetNextToken().Trim( true ).Trim( false ) );

        if( tokenizer.HasMoreTokens() )
            tmp += wxT( "\n" );

        adjustedText += tmp;
    }

    schtext->SetText( adjustedText.IsEmpty() ? wxString( wxS( "\" \"" ) )
                                             : escapeName( adjustedText ) );

    schtext->SetPosition( VECTOR2I( aText->x.ToSchUnits(), -aText->y.ToSchUnits() ) );
    loadTextAttributes( schtext.get(), aText );
    schtext->SetItalic( false );

    return schtext.release();
}


void SCH_IO_EAGLE::loadTextAttributes( EDA_TEXT* aText,
                                       const std::unique_ptr<ETEXT>& aAttributes ) const
{
    wxCHECK( aText && aAttributes, /* void */ );

    aText->SetTextSize( aAttributes->ConvertSize() );

    // Must come after SetTextSize()
    if( aAttributes->ratio && aAttributes->ratio.CGet() > 12 )
        aText->SetBold( true );

    int  align   = aAttributes->align ? *aAttributes->align : ETEXT::BOTTOM_LEFT;
    int  degrees = aAttributes->rot ? aAttributes->rot->degrees : 0;
    bool mirror  = aAttributes->rot ? aAttributes->rot->mirror : false;
    bool spin    = aAttributes->rot ? aAttributes->rot->spin : false;

    eagleToKicadAlignment( aText, align, degrees, mirror, spin, 0 );
}


void SCH_IO_EAGLE::loadFieldAttributes( SCH_FIELD* aField, const SCH_TEXT* aText ) const
{
    wxCHECK( aField && aText, /* void */ );

    aField->SetTextPos( aText->GetPosition() );
    aField->SetTextSize( aText->GetTextSize() );
    aField->SetTextAngle( aText->GetTextAngle() );

    // Must come after SetTextSize()
    aField->SetBold( aText->IsBold() );
    aField->SetItalic( false );

    aField->SetVertJustify( aText->GetVertJustify() );
    aField->SetHorizJustify( aText->GetHorizJustify() );
}


void SCH_IO_EAGLE::adjustNetLabels()
{
    // Eagle supports detached labels, so a label does not need to be placed on a wire
    // to be associated with it. KiCad needs to move them, so the labels actually touch the
    // corresponding wires.

    // Sort the intersection points to speed up the search process
    std::sort( m_wireIntersections.begin(), m_wireIntersections.end() );

    auto onIntersection =
            [&]( const VECTOR2I& aPos )
            {
                return std::binary_search( m_wireIntersections.begin(),
                                           m_wireIntersections.end(), aPos );
            };

    for( SEG_DESC& segDesc : m_segments )
    {
        for( SCH_TEXT* label : segDesc.labels )
        {
            VECTOR2I   labelPos( label->GetPosition() );
            const SEG* segAttached = segDesc.LabelAttached( label );

            if( segAttached && !onIntersection( labelPos ) )
                continue; // label is placed correctly

            // Move the label to the nearest wire
            if( !segAttached )
            {
                std::tie( labelPos, segAttached ) = findNearestLinePoint( label->GetPosition(),
                                                                          segDesc.segs );

                if( !segAttached ) // we cannot do anything
                    continue;
            }

            // Create a vector pointing in the direction of the wire, 50 mils long
            VECTOR2I wireDirection( segAttached->B - segAttached->A );
            wireDirection = wireDirection.Resize( schIUScale.MilsToIU( 50 ) );
            const VECTOR2I origPos( labelPos );

            // Flags determining the search direction
            bool checkPositive = true, checkNegative = true, move = false;
            int  trial = 0;

            // Be sure the label is not placed on a wire intersection
            while( ( !move || onIntersection( labelPos ) ) && ( checkPositive || checkNegative ) )
            {
                move = false;

                // Move along the attached wire to find the new label position
                if( trial % 2 == 1 )
                {
                    labelPos = VECTOR2I( origPos + wireDirection * trial / 2 );
                    move = checkPositive = segAttached->Contains( labelPos );
                }
                else
                {
                    labelPos = VECTOR2I( origPos - wireDirection * trial / 2 );
                    move = checkNegative = segAttached->Contains( labelPos );
                }

                ++trial;
            }

            if( move )
                label->SetPosition( VECTOR2I( labelPos ) );
        }
    }

    m_segments.clear();
    m_wireIntersections.clear();
}


bool SCH_IO_EAGLE::CanReadSchematicFile( const wxString& aFileName ) const
{
    if( !SCH_IO::CanReadSchematicFile( aFileName ) )
        return false;

    return checkHeader( aFileName );
}


bool SCH_IO_EAGLE::CanReadLibrary( const wxString& aFileName ) const
{
    if( !SCH_IO::CanReadLibrary( aFileName ) )
        return false;

    return checkHeader( aFileName );
}


bool SCH_IO_EAGLE::checkHeader( const wxString& aFileName ) const
{
    wxFileInputStream input( aFileName );

    if( !input.IsOk() )
        return false;

    wxTextInputStream text( input );

    for( int i = 0; i < 4; i++ )
    {
        if( input.Eof() )
            return false;

        if( text.ReadLine().Contains( wxS( "<eagle" ) ) )
            return true;
    }

    return false;
}


void SCH_IO_EAGLE::moveLabels( SCH_LINE* aWire, const VECTOR2I& aNewEndPoint )
{
    wxCHECK( aWire, /* void */ );

    SCH_SCREEN* screen = getCurrentScreen();

    wxCHECK( screen, /* void */ );

    for( SCH_ITEM* item : screen->Items().Overlapping( aWire->GetBoundingBox() ) )
    {
        if( !item->IsType( { SCH_LABEL_LOCATE_ANY_T } ) )
            continue;

        if( TestSegmentHit( item->GetPosition(), aWire->GetStartPoint(), aWire->GetEndPoint(), 0 ) )
            item->SetPosition( aNewEndPoint );
    }
}


void SCH_IO_EAGLE::addBusEntries()
{
    // Add bus entry symbols
    // TODO: Cleanup this function and break into pieces

    // for each wire segment, compare each end with all busses.
    // If the wire end is found to end on a bus segment, place a bus entry symbol.

    std::vector<SCH_LINE*> buses;
    std::vector<SCH_LINE*> wires;

    SCH_SCREEN* screen = getCurrentScreen();

    wxCHECK( screen, /* void */ );

    for( SCH_ITEM* ii : screen->Items().OfType( SCH_LINE_T ) )
    {
        SCH_LINE* line = static_cast<SCH_LINE*>( ii );

        if( line->IsBus() )
            buses.push_back( line );
        else if( line->IsWire() )
            wires.push_back( line );
    }

    for( SCH_LINE* wire : wires )
    {
        VECTOR2I wireStart = wire->GetStartPoint();
        VECTOR2I wireEnd = wire->GetEndPoint();

        for( SCH_LINE* bus : buses )
        {
            VECTOR2I busStart = bus->GetStartPoint();
            VECTOR2I busEnd = bus->GetEndPoint();

            auto entrySize =
                    []( int signX, int signY ) -> VECTOR2I
                    {
                        return VECTOR2I( schIUScale.MilsToIU( DEFAULT_SCH_ENTRY_SIZE ) * signX,
                                         schIUScale.MilsToIU( DEFAULT_SCH_ENTRY_SIZE ) * signY );
                    };

            auto testBusHit =
                    [&]( const VECTOR2I& aPt ) -> bool
                    {
                        return TestSegmentHit( aPt, busStart, busEnd, 0 );
                    };

            if( wireStart.y == wireEnd.y && busStart.x == busEnd.x )
            {
                // Horizontal wire and vertical bus

                if( testBusHit( wireStart ) )
                {
                    // Wire start is on the vertical bus

                    if( wireEnd.x < busStart.x )
                    {
                        /* the end of the wire is to the left of the bus
                         *         ⎥⎢
                         *   ——————⎥⎢
                         *         ⎥⎢
                         */
                        VECTOR2I p = wireStart + entrySize( -1, 0 );

                        if( testBusHit( wireStart + entrySize( 0, -1 )  ) )
                        {
                            /* there is room above the wire for the bus entry
                             *         ⎥⎢
                             *   _____/⎥⎢
                             *         ⎥⎢
                             */
                            SCH_BUS_WIRE_ENTRY* busEntry = new SCH_BUS_WIRE_ENTRY( p, 1 );
                            busEntry->SetFlags( IS_NEW );
                            screen->Append( busEntry );
                            moveLabels( wire, p );
                            wire->SetStartPoint( p );
                        }
                        else if( testBusHit( wireStart + entrySize( 0, 1 ) ) )
                        {
                            /* there is room below the wire for the bus entry
                             *   _____ ⎥⎢
                             *        \⎥⎢
                             *         ⎥⎢
                             */
                            SCH_BUS_WIRE_ENTRY* busEntry = new SCH_BUS_WIRE_ENTRY( p, 2 );
                            busEntry->SetFlags( IS_NEW );
                            screen->Append( busEntry );
                            moveLabels( wire, p );
                            wire->SetStartPoint( p );
                        }
                        else
                        {
                            auto        ercItem = ERC_ITEM::Create( ERCE_BUS_ENTRY_NEEDED );
                            SCH_MARKER* marker = new SCH_MARKER( ercItem, wireStart );
                            screen->Append( marker );
                        }
                    }
                    else
                    {
                        /* the wire end is to the right of the bus
                         *   ⎥⎢
                         *   ⎥⎢——————
                         *   ⎥⎢
                         */
                        VECTOR2I p = wireStart + entrySize( 1, 0 );

                        if( testBusHit( wireStart + entrySize( 0, -1 ) ) )
                        {
                            /* There is room above the wire for the bus entry
                             *   ⎥⎢
                             *   ⎥⎢\_____
                             *   ⎥⎢
                             */
                            SCH_BUS_WIRE_ENTRY* busEntry = new SCH_BUS_WIRE_ENTRY( p , 4 );
                            busEntry->SetFlags( IS_NEW );
                            screen->Append( busEntry );
                            moveLabels( wire, p );
                            wire->SetStartPoint( p );
                        }
                        else if( testBusHit( wireStart + entrySize( 0, 1 ) ) )
                        {
                            /* There is room below the wire for the bus entry
                             *   ⎥⎢ _____
                             *   ⎥⎢/
                             *   ⎥⎢
                             */
                            SCH_BUS_WIRE_ENTRY* busEntry = new SCH_BUS_WIRE_ENTRY( p, 3 );
                            busEntry->SetFlags( IS_NEW );
                            screen->Append( busEntry );
                            moveLabels( wire, p );
                            wire->SetStartPoint( p );
                        }
                        else
                        {
                            auto        ercItem = ERC_ITEM::Create( ERCE_BUS_ENTRY_NEEDED );
                            SCH_MARKER* marker = new SCH_MARKER( ercItem, wireStart );
                            screen->Append( marker );
                        }
                    }

                    break;
                }
                else if( testBusHit( wireEnd ) )
                {
                    // Wire end is on the vertical bus

                    if( wireStart.x < busStart.x )
                    {
                        /* start of the wire is to the left of the bus
                         *        ⎥⎢
                         *  ——————⎥⎢
                         *        ⎥⎢
                         */
                        VECTOR2I p = wireEnd + entrySize( -1, 0 );

                        if( testBusHit( wireEnd + entrySize( 0, -1 ) ) )
                        {
                            /* there is room above the wire for the bus entry
                             *         ⎥⎢
                             *   _____/⎥⎢
                             *         ⎥⎢
                             */
                            SCH_BUS_WIRE_ENTRY* busEntry = new SCH_BUS_WIRE_ENTRY( p, 1 );
                            busEntry->SetFlags( IS_NEW );
                            screen->Append( busEntry );
                            moveLabels( wire, p );
                            wire->SetEndPoint( p );
                        }
                        else if( testBusHit( wireEnd + entrySize( 0, -1 ) ) )
                        {
                            /* there is room below the wire for the bus entry
                             *   _____ ⎥⎢
                             *        \⎥⎢
                             *         ⎥⎢
                             */
                            SCH_BUS_WIRE_ENTRY* busEntry = new SCH_BUS_WIRE_ENTRY( p, 2 );
                            busEntry->SetFlags( IS_NEW );
                            screen->Append( busEntry );
                            moveLabels( wire, wireEnd + entrySize( -1, 0 ) );
                            wire->SetEndPoint( wireEnd + entrySize( -1, 0 ) );
                        }
                        else
                        {
                            auto        ercItem = ERC_ITEM::Create( ERCE_BUS_ENTRY_NEEDED );
                            SCH_MARKER* marker = new SCH_MARKER( ercItem, wireEnd );
                            screen->Append( marker );
                        }
                    }
                    else
                    {
                        /* the start of the wire is to the right of the bus
                         *   ⎥⎢
                         *   ⎥⎢——————
                         *   ⎥⎢
                         */
                        VECTOR2I p = wireEnd + entrySize( 1, 0 );

                        if( testBusHit( wireEnd + entrySize( 0, -1 ) ) )
                        {
                            /* There is room above the wire for the bus entry
                             *   ⎥⎢
                             *   ⎥⎢\_____
                             *   ⎥⎢
                             */
                            SCH_BUS_WIRE_ENTRY* busEntry = new SCH_BUS_WIRE_ENTRY( p, 4 );
                            busEntry->SetFlags( IS_NEW );
                            screen->Append( busEntry );
                            moveLabels( wire, p );
                            wire->SetEndPoint( p );
                        }
                        else if( testBusHit( wireEnd + entrySize( 0, 1 ) ) )
                        {
                            /* There is room below the wire for the bus entry
                             *   ⎥⎢ _____
                             *   ⎥⎢/
                             *   ⎥⎢
                             */
                            SCH_BUS_WIRE_ENTRY* busEntry = new SCH_BUS_WIRE_ENTRY( p, 3 );
                            busEntry->SetFlags( IS_NEW );
                            screen->Append( busEntry );
                            moveLabels( wire, p );
                            wire->SetEndPoint( p );
                        }
                        else
                        {
                            auto        ercItem = ERC_ITEM::Create( ERCE_BUS_ENTRY_NEEDED );
                            SCH_MARKER* marker = new SCH_MARKER( ercItem, wireEnd );
                            screen->Append( marker );
                        }
                    }

                    break;
                }
            }
            else if( wireStart.x == wireEnd.x && busStart.y == busEnd.y )
            {
                // Vertical wire and horizontal bus

                if( testBusHit( wireStart ) )
                {
                    // Wire start is on the bus

                    if( wireEnd.y < busStart.y )
                    {
                        /* the end of the wire is above the bus
                         *      |
                         *      |
                         *      |
                         *   =======
                         */
                        VECTOR2I p = wireStart + entrySize( 0, -1 );

                        if( testBusHit( wireStart + entrySize( -1, 0 ) ) )
                        {
                            /* there is room to the left of the wire for the bus entry
                             *      |
                             *      |
                             *     /
                             *  =======
                             */
                            SCH_BUS_WIRE_ENTRY* busEntry = new SCH_BUS_WIRE_ENTRY( p, 3 );
                            busEntry->SetFlags( IS_NEW );
                            screen->Append( busEntry );
                            moveLabels( wire, p );
                            wire->SetStartPoint( p );
                        }
                        else if( testBusHit( wireStart + entrySize( 1, 0 ) ) )
                        {
                            /* there is room to the right of the wire for the bus entry
                             *    |
                             *    |
                             *     \
                             *  =======
                             */
                            SCH_BUS_WIRE_ENTRY* busEntry = new SCH_BUS_WIRE_ENTRY( p, 2 );
                            busEntry->SetFlags( IS_NEW );
                            screen->Append( busEntry );
                            moveLabels( wire, p );
                            wire->SetStartPoint( p );
                        }
                        else
                        {
                            auto        ercItem = ERC_ITEM::Create( ERCE_BUS_ENTRY_NEEDED );
                            SCH_MARKER* marker = new SCH_MARKER( ercItem, wireStart );
                            screen->Append( marker );
                        }
                    }
                    else
                    {
                        /* wire end is below the bus
                         *   =======
                         *      |
                         *      |
                         *      |
                         */
                        VECTOR2I p = wireStart + entrySize( 0, 1 );

                        if( testBusHit( wireStart + entrySize( -1, 0 ) ) )
                        {
                            /* there is room to the left of the wire for the bus entry
                             *  =======
                             *     \
                             *      |
                             *      |
                             */
                            SCH_BUS_WIRE_ENTRY* busEntry = new SCH_BUS_WIRE_ENTRY( p, 4 );
                            busEntry->SetFlags( IS_NEW );
                            screen->Append( busEntry );
                            moveLabels( wire, p );
                            wire->SetStartPoint( p );
                        }
                        else if( testBusHit( wireStart + entrySize( 1, 0 ) ) )
                        {
                            /* there is room to the right of the wire for the bus entry
                             *  =======
                             *     /
                             *    |
                             *    |
                             */
                            SCH_BUS_WIRE_ENTRY* busEntry = new SCH_BUS_WIRE_ENTRY( p, 1 );
                            busEntry->SetFlags( IS_NEW );
                            screen->Append( busEntry );
                            moveLabels( wire, p );
                            wire->SetStartPoint( p );
                        }
                        else
                        {
                            auto        ercItem = ERC_ITEM::Create( ERCE_BUS_ENTRY_NEEDED );
                            SCH_MARKER* marker = new SCH_MARKER( ercItem, wireStart );
                            screen->Append( marker );
                        }
                    }

                    break;
                }
                else if( testBusHit( wireEnd ) )
                {
                    // Wire end is on the bus

                    if( wireStart.y < busStart.y )
                    {
                        /* the start of the wire is above the bus
                         *      |
                         *      |
                         *      |
                         *   =======
                         */
                        VECTOR2I p = wireEnd + entrySize( 0, -1 );

                        if( testBusHit( wireEnd + entrySize( -1, 0 ) ) )
                        {
                            /* there is room to the left of the wire for the bus entry
                             *      |
                             *      |
                             *     /
                             *  =======
                             */
                            SCH_BUS_WIRE_ENTRY* busEntry = new SCH_BUS_WIRE_ENTRY( p, 3 );
                            busEntry->SetFlags( IS_NEW );
                            screen->Append( busEntry );
                            moveLabels( wire, p );
                            wire->SetEndPoint( p );
                        }
                        else if( testBusHit( wireEnd + entrySize( 1, 0 ) ) )
                        {
                            /* there is room to the right of the wire for the bus entry
                             *    |
                             *    |
                             *     \
                             *  =======
                             */
                            SCH_BUS_WIRE_ENTRY* busEntry = new SCH_BUS_WIRE_ENTRY( p, 2 );
                            busEntry->SetFlags( IS_NEW );
                            screen->Append( busEntry );
                            moveLabels( wire, p );
                            wire->SetEndPoint( p );
                        }
                        else
                        {
                            auto        ercItem = ERC_ITEM::Create( ERCE_BUS_ENTRY_NEEDED );
                            SCH_MARKER* marker = new SCH_MARKER( ercItem, wireEnd );
                            screen->Append( marker );
                        }
                    }
                    else
                    {
                        /* wire start is below the bus
                         *   =======
                         *      |
                         *      |
                         *      |
                         */
                        VECTOR2I p = wireEnd + entrySize( 0, 1 );

                        if( testBusHit( wireEnd + entrySize( -1, 0 ) ) )
                        {
                            /* there is room to the left of the wire for the bus entry
                             *  =======
                             *     \
                             *      |
                             *      |
                             */
                            SCH_BUS_WIRE_ENTRY* busEntry = new SCH_BUS_WIRE_ENTRY( p, 4 );
                            busEntry->SetFlags( IS_NEW );
                            screen->Append( busEntry );
                            moveLabels( wire, p );
                            wire->SetEndPoint( p );
                        }
                        else if( testBusHit( wireEnd + entrySize( 1, 0 ) ) )
                        {
                            /* there is room to the right of the wire for the bus entry
                             *  =======
                             *     /
                             *    |
                             *    |
                             */
                            SCH_BUS_WIRE_ENTRY* busEntry = new SCH_BUS_WIRE_ENTRY( p, 1 );
                            busEntry->SetFlags( IS_NEW );
                            screen->Append( busEntry );
                            moveLabels( wire, p );
                            wire->SetEndPoint( p );
                        }
                        else
                        {
                            auto        ercItem = ERC_ITEM::Create( ERCE_BUS_ENTRY_NEEDED );
                            SCH_MARKER* marker = new SCH_MARKER( ercItem, wireEnd );
                            screen->Append( marker );
                        }
                    }

                    break;
                }
            }
            else
            {
                // Wire isn't horizontal or vertical

                if( testBusHit( wireStart ) )
                {
                    VECTOR2I wirevector = wireStart - wireEnd;

                    if( wirevector.x > 0 )
                    {
                        if( wirevector.y > 0 )
                        {
                            VECTOR2I            p = wireStart + entrySize( -1, -1 );
                            SCH_BUS_WIRE_ENTRY* busEntry = new SCH_BUS_WIRE_ENTRY( p, 2 );
                            busEntry->SetFlags( IS_NEW );
                            screen->Append( busEntry );

                            moveLabels( wire, p );
                            wire->SetStartPoint( p );
                        }
                        else
                        {
                            VECTOR2I            p = wireStart + entrySize( -1, 1 );
                            SCH_BUS_WIRE_ENTRY* busEntry = new SCH_BUS_WIRE_ENTRY( p, 1 );
                            busEntry->SetFlags( IS_NEW );
                            screen->Append( busEntry );

                            moveLabels( wire, p );
                            wire->SetStartPoint( p );
                        }
                    }
                    else
                    {
                        if( wirevector.y > 0 )
                        {
                            VECTOR2I            p = wireStart + entrySize( 1, -1 );
                            SCH_BUS_WIRE_ENTRY* busEntry = new SCH_BUS_WIRE_ENTRY( p, 3 );
                            busEntry->SetFlags( IS_NEW );
                            screen->Append( busEntry );

                            moveLabels( wire, p );
                            wire->SetStartPoint( p );
                        }
                        else
                        {
                            VECTOR2I            p = wireStart + entrySize( 1, 1 );
                            SCH_BUS_WIRE_ENTRY* busEntry = new SCH_BUS_WIRE_ENTRY( p, 4 );
                            busEntry->SetFlags( IS_NEW );
                            screen->Append( busEntry );

                            moveLabels( wire, p );
                            wire->SetStartPoint( p );
                        }
                    }

                    break;
                }
                else if( testBusHit( wireEnd ) )
                {
                    VECTOR2I wirevector = wireStart - wireEnd;

                    if( wirevector.x > 0 )
                    {
                        if( wirevector.y > 0 )
                        {
                            VECTOR2I            p = wireEnd + entrySize( 1, 1 );
                            SCH_BUS_WIRE_ENTRY* busEntry = new SCH_BUS_WIRE_ENTRY( p, 4 );
                            busEntry->SetFlags( IS_NEW );
                            screen->Append( busEntry );

                            moveLabels( wire, p );
                            wire->SetEndPoint( p );
                        }
                        else
                        {
                            VECTOR2I            p = wireEnd + entrySize( 1, -1 );
                            SCH_BUS_WIRE_ENTRY* busEntry = new SCH_BUS_WIRE_ENTRY( p, 3 );
                            busEntry->SetFlags( IS_NEW );
                            screen->Append( busEntry );

                            moveLabels( wire, p );
                            wire->SetEndPoint( p );
                        }
                    }
                    else
                    {
                        if( wirevector.y > 0 )
                        {
                            VECTOR2I            p = wireEnd + entrySize( -1, 1 );
                            SCH_BUS_WIRE_ENTRY* busEntry = new SCH_BUS_WIRE_ENTRY( p, 1 );
                            busEntry->SetFlags( IS_NEW );
                            screen->Append( busEntry );

                            moveLabels( wire, p );
                            wire->SetEndPoint( p );
                        }
                        else
                        {
                            VECTOR2I            p = wireEnd + entrySize( -1, -1 );
                            SCH_BUS_WIRE_ENTRY* busEntry = new SCH_BUS_WIRE_ENTRY( p, 2 );
                            busEntry->SetFlags( IS_NEW );
                            screen->Append( busEntry );

                            moveLabels( wire, p );
                            wire->SetEndPoint( p );
                        }
                    }

                    break;
                }
            }
        }
    }
}


const SEG* SCH_IO_EAGLE::SEG_DESC::LabelAttached( const SCH_TEXT* aLabel ) const
{
    wxCHECK( aLabel, nullptr );

    VECTOR2I labelPos( aLabel->GetPosition() );

    for( const SEG& seg : segs )
    {
        if( seg.Contains( labelPos ) )
            return &seg;
    }

    return nullptr;
}


// TODO could be used to place junctions, instead of IsJunctionNeeded()
// (see SCH_EDIT_FRAME::importFile())
bool SCH_IO_EAGLE::checkConnections( const SCH_SYMBOL* aSymbol, const SCH_PIN* aPin ) const
{
    wxCHECK( aSymbol && aPin, false );

    VECTOR2I pinPosition = aSymbol->GetPinPhysicalPosition( aPin );
    auto     pointIt     = m_connPoints.find( pinPosition );

    if( pointIt == m_connPoints.end() )
        return false;

    const auto& items = pointIt->second;

    wxCHECK( items.find( aPin ) != items.end(), false );

    return items.size() > 1;
}


void SCH_IO_EAGLE::addImplicitConnections( SCH_SYMBOL* aSymbol, SCH_SCREEN* aScreen,
                                           bool aUpdateSet )
{
    wxCHECK( aSymbol && aScreen && aSymbol->GetLibSymbolRef(), /*void*/ );

    // Normally power parts also have power input pins,
    // but they already force net names on the attached wires
    if( aSymbol->GetLibSymbolRef()->IsPower() )
        return;

    int                   unit      = aSymbol->GetUnit();
    const wxString        reference = aSymbol->GetField( REFERENCE_FIELD )->GetText();
    std::vector<SCH_PIN*> pins      = aSymbol->GetLibSymbolRef()->GetAllLibPins();
    std::set<int>         missingUnits;

    // Search all units for pins creating implicit connections
    for( const SCH_PIN* pin : pins )
    {
        if( pin->GetType() == ELECTRICAL_PINTYPE::PT_POWER_IN )
        {
            bool pinInUnit = !unit || pin->GetUnit() == unit; // pin belongs to the tested unit

            // Create a global net label only if there are no other wires/pins attached
            if( pinInUnit )
            {
                if( !checkConnections( aSymbol, pin ) )
                {
                    // Create a net label to force the net name on the pin
                    SCH_GLOBALLABEL* netLabel = new SCH_GLOBALLABEL;
                    netLabel->SetPosition( aSymbol->GetPinPhysicalPosition( pin ) );
                    netLabel->SetText( extractNetName( pin->GetName() ) );
                    netLabel->SetTextSize( VECTOR2I( schIUScale.MilsToIU( 40 ),
                                                     schIUScale.MilsToIU( 40 ) ) );

                    switch( pin->GetOrientation() )
                    {
                    default:
                    case PIN_ORIENTATION::PIN_RIGHT:
                        netLabel->SetSpinStyle( SPIN_STYLE::LEFT );
                        break;
                    case PIN_ORIENTATION::PIN_LEFT:
                        netLabel->SetSpinStyle( SPIN_STYLE::RIGHT );
                        break;
                    case PIN_ORIENTATION::PIN_UP:
                        netLabel->SetSpinStyle( SPIN_STYLE::UP );
                        break;
                    case PIN_ORIENTATION::PIN_DOWN:
                        netLabel->SetSpinStyle( SPIN_STYLE::BOTTOM );
                        break;
                    }

                    aScreen->Append( netLabel );
                }
            }
            else if( aUpdateSet )
            {
                // Found a pin creating implicit connection information in another unit.
                // Such units will be instantiated if they do not appear in another sheet and
                // processed later.
                wxASSERT( pin->GetUnit() );
                missingUnits.insert( pin->GetUnit() );
            }
        }
    }

    if( aUpdateSet && aSymbol->GetLibSymbolRef()->GetUnitCount() > 1 )
    {
        auto cmpIt = m_missingCmps.find( reference );

        // The first unit found has always already been processed.
        if( cmpIt == m_missingCmps.end() )
        {
            EAGLE_MISSING_CMP& entry = m_missingCmps[reference];
            entry.cmp                = aSymbol;
            entry.units.emplace( unit, false );
        }
        else
        {
            // Set the flag indicating this unit has been processed.
            cmpIt->second.units[unit] = false;
        }

        if( !missingUnits.empty() )        // Save the units that need later processing
        {
            EAGLE_MISSING_CMP& entry = m_missingCmps[reference];
            entry.cmp                = aSymbol;

            // Add units that haven't already been processed.
            for( int i : missingUnits )
            {
                if( entry.units.find( i ) != entry.units.end() )
                    entry.units.emplace( i, true );
            }
        }
    }
}


wxString SCH_IO_EAGLE::translateEagleBusName( const wxString& aEagleName ) const
{
    if( NET_SETTINGS::ParseBusVector( aEagleName, nullptr, nullptr ) )
        return aEagleName;

    wxString ret = wxT( "{" );

    wxStringTokenizer tokenizer( aEagleName, wxT( "," ) );

    while( tokenizer.HasMoreTokens() )
    {
        wxString member = tokenizer.GetNextToken();

        // In Eagle, overbar text is automatically stopped at the end of the net name, even when
        // that net name is part of a bus definition.  In KiCad, we don't (currently) do that, so
        // if there is an odd number of overbar markers in this net name, we need to append one
        // to close it out before appending the space.

        if( member.Freq( '!' ) % 2 > 0 )
            member << wxT( "!" );

        ret << member << wxS( " " );
    }

    ret.Trim( true );
    ret << wxT( "}" );

    return ret;
}


const ESYMBOL* SCH_IO_EAGLE::getEagleSymbol( const std::unique_ptr<EINSTANCE>& aInstance )
{
    wxCHECK( m_eagleDoc && m_eagleDoc->drawing && m_eagleDoc->drawing->schematic && aInstance,
             nullptr );

    std::unique_ptr<EPART>& epart = m_eagleDoc->drawing->schematic->parts[aInstance->part];

    if( !epart || epart->deviceset.IsEmpty() )
        return nullptr;

    std::unique_ptr<ELIBRARY>& elibrary = m_eagleDoc->drawing->schematic->libraries[epart->library];

    if( !elibrary )
        return nullptr;

    std::unique_ptr<EDEVICE_SET>& edeviceset = elibrary->devicesets[epart->deviceset];

    if( !edeviceset )
        return nullptr;

    std::unique_ptr<EGATE>& egate = edeviceset->gates[aInstance->gate];

    if( !egate )
        return nullptr;

    std::unique_ptr<ESYMBOL>& esymbol = elibrary->symbols[egate->symbol];

    if( esymbol )
        return esymbol.get();

    return nullptr;
}


void SCH_IO_EAGLE::getEagleSymbolFieldAttributes( const std::unique_ptr<EINSTANCE>& aInstance,
                                                  const wxString& aEagleFieldName,
                                                  SCH_FIELD* aField )
{
    wxCHECK( aField && !aEagleFieldName.IsEmpty(), /* void */ );

    const ESYMBOL* esymbol = getEagleSymbol( aInstance );

    if( esymbol )
    {
        for( const std::unique_ptr<ETEXT>& text : esymbol->texts )
        {
            if( text->text == aEagleFieldName )
            {
                aField->SetVisible( true );
                VECTOR2I pos( text->x.ToSchUnits() + aInstance->x.ToSchUnits(),
                              -text->y.ToSchUnits() - aInstance->y.ToSchUnits() );

                bool mirror = text->rot ? text->rot->mirror : false;

                if( aInstance->rot && aInstance->rot->mirror )
                    mirror = !mirror;

                if( mirror )
                    pos.y = -aInstance->y.ToSchUnits() + text->y.ToSchUnits();

                aField->SetPosition( pos );
            }
        }
    }
}
