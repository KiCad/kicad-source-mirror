/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2017 CERN
 * @author Alejandro García Montoro <alejandro.garciamontoro@gmail.com>
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
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <properties.h>
#include <kiway.h>

#include <wx/filename.h>
#include <memory>
#include <string>
#include <unordered_map>

#include <sch_junction.h>
#include <sch_sheet.h>
#include <schframe.h>
#include <template_fieldnames.h>
#include <wildcards_and_files_ext.h>
#include <class_sch_screen.h>
#include <class_library.h>
#include <class_libentry.h>
#include <lib_draw_item.h>
#include <sch_component.h>
#include <lib_arc.h>
#include <lib_circle.h>
#include <lib_rectangle.h>
#include <lib_polyline.h>
#include <lib_pin.h>
#include <lib_text.h>
#include <sch_text.h>
#include <drawtxt.h>
#include <sch_marker.h>
#include <sch_bus_entry.h>
#include <eagle_parser.h>
#include <sch_eagle_plugin.h>


using std::string;

// Eagle schematic internal units are millimeters
// Kicad schematic units are thousandths of an inch
constexpr double EUNIT_TO_MIL = 1000.0 / 25.4;

// Eagle schematic axes are aligned with x increasing left to right and Y increasing bottom to top
// Kicad schematic axes are aligned with x increasing left to rigth and Y increasing top to bottom.

using namespace std;

static NODE_MAP mapChildren( wxXmlNode* aCurrentNode )
{
    // Map node_name -> node_pointer
    NODE_MAP nodesMap;

    // Loop through all children mapping them in nodesMap
    aCurrentNode = aCurrentNode->GetChildren();

    while( aCurrentNode )
    {
        // Create a new pair in the map
        // key: current node name
        // value: current node pointer
        nodesMap[aCurrentNode->GetName().ToStdString()] = aCurrentNode;

        // Get next child
        aCurrentNode = aCurrentNode->GetNext();
    }

    return nodesMap;
}


static int countChildren( wxXmlNode* aCurrentNode, const std::string& aName )
{
    // Map node_name -> node_pointer
    int count = 0;

    // Loop through all children counting them if they match the given name
    aCurrentNode = aCurrentNode->GetChildren();

    while( aCurrentNode )
    {
        if( aCurrentNode->GetName().ToStdString() == aName )
            count++;

        // Get next child
        aCurrentNode = aCurrentNode->GetNext();
    }

    return count;
}

void SCH_EAGLE_PLUGIN::loadLayerDefs( wxXmlNode* aLayers )
{
    std::vector<ELAYER>     eagleLayers;

    // Get the first layer and iterate
    wxXmlNode* layerNode = aLayers->GetChildren();

    // find the subset of layers that are copper, and active
    while( layerNode )
    {
        ELAYER  elayer( layerNode );
        eagleLayers.push_back(elayer);

        layerNode = layerNode->GetNext();
    }

    for( const auto &elayer : eagleLayers )
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

        if(elayer.name == "Nets")
        {
            m_LayerMap[elayer.number] = LAYER_WIRE;
        }
        else if(elayer.name == "Info" || elayer.name == "Guide")
        {
            m_LayerMap[elayer.number] = LAYER_NOTES;
        }
        else if(elayer.name == "Busses")
        {
            m_LayerMap[elayer.number] = LAYER_BUS;
        }
    }
}


SCH_LAYER_ID SCH_EAGLE_PLUGIN::kicadLayer( int aEagleLayer )
{
    if(m_LayerMap.find(aEagleLayer) == m_LayerMap.end() )
    {
            return LAYER_NOTES;
    }
    else
    {
        return m_LayerMap[aEagleLayer];
    }
}


static COMPONENT_ORIENTATION_T kicadComponentRotation( float eagleDegrees )
{
    int roti = int(eagleDegrees);


    switch( roti )
    {
    default:
        wxASSERT_MSG( false, wxString::Format( "Unhandled orientation (%d degrees)", roti ) );

    // fall through
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



void eagleToKicadAlignment( EDA_TEXT* aText, int aEagleAlignment, int reldegrees, bool mirror, bool spin, int absdegrees )
{
    int align = aEagleAlignment;

    if( reldegrees == 90)
    {
        aText->SetTextAngle( 900 );
    }
    else if( reldegrees == 180 )
        align = -align;
    else if( reldegrees == 270 )
    {
        aText->SetTextAngle( 900 );
        align = -align;
    }

    if( mirror == true){

        if(absdegrees == 90 || absdegrees == 270)
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
        else if (absdegrees == 0 || absdegrees == 180)
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
    m_rootSheet = nullptr;
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


int SCH_EAGLE_PLUGIN::GetModifyHash() const
{
    return 0;
}


void SCH_EAGLE_PLUGIN::SaveLibrary( const wxString& aFileName, const PROPERTIES* aProperties )
{
}


SCH_SHEET* SCH_EAGLE_PLUGIN::Load( const wxString& aFileName, KIWAY* aKiway,
        SCH_SHEET* aAppendToMe, const PROPERTIES* aProperties )
{
    // TODO: Handle Kiway and uncomment next line.
    // wxASSERT( !aFileName || aKiway != null );

    // Load the document
    wxXmlDocument xmlDocument;

    m_filename = aFileName;
    m_kiway = aKiway;

    if( !xmlDocument.Load( m_filename.GetFullPath() ) )
        THROW_IO_ERROR( wxString::Format( _( "Unable to read file '%s'" ),
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

    // TODO change to loadSheet, so it can handle multiple sheets
    if( !m_rootSheet->GetScreen() )
    {
        SCH_SCREEN* screen = new SCH_SCREEN( aKiway );
        screen->SetFileName( aFileName );
        m_rootSheet->SetScreen( screen );
    }

    // Create a schematic symbol library
    wxFileName libfn = aFileName;
    libfn.SetName( libfn.GetName() );
    libfn.SetExt( SchematicLibraryFileExtension );
    std::unique_ptr<PART_LIB> lib( new PART_LIB( LIBRARY_TYPE_EESCHEMA, libfn.GetFullPath() ) );
    lib->EnableBuffering();

    if( !wxFileName::FileExists( lib->GetFullFileName() ) )
    {
        lib->Create();
    }

    m_partlib = lib.release();

    // Retrieve the root as current node
    wxXmlNode* currentNode = xmlDocument.GetRoot();

    // If the attribute is found, store the Eagle version;
    // otherwise, store the dummy "0.0" version.
    m_version = currentNode->GetAttribute( "version", "0.0" );

    // Map all children into a readable dictionary
    NODE_MAP children = mapChildren( currentNode );

    // TODO: handle compatibility nodes
    // wxXmlNode* compatibility = children["compatibility"];

    // Load drawing
    loadDrawing( children["drawing"] );

    PART_LIBS* prjLibs = aKiway->Prj().SchLibs();

    // There are two ways to add a new library, the official one that requires creating a file:
    m_partlib->Save( false );
    // prjLibs->AddLibrary( m_partlib->GetFullFileName() );
    // or undocumented one:
    prjLibs->insert( prjLibs->begin(), m_partlib );

    deleter.release();
    return m_rootSheet;
}


void SCH_EAGLE_PLUGIN::loadDrawing( wxXmlNode* aDrawingNode )
{
    // Map all children into a readable dictionary
    NODE_MAP drawingChildren = mapChildren( aDrawingNode );

    // Board nodes should not appear in .sch files
    // wxXmlNode* board = drawingChildren["board"]

    // TODO: handle grid nodes
    // wxXmlNode* grid = drawingChildren["grid"]

    // TODO: handle layers nodes
     wxXmlNode* layers = drawingChildren["layers"];
     loadLayerDefs(layers);

    // TODO: handle library nodes
    // wxXmlNode* library = drawingChildren["library"]

    // TODO: handle settings nodes
    // wxXmlNode* settings = drawingChildren["settings"]


    // Load schematic
    loadSchematic( drawingChildren["schematic"] );
}

void SCH_EAGLE_PLUGIN::countNets( wxXmlNode* aSchematicNode )
{
    // Map all children into a readable dictionary
    NODE_MAP schematicChildren = mapChildren( aSchematicNode );
    // Loop through all the sheets
    wxXmlNode* sheetNode = schematicChildren["sheets"]->GetChildren();
    while( sheetNode )
    {
        NODE_MAP sheetChildren = mapChildren( sheetNode );
        // Loop through all nets
        // From the DTD: "Net is an electrical connection in a schematic."
        wxXmlNode* netNode = getChildrenNodes( sheetChildren, "nets" );

        while( netNode )
        {
            std::string netName = netNode->GetAttribute( "name" ).ToStdString();
            if( m_NetCounts.count(netName) ) m_NetCounts[netName] = m_NetCounts[netName]+1;
            else  m_NetCounts[netName]=1;

            // Get next net
            netNode = netNode->GetNext();
        }


        sheetNode = sheetNode->GetNext();
    }

}

void SCH_EAGLE_PLUGIN::loadSchematic( wxXmlNode* aSchematicNode )
{
    // Map all children into a readable dictionary
    NODE_MAP schematicChildren = mapChildren( aSchematicNode );

    // TODO : handle classes nodes
    // wxXmlNode* classes = schematicChildren["classes"];

    // TODO : handle description nodes
    // wxXmlNode* description = schematicChildren["description"];

    // TODO : handle errors nodes
    // wxXmlNode* errors = schematicChildren["errors"];

    // TODO : handle modules nodes
    // wxXmlNode* modules = schematicChildren["modules"];

    wxXmlNode* partNode = schematicChildren["parts"]->GetChildren();

    while( partNode )
    {
        std::unique_ptr<EPART>  epart( new EPART(partNode) );
        string name = epart->name;
        m_partlist[name] = std::move(epart);
        partNode = partNode->GetNext();
    }

    // TODO : handle variantdefs nodes
    // wxXmlNode* variantdefs = schematicChildren["variantdefs"];

    // TODO: handle attributes node
    // wxXmlNode* attributes = schematicChildren["attributes"];
    // Possible children: constant, display, font, layer, name, ratio, rot, size, value, x, y

    // Loop through all the libraries
    wxXmlNode* libraryNode = schematicChildren["libraries"]->GetChildren();

    while( libraryNode )
    {
        EAGLE_LIBRARY elib  = loadLibrary( libraryNode );
        m_eaglelibraries[elib.name] = elib;
        libraryNode = libraryNode->GetNext();
    }

    // find all nets and count how many sheets they appear on.
    // local labels will be used for nets found only on that sheet.
    countNets(aSchematicNode);

    // Loop through all the sheets
    wxXmlNode* sheetNode = schematicChildren["sheets"]->GetChildren();

    int sheet_count = countChildren( schematicChildren["sheets"], "sheet" );

    // If eagle schematic has multiple sheets.

    if( sheet_count > 1 )
    {
        // TODO: set up a heirachical sheet for each Eagle sheet.
        int x, y, i;
        i = 1;
        x = 1;
        y = 1;

        while( sheetNode )
        {
            wxPoint pos = wxPoint( x * 1000, y * 1000 );
            std::unique_ptr<SCH_SHEET> sheet( new SCH_SHEET( pos ) );
            SCH_SCREEN* screen = new SCH_SCREEN( m_kiway );

            sheet->SetTimeStamp( GetNewTimeStamp()-i ); // minus the sheet index to make it unique.
            sheet->SetParent( m_rootSheet->GetScreen() );
            sheet->SetScreen( screen );

            m_currentSheet = sheet.get();
            loadSheet( sheetNode, i);
            sheet->GetScreen()->SetFileName( sheet->GetFileName() );
            m_rootSheet->GetScreen()->Append( sheet.release() );

            sheetNode = sheetNode->GetNext();
            x += 2;

            if( x > 10 )
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
}


void SCH_EAGLE_PLUGIN::loadSheet( wxXmlNode* aSheetNode, int sheetcount )
{
    // Map all children into a readable dictionary
    NODE_MAP sheetChildren = mapChildren( aSheetNode );

    // Get description node

    wxXmlNode* descriptionNode = getChildrenNodes( sheetChildren, "description" );

    wxString des;
    std::string filename;
    if( descriptionNode )
    {
        des = descriptionNode->GetContent();
        m_currentSheet->SetName( des );
        filename = des.ToStdString();
    }
    else
    {
        filename = m_filename.GetName().ToStdString() + "_" + std::to_string(sheetcount);
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
        wxString busName = busNode->GetAttribute( "name" );

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
        wxString netName = netNode->GetAttribute( "name" );
        wxString netClass = netNode->GetAttribute( "class" );

        // Load segments of this net
        loadSegments( netNode, netName, netClass );

        // Get next net
        netNode = netNode->GetNext();
    }

    addBusEntries();

    // Loop through all instances
    wxXmlNode* instanceNode = getChildrenNodes( sheetChildren, "instances" );

    while( instanceNode )
    {
        loadInstance( instanceNode );
        instanceNode = instanceNode->GetNext();
    }

    // Loop through all moduleinsts
    wxXmlNode* moduleinstNode = getChildrenNodes( sheetChildren, "moduleinsts" );

    while( moduleinstNode )
    {
        loadModuleinst( moduleinstNode );
        moduleinstNode = moduleinstNode->GetNext();
    }

    // TODO: do something with the description
    // wxXmlNode* description = sheetChildren["description"];
    // wxString language = description->GetAttribute( "language", "en" ); // Defaults to "en"
    // wxString description = description->GetNodeContent();

    // TODO: do something with the plain
    wxXmlNode* plainNode = getChildrenNodes( sheetChildren, "plain" );

    while( plainNode )
    {
        wxString nodeName = plainNode->GetName();

        if( nodeName == "text" )
        {
            m_currentSheet->GetScreen()->Append( loadplaintext( plainNode ) );
        }
        else if( nodeName == "wire" )
        {
            m_currentSheet->GetScreen()->Append( loadWire( plainNode ) );
        }

        plainNode = plainNode->GetNext();
    }

    SCH_ITEM* item = m_currentSheet->GetScreen()->GetDrawItems();
    sheetBoundingBox = item->GetBoundingBox();
    item = item->Next();
    while( item )
    {
        sheetBoundingBox.Merge( item->GetBoundingBox() );
        item = item->Next();
    }

    wxSize targetSheetSize = sheetBoundingBox.GetSize();
    targetSheetSize.IncBy(1500,1500);

    wxPoint itemsCentre = sheetBoundingBox.Centre();

    wxSize pageSizeIU = m_currentSheet->GetScreen()->GetPageSettings().GetSizeIU();
    PAGE_INFO pageInfo = m_currentSheet->GetScreen()->GetPageSettings();

    if( pageSizeIU.x<targetSheetSize.x )
        pageInfo.SetWidthMils( targetSheetSize.x );

    if( pageSizeIU.y<targetSheetSize.y )
        pageInfo.SetHeightMils( targetSheetSize.y );

    m_currentSheet->GetScreen()->SetPageSettings( pageInfo );

    pageSizeIU = m_currentSheet->GetScreen()->GetPageSettings().GetSizeIU();
    wxPoint sheetcentre( pageSizeIU.x / 2, pageSizeIU.y / 2 );

    // round the translation to nearest 100mil.
    wxPoint translation = sheetcentre- itemsCentre;
    translation.x = translation.x - translation.x%100;
    translation.y = translation.y - translation.y%100;

    item = m_currentSheet->GetScreen()->GetDrawItems();



    while( item )
    {
        item->SetPosition( item->GetPosition()+translation );
        item->ClearFlags();
        item = item->Next();
    }



}



void SCH_EAGLE_PLUGIN::loadSegments( wxXmlNode* aSegmentsNode, const wxString& netName,
        const wxString& netClass )
{
    // Loop through all segments
    wxXmlNode* currentSegment = aSegmentsNode->GetChildren();
    SCH_SCREEN* screen = m_currentSheet->GetScreen();

    int segmentCount = countChildren(aSegmentsNode, "segment");

    // wxCHECK( screen, [>void<] );
    while( currentSegment )
    {
        bool labelled = false; // has a label been added to this continously connected segment
        NODE_MAP segmentChildren = mapChildren( currentSegment );

        // Loop through all segment children
        wxXmlNode* segmentAttribute = currentSegment->GetChildren();

        // load wire nodes first
        // label positions will then be tested for an underlying wire, since eagle labels can be seperated from the wire

        DLIST<SCH_LINE> segmentWires;
        segmentWires.SetOwnership( false );

        while( segmentAttribute )
        {
            if( segmentAttribute->GetName() == "wire" )
            {
                segmentWires.Append( loadWire( segmentAttribute ) );
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
                screen->Append( loadLabel( segmentAttribute, netName, segmentWires ) );
                labelled = true;
            }
            else if( nodeName == "pinref" )
            {
                // TODO: handle pinref attributes
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
                // TODO uncomment
                // THROW_IO_ERROR( wxString::Format( _( "XML node '%s' unknown" ), nodeName ) );
            }

            // Get next segment attribute
            segmentAttribute = segmentAttribute->GetNext();
        }

        SCH_LINE* wire = segmentWires.begin();

        if(labelled == false && wire != NULL )
        {
            wxString netname = netName;
            netname.Replace("!", "~");
            if(m_NetCounts[netName.ToStdString()]>1){
                std::unique_ptr<SCH_GLOBALLABEL> glabel( new SCH_GLOBALLABEL );
                glabel->SetPosition( wire->MidPoint() );
                glabel->SetText( netname);
                glabel->SetTextSize( wxSize( 10, 10 ) );
                screen->Append( glabel.release() );
            }
            else if ( segmentCount > 1)
            {
                std::unique_ptr<SCH_LABEL> label( new SCH_LABEL );
                label->SetPosition( wire->MidPoint() );
                label->SetText( netname );
                label->SetTextSize( wxSize( 10, 10 ) );
                screen->Append( label.release() );
            }
        }



        SCH_LINE* next_wire;
        while( wire != NULL )
        {
            next_wire = wire->Next();
            screen->Append( wire ) ;
            wire = next_wire;
        }

        currentSegment = currentSegment->GetNext();
    }
}


SCH_LINE* SCH_EAGLE_PLUGIN::loadWire( wxXmlNode* aWireNode )
{
    std::unique_ptr<SCH_LINE> wire( new SCH_LINE );

    auto ewire = EWIRE( aWireNode );

    wire->SetLayer( kicadLayer(ewire.layer) );

    wxPoint begin, end;

    begin.x = ewire.x1 * EUNIT_TO_MIL;
    begin.y = -ewire.y1 * EUNIT_TO_MIL;
    end.x   = ewire.x2 * EUNIT_TO_MIL;
    end.y   = -ewire.y2 * EUNIT_TO_MIL;

    wire->SetStartPoint( begin );
    wire->SetEndPoint( end );

    return wire.release();
}


SCH_JUNCTION* SCH_EAGLE_PLUGIN::loadJunction( wxXmlNode* aJunction )
{
    std::unique_ptr<SCH_JUNCTION> junction( new SCH_JUNCTION );

    auto ejunction = EJUNCTION( aJunction );
    wxPoint pos( ejunction.x * EUNIT_TO_MIL, -ejunction.y * EUNIT_TO_MIL );

    junction->SetPosition( pos  );

    return junction.release();
}




SCH_TEXT* SCH_EAGLE_PLUGIN::loadLabel( wxXmlNode* aLabelNode, const wxString& aNetName, const DLIST< SCH_LINE >& segmentWires )
{
    auto elabel = ELABEL( aLabelNode, aNetName );

    wxPoint elabelpos( elabel.x * EUNIT_TO_MIL, -elabel.y * EUNIT_TO_MIL );

    wxString netname = elabel.netname;
    netname.Replace("~", "~~");
    netname.Replace("!", "~");

    if(m_NetCounts[aNetName.ToStdString()]>1){
        std::unique_ptr<SCH_GLOBALLABEL> glabel( new SCH_GLOBALLABEL );
        glabel->SetPosition( elabelpos );
        glabel->SetText( netname );
        glabel->SetTextSize( wxSize( elabel.size*EUNIT_TO_MIL, elabel.size*EUNIT_TO_MIL ) );

        glabel->SetLabelSpinStyle(0);
        if( elabel.rot )
        {
            glabel->SetLabelSpinStyle( int( elabel.rot->degrees / 90) % 4 );
            if(elabel.rot->mirror && ( glabel->GetLabelSpinStyle() == 0 || glabel->GetLabelSpinStyle() == 2 )) glabel->SetLabelSpinStyle((glabel->GetLabelSpinStyle()+2)%4);
        }

        SCH_LINE* wire;
        SCH_LINE* next_wire;

        bool labelOnWire = false;
        auto glabelPosition = glabel->GetPosition();
        for( wire = segmentWires.begin(); wire; wire = next_wire )
        {
            next_wire = wire->Next();
            if( wire->HitTest( glabelPosition, 0) )
            {
                labelOnWire = true;
                break;
            }
        }

        wire = segmentWires.begin();

        if( labelOnWire == false )
        {
            wxPoint newLabelPos = findNearestLinePoint(elabelpos, segmentWires);
            if( wire )
            {
                glabel->SetPosition( newLabelPos );
            }
        }

        return glabel.release();
    }
    else
    {
        std::unique_ptr<SCH_LABEL> label( new SCH_LABEL );
        label->SetPosition(elabelpos);
        label->SetText( netname );
        label->SetTextSize( wxSize( elabel.size*EUNIT_TO_MIL, elabel.size*EUNIT_TO_MIL ) );

        label->SetLabelSpinStyle(0);
        if( elabel.rot )
        {
            label->SetLabelSpinStyle( int( elabel.rot->degrees / 90) % 4 );
            if(elabel.rot->mirror && ( label->GetLabelSpinStyle() == 0 || label->GetLabelSpinStyle() == 2 )) label->SetLabelSpinStyle((label->GetLabelSpinStyle()+2)%4);
        }

        SCH_LINE* wire;
        SCH_LINE* next_wire;

        bool labelOnWire = false;
        auto labelPosition = label->GetPosition();
        for( wire = segmentWires.begin(); wire; wire = next_wire )
        {
            next_wire = wire->Next();
            if( wire->HitTest( labelPosition, 0) )
            {
                labelOnWire = true;
                break;
            }
        }

        wire = segmentWires.begin();

        if( labelOnWire == false )
        {

            if( wire )
            {
                wxPoint newLabelPos = findNearestLinePoint( elabelpos , segmentWires);
                label->SetPosition( newLabelPos );
            }

        }

        return label.release();
    }

}


wxPoint SCH_EAGLE_PLUGIN::findNearestLinePoint( const wxPoint aPoint, const DLIST< SCH_LINE >& lines)
{
    wxPoint nearestPoint;

    float mindistance = std::numeric_limits<float>::max();
    float d;
    SCH_LINE* line = lines.begin();
    while( line != NULL )
    {
        auto testpoint = line->GetStartPoint();
        d = sqrt( abs( ( (aPoint.x-testpoint.x)^2 ) + ( (aPoint.y-testpoint.y)^2 ) ) );

        if( d < mindistance )
        {
            mindistance  = d;
            nearestPoint = testpoint;
        }

        testpoint = line->MidPoint();
        d = sqrt( abs( ( (aPoint.x-testpoint.x)^2 ) + ( (aPoint.y-testpoint.y)^2 ) ) );

        if( d < mindistance )
        {
            mindistance  = d;
            nearestPoint = testpoint;
        }

        testpoint = line->GetEndPoint();
        d = sqrt( abs( ( (aPoint.x-testpoint.x)^2 ) + ( (aPoint.y-testpoint.y)^2 ) ) );

        if( d < mindistance )
        {
            mindistance  = d;
            nearestPoint = testpoint;
        }

        line = line->Next();
    }
    return nearestPoint;
}


void SCH_EAGLE_PLUGIN::loadInstance( wxXmlNode* aInstanceNode )
{
    auto einstance = EINSTANCE( aInstanceNode );

    SCH_SCREEN* screen = m_currentSheet->GetScreen();

    // Find the part in the list for the sheet.
    // Assign the component its value from the part entry
    // Calculate the unit number from the gate entry of the instance
    // Assign the the LIB_ID from deviceset and device names


    EPART* epart = m_partlist[einstance.part].get();

    std::string gatename = epart->deviceset + epart->device + einstance.gate;
    std::string symbolname = epart->deviceset + epart->device;
    // KiCad enumerates units starting from 1, Eagle starts with 0
    int unit = m_eaglelibraries[epart->library].gate_unit[gatename];
    std::string package = m_eaglelibraries[epart->library].package[symbolname];

    // std::cout << "Instance> part: " << einstance.part << " Gate: " << einstance.gate << " " << symbolname << '\n';


    LIB_ID libId( wxEmptyString, symbolname );

    LIB_PART* part = m_partlib->FindPart(symbolname);

    std::unique_ptr<SCH_COMPONENT> component( new SCH_COMPONENT() );
    component->SetLibId( libId );
    component->SetUnit( unit );
    component->SetConvert( 0 );
    component->SetPosition( wxPoint( einstance.x * EUNIT_TO_MIL, -einstance.y * EUNIT_TO_MIL ) );
    component->GetField( FOOTPRINT )->SetText( wxString( package ) );
    component->SetTimeStamp( moduleTstamp( einstance.part, epart->value ? *epart->value : "" ) );
    // component->AddHierarchicalReference( path, reference, (int)tmp ); // TODO ??

    if( einstance.rot )
    {
        component->SetOrientation( kicadComponentRotation( einstance.rot->degrees ) );

        if( einstance.rot->mirror )
        {
            component->MirrorY( einstance.x * EUNIT_TO_MIL );
        }
    }

    LIB_FIELDS partFields;
    part->GetFields(partFields);
    for( auto const& field : partFields )
    {
        component->GetField(field.GetId())->ImportValues(field);
        component->GetField(field.GetId())->SetTextPos( component->GetPosition() + field.GetTextPos() );
    }

    component->GetField( REFERENCE )->SetText( einstance.part );



    if( epart->value )
    {
        component->GetField( VALUE )->SetText( *epart->value );
    }
    else
    {
        if(epart->device.length()>0)
        component->GetField( VALUE )->SetText( epart->device );
        else  if(epart->deviceset.length()>0)
        component->GetField( VALUE )->SetText( epart->deviceset );
    }

    if(part->GetField(REFERENCE)->IsVisible())
    component->GetField( REFERENCE )->SetVisible( true );
    else
    component->GetField( REFERENCE )->SetVisible( false );

    if(part->GetField( VALUE )->IsVisible())
    component->GetField( VALUE )->SetVisible( true );
    else
    component->GetField( VALUE )->SetVisible( false );

    wxXmlNode* attributeNode = aInstanceNode->GetChildren();
    while(attributeNode)
    {
        if(attributeNode->GetName() == "attribute")
        {
            auto attr = EATTR(attributeNode);

            SCH_FIELD* field;
            if(attr.name == "NAME"){

                field = component->GetField( REFERENCE );
                field->SetPosition( wxPoint(*attr.x* EUNIT_TO_MIL, *attr.y*-EUNIT_TO_MIL) );
                int align = attr.align ? *attr.align : ETEXT::BOTTOM_LEFT;

                int absdegrees = attr.rot ? attr.rot->degrees : 0;
                bool mirror = attr.rot ? attr.rot->mirror : false;
                if(einstance.rot)if(einstance.rot->mirror){
                    mirror = !mirror;
                }
                bool spin = attr.rot ? attr.rot->spin : false;

                int rotation = einstance.rot ? einstance.rot->degrees : 0;
                int reldegrees = ( absdegrees - rotation + 360.0);
                reldegrees %= 360;

                eagleToKicadAlignment((EDA_TEXT*)field, align, reldegrees, mirror, spin, absdegrees);

            }

            else if (attr.name == "VALUE"){
                field = component->GetField( VALUE );
                field->SetPosition( wxPoint(*attr.x* EUNIT_TO_MIL, *attr.y*-EUNIT_TO_MIL) );
                int align = attr.align ? *attr.align : ETEXT::BOTTOM_LEFT;

                int absdegrees = attr.rot ? attr.rot->degrees : 0;
                bool mirror = attr.rot ? attr.rot->mirror : false;
                bool spin = attr.rot ? attr.rot->spin : false;
                if(einstance.rot)if(einstance.rot->mirror){
                    mirror = !mirror;
                }

                int rotation = einstance.rot ? einstance.rot->degrees : 0;
                int reldegrees = ( absdegrees - rotation + 360.0);
                reldegrees %= 360;

                eagleToKicadAlignment((EDA_TEXT*)field, align, reldegrees, mirror, spin, absdegrees);
            }
        }
        attributeNode = attributeNode->GetNext();

    }

    component->ClearFlags();

    screen->Append( component.release() );
}


void SCH_EAGLE_PLUGIN::loadModuleinst( wxXmlNode* aModuleinstNode )
{
}


EAGLE_LIBRARY SCH_EAGLE_PLUGIN::loadLibrary( wxXmlNode* aLibraryNode )
{

    EAGLE_LIBRARY elib;


    // Read the library name
    wxString libName = aLibraryNode->GetAttribute( "name" );
    elib.name = libName.ToStdString();

    ////std::cout << "Importing Eagle Library "<< libName.ToStdString() << std::endl;

    // Query all children and map them into a readable dictionary
    NODE_MAP libraryChildren = mapChildren( aLibraryNode );

    // TODO: Do something with the description
    // wxXmlNode* libraryChildren["description"];


    // Loop through the packages and load each of them
    // wxXmlNode* packageNode = libraryChildren["packages"]->GetChildren();
    // while( packageNode )
    // {
    // loadPackage( packageNode );
    // packageNode = packageNode->GetNext();
    // }

    // Loop through the symbols and load each of them
    wxXmlNode* symbolNode = libraryChildren["symbols"]->GetChildren();

    while( symbolNode )
    {
        string symbolName = symbolNode->GetAttribute( "name" ).ToStdString();
        elib.symbolnodes[symbolName] = symbolNode;
        symbolNode = symbolNode->GetNext();
    }

    // Loop through the devicesets and load each of them
    wxXmlNode* devicesetNode = libraryChildren["devicesets"]->GetChildren();

    while( devicesetNode )
    {
        // Get Device set information
        EDEVICESET edeviceset = EDEVICESET( devicesetNode );

        // std::cout << "Importing Eagle device set "<< edeviceset.name << std::endl;

        NODE_MAP aDeviceSetChildren = MapChildren( devicesetNode );
        wxXmlNode* deviceNode = getChildrenNodes( aDeviceSetChildren, "devices" );

        // For each device in the device set:
        while( deviceNode )
        {
            // Get device information
            EDEVICE edevice = EDEVICE( deviceNode );

            // Create symbol name from deviceset and device names.
            wxString symbolName = wxString( edeviceset.name + edevice.name );
            // std::cout << "Creating Kicad Symbol: " << symbolName.ToStdString() << '\n';

            if( edevice.package )
                elib.package[symbolName.ToStdString()] = edevice.package.Get();

            // Create kicad symbol.
            unique_ptr<LIB_PART> kpart( new LIB_PART( symbolName ) );

            // Process each gate in the deviceset for this device.
            wxXmlNode* gateNode = getChildrenNodes( aDeviceSetChildren, "gates" );
            int gates_count = countChildren( aDeviceSetChildren["gates"], "gate" );
            kpart->SetUnitCount( gates_count );
            int gateindex;
            bool ispower = false;

            gateindex = 1;

            while( gateNode )
            {
                EGATE egate = EGATE( gateNode );

                elib.gate_unit[edeviceset.name + edevice.name + egate.name] = gateindex;


                ispower = loadSymbol( elib.symbolnodes[egate.symbol],
                        kpart, &edevice, gateindex, egate.name );

                gateindex++;
                gateNode = gateNode->GetNext();
            }    // gateNode

            kpart->SetUnitCount( gates_count );
            if(gates_count == 1 && ispower) kpart->SetPower();

            string name = kpart->GetName().ToStdString();
            m_partlib->AddPart( kpart.get() );
            elib.kicadsymbols.insert( name, kpart.release() );

            deviceNode = deviceNode->GetNext();
        }    // devicenode

        devicesetNode = devicesetNode->GetNext();
    }    // devicesetNode

    return elib;
}


bool SCH_EAGLE_PLUGIN::loadSymbol( wxXmlNode* aSymbolNode,
        std::unique_ptr< LIB_PART >& aPart,
        EDEVICE* aDevice,
        int gateNumber,
        string gateName )
{
    wxString symbolName = aSymbolNode->GetAttribute( "name" );
    std::vector<LIB_ITEM*> items;

    wxXmlNode* currentNode = aSymbolNode->GetChildren();

    bool foundName = false;
    bool foundValue = false;
    bool ispower = false;
    int pincount = 0;

    while( currentNode )
    {
        wxString nodeName = currentNode->GetName();

        if( nodeName == "description" )
        {
            // TODO
            // wxASSERT_MSG( false, "'description' nodes are not implemented yet" );
        }
        else if( nodeName == "dimension" )
        {
            // TODO
            // wxASSERT_MSG( false, "'description' nodes are not implemented yet" );
        }
        else if( nodeName == "frame" )
        {
        }
        else if( nodeName == "circle" )
        {
            aPart->AddDrawItem( loadSymbolCircle( aPart, currentNode, gateNumber) );
        }
        else if( nodeName == "pin" )
        {
            EPIN ePin =  EPIN( currentNode );
            std::unique_ptr<LIB_PIN> pin ( loadPin( aPart, currentNode, &ePin, gateNumber ) );
            pincount++;

            if(ePin.direction)
            {
                if(wxString(*ePin.direction).Lower()== "sup")
                {
                    ispower = true;
                    pin->SetType(PIN_POWER_IN);
                }
                else if(wxString(*ePin.direction).Lower()== "pas")
                {
                  pin->SetType(PIN_PASSIVE);
                }
                else if(wxString(*ePin.direction).Lower()== "out")
                {
                  pin->SetType(PIN_OUTPUT);
                }
                else if(wxString(*ePin.direction).Lower()== "in")
                {
                  pin->SetType(PIN_INPUT);
                }
                else if(wxString(*ePin.direction).Lower()== "nc")
                {
                  pin->SetType(PIN_NC);
                }
                else if(wxString(*ePin.direction).Lower()== "io")
                {
                  pin->SetType(PIN_BIDI);
                }
                else if(wxString(*ePin.direction).Lower()== "oc")
                {
                  pin->SetType(PIN_OPENEMITTER);
                }
                else if(wxString(*ePin.direction).Lower()== "hiz")
                {
                  pin->SetType(PIN_TRISTATE);
                }
                else
                {
                  pin->SetType(PIN_UNSPECIFIED);
                }







            }



            if(aDevice->connects.size() != 0)
            {
                for( auto connect : aDevice->connects )
                {
                    if( connect.gate == gateName and pin->GetName().ToStdString() == connect.pin )
                    {

                        wxString padname( connect.pad );
                        pin->SetPinNumFromString( padname );
                        pin->SetPartNumber( gateNumber );
                        pin->SetUnit( gateNumber );


                        wxString pinname = pin->GetName();
                        pinname.Replace("~", "~~");
                        pinname.Replace("!", "~");
                        pin->SetName( pinname );

                        aPart->AddDrawItem( pin.release() );
                        break;
                    }
                }
            }
            else
            {
                pin->SetPartNumber( gateNumber );
                pin->SetUnit( gateNumber );
                wxString stringPinNum = wxString::Format(wxT("%i"),pincount);
                pin->SetPinNumFromString(stringPinNum);
                aPart->AddDrawItem( pin.release() );
            }
        }
        else if( nodeName == "polygon" )
        {
            aPart->AddDrawItem( loadSymbolPolyLine( aPart, currentNode, gateNumber ) );
        }
        else if( nodeName == "rectangle" )
        {
            aPart->AddDrawItem( loadSymbolRectangle( aPart, currentNode, gateNumber ));
        }
        else if( nodeName == "text" )
        {
            std::unique_ptr<LIB_TEXT> libtext ( loadSymboltext( aPart, currentNode , gateNumber) );

            if( libtext->GetText().Upper() ==">NAME" )
            {
                aPart->GetField( REFERENCE )->SetTextPos( libtext->GetPosition() );
                aPart->GetField( REFERENCE )->SetTextSize( libtext->GetTextSize() );
                aPart->GetField( REFERENCE )->SetTextAngle( libtext->GetTextAngle() );
                aPart->GetField( REFERENCE )->SetBold( libtext->IsBold() );
                aPart->GetField( REFERENCE )->SetVertJustify(libtext->GetVertJustify());
                aPart->GetField( REFERENCE )->SetHorizJustify(libtext->GetHorizJustify());
                aPart->GetField( REFERENCE )->SetVisible(true);
                foundName = true;
            }
            else if( libtext->GetText().Upper() == ">VALUE" )
            {
                aPart->GetField( VALUE )->SetTextPos( libtext->GetPosition() );
                aPart->GetField( VALUE )->SetTextSize( libtext->GetTextSize() );
                aPart->GetField( VALUE )->SetTextAngle( libtext->GetTextAngle() );
                aPart->GetField( VALUE )->SetBold( libtext->IsBold() );
                aPart->GetField( VALUE )->SetVertJustify(libtext->GetVertJustify());
                aPart->GetField( VALUE )->SetHorizJustify(libtext->GetHorizJustify());
                aPart->GetField( VALUE )->SetVisible(true);
                foundValue = true;
            }
            else
            {
                aPart->AddDrawItem( libtext.release() );
            }

        }
        else if( nodeName == "wire" )
        {
            aPart->AddDrawItem( loadSymbolWire( aPart, currentNode, gateNumber ) );
        }

        currentNode = currentNode->GetNext();
    }

    if( foundName == false )
        aPart->GetField( REFERENCE )->SetVisible(false);
    if( foundValue == false )
        aPart->GetField( VALUE )->SetVisible(false);

    return pincount == 1 ? ispower : false;
}


LIB_CIRCLE* SCH_EAGLE_PLUGIN::loadSymbolCircle( std::unique_ptr< LIB_PART >& aPart, wxXmlNode* aCircleNode , int gateNumber)
{
    // Parse the circle properties
    ECIRCLE c( aCircleNode );

    unique_ptr<LIB_CIRCLE> circle( new LIB_CIRCLE( aPart.get() ) );

    circle->SetPosition( wxPoint( c.x * EUNIT_TO_MIL, c.y * EUNIT_TO_MIL ) );
    circle->SetRadius( c.radius * EUNIT_TO_MIL );
    circle->SetWidth( c.width * EUNIT_TO_MIL );
    circle->SetUnit( gateNumber );

    return circle.release();
}


LIB_RECTANGLE* SCH_EAGLE_PLUGIN::loadSymbolRectangle( std::unique_ptr< LIB_PART >& aPart, wxXmlNode* aRectNode , int gateNumber )
{
    ERECT rect( aRectNode );

    unique_ptr<LIB_RECTANGLE> rectangle( new LIB_RECTANGLE( aPart.get() ) );

    rectangle->SetPosition( wxPoint( rect.x1 * EUNIT_TO_MIL, rect.y1 * EUNIT_TO_MIL ) );
    rectangle->SetEnd( wxPoint( rect.x2 * EUNIT_TO_MIL, rect.y2 * EUNIT_TO_MIL ) );

    // TODO: Manage rotation
    // Probaly using a transform.
    rectangle->SetUnit( gateNumber );
    // Eagle rectangles are filled by definition.
    rectangle->SetFillMode(FILLED_SHAPE);

    return rectangle.release();
}


LIB_ITEM* SCH_EAGLE_PLUGIN::loadSymbolWire( std::unique_ptr< LIB_PART >& aPart, wxXmlNode* aWireNode, int gateNumber)
{
    // TODO: Layer map


    auto ewire = EWIRE( aWireNode );

    wxRealPoint begin, end;

    begin.x = ewire.x1 * EUNIT_TO_MIL;
    begin.y = ewire.y1 * EUNIT_TO_MIL;
    end.x   = ewire.x2 * EUNIT_TO_MIL;
    end.y   = ewire.y2 * EUNIT_TO_MIL;
    if( ewire.curve )
    {
        std::unique_ptr<LIB_ARC> arc( new LIB_ARC( aPart.get() ) );
        wxRealPoint center = kicad_arc_center( begin, end, *ewire.curve);

        arc->SetPosition(center);
        if(*ewire.curve >0)
        {
        arc->SetStart( begin );
        arc->SetEnd( end );
        }
        else
        {
        arc->SetStart( end );
        arc->SetEnd( begin );
        }

        arc->SetWidth(ewire.width*EUNIT_TO_MIL);


        double radius = sqrt( abs( ( ( center.x - begin.x ) * ( center.x - begin.x ) )
                                   + ( ( center.y - begin.y ) * ( center.y - begin.y ) ) ) ) * 2;

        arc->SetRadius(radius);
        arc->CalcRadiusAngles();

        if(ewire.width*2*EUNIT_TO_MIL > radius){
            wxRealPoint centerStartVector = begin-center;

            wxRealPoint centerEndVector   = end - center;
            centerStartVector.x = centerStartVector.x/radius;
            centerStartVector.y = centerStartVector.y/radius;


            centerEndVector.x = centerEndVector.x/radius;
            centerEndVector.y = centerEndVector.y/radius;
            centerStartVector.x = centerStartVector.x*(ewire.width*EUNIT_TO_MIL+radius);
            centerStartVector.y = centerStartVector.y*(ewire.width*EUNIT_TO_MIL+radius);

            centerEndVector.x = centerEndVector.x*(ewire.width*EUNIT_TO_MIL+radius);
            centerEndVector.y = centerEndVector.y*(ewire.width*EUNIT_TO_MIL+radius);


            begin = center + centerStartVector;
            end = center + centerEndVector;
            radius = sqrt( abs( ( ( center.x - begin.x ) * ( center.x - begin.x ) )
                                + ( ( center.y - begin.y ) * ( center.y - begin.y ) ) ) ) * 2;


            arc->SetPosition(center);
            if(*ewire.curve >0)
            {
            arc->SetStart( begin );
            arc->SetEnd( end );
            }
            else
            {
            arc->SetStart( end );
            arc->SetEnd( begin );
            }
            arc->SetRadius(radius);
            arc->CalcRadiusAngles();
            arc->SetWidth(1);
            arc->SetFillMode(FILLED_SHAPE);

        }

        return (LIB_ITEM*) arc.release();

    }
    else
    {
        std::unique_ptr<LIB_POLYLINE> polyLine( new LIB_POLYLINE( aPart.get() ) );

        polyLine->AddPoint( begin );
        polyLine->AddPoint( end );

        return (LIB_ITEM*) polyLine.release();

    }

}


LIB_POLYLINE* SCH_EAGLE_PLUGIN::loadSymbolPolyLine( std::unique_ptr< LIB_PART >& aPart, wxXmlNode* aPolygonNode, int gateNumber )
{
    // TODO: Layer map
    std::unique_ptr<LIB_POLYLINE> polyLine( new LIB_POLYLINE( aPart.get() ) );

    EPOLYGON epoly( aPolygonNode );
    wxXmlNode* vertex = aPolygonNode->GetChildren();


    wxPoint pt;
    while( vertex )
    {
        if( vertex->GetName() == "vertex" )     // skip <xmlattr> node
        {
            EVERTEX evertex( vertex );
            pt = wxPoint( evertex.x * EUNIT_TO_MIL, evertex.y * EUNIT_TO_MIL );
            polyLine->AddPoint( pt );
        }
        vertex = vertex->GetNext();
    }

    polyLine->SetFillMode( FILLED_SHAPE );
    polyLine->SetUnit( gateNumber );

    return polyLine.release();
}


LIB_PIN* SCH_EAGLE_PLUGIN::loadPin( std::unique_ptr< LIB_PART >& aPart, wxXmlNode* aPin, EPIN* epin , int gateNumber )
{
    std::unique_ptr<LIB_PIN> pin( new LIB_PIN( aPart.get() ) );


    pin->SetPosition( wxPoint( epin->x * EUNIT_TO_MIL, epin->y * EUNIT_TO_MIL ) );
    pin->SetName( epin->name );
    pin->SetUnit( gateNumber );

    int roti = 0;

    if( epin->rot )
    {
        roti = int(epin->rot->degrees);
    }

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

    if( epin->length )
    {
        wxString length = epin->length.Get();

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

    if( epin->visible )
    {
        wxString visible = epin->visible.Get();

        if( visible == "off" )
        {
            pin->SetNameTextSize( 0 );
            pin->SetNumberTextSize( 0 );
        }
        else if (visible == "pad")
        {
            pin->SetNameTextSize( 0 );
        }
        else if( visible == "pin" )
        {
            pin->SetNumberTextSize( 0 );
        }
        /*
        else if( visible == "both" )
        {
        }
        */
    }

    return pin.release();
}


LIB_TEXT* SCH_EAGLE_PLUGIN::loadSymboltext( std::unique_ptr< LIB_PART >& aPart, wxXmlNode* aLibText, int gateNumber )
{
    std::unique_ptr<LIB_TEXT> libtext( new LIB_TEXT( aPart.get() ) );

    ETEXT etext( aLibText );

    libtext->SetUnit( gateNumber );
    libtext->SetPosition( wxPoint( etext.x * EUNIT_TO_MIL, etext.y * EUNIT_TO_MIL ) );
    libtext->SetText( aLibText->GetNodeContent() );
    libtext->SetTextSize( wxSize( etext.size * EUNIT_TO_MIL*0.95,
                    etext.size * EUNIT_TO_MIL*0.95 ) );

    if( etext.ratio )
    {
        if( etext.ratio.Get()>12 )
        {
            libtext->SetBold( true );
            libtext->SetThickness( GetPenSizeForBold( libtext->GetTextWidth() ) );
        }
    }

    int align = etext.align ? *etext.align : ETEXT::BOTTOM_LEFT;


    int degrees = etext.rot ? etext.rot->degrees : 0;
    bool mirror = etext.rot ? etext.rot->mirror : false;
    bool spin = etext.rot ? etext.rot->spin : false;

    eagleToKicadAlignment((EDA_TEXT*)libtext.get(), align, degrees, mirror, spin, 0);


    return libtext.release();
}


SCH_TEXT* SCH_EAGLE_PLUGIN::loadplaintext( wxXmlNode* aSchText )
{
    std::unique_ptr<SCH_TEXT> schtext( new SCH_TEXT() );

    auto etext = ETEXT( aSchText );


    schtext->SetItalic( false );
    schtext->SetPosition( wxPoint( etext.x * EUNIT_TO_MIL, -etext.y * EUNIT_TO_MIL ) );
    wxString thetext = aSchText->GetNodeContent();
    thetext.Replace("~", "~~");
    thetext.Replace("!", "~");
    schtext->SetText( thetext );

    if( etext.ratio )
    {
        if( etext.ratio.Get()>12 )
        {
            schtext->SetBold( true );
            schtext->SetThickness( GetPenSizeForBold( schtext->GetTextWidth() ) );
        }
    }

    schtext->SetTextSize( wxSize( int(etext.size * EUNIT_TO_MIL),
                    int(etext.size * EUNIT_TO_MIL) ) );

    int align = etext.align ? *etext.align : ETEXT::BOTTOM_LEFT;


    int degrees = etext.rot ? etext.rot->degrees : 0;
    bool mirror = etext.rot ? etext.rot->mirror : false;
    bool spin = etext.rot ? etext.rot->spin : false;

    eagleToKicadAlignment((EDA_TEXT*)schtext.get(), align, degrees, mirror, spin, 0);



    return schtext.release();
}


bool SCH_EAGLE_PLUGIN::CheckHeader( const wxString& aFileName )
{
    // Open file and check first line
    wxTextFile tempFile;

    tempFile.Open( aFileName );
    wxString firstline;
    // read the first line
    firstline = tempFile.GetFirstLine();
    wxString secondline = tempFile.GetNextLine();
    wxString thirdline = tempFile.GetNextLine();
    tempFile.Close();

    return firstline.StartsWith( "<?xml" ) && secondline.StartsWith("<!DOCTYPE eagle SYSTEM") && thirdline.StartsWith("<eagle version");
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
        for(  SCH_ITEM* line = m_currentSheet->GetScreen()->GetDrawItems() ; line; line = nextline )
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
                            SCH_MARKER* markera = new SCH_MARKER( linestart,
                                    "Bus Entry neeeded" );

                            m_currentSheet->GetScreen()->Append( markera );
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
                                    ( (SCH_LINE*) line )->SetStartPoint( linestart +
                                            wxPoint( -100, 0 ) );
                                }
                                else
                                {
                                    SCH_MARKER* marker = new SCH_MARKER( linestart,
                                            "Bus Entry neeeded" );

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
                                    ( (SCH_LINE*) line )->SetStartPoint( linestart + wxPoint( 100,
                                                    0 ) );
                                }
                                else
                                {
                                    SCH_MARKER* marker = new SCH_MARKER( linestart,
                                            "Bus Entry neeeded" );

                                    m_currentSheet->GetScreen()->Append( marker );
                                }
                            }
                        }

                        // Same thing but test end of the wire instead.
                        if( TestSegmentHit( lineend, busstart, busend, 0 ) )
                        {
                            SCH_MARKER* markera = new SCH_MARKER( lineend,
                                    "Bus Entry neeeded" );

                            m_currentSheet->GetScreen()->Append( markera );
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
                                    ( (SCH_LINE*) line )->SetEndPoint( lineend +
                                            wxPoint( -100, 0 ) );
                                }
                                else
                                {
                                    SCH_MARKER* marker = new SCH_MARKER( lineend,
                                            "Bus Entry neeeded" );

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
                                    ( (SCH_LINE*) line )->SetEndPoint( lineend +
                                            wxPoint( 100, 0 ) );
                                }
                                else
                                {
                                    SCH_MARKER* marker = new SCH_MARKER( lineend,
                                            "Bus Entry neeeded" );

                                    m_currentSheet->GetScreen()->Append( marker );
                                }
                            }
                        }
                    } // if( linestart.y == lineend.y && busstart.x == busend.x)

                    // Test for horizontal wire and vertical bus
                    if( linestart.x == lineend.x && busstart.y == busend.y )
                    {
                        if( TestSegmentHit( linestart, busstart, busend, 0 ) )
                        {
                            SCH_MARKER* markera = new SCH_MARKER( linestart,
                                    "Bus Entry neeeded" );

                            m_currentSheet->GetScreen()->Append( markera );

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
                                    ( (SCH_LINE*) line )->SetStartPoint( linestart +
                                            wxPoint( 0, -100 ) );
                                }
                                else
                                {
                                    SCH_MARKER* marker = new SCH_MARKER( linestart,
                                            "Bus Entry neeeded" );

                                    m_currentSheet->GetScreen()->Append( marker );
                                }
                            }
                            else // wire end is below the bus.
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
                                    ( (SCH_LINE*) line )->SetStartPoint( linestart + wxPoint( 0,
                                                    100 ) );
                                }
                                else
                                {
                                    SCH_MARKER* marker = new SCH_MARKER( linestart,
                                            "Bus Entry neeeded" );

                                    m_currentSheet->GetScreen()->Append( marker );
                                }
                            }
                        }

                        if( TestSegmentHit( lineend, busstart, busend, 0 ) )
                        {
                            // Wire end is on the bus
                            // If wire start is above the bus,
                            SCH_MARKER* markera = new SCH_MARKER( lineend,
                                    "Bus Entry neeeded" );

                            m_currentSheet->GetScreen()->Append( markera );

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
                                    ( (SCH_LINE*) line )->SetEndPoint( lineend +
                                            wxPoint( 0, -100 ) );
                                }
                                else
                                {
                                    SCH_MARKER* marker = new SCH_MARKER( lineend,
                                            "Bus Entry neeeded" );

                                    m_currentSheet->GetScreen()->Append( marker );
                                }
                            }
                            else // wire end is below the bus.
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
                                    ( (SCH_LINE*) line )->SetEndPoint( lineend +
                                            wxPoint( 0, 100 ) );
                                }
                                else
                                {
                                    SCH_MARKER* marker = new SCH_MARKER( lineend,
                                            "Bus Entry neeeded" );

                                    m_currentSheet->GetScreen()->Append( marker );
                                }
                            }
                        }
                    }

                    linestart = ( (SCH_LINE*) line )->GetStartPoint();
                    lineend = ( (SCH_LINE*) line )->GetEndPoint();
                    busstart = ( (SCH_LINE*) bus )->GetStartPoint();
                    busend = ( (SCH_LINE*) bus )->GetEndPoint();


                    // bus entry wire isn't horizontal or vertical
                    if( TestSegmentHit( linestart, busstart, busend, 0 ) )
                    {
                        SCH_MARKER* markera = new SCH_MARKER( linestart,
                                "Bus Entry neeeded" );

                        m_currentSheet->GetScreen()->Append( markera );
                        wxPoint wirevector = linestart - lineend;

                        if( wirevector.x > 0 )
                        {
                            if( wirevector.y > 0 )
                            {
                                wxPoint p = linestart + wxPoint( -100, -100 );
                                SCH_BUS_WIRE_ENTRY* busEntry = new SCH_BUS_WIRE_ENTRY( p, '\\' );
                                busEntry->SetFlags( IS_NEW );
                                m_currentSheet->GetScreen()->Append( busEntry );

                                if( p == lineend ) // wire is overlapped by bus entry symbol
                                {
                                    m_currentSheet->GetScreen()->DeleteItem( line );
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

                                if( p== lineend ) // wire is overlapped by bus entry symbol
                                {
                                    m_currentSheet->GetScreen()->DeleteItem( line );
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

                                if( linestart + wxPoint( 100, -100 )== lineend ) // wire is overlapped by bus entry symbol
                                {
                                    m_currentSheet->GetScreen()->DeleteItem( line );
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

                                if( linestart + wxPoint( 100, 100 )== lineend ) // wire is overlapped by bus entry symbol
                                {
                                    m_currentSheet->GetScreen()->DeleteItem( line );
                                }
                                else
                                {
                                    ( (SCH_LINE*) line )->SetStartPoint( linestart +
                                            wxPoint( 100, 100 ) );
                                }
                            }
                        }
                    }

                    if( TestSegmentHit( lineend, busstart, busend, 0 ) )
                    {
                        SCH_MARKER* markera = new SCH_MARKER( lineend,
                                "Bus Entry neeeded" );

                        m_currentSheet->GetScreen()->Append( markera );
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

                                if( p == linestart ) // wire is overlapped by bus entry symbol
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

                                if( p== linestart ) // wire is overlapped by bus entry symbol
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
                                SCH_BUS_WIRE_ENTRY* busEntry =
                                    new SCH_BUS_WIRE_ENTRY( lineend + wxPoint( -100, 100 ), '/' );
                                busEntry->SetFlags( IS_NEW );
                                m_currentSheet->GetScreen()->Append( busEntry );

                                if( lineend + wxPoint( -100, 100 )== linestart ) // wire is overlapped by bus entry symbol
                                {
                                    m_currentSheet->GetScreen()->DeleteItem( line );
                                }
                                else
                                {
                                    ( (SCH_LINE*) line )->SetEndPoint( lineend + wxPoint( -100,
                                                    100 ) );
                                }
                            }
                            else
                            {
                                SCH_BUS_WIRE_ENTRY* busEntry =
                                    new SCH_BUS_WIRE_ENTRY( lineend + wxPoint( -100, -100 ), '\\' );
                                busEntry->SetFlags( IS_NEW );
                                m_currentSheet->GetScreen()->Append( busEntry );

                                if( lineend + wxPoint( -100, -100 )== linestart ) // wire is overlapped by bus entry symbol
                                {
                                    m_currentSheet->GetScreen()->DeleteItem( line );
                                }
                                else
                                {
                                    ( (SCH_LINE*) line )->SetEndPoint( lineend + wxPoint( -100,
                                                    -100 ) );
                                }
                            }
                        }
                    }
                }
            }
        }    // for ( line ..
    }    // for ( bus ..
}




void SCH_EAGLE_PLUGIN::Save( const wxString& aFileName, SCH_SCREEN* aSchematic, KIWAY* aKiway,
        const PROPERTIES* aProperties )
{
    // std::cout << "SCH_EAGLE_PLUGIN::Save" << '\n';
}


size_t SCH_EAGLE_PLUGIN::GetSymbolLibCount( const wxString& aLibraryPath,
        const PROPERTIES* aProperties )
{
    return 0;
}


void SCH_EAGLE_PLUGIN::EnumerateSymbolLib( wxArrayString& aAliasNameList,
        const wxString& aLibraryPath,
        const PROPERTIES* aProperties )
{
}


LIB_ALIAS* SCH_EAGLE_PLUGIN::LoadSymbol( const wxString& aLibraryPath, const wxString& aSymbolName,
        const PROPERTIES* aProperties )
{
    return nullptr;
}


void SCH_EAGLE_PLUGIN::SaveSymbol( const wxString& aLibraryPath, const LIB_PART* aSymbol,
        const PROPERTIES* aProperties )
{
    // std::cout << "SCH_EAGLE_PLUGIN::SaveSymbol" << '\n';
}


void SCH_EAGLE_PLUGIN::DeleteAlias( const wxString& aLibraryPath, const wxString& aAliasName,
        const PROPERTIES* aProperties )
{
}


void SCH_EAGLE_PLUGIN::DeleteSymbol( const wxString& aLibraryPath, const wxString& aAliasName,
        const PROPERTIES* aProperties )
{
}


void SCH_EAGLE_PLUGIN::CreateSymbolLib( const wxString& aLibraryPath,
        const PROPERTIES* aProperties )
{
}


bool SCH_EAGLE_PLUGIN::DeleteSymbolLib( const wxString& aLibraryPath,
        const PROPERTIES* aProperties )
{
    return false;
}


bool SCH_EAGLE_PLUGIN::IsSymbolLibWritable( const wxString& aLibraryPath )
{
    return false;
}


void SCH_EAGLE_PLUGIN::SymbolLibOptions( PROPERTIES* aListToAppendTo ) const
{
}


// approved
// attribute
// circle
// clearance
// connect
// contactref
// description
// dimension
// frame
// gate
// grid
// hole
// layer
// note
// pad
// param
// pin
// pinref
// port
// portref
// rectangle
// setting
// smd
// textvariant
// variantdef
// vertex
// via
// wire
