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
#include "kicadmodel.h"


KICADMODEL::KICADMODEL() : m_scale( 1.0, 1.0, 1.0 )
{
    return;
}


KICADMODEL::~KICADMODEL()
{
    return;
}


bool KICADMODEL::Read( SEXPR::SEXPR* aEntry )
{
    // form: ( pad N thru_hole shape (at x y {r}) (size x y) (drill {oval} x {y}) (layers X X X) )
    int nchild = aEntry->GetNumberOfChildren();

    if( nchild < 2 )
    {
        std::ostringstream ostr;
        ostr << "* invalid model entry";
        wxLogMessage( "%s\n", ostr.str().c_str() );
        return false;
    }

    SEXPR::SEXPR* child = aEntry->GetChild( 1 );

    if( child->IsSymbol() )
        m_modelname = child->GetSymbol();
    else if( child->IsString() )
        m_modelname = child->GetString();
    else
    {
        std::ostringstream ostr;
        ostr << "* invalid model entry; invalid path";
        wxLogMessage( "%s\n", ostr.str().c_str() );
        return false;
    }

    for( int i = 2; i < nchild; ++i )
    {
        child = aEntry->GetChild( i );

        if( !child->IsList() )
            continue;

        std::string name = child->GetChild( 0 )->GetSymbol();
        bool ret = true;

        if( name == "at" )
            ret = Get3DCoordinate( child->GetChild( 1 ), m_offset );
        else if( name == "scale" )
            ret = Get3DCoordinate( child->GetChild( 1 ), m_scale );
        else if( name == "rotate" )
            ret = GetXYZRotation( child->GetChild( 1 ), m_rotation );

        if( !ret )
            return false;
    }

    return true;
}
