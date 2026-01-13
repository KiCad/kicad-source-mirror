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

#include <core/type_helpers.h>
#include <bitmaps.h>
#include <gr_basic.h>
#include <macros.h>
#include <pcb_edit_frame.h>
#include <richio.h>
#include <trigo.h>

#include <base_units.h>
#include <pcb_barcode.h>
#include <board.h>
#include <geometry/shape_poly_set.h>
#include <pcb_text.h>
#include <math/util.h> // for KiROUND
#include <convert_basic_shapes_to_polygon.h>
#include <wx/log.h>
#include <pgm_base.h>
#include <settings/color_settings.h>
#include <settings/settings_manager.h>
#include <scoped_set_reset.h>
#include <stdexcept>
#include <utility>
#include <algorithm>
#include <footprint.h>

#include <backend/zint.h>
#include <board_design_settings.h>

PCB_BARCODE::PCB_BARCODE( BOARD_ITEM* aParent ) :
        BOARD_ITEM( aParent, PCB_BARCODE_T ),
        m_width( pcbIUScale.mmToIU( 40 ) ),
        m_height( pcbIUScale.mmToIU( 40 ) ),
        m_pos( 0, 0 ),
        m_text( this ),
        m_kind( BARCODE_T::QR_CODE ),
        m_angle( 0 ),
        m_errorCorrection( BARCODE_ECC_T::L )
{
    m_layer = Dwgs_User;
}


PCB_BARCODE::~PCB_BARCODE()
{
}


void PCB_BARCODE::SetPosition( const VECTOR2I& aPos )
{
    VECTOR2I delta = aPos - m_pos;
    Move( delta );
}


VECTOR2I PCB_BARCODE::GetPosition() const
{
    return m_pos;
}


void PCB_BARCODE::SetText( const wxString& aNewText )
{
    m_text.SetText( aNewText );
}


wxString PCB_BARCODE::GetText() const
{
    return m_text.GetText();
}


wxString PCB_BARCODE::GetShownText() const
{
    return m_text.GetShownText( true );
}


void PCB_BARCODE::SetLayer( PCB_LAYER_ID aLayer )
{
    m_layer = aLayer;
    m_text.SetLayer( aLayer );

    AssembleBarcode();
}


void PCB_BARCODE::SetTextSize( int aTextSize )
{
    m_text.SetTextSize( VECTOR2I( std::max( 1, aTextSize ), std::max( 1, aTextSize ) ) );
    m_text.SetTextThickness( std::max( 1, GetPenSizeForNormal( m_text.GetTextHeight() ) ) );

    AssembleBarcode();
}


int PCB_BARCODE::GetTextSize() const
{
    return m_text.GetTextHeight();
}


void PCB_BARCODE::Move( const VECTOR2I& offset )
{
    m_pos += offset;
    m_symbolPoly.Move( offset );
    m_textPoly.Move( offset );
    m_poly.Move( offset );
    m_text.Move( offset );
    m_bbox.Move( offset );
}


void PCB_BARCODE::Rotate( const VECTOR2I& aRotCentre, const EDA_ANGLE& aAngle )
{
    RotatePoint( m_pos, aRotCentre, aAngle );
    m_angle += aAngle;

    AssembleBarcode();
}


void PCB_BARCODE::Flip( const VECTOR2I& aCentre, FLIP_DIRECTION aFlipDirection )
{
    MIRROR( m_pos, aCentre, aFlipDirection );

    if( aFlipDirection == FLIP_DIRECTION::TOP_BOTTOM )
        m_angle += ANGLE_180;

    SetLayer( GetBoard()->FlipLayer( GetLayer() ) );

    AssembleBarcode();
}


void PCB_BARCODE::StyleFromSettings( const BOARD_DESIGN_SETTINGS& settings, bool aCheckSide )
{
    SetTextSize( settings.GetTextSize( GetLayer() ).y );
}


void PCB_BARCODE::AssembleBarcode()
{
    ComputeBarcode();

    // Scale the symbol polygon to the desired barcode width/height (property values) and center it at m_pos
    // Note: SetRect will rescale the symbol-only polygon and then rebuild m_poly
    SetRect( m_pos - VECTOR2I( m_width / 2, m_height / 2 ),
             m_pos + VECTOR2I( m_width / 2, m_height / 2 ) );

    ComputeTextPoly();

    // Build full m_poly from symbol + optional text, then apply knockout if requested
    m_poly.RemoveAllContours();
    m_poly.Append( m_symbolPoly );

    if( m_text.IsVisible() && m_textPoly.OutlineCount() )
        m_poly.Append( m_textPoly );

    m_poly.Fracture();

    if( IsKnockout() )
    {
        // Enforce minimum margin: at least 10% of the smallest side of the barcode, rounded up
        // to the nearest 0.1 mm. Use this as a lower bound for both axes.
        int minSide = std::min( m_width, m_height );
        int tenPercent = ( minSide + 9 ) / 10; // ceil(minSide * 0.1)
        int step01mm = std::max( 1, pcbIUScale.mmToIU( 0.1 ) );
        int tenPercentRounded = ( ( tenPercent + step01mm - 1 ) / step01mm ) * step01mm;

        // Build inversion rectangle based on the local bbox of the current combined geometry
        BOX2I bbox = m_poly.BBox();
        bbox.Inflate( std::max( m_margin.x, tenPercentRounded ), std::max( m_margin.y, tenPercentRounded ) );

        SHAPE_LINE_CHAIN rect;
        rect.Append( bbox.GetLeft(),  bbox.GetTop() );
        rect.Append( bbox.GetRight(), bbox.GetTop() );
        rect.Append( bbox.GetRight(), bbox.GetBottom() );
        rect.Append( bbox.GetLeft(),  bbox.GetBottom() );
        rect.SetClosed( true );

        SHAPE_POLY_SET ko;
        ko.AddOutline( rect );
        ko.BooleanSubtract( m_poly );
        ko.Fracture();
        m_poly = std::move( ko );
    }

    if( IsSideSpecific() && GetBoard() && GetBoard()->IsBackLayer( m_layer ) )
        m_poly.Mirror( m_pos, FLIP_DIRECTION::LEFT_RIGHT );

    if( !m_angle.IsZero() )
        m_poly.Rotate( m_angle, m_pos );

    m_poly.CacheTriangulation( false );
    m_bbox = m_poly.BBox();
}


void PCB_BARCODE::ComputeTextPoly()
{
    m_textPoly.RemoveAllContours();

    if( !m_text.IsVisible() )
        return;

    SHAPE_POLY_SET textPoly;
    m_text.TransformTextToPolySet( textPoly, 0, GetMaxError(), ERROR_INSIDE );

    if( textPoly.OutlineCount() == 0 )
        return;

    if( m_symbolPoly.OutlineCount() == 0 )
        return;

    BOX2I textBBox = textPoly.BBox();
    BOX2I symbolBBox = m_symbolPoly.BBox();
    VECTOR2I textPos;
    int textOffset = pcbIUScale.mmToIU( 1 );
    textPos.x = symbolBBox.GetCenter().x - textBBox.GetCenter().x;
    textPos.y = symbolBBox.GetBottom() - textBBox.GetTop() + textOffset;

    textPoly.Move( textPos );

    m_textPoly = std::move( textPoly );
    m_textPoly.CacheTriangulation();
}


void PCB_BARCODE::ComputeBarcode()
{
    m_symbolPoly.RemoveAllContours();

    std::unique_ptr<zint_symbol, decltype( &ZBarcode_Delete )> symbol( ZBarcode_Create(), &ZBarcode_Delete );

    if( !symbol )
    {
        wxLogError( wxT( "Zint: failed to allocate symbol" ) );
        return;
    }

    symbol->input_mode = UNICODE_MODE;
    symbol->show_hrt = 0; // do not show HRT

    switch( m_kind )
    {
    case BARCODE_T::CODE_39:
        symbol->symbology = BARCODE_CODE39;
        break;
    case BARCODE_T::CODE_128:
        symbol->symbology = BARCODE_CODE128;
        break;
    case BARCODE_T::QR_CODE:
        symbol->symbology = BARCODE_QRCODE;
        symbol->option_1 = to_underlying( m_errorCorrection );
        break;
    case BARCODE_T::MICRO_QR_CODE:
        symbol->symbology = BARCODE_MICROQR;
        symbol->option_1 = to_underlying( m_errorCorrection );
        break;
    case BARCODE_T::DATA_MATRIX:
        symbol->symbology = BARCODE_DATAMATRIX;
        break;
    default:
        wxLogError( wxT( "Zint: invalid barcode type" ) );
        return;
    }

    wxString text = GetText();
    wxScopedCharBuffer utf8Text = text.ToUTF8();
    size_t length = utf8Text.length();
    unsigned char* dataPtr = reinterpret_cast<unsigned char*>( utf8Text.data() );

    if( text.empty() )
        return;

    if( ZBarcode_Encode( symbol.get(), dataPtr, length ) )
        return;

    if( ZBarcode_Buffer_Vector( symbol.get(), 0 ) ) // 0 means success
        return;

    for( zint_vector_rect* rect = symbol->vector->rectangles; rect != nullptr; rect = rect->next )
    {
        // Round using absolute edges to avoid cumulative rounding drift across modules.
        int x1 = KiROUND( rect->x * symbol->scale );
        int x2 = KiROUND( ( rect->x + rect->width ) * symbol->scale );
        int y1 = KiROUND( rect->y * symbol->scale );
        int y2 = KiROUND( ( rect->y + rect->height ) * symbol->scale );

        SHAPE_LINE_CHAIN shapeline;
        shapeline.Append( x1, y1 );
        shapeline.Append( x2, y1 );
        shapeline.Append( x2, y2 );
        shapeline.Append( x1, y2 );
        shapeline.SetClosed( true );

        m_symbolPoly.AddOutline( shapeline );
    }

    for( zint_vector_hexagon* hex = symbol->vector->hexagons; hex != nullptr; hex = hex->next )
    {
        // Compute vertices from center using minimal-diameter (inscribed circle) radius.
        double r  = hex->diameter / 2.0; // minimal radius
        double cx = hex->x;
        double cy = hex->y;

        // Base orientation has apex at top; hex->rotation rotates by 0/90/180/270 degrees.
        double baseAngles[6] = { 90.0, 30.0, -30.0, -90.0, -150.0, 150.0 };
        double rot = static_cast<double>( hex->rotation );

        SHAPE_LINE_CHAIN poly;

        for( int k = 0; k < 6; ++k )
        {
            double ang = ( baseAngles[k] + rot ) * M_PI / 180.0;
            int vx = KiROUND( cx + r * cos( ang ) );
            int vy = KiROUND( cy + r * sin( ang ) );
            poly.Append( vx, vy );
        }
        poly.SetClosed( true );

        m_symbolPoly.AddOutline( poly );
    }

    // Set the position of the barcode to the center of the symbol polygon
    if( m_symbolPoly.OutlineCount() > 0 )
    {
        VECTOR2I pos = m_symbolPoly.BBox().GetCenter();
        m_symbolPoly.Move( -pos );
    }

    m_symbolPoly.CacheTriangulation();
}


void PCB_BARCODE::GetMsgPanelInfo( EDA_DRAW_FRAME* aFrame, std::vector<MSG_PANEL_ITEM>& aList )
{
    FOOTPRINT* parentFP = GetParentFootprint();

    if( parentFP && aFrame->GetName() == PCB_EDIT_FRAME_NAME )
        aList.emplace_back( _( "Footprint" ), parentFP->GetReference() );

    aList.emplace_back( _( "Barcode" ), ENUM_MAP<BARCODE_T>::Instance().ToString( m_kind ) );

    // Don't use GetShownText() here; we want to show the user the variable references
    aList.emplace_back( _( "Text" ), KIUI::EllipsizeStatusText( aFrame, GetText() ) );

    if( aFrame->GetName() == PCB_EDIT_FRAME_NAME && IsLocked() )
        aList.emplace_back( _( "Status" ), _( "Locked" ) );

    aList.emplace_back( _( "Layer" ), GetLayerName() );

    aList.emplace_back( _( "Angle" ), wxString::Format( wxT( "%g" ), m_angle.AsDegrees() ) );

    aList.emplace_back( _( "Text Height" ), aFrame->MessageTextFromValue( m_text.GetTextHeight() ) );
}


bool PCB_BARCODE::HitTest( const VECTOR2I& aPosition, int aAccuracy ) const
{
    if( !GetBoundingBox().Contains( aPosition ) )
        return false;

    SHAPE_POLY_SET hulls;

    GetBoundingHull( hulls, UNDEFINED_LAYER, aAccuracy, ARC_LOW_DEF, ERROR_OUTSIDE );

    return hulls.Collide( aPosition );
}


bool PCB_BARCODE::HitTest( const BOX2I& aRect, bool aContained, int aAccuracy ) const
{
    BOX2I arect = aRect;
    arect.Inflate( aAccuracy );

    BOX2I rect = GetBoundingBox();

    if( aAccuracy )
        rect.Inflate( aAccuracy );

    if( aContained )
        return arect.Contains( rect );

    return arect.Intersects( rect );
}


void PCB_BARCODE::SetRect( const VECTOR2I& aTopLeft, const VECTOR2I& aBotRight )
{
    // Rescale only the symbol polygon to the requested rectangle; text is rebuilt below
    BOX2I bbox = m_symbolPoly.BBox();
    int oldW = bbox.GetWidth();
    int oldH = bbox.GetHeight();

    VECTOR2I newPosition = ( aTopLeft + aBotRight ) / 2;
    SetPosition( newPosition );
    int newW = aBotRight.x - aTopLeft.x;
    int newH = aBotRight.y - aTopLeft.y;
    // Guard against zero/negative sizes from interactive edits; enforce a tiny minimum
    int minIU = std::max( 1, pcbIUScale.mmToIU( 0.01 ) );
    newW = std::max( newW, minIU );
    newH = std::max( newH, minIU );

    double scaleX = oldW ? static_cast<double>( newW ) / oldW : 1.0;
    double scaleY = oldH ? static_cast<double>( newH ) / oldH : 1.0;

    VECTOR2I oldCenter = bbox.GetCenter();
    m_symbolPoly.Scale( scaleX, scaleY, oldCenter );

    // After scaling, move the symbol polygon to be centered at the new position
    VECTOR2I newCenter = m_symbolPoly.BBox().GetCenter();
    VECTOR2I delta = newPosition - newCenter;

    if( delta != VECTOR2I( 0, 0 ) )
        m_symbolPoly.Move( delta );

    // Update intended barcode symbol size (without text/margins)
    m_width = newW;
    m_height = newH;
}


const BOX2I PCB_BARCODE::GetBoundingBox() const
{
    return m_bbox;
}


wxString PCB_BARCODE::GetItemDescription( UNITS_PROVIDER* aUnitsProvider, bool aFull ) const
{
    return wxString::Format( _( "Barcode '%s' on %s" ), GetText(), GetLayerName() );
}


BITMAPS PCB_BARCODE::GetMenuImage() const
{
    return BITMAPS::add_barcode;
}


const BOX2I PCB_BARCODE::ViewBBox() const
{
    return m_bbox;
}


void PCB_BARCODE::TransformShapeToPolygon( SHAPE_POLY_SET& aBuffer, PCB_LAYER_ID aLayer,
                                           int aClearance, int aMaxError,
                                           ERROR_LOC aErrorLoc, bool ignoreLineWidth ) const
{
    if( aLayer != m_layer && aLayer != UNDEFINED_LAYER )
        return;

    if( aClearance == 0 )
    {
        aBuffer.Append( m_poly );
    }
    else
    {
        SHAPE_POLY_SET poly = m_poly;
        poly.Inflate( aClearance, CORNER_STRATEGY::CHAMFER_ACUTE_CORNERS, aMaxError, aErrorLoc );
        aBuffer.Append( poly );
    }
}


std::shared_ptr<SHAPE> PCB_BARCODE::GetEffectiveShape( PCB_LAYER_ID aLayer, FLASHING aFlash ) const
{
    SHAPE_POLY_SET poly;
    TransformShapeToPolygon( poly, aLayer, 0, 0, ERROR_INSIDE, true );

    return std::make_shared<SHAPE_POLY_SET>( std::move( poly ) );
}


void PCB_BARCODE::GetBoundingHull( SHAPE_POLY_SET& aBuffer, PCB_LAYER_ID aLayer, int aClearance,
                                   int aMaxError, ERROR_LOC aErrorLoc ) const
{
    auto getBoundingHull =
            [this]( SHAPE_POLY_SET& aLocBuffer, const SHAPE_POLY_SET& aSource, int aLocClearance )
            {
                BOX2I    rect = aSource.BBox( aLocClearance );
                VECTOR2I corners[4];

                corners[0].x = rect.GetOrigin().x;
                corners[0].y = rect.GetOrigin().y;
                corners[1].y = corners[0].y;
                corners[1].x = rect.GetRight();
                corners[2].x = corners[1].x;
                corners[2].y = rect.GetBottom();
                corners[3].y = corners[2].y;
                corners[3].x = corners[0].x;

                aLocBuffer.NewOutline();

                for( VECTOR2I& corner : corners )
                {
                    RotatePoint( corner, m_pos, m_angle );
                    aLocBuffer.Append( corner.x, corner.y );
                }
            };

    if( aLayer == m_layer || aLayer == UNDEFINED_LAYER )
    {
        getBoundingHull( aBuffer, m_symbolPoly, aClearance );
        getBoundingHull( aBuffer, m_textPoly, aClearance );
    }
}


void PCB_BARCODE::SetErrorCorrection( BARCODE_ECC_T aErrorCorrection )
{
    // Micro QR codes do not support High (H) error correction level
    if( m_kind == BARCODE_T::MICRO_QR_CODE && aErrorCorrection == BARCODE_ECC_T::H )
        m_errorCorrection = BARCODE_ECC_T::Q;
    else
        m_errorCorrection = aErrorCorrection;
    // Don't auto-compute here as it may be called during loading
}


void PCB_BARCODE::SetKind( BARCODE_T aKind )
{
    m_kind = aKind;

    // When switching to Micro QR, validate and adjust ECC if needed
    if( m_kind == BARCODE_T::MICRO_QR_CODE && m_errorCorrection == BARCODE_ECC_T::H )
        m_errorCorrection = BARCODE_ECC_T::Q;

    // Don't auto-compute here as it may be called during loading
}


void PCB_BARCODE::SetBarcodeErrorCorrection( BARCODE_ECC_T aErrorCorrection )
{
    SetErrorCorrection( aErrorCorrection );
    AssembleBarcode();
}


void PCB_BARCODE::SetBarcodeWidth( int aWidth )
{
    m_width = aWidth;

    if( KeepSquare() )
        m_height = aWidth;

    AssembleBarcode();
}


void PCB_BARCODE::SetBarcodeHeight( int aHeight )
{
    m_height = aHeight;

    if( KeepSquare() )
        m_width = aHeight;

    AssembleBarcode();
}


void PCB_BARCODE::SetBarcodeKind( BARCODE_T aKind )
{
    SetKind( aKind );
    AssembleBarcode();
}


EDA_ITEM* PCB_BARCODE::Clone() const
{
    PCB_BARCODE* item = new PCB_BARCODE( *this );
    item->CopyFrom( this );
    item->m_text.SetParent( item );
    return item;
}


void PCB_BARCODE::swapData( BOARD_ITEM* aImage )
{
    wxCHECK_RET( aImage && aImage->Type() == PCB_BARCODE_T,
                 wxT( "Cannot swap data with non-barcode item." ) );

    PCB_BARCODE* other = static_cast<PCB_BARCODE*>( aImage );

    std::swap( *this, *other );

    m_text.SetParent( this );
    other->m_text.SetParent( other );
}

double PCB_BARCODE::Similarity( const BOARD_ITEM& aItem ) const
{
    if( !ClassOf( &aItem ) )
        return 0.0;

    const PCB_BARCODE* other = static_cast<const PCB_BARCODE*>( &aItem );

    // Compare text, width, height, text height, position, and kind
    double similarity = 0.0;
    const double weight = 1.0 / 6.0;

    if( GetText() == other->GetText() )
        similarity += weight;
    if( m_width == other->m_width )
        similarity += weight;
    if( m_height == other->m_height )
        similarity += weight;
    if( GetTextSize() == other->GetTextSize() )
        similarity += weight;
    if( GetPosition() == other->GetPosition() )
        similarity += weight;
    if( m_kind == other->m_kind )
        similarity += weight;

    return similarity;
}

int PCB_BARCODE::Compare( const PCB_BARCODE* aBarcode, const PCB_BARCODE* aOther )
{
    int diff;

    if( ( diff = aBarcode->GetPosition().x - aOther->GetPosition().x ) != 0 )
        return diff;

    if( ( diff = aBarcode->GetPosition().y - aOther->GetPosition().y ) != 0 )
        return diff;

    if( ( diff = aBarcode->GetText().Cmp( aOther->GetText() ) ) != 0 )
        return diff;

    if( ( diff = aBarcode->GetWidth() - aOther->GetWidth() ) != 0 )
        return diff;

    if( ( diff = aBarcode->GetHeight() - aOther->GetHeight() ) != 0 )
        return diff;

    if( ( diff = aBarcode->GetTextSize() - aOther->GetTextSize() ) != 0 )
        return diff;

    if( ( diff = (int) aBarcode->GetKind() - (int) aOther->GetKind() ) != 0 )
        return diff;

    if( ( diff = (int) aBarcode->GetErrorCorrection() - (int) aOther->GetErrorCorrection() ) != 0 )
        return diff;

    return 0;
}


bool PCB_BARCODE::operator==( const BOARD_ITEM& aItem ) const
{
    if( !ClassOf( &aItem ) )
        return false;

    const PCB_BARCODE* other = static_cast<const PCB_BARCODE*>( &aItem );

    // Compare text, width, height, text height, position, and kind
    return ( GetText() == other->GetText()
            && m_width == other->m_width
            && m_height == other->m_height
            && GetTextSize() == other->GetTextSize()
            && GetPosition() == other->GetPosition()
            && m_kind == other->m_kind );
}

// ---- Property registration ----
static struct PCB_BARCODE_DESC
{
    PCB_BARCODE_DESC()
    {
        PROPERTY_MANAGER& propMgr = PROPERTY_MANAGER::Instance();
        REGISTER_TYPE( PCB_BARCODE );
        propMgr.InheritsAfter( TYPE_HASH( PCB_BARCODE ), TYPE_HASH( BOARD_ITEM ) );

        const wxString groupBarcode = _HKI( "Barcode Properties" );

        ENUM_MAP<BARCODE_T>& kindMap = ENUM_MAP<BARCODE_T>::Instance();
        if( kindMap.Choices().GetCount() == 0 )
        {
            kindMap.Undefined( BARCODE_T::QR_CODE );
            kindMap.Map( BARCODE_T::CODE_39,       _HKI( "CODE_39" ) )
                   .Map( BARCODE_T::CODE_128,      _HKI( "CODE_128" ) )
                   .Map( BARCODE_T::DATA_MATRIX,   _HKI( "DATA_MATRIX" ) )
                   .Map( BARCODE_T::QR_CODE,       _HKI( "QR_CODE" ) )
                   .Map( BARCODE_T::MICRO_QR_CODE, _HKI( "MICRO_QR_CODE" ) );
        }

        ENUM_MAP<BARCODE_ECC_T>& eccMap = ENUM_MAP<BARCODE_ECC_T>::Instance();
        if( eccMap.Choices().GetCount() == 0 )
        {
            eccMap.Undefined( BARCODE_ECC_T::L );
            eccMap.Map( BARCODE_ECC_T::L, _HKI( "L (Low)" ) )
                  .Map( BARCODE_ECC_T::M, _HKI( "M (Medium)" ) )
                  .Map( BARCODE_ECC_T::Q, _HKI( "Q (Quartile)" ) )
                  .Map( BARCODE_ECC_T::H, _HKI( "H (High)" ) );
        }

        auto hasKnockout =
                []( INSPECTABLE* aItem ) -> bool
                {
                    if( PCB_BARCODE* bc = dynamic_cast<PCB_BARCODE*>( aItem ) )
                        return bc->IsKnockout();
                    return false;
                };

        propMgr.AddProperty( new PROPERTY<PCB_BARCODE, wxString>( _HKI( "Text" ),
                                    &PCB_BARCODE::SetBarcodeText, &PCB_BARCODE::GetText ), groupBarcode );

        propMgr.AddProperty( new PROPERTY<PCB_BARCODE, bool>( _HKI( "Show Text" ),
                                    &PCB_BARCODE::SetShowText, &PCB_BARCODE::GetShowText ), groupBarcode );

        propMgr.AddProperty( new PROPERTY<PCB_BARCODE, int>( _HKI( "Text Size" ),
                                    &PCB_BARCODE::SetTextSize, &PCB_BARCODE::GetTextSize,
                                    PROPERTY_DISPLAY::PT_COORD ), groupBarcode );

        propMgr.AddProperty( new PROPERTY<PCB_BARCODE, int>( _HKI( "Width" ),
                                    &PCB_BARCODE::SetBarcodeWidth, &PCB_BARCODE::GetWidth,
                                    PROPERTY_DISPLAY::PT_COORD ), groupBarcode );

        propMgr.AddProperty( new PROPERTY<PCB_BARCODE, int>( _HKI( "Height" ),
                                    &PCB_BARCODE::SetBarcodeHeight, &PCB_BARCODE::GetHeight,
                                    PROPERTY_DISPLAY::PT_COORD ), groupBarcode );

        propMgr.AddProperty( new PROPERTY<PCB_BARCODE, double>( _HKI( "Orientation" ),
                                    &PCB_BARCODE::SetOrientation, &PCB_BARCODE::GetOrientation ), groupBarcode );

        propMgr.AddProperty( new PROPERTY_ENUM<PCB_BARCODE, BARCODE_T>( _HKI( "Barcode Type" ),
                                    &PCB_BARCODE::SetBarcodeKind, &PCB_BARCODE::GetKind ), groupBarcode );

        auto isQRCode =
                []( INSPECTABLE* aItem ) -> bool
                {
                    if( PCB_BARCODE* bc = dynamic_cast<PCB_BARCODE*>( aItem ) )
                        return bc->GetKind() == BARCODE_T::QR_CODE || bc->GetKind() == BARCODE_T::MICRO_QR_CODE;

                    return false;
                };

        propMgr.AddProperty( new PROPERTY_ENUM<PCB_BARCODE, BARCODE_ECC_T>( _HKI( "Error Correction" ),
                                    &PCB_BARCODE::SetBarcodeErrorCorrection, &PCB_BARCODE::GetErrorCorrection ),
                             groupBarcode )
                .SetAvailableFunc( isQRCode )
                .SetChoicesFunc( []( INSPECTABLE* aItem )
                                 {
                                     PCB_BARCODE* barcode = static_cast<PCB_BARCODE*>( aItem );
                                     wxPGChoices  choices;

                                     choices.Add( _( "L (Low)" ), static_cast<int>( BARCODE_ECC_T::L ) );
                                     choices.Add( _( "M (Medium)" ), static_cast<int>( BARCODE_ECC_T::M ) );
                                     choices.Add( _( "Q (Quartile)" ), static_cast<int>( BARCODE_ECC_T::Q ) );

                                     // Only QR_CODE has High
                                     if( barcode->GetKind() == BARCODE_T::QR_CODE )
                                         choices.Add( _( "H (High)" ), static_cast<int>( BARCODE_ECC_T::H ) );

                                     return choices;
                                 } );

        propMgr.AddProperty( new PROPERTY<PCB_BARCODE, bool>( _HKI( "Knockout" ),
                                    &PCB_BARCODE::SetIsKnockout, &PCB_BARCODE::IsKnockout ), groupBarcode );

        propMgr.AddProperty( new PROPERTY<PCB_BARCODE, int>( _HKI( "Margin X" ),
                                    &PCB_BARCODE::SetMarginX, &PCB_BARCODE::GetMarginX,
                                    PROPERTY_DISPLAY::PT_COORD ), groupBarcode ).SetAvailableFunc( hasKnockout );

        propMgr.AddProperty( new PROPERTY<PCB_BARCODE, int>( _HKI( "Margin Y" ),
                                    &PCB_BARCODE::SetMarginY, &PCB_BARCODE::GetMarginY,
                                    PROPERTY_DISPLAY::PT_COORD ), groupBarcode ).SetAvailableFunc( hasKnockout );
    }
} _PCB_BARCODE_DESC;

// wxAny conversion implementations for enum properties (declarations in header)
IMPLEMENT_ENUM_TO_WXANY( BARCODE_T );
IMPLEMENT_ENUM_TO_WXANY( BARCODE_ECC_T );
