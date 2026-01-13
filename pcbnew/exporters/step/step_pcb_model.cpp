/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2022 Mark Roszko <mark.roszko@gmail.com>
 * Copyright (C) 2016 Cirilo Bernardo <cirilo.bernardo@gmail.com>
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
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
#include <functional>
#include <sstream>
#include <string>
#include <utility>
#include <wx/file.h>
#include <wx/filename.h>
#include <wx/filefn.h>
#include <wx/sstream.h>
#include <wx/stdpaths.h>
#include <wx/stream.h>
#include <wx/string.h>
#include <wx/zstream.h>
#include <wx/wfstream.h>
#include <wx/zipstrm.h>
#include <wx/stdstream.h>
#include <wx/crt.h>

#include <decompress.hpp>

#include <thread_pool.h>
#include <trace_helpers.h>
#include <board.h>
#include <board_design_settings.h>
#include <footprint.h>
#include <pad.h>
#include <pcb_track.h>
#include <kiplatform/io.h>
#include <string_utils.h>
#include <build_version.h>
#include <geometry/shape_segment.h>
#include <geometry/shape_circle.h>
#include <board_stackup_manager/board_stackup.h>
#include <board_stackup_manager/stackup_predefined_prms.h>
#include <reporter.h>

#include <exporters/u3d/writer.h>

#include <plotters/plotters_pslike.h>
#include <pcb_painter.h>

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
#include <Standard_Failure.hxx>
#include <Standard_Handle.hxx>
#include <Standard_Version.hxx>
#include <TCollection_ExtendedString.hxx>
#include <TDocStd_Document.hxx>
#include <TDocStd_XLinkTool.hxx>
#include <TDataStd_Name.hxx>
#include <TDataStd_TreeNode.hxx>
#include <TDF_ChildIterator.hxx>
#include <TDF_LabelSequence.hxx>
#include <TDF_Tool.hxx>
#include <TopExp_Explorer.hxx>
#include <TopoDS.hxx>
#include <XCAFApp_Application.hxx>
#include <XCAFDoc.hxx>
#include <XCAFDoc_DocumentTool.hxx>
#include <XCAFDoc_ColorTool.hxx>
#include <XCAFDoc_ShapeTool.hxx>
#include <XCAFDoc_VisMaterialTool.hxx>
#include <XCAFDoc_Area.hxx>
#include <XCAFDoc_Centroid.hxx>
#include <XCAFDoc_Location.hxx>
#include <XCAFDoc_Volume.hxx>
#include "kicad3d_info.h"

#include "KI_XCAFDoc_AssemblyGraph.hxx"

#include <BRep_Tool.hxx>
#include <BRepMesh_IncrementalMesh.hxx>
#include <BRepBuilderAPI_GTransform.hxx>
#include <BRepBuilderAPI_MakeEdge.hxx>
#include <BRepBuilderAPI_MakeWire.hxx>
#include <BRepBuilderAPI_MakeFace.hxx>
#include <BRepExtrema_DistShapeShape.hxx>
#include <BRepPrimAPI_MakeCone.hxx>
#include <BRepPrimAPI_MakeCylinder.hxx>
#include <BRepPrimAPI_MakePrism.hxx>
#include <BRepTools.hxx>
#include <BRepLib_MakeWire.hxx>
#include <BRepAdaptor_Surface.hxx>
#include <BRepAlgoAPI_Check.hxx>
#include <BRepAlgoAPI_Cut.hxx>
#include <BRepAlgoAPI_Fuse.hxx>
#include <ShapeUpgrade_UnifySameDomain.hxx>

#include <BRepBndLib.hxx>
#include <Bnd_BoundSortBox.hxx>
#include <GProp_GProps.hxx>
#include <BRepGProp.hxx>

#include <Geom_Curve.hxx>
#include <Geom_TrimmedCurve.hxx>

#include <gp_Ax2.hxx>
#include <gp_Dir.hxx>
#include <gp_Pnt.hxx>
#include <GC_MakeArcOfCircle.hxx>
#include <GC_MakeCircle.hxx>

#include <RWGltf_CafWriter.hxx>
#include <StlAPI_Writer.hxx>

#if OCC_VERSION_HEX >= 0x070700
#include <VrmlAPI_CafReader.hxx>
#include <RWPly_CafWriter.hxx>
#endif

#include <macros.h>
#include <convert_basic_shapes_to_polygon.h>

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
        return FMT_NONE;

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
    MODEL3D_FORMAT_TYPE format_type = FMT_NONE;

    // The expected header should be the first line.
    // However some files can have a comment at the beginning of the file
    // So read up to max_line_count lines to try to find the actual header
    const int max_line_count = 3;

    for( int ii = 0; ii < max_line_count; ii++ )
    {
        memset( iline, 0, 82 );
        ifile.getline( iline, 82 );

        iline[81] = 0;  // ensure NULL termination when string is too long

        // check for STEP in Part 21 format
        // (this can give false positives since Part 21 is not exclusively STEP)
        if( !strncmp( iline, "ISO-10303-21;", 13 ) )
        {
            format_type = FMT_STEP;
            break;
        }

        std::string fstr = iline;

        // check for STEP in XML format
        // (this can give both false positive and false negatives)
        if( fstr.find( "urn:oid:1.0.10303." ) != std::string::npos )
        {
            format_type = FMT_STEP;
            break;
        }

        // Note: this is a very simple test which can yield false positives; the only
        // sure method for determining if a file *not* an IGES model is to attempt
        // to load it.
        if( iline[72] == 'S' && ( iline[80] == 0 || iline[80] == 13 || iline[80] == 10 ) )
        {
            format_type = FMT_IGES;
            break;
        }

        // Only a comment (starting by "/*") is allowed as header
        if( strncmp( iline, "/*", 2 ) != 0 )    // not a comment
             break;
    }

    CLOSE_STREAM( ifile );

    return format_type;
}


static VECTOR2D CircleCenterFrom3Points( const VECTOR2D& p1, const VECTOR2D& p2,
                                         const VECTOR2D& p3 )
{
    VECTOR2D center;

    // Move coordinate origin to p2, to simplify calculations
    VECTOR2D b = p1 - p2;
    VECTOR2D d = p3 - p2;
    double   bc = ( b.x * b.x + b.y * b.y ) / 2.0;
    double   cd = ( -d.x * d.x - d.y * d.y ) / 2.0;
    double   det = -b.x * d.y + d.x * b.y;

    // We're fine with divisions by 0
    det = 1.0 / det;
    center.x = ( -bc * d.y - cd * b.y ) * det;
    center.y = ( b.x * cd + d.x * bc ) * det;
    center += p2;

    return center;
}


#define APPROX_DBG( stmt )
//#define APPROX_DBG( stmt ) stmt

static SHAPE_LINE_CHAIN approximateLineChainWithArcs( const SHAPE_LINE_CHAIN& aSrc )
{
    // An algo that takes 3 points, calculates a circle center,
    // then tries to find as many points fitting the circle.

    static const double c_radiusDeviation = 1000.0;
    static const double c_arcCenterDeviation = 1000.0;
    static const double c_relLengthDeviation = 0.8;
    static const int    c_last_none = -1000; // Meaning the arc cannot be constructed
    // Allow larger angles for segments below this size
    static const double c_smallSize = pcbIUScale.mmToIU( 0.1 );
    static const double c_circleCloseGap = pcbIUScale.mmToIU( 1.0 );

    APPROX_DBG( std::cout << std::endl );

    if( aSrc.PointCount() < 4 )
        return aSrc;

    if( !aSrc.IsClosed() )
        return aSrc; // non-closed polygons are not supported

    SHAPE_LINE_CHAIN dst;

    int jEndIdx = aSrc.PointCount() - 3;

    for( int i = 0; i < aSrc.PointCount(); i++ )
    {
        int first = i - 3;
        int last = c_last_none;

        VECTOR2D p0 = aSrc.CPoint( i - 3 );
        VECTOR2D p1 = aSrc.CPoint( i - 2 );
        VECTOR2D p2 = aSrc.CPoint( i - 1 );

        APPROX_DBG( std::cout << i << " " << aSrc.CPoint( i ) << " " << ( i - 3 ) << " "
                              << VECTOR2I( p0 ) << " " << ( i - 2 ) << " " << VECTOR2I( p1 ) << " "
                              << ( i - 1 ) << " " << VECTOR2I( p2 ) << std::endl );

        VECTOR2D v01 = p1 - p0;
        VECTOR2D v12 = p2 - p1;

        bool defective = false;

        double d01 = v01.EuclideanNorm();
        double d12 = v12.EuclideanNorm();

        // Check distance differences between 3 first points
        defective |= std::abs( d01 - d12 ) > ( std::max( d01, d12 ) * c_relLengthDeviation );

        if( !defective )
        {
            // Check angles between 3 first points
            EDA_ANGLE a01( v01 );
            EDA_ANGLE a12( v12 );

            double a_diff = ( a01 - a12 ).Normalize180().AsDegrees();
            defective |= std::abs( a_diff ) < 0.1;

            // Larger angles are allowed for smaller geometry
            double maxAngleDiff = std::max( d01, d12 ) < c_smallSize ? 46.0 : 30.0;
            defective |= std::abs( a_diff ) >= maxAngleDiff;
        }

        if( !defective )
        {
            // Find last point lying on the circle created from 3 first points
            VECTOR2D  center = CircleCenterFrom3Points( p0, p1, p2 );
            double    radius = ( p0 - center ).EuclideanNorm();
            VECTOR2D  p_prev = p2;
            EDA_ANGLE a_prev( v12 );

            for( int j = i; j <= jEndIdx; j++ )
            {
                VECTOR2D p_test = aSrc.CPoint( j );

                EDA_ANGLE a_test( p_test - p_prev );
                double    rad_test = ( p_test - center ).EuclideanNorm();
                double    d_tl = ( p_test - p_prev ).EuclideanNorm();
                double    rad_dev = std::abs( radius - rad_test );

                APPROX_DBG( std::cout << " " << j << " " << aSrc.CPoint( j ) << " rad "
                                      << int64_t( rad_test ) << " ref " << int64_t( radius )
                                      << std::endl );

                if( rad_dev > c_radiusDeviation )
                {
                    APPROX_DBG( std::cout << "  " << j
                                          << " Radius deviation too large: " << int64_t( rad_dev )
                                          << " > " << c_radiusDeviation << std::endl );
                    break;
                }

                // Larger angles are allowed for smaller geometry
                double maxAngleDiff =
                        std::max( std::max( d01, d12 ), d_tl ) < c_smallSize ? 46.0 : 30.0;

                double a_diff_test = ( a_prev - a_test ).Normalize180().AsDegrees();
                if( std::abs( a_diff_test ) >= maxAngleDiff )
                {
                    APPROX_DBG( std::cout << "  " << j << " Angles differ too much " << a_diff_test
                                          << std::endl );
                    break;
                }

                if( std::abs( d_tl - d01 ) > ( std::max( d_tl, d01 ) * c_relLengthDeviation ) )
                {
                    APPROX_DBG( std::cout << "  " << j << " Lengths differ too much " << d_tl
                                          << "; " << d01 << std::endl );
                    break;
                }

                last = j;
                p_prev = p_test;
                a_prev = a_test;
            }
        }

        if( last != c_last_none )
        {
            // Try to add an arc, testing for self-interference
            SHAPE_ARC arc( aSrc.CPoint( first ), aSrc.CPoint( ( first + last ) / 2 ),
                           aSrc.CPoint( last ), 0 );

            if( last > aSrc.PointCount() - 3 && !dst.IsArcSegment( 0 ) )
            {
                // If we've found an arc at the end, but already added segments at the start, remove them.
                int toRemove = last - ( aSrc.PointCount() - 3 );

                while( toRemove )
                {
                    dst.RemoveShape( 0 );
                    toRemove--;
                }
            }

            SHAPE_LINE_CHAIN testChain = dst;

            testChain.Append( arc );
            testChain.Append( aSrc.Slice( last, std::max( last, aSrc.PointCount() - 3 ) ) );
            testChain.SetClosed( aSrc.IsClosed() );

            if( !testChain.SelfIntersectingWithArcs() )
            {
                // Add arc
                dst.Append( arc );

                APPROX_DBG( std::cout << " Add arc start " << arc.GetP0() << " mid "
                                      << arc.GetArcMid() << " end " << arc.GetP1() << std::endl );

                i = last + 3;
            }
            else
            {
                // Self-interference
                last = c_last_none;

                APPROX_DBG( std::cout << " Self-intersection check failed" << std::endl );
            }
        }

        if( last == c_last_none )
        {
            if( first < 0 )
                jEndIdx = first + aSrc.PointCount();

            // Add point
            dst.Append( p0 );
            APPROX_DBG( std::cout << " Add pt " << VECTOR2I( p0 ) << std::endl );
        }
    }

    dst.SetClosed( true );

    // Try to merge arcs
    int iarc0 = dst.ArcIndex( 0 );
    int iarc1 = dst.ArcIndex( dst.GetSegmentCount() - 1 );

    if( iarc0 != -1 && iarc1 != -1 )
    {
        APPROX_DBG( std::cout << "Final arcs " << iarc0 << " " << iarc1 << std::endl );

        if( iarc0 == iarc1 )
        {
            SHAPE_ARC arc = dst.Arc( iarc0 );

            VECTOR2D p0 = arc.GetP0();
            VECTOR2D p1 = arc.GetP1();

            // If we have only one arc and the gap is small, make it a circle
            if( ( p1 - p0 ).EuclideanNorm() < c_circleCloseGap )
            {
                dst.Clear();
                dst.Append( SHAPE_ARC( arc.GetCenter(), arc.GetP0(), ANGLE_360 ) );
            }
        }
        else
        {
            // Merge first and last arcs if they are similar
            SHAPE_ARC arc0 = dst.Arc( iarc0 );
            SHAPE_ARC arc1 = dst.Arc( iarc1 );

            VECTOR2D ac0 = arc0.GetCenter();
            VECTOR2D ac1 = arc1.GetCenter();

            double ar0 = arc0.GetRadius();
            double ar1 = arc1.GetRadius();

            if( std::abs( ar0 - ar1 ) <= c_radiusDeviation
                && ( ac0 - ac1 ).EuclideanNorm() <= c_arcCenterDeviation )
            {
                dst.RemoveShape( 0 );
                dst.RemoveShape( -1 );

                SHAPE_ARC merged( arc1.GetP0(), arc1.GetArcMid(), arc0.GetP1(), 0 );

                dst.Append( merged );
            }
        }
    }

    return dst;
}


static TopoDS_Shape getOneShape( Handle( XCAFDoc_ShapeTool ) aShapeTool )
{
    TDF_LabelSequence theLabels;
    aShapeTool->GetFreeShapes( theLabels );

    TopoDS_Shape aShape;

    if( theLabels.Length() == 1 )
        return aShapeTool->GetShape( theLabels.Value( 1 ) );

    TopoDS_Compound aCompound;
    BRep_Builder    aBuilder;
    aBuilder.MakeCompound( aCompound );

    for( TDF_LabelSequence::Iterator anIt( theLabels ); anIt.More(); anIt.Next() )
    {
        TopoDS_Shape aFreeShape;

        if( !aShapeTool->GetShape( anIt.Value(), aFreeShape ) )
            continue;

        aBuilder.Add( aCompound, aFreeShape );
    }

    if( aCompound.NbChildren() > 0 )
        aShape = aCompound;

    return aShape;
}


// Apply scaling to shapes within theLabel.
// Based on XCAFDoc_Editor::RescaleGeometry
static Standard_Boolean rescaleShapes( const TDF_Label& theLabel, const gp_XYZ& aScale )
{
    if( theLabel.IsNull() )
    {
        Message::SendFail( "Null label." );
        return Standard_False;
    }

    if( Abs( aScale.X() ) <= gp::Resolution() || Abs( aScale.Y() ) <= gp::Resolution()
        || Abs( aScale.Z() ) <= gp::Resolution() )
    {
        Message::SendFail( "Scale factor is too small." );
        return Standard_False;
    }

    Handle( XCAFDoc_ShapeTool ) aShapeTool = XCAFDoc_DocumentTool::ShapeTool( theLabel );

    if( aShapeTool.IsNull() )
    {
        Message::SendFail( "Couldn't find XCAFDoc_ShapeTool attribute." );
        return Standard_False;
    }

    Handle( KI_XCAFDoc_AssemblyGraph ) aG = new KI_XCAFDoc_AssemblyGraph( theLabel );

    if( aG.IsNull() )
    {
        Message::SendFail( "Couldn't create assembly graph." );
        return Standard_False;
    }

    Standard_Boolean anIsDone = Standard_True;

    // clang-format off
    gp_GTrsf aGTrsf;
    aGTrsf.SetVectorialPart( gp_Mat( aScale.X(), 0, 0,
                                     0, aScale.Y(), 0,
                                     0, 0, aScale.Z() ) );
    // clang-format on

    BRepBuilderAPI_GTransform aBRepTrsf( aGTrsf );

    for( Standard_Integer idx = 1; idx <= aG->NbNodes(); idx++ )
    {
        const KI_XCAFDoc_AssemblyGraph::NodeType aNodeType = aG->GetNodeType( idx );

        if( ( aNodeType != KI_XCAFDoc_AssemblyGraph::NodeType_Part )
            && ( aNodeType != KI_XCAFDoc_AssemblyGraph::NodeType_Occurrence ) )
        {
            continue;
        }

        const TDF_Label& aLabel = aG->GetNode( idx );

        if( aNodeType == KI_XCAFDoc_AssemblyGraph::NodeType_Part )
        {
            const TopoDS_Shape aShape = aShapeTool->GetShape( aLabel );
            aBRepTrsf.Perform( aShape, Standard_True );
            if( !aBRepTrsf.IsDone() )
            {
                Standard_SStream        aSS;
                TCollection_AsciiString anEntry;
                TDF_Tool::Entry( aLabel, anEntry );
                aSS << "Shape " << anEntry << " is not scaled!";
                Message::SendFail( aSS.str().c_str() );
                anIsDone = Standard_False;
                return Standard_False;
            }
            TopoDS_Shape aScaledShape = aBRepTrsf.Shape();
            aShapeTool->SetShape( aLabel, aScaledShape );

            // Update sub-shapes
            TDF_LabelSequence aSubshapes;
            aShapeTool->GetSubShapes( aLabel, aSubshapes );
            for( TDF_LabelSequence::Iterator anItSs( aSubshapes ); anItSs.More(); anItSs.Next() )
            {
                const TDF_Label&   aLSs = anItSs.Value();
                const TopoDS_Shape aSs = aShapeTool->GetShape( aLSs );
                const TopoDS_Shape aSs1 = aBRepTrsf.ModifiedShape( aSs );
                aShapeTool->SetShape( aLSs, aSs1 );
            }

            // These attributes will be recomputed eventually, but clear them just in case
            aLabel.ForgetAttribute( XCAFDoc_Area::GetID() );
            aLabel.ForgetAttribute( XCAFDoc_Centroid::GetID() );
            aLabel.ForgetAttribute( XCAFDoc_Volume::GetID() );
        }
        else if( aNodeType == KI_XCAFDoc_AssemblyGraph::NodeType_Occurrence )
        {
            TopLoc_Location aLoc = aShapeTool->GetLocation( aLabel );
            gp_Trsf         aTrsf = aLoc.Transformation();
            aTrsf.SetTranslationPart( aTrsf.TranslationPart().Multiplied( aScale ) );
            XCAFDoc_Location::Set( aLabel, aTrsf );
        }
    }

    if( !anIsDone )
    {
        return Standard_False;
    }

    aShapeTool->UpdateAssemblies();

    return anIsDone;
}


static bool fuseShapes( auto& aInputShapes, TopoDS_Shape& aOutShape, REPORTER* aReporter )
{
    BRepAlgoAPI_Fuse     mkFuse;
    TopTools_ListOfShape shapeArguments, shapeTools;

    for( TopoDS_Shape& sh : aInputShapes )
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

    if( mkFuse.HasErrors() || mkFuse.HasWarnings() )
    {
        aReporter->Report( _( "** Got problems while fusing shapes **" ), RPT_SEVERITY_ERROR );

        if( mkFuse.HasErrors() )
        {
            wxString             msg = _( "Errors:\n" );
            wxStringOutputStream os_stream( &msg );
            wxStdOutputStream    out( os_stream );

            mkFuse.DumpErrors( out );
            aReporter->Report( msg, RPT_SEVERITY_ERROR );
        }

        if( mkFuse.HasWarnings() )
        {
            wxString             msg = _( "Warnings:\n" );
            wxStringOutputStream os_stream( &msg );
            wxStdOutputStream    out( os_stream );

            mkFuse.DumpWarnings( out );
            aReporter->Report( msg, RPT_SEVERITY_WARNING );
        }
    }

    if( mkFuse.IsDone() )
    {
        TopoDS_Shape fusedShape = mkFuse.Shape();

        ShapeUpgrade_UnifySameDomain unify( fusedShape, true, true, false );
        unify.History() = nullptr;
        unify.Build();

        TopoDS_Shape unifiedShapes = unify.Shape();

        if( unifiedShapes.IsNull() )
        {
            aReporter->Report( _( "** ShapeUpgrade_UnifySameDomain produced a null shape **" ),
                               RPT_SEVERITY_ERROR );
        }
        else
        {
            aOutShape = unifiedShapes;
            return true;
        }
    }

    return false;
}


static TopoDS_Compound makeCompound( const auto& aInputShapes )
{
    TopoDS_Compound compound;
    BRep_Builder    builder;
    builder.MakeCompound( compound );

    for( const TopoDS_Shape& shape : aInputShapes )
        builder.Add( compound, shape );

    return compound;
}


// Try to fuse shapes. If that fails, just add them to a compound
static TopoDS_Shape fuseShapesOrCompound( const TopTools_ListOfShape& aInputShapes,
                                          REPORTER* aReporter )
{
    TopoDS_Shape outShape;

    if( aInputShapes.Size() == 1 )
        return aInputShapes.First();

    if( fuseShapes( aInputShapes, outShape, aReporter ) )
        return outShape;

    return makeCompound( aInputShapes );
}


// Sets names in assembly to <aPrefix> (<old name>), or to <aPrefix>
static Standard_Boolean prefixNames( const TDF_Label&                  aLabel,
                                     const TCollection_ExtendedString& aPrefix )
{
    Handle( KI_XCAFDoc_AssemblyGraph ) aG = new KI_XCAFDoc_AssemblyGraph( aLabel );

    if( aG.IsNull() )
    {
        Message::SendFail( "Couldn't create assembly graph." );
        return Standard_False;
    }

    Standard_Boolean anIsDone = Standard_True;

    for( Standard_Integer idx = 1; idx <= aG->NbNodes(); idx++ )
    {
        const TDF_Label& lbl = aG->GetNode( idx );
        Handle( TDataStd_Name ) nameHandle;

        if( lbl.FindAttribute( TDataStd_Name::GetID(), nameHandle ) )
        {
            TCollection_ExtendedString name;

            name += aPrefix;
            name += " (";
            name += nameHandle->Get();
            name += ")";

            TDataStd_Name::Set( lbl, name );
        }
        else
        {
            TDataStd_Name::Set( lbl, aPrefix );
        }
    }

    return anIsDone;
}


STEP_PCB_MODEL::STEP_PCB_MODEL( const wxString& aPcbName, REPORTER* aReporter ) :
        m_reporter( aReporter )
{
    m_app = XCAFApp_Application::GetApplication();
    m_app->NewDocument( "MDTV-XCAF", m_doc );
    m_assy = XCAFDoc_DocumentTool::ShapeTool( m_doc->Main() );
    m_assy_label = m_assy->NewShape();
    m_hasPCB = false;
    m_simplifyShapes = true;
    m_components = 0;
    m_precision = USER_PREC;
    m_angleprec = USER_ANGLE_PREC;
    m_mergeOCCMaxDist = OCC_MAX_DISTANCE_TO_MERGE_POINTS;
    m_minx = 1.0e10;    // absurdly large number; any valid PCB X value will be smaller
    m_pcbName = aPcbName;
    m_fuseShapes = false;
    m_extraPadThickness = true;
    m_outFmt = OUTPUT_FORMAT::FMT_OUT_UNKNOWN;
}


STEP_PCB_MODEL::~STEP_PCB_MODEL()
{
    if( m_doc->CanClose() == CDM_CCS_OK )
        m_doc->Close();
}


bool STEP_PCB_MODEL::AddPadShape( const PAD* aPad, const VECTOR2D& aOrigin, bool aVia,
                                  SHAPE_POLY_SET* aClipPolygon )
{
    const double              c_padExtraThickness = 0.005;
    bool                      success = true;
    std::vector<TopoDS_Shape> padShapes;
    bool castellated = aClipPolygon && aPad->GetProperty() == PAD_PROP::CASTELLATED;

    for( PCB_LAYER_ID pcb_layer : aPad->GetLayerSet().Seq() )
    {
        if( !m_enabledLayers.Contains( pcb_layer ) )
            continue;

        if( pcb_layer == F_Mask || pcb_layer == B_Mask )
            continue;

        if( !aPad->FlashLayer( pcb_layer ) )
            continue;

        double Zpos, thickness;
        getLayerZPlacement( pcb_layer, Zpos, thickness );

        if( !aVia && m_extraPadThickness )
        {
            // Pad surface as a separate face for FEM simulations.
            if( pcb_layer == F_Cu )
                thickness += c_padExtraThickness;
            else if( pcb_layer == B_Cu )
                thickness -= c_padExtraThickness;
        }

        TopoDS_Shape testShape;

        // Make a shape on copper layers
        SHAPE_POLY_SET polySet;
        aPad->TransformShapeToPolygon( polySet, pcb_layer, 0, aPad->GetMaxError(), ERROR_INSIDE );

        if( castellated )
        {
            polySet.ClearArcs();
            polySet.BooleanIntersection( *aClipPolygon );
        }

        success &= MakeShapes( padShapes, polySet, m_simplifyShapes, thickness, Zpos, aOrigin );

        if( testShape.IsNull() )
        {
            std::vector<TopoDS_Shape> testShapes;

            MakeShapes( testShapes, polySet, m_simplifyShapes, 0.0, Zpos + thickness, aOrigin );

            if( testShapes.size() > 0 )
                testShape = testShapes.front();
        }

        if( !aVia && m_extraPadThickness && !testShape.IsNull() )
        {
            if( pcb_layer == F_Cu || pcb_layer == B_Cu )
            {
                wxString name;

                name << "Pad_";

                if( pcb_layer == F_Cu )
                    name << 'F' << '_';
                else if( pcb_layer == B_Cu )
                    name << 'B' << '_';

                name << aPad->GetParentFootprint()->GetReferenceAsString() << '_'
                     << aPad->GetNumber() << '_' << aPad->GetShortNetname();

                gp_Pnt point( pcbIUScale.IUTomm( aPad->GetX() - aOrigin.x ),
                              -pcbIUScale.IUTomm( aPad->GetY() - aOrigin.y ), Zpos + thickness );

                m_pad_points[name].emplace_back( point, testShape );
            }
        }
    }

    if( aPad->GetAttribute() == PAD_ATTRIB::PTH && aPad->IsOnLayer( F_Cu ) && aPad->IsOnLayer( B_Cu ) )
    {
        double f_pos, f_thickness;
        double b_pos, b_thickness;
        getLayerZPlacement( F_Cu, f_pos, f_thickness );
        getLayerZPlacement( B_Cu, b_pos, b_thickness );

        if( !aVia && m_extraPadThickness )
        {
            // Pad surface is slightly thicker
            f_thickness += c_padExtraThickness;
            b_thickness -= c_padExtraThickness;
        }

        double top = std::max( f_pos, f_pos + f_thickness );
        double bottom = std::min( b_pos, b_pos + b_thickness );
        double hole_height = top - bottom;

        TopoDS_Shape plating;

        std::shared_ptr<SHAPE_SEGMENT> seg_hole = aPad->GetEffectiveHoleShape();
        double width = std::min( aPad->GetDrillSize().x, aPad->GetDrillSize().y );

        if( !castellated )
        {
            if( MakeShapeAsThickSegment( plating, seg_hole->GetSeg().A, seg_hole->GetSeg().B, width,
                                         hole_height, bottom, aOrigin ) )
            {
                padShapes.push_back( plating );
            }
            else
            {
                success = false;
            }
        }
        else
        {
            // Note:
            // the truncated hole shape is exported as a vertical filled shape. The hole itself
            // will be removed later, when all holes are removed from the board
            SHAPE_POLY_SET polyHole;

            if( seg_hole->GetSeg().A == seg_hole->GetSeg().B )  // Hole is a circle
            {
                TransformCircleToPolygon( polyHole, seg_hole->GetSeg().A, width/2,
                                          aPad->GetMaxError(), ERROR_OUTSIDE );

            }
            else
            {
                TransformOvalToPolygon( polyHole, seg_hole->GetSeg().A, seg_hole->GetSeg().B, width,
                                        aPad->GetMaxError(), ERROR_OUTSIDE );
            }

            polyHole.ClearArcs();
            polyHole.BooleanIntersection( *aClipPolygon );

            if( MakePolygonAsWall( plating, polyHole, hole_height, bottom, aOrigin ) )
            {
                padShapes.push_back( plating );
            }
            else
            {
                success = false;
            }
        }
    }

    if( !success ) // Error
        m_reporter->Report( _( "OCC error adding pad/via polygon." ), RPT_SEVERITY_ERROR );

    if( !padShapes.empty() )
    {
        // Fuse pad shapes here before fusing them with tracks because OCCT sometimes has trouble
        if( m_fuseShapes )
        {
            TopTools_ListOfShape padShapesList;

            for( const TopoDS_Shape& shape : padShapes )
                padShapesList.Append( shape );

            m_board_copper_pads[aPad->GetNetname()].push_back( fuseShapesOrCompound( padShapesList, m_reporter ) );
        }
        else
        {
            for( const TopoDS_Shape& shape : padShapes )
                m_board_copper_pads[aPad->GetNetname()].push_back( shape );
        }
    }

    return success;
}


bool STEP_PCB_MODEL::AddHole( const SHAPE_SEGMENT& aShape, int aPlatingThickness,
                              PCB_LAYER_ID aLayerTop, PCB_LAYER_ID aLayerBot, bool aVia,
                              const VECTOR2D& aOrigin, bool aCutCopper, bool aCutBody )
{
    double margin = 0.001; // a small margin on the Z axix to be sure the hole
                           // is bigger than the board with copper
                           // must be > OCC_MAX_DISTANCE_TO_MERGE_POINTS

    // Pads are taller by 0.01 mm
    if( !aVia && m_extraPadThickness)
        margin += 0.01;

    double f_pos, f_thickness;
    double b_pos, b_thickness;
    getLayerZPlacement( aLayerTop, f_pos, f_thickness );
    getLayerZPlacement( aLayerBot, b_pos, b_thickness );
    double top = std::max( f_pos, f_pos + f_thickness );
    double bottom = std::min( b_pos, b_pos + b_thickness );

    double holeZsize = ( top - bottom ) + ( margin * 2 );

    double boardDrill = aShape.GetWidth();
    double copperDrill = boardDrill - aPlatingThickness * 2;

    TopoDS_Shape copperHole, boardHole;

    if( aCutCopper )
    {
        if( MakeShapeAsThickSegment( copperHole, aShape.GetSeg().A, aShape.GetSeg().B, copperDrill,
                                     holeZsize, bottom - margin, aOrigin ) )
        {
            m_copperCutouts.push_back( copperHole );
        }
        else
        {
            return false;
        }
    }

    if( aCutBody )
    {
        if( MakeShapeAsThickSegment( boardHole, aShape.GetSeg().A, aShape.GetSeg().B, boardDrill,
                                     holeZsize, bottom - margin, aOrigin ) )
        {
            m_boardCutouts.push_back( boardHole );
        }
        else
        {
            return false;
        }
    }

    return true;
}


bool STEP_PCB_MODEL::AddBarrel( const SHAPE_SEGMENT& aShape, PCB_LAYER_ID aLayerTop,
                                PCB_LAYER_ID aLayerBot, bool aVia, const VECTOR2D& aOrigin,
                                const wxString& aNetname )
{
    double f_pos, f_thickness;
    double b_pos, b_thickness;
    getLayerZPlacement( aLayerTop, f_pos, f_thickness );
    getLayerZPlacement( aLayerBot, b_pos, b_thickness );
    double top = std::max( f_pos, f_pos + f_thickness );
    double bottom = std::min( b_pos, b_pos + b_thickness );

    TopoDS_Shape plating;

    if( !MakeShapeAsThickSegment( plating, aShape.GetSeg().A, aShape.GetSeg().B, aShape.GetWidth(),
                                  ( top - bottom ), bottom, aOrigin ) )
    {
        return false;
    }

    if( aVia )
        m_board_copper_vias[aNetname].push_back( plating );
    else
        m_board_copper_pads[aNetname].push_back( plating );

    return true;
}


bool STEP_PCB_MODEL::AddBackdrill( const SHAPE_SEGMENT& aShape, PCB_LAYER_ID aLayerStart,
                                   PCB_LAYER_ID aLayerEnd, const VECTOR2D& aOrigin )
{
    // A backdrill removes board material and copper plating between two layers.
    // The backdrill typically starts from an outer layer and drills into an inner layer.
    // For example, a top backdrill starts at F_Cu and ends at an inner layer.
    // A bottom backdrill starts at B_Cu and ends at an inner layer.

    double margin = 0.001; // a small margin on the Z axis to ensure the hole
                           // is bigger than the board section being removed

    // Extra margin to extend past outer copper layers to ensure complete annular ring removal
    double copperMargin = 0.5;  // 0.5mm extra to cut through any copper/pad thickness

    double start_pos, start_thickness;
    double end_pos, end_thickness;
    getLayerZPlacement( aLayerStart, start_pos, start_thickness );
    getLayerZPlacement( aLayerEnd, end_pos, end_thickness );

    // Calculate the Z extent of the backdrill
    double top = std::max( { start_pos, start_pos + start_thickness,
                             end_pos, end_pos + end_thickness } );
    double bottom = std::min( { start_pos, start_pos + start_thickness,
                                end_pos, end_pos + end_thickness } );

    // Extend past outer copper layers if the backdrill reaches them
    if( aLayerStart == F_Cu || aLayerEnd == F_Cu )
        top += copperMargin;
    if( aLayerStart == B_Cu || aLayerEnd == B_Cu )
        bottom -= copperMargin;

    double holeZsize = ( top - bottom ) + ( margin * 2 );
    double holeZpos = bottom - margin;

    double backdrillDiameter = aShape.GetWidth();

    TopoDS_Shape backdrillHole;

    // Create the backdrill hole shape - this cuts the board body
    if( MakeShapeAsThickSegment( backdrillHole, aShape.GetSeg().A, aShape.GetSeg().B,
                                 backdrillDiameter, holeZsize, holeZpos, aOrigin ) )
    {
        m_boardCutouts.push_back( backdrillHole );

        // This removes annular rings and barrel copper between the backdrill layers.
        m_copperCutouts.push_back( backdrillHole );
    }
    else
    {
        return false;
    }

    return true;
}


bool STEP_PCB_MODEL::AddCounterbore( const VECTOR2I& aPosition, int aDiameter, int aDepth,
                                     bool aFrontSide, const VECTOR2D& aOrigin )
{
    wxLogTrace( traceKiCad2Step, wxT( "AddCounterbore: pos=(%d,%d) diameter=%d depth=%d frontSide=%d origin=(%f,%f)" ),
                aPosition.x, aPosition.y, aDiameter, aDepth, aFrontSide ? 1 : 0, aOrigin.x, aOrigin.y );

    // A counterbore is a cylindrical recess from the top or bottom of the board
    if( aDiameter <= 0 || aDepth <= 0 )
    {
        wxLogTrace( traceKiCad2Step, wxT( "AddCounterbore: REJECTED - invalid diameter=%d or depth=%d" ),
                    aDiameter, aDepth );
        return false;
    }

    double margin = 0.001;  // small margin to ensure clean cuts

    // Extra margin to extend past outer copper layers to ensure complete annular ring removal
    double copperMargin = 0.5;  // 0.5mm extra to cut through any copper/pad thickness

    // Get board body position (between copper layers)
    double boardZpos, boardThickness;
    getBoardBodyZPlacement( boardZpos, boardThickness );

    // Get copper layer positions - these extend beyond the board body
    double f_pos, f_thickness, b_pos, b_thickness;
    getLayerZPlacement( F_Cu, f_pos, f_thickness );
    getLayerZPlacement( B_Cu, b_pos, b_thickness );

    // Calculate actual outer surfaces including copper
    // F_Cu: f_pos is inner surface, f_pos + f_thickness is outer surface (copper extends upward)
    // B_Cu: b_pos is inner surface, b_pos + b_thickness is outer surface (thickness is negative, copper extends downward)
    double topOuterSurface = std::max( f_pos, f_pos + f_thickness );
    double bottomOuterSurface = std::min( b_pos, b_pos + b_thickness );

    wxLogTrace( traceKiCad2Step, wxT( "AddCounterbore: boardZpos=%f boardThickness=%f f_pos=%f f_thickness=%f topOuter=%f bottomOuter=%f" ),
                boardZpos, boardThickness, f_pos, f_thickness, topOuterSurface, bottomOuterSurface );

    // Convert dimensions to mm
    double diameter_mm = pcbIUScale.IUTomm( aDiameter );
    double depth_mm = pcbIUScale.IUTomm( aDepth );
    double radius_mm = diameter_mm / 2.0;

    wxLogTrace( traceKiCad2Step, wxT( "AddCounterbore: diameter_mm=%f depth_mm=%f radius_mm=%f" ),
                diameter_mm, depth_mm, radius_mm );

    // Calculate cylinder position based on which side
    // The cylinder must extend past the outer surface to ensure complete copper removal
    double cylinderZpos;
    double cylinderHeight;

    if( aFrontSide )
    {
        // Counterbore from top - cylinder extends from above outer copper surface down to depth
        // Add copperMargin above the surface to ensure complete annular ring removal
        cylinderZpos = topOuterSurface - depth_mm - margin;
        cylinderHeight = depth_mm + copperMargin + 2 * margin;
    }
    else
    {
        // Counterbore from bottom - cylinder extends from below outer copper surface up to depth
        // Add copperMargin below the surface to ensure complete annular ring removal
        cylinderZpos = bottomOuterSurface - copperMargin - margin;
        cylinderHeight = depth_mm + copperMargin + 2 * margin;
    }

    // Convert position to mm
    double posX_mm = pcbIUScale.IUTomm( aPosition.x - aOrigin.x );
    double posY_mm = -pcbIUScale.IUTomm( aPosition.y - aOrigin.y );

    wxLogTrace( traceKiCad2Step, wxT( "AddCounterbore: posX_mm=%f posY_mm=%f cylinderZpos=%f cylinderHeight=%f" ),
                posX_mm, posY_mm, cylinderZpos, cylinderHeight );

    try
    {
        // Create coordinate system for the cylinder
        // The cylinder axis is along Z, positioned at the counterbore center
        gp_Ax2 axis( gp_Pnt( posX_mm, posY_mm, cylinderZpos ), gp::DZ() );

        TopoDS_Shape cylinder = BRepPrimAPI_MakeCylinder( axis, radius_mm, cylinderHeight );

        if( cylinder.IsNull() )
        {
            wxLogTrace( traceKiCad2Step, wxT( "AddCounterbore: FAILED - cylinder shape is null" ) );
            m_reporter->Report( _( "Failed to create counterbore cylinder shape" ),
                                RPT_SEVERITY_ERROR );
            return false;
        }

        // Add to both board and copper cutouts
        m_boardCutouts.push_back( cylinder );
        m_copperCutouts.push_back( cylinder );

        wxLogTrace( traceKiCad2Step, wxT( "AddCounterbore: SUCCESS - added cylinder. boardCutouts=%zu copperCutouts=%zu" ),
                    m_boardCutouts.size(), m_copperCutouts.size() );
    }
    catch( const Standard_Failure& e )
    {
        wxLogTrace( traceKiCad2Step, wxT( "AddCounterbore: EXCEPTION - %s" ), e.GetMessageString() );
        m_reporter->Report( wxString::Format( _( "OCC exception creating counterbore: %s" ),
                                              e.GetMessageString() ),
                            RPT_SEVERITY_ERROR );
        return false;
    }

    return true;
}


bool STEP_PCB_MODEL::AddCountersink( const VECTOR2I& aPosition, int aDiameter, int aDepth,
                                     int aAngle, bool aFrontSide, const VECTOR2D& aOrigin )
{
    wxLogTrace( traceKiCad2Step, wxT( "AddCountersink: pos=(%d,%d) diameter=%d depth=%d angle=%d frontSide=%d origin=(%f,%f)" ),
                aPosition.x, aPosition.y, aDiameter, aDepth, aAngle, aFrontSide ? 1 : 0, aOrigin.x, aOrigin.y );

    // A countersink is a conical recess from the top or bottom of the board
    // The angle parameter is the total cone angle in decidegrees
    // (angle between opposite sides of the cone)
    if( aDiameter <= 0 || aAngle <= 0 )
    {
        wxLogTrace( traceKiCad2Step, wxT( "AddCountersink: REJECTED - invalid diameter=%d or angle=%d" ),
                    aDiameter, aAngle );
        return false;
    }

    double margin = 0.001;  // small margin to ensure clean cuts

    // Extra margin to extend past outer copper layers to ensure complete annular ring removal
    double copperMargin = 0.5;  // 0.5mm extra to cut through any copper/pad thickness

    // Get board body position (between copper layers)
    double boardZpos, boardThickness;
    getBoardBodyZPlacement( boardZpos, boardThickness );

    // Get copper layer positions - these extend beyond the board body
    double f_pos, f_thickness, b_pos, b_thickness;
    getLayerZPlacement( F_Cu, f_pos, f_thickness );
    getLayerZPlacement( B_Cu, b_pos, b_thickness );

    // Calculate actual outer surfaces including copper
    double topOuterSurface = std::max( f_pos, f_pos + f_thickness );
    double bottomOuterSurface = std::min( b_pos, b_pos + b_thickness );

    wxLogTrace( traceKiCad2Step, wxT( "AddCountersink: boardZpos=%f boardThickness=%f f_pos=%f f_thickness=%f topOuter=%f bottomOuter=%f" ),
                boardZpos, boardThickness, f_pos, f_thickness, topOuterSurface, bottomOuterSurface );

    // Convert dimensions to mm
    double diameter_mm = pcbIUScale.IUTomm( aDiameter );
    double radius_mm = diameter_mm / 2.0;

    // Convert angle from decidegrees to radians
    // aAngle is the total cone angle, so half-angle is used for geometry
    double halfAngleRad = ( aAngle / 10.0 ) * M_PI / 180.0 / 2.0;

    // If depth is not specified, calculate it from the diameter and angle
    // The countersink depth is the full cone height: depth = radius / tan(halfAngle)
    double depth_mm;
    if( aDepth <= 0 )
    {
        // Calculate depth from diameter and angle
        depth_mm = radius_mm / tan( halfAngleRad );
        wxLogTrace( traceKiCad2Step, wxT( "AddCountersink: depth not specified, calculated depth_mm=%f from radius=%f and angle" ),
                    depth_mm, radius_mm );
    }
    else
    {
        depth_mm = pcbIUScale.IUTomm( aDepth );
    }

    wxLogTrace( traceKiCad2Step, wxT( "AddCountersink: diameter_mm=%f depth_mm=%f radius_mm=%f halfAngleRad=%f (deg=%f)" ),
                diameter_mm, depth_mm, radius_mm, halfAngleRad, halfAngleRad * 180.0 / M_PI );

    // Calculate the cone geometry
    // For a countersink, R1 (bottom radius) may be 0 (sharp point) or non-zero
    // R2 (top radius) is at the surface
    // The cone depth determines how deep it goes

    // Calculate bottom radius based on depth and angle
    // tan(halfAngle) = (R2 - R1) / depth
    // If we want the surface radius to be radius_mm and depth to be depth_mm:
    // R1 = R2 - depth * tan(halfAngle)
    double bottomRadius_mm = radius_mm - depth_mm * tan( halfAngleRad );

    wxLogTrace( traceKiCad2Step, wxT( "AddCountersink: bottomRadius_mm=%f (before clamp), tan(halfAngle)=%f" ),
                bottomRadius_mm, tan( halfAngleRad ) );

    if( bottomRadius_mm < 0 )
        bottomRadius_mm = 0;  // Cone comes to a point before reaching full depth

    // Calculate position based on which side
    // Extend the cone past the outer surface by copperMargin to ensure complete copper removal
    double coneZpos;
    double coneHeight = depth_mm + copperMargin + margin;
    double r1, r2;  // bottom and top radii for BRepPrimAPI_MakeCone

    // Convert position to mm
    double posX_mm = pcbIUScale.IUTomm( aPosition.x - aOrigin.x );
    double posY_mm = -pcbIUScale.IUTomm( aPosition.y - aOrigin.y );

    try
    {
        TopoDS_Shape cone;

        if( aFrontSide )
        {
            // Countersink from top - cone apex points down
            // In OCC, cone is built from z=0 to z=H with R1 at z=0 and R2 at z=H
            // For a top countersink, we want large radius at top, small at bottom
            coneZpos = topOuterSurface - depth_mm - margin;
            r1 = bottomRadius_mm;  // smaller radius at bottom (deeper into board)
            // Extend the top radius to account for the copperMargin extension above the surface
            r2 = radius_mm + ( copperMargin + margin ) * tan( halfAngleRad );

            wxLogTrace( traceKiCad2Step, wxT( "AddCountersink: FRONT - coneZpos=%f r1=%f r2=%f coneHeight=%f" ),
                        coneZpos, r1, r2, coneHeight );

            gp_Ax2 axis( gp_Pnt( posX_mm, posY_mm, coneZpos ), gp::DZ() );
            cone = BRepPrimAPI_MakeCone( axis, r1, r2, coneHeight );
        }
        else
        {
            // Countersink from bottom - cone apex points up
            // For bottom countersink, large radius at bottom, small at top
            // Extend below the surface by copperMargin
            coneZpos = bottomOuterSurface - copperMargin - margin;
            // Extend the bottom radius to account for the copperMargin extension below the surface
            r1 = radius_mm + ( copperMargin + margin ) * tan( halfAngleRad );
            r2 = bottomRadius_mm;  // smaller radius at top (deeper into board)

            wxLogTrace( traceKiCad2Step, wxT( "AddCountersink: BACK - coneZpos=%f r1=%f r2=%f coneHeight=%f" ),
                        coneZpos, r1, r2, coneHeight );

            gp_Ax2 axis( gp_Pnt( posX_mm, posY_mm, coneZpos ), gp::DZ() );
            cone = BRepPrimAPI_MakeCone( axis, r1, r2, coneHeight );
        }

        if( cone.IsNull() )
        {
            wxLogTrace( traceKiCad2Step, wxT( "AddCountersink: FAILED - cone shape is null" ) );
            m_reporter->Report( _( "Failed to create countersink cone shape" ),
                                RPT_SEVERITY_ERROR );
            return false;
        }

        // Add to both board and copper cutouts
        m_boardCutouts.push_back( cone );
        m_copperCutouts.push_back( cone );

        wxLogTrace( traceKiCad2Step, wxT( "AddCountersink: SUCCESS - added cone. boardCutouts=%zu copperCutouts=%zu" ),
                    m_boardCutouts.size(), m_copperCutouts.size() );
    }
    catch( const Standard_Failure& e )
    {
        wxLogTrace( traceKiCad2Step, wxT( "AddCountersink: EXCEPTION - %s" ), e.GetMessageString() );
        m_reporter->Report( wxString::Format( _( "OCC exception creating countersink: %s" ),
                                              e.GetMessageString() ),
                            RPT_SEVERITY_ERROR );
        return false;
    }

    return true;
}


std::map<PCB_LAYER_ID, int> STEP_PCB_MODEL::GetCopperLayerKnockouts( int aDiameter, int aDepth,
                                                                     int aAngle, bool aFrontSide )
{
    std::map<PCB_LAYER_ID, int> knockouts;

    // Get the outer surface positions (including copper)
    double f_pos, f_thickness, b_pos, b_thickness;
    getLayerZPlacement( F_Cu, f_pos, f_thickness );
    getLayerZPlacement( B_Cu, b_pos, b_thickness );

    double topOuterSurface = std::max( f_pos, f_pos + f_thickness );
    double bottomOuterSurface = std::min( b_pos, b_pos + b_thickness );

    // Convert dimensions to mm
    double diameter_mm = pcbIUScale.IUTomm( aDiameter );
    double radius_mm = diameter_mm / 2.0;

    // Calculate depth in mm
    double depth_mm;
    double halfAngleRad = 0.0;

    if( aAngle > 0 )
    {
        // Countersink - calculate half angle
        halfAngleRad = ( aAngle / 10.0 ) * M_PI / 180.0 / 2.0;

        // If depth is not specified for countersink, calculate from diameter and angle
        if( aDepth <= 0 )
            depth_mm = radius_mm / tan( halfAngleRad );
        else
            depth_mm = pcbIUScale.IUTomm( aDepth );
    }
    else
    {
        // Counterbore - use specified depth
        depth_mm = pcbIUScale.IUTomm( aDepth );
    }

    // Determine the Z range of the feature
    double featureTop, featureBottom;

    if( aFrontSide )
    {
        featureTop = topOuterSurface;
        featureBottom = topOuterSurface - depth_mm;
    }
    else
    {
        featureBottom = bottomOuterSurface;
        featureTop = bottomOuterSurface + depth_mm;
    }

    wxLogTrace( traceKiCad2Step, wxT( "GetCopperLayerKnockouts: featureTop=%f featureBottom=%f depth_mm=%f frontSide=%d" ),
                featureTop, featureBottom, depth_mm, aFrontSide ? 1 : 0 );

    // Iterate through all copper layers and check if they fall within the feature range
    for( const BOARD_STACKUP_ITEM* item : m_stackup.GetList() )
    {
        if( item->GetType() != BS_ITEM_TYPE_COPPER )
            continue;

        PCB_LAYER_ID layer = item->GetBrdLayerId();
        double layerZ, layerThickness;
        getLayerZPlacement( layer, layerZ, layerThickness );

        // Get the Z range of this copper layer (both inner and outer surfaces)
        double layerTop = std::max( layerZ, layerZ + layerThickness );
        double layerBottom = std::min( layerZ, layerZ + layerThickness );

        // Check if this layer overlaps with the feature Z range
        // A layer is affected if any part of it is within the feature range
        bool layerInRange = ( layerTop >= featureBottom && layerBottom <= featureTop );

        wxLogTrace( traceKiCad2Step, wxT( "GetCopperLayerKnockouts: layer %d Z=[%f, %f] feature=[%f, %f] inRange=%d" ),
                    static_cast<int>( layer ), layerBottom, layerTop, featureBottom, featureTop, layerInRange ? 1 : 0 );

        if( !layerInRange )
            continue;

        int knockoutDiameter;

        if( aAngle > 0 )
        {
            // Countersink - calculate diameter at this layer's Z level
            // Use the layer surface that's closest to the feature origin surface
            double layerSurfaceZ;
            if( aFrontSide )
            {
                // For front-side countersink, use the top surface of the layer
                layerSurfaceZ = layerTop;
            }
            else
            {
                // For back-side countersink, use the bottom surface of the layer
                layerSurfaceZ = layerBottom;
            }

            // Distance from the surface determines the radius at this Z
            double distanceFromSurface;
            if( aFrontSide )
                distanceFromSurface = topOuterSurface - layerSurfaceZ;
            else
                distanceFromSurface = layerSurfaceZ - bottomOuterSurface;

            // Radius at this depth: r = R - d * tan(halfAngle)
            double radiusAtLayer_mm = radius_mm - distanceFromSurface * tan( halfAngleRad );

            if( radiusAtLayer_mm <= 0 )
            {
                wxLogTrace( traceKiCad2Step, wxT( "GetCopperLayerKnockouts: layer %d - countersink tapers to point before this layer" ),
                            static_cast<int>( layer ) );
                continue;  // Cone tapers to a point before reaching this layer
            }

            knockoutDiameter = pcbIUScale.mmToIU( radiusAtLayer_mm * 2.0 );
            wxLogTrace( traceKiCad2Step, wxT( "GetCopperLayerKnockouts: layer %d (countersink) - distFromSurface=%f radiusAtLayer=%f diameter=%d" ),
                        static_cast<int>( layer ), distanceFromSurface, radiusAtLayer_mm, knockoutDiameter );
        }
        else
        {
            // Counterbore - constant diameter
            knockoutDiameter = aDiameter;
            wxLogTrace( traceKiCad2Step, wxT( "GetCopperLayerKnockouts: layer %d (counterbore) - diameter=%d" ),
                        static_cast<int>( layer ), knockoutDiameter );
        }

        knockouts[layer] = knockoutDiameter;
    }

    return knockouts;
}


void STEP_PCB_MODEL::getLayerZPlacement( const PCB_LAYER_ID aLayer, double& aZPos,
                                         double& aThickness )
{
    // Offsets above copper in mm
    static const double c_silkscreenAboveCopper = 0.04;
    static const double c_soldermaskAboveCopper = 0.015;

    if( IsCopperLayer( aLayer ) )
    {
        getCopperLayerZPlacement( aLayer, aZPos, aThickness );
    }
    else if( IsFrontLayer( aLayer ) )
    {
        double f_pos, f_thickness;
        getCopperLayerZPlacement( F_Cu, f_pos, f_thickness );
        double top = std::max( f_pos, f_pos + f_thickness );

        if( aLayer == F_SilkS )
            aZPos = top + c_silkscreenAboveCopper;
        else
            aZPos = top + c_soldermaskAboveCopper;

        aThickness = 0.0; // Normal points up
    }
    else if( IsBackLayer( aLayer ) )
    {
        double b_pos, b_thickness;
        getCopperLayerZPlacement( B_Cu, b_pos, b_thickness );
        double bottom = std::min( b_pos, b_pos + b_thickness );

        if( aLayer == B_SilkS )
            aZPos = bottom - c_silkscreenAboveCopper;
        else
            aZPos = bottom - c_soldermaskAboveCopper;

        aThickness = -0.0; // Normal points down
    }
}


void STEP_PCB_MODEL::getCopperLayerZPlacement( const PCB_LAYER_ID aLayer, double& aZPos,
                                               double& aThickness )
{
    int  z = 0;
    int  thickness = 0;
    bool wasPrepreg = false;

    const std::vector<BOARD_STACKUP_ITEM*>& materials = m_stackup.GetList();

    // Iterate from bottom to top
    for( auto it = materials.rbegin(); it != materials.rend(); ++it )
    {
        const BOARD_STACKUP_ITEM* item = *it;

        if( item->GetType() == BS_ITEM_TYPE_COPPER )
        {
            if( aLayer == B_Cu )
            {
                // This is the first encountered layer
                thickness = -item->GetThickness();
                break;
            }

            // Inner copper position is usually inside prepreg
            if( wasPrepreg && item->GetBrdLayerId() != F_Cu )
            {
                z += item->GetThickness();
                thickness = -item->GetThickness();
            }
            else
            {
                thickness = item->GetThickness();
            }

            if( item->GetBrdLayerId() == aLayer )
                break;

            if( !wasPrepreg && item->GetBrdLayerId() != B_Cu )
                z += item->GetThickness();
        }
        else if( item->GetType() == BS_ITEM_TYPE_DIELECTRIC )
        {
            wasPrepreg = ( item->GetTypeName() == KEY_PREPREG );

            // Dielectric can have sub-layers. Layer 0 is the main layer
            // Not frequent, but possible
            thickness = 0;
            for( int idx = 0; idx < item->GetSublayersCount(); idx++ )
                thickness += item->GetThickness( idx );

            z += thickness;
        }
    }

    aZPos = pcbIUScale.IUTomm( z );
    aThickness = pcbIUScale.IUTomm( thickness );
}


void STEP_PCB_MODEL::getBoardBodyZPlacement( double& aZPos, double& aThickness )
{
    double f_pos, f_thickness;
    double b_pos, b_thickness;
    getLayerZPlacement( F_Cu, f_pos, f_thickness );
    getLayerZPlacement( B_Cu, b_pos, b_thickness );
    double top = std::min( f_pos, f_pos + f_thickness );
    double bottom = std::max( b_pos, b_pos + b_thickness );

    aThickness = ( top - bottom );
    aZPos = bottom;

    wxASSERT( aZPos == 0.0 );
}


bool STEP_PCB_MODEL::AddPolygonShapes( const SHAPE_POLY_SET* aPolyShapes, PCB_LAYER_ID aLayer,
                                       const VECTOR2D& aOrigin, const wxString& aNetname )
{
    bool success = true;

    if( aPolyShapes->IsEmpty() )
        return true;

    if( !m_enabledLayers.Contains( aLayer ) )
        return true;

    double z_pos, thickness;
    getLayerZPlacement( aLayer, z_pos, thickness );

    std::vector<TopoDS_Shape>* targetVec = nullptr;

    if( IsCopperLayer( aLayer ) )
        targetVec = &m_board_copper[aNetname];
    else if( aLayer == F_SilkS )
        targetVec = &m_board_front_silk;
    else if( aLayer == B_SilkS )
        targetVec = &m_board_back_silk;
    else if( aLayer == F_Mask )
        targetVec = &m_board_front_mask;
    else
        targetVec = &m_board_back_mask;

    if( !MakeShapes( *targetVec, *aPolyShapes, m_simplifyShapes, thickness, z_pos, aOrigin ) )
    {
        m_reporter->Report( wxString::Format( _( "Could not add shape (%d points) to copper layer %s." ),
                                              aPolyShapes->FullPointCount(),
                                              LayerName( aLayer ) ),
                            RPT_SEVERITY_ERROR );

        success = false;
    }

    return success;
}


bool STEP_PCB_MODEL::AddComponent( const wxString& aBaseName, const wxString& aFileName,
                                   const std::vector<wxString>& aAltFilenames,
                                   const wxString& aRefDes, bool aBottom, VECTOR2D aPosition,
                                   double aRotation, VECTOR3D aOffset, VECTOR3D aOrientation,
                                   VECTOR3D aScale, bool aSubstituteModels )
{
    if( aFileName.empty() )
    {
        m_reporter->Report( wxString::Format( _( "No model defined for %s." ), aRefDes ),
                            RPT_SEVERITY_WARNING );
        return false;
    }

    m_reporter->Report( wxString::Format( wxT( "Adding component %s." ), aRefDes ), RPT_SEVERITY_DEBUG );

    // first retrieve a label
    TDF_Label lmodel;
    wxString  errorMessage;

    if( !getModelLabel( aBaseName, aFileName, aAltFilenames, aScale, lmodel, aSubstituteModels,
                        &errorMessage ) )
    {
        if( errorMessage.IsEmpty() )
            errorMessage.Printf( _( "No model for filename '%s'." ), aFileName );

        m_reporter->Report( errorMessage, RPT_SEVERITY_ERROR );
        return false;
    }

    // calculate the Location transform
    TopLoc_Location toploc;

    if( !getModelLocation( aBottom, aPosition, aRotation, aOffset, aOrientation, toploc ) )
    {
        m_reporter->Report(
                wxString::Format( _( "No location data for filename '%s'." ), aFileName ),
                            RPT_SEVERITY_ERROR );
        return false;
    }

    // add the located sub-assembly
    TDF_Label llabel = m_assy->AddComponent( m_assy_label, lmodel, toploc );

    if( llabel.IsNull() )
    {
        m_reporter->Report(
                wxString::Format( _( "Could not add component with filename '%s'." ), aFileName ),
                            RPT_SEVERITY_ERROR );
        return false;
    }

    // attach the RefDes name
    TCollection_ExtendedString refdes( aRefDes.utf8_str() );
    TDataStd_Name::Set( llabel, refdes );

    KICAD3D_INFO::Set( llabel, KICAD3D_MODEL_TYPE::COMPONENT, aRefDes.utf8_string() );

    return true;
}


void STEP_PCB_MODEL::SetEnabledLayers( const LSET& aLayers )
{
    m_enabledLayers = aLayers;
}


void STEP_PCB_MODEL::SetFuseShapes( bool aValue )
{
    m_fuseShapes = aValue;
}


void STEP_PCB_MODEL::SetSimplifyShapes( bool aValue )
{
    m_simplifyShapes = aValue;
}


void STEP_PCB_MODEL::SetStackup( const BOARD_STACKUP& aStackup )
{
    m_stackup = aStackup;
}


void STEP_PCB_MODEL::SetNetFilter( const wxString& aFilter )
{
    m_netFilter = aFilter;
}


void STEP_PCB_MODEL::SetExtraPadThickness( bool aValue )
{
    m_extraPadThickness = aValue;
}


void STEP_PCB_MODEL::SetCopperColor( double r, double g, double b )
{
    m_copperColor[0] = r;
    m_copperColor[1] = g;
    m_copperColor[2] = b;
}


void STEP_PCB_MODEL::SetPadColor( double r, double g, double b )
{
    m_padColor[0] = r;
    m_padColor[1] = g;
    m_padColor[2] = b;
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


bool STEP_PCB_MODEL::MakeShapeAsThickSegment( TopoDS_Shape& aShape, const VECTOR2D& aStartPoint,
                                              const VECTOR2D& aEndPoint, double aWidth, double aThickness,
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
        m_reporter->Report( wxString::Format( _( "OCC exception building shape segment: %s" ),
                                              e.GetMessageString() ),
                            RPT_SEVERITY_ERROR );
        return false;
    }

    BRepBuilderAPI_MakeFace face;

    try
    {
        gp_Pln plane( coords3D[0], gp::DZ() );
        face = BRepBuilderAPI_MakeFace( plane, wire );
    }
    catch( const Standard_Failure& e )
    {
        m_reporter->Report( wxString::Format( _( "OCC exception building face: %s" ),
                                              e.GetMessageString() ),
                            RPT_SEVERITY_ERROR );
        return false;
    }

    if( aThickness != 0.0 )
    {
        aShape = BRepPrimAPI_MakePrism( face, gp_Vec( 0, 0, aThickness ) );

        if( aShape.IsNull() )
        {
            m_reporter->Report( _( "Failed to create a prismatic shape" ),
                                RPT_SEVERITY_ERROR );
            return false;
        }
    }
    else
    {
        aShape = face;
    }

    return success;
}


bool STEP_PCB_MODEL::MakePolygonAsWall( TopoDS_Shape& aShape,
                                        SHAPE_POLY_SET& aPolySet,
                                        double aHeight,
                                        double aZposition, const VECTOR2D& aOrigin )
{
    std::vector<TopoDS_Shape> testShapes;

    bool success = MakeShapes( testShapes, aPolySet, m_simplifyShapes,
                               aHeight, aZposition, aOrigin );

    if( testShapes.size() > 0 )
        aShape = testShapes.front();
    else
        success = false;

    return success;
}


static wxString formatBBox( const BOX2I& aBBox )
{
    wxString       str;
    UNITS_PROVIDER unitsProvider( pcbIUScale, EDA_UNITS::MM );

    str << "x0: " << unitsProvider.StringFromValue( aBBox.GetLeft(), false ) << "; ";
    str << "y0: " << unitsProvider.StringFromValue( aBBox.GetTop(), false ) << "; ";
    str << "x1: " << unitsProvider.StringFromValue( aBBox.GetRight(), false ) << "; ";
    str << "y1: " << unitsProvider.StringFromValue( aBBox.GetBottom(), false );

    return str;
}


static bool makeWireFromChain( BRepLib_MakeWire& aMkWire, const SHAPE_LINE_CHAIN& aChain,
                               double aMergeOCCMaxDist, double aZposition, const VECTOR2D& aOrigin,
                               REPORTER* aReporter )
{
    auto toPoint =
            [&]( const VECTOR2D& aKiCoords ) -> gp_Pnt
            {
                return gp_Pnt( pcbIUScale.IUTomm( aKiCoords.x - aOrigin.x ),
                               -pcbIUScale.IUTomm( aKiCoords.y - aOrigin.y ), aZposition );
            };

    try
    {
        auto addSegment = [&]( const VECTOR2I& aPt0, const VECTOR2I& aPt1 ) -> bool
        {
            if( aPt0 == aPt1 )
                return false;

            gp_Pnt start = toPoint( aPt0 );
            gp_Pnt end = toPoint( aPt1 );

            BRepBuilderAPI_MakeEdge mkEdge( start, end );

            if( !mkEdge.IsDone() || mkEdge.Edge().IsNull() )
            {
                aReporter->Report( wxString::Format( _( "Failed to make segment edge (%d %d) -> (%d %d), "
                                                        "skipping" ),
                                                     aPt0.x, aPt0.y,
                                                     aPt1.x, aPt1.y ),
                                   RPT_SEVERITY_ERROR );
            }
            else
            {
                aMkWire.Add( mkEdge.Edge() );

                if( aMkWire.Error() != BRepLib_WireDone )
                {
                    aReporter->Report( wxString::Format( _( "Failed to add segment edge (%d %d) -> (%d %d)" ),
                                                         aPt0.x, aPt0.y,
                                                         aPt1.x, aPt1.y ),
                                       RPT_SEVERITY_ERROR );
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

                curve = GC_MakeCircle( axis, pcbIUScale.IUTomm( aArc.GetRadius() ) ).Value();
            }
            else
            {
                curve = GC_MakeArcOfCircle( toPoint( aPt0 ), toPoint( aArc.GetArcMid() ),
                                            toPoint( aArc.GetP1() ) ).Value();
            }

            if( curve.IsNull() )
                return false;

            aMkWire.Add( BRepBuilderAPI_MakeEdge( curve ) );

            if( !aMkWire.IsDone() )
            {
                aReporter->Report( wxString::Format( _( "Failed to add arc curve from (%d %d), arc p0 "
                                                        "(%d %d), mid (%d %d), p1 (%d %d)" ),
                                                     aPt0.x, aPt0.y,
                                                     aArc.GetP0().x, aArc.GetP0().y,
                                                     aArc.GetArcMid().x, aArc.GetArcMid().y,
                                                     aArc.GetP1().x, aArc.GetP1().y ),
                                   RPT_SEVERITY_ERROR );
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
                if( aChain.IsArcSegment( 0 ) && aChain.IsArcSegment( aChain.PointCount() - 1 )
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

        if( lastPt != firstPt && !addSegment( lastPt, firstPt ) )
        {
            aReporter->Report( wxString::Format( _( "Failed to close wire at %d, %d -> %d, %d **" ),
                                                 lastPt.x, lastPt.y,
                                                 firstPt.x, firstPt.y ),
                               RPT_SEVERITY_ERROR );

            return false;
        }
    }
    catch( const Standard_Failure& e )
    {
        aReporter->Report( wxString::Format( _( "OCC exception creating wire: %s" ),
                                             e.GetMessageString() ),
                           RPT_SEVERITY_ERROR );
        return false;
    }

    return true;
}


bool STEP_PCB_MODEL::MakeShapes( std::vector<TopoDS_Shape>& aShapes, const SHAPE_POLY_SET& aPolySet,
                                 bool aConvertToArcs, double aThickness, double aZposition,
                                 const VECTOR2D& aOrigin )
{
    SHAPE_POLY_SET workingPoly = aPolySet;
    workingPoly.Simplify();

    SHAPE_POLY_SET fallbackPoly = workingPoly;

    if( aConvertToArcs )
    {
        SHAPE_POLY_SET approximated = workingPoly;

        for( size_t polyId = 0; polyId < approximated.CPolygons().size(); polyId++ )
        {
            SHAPE_POLY_SET::POLYGON& polygon = approximated.Polygon( polyId );

            for( size_t contId = 0; contId < polygon.size(); contId++ )
                polygon[contId] = approximateLineChainWithArcs( polygon[contId] );
        }

        fallbackPoly = workingPoly;
        workingPoly = approximated;

        // TODO: this is not accurate because it doesn't check arcs.
        /*if( approximated.IsSelfIntersecting() )
        {
            m_reporter->Report( wxString::Format( _( "Approximated polygon self-intersection check failed\n"
                                                     "z: %g; bounding box: %s" ) ),
                                                  aZposition,
                                                  formatBBox( workingPoly.BBox() ) ),
                                RPT_SEVERITY_ERROR );
        }
        else
        {
            fallbackPoly = workingPoly;
            workingPoly = approximated;
        }*/
    }

#if 0   // No longer in use
    auto toPoint = [&]( const VECTOR2D& aKiCoords ) -> gp_Pnt
    {
        return gp_Pnt( pcbIUScale.IUTomm( aKiCoords.x - aOrigin.x ),
                       -pcbIUScale.IUTomm( aKiCoords.y - aOrigin.y ), aZposition );
    };
#endif

    gp_Pln basePlane( gp_Pnt( 0.0, 0.0, aZposition ),
                      std::signbit( aThickness ) ? -gp::DZ() : gp::DZ() );

    for( size_t polyId = 0; polyId < workingPoly.CPolygons().size(); polyId++ )
    {
        SHAPE_POLY_SET::POLYGON& polygon = workingPoly.Polygon( polyId );

        auto tryMakeWire = [this, &aZposition,
                            &aOrigin]( const SHAPE_LINE_CHAIN& aContour, bool aAllowRetry ) -> TopoDS_Wire
        {
            TopoDS_Wire      wire;
            BRepLib_MakeWire mkWire;

            makeWireFromChain( mkWire, aContour, m_mergeOCCMaxDist, aZposition, aOrigin, m_reporter );

            if( mkWire.IsDone() )
            {
                wire = mkWire.Wire();
            }
            else
            {
                m_reporter->Report(
                        wxString::Format( _( "Wire not done (contour points %d): OCC error %d\n"
                                             "z: %g; bounding box: %s" ),
                                          static_cast<int>( aContour.PointCount() ),
                                          static_cast<int>( mkWire.Error() ),
                                          formatBBox( aContour.BBox() ) ),
                        RPT_SEVERITY_WARNING );
            }

            if( !wire.IsNull() )
            {
                BRepAlgoAPI_Check check( wire, false, true );

                if( !check.IsValid() )
                {
                    m_reporter->Report( wxString::Format( _( "Wire self-interference check failed\n"
                                                             "z: %g; bounding box: %s" ),
                                                          aZposition,
                                                          formatBBox( aContour.BBox() ) ),
                                        RPT_SEVERITY_WARNING );

                    wire.Nullify();
                }
            }

            return wire;
        };

        BRepBuilderAPI_MakeFace mkFace;

        for( size_t contId = 0; contId < polygon.size(); contId++ )
        {
            try
            {
                // We allow retry when trying to convert polygon[contId] when a convert error
                // happens, using an equivalent polygon shape.
                bool allow_retry = aConvertToArcs ? true : false;

                TopoDS_Wire wire = tryMakeWire( polygon[contId], allow_retry );

                if( aConvertToArcs && wire.IsNull() )
                {
                    m_reporter->Report( wxString::Format( _( "Using non-simplified polygon." ) ),
                                        RPT_SEVERITY_DEBUG );

                    // Fall back to original shape. Do not allow retry
                    allow_retry = false;
                    wire = tryMakeWire( fallbackPoly.CPolygon( polyId )[contId], allow_retry );
                }

                if( contId == 0 ) // Outline
                {
                    if( !wire.IsNull() )
                    {
                        if( basePlane.Axis().Direction().Z() < 0 )
                            wire.Reverse();

                        mkFace = BRepBuilderAPI_MakeFace( basePlane, wire );
                    }
                    else
                    {
                        m_reporter->Report( wxString::Format( wxT( "** Outline skipped **\n"
                                                                   "z: %g; bounding box: %s" ),
                                                              aZposition,
                                                              formatBBox( polygon[contId].BBox() ) ),
                                            RPT_SEVERITY_DEBUG );
                        break;
                    }
                }
                else // Hole
                {
                    if( !wire.IsNull() )
                    {
                        if( basePlane.Axis().Direction().Z() > 0 )
                            wire.Reverse();

                        mkFace.Add( wire );
                    }
                    else
                    {
                        m_reporter->Report( wxString::Format( wxT( "** Hole skipped **\n"
                                                                   "z: %g; bounding box: %s" ),
                                                              aZposition,
                                                              formatBBox( polygon[contId].BBox() ) ),
                                            RPT_SEVERITY_DEBUG );
                    }
                }
            }
            catch( const Standard_Failure& e )
            {
                m_reporter->Report( wxString::Format( _( "OCC exception creating contour %d: %s" ),
                                                      static_cast<int>( contId ),
                                                      e.GetMessageString() ),
                                    RPT_SEVERITY_ERROR );
                return false;
            }
        }

        if( mkFace.IsDone() )
        {
            TopoDS_Shape faceShape = mkFace.Shape();

            if( aThickness != 0.0 )
            {
                TopoDS_Shape prism = BRepPrimAPI_MakePrism( faceShape, gp_Vec( 0, 0, aThickness ) );
                aShapes.push_back( prism );

                if( prism.IsNull() )
                {
                    m_reporter->Report( _( "Failed to create a prismatic shape" ), RPT_SEVERITY_ERROR );
                    return false;
                }
            }
            else
            {
                aShapes.push_back( faceShape );
            }
        }
        else
        {
            m_reporter->Report( _( "** Face skipped **" ), RPT_SEVERITY_DEBUG );
        }
    }

    return true;
}


// These colors are based on 3D viewer's colors and are different to "gbrjobColors"
static std::vector<FAB_LAYER_COLOR> s_soldermaskColors = {
    { NotSpecifiedPrm(), wxColor( 20, 51, 36 ) },        // Not specified, not in .gbrjob file
    { _HKI( "Green" ), wxColor( 20, 51, 36 ) },          // used in .gbrjob file
    { _HKI( "Red" ), wxColor( 181, 19, 21 ) },           // used in .gbrjob file
    { _HKI( "Blue" ), wxColor( 2, 59, 162 ) },           // used in .gbrjob file
    { _HKI( "Purple" ), wxColor( 32, 2, 53 ) },          // used in .gbrjob file
    { _HKI( "Black" ), wxColor( 11, 11, 11 ) },          // used in .gbrjob file
    { _HKI( "White" ), wxColor( 245, 245, 245 ) },       // used in .gbrjob file
    { _HKI( "Yellow" ), wxColor( 194, 195, 0 ) },        // used in .gbrjob file
    { _HKI( "User defined" ), wxColor( 128, 128, 128 ) } // Free; the name is a dummy name here
};


static bool colorFromStackup( BOARD_STACKUP_ITEM_TYPE aType, const wxString& aColorStr,
                              COLOR4D& aColorOut )
{
    if( !IsPrmSpecified( aColorStr ) )
        return false;

    if( aColorStr.StartsWith( wxT( "#" ) ) ) // User defined color
    {
        aColorOut = COLOR4D( aColorStr );
        return true;
    }
    else
    {
        const std::vector<FAB_LAYER_COLOR>& colors =
                ( aType == BS_ITEM_TYPE_SOLDERMASK || aType == BS_ITEM_TYPE_SILKSCREEN )
                        ? s_soldermaskColors
                        : GetStandardColors( aType );

        for( const FAB_LAYER_COLOR& fabColor : colors )
        {
            if( fabColor.GetName() == aColorStr )
            {
                aColorOut = fabColor.GetColor( aType );
                return true;
            }
        }
    }

    return false;
}


bool STEP_PCB_MODEL::CreatePCB( SHAPE_POLY_SET& aOutline, const VECTOR2D& aOrigin, bool aPushBoardBody )
{
    if( m_hasPCB )
    {
        if( !isBoardOutlineValid() )
            return false;

        return true;
    }

    thread_pool& tp = GetKiCadThreadPool();

    Handle( XCAFDoc_VisMaterialTool ) visMatTool = XCAFDoc_DocumentTool::VisMaterialTool( m_doc->Main() );

    m_hasPCB = true; // whether or not operations fail we note that CreatePCB has been invoked

    // Support for more than one main outline (more than one board)
    m_reporter->Report( wxString::Format( wxT( "Build board outlines (%d outlines) with %d points." ),
                                          aOutline.OutlineCount(),
                                          aOutline.FullPointCount() ),
                        RPT_SEVERITY_DEBUG );

    double boardThickness;
    double boardZPos;
    getBoardBodyZPlacement( boardZPos, boardThickness );

#if 1
    // This code should work, and it is working most of time
    // However there are issues if the main outline is a circle with holes:
    // holes from vias and pads are not working
    // see bug https://gitlab.com/kicad/code/kicad/-/issues/17446
    // (Holes are missing from STEP export with circular PCB outline)
    // Hard to say if the bug is in our code or in OCC 7.7
    if( !MakeShapes( m_board_outlines, aOutline, false, boardThickness, boardZPos, aOrigin ) )
    {
        // Error
        m_reporter->Report( _( "OCC error creating main outline." ), RPT_SEVERITY_ERROR );
    }
#else
    // Workaround for bug #17446 Holes are missing from STEP export with circular PCB outline
    for( const SHAPE_POLY_SET::POLYGON& polygon : aOutline.CPolygons() )
    {
        for( size_t contId = 0; contId < polygon.size(); contId++ )
        {
            const SHAPE_LINE_CHAIN& contour = polygon[contId];
            SHAPE_POLY_SET          polyset;
            polyset.Append( contour );

            if( contId == 0 ) // main Outline
            {
                if( !MakeShapes( m_board_outlines, polyset, false, boardThickness, boardZPos,
                                 aOrigin ) )
                {
                    m_reporter->Report( _( "OCC error creating main outline." ),
                                        RPT_SEVERITY_ERROR );
                }
            }
            else // Hole inside the main outline
            {
                if( !MakeShapes( m_boardCutouts, polyset, false, boardThickness, boardZPos,
                                 aOrigin ) )
                {
                    m_reporter->Report( _( "OCC error creating hole in main outline." ),
                                        RPT_SEVERITY_ERROR );
                }
            }
        }
    }
#endif

    // Even if we've disabled board body export, we still need the shapes for bounding box calculations.
    Bnd_Box brdBndBox;

    for( const TopoDS_Shape& brdShape : m_board_outlines )
        BRepBndLib::Add( brdShape, brdBndBox );

    // subtract cutouts (if any)
    m_reporter->Report( wxString::Format( wxT( "Build board cutouts and holes (%d hole(s))." ),
                                          (int) ( m_boardCutouts.size() + m_copperCutouts.size() ) ),
                        RPT_SEVERITY_DEBUG );

    auto buildBSB =
            [&brdBndBox]( std::vector<TopoDS_Shape>& input, Bnd_BoundSortBox& bsbHoles )
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

    auto subtractShapesMap =
            [&tp, this]( const wxString& aWhat, std::map<wxString, std::vector<TopoDS_Shape>>& aShapesMap,
                         std::vector<TopoDS_Shape>& aHolesList, Bnd_BoundSortBox& aBSBHoles )
            {
                m_reporter->Report( wxString::Format( _( "Subtracting holes for %s" ), aWhat ),
                                    RPT_SEVERITY_DEBUG );

                for( auto& [netname, vec] : aShapesMap )
                {
                    std::mutex mutex;

                    auto subtractLoopFn = [&]( const int shapeId )
                    {
                        TopoDS_Shape& shape = vec[shapeId];

                        Bnd_Box shapeBbox;
                        BRepBndLib::Add( shape, shapeBbox );

                        TopTools_ListOfShape holelist;

                        {
                            std::unique_lock lock( mutex );

                            const TColStd_ListOfInteger& indices = aBSBHoles.Compare( shapeBbox );

                            for( const Standard_Integer& index : indices )
                                holelist.Append( aHolesList[index] );
                        }

                        if( holelist.IsEmpty() )
                            return; // nothing to cut for this shape

                        TopTools_ListOfShape cutArgs;
                        cutArgs.Append( shape );

                        BRepAlgoAPI_Cut cut;

                        cut.SetRunParallel( true );
                        cut.SetToFillHistory( false );

                        cut.SetArguments( cutArgs );
                        cut.SetTools( holelist );
                        cut.Build();

                        if( cut.HasErrors() || cut.HasWarnings() )
                        {
                            m_reporter->Report( wxString::Format( _( "** Got problems while cutting "
                                                                        "%s net '%s' **" ),
                                                                    aWhat,
                                                                    UnescapeString( netname ) ),
                                                RPT_SEVERITY_WARNING );
                            shapeBbox.Dump();

                            if( cut.HasErrors() )
                            {
                                wxString             msg = _( "Errors:\n" );
                                wxStringOutputStream os_stream( &msg );
                                wxStdOutputStream    out( os_stream );

                                cut.DumpErrors( out );
                                m_reporter->Report( msg, RPT_SEVERITY_WARNING);
                            }

                            if( cut.HasWarnings() )
                            {
                                wxString             msg = _( "Warnings:\n" );
                                wxStringOutputStream os_stream( &msg );
                                wxStdOutputStream    out( os_stream );

                                cut.DumpWarnings( out );
                                m_reporter->Report( msg, RPT_SEVERITY_WARNING );
                            }
                        }

                        shape = cut.Shape();
                    };

                    tp.submit_loop( 0, vec.size(), subtractLoopFn ).wait();
                }
            };

    auto subtractShapes =
            [subtractShapesMap]( const wxString& aWhat, std::vector<TopoDS_Shape>& aShapesList,
                                 std::vector<TopoDS_Shape>& aHolesList, Bnd_BoundSortBox& aBSBHoles )
            {
                std::map<wxString, std::vector<TopoDS_Shape>> aShapesMap{ { wxEmptyString, aShapesList } };

                subtractShapesMap( aWhat, aShapesMap, aHolesList, aBSBHoles );
                aShapesList = aShapesMap[wxEmptyString];
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

        subtractShapesMap( _( "pads" ), m_board_copper_pads, m_copperCutouts, bsbHoles );
        subtractShapesMap( _( "vias" ), m_board_copper_vias, m_copperCutouts, bsbHoles );
    }

    if( m_fuseShapes )
    {
        std::map<wxString, TopTools_ListOfShape> shapesToFuseMap;

        auto addShapes = [&shapesToFuseMap]( const wxString&                  aNetname,
                                             const std::vector<TopoDS_Shape>& aShapes )
        {
            for( const TopoDS_Shape& shape : aShapes )
                shapesToFuseMap[aNetname].Append( shape );
        };

        for( const auto& [netname, shapes] : m_board_copper )
            addShapes( netname, shapes );

        for( const auto& [netname, shapes] : m_board_copper_pads )
            addShapes( netname, shapes );

        for( const auto& [netname, shapes] : m_board_copper_vias )
            addShapes( netname, shapes );

        m_reporter->Report( wxT( "Fusing shapes" ), RPT_SEVERITY_DEBUG );

        // Do fusing in parallel
        std::mutex mutex;

        auto fuseLoopFn = [&]( const wxString& aNetname )
        {
            auto&        toFuse = shapesToFuseMap[aNetname];
            TopoDS_Shape fusedShape = fuseShapesOrCompound( toFuse, m_reporter );

            if( !fusedShape.IsNull() )
            {
                std::unique_lock lock( mutex );

                m_board_copper_fused[aNetname].emplace_back( fusedShape );

                m_board_copper[aNetname].clear();
                m_board_copper_pads[aNetname].clear();
                m_board_copper_vias[aNetname].clear();
            }
        };

        BS::multi_future<void> mf;

        for( const auto& [netname, _] : shapesToFuseMap )
            mf.push_back( tp.submit_task( [&, netname]() { fuseLoopFn( netname ); } ) );

        mf.wait();
    }

    // push the board to the data structure
    m_reporter->Report( wxT( "Generate board full shape." ), RPT_SEVERITY_DEBUG );

    // AddComponent adds a label that has a reference (not a parent/child relation) to the real
    // label.  We need to extract that real label to name it for the STEP output cleanly
    // Why are we trying to name the bare board? Because CAD tools like SolidWorks do fun things
    // like "deduplicate" imported STEPs by swapping STEP assembly components with already
    // identically named assemblies.  So we want to avoid having the PCB be generally defaulted
    // to "Component" or "Assembly".

    // aCompoundNets will place all geometry within a net into one compound.
    // aCompoundAll will place all geometry into one compound.
    auto pushToAssemblyMap =
            [&]( const std::map<wxString, std::vector<TopoDS_Shape>>& aShapesMap,
                 const TDF_Label& aVisMatLabel, const wxString& aShapeName, bool aCompoundNets,
                 bool aCompoundAll, const wxString& aNiceName )
            {
                std::map<wxString, std::vector<TopoDS_Shape>> shapesMap;

                if( aCompoundAll )
                {
                    std::vector<TopoDS_Shape> allShapes;

                    for( const auto& [netname, shapesList] : aShapesMap )
                        allShapes.insert( allShapes.end(), shapesList.begin(), shapesList.end() );

                    if( !allShapes.empty() )
                        shapesMap[wxEmptyString].emplace_back( makeCompound( allShapes ) );
                }
                else
                {
                    shapesMap = aShapesMap;
                }

                for( const auto& [netname, shapesList] : shapesMap )
                {
                    std::vector<TopoDS_Shape> newList;

                    if( aCompoundNets )
                        newList.emplace_back( makeCompound( shapesList ) );
                    else
                        newList = shapesList;

                    int i = 1;

                    for( TopoDS_Shape& shape : newList )
                    {
                        Handle( TDataStd_TreeNode ) node;

                        // Dont expand the component or else coloring it gets hard
                        TDF_Label lbl = m_assy->AddComponent( m_assy_label, shape, false );
                        KICAD3D_INFO::Set( lbl, KICAD3D_MODEL_TYPE::BOARD, aNiceName.ToStdString() );
                        m_pcb_labels.push_back( lbl );

                        if( m_pcb_labels.back().IsNull() )
                            return;

                        lbl.FindAttribute( XCAFDoc::ShapeRefGUID(), node );
                        TDF_Label shpLbl = node->Father()->Label();

                        if( !shpLbl.IsNull() )
                        {
                            if( visMatTool && !aVisMatLabel.IsNull() )
                                visMatTool->SetShapeMaterial( shpLbl, aVisMatLabel );

                            wxString shapeName;

                            shapeName << m_pcbName;
                            shapeName << '_';
                            shapeName << aShapeName;

                            if( !netname.empty() )
                            {
                                shapeName << '_';
                                shapeName << netname;
                            }

                            if( newList.size() > 1 )
                            {
                                shapeName << '_';
                                shapeName << i;
                            }

                            TCollection_ExtendedString partname( shapeName.ToUTF8().data() );
                            TDataStd_Name::Set( shpLbl, partname );
                        }

                        i++;
                    }
                }
            };

    auto pushToAssembly =
            [&]( const std::vector<TopoDS_Shape>& aShapesList, const TDF_Label& aVisMatLabel,
                 const wxString& aShapeName, bool aCompound, const wxString& aNiceName )
            {
                const std::map<wxString, std::vector<TopoDS_Shape>> shapesMap{ { wxEmptyString, aShapesList } };

                pushToAssemblyMap( shapesMap, aVisMatLabel, aShapeName, aCompound, aCompound, aNiceName );
            };

    auto makeMaterial =
            [&]( const TCollection_AsciiString& aName, const Quantity_ColorRGBA& aBaseColor,
                 double aMetallic, double aRoughness ) -> TDF_Label
            {
                Handle( XCAFDoc_VisMaterial ) vismat = new XCAFDoc_VisMaterial;
                XCAFDoc_VisMaterialPBR pbr;
                pbr.BaseColor = aBaseColor;
                pbr.Metallic = aMetallic;
                pbr.Roughness = aRoughness;
                vismat->SetPbrMaterial( pbr );
                return visMatTool->AddMaterial( vismat, aName );
            };

    // Init colors for the board items
    Quantity_ColorRGBA copper_color( m_copperColor[0], m_copperColor[1], m_copperColor[2], 1.0 );
    Quantity_ColorRGBA pad_color( m_padColor[0], m_padColor[1], m_padColor[2], 1.0 );

    Quantity_ColorRGBA board_color( 0.42f, 0.45f, 0.29f, 0.98f );
    Quantity_ColorRGBA front_silk_color( 1.0f, 1.0f, 1.0f, 0.9f );
    Quantity_ColorRGBA back_silk_color = front_silk_color;
    Quantity_ColorRGBA front_mask_color( 0.08f, 0.2f, 0.14f, 0.83f );
    Quantity_ColorRGBA back_mask_color = front_mask_color;

    // Get colors from stackup
    for( const BOARD_STACKUP_ITEM* item : m_stackup.GetList() )
    {
        COLOR4D col;

        if( !colorFromStackup( item->GetType(), item->GetColor(), col ) )
            continue;

        if( item->GetBrdLayerId() == F_Mask || item->GetBrdLayerId() == B_Mask )
        {
            col.Darken( 0.2 );

            if( item->GetBrdLayerId() == F_Mask )
                front_mask_color.SetValues( col.r, col.g, col.b, col.a );
            else
                back_mask_color.SetValues( col.r, col.g, col.b, col.a );
        }

        if( item->GetBrdLayerId() == F_SilkS )
            front_silk_color.SetValues( col.r, col.g, col.b, col.a );
        else if( item->GetBrdLayerId() == B_SilkS )
            back_silk_color.SetValues( col.r, col.g, col.b, col.a );

        if( item->GetType() == BS_ITEM_TYPE_DIELECTRIC && item->GetTypeName() == KEY_CORE )
            board_color.SetValues( col.r, col.g, col.b, col.a );
    }

    // Paint board body in soldermask colors if soldermask is not exported as a layer
    if( !m_enabledLayers.Contains( F_Mask ) && !m_enabledLayers.Contains( B_Mask ) )
    {
        board_color = front_mask_color;
        board_color.SetAlpha( 1.0 );
    }

    TDF_Label front_mask_mat = makeMaterial( "soldermask", front_mask_color, 0.0, 0.6 );
    TDF_Label back_mask_mat = makeMaterial( "soldermask", back_mask_color, 0.0, 0.6 );
    TDF_Label front_silk_mat = makeMaterial( "silkscreen", front_silk_color, 0.0, 0.9 );
    TDF_Label back_silk_mat = makeMaterial( "silkscreen", back_silk_color, 0.0, 0.9 );
    TDF_Label copper_mat = makeMaterial( "copper", copper_color, 1.0, 0.4 );
    TDF_Label pad_mat = makeMaterial( "pad", pad_color, 1.0, 0.4 );
    TDF_Label board_mat = makeMaterial( "board", board_color, 0.0, 0.8 );

    pushToAssemblyMap( m_board_copper, copper_mat, "copper", true, true, "Copper" );
    pushToAssemblyMap( m_board_copper_pads, pad_mat, "pad", true, true, "Pads" );
    pushToAssemblyMap( m_board_copper_vias, copper_mat, "via", true, true, "Via" );
    pushToAssemblyMap( m_board_copper_fused, copper_mat, "copper", true, true, "Copper" );
    pushToAssembly( m_board_front_silk, front_silk_mat, "silkscreen", true, "Top Silkscreen" );
    pushToAssembly( m_board_back_silk, back_silk_mat, "silkscreen", true, "Bottom Silkscreen" );
    pushToAssembly( m_board_front_mask, front_mask_mat, "soldermask", true, "Top Soldermask" );
    pushToAssembly( m_board_back_mask, back_mask_mat, "soldermask", true, "Bottom Soldermask" );

    if( aPushBoardBody )
        pushToAssembly( m_board_outlines, board_mat, "PCB", false, "Body" );

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
        m_reporter->Report( wxString::Format( _( "No valid PCB assembly; cannot create output file '%s'." ),
                                              aFileName ),
                            RPT_SEVERITY_ERROR );
        return false;
    }

    m_outFmt = OUTPUT_FORMAT::FMT_OUT_IGES;

    wxFileName fn( aFileName );
    IGESControl_Controller::Init();
    IGESCAFControl_Writer writer;
    writer.SetColorMode( Standard_True );
    writer.SetNameMode( Standard_True );
    IGESData_GlobalSection header = writer.Model()->GlobalSection();
    header.SetFileName( new TCollection_HAsciiString( fn.GetFullName().ToAscii() ) );
    header.SetSendName( new TCollection_HAsciiString( "KiCad electronic assembly" ) );
    header.SetAuthorName( new TCollection_HAsciiString( Interface_Static::CVal( "write.iges.header.author" ) ) );
    header.SetCompanyName( new TCollection_HAsciiString( Interface_Static::CVal( "write.iges.header.company" ) ) );
    writer.Model()->SetGlobalSection( header );

    if( Standard_False == writer.Perform( m_doc, aFileName.c_str() ) )
        return false;

    return true;
}
#endif

bool STEP_PCB_MODEL::CompressSTEP( wxString& inputFile, wxString& outputFile )
{
    wxFileInputStream  input( inputFile );
    wxFileOutputStream output( outputFile );

    if( !input.IsOk() )
    {
        m_reporter->Report( wxString::Format( _( "Cannot create input stream '%s'.\n" ), inputFile ) );
        return false;
    }

    if( !output.IsOk() )
    {
        m_reporter->Report( wxString::Format( _( "Cannot create output stream '%s'.\n" ), outputFile ) );
        return false;
    }

    wxZlibOutputStream zlibStream( output, -1, wxZLIB_GZIP );

    if( !zlibStream.IsOk() )
    {
        m_reporter->Report( _( "Impossible create compress stream" ) );
        return false;
    }

    input.Read( zlibStream );

    if( input.LastRead() == 0 || zlibStream.LastWrite() == 0 )
    {
        m_reporter->Report( _( "Compress read or write error" ) );
        return false;
    }

    zlibStream.Close();
    output.Close();

    return true;
}

bool STEP_PCB_MODEL::WriteSTEP( const wxString& aFileName, bool aOptimize, bool compress )
{
    if( !isBoardOutlineValid() )
    {
        m_reporter->Report( wxString::Format( _( "No valid PCB assembly; cannot create output file '%s'." ),
                                              aFileName ),
                            RPT_SEVERITY_ERROR );
        return false;
    }

    m_outFmt = OUTPUT_FORMAT::FMT_OUT_STEP;

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
    {
        m_reporter->Report( _( "Failed to set STEP product name, but will attempt to continue." ),
                            RPT_SEVERITY_WARNING );
    }

    // Setting write.surfacecurve.mode to 0 reduces file size and write/read times.
    // But there are reports that this mode might be less compatible in some cases.
    if( !Interface_Static::SetIVal( "write.surfacecurve.mode", aOptimize ? 0 : 1 ) )
    {
        m_reporter->Report( _( "Failed to set surface curve mode, but will attempt to continue." ),
                            RPT_SEVERITY_WARNING );
    }

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

    wxString tmpfname( "$tempfile$.step" );

    if( Standard_False == writer.Write( tmpfname.c_str() ) )
        success = false;

    if( compress && success )
    {
        wxString srcTmp( tmpfname );
        wxString dstTmp( "$tempfile$.stpz" );

        success = STEP_PCB_MODEL::CompressSTEP( srcTmp, dstTmp );
        wxRemoveFile( srcTmp );

        tmpfname = dstTmp;
    }

    if( success )
    {

        // Preserve the permissions of the current file
        KIPLATFORM::IO::DuplicatePermissions( fn.GetFullPath(), tmpfname.c_str() );

        if( !wxRenameFile( tmpfname, fn.GetFullName(), true ) )
        {
            m_reporter->Report( wxString::Format( _( "Cannot rename temporary file '%s' to '%s'." ),
                                                  tmpfname,
                                                  fn.GetFullName() ),
                                RPT_SEVERITY_ERROR );
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
        m_reporter->Report( wxString::Format( _( "No valid PCB assembly; cannot create output file '%s'." ),
                                              aFileName ),
                            RPT_SEVERITY_ERROR );
        return false;
    }

    m_outFmt = OUTPUT_FORMAT::FMT_OUT_BREP;

    // s_assy = shape tool for the source
    Handle( XCAFDoc_ShapeTool ) s_assy = XCAFDoc_DocumentTool::ShapeTool( m_doc->Main() );

    // retrieve assembly as a single shape
    TopoDS_Shape shape = getOneShape( s_assy );

    wxFileName fn( aFileName );

    wxFFileOutputStream ffStream( fn.GetFullPath() );
    wxStdOutputStream   stdStream( ffStream );

#if OCC_VERSION_HEX >= 0x070600
    BRepTools::Write( shape, stdStream, false, false, TopTools_FormatVersion_VERSION_1 );
#else
    BRepTools::Write( shape, stdStream );
#endif

    return true;
}


bool STEP_PCB_MODEL::WriteXAO( const wxString& aFileName )
{
    wxFileName fn( aFileName );

    wxFFileOutputStream ffStream( fn.GetFullPath() );
    wxStdOutputStream   file( ffStream );

    if( !ffStream.IsOk() )
    {
        m_reporter->Report( wxString::Format( "Could not open file '%s'", fn.GetFullPath() ),
                            RPT_SEVERITY_ERROR );
        return false;
    }

    m_outFmt = OUTPUT_FORMAT::FMT_OUT_XAO;

    // s_assy = shape tool for the source
    Handle( XCAFDoc_ShapeTool ) s_assy = XCAFDoc_DocumentTool::ShapeTool( m_doc->Main() );

    // retrieve assembly as a single shape
    const TopoDS_Shape shape = getOneShape( s_assy );

    std::map<wxString, std::vector<int>> groups[4];
    std::map<wxString, double>           groupAreas;
    TopExp_Explorer                      exp;
    int                                  faceIndex = 0;

    for( exp.Init( shape, TopAbs_FACE ); exp.More(); exp.Next() )
    {
        TopoDS_Shape subShape = exp.Current();

        Bnd_Box bbox;
        BRepBndLib::Add( subShape, bbox );

        for( const auto& [padKey, pairs] : m_pad_points )
        {
            for( const auto& pair : pairs )
            {
                const auto& [point, padTestShape] = pair;

                if( bbox.IsOut( point ) )
                    continue;

                BRepAdaptor_Surface surface( TopoDS::Face( subShape ) );

                if( surface.GetType() != GeomAbs_Plane )
                    continue;

                BRepExtrema_DistShapeShape dist( padTestShape, subShape );
                dist.Perform();

                if( !dist.IsDone() )
                    continue;

                if( dist.Value() < Precision::Approximation() )
                {
                    // Push as a face group
                    groups[2][padKey].push_back( faceIndex );

                    GProp_GProps system;
                    BRepGProp::SurfaceProperties( subShape, system );

                    double surfaceArea = system.Mass() / 1e6; // Convert to meters^2
                    groupAreas[padKey] += surfaceArea;
                }
            }
        }

        faceIndex++;
    }

    // Based on Gmsh code
    file << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>" << std::endl;
    file << "<XAO version=\"1.0\" author=\"KiCad\">" << std::endl;
    file << "  <geometry name=\"" << fn.GetName() << "\">" << std::endl;
    file << "    <shape format=\"BREP\"><![CDATA[";
#if OCC_VERSION_HEX < 0x070600
    BRepTools::Write( shape, file );
#else
    BRepTools::Write( shape, file, Standard_True, Standard_True, TopTools_FormatVersion_VERSION_1 );
#endif
    file << "]]></shape>" << std::endl;
    file << "    <topology>" << std::endl;

    TopTools_IndexedMapOfShape mainMap;
    TopExp::MapShapes( shape, mainMap );
    std::set<int> topo[4];

    static const TopAbs_ShapeEnum c_dimShapeTypes[] = { TopAbs_VERTEX, TopAbs_EDGE, TopAbs_FACE,
                                                        TopAbs_SOLID };

    static const std::string c_dimLabel[] = { "vertex", "edge", "face", "solid" };
    static const std::string c_dimLabels[] = { "vertices", "edges", "faces", "solids" };

    for( int dim = 0; dim < 4; dim++ )
    {
        for( exp.Init( shape, c_dimShapeTypes[dim] ); exp.More(); exp.Next() )
        {
            TopoDS_Shape subShape = exp.Current();
            int          idx = mainMap.FindIndex( subShape );

            if( idx && !topo[dim].count( idx ) )
                topo[dim].insert( idx );
        }
    }

    for( int dim = 0; dim <= 3; dim++ )
    {
        std::string labels = c_dimLabels[dim];
        std::string label = c_dimLabel[dim];

        file << "      <" << labels << " count=\"" << topo[dim].size() << "\">" << std::endl;
        int index = 0;

        for( auto p : topo[dim] )
        {
            std::string name( "" );
            file << "        <" << label << " index=\"" << index << "\" "
                 << "name=\"" << name << "\" "
                 << "reference=\"" << p << "\"/>" << std::endl;

            index++;
        }
        file << "      </" << labels << ">" << std::endl;
    }

    file << "    </topology>" << std::endl;
    file << "  </geometry>" << std::endl;
    file << "  <groups count=\""
         << groups[0].size() + groups[1].size() + groups[2].size() + groups[3].size() << "\">"
         << std::endl;

    int groupNumber = 1;

    m_reporter->Report( wxT( "Pad definitions:" ), RPT_SEVERITY_DEBUG );
    m_reporter->Report( wxT( "Number\tName\tArea (m^2)" ), RPT_SEVERITY_DEBUG );

    for( int dim = 0; dim <= 3; dim++ )
    {
        std::string label = c_dimLabel[dim];

        for( auto g : groups[dim] )
        {
            //std::string name = model->getPhysicalName( dim, g.first );
            wxString name = g.first;

            if( name.empty() )
            { // create same unique name as for MED export
                std::ostringstream gs;
                gs << "G_" << dim << "D_" << g.first;
                name = gs.str();
            }
            file << "    <group name=\"" << name << "\" dimension=\"" << label;
//#if 1
//            // Gmsh XAO extension: also save the physical tag, so that XAO can be used
//            // to serialize OCC geometries, ready to be used by GetDP, GmshFEM & co
//            file << "\" tag=\"" << g.first;
//#endif
            file << "\" count=\"" << g.second.size() << "\">" << std::endl;

            for( auto index : g.second )
                file << "      <element index=\"" << index << "\"/>" << std::endl;

            file << "    </group>" << std::endl;

            m_reporter->Report( wxString::Format( "%d\t%s\t%g",
                                                  groupNumber,
                                                  name,
                                                  groupAreas[name] ),
                                RPT_SEVERITY_DEBUG );

            groupNumber++;
        }
    }

    m_reporter->Report( wxT( "" ), RPT_SEVERITY_DEBUG );

    file << "  </groups>" << std::endl;
    file << "  <fields count=\"0\"/>" << std::endl;
    file << "</XAO>" << std::endl;

    return true;
}


bool STEP_PCB_MODEL::getModelLabel( const wxString& aBaseName, const wxString& aFileName,
                                    const std::vector<wxString>& aAltFilenames, VECTOR3D aScale,
                                    TDF_Label& aLabel, bool aSubstituteModels,
                                    wxString* aErrorMessage )
{
    std::string fileNameUTF8 = aFileName.utf8_string();

    std::string model_key = fileNameUTF8 + "_" + std::to_string( aScale.x ) + "_"
                            + std::to_string( aScale.y ) + "_" + std::to_string( aScale.z );

    MODEL_MAP::const_iterator mm = m_models.find( model_key );

    if( mm != m_models.end() )
    {
        aLabel = mm->second;
        return true;
    }

    aLabel.Nullify();

    Handle( TDocStd_Document )  doc;
    m_app->NewDocument( "MDTV-XCAF", doc );

    MODEL3D_FORMAT_TYPE modelFmt = fileType( fileNameUTF8.c_str() );
    TCollection_ExtendedString partname( aBaseName.utf8_str() );

    switch( modelFmt )
    {
    case FMT_IGES:
        if( !readIGES( doc, fileNameUTF8.c_str() ) )
        {
            m_reporter->Report( wxString::Format( wxT( "readIGES() failed on filename '%s'." ), aFileName ),
                                RPT_SEVERITY_ERROR );
            return false;
        }

        break;

    case FMT_STEP:
        if( !readSTEP( doc, fileNameUTF8.c_str() ) )
        {
            m_reporter->Report( wxString::Format( wxT( "readSTEP() failed on filename '%s'." ), aFileName ),
                                RPT_SEVERITY_ERROR );
            return false;
        }

        break;

    case FMT_STEPZ:
    {
        // To export a compressed step file (.stpz or .stp.gz file), the best way is to
        // decaompress it in a temporaty file and load this temporary file
        wxFFileInputStream ifile( aFileName );
        wxFileName         outFile( aFileName );

        outFile.SetPath( wxStandardPaths::Get().GetTempDir() );
        outFile.SetExt( wxT( "step" ) );
        wxFileOffset size = ifile.GetLength();

        if( size == wxInvalidOffset )
        {
            m_reporter->Report( wxString::Format( wxT( "getModelLabel() failed on filename '%s'." ),
                                                  aFileName ),
                                RPT_SEVERITY_ERROR );
            return false;
        }

        {
            bool success = false;

            {
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
                    m_reporter->Report(
                            wxString::Format( wxT( "failed to decompress '%s'." ), aFileName ),
                            RPT_SEVERITY_ERROR );
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
            }

            if( success )
            {
                success = getModelLabel( aBaseName, outFile.GetFullPath(), aAltFilenames,
                                         VECTOR3D( 1.0, 1.0, 1.0 ), aLabel, false );
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
            wxFileName wrlName( aFileName );

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

            for( const auto& altExt : alts )
            {
                wxFileName altFile;

                if( !aAltFilenames.empty() )
                {
                    for( const wxString& altPath : aAltFilenames )
                    {
                        wxFileName iterFn( altPath );

                        if( iterFn.GetExt() == altExt )
                        {
                            altFile = iterFn;
                            break;
                        }
                    }
                }
                else
                {
                    altFile = wxFileName( basePath, baseName + wxT( "." ) + altExt );
                }

                if( altFile.IsOk() && altFile.FileExists() )
                {
                    // When substituting a STEP/IGS file for VRML, do not apply the VRML scaling
                    // to the new STEP model.  This process of auto-substitution is janky as all
                    // heck so let's not mix up un-displayed scale factors with potentially
                    // mis-matched files.  And hope that the user doesn't have multiples files
                    // named "model.wrl" and "model.stp" referring to different parts.
                    // TODO: Fix model handling in v7.  Default models should only be STP.
                    //       Have option to override this in DISPLAY.
                    if( getModelLabel( aBaseName, altFile.GetFullPath(), {},
                                       VECTOR3D( 1.0, 1.0, 1.0 ), aLabel, false ) )
                    {
                        return true;
                    }
                }
            }
        }

        // VRML models only work when exporting to mesh formats
        // Also OCCT < 7.9.0 fails to load most VRML 2.0 models because of Switch nodes
        if( m_outFmt == OUTPUT_FORMAT::FMT_OUT_GLTF || m_outFmt == OUTPUT_FORMAT::FMT_OUT_STL
            || m_outFmt == OUTPUT_FORMAT::FMT_OUT_PLY  || m_outFmt == OUTPUT_FORMAT::FMT_OUT_U3D
            || m_outFmt == OUTPUT_FORMAT::FMT_OUT_PDF )
        {
            if( readVRML( doc, fileNameUTF8.c_str() ) )
            {
                Handle( XCAFDoc_ShapeTool ) shapeTool =
                        XCAFDoc_DocumentTool::ShapeTool( doc->Main() );

                prefixNames( shapeTool->Label(), partname );
            }
            else
            {
                m_reporter->Report(
                        wxString::Format( wxT( "readVRML() failed on filename '%s'." ),
                                            aFileName ),
                        RPT_SEVERITY_ERROR );

                return false;
            }
        }
        else // Substitution is not allowed
        {
            if( aErrorMessage )
                aErrorMessage->Printf( _( "Cannot use VRML models when exporting to non-mesh formats." ) );

            return false;
        }

        break;

        // TODO: implement IDF and EMN converters

    default:
        m_reporter->Report( wxString::Format( _( "Cannot identify actual file type for '%s'." ), aFileName ),
                            RPT_SEVERITY_ERROR );
        return false;
    }

    aLabel = transferModel( doc, m_doc, aScale );

    if( aLabel.IsNull() )
    {
        m_reporter->Report( wxString::Format( _( "Could not transfer model data from file '%s'." ), aFileName ),
                            RPT_SEVERITY_ERROR );
        return false;
    }

    // attach the PART NAME ( base filename: note that in principle
    // different models may have the same base filename )
    TDataStd_Name::Set( aLabel, partname );

    m_models.insert( MODEL_DATUM( model_key, aLabel ) );
    ++m_components;
    return true;
}


bool STEP_PCB_MODEL::getModelLocation( bool aBottom, const VECTOR2D& aPosition, double aRotation,
                                       const VECTOR3D& aOffset, const VECTOR3D& aOrientation,
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
    VECTOR3D offset( aOffset );
    offset.z += BOARD_OFFSET;

    double boardThickness;
    double boardZPos;
    getBoardBodyZPlacement( boardZPos, boardThickness );
    double top = std::max( boardZPos, boardZPos + boardThickness );
    double bottom = std::min( boardZPos, boardZPos + boardThickness );

    // 3D step models are placed on the top of copper layers.
    // This is true for SMD shapes, and perhaps not always true for TH shapes,
    // but we use this Z position for any 3D shape.
    double f_pos, f_thickness;
    getLayerZPlacement( F_Cu, f_pos, f_thickness );
    top += f_thickness;
    getLayerZPlacement( B_Cu, f_pos, f_thickness );
    bottom += f_thickness;      // f_thickness is < 0 for B_Cu layer

    gp_Trsf lRot;

    if( aBottom )
    {
        offset.z -= bottom;
        lRot.SetRotation( gp_Ax1( gp_Pnt( 0.0, 0.0, 0.0 ), gp_Dir( 0.0, 0.0, 1.0 ) ), aRotation );
        lPos.Multiply( lRot );
        lRot.SetRotation( gp_Ax1( gp_Pnt( 0.0, 0.0, 0.0 ), gp_Dir( 1.0, 0.0, 0.0 ) ), M_PI );
        lPos.Multiply( lRot );
    }
    else
    {
        offset.z += top;
        lRot.SetRotation( gp_Ax1( gp_Pnt( 0.0, 0.0, 0.0 ), gp_Dir( 0.0, 0.0, 1.0 ) ), aRotation );
        lPos.Multiply( lRot );
    }

    gp_Trsf lOff;
    lOff.SetTranslation( gp_Vec( offset.x, offset.y, offset.z ) );
    lPos.Multiply( lOff );

    gp_Trsf lOrient;
    lOrient.SetRotation( gp_Ax1( gp_Pnt( 0.0, 0.0, 0.0 ), gp_Dir( 0.0, 0.0, 1.0 ) ), -aOrientation.z );
    lPos.Multiply( lOrient );
    lOrient.SetRotation( gp_Ax1( gp_Pnt( 0.0, 0.0, 0.0 ), gp_Dir( 0.0, 1.0, 0.0 ) ), -aOrientation.y );
    lPos.Multiply( lOrient );
    lOrient.SetRotation( gp_Ax1( gp_Pnt( 0.0, 0.0, 0.0 ), gp_Dir( 1.0, 0.0, 0.0 ) ), -aOrientation.x );
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


bool STEP_PCB_MODEL::readVRML( Handle( TDocStd_Document ) & doc, const char* fname )
{
#if OCC_VERSION_HEX >= 0x070700
    VrmlAPI_CafReader                reader;
    RWMesh_CoordinateSystemConverter conv;
    conv.SetInputLengthUnit( 2.54 );
    reader.SetCoordinateSystemConverter( conv );
    reader.SetDocument( doc );

    if( !reader.Perform( TCollection_AsciiString( fname ), Message_ProgressRange() ) )
        return false;

    return true;
#else
    return false;
#endif
}


void STEP_PCB_MODEL::transferColors( Handle( XCAFDoc_ShapeTool )& aSrcShapeTool,
                                     Handle( XCAFDoc_ColorTool )& aSrcColorTool,
                                     Handle( XCAFDoc_ShapeTool )& aDstShapeTool,
                                     Handle( XCAFDoc_ColorTool )& aDstColorTool )
{
    // Get all shapes from the source document
    TDF_LabelSequence srcLabels;
    aSrcShapeTool->GetShapes( srcLabels );

    for( Standard_Integer i = 1; i <= srcLabels.Length(); i++ )
    {
        TDF_Label srcLabel = srcLabels.Value( i );
        TopoDS_Shape srcShape = aSrcShapeTool->GetShape( srcLabel );

        if( srcShape.IsNull() )
            continue;

        // Try to find the same shape in the destination document
        TDF_Label dstLabel;

        if( !aDstShapeTool->Search( srcShape, dstLabel, Standard_True, Standard_True, Standard_False ) )
            continue;

        // Transfer surface color
        Quantity_ColorRGBA surfColor;

        if( aSrcColorTool->GetColor( srcLabel, XCAFDoc_ColorSurf, surfColor ) )
            aDstColorTool->SetColor( dstLabel, surfColor, XCAFDoc_ColorSurf );

        // Transfer curve color
        Quantity_ColorRGBA curvColor;

        if( aSrcColorTool->GetColor( srcLabel, XCAFDoc_ColorCurv, curvColor ) )
            aDstColorTool->SetColor( dstLabel, curvColor, XCAFDoc_ColorCurv );

        // Transfer generic color
        Quantity_ColorRGBA genColor;

        if( aSrcColorTool->GetColor( srcLabel, XCAFDoc_ColorGen, genColor ) )
            aDstColorTool->SetColor( dstLabel, genColor, XCAFDoc_ColorGen );

        // Also check for colors on individual faces
        if( aSrcShapeTool->IsSimpleShape( srcLabel ) )
        {
            TopoDS_Shape shape = aSrcShapeTool->GetShape( srcLabel );

            for( TopExp_Explorer exp( shape, TopAbs_FACE ); exp.More(); exp.Next() )
            {
                TopoDS_Face face = TopoDS::Face( exp.Current() );
                Quantity_ColorRGBA faceColor;

                if( aSrcColorTool->GetColor( face, XCAFDoc_ColorSurf, faceColor ) )
                    aDstColorTool->SetColor( face, faceColor, XCAFDoc_ColorSurf );
                else if( aSrcColorTool->GetColor( face, XCAFDoc_ColorGen, faceColor ) )
                    aDstColorTool->SetColor( face, faceColor, XCAFDoc_ColorGen );
            }
        }
    }

    // Also iterate through subshapes and components recursively
    TDF_LabelSequence srcFreeShapes;
    aSrcShapeTool->GetFreeShapes( srcFreeShapes );

    std::function<void( const TDF_Label& )> transferColorsRecursive = [&]( const TDF_Label& aLabel )
    {
        TopoDS_Shape shape = aSrcShapeTool->GetShape( aLabel );

        if( shape.IsNull() )
            return;

        // Find this shape in destination
        TDF_Label dstLabel;

        if( aDstShapeTool->Search( shape, dstLabel, Standard_True, Standard_True, Standard_False ) )
        {
            Quantity_ColorRGBA color;

            if( aSrcColorTool->GetColor( aLabel, XCAFDoc_ColorSurf, color ) )
                aDstColorTool->SetColor( dstLabel, color, XCAFDoc_ColorSurf );

            if( aSrcColorTool->GetColor( aLabel, XCAFDoc_ColorCurv, color ) )
                aDstColorTool->SetColor( dstLabel, color, XCAFDoc_ColorCurv );

            if( aSrcColorTool->GetColor( aLabel, XCAFDoc_ColorGen, color ) )
                aDstColorTool->SetColor( dstLabel, color, XCAFDoc_ColorGen );
        }

        // Process children
        for( TDF_ChildIterator it( aLabel ); it.More(); it.Next() )
            transferColorsRecursive( it.Value() );

        // Process components if this is an assembly
        if( aSrcShapeTool->IsAssembly( aLabel ) )
        {
            TDF_LabelSequence components;
            aSrcShapeTool->GetComponents( aLabel, components );

            for( Standard_Integer j = 1; j <= components.Length(); j++ )
            {
                TDF_Label compLabel = components.Value( j );
                TDF_Label refLabel;

                if( aSrcShapeTool->GetReferredShape( compLabel, refLabel ) )
                    transferColorsRecursive( refLabel );
            }
        }
    };

    for( Standard_Integer i = 1; i <= srcFreeShapes.Length(); i++ )
        transferColorsRecursive( srcFreeShapes.Value( i ) );
}


TDF_Label STEP_PCB_MODEL::transferModel( Handle( TDocStd_Document ) & source,
                                         Handle( TDocStd_Document ) & dest, const VECTOR3D& aScale )
{
    // transfer data from Source into a top level component of Dest
    // s_assy = shape tool for the source
    Handle( XCAFDoc_ShapeTool ) s_assy = XCAFDoc_DocumentTool::ShapeTool( source->Main() );
    Handle( XCAFDoc_ColorTool ) s_color = XCAFDoc_DocumentTool::ColorTool( source->Main() );

    // retrieve all free shapes within the assembly
    TDF_LabelSequence frshapes;
    s_assy->GetFreeShapes( frshapes );

    // d_assy = shape tool for the destination
    Handle( XCAFDoc_ShapeTool ) d_assy = XCAFDoc_DocumentTool::ShapeTool( dest->Main() );
    Handle( XCAFDoc_ColorTool ) d_color = XCAFDoc_DocumentTool::ColorTool( dest->Main() );

    // create a new shape within the destination and set the assembly tool to point to it
    TDF_Label d_targetLabel = d_assy->NewShape();

    auto copyLabel = [&]( TDF_Label& d_label, const TDF_Label& s_label ) -> bool
    {
        // TDocStd_XLinkTool::Copy requires the source to be "self-contained", meaning it has
        // no external references. Some STEP files (e.g. from Fusion 360 with linked components)
        // may contain internal references that violate this constraint. In such cases, we fall
        // back to extracting just the geometric shape without the full XDE document structure.
        if( TDF_Tool::IsSelfContained( s_label ) )
        {
            TDocStd_XLinkTool link;
            link.Copy( d_label, s_label );
            return true;
        }
        else
        {
            // The source label is not self-contained. Extract the shape directly.
            TopoDS_Shape shape = s_assy->GetShape( s_label );

            if( shape.IsNull() )
                return false;

            // Add the shape directly without the XDE structure. This loses some metadata
            // like colors and names, but allows the model to be successfully transferred.
            d_assy->SetShape( d_label, shape );

            m_reporter->Report( wxT( "Model contains non-self-contained data; some metadata may be lost." ),
                                RPT_SEVERITY_INFO );
            return true;
        }
    };

    if( frshapes.Size() == 1 )
    {
        if( !copyLabel( d_targetLabel, frshapes.First() ) )
        {
            m_reporter->Report( wxT( "Failed to transfer model." ), RPT_SEVERITY_ERROR );
            return TDF_Label();
        }
    }
    else
    {
        // Rare case with multiple free shapes
        for( TDF_Label& s_shapeLabel : frshapes )
        {
            TDF_Label d_component = d_assy->NewShape();

            if( !copyLabel( d_component, s_shapeLabel ) )
            {
                m_reporter->Report( wxT( "Failed to transfer model component." ), RPT_SEVERITY_ERROR );
                return TDF_Label();
            }

            d_assy->AddComponent( d_targetLabel, d_component, TopLoc_Location() );
        }
    }

    // Transfer colors from source to destination document
    // This is necessary because TDocStd_XLinkTool::Copy may not properly transfer
    // color associations which are stored separately in the ColorTool section
    transferColors( s_assy, s_color, d_assy, d_color );

    if( aScale.x != 1.0 || aScale.y != 1.0 || aScale.z != 1.0 )
        rescaleShapes( d_targetLabel, gp_XYZ( aScale.x, aScale.y, aScale.z ) );

    return d_targetLabel;
}


bool STEP_PCB_MODEL::performMeshing( Handle( XCAFDoc_ShapeTool ) & aShapeTool )
{
    TDF_LabelSequence freeShapes;
    aShapeTool->GetFreeShapes( freeShapes );

    m_reporter->Report( wxT( "Meshing model" ), RPT_SEVERITY_DEBUG );

    // GLTF is a mesh format, we have to trigger opencascade to mesh the shapes we composited into the asesmbly
    // To mesh models, lets just grab the free shape root and execute on them
    for( Standard_Integer i = 1; i <= freeShapes.Length(); ++i )
    {
        TDF_Label    label = freeShapes.Value( i );
        TopoDS_Shape shape;
        aShapeTool->GetShape( label, shape );

        // These deflection values basically affect the accuracy of the mesh generated, a tighter
        // deflection will result in larger meshes
        // We could make this a tunable parameter, but for now fix it
        const Standard_Real      linearDeflection = 0.14;
        const Standard_Real      angularDeflection = DEG2RAD( 30.0 );
        BRepMesh_IncrementalMesh mesh( shape, linearDeflection, Standard_False, angularDeflection,
                                       Standard_True );
    }

    return true;
}


bool STEP_PCB_MODEL::WriteGLTF( const wxString& aFileName )
{
    /*if( !isBoardOutlineValid() )
    {
        m_reporter->Report( wxString::Format( _( "No valid PCB assembly; cannot create output file '%s'." ),
                                              aFileName ),
                            RPT_SEVERITY_ERROR );
        return false;
    }*/

    m_outFmt = OUTPUT_FORMAT::FMT_OUT_GLTF;

    performMeshing( m_assy );

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
            m_reporter->Report( wxString::Format( _( "Cannot rename temporary file '%s' to '%s'." ),
                                                  tmpGltfname,
                                                  fn.GetFullName() ),
                                RPT_SEVERITY_ERROR );
            success = false;
        }
    }

    wxSetWorkingDirectory( currCWD );

    return success;
}


bool STEP_PCB_MODEL::WritePLY( const wxString& aFileName )
{
#if OCC_VERSION_HEX < 0x070700
    m_reporter->Report( wxT( "PLY export is not supported before OCCT 7.7.0" ), RPT_SEVERITY_ERROR );
    return false;
#else

    if( !isBoardOutlineValid() )
    {
        m_reporter->Report( wxString::Format( _( "No valid PCB assembly; cannot create output file '%s'." ),
                                              aFileName ),
                            RPT_SEVERITY_ERROR );
        return false;
    }

    m_outFmt = OUTPUT_FORMAT::FMT_OUT_PLY;

    performMeshing( m_assy );

    wxFileName fn( aFileName );

    const char*     tmpFname = "$tempfile$.ply";
    RWPly_CafWriter cafWriter( tmpFname );

    cafWriter.SetFaceId( true ); // TODO: configurable SetPartId/SetFaceId
    cafWriter.ChangeCoordinateSystemConverter().SetInputLengthUnit( 0.001 );
    cafWriter.ChangeCoordinateSystemConverter().SetInputCoordinateSystem( RWMesh_CoordinateSystem_Zup );

    TColStd_IndexedDataMapOfStringString metadata;

    metadata.Add( TCollection_AsciiString( "pcb_name" ),
                  TCollection_ExtendedString( fn.GetName().wc_str() ) );
    metadata.Add( TCollection_AsciiString( "source_pcb_file" ),
                  TCollection_ExtendedString( fn.GetFullName().wc_str() ) );
    metadata.Add( TCollection_AsciiString( "generator" ),
                  TCollection_AsciiString( wxString::Format( wxS( "KiCad %s" ),
                                                             GetSemanticVersion() ).ToAscii() ) );
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
        KIPLATFORM::IO::DuplicatePermissions( fn.GetFullPath(), tmpFname );

        if( !wxRenameFile( tmpFname, fn.GetFullName(), true ) )
        {
            m_reporter->Report( wxString::Format( _( "Cannot rename temporary file '%s' to '%s'." ),
                                                  tmpFname,
                                                  fn.GetFullName() ),
                                RPT_SEVERITY_ERROR );
            success = false;
        }
    }

    wxSetWorkingDirectory( currCWD );

    return success;
#endif
}


bool STEP_PCB_MODEL::WriteSTL( const wxString& aFileName )
{
    if( !isBoardOutlineValid() )
    {
        m_reporter->Report( wxString::Format( _( "No valid PCB assembly; cannot create output file '%s'." ),
                                              aFileName ),
                            RPT_SEVERITY_ERROR );
        return false;
    }

    m_outFmt = OUTPUT_FORMAT::FMT_OUT_STL;

    performMeshing( m_assy );

    wxFileName fn( aFileName );

    const char* tmpFname = "$tempfile$.stl";

    // Creates a temporary file with a ascii7 name, because writer does not know unicode filenames.
    wxString currCWD = wxGetCwd();
    wxString workCWD = fn.GetPath();

    if( !workCWD.IsEmpty() )
        wxSetWorkingDirectory( workCWD );

    bool success = StlAPI_Writer().Write( getOneShape( m_assy ), tmpFname );

    if( success )
    {
        // Preserve the permissions of the current file
        KIPLATFORM::IO::DuplicatePermissions( fn.GetFullPath(), tmpFname );

        if( !wxRenameFile( tmpFname, fn.GetFullName(), true ) )
        {
            m_reporter->Report( wxString::Format( _( "Cannot rename temporary file '%s' to '%s'." ),
                                                  tmpFname,
                                                  fn.GetFullName() ),
                                RPT_SEVERITY_ERROR );
            success = false;
        }
    }

    wxSetWorkingDirectory( currCWD );

    return success;
}



bool STEP_PCB_MODEL::WriteU3D( const wxString& aFileName )
{
    if( !isBoardOutlineValid() )
    {
        m_reporter->Report(
                wxString::Format( _( "No valid PCB assembly; cannot create output file '%s'.\n" ), aFileName ),
                RPT_SEVERITY_ERROR );
        return false;
    }

    m_outFmt = OUTPUT_FORMAT::FMT_OUT_U3D;

    performMeshing( m_assy );

    wxFileName fn( aFileName );

    const char* tmpFname = "$tempfile$.u3d";

    // Creates a temporary file with a ascii7 name, because writer does not know unicode filenames.
    wxString currCWD = wxGetCwd();
    wxString workCWD = fn.GetPath();

    if( !workCWD.IsEmpty() )
        wxSetWorkingDirectory( workCWD );

    U3D::WRITER writer( tmpFname );
    bool success = writer.Perform( m_doc );
    if( success )
    {
        // Preserve the permissions of the current file
        KIPLATFORM::IO::DuplicatePermissions( fn.GetFullPath(), tmpFname );

        if( !wxRenameFile( tmpFname, fn.GetFullName(), true ) )
        {
            m_reporter->Report( wxString::Format( wxT( "Cannot rename temporary file '%s' to '%s'.\n" ), tmpFname,
                                                  fn.GetFullName() ),
                    RPT_SEVERITY_ERROR );
            success = false;
        }
    }

    wxSetWorkingDirectory( currCWD );

    return success;
}


bool STEP_PCB_MODEL::WritePDF( const wxString& aFileName )
{
    if( !isBoardOutlineValid() )
    {
        m_reporter->Report(
                wxString::Format( _( "No valid PCB assembly; cannot create output file '%s'.\n" ), aFileName ),
                RPT_SEVERITY_ERROR );
        return false;
    }

    m_outFmt = OUTPUT_FORMAT::FMT_OUT_PDF;

    performMeshing( m_assy );

    wxFileName fn( aFileName );

    wxFileName  u3dTmpfn = wxFileName::CreateTempFileName( "" );
    wxFileName  pdfTmpfn = wxFileName::CreateTempFileName( "" );

    U3D::WRITER writer( u3dTmpfn.GetFullPath().ToStdString() );
    bool        success = writer.Perform( m_doc );

    // PDF test
    std::unique_ptr<PDF_PLOTTER> plotter = std::make_unique<PDF_PLOTTER>();

    plotter->SetColorMode( true );
    plotter->Set3DExport( true );
    plotter->SetCreator( wxT( "Mark's awesome 3d exporter" ) );
    KIGFX::PCB_RENDER_SETTINGS renderSettings;
    plotter->SetRenderSettings( &renderSettings );

    if( !plotter->OpenFile( pdfTmpfn.GetFullPath() ) )
    {
        m_reporter->Report( wxString::Format( wxT( "Cannot open temporary file '%s'.\n" ), pdfTmpfn.GetFullPath() ),
                RPT_SEVERITY_ERROR );
        success = false;
    }
    else
    {
        plotter->StartPlot( "1", "3D Model" );
        double fov_degrees = 16.5f;

        // kind of an arbitrary distance determination
        float distance = sqrt( writer.GetMeshBoundingBox().SquareExtent() ) * 3;

        std::vector<PDF_3D_VIEW> views;

        VECTOR3D camTarget = writer.GetCenter();


        std::vector<float> c2wMatrix =
                PDF_PLOTTER::CreateC2WMatrixFromAngles( camTarget, distance, 180.0f, -75.0f, 25.0f );

        views.emplace_back( PDF_3D_VIEW{
                .m_name = "Default",
                .m_cameraMatrix = c2wMatrix,
                .m_cameraCenter = (float) distance,
                .m_fov = (float) fov_degrees,
        } );



        c2wMatrix = PDF_PLOTTER::CreateC2WMatrixFromAngles( camTarget, distance, 180.0, 0.0f, 0.0f );

        views.emplace_back( PDF_3D_VIEW{
                .m_name = "Top",
                .m_cameraMatrix = c2wMatrix,
                .m_cameraCenter = (float) distance,
                .m_fov = (float) fov_degrees,
        } );



        c2wMatrix = PDF_PLOTTER::CreateC2WMatrixFromAngles( camTarget, distance, 0.0, 0.0f, 0.0f );

        views.emplace_back( PDF_3D_VIEW{
                .m_name = "Bottom",
                .m_cameraMatrix = c2wMatrix,
                .m_cameraCenter = (float) distance,
                .m_fov = (float) fov_degrees,
        } );



        c2wMatrix = PDF_PLOTTER::CreateC2WMatrixFromAngles( camTarget, distance, 90.0f, -90.0f, 90.0f );

        views.emplace_back( PDF_3D_VIEW{
                .m_name = "Front",
                .m_cameraMatrix = c2wMatrix,
                .m_cameraCenter = (float) distance,
                .m_fov = (float) fov_degrees,
        } );

        plotter->Plot3DModel( u3dTmpfn.GetFullPath(), views );
        plotter->EndPlot();
    }

    if( success )
    {
        // Preserve the permissions of the current file
        KIPLATFORM::IO::DuplicatePermissions( fn.GetFullPath(), pdfTmpfn.GetFullPath() );

        if( !wxRenameFile( pdfTmpfn.GetFullPath(), fn.GetFullPath(), true ) )
        {
            m_reporter->Report(  wxString::Format( wxT( "Cannot rename temporary file '%s' to '%s'.\n" ),
                                             pdfTmpfn.GetFullPath(), fn.GetFullPath() ),
                                RPT_SEVERITY_ERROR );
            success = false;
        }
    }

    wxRemoveFile( u3dTmpfn.GetFullPath() );

    return success;
}