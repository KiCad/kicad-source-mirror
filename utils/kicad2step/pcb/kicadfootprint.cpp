/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2016 Cirilo Bernardo <cirilo.bernardo@gmail.com>
 * Copyright  2018-2021 KiCad Developers, see AUTHORS.txt for contributors.
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

#include "kicadfootprint.h"

#include "3d_resolver.h"
#include "kicadcurve.h"
#include "kicadmodel.h"
#include "kicadpad.h"
#include "oce_utils.h"

#include <sexpr/sexpr.h>

#include <wx/log.h>
#include <wx/filename.h>

#include <iostream>
#include <limits>
#include <sstream>

#include <Standard_Failure.hxx>


KICADFOOTPRINT::KICADFOOTPRINT( KICADPCB* aParent )
{
    m_parent = aParent;
    m_side = LAYER_NONE;
    m_rotation = 0.0;
    m_smd = false;
    m_tht = false;
    m_virtual = false;

    return;
}


KICADFOOTPRINT::~KICADFOOTPRINT()
{
    for( KICADPAD* i : m_pads )
        delete i;

    for( KICADCURVE* i : m_curves )
        delete i;

    for( KICADMODEL* i : m_models )
        delete i;

    return;
}


bool KICADFOOTPRINT::Read( SEXPR::SEXPR* aEntry )
{
    if( !aEntry )
        return false;

    if( aEntry->IsList() )
    {
        size_t nc = aEntry->GetNumberOfChildren();
        SEXPR::SEXPR* child = aEntry->GetChild( 0 );
        std::string name = child->GetSymbol();

        if( name != "module" && name != "footprint" )
        {
            std::ostringstream ostr;
            ostr << "* BUG: module parser invoked for type '" << name << "'\n";
            wxLogMessage( "%s\n", ostr.str().c_str() );
            return false;
        }

        bool result = true;

        for( size_t i = 2; i < nc && result; ++i )
        {
            child = aEntry->GetChild( i );
            std::string symname;

            // skip the optional locked and/or placed attributes; due to the vagaries of the
            // KiCad version of sexpr, the attribute may be a Symbol or a String
            if( child->IsSymbol() || child->IsString() )
            {
                if( child->IsSymbol() )
                    symname = child->GetSymbol();
                else if( child->IsString() )
                    symname = child->GetString();

                if( symname == "locked" || symname == "placed" )
                    continue;

                wxLogMessage( "* module descr in PCB file at line %d: unexpected keyword '%s'\n",
                              child->GetLineNumber(), symname.c_str() );
                return false;
            }

            if( !child->IsList() )
            {
                wxLogMessage( "* corrupt module in PCB file at line %d\n",
                              child->GetLineNumber() );
                return false;
            }

            symname = child->GetChild( 0 )->GetSymbol();

            if( symname == "layer" )
                result = parseLayer( child );
            else if( symname == "at" )
                result = parsePosition( child );
            else if( symname == "attr" )
                result = parseAttribute( child );
            else if( symname == "fp_text" )
                result = parseText( child );
            else if( symname == "fp_arc" )
                result = parseCurve( child, CURVE_ARC );
            else if( symname == "fp_line" )
                result = parseCurve( child, CURVE_LINE );
            else if( symname == "fp_circle" )
                result = parseCurve( child, CURVE_CIRCLE );
            else if( symname == "fp_rect" )
                result = parseRect( child );
            else if( symname == "fp_curve" )
                result = parseCurve( child, CURVE_BEZIER );
            else if( symname == "pad" )
                result = parsePad( child );
            else if( symname == "model" )
                result = parseModel( child );
        }

        if( !m_smd && !m_tht )
            m_virtual = true;

        return result;
    }

    std::ostringstream ostr;
    ostr << "* data is not a valid PCB module\n";
    wxLogMessage( "%s\n", ostr.str().c_str() );

    return false;
}


bool KICADFOOTPRINT::parseRect( SEXPR::SEXPR* data )
{
    std::unique_ptr<KICADCURVE> rect = std::make_unique<KICADCURVE>();

    if( !rect->Read( data, CURVE_LINE ) )
        return false;

    // reject any curves not on the Edge.Cuts layer
    if( rect->GetLayer() != LAYER_EDGE )
        return true;

    KICADCURVE* top = new KICADCURVE( *rect );
    KICADCURVE* right = new KICADCURVE( *rect );
    KICADCURVE* bottom = new KICADCURVE( *rect );
    KICADCURVE* left = new KICADCURVE( *rect );

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


bool KICADFOOTPRINT::parseModel( SEXPR::SEXPR* data )
{
    KICADMODEL* mp = new KICADMODEL();

    if( !mp->Read( data ) )
    {
        delete mp;
        return false;
    }

    if( mp->Hide() )
        delete mp;
    else
        m_models.push_back( mp );

    return true;
}


bool KICADFOOTPRINT::parseCurve( SEXPR::SEXPR* data, CURVE_TYPE aCurveType )
{
    KICADCURVE* mp = new KICADCURVE();

    if( !mp->Read( data, aCurveType ) )
    {
        delete mp;
        return false;
    }

    // NOTE: for now we are only interested in glyphs on the outline layer
    if( LAYER_EDGE != mp->GetLayer() )
    {
        delete mp;
        return true;
    }

    m_curves.push_back( mp );
    return true;
}


bool KICADFOOTPRINT::parseLayer( SEXPR::SEXPR* data )
{
    SEXPR::SEXPR* val = data->GetChild( 1 );
    std::string layername;

    if( val->IsSymbol() )
        layername = val->GetSymbol();
    else if( val->IsString() )
        layername = val->GetString();
    else
    {
        std::ostringstream ostr;
        ostr << "* corrupt module in PCB file (line ";
        ostr << val->GetLineNumber() << "); layer cannot be parsed\n";
        wxLogMessage( "%s\n", ostr.str().c_str() );
        return false;
    }

    int layerId = m_parent->GetLayerId( layername );

    if( layerId == 31 )
        m_side = LAYER_BOTTOM;
    else
        m_side = LAYER_TOP;

    return true;
}


bool KICADFOOTPRINT::parsePosition( SEXPR::SEXPR* data )
{
    return Get2DPositionAndRotation( data, m_position, m_rotation );
}


bool KICADFOOTPRINT::parseAttribute( SEXPR::SEXPR* data )
{
    if( data->GetNumberOfChildren() < 2 )
    {
        std::ostringstream ostr;
        ostr << "* corrupt module in PCB file (line ";
        ostr << data->GetLineNumber() << "); attribute cannot be parsed\n";
        wxLogMessage( "%s\n", ostr.str().c_str() );
        return false;
    }

    SEXPR::SEXPR* child = data->GetChild( 1 );
    std::string text;

    if( child->IsSymbol() )
        text = child->GetSymbol();
    else if( child->IsString() )
        text = child->GetString();

    if( text == "smd" )
        m_smd = true;
    else if( text == "through_hole" )
        m_tht = true;
    else if( text == "virtual" )
        m_virtual = true;

    return true;
}


bool KICADFOOTPRINT::parseText( SEXPR::SEXPR* data )
{
    // we're only interested in the Reference Designator
    if( data->GetNumberOfChildren() < 3 )
        return true;

    SEXPR::SEXPR* child = data->GetChild( 1 );
    std::string text;

    if( child->IsSymbol() )
        text = child->GetSymbol();
    else if( child->IsString() )
        text = child->GetString();

    if( text != "reference" )
        return true;

    child = data->GetChild( 2 );

    if( child->IsSymbol() )
        text = child->GetSymbol();
    else if( child->IsString() )
        text = child->GetString();

    m_refdes = text;
    return true;
}


bool KICADFOOTPRINT::parsePad( SEXPR::SEXPR* data )
{
    KICADPAD* mp = new KICADPAD();

    if( !mp->Read( data ) )
    {
        delete mp;
        return false;
    }

    // NOTE: for now we only accept thru-hole pads
    // for the MCAD description
    if( !mp->IsThruHole() )
    {
        delete mp;
        return true;
    }

    m_pads.push_back( mp );
    return true;
}


bool KICADFOOTPRINT::ComposePCB( class PCBMODEL* aPCB, S3D_RESOLVER* resolver,
                                 DOUBLET aOrigin, bool aComposeVirtual, bool aSubstituteModels )
{
    // translate pads and curves to final position and append to PCB.
    double dlim = (double)std::numeric_limits< float >::epsilon();

    double vsin = sin( m_rotation );
    double vcos = cos( m_rotation );
    bool hasdata = false;

    double posX = m_position.x - aOrigin.x;
    double posY = m_position.y - aOrigin.y;

    for( KICADCURVE* i : m_curves )
    {
        if( i->m_layer != LAYER_EDGE || CURVE_NONE == i->m_form )
            continue;

        KICADCURVE lcurve = *i;

        lcurve.m_start.y = -lcurve.m_start.y;
        lcurve.m_end.y   = -lcurve.m_end.y;
        lcurve.m_angle   = -lcurve.m_angle;

        if( m_rotation < -dlim || m_rotation > dlim )
        {
            double x = lcurve.m_start.x * vcos - lcurve.m_start.y * vsin;
            double y = lcurve.m_start.x * vsin + lcurve.m_start.y * vcos;
            lcurve.m_start.x = x;
            lcurve.m_start.y = y;
            x = lcurve.m_end.x * vcos - lcurve.m_end.y * vsin;
            y = lcurve.m_end.x * vsin + lcurve.m_end.y * vcos;
            lcurve.m_end.x = x;
            lcurve.m_end.y = y;
        }

        lcurve.m_start.x += posX;
        lcurve.m_start.y -= posY;
        lcurve.m_end.x += posX;
        lcurve.m_end.y -= posY;

        if( aPCB->AddOutlineSegment( &lcurve ) )
            hasdata = true;

    }

    for( KICADPAD* i : m_pads )
    {
        if( !i->IsThruHole() )
            continue;

        KICADPAD lpad = *i;
        lpad.m_position.y = -lpad.m_position.y;

        if( m_rotation < -dlim || m_rotation > dlim )
        {
            double x = lpad.m_position.x * vcos - lpad.m_position.y * vsin;
            double y = lpad.m_position.x * vsin + lpad.m_position.y * vcos;
            lpad.m_position.x = x;
            lpad.m_position.y = y;
        }

        lpad.m_position.x += posX;
        lpad.m_position.y -= posY;

        if( aPCB->AddPadHole( &lpad ) )
            hasdata = true;

    }

    if( m_virtual && !aComposeVirtual )
        return hasdata;

    DOUBLET newpos( posX, posY );

    for( KICADMODEL* i : m_models )
    {
        wxString mname = wxString::FromUTF8Unchecked( i->m_modelname.c_str() );

        if( mname.empty() )
        {
            ReportMessage( wxString::Format( "No model defined for component %s.\n", m_refdes ) );
            return false;
        }

        mname = resolver->ResolvePath( mname );

        if( !wxFileName::FileExists( mname ) )
        {
            ReportMessage( wxString::Format( "Could not add component %s.\nFile not found: %s\n",
                                             m_refdes,
                                             mname ) );
            return false;
        }

        std::string fname( mname.ToUTF8() );

        try
        {
            if( aPCB->AddComponent( fname, m_refdes, LAYER_BOTTOM == m_side ? true : false,
                                    newpos, m_rotation, i->m_offset, i->m_rotation, i->m_scale,
                                    aSubstituteModels ) )
            {
                hasdata = true;
            }
        }
        catch( const Standard_Failure& e)
        {
            ReportMessage( wxString::Format( "Could not add component %s.\nOpenCASCADE error: %s\n",
                                             m_refdes, 
                                             e.GetMessageString() ) );
        }
    }

    return hasdata;
}
