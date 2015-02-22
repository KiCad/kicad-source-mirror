/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2006 Jean-Pierre Charras, jaen-pierre.charras@gipsa-lab.inpg.com
 * Copyright (C) 1992-2011 KiCad Developers, see AUTHORS.txt for contributors.
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

/**
 * @file sch_item_struct.cpp
 */

#include <fctsys.h>
#include <common.h>
#include <gr_basic.h>
#include <base_struct.h>
#include <sch_item_struct.h>
#include <class_sch_screen.h>
#include <class_drawpanel.h>
#include <schframe.h>

#include <general.h>


const wxString traceFindItem( wxT( "KicadFindItem" ) );


bool sort_schematic_items( const SCH_ITEM* aItem1, const SCH_ITEM* aItem2 )
{
    return *aItem1 < *aItem2;
}


/* Constructor and destructor for SCH_ITEM */
/* They are not inline because this creates problems with gcc at linking time
 * in debug mode
 */

SCH_ITEM::SCH_ITEM( EDA_ITEM* aParent, KICAD_T aType ) :
    EDA_ITEM( aParent, aType )
{
    m_Layer = LAYER_WIRE; // It's only a default, in fact
}


SCH_ITEM::SCH_ITEM( const SCH_ITEM& aItem ) :
    EDA_ITEM( aItem )
{
    m_Layer = aItem.m_Layer;
}


SCH_ITEM::~SCH_ITEM()
{
    // Do not let the connections container go out of scope with any objects or they
    // will be deleted by the container will cause the Eeschema to crash.  These objects
    // are owned by the sheet object container.
    if( !m_connections.empty() )
        m_connections.clear();
}


bool SCH_ITEM::IsConnected( const wxPoint& aPosition ) const
{
    if( m_Flags & STRUCT_DELETED || m_Flags & SKIP_STRUCT )
        return false;

    return doIsConnected( aPosition );
}


void SCH_ITEM::SwapData( SCH_ITEM* aItem )
{
    wxFAIL_MSG( wxT( "SwapData() method not implemented for class " ) + GetClass() );
}


bool SCH_ITEM::operator < ( const SCH_ITEM& aItem ) const
{
    wxCHECK_MSG( false, this->Type() < aItem.Type(),
                 wxT( "Less than operator not defined for " ) + GetClass() );
}


void SCH_ITEM::Plot( PLOTTER* aPlotter )
{
    wxFAIL_MSG( wxT( "Plot() method not implemented for class " ) + GetClass() );
}


std::string SCH_ITEM::FormatInternalUnits( int aValue )
{
    char    buf[50];
    double  engUnits = aValue;
    int     len;

    if( engUnits != 0.0 && fabs( engUnits ) <= 0.0001 )
    {
        // printf( "f: " );
        len = snprintf( buf, 49, "%.10f", engUnits );

        while( --len > 0 && buf[len] == '0' )
            buf[len] = '\0';

        ++len;
    }
    else
    {
        // printf( "g: " );
        len = snprintf( buf, 49, "%.10g", engUnits );
    }

    return std::string( buf, len );
}


std::string SCH_ITEM::FormatAngle( double aAngle )
{
    char temp[50];
    int len;

    len = snprintf( temp, 49, "%.10g", aAngle / 10.0 );

    return std::string( temp, len );
}


std::string SCH_ITEM::FormatInternalUnits( const wxPoint& aPoint )
{
    return FormatInternalUnits( aPoint.x ) + " " + FormatInternalUnits( aPoint.y );
}


std::string SCH_ITEM::FormatInternalUnits( const wxSize& aSize )
{
    return FormatInternalUnits( aSize.GetWidth() ) + " " + FormatInternalUnits( aSize.GetHeight() );
}
