/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2023 Alex Shvartzkop <dudesuchamazing@gmail.com>
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

#include "pcbnew_utils_3d.h"

#include <memory>

#include <wx/filename.h>
#include <wx/wfstream.h>
#include <wx/log.h>

#include <TDocStd_Document.hxx>
#include <XCAFApp_Application.hxx>
#include <XCAFDoc_DocumentTool.hxx>
#include <XCAFDoc_ShapeTool.hxx>
#include <XCAFDoc_ColorTool.hxx>

#include <TopoDS.hxx>
#include <TopExp_Explorer.hxx>

#include <Bnd_Box.hxx>
#include <BRepBndLib.hxx>

#include <BRepTools.hxx>
#include <BRep_Builder.hxx>
#include <BRepBuilderAPI_Transform.hxx>
#include <BRepBuilderAPI_GTransform.hxx>

#include <STEPCAFControl_Reader.hxx>
#include <STEPCAFControl_Writer.hxx>
#include <Interface_Static.hxx>

#include <XCAFDoc_Color.hxx>
#include <Quantity_Color.hxx>
#include <ShapeCustom.hxx>

// precision for mesh creation; 0.07 should be good enough for ECAD viewing
#define USER_PREC ( 0.14 )


static void transformDoc( Handle( TDocStd_Document ) & source, Handle( TDocStd_Document ) & dest,
                          const gp_GTrsf& transform )
{
    // transfer data from Source into a top level component of Dest
    BRepBuilderAPI_GTransform brep( transform );

    // s_assy = shape tool for the source
    Handle( XCAFDoc_ShapeTool ) s_assy = XCAFDoc_DocumentTool::ShapeTool( source->Main() );

    // retrieve all free shapes within the assembly
    TDF_LabelSequence frshapes;
    s_assy->GetFreeShapes( frshapes );

    // d_assy = shape tool for the destination
    Handle( XCAFDoc_ShapeTool ) d_assy = XCAFDoc_DocumentTool::ShapeTool( dest->Main() );

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
            TopoDS_Shape transformed_shape( shape );

            if( transform.Trsf().ScaleFactor() != 1.0 )
            {
                transformed_shape =
                        ShapeCustom::ScaleShape( shape, transform.Trsf().ScaleFactor() );
            }
            else
            {
                brep.Perform( shape, Standard_False );

                if( brep.IsDone() )
                    transformed_shape = brep.Shape();
            }

            TDF_Label niulab = d_assy->AddShape( transformed_shape, false, false );

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
}


VECTOR3D UTILS_BOX3D::GetCenter()
{
    return VECTOR3D( ( m_min.x + m_max.x ) * 0.5, ( m_min.y + m_max.y ) * 0.5,
                     ( m_min.z + m_max.z ) * 0.5 );
}


VECTOR3D UTILS_BOX3D::GetSize()
{
    return VECTOR3D( m_max.x - m_min.x, m_max.y - m_min.y, m_max.z - m_min.z );
}


VECTOR3D& UTILS_BOX3D::Min()
{
    return m_min;
}


VECTOR3D& UTILS_BOX3D::Max()
{
    return m_max;
}


struct UTILS_STEP_MODEL::DATA
{
    Handle( TDocStd_Document ) m_frontDoc;
    Handle( TDocStd_Document ) m_backDoc;
    STEPCAFControl_Reader m_cafReader;
};


UTILS_STEP_MODEL* UTILS_STEP_MODEL::LoadSTEP( const wxString& aFileName )
{
    std::unique_ptr<UTILS_STEP_MODEL::DATA> data = std::make_unique<UTILS_STEP_MODEL::DATA>();

    Handle( XCAFApp_Application ) m_app = XCAFApp_Application::GetApplication();
    m_app->NewDocument( "MDTV-XCAF", data->m_frontDoc );
    m_app->NewDocument( "MDTV-XCAF", data->m_backDoc );

    STEPCAFControl_Reader reader;
    IFSelect_ReturnStatus stat = reader.ReadFile( aFileName.ToStdString().c_str() );

    if( stat != IFSelect_RetDone )
        return nullptr;

    // Enable user-defined shape precision
    if( !Interface_Static::SetIVal( "read.precision.mode", 1 ) )
        return nullptr;

    // Set the shape conversion precision to USER_PREC (default 0.0001 has too many triangles)
    if( !Interface_Static::SetRVal( "read.precision.val", USER_PREC ) )
        return nullptr;

    // set other translation options
    reader.SetColorMode( true );  // use model colors
    reader.SetNameMode( true );   // use label names
    reader.SetLayerMode( false ); // ignore LAYER data

    if( !reader.Transfer( data->m_frontDoc ) )
        return nullptr;

    data->m_cafReader = reader;

    UTILS_STEP_MODEL* model = new UTILS_STEP_MODEL();
    model->m_data = data.release();
    return model;
}


UTILS_STEP_MODEL::~UTILS_STEP_MODEL()
{
    delete m_data;
}


UTILS_BOX3D UTILS_STEP_MODEL::GetBoundingBox()
{
    Handle( XCAFDoc_ShapeTool ) assy =
            XCAFDoc_DocumentTool::ShapeTool( m_data->m_frontDoc->Main() );

    TDF_LabelSequence frshapes;
    assy->GetFreeShapes( frshapes );

    int         nshapes = frshapes.Length();
    Bnd_Box     bbox;
    UTILS_BOX3D result;

    if( nshapes == 0 )
        return result;

    for( const TDF_Label& label : frshapes )
    {
        TopoDS_Shape shape = assy->GetShape( label );
        BRepBndLib::Add( shape, bbox );
    }

    gp_Pnt min = bbox.CornerMin();
    gp_Pnt max = bbox.CornerMax();

    result.Min().x = min.X();
    result.Min().y = min.Y();
    result.Min().z = min.Z();

    result.Max().x = max.X();
    result.Max().y = max.Y();
    result.Max().z = max.Z();

    return result;
}


void UTILS_STEP_MODEL::Translate( double aX, double aY, double aZ )
{
    // Create a translation transformation
    gp_Trsf transform;
    transform.SetTranslation( gp_Vec( aX, aY, aZ ) );

    transformDoc( m_data->m_frontDoc, m_data->m_backDoc, transform );
    std::swap( m_data->m_frontDoc, m_data->m_backDoc );

    Handle( XCAFApp_Application ) m_app = XCAFApp_Application::GetApplication();

    m_data->m_backDoc->Main().ForgetAllAttributes();
    m_app->Close( m_data->m_backDoc );
    m_app->NewDocument( "MDTV-XCAF", m_data->m_backDoc );
}


void UTILS_STEP_MODEL::Scale( double aScale )
{

    // Create a scale transformation
    gp_Trsf transform;
    transform.SetScaleFactor( aScale );

    transformDoc( m_data->m_frontDoc, m_data->m_backDoc, transform );
    std::swap( m_data->m_frontDoc, m_data->m_backDoc );

    Handle( XCAFApp_Application ) m_app = XCAFApp_Application::GetApplication();

    m_data->m_backDoc->Main().ForgetAllAttributes();
    m_app->Close( m_data->m_backDoc );
    m_app->NewDocument( "MDTV-XCAF", m_data->m_backDoc );
}


bool UTILS_STEP_MODEL::SaveSTEP( const wxString& aFileName )
{
    STEPCAFControl_Writer writer;
    writer.SetColorMode( true );
    writer.SetNameMode( true );

    // To reduce file size
    if( !Interface_Static::SetIVal( "write.surfacecurve.mode", 0 ) )
        return false;

    if( !writer.Transfer( m_data->m_frontDoc, STEPControl_AsIs ) )
        return false;

    if( writer.Write( aFileName.ToStdString().c_str() ) != IFSelect_RetDone )
        return false;

    Handle( XCAFApp_Application ) m_app = XCAFApp_Application::GetApplication();

    m_data->m_frontDoc->Main().ForgetAllAttributes();
    m_data->m_backDoc->Main().ForgetAllAttributes();

    m_app->Close( m_data->m_frontDoc );
    m_app->Close( m_data->m_backDoc );

    return true;
}