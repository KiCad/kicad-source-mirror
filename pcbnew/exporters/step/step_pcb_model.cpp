/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2022 Mark Roszko <mark.roszko@gmail.com>
 * Copyright (C) 2016 Cirilo Bernardo <cirilo.bernardo@gmail.com>
 * Copyright (C) 2016-2023 KiCad Developers, see AUTHORS.txt for contributors.
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

#include <algorithm>
#include <cmath>
#include <sstream>
#include <string>
#include <utility>
#include <wx/filename.h>
#include <wx/filefn.h>
#include <wx/stdpaths.h>
#include <wx/wfstream.h>
#include <wx/zipstrm.h>

#include <decompress.hpp>

#include <footprint.h>
#include <pad.h>

#include "step_pcb_model.h"
#include "streamwrapper.h"

#include <IGESCAFControl_Reader.hxx>
#include <IGESCAFControl_Writer.hxx>
#include <IGESControl_Controller.hxx>
#include <IGESData_GlobalSection.hxx>
#include <IGESData_IGESModel.hxx>
#include <Interface_Static.hxx>
#include <Quantity_Color.hxx>
#include <STEPCAFControl_Reader.hxx>
#include <STEPCAFControl_Writer.hxx>
#include <APIHeaderSection_MakeHeader.hxx>
#include <Standard_Version.hxx>
#include <TCollection_ExtendedString.hxx>
#include <TDataStd_Name.hxx>
#include <TDataStd_TreeNode.hxx>
#include <TDF_LabelSequence.hxx>
#include <TDF_ChildIterator.hxx>
#include <TopExp_Explorer.hxx>
#include <XCAFDoc.hxx>
#include <XCAFDoc_DocumentTool.hxx>
#include <XCAFDoc_ColorTool.hxx>

#include <BRep_Tool.hxx>
#include <BRepMesh_IncrementalMesh.hxx>
#include <BRepBuilderAPI.hxx>
#include <BRepBuilderAPI_MakeEdge.hxx>
#include <BRepBuilderAPI_Transform.hxx>
#include <BRepBuilderAPI_GTransform.hxx>
#include <BRepBuilderAPI_MakeFace.hxx>
#include <BRepPrimAPI_MakePrism.hxx>
#include <BRepPrimAPI_MakeCylinder.hxx>
#include <BRepAlgoAPI_Cut.hxx>

#include <TopoDS.hxx>
#include <TopoDS_Wire.hxx>
#include <TopoDS_Face.hxx>
#include <TopoDS_Compound.hxx>
#include <TopoDS_Builder.hxx>

#include <Standard_Failure.hxx>

#include <gp_Ax2.hxx>
#include <gp_Circ.hxx>
#include <gp_Dir.hxx>
#include <gp_Pnt.hxx>
#include <Geom_BezierCurve.hxx>

#include <macros.h>

static constexpr double USER_PREC = 1e-4;
static constexpr double USER_ANGLE_PREC = 1e-6;

// nominal offset from the board
static constexpr double BOARD_OFFSET = 0.05;

// supported file types for 3D models
enum MODEL3D_FORMAT_TYPE
{
    FMT_NONE,
    FMT_STEP,
    FMT_STEPZ,
    FMT_IGES,
    FMT_EMN,
    FMT_IDF,
    FMT_WRL,
    FMT_WRZ
};


MODEL3D_FORMAT_TYPE fileType( const char* aFileName )
{
    wxFileName lfile( wxString::FromUTF8Unchecked( aFileName ) );

    if( !lfile.FileExists() )
    {
        wxString msg;
        msg.Printf( wxT( " * fileType(): no such file: %s\n" ),
                    wxString::FromUTF8Unchecked( aFileName ) );

        ReportMessage( msg );
        return FMT_NONE;
    }

    wxString ext = lfile.GetExt().Lower();

    if( ext == wxT( "wrl" ) )
        return FMT_WRL;

    if( ext == wxT( "wrz" ) )
        return FMT_WRZ;

    if( ext == wxT( "idf" ) )
        return FMT_IDF;     // component outline

    if( ext == wxT( "emn" ) )
        return FMT_EMN;     // PCB assembly

    if( ext == wxT( "stpz" ) || ext == wxT( "gz" ) )
        return FMT_STEPZ;

    OPEN_ISTREAM( ifile, aFileName );

    if( ifile.fail() )
        return FMT_NONE;

    char iline[82];
    memset( iline, 0, 82 );
    ifile.getline( iline, 82 );
    CLOSE_STREAM( ifile );
    iline[81] = 0;  // ensure NULL termination when string is too long

    // check for STEP in Part 21 format
    // (this can give false positives since Part 21 is not exclusively STEP)
    if( !strncmp( iline, "ISO-10303-21;", 13 ) )
        return FMT_STEP;

    std::string fstr = iline;

    // check for STEP in XML format
    // (this can give both false positive and false negatives)
    if( fstr.find( "urn:oid:1.0.10303." ) != std::string::npos )
        return FMT_STEP;

    // Note: this is a very simple test which can yield false positives; the only
    // sure method for determining if a file *not* an IGES model is to attempt
    // to load it.
    if( iline[72] == 'S' && ( iline[80] == 0 || iline[80] == 13 || iline[80] == 10 ) )
        return FMT_IGES;

    return FMT_NONE;
}


STEP_PCB_MODEL::STEP_PCB_MODEL( const wxString& aPcbName )
{
    m_app = XCAFApp_Application::GetApplication();
    m_app->NewDocument( "MDTV-XCAF", m_doc );
    m_assy = XCAFDoc_DocumentTool::ShapeTool( m_doc->Main() );
    m_assy_label = m_assy->NewShape();
    m_hasPCB = false;
    m_components = 0;
    m_precision = USER_PREC;
    m_angleprec = USER_ANGLE_PREC;
    m_boardThickness = BOARD_THICKNESS_DEFAULT_MM;
    m_copperThickness = COPPER_THICKNESS_DEFAULT_MM;
    m_mergeOCCMaxDist = OCC_MAX_DISTANCE_TO_MERGE_POINTS;
    m_minx = 1.0e10;    // absurdly large number; any valid PCB X value will be smaller
    m_pcbName = aPcbName;
    BRepBuilderAPI::Precision( m_mergeOCCMaxDist );
    m_maxError = pcbIUScale.mmToIU( ARC_TO_SEGMENT_MAX_ERROR_MM );
}


STEP_PCB_MODEL::~STEP_PCB_MODEL()
{
    m_doc->Close();
}

bool STEP_PCB_MODEL::AddPadShape( const PAD* aPad, const VECTOR2D& aOrigin )
{
    const std::shared_ptr<SHAPE_POLY_SET>& pad_shape = aPad->GetEffectivePolygon();
    bool success = true;
    VECTOR2I pos = aPad->GetPosition();

    for( PCB_LAYER_ID pcb_layer = F_Cu; ; pcb_layer = B_Cu )
    {
        TopoDS_Shape curr_shape;
        double Zpos = pcb_layer == F_Cu ? m_boardThickness : -m_copperThickness;

        if( aPad->IsOnLayer( pcb_layer ) )
        {
            // Make a shape on top/bottom copper layer: a cylinder for rond shapes (pad or via)
            // and a polygon for other shapes:
            if( aPad->GetShape() == PAD_SHAPE::CIRCLE )
            {
                curr_shape = BRepPrimAPI_MakeCylinder(
                                pcbIUScale.IUTomm( aPad->GetSizeX() ) * 0.5, m_copperThickness ).Shape();
                gp_Trsf shift;
                shift.SetTranslation( gp_Vec( pcbIUScale.IUTomm( pos.x - aOrigin.x ),
                                              -pcbIUScale.IUTomm( pos.y - aOrigin.y ),
                                              Zpos ) );
                BRepBuilderAPI_Transform round_shape( curr_shape, shift );
                m_board_outlines.push_back( round_shape.Shape() );
            }
            else
            {
                success = MakeShape( curr_shape, pad_shape.get()->COutline(0), m_copperThickness, Zpos, aOrigin );

                if( success )
                    m_board_outlines.push_back( curr_shape );
            }
        }

        if( pcb_layer == B_Cu )
            break;
    }

    if( !success )  // Error
        ReportMessage( wxT( "OCC error adding pad/via polygon.\n" ) );

    return success;
}


bool STEP_PCB_MODEL::AddCopperPolygonShapes( const SHAPE_POLY_SET* aPolyShapes, bool aOnTop, const VECTOR2D& aOrigin )
{
    bool success = true;

    for( int ii = 0; ii < aPolyShapes->OutlineCount(); ii++ )
    {
        TopoDS_Shape copper_shape;
        double z_pos = aOnTop ? m_boardThickness : -m_copperThickness;

        if( MakeShape( copper_shape, aPolyShapes->COutline( ii ), m_copperThickness, z_pos, aOrigin ) )
        {
            m_board_outlines.push_back( copper_shape );
        }
        else
        {
            ReportMessage( wxString::Format( wxT( "Could not add shape (%d points) to copper layer on %s.\n" ),
                           aPolyShapes->COutline( ii ).PointCount(),
                           aOnTop ? wxT( "top" ) : wxT( "bottom" ) ) );
            success = false;
        }
    }

    return success;
}


bool STEP_PCB_MODEL::AddPadHole( const PAD* aPad, const VECTOR2D& aOrigin )
{
    if( aPad == nullptr || !aPad->GetDrillSize().x )
        return false;

    VECTOR2I pos = aPad->GetPosition();
    const double margin = 0.01;     // a small margin on the Z axix to be sure the hole
                                    // is bigget than the board with copper
                                    // must be > OCC_MAX_DISTANCE_TO_MERGE_POINTS
    double holeZsize = m_boardThickness + ( m_copperThickness * 2 ) + ( margin * 2 );

    if( aPad->GetDrillShape() == PAD_DRILL_SHAPE_CIRCLE )
    {
        TopoDS_Shape s =
                BRepPrimAPI_MakeCylinder( pcbIUScale.IUTomm( aPad->GetDrillSize().x ) * 0.5, holeZsize ).Shape();
        gp_Trsf shift;
        shift.SetTranslation( gp_Vec( pcbIUScale.IUTomm( pos.x - aOrigin.x ),
                                      -pcbIUScale.IUTomm( pos.y - aOrigin.y ),
                                      -m_copperThickness - margin ) );
        BRepBuilderAPI_Transform hole( s, shift );
        m_cutouts.push_back( hole.Shape() );
        return true;
    }

    // slotted hole
    SHAPE_POLY_SET holeOutlines;

    if( !aPad->TransformHoleToPolygon( holeOutlines, 0, m_maxError, ERROR_INSIDE ) )
    {
        return false;
    }

    TopoDS_Shape hole;

    if( holeOutlines.OutlineCount() > 0 )
    {
        if( MakeShape( hole, holeOutlines.COutline( 0 ), holeZsize, -m_copperThickness - margin, aOrigin ) )
        {
            m_cutouts.push_back( hole );
        }
    }
    else
    {
        return false;
    }

    return true;
}


bool STEP_PCB_MODEL::AddComponent( const std::string& aFileNameUTF8, const std::string& aRefDes,
                             bool aBottom, VECTOR2D aPosition, double aRotation, VECTOR3D aOffset,
                             VECTOR3D aOrientation, VECTOR3D aScale, bool aSubstituteModels )
{
    if( aFileNameUTF8.empty() )
    {
        ReportMessage( wxString::Format( wxT( "No model defined for component %s.\n" ), aRefDes ) );
        return false;
    }

    wxString fileName( wxString::FromUTF8( aFileNameUTF8.c_str() ) );
    ReportMessage( wxString::Format( wxT( "Add component %s.\n" ), aRefDes ) );

    // first retrieve a label
    TDF_Label lmodel;
    wxString  errorMessage;

    if( !getModelLabel( aFileNameUTF8, aScale, lmodel, aSubstituteModels, &errorMessage ) )
    {
        if( errorMessage.IsEmpty() )
            ReportMessage( wxString::Format( wxT( "No model for filename '%s'.\n" ), fileName ) );
        else
            ReportMessage( errorMessage );

        return false;
    }

    // calculate the Location transform
    TopLoc_Location toploc;

    if( !getModelLocation( aBottom, aPosition, aRotation, aOffset, aOrientation, toploc ) )
    {
        ReportMessage(
                wxString::Format( wxT( "No location data for filename '%s'.\n" ), fileName ) );
        return false;
    }

    // add the located sub-assembly
    TDF_Label llabel = m_assy->AddComponent( m_assy_label, lmodel, toploc );

    if( llabel.IsNull() )
    {
        ReportMessage( wxString::Format( wxT( "Could not add component with filename '%s'.\n" ),
                                         fileName ) );
        return false;
    }

    // attach the RefDes name
    TCollection_ExtendedString refdes( aRefDes.c_str() );
    TDataStd_Name::Set( llabel, refdes );

    return true;
}


void STEP_PCB_MODEL::SetPCBThickness( double aThickness )
{
    if( aThickness < 0.0 )
        m_boardThickness = BOARD_THICKNESS_DEFAULT_MM;
    else if( aThickness < BOARD_THICKNESS_MIN_MM )
        m_boardThickness = BOARD_THICKNESS_MIN_MM;
    else
        m_boardThickness = aThickness;
}


void STEP_PCB_MODEL::SetBoardColor( double r, double g, double b )
{
    m_boardColor[0] = r;
    m_boardColor[1] = g;
    m_boardColor[2] = b;
}


void STEP_PCB_MODEL::SetCopperColor( double r, double g, double b )
{
    m_copperColor[0] = r;
    m_copperColor[1] = g;
    m_copperColor[2] = b;
}


void STEP_PCB_MODEL::OCCSetMergeMaxDistance( double aDistance )
{
    // Ensure a minimal value (in mm)
    m_mergeOCCMaxDist = aDistance;
    BRepBuilderAPI::Precision( m_mergeOCCMaxDist );
}

bool STEP_PCB_MODEL::isBoardOutlineValid()
{
    return m_pcb_labels.size() > 0;
}

// A helper function to know if a SHAPE_LINE_CHAIN is encoding a circle
static bool IsChainCircle( const SHAPE_LINE_CHAIN& aChain )
{
    // If aChain is a circle it
    // - contains only one arc
    // - this arc has the same start and end point
    const std::vector<SHAPE_ARC>& arcs = aChain.CArcs();

    if( arcs.size() == 1 )
    {
        const SHAPE_ARC& arc = arcs[0];

        if( arc. GetP0() == arc.GetP1() )
            return true;
    }

    return false;
}

bool STEP_PCB_MODEL::MakeShapeAsCylinder( TopoDS_Shape& aShape,
                                          const SHAPE_LINE_CHAIN& aChain, double aThickness,
                                          double aZposition, const VECTOR2D& aOrigin )
{
    if( !aShape.IsNull() )
        return false; // there is already data in the shape object

    if( !aChain.IsClosed() )
        return false; // the loop is not closed

    const std::vector<SHAPE_ARC>& arcs = aChain.CArcs();
    const SHAPE_ARC& arc = arcs[0];

    TopoDS_Shape base_shape;
    base_shape = BRepPrimAPI_MakeCylinder(
                        pcbIUScale.IUTomm( arc.GetRadius() ), aThickness ).Shape();
    gp_Trsf shift;
    shift.SetTranslation( gp_Vec( pcbIUScale.IUTomm( arc.GetCenter().x - aOrigin.x ),
                                  -pcbIUScale.IUTomm( arc.GetCenter().y - aOrigin.y ),
                                  aZposition ) );
    BRepBuilderAPI_Transform round_shape( base_shape, shift );
    aShape = round_shape;

    if( aShape.IsNull() )
    {
        ReportMessage( wxT( "failed to create a cylinder vertical shape\n" ) );
        return false;
    }

    return true;
}

bool STEP_PCB_MODEL::MakeShape( TopoDS_Shape& aShape, const SHAPE_LINE_CHAIN& aChain,
                                double aThickness, double aZposition, const VECTOR2D& aOrigin )
{
    if( !aShape.IsNull() )
        return false; // there is already data in the shape object

    if( !aChain.IsClosed() )
        return false; // the loop is not closed

    // a SHAPE_LINE_CHAIN that is in fact a circle (one 360deg arc) is exported as cylinder
    if( IsChainCircle( aChain ) )
        return MakeShapeAsCylinder( aShape, aChain, aThickness, aZposition, aOrigin );

    BRepBuilderAPI_MakeWire wire;
    bool success = true;

    gp_Pnt start = gp_Pnt( pcbIUScale.IUTomm( aChain.CPoint( 0 ).x - aOrigin.x ),
                           -pcbIUScale.IUTomm( aChain.CPoint( 0 ).y - aOrigin.y ), aZposition );

    int items_count = 0;

    for( int j = 0; j < aChain.PointCount(); j++ )
    {
        int next = j+1;

        gp_Pnt end;

        if( next >= aChain.PointCount() )
            next = 0;

        end = gp_Pnt( pcbIUScale.IUTomm( aChain.CPoint( next ).x - aOrigin.x ),
                      -pcbIUScale.IUTomm( aChain.CPoint( next ).y - aOrigin.y ), aZposition );

        // Do not export too short segments: they create broken shape because OCC thinks
        // start point and end point are at the same place
        double seg_len = std::hypot( end.X() - start.X(), end.Y() - start.Y() );

        if( seg_len <= m_mergeOCCMaxDist )
            continue;
        try
        {
            TopoDS_Edge edge;
            edge = BRepBuilderAPI_MakeEdge( start, end );
            wire.Add( edge );

            if( wire.Error() != BRepBuilderAPI_WireDone )
            {
                ReportMessage( wxT( "failed to add curve\n" ) );
                return false;
            }

            items_count++;
        }
        catch( const Standard_Failure& e )
        {
            ReportMessage( wxString::Format( wxT( "MakeShape1: OCC exception: %s\n" ),
                                             e.GetMessageString() ) );
            success = false;
        }

        if( !success )
        {
            ReportMessage( wxS( "failed to add edge\n" ) );
            return false;
        }

        start = end;
    }

    BRepBuilderAPI_MakeFace face;

    try
    {
        face = BRepBuilderAPI_MakeFace( wire );
    }
    catch(  const Standard_Failure& e )
    {
        ReportMessage(
                wxString::Format( wxT( "MakeShape2 (items_count %d): OCC exception: %s\n" ),
                                  items_count, e.GetMessageString() ) );
        return false;
    }

    aShape = BRepPrimAPI_MakePrism( face, gp_Vec( 0, 0, aThickness ) );

    if( aShape.IsNull() )
    {
        ReportMessage( wxT( "failed to create a prismatic shape\n" ) );
        return false;
    }

    return true;
}


bool STEP_PCB_MODEL::CreatePCB( SHAPE_POLY_SET& aOutline, VECTOR2D aOrigin )
{
    if( m_hasPCB )
    {
        if( !isBoardOutlineValid() )
            return false;

        return true;
    }

    m_hasPCB = true; // whether or not operations fail we note that CreatePCB has been invoked

    // Number of items having the copper color
    int copper_item_count = m_board_outlines.size();

    // Support for more than one main outline (more than one board)
    for( int cnt = 0; cnt < aOutline.OutlineCount(); cnt++ )
    {
        const SHAPE_LINE_CHAIN& outline = aOutline.COutline( cnt );

        ReportMessage( wxString::Format( wxT( "Build board main outline %d with %d points.\n" ),
                                         cnt+1, outline.PointCount() ) );

        TopoDS_Shape curr_brd;

        if( !MakeShape( curr_brd, outline, m_boardThickness, 0.0, aOrigin ) )
        {
            // Error
            ReportMessage( wxString::Format(
                           wxT( "OCC error adding main outline polygon %d with %d points.\n" ),
                           cnt+1, outline.PointCount() ) );
        }
        else
            m_board_outlines.push_back( curr_brd );

        // Generate board cutouts in current main outline:
        if( aOutline.HoleCount( cnt ) > 0 )
        {
            ReportMessage( wxString::Format( wxT( "Add cutouts in outline %d (%d cutout(s)).\n" ),
                                             cnt+1, aOutline.HoleCount( cnt ) ) );
        }

        for( int ii = 0; ii < aOutline.HoleCount( cnt ); ii++ )
        {
            const SHAPE_LINE_CHAIN& holeOutline = aOutline.Hole( cnt, ii );
            TopoDS_Shape hole;

            if( MakeShape( hole, holeOutline, m_boardThickness + (m_copperThickness*4),
                           -m_copperThickness*2, aOrigin ) )
            {
                m_cutouts.push_back( hole );
            }
        }
    }

    // subtract cutouts (if any)
    if( m_cutouts.size() )
    {
        ReportMessage( wxString::Format( wxT( "Build board cutouts and holes (%d hole(s)).\n" ),
                                         (int) m_cutouts.size() ) );

        TopTools_ListOfShape holelist;

        for( TopoDS_Shape& hole : m_cutouts )
            holelist.Append( hole );

        // Remove holes for each item (board body or bodies, one can have more than one board)
        // and copper items (copper_item_count items)
        int cnt = 0;
        for( TopoDS_Shape& board: m_board_outlines )
        {
            cnt++;

            if( cnt % 10 == 0 )
                ReportMessage( wxString::Format( wxT( "added %d/%d shapes\n" ),
                                                 cnt, (int)m_board_outlines.size() ) );

            TopTools_ListOfShape mainbrd;
            mainbrd.Append( board );

            BRepAlgoAPI_Cut      Cut;
            Cut.SetArguments( mainbrd );

            Cut.SetTools( holelist );
            Cut.Build();

            board = Cut.Shape();
        }
    }

    // push the board to the data structure
    ReportMessage( wxT( "\nGenerate board full shape.\n" ) );

    // Dont expand the component or else coloring it gets hard
    for( TopoDS_Shape& board: m_board_outlines )
    {
        m_pcb_labels.push_back( m_assy->AddComponent( m_assy_label, board, false ) );

        if( m_pcb_labels.back().IsNull() )
            return false;
    }

    // AddComponent adds a label that has a reference (not a parent/child relation) to the real
    // label.  We need to extract that real label to name it for the STEP output cleanly
    // Why are we trying to name the bare board? Because CAD tools like SolidWorks do fun things
    // like "deduplicate" imported STEPs by swapping STEP assembly components with already
    // identically named assemblies.  So we want to avoid having the PCB be generally defaulted
    // to "Component" or "Assembly".

    // Init colors  for the board body and the copper items (if any)
    Handle( XCAFDoc_ColorTool ) colorTool = XCAFDoc_DocumentTool::ColorTool( m_doc->Main() );
    Quantity_Color board_color( m_boardColor[0], m_boardColor[1], m_boardColor[2],
                                Quantity_TOC_RGB );
    Quantity_Color copper_color( m_copperColor[0], m_copperColor[1], m_copperColor[2],
                                 Quantity_TOC_RGB );

    int pcbIdx = 1;
    int copper_objects_cnt = 0;

    for( TDF_Label& pcb_label : m_pcb_labels )
    {
        Handle( TDataStd_TreeNode ) node;

        if( pcb_label.FindAttribute( XCAFDoc::ShapeRefGUID(), node ) )
        {
            // Gives a name to each board object
            TDF_Label label = node->Father()->Label();

            if( !label.IsNull() )
            {
                wxString pcbName;

                // Note, we include the pcb/project name as a prefix
                // because several STEP importing CAD software like SolidWorks
                // will deduplicate anything imported by it's STEP name

                if( copper_objects_cnt < copper_item_count )
                {
                    pcbName = wxString::Format( wxT( "%s_Copper_Item%d" ),
                                                m_pcbName, copper_objects_cnt+1 );
                }
                else
                {
                    if( m_pcb_labels.size() == 1 )
                        pcbName = wxString::Format( wxT( "%s_PCB" ), m_pcbName );
                    else
                        pcbName = wxString::Format( wxT( "%s_PCB%d" ), m_pcbName, pcbIdx++ );
                }

                std::string                pcbNameStdString( pcbName.ToUTF8() );
                TCollection_ExtendedString partname( pcbNameStdString.c_str() );
                TDataStd_Name::Set( label, partname );
            }
        }

        // color the PCB
        TopExp_Explorer topex;
        topex.Init( m_assy->GetShape( pcb_label ), TopAbs_SOLID );

        while( topex.More() )
        {
            // First objects are copper objects, last(s) is the board body
            if( copper_objects_cnt < copper_item_count )
                colorTool->SetColor( topex.Current(), copper_color, XCAFDoc_ColorSurf );
            else
                colorTool->SetColor( topex.Current(), board_color, XCAFDoc_ColorSurf );

            topex.Next();
        }

        copper_objects_cnt++;
    }

#if( defined OCC_VERSION_HEX ) && ( OCC_VERSION_HEX > 0x070101 )
    m_assy->UpdateAssemblies();
#endif

    return true;
}


#ifdef SUPPORTS_IGES
// write the assembly model in IGES format
bool STEP_PCB_MODEL::WriteIGES( const wxString& aFileName )
{
    if( !isBoardOutlineValid() )
    {
        ReportMessage( wxString::Format( wxT( "No valid PCB assembly; cannot create output file "
                                              "'%s'.\n" ),
                                         aFileName ) );
        return false;
    }

    wxFileName fn( aFileName );
    IGESControl_Controller::Init();
    IGESCAFControl_Writer writer;
    writer.SetColorMode( Standard_True );
    writer.SetNameMode( Standard_True );
    IGESData_GlobalSection header = writer.Model()->GlobalSection();
    header.SetFileName( new TCollection_HAsciiString( fn.GetFullName().ToAscii() ) );
    header.SetSendName( new TCollection_HAsciiString( "KiCad electronic assembly" ) );
    header.SetAuthorName(
            new TCollection_HAsciiString( Interface_Static::CVal( "write.iges.header.author" ) ) );
    header.SetCompanyName(
            new TCollection_HAsciiString( Interface_Static::CVal( "write.iges.header.company" ) ) );
    writer.Model()->SetGlobalSection( header );

    if( Standard_False == writer.Perform( m_doc, aFileName.c_str() ) )
        return false;

    return true;
}
#endif


bool STEP_PCB_MODEL::WriteSTEP( const wxString& aFileName )
{
    if( !isBoardOutlineValid() )
    {
        ReportMessage( wxString::Format( wxT( "No valid PCB assembly; cannot create output file "
                                              "'%s'.\n" ),
                                         aFileName ) );
        return false;
    }

    wxFileName fn( aFileName );

    STEPCAFControl_Writer writer;
    writer.SetColorMode( Standard_True );
    writer.SetNameMode( Standard_True );

    // This must be set before we "transfer" the document.
    // Should default to kicad_pcb.general.title_block.title,
    // but in the meantime, defaulting to the basename of the output
    // target is still better than "open cascade step translter v..."
    // UTF8 should be ok from ISO 10303-21:2016, but... older stuff? use boring ascii
    if( !Interface_Static::SetCVal( "write.step.product.name", fn.GetName().ToAscii() ) )
        ReportMessage( wxT( "Failed to set step product name, but will attempt to continue." ) );

    if( Standard_False == writer.Transfer( m_doc, STEPControl_AsIs ) )
        return false;

    APIHeaderSection_MakeHeader hdr( writer.ChangeWriter().Model() );

    // Note: use only Ascii7 chars, non Ascii7 chars (therefore UFT8 chars)
    // are creating issues in the step file
    hdr.SetName( new TCollection_HAsciiString( fn.GetFullName().ToAscii() ) );

    // TODO: how to control and ensure consistency with IGES?
    hdr.SetAuthorValue( 1, new TCollection_HAsciiString( "Pcbnew" ) );
    hdr.SetOrganizationValue( 1, new TCollection_HAsciiString( "Kicad" ) );
    hdr.SetOriginatingSystem( new TCollection_HAsciiString( "KiCad to STEP converter" ) );
    hdr.SetDescriptionValue( 1, new TCollection_HAsciiString( "KiCad electronic assembly" ) );

    bool success = true;

    // Creates a temporary file with a ascii7 name, because writer does not know unicode filenames.
    wxString currCWD = wxGetCwd();
    wxString workCWD = fn.GetPath();

    if( !workCWD.IsEmpty() )
        wxSetWorkingDirectory( workCWD );

    char tmpfname[] = "$tempfile$.step";

    if( Standard_False == writer.Write( tmpfname ) )
        success = false;

    if( success )
    {
        if( !wxRenameFile( tmpfname, fn.GetFullName(), true ) )
        {
            ReportMessage( wxString::Format( wxT( "Cannot rename temporary file '%s' to '%s'.\n" ),
                                             tmpfname,
                                             fn.GetFullName() ) );
            success = false;
        }
    }

    wxSetWorkingDirectory( currCWD );

    return success;
}


bool STEP_PCB_MODEL::getModelLabel( const std::string& aFileNameUTF8, VECTOR3D aScale, TDF_Label& aLabel,
                              bool aSubstituteModels, wxString* aErrorMessage )
{
    std::string model_key = aFileNameUTF8 + "_" + std::to_string( aScale.x )
                            + "_" + std::to_string( aScale.y ) + "_" + std::to_string( aScale.z );

    MODEL_MAP::const_iterator mm = m_models.find( model_key );

    if( mm != m_models.end() )
    {
        aLabel = mm->second;
        return true;
    }

    aLabel.Nullify();

    Handle( TDocStd_Document )  doc;
    m_app->NewDocument( "MDTV-XCAF", doc );

    wxString fileName( wxString::FromUTF8( aFileNameUTF8.c_str() ) );
    MODEL3D_FORMAT_TYPE modelFmt = fileType( aFileNameUTF8.c_str() );

    switch( modelFmt )
    {
    case FMT_IGES:
        if( !readIGES( doc, aFileNameUTF8.c_str() ) )
        {
            ReportMessage( wxString::Format( wxT( "readIGES() failed on filename '%s'.\n" ),
                                             fileName ) );
            return false;
        }
        break;

    case FMT_STEP:
        if( !readSTEP( doc, aFileNameUTF8.c_str() ) )
        {
            ReportMessage( wxString::Format( wxT( "readSTEP() failed on filename '%s'.\n" ),
                                             fileName ) );
            return false;
        }
        break;

    case FMT_STEPZ:
    {
        // To export a compressed step file (.stpz or .stp.gz file), the best way is to
        // decaompress it in a temporaty file and load this temporary file
        wxFFileInputStream ifile( fileName );
        wxFileName         outFile( fileName );

        outFile.SetPath( wxStandardPaths::Get().GetTempDir() );
        outFile.SetExt( wxT( "step" ) );
        wxFileOffset size = ifile.GetLength();

        if( size == wxInvalidOffset )
        {
            ReportMessage( wxString::Format( wxT( "getModelLabel() failed on filename '%s'.\n" ),
                                             fileName ) );
            return false;
        }

        {
            bool                success = false;
            wxFFileOutputStream ofile( outFile.GetFullPath() );

            if( !ofile.IsOk() )
                return false;

            char* buffer = new char[size];

            ifile.Read( buffer, size );
            std::string expanded;

            try
            {
                expanded = gzip::decompress( buffer, size );
                success = true;
            }
            catch( ... )
            {
                ReportMessage( wxString::Format( wxT( "failed to decompress '%s'.\n" ),
                                                 fileName ) );
            }

            if( expanded.empty() )
            {
                ifile.Reset();
                ifile.SeekI( 0 );
                wxZipInputStream            izipfile( ifile );
                std::unique_ptr<wxZipEntry> zip_file( izipfile.GetNextEntry() );

                if( zip_file && !zip_file->IsDir() && izipfile.CanRead() )
                {
                    izipfile.Read( ofile );
                    success = true;
                }
            }
            else
            {
                ofile.Write( expanded.data(), expanded.size() );
            }

            delete[] buffer;
            ofile.Close();

            if( success )
            {
                std::string altFileNameUTF8 = TO_UTF8( outFile.GetFullPath() );
                success =
                        getModelLabel( altFileNameUTF8, VECTOR3D( 1.0, 1.0, 1.0 ), aLabel, false );
            }

            return success;
        }

        break;
    }

    case FMT_WRL:
    case FMT_WRZ:
        /* WRL files are preferred for internal rendering, due to superior material properties, etc.
         * However they are not suitable for MCAD export.
         *
         * If a .wrl file is specified, attempt to locate a replacement file for it.
         *
         * If a valid replacement file is found, the label for THAT file will be associated with
         * the .wrl file
         */
        if( aSubstituteModels )
        {
            wxFileName wrlName( fileName );

            wxString basePath = wrlName.GetPath();
            wxString baseName = wrlName.GetName();

            // List of alternate files to look for
            // Given in order of preference
            // (Break if match is found)
            wxArrayString alts;

            // Step files
            alts.Add( wxT( "stp" ) );
            alts.Add( wxT( "step" ) );
            alts.Add( wxT( "STP" ) );
            alts.Add( wxT( "STEP" ) );
            alts.Add( wxT( "Stp" ) );
            alts.Add( wxT( "Step" ) );
            alts.Add( wxT( "stpz" ) );
            alts.Add( wxT( "stpZ" ) );
            alts.Add( wxT( "STPZ" ) );
            alts.Add( wxT( "step.gz" ) );
            alts.Add( wxT( "stp.gz" ) );

            // IGES files
            alts.Add( wxT( "iges" ) );
            alts.Add( wxT( "IGES" ) );
            alts.Add( wxT( "igs" ) );
            alts.Add( wxT( "IGS" ) );

            //TODO - Other alternative formats?

            for( const auto& alt : alts )
            {
                wxFileName altFile( basePath, baseName + wxT( "." ) + alt );

                if( altFile.IsOk() && altFile.FileExists() )
                {
                    std::string altFileNameUTF8 = TO_UTF8( altFile.GetFullPath() );

                    // When substituting a STEP/IGS file for VRML, do not apply the VRML scaling
                    // to the new STEP model.  This process of auto-substitution is janky as all
                    // heck so let's not mix up un-displayed scale factors with potentially
                    // mis-matched files.  And hope that the user doesn't have multiples files
                    // named "model.wrl" and "model.stp" referring to different parts.
                    // TODO: Fix model handling in v7.  Default models should only be STP.
                    //       Have option to override this in DISPLAY.
                    if( getModelLabel( altFileNameUTF8, VECTOR3D( 1.0, 1.0, 1.0 ), aLabel, false ) )
                    {
                        return true;
                    }
                }
            }

            return false; // No replacement model found
        }
        else // Substitution is not allowed
        {
            if( aErrorMessage )
                aErrorMessage->Printf( wxT( "Cannot add a VRML model to a STEP file.\n" ) );

            return false;
        }

        break;

        // TODO: implement IDF and EMN converters

    default:
        return false;
    }

    aLabel = transferModel( doc, m_doc, aScale );

    if( aLabel.IsNull() )
    {
        ReportMessage( wxString::Format( wxT( "Could not transfer model data from file '%s'.\n" ),
                                         fileName  ) );
        return false;
    }

    // attach the PART NAME ( base filename: note that in principle
    // different models may have the same base filename )
    wxFileName afile( fileName );
    std::string pname( afile.GetName().ToUTF8() );
    TCollection_ExtendedString partname( pname.c_str() );
    TDataStd_Name::Set( aLabel, partname );

    m_models.insert( MODEL_DATUM( model_key, aLabel ) );
    ++m_components;
    return true;
}


bool STEP_PCB_MODEL::getModelLocation( bool aBottom, VECTOR2D aPosition, double aRotation, VECTOR3D aOffset, VECTOR3D aOrientation,
                                 TopLoc_Location& aLocation )
{
    // Order of operations:
    // a. aOrientation is applied -Z*-Y*-X
    // b. aOffset is applied
    //      Top ? add thickness to the Z offset
    // c. Bottom ? Rotate on X axis (in contrast to most ECAD which mirror on Y),
    //             then rotate on +Z
    //    Top ? rotate on -Z
    // d. aPosition is applied
    //
    // Note: Y axis is inverted in KiCad

    gp_Trsf lPos;
    lPos.SetTranslation( gp_Vec( aPosition.x, -aPosition.y, 0.0 ) );

    // Offset board thickness
    aOffset.z += BOARD_OFFSET;

    gp_Trsf lRot;

    if( aBottom )
    {
        lRot.SetRotation( gp_Ax1( gp_Pnt( 0.0, 0.0, 0.0 ), gp_Dir( 0.0, 0.0, 1.0 ) ), aRotation );
        lPos.Multiply( lRot );
        lRot.SetRotation( gp_Ax1( gp_Pnt( 0.0, 0.0, 0.0 ), gp_Dir( 1.0, 0.0, 0.0 ) ), M_PI );
        lPos.Multiply( lRot );
    }
    else
    {
        aOffset.z += m_boardThickness;
        lRot.SetRotation( gp_Ax1( gp_Pnt( 0.0, 0.0, 0.0 ), gp_Dir( 0.0, 0.0, 1.0 ) ), aRotation );
        lPos.Multiply( lRot );
    }

    gp_Trsf lOff;
    lOff.SetTranslation( gp_Vec( aOffset.x, aOffset.y, aOffset.z ) );
    lPos.Multiply( lOff );

    gp_Trsf lOrient;
    lOrient.SetRotation( gp_Ax1( gp_Pnt( 0.0, 0.0, 0.0 ), gp_Dir( 0.0, 0.0, 1.0 ) ),
                         -aOrientation.z );
    lPos.Multiply( lOrient );
    lOrient.SetRotation( gp_Ax1( gp_Pnt( 0.0, 0.0, 0.0 ), gp_Dir( 0.0, 1.0, 0.0 ) ),
                         -aOrientation.y );
    lPos.Multiply( lOrient );
    lOrient.SetRotation( gp_Ax1( gp_Pnt( 0.0, 0.0, 0.0 ), gp_Dir( 1.0, 0.0, 0.0 ) ),
                         -aOrientation.x );
    lPos.Multiply( lOrient );

    aLocation = TopLoc_Location( lPos );
    return true;
}


bool STEP_PCB_MODEL::readIGES( Handle( TDocStd_Document )& doc, const char* fname )
{
    IGESControl_Controller::Init();
    IGESCAFControl_Reader reader;
    IFSelect_ReturnStatus stat  = reader.ReadFile( fname );

    if( stat != IFSelect_RetDone )
        return false;

    // Enable user-defined shape precision
    if( !Interface_Static::SetIVal( "read.precision.mode", 1 ) )
        return false;

    // Set the shape conversion precision to USER_PREC (default 0.0001 has too many triangles)
    if( !Interface_Static::SetRVal( "read.precision.val", USER_PREC ) )
        return false;

    // set other translation options
    reader.SetColorMode( true );  // use model colors
    reader.SetNameMode( false );  // don't use IGES label names
    reader.SetLayerMode( false ); // ignore LAYER data

    if( !reader.Transfer( doc ) )
    {
        doc->Close();
        return false;
    }

    // are there any shapes to translate?
    if( reader.NbShapes() < 1 )
    {
        doc->Close();
        return false;
    }

    return true;
}


bool STEP_PCB_MODEL::readSTEP( Handle( TDocStd_Document )& doc, const char* fname )
{
    STEPCAFControl_Reader reader;
    IFSelect_ReturnStatus stat  = reader.ReadFile( fname );

    if( stat != IFSelect_RetDone )
        return false;

    // Enable user-defined shape precision
    if( !Interface_Static::SetIVal( "read.precision.mode", 1 ) )
        return false;

    // Set the shape conversion precision to USER_PREC (default 0.0001 has too many triangles)
    if( !Interface_Static::SetRVal( "read.precision.val", USER_PREC ) )
        return false;

    // set other translation options
    reader.SetColorMode( true );  // use model colors
    reader.SetNameMode( false );  // don't use label names
    reader.SetLayerMode( false ); // ignore LAYER data

    if( !reader.Transfer( doc ) )
    {
        doc->Close();
        return false;
    }

    // are there any shapes to translate?
    if( reader.NbRootsForTransfer() < 1 )
    {
        doc->Close();
        return false;
    }

    return true;
}


TDF_Label STEP_PCB_MODEL::transferModel( Handle( TDocStd_Document )& source,
                                   Handle( TDocStd_Document )& dest, VECTOR3D aScale )
{
    // transfer data from Source into a top level component of Dest
    gp_GTrsf scale_transform;
    scale_transform.SetVectorialPart( gp_Mat( aScale.x, 0, 0,
                                              0, aScale.y, 0,
                                              0, 0, aScale.z ) );
    BRepBuilderAPI_GTransform brep( scale_transform );

    // s_assy = shape tool for the source
    Handle(XCAFDoc_ShapeTool) s_assy = XCAFDoc_DocumentTool::ShapeTool( source->Main() );

    // retrieve all free shapes within the assembly
    TDF_LabelSequence frshapes;
    s_assy->GetFreeShapes( frshapes );

    // d_assy = shape tool for the destination
    Handle( XCAFDoc_ShapeTool ) d_assy = XCAFDoc_DocumentTool::ShapeTool ( dest->Main() );

    // create a new shape within the destination and set the assembly tool to point to it
    TDF_Label component = d_assy->NewShape();

    int nshapes = frshapes.Length();
    int id = 1;
    Handle( XCAFDoc_ColorTool ) scolor = XCAFDoc_DocumentTool::ColorTool( source->Main() );
    Handle( XCAFDoc_ColorTool ) dcolor = XCAFDoc_DocumentTool::ColorTool( dest->Main() );
    TopExp_Explorer dtop;
    TopExp_Explorer stop;

    while( id <= nshapes )
    {
        TopoDS_Shape shape = s_assy->GetShape( frshapes.Value( id ) );

        if( !shape.IsNull() )
        {
            TopoDS_Shape scaled_shape( shape );

            if( aScale.x != 1.0 || aScale.y != 1.0 || aScale.z != 1.0 )
            {
                brep.Perform( shape, Standard_False );

                if( brep.IsDone() )
                {
                    scaled_shape = brep.Shape();
                }
                else
                {
                    ReportMessage( wxT( "  * transfertModel(): failed to scale model\n" ) );

                    scaled_shape = shape;
                }
            }

            TDF_Label niulab = d_assy->AddComponent( component, scaled_shape, Standard_False );

            // check for per-surface colors
            stop.Init( shape, TopAbs_FACE );
            dtop.Init( d_assy->GetShape( niulab ), TopAbs_FACE );

            while( stop.More() && dtop.More() )
            {
                Quantity_Color face_color;

                TDF_Label tl;

                // give priority to the base shape's color
                if( s_assy->FindShape( stop.Current(), tl ) )
                {
                    if( scolor->GetColor( tl, XCAFDoc_ColorSurf, face_color )
                        || scolor->GetColor( tl, XCAFDoc_ColorGen, face_color )
                        || scolor->GetColor( tl, XCAFDoc_ColorCurv, face_color ) )
                    {
                        dcolor->SetColor( dtop.Current(), face_color, XCAFDoc_ColorSurf );
                    }
                }
                else if( scolor->GetColor( stop.Current(), XCAFDoc_ColorSurf, face_color )
                         || scolor->GetColor( stop.Current(), XCAFDoc_ColorGen, face_color )
                         || scolor->GetColor( stop.Current(), XCAFDoc_ColorCurv, face_color ) )
                {
                    dcolor->SetColor( dtop.Current(), face_color, XCAFDoc_ColorSurf );
                }

                stop.Next();
                dtop.Next();
            }

            // check for per-solid colors
            stop.Init( shape, TopAbs_SOLID );
            dtop.Init( d_assy->GetShape( niulab ), TopAbs_SOLID, TopAbs_FACE );

            while( stop.More() && dtop.More() )
            {
                Quantity_Color face_color;

                TDF_Label tl;

                // give priority to the base shape's color
                if( s_assy->FindShape( stop.Current(), tl ) )
                {
                    if( scolor->GetColor( tl, XCAFDoc_ColorSurf, face_color )
                        || scolor->GetColor( tl, XCAFDoc_ColorGen, face_color )
                        || scolor->GetColor( tl, XCAFDoc_ColorCurv, face_color ) )
                    {
                        dcolor->SetColor( dtop.Current(), face_color, XCAFDoc_ColorGen );
                    }
                }
                else if( scolor->GetColor( stop.Current(), XCAFDoc_ColorSurf, face_color )
                         || scolor->GetColor( stop.Current(), XCAFDoc_ColorGen, face_color )
                         || scolor->GetColor( stop.Current(), XCAFDoc_ColorCurv, face_color ) )
                {
                    dcolor->SetColor( dtop.Current(), face_color, XCAFDoc_ColorSurf );
                }

                stop.Next();
                dtop.Next();
            }
        }

        ++id;
    };

    return component;
}