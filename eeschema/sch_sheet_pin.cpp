/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2006 Jean-Pierre Charras, jaen-pierre.charras@gipsa-lab.inpg.com
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

#include <bitmaps.h>
#include <general.h>
#include <geometry/shape_line_chain.h>
#include <string_utils.h>
#include <plotters/plotter.h>
#include <sch_draw_panel.h>
#include <sch_edit_frame.h>
#include <sch_sheet.h>
#include <sch_sheet_pin.h>
#include <sch_painter.h>
#include <schematic.h>
#include <trigo.h>


SCH_SHEET_PIN::SCH_SHEET_PIN( SCH_SHEET* parent, const VECTOR2I& pos, const wxString& text ) :
    SCH_HIERLABEL( pos, text, SCH_SHEET_PIN_T ),
    m_edge( SHEET_SIDE::UNDEFINED )
{
    SetParent( parent );
    wxASSERT( parent );
    m_layer = LAYER_SHEETLABEL;

    SetTextPos( pos );

    if( parent->IsVerticalOrientation() )
        SetSide( SHEET_SIDE::TOP );
    else
        SetSide( SHEET_SIDE::LEFT );

    m_shape      = LABEL_FLAG_SHAPE::L_INPUT;
    m_isDangling = true;
    m_number     = 2;
}


EDA_ITEM* SCH_SHEET_PIN::Clone() const
{
    return new SCH_SHEET_PIN( *this );
}


void SCH_SHEET_PIN::swapData( SCH_ITEM* aItem )
{
    SCH_HIERLABEL::swapData( aItem );

    wxCHECK_RET( aItem->Type() == SCH_SHEET_PIN_T,
                 wxString::Format( "SCH_SHEET_PIN object cannot swap data with %s object.",
                                   aItem->GetClass() ) );

    SCH_SHEET_PIN* pin = static_cast<SCH_SHEET_PIN*>( aItem );

    std::swap( m_number, pin->m_number );
    std::swap( m_edge, pin->m_edge );
}


bool SCH_SHEET_PIN::operator==( const SCH_SHEET_PIN* aPin ) const
{
    return aPin == this;
}


int SCH_SHEET_PIN::GetPenWidth() const
{
    if( Schematic() )
        return Schematic()->Settings().m_DefaultLineWidth;

    return schIUScale.MilsToIU( DEFAULT_LINE_WIDTH_MILS );
}


void SCH_SHEET_PIN::SetNumber( int aNumber )
{
    wxASSERT( aNumber >= 2 );

    m_number = aNumber;
}


void SCH_SHEET_PIN::SetSide( SHEET_SIDE aEdge )
{
    SCH_SHEET* sheet = GetParent();

    // use SHEET_UNDEFINED_SIDE to adjust text orientation without changing edge

    switch( aEdge )
    {
    case SHEET_SIDE::LEFT:
        m_edge = aEdge;
        SetTextX( sheet->m_pos.x );
        SetSpinStyle( SPIN_STYLE::RIGHT ); // Orientation horiz inverse
        break;

    case SHEET_SIDE::RIGHT:
        m_edge = aEdge;
        SetTextX( sheet->m_pos.x + sheet->m_size.x );
        SetSpinStyle( SPIN_STYLE::LEFT ); // Orientation horiz normal
        break;

    case SHEET_SIDE::TOP:
        m_edge = aEdge;
        SetTextY( sheet->m_pos.y );
        SetSpinStyle( SPIN_STYLE::BOTTOM ); // Orientation vert BOTTOM
        break;

    case SHEET_SIDE::BOTTOM:
        m_edge = aEdge;
        SetTextY( sheet->m_pos.y + sheet->m_size.y );
        SetSpinStyle( SPIN_STYLE::UP ); // Orientation vert UP
        break;

    default:
        break;
    }
}


enum SHEET_SIDE SCH_SHEET_PIN::GetSide() const
{
    return m_edge;
}


void SCH_SHEET_PIN::ConstrainOnEdge( const VECTOR2I& aPos, bool aAllowEdgeSwitch )
{
    SCH_SHEET* sheet = GetParent();

    if( sheet == nullptr )
        return;

    int leftSide  = sheet->m_pos.x;
    int rightSide = sheet->m_pos.x + sheet->m_size.x;
    int topSide   = sheet->m_pos.y;
    int botSide   = sheet->m_pos.y + sheet->m_size.y;

    SHAPE_LINE_CHAIN sheetEdge;

    sheetEdge.Append( leftSide,  topSide );
    sheetEdge.Append( rightSide, topSide );
    sheetEdge.Append( rightSide, botSide );
    sheetEdge.Append( leftSide,  botSide );
    sheetEdge.Append( leftSide,  topSide );

    if( aAllowEdgeSwitch )
    {
        switch( sheetEdge.NearestSegment( aPos ) )
        {
        case 0:  SetSide( SHEET_SIDE::TOP );    break;
        case 1:  SetSide( SHEET_SIDE::RIGHT );  break;
        case 2:  SetSide( SHEET_SIDE::BOTTOM ); break;
        case 3:  SetSide( SHEET_SIDE::LEFT );   break;
        default: wxASSERT( "Invalid segment number" );
        }
    }
    else
    {
        SetSide( GetSide() );
    }

    switch( GetSide() )
    {
    case SHEET_SIDE::RIGHT:
    case SHEET_SIDE::LEFT:
        SetTextY( aPos.y );

        if( GetTextPos().y < topSide )
            SetTextY( topSide );

        if( GetTextPos().y > botSide )
            SetTextY( botSide );

        break;

    case SHEET_SIDE::BOTTOM:
    case SHEET_SIDE::TOP:
        SetTextX( aPos.x );

        if( GetTextPos().x < leftSide )
            SetTextX( leftSide );

        if( GetTextPos().x > rightSide )
            SetTextX( rightSide );

        break;

    case SHEET_SIDE::UNDEFINED:
        wxASSERT( "Undefined sheet side" );
    }
}


void SCH_SHEET_PIN::MirrorVertically( int aCenter )
{
    int p = GetTextPos().y - aCenter;

    SetTextY( aCenter - p );

    switch( m_edge )
    {
    case SHEET_SIDE::TOP: SetSide( SHEET_SIDE::BOTTOM ); break;
    case SHEET_SIDE::BOTTOM: SetSide( SHEET_SIDE::TOP ); break;
    default: break;
    }
}


void SCH_SHEET_PIN::MirrorHorizontally( int aCenter )
{
    int p = GetTextPos().x - aCenter;

    SetTextX( aCenter - p );

    switch( m_edge )
    {
    case SHEET_SIDE::LEFT: SetSide( SHEET_SIDE::RIGHT ); break;
    case SHEET_SIDE::RIGHT: SetSide( SHEET_SIDE::LEFT ); break;
    default: break;
    }
}


void SCH_SHEET_PIN::Rotate( const VECTOR2I& aCenter, bool aRotateCCW )
{
    VECTOR2I pt = GetTextPos();
    VECTOR2I delta = pt - aCenter;

    RotatePoint( pt, aCenter, aRotateCCW ? ANGLE_90 : ANGLE_270 );

    SHEET_SIDE oldSide = GetSide();
    ConstrainOnEdge( pt, true );

    // If the new side is the same as the old side, instead mirror across the center of that side.
    if( GetSide() == oldSide )
    {
        switch( GetSide() )
        {
        case SHEET_SIDE::TOP:
        case SHEET_SIDE::BOTTOM:
            SetTextPos( VECTOR2I( aCenter.x - delta.x, GetTextPos().y ) );
            break;

        case SHEET_SIDE::LEFT:
        case SHEET_SIDE::RIGHT:
            SetTextPos( VECTOR2I( GetTextPos().x, aCenter.y - delta.y ) );
            break;

        default:
            break;
        }
    }
    // If the new side is opposite to the old side, instead mirror across the center of an adjacent
    // side.
    else if( GetSide() == GetOppositeSide( oldSide ) )
    {
        switch( GetSide() )
        {
        case SHEET_SIDE::TOP:
        case SHEET_SIDE::BOTTOM:
            SetTextPos( VECTOR2I( aCenter.x + delta.x, GetTextPos().y ) );
            break;

        case SHEET_SIDE::LEFT:
        case SHEET_SIDE::RIGHT:
            SetTextPos( VECTOR2I( GetTextPos().x, aCenter.y + delta.y ) );
            break;

        default:
            break;
        }
    }
}


void SCH_SHEET_PIN::CreateGraphicShape( const RENDER_SETTINGS* aSettings,
                                        std::vector<VECTOR2I>& aPoints, const VECTOR2I& aPos ) const
{
    /*
     * These are the same icon shapes as SCH_HIERLABEL but the graphic icon is slightly
     * different in 2 cases:
     * for INPUT type the icon is the OUTPUT shape of SCH_HIERLABEL
     * for OUTPUT type the icon is the INPUT shape of SCH_HIERLABEL
     */
    LABEL_FLAG_SHAPE shape = m_shape;

    switch( shape )
    {
    case LABEL_FLAG_SHAPE::L_INPUT:  shape = LABEL_FLAG_SHAPE::L_OUTPUT; break;
    case LABEL_FLAG_SHAPE::L_OUTPUT: shape = LABEL_FLAG_SHAPE::L_INPUT;  break;
    default:                                                             break;
    }

    SCH_HIERLABEL::CreateGraphicShape( aSettings, aPoints, aPos, shape );
}


void SCH_SHEET_PIN::GetEndPoints( std::vector<DANGLING_END_ITEM>& aItemList )
{
    DANGLING_END_ITEM item( SHEET_LABEL_END, this, GetTextPos() );
    aItemList.push_back( item );
}


wxString SCH_SHEET_PIN::GetItemDescription( UNITS_PROVIDER* aUnitsProvider, bool aFull ) const
{
    return wxString::Format( _( "Hierarchical Sheet Pin '%s'" ),
                             aFull ? GetShownText( false ) : KIUI::EllipsizeMenuText( GetText() ) );
}


BITMAPS SCH_SHEET_PIN::GetMenuImage() const
{
    return BITMAPS::add_hierar_pin;
}


bool SCH_SHEET_PIN::HitTest( const VECTOR2I& aPoint, int aAccuracy ) const
{
    BOX2I rect = GetBoundingBox();

    rect.Inflate( aAccuracy );

    return rect.Contains( aPoint );
}


bool SCH_SHEET_PIN::operator==( const SCH_ITEM& aOther ) const
{
    if( aOther.Type() != Type() )
        return false;

    const SCH_SHEET_PIN* other = static_cast<const SCH_SHEET_PIN*>( &aOther );

    return m_edge == other->m_edge && m_number == other->m_number
           && SCH_HIERLABEL::operator==( aOther );
}


double SCH_SHEET_PIN::Similarity( const SCH_ITEM& aOther ) const
{
    if( aOther.Type() != Type() )
        return 0.0;

    const SCH_SHEET_PIN* other = static_cast<const SCH_SHEET_PIN*>( &aOther );

    double similarity = 1.0;

    if( m_edge != other->m_edge )
        similarity *= 0.9;

    if( m_number != other->m_number )
        similarity *= 0.9;

    similarity *= SCH_HIERLABEL::Similarity( aOther );

    return similarity;
}


bool SCH_SHEET_PIN::HasConnectivityChanges( const SCH_ITEM* aItem,
                                            const SCH_SHEET_PATH* aInstance ) const
{
    // Do not compare to ourself.
    if( aItem == this )
        return false;

    const SCH_SHEET_PIN* pin = dynamic_cast<const SCH_SHEET_PIN*>( aItem );

    // Don't compare against a different SCH_ITEM.
    wxCHECK( pin, false );

    if( GetPosition() != pin->GetPosition() )
        return true;

    return GetText() != pin->GetText();
}


#if defined(DEBUG)

void SCH_SHEET_PIN::Show( int nestLevel, std::ostream& os ) const
{
    // XML output:
    NestedSpace( nestLevel, os ) << '<' << GetClass().Lower().mb_str() << ">"
                                 << " pin_name=\"" << TO_UTF8( GetText() )
                                 << '"' << "/>\n" << std::flush;
}

#endif


static struct SCH_SHEET_PIN_DESC
{
    SCH_SHEET_PIN_DESC()
    {
        PROPERTY_MANAGER& propMgr = PROPERTY_MANAGER::Instance();
        REGISTER_TYPE( SCH_SHEET_PIN );
        propMgr.AddTypeCast( new TYPE_CAST<SCH_SHEET_PIN, SCH_HIERLABEL> );
        propMgr.AddTypeCast( new TYPE_CAST<SCH_SHEET_PIN, SCH_LABEL_BASE> );
        propMgr.AddTypeCast( new TYPE_CAST<SCH_SHEET_PIN, SCH_TEXT> );
        propMgr.AddTypeCast( new TYPE_CAST<SCH_SHEET_PIN, EDA_TEXT> );

        propMgr.InheritsAfter( TYPE_HASH( SCH_SHEET_PIN ), TYPE_HASH( SCH_HIERLABEL ) );
    }
} _SCH_SHEET_PIN_DESC;
