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



void eagleToKicadAlignment(EDA_TEXT* aText, int aEagleAlignment)
{

    switch( aEagleAlignment )
    {
    case ETEXT::CENTER:
        // this was the default in eda_text's constructor
        break;

    case ETEXT::CENTER_LEFT:
        aText->SetHorizJustify( GR_TEXT_HJUSTIFY_LEFT );
        break;

    case ETEXT::CENTER_RIGHT:
        aText->SetHorizJustify( GR_TEXT_HJUSTIFY_RIGHT );
        break;

    case ETEXT::TOP_CENTER:
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
        aText->SetHorizJustify( GR_TEXT_HJUSTIFY_LEFT );
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
        std::unique_ptr<EPART> epart( new EPART( partNode ) );
        const string& name = epart->name;
        m_partlist[name] = epart.release();
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
        auto elib = loadLibrary( libraryNode );
        m_eaglelibraries[elib->name] = elib;
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
        int x, y;
        x = 1;
        y = 1;

        while( sheetNode )
        {
            wxPoint pos = wxPoint( x * 1000, y * 1000 );
            std::unique_ptr<SCH_SHEET> sheet( new SCH_SHEET( pos ) );
            SCH_SCREEN* screen = new SCH_SCREEN( m_kiway );

            sheet->SetTimeStamp( GetNewTimeStamp() );
            sheet->SetParent( m_rootSheet->GetScreen() );
            sheet->SetScreen( screen );

            m_currentSheet = sheet.get();
            loadSheet( sheetNode );
            sheet->GetScreen()->SetFileName( sheet->GetFileName() );
            m_rootSheet->GetScreen()->Append( sheet.release() );

            sheetNode = sheetNode->GetNext();
            x += 2;

            if( x > 10 )
            {
                x = 1;
                y += 2;
            }
        }
    }
    else
    {
        while( sheetNode )
        {
            m_currentSheet = m_rootSheet;
            loadSheet( sheetNode );
            sheetNode = sheetNode->GetNext();
        }
    }
}


void SCH_EAGLE_PLUGIN::loadSheet( wxXmlNode* aSheetNode )
{
    // Map all children into a readable dictionary
    NODE_MAP sheetChildren = mapChildren( aSheetNode );

    // Get description node

    wxXmlNode* descriptionNode = getChildrenNodes( sheetChildren, "description" );

    if( descriptionNode )
    {
        wxString des = descriptionNode->GetContent();
        m_currentSheet->SetName( des );

        std::string filename = des.ToStdString();
        ReplaceIllegalFileNameChars( &filename );
        replace( filename.begin(), filename.end(), ' ', '_' );

        wxString fn = wxString( filename + ".sch" );
        m_currentSheet->SetFileName( fn );
        wxFileName fileName = m_currentSheet->GetFileName();
        m_currentSheet->GetScreen()->SetFileName( fileName.GetFullPath() );
    }

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

    addBusEntries();

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

        /*
        if(labelled == false && wire != NULL )
        {
            wxString netname = netName;
            netname.Replace("!", "~");
            if(m_NetCounts[netName.ToStdString()]>1){
                std::unique_ptr<SCH_GLOBALLABEL> glabel( new SCH_GLOBALLABEL );
                glabel->SetPosition( wire->GetStartPoint() );
                glabel->SetText( netname);
                glabel->SetTextSize( wxSize( GetDefaultTextSize(), GetDefaultTextSize() ) );
                screen->Append( glabel.release() );
            }
            else if ( segmentCount > 1)
            {
                std::unique_ptr<SCH_LABEL> label( new SCH_LABEL );
                label->SetPosition( wire->GetStartPoint() );
                label->SetText( netname );
                label->SetTextSize( wxSize( GetDefaultTextSize(), GetDefaultTextSize() ) );
                screen->Append( label.release() );
            }
        }
        */


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


    EPART* epart = m_partlist[einstance.part];

    std::string gatename = epart->deviceset + epart->device + einstance.gate;
    std::string symbolname = epart->deviceset + epart->device;
    int unit = m_eaglelibraries[epart->library]->gate_unit[gatename];
    std::string package = m_eaglelibraries[epart->library]->package[symbolname];

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
        component->GetField( VALUE )->SetVisible( true );
    }
    else
    {
        component->GetField( VALUE )->SetVisible( false );
    }

    if(part->GetField(REFERENCE)->IsVisible())
    component->GetField( REFERENCE )->SetVisible( true );

    else
    component->GetField( REFERENCE )->SetVisible( false );

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
                eagleToKicadAlignment((EDA_TEXT*)field, align);
            }

            else if (attr.name == "VALUE"){
                field = component->GetField( VALUE );
                field->SetPosition( wxPoint(*attr.x* EUNIT_TO_MIL, *attr.y*-EUNIT_TO_MIL) );
                int align = attr.align ? *attr.align : ETEXT::BOTTOM_LEFT;
                eagleToKicadAlignment((EDA_TEXT*)field, align);
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


EAGLE_LIBRARY* SCH_EAGLE_PLUGIN::loadLibrary( wxXmlNode* aLibraryNode )
{
    unique_ptr<EAGLE_LIBRARY> elib( new EAGLE_LIBRARY );

    std::map<std::string, wxXmlNode*> gate;

    // Read the library name
    wxString libName = aLibraryNode->GetAttribute( "name" );
    elib.get()->name = libName.ToStdString();

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
        wxString symbolName = symbolNode->GetAttribute( "name" );
        elib->symbolnodes[symbolName.ToStdString()] = symbolNode;
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
                elib.get()->package[symbolName.ToStdString()] = edevice.package.Get();

            // Create kicad symbol.
            unique_ptr<LIB_PART> kpart( new LIB_PART( symbolName ) );

            // Process each gate in the deviceset for this device.
            wxXmlNode* gateNode = getChildrenNodes( aDeviceSetChildren, "gates" );
            int gates_count = countChildren( aDeviceSetChildren["gates"], "gate" );
            kpart->SetUnitCount( gates_count );
            int gateindex;

            if( gates_count>1 )
            {
                gateindex = 1;
            }
            else
            {
                gateindex = 0;
            }

            while( gateNode )
            {
                EGATE egate = EGATE( gateNode );

                elib.get()->gate_unit[edeviceset.name + edevice.name + egate.name] = gateindex;


                loadSymbol( elib->symbolnodes[egate.symbol],
                        (LIB_PART*) kpart.get(), &edevice, gateindex, egate.name );

                gateindex++;
                gateNode = gateNode->GetNext();
            }    // gateNode

            kpart->SetUnitCount( gates_count );


            const string& name = kpart->GetName().ToStdString();
            m_partlib->AddPart( kpart.get() );
            elib->kicadsymbols[name] = kpart.release();

            deviceNode = deviceNode->GetNext();
        }    // devicenode

        devicesetNode = devicesetNode->GetNext();
    }    // devicesetNode

    return elib.release();
}


void SCH_EAGLE_PLUGIN::loadSymbol( wxXmlNode* aSymbolNode,
        LIB_PART* aPart,
        EDEVICE* aDevice,
        int gateNumber,
        string gateName )
{
    wxString symbolName = aSymbolNode->GetAttribute( "name" );
    std::vector<LIB_ITEM*> items;

    wxXmlNode* currentNode = aSymbolNode->GetChildren();

    bool foundName = false;
    bool foundValue = false;

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
            LIB_CIRCLE* circle = loadSymbolCircle( aPart, currentNode );
            circle->SetUnit( gateNumber );
            aPart->AddDrawItem( circle );
        }
        else if( nodeName == "pin" )
        {
            LIB_PIN* pin = loadPin( aPart, currentNode );

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


                        string pinname = pin->GetName().ToStdString();
                        if(pinname[0] == '!'){
                            pinname[0] = '~';
                            pin->SetName( pinname );
                        }

                        aPart->AddDrawItem( pin );
                        break;
                    }
                }
            }
            else
            {
                pin->SetPartNumber( gateNumber );
                pin->SetUnit( gateNumber );
                aPart->AddDrawItem( pin );
                break;
            }
        }
        else if( nodeName == "polygon" )
        {
            LIB_POLYLINE* libpoly = loadSymbolPolyLine( aPart, currentNode );
            libpoly->SetUnit( gateNumber );
            aPart->AddDrawItem(libpoly);
        }
        else if( nodeName == "rectangle" )
        {
            LIB_RECTANGLE* rectangle = loadSymbolRectangle( aPart, currentNode );
            rectangle->SetUnit( gateNumber );
            aPart->AddDrawItem( rectangle );
        }
        else if( nodeName == "text" )
        {
            LIB_TEXT* libtext = loadSymboltext( aPart, currentNode );
            libtext->SetUnit( gateNumber );



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
                aPart->AddDrawItem( libtext );
            }
        }
        else if( nodeName == "wire" )
        {
            LIB_POLYLINE* pline = loadSymbolWire( aPart, currentNode );
            pline->SetUnit( gateNumber );
            aPart->AddDrawItem( pline );
        }

        currentNode = currentNode->GetNext();
    }

    if( foundName == false )
        aPart->GetField( REFERENCE )->SetVisible(false);
    if( foundValue == false )
        aPart->GetField( VALUE )->SetVisible(false);

}


LIB_CIRCLE* SCH_EAGLE_PLUGIN::loadSymbolCircle( LIB_PART* aPart, wxXmlNode* aCircleNode )
{
    // Parse the circle properties
    ECIRCLE c( aCircleNode );

    unique_ptr<LIB_CIRCLE> circle( new LIB_CIRCLE( aPart ) );

    circle->SetPosition( wxPoint( c.x * EUNIT_TO_MIL, c.y * EUNIT_TO_MIL ) );
    circle->SetRadius( c.radius * EUNIT_TO_MIL );
    circle->SetWidth( c.width * EUNIT_TO_MIL );

    return circle.release();
}


LIB_RECTANGLE* SCH_EAGLE_PLUGIN::loadSymbolRectangle( LIB_PART* aPart, wxXmlNode* aRectNode )
{
    ERECT rect( aRectNode );

    unique_ptr<LIB_RECTANGLE> rectangle( new LIB_RECTANGLE( aPart ) );

    rectangle->SetPosition( wxPoint( rect.x1 * EUNIT_TO_MIL, rect.y1 * EUNIT_TO_MIL ) );
    rectangle->SetEnd( wxPoint( rect.x2 * EUNIT_TO_MIL, rect.y2 * EUNIT_TO_MIL ) );

    // TODO: Manage rotation
    // Probaly using a transform.

    // Eagle rectangles are filled by definition.
    rectangle->SetFillMode(FILLED_SHAPE);

    return rectangle.release();
}


LIB_POLYLINE* SCH_EAGLE_PLUGIN::loadSymbolWire( LIB_PART* aPart, wxXmlNode* aWireNode )
{
    // TODO: Layer map
    std::unique_ptr<LIB_POLYLINE> polyLine( new LIB_POLYLINE( aPart ) );

    auto ewire = EWIRE( aWireNode );
    wxPoint begin, end;

    begin.x = ewire.x1 * EUNIT_TO_MIL;
    begin.y = ewire.y1 * EUNIT_TO_MIL;
    end.x   = ewire.x2 * EUNIT_TO_MIL;
    end.y   = ewire.y2 * EUNIT_TO_MIL;

    polyLine->AddPoint( begin );
    polyLine->AddPoint( end );

    return polyLine.release();
}


LIB_POLYLINE* SCH_EAGLE_PLUGIN::loadSymbolPolyLine( LIB_PART* aPart, wxXmlNode* aPolygonNode )
{
    // TODO: Layer map
    std::unique_ptr<LIB_POLYLINE> polyLine( new LIB_POLYLINE( aPart ) );

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

    return polyLine.release();
}


LIB_PIN* SCH_EAGLE_PLUGIN::loadPin( LIB_PART* aPart, wxXmlNode* aPin )
{
    std::unique_ptr<LIB_PIN> pin( new LIB_PIN( aPart ) );

    auto epin = EPIN( aPin );

    pin->SetPosition( wxPoint( epin.x * EUNIT_TO_MIL, epin.y * EUNIT_TO_MIL ) );
    pin->SetName( epin.name );

    int roti = 0;

    if( epin.rot )
    {
        roti = int(epin.rot->degrees);
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

    if( epin.length )
    {
        wxString length = epin.length.Get();

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

    if( epin.visible )
    {
        wxString visible = epin.visible.Get();

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


LIB_TEXT* SCH_EAGLE_PLUGIN::loadSymboltext( LIB_PART* aPart, wxXmlNode* aLibText )
{
    std::unique_ptr<LIB_TEXT> libtext( new LIB_TEXT( aPart ) );

    auto etext = ETEXT( aLibText );

    libtext->SetPosition( wxPoint( etext.x * EUNIT_TO_MIL, etext.y * EUNIT_TO_MIL ) );
    libtext->SetText( aLibText->GetNodeContent() );
    libtext->SetTextSize( wxSize( int(etext.size * EUNIT_TO_MIL*0.95),
                    int(etext.size * EUNIT_TO_MIL*0.95) ) );

    if( etext.ratio )
    {
        if( etext.ratio.Get()>12 )
        {
            libtext->SetBold( true );
            libtext->SetThickness( GetPenSizeForBold( libtext->GetTextWidth() ) );
        }
    }

    int align = etext.align ? *etext.align : ETEXT::BOTTOM_LEFT;

    eagleToKicadAlignment((EDA_TEXT*)libtext.get(), align);


    return libtext.release();
}


SCH_TEXT* SCH_EAGLE_PLUGIN::loadplaintext( wxXmlNode* aSchText )
{
    std::unique_ptr<SCH_TEXT> schtext( new SCH_TEXT() );

    auto etext = ETEXT( aSchText );


    schtext->SetItalic( false );
    schtext->SetPosition( wxPoint( etext.x * EUNIT_TO_MIL, -etext.y * EUNIT_TO_MIL ) );
    schtext->SetText( aSchText->GetNodeContent() );

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

    eagleToKicadAlignment((EDA_TEXT*)schtext.get(), align);


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
    tempFile.Close();

    return firstline.StartsWith( "<?xml" );
}


void SCH_EAGLE_PLUGIN::addBusEntries()
{
    // Add bus entry symbols

    // for each wire segment, compare each end with all busess.
    // If the wire end is found to end on a bus segment and is perpendicular to it, place a bus entry symbol.

    SCH_ITEM* item = m_currentSheet->GetScreen()->GetDrawItems();

    for( ; item != NULL; item = item->Next() )
    {
        // Check item type for line
        if( item->Type() != SCH_LINE_T )
            continue;

        // Check item type for wire
        if( ( (SCH_LINE*) item )->GetLayer() != LAYER_WIRE )
            continue;

        SCH_ITEM* testitem = m_currentSheet->GetScreen()->GetDrawItems();

        for( ; testitem != NULL; testitem = testitem->Next() )
        {
            // Check item type for line
            if( testitem->Type() != SCH_LINE_T )
                continue;

            // Check item type for bus
            if( ( (SCH_LINE*) testitem )->GetLayer() != LAYER_BUS )
                continue;

            // Get points of both segments.
            wxPoint itemstart = ( (SCH_LINE*) item )->GetStartPoint();
            wxPoint itemend = ( (SCH_LINE*) item )->GetEndPoint();
            wxPoint testitemstart = ( (SCH_LINE*) testitem )->GetStartPoint();
            wxPoint testitemend = ( (SCH_LINE*) testitem )->GetEndPoint();

            // Test for horizontal wire and         vertical bus
            if( itemstart.y == itemend.y && testitemstart.x == testitemend.x )
            {
                if( TestSegmentHit( itemstart, testitemstart, testitemend, 0 ) )
                {
                    // Wire start is on a bus.
                    SCH_MARKER* marker =
                        new SCH_MARKER( itemstart, "horizontal wire, vertical bus" );
                    m_currentSheet->GetScreen()->Append( marker );

                    // Wire start is on the vertical bus

                    // if the end of the wire is to the left of the bus
                    if( itemend.x < testitemstart.x )
                    {
                        // |
                        // ---|
                        // |
                        if( TestSegmentHit( itemstart + wxPoint( 0, -100 ), testitemstart,
                                    testitemend, 0 ) )
                        {
                            // |
                            // ___/|
                            SCH_BUS_WIRE_ENTRY* busEntry = new SCH_BUS_WIRE_ENTRY( itemstart + wxPoint(
                                            -100,
                                            0 ),
                                    '/' );
                            busEntry->SetFlags( IS_NEW );
                            m_currentSheet->GetScreen()->Append( busEntry );
                        }
                        else if( TestSegmentHit( itemstart + wxPoint( 0, 100 ), testitemstart,
                                         testitemend, 0 ) )
                        {
                            SCH_BUS_WIRE_ENTRY* busEntry = new SCH_BUS_WIRE_ENTRY( itemstart + wxPoint(
                                            -100,
                                            0 ),
                                    '\\' );
                            busEntry->SetFlags( IS_NEW );
                            m_currentSheet->GetScreen()->Append( busEntry );
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
                        if( TestSegmentHit( itemstart + wxPoint( 0, -100 ), testitemstart,
                                    testitemend, 0 ) )
                        {
                            SCH_BUS_WIRE_ENTRY* busEntry = new SCH_BUS_WIRE_ENTRY( itemstart + wxPoint(
                                            0,
                                            -100 ),
                                    '\\' );
                            busEntry->SetFlags( IS_NEW );
                            m_currentSheet->GetScreen()->Append( busEntry );
                        }
                        // test is bus exists below the wire
                        else if( TestSegmentHit( itemstart + wxPoint( 0, 100 ), testitemstart,
                                         testitemend, 0 ) )
                        {
                            SCH_BUS_WIRE_ENTRY* busEntry = new SCH_BUS_WIRE_ENTRY( itemstart + wxPoint(
                                            0,
                                            100 ),
                                    '/' );
                            busEntry->SetFlags( IS_NEW );
                            m_currentSheet->GetScreen()->Append( busEntry );
                        }
                    }
                }

                // Same thing but test end of the wire instead.
                if( TestSegmentHit( itemend, testitemstart, testitemend, 0 ) )
                {
                    SCH_MARKER* marker = new SCH_MARKER( itemend, "horizontal wire, vertical bus" );

                    m_currentSheet->GetScreen()->Append( marker );

                    // Wire end is on the vertical bus

                    // if the start of the wire is to the left of the bus
                    if( itemstart.x < testitemstart.x )
                    {
                        // Test if bus exists above the wire
                        if( TestSegmentHit( itemend + wxPoint( 0, 100 ), testitemstart, testitemend,
                                    0 ) )
                        {
                            // |
                            // ___/|
                            SCH_BUS_WIRE_ENTRY* busEntry = new SCH_BUS_WIRE_ENTRY( itemend + wxPoint(
                                            -100,
                                            0 ),
                                    '\\' );
                            busEntry->SetFlags( IS_NEW );
                            m_currentSheet->GetScreen()->Append( busEntry );
                        }
                        // Test if bus exists below the wire
                        else if( TestSegmentHit( itemend + wxPoint( 0, -100 ), testitemstart,
                                         testitemend, 0 ) )
                        {
                            SCH_BUS_WIRE_ENTRY* busEntry = new SCH_BUS_WIRE_ENTRY( itemend + wxPoint(
                                            -100,
                                            0 ),
                                    '/' );
                            busEntry->SetFlags( IS_NEW );
                            m_currentSheet->GetScreen()->Append( busEntry );
                        }
                    }
                    // else the start of the wire is to the right of the bus
                    // |
                    // |----
                    // |
                    else
                    {
                        // test if bus existed above the wire
                        if( TestSegmentHit( itemend + wxPoint( 0, -100 ), testitemstart,
                                    testitemend, 0 ) )
                        {
                            SCH_BUS_WIRE_ENTRY* busEntry = new SCH_BUS_WIRE_ENTRY( itemend + wxPoint(
                                            0,
                                            -100 ),
                                    '\\' );
                            busEntry->SetFlags( IS_NEW );
                            m_currentSheet->GetScreen()->Append( busEntry );
                        }
                        // test if bus existed below the wire
                        else if( TestSegmentHit( itemend + wxPoint( 0, 100 ), testitemstart,
                                         testitemend, 0 ) )
                        {
                            SCH_BUS_WIRE_ENTRY* busEntry = new SCH_BUS_WIRE_ENTRY( itemend + wxPoint(
                                            0,
                                            100 ),
                                    '/' );
                            busEntry->SetFlags( IS_NEW );
                            m_currentSheet->GetScreen()->Append( busEntry );
                        }
                    }
                }
            }    // if( itemstart.y == itemend.y && testitemstart.x == testitemend.x)

            // Test for horizontal wire and vertical bus
            if( itemstart.x == itemend.x && testitemstart.y == testitemend.y )
            {
                if( TestSegmentHit( itemstart, testitemstart, testitemend, 0 ) )
                {
                    // Wire start is on the bus
                    // If wire end is above the bus,
                    if( itemend.y < testitemstart.y )
                    {
                        // Test for bus existance to the left of the wire
                        if( TestSegmentHit( itemstart + wxPoint( -100, 0 ), testitemstart,
                                    testitemend, 0 ) )
                        {
                            SCH_BUS_WIRE_ENTRY* busEntry = new SCH_BUS_WIRE_ENTRY( itemstart + wxPoint(
                                            -100,
                                            0 ),
                                    '/' );
                            busEntry->SetFlags( IS_NEW );
                            m_currentSheet->GetScreen()->Append( busEntry );
                            SCH_MARKER* marker = new SCH_MARKER( itemstart,
                                    "vertical wire, horizontal bus, above, left of wire" );

                            m_currentSheet->GetScreen()->Append( marker );
                        }
                        else if( TestSegmentHit( itemstart + wxPoint( 100, 0 ), testitemstart,
                                         testitemend, 0 ) )
                        {
                            SCH_BUS_WIRE_ENTRY* busEntry = new SCH_BUS_WIRE_ENTRY( itemstart + wxPoint(
                                            0,
                                            100 ),
                                    '\\' );
                            busEntry->SetFlags( IS_NEW );
                            m_currentSheet->GetScreen()->Append( busEntry );
                            SCH_MARKER* marker = new SCH_MARKER( itemstart,
                                    "vertical wire, horizontal bus, above, right of wire" );

                            m_currentSheet->GetScreen()->Append( marker );
                        }
                    }
                    else    // wire end is below the bus.
                    {
                        // Test for bus existance to the left of the wire
                        if( TestSegmentHit( itemstart + wxPoint( -100, 0 ), testitemstart,
                                    testitemend, 0 ) )
                        {
                            SCH_BUS_WIRE_ENTRY* busEntry = new SCH_BUS_WIRE_ENTRY( itemstart + wxPoint(
                                            -100,
                                            0 ),
                                    '\\' );
                            busEntry->SetFlags( IS_NEW );
                            m_currentSheet->GetScreen()->Append( busEntry );
                            SCH_MARKER* marker = new SCH_MARKER( itemstart,
                                    "vertical wire, horizontal bus, below, left of wire" );

                            m_currentSheet->GetScreen()->Append( marker );
                        }
                        else if( TestSegmentHit( itemstart + wxPoint( 100, 0 ), testitemstart,
                                         testitemend, 0 ) )
                        {
                            SCH_BUS_WIRE_ENTRY* busEntry = new SCH_BUS_WIRE_ENTRY( itemstart + wxPoint(
                                            100,
                                            0 ),
                                    '/' );
                            busEntry->SetFlags( IS_NEW );
                            m_currentSheet->GetScreen()->Append( busEntry );
                            SCH_MARKER* marker = new SCH_MARKER( itemstart,
                                    "vertical wire, horizontal bus, below, right of wire" );

                            m_currentSheet->GetScreen()->Append( marker );
                        }
                    }
                }

                if( TestSegmentHit( itemend, testitemstart, testitemend, 0 ) )
                {
                    // Wire start is on the bus
                    // If wire end is above the bus,
                    if( itemstart.y < testitemstart.y )
                    {
                        // Test for bus existance to the left of the wire
                        if( TestSegmentHit( itemend + wxPoint( -100, 0 ), testitemstart,
                                    testitemend, 0 ) )
                        {
                            SCH_BUS_WIRE_ENTRY* busEntry = new SCH_BUS_WIRE_ENTRY( itemend + wxPoint(
                                            -100,
                                            0 ),
                                    '/' );
                            busEntry->SetFlags( IS_NEW );
                            m_currentSheet->GetScreen()->Append( busEntry );
                            SCH_MARKER* marker = new SCH_MARKER( itemend,
                                    "vertical wire, horizontal bus, above, left of wire, end" );

                            m_currentSheet->GetScreen()->Append( marker );
                        }
                        else if( TestSegmentHit( itemend + wxPoint( 100, 0 ), testitemstart,
                                         testitemend, 0 ) )
                        {
                            SCH_BUS_WIRE_ENTRY* busEntry = new SCH_BUS_WIRE_ENTRY( itemend + wxPoint(
                                            0,
                                            -100 ),
                                    '\\' );
                            busEntry->SetFlags( IS_NEW );
                            m_currentSheet->GetScreen()->Append( busEntry );
                            SCH_MARKER* marker = new SCH_MARKER( itemend,
                                    "vertical wire, horizontal bus, above, right of wire, end" );

                            m_currentSheet->GetScreen()->Append( marker );
                        }
                    }
                    else    // wire end is below the bus.
                    {
                        // Test for bus existance to the left of the wire
                        if( TestSegmentHit( itemend + wxPoint( -100, 0 ), testitemstart,
                                    testitemend, 0 ) )
                        {
                            SCH_BUS_WIRE_ENTRY* busEntry = new SCH_BUS_WIRE_ENTRY( itemend + wxPoint(
                                            -100,
                                            0 ),
                                    '\\' );
                            busEntry->SetFlags( IS_NEW );
                            m_currentSheet->GetScreen()->Append( busEntry );
                            SCH_MARKER* marker = new SCH_MARKER( itemend,
                                    "vertical wire, horizontal bus, below, left of wire, end" );

                            m_currentSheet->GetScreen()->Append( marker );
                        }
                        else if( TestSegmentHit( itemend + wxPoint( 100, 0 ), testitemstart,
                                         testitemend, 0 ) )
                        {
                            SCH_BUS_WIRE_ENTRY* busEntry = new SCH_BUS_WIRE_ENTRY( itemend + wxPoint(
                                            0,
                                            100 ),
                                    '/' );
                            busEntry->SetFlags( IS_NEW );
                            m_currentSheet->GetScreen()->Append( busEntry );
                            SCH_MARKER* marker = new SCH_MARKER( itemend,
                                    "vertical wire, horizontal bus, below, right of wire, end" );

                            m_currentSheet->GetScreen()->Append( marker );
                        }
                    }
                }
            }
        }
    }
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
