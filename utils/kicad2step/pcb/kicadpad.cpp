/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2016 Cirilo Bernardo <cirilo.bernardo@gmail.com>
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

#include <wx/log.h>
#include <iostream>
#include <sstream>
#include "sexpr/sexpr.h"
#include "kicadpad.h"


static const char bad_pad[] = "* corrupt module in PCB file; bad pad";


KICADPAD::KICADPAD()
{
    m_rotation = 0.0;
    m_thruhole = false;
    m_drill.oval = false;
    return;
}


KICADPAD::~KICADPAD()
{
    return;
}


bool KICADPAD::Read( SEXPR::SEXPR* aEntry )
{
    // form: ( pad N thru_hole shape (at x y {r}) (size x y) (drill {oval} x {y}) (layers X X X) )
    int nchild = aEntry->GetNumberOfChildren();

    if( nchild < 2 )
    {
        std::ostringstream ostr;
        ostr << bad_pad << " (line " << aEntry->GetLineNumber() << ")";
        wxLogMessage( "%s\n", ostr.str().c_str() );
        return false;
    }

    SEXPR::SEXPR* child;

    for( int i = 1; i < nchild; ++i )
    {
        child = aEntry->GetChild( i );

        if( child->IsSymbol() &&
            ( child->GetSymbol() == "thru_hole" || child->GetSymbol() == "np_thru_hole" ) )
        {
            m_thruhole = true;
            continue;
        }

        if( child->IsList() )
        {
            std::string name = child->GetChild( 0 )->GetSymbol();
            bool ret = true;

            if( name == "drill" )
            {
                // ignore any drill info for SMD pads
                if( m_thruhole )
                    ret = parseDrill( child );
            }
            else if( name == "at" )
            {
                ret = Get2DPositionAndRotation( child, m_position, m_rotation );
            }

            if( !ret )
                return false;
        }
    }

    return true;
}


bool KICADPAD::parseDrill( SEXPR::SEXPR* aDrill )
{
    // form: (drill {oval} X {Y})
    const char bad_drill[] = "* corrupt module in PCB file; bad drill";
    int nchild = aDrill->GetNumberOfChildren();

    if( nchild < 2 )
    {
        std::ostringstream ostr;
        ostr << bad_drill << " (line " << aDrill->GetLineNumber() << ")";
        wxLogMessage( "%s\n", ostr.str().c_str() );
        return false;
    }

    SEXPR::SEXPR* child = aDrill->GetChild( 1 );
    int idx = 1;
    m_drill.oval = false;

    if( child->IsSymbol() )
    {
        if( child->GetSymbol() == "oval" && nchild >= 4 )
        {
            m_drill.oval = true;
            child = aDrill->GetChild( ++idx );
        }
        else
        {
            std::ostringstream ostr;
            ostr << bad_drill << " (line " << child->GetLineNumber();
            ostr << ") (unexpected symbol: ";
            ostr << child->GetSymbol() << "), nchild = " << nchild;
            wxLogMessage( "%s\n", ostr.str().c_str() );
            return false;
        }
    }

    double x;

    if( child->IsDouble() )
        x = child->GetDouble();
    else if( child->IsInteger() )
        x = (double) child->GetInteger();
    else
    {
        std::ostringstream ostr;
        ostr << bad_drill << " (line " << child->GetLineNumber();
        ostr << ") (did not find X size)";
        wxLogMessage( "%s\n", ostr.str().c_str() );
        return false;
    }

    m_drill.size.x = x;
    m_drill.size.y = x;

    if( ++idx == nchild || !m_drill.oval )
        return true;

    for( int i = idx; i < nchild; ++i )
    {
        child = aDrill->GetChild( i );

        // NOTE: the Offset of the copper pad is stored
        // in the drill string but since the copper is not
        // needed in the MCAD model the Offset is simply ignored.
        if( !child->IsList() )
        {
            double y;

            if( child->IsDouble() )
                y = child->GetDouble();
            else if( child->IsInteger() )
                y = (double) child->GetInteger();
            else
            {
                std::ostringstream ostr;
                ostr << bad_drill << " (line " << child->GetLineNumber();
                ostr << ") (did not find Y size)";
                wxLogMessage( "%s\n", ostr.str().c_str() );
                return false;
            }

            m_drill.size.y = y;
        }
    }

    return true;
}
