/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2004 Jean-Pierre Charras, jaen-pierre.charras@gipsa-lab.inpg.com
 * Copyright (C) 2008-2011 Wayne Stambaugh <stambaughw@verizon.net>
 * Copyright (C) 2004-2011 KiCad Developers, see change_log.txt for contributors.
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
 * @file lib_draw_item.cpp
 */

#include "fctsys.h"
#include "gr_basic.h"
#include "class_drawpanel.h"
#include "wxstruct.h"

#include "protos.h"
#include "general.h"
#include "lib_draw_item.h"

const int fill_tab[3] = { 'N', 'F', 'f' };

//#define DRAW_ARC_WITH_ANGLE       // Used to draw arcs


LIB_ITEM::LIB_ITEM( KICAD_T        aType,
                    LIB_COMPONENT* aComponent,
                    int            aUnit,
                    int            aConvert,
                    FILL_T         aFillType ) :
    EDA_ITEM( aType )
{
    m_Unit              = aUnit;
    m_Convert           = aConvert;
    m_Fill              = aFillType;
    m_Parent            = (EDA_ITEM*) aComponent;
    m_typeName          = _( "Undefined" );
    m_isFillable        = false;
    m_eraseLastDrawItem = false;
}


LIB_ITEM::LIB_ITEM( const LIB_ITEM& aItem ) :
    EDA_ITEM( aItem )
{
    m_Unit = aItem.m_Unit;
    m_Convert = aItem.m_Convert;
    m_Fill = aItem.m_Fill;
    m_typeName = aItem.m_typeName;
    m_isFillable = aItem.m_isFillable;
    m_eraseLastDrawItem = false;
}


void LIB_ITEM::DisplayInfo( EDA_DRAW_FRAME* aFrame )
{
    wxString msg;

    aFrame->ClearMsgPanel();
    aFrame->AppendMsgPanel( _( "Type" ), m_typeName, CYAN );

    if( m_Unit == 0 )
        msg = _( "All" );
    else
        msg.Printf( wxT( "%d" ), m_Unit );
    aFrame->AppendMsgPanel( _( "Unit" ), msg, BROWN );

    if( m_Convert == 0 )
        msg = _( "All" );
    else if( m_Convert == 1 )
        msg = _( "no" );
    else if( m_Convert == 2 )
        msg = _( "yes" );
    else
        msg = wxT( "?" );
    aFrame->AppendMsgPanel( _( "Convert" ), msg, BROWN );
}


bool LIB_ITEM::operator==( const LIB_ITEM& aOther ) const
{
    return ( ( Type() == aOther.Type() )
             && ( m_Unit == aOther.m_Unit )
             && ( m_Convert == aOther.m_Convert )
             && DoCompare( aOther ) == 0 );
}


bool LIB_ITEM::operator<( const LIB_ITEM& aOther ) const
{
    int result = m_Convert - aOther.m_Convert;

    if( result != 0 )
        return result < 0;

    result = m_Unit - aOther.m_Unit;

    if( result != 0 )
        return result < 0;

    result = Type() - aOther.Type();

    if( result != 0 )
        return result < 0;

    return ( DoCompare( aOther ) < 0 );
}


void LIB_ITEM::Draw( EDA_DRAW_PANEL* aPanel, wxDC* aDC, const wxPoint& aOffset, int aColor,
                     int aDrawMode, void* aData, const TRANSFORM& aTransform )
{
    if( InEditMode() )
    {
        // Temporarily disable filling while the item is being edited.
        FILL_T fillMode = m_Fill;
        int  color = GetDefaultColor();

        m_Fill = NO_FILL;

        // Erase the old items using the previous attributes.
        if( m_eraseLastDrawItem )
        {
            GRSetDrawMode( aDC, g_XorMode );
            drawEditGraphics( aPanel->GetClipBox(), aDC, color );
            drawGraphic( aPanel, aDC, wxPoint( 0, 0 ), color, g_XorMode, aData, aTransform );
        }

        // Calculate the new attributes at the current cursor position.
        calcEdit( aOffset );

        // Draw the items using the new attributes.
        drawEditGraphics( aPanel->GetClipBox(), aDC, color );
        drawGraphic( aPanel, aDC, wxPoint( 0, 0 ), color, g_XorMode, aData, aTransform );

        m_Fill = fillMode;
    }
    else
    {
        drawGraphic( aPanel, aDC, aOffset, aColor, aDrawMode, aData, aTransform );
    }
}


int LIB_ITEM::GetDefaultColor()
{
    return ReturnLayerColor( LAYER_DEVICE );
}
