/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2022 Mark Roszko <mark.roszko@gmail.com>
 * Copyright (C) 2016 Cirilo Bernardo <cirilo.bernardo@gmail.com>
 * Copyright (C) 2016-2024 KiCad Developers, see AUTHORS.txt for contributors.
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
#include <wx/stdstream.h>

#include <decompress.hpp>

#include <footprint.h>
#include <pad.h>
#include <pcb_track.h>
#include <kiplatform/io.h>
#include <string_utils.h>
#include <build_version.h>
#include <geometry/shape_segment.h>

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
#include <BRepTools.hxx>
#include <BRepAlgoAPI_Cut.hxx>

#include <BRepBndLib.hxx>
#include <Bnd_BoundSortBox.hxx>

#include <TopoDS.hxx>
#include <TopoDS_Wire.hxx>
#include <TopoDS_Face.hxx>
#include <TopoDS_Compound.hxx>
#include <TopoDS_Builder.hxx>
#include <Standard_Failure.hxx>

#include <Geom_Curve.hxx>
#include <Geom_BezierCurve.hxx>
#include <Geom_TrimmedCurve.hxx>

#include <gp_Ax2.hxx>
#include <gp_Circ.hxx>
#include <gp_Dir.hxx>
#include <gp_Pnt.hxx>
#include <GC_MakeArcOfCircle.hxx>
#include <GC_MakeCircle.hxx>

#include <RWGltf_CafWriter.hxx>

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
    m_maxError = pcbIUScale.mmToIU( ARC_TO_SEGMENT_MAX_ERROR_MM );
}


STEP_PCB_MODEL::~STEP_PCB_MODEL()
{
    if( m_doc->CanClose() == CDM_CCS_OK )
        m_doc->Close();
}

bool STEP_PCB_MODEL::AddPadShape( const PAD* aPad, const VECTOR2D& aOrigin )
{
    bool success = true;

    for( PCB_LAYER_ID pcb_layer = F_Cu; ; pcb_layer = B_Cu )
    {
        TopoDS_Shape curr_shape;
        double       Zpos = pcb_layer == F_Cu ? m_boardThickness : -m_copperThickness;

        if( aPad->IsOnLayer( pcb_layer ) )
        {
            // Make a shape on top/bottom copper layer
            std::shared_ptr<SHAPE> effShapePtr = aPad->GetEffectiveShape( pcb_layer );

            wxCHECK( effShapePtr->Type() == SHAPE_TYPE::SH_COMPOUND, false );
            SHAPE_COMPOUND* compoundShape = static_cast<SHAPE_COMPOUND*>( effShapePtr.get() );

            std::vector<TopoDS_Shape> topodsShapes;

            for( SHAPE* shape : compoundShape->Shapes() )
            {
                if( shape->Type() == SHAPE_TYPE::SH_SEGMENT
                    || shape->Type() == SHAPE_TYPE::SH_CIRCLE )
                {
                    VECTOR2I start, end;
                    int      width = 0;

                    if( shape->Type() == SHAPE_TYPE::SH_SEGMENT )
                    {
                        SHAPE_SEGMENT* sh_seg = static_cast<SHAPE_SEGMENT*>( shape );

                        start = sh_seg->GetSeg().A;
                        end = sh_seg->GetSeg().B;
                        width = sh_seg->GetWidth();
                    }
                    else if( shape->Type() == SHAPE_TYPE::SH_CIRCLE )
                    {
                        SHAPE_CIRCLE* sh_circ = static_cast<SHAPE_CIRCLE*>( shape );

                        start = end = sh_circ->GetCenter();
                        width = sh_circ->GetRadius() * 2;
                    }

                    TopoDS_Shape topods_shape;

                    if( MakeShapeAsThickSegment( topods_shape, start, end, width, m_copperThickness,
                                                 Zpos, aOrigin ) )
                    {
                        topodsShapes.emplace_back( topods_shape );
                    }
                    else
                    {
                        success = false;
                    }
                }
                else
                {
                    SHAPE_POLY_SET polySet;
                    shape->TransformToPolygon( polySet, ARC_HIGH_DEF, ERROR_INSIDE );

                    success &= MakeShapes( topodsShapes, polySet, m_copperThickness, Zpos, aOrigin );
                }
            }

            // Fuse shapes
            if( topodsShapes.size() == 1 )
            {
                m_board_copper_pads.emplace_back( topodsShapes.front() );
            }
            else
            {
                BRepAlgoAPI_Fuse     mkFuse;
                TopTools_ListOfShape shapeArguments, shapeTools;

                for( TopoDS_Shape& sh : topodsShapes )
                {
                    if( sh.IsNull() )
                        continue;

                    if( shapeArguments.IsEmpty() )
                        shapeArguments.Append( sh );
                    else
                        shapeTools.Append( sh );
                }

                mkFuse.SetRunParallel( true );
                mkFuse.SetToFillHistory( false );
                mkFuse.SetArguments( shapeArguments );
                mkFuse.SetTools( shapeTools );
                mkFuse.Build();

                if( mkFuse.IsDone() )
                {
                    TopoDS_Shape fusedShape = mkFuse.Shape();

                    ShapeUpgrade_UnifySameDomain unify( fusedShape, false, true, false );
                    unify.History() = nullptr;
                    unify.Build();

                    TopoDS_Shape unifiedShapes = unify.Shape();

                    if( !unifiedShapes.IsNull() )
                    {
                        m_board_copper_pads.emplace_back( unifiedShapes );
                    }
                    else
                    {
                        ReportMessage(
                                _( "** ShapeUpgrade_UnifySameDomain produced a null shape **\n" ) );
                        m_board_copper_pads.emplace_back( fusedShape );
                    }
            }
            else
            {
                    for( TopoDS_Shape& sh : topodsShapes )
                        m_board_copper_pads.emplace_back( sh );
                }
            }
        }

        if( pcb_layer == B_Cu )
            break;
    }

    if( aPad->GetAttribute() == PAD_ATTRIB::PTH && aPad->IsOnLayer( F_Cu )
        && aPad->IsOnLayer( B_Cu ) )
    {
        TopoDS_Shape plating;

        std::shared_ptr<SHAPE_SEGMENT> seg_hole = aPad->GetEffectiveHoleShape();
        double width = std::min( aPad->GetDrillSize().x, aPad->GetDrillSize().y );

        if( MakeShapeAsThickSegment( plating, seg_hole->GetSeg().A, seg_hole->GetSeg().B, width,
                                     m_boardThickness + m_copperThickness * 2, -m_copperThickness,
                                     aOrigin ) )
        {
            m_board_copper_pads.push_back( plating );
        }
        else
        {
            success = false;
        }
    }

    if( !success )  // Error
        ReportMessage( wxT( "OCC error adding pad/via polygon.\n" ) );

    return success;
}


bool STEP_PCB_MODEL::AddViaShape( const PCB_VIA* aVia, const VECTOR2D& aOrigin )
{
    // A via is very similar to a round pad. So, for now, used AddPadHole() to
    // create a via+hole shape
    PAD dummy( nullptr );
    int hole = aVia->GetDrillValue();
    dummy.SetDrillSize( VECTOR2I( hole, hole ) );
    dummy.SetPosition( aVia->GetStart() );
    dummy.SetSize( VECTOR2I( aVia->GetWidth(), aVia->GetWidth() ) );

    if( AddPadHole( &dummy, aOrigin ) )
    {
        if( !AddPadShape( &dummy, aOrigin ) )
            return false;
    }

    return true;
}


bool STEP_PCB_MODEL::AddTrackSegment( const PCB_TRACK* aTrack, const VECTOR2D& aOrigin )
{
    PCB_LAYER_ID pcblayer = aTrack->GetLayer();

    if( pcblayer != F_Cu && pcblayer != B_Cu )
        return false;

    TopoDS_Shape shape;
    double zposition = pcblayer == F_Cu ? m_boardThickness : -m_copperThickness;

    bool success = MakeShapeAsThickSegment( shape, aTrack->GetStart(), aTrack->GetEnd(),
                                            aTrack->GetWidth(), m_copperThickness,
                                            zposition, aOrigin );

    if( success )
        m_board_copper_tracks.push_back( shape );

    return success;
}


bool STEP_PCB_MODEL::AddCopperPolygonShapes( const SHAPE_POLY_SET* aPolyShapes, bool aOnTop,
                                             const VECTOR2D& aOrigin, bool aTrack )
{
    bool success = true;

    if( aPolyShapes->IsEmpty() )
        return true;

    double z_pos = aOnTop ? m_boardThickness : -m_copperThickness;

    if( !MakeShapes( aTrack ? m_board_copper_tracks : m_board_copper_zones, *aPolyShapes,
                     m_copperThickness, z_pos, aOrigin ) )
    {
        ReportMessage( wxString::Format(
                wxT( "Could not add shape (%d points) to copper layer on %s.\n" ),
                aPolyShapes->FullPointCount(), aOnTop ? wxT( "top" ) : wxT( "bottom" ) ) );

        success = false;
    }

    return success;
}


bool STEP_PCB_MODEL::AddPadHole( const PAD* aPad, const VECTOR2D& aOrigin )
{
    if( aPad == nullptr || !aPad->GetDrillSize().x )
        return false;

    // TODO: make configurable
    int platingThickness = aPad->GetAttribute() == PAD_ATTRIB::PTH ? pcbIUScale.mmToIU( 0.025 ) : 0;

    const double margin = 0.01; // a small margin on the Z axix to be sure the hole
                                // is bigger than the board with copper
                                // must be > OCC_MAX_DISTANCE_TO_MERGE_POINTS
    double holeZsize = m_boardThickness + ( m_copperThickness * 2 ) + ( margin * 2 );

    std::shared_ptr<SHAPE_SEGMENT> seg_hole = aPad->GetEffectiveHoleShape();

    double boardDrill = std::min( aPad->GetDrillSize().x, aPad->GetDrillSize().y );
    double copperDrill = boardDrill - platingThickness * 2;

    TopoDS_Shape copperHole, boardHole;

    if( MakeShapeAsThickSegment( copperHole, seg_hole->GetSeg().A, seg_hole->GetSeg().B,
                                 copperDrill, holeZsize, -m_copperThickness - margin, aOrigin ) )
    {
        m_copperCutouts.push_back( copperHole );
    }
    else
    {
        return false;
    }

    if( MakeShapeAsThickSegment( boardHole, seg_hole->GetSeg().A, seg_hole->GetSeg().B, boardDrill,
                                 holeZsize, -m_copperThickness - margin, aOrigin ) )
    {
        m_boardCutouts.push_back( boardHole );
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
}


bool STEP_PCB_MODEL::isBoardOutlineValid()
{
    return m_pcb_labels.size() > 0;
}


// A helper function to know if a SHAPE_LINE_CHAIN is encoding a circle (now unused)
#if 0
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
#endif


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


bool STEP_PCB_MODEL::MakeShapeAsThickSegment( TopoDS_Shape& aShape,
                                              VECTOR2D aStartPoint, VECTOR2D aEndPoint,
                                              double aWidth, double aThickness,
                                              double aZposition, const VECTOR2D& aOrigin )
{
    // make a wide segment from 2 lines and 2 180 deg arcs
    // We need 6 points (3 per arcs)
    VECTOR2D coords[6];

    // We build a horizontal segment, and after rotate it
    double len = ( aEndPoint - aStartPoint ).EuclideanNorm();
    double h_width = aWidth/2.0;
    // First is end point of first arc, and also start point of first line
    coords[0] = VECTOR2D{ 0.0, h_width };

    // end  point of first line and start point of second arc
    coords[1] = VECTOR2D{ len, h_width };

    // middle point of second arc
    coords[2] = VECTOR2D{ len + h_width, 0.0 };

    // start point of second line and end point of second arc
    coords[3] = VECTOR2D{ len, -h_width };

    // end point of second line and start point of first arc
    coords[4] = VECTOR2D{ 0, -h_width };

    // middle point of first arc
    coords[5] = VECTOR2D{ -h_width, 0.0 };

    // Rotate and move to segment position
    EDA_ANGLE seg_angle( aEndPoint - aStartPoint );

    for( int ii = 0; ii < 6; ii++ )
    {
        RotatePoint( coords[ii], VECTOR2D{ 0, 0 }, -seg_angle ),
        coords[ii] += aStartPoint;
    }


    // Convert to 3D points
    gp_Pnt coords3D[ 6 ];

    for( int ii = 0; ii < 6; ii++ )
    {
        coords3D[ii] = gp_Pnt( pcbIUScale.IUTomm( coords[ii].x - aOrigin.x ),
                               -pcbIUScale.IUTomm( coords[ii].y - aOrigin.y ), aZposition );
    }

    // Build OpenCascade shape outlines
    BRepBuilderAPI_MakeWire wire;
    bool success = true;

    // Short segments (distance between end points < m_mergeOCCMaxDist(in mm)) must be
    // skipped because OCC merge end points, and a null shape is created
    bool short_seg = pcbIUScale.IUTomm( len ) <= m_mergeOCCMaxDist;

    try
    {
        TopoDS_Edge edge;

        if( short_seg )
        {
            Handle( Geom_Circle ) circle = GC_MakeCircle( coords3D[1], // arc1 start point
                                                          coords3D[2], // arc1 mid point
                                                          coords3D[5]  // arc2 mid point
            );

            edge = BRepBuilderAPI_MakeEdge( circle );
            wire.Add( edge );
        }
        else
        {
            edge = BRepBuilderAPI_MakeEdge( coords3D[0], coords3D[1] );
            wire.Add( edge );

            Handle( Geom_TrimmedCurve ) arcOfCircle =
                    GC_MakeArcOfCircle( coords3D[1], // start point
                                        coords3D[2], // mid point
                                        coords3D[3]  // end point
                    );
            edge = BRepBuilderAPI_MakeEdge( arcOfCircle );
            wire.Add( edge );

            edge = BRepBuilderAPI_MakeEdge( coords3D[3], coords3D[4] );
            wire.Add( edge );

            Handle( Geom_TrimmedCurve ) arcOfCircle2 =
                    GC_MakeArcOfCircle( coords3D[4], // start point
                                        coords3D[5], // mid point
                                        coords3D[0]  // end point
                    );
            edge = BRepBuilderAPI_MakeEdge( arcOfCircle2 );
            wire.Add( edge );
        }
    }
    catch( const Standard_Failure& e )
    {
        ReportMessage( wxString::Format( wxT( "build shape segment: OCC exception: %s\n" ),
                                         e.GetMessageString() ) );
        return false;
    }


    BRepBuilderAPI_MakeFace face;

    try
    {
        gp_Pln plane( coords3D[0], gp::DZ() );
        face = BRepBuilderAPI_MakeFace( plane, wire );
    }
    catch(  const Standard_Failure& e )
    {
        ReportMessage(
                wxString::Format( wxT( "MakeShapeThickSegment: OCC exception: %s\n" ),
                                  e.GetMessageString() ) );
        return false;
    }

    aShape = BRepPrimAPI_MakePrism( face, gp_Vec( 0, 0, aThickness ) );

    if( aShape.IsNull() )
    {
        ReportMessage( wxT( "failed to create a prismatic shape\n" ) );
        return false;
    }

    return success;
}


bool STEP_PCB_MODEL::MakeShapes( std::vector<TopoDS_Shape>& aShapes, const SHAPE_POLY_SET& aPolySet,
                                 double aThickness, double aZposition, const VECTOR2D& aOrigin )
{
    SHAPE_POLY_SET simplified = aPolySet;
    simplified.Simplify( SHAPE_POLY_SET::PM_STRICTLY_SIMPLE );

    auto toPoint = [&]( const VECTOR2D& aKiCoords ) -> gp_Pnt
    {
        return gp_Pnt( pcbIUScale.IUTomm( aKiCoords.x - aOrigin.x ),
                       -pcbIUScale.IUTomm( aKiCoords.y - aOrigin.y ), aZposition );
    };

    for( const SHAPE_POLY_SET::POLYGON& polygon : simplified.CPolygons() )
    {
        auto makeWireFromChain = [&]( BRepLib_MakeWire&       aMkWire,
                                      const SHAPE_LINE_CHAIN& aChain ) -> bool
        {
            try
            {
                auto addSegment = [&]( const VECTOR2I& aPt0, const VECTOR2I& aPt1 ) -> bool
                {
                    if( aPt0 == aPt1 )
                        return false;

                    gp_Pnt start = toPoint( aPt0 );
                    gp_Pnt end = toPoint( aPt1 );

                    // Do not export too short segments: they create broken shape because OCC thinks
                    // start point and end point are at the same place
                    double seg_len = std::hypot( end.X() - start.X(), end.Y() - start.Y() );

                    if( seg_len <= m_mergeOCCMaxDist )
                        return false;

                    BRepBuilderAPI_MakeEdge mkEdge( start, end );

                    if( !mkEdge.IsDone() || mkEdge.Edge().IsNull() )
                    {
                        ReportMessage( wxString::Format( wxT( "failed to make segment edge at (%d "
                                                              "%d) -> (%d %d), skipping\n" ),
                                                         aPt0.x, aPt0.y, aPt1.x, aPt1.y ) );
                    }
                    else
                    {
                        aMkWire.Add( mkEdge.Edge() );

                        if( aMkWire.Error() != BRepLib_WireDone )
                        {
                            ReportMessage( wxString::Format( wxT( "failed to add segment edge "
                                                                  "at (%d %d) -> (%d %d)\n" ),
                                                             aPt0.x, aPt0.y, aPt1.x, aPt1.y ) );
                            return false;
                        }
                    }

                    return true;
                };

                auto addArc = [&]( const VECTOR2I& aPt0, const SHAPE_ARC& aArc ) -> bool
                {
                    // Do not export too short segments: they create broken shape because OCC thinks
                    Handle( Geom_Curve ) curve;

                    if( aArc.GetCentralAngle() == ANGLE_360 )
                    {
                        gp_Ax2 axis = gp::XOY();
                        axis.SetLocation( toPoint( aArc.GetCenter() ) );

                        curve = GC_MakeCircle( axis, pcbIUScale.IUTomm( aArc.GetRadius() ) )
                                        .Value();
                    }
                    else
                    {
                        curve = GC_MakeArcOfCircle( toPoint( aPt0 ),
                                                    toPoint( aArc.GetArcMid() ),
                                                    toPoint( aArc.GetP1() ) )
                                        .Value();
                    }

                    if( curve.IsNull() )
                        return false;

                    aMkWire.Add( BRepBuilderAPI_MakeEdge( curve ) );

                    if( !aMkWire.IsDone() )
                    {
                        ReportMessage( wxString::Format(
                                wxT( "failed to add arc curve from (%d %d), arc p0 "
                                     "(%d %d), mid (%d %d), p1 (%d %d)\n" ),
                                aPt0.x, aPt0.y, aArc.GetP0().x, aArc.GetP0().y, aArc.GetArcMid().x,
                                aArc.GetArcMid().y, aArc.GetP1().x, aArc.GetP1().y ) );
                        return false;
                    }

                    return true;
                };

                VECTOR2I firstPt;
                VECTOR2I lastPt;
                bool     isFirstShape = true;

                for( int i = 0; i <= aChain.PointCount() && i != -1; i = aChain.NextShape( i ) )
                {
                    if( i == 0 )
                    {
                        if( aChain.IsArcSegment( 0 )
                            && aChain.IsArcSegment( aChain.PointCount() - 1 )
                            && aChain.ArcIndex( 0 ) == aChain.ArcIndex( aChain.PointCount() - 1 ) )
                        {
                            // Skip first arc (we should encounter it later)
                            int nextShape = aChain.NextShape( i );

                            // If nextShape points to the end, then we have a circle.
                            if( nextShape != -1 )
                                i = nextShape;
                        }
                    }

                    if( isFirstShape )
                        lastPt = aChain.CPoint( i );

                    bool isArc = aChain.IsArcSegment( i );

                    if( aChain.IsArcStart( i ) )
                    {
                        const SHAPE_ARC& currentArc = aChain.Arc( aChain.ArcIndex( i ) );

                        if( isFirstShape )
                        {
                            firstPt = currentArc.GetP0();
                            lastPt = firstPt;
                        }

                        if( addSegment( lastPt, currentArc.GetP0() ) )
                            lastPt = currentArc.GetP0();

                        if( addArc( lastPt, currentArc ) )
                            lastPt = currentArc.GetP1();
                    }
                    else if( !isArc )
                    {
                        const SEG& seg = aChain.CSegment( i );

                        if( isFirstShape )
                        {
                            firstPt = seg.A;
                            lastPt = firstPt;
                        }

                        if( addSegment( lastPt, seg.A ) )
                            lastPt = seg.A;

                        if( addSegment( lastPt, seg.B ) )
                            lastPt = seg.B;
                    }

                    isFirstShape = false;
                }

                if( lastPt != firstPt )
                    addSegment( lastPt, firstPt );
            }
            catch( const Standard_Failure& e )
            {
                ReportMessage( wxString::Format( wxT( "makeWireFromChain: OCC exception: %s\n" ),
                                                 e.GetMessageString() ) );
                return false;
            }

            return true;
        };

        BRepBuilderAPI_MakeFace mkFace;

        for( size_t contId = 0; contId < polygon.size(); contId++ )
        {
            const SHAPE_LINE_CHAIN& contour = polygon[contId];
            BRepLib_MakeWire        mkWire;

            try
            {
                if( !makeWireFromChain( mkWire, contour ) )
                    continue;

                if( !mkWire.IsDone() )
                {
                    ReportMessage( wxString::Format(
                            _( "Wire not done (contour %d, points %d): OCC error %d\n" ),
                            static_cast<int>( contId ), static_cast<int>( contour.PointCount() ),
                            static_cast<int>( mkWire.Error() ) ) );
                }

                if( contId == 0 ) // Outline
                {
                    if( mkWire.IsDone() )
                        mkFace = BRepBuilderAPI_MakeFace( mkWire.Wire() );
                    else
                        continue;
                }
                else // Hole
                {
                    if( mkWire.IsDone() )
                        mkFace.Add( mkWire );
                }
            }
            catch( const Standard_Failure& e )
            {
                ReportMessage(
                        wxString::Format( wxT( "MakeShapes (contour %d): OCC exception: %s\n" ),
                                          static_cast<int>( contId ), e.GetMessageString() ) );
                return false;
            }
        }

        if( mkFace.IsDone() )
        {
            TopoDS_Shape prism = BRepPrimAPI_MakePrism( mkFace, gp_Vec( 0, 0, aThickness ) );
            aShapes.push_back( prism );

            if( prism.IsNull() )
            {
                ReportMessage( wxT( "Failed to create a prismatic shape\n" ) );
                return false;
            }
        }
        else
        {
            wxASSERT( false );
        }
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

    Handle( XCAFDoc_ColorTool ) colorTool = XCAFDoc_DocumentTool::ColorTool( m_doc->Main() );
    m_hasPCB = true; // whether or not operations fail we note that CreatePCB has been invoked


    // Support for more than one main outline (more than one board)
    ReportMessage( wxString::Format( wxT( "Build board outlines (%d outlines) with %d points.\n" ),
                                     aOutline.OutlineCount(), aOutline.FullPointCount() ) );
#if 0
    // This code should work, and it is working most of time
    // However there are issues if the main outline is a circle with holes:
    // holes from vias and pads are not working
    // see bug https://gitlab.com/kicad/code/kicad/-/issues/17446
    // (Holes are missing from STEP export with circular PCB outline)
    // Hard to say if the bug is in our code or in OCC 7.7
    if( !MakeShapes( m_board_outlines, aOutline, m_boardThickness, 0.0, aOrigin ) )
    {
        // Error
        ReportMessage( wxString::Format(
                wxT( "OCC error creating main outline.\n" ) ) );
    }
#else
    // Workaround for bug #17446 Holes are missing from STEP export with circular PCB outline
    for( const SHAPE_POLY_SET::POLYGON& polygon : aOutline.CPolygons() )
    {
        for( size_t contId = 0; contId < polygon.size(); contId++ )
        {
            const SHAPE_LINE_CHAIN& contour = polygon[contId];
            SHAPE_POLY_SET polyset;
            polyset.Append( contour );

            if( contId == 0 ) // main Outline
            {
                if( !MakeShapes( m_board_outlines, polyset, m_boardThickness, 0.0, aOrigin ) )
                    ReportMessage( wxT( "OCC error creating main outline.\n" ) );
            }
            else // Hole inside the main outline
            {
                if( !MakeShapes( m_boardCutouts, polyset, m_boardThickness, 0.0, aOrigin ) )
                    ReportMessage( wxT( "OCC error creating hole in main outline.\n" ) );
            }
        }
    }
#endif

    Bnd_Box brdBndBox;

    for( const TopoDS_Shape& brdShape : m_board_outlines )
        BRepBndLib::Add( brdShape, brdBndBox );

    // subtract cutouts (if any)
    ReportMessage( wxString::Format( wxT( "Build board cutouts and holes (%d hole(s)).\n" ),
                                     (int) ( m_boardCutouts.size() + m_copperCutouts.size() ) ) );

    auto buildBSB = [&brdBndBox]( std::vector<TopoDS_Shape>& input, Bnd_BoundSortBox& bsbHoles )
    {
        // We need to encompass every location we'll need to test in the global bbox,
        // otherwise Bnd_BoundSortBox doesn't work near the boundaries.
        Bnd_Box brdWithHolesBndBox = brdBndBox;

        Handle( Bnd_HArray1OfBox ) holeBoxSet = new Bnd_HArray1OfBox( 0, input.size() - 1 );

        for( size_t i = 0; i < input.size(); i++ )
        {
            Bnd_Box bbox;
            BRepBndLib::Add( input[i], bbox );
            brdWithHolesBndBox.Add( bbox );
            ( *holeBoxSet )[i] = bbox;
        }

        bsbHoles.Initialize( brdWithHolesBndBox, holeBoxSet );
    };

    auto subtractShapes = []( const wxString& aWhat, std::vector<TopoDS_Shape>& aShapesList,
                              std::vector<TopoDS_Shape>& aHolesList, Bnd_BoundSortBox& aBSBHoles )
    {
        // Remove holes for each item (board body or bodies, one can have more than one board)
        int cnt = 0;
        for( TopoDS_Shape& shape : aShapesList )
        {
            Bnd_Box shapeBbox;
            BRepBndLib::Add( shape, shapeBbox );

            const TColStd_ListOfInteger& indices = aBSBHoles.Compare( shapeBbox );

            TopTools_ListOfShape holelist;

            for( const Standard_Integer& index : indices )
                holelist.Append( aHolesList[index] );

            if( cnt == 0 )
                ReportMessage( wxString::Format( _( "Build holes for %s\n" ), aWhat ) );

            cnt++;

            if( cnt % 10 == 0 )
                ReportMessage( wxString::Format( _( "Cutting %d/%d %s\n" ), cnt,
                                                 (int) aShapesList.size(), aWhat ) );

            if( holelist.IsEmpty() )
                continue;

            TopTools_ListOfShape cutArgs;
            cutArgs.Append( shape );

            BRepAlgoAPI_Cut cut;

            // This helps cutting circular holes in zones where a hole is already cut in Clipper
            cut.SetFuzzyValue( 0.0005 );
            cut.SetRunParallel( true );
            cut.SetToFillHistory( false );

            cut.SetArguments( cutArgs );
            cut.SetTools( holelist );
            cut.Build();

            if( cut.HasErrors() || cut.HasWarnings() )
            {
                ReportMessage( wxString::Format(
                        _( "\n** Got problems while cutting %s number %d **\n" ), aWhat, cnt ) );
                shapeBbox.Dump();

                if( cut.HasErrors() )
                {
                    ReportMessage( _( "Errors:\n" ) );
                    cut.DumpErrors( std::cout );
                }

                if( cut.HasWarnings() )
                {
                    ReportMessage( _( "Warnings:\n" ) );
                    cut.DumpWarnings( std::cout );
                }

                std::cout << "\n";
            }

            shape = cut.Shape();
        }
    };

    if( m_boardCutouts.size() )
    {
        Bnd_BoundSortBox bsbHoles;
        buildBSB( m_boardCutouts, bsbHoles );

        subtractShapes( _( "shapes" ), m_board_outlines, m_boardCutouts, bsbHoles );
    }

    if( m_copperCutouts.size() )
    {
        Bnd_BoundSortBox bsbHoles;
        buildBSB( m_copperCutouts, bsbHoles );

        subtractShapes( _( "pads" ), m_board_copper_pads, m_copperCutouts, bsbHoles );
        subtractShapes( _( "tracks" ), m_board_copper_tracks, m_copperCutouts, bsbHoles );
        subtractShapes( _( "zones" ), m_board_copper_zones, m_copperCutouts, bsbHoles );
    }

    // push the board to the data structure
    ReportMessage( wxT( "\nGenerate board full shape.\n" ) );

    auto pushToAssembly = [&]( std::vector<TopoDS_Shape>& aShapesList, Quantity_Color aColor,
                               const wxString& aShapeName )
    {
        int i = 1;
        for( TopoDS_Shape& shape : aShapesList )
        {
            Handle( TDataStd_TreeNode ) node;

            // Dont expand the component or else coloring it gets hard
            TDF_Label lbl = m_assy->AddComponent( m_assy_label, shape, false );
            m_pcb_labels.push_back( lbl );

            if( m_pcb_labels.back().IsNull() )
                break;

            lbl.FindAttribute( XCAFDoc::ShapeRefGUID(), node );
            TDF_Label shpLbl = node->Father()->Label();
            if( !shpLbl.IsNull() )
            {
                colorTool->SetColor( shpLbl, aColor, XCAFDoc_ColorSurf );
                wxString shapeName;

                if( aShapesList.size() > 1 )
                {
                    shapeName = wxString::Format( wxT( "%s_%s_%d" ), m_pcbName, aShapeName, i );
                }
                else
                {
                    shapeName = wxString::Format( wxT( "%s_%s" ), m_pcbName, aShapeName );
                }


                TCollection_ExtendedString partname( shapeName.ToUTF8().data() );
                TDataStd_Name::Set( shpLbl, partname );
            }

            i++;
        }
    };

    // AddComponent adds a label that has a reference (not a parent/child relation) to the real
    // label.  We need to extract that real label to name it for the STEP output cleanly
    // Why are we trying to name the bare board? Because CAD tools like SolidWorks do fun things
    // like "deduplicate" imported STEPs by swapping STEP assembly components with already
    // identically named assemblies.  So we want to avoid having the PCB be generally defaulted
    // to "Component" or "Assembly".

    // Init colors  for the board body and the copper items (if any)
    Quantity_Color board_color( m_boardColor[0], m_boardColor[1], m_boardColor[2],
                                Quantity_TOC_RGB );
    Quantity_Color copper_color( m_copperColor[0], m_copperColor[1], m_copperColor[2],
                                 Quantity_TOC_RGB );

    pushToAssembly( m_board_copper_tracks, copper_color, "track" );
    pushToAssembly( m_board_copper_zones, copper_color, "zone" );
    pushToAssembly( m_board_copper_pads, copper_color, "pad" );
    pushToAssembly( m_board_outlines, board_color, "PCB" );

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


bool STEP_PCB_MODEL::WriteSTEP( const wxString& aFileName, bool aOptimize )
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

    // Setting write.surfacecurve.mode to 0 reduces file size and write/read times.
    // But there are reports that this mode might be less compatible in some cases.
    if( !Interface_Static::SetIVal( "write.surfacecurve.mode", aOptimize ? 0 : 1 ) )
        ReportMessage( wxT( "Failed to set surface curve mode, but will attempt to continue." ) );

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

        // Preserve the permissions of the current file
        KIPLATFORM::IO::DuplicatePermissions( fn.GetFullPath(), tmpfname );

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


bool STEP_PCB_MODEL::WriteBREP( const wxString& aFileName )
{
    if( !isBoardOutlineValid() )
    {
        ReportMessage( wxString::Format( wxT( "No valid PCB assembly; cannot create output file "
                                              "'%s'.\n" ),
                                         aFileName ) );
        return false;
    }

    // s_assy = shape tool for the source
    Handle( XCAFDoc_ShapeTool ) s_assy = XCAFDoc_DocumentTool::ShapeTool( m_doc->Main() );

    // retrieve all free shapes within the assembly
    TDF_LabelSequence freeShapes;
    s_assy->GetFreeShapes( freeShapes );

    for( Standard_Integer i = 1; i <= freeShapes.Length(); ++i )
    {
        TDF_Label    label = freeShapes.Value( i );
        TopoDS_Shape shape;
        m_assy->GetShape( label, shape );

        wxFileName fn( aFileName );

        if( freeShapes.Length() > 1 )
            fn.SetName( wxString::Format( "%s_%s", fn.GetName(), i ) );

        wxFFileOutputStream ffStream( fn.GetFullPath() );
        wxStdOutputStream   stdStream( ffStream );
        BRepTools::Write( shape, stdStream, false, false, TopTools_FormatVersion_VERSION_1 );
    }

    return true;
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
                aErrorMessage->Printf( wxT( "Cannot load any VRML model for this export.\n" ) );

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
        if( doc->CanClose() == CDM_CCS_OK )
            doc->Close();

        return false;
    }

    // are there any shapes to translate?
    if( reader.NbShapes() < 1 )
    {
        if( doc->CanClose() == CDM_CCS_OK )
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
    reader.SetNameMode( true );  // use label names
    reader.SetLayerMode( false ); // ignore LAYER data

    if( !reader.Transfer( doc ) )
    {
        if( doc->CanClose() == CDM_CCS_OK )
            doc->Close();

        return false;
    }

    // are there any shapes to translate?
    if( reader.NbRootsForTransfer() < 1 )
    {
        if( doc->CanClose() == CDM_CCS_OK )
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
        const TDF_Label& s_shapeLabel = frshapes.Value( id );
        TopoDS_Shape     shape = s_assy->GetShape( s_shapeLabel );

        if( !shape.IsNull() )
        {
            Handle( TDataStd_Name ) s_nameAttr;
            s_shapeLabel.FindAttribute( TDataStd_Name::GetID(), s_nameAttr );

            TCollection_ExtendedString s_labelName =
                    s_nameAttr ? s_nameAttr->Get() : TCollection_ExtendedString();

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

            TDF_Label d_shapeLabel = d_assy->AddShape( scaled_shape, Standard_False );

            if( s_labelName.Length() > 0 )
                TDataStd_Name::Set( d_shapeLabel, s_labelName );

            TDF_Label niulab =
                    d_assy->AddComponent( component, d_shapeLabel, scaled_shape.Location() );

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


bool STEP_PCB_MODEL::WriteGLTF( const wxString& aFileName )
{
    if( !isBoardOutlineValid() )
    {
        ReportMessage( wxString::Format( wxT( "No valid PCB assembly; cannot create output file "
                                              "'%s'.\n" ),
                                         aFileName ) );
        return false;
    }

    TDF_LabelSequence freeShapes;
    m_assy->GetFreeShapes( freeShapes );

    ReportMessage( wxT( "Meshing model\n" ) );

    // GLTF is a mesh format, we have to trigger opencascade to mesh the shapes we composited into the asesmbly
    // To mesh models, lets just grab the free shape root and execute on them
    for( Standard_Integer i = 1; i <= freeShapes.Length(); ++i )
    {
        TDF_Label    label = freeShapes.Value( i );
        TopoDS_Shape shape;
        m_assy->GetShape( label, shape );

        // These deflection values basically affect the accuracy of the mesh generated, a tighter
        // deflection will result in larger meshes
        // We could make this a tunable parameter, but for now fix it
        const Standard_Real      linearDeflection = 0.01;
        const Standard_Real      angularDeflection = 0.5;
        BRepMesh_IncrementalMesh mesh( shape, linearDeflection, Standard_False, angularDeflection,
                                       Standard_True );
    }

    wxFileName fn( aFileName );

    const char* tmpGltfname = "$tempfile$.glb";
    RWGltf_CafWriter cafWriter( tmpGltfname, true );

    cafWriter.SetTransformationFormat( RWGltf_WriterTrsfFormat_Compact );
    cafWriter.ChangeCoordinateSystemConverter().SetInputLengthUnit( 0.001 );
    cafWriter.ChangeCoordinateSystemConverter().SetInputCoordinateSystem(
            RWMesh_CoordinateSystem_Zup );
#if OCC_VERSION_HEX >= 0x070700
    cafWriter.SetParallel( true );
#endif
    TColStd_IndexedDataMapOfStringString metadata;

    metadata.Add( TCollection_AsciiString( "pcb_name" ),
                  TCollection_ExtendedString( fn.GetName().wc_str() ) );
    metadata.Add( TCollection_AsciiString( "source_pcb_file" ),
                  TCollection_ExtendedString( fn.GetFullName().wc_str() ) );
    metadata.Add( TCollection_AsciiString( "generator" ),
                  TCollection_AsciiString( wxString::Format( wxS( "KiCad %s" ), GetSemanticVersion() ).ToAscii() ) );
    metadata.Add( TCollection_AsciiString( "generated_at" ),
                  TCollection_AsciiString( GetISO8601CurrentDateTime().ToAscii() ) );

    bool success = true;

    // Creates a temporary file with a ascii7 name, because writer does not know unicode filenames.
    wxString currCWD = wxGetCwd();
    wxString workCWD = fn.GetPath();

    if( !workCWD.IsEmpty() )
        wxSetWorkingDirectory( workCWD );

    success = cafWriter.Perform( m_doc, metadata, Message_ProgressRange() );

    if( success )
    {
        // Preserve the permissions of the current file
        KIPLATFORM::IO::DuplicatePermissions( fn.GetFullPath(), tmpGltfname );

        if( !wxRenameFile( tmpGltfname, fn.GetFullName(), true ) )
        {
            ReportMessage( wxString::Format( wxT( "Cannot rename temporary file '%s' to '%s'.\n" ),
                                             tmpGltfname, fn.GetFullName() ) );
            success = false;
        }
    }

    wxSetWorkingDirectory( currCWD );

    return success;
}