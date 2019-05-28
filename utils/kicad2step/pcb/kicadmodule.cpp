/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2016 Cirilo Bernardo <cirilo.bernardo@gmail.com>
 * Copyright  2018 KiCad Developers, see AUTHORS.txt for contributors.
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

#include "kicadmodule.h"

#include "3d_resolver.h"
#include "kicadcurve.h"
#include "kicadmodel.h"
#include "kicadpad.h"
#include "oce_utils.h"

#include <sexpr/sexpr.h>

#include <wx/log.h>

#include <iostream>
#include <limits>
#include <sstream>


KICADMODULE::KICADMODULE( KICADPCB* aParent )
{
    m_parent = aParent;
    m_side = LAYER_NONE;
    m_rotation = 0.0;
    m_virtual = false;

    return;
}


KICADMODULE::~KICADMODULE()
{
    for( auto i : m_pads )
        delete i;

    for( auto i : m_curves )
        delete i;

    for( auto i : m_models )
        delete i;

    return;
}


bool KICADMODULE::Read( SEXPR::SEXPR* aEntry )
{
    if( NULL == aEntry )
        return false;

    if( aEntry->IsList() )
    {
        size_t nc = aEntry->GetNumberOfChildren();
        SEXPR::SEXPR* child = aEntry->GetChild( 0 );
        std::string name = child->GetSymbol();

        if( name != "module" )
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
            // kicad version of sexpr, the attribute may be a Symbol or a String
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
                result = result && parseLayer( child );
            else if( symname == "at" )
                result = result && parsePosition( child );
            else if( symname == "attr" )
                result = result && parseAttribute( child );
            else if( symname == "fp_text" )
                result = result && parseText( child );
            else if( symname == "fp_arc" )
                result = result && parseCurve( child, CURVE_ARC );
            else if( symname == "fp_line" )
                result = result && parseCurve( child, CURVE_LINE );
            else if( symname == "fp_circle" )
                result = result && parseCurve( child, CURVE_CIRCLE );
            else if( symname == "pad" )
                result = result && parsePad( child );
            else if( symname == "model" )
                result = result && parseModel( child );
        }

        return result;
    }

    std::ostringstream ostr;
    ostr << "* data is not a valid PCB module\n";
    wxLogMessage( "%s\n", ostr.str().c_str() );

    return false;
}


bool KICADMODULE::parseModel( SEXPR::SEXPR* data )
{
    KICADMODEL* mp = new KICADMODEL();

    if( !mp->Read( data ) )
    {
        delete mp;
        return false;
    }

    m_models.push_back( mp );
    return true;
}


bool KICADMODULE::parseCurve( SEXPR::SEXPR* data, CURVE_TYPE aCurveType )
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


bool KICADMODULE::parseLayer( SEXPR::SEXPR* data )
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


bool KICADMODULE::parsePosition( SEXPR::SEXPR* data )
{
    return Get2DPositionAndRotation( data, m_position, m_rotation );
}


bool KICADMODULE::parseAttribute( SEXPR::SEXPR* data )
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

    if( text == "virtual" )
        m_virtual = true;

    return true;
}


bool KICADMODULE::parseText( SEXPR::SEXPR* data )
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


bool KICADMODULE::parsePad( SEXPR::SEXPR* data )
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


bool KICADMODULE::ComposePCB( class PCBMODEL* aPCB, S3D_RESOLVER* resolver,
    DOUBLET aOrigin, bool aComposeVirtual )
{
    // translate pads and curves to final position and append to PCB.
    double dlim = (double)std::numeric_limits< float >::epsilon();

    double vsin = sin( m_rotation );
    double vcos = cos( m_rotation );
    bool hasdata = false;

    double posX = m_position.x - aOrigin.x;
    double posY = m_position.y - aOrigin.y;

    for( auto i : m_curves )
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

    for( auto i : m_pads )
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

    for( auto i : m_models )
    {
        std::string fname( resolver->ResolvePath(
            wxString::FromUTF8Unchecked( i->m_modelname.c_str() ) ).ToUTF8() );

        if( aPCB->AddComponent( fname, m_refdes, LAYER_BOTTOM == m_side ? true : false,
            newpos, m_rotation, i->m_offset, i->m_rotation, i->m_scale ) )
            hasdata = true;

    }

    return hasdata;
}
