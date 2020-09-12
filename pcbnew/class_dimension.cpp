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


DIMENSION::DIMENSION( BOARD_ITEM* aParent ) :
        BOARD_ITEM( aParent, PCB_DIMENSION_T ),
        m_overrideTextEnabled( false ),
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
        m_text( aParent ),
        m_measuredValue( 0 )
{
    m_Layer = Dwgs_User;
}


void DIMENSION::SetParent( EDA_ITEM* aParent )
{
    BOARD_ITEM::SetParent( aParent );
    m_text.SetParent( aParent );
}


void DIMENSION::SetPosition( const wxPoint& aPos )
{
    m_text.SetTextPos( aPos );
}


wxPoint DIMENSION::GetPosition() const
{
    return m_text.GetTextPos();
}


void DIMENSION::updateText()
{
    wxString text = m_overrideTextEnabled ? m_valueString : GetValueText();

    switch( m_unitsFormat )
    {
    case DIM_UNITS_FORMAT::NO_SUFFIX: // no units
        break;

    case DIM_UNITS_FORMAT::BARE_SUFFIX: // normal
        text += " ";
        text += GetAbbreviatedUnitsLabel( m_units, m_useMils );
        break;

    case DIM_UNITS_FORMAT::PAREN_SUFFIX: // parenthetical
        text += " (";
        text += GetAbbreviatedUnitsLabel( m_units, m_useMils );
        text += ")";
        break;
    }

    text.Prepend( m_prefix );
    text.Append( m_suffix );

    m_text.SetText( text );
}


wxString DIMENSION::GetValueText() const
{
    int val = GetMeasuredValue();

    wxString text;
    wxString format = wxT( "%." ) + wxString::Format( "%i", m_precision ) + wxT( "f" );

    text.Printf( format, To_User_Unit( m_units, val, m_useMils ) );

    if( m_suppressZeroes )
    {
        while( text.Last() == '0' )
        {
            text.RemoveLast();

            if( text.Last() == '.' )
            {
                text.RemoveLast();
                break;
            }
        }

    }

    return text;
}


void DIMENSION::SetPrefix( const wxString& aPrefix )
{
    m_prefix = aPrefix;
}


void DIMENSION::SetSuffix( const wxString& aSuffix )
{
    m_suffix = aSuffix;
}


void DIMENSION::SetUnits( EDA_UNITS aUnits, bool aUseMils )
{
    m_units = aUnits;
    m_useMils = aUseMils;
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
    m_autoUnits = false;
    m_useMils   = false;

    switch( aMode )
    {
    case DIM_UNITS_MODE::INCHES:
        m_units = EDA_UNITS::INCHES;
        break;

    case DIM_UNITS_MODE::MILS:
        m_units = EDA_UNITS::INCHES;
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
    m_valueString = aNewText;
    updateText();
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

    Update();
}


void DIMENSION::Rotate( const wxPoint& aRotCentre, double aAngle )
{
    if( m_keepTextAligned )
        m_keepTextAligned = false;

    double newAngle = m_text.GetTextAngle() + aAngle;

    if( newAngle >= 3600 )
        newAngle -= 3600;

    m_text.SetTextAngle( newAngle );

    Update();
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

    Update();
}


void DIMENSION::SetStart( const wxPoint& aOrigin )
{
    m_start = aOrigin;
    Update();
}


void DIMENSION::SetEnd( const wxPoint& aEnd )
{
    m_end = aEnd;
    Update();
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
    m_extensionHeight = static_cast<int>( m_arrowLength * std::sin( DEG2RAD( s_arrowAngle ) ) );
}


EDA_ITEM* ALIGNED_DIMENSION::Clone() const
{
    return new ALIGNED_DIMENSION( *this );
}


void ALIGNED_DIMENSION::SwapData( BOARD_ITEM* aImage )
{
    assert( aImage->Type() == PCB_DIMENSION_T );

    std::swap( *static_cast<ALIGNED_DIMENSION*>( this ),
               *static_cast<ALIGNED_DIMENSION*>( aImage ) );
}


void ALIGNED_DIMENSION::SetHeight( int aHeight )
{
    m_height = aHeight;
    Update();
}


void ALIGNED_DIMENSION::UpdateHeight( const wxPoint& aCrossbarStart, const wxPoint& aCrossbarEnd )
{
    VECTOR2D height( aCrossbarStart - GetStart() );
    VECTOR2D crossBar( aCrossbarEnd - aCrossbarStart );

    if( height.Cross( crossBar ) > 0 )
        m_height = -height.EuclideanNorm();
    else
        m_height = height.EuclideanNorm();

    Update();
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

    // Now that we have the text updated, we can determine how to draw the crossbar.
    // First we need to create an appropriate bounding polygon to collide with
    EDA_RECT textBox = m_text.GetTextBox().Inflate( m_text.GetTextWidth() / 2,
                                                    m_text.GetEffectiveTextPenWidth() );

    SHAPE_POLY_SET polyBox;
    polyBox.NewOutline();
    polyBox.Append( textBox.GetOrigin() );
    polyBox.Append( textBox.GetOrigin().x, textBox.GetEnd().y );
    polyBox.Append( textBox.GetEnd() );
    polyBox.Append( textBox.GetEnd().x, textBox.GetOrigin().y );
    polyBox.Rotate( -m_text.GetTextAngleRadians(), textBox.GetCenter() );

    // The ideal crossbar, if the text doesn't collide
    SEG crossbar( m_crossBarStart, m_crossBarEnd );

    auto findEndpoint =
            [&]( const VECTOR2I& aStart, const VECTOR2I& aEnd ) -> VECTOR2I
            {
                VECTOR2I endpoint( aEnd );

                for( SHAPE_POLY_SET::SEGMENT_ITERATOR seg = polyBox.IterateSegments(); seg; seg++ )
                {
                    if( OPT_VECTOR2I intersection = ( *seg ).Intersect( crossbar ) )
                    {
                        if( ( *intersection - aStart ).SquaredEuclideanNorm() <
                            ( endpoint - aStart ).SquaredEuclideanNorm() )
                            endpoint = *intersection;
                    }
                }

                return endpoint;
            };

    // Now we can draw 0, 1, or 2 crossbar lines depending on how the polygon collides

    bool containsA = polyBox.Contains( crossbar.A );
    bool containsB = polyBox.Contains( crossbar.B );

    if( containsA && !containsB )
    {
        m_lines.emplace_back( SEG( findEndpoint( crossbar.B, crossbar.A ), crossbar.B ) );
    }
    else if( containsB && !containsA )
    {
        m_lines.emplace_back( SEG( crossbar.A, findEndpoint( crossbar.A, crossbar.B ) ) );
    }
    else if( polyBox.Collide( crossbar ) )
    {
        // text box collides and we need two segs
        VECTOR2I endpoint1 = findEndpoint( crossbar.B, crossbar.A );
        VECTOR2I endpoint2 = findEndpoint( crossbar.A, crossbar.B );

        if( ( crossbar.B - endpoint1 ).SquaredEuclideanNorm() >
            ( crossbar.B - endpoint2 ).SquaredEuclideanNorm() )
            std::swap( endpoint1, endpoint2 );

        m_lines.emplace_back( SEG( endpoint1, crossbar.B ) );
        m_lines.emplace_back( SEG( crossbar.A, endpoint2 ) );
    }
    else if( !containsA && !containsB )
    {
        // No collision
        m_lines.emplace_back( crossbar );
    }

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

    if( m_textPosition == DIM_TEXT_POSITION::OUTSIDE )
    {
        int textOffsetDistance = m_text.GetEffectiveTextPenWidth() + m_text.GetTextHeight();

        double rotation = std::copysign( DEG2RAD( 90 ), m_height );
        VECTOR2I textOffset = crossbarCenter.Rotate( rotation ).Resize( textOffsetDistance );
        textOffset += crossbarCenter;

        m_text.SetTextPos( m_crossBarStart + wxPoint( textOffset ) );
    }
    else if( m_textPosition == DIM_TEXT_POSITION::INLINE )
    {
        m_text.SetTextPos( m_crossBarStart + wxPoint( crossbarCenter ) );
    }

    if( m_keepTextAligned )
    {
        double textAngle = 3600 - RAD2DECIDEG( crossbarCenter.Angle() );

        NORMALIZE_ANGLE_POS( textAngle );

        if( textAngle > 900 && textAngle < 2700 )
            textAngle -= 1800;

        m_text.SetTextAngle( textAngle );
    }

    DIMENSION::updateText();
}
