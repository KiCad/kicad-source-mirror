/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2020 Thomas Pointhuber <thomas.pointhuber@gmx.at>
 * Copyright (C) 2020 KiCad Developers, see AUTHORS.txt for contributors.
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

#include <bitmaps.h>
#include <fctsys.h>
#include <gr_basic.h>
#include <kicad_string.h>
#include <macros.h>
#include <pcb_edit_frame.h>
#include <richio.h>
#include <trigo.h>

#include <base_units.h>
#include <class_barcode.h>
#include <class_board.h>
#include <class_pcb_text.h>
#include <math/util.h> // for KiROUND
#include <pgm_base.h>
#include <settings/color_settings.h>
#include <settings/settings_manager.h>

#include <backend/zint.h>

PCB_BARCODE::PCB_BARCODE( BOARD_ITEM* aParent )
        : BOARD_ITEM( aParent, PCB_BARCODE_T ),
          m_Width( Millimeter2iu( 40 ) ),
          m_Height( Millimeter2iu( 40 ) ),
          m_Text( this ),
          m_Kind( BARCODE_T::QR_CODE )
{
    m_Layer = Dwgs_User;
}


PCB_BARCODE::~PCB_BARCODE()
{
}


void PCB_BARCODE::SetPosition( const wxPoint& aPos )
{
    m_Text.SetTextPos( aPos );
}


const wxPoint PCB_BARCODE::GetPosition() const
{
    return m_Text.GetTextPos();
}


void PCB_BARCODE::SetText( const wxString& aNewText )
{
    m_Text.SetText( aNewText );
}


const wxString PCB_BARCODE::GetText() const
{
    return m_Text.GetText();
}


void PCB_BARCODE::SetLayer( PCB_LAYER_ID aLayer )
{
    m_Layer = aLayer;
    m_Text.SetLayer( aLayer );
}


void PCB_BARCODE::Move( const wxPoint& offset )
{
    m_Text.Offset( offset );

    // TODO
}


void PCB_BARCODE::Rotate( const wxPoint& aRotCentre, double aAngle )
{
    wxPoint tmp = m_Text.GetTextPos();
    RotatePoint( &tmp, aRotCentre, aAngle );
    m_Text.SetTextPos( tmp );

    double newAngle = m_Text.GetTextAngle() + aAngle;

    if( newAngle >= 3600 )
        newAngle -= 3600;

    if( newAngle > 900 && newAngle < 2700 )
        newAngle -= 1800;

    m_Text.SetTextAngle( newAngle );

    // TODO
}


void PCB_BARCODE::Flip( const wxPoint& aCentre, bool aFlipLeftRight )
{

    // BARCODE items are not usually on copper layers, so
    // copper layers count is not taken in accoun in Flip transform
    SetLayer( FlipLayer( GetLayer() ) );
}


void PCB_BARCODE::ComputeBarcode()
{
    m_Poly.RemoveAllContours();

    zint_symbol* symbol = ZBarcode_Create();
    if( symbol == nullptr )
    {
        return; // TODO: malloc was not possible
    }

    symbol->input_mode = UNICODE_MODE;
    switch( m_Kind )
    {
    case BARCODE_T::CODE_39:
        symbol->symbology = BARCODE_CODE39;
        break;
    case BARCODE_T::CODE_128:
        symbol->symbology = BARCODE_CODE128;
        break;
    case BARCODE_T::QR_CODE:
        symbol->symbology = BARCODE_QRCODE;
        break;
    default:
        return; // invalid code
    }

    wxString text = GetText();
    ZBarcode_Encode( symbol, text.c_str(), text.length() );

    // TODO: for now only lines are rendered!
    ZBarcode_Render( symbol, static_cast<float>( m_Width ), static_cast<float>( m_Height ) );

    wxPoint start = -wxPoint( m_Width / 2, m_Height / 2 );
    for( zint_render_line* line = symbol->rendered->lines; line != nullptr; line = line->next )
    {
        int x1 = start.x + static_cast<int>( line->x );
        int x2 = x1 + static_cast<int>( line->width );
        int y1 = start.y + static_cast<int>( line->y );
        int y2 = y1 - static_cast<int>( line->length );

        SHAPE_LINE_CHAIN shapeline;
        shapeline.Append( x1, y1 );
        shapeline.Append( x2, y1 );
        shapeline.Append( x2, y2 );
        shapeline.Append( x1, y2 );
        shapeline.SetClosed( true );

        m_Poly.AddOutline( shapeline );
    }

    ZBarcode_Delete( symbol );
}


// see class_cotation.h
void PCB_BARCODE::GetMsgPanelInfo( EDA_DRAW_FRAME* aFrame, std::vector<MSG_PANEL_ITEM>& aList )
{
    // for now, display only the text within the BARCODE using class TEXTE_PCB.
    m_Text.GetMsgPanelInfo( aFrame, aList );
}


bool PCB_BARCODE::HitTest( const wxPoint& aPosition, int aAccuracy ) const
{
    if( m_Text.TextHitTest( aPosition ) )
        return true;

    return GetBoundingBox().Contains( aPosition ); // TODO: simple hit test
}


bool PCB_BARCODE::HitTest( const EDA_RECT& aRect, bool aContained, int aAccuracy ) const
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


const EDA_RECT PCB_BARCODE::GetBoundingBox() const
{
    EDA_RECT bBox;
    int      xmin, xmax, ymin, ymax;

    bBox = m_Text.GetTextBox();
    xmin = bBox.GetX();
    xmax = bBox.GetRight();
    ymin = bBox.GetY();
    ymax = bBox.GetBottom();

    xmin = std::min( xmin, GetPosition().x - m_Width / 2 );
    xmax = std::min( xmax, GetPosition().x + m_Width / 2 );
    ymin = std::min( ymin, GetPosition().y - m_Height / 2 );
    ymax = std::min( ymax, GetPosition().y + m_Height / 2 );

    bBox.SetX( xmin );
    bBox.SetY( ymin );
    bBox.SetWidth( xmax - xmin + 1 );
    bBox.SetHeight( ymax - ymin + 1 );

    bBox.Normalize();

    return bBox;
}


wxString PCB_BARCODE::GetSelectMenuText( EDA_UNITS aUnits ) const
{
    return wxString::Format( _( "BARCODE \"%s\" on %s" ), GetText(), GetLayerName() );
}


BITMAP_DEF PCB_BARCODE::GetMenuImage() const
{
    return add_barcode_xpm;
}


const BOX2I PCB_BARCODE::ViewBBox() const
{
    BOX2I dimBBox = BOX2I(
            VECTOR2I( GetBoundingBox().GetPosition() ), VECTOR2I( GetBoundingBox().GetSize() ) );
    dimBBox.Merge( m_Text.ViewBBox() );

    return dimBBox;
}


EDA_ITEM* PCB_BARCODE::Clone() const
{
    return new PCB_BARCODE( *this );
}

void PCB_BARCODE::SwapData( BOARD_ITEM* aImage )
{
    assert( aImage->Type() == PCB_BARCODE_T );

    std::swap( *( (PCB_BARCODE*) this ), *( (PCB_BARCODE*) aImage ) );
}
