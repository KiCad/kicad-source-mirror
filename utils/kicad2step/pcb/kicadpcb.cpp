/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2016 Cirilo Bernardo <cirilo.bernardo@gmail.com>
 * Copyright (C) 2020-2022 KiCad Developers, see AUTHORS.txt for contributors.
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

#include "kicadpcb.h"

#include "kicadcurve.h"
#include "kicadfootprint.h"
#include "oce_utils.h"

#include <sexpr/sexpr.h>
#include <sexpr/sexpr_parser.h>

#include <wx/filename.h>
#include <wx/wxcrtvararg.h>

#include <memory>
#include <string>




KICADPCB::KICADPCB( const wxString& aPcbName )
{
    m_resolver.Set3DConfigDir( wxT( "" ) );

    m_topSolderMask    = wxColour(  15, 102,  15 );
    m_bottomSolderMask = wxColour(  15, 102,  15 );
    m_topSilk          = wxColour( 240, 240, 240 );
    m_bottomSilk       = wxColour( 240, 240, 240 );
    m_copperFinish     = wxColour( 191, 155,  58 );

    m_thickness = 1.6;
    m_pcb_model = nullptr;
    m_minDistance = MIN_DISTANCE;
    m_useGridOrigin = false;
    m_useDrillOrigin = false;
    m_hasGridOrigin = false;
    m_hasDrillOrigin = false;
    m_pcbName = aPcbName;
}


KICADPCB::~KICADPCB()
{
    for( KICADFOOTPRINT* i : m_footprints )
        delete i;

    for( KICADCURVE* i : m_curves )
        delete i;

    delete m_pcb_model;

    return;
}


bool KICADPCB::ReadFile( const wxString& aFileName )
{
    wxFileName fname( aFileName );

    if( fname.GetExt() != "kicad_pcb" )
    {
        ReportMessage( wxString::Format( wxT( "expecting extension kicad_pcb, got %s\n" ),
                                         fname.GetExt() ) );
        return false;
    }

    if( !fname.FileExists() )
    {
        ReportMessage( wxString::Format( wxT( "No such file: %s\n" ), aFileName ) );
        return false;
    }

    fname.Normalize();
    m_filename = fname.GetFullPath();

    try
    {
        SEXPR::PARSER parser;
        std::string infile( fname.GetFullPath().ToUTF8() );
        std::unique_ptr<SEXPR::SEXPR> data( parser.ParseFromFile( infile ) );

        if( !data )
        {
            ReportMessage( wxString::Format( wxT( "No data in file: %s\n" ), aFileName ) );
            return false;
        }

        if( !parsePCB( data.get() ) )
            return false;
    }
    catch( std::exception& e )
    {
        ReportMessage( wxString::Format( wxT( "error reading file: %s\n%s\n" ),
                                         aFileName,
                                         e.what() ) );
        return false;
    }
    catch( ... )
    {
        ReportMessage( wxString::Format( wxT( "unexpected exception while reading file: %s\n" ),
                                         aFileName ) );
        return false;
    }

    return true;
}


bool KICADPCB::WriteSTEP( const wxString& aFileName )
{
    if( m_pcb_model )
        return m_pcb_model->WriteSTEP( aFileName );

    return false;
}


#ifdef SUPPORTS_IGES
bool KICADPCB::WriteIGES( const wxString& aFileName )
{
    if( m_pcb_model )
        return m_pcb_model->WriteIGES( aFileName );

    return false;
}
#endif


bool KICADPCB::parsePCB( SEXPR::SEXPR* data )
{
    if( NULL == data )
        return false;

    if( data->IsList() )
    {
        size_t nc = data->GetNumberOfChildren();
        SEXPR::SEXPR* child = data->GetChild( 0 );
        std::string name = child->GetSymbol();

        bool result = true;

        for( size_t i = 1; i < nc && result; ++i )
        {
            child = data->GetChild( i );

            if( !child->IsList() )
            {
                ReportMessage( wxString::Format( wxT( "corrupt PCB file (line %d)\n" ),
                                                 child->GetLineNumber() ) );
                return false;
            }

            std::string symname( child->GetChild( 0 )->GetSymbol() );

            if( symname == "general" )
                result = result && parseGeneral( child );
            else if( symname == "setup" )
                result = result && parseSetup( child );
            else if( symname == "layers" )
                result = result && parseLayers( child );
            else if( symname == "module" )
                result = result && parseModule( child );
            else if( symname == "footprint" )
                result = result && parseModule( child );
            else if( symname == "gr_arc" )
                result = result && parseCurve( child, CURVE_ARC );
            else if( symname == "gr_line" )
                result = result && parseCurve( child, CURVE_LINE );
            else if( symname == "gr_rect" )
                result = result && parseRect( child );
            else if( symname == "gr_poly" )
                result = result && parsePolygon( child );
            else if( symname == "gr_circle" )
                result = result && parseCurve( child, CURVE_CIRCLE );
            else if( symname == "gr_curve" )
                result = result && parseCurve( child, CURVE_BEZIER );
        }

        return result;
    }

    ReportMessage( wxString::Format( wxT( "data is not a valid PCB file: %s\n" ), m_filename ) );
    return false;
}


bool KICADPCB::parseGeneral( SEXPR::SEXPR* data )
{
    size_t nc = data->GetNumberOfChildren();
    SEXPR::SEXPR* child = NULL;

    for( size_t i = 1; i < nc; ++i )
    {
        child = data->GetChild( i );

        if( !child->IsList() )
        {
            ReportMessage( wxString::Format( wxT( "corrupt PCB file (line %d)\n" ),
                                             child->GetLineNumber() ) );
            return false;
        }

        // at the moment only the thickness is of interest in the general section
        if( child->GetChild( 0 )->GetSymbol() != "thickness" )
            continue;

        m_thickness = child->GetChild( 1 )->GetDouble();
        return true;
    }

    ReportMessage( wxString::Format( wxT( "corrupt PCB file (line %d)\n"
                                          "no PCB thickness specified in general section\n" ),
                                     child->GetLineNumber() ) );
    return false;
}


bool KICADPCB::parseLayers( SEXPR::SEXPR* data )
{
    size_t nc = data->GetNumberOfChildren();
    SEXPR::SEXPR* child = NULL;

    // Read the layername and the corresponding layer id list:
    for( size_t i = 1; i < nc; ++i )
    {
        child = data->GetChild( i );

        if( !child->IsList() )
        {
            ReportMessage( wxString::Format( wxT( "corrupt PCB file (line %d)\n" ),
                                             child->GetLineNumber() ) );
            return false;
        }

        std::string ref;

        if( child->GetChild( 1 )->IsSymbol() )
            ref = child->GetChild( 1 )->GetSymbol();
        else
            ref = child->GetChild( 1 )->GetString();

        m_layersNames[ref] = child->GetChild( 0 )->GetInteger();
    }

    return true;
}


bool KICADPCB::parseStackupLayer( SEXPR::SEXPR* data )
{
    if( data->IsList() && data->GetNumberOfChildren() >= 3 )
    {
        size_t nc = data->GetNumberOfChildren();
        SEXPR::SEXPR* child = NULL;
        std::string   ref;
        std::string   value;

        for( size_t i = 1; i < nc; ++i )
        {
            child = data->GetChild( i );

            if( child->IsList() && child->GetChild( 0 )->GetSymbol() == "type" )
            {
                if( child->GetChild( 1 )->IsSymbol() )
                    ref = child->GetChild( 1 )->GetSymbol();
                else
                    ref = child->GetChild( 1 )->GetString();
            }
            else if( child->IsList() && child->GetChild( 0 )->GetSymbol() == "color" )
            {
                if( child->GetChild( 1 )->IsSymbol() )
                    value = child->GetChild( 1 )->GetSymbol();
                else
                    value = child->GetChild( 1 )->GetString();
            }
        }

        if( !value.empty() )
        {
            wxString colorName( value );
            wxColour color;

            if( colorName.StartsWith( wxT( "#" ) ) )
                color = wxColour( colorName.Left( 7 ) /* drop alpha component */ );
            else if( colorName == wxT( "Green" ) )
                color = wxColour(  20,  51,  36 );
            else if( colorName == wxT( "Light Green" ) )
                color = wxColour(  91, 168,  12);
            else if( colorName == wxT( "Saturated Green" ) )
                color = wxColour(  13, 104,  11 );
            else if( colorName == wxT( "Red" ) )
                color = wxColour( 181,  19,  21 );
            else if( colorName == wxT( "Light Red" ) )
                color = wxColour( 210,  40,  14 );
            else if( colorName == wxT( "Red/Orange" ) )
                color = wxColour( 239,  53,  41 );
            else if( colorName == wxT( "Blue" ) )
                color = wxColour(   2,  59, 162 );
            else if( colorName == wxT( "Light Blue 1" ) )
                color = wxColour(  54,  79, 116 );
            else if( colorName == wxT( "Light Blue 2" ) )
                color = wxColour(  61,  85, 130 );
            else if( colorName == wxT( "Green/Blue" ) )
                color = wxColour(  21,  70,  80 );
            else if( colorName == wxT( "Black" ) )
                color = wxColour(  11,  11,  11 );
            else if( colorName == wxT( "White" ) )
                color = wxColour( 245, 245, 245 );
            else if( colorName == wxT( "Purple" ) )
                color = wxColour(  32,   2,  53 );
            else if( colorName == wxT( "Light Purple" ) )
                color = wxColour( 119,  31,  91 );
            else if( colorName == wxT( "Yellow" ) )
                color = wxColour( 194,  195,  0 );

            if( ref == "Top Silk Screen" )
                m_topSilk = color;
            else if( ref == "Top Solder Mask" )
                m_topSolderMask = color;
            else if( ref == "Bottom Silk Screen" )
                m_bottomSilk = color;
            else if( ref == "Bottom Solder Mask" )
                m_bottomSolderMask = color;
        }
    }

    return true;
}


bool KICADPCB::parseStackup( SEXPR::SEXPR* data )
{
    size_t nc = data->GetNumberOfChildren();
    SEXPR::SEXPR* child = NULL;

    for( size_t i = 1; i < nc; ++i )
    {
        child = data->GetChild( i );

        if( !child->IsList() )
        {
            ReportMessage( wxString::Format( wxT( "corrupt PCB file (line %d)\n" ),
                                             child->GetLineNumber() ) );
            return false;
        }

        std::string ref;

        if( child->GetChild( 0 )->GetSymbol() == "layer" )
        {
            parseStackupLayer( child );
        }
        else if( child->GetChild( 0 )->GetSymbol() == "copper_finish" )
        {
            if( child->GetChild( 1 )->IsSymbol() )
                ref = child->GetChild( 1 )->GetSymbol();
            else
                ref = child->GetChild( 1 )->GetString();

            wxString finishName( ref );

            if( finishName.EndsWith( wxT( "OSP" ) ) )
            {
                m_copperFinish = wxColour( 184, 115, 50 );
            }
            else if( finishName.EndsWith( wxT( "IG" ) )
                  || finishName.EndsWith( wxT( "gold" ) ) )
            {
                m_copperFinish = wxColour( 178, 156, 0 );
            }
            else if( finishName.StartsWith( wxT( "HAL" ) )
                  || finishName.StartsWith( wxT( "HASL" ) )
                  || finishName.EndsWith( wxT( "tin" ) )
                  || finishName.EndsWith( wxT( "nickel" ) ) )
            {
                m_copperFinish = wxColour( 160, 160, 160 );
            }
            else if( finishName.EndsWith( wxT( "silver" ) ) )
            {
                m_copperFinish = wxColour( 213, 213, 213 );
            }
        }
    }

    return true;
}


int KICADPCB::GetLayerId( std::string& aLayerName )
{
    int lid = -1;
    auto item = m_layersNames.find( aLayerName );

    if( item != m_layersNames.end() )
        lid = item->second;

    return lid;
}


bool KICADPCB::parseSetup( SEXPR::SEXPR* data )
{
    size_t nc = data->GetNumberOfChildren();
    SEXPR::SEXPR* child = NULL;

    for( size_t i = 1; i < nc; ++i )
    {
        child = data->GetChild( i );

        if( !child->IsList() )
        {
            ReportMessage( wxString::Format( wxT( "corrupt PCB file (line %d)\n" ),
                                             child->GetLineNumber() ) );
            return false;
        }

        // at the moment only the Grid and Drill origins are of interest in the setup section
        if( child->GetChild( 0 )->GetSymbol() == "grid_origin" )
        {
            if( child->GetNumberOfChildren() != 3 )
            {
                ReportMessage( wxString::Format( wxT( "corrupt PCB file (line %d): grid_origin has "
                                                      "%d children (expected: 3)\n" ),
                                                 child->GetLineNumber(),
                                                 child->GetNumberOfChildren() ) );
                return false;
            }

            m_gridOrigin.x = child->GetChild( 1 )->GetDouble();
            m_gridOrigin.y = child->GetChild( 2 )->GetDouble();
            m_hasGridOrigin = true;
        }
        else if( child->GetChild( 0 )->GetSymbol() == "aux_axis_origin" )
        {
            if( child->GetNumberOfChildren() != 3 )
            {
                ReportMessage( wxString::Format( wxT( "corrupt PCB file (line %d): aux_axis_origin "
                                                      "has %d children (expected: 3)\n" ),
                                                 child->GetLineNumber(),
                                                 child->GetNumberOfChildren() ) );
                return false;
            }

            m_drillOrigin.x = child->GetChild( 1 )->GetDouble();
            m_drillOrigin.y = child->GetChild( 2 )->GetDouble();
            m_hasDrillOrigin = true;
        }
        else if( child->GetChild( 0 )->GetSymbol() == "stackup" )
        {
            parseStackup( child );
        }
    }

    return true;
}


bool KICADPCB::parseModule( SEXPR::SEXPR* data )
{
    KICADFOOTPRINT* footprint = new KICADFOOTPRINT( this );

    if( !footprint->Read( data ) )
    {
        delete footprint;
        return false;
    }

    m_footprints.push_back( footprint );
    return true;
}


bool KICADPCB::parseRect( SEXPR::SEXPR* data )
{
    KICADCURVE* rect = new KICADCURVE();

    if( !rect->Read( data, CURVE_LINE ) )
    {
        delete rect;
        return false;
    }

    // reject any curves not on the Edge.Cuts layer
    if( rect->GetLayer() != LAYER_EDGE )
    {
        delete rect;
        return true;
    }

    KICADCURVE* top = new KICADCURVE( *rect );
    KICADCURVE* right = new KICADCURVE( *rect );
    KICADCURVE* bottom = new KICADCURVE( *rect );
    KICADCURVE* left = new KICADCURVE( *rect );
    delete rect;

    top->m_end.y = right->m_start.y;
    m_curves.push_back( top );

    right->m_start.x = bottom->m_end.x;
    m_curves.push_back( right );

    bottom->m_start.y = left->m_end.y;
    m_curves.push_back( bottom );

    left->m_end.x = top->m_start.x;
    m_curves.push_back( left );

    return true;
}


bool KICADPCB::parsePolygon( SEXPR::SEXPR* data )
{
    std::unique_ptr<KICADCURVE> poly = std::make_unique<KICADCURVE>();

    if( !poly->Read( data, CURVE_POLYGON ) )
        return false;

    // reject any curves not on the Edge.Cuts layer
    if( poly->GetLayer() != LAYER_EDGE )
        return true;

    auto pts = poly->m_poly;

    for( std::size_t ii = 1; ii < pts.size(); ++ii )
    {
        KICADCURVE* seg = new KICADCURVE();
        seg->m_form = CURVE_LINE;
        seg->m_layer = poly->GetLayer();
        seg->m_start = pts[ii - 1];
        seg->m_end = pts[ii];
        m_curves.push_back( seg );
    }

    KICADCURVE* seg = new KICADCURVE();
    seg->m_form = CURVE_LINE;
    seg->m_layer = poly->GetLayer();
    seg->m_start = pts.back();
    seg->m_end = pts.front();
    m_curves.push_back( seg );

    return true;
}


bool KICADPCB::parseCurve( SEXPR::SEXPR* data, CURVE_TYPE aCurveType )
{
    KICADCURVE* curve = new KICADCURVE();

    if( !curve->Read( data, aCurveType ) )
    {
        delete curve;
        return false;
    }

    // reject any curves not on the Edge.Cuts layer
    if( curve->GetLayer() != LAYER_EDGE )
    {
        delete curve;
        return true;
    }

    m_curves.push_back( curve );
    return true;
}


bool KICADPCB::ComposePCB( bool aComposeVirtual, bool aSubstituteModels )
{
    if( m_pcb_model )
        return true;

    if( m_footprints.empty() && m_curves.empty() )
    {
        ReportMessage( wxT( "Error: no PCB data (no footprint, no outline) to render\n" ) );
        return false;
    }

    DOUBLET origin;

    // Determine the coordinate system reference:
    // Precedence of reference point is Drill Origin > Grid Origin > User Offset
    if( m_useDrillOrigin && m_hasDrillOrigin )
        origin = m_drillOrigin;
    else if( m_useGridOrigin && m_hasDrillOrigin )
        origin = m_gridOrigin;
    else
        origin = m_origin;

    m_pcb_model = new PCBMODEL( m_pcbName );

    // TODO: Handle when top & bottom soldermask colours are different...
    m_pcb_model->SetBoardColor( m_topSolderMask.Red() / 255.0,
                                m_topSolderMask.Green() / 255.0,
                                m_topSolderMask.Blue() / 255.0 );

    m_pcb_model->SetPCBThickness( m_thickness );
    m_pcb_model->SetMinDistance( m_minDistance );

    for( KICADCURVE* i : m_curves )
    {
        if( CURVE_NONE == i->m_form || LAYER_EDGE != i->m_layer )
            continue;

        // adjust the coordinate system
        // Note: we negate the Y coordinates due to the fact in Pcbnew the Y axis
        // is from top to bottom.
        KICADCURVE lcurve = *i;
        lcurve.m_start.y = -( lcurve.m_start.y - origin.y );
        lcurve.m_end.y = -( lcurve.m_end.y - origin.y );
        lcurve.m_start.x -= origin.x;
        lcurve.m_end.x -= origin.x;
        // used in bezier curves:
        lcurve.m_bezierctrl1.y = -( lcurve.m_bezierctrl1.y - origin.y );
        lcurve.m_bezierctrl1.x -= origin.x;
        lcurve.m_bezierctrl2.y = -( lcurve.m_bezierctrl2.y - origin.y );
        lcurve.m_bezierctrl2.x -= origin.x;

        if( CURVE_ARC == lcurve.m_form )
            lcurve.m_angle = -lcurve.m_angle;

        m_pcb_model->AddOutlineSegment( &lcurve );
    }

    for( KICADFOOTPRINT* i : m_footprints )
        i->ComposePCB( m_pcb_model, &m_resolver, origin, aComposeVirtual, aSubstituteModels );

    ReportMessage( wxT( "Create PCB solid model\n" ) );

    if( !m_pcb_model->CreatePCB() )
    {
        ReportMessage( wxT( "could not create PCB solid model\n" ) );
        delete m_pcb_model;
        m_pcb_model = NULL;
        return false;
    }

    return true;
}
