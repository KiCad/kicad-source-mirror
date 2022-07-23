/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2017 CERN
 * Copyright (C) 2017-2022 Kicad Developers, see AUTHORS.txt for contributors.
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

#include <sch_plugins/eagle/sch_eagle_plugin.h>

#include <locale_io.h>
#include <properties.h>

#include <algorithm>
#include <memory>
#include <wx/filename.h>
#include <wx/tokenzr.h>
#include <wx/wfstream.h>
#include <wx/xml/xml.h>

#include <symbol_library.h>
#include <plugins/eagle/eagle_parser.h>
#include <string_utils.h>
#include <gr_text.h>
#include <lib_shape.h>
#include <lib_id.h>
#include <lib_item.h>
#include <lib_pin.h>
#include <lib_text.h>
#include <project.h>
#include <sch_bus_entry.h>
#include <sch_symbol.h>
#include <project/net_settings.h>
#include <sch_edit_frame.h>
#include <sch_junction.h>
#include <sch_plugins/legacy/sch_legacy_plugin.h>
#include <sch_marker.h>
#include <sch_screen.h>
#include <sch_sheet.h>
#include <sch_sheet_path.h>
#include <sch_label.h>
#include <schematic.h>
#include <symbol_lib_table.h>
#include <wildcards_and_files_ext.h>
#include <progress_reporter.h>


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


/**
 * Provide an easy access to the children of an XML node via their names.
 *
 * @param aCurrentNode is a pointer to a wxXmlNode, whose children will be mapped.
 * @param aName the name of the specific child names to be counted.
 * @return number of children with the give node name.
 */
static int countChildren( wxXmlNode* aCurrentNode, const wxString& aName )
{
    // Map node_name -> node_pointer
    int count = 0;

    // Loop through all children counting them if they match the given name
    aCurrentNode = aCurrentNode->GetChildren();

    while( aCurrentNode )
    {
        if( aCurrentNode->GetName() == aName )
            count++;

        // Get next child
        aCurrentNode = aCurrentNode->GetNext();
    }

    return count;
}


///< Compute a bounding box for all items in a schematic sheet
static EDA_RECT getSheetBbox( SCH_SHEET* aSheet )
{
    EDA_RECT bbox;

    for( SCH_ITEM* item : aSheet->GetScreen()->Items() )
        bbox.Merge( item->GetBoundingBox() );

    return bbox;
}


///< Extract the net name part from a pin name (e.g. return 'GND' for pin named 'GND@2')
static inline wxString extractNetName( const wxString& aPinName )
{
    return aPinName.BeforeFirst( '@' );
}


wxString SCH_EAGLE_PLUGIN::getLibName()
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
        m_libName = LIB_ID::FixIllegalChars( m_libName, true );
    }

    return m_libName;
}


wxFileName SCH_EAGLE_PLUGIN::getLibFileName()
{
    wxFileName fn( m_schematic->Prj().GetProjectPath(), getLibName(), KiCadSymbolLibFileExtension );

    return fn;
}


void SCH_EAGLE_PLUGIN::loadLayerDefs( wxXmlNode* aLayers )
{
    std::vector<ELAYER> eagleLayers;

    // Get the first layer and iterate
    wxXmlNode* layerNode = aLayers->GetChildren();

    while( layerNode )
    {
        ELAYER elayer( layerNode );
        eagleLayers.push_back( elayer );

        layerNode = layerNode->GetNext();
    }

    // match layers based on their names
    for( const ELAYER& elayer : eagleLayers )
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

        if( elayer.name == wxT( "Nets" ) )
        {
            m_layerMap[elayer.number] = LAYER_WIRE;
        }
        else if( elayer.name == wxT( "Info" ) || elayer.name == wxT( "Guide" ) )
        {
            m_layerMap[elayer.number] = LAYER_NOTES;
        }
        else if( elayer.name == wxT( "Busses" ) )
        {
            m_layerMap[elayer.number] = LAYER_BUS;
        }
    }
}


SCH_LAYER_ID SCH_EAGLE_PLUGIN::kiCadLayer( int aEagleLayer )
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
    default:
        wxASSERT_MSG( false, wxString::Format( wxT( "Unhandled orientation (%d degrees)" ), roti ) );
        KI_FALLTHROUGH;

    case 0:
        return SYM_ORIENT_0;

    case 90:
        return SYM_ORIENT_90;

    case 180:
        return SYM_ORIENT_180;

    case 270:
        return SYM_ORIENT_270;
    }

    return SYM_ORIENT_0;
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
        align = -align;
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


SCH_EAGLE_PLUGIN::SCH_EAGLE_PLUGIN() :
    m_progressReporter( nullptr ),
    m_doneCount( 0 ),
    m_lastProgressCount( 0 ),
    m_totalCount( 0 )
{
    m_rootSheet    = nullptr;
    m_currentSheet = nullptr;
    m_schematic    = nullptr;

    m_reporter     = &WXLOG_REPORTER::GetInstance();
}


SCH_EAGLE_PLUGIN::~SCH_EAGLE_PLUGIN()
{
}


const wxString SCH_EAGLE_PLUGIN::GetName() const
{
    return wxT( "EAGLE" );
}


const wxString SCH_EAGLE_PLUGIN::GetFileExtension() const
{
    return wxT( "sch" );
}


const wxString SCH_EAGLE_PLUGIN::GetLibraryFileExtension() const
{
    return wxT( "lbr" );
}


int SCH_EAGLE_PLUGIN::GetModifyHash() const
{
    return 0;
}


void SCH_EAGLE_PLUGIN::checkpoint()
{
    const unsigned PROGRESS_DELTA = 5;

    if( m_progressReporter )
    {
        if( ++m_doneCount > m_lastProgressCount + PROGRESS_DELTA )
        {
            m_progressReporter->SetCurrentProgress( ( (double) m_doneCount )
                                                            / std::max( 1U, m_totalCount ) );

            if( !m_progressReporter->KeepRefreshing() )
                THROW_IO_ERROR( ( "Open canceled by user." ) );

            m_lastProgressCount = m_doneCount;
        }
    }
}


SCH_SHEET* SCH_EAGLE_PLUGIN::Load( const wxString& aFileName, SCHEMATIC* aSchematic,
                                   SCH_SHEET* aAppendToMe, const PROPERTIES* aProperties )
{
    wxASSERT( !aFileName || aSchematic != nullptr );
    LOCALE_IO toggle; // toggles on, then off, the C locale.

    m_filename = aFileName;
    m_schematic = aSchematic;

    if( m_progressReporter )
    {
        m_progressReporter->Report( wxString::Format( _( "Loading %s..." ), aFileName ) );

        if( !m_progressReporter->KeepRefreshing() )
            THROW_IO_ERROR( ( "Open canceled by user." ) );
    }

    // Load the document
    wxXmlDocument xmlDocument;
    wxFFileInputStream stream( m_filename.GetFullPath() );

    if( !stream.IsOk() || !xmlDocument.Load( stream ) )
    {
        THROW_IO_ERROR( wxString::Format( _( "Unable to read file '%s'." ),
                                          m_filename.GetFullPath() ) );
    }

    // Delete on exception, if I own m_rootSheet, according to aAppendToMe
    unique_ptr<SCH_SHEET> deleter( aAppendToMe ? nullptr : m_rootSheet );

    wxFileName newFilename( m_filename );
    newFilename.SetExt( KiCadSchematicFileExtension );

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
    }

    SYMBOL_LIB_TABLE* libTable = m_schematic->Prj().SchSymbolLibTable();

    wxCHECK_MSG( libTable, nullptr, wxT( "Could not load symbol lib table." ) );

    m_pi.set( SCH_IO_MGR::FindPlugin( SCH_IO_MGR::SCH_KICAD ) );
    m_properties = std::make_unique<PROPERTIES>();
    ( *m_properties )[SCH_LEGACY_PLUGIN::PropBuffering] = "";

    /// @note No check is being done here to see if the existing symbol library exists so this
    ///       will overwrite the existing one.
    if( !libTable->HasLibrary( getLibName() ) )
    {
        // Create a new empty symbol library.
        m_pi->CreateSymbolLib( getLibFileName().GetFullPath() );
        wxString libTableUri = wxT( "${KIPRJMOD}/" ) + getLibFileName().GetFullName();

        // Add the new library to the project symbol library table.
        libTable->InsertRow( new SYMBOL_LIB_TABLE_ROW( getLibName(), libTableUri, wxT( "KiCad" ) ) );

        // Save project symbol library table.
        wxFileName fn( m_schematic->Prj().GetProjectPath(),
                SYMBOL_LIB_TABLE::GetSymbolLibTableFileName() );

        // So output formatter goes out of scope and closes the file before reloading.
        {
            FILE_OUTPUTFORMATTER formatter( fn.GetFullPath() );
            libTable->Format( &formatter, 0 );
        }

        // Reload the symbol library table.
        m_schematic->Prj().SetElem( PROJECT::ELEM_SYMBOL_LIB_TABLE, nullptr );
        m_schematic->Prj().SchSymbolLibTable();
    }

    // Retrieve the root as current node
    wxXmlNode* currentNode = xmlDocument.GetRoot();

    // If the attribute is found, store the Eagle version;
    // otherwise, store the dummy "0.0" version.
    m_version = currentNode->GetAttribute( wxT( "version" ), wxT( "0.0" ) );

    // Map all children into a readable dictionary
    NODE_MAP children = MapChildren( currentNode );

    // Load drawing
    loadDrawing( children["drawing"] );

    m_pi->SaveLibrary( getLibFileName().GetFullPath() );

    SCH_SCREENS allSheets( m_rootSheet );
    allSheets.UpdateSymbolLinks(); // Update all symbol library links for all sheets.

    return m_rootSheet;
}


void SCH_EAGLE_PLUGIN::loadDrawing( wxXmlNode* aDrawingNode )
{
    // Map all children into a readable dictionary
    NODE_MAP drawingChildren = MapChildren( aDrawingNode );

    // Board nodes should not appear in .sch files
    // wxXmlNode* board = drawingChildren["board"]

    // wxXmlNode* grid = drawingChildren["grid"]

    auto layers = drawingChildren["layers"];

    if( layers )
        loadLayerDefs( layers );

    // wxXmlNode* library = drawingChildren["library"]

    // wxXmlNode* settings = drawingChildren["settings"]

    // Load schematic
    auto schematic = drawingChildren["schematic"];

    if( schematic )
        loadSchematic( schematic );
}


void SCH_EAGLE_PLUGIN::countNets( wxXmlNode* aSchematicNode )
{
    // Map all children into a readable dictionary
    NODE_MAP schematicChildren = MapChildren( aSchematicNode );

    // Loop through all the sheets
    wxXmlNode* sheetNode = getChildrenNodes( schematicChildren, wxT( "sheets" ) );

    while( sheetNode )
    {
        NODE_MAP sheetChildren = MapChildren( sheetNode );

        // Loop through all nets
        // From the DTD: "Net is an electrical connection in a schematic."
        wxXmlNode* netNode = getChildrenNodes( sheetChildren, wxT( "nets" ) );

        while( netNode )
        {
            wxString netName = netNode->GetAttribute( wxT( "name" ) );

            if( m_netCounts.count( netName ) )
                m_netCounts[netName] = m_netCounts[netName] + 1;
            else
                m_netCounts[netName] = 1;

            // Get next net
            netNode = netNode->GetNext();
        }

        sheetNode = sheetNode->GetNext();
    }
}


void SCH_EAGLE_PLUGIN::loadSchematic( wxXmlNode* aSchematicNode )
{
    // Map all children into a readable dictionary
    NODE_MAP   schematicChildren = MapChildren( aSchematicNode );
    wxXmlNode* partNode          = getChildrenNodes( schematicChildren, wxT( "parts" ) );
    wxXmlNode* libraryNode       = getChildrenNodes( schematicChildren, wxT( "libraries" ) );
    wxXmlNode* sheetNode         = getChildrenNodes( schematicChildren, wxT( "sheets" ) );

    if( !sheetNode )
        return;

    auto count_nodes =
            []( wxXmlNode* aNode ) -> unsigned
            {
                unsigned count = 0;

                while( aNode )
                {
                    count++;
                    aNode = aNode->GetNext();
                }

                return count;
            };

    if( m_progressReporter )
    {
        m_totalCount = 0;
        m_doneCount = 0;

        m_totalCount += count_nodes( partNode );

        while( libraryNode )
        {
            NODE_MAP libraryChildren = MapChildren( libraryNode );
            wxXmlNode* devicesetNode = getChildrenNodes( libraryChildren, wxT( "devicesets" ) );

            while( devicesetNode )
            {
                NODE_MAP deviceSetChildren = MapChildren( devicesetNode );
                wxXmlNode* deviceNode = getChildrenNodes( deviceSetChildren, wxT( "devices" ) );
                wxXmlNode* gateNode = getChildrenNodes( deviceSetChildren, wxT( "gates" ) );

                m_totalCount += count_nodes( deviceNode ) * count_nodes( gateNode );

                devicesetNode = devicesetNode->GetNext();
            }

            libraryNode = libraryNode->GetNext();
        }

        // Rewind
        libraryNode = getChildrenNodes( schematicChildren, wxT( "libraries" ) );

        while( sheetNode )
        {
            NODE_MAP sheetChildren = MapChildren( sheetNode );

            m_totalCount += count_nodes( getChildrenNodes( sheetChildren, wxT( "instances" ) ) );
            m_totalCount += count_nodes( getChildrenNodes( sheetChildren, wxT( "busses" ) ) );
            m_totalCount += count_nodes( getChildrenNodes( sheetChildren, wxT( "nets" ) ) );
            m_totalCount += count_nodes( getChildrenNodes( sheetChildren, wxT( "plain" ) ) );

            sheetNode = sheetNode->GetNext();
        }

        // Rewind
        sheetNode = getChildrenNodes( schematicChildren, wxT( "sheets" ) );
    }

    while( partNode )
    {
        checkpoint();

        std::unique_ptr<EPART> epart = std::make_unique<EPART>( partNode );

        // N.B. Eagle parts are case-insensitive in matching but we keep the display case
        m_partlist[epart->name.Upper()] = std::move( epart );
        partNode                        = partNode->GetNext();
    }

    if( libraryNode )
    {
        while( libraryNode )
        {
            // Read the library name
            wxString libName = libraryNode->GetAttribute( wxT( "name" ) );

            EAGLE_LIBRARY* elib = &m_eagleLibs[libName];
            elib->name          = libName;

            loadLibrary( libraryNode, &m_eagleLibs[libName] );

            libraryNode = libraryNode->GetNext();
        }
        m_pi->SaveLibrary( getLibFileName().GetFullPath() );
    }

    // find all nets and count how many sheets they appear on.
    // local labels will be used for nets found only on that sheet.
    countNets( aSchematicNode );

    // Loop through all the sheets
    int sheet_count = countChildren( sheetNode->GetParent(), wxT( "sheet" ) );

    // If eagle schematic has multiple sheets then create corresponding subsheets on the root sheet
    if( sheet_count > 1 )
    {
        int x, y, i;
        i = 2;
        x = 1;
        y = 1;

        SCH_SHEET_PATH rootsheetpath;
        rootsheetpath.push_back( m_rootSheet );
        m_rootSheet->AddInstance( rootsheetpath );
        m_rootSheet->SetPageNumber( rootsheetpath, wxT( "1" ) );

        while( sheetNode )
        {
            VECTOR2I                   pos    = VECTOR2I( x * Mils2iu( 1000 ), y * Mils2iu( 1000 ) );
            std::unique_ptr<SCH_SHEET> sheet  = std::make_unique<SCH_SHEET>( m_rootSheet, pos );
            SCH_SCREEN*                screen = new SCH_SCREEN( m_schematic );
            wxString                   pageNo = wxString::Format( wxT( "%d" ), i );

            sheet->SetScreen( screen );
            sheet->GetScreen()->SetFileName( sheet->GetFileName() );

            m_currentSheet = sheet.get();
            loadSheet( sheetNode, i );
            m_rootSheet->GetScreen()->Append( sheet.release() );

            SCH_SHEET_PATH sheetpath;
            m_rootSheet->LocatePathOfScreen( m_currentSheet->GetScreen(), &sheetpath );
            m_currentSheet->AddInstance( sheetpath );
            m_currentSheet->SetPageNumber( sheetpath, pageNo );
            m_rootSheet->AddInstance( sheetpath );
            m_rootSheet->SetPageNumber( sheetpath, pageNo );

            sheetNode = sheetNode->GetNext();
            x += 2;

            if( x > 10 ) // start next row
            {
                x = 1;
                y += 2;
            }

            i++;
        }
    }
    else
    {
        while( sheetNode )
        {
            m_currentSheet = m_rootSheet;
            loadSheet( sheetNode, 0 );
            sheetNode = sheetNode->GetNext();

            SCH_SHEET_PATH rootPath;
            rootPath.push_back( m_rootSheet );
            m_rootSheet->AddInstance( rootPath );
            m_rootSheet->SetPageNumber( rootPath, wxT( "1" ) );
        }
    }

    // Handle the missing symbol units that need to be instantiated
    // to create the missing implicit connections

    // Calculate the already placed items bounding box and the page size to determine
    // placement for the new symbols
    wxSize   pageSizeIU = m_rootSheet->GetScreen()->GetPageSettings().GetSizeIU();
    EDA_RECT sheetBbox  = getSheetBbox( m_rootSheet );
    VECTOR2I newCmpPosition( sheetBbox.GetLeft(), sheetBbox.GetBottom() );
    int      maxY = sheetBbox.GetY();

    SCH_SHEET_PATH sheetpath;
    m_rootSheet->LocatePathOfScreen( m_rootSheet->GetScreen(), &sheetpath );

    for( auto& cmp : m_missingCmps )
    {
        const SCH_SYMBOL* origSymbol = cmp.second.cmp;

        for( auto unitEntry : cmp.second.units )
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
            EDA_RECT cmpBbox = symbol->GetBoundingBox();
            int      posY    = newCmpPosition.y + cmpBbox.GetHeight();
            symbol->SetPosition( VECTOR2I( newCmpPosition.x, posY ) );
            newCmpPosition.x += cmpBbox.GetWidth();
            maxY = std::max( maxY, posY );

            if( newCmpPosition.x >= pageSizeIU.GetWidth() )            // reached the page boundary?
                newCmpPosition = VECTOR2I( sheetBbox.GetLeft(), maxY ); // then start a new row

            // Add the global net labels to recreate the implicit connections
            addImplicitConnections( symbol.get(), m_rootSheet->GetScreen(), false );
            m_rootSheet->GetScreen()->Append( symbol.release() );
        }
    }

    m_missingCmps.clear();
}


void SCH_EAGLE_PLUGIN::loadSheet( wxXmlNode* aSheetNode, int aSheetIndex )
{
    // Map all children into a readable dictionary
    NODE_MAP sheetChildren = MapChildren( aSheetNode );

    // Get description node
    wxXmlNode* descriptionNode = getChildrenNodes( sheetChildren, wxT( "description" ) );

    wxString    des;
    std::string filename;
    SCH_FIELD&  sheetNameField = m_currentSheet->GetFields()[SHEETNAME];
    SCH_FIELD&  filenameField = m_currentSheet->GetFields()[SHEETFILENAME];

    if( descriptionNode )
    {
        des = descriptionNode->GetContent();
        des.Replace( wxT( "\n" ), wxT( "_" ), true );
        sheetNameField.SetText( des );
        filename = des.ToStdString();
    }
    else
    {
        filename = wxString::Format( wxT( "%s_%d" ), m_filename.GetName(), aSheetIndex );
        sheetNameField.SetText( filename );
    }

    ReplaceIllegalFileNameChars( &filename );
    replace( filename.begin(), filename.end(), ' ', '_' );

    wxFileName fn( m_filename );
    fn.SetName( filename );
    fn.SetExt( KiCadSchematicFileExtension );

    filenameField.SetText( fn.GetFullName() );
    m_currentSheet->GetScreen()->SetFileName( fn.GetFullPath() );
    m_currentSheet->AutoplaceFields( m_currentSheet->GetScreen(), true );


    // Loop through all of the symbol instances.
    wxXmlNode* instanceNode = getChildrenNodes( sheetChildren, wxT( "instances" ) );

    while( instanceNode )
    {
        checkpoint();

        loadInstance( instanceNode );
        instanceNode = instanceNode->GetNext();
    }

    // Loop through all buses
    // From the DTD: "Buses receive names which determine which signals they include.
    // A bus is a drawing object. It does not create any electrical connections.
    // These are always created by means of the nets and their names."
    wxXmlNode* busNode = getChildrenNodes( sheetChildren, wxT( "busses" ) );

    while( busNode )
    {
        checkpoint();

        // Get the bus name
        wxString busName = translateEagleBusName( busNode->GetAttribute( wxT( "name" ) ) );

        // Load segments of this bus
        loadSegments( busNode, busName, wxString() );

        // Get next bus
        busNode = busNode->GetNext();
    }

    // Loop through all nets
    // From the DTD: "Net is an electrical connection in a schematic."
    wxXmlNode* netNode = getChildrenNodes( sheetChildren, wxT( "nets" ) );

    while( netNode )
    {
        checkpoint();

        // Get the net name and class
        wxString netName  = netNode->GetAttribute( wxT( "name" ) );
        wxString netClass = netNode->GetAttribute( wxT( "class" ) );

        // Load segments of this net
        loadSegments( netNode, netName, netClass );

        // Get next net
        netNode = netNode->GetNext();
    }

    adjustNetLabels(); // needs to be called before addBusEntries()
    addBusEntries();

    /*  moduleinst is a design block definition and is an EagleCad 8 feature,
     *
     *  // Loop through all moduleinsts
     *  wxXmlNode* moduleinstNode = getChildrenNodes( sheetChildren, "moduleinsts" );
     *
     *  while( moduleinstNode )
     *  {
     *   loadModuleinst( moduleinstNode );
     *   moduleinstNode = moduleinstNode->GetNext();
     *  }
     */

    wxXmlNode* plainNode = getChildrenNodes( sheetChildren, wxT( "plain" ) );

    while( plainNode )
    {
        checkpoint();

        wxString nodeName = plainNode->GetName();

        if( nodeName == wxT( "text" ) )
        {
            m_currentSheet->GetScreen()->Append( loadPlainText( plainNode ) );
        }
        else if( nodeName == wxT( "wire" ) )
        {
            m_currentSheet->GetScreen()->Append( loadWire( plainNode ) );
        }
        else if( nodeName == wxT( "frame" ) )
        {
            std::vector<SCH_LINE*> lines;

            loadFrame( plainNode, lines );

            for( SCH_LINE* line : lines )
                m_currentSheet->GetScreen()->Append( line );
        }

        plainNode = plainNode->GetNext();
    }

    // Calculate the new sheet size.
    EDA_RECT sheetBoundingBox = getSheetBbox( m_currentSheet );
    VECTOR2I targetSheetSize = sheetBoundingBox.GetSize();
    targetSheetSize += VECTOR2I( Mils2iu( 1500 ), Mils2iu( 1500 ) );

    // Get current Eeschema sheet size.
    wxSize    pageSizeIU = m_currentSheet->GetScreen()->GetPageSettings().GetSizeIU();
    PAGE_INFO pageInfo   = m_currentSheet->GetScreen()->GetPageSettings();

    // Increase if necessary
    if( pageSizeIU.x < targetSheetSize.x )
        pageInfo.SetWidthMils( Iu2Mils( targetSheetSize.x ) );

    if( pageSizeIU.y < targetSheetSize.y )
        pageInfo.SetHeightMils( Iu2Mils( targetSheetSize.y ) );

    // Set the new sheet size.
    m_currentSheet->GetScreen()->SetPageSettings( pageInfo );

    pageSizeIU = m_currentSheet->GetScreen()->GetPageSettings().GetSizeIU();
    VECTOR2I sheetcentre( pageSizeIU.x / 2, pageSizeIU.y / 2 );
    VECTOR2I itemsCentre = sheetBoundingBox.Centre();

    // round the translation to nearest 100mil to place it on the grid.
    VECTOR2I translation = sheetcentre - itemsCentre;
    translation.x       = translation.x - translation.x % Mils2iu( 100 );
    translation.y       = translation.y - translation.y % Mils2iu( 100 );

    // Add global net labels for the named power input pins in this sheet
    for( SCH_ITEM* item : m_currentSheet->GetScreen()->Items().OfType( SCH_SYMBOL_T ) )
    {
        SCH_SYMBOL* symbol = static_cast<SCH_SYMBOL*>( item );
        addImplicitConnections( symbol, m_currentSheet->GetScreen(), true );
    }

    m_connPoints.clear();

    // Translate the items.
    std::vector<SCH_ITEM*> allItems;

    std::copy( m_currentSheet->GetScreen()->Items().begin(),
            m_currentSheet->GetScreen()->Items().end(), std::back_inserter( allItems ) );

    for( SCH_ITEM* item : allItems )
    {
        item->SetPosition( item->GetPosition() + translation );
        item->ClearFlags();
        m_currentSheet->GetScreen()->Update( item );

    }
}


void SCH_EAGLE_PLUGIN::loadFrame( wxXmlNode* aFrameNode, std::vector<SCH_LINE*>& aLines )
{
    EFRAME eframe( aFrameNode );

    VECTOR2I corner1( eframe.x1.ToSchUnits(), -eframe.y1.ToSchUnits() );
    VECTOR2I corner3( eframe.x2.ToSchUnits(), -eframe.y2.ToSchUnits() );
    VECTOR2I corner2( corner3.x, corner1.y );
    VECTOR2I corner4( corner1.x, corner3.y );

    SCH_LINE* line = new SCH_LINE();
    line->SetLineStyle( PLOT_DASH_TYPE::SOLID );
    line->SetStartPoint( corner1 );
    line->SetEndPoint( corner2 );
    aLines.push_back( line );

    line = new SCH_LINE();
    line->SetLineStyle( PLOT_DASH_TYPE::SOLID );
    line->SetStartPoint( corner2 );
    line->SetEndPoint( corner3 );
    aLines.push_back( line );

    line = new SCH_LINE();
    line->SetLineStyle( PLOT_DASH_TYPE::SOLID );
    line->SetStartPoint( corner3 );
    line->SetEndPoint( corner4 );
    aLines.push_back( line );

    line = new SCH_LINE();
    line->SetLineStyle( PLOT_DASH_TYPE::SOLID );
    line->SetStartPoint( corner4 );
    line->SetEndPoint( corner1 );
    aLines.push_back( line );
}


void SCH_EAGLE_PLUGIN::loadSegments( wxXmlNode* aSegmentsNode, const wxString& netName,
                                     const wxString& aNetClass )
{
    // Loop through all segments
    wxXmlNode*  currentSegment = aSegmentsNode->GetChildren();
    SCH_SCREEN* screen         = m_currentSheet->GetScreen();

    int segmentCount = countChildren( aSegmentsNode, wxT( "segment" ) );

    // wxCHECK( screen, [>void<] );
    while( currentSegment )
    {
        bool      labelled = false; // has a label been added to this continuously connected segment
        NODE_MAP  segmentChildren = MapChildren( currentSegment );
        SCH_LINE* firstWire       = nullptr;
        m_segments.emplace_back();
        SEG_DESC& segDesc = m_segments.back();

        // Loop through all segment children
        wxXmlNode* segmentAttribute = currentSegment->GetChildren();

        while( segmentAttribute )
        {
            if( segmentAttribute->GetName() == wxT( "wire" ) )
            {
                SCH_LINE* wire = loadWire( segmentAttribute );

                if( !firstWire )
                    firstWire = wire;

                // Test for intersections with other wires
                SEG thisWire( wire->GetStartPoint(), wire->GetEndPoint() );

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

            segmentAttribute = segmentAttribute->GetNext();
        }

        segmentAttribute = currentSegment->GetChildren();

        while( segmentAttribute )
        {
            wxString nodeName = segmentAttribute->GetName();

            if( nodeName == wxT( "junction" ) )
            {
                screen->Append( loadJunction( segmentAttribute ) );
            }
            else if( nodeName == wxT( "label" ) )
            {
                SCH_TEXT* label = loadLabel( segmentAttribute, netName );
                screen->Append( label );
                wxASSERT( segDesc.labels.empty()
                          || segDesc.labels.front()->GetText() == label->GetText() );
                segDesc.labels.push_back( label );
                labelled = true;
            }
            else if( nodeName == wxT( "pinref" ) )
            {
                segmentAttribute->GetAttribute( wxT( "gate" ) ); // REQUIRED
                wxString part = segmentAttribute->GetAttribute( wxT( "part" ) ); // REQUIRED
                wxString pin = segmentAttribute->GetAttribute( wxT( "pin" ) );  // REQUIRED

                auto powerPort = m_powerPorts.find( wxT( "#" ) + part );

                if( powerPort != m_powerPorts.end()
                        && powerPort->second == EscapeString( pin, CTX_NETNAME ) )
                {
                    labelled = true;
                }
            }
            else if( nodeName == wxT( "wire" ) )
            {
                // already handled;
            }
            else // DEFAULT
            {
                // THROW_IO_ERROR( wxString::Format( _( "XML node '%s' unknown" ), nodeName ) );
            }

            // Get next segment attribute
            segmentAttribute = segmentAttribute->GetNext();
        }

        // Add a small label to the net segment if it hasn't been labeled already or is not
        // connect to a power symbol with a pin on the same net.  This preserves the named net
        // feature of Eagle schematics.
        if( !labelled && firstWire )
        {
            std::unique_ptr<SCH_TEXT> label;

            // Add a global label if the net appears on more than one Eagle sheet
            if( m_netCounts[netName.ToStdString()] > 1 )
                label.reset( new SCH_GLOBALLABEL );
            else if( segmentCount > 1 )
                label.reset( new SCH_LABEL );

            if( label )
            {
                label->SetPosition( firstWire->GetStartPoint() );
                label->SetText( escapeName( netName ) );
                label->SetTextSize( wxSize( Mils2iu( 40 ), Mils2iu( 40 ) ) );

                if( firstWire->GetEndPoint().x > firstWire->GetStartPoint().x )
                    label->SetTextSpinStyle( TEXT_SPIN_STYLE::LEFT );
                else
                    label->SetTextSpinStyle( TEXT_SPIN_STYLE::RIGHT );

                screen->Append( label.release() );
            }
        }

        currentSegment = currentSegment->GetNext();
    }
}


SCH_LINE* SCH_EAGLE_PLUGIN::loadWire( wxXmlNode* aWireNode )
{
    std::unique_ptr<SCH_LINE> wire = std::make_unique<SCH_LINE>();

    EWIRE ewire = EWIRE( aWireNode );

    wire->SetLayer( kiCadLayer( ewire.layer ) );

    VECTOR2I begin, end;

    begin.x = ewire.x1.ToSchUnits();
    begin.y = -ewire.y1.ToSchUnits();
    end.x   = ewire.x2.ToSchUnits();
    end.y   = -ewire.y2.ToSchUnits();

    wire->SetStartPoint( begin );
    wire->SetEndPoint( end );

    m_connPoints[begin].emplace( wire.get() );
    m_connPoints[end].emplace( wire.get() );

    return wire.release();
}


SCH_JUNCTION* SCH_EAGLE_PLUGIN::loadJunction( wxXmlNode* aJunction )
{
    std::unique_ptr<SCH_JUNCTION> junction = std::make_unique<SCH_JUNCTION>();

    EJUNCTION ejunction = EJUNCTION( aJunction );
    VECTOR2I  pos( ejunction.x.ToSchUnits(), -ejunction.y.ToSchUnits() );

    junction->SetPosition( pos );

    return junction.release();
}


SCH_TEXT* SCH_EAGLE_PLUGIN::loadLabel( wxXmlNode* aLabelNode, const wxString& aNetName )
{
    ELABEL  elabel = ELABEL( aLabelNode, aNetName );
    VECTOR2I elabelpos( elabel.x.ToSchUnits(), -elabel.y.ToSchUnits() );

    // Determine if the label is local or global depending on
    // the number of sheets the net appears in
    bool                      global = m_netCounts[aNetName] > 1;
    std::unique_ptr<SCH_TEXT> label;

    wxSize textSize = wxSize( KiROUND( elabel.size.ToSchUnits() * 0.7 ),
                              KiROUND( elabel.size.ToSchUnits() * 0.7 ) );

    if( global )
        label = std::make_unique<SCH_GLOBALLABEL>();
    else
        label = std::make_unique<SCH_LABEL>();

    label->SetPosition( elabelpos );
    label->SetText( escapeName( elabel.netname ) );
    label->SetTextSize( textSize );
    label->SetTextSpinStyle( TEXT_SPIN_STYLE::RIGHT );

    if( elabel.rot )
    {
        for( int i = 0; i < KiROUND( elabel.rot->degrees / 90 ) %4; ++i )
            label->Rotate90( false );

        if( elabel.rot->mirror )
            label->MirrorSpinStyle( false );
    }

    return label.release();
}


std::pair<VECTOR2I, const SEG*>
SCH_EAGLE_PLUGIN::findNearestLinePoint( const VECTOR2I&         aPoint,
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


void SCH_EAGLE_PLUGIN::loadInstance( wxXmlNode* aInstanceNode )
{
    EINSTANCE   einstance = EINSTANCE( aInstanceNode );
    SCH_SCREEN* screen = m_currentSheet->GetScreen();

    // Find the part in the list for the sheet.
    // Assign the symbol its value from the part entry
    // Calculate the unit number from the gate entry of the instance
    // Assign the LIB_ID from device set and device names

    auto part_it = m_partlist.find( einstance.part.Upper() );

    if( part_it == m_partlist.end() )
    {
        m_reporter->Report( wxString::Format( _( "Error parsing Eagle file. Could not find '%s' "
                                                 "instance but it is referenced in the schematic." ),
                                              einstance.part ),
                            RPT_SEVERITY_ERROR );

        return;
    }

    EPART* epart = part_it->second.get();

    wxString libraryname = epart->library;
    wxString gatename    = epart->deviceset + epart->device + einstance.gate;
    wxString symbolname  = wxString( epart->deviceset + epart->device );
    symbolname.Replace( wxT( "*" ), wxEmptyString );
    wxString kisymbolname = EscapeString( symbolname, CTX_LIBID );

    int unit = m_eagleLibs[libraryname].GateUnit[gatename];

    wxString       package;
    EAGLE_LIBRARY* elib = &m_eagleLibs[libraryname];

    auto p = elib->package.find( kisymbolname );

    if( p != elib->package.end() )
        package = p->second;

    LIB_SYMBOL* part = m_pi->LoadSymbol( getLibFileName().GetFullPath(), kisymbolname,
                                         m_properties.get() );

    if( !part )
    {
        m_reporter->Report( wxString::Format( _( "Could not find '%s' in the imported library." ),
                                              UnescapeString( kisymbolname ) ),
                            RPT_SEVERITY_ERROR );
        return;
    }

    LIB_ID                      libId( getLibName(), kisymbolname );
    std::unique_ptr<SCH_SYMBOL> symbol = std::make_unique<SCH_SYMBOL>();
    symbol->SetLibId( libId );
    symbol->SetUnit( unit );
    symbol->SetPosition( VECTOR2I( einstance.x.ToSchUnits(), -einstance.y.ToSchUnits() ) );
    // assume that footprint library is identical to project name
    wxString packageString = m_schematic->Prj().GetProjectName() + wxT( ":" ) + package;
    symbol->GetField( FOOTPRINT_FIELD )->SetText( packageString );

    if( einstance.rot )
    {
        symbol->SetOrientation( kiCadComponentRotation( einstance.rot->degrees ) );

        if( einstance.rot->mirror )
            symbol->MirrorHorizontally( einstance.x.ToSchUnits() );
    }

    std::vector<LIB_FIELD*> partFields;
    part->GetFields( partFields );

    for( const LIB_FIELD* field : partFields )
    {
        symbol->GetFieldById( field->GetId() )->ImportValues( *field );
        symbol->GetFieldById( field->GetId() )->SetTextPos( (VECTOR2I)symbol->GetPosition()
                                                            + field->GetTextPos() );
    }

    // If there is no footprint assigned, then prepend the reference value
    // with a hash character to mute netlist updater complaints
    wxString reference = package.IsEmpty() ? '#' + einstance.part : einstance.part;

    // reference must end with a number but EAGLE does not enforce this
    if( reference.find_last_not_of( wxT( "0123456789" ) ) == (reference.Length()-1) )
        reference.Append( wxT( "0" ) );

    // EAGLE allows references to be single digits.  This breaks KiCad netlisting, which requires
    // parts to have non-digit + digit annotation.  If the reference begins with a number,
    // we prepend 'UNK' (unknown) for the symbol designator
    if( reference.find_first_not_of( wxT( "0123456789" ) ) != 0 )
        reference.Prepend( wxT( "UNK" ) );

    // EAGLE allows designator to start with # but that is used in KiCad
    // for symbols which do not have a footprint
    if( einstance.part.find_first_not_of( wxT( "#" ) ) != 0 )
        reference.Prepend( wxT( "UNK" ) );

    SCH_SHEET_PATH sheetpath;
    m_rootSheet->LocatePathOfScreen( screen, &sheetpath );
    wxString current_sheetpath = sheetpath.PathAsString() + symbol->m_Uuid.AsString();

    symbol->GetField( REFERENCE_FIELD )->SetText( reference );
    symbol->AddHierarchicalReference( current_sheetpath, reference, unit );

    if( epart->value )
        symbol->GetField( VALUE_FIELD )->SetText( *epart->value );
    else
        symbol->GetField( VALUE_FIELD )->SetText( kisymbolname );

    // Set the visibility of fields.
    symbol->GetField( REFERENCE_FIELD )->SetVisible(
            part->GetFieldById( REFERENCE_FIELD )->IsVisible() );
    symbol->GetField( VALUE_FIELD )->SetVisible( part->GetFieldById( VALUE_FIELD )->IsVisible() );

    for( const auto& a : epart->attribute )
    {
        SCH_FIELD* field = symbol->AddField( *symbol->GetField( VALUE_FIELD ) );
        field->SetName( a.first );
        field->SetText( a.second );
        field->SetVisible( false );
    }

    for( const auto& a : epart->variant )
    {
        SCH_FIELD* field = symbol->AddField( *symbol->GetField( VALUE_FIELD ) );
        field->SetName( wxT( "VARIANT_" ) + a.first );
        field->SetText( a.second );
        field->SetVisible( false );
    }

    bool valueAttributeFound = false;
    bool nameAttributeFound  = false;

    wxXmlNode* attributeNode = aInstanceNode->GetChildren();

    // Parse attributes for the instance
    while( attributeNode )
    {
        if( attributeNode->GetName() == wxT( "attribute" ) )
        {
            EATTR      attr  = EATTR( attributeNode );
            SCH_FIELD* field = nullptr;

            if( attr.name.Lower() == wxT( "name" ) )
            {
                field              = symbol->GetField( REFERENCE_FIELD );
                nameAttributeFound = true;
            }
            else if( attr.name.Lower() == wxT( "value" ) )
            {
                field               = symbol->GetField( VALUE_FIELD );
                valueAttributeFound = true;
            }
            else
            {
                field = symbol->FindField( attr.name );

                if( field )
                    field->SetVisible( false );
            }

            if( field )
            {

                field->SetPosition( VECTOR2I( attr.x->ToSchUnits(), -attr.y->ToSchUnits() ) );
                int  align      = attr.align ? *attr.align : ETEXT::BOTTOM_LEFT;
                int  absdegrees = attr.rot ? attr.rot->degrees : 0;
                bool mirror     = attr.rot ? attr.rot->mirror : false;

                if( einstance.rot && einstance.rot->mirror )
                    mirror = !mirror;

                bool spin = attr.rot ? attr.rot->spin : false;

                if( attr.display == EATTR::Off || attr.display == EATTR::NAME )
                    field->SetVisible( false );

                int rotation   = einstance.rot ? einstance.rot->degrees : 0;
                int reldegrees = ( absdegrees - rotation + 360.0 );
                reldegrees %= 360;

                eagleToKicadAlignment( (EDA_TEXT*) field, align, reldegrees, mirror, spin,
                                       absdegrees );
            }
        }
        else if( attributeNode->GetName() == wxT( "variant" ) )
        {
            wxString variant, value;

            if( attributeNode->GetAttribute( wxT( "name" ), &variant )
                    && attributeNode->GetAttribute( wxT( "value" ), &value ) )
            {
                SCH_FIELD* field = symbol->AddField( *symbol->GetField( VALUE_FIELD ) );
                field->SetName( wxT( "VARIANT_" ) + variant );
                field->SetText( value );
                field->SetVisible( false );
            }
        }

        attributeNode = attributeNode->GetNext();
    }

    if( einstance.smashed && einstance.smashed.Get() )
    {
        if( !valueAttributeFound )
            symbol->GetField( VALUE_FIELD )->SetVisible( false );

        if( !nameAttributeFound )
            symbol->GetField( REFERENCE_FIELD )->SetVisible( false );
    }

    // Save the pin positions
    SYMBOL_LIB_TABLE& schLibTable = *m_schematic->Prj().SchSymbolLibTable();
    LIB_SYMBOL* libSymbol = schLibTable.LoadSymbol( symbol->GetLibId() );

    wxCHECK( libSymbol, /*void*/ );

    symbol->SetLibSymbol( new LIB_SYMBOL( *libSymbol ) );

    std::vector<LIB_PIN*> pins;
    symbol->GetLibPins( pins );

    for( const LIB_PIN* pin : pins )
        m_connPoints[symbol->GetPinPhysicalPosition( pin )].emplace( pin );

    if( part->IsPower() )
        m_powerPorts[ reference ] = symbol->GetField( VALUE_FIELD )->GetText();

    symbol->ClearFlags();

    screen->Append( symbol.release() );
}


EAGLE_LIBRARY* SCH_EAGLE_PLUGIN::loadLibrary( wxXmlNode* aLibraryNode,
                                              EAGLE_LIBRARY* aEagleLibrary )
{
    NODE_MAP libraryChildren = MapChildren( aLibraryNode );

    // Loop through the symbols and load each of them
    wxXmlNode* symbolNode = getChildrenNodes( libraryChildren, wxT( "symbols" ) );

    while( symbolNode )
    {
        wxString symbolName                    = symbolNode->GetAttribute( wxT( "name" ) );
        aEagleLibrary->SymbolNodes[symbolName] = symbolNode;
        symbolNode                             = symbolNode->GetNext();
    }

    // Loop through the device sets and load each of them
    wxXmlNode* devicesetNode = getChildrenNodes( libraryChildren, wxT( "devicesets" ) );

    while( devicesetNode )
    {
        // Get Device set information
        EDEVICE_SET edeviceset = EDEVICE_SET( devicesetNode );

        wxString prefix = edeviceset.prefix ? edeviceset.prefix.Get() : wxT( "" );

        NODE_MAP   deviceSetChildren = MapChildren( devicesetNode );
        wxXmlNode* deviceNode        = getChildrenNodes( deviceSetChildren, wxT( "devices" ) );

        // For each device in the device set:
        while( deviceNode )
        {
            // Get device information
            EDEVICE edevice = EDEVICE( deviceNode );

            // Create symbol name from deviceset and device names.
            wxString symbolName = edeviceset.name + edevice.name;
            symbolName.Replace( wxT( "*" ), wxEmptyString );
            wxASSERT( !symbolName.IsEmpty() );
            symbolName = EscapeString( symbolName, CTX_LIBID );

            if( edevice.package )
                aEagleLibrary->package[symbolName] = edevice.package.Get();

            // Create KiCad symbol.
            std::unique_ptr<LIB_SYMBOL> libSymbol = std::make_unique<LIB_SYMBOL>( symbolName );

            // Process each gate in the deviceset for this device.
            wxXmlNode* gateNode    = getChildrenNodes( deviceSetChildren, wxT( "gates" ) );
            int        gates_count = countChildren( deviceSetChildren["gates"], wxT( "gate" ) );
            libSymbol->SetUnitCount( gates_count );
            libSymbol->LockUnits( true );

            LIB_FIELD* reference = libSymbol->GetFieldById( REFERENCE_FIELD );

            if( prefix.length() == 0 )
            {
                reference->SetVisible( false );
            }
            else
            {
                // If there is no footprint assigned, then prepend the reference value
                // with a hash character to mute netlist updater complaints
                reference->SetText( edevice.package ? prefix : '#' + prefix );
            }

            int  gateindex = 1;
            bool ispower   = false;

            while( gateNode )
            {
                checkpoint();

                EGATE egate = EGATE( gateNode );

                aEagleLibrary->GateUnit[edeviceset.name + edevice.name + egate.name] = gateindex;
                ispower = loadSymbol( aEagleLibrary->SymbolNodes[egate.symbol], libSymbol, &edevice,
                                      gateindex, egate.name );

                gateindex++;
                gateNode = gateNode->GetNext();
            } // gateNode

            libSymbol->SetUnitCount( gates_count );

            if( gates_count == 1 && ispower )
                libSymbol->SetPower();

            // assume that footprint library is identical to project name
            wxString packageString = m_schematic->Prj().GetProjectName() + wxT( ":" ) + aEagleLibrary->package[symbolName];
            libSymbol->GetFootprintField().SetText( packageString );

            wxString name = libSymbol->GetName();
            libSymbol->SetName( name );
            m_pi->SaveSymbol( getLibFileName().GetFullPath(), new LIB_SYMBOL( *libSymbol.get() ),
                              m_properties.get() );
            aEagleLibrary->KiCadSymbols.insert( name, libSymbol.release() );

            deviceNode = deviceNode->GetNext();
        } // devicenode

        devicesetNode = devicesetNode->GetNext();
    } // devicesetNode

    return aEagleLibrary;
}


bool SCH_EAGLE_PLUGIN::loadSymbol( wxXmlNode* aSymbolNode, std::unique_ptr<LIB_SYMBOL>& aSymbol,
                                   EDEVICE* aDevice, int aGateNumber, const wxString& aGateName )
{
    wxString               symbolName = aSymbolNode->GetAttribute( wxT( "name" ) );
    std::vector<LIB_ITEM*> items;

    wxXmlNode* currentNode = aSymbolNode->GetChildren();

    bool foundName  = false;
    bool foundValue = false;
    bool ispower    = false;
    int  pincount   = 0;

    while( currentNode )
    {
        wxString nodeName = currentNode->GetName();

        if( nodeName == wxT( "circle" ) )
        {
            aSymbol->AddDrawItem( loadSymbolCircle( aSymbol, currentNode, aGateNumber ) );
        }
        else if( nodeName == wxT( "pin" ) )
        {
            EPIN                     ePin = EPIN( currentNode );
            std::unique_ptr<LIB_PIN> pin( loadPin( aSymbol, currentNode, &ePin, aGateNumber ) );
            pincount++;

            pin->SetType( ELECTRICAL_PINTYPE::PT_BIDI );

            if( ePin.direction )
            {
                for( const auto& pinDir : pinDirectionsMap )
                {
                    if( ePin.direction->Lower() == pinDir.first )
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
                for( const ECONNECT& connect : aDevice->connects )
                {
                    if( connect.gate == aGateName && pin->GetName() == connect.pin )
                    {
                        wxArrayString pads = wxSplit( wxString( connect.pad ), ' ' );

                        pin->SetUnit( aGateNumber );
                        pin->SetName( escapeName( pin->GetName() ) );

                        if( pads.GetCount() > 1 )
                        {
                            pin->SetNumberTextSize( 0 );
                        }

                        for( unsigned i = 0; i < pads.GetCount(); i++ )
                        {
                            LIB_PIN* apin = new LIB_PIN( *pin );

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
        else if( nodeName == wxT( "polygon" ) )
        {
            aSymbol->AddDrawItem( loadSymbolPolyLine( aSymbol, currentNode, aGateNumber ) );
        }
        else if( nodeName == wxT( "rectangle" ) )
        {
            aSymbol->AddDrawItem( loadSymbolRectangle( aSymbol, currentNode, aGateNumber ) );
        }
        else if( nodeName == wxT( "text" ) )
        {
            std::unique_ptr<LIB_TEXT> libtext( loadSymbolText( aSymbol, currentNode,
                                                               aGateNumber ) );

            if( libtext->GetText().Upper() == wxT( ">NAME" ) )
            {
                LIB_FIELD* field = aSymbol->GetFieldById( REFERENCE_FIELD );
                loadFieldAttributes( field, libtext.get() );
                foundName = true;
            }
            else if( libtext->GetText().Upper() == wxT( ">VALUE" ) )
            {
                LIB_FIELD* field = aSymbol->GetFieldById( VALUE_FIELD );
                loadFieldAttributes( field, libtext.get() );
                foundValue = true;
            }
            else
            {
                aSymbol->AddDrawItem( libtext.release() );
            }
        }
        else if( nodeName == wxT( "wire" ) )
        {
            aSymbol->AddDrawItem( loadSymbolWire( aSymbol, currentNode, aGateNumber ) );
        }
        else if( nodeName == wxT( "frame" ) )
        {
            std::vector<LIB_ITEM*> frameItems;

            loadFrame( currentNode, frameItems );

            for( LIB_ITEM* item : frameItems )
            {
                item->SetParent( aSymbol.get() );
                item->SetUnit( aGateNumber );
                aSymbol->AddDrawItem( item );
            }
        }

        /*
         *  else if( nodeName == "description" )
         *  {
         *  }
         *  else if( nodeName == "dimension" )
         *  {
         *  }
         */

        currentNode = currentNode->GetNext();
    }

    if( foundName == false )
        aSymbol->GetFieldById( REFERENCE_FIELD )->SetVisible( false );

    if( foundValue == false )
        aSymbol->GetFieldById( VALUE_FIELD )->SetVisible( false );

    return pincount == 1 ? ispower : false;
}


LIB_SHAPE* SCH_EAGLE_PLUGIN::loadSymbolCircle( std::unique_ptr<LIB_SYMBOL>& aSymbol,
                                               wxXmlNode* aCircleNode, int aGateNumber )
{
    // Parse the circle properties
    ECIRCLE    c( aCircleNode );
    LIB_SHAPE* circle = new LIB_SHAPE( aSymbol.get(), SHAPE_T::CIRCLE );
    VECTOR2I   center( c.x.ToSchUnits(), c.y.ToSchUnits() );

    circle->SetPosition( center );
    circle->SetEnd( VECTOR2I( center.x + c.radius.ToSchUnits(), center.y ) );
    circle->SetStroke( STROKE_PARAMS( c.width.ToSchUnits(), PLOT_DASH_TYPE::SOLID ) );
    circle->SetUnit( aGateNumber );

    return circle;
}


LIB_SHAPE* SCH_EAGLE_PLUGIN::loadSymbolRectangle( std::unique_ptr<LIB_SYMBOL>& aSymbol,
                                                  wxXmlNode* aRectNode, int aGateNumber )
{
    ERECT      rect( aRectNode );
    LIB_SHAPE* rectangle = new LIB_SHAPE( aSymbol.get(), SHAPE_T::RECT );

    rectangle->SetPosition( VECTOR2I( rect.x1.ToSchUnits(), rect.y1.ToSchUnits() ) );
    rectangle->SetEnd( VECTOR2I( rect.x2.ToSchUnits(), rect.y2.ToSchUnits() ) );

    if( rect.rot )
    {
        VECTOR2I pos( rectangle->GetPosition() );
        VECTOR2I end( rectangle->GetEnd() );
        VECTOR2I center( rectangle->GetCenter() );

        RotatePoint( pos, center, EDA_ANGLE( rect.rot->degrees, DEGREES_T ) );
        RotatePoint( end,  center, EDA_ANGLE( rect.rot->degrees, DEGREES_T ) );

        rectangle->SetPosition( pos );
        rectangle->SetEnd( end );
    }

    rectangle->SetUnit( aGateNumber );

    // Eagle rectangles are filled by definition.
    rectangle->SetFillMode( FILL_T::FILLED_SHAPE );

    return rectangle;
}


LIB_ITEM* SCH_EAGLE_PLUGIN::loadSymbolWire( std::unique_ptr<LIB_SYMBOL>& aSymbol,
                                            wxXmlNode* aWireNode, int aGateNumber )
{
    EWIRE ewire = EWIRE( aWireNode );

    VECTOR2I begin, end;

    begin.x = ewire.x1.ToSchUnits();
    begin.y = ewire.y1.ToSchUnits();
    end.x   = ewire.x2.ToSchUnits();
    end.y   = ewire.y2.ToSchUnits();

    if( begin == end )
        return nullptr;

    // if the wire is an arc
    if( ewire.curve )
    {
        LIB_SHAPE* arc = new LIB_SHAPE( aSymbol.get(), SHAPE_T::ARC );
        VECTOR2I   center = ConvertArcCenter( begin, end, *ewire.curve * -1 );
        double     radius = sqrt( abs( ( ( center.x - begin.x ) * ( center.x - begin.x ) )
                                     + ( ( center.y - begin.y ) * ( center.y - begin.y ) ) ) )
                            * 2;

        // this emulates the filled semicircles created by a thick arc with flat ends caps.
        if( ewire.width.ToSchUnits() * 2 > radius )
        {
            VECTOR2I centerStartVector = begin - center;
            VECTOR2I centerEndVector = end - center;

            centerStartVector.x = centerStartVector.x * ewire.width.ToSchUnits() * 2 / radius;
            centerStartVector.y = centerStartVector.y * ewire.width.ToSchUnits() * 2 / radius;

            centerEndVector.x = centerEndVector.x * ewire.width.ToSchUnits() * 2 / radius;
            centerEndVector.y = centerEndVector.y * ewire.width.ToSchUnits() * 2 / radius;

            begin = center + centerStartVector;
            end   = center + centerEndVector;

            radius = sqrt( abs( ( ( center.x - begin.x ) * ( center.x - begin.x ) )
                                + ( ( center.y - begin.y ) * ( center.y - begin.y ) ) ) )
                     * 2;

            arc->SetStroke( STROKE_PARAMS( 1, PLOT_DASH_TYPE::SOLID ) );
            arc->SetFillMode( FILL_T::FILLED_SHAPE );
        }
        else
        {
            arc->SetStroke( STROKE_PARAMS( ewire.width.ToSchUnits(), PLOT_DASH_TYPE::SOLID ) );
        }

        arc->SetArcGeometry( begin, CalcArcMid( begin, end, center ), end );
        arc->SetUnit( aGateNumber );

        return arc;
    }
    else
    {
        LIB_SHAPE* poly = new LIB_SHAPE( aSymbol.get(), SHAPE_T::POLY );

        poly->AddPoint( begin );
        poly->AddPoint( end );
        poly->SetUnit( aGateNumber );
        poly->SetStroke( STROKE_PARAMS( ewire.width.ToSchUnits(), PLOT_DASH_TYPE::SOLID ) );

        return poly;
    }
}


LIB_SHAPE* SCH_EAGLE_PLUGIN::loadSymbolPolyLine( std::unique_ptr<LIB_SYMBOL>& aSymbol,
                                                 wxXmlNode* aPolygonNode, int aGateNumber )
{
    LIB_SHAPE* poly = new LIB_SHAPE( aSymbol.get(), SHAPE_T::POLY );
    EPOLYGON   epoly( aPolygonNode );
    wxXmlNode* vertex = aPolygonNode->GetChildren();
    VECTOR2I   pt;

    while( vertex )
    {
        if( vertex->GetName() == wxT( "vertex" ) ) // skip <xmlattr> node
        {
            EVERTEX evertex( vertex );
            pt = VECTOR2I( evertex.x.ToSchUnits(), evertex.y.ToSchUnits() );
            poly->AddPoint( pt );
        }

        vertex = vertex->GetNext();
    }

    poly->SetFillMode( FILL_T::FILLED_SHAPE );
    poly->SetUnit( aGateNumber );

    return poly;
}


LIB_PIN* SCH_EAGLE_PLUGIN::loadPin( std::unique_ptr<LIB_SYMBOL>& aSymbol, wxXmlNode* aPin,
                                    EPIN* aEPin, int aGateNumber )
{
    std::unique_ptr<LIB_PIN> pin = std::make_unique<LIB_PIN>( aSymbol.get() );
    pin->SetPosition( VECTOR2I( aEPin->x.ToSchUnits(), aEPin->y.ToSchUnits() ) );
    pin->SetName( aEPin->name );
    pin->SetUnit( aGateNumber );

    int roti = aEPin->rot ? aEPin->rot->degrees : 0;

    switch( roti )
    {
    case 0:   pin->SetOrientation( 'R' ); break;
    case 90:  pin->SetOrientation( 'U' ); break;
    case 180: pin->SetOrientation( 'L' ); break;
    case 270: pin->SetOrientation( 'D' ); break;
    default:  wxFAIL_MSG( wxString::Format( wxT( "Unhandled orientation (%d degrees)." ), roti ) );
    }

    pin->SetLength( Mils2iu( 300 ) );  // Default pin length when not defined.

    if( aEPin->length )
    {
        wxString length = aEPin->length.Get();

        if( length == wxT( "short" ) )
            pin->SetLength( Mils2iu( 100 ) );
        else if( length == wxT( "middle" ) )
            pin->SetLength( Mils2iu( 200 ) );
        else if( length == wxT( "long" ) )
            pin->SetLength( Mils2iu( 300 ) );
        else if( length == wxT( "point" ) )
            pin->SetLength( Mils2iu( 0 ) );
    }

    // emulate the visibility of pin elements
    if( aEPin->visible )
    {
        wxString visible = aEPin->visible.Get();

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

    if( aEPin->function )
    {
        wxString function = aEPin->function.Get();

        if( function == wxT( "dot" ) )
            pin->SetShape( GRAPHIC_PINSHAPE::INVERTED );
        else if( function == wxT( "clk" ) )
            pin->SetShape( GRAPHIC_PINSHAPE::CLOCK );
        else if( function == wxT( "dotclk" ) )
            pin->SetShape( GRAPHIC_PINSHAPE::INVERTED_CLOCK );
    }

    return pin.release();
}


LIB_TEXT* SCH_EAGLE_PLUGIN::loadSymbolText( std::unique_ptr<LIB_SYMBOL>& aSymbol,
                                            wxXmlNode* aLibText, int aGateNumber )
{
    std::unique_ptr<LIB_TEXT> libtext = std::make_unique<LIB_TEXT>( aSymbol.get() );
    ETEXT                     etext( aLibText );

    libtext->SetUnit( aGateNumber );
    libtext->SetPosition( VECTOR2I( etext.x.ToSchUnits(), etext.y.ToSchUnits() ) );

    // Eagle supports multiple line text in library symbols.  Legacy library symbol text cannot
    // contain CRs or LFs.
    //
    // @todo Split this into multiple text objects and offset the Y position so that it looks
    //       more like the original Eagle schematic.
    wxString text = aLibText->GetNodeContent();
    std::replace( text.begin(), text.end(), '\n', '_' );
    std::replace( text.begin(), text.end(), '\r', '_' );

    libtext->SetText( text.IsEmpty() ? wxT( "~" ) : text );
    loadTextAttributes( libtext.get(), etext );

    return libtext.release();
}


void SCH_EAGLE_PLUGIN::loadFrame( wxXmlNode* aFrameNode, std::vector<LIB_ITEM*>& aItems )
{
    EFRAME eframe( aFrameNode );

    int xMin = eframe.x1.ToSchUnits();
    int xMax = eframe.x2.ToSchUnits();
    int yMin = eframe.y1.ToSchUnits();
    int yMax = eframe.y2.ToSchUnits();

    if( xMin > xMax )
        std::swap( xMin, xMax );

    if( yMin > yMax )
        std::swap( yMin, yMax );

    LIB_SHAPE* lines = new LIB_SHAPE( nullptr, SHAPE_T::POLY );
    lines->AddPoint( VECTOR2I( xMin, yMin ) );
    lines->AddPoint( VECTOR2I( xMax, yMin ) );
    lines->AddPoint( VECTOR2I( xMax, yMax ) );
    lines->AddPoint( VECTOR2I( xMin, yMax ) );
    lines->AddPoint( VECTOR2I( xMin, yMin ) );
    aItems.push_back( lines );

    if( !eframe.border_left )
    {
        lines = new LIB_SHAPE( nullptr, SHAPE_T::POLY );
        lines->AddPoint( VECTOR2I( xMin + Mils2iu( 150 ), yMin + Mils2iu( 150 ) ) );
        lines->AddPoint( VECTOR2I( xMin + Mils2iu( 150 ), yMax - Mils2iu( 150 ) ) );
        aItems.push_back( lines );

        int i;
        int height = yMax - yMin;
        int x1 = xMin;
        int x2 = x1 + Mils2iu( 150 );
        int legendPosX = xMin + Mils2iu( 75 );
        double rowSpacing = height / double( eframe.rows );
        double legendPosY = yMax - ( rowSpacing / 2 );

        for( i = 1; i < eframe.rows; i++ )
        {
            int newY = KiROUND( yMin + ( rowSpacing * (double) i ) );
            lines = new LIB_SHAPE( nullptr, SHAPE_T::POLY );
            lines->AddPoint( VECTOR2I( x1, newY ) );
            lines->AddPoint( VECTOR2I( x2, newY ) );
            aItems.push_back( lines );
        }

        char legendChar = 'A';

        for( i = 0; i < eframe.rows; i++ )
        {
            LIB_TEXT* legendText = new LIB_TEXT( nullptr );
            legendText->SetPosition( VECTOR2I( legendPosX, KiROUND( legendPosY ) ) );
            legendText->SetText( wxString( legendChar ) );
            legendText->SetTextSize( wxSize( Mils2iu( 90 ), Mils2iu( 100 ) ) );
            aItems.push_back( legendText );
            legendChar++;
            legendPosY -= rowSpacing;
        }
    }

    if( !eframe.border_right )
    {
        lines = new LIB_SHAPE( nullptr, SHAPE_T::POLY );
        lines->AddPoint( VECTOR2I( xMax - Mils2iu( 150 ), yMin + Mils2iu( 150 ) ) );
        lines->AddPoint( VECTOR2I( xMax - Mils2iu( 150 ), yMax - Mils2iu( 150 ) ) );
        aItems.push_back( lines );

        int i;
        int height = yMax - yMin;
        int x1 = xMax - Mils2iu( 150 );
        int x2 = xMax;
        int legendPosX = xMax - Mils2iu( 75 );
        double rowSpacing = height / double( eframe.rows );
        double legendPosY = yMax - ( rowSpacing / 2 );

        for( i = 1; i < eframe.rows; i++ )
        {
            int newY = KiROUND( yMin + ( rowSpacing * (double) i ) );
            lines = new LIB_SHAPE( nullptr, SHAPE_T::POLY );
            lines->AddPoint( VECTOR2I( x1, newY ) );
            lines->AddPoint( VECTOR2I( x2, newY ) );
            aItems.push_back( lines );
        }

        char legendChar = 'A';

        for( i = 0; i < eframe.rows; i++ )
        {
            LIB_TEXT* legendText = new LIB_TEXT( nullptr );
            legendText->SetPosition( VECTOR2I( legendPosX, KiROUND( legendPosY ) ) );
            legendText->SetText( wxString( legendChar ) );
            legendText->SetTextSize( wxSize( Mils2iu( 90 ), Mils2iu( 100 ) ) );
            aItems.push_back( legendText );
            legendChar++;
            legendPosY -= rowSpacing;
        }
    }

    if( !eframe.border_top )
    {
        lines = new LIB_SHAPE( nullptr, SHAPE_T::POLY );
        lines->AddPoint( VECTOR2I( xMax - Mils2iu( 150 ), yMax - Mils2iu( 150 ) ) );
        lines->AddPoint( VECTOR2I( xMin + Mils2iu( 150 ), yMax - Mils2iu( 150 ) ) );
        aItems.push_back( lines );

        int i;
        int width = xMax - xMin;
        int y1 = yMin;
        int y2 = yMin + Mils2iu( 150 );
        int legendPosY = yMax - Mils2iu( 75 );
        double columnSpacing = width / double( eframe.columns );
        double legendPosX = xMin + ( columnSpacing / 2 );

        for( i = 1; i < eframe.columns; i++ )
        {
            int newX = KiROUND( xMin + ( columnSpacing * (double) i ) );
            lines = new LIB_SHAPE( nullptr, SHAPE_T::POLY );
            lines->AddPoint( VECTOR2I( newX, y1 ) );
            lines->AddPoint( VECTOR2I( newX, y2 ) );
            aItems.push_back( lines );
        }

        char legendChar = '1';

        for( i = 0; i < eframe.columns; i++ )
        {
            LIB_TEXT* legendText = new LIB_TEXT( nullptr );
            legendText->SetPosition( VECTOR2I( KiROUND( legendPosX ), legendPosY ) );
            legendText->SetText( wxString( legendChar ) );
            legendText->SetTextSize( wxSize( Mils2iu( 90 ), Mils2iu( 100 ) ) );
            aItems.push_back( legendText );
            legendChar++;
            legendPosX += columnSpacing;
        }
    }

    if( !eframe.border_bottom )
    {
        lines = new LIB_SHAPE( nullptr, SHAPE_T::POLY );
        lines->AddPoint( VECTOR2I( xMax - Mils2iu( 150 ), yMin + Mils2iu( 150 ) ) );
        lines->AddPoint( VECTOR2I( xMin + Mils2iu( 150 ), yMin + Mils2iu( 150 ) ) );
        aItems.push_back( lines );

        int i;
        int width = xMax - xMin;
        int y1 = yMax - Mils2iu( 150 );
        int y2 = yMax;
        int legendPosY = yMin + Mils2iu( 75 );
        double columnSpacing = width / double( eframe.columns );
        double legendPosX = xMin + ( columnSpacing / 2 );

        for( i = 1; i < eframe.columns; i++ )
        {
            int newX = KiROUND( xMin + ( columnSpacing * (double) i ) );
            lines = new LIB_SHAPE( nullptr, SHAPE_T::POLY );
            lines->AddPoint( VECTOR2I( newX, y1 ) );
            lines->AddPoint( VECTOR2I( newX, y2 ) );
            aItems.push_back( lines );
        }

        char legendChar = '1';

        for( i = 0; i < eframe.columns; i++ )
        {
            LIB_TEXT* legendText = new LIB_TEXT( nullptr );
            legendText->SetPosition( VECTOR2I( KiROUND( legendPosX ), legendPosY ) );
            legendText->SetText( wxString( legendChar ) );
            legendText->SetTextSize( wxSize( Mils2iu( 90 ), Mils2iu( 100 ) ) );
            aItems.push_back( legendText );
            legendChar++;
            legendPosX += columnSpacing;
        }
    }
}


SCH_TEXT* SCH_EAGLE_PLUGIN::loadPlainText( wxXmlNode* aSchText )
{
    std::unique_ptr<SCH_TEXT> schtext = std::make_unique<SCH_TEXT>();
    ETEXT                     etext   = ETEXT( aSchText );

    const wxString& thetext = aSchText->GetNodeContent();

    wxString adjustedText;
    wxStringTokenizer tokenizer( thetext, "\r\n" );

    // Strip the whitespace from both ends of each line.
    while( tokenizer.HasMoreTokens() )
    {
        wxString tmp = tokenizer.GetNextToken().Trim();

        tmp = tmp.Trim( false );

        if( tokenizer.HasMoreTokens() )
            tmp += wxT( "\n" );

        adjustedText += tmp;
    }

    schtext->SetText( adjustedText.IsEmpty() ? wxT( "\" \"" ) : escapeName( adjustedText ) );
    schtext->SetPosition( VECTOR2I( etext.x.ToSchUnits(), -etext.y.ToSchUnits() ) );
    loadTextAttributes( schtext.get(), etext );
    schtext->SetItalic( false );

    return schtext.release();
}


void SCH_EAGLE_PLUGIN::loadTextAttributes( EDA_TEXT* aText, const ETEXT& aAttribs ) const
{
    aText->SetTextSize( aAttribs.ConvertSize() );

    if( aAttribs.ratio )
    {
        if( aAttribs.ratio.CGet() > 12 )
        {
            aText->SetBold( true );
            aText->SetTextThickness( GetPenSizeForBold( aText->GetTextWidth() ) );
        }
    }

    int  align   = aAttribs.align ? *aAttribs.align : ETEXT::BOTTOM_LEFT;
    int  degrees = aAttribs.rot ? aAttribs.rot->degrees : 0;
    bool mirror  = aAttribs.rot ? aAttribs.rot->mirror : false;
    bool spin    = aAttribs.rot ? aAttribs.rot->spin : false;

    eagleToKicadAlignment( aText, align, degrees, mirror, spin, 0 );
}


void SCH_EAGLE_PLUGIN::loadFieldAttributes( LIB_FIELD* aField, const LIB_TEXT* aText ) const
{
    aField->SetTextPos( aText->GetPosition() );
    aField->SetTextSize( aText->GetTextSize() );
    aField->SetTextAngle( aText->GetTextAngle() );
    aField->SetBold( aText->IsBold() );
    aField->SetVertJustify( aText->GetVertJustify() );
    aField->SetHorizJustify( aText->GetHorizJustify() );
    aField->SetVisible( true );
}


void SCH_EAGLE_PLUGIN::adjustNetLabels()
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
            wireDirection = wireDirection.Resize( Mils2iu( 50 ) );
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


bool SCH_EAGLE_PLUGIN::CheckHeader( const wxString& aFileName )
{
    // Open file and check first line
    wxTextFile tempFile;

    tempFile.Open( aFileName );
    wxString firstline;

    // read the first line
    firstline           = tempFile.GetFirstLine();
    wxString secondline = tempFile.GetNextLine();
    wxString thirdline  = tempFile.GetNextLine();
    tempFile.Close();

    return firstline.StartsWith( wxT( "<?xml" ) )
            && secondline.StartsWith( wxT( "<!DOCTYPE eagle SYSTEM" ) )
            && thirdline.StartsWith( wxT( "<eagle version" ) );
}


void SCH_EAGLE_PLUGIN::moveLabels( SCH_LINE* aWire, const VECTOR2I& aNewEndPoint )
{
    static KICAD_T labelTypes[] = { SCH_LABEL_LOCATE_ANY_T, EOT };
    SCH_SCREEN*    screen = m_currentSheet->GetScreen();

    for( SCH_ITEM* item : screen->Items().Overlapping( aWire->GetBoundingBox() ) )
    {
        if( !item->IsType( labelTypes ) )
            continue;

        if( TestSegmentHit( item->GetPosition(), aWire->GetStartPoint(), aWire->GetEndPoint(), 0 ) )
            item->SetPosition( aNewEndPoint );
    }
}


void SCH_EAGLE_PLUGIN::addBusEntries()
{
    // Add bus entry symbols
    // TODO: Cleanup this function and break into pieces

    // for each wire segment, compare each end with all busses.
    // If the wire end is found to end on a bus segment, place a bus entry symbol.

    std::vector<SCH_LINE*> buses;
    std::vector<SCH_LINE*> wires;

    for( SCH_ITEM* ii : m_currentSheet->GetScreen()->Items().OfType( SCH_LINE_T ) )
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
                        return VECTOR2I( Mils2iu( DEFAULT_SCH_ENTRY_SIZE ) * signX,
                                        Mils2iu( DEFAULT_SCH_ENTRY_SIZE ) * signY );
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
                            m_currentSheet->GetScreen()->Append( busEntry );
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
                            m_currentSheet->GetScreen()->Append( busEntry );
                            moveLabels( wire, p );
                            wire->SetStartPoint( p );
                        }
                        else
                        {
                            auto        ercItem = ERC_ITEM::Create( ERCE_BUS_ENTRY_NEEDED );
                            SCH_MARKER* marker = new SCH_MARKER( ercItem, wireStart );
                            m_currentSheet->GetScreen()->Append( marker );
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
                            m_currentSheet->GetScreen()->Append( busEntry );
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
                            m_currentSheet->GetScreen()->Append( busEntry );
                            moveLabels( wire, p );
                            wire->SetStartPoint( p );
                        }
                        else
                        {
                            auto        ercItem = ERC_ITEM::Create( ERCE_BUS_ENTRY_NEEDED );
                            SCH_MARKER* marker = new SCH_MARKER( ercItem, wireStart );
                            m_currentSheet->GetScreen()->Append( marker );
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
                            m_currentSheet->GetScreen()->Append( busEntry );
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
                            m_currentSheet->GetScreen()->Append( busEntry );
                            moveLabels( wire, wireEnd + entrySize( -1, 0 ) );
                            wire->SetEndPoint( wireEnd + entrySize( -1, 0 ) );
                        }
                        else
                        {
                            auto        ercItem = ERC_ITEM::Create( ERCE_BUS_ENTRY_NEEDED );
                            SCH_MARKER* marker = new SCH_MARKER( ercItem, wireEnd );
                            m_currentSheet->GetScreen()->Append( marker );
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
                            m_currentSheet->GetScreen()->Append( busEntry );
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
                            m_currentSheet->GetScreen()->Append( busEntry );
                            moveLabels( wire, p );
                            wire->SetEndPoint( p );
                        }
                        else
                        {
                            auto        ercItem = ERC_ITEM::Create( ERCE_BUS_ENTRY_NEEDED );
                            SCH_MARKER* marker = new SCH_MARKER( ercItem, wireEnd );
                            m_currentSheet->GetScreen()->Append( marker );
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
                            m_currentSheet->GetScreen()->Append( busEntry );
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
                            m_currentSheet->GetScreen()->Append( busEntry );
                            moveLabels( wire, p );
                            wire->SetStartPoint( p );
                        }
                        else
                        {
                            auto        ercItem = ERC_ITEM::Create( ERCE_BUS_ENTRY_NEEDED );
                            SCH_MARKER* marker = new SCH_MARKER( ercItem, wireStart );
                            m_currentSheet->GetScreen()->Append( marker );
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
                            m_currentSheet->GetScreen()->Append( busEntry );
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
                            m_currentSheet->GetScreen()->Append( busEntry );
                            moveLabels( wire, p );
                            wire->SetStartPoint( p );
                        }
                        else
                        {
                            auto        ercItem = ERC_ITEM::Create( ERCE_BUS_ENTRY_NEEDED );
                            SCH_MARKER* marker = new SCH_MARKER( ercItem, wireStart );
                            m_currentSheet->GetScreen()->Append( marker );
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
                            m_currentSheet->GetScreen()->Append( busEntry );
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
                            m_currentSheet->GetScreen()->Append( busEntry );
                            moveLabels( wire, p );
                            wire->SetEndPoint( p );
                        }
                        else
                        {
                            auto        ercItem = ERC_ITEM::Create( ERCE_BUS_ENTRY_NEEDED );
                            SCH_MARKER* marker = new SCH_MARKER( ercItem, wireEnd );
                            m_currentSheet->GetScreen()->Append( marker );
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
                            m_currentSheet->GetScreen()->Append( busEntry );
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
                            m_currentSheet->GetScreen()->Append( busEntry );
                            moveLabels( wire, p );
                            wire->SetEndPoint( p );
                        }
                        else
                        {
                            auto        ercItem = ERC_ITEM::Create( ERCE_BUS_ENTRY_NEEDED );
                            SCH_MARKER* marker = new SCH_MARKER( ercItem, wireEnd );
                            m_currentSheet->GetScreen()->Append( marker );
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
                            m_currentSheet->GetScreen()->Append( busEntry );

                            moveLabels( wire, p );
                            wire->SetStartPoint( p );
                        }
                        else
                        {
                            VECTOR2I            p = wireStart + entrySize( -1, 1 );
                            SCH_BUS_WIRE_ENTRY* busEntry = new SCH_BUS_WIRE_ENTRY( p, 1 );
                            busEntry->SetFlags( IS_NEW );
                            m_currentSheet->GetScreen()->Append( busEntry );

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
                            m_currentSheet->GetScreen()->Append( busEntry );

                            moveLabels( wire, p );
                            wire->SetStartPoint( p );
                        }
                        else
                        {
                            VECTOR2I            p = wireStart + entrySize( 1, 1 );
                            SCH_BUS_WIRE_ENTRY* busEntry = new SCH_BUS_WIRE_ENTRY( p, 4 );
                            busEntry->SetFlags( IS_NEW );
                            m_currentSheet->GetScreen()->Append( busEntry );

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
                            m_currentSheet->GetScreen()->Append( busEntry );

                            moveLabels( wire, p );
                            wire->SetEndPoint( p );
                        }
                        else
                        {
                            VECTOR2I            p = wireEnd + entrySize( 1, -1 );
                            SCH_BUS_WIRE_ENTRY* busEntry = new SCH_BUS_WIRE_ENTRY( p, 3 );
                            busEntry->SetFlags( IS_NEW );
                            m_currentSheet->GetScreen()->Append( busEntry );

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
                            m_currentSheet->GetScreen()->Append( busEntry );

                            moveLabels( wire, p );
                            wire->SetEndPoint( p );
                        }
                        else
                        {
                            VECTOR2I            p = wireEnd + entrySize( -1, -1 );
                            SCH_BUS_WIRE_ENTRY* busEntry = new SCH_BUS_WIRE_ENTRY( p, 2 );
                            busEntry->SetFlags( IS_NEW );
                            m_currentSheet->GetScreen()->Append( busEntry );

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


const SEG* SCH_EAGLE_PLUGIN::SEG_DESC::LabelAttached( const SCH_TEXT* aLabel ) const
{
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
bool SCH_EAGLE_PLUGIN::checkConnections( const SCH_SYMBOL* aSymbol, const LIB_PIN* aPin ) const
{
    VECTOR2I pinPosition = aSymbol->GetPinPhysicalPosition( aPin );
    auto     pointIt     = m_connPoints.find( pinPosition );

    if( pointIt == m_connPoints.end() )
        return false;

    const auto& items = pointIt->second;
    wxASSERT( items.find( aPin ) != items.end() );
    return items.size() > 1;
}


void SCH_EAGLE_PLUGIN::addImplicitConnections( SCH_SYMBOL* aSymbol, SCH_SCREEN* aScreen,
                                               bool aUpdateSet )
{
    wxCHECK( aSymbol->GetLibSymbolRef(), /*void*/ );

    // Normally power parts also have power input pins,
    // but they already force net names on the attached wires
    if( aSymbol->GetLibSymbolRef()->IsPower() )
        return;

    int                   unit      = aSymbol->GetUnit();
    const wxString        reference = aSymbol->GetField( REFERENCE_FIELD )->GetText();
    std::vector<LIB_PIN*> pins;
    aSymbol->GetLibSymbolRef()->GetPins( pins );
    std::set<int> missingUnits;

    // Search all units for pins creating implicit connections
    for( const LIB_PIN* pin : pins )
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
                    netLabel->SetTextSize( wxSize( Mils2iu( 40 ), Mils2iu( 40 ) ) );

                    switch( pin->GetOrientation() )
                    {
                    case PIN_LEFT:  netLabel->SetTextSpinStyle( TEXT_SPIN_STYLE::RIGHT );  break;
                    case PIN_RIGHT: netLabel->SetTextSpinStyle( TEXT_SPIN_STYLE::LEFT );   break;
                    case PIN_UP:    netLabel->SetTextSpinStyle( TEXT_SPIN_STYLE::UP );     break;
                    case PIN_DOWN:  netLabel->SetTextSpinStyle( TEXT_SPIN_STYLE::BOTTOM ); break;
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


wxString SCH_EAGLE_PLUGIN::translateEagleBusName( const wxString& aEagleName ) const
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
