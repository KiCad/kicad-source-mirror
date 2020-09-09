/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2012 Jean-Pierre Charras, jean-pierre.charras@ujf-grenoble.fr
 * Copyright (C) 2012 SoftPLC Corporation, Dick Hollenbeck <dick@softplc.com>
 * Copyright (C) 2012 Wayne Stambaugh <stambaughw@verizon.net>
 * Copyright (C) 1992-2012 KiCad Developers, see AUTHORS.txt for contributors.
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

#include <fctsys.h>
#include <gr_basic.h>
#include <bitmaps.h>
#include <pcb_edit_frame.h>
#include <base_units.h>
#include <class_board.h>
#include <class_dimension.h>
#include <class_pcb_text.h>
#include <settings/color_settings.h>
#include <settings/settings_manager.h>


DIMENSION::DIMENSION( BOARD_ITEM* aParent )
        : BOARD_ITEM( aParent, PCB_DIMENSION_T ),
          m_units( EDA_UNITS::INCHES ),
          m_useMils( false ),
          m_autoUnits( false ),
          m_unitsFormat( DIM_UNITS_FORMAT::BARE_SUFFIX ),
          m_precision( 4 ),
          m_suppressZeroes( false ),
          m_lineThickness( Millimeter2iu( 0.2 ) ),
          m_arrowLength( Mils2iu( 50 ) ),
          m_extensionOffset( 0 ),
          m_textPosition( DIM_TEXT_POSITION::OUTSIDE ),
          m_keepTextAligned( true ),
          m_text( this ),
          m_measuredValue( 0 )
{
    m_Layer = Dwgs_User;
}


void DIMENSION::SetPosition( const wxPoint& aPos )
{
    m_text.SetTextPos( aPos );
}


wxPoint DIMENSION::GetPosition() const
{
    return m_text.GetTextPos();
}


DIM_UNITS_MODE DIMENSION::GetUnitsMode() const
{
    if( m_autoUnits )
        return DIM_UNITS_MODE::AUTOMATIC;
    else if( m_units == EDA_UNITS::MILLIMETRES )
        return DIM_UNITS_MODE::MILLIMETRES;
    else if( m_useMils )
        return DIM_UNITS_MODE::MILS;
    else
        return DIM_UNITS_MODE::INCHES;
}


void DIMENSION::SetUnitsMode( DIM_UNITS_MODE aMode )
{
    m_units     = EDA_UNITS::INCHES;
    m_autoUnits = false;
    m_useMils   = false;

    switch( aMode )
    {
    case DIM_UNITS_MODE::INCHES:
        break;

    case DIM_UNITS_MODE::MILS:
        m_useMils = true;
        break;

    case DIM_UNITS_MODE::MILLIMETRES:
        m_units = EDA_UNITS::MILLIMETRES;
        break;

    case DIM_UNITS_MODE::AUTOMATIC:
        m_autoUnits = true;
        break;
    }
}


void DIMENSION::SetText( const wxString& aNewText )
{
    m_text.SetText( aNewText );
}


const wxString DIMENSION::GetText() const
{
    return m_text.GetText();
}


void DIMENSION::SetLayer( PCB_LAYER_ID aLayer )
{
    m_Layer = aLayer;
    m_text.SetLayer( aLayer );
}


void DIMENSION::Move( const wxPoint& offset )
{
    m_text.Offset( offset );

    m_start += offset;
    m_end   += offset;

    updateGeometry();
}


void DIMENSION::Rotate( const wxPoint& aRotCentre, double aAngle )
{
    wxPoint tmp = m_text.GetTextPos();
    RotatePoint( &tmp, aRotCentre, aAngle );
    m_text.SetTextPos( tmp );

    double newAngle = m_text.GetTextAngle() + aAngle;

    if( newAngle >= 3600 )
        newAngle -= 3600;

    if( newAngle > 900  &&  newAngle < 2700 )
        newAngle -= 1800;

    m_text.SetTextAngle( newAngle );

    RotatePoint( &m_start, aRotCentre, aAngle );
    RotatePoint( &m_end, aRotCentre, aAngle );

    updateGeometry();
}


void DIMENSION::Flip( const wxPoint& aCentre, bool aFlipLeftRight )
{
    Mirror( aCentre );

    // DIMENSION items are not usually on copper layers, so
    // copper layers count is not taken in accoun in Flip transform
    SetLayer( FlipLayer( GetLayer() ) );
}


void DIMENSION::Mirror( const wxPoint& axis_pos, bool aMirrorLeftRight )
{
    int axis = aMirrorLeftRight ? axis_pos.x : axis_pos.y;
    wxPoint newPos = m_text.GetTextPos();

#define INVERT( pos ) ( ( pos ) = axis - ( ( pos ) - axis ) )

    if( aMirrorLeftRight )
        INVERT( newPos.x );
    else
        INVERT( newPos.y );

    m_text.SetTextPos( newPos );

    // invert angle
    m_text.SetTextAngle( -m_text.GetTextAngle() );

    if( aMirrorLeftRight )
    {
        INVERT( m_start.x );
        INVERT( m_end.x );
    }
    else
    {
        INVERT( m_start.y );
        INVERT( m_end.y );
    }

    updateGeometry();
}


void DIMENSION::SetStart( const wxPoint& aOrigin )
{
    m_start = aOrigin;
    updateGeometry();
}


void DIMENSION::SetEnd( const wxPoint& aEnd )
{
    m_end = aEnd;
    updateGeometry();
}


void DIMENSION::GetMsgPanelInfo( EDA_DRAW_FRAME* aFrame, std::vector<MSG_PANEL_ITEM>& aList )
{
    // for now, display only the text within the DIMENSION using class TEXTE_PCB.
    m_text.GetMsgPanelInfo( aFrame, aList );
}


bool DIMENSION::HitTest( const wxPoint& aPosition, int aAccuracy ) const
{
    if( m_text.TextHitTest( aPosition ) )
        return true;

    int dist_max = aAccuracy + ( m_lineThickness / 2 );

    // Locate SEGMENTS

    for( const SEG& seg : GetLines() )
    {
        if( TestSegmentHit( aPosition, wxPoint( seg.A ), wxPoint( seg.B ), dist_max ) )
            return true;
    }

    return false;
}


bool DIMENSION::HitTest( const EDA_RECT& aRect, bool aContained, int aAccuracy ) const
{
    EDA_RECT arect = aRect;
    arect.Inflate( aAccuracy );

    EDA_RECT rect = GetBoundingBox();
    if( aAccuracy )
        rect.Inflate( aAccuracy );

    if( aContained )
        return arect.Contains( rect );

    return arect.Intersects( rect );
}


const EDA_RECT DIMENSION::GetBoundingBox() const
{
    EDA_RECT    bBox;
    int         xmin, xmax, ymin, ymax;

    bBox    = m_text.GetTextBox();
    xmin    = bBox.GetX();
    xmax    = bBox.GetRight();
    ymin    = bBox.GetY();
    ymax    = bBox.GetBottom();

    for( const SEG& seg : GetLines() )
    {
        xmin = std::min( xmin, seg.A.x );
        xmin = std::min( xmin, seg.B.x );
        xmax = std::max( xmax, seg.A.x );
        xmax = std::max( xmax, seg.B.x );
        ymin = std::min( ymin, seg.A.y );
        ymin = std::min( ymin, seg.B.y );
        ymax = std::max( ymax, seg.A.y );
        ymax = std::max( ymax, seg.B.y );
    }

    bBox.SetX( xmin );
    bBox.SetY( ymin );
    bBox.SetWidth( xmax - xmin + 1 );
    bBox.SetHeight( ymax - ymin + 1 );

    bBox.Normalize();

    return bBox;
}


wxString DIMENSION::GetSelectMenuText( EDA_UNITS aUnits ) const
{
    return wxString::Format( _( "Dimension \"%s\" on %s" ), GetText(), GetLayerName() );
}


BITMAP_DEF DIMENSION::GetMenuImage() const
{
    return add_dimension_xpm;
}


const BOX2I DIMENSION::ViewBBox() const
{
    BOX2I dimBBox = BOX2I( VECTOR2I( GetBoundingBox().GetPosition() ),
                           VECTOR2I( GetBoundingBox().GetSize() ) );
    dimBBox.Merge( m_text.ViewBBox() );

    return dimBBox;
}


static struct DIMENSION_DESC
{
    DIMENSION_DESC()
    {
        PROPERTY_MANAGER& propMgr = PROPERTY_MANAGER::Instance();
        REGISTER_TYPE( DIMENSION );
        propMgr.InheritsAfter( TYPE_HASH( DIMENSION ), TYPE_HASH( BOARD_ITEM ) );
        //propMgr.AddProperty( new PROPERTY<DIMENSION, int>( "Height",
                    //&DIMENSION::SetHeight, &DIMENSION::GetHeight, PROPERTY_DISPLAY::DISTANCE ) );
    }
} _DIMENSION_DESC;


ALIGNED_DIMENSION::ALIGNED_DIMENSION( BOARD_ITEM* aParent ) :
        DIMENSION( aParent ),
        m_height( 0 )
{
    // To preserve look of old dimensions, initialize extension height based on default arrow length
    m_extensionHeight = m_arrowLength * std::sin( DEG2RAD( s_arrowAngle ) );
}


EDA_ITEM* ALIGNED_DIMENSION::Clone() const
{
    return new ALIGNED_DIMENSION( *this );
}


void ALIGNED_DIMENSION::SwapData( BOARD_ITEM* aImage )
{
    assert( aImage->Type() == PCB_DIMENSION_T );

    std::swap( *static_cast<ALIGNED_DIMENSION*>( this ), *static_cast<ALIGNED_DIMENSION*>( aImage ) );
}


void ALIGNED_DIMENSION::SetHeight( int aHeight )
{
    m_height = aHeight;
    updateGeometry();
}


void ALIGNED_DIMENSION::UpdateHeight( const wxPoint& aCrossbarStart, const wxPoint& aCrossbarEnd )
{
    VECTOR2D height( aCrossbarStart - GetStart() );
    VECTOR2D crossBar( aCrossbarEnd - aCrossbarStart );

    if( height.Cross( crossBar ) > 0 )
        m_height = -height.EuclideanNorm();
    else
        m_height = height.EuclideanNorm();

    updateGeometry();
}


void ALIGNED_DIMENSION::updateGeometry()
{
    m_lines.clear();

    VECTOR2I dimension( m_end - m_start );

    m_measuredValue = KiROUND( dimension.EuclideanNorm() );

    VECTOR2I extension;

    if( m_height > 0 )
        extension = VECTOR2I( -dimension.y, dimension.x );
    else
        extension = VECTOR2I( dimension.y, -dimension.x );

    // Add extension lines
    int extensionHeight = std::abs( m_height ) - m_extensionOffset + m_extensionHeight;

    VECTOR2I extensionStart( m_start );
    extensionStart += extension.Resize( m_extensionOffset );

    m_lines.emplace_back( SEG( extensionStart,
                               extensionStart + extension.Resize( extensionHeight ) ) );

    extensionStart = VECTOR2I( m_end );
    extensionStart += extension.Resize( m_extensionOffset );

    m_lines.emplace_back( SEG( extensionStart,
                               extensionStart + extension.Resize( extensionHeight ) ) );

    // Add crossbar
    VECTOR2I crossBarDistance = sign( m_height ) * extension.Resize( m_height );
    m_crossBarStart = m_start + wxPoint( crossBarDistance );
    m_crossBarEnd   = m_end + wxPoint( crossBarDistance );

    // Update text after calculating crossbar position but before adding crossbar lines
    updateText();

    // Now that we have the text updated, we can determine if the crossbar needs to be broken
    if( m_textPosition == DIM_TEXT_POSITION::INLINE )
    {

    }

    m_lines.emplace_back( SEG( m_crossBarStart, m_crossBarEnd ) );

    // Add arrows
    VECTOR2I arrowEnd( m_arrowLength, 0 );

    double arrowRotPos = dimension.Angle() + DEG2RAD( s_arrowAngle );
    double arrowRotNeg = dimension.Angle() - DEG2RAD( s_arrowAngle );

    m_lines.emplace_back( SEG( m_crossBarStart,
                               m_crossBarStart + wxPoint( arrowEnd.Rotate( arrowRotPos ) ) ) );

    m_lines.emplace_back( SEG( m_crossBarStart,
                               m_crossBarStart + wxPoint( arrowEnd.Rotate( arrowRotNeg ) ) ) );

    m_lines.emplace_back( SEG( m_crossBarEnd,
                               m_crossBarEnd - wxPoint( arrowEnd.Rotate( arrowRotPos ) ) ) );

    m_lines.emplace_back( SEG( m_crossBarEnd,
                               m_crossBarEnd - wxPoint( arrowEnd.Rotate( arrowRotNeg ) ) ) );
}


void ALIGNED_DIMENSION::updateText()
{
    VECTOR2I crossbarCenter( ( m_crossBarEnd - m_crossBarStart ) / 2 );

    if( m_textPosition != DIM_TEXT_POSITION::MANUAL )
    {
        int textOffsetDistance = m_text.GetEffectiveTextPenWidth() + m_text.GetTextHeight();

        double rotation = std::copysign( DEG2RAD( 90 ), m_height );
        VECTOR2I textOffset = crossbarCenter.Rotate( rotation ).Resize( textOffsetDistance );
        textOffset += crossbarCenter;

        m_text.SetTextPos( m_crossBarStart + wxPoint( textOffset ) );
    }

    if( m_keepTextAligned )
    {
        double textAngle = 3600 - RAD2DECIDEG( crossbarCenter.Angle() );

        NORMALIZE_ANGLE_POS( textAngle );

        if( textAngle > 900 && textAngle < 2700 )
            textAngle -= 1800;

        m_text.SetTextAngle( textAngle );
    }
    else
    {
        m_text.SetTextAngle( 0 );
    }

    wxString text;
    wxString format = wxT( "%." ) + wxString::Format( "%i", m_precision ) + wxT( "f" );

    text.Printf( format, To_User_Unit( m_units, m_measuredValue, m_useMils ) );
    text += " ";
    text += GetAbbreviatedUnitsLabel( m_units, m_useMils );

    SetText( text );
}
