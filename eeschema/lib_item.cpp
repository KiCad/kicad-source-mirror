/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2015 Jean-Pierre Charras, jaen-pierre.charras at wanadoo.fr
 * Copyright (C) 2015 Wayne Stambaugh <stambaughw@verizon.net>
 * Copyright (C) 2004-2019 KiCad Developers, see change_log.txt for contributors.
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

#include <sch_draw_panel.h>
#include <widgets/msgpanel.h>

#include <general.h>
#include <lib_item.h>

const int fill_tab[3] = { 'N', 'F', 'f' };


LIB_ITEM::LIB_ITEM( KICAD_T        aType,
                    LIB_PART*      aComponent,
                    int            aUnit,
                    int            aConvert,
                    FILL_TYPE      aFillType ) :
    EDA_ITEM( aType )
{
    m_unit              = aUnit;
    m_convert           = aConvert;
    m_fill              = aFillType;
    m_parent            = (EDA_ITEM*) aComponent;
    m_isFillable        = false;
}


void LIB_ITEM::GetMsgPanelInfo( EDA_DRAW_FRAME* aFrame, MSG_PANEL_ITEMS& aList )
{
    wxString msg;

    aList.push_back( MSG_PANEL_ITEM( _( "Type" ), GetTypeName() ) );

    if( m_unit == 0 )
        msg = _( "All" );
    else
        msg.Printf( wxT( "%d" ), m_unit );

    aList.push_back( MSG_PANEL_ITEM( _( "Unit" ), msg ) );

    if( m_convert == LIB_ITEM::LIB_CONVERT::BASE )
        msg = _( "no" );
    else if( m_convert == LIB_ITEM::LIB_CONVERT::DEMORGAN )
        msg = _( "yes" );
    else
        msg = wxT( "?" );

    aList.push_back( MSG_PANEL_ITEM( _( "Converted" ), msg ) );
}


int LIB_ITEM::compare( const LIB_ITEM& aOther, LIB_ITEM::COMPARE_FLAGS aCompareFlags ) const
{
    if( Type() != aOther.Type() )
        return Type() - aOther.Type();

    // When comparing unit LIB_ITEM objects, we ignore the unit number.
    if( !( aCompareFlags & COMPARE_FLAGS::UNIT ) && m_unit != aOther.m_unit )
        return m_unit - aOther.m_unit;

    if( !( aCompareFlags & COMPARE_FLAGS::UNIT ) && m_convert != aOther.m_convert )
        return m_convert - aOther.m_convert;

    if( m_fill != aOther.m_fill )
        return static_cast<int>( m_fill ) - static_cast<int>( aOther.m_fill );

    return 0;
}


bool LIB_ITEM::operator==( const LIB_ITEM& aOther ) const
{
    return compare( aOther ) == 0;
}


bool LIB_ITEM::operator<( const LIB_ITEM& aOther ) const
{
    int result = m_convert - aOther.m_convert;

    if( result != 0 )
        return result < 0;

    result = m_unit - aOther.m_unit;

    if( result != 0 )
        return result < 0;

    result = Type() - aOther.Type();

    if( result != 0 )
        return result < 0;

    return ( compare( aOther ) < 0 );
}


bool LIB_ITEM::HitTest( const EDA_RECT& aRect, bool aContained, int aAccuracy ) const
{
    if( m_flags & (STRUCT_DELETED | SKIP_STRUCT ) )
        return false;

    EDA_RECT sel = aRect;

    if ( aAccuracy )
        sel.Inflate( aAccuracy );

    if( aContained )
        return sel.Contains( GetBoundingBox() );

    return sel.Intersects( GetBoundingBox() );
}


void LIB_ITEM::Print( const RENDER_SETTINGS* aSettings, const wxPoint& aOffset,
                      void* aData, const TRANSFORM& aTransform )
{
    print( aSettings, aOffset, aData, aTransform );
}


void LIB_ITEM::ViewGetLayers( int aLayers[], int& aCount ) const
{
    // Basic fallback
    aCount      = 3;
    aLayers[0]  = LAYER_DEVICE;
    aLayers[1]  = LAYER_DEVICE_BACKGROUND;
    aLayers[2]  = LAYER_SELECTION_SHADOWS;
}


