/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2017 CERN
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

#include <properties.h>
#include <kiway.h>

#include <wx/filename.h>
#include <wx/tokenzr.h>
#include <memory>
#include <algorithm>

#include <sch_junction.h>
#include <sch_sheet.h>
#include <sch_edit_frame.h>
#include <ws_draw_item.h>
#include <template_fieldnames.h>
#include <wildcards_and_files_ext.h>
#include <sch_screen.h>
#include <class_library.h>
#include <class_libentry.h>
#include <lib_item.h>
#include <lib_id.h>
#include <sch_component.h>
#include <sch_connection.h>
#include <sch_sheet_path.h>
#include <lib_arc.h>
#include <lib_circle.h>
#include <lib_rectangle.h>
#include <lib_polyline.h>
#include <lib_pin.h>
#include <lib_text.h>
#include <sch_text.h>
#include <gr_text.h>
#include <sch_marker.h>
#include <sch_bus_entry.h>
#include <eagle_parser.h>
#include <symbol_lib_table.h>
#include <sch_legacy_plugin.h>
#include <sch_eagle_plugin.h>



// Eagle schematic axes are aligned with x increasing left to right and Y increasing bottom to top
// Kicad schematic axes are aligned with x increasing left to right and Y increasing top to bottom.

using namespace std;

/**
 * Provides an easy access to the children of an XML node via their names.
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


///> Computes a bounding box for all items in a schematic sheet
static EDA_RECT getSheetBbox( SCH_SHEET* aSheet )
{
    EDA_RECT bbox;
    SCH_ITEM* item = aSheet->GetScreen()->GetDrawItems();

    if( !item )
        return bbox;

    bbox = item->GetBoundingBox();
    item = item->Next();

    while( item )
    {
        bbox.Merge( item->GetBoundingBox() );
        item = item->Next();
    }

    return bbox;
}


///> Extracts the net name part from a pin name (e.g. return 'GND' for pin named 'GND@2')
static inline wxString extractNetName( const wxString& aPinName )
{
    return aPinName.BeforeFirst( '@' );
}


wxString SCH_EAGLE_PLUGIN::getLibName()
{
    if( m_libName.IsEmpty() )
    {
        // Try to come up with a meaningful name
        m_libName = m_kiway->Prj().GetProjectName();

        if( m_libName.IsEmpty() )
        {
            wxFileName fn( m_rootSheet->GetFileName() );
            m_libName = fn.GetName();
        }

        if( m_libName.IsEmpty() )
            m_libName = "noname";

        m_libName += "-eagle-import";
        m_libName = LIB_ID::FixIllegalChars( m_libName, LIB_ID::ID_SCH, true );
    }

    return m_libName;
}


wxFileName SCH_EAGLE_PLUGIN::getLibFileName()
{
    wxFileName fn( m_kiway->Prj().GetProjectPath(), getLibName(), SchematicLibraryFileExtension );

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
    for( const auto& elayer : eagleLayers )
    {
        /**
         * Layers in Kicad schematics are not actually layers, but abstract groups mainly used to
         * decide item colours.
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


        if( elayer.name == "Nets" )
        {
            m_layerMap[elayer.number] = LAYER_WIRE;
        }
        else if( elayer.name == "Info" || elayer.name == "Guide" )
        {
            m_layerMap[elayer.number] = LAYER_NOTES;
        }
        else if( elayer.name == "Busses" )
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


// Return the kicad component orientation based on eagle rotation degrees.
static COMPONENT_ORIENTATION_T kiCadComponentRotation( float eagleDegrees )
{
    int roti = int( eagleDegrees );

    switch( roti )
    {
    default:
        wxASSERT_MSG( false, wxString::Format( "Unhandled orientation (%d degrees)", roti ) );

    case 0:
        return CMP_ORIENT_0;

    case 90:
        return CMP_ORIENT_90;

    case 180:
        return CMP_ORIENT_180;

    case 270:
        return CMP_ORIENT_270;
    }

    return CMP_ORIENT_0;
}


// Calculate text alignment based on the given Eagle text alignment parameters.
static void eagleToKicadAlignment( EDA_TEXT* aText, int aEagleAlignment,
        int aRelDegress, bool aMirror, bool aSpin, int aAbsDegress )
{
    int align = aEagleAlignment;

    if( aRelDegress == 90 )
    {
        aText->SetTextAngle( 900 );
    }
    else if( aRelDegress == 180 )
        align = -align;
    else if( aRelDegress == 270 )
    {
        aText->SetTextAngle( 900 );
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
        aText->SetHorizJustify( GR_TEXT_HJUSTIFY_CENTER );
        aText->SetVertJustify( GR_TEXT_VJUSTIFY_CENTER );
        break;

    case ETEXT::CENTER_LEFT:
        aText->SetHorizJustify( GR_TEXT_HJUSTIFY_LEFT );
        aText->SetVertJustify( GR_TEXT_VJUSTIFY_CENTER );
        break;

    case ETEXT::CENTER_RIGHT:
        aText->SetHorizJustify( GR_TEXT_HJUSTIFY_RIGHT );
        aText->SetVertJustify( GR_TEXT_VJUSTIFY_CENTER );
        break;

    case ETEXT::TOP_CENTER:
        aText->SetHorizJustify( GR_TEXT_HJUSTIFY_CENTER );
        aText->SetVertJustify( GR_TEXT_VJUSTIFY_TOP );
        break;

    case ETEXT::TOP_LEFT:
        aText->SetHorizJustify( GR_TEXT_HJUSTIFY_LEFT );
        aText->SetVertJustify( GR_TEXT_VJUSTIFY_TOP );
        break;

    case ETEXT::TOP_RIGHT:
        aText->SetHorizJustify( GR_TEXT_HJUSTIFY_RIGHT );
        aText->SetVertJustify( GR_TEXT_VJUSTIFY_TOP );
        break;

    case ETEXT::BOTTOM_CENTER:
        aText->SetHorizJustify( GR_TEXT_HJUSTIFY_CENTER );
        aText->SetVertJustify( GR_TEXT_VJUSTIFY_BOTTOM );
        break;

    case ETEXT::BOTTOM_LEFT:
        aText->SetHorizJustify( GR_TEXT_HJUSTIFY_LEFT );
        aText->SetVertJustify( GR_TEXT_VJUSTIFY_BOTTOM );
        break;

    case ETEXT::BOTTOM_RIGHT:
        aText->SetHorizJustify( GR_TEXT_HJUSTIFY_RIGHT );
        aText->SetVertJustify( GR_TEXT_VJUSTIFY_BOTTOM );
        break;

    default:
        aText->SetHorizJustify( GR_TEXT_HJUSTIFY_RIGHT );
        aText->SetVertJustify( GR_TEXT_VJUSTIFY_BOTTOM );
    }
}


SCH_EAGLE_PLUGIN::SCH_EAGLE_PLUGIN()
{
    m_kiway = nullptr;
    m_rootSheet = nullptr;
    m_currentSheet = nullptr;
}


SCH_EAGLE_PLUGIN::~SCH_EAGLE_PLUGIN()
{
}


const wxString SCH_EAGLE_PLUGIN::GetName() const
{
    return "EAGLE";
}


const wxString SCH_EAGLE_PLUGIN::GetFileExtension() const
{
    return "sch";
}


int SCH_EAGLE_PLUGIN::GetModifyHash() const
{
    return 0;
}


SCH_SHEET* SCH_EAGLE_PLUGIN::Load( const wxString& aFileName, KIWAY* aKiway,
        SCH_SHEET* aAppendToMe, const PROPERTIES* aProperties )
{
    wxASSERT( !aFileName || aKiway != NULL );
    LOCALE_IO toggle;     // toggles on, then off, the C locale.

    // Load the document
    wxXmlDocument xmlDocument;

    m_filename = aFileName;
    m_kiway = aKiway;

    if( !xmlDocument.Load( m_filename.GetFullPath() ) )
        THROW_IO_ERROR( wxString::Format( _( "Unable to read file \"%s\"" ),
                        m_filename.GetFullPath() ) );

    // Delete on exception, if I own m_rootSheet, according to aAppendToMe
    unique_ptr<SCH_SHEET> deleter( aAppendToMe ? nullptr : m_rootSheet );

    if( aAppendToMe )
    {
        m_rootSheet = aAppendToMe->GetRootSheet();
    }
    else
    {
        m_rootSheet = new SCH_SHEET();
        m_rootSheet->SetFileName( aFileName );
    }

    if( !m_rootSheet->GetScreen() )
    {
        SCH_SCREEN* screen = new SCH_SCREEN( aKiway );
        screen->SetFileName( aFileName );
        m_rootSheet->SetScreen( screen );
    }

    SYMBOL_LIB_TABLE* libTable = m_kiway->Prj().SchSymbolLibTable();

    wxCHECK_MSG( libTable, NULL, "Could not load symbol lib table." );

    m_pi.set( SCH_IO_MGR::FindPlugin( SCH_IO_MGR::SCH_LEGACY ) );
    m_properties = std::make_unique<PROPERTIES>();
    (*m_properties)[ SCH_LEGACY_PLUGIN::PropBuffering ] = "";

    /// @note No check is being done here to see if the existing symbol library exists so this
    ///       will overwrite the existing one.
    if( !libTable->HasLibrary( getLibName() ) )
    {
        // Create a new empty symbol library.
        m_pi->CreateSymbolLib( getLibFileName().GetFullPath() );
        wxString libTableUri = "${KIPRJMOD}/" + getLibFileName().GetFullName();

        // Add the new library to the project symbol library table.
        libTable->InsertRow( new SYMBOL_LIB_TABLE_ROW( getLibName(), libTableUri,
                                                       wxString( "Legacy" ) ) );

        // Save project symbol library table.
        wxFileName fn( m_kiway->Prj().GetProjectPath(),
                       SYMBOL_LIB_TABLE::GetSymbolLibTableFileName() );

        // So output formatter goes out of scope and closes the file before reloading.
        {
        FILE_OUTPUTFORMATTER formatter( fn.GetFullPath() );
        libTable->Format( &formatter, 0 );
        }

        // Relaod the symbol library table.
        m_kiway->Prj().SetElem( PROJECT::ELEM_SYMBOL_LIB_TABLE, NULL );
        m_kiway->Prj().SchSymbolLibTable();
    }

    // Retrieve the root as current node
    wxXmlNode* currentNode = xmlDocument.GetRoot();

    // If the attribute is found, store the Eagle version;
    // otherwise, store the dummy "0.0" version.
    m_version = currentNode->GetAttribute( "version", "0.0" );

    // Map all children into a readable dictionary
    NODE_MAP children = MapChildren( currentNode );

    // Load drawing
    loadDrawing( children["drawing"] );

    m_pi->SaveLibrary( getLibFileName().GetFullPath() );

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

    wxXmlNode* sheetNode = getChildrenNodes( schematicChildren, "sheets" );

    while( sheetNode )
    {
        NODE_MAP sheetChildren = MapChildren( sheetNode );
        // Loop through all nets
        // From the DTD: "Net is an electrical connection in a schematic."
        wxXmlNode* netNode = getChildrenNodes( sheetChildren, "nets" );

        while( netNode )
        {
            wxString netName = netNode->GetAttribute( "name" );

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
    NODE_MAP schematicChildren = MapChildren( aSchematicNode );
    auto partNode = getChildrenNodes( schematicChildren, "parts" );
    auto libraryNode = getChildrenNodes( schematicChildren, "libraries" );
    auto sheetNode = getChildrenNodes( schematicChildren, "sheets" );

    if( !partNode || !libraryNode || !sheetNode )
        return;

    while( partNode )
    {
        std::unique_ptr<EPART> epart( new EPART( partNode ) );

        // N.B. Eagle parts are case-insensitive in matching but we keep the display case
        m_partlist[epart->name.Upper()] = std::move( epart );
        partNode = partNode->GetNext();
    }

    // Loop through all the libraries
    while( libraryNode )
    {
        // Read the library name
        wxString libName = libraryNode->GetAttribute( "name" );

        EAGLE_LIBRARY* elib = &m_eagleLibs[libName];
        elib->name = libName;

        loadLibrary( libraryNode, &m_eagleLibs[libName] );

        libraryNode = libraryNode->GetNext();
    }

    m_pi->SaveLibrary( getLibFileName().GetFullPath() );

    // find all nets and count how many sheets they appear on.
    // local labels will be used for nets found only on that sheet.
    countNets( aSchematicNode );

    // Loop through all the sheets
    int sheet_count = countChildren( sheetNode->GetParent(), "sheet" );

    // If eagle schematic has multiple sheets then create corresponding subsheets on the root sheet
    if( sheet_count > 1 )
    {
        int x, y, i;
        i   = 1;
        x   = 1;
        y   = 1;

        while( sheetNode )
        {
            wxPoint pos = wxPoint( x * 1000, y * 1000 );
            std::unique_ptr<SCH_SHEET> sheet( new SCH_SHEET( pos ) );
            SCH_SCREEN* screen = new SCH_SCREEN( m_kiway );

            sheet->SetTimeStamp( GetNewTimeStamp() - i );    // minus the sheet index to make it unique.
            sheet->SetParent( m_rootSheet->GetScreen() );
            sheet->SetScreen( screen );
            sheet->GetScreen()->SetFileName( sheet->GetFileName() );

            m_currentSheet = sheet.get();
            loadSheet( sheetNode, i );
            m_rootSheet->GetScreen()->Append( sheet.release() );

            sheetNode = sheetNode->GetNext();
            x += 2;

            if( x > 10 )    // start next row
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
        }
    }


    // Handle the missing component units that need to be instantiated
    // to create the missing implicit connections

    // Calculate the already placed items bounding box and the page size to determine
    // placement for the new components
    wxSize pageSizeIU = m_rootSheet->GetScreen()->GetPageSettings().GetSizeIU();
    EDA_RECT sheetBbox = getSheetBbox( m_rootSheet );
    wxPoint newCmpPosition( sheetBbox.GetLeft(), sheetBbox.GetBottom() );
    int maxY = sheetBbox.GetY();

    SCH_SHEET_PATH sheetpath;
    m_rootSheet->LocatePathOfScreen( m_rootSheet->GetScreen(), &sheetpath );

    for( auto& cmp : m_missingCmps )
    {
        const SCH_COMPONENT* origCmp = cmp.second.cmp;

        for( auto unitEntry : cmp.second.units )
        {
            if( unitEntry.second == false )
                continue;     // unit has been already processed

            // Instantiate the missing component unit
            int unit = unitEntry.first;
            const wxString reference = origCmp->GetField( REFERENCE )->GetText();
            std::unique_ptr<SCH_COMPONENT> component( new SCH_COMPONENT( *origCmp ) );
            component->SetUnitSelection( &sheetpath, unit );
            component->SetUnit( unit );
            component->SetTimeStamp( EagleModuleTstamp( reference, origCmp->GetField( VALUE )->GetText(), unit ) );
            component->SetOrientation( 0 );
            component->AddHierarchicalReference( sheetpath.Path(), reference, unit );

            // Calculate the placement position
            EDA_RECT cmpBbox = component->GetBoundingBox();
            int posY = newCmpPosition.y + cmpBbox.GetHeight();
            component->SetPosition( wxPoint( newCmpPosition.x, posY ) );
            newCmpPosition.x += cmpBbox.GetWidth();
            maxY = std::max( maxY, posY );

            if( newCmpPosition.x >= pageSizeIU.GetWidth() )             // reached the page boundary?
                newCmpPosition = wxPoint( sheetBbox.GetLeft(), maxY );  // then start a new row

            // Add the global net labels to recreate the implicit connections
            addImplicitConnections( component.get(), m_rootSheet->GetScreen(), false );
            m_rootSheet->GetScreen()->Append( component.release() );
        }
    }

    m_missingCmps.clear();
}


void SCH_EAGLE_PLUGIN::loadSheet( wxXmlNode* aSheetNode, int aSheetIndex )
{
    // Map all children into a readable dictionary
    NODE_MAP sheetChildren = MapChildren( aSheetNode );

    // Get description node
    wxXmlNode* descriptionNode = getChildrenNodes( sheetChildren, "description" );

    wxString des;
    std::string filename;

    if( descriptionNode )
    {
        des = descriptionNode->GetContent();
        des.Replace( "\n", "_", true );
        m_currentSheet->SetName( des );
        filename = des.ToStdString();
    }
    else
    {
        filename = wxString::Format( "%s_%d", m_filename.GetName(), aSheetIndex );
        m_currentSheet->SetName( filename );
    }

    ReplaceIllegalFileNameChars( &filename );
    replace( filename.begin(), filename.end(), ' ', '_' );

    wxString fn = wxString( filename + ".sch" );
    m_currentSheet->SetFileName( fn );
    wxFileName fileName = m_currentSheet->GetFileName();
    m_currentSheet->GetScreen()->SetFileName( fileName.GetFullPath() );

    // Loop through all busses
    // From the DTD: "Buses receive names which determine which signals they include.
    // A bus is a drawing object. It does not create any electrical connections.
    // These are always created by means of the nets and their names."
    wxXmlNode* busNode = getChildrenNodes( sheetChildren, "busses" );

    while( busNode )
    {
        // Get the bus name
        wxString busName = translateEagleBusName( busNode->GetAttribute( "name" ) );

        // Load segments of this bus
        loadSegments( busNode, busName, wxString() );

        // Get next bus
        busNode = busNode->GetNext();
    }

    // Loop through all nets
    // From the DTD: "Net is an electrical connection in a schematic."
    wxXmlNode* netNode = getChildrenNodes( sheetChildren, "nets" );

    while( netNode )
    {
        // Get the net name and class
        wxString netName  = netNode->GetAttribute( "name" );
        wxString netClass = netNode->GetAttribute( "class" );

        // Load segments of this net
        loadSegments( netNode, netName, netClass );

        // Get next net
        netNode = netNode->GetNext();
    }

    adjustNetLabels();        // needs to be called before addBusEntries()
    addBusEntries();

    // Loop through all instances
    wxXmlNode* instanceNode = getChildrenNodes( sheetChildren, "instances" );

    while( instanceNode )
    {
        loadInstance( instanceNode );
        instanceNode = instanceNode->GetNext();
    }

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

    wxXmlNode* plainNode = getChildrenNodes( sheetChildren, "plain" );

    while( plainNode )
    {
        wxString nodeName = plainNode->GetName();

        if( nodeName == "text" )
        {
            m_currentSheet->GetScreen()->Append( loadPlainText( plainNode ) );
        }
        else if( nodeName == "wire" )
        {
            m_currentSheet->GetScreen()->Append( loadWire( plainNode ) );
        }

        plainNode = plainNode->GetNext();
    }

    // Calculate the new sheet size.
    EDA_RECT sheetBoundingBox = getSheetBbox( m_currentSheet );
    wxSize targetSheetSize = sheetBoundingBox.GetSize();
    targetSheetSize.IncBy( 1500, 1500 );

    // Get current Eeschema sheet size.
    wxSize pageSizeIU   = m_currentSheet->GetScreen()->GetPageSettings().GetSizeIU();
    PAGE_INFO pageInfo  = m_currentSheet->GetScreen()->GetPageSettings();

    // Increase if necessary
    if( pageSizeIU.x < targetSheetSize.x )
        pageInfo.SetWidthMils( targetSheetSize.x );

    if( pageSizeIU.y < targetSheetSize.y )
        pageInfo.SetHeightMils( targetSheetSize.y );

    // Set the new sheet size.
    m_currentSheet->GetScreen()->SetPageSettings( pageInfo );

    pageSizeIU = m_currentSheet->GetScreen()->GetPageSettings().GetSizeIU();
    wxPoint sheetcentre( pageSizeIU.x / 2, pageSizeIU.y / 2 );
    wxPoint itemsCentre = sheetBoundingBox.Centre();

    // round the translation to nearest 100mil to place it on the grid.
    wxPoint translation = sheetcentre - itemsCentre;
    translation.x   = translation.x - translation.x % 100;
    translation.y   = translation.y - translation.y % 100;

    // Add global net labels for the named power input pins in this sheet
    for( SCH_ITEM* item = m_currentSheet->GetScreen()->GetDrawItems(); item; item = item->Next() )
    {
        if( item->Type() != SCH_COMPONENT_T )
            continue;

        addImplicitConnections( static_cast<SCH_COMPONENT*>( item ), m_currentSheet->GetScreen(), true );
    }

    m_connPoints.clear();

    // Translate the items.
    for( SCH_ITEM* item = m_currentSheet->GetScreen()->GetDrawItems(); item; item = item->Next() )
    {
        item->SetPosition( item->GetPosition() + translation );
        item->ClearFlags();
    }
}


void SCH_EAGLE_PLUGIN::loadSegments( wxXmlNode* aSegmentsNode, const wxString& netName,
        const wxString& aNetClass )
{
    // Loop through all segments
    wxXmlNode* currentSegment = aSegmentsNode->GetChildren();
    SCH_SCREEN* screen = m_currentSheet->GetScreen();

    int segmentCount = countChildren( aSegmentsNode, "segment" );

    // wxCHECK( screen, [>void<] );
    while( currentSegment )
    {
        bool labelled = false;    // has a label been added to this continously connected segment
        NODE_MAP segmentChildren = MapChildren( currentSegment );
        SCH_LINE* firstWire = nullptr;
        m_segments.emplace_back();
        SEG_DESC& segDesc = m_segments.back();

        // Loop through all segment children
        wxXmlNode* segmentAttribute = currentSegment->GetChildren();

        while( segmentAttribute )
        {
            if( segmentAttribute->GetName() == "wire" )
            {
                SCH_LINE* wire = loadWire( segmentAttribute );

                if( !firstWire )
                    firstWire = wire;

                // Test for intersections with other wires
                SEG thisWire( wire->GetStartPoint(), wire->GetEndPoint() );

                for( auto& desc : m_segments )
                {
                    if( !desc.labels.empty() && desc.labels.front()->GetText() == netName )
                        continue;   // no point in saving intersections of the same net

                    for( const auto& seg : desc.segs )
                    {
                        auto intersection = thisWire.Intersect( seg, true );

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

            if( nodeName == "junction" )
            {
                screen->Append( loadJunction( segmentAttribute ) );
            }
            else if( nodeName == "label" )
            {
                SCH_TEXT* label = loadLabel( segmentAttribute, netName );
                screen->Append( label );
                wxASSERT( segDesc.labels.empty() || segDesc.labels.front()->GetText() == label->GetText() );
                segDesc.labels.push_back( label );
                labelled = true;
            }
            else if( nodeName == "pinref" )
            {
                segmentAttribute->GetAttribute( "gate" );   // REQUIRED
                segmentAttribute->GetAttribute( "part" );   // REQUIRED
                segmentAttribute->GetAttribute( "pin" );    // REQUIRED
            }
            else if( nodeName == "wire" )
            {
                // already handled;
            }
            else    // DEFAULT
            {
                // THROW_IO_ERROR( wxString::Format( _( "XML node \"%s\" unknown" ), nodeName ) );
            }

            // Get next segment attribute
            segmentAttribute = segmentAttribute->GetNext();
        }

        // Add a small label to the net segment if it hasn't been labelled already
        // this preserves the named net feature of Eagle schematics.
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
                label->SetTextSize( wxSize( 10, 10 ) );
                label->SetLabelSpinStyle( 0 );
                screen->Append( label.release() );
            }
        }

        currentSegment = currentSegment->GetNext();
    }
}


SCH_LINE* SCH_EAGLE_PLUGIN::loadWire( wxXmlNode* aWireNode )
{
    std::unique_ptr<SCH_LINE> wire( new SCH_LINE );

    auto ewire = EWIRE( aWireNode );

    wire->SetLayer( kiCadLayer( ewire.layer ) );

    wxPoint begin, end;

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
    std::unique_ptr<SCH_JUNCTION> junction( new SCH_JUNCTION );

    auto ejunction = EJUNCTION( aJunction );
    wxPoint pos( ejunction.x.ToSchUnits(), -ejunction.y.ToSchUnits() );

    junction->SetPosition( pos );

    return junction.release();
}


SCH_TEXT* SCH_EAGLE_PLUGIN::loadLabel( wxXmlNode* aLabelNode, const wxString& aNetName )
{
    auto elabel = ELABEL( aLabelNode, aNetName );
    wxPoint elabelpos( elabel.x.ToSchUnits(), -elabel.y.ToSchUnits() );

    // Determine if the label is local or global depending on
    // the number of sheets the net appears in
    bool global = m_netCounts[aNetName] > 1;
    std::unique_ptr<SCH_TEXT> label;

    if( global )
        label.reset( new SCH_GLOBALLABEL );
    else
        label.reset( new SCH_LABEL );

    label->SetPosition( elabelpos );
    label->SetText( escapeName( elabel.netname ) );
    label->SetTextSize( wxSize( elabel.size.ToSchUnits(), elabel.size.ToSchUnits() ) );
    label->SetLabelSpinStyle( global ? 2 : 0 );

    if( elabel.rot )
    {
        int offset = global ? 2 : 0;
        label->SetLabelSpinStyle( int( elabel.rot->degrees / 90 + offset ) % 4 );

        if( elabel.rot->mirror
            && ( label->GetLabelSpinStyle() == 0 || label->GetLabelSpinStyle() == 2 ) )
            label->SetLabelSpinStyle( (label->GetLabelSpinStyle() + 2 ) % 4 );
    }

    return label.release();
}


std::pair<VECTOR2I, const SEG*> SCH_EAGLE_PLUGIN::findNearestLinePoint( const wxPoint& aPoint,
        const std::vector<SEG>& aLines ) const
{
    VECTOR2I nearestPoint;
    const SEG* nearestLine = nullptr;

    float d, mindistance = std::numeric_limits<float>::max();

    // Find the nearest start, middle or end of a line from the list of lines.
    for( const SEG& line : aLines )
    {
        auto testpoint = line.A;
        d = sqrt( abs( ( (aPoint.x - testpoint.x) ^ 2 ) + ( (aPoint.y - testpoint.y) ^ 2 ) ) );

        if( d < mindistance )
        {
            mindistance     = d;
            nearestPoint    = testpoint;
            nearestLine     = &line;
        }

        testpoint = line.Center();
        d = sqrt( abs( ( (aPoint.x - testpoint.x) ^ 2 ) + ( (aPoint.y - testpoint.y) ^ 2 ) ) );

        if( d < mindistance )
        {
            mindistance     = d;
            nearestPoint    = testpoint;
            nearestLine     = &line;
        }

        testpoint = line.B;
        d = sqrt( abs( ( (aPoint.x - testpoint.x) ^ 2 ) + ( (aPoint.y - testpoint.y) ^ 2 ) ) );

        if( d < mindistance )
        {
            mindistance     = d;
            nearestPoint    = testpoint;
            nearestLine     = &line;
        }
    }

    return std::make_pair( nearestPoint, nearestLine );
}


void SCH_EAGLE_PLUGIN::loadInstance( wxXmlNode* aInstanceNode )
{
    auto einstance = EINSTANCE( aInstanceNode );

    SCH_SCREEN* screen = m_currentSheet->GetScreen();

    // Find the part in the list for the sheet.
    // Assign the component its value from the part entry
    // Calculate the unit number from the gate entry of the instance
    // Assign the the LIB_ID from deviceset and device names

    auto part_it = m_partlist.find( einstance.part.Upper() );

    if( part_it == m_partlist.end() )
    {
        wxLogError( _( "Error parsing Eagle file.  "
                       "Could not find \"%s\" instance but it is referenced in the schematic." ),
                einstance.part );

        return;
    }

    EPART* epart = part_it->second.get();

    wxString libraryname = epart->library;
    wxString gatename = epart->deviceset + epart->device + einstance.gate;
    wxString symbolname = wxString( epart->deviceset + epart->device );
    symbolname.Replace( "*", "" );
    wxString kisymbolname = fixSymbolName( symbolname );

    int unit = m_eagleLibs[libraryname].GateUnit[gatename];

    wxString package;
    EAGLE_LIBRARY* elib = &m_eagleLibs[libraryname];

    auto p = elib->package.find( kisymbolname );

    if( p != elib->package.end() )
    {
        package = p->second;
    }

    LIB_ALIAS* alias = m_pi->LoadSymbol( getLibFileName().GetFullPath(), kisymbolname,
                                         m_properties.get() );

    if( !alias || !alias->GetPart() )
    {
        wxLogMessage( wxString::Format( _( "Could not find %s in the imported library" ), kisymbolname ) );
        return;
    }

    LIB_PART* part = alias->GetPart();
    LIB_ID libId( getLibName(), kisymbolname );
    std::unique_ptr<SCH_COMPONENT> component( new SCH_COMPONENT() );
    component->SetLibId( libId );
    component->SetUnit( unit );
    component->SetPosition( wxPoint( einstance.x.ToSchUnits(), -einstance.y.ToSchUnits() ) );
    component->GetField( FOOTPRINT )->SetText( package );
    component->SetTimeStamp( EagleModuleTstamp( einstance.part, epart->value ? *epart->value : "",
                    unit ) );

    if( einstance.rot )
    {
        component->SetOrientation( kiCadComponentRotation( einstance.rot->degrees ) );

        if( einstance.rot->mirror )
        {
            component->MirrorY( einstance.x.ToSchUnits() );
        }
    }


    LIB_FIELDS partFields;
    part->GetFields( partFields );

    for( auto const& field : partFields )
    {
        component->GetField( field.GetId() )->ImportValues( field );
        component->GetField( field.GetId() )->SetTextPos(
                component->GetPosition() + field.GetTextPos() );
    }

    // If there is no footprint assigned, then prepend the reference value
    // with a hash character to mute netlist updater complaints
    wxString reference = package.IsEmpty() ? '#' + einstance.part : einstance.part;

    // EAGLE allows references to be single digits.  This breaks KiCad netlisting, which requires
    // parts to have non-digit + digit annotation.  If the reference begins with a number,
    // we prepend 'UNK' (unknown) for the symbol designator
    if( reference.find_first_not_of( "0123456789" ) == wxString::npos )
        reference.Prepend( "UNK" );

    SCH_SHEET_PATH sheetpath;
    m_rootSheet->LocatePathOfScreen( screen, &sheetpath );
    wxString current_sheetpath = sheetpath.Path();

    wxString tstamp;
    tstamp.Printf( "%8.8lX", (unsigned long) component->GetTimeStamp() );
    current_sheetpath += tstamp;

    component->GetField( REFERENCE )->SetText( reference );
    component->AddHierarchicalReference( current_sheetpath, reference, unit );

    if( epart->value )
        component->GetField( VALUE )->SetText( *epart->value );
    else
        component->GetField( VALUE )->SetText( kisymbolname );

    // Set the visibility of fields.
    component->GetField( REFERENCE )->SetVisible( part->GetField( REFERENCE )->IsVisible() );
    component->GetField( VALUE )->SetVisible( part->GetField( VALUE )->IsVisible() );

    for( auto a:epart->attribute )
    {
        auto field = component->AddField( *component->GetField( VALUE ) );
        field->SetName( a.first );
        field->SetText( a.second );
        field->SetVisible( false );
    }

    for( auto a:epart->variant )
    {
        auto field = component->AddField( *component->GetField( VALUE ) );
        field->SetName( "VARIANT_" + a.first );
        field->SetText( a.second );
        field->SetVisible( false );
    }

    bool valueAttributeFound = false;
    bool nameAttributeFound  = false;


    wxXmlNode* attributeNode = aInstanceNode->GetChildren();

    // Parse attributes for the instance
    while( attributeNode )
    {
        if( attributeNode->GetName() == "attribute" )
        {
            auto attr = EATTR( attributeNode );
            SCH_FIELD* field = NULL;

            if( attr.name.Lower() == "name" )
            {
                field = component->GetField( REFERENCE );
                nameAttributeFound = true;
            }
            else if( attr.name.Lower() == "value" )
            {
                field = component->GetField( VALUE );
                valueAttributeFound = true;
            }
            else
            {
                field = component->FindField( attr.name );

                if(field)
                    field->SetVisible( false );
            }

            if( field )
            {

                field->SetPosition( wxPoint( attr.x->ToSchUnits(), -attr.y->ToSchUnits() ) );
                int align = attr.align ? *attr.align : ETEXT::BOTTOM_LEFT;
                int absdegrees = attr.rot ? attr.rot->degrees : 0;
                bool mirror = attr.rot ? attr.rot->mirror : false;

                if( einstance.rot && einstance.rot->mirror )
                    mirror = !mirror;

                bool spin = attr.rot ? attr.rot->spin : false;

                if( attr.display == EATTR::Off || attr.display == EATTR::NAME )
                    field->SetVisible( false );

                int rotation = einstance.rot ? einstance.rot->degrees : 0;
                int reldegrees = ( absdegrees - rotation + 360.0 );
                reldegrees %= 360;

                eagleToKicadAlignment( (EDA_TEXT*) field, align, reldegrees, mirror, spin,
                        absdegrees );
            }
        }
        else if( attributeNode->GetName() == "variant" )
        {
            wxString variant, value;

            if( attributeNode->GetAttribute( "name", &variant )
                && attributeNode->GetAttribute( "value", &value ) )
            {
                auto field = component->AddField( *component->GetField( VALUE ) );
                field->SetName( "VARIANT_" + variant );
                field->SetText( value );
                field->SetVisible( false );
            }
        }

        attributeNode = attributeNode->GetNext();
    }

    if( einstance.smashed && einstance.smashed.Get() )
    {
        if( !valueAttributeFound )
            component->GetField( VALUE )->SetVisible( false );

        if( !nameAttributeFound )
            component->GetField( REFERENCE )->SetVisible( false );
    }


    // Save the pin positions
    auto& schLibTable = *m_kiway->Prj().SchSymbolLibTable();
    wxCHECK( component->Resolve( schLibTable ), /*void*/ );
    std::vector<LIB_PIN*> pins;
    component->GetPins( pins );

    for( const auto& pin : pins )
        m_connPoints[component->GetPinPhysicalPosition( pin )].emplace( pin );


    component->ClearFlags();

    screen->Append( component.release() );
}


EAGLE_LIBRARY* SCH_EAGLE_PLUGIN::loadLibrary( wxXmlNode* aLibraryNode,
        EAGLE_LIBRARY* aEagleLibrary )
{
    NODE_MAP libraryChildren = MapChildren( aLibraryNode );

    // Loop through the symbols and load each of them
    wxXmlNode* symbolNode = getChildrenNodes( libraryChildren, "symbols" );

    while( symbolNode )
    {
        wxString symbolName = symbolNode->GetAttribute( "name" );
        aEagleLibrary->SymbolNodes[symbolName] = symbolNode;
        symbolNode = symbolNode->GetNext();
    }

    // Loop through the devicesets and load each of them
    wxXmlNode* devicesetNode = getChildrenNodes( libraryChildren, "devicesets" );

    while( devicesetNode )
    {
        // Get Device set information
        EDEVICE_SET edeviceset = EDEVICE_SET( devicesetNode );

        wxString prefix = edeviceset.prefix ? edeviceset.prefix.Get() : "";

        NODE_MAP aDeviceSetChildren = MapChildren( devicesetNode );
        wxXmlNode* deviceNode = getChildrenNodes( aDeviceSetChildren, "devices" );

        // For each device in the device set:
        while( deviceNode )
        {
            // Get device information
            EDEVICE edevice = EDEVICE( deviceNode );

            // Create symbol name from deviceset and device names.
            wxString symbolName = edeviceset.name + edevice.name;
            symbolName.Replace( "*", "" );
            wxASSERT( !symbolName.IsEmpty() );
            symbolName = fixSymbolName( symbolName );

            if( edevice.package )
                aEagleLibrary->package[symbolName] = edevice.package.Get();

            // Create KiCad symbol.
            unique_ptr<LIB_PART> kpart( new LIB_PART( symbolName ) );

            // Process each gate in the deviceset for this device.
            wxXmlNode* gateNode = getChildrenNodes( aDeviceSetChildren, "gates" );
            int gates_count = countChildren( aDeviceSetChildren["gates"], "gate" );
            kpart->SetUnitCount( gates_count );
            kpart->LockUnits( true );

            LIB_FIELD* reference = kpart->GetField( REFERENCE );

            if( prefix.length() == 0 )
                reference->SetVisible( false );
            else
                // If there is no footprint assigned, then prepend the reference value
                // with a hash character to mute netlist updater complaints
                reference->SetText( edevice.package ? prefix : '#' + prefix );

            int gateindex = 1;
            bool ispower = false;

            while( gateNode )
            {
                EGATE egate = EGATE( gateNode );

                aEagleLibrary->GateUnit[edeviceset.name + edevice.name + egate.name] = gateindex;

                ispower = loadSymbol( aEagleLibrary->SymbolNodes[egate.symbol],
                        kpart, &edevice, gateindex, egate.name );

                gateindex++;
                gateNode = gateNode->GetNext();
            }    // gateNode

            kpart->SetUnitCount( gates_count );

            if( gates_count == 1 && ispower )
                kpart->SetPower();

            wxString name = fixSymbolName( kpart->GetName() );
            kpart->SetName( name );
            m_pi->SaveSymbol( getLibFileName().GetFullPath(), new LIB_PART( *kpart.get() ),
                              m_properties.get() );
            aEagleLibrary->KiCadSymbols.insert( name, kpart.release() );

            deviceNode = deviceNode->GetNext();
        }    // devicenode

        devicesetNode = devicesetNode->GetNext();
    }    // devicesetNode

    return aEagleLibrary;
}


bool SCH_EAGLE_PLUGIN::loadSymbol( wxXmlNode* aSymbolNode, std::unique_ptr<LIB_PART>& aPart,
        EDEVICE* aDevice, int aGateNumber, const wxString& aGateName )
{
    wxString symbolName = aSymbolNode->GetAttribute( "name" );
    std::vector<LIB_ITEM*> items;

    wxXmlNode* currentNode = aSymbolNode->GetChildren();

    bool    foundName   = false;
    bool    foundValue  = false;
    bool    ispower     = false;
    int     pincount    = 0;

    while( currentNode )
    {
        wxString nodeName = currentNode->GetName();

        if( nodeName == "circle" )
        {
            aPart->AddDrawItem( loadSymbolCircle( aPart, currentNode, aGateNumber ) );
        }
        else if( nodeName == "pin" )
        {
            EPIN ePin = EPIN( currentNode );
            std::unique_ptr<LIB_PIN> pin( loadPin( aPart, currentNode, &ePin, aGateNumber ) );
            pincount++;

            pin->SetType( PIN_BIDI );

            if( ePin.direction )
            {
                const std::map<wxString, ELECTRICAL_PINTYPE> pinDirectionsMap =
                {
                    { "sup", PIN_POWER_IN },        { "pas", PIN_PASSIVE },
                    { "out", PIN_OUTPUT },          { "in", PIN_INPUT },
                    { "nc",  PIN_NC },              { "io", PIN_BIDI },
                    { "oc",  PIN_OPENCOLLECTOR },   { "hiz", PIN_TRISTATE },
                    { "pwr", PIN_POWER_IN },
                };

                for( const auto& pinDir : pinDirectionsMap )
                {
                    if( ePin.direction->Lower() == pinDir.first )
                    {
                        pin->SetType( pinDir.second );

                        if( pinDir.first == "sup" )         // power supply symbol
                            ispower = true;

                        break;
                    }
                }
            }


            if( aDevice->connects.size() != 0 )
            {
                for( auto connect : aDevice->connects )
                {
                    if( connect.gate == aGateName && pin->GetName() == connect.pin )
                    {
                        wxArrayString pads = wxSplit( wxString( connect.pad ), ' ');

                        pin->SetPartNumber( aGateNumber );
                        pin->SetUnit( aGateNumber );
                        pin->SetName( escapeName( pin->GetName() ) );

                        if( pads.GetCount() > 1)
                        {
                            pin->SetNumberTextSize( 0 );
                        }

                        // Eagle does not connect multiple NC pins together when they are stacked.
                        // KiCad will do this for pins that are coincident.  We opt here for correct
                        // schematic netlist and leave out the multiple NC pins when stacked.
                        for( unsigned i = 0; i < pads.GetCount(); i++)
                        {
                            if( pin->GetType() == PIN_NC && i > 0)
                                break;

                            LIB_PIN* apin = new LIB_PIN( *pin );

                            wxString padname( pads[i] );
                            apin->SetNumber( padname );
                            aPart->AddDrawItem( apin );
                        }

                        break;
                    }
                }
            }
            else
            {
                pin->SetPartNumber( aGateNumber );
                pin->SetUnit( aGateNumber );
                pin->SetNumber( wxString::Format( "%i", pincount ) );
                aPart->AddDrawItem( pin.release() );
            }
        }
        else if( nodeName == "polygon" )
        {
            aPart->AddDrawItem( loadSymbolPolyLine( aPart, currentNode, aGateNumber ) );
        }
        else if( nodeName == "rectangle" )
        {
            aPart->AddDrawItem( loadSymbolRectangle( aPart, currentNode, aGateNumber ) );
        }
        else if( nodeName == "text" )
        {
            std::unique_ptr<LIB_TEXT> libtext( loadSymbolText( aPart, currentNode, aGateNumber ) );

            if( libtext->GetText().Upper() ==">NAME" )
            {
                LIB_FIELD* field = aPart->GetField( REFERENCE );
                loadFieldAttributes( field, libtext.get() );
                foundName = true;
            }
            else if( libtext->GetText().Upper() ==">VALUE" )
            {
                LIB_FIELD* field = aPart->GetField( VALUE );
                loadFieldAttributes( field, libtext.get() );
                foundValue = true;
            }
            else
            {
                aPart->AddDrawItem( libtext.release() );
            }
        }
        else if( nodeName == "wire" )
        {
            aPart->AddDrawItem( loadSymbolWire( aPart, currentNode, aGateNumber ) );
        }

        /*
         *  else if( nodeName == "description" )
         *  {
         *  }
         *  else if( nodeName == "dimension" )
         *  {
         *  }
         *  else if( nodeName == "frame" )
         *  {
         *  }
         */

        currentNode = currentNode->GetNext();
    }

    if( foundName == false )
        aPart->GetField( REFERENCE )->SetVisible( false );

    if( foundValue == false )
        aPart->GetField( VALUE )->SetVisible( false );

    return pincount == 1 ? ispower : false;
}


LIB_CIRCLE* SCH_EAGLE_PLUGIN::loadSymbolCircle( std::unique_ptr<LIB_PART>& aPart,
        wxXmlNode* aCircleNode,
        int aGateNumber )
{
    // Parse the circle properties
    ECIRCLE c( aCircleNode );

    unique_ptr<LIB_CIRCLE> circle( new LIB_CIRCLE( aPart.get() ) );

    circle->SetPosition( wxPoint( c.x.ToSchUnits(), c.y.ToSchUnits() ) );
    circle->SetRadius( c.radius.ToSchUnits() );
    circle->SetWidth( c.width.ToSchUnits() );
    circle->SetUnit( aGateNumber );

    return circle.release();
}


LIB_RECTANGLE* SCH_EAGLE_PLUGIN::loadSymbolRectangle( std::unique_ptr<LIB_PART>& aPart,
        wxXmlNode* aRectNode,
        int aGateNumber )
{
    ERECT rect( aRectNode );

    unique_ptr<LIB_RECTANGLE> rectangle( new LIB_RECTANGLE( aPart.get() ) );

    rectangle->SetPosition( wxPoint( rect.x1.ToSchUnits(), rect.y1.ToSchUnits() ) );
    rectangle->SetEnd( wxPoint( rect.x2.ToSchUnits(), rect.y2.ToSchUnits() ) );

    rectangle->SetUnit( aGateNumber );
    // Eagle rectangles are filled by definition.
    rectangle->SetFillMode( FILLED_SHAPE );

    return rectangle.release();
}


LIB_ITEM* SCH_EAGLE_PLUGIN::loadSymbolWire( std::unique_ptr<LIB_PART>& aPart,
        wxXmlNode* aWireNode,
        int aGateNumber )
{
    auto ewire = EWIRE( aWireNode );

    wxPoint begin, end;

    begin.x = ewire.x1.ToSchUnits();
    begin.y = ewire.y1.ToSchUnits();
    end.x   = ewire.x2.ToSchUnits();
    end.y   = ewire.y2.ToSchUnits();

    if( begin == end )
        return nullptr;

    // if the wire is an arc
    if( ewire.curve )
    {
        std::unique_ptr<LIB_ARC> arc( new LIB_ARC( aPart.get() ) );
        wxPoint center = ConvertArcCenter( begin, end, *ewire.curve * -1 );

        double radius = sqrt( abs( ( ( center.x - begin.x ) * ( center.x - begin.x ) )
                        + ( ( center.y - begin.y ) * ( center.y - begin.y ) ) ) ) * 2;

        // this emulates the filled semicircles created by a thick arc with flat ends caps.
        if( ewire.width.ToSchUnits() * 2 > radius )
        {
            wxPoint centerStartVector = begin - center;
            wxPoint centerEndVector = end - center;

            centerStartVector.x = centerStartVector.x * ewire.width.ToSchUnits() * 2 / radius;
            centerStartVector.y = centerStartVector.y * ewire.width.ToSchUnits() * 2 / radius;

            centerEndVector.x = centerEndVector.x * ewire.width.ToSchUnits() * 2 / radius;
            centerEndVector.y = centerEndVector.y * ewire.width.ToSchUnits() * 2 / radius;

            begin = center + centerStartVector;
            end = center + centerEndVector;

            radius = sqrt( abs( ( ( center.x - begin.x ) * ( center.x - begin.x ) )
                            + ( ( center.y - begin.y ) * ( center.y - begin.y ) ) ) ) * 2;

            arc->SetWidth( 1 );
            arc->SetFillMode( FILLED_SHAPE );
        }
        else
        {
            arc->SetWidth( ewire.width.ToSchUnits() );
        }

        arc->SetPosition( center );

        if( *ewire.curve > 0 )
        {
            arc->SetStart( begin );
            arc->SetEnd( end );
        }
        else
        {
            arc->SetStart( end );
            arc->SetEnd( begin );
        }

        arc->SetRadius( radius );
        arc->CalcRadiusAngles();
        arc->SetUnit( aGateNumber );

        return (LIB_ITEM*) arc.release();
    }
    else
    {
        std::unique_ptr<LIB_POLYLINE> polyLine( new LIB_POLYLINE( aPart.get() ) );

        polyLine->AddPoint( begin );
        polyLine->AddPoint( end );
        polyLine->SetUnit( aGateNumber );

        return (LIB_ITEM*) polyLine.release();
    }
}


LIB_POLYLINE* SCH_EAGLE_PLUGIN::loadSymbolPolyLine( std::unique_ptr<LIB_PART>& aPart,
        wxXmlNode* aPolygonNode, int aGateNumber )
{
    std::unique_ptr<LIB_POLYLINE> polyLine( new LIB_POLYLINE( aPart.get() ) );

    EPOLYGON epoly( aPolygonNode );
    wxXmlNode* vertex = aPolygonNode->GetChildren();


    wxPoint pt;

    while( vertex )
    {
        if( vertex->GetName() == "vertex" )     // skip <xmlattr> node
        {
            EVERTEX evertex( vertex );
            pt = wxPoint( evertex.x.ToSchUnits(), evertex.y.ToSchUnits() );
            polyLine->AddPoint( pt );
        }

        vertex = vertex->GetNext();
    }

    polyLine->SetFillMode( FILLED_SHAPE );
    polyLine->SetUnit( aGateNumber );

    return polyLine.release();
}


LIB_PIN* SCH_EAGLE_PLUGIN::loadPin( std::unique_ptr<LIB_PART>& aPart,
        wxXmlNode* aPin,
        EPIN* aEPin,
        int aGateNumber )
{
    std::unique_ptr<LIB_PIN> pin( new LIB_PIN( aPart.get() ) );
    pin->SetPosition( wxPoint( aEPin->x.ToSchUnits(), aEPin->y.ToSchUnits() ) );
    pin->SetName( aEPin->name );
    pin->SetUnit( aGateNumber );

    int roti = aEPin->rot ? aEPin->rot->degrees : 0;

    switch( roti )
    {
    default:
        wxASSERT_MSG( false, wxString::Format( "Unhandled orientation (%d degrees)", roti ) );

    // fall through
    case 0:
        pin->SetOrientation( 'R' );
        break;

    case 90:
        pin->SetOrientation( 'U' );
        break;

    case 180:
        pin->SetOrientation( 'L' );
        break;

    case 270:
        pin->SetOrientation( 'D' );
        break;
    }

    if( aEPin->length )
    {
        wxString length = aEPin->length.Get();

        if( length =="short" )
        {
            pin->SetLength( 100 );
        }
        else if( length =="middle" )
        {
            pin->SetLength( 200 );
        }
        else if( length == "long" )
        {
            pin->SetLength( 300 );
        }
        else if( length == "point" )
        {
            pin->SetLength( 0 );
        }
    }

    // emulate the visibility of pin elements
    if( aEPin->visible )
    {
        wxString visible = aEPin->visible.Get();

        if( visible == "off" )
        {
            pin->SetNameTextSize( 0 );
            pin->SetNumberTextSize( 0 );
        }
        else if( visible == "pad" )
        {
            pin->SetNameTextSize( 0 );
        }
        else if( visible == "pin" )
        {
            pin->SetNumberTextSize( 0 );
        }

        /*
         *  else if( visible == "both" )
         *  {
         *  }
         */
    }

    if( aEPin->function )
    {
        wxString function = aEPin->function.Get();

        if( function == "dot" )
        {
            pin->SetShape( PINSHAPE_INVERTED );
        }
        else if( function == "clk" )
        {
            pin->SetShape( PINSHAPE_CLOCK );
        }
        else if( function == "dotclk" )
        {
            pin->SetShape( PINSHAPE_INVERTED_CLOCK );
        }
    }

    return pin.release();
}


LIB_TEXT* SCH_EAGLE_PLUGIN::loadSymbolText( std::unique_ptr<LIB_PART>& aPart,
        wxXmlNode* aLibText, int aGateNumber )
{
    std::unique_ptr<LIB_TEXT> libtext( new LIB_TEXT( aPart.get() ) );
    ETEXT etext( aLibText );

    libtext->SetUnit( aGateNumber );
    libtext->SetPosition( wxPoint( etext.x.ToSchUnits(), etext.y.ToSchUnits() ) );

    // Eagle supports multiple line text in library symbols.  Legacy library symbol text cannot
    // contain CRs or LFs.
    //
    // @todo Split this into multiple text objects and offset the Y position so that it looks
    //       more like the original Eagle schematic.
    wxString text = aLibText->GetNodeContent();
    std::replace( text.begin(), text.end(), '\n', '_' );
    std::replace( text.begin(), text.end(), '\r', '_' );

    libtext->SetText( text.IsEmpty() ? "~~" : text );
    loadTextAttributes( libtext.get(), etext );

    return libtext.release();
}


SCH_TEXT* SCH_EAGLE_PLUGIN::loadPlainText( wxXmlNode* aSchText )
{
    std::unique_ptr<SCH_TEXT> schtext( new SCH_TEXT() );
    ETEXT etext = ETEXT( aSchText );

    const wxString& thetext = aSchText->GetNodeContent();
    schtext->SetText( thetext.IsEmpty() ? "\" \"" : escapeName( thetext ) );
    schtext->SetPosition( wxPoint( etext.x.ToSchUnits(), -etext.y.ToSchUnits() ) );
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
            aText->SetThickness( GetPenSizeForBold( aText->GetTextWidth() ) );
        }
    }

    int align = aAttribs.align ? *aAttribs.align : ETEXT::BOTTOM_LEFT;
    int degrees = aAttribs.rot ? aAttribs.rot->degrees : 0;
    bool mirror = aAttribs.rot ? aAttribs.rot->mirror : false;
    bool spin = aAttribs.rot ? aAttribs.rot->spin : false;

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

    auto onIntersection = [&]( const VECTOR2I& aPos )
    {
        return std::binary_search( m_wireIntersections.begin(), m_wireIntersections.end(), aPos );
    };

    for( auto& segDesc : m_segments )
    {
        for( SCH_TEXT* label : segDesc.labels )
        {
            VECTOR2I labelPos( label->GetPosition() );
            const SEG* segAttached = segDesc.LabelAttached( label );

            if( segAttached && !onIntersection( labelPos ) )
                continue;       // label is placed correctly


            // Move the label to the nearest wire
            if( !segAttached )
            {
                std::tie( labelPos, segAttached ) = findNearestLinePoint( label->GetPosition(), segDesc.segs );

                if( !segAttached )     // we cannot do anything
                    continue;
            }


            // Create a vector pointing in the direction of the wire, 50 mils long
            VECTOR2I wireDirection( segAttached->B - segAttached->A );
            wireDirection = wireDirection.Resize( 50 );
            const VECTOR2I origPos( labelPos );

            // Flags determining the search direction
            bool checkPositive = true, checkNegative = true, move = false;
            int trial = 0;

            // Be sure the label is not placed on a wire intersection
            while( ( !move || onIntersection( labelPos ) ) && ( checkPositive || checkNegative ) )
            {
                move = false;

                // Move along the attached wire to find the new label position
                if( trial % 2 == 1 )
                {
                    labelPos = wxPoint( origPos + wireDirection * trial / 2 );
                    move = checkPositive = segAttached->Contains( labelPos );
                }
                else
                {
                    labelPos = wxPoint( origPos - wireDirection * trial / 2 );
                    move = checkNegative = segAttached->Contains( labelPos );
                }

                ++trial;
            }

            if( move )
                label->SetPosition( wxPoint( labelPos ) );
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
    firstline = tempFile.GetFirstLine();
    wxString    secondline  = tempFile.GetNextLine();
    wxString    thirdline   = tempFile.GetNextLine();
    tempFile.Close();

    return firstline.StartsWith( "<?xml" ) && secondline.StartsWith( "<!DOCTYPE eagle SYSTEM" )
           && thirdline.StartsWith( "<eagle version" );
}


void SCH_EAGLE_PLUGIN::moveLabels( SCH_ITEM* aWire, const wxPoint& aNewEndPoint )
{
    for( SCH_ITEM* item = m_currentSheet->GetScreen()->GetDrawItems(); item; item = item->Next() )
    {
        if( item->Type() == SCH_LABEL_T || item->Type() == SCH_GLOBAL_LABEL_T )
        {
            if( TestSegmentHit( item->GetPosition(), ( (SCH_LINE*) aWire )->GetStartPoint(),
                        ( (SCH_LINE*) aWire )->GetEndPoint(), 0 ) )
            {
                item->SetPosition( aNewEndPoint );
            }
        }
    }
}


void SCH_EAGLE_PLUGIN::addBusEntries()
{
    // Add bus entry symbols

    // for each wire segment, compare each end with all busess.
    // If the wire end is found to end on a bus segment, place a bus entry symbol.

    for( SCH_ITEM* bus = m_currentSheet->GetScreen()->GetDrawItems(); bus; bus = bus->Next() )
    {
        // Check line type for line
        if( bus->Type() != SCH_LINE_T )
            continue;

        // Check line type for wire
        if( ( (SCH_LINE*) bus )->GetLayer() != LAYER_BUS )
            continue;


        wxPoint busstart = ( (SCH_LINE*) bus )->GetStartPoint();
        wxPoint busend = ( (SCH_LINE*) bus )->GetEndPoint();

        SCH_ITEM* nextline;

        for( SCH_ITEM* line = m_currentSheet->GetScreen()->GetDrawItems(); line; line = nextline )
        {
            nextline = line->Next();

            // Check line type for line
            if( line->Type() == SCH_LINE_T )
            {
                // Check line type for bus
                if( ( (SCH_LINE*) line )->GetLayer() == LAYER_WIRE )
                {
                    // Get points of both segments.

                    wxPoint linestart = ( (SCH_LINE*) line )->GetStartPoint();
                    wxPoint lineend = ( (SCH_LINE*) line )->GetEndPoint();


                    // Test for horizontal wire and         vertical bus
                    if( linestart.y == lineend.y && busstart.x == busend.x )
                    {
                        if( TestSegmentHit( linestart, busstart, busend, 0 ) )
                        {
                            // Wire start is on a bus.
                            // Wire start is on the vertical bus

                            // if the end of the wire is to the left of the bus
                            if( lineend.x < busstart.x )
                            {
                                // |
                                // ---|
                                // |
                                if( TestSegmentHit( linestart + wxPoint( 0, -100 ), busstart,
                                            busend, 0 ) )
                                {
                                    SCH_BUS_WIRE_ENTRY* busEntry = new SCH_BUS_WIRE_ENTRY( linestart + wxPoint(
                                                    -100,
                                                    0 ),
                                            '/' );
                                    busEntry->SetFlags( IS_NEW );
                                    m_currentSheet->GetScreen()->Append( busEntry );
                                    moveLabels( line, linestart + wxPoint( -100, 0 ) );
                                    ( (SCH_LINE*) line )->SetStartPoint( linestart +
                                            wxPoint( -100, 0 ) );
                                }
                                else if( TestSegmentHit( linestart + wxPoint( 0, 100 ), busstart,
                                                 busend, 0 ) )
                                {
                                    SCH_BUS_WIRE_ENTRY* busEntry = new SCH_BUS_WIRE_ENTRY( linestart + wxPoint(
                                                    -100,
                                                    0 ),
                                            '\\' );
                                    busEntry->SetFlags( IS_NEW );
                                    m_currentSheet->GetScreen()->Append( busEntry );
                                    moveLabels( line, linestart + wxPoint( -100, 0 ) );
                                    ( (SCH_LINE*) line )->SetStartPoint( linestart +
                                            wxPoint( -100, 0 ) );
                                }
                                else
                                {
                                    SCH_MARKER* marker = new SCH_MARKER( linestart,
                                            "Bus Entry needed" );

                                    m_currentSheet->GetScreen()->Append( marker );
                                }
                            }
                            // else the wire end is to the right of the bus
                            // Wire is to the right of the bus
                            // |
                            // |----
                            // |
                            else
                            {
                                // test is bus exists above the wire
                                if( TestSegmentHit( linestart + wxPoint( 0, -100 ), busstart,
                                            busend, 0 ) )
                                {
                                    SCH_BUS_WIRE_ENTRY* busEntry = new SCH_BUS_WIRE_ENTRY( linestart + wxPoint(
                                                    0,
                                                    -100 ),
                                            '\\' );
                                    busEntry->SetFlags( IS_NEW );
                                    m_currentSheet->GetScreen()->Append( busEntry );
                                    moveLabels( line, linestart + wxPoint( 100, 0 ) );
                                    ( (SCH_LINE*) line )->SetStartPoint( linestart + wxPoint( 100,
                                                    0 ) );
                                }
                                // test is bus exists below the wire
                                else if( TestSegmentHit( linestart + wxPoint( 0, 100 ), busstart,
                                                 busend, 0 ) )
                                {
                                    SCH_BUS_WIRE_ENTRY* busEntry = new SCH_BUS_WIRE_ENTRY( linestart + wxPoint(
                                                    0,
                                                    100 ),
                                            '/' );
                                    busEntry->SetFlags( IS_NEW );
                                    m_currentSheet->GetScreen()->Append( busEntry );
                                    moveLabels( line, linestart + wxPoint( 100, 0 ) );
                                    ( (SCH_LINE*) line )->SetStartPoint( linestart + wxPoint( 100,
                                                    0 ) );
                                }
                                else
                                {
                                    SCH_MARKER* marker = new SCH_MARKER( linestart,
                                            "Bus Entry needed" );

                                    m_currentSheet->GetScreen()->Append( marker );
                                }
                            }
                        }

                        // Same thing but test end of the wire instead.
                        if( TestSegmentHit( lineend, busstart, busend, 0 ) )
                        {
                            // Wire end is on the vertical bus

                            // if the start of the wire is to the left of the bus
                            if( linestart.x < busstart.x )
                            {
                                // Test if bus exists above the wire
                                if( TestSegmentHit( lineend + wxPoint( 0, 100 ), busstart, busend,
                                            0 ) )
                                {
                                    // |
                                    // ___/|
                                    SCH_BUS_WIRE_ENTRY* busEntry = new SCH_BUS_WIRE_ENTRY( lineend + wxPoint(
                                                    -100,
                                                    0 ),
                                            '\\' );
                                    busEntry->SetFlags( IS_NEW );
                                    m_currentSheet->GetScreen()->Append( busEntry );
                                    moveLabels( line, lineend + wxPoint( -100, 0 ) );
                                    ( (SCH_LINE*) line )->SetEndPoint( lineend +
                                            wxPoint( -100, 0 ) );
                                }
                                // Test if bus exists below the wire
                                else if( TestSegmentHit( lineend + wxPoint( 0, -100 ), busstart,
                                                 busend, 0 ) )
                                {
                                    SCH_BUS_WIRE_ENTRY* busEntry = new SCH_BUS_WIRE_ENTRY( lineend + wxPoint(
                                                    -100,
                                                    0 ),
                                            '/' );
                                    busEntry->SetFlags( IS_NEW );
                                    m_currentSheet->GetScreen()->Append( busEntry );
                                    moveLabels( line, lineend + wxPoint( -100, 0 ) );
                                    ( (SCH_LINE*) line )->SetEndPoint( lineend +
                                            wxPoint( -100, 0 ) );
                                }
                                else
                                {
                                    SCH_MARKER* marker = new SCH_MARKER( lineend,
                                            "Bus Entry needed" );

                                    m_currentSheet->GetScreen()->Append( marker );
                                }
                            }
                            // else the start of the wire is to the right of the bus
                            // |
                            // |----
                            // |
                            else
                            {
                                // test if bus existed above the wire
                                if( TestSegmentHit( lineend + wxPoint( 0, -100 ), busstart,
                                            busend, 0 ) )
                                {
                                    SCH_BUS_WIRE_ENTRY* busEntry = new SCH_BUS_WIRE_ENTRY( lineend + wxPoint(
                                                    0,
                                                    -100 ),
                                            '\\' );
                                    busEntry->SetFlags( IS_NEW );
                                    m_currentSheet->GetScreen()->Append( busEntry );
                                    moveLabels( line, lineend + wxPoint( 100, 0 ) );
                                    ( (SCH_LINE*) line )->SetEndPoint( lineend +
                                            wxPoint( 100, 0 ) );
                                }
                                // test if bus existed below the wire
                                else if( TestSegmentHit( lineend + wxPoint( 0, 100 ), busstart,
                                                 busend, 0 ) )
                                {
                                    SCH_BUS_WIRE_ENTRY* busEntry = new SCH_BUS_WIRE_ENTRY( lineend + wxPoint(
                                                    0,
                                                    100 ),
                                            '/' );
                                    busEntry->SetFlags( IS_NEW );
                                    m_currentSheet->GetScreen()->Append( busEntry );
                                    moveLabels( line, lineend + wxPoint( 100, 0 ) );
                                    ( (SCH_LINE*) line )->SetEndPoint( lineend +
                                            wxPoint( 100, 0 ) );
                                }
                                else
                                {
                                    SCH_MARKER* marker = new SCH_MARKER( lineend,
                                            "Bus Entry needed" );

                                    m_currentSheet->GetScreen()->Append( marker );
                                }
                            }
                        }
                    }    // if( linestart.y == lineend.y && busstart.x == busend.x)

                    // Test for horizontal wire and vertical bus
                    if( linestart.x == lineend.x && busstart.y == busend.y )
                    {
                        if( TestSegmentHit( linestart, busstart, busend, 0 ) )
                        {
                            // Wire start is on the bus
                            // If wire end is above the bus,
                            if( lineend.y < busstart.y )
                            {
                                // Test for bus existance to the left of the wire
                                if( TestSegmentHit( linestart + wxPoint( -100, 0 ), busstart,
                                            busend, 0 ) )
                                {
                                    SCH_BUS_WIRE_ENTRY* busEntry = new SCH_BUS_WIRE_ENTRY( linestart + wxPoint(
                                                    -100,
                                                    0 ),
                                            '/' );
                                    busEntry->SetFlags( IS_NEW );
                                    m_currentSheet->GetScreen()->Append( busEntry );
                                    moveLabels( line, linestart + wxPoint( 0, -100 ) );
                                    ( (SCH_LINE*) line )->SetStartPoint( linestart +
                                            wxPoint( 0, -100 ) );
                                }
                                else if( TestSegmentHit( linestart + wxPoint( 100, 0 ), busstart,
                                                 busend, 0 ) )
                                {
                                    SCH_BUS_WIRE_ENTRY* busEntry = new SCH_BUS_WIRE_ENTRY( linestart + wxPoint(
                                                    0,
                                                    100 ),
                                            '\\' );
                                    busEntry->SetFlags( IS_NEW );
                                    m_currentSheet->GetScreen()->Append( busEntry );
                                    moveLabels( line, linestart + wxPoint( 0, -100 ) );
                                    ( (SCH_LINE*) line )->SetStartPoint( linestart +
                                            wxPoint( 0, -100 ) );
                                }
                                else
                                {
                                    SCH_MARKER* marker = new SCH_MARKER( linestart,
                                            "Bus Entry needed" );

                                    m_currentSheet->GetScreen()->Append( marker );
                                }
                            }
                            else    // wire end is below the bus.
                            {
                                // Test for bus existance to the left of the wire
                                if( TestSegmentHit( linestart + wxPoint( -100, 0 ), busstart,
                                            busend, 0 ) )
                                {
                                    SCH_BUS_WIRE_ENTRY* busEntry = new SCH_BUS_WIRE_ENTRY( linestart + wxPoint(
                                                    -100,
                                                    0 ),
                                            '\\' );
                                    busEntry->SetFlags( IS_NEW );
                                    m_currentSheet->GetScreen()->Append( busEntry );
                                    moveLabels( line, linestart + wxPoint( 0, 100 ) );
                                    ( (SCH_LINE*) line )->SetStartPoint( linestart + wxPoint( 0,
                                                    100 ) );
                                }
                                else if( TestSegmentHit( linestart + wxPoint( 100, 0 ), busstart,
                                                 busend, 0 ) )
                                {
                                    SCH_BUS_WIRE_ENTRY* busEntry = new SCH_BUS_WIRE_ENTRY( linestart + wxPoint(
                                                    100,
                                                    0 ),
                                            '/' );
                                    busEntry->SetFlags( IS_NEW );
                                    m_currentSheet->GetScreen()->Append( busEntry );
                                    moveLabels( line, linestart + wxPoint( 0, 100 ) );
                                    ( (SCH_LINE*) line )->SetStartPoint( linestart + wxPoint( 0,
                                                    100 ) );
                                }
                                else
                                {
                                    SCH_MARKER* marker = new SCH_MARKER( linestart,
                                            "Bus Entry needed" );

                                    m_currentSheet->GetScreen()->Append( marker );
                                }
                            }
                        }

                        if( TestSegmentHit( lineend, busstart, busend, 0 ) )
                        {
                            // Wire end is on the bus
                            // If wire start is above the bus,

                            if( linestart.y < busstart.y )
                            {
                                // Test for bus existance to the left of the wire
                                if( TestSegmentHit( lineend + wxPoint( -100, 0 ), busstart,
                                            busend, 0 ) )
                                {
                                    SCH_BUS_WIRE_ENTRY* busEntry = new SCH_BUS_WIRE_ENTRY( lineend + wxPoint(
                                                    -100,
                                                    0 ),
                                            '/' );
                                    busEntry->SetFlags( IS_NEW );
                                    m_currentSheet->GetScreen()->Append( busEntry );
                                    moveLabels( line, lineend + wxPoint( 0, -100 ) );
                                    ( (SCH_LINE*) line )->SetEndPoint( lineend +
                                            wxPoint( 0, -100 ) );
                                }
                                else if( TestSegmentHit( lineend + wxPoint( 100, 0 ), busstart,
                                                 busend, 0 ) )
                                {
                                    SCH_BUS_WIRE_ENTRY* busEntry = new SCH_BUS_WIRE_ENTRY( lineend + wxPoint(
                                                    0,
                                                    -100 ),
                                            '\\' );
                                    busEntry->SetFlags( IS_NEW );
                                    m_currentSheet->GetScreen()->Append( busEntry );
                                    moveLabels( line, lineend + wxPoint( 0, -100 ) );
                                    ( (SCH_LINE*) line )->SetEndPoint( lineend +
                                            wxPoint( 0, -100 ) );
                                }
                                else
                                {
                                    SCH_MARKER* marker = new SCH_MARKER( lineend,
                                            "Bus Entry needed" );

                                    m_currentSheet->GetScreen()->Append( marker );
                                }
                            }
                            else    // wire end is below the bus.
                            {
                                // Test for bus existance to the left of the wire
                                if( TestSegmentHit( lineend + wxPoint( -100, 0 ), busstart,
                                            busend, 0 ) )
                                {
                                    SCH_BUS_WIRE_ENTRY* busEntry = new SCH_BUS_WIRE_ENTRY( lineend + wxPoint(
                                                    -100,
                                                    0 ),
                                            '\\' );
                                    busEntry->SetFlags( IS_NEW );
                                    m_currentSheet->GetScreen()->Append( busEntry );
                                    moveLabels( line, lineend + wxPoint( 0, 100 ) );
                                    ( (SCH_LINE*) line )->SetEndPoint( lineend +
                                            wxPoint( 0, 100 ) );
                                }
                                else if( TestSegmentHit( lineend + wxPoint( 100, 0 ), busstart,
                                                 busend, 0 ) )
                                {
                                    SCH_BUS_WIRE_ENTRY* busEntry = new SCH_BUS_WIRE_ENTRY( lineend + wxPoint(
                                                    0,
                                                    100 ),
                                            '/' );
                                    busEntry->SetFlags( IS_NEW );
                                    m_currentSheet->GetScreen()->Append( busEntry );
                                    moveLabels( line, lineend + wxPoint( 0, 100 ) );
                                    ( (SCH_LINE*) line )->SetEndPoint( lineend +
                                            wxPoint( 0, 100 ) );
                                }
                                else
                                {
                                    SCH_MARKER* marker = new SCH_MARKER( lineend,
                                            "Bus Entry needed" );

                                    m_currentSheet->GetScreen()->Append( marker );
                                }
                            }
                        }
                    }

                    linestart = ( (SCH_LINE*) line )->GetStartPoint();
                    lineend     = ( (SCH_LINE*) line )->GetEndPoint();
                    busstart    = ( (SCH_LINE*) bus )->GetStartPoint();
                    busend = ( (SCH_LINE*) bus )->GetEndPoint();


                    // bus entry wire isn't horizontal or vertical
                    if( TestSegmentHit( linestart, busstart, busend, 0 ) )
                    {
                        wxPoint wirevector = linestart - lineend;

                        if( wirevector.x > 0 )
                        {
                            if( wirevector.y > 0 )
                            {
                                wxPoint p = linestart + wxPoint( -100, -100 );
                                SCH_BUS_WIRE_ENTRY* busEntry = new SCH_BUS_WIRE_ENTRY( p, '\\' );
                                busEntry->SetFlags( IS_NEW );
                                m_currentSheet->GetScreen()->Append( busEntry );
                                moveLabels( line, p );

                                if( p == lineend )    // wire is overlapped by bus entry symbol
                                {
                                    m_currentSheet->GetScreen()->DeleteItem( line );
                                    line = nullptr;
                                }
                                else
                                {
                                    ( (SCH_LINE*) line )->SetStartPoint( p );
                                }
                            }
                            else
                            {
                                wxPoint p = linestart + wxPoint( -100, 100 );
                                SCH_BUS_WIRE_ENTRY* busEntry = new SCH_BUS_WIRE_ENTRY( p, '/' );
                                busEntry->SetFlags( IS_NEW );
                                m_currentSheet->GetScreen()->Append( busEntry );

                                moveLabels( line, p );

                                if( p == lineend )    // wire is overlapped by bus entry symbol
                                {
                                    m_currentSheet->GetScreen()->DeleteItem( line );
                                    line = nullptr;
                                }
                                else
                                {
                                    ( (SCH_LINE*) line )->SetStartPoint( p );
                                }
                            }
                        }
                        else
                        {
                            if( wirevector.y > 0 )
                            {
                                SCH_BUS_WIRE_ENTRY* busEntry = new SCH_BUS_WIRE_ENTRY( linestart,
                                        '/' );
                                busEntry->SetFlags( IS_NEW );
                                m_currentSheet->GetScreen()->Append( busEntry );

                                moveLabels( line, linestart + wxPoint( 100, -100 ) );

                                if( linestart + wxPoint( 100, -100 )== lineend )    // wire is overlapped by bus entry symbol
                                {
                                    m_currentSheet->GetScreen()->DeleteItem( line );
                                    line = nullptr;
                                }
                                else
                                {
                                    ( (SCH_LINE*) line )->SetStartPoint( linestart +
                                            wxPoint( 100, -100 ) );
                                }
                            }
                            else
                            {
                                SCH_BUS_WIRE_ENTRY* busEntry = new SCH_BUS_WIRE_ENTRY( linestart,
                                        '\\' );
                                busEntry->SetFlags( IS_NEW );
                                m_currentSheet->GetScreen()->Append( busEntry );
                                moveLabels( line, linestart + wxPoint( 100, 100 ) );

                                if( linestart + wxPoint( 100, 100 )== lineend )    // wire is overlapped by bus entry symbol
                                {
                                    m_currentSheet->GetScreen()->DeleteItem( line );
                                    line = nullptr;
                                }
                                else
                                {
                                    ( (SCH_LINE*) line )->SetStartPoint( linestart +
                                            wxPoint( 100, 100 ) );
                                }
                            }
                        }
                    }

                    if( line && TestSegmentHit( lineend, busstart, busend, 0 ) )
                    {
                        wxPoint wirevector = linestart - lineend;

                        if( wirevector.x > 0 )
                        {
                            if( wirevector.y > 0 )
                            {
                                wxPoint p = lineend + wxPoint( 100, 100 );
                                SCH_BUS_WIRE_ENTRY* busEntry =
                                    new SCH_BUS_WIRE_ENTRY( lineend, '\\' );
                                busEntry->SetFlags( IS_NEW );
                                m_currentSheet->GetScreen()->Append( busEntry );

                                moveLabels( line, p );

                                if( p == linestart )    // wire is overlapped by bus entry symbol
                                {
                                    m_currentSheet->GetScreen()->DeleteItem( line );
                                }
                                else
                                {
                                    ( (SCH_LINE*) line )->SetEndPoint( p );
                                }
                            }
                            else
                            {
                                wxPoint p = lineend + wxPoint( 100, -100 );
                                SCH_BUS_WIRE_ENTRY* busEntry =
                                    new SCH_BUS_WIRE_ENTRY( lineend, '/' );
                                busEntry->SetFlags( IS_NEW );
                                m_currentSheet->GetScreen()->Append( busEntry );

                                moveLabels( line, p );

                                if( p== linestart )    // wire is overlapped by bus entry symbol
                                {
                                    m_currentSheet->GetScreen()->DeleteItem( line );
                                }
                                else
                                {
                                    ( (SCH_LINE*) line )->SetEndPoint( p );
                                }
                            }
                        }
                        else
                        {
                            if( wirevector.y > 0 )
                            {
                                wxPoint p = lineend + wxPoint( -100, 100 );
                                SCH_BUS_WIRE_ENTRY* busEntry =
                                    new SCH_BUS_WIRE_ENTRY( p, '/' );
                                busEntry->SetFlags( IS_NEW );
                                m_currentSheet->GetScreen()->Append( busEntry );
                                moveLabels( line, p );

                                if( p == linestart )    // wire is overlapped by bus entry symbol
                                {
                                    m_currentSheet->GetScreen()->DeleteItem( line );
                                }
                                else
                                {
                                    ( (SCH_LINE*) line )->SetEndPoint( p );
                                }
                            }
                            else
                            {
                                wxPoint p = lineend + wxPoint( -100, -100 );
                                SCH_BUS_WIRE_ENTRY* busEntry =
                                    new SCH_BUS_WIRE_ENTRY( p, '\\' );
                                busEntry->SetFlags( IS_NEW );
                                m_currentSheet->GetScreen()->Append( busEntry );
                                moveLabels( line, p );

                                if( p == linestart )    // wire is overlapped by bus entry symbol
                                {
                                    m_currentSheet->GetScreen()->DeleteItem( line );
                                }
                                else
                                {
                                    ( (SCH_LINE*) line )->SetEndPoint( p );
                                }
                            }
                        }
                    }
                }
            }
        }   // for ( line ..
    }       // for ( bus ..
}


const SEG* SCH_EAGLE_PLUGIN::SEG_DESC::LabelAttached( const SCH_TEXT* aLabel ) const
{
    VECTOR2I labelPos( aLabel->GetPosition() );

    for( const auto& seg : segs )
    {
        if( seg.Contains( labelPos ) )
            return &seg;
    }

    return nullptr;
}


// TODO could be used to place junctions, instead of IsJunctionNeeded() (see SCH_EDIT_FRAME::importFile())
bool SCH_EAGLE_PLUGIN::checkConnections( const SCH_COMPONENT* aComponent, const LIB_PIN* aPin ) const
{
    wxPoint pinPosition = aComponent->GetPinPhysicalPosition( aPin );
    auto pointIt = m_connPoints.find( pinPosition );

    if( pointIt == m_connPoints.end() )
        return false;

    const auto& items = pointIt->second;
    wxASSERT( items.find( aPin ) != items.end() );
    return items.size() > 1;
}


void SCH_EAGLE_PLUGIN::addImplicitConnections( SCH_COMPONENT* aComponent,
        SCH_SCREEN* aScreen, bool aUpdateSet )
{
    auto partRef = aComponent->GetPartRef().lock();
    wxCHECK( partRef, /*void*/ );

    // Normally power parts also have power input pins,
    // but they already force net names on the attached wires
    if( partRef->IsPower() )
        return;

    int unit = aComponent->GetUnit();
    const wxString reference = aComponent->GetField( REFERENCE )->GetText();
    std::vector<LIB_PIN*> pins;
    partRef->GetPins( pins );
    std::set<int> missingUnits;

    // Search all units for pins creating implicit connections
    for( const auto& pin : pins )
    {
        if( pin->GetType() == PIN_POWER_IN )
        {
            bool pinInUnit = !unit || pin->GetUnit() == unit;   // pin belongs to the tested unit

            // Create a global net label only if there are no other wires/pins attached
            if( pinInUnit && !checkConnections( aComponent, pin ) )
            {
                // Create a net label to force the net name on the pin
                SCH_GLOBALLABEL* netLabel = new SCH_GLOBALLABEL;
                netLabel->SetPosition( aComponent->GetPinPhysicalPosition( pin ) );
                netLabel->SetText( extractNetName( pin->GetName() ) );
                netLabel->SetTextSize( wxSize( 10, 10 ) );
                netLabel->SetLabelSpinStyle( 0 );
                aScreen->Append( netLabel );
            }

            else if( !pinInUnit && aUpdateSet )
            {
                // Found a pin creating implicit connection information in another unit.
                // Such units will be instantiated if they do not appear in another sheet and
                // processed later.
                wxASSERT( pin->GetUnit() );
                missingUnits.insert( pin->GetUnit() );
            }
        }
    }

    if( aUpdateSet )
    {
        auto cmpIt = m_missingCmps.find( reference );

        // Set the flag indicating this unit has been processed
        if( cmpIt != m_missingCmps.end() )
            cmpIt->second.units[unit] = false;

        // Save the units that need later processing
        else if( !missingUnits.empty() )
        {
            EAGLE_MISSING_CMP& entry = m_missingCmps[reference];
            entry.cmp = aComponent;

            for( int i : missingUnits )
                entry.units.emplace( i, true );
        }
    }
}


wxString SCH_EAGLE_PLUGIN::fixSymbolName( const wxString& aName )
{
    wxString ret = LIB_ID::FixIllegalChars( aName, LIB_ID::ID_SCH );

    return ret;
}


wxString SCH_EAGLE_PLUGIN::translateEagleBusName( const wxString& aEagleName ) const
{
    if( SCH_CONNECTION::IsBusVectorLabel( aEagleName ) )
        return aEagleName;

    wxString ret = "{";

    wxStringTokenizer tokenizer( aEagleName, "," );

    while( tokenizer.HasMoreTokens() )
    {
        wxString member = tokenizer.GetNextToken();

        // In Eagle, overbar text is automatically stopped at the end of the net name, even when
        // that net name is part of a bus definition.  In KiCad, we don't (currently) do that, so
        // if there is an odd number of overbar markers in this net name, we need to append one
        // to close it out before appending the space.

        if( member.Freq( '!' ) % 2 > 0 )
            member << "!";

        ret << member << " ";
    }

    ret.Trim( true );
    ret << "}";

    return ret;
}
