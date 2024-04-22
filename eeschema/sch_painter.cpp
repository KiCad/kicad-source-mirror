/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2014 CERN
 * Copyright (C) 2019-2024 KiCad Developers, see AUTHORS.txt for contributors.
 *
 * @author Tomasz Wlostowski <tomasz.wlostowski@cern.ch>
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


#include <trigo.h>
#include <connection_graph.h>
#include <gal/graphics_abstraction_layer.h>
#include <callback_gal.h>
#include <geometry/shape_segment.h>
#include <geometry/shape_rect.h>
#include <gr_text.h>
#include <sch_pin.h>
#include <math/util.h>
#include <pgm_base.h>
#include <sch_bitmap.h>
#include <sch_bus_entry.h>
#include <sch_symbol.h>
#include <sch_edit_frame.h>
#include <sch_field.h>
#include <sch_junction.h>
#include <sch_line.h>
#include <sch_shape.h>
#include <sch_marker.h>
#include <sch_no_connect.h>
#include <sch_sheet.h>
#include <sch_sheet_pin.h>
#include <sch_text.h>
#include <sch_textbox.h>
#include <sch_table.h>
#include <schematic.h>
#include <settings/color_settings.h>
#include <view/view.h>
#include <kiface_base.h>
#include <default_values.h>
#include <advanced_config.h>
#include <settings/settings_manager.h>
#include <stroke_params.h>
#include "sch_painter.h"
#include "common.h"


namespace KIGFX
{

EESCHEMA_SETTINGS* eeconfig()
{
    return dynamic_cast<EESCHEMA_SETTINGS*>( Kiface().KifaceSettings() );
}


std::vector<KICAD_T> SCH_PAINTER::g_ScaledSelectionTypes = {
    SCH_MARKER_T,
    SCH_JUNCTION_T,
    SCH_NO_CONNECT_T,
    SCH_BUS_WIRE_ENTRY_T,
    SCH_BUS_BUS_ENTRY_T,
    SCH_LINE_T,
    SCH_SHAPE_T,
    SCH_BITMAP_T,
    SCH_TEXT_T,
    SCH_GLOBAL_LABEL_T,
    SCH_DIRECTIVE_LABEL_T,
    SCH_FIELD_T,
    SCH_HIER_LABEL_T,
    SCH_SHEET_PIN_T,
    LIB_SYMBOL_T, SCH_SYMBOL_T,
    SCH_SHEET_T,
    SCH_PIN_T
};


/**
 * Used when a LIB_SYMBOL is not found in library to draw a dummy shape.
 * This symbol is a 400 mils square with the text "??"
 *
 *   DEF DUMMY U 0 40 Y Y 1 0 N
 *     F0 "U" 0 -350 60 H V
 *     F1 "DUMMY" 0 350 60 H V
 *     DRAW
 *       T 0 0 0 150 0 0 0 ??
 *       S -200 200 200 -200 0 1 0
 *     ENDDRAW
 *   ENDDEF
 */
static LIB_SYMBOL* dummy()
{
    static LIB_SYMBOL* symbol;

    if( !symbol )
    {
        symbol = new LIB_SYMBOL( wxEmptyString );

        SCH_SHAPE* square = new SCH_SHAPE( SHAPE_T::RECTANGLE, LAYER_DEVICE );

        square->SetPosition( VECTOR2I( schIUScale.MilsToIU( -200 ), schIUScale.MilsToIU( 200 ) ) );
        square->SetEnd( VECTOR2I( schIUScale.MilsToIU( 200 ), schIUScale.MilsToIU( -200 ) ) );
        symbol->AddDrawItem( square );

        SCH_TEXT* text = new SCH_TEXT( { 0, 0 }, wxT( "??" ), LAYER_DEVICE );

        text->SetTextSize( VECTOR2I( schIUScale.MilsToIU( 150 ), schIUScale.MilsToIU( 150 ) ) );
        symbol->AddDrawItem( text );
    }

    return symbol;
}


SCH_PAINTER::SCH_PAINTER( GAL* aGal ) :
        KIGFX::PAINTER( aGal ),
        m_schematic( nullptr )
{ }


bool SCH_PAINTER::Draw( const VIEW_ITEM* aItem, int aLayer )
{
    const EDA_ITEM* item = dynamic_cast<const EDA_ITEM*>( aItem );

    if( !item )
        return false;

    draw( item, aLayer, false );

    return false;
}

void SCH_PAINTER::draw( const EDA_ITEM* aItem, int aLayer, bool aDimmed )
{

#ifdef CONNECTIVITY_DEBUG

    auto sch_item = dynamic_cast<const SCH_ITEM*>( aItem );
    auto conn = sch_item ? sch_item->Connection( *g_CurrentSheet ) : nullptr;

    if( conn )
    {
        auto pos = aItem->GetBoundingBox().Centre();
        auto label = conn->Name( true );

        m_gal->SetHorizontalJustify( GR_TEXT_H_ALIGN_CENTER );
        m_gal->SetVerticalJustify( GR_TEXT_V_ALIGN_CENTER );
        m_gal->SetStrokeColor( COLOR4D( LIGHTRED ) );
        m_gal->SetLineWidth( Mils2ui( 2 ) );
        m_gal->SetGlyphSize( VECTOR2D( Mils2ui( 20 ), Mils2ui( 20 ) ) );
        m_gal->StrokeText( conn->Name( true ), pos, 0.0, 0 );
    }

#endif

    // Enable draw bounding box on request. Some bboxes are handled locally.
    bool drawBoundingBox = m_schSettings.GetDrawBoundingBoxes();

    switch( aItem->Type() )
    {
        case LIB_SYMBOL_T:
            draw( static_cast<const LIB_SYMBOL*>( aItem ), aLayer );
            break;
        case SCH_PIN_T:
            drawBoundingBox = false;
            draw( static_cast<const SCH_PIN*>( aItem ), aLayer, aDimmed  );
            break;
        case SCH_SYMBOL_T:
            draw( static_cast<const SCH_SYMBOL*>( aItem ), aLayer );
            break;
        case SCH_JUNCTION_T:
            draw( static_cast<const SCH_JUNCTION*>( aItem ), aLayer );
            break;
        case SCH_LINE_T:
            draw( static_cast<const SCH_LINE*>( aItem ), aLayer );
            break;
        case SCH_SHAPE_T:
            draw( static_cast<const SCH_SHAPE*>( aItem ), aLayer, aDimmed );
            break;
        case SCH_TEXT_T:
            draw( static_cast<const SCH_TEXT*>( aItem ), aLayer, aDimmed );
            break;
        case SCH_TEXTBOX_T:
            draw( static_cast<const SCH_TEXTBOX*>( aItem ), aLayer, aDimmed );
            break;
        case SCH_TABLE_T:
            draw( static_cast<const SCH_TABLE*>( aItem ), aLayer, aDimmed );
            break;
        case SCH_LABEL_T:
            draw( static_cast<const SCH_LABEL*>( aItem ), aLayer );
            break;
        case SCH_DIRECTIVE_LABEL_T:
            draw( static_cast<const SCH_DIRECTIVE_LABEL*>( aItem ), aLayer );
            break;
        case SCH_FIELD_T:
            draw( static_cast<const SCH_FIELD*>( aItem ), aLayer, aDimmed );
            break;
        case SCH_HIER_LABEL_T:
            draw( static_cast<const SCH_HIERLABEL*>( aItem ), aLayer );
            break;
        case SCH_GLOBAL_LABEL_T:
            draw( static_cast<const SCH_GLOBALLABEL*>( aItem ), aLayer );
            break;
        case SCH_SHEET_T:
            draw( static_cast<const SCH_SHEET*>( aItem ), aLayer );
            break;
        case SCH_SHEET_PIN_T:
            draw( static_cast<const SCH_HIERLABEL*>( aItem ), aLayer );
            break;
        case SCH_NO_CONNECT_T:
            draw( static_cast<const SCH_NO_CONNECT*>( aItem ), aLayer );
            break;
        case SCH_BUS_WIRE_ENTRY_T:
            draw( static_cast<const SCH_BUS_ENTRY_BASE*>( aItem ), aLayer );
            break;
        case SCH_BUS_BUS_ENTRY_T:
            draw( static_cast<const SCH_BUS_ENTRY_BASE*>( aItem ), aLayer );
            break;
        case SCH_BITMAP_T:
            draw( static_cast<const SCH_BITMAP*>( aItem ), aLayer );
            break;
        case SCH_MARKER_T:
            draw( static_cast<const SCH_MARKER*>( aItem ), aLayer );
            break;

        default: return;
    }

    if( drawBoundingBox )
        drawItemBoundingBox( aItem );
}


void SCH_PAINTER::drawItemBoundingBox( const EDA_ITEM* aItem )
{
    if( const SCH_ITEM* item = dynamic_cast<const SCH_ITEM*>( aItem ) )
    {
        if( item->IsPrivate() && !m_schSettings.m_IsSymbolEditor )
            return;
    }

    BOX2I box = aItem->GetBoundingBox();

    if( aItem->Type() == SCH_SYMBOL_T )
        box = static_cast<const SCH_SYMBOL*>( aItem )->GetBodyBoundingBox();

    m_gal->SetIsFill( false );
    m_gal->SetIsStroke( true );
    m_gal->SetStrokeColor( aItem->IsSelected() ? COLOR4D( 1.0, 0.2, 0.2, 1 )
                                              : COLOR4D( 0.2, 0.2, 0.2, 1 ) );
    m_gal->SetLineWidth( schIUScale.MilsToIU( 3 ) );
    m_gal->DrawRectangle( box.GetOrigin(), box.GetEnd() );
}


bool SCH_PAINTER::nonCached( const EDA_ITEM* aItem )
{
    // TODO: it would be nice to have a more definitive test for this, but we've currently got
    // no access to the VIEW_GROUP to see if it's cached or not.
    return aItem->IsSelected();
}


bool SCH_PAINTER::isUnitAndConversionShown( const SCH_ITEM* aItem ) const
{
    if( m_schSettings.m_ShowUnit            // showing a specific unit
            && aItem->GetUnit()             // item is unit-specific
            && aItem->GetUnit() != m_schSettings.m_ShowUnit )
    {
        return false;
    }

    if( m_schSettings.m_ShowBodyStyle       // showing a specific body style
            && aItem->GetBodyStyle()        // item is body-style-specific
            && aItem->GetBodyStyle() != m_schSettings.m_ShowBodyStyle )
    {
        return false;
    }

    return true;
}


KIFONT::FONT* SCH_PAINTER::getFont( const EDA_TEXT* aItem ) const
{
    if( KIFONT::FONT* font = aItem->GetFont() )
        return font;

    return KIFONT::FONT::GetFont( m_schSettings.GetDefaultFont(), aItem->IsBold(),
                                  aItem->IsItalic() );
}


float SCH_PAINTER::getShadowWidth( bool aForHighlight ) const
{
    const MATRIX3x3D& matrix = m_gal->GetScreenWorldMatrix();

    int milsWidth = aForHighlight ? eeconfig()->m_Selection.highlight_thickness
                                  : eeconfig()->m_Selection.selection_thickness;

    // For best visuals the selection width must be a cross between the zoom level and the
    // default line width.
    return (float) std::fabs( matrix.GetScale().x * milsWidth ) + schIUScale.MilsToIU( milsWidth );
}


COLOR4D SCH_PAINTER::getRenderColor( const SCH_ITEM* aItem, int aLayer, bool aDrawingShadows,
                                     bool aDimmed ) const
{
    COLOR4D color = m_schSettings.GetLayerColor( aLayer );
    // Graphic items of a SYMBOL frequently use the LAYER_DEVICE layer color
    // (i.e. when no specific color is set)
    bool isSymbolChild = aItem->GetParentSymbol() != nullptr;

    if( aItem->Type() == SCH_LINE_T )
    {
        color = static_cast<const SCH_LINE*>( aItem )->GetLineColor();
    }
    else if( aItem->Type() == SCH_BUS_WIRE_ENTRY_T )
    {
        color = static_cast<const SCH_BUS_WIRE_ENTRY*>( aItem )->GetBusEntryColor();
    }
    else if( aItem->Type() == SCH_JUNCTION_T )
    {
        color = static_cast<const SCH_JUNCTION*>( aItem )->GetJunctionColor();
    }
    else if( !m_schSettings.m_OverrideItemColors )
    {
        if( aItem->Type() == SCH_SHEET_T )
        {
            const SCH_SHEET* sheet = static_cast<const SCH_SHEET*>( aItem );

            if( aLayer == LAYER_SHEET_BACKGROUND )
                color = sheet->GetBackgroundColor();
            else
                color = sheet->GetBorderColor();
        }
        else if( aItem->Type() == SCH_SHAPE_T )
        {
            const SCH_SHAPE* shape = static_cast<const SCH_SHAPE*>( aItem );

            if( aLayer == LAYER_DEVICE_BACKGROUND || aLayer == LAYER_NOTES_BACKGROUND )
            {
                if( !isSymbolChild || shape->GetFillColor() != COLOR4D::UNSPECIFIED )
                    color = shape->GetFillColor();

                if( isSymbolChild && shape->GetFillMode() == FILL_T::FILLED_SHAPE )
                    color = shape->GetStroke().GetColor();
            }
            else
            {
                if( !isSymbolChild || shape->GetStroke().GetColor() != COLOR4D::UNSPECIFIED )
                    color = shape->GetStroke().GetColor();
            }

            // A filled shape means filled; if they didn't specify a fill colour then use the
            // border colour.
            if( color == COLOR4D::UNSPECIFIED )
                color = m_schSettings.GetLayerColor( isSymbolChild ? LAYER_DEVICE : LAYER_NOTES );
        }
        else if( aItem->IsType( { SCH_LABEL_LOCATE_ANY_T } ) )
        {
            color = static_cast<const SCH_LABEL_BASE*>( aItem )->GetLabelColor();
        }
        else if( aItem->Type() == SCH_FIELD_T )
        {
            color = static_cast<const SCH_FIELD*>( aItem )->GetFieldColor();
        }
        else if( aItem->Type() == SCH_TEXTBOX_T || aItem->Type() == SCH_TABLECELL_T )
        {
            const SCH_TEXTBOX* textBox = dynamic_cast<const SCH_TEXTBOX*>( aItem );

            if( aLayer == LAYER_DEVICE_BACKGROUND || aLayer == LAYER_NOTES_BACKGROUND )
                color = textBox->GetFillColor();
            else if( !isSymbolChild || textBox->GetTextColor() != COLOR4D::UNSPECIFIED )
                color = textBox->GetTextColor();
        }
        else if( const EDA_TEXT* otherTextItem = dynamic_cast<const EDA_TEXT*>( aItem ) )
        {
            if( !isSymbolChild || otherTextItem->GetTextColor() != COLOR4D::UNSPECIFIED )
                color = otherTextItem->GetTextColor();
        }
    }

    if( color == COLOR4D::UNSPECIFIED )
        color = m_schSettings.GetLayerColor( aLayer );

    if( aItem->IsBrightened() ) // Selection disambiguation, net highlighting, etc.
    {
        color = m_schSettings.GetLayerColor( LAYER_BRIGHTENED );

        if( aDrawingShadows )
        {
            if( aItem->IsSelected() )
                color = m_schSettings.GetLayerColor( LAYER_SELECTION_SHADOWS );
            else
                color = color.WithAlpha( 0.15 );
        }
        else if( aLayer == LAYER_DEVICE_BACKGROUND || aLayer == LAYER_SHEET_BACKGROUND )
        {
            color = color.WithAlpha( 0.2 );
        }
    }
    else if( aItem->IsSelected() && aDrawingShadows )
    {
        if( aDrawingShadows )
            color = m_schSettings.GetLayerColor( LAYER_SELECTION_SHADOWS );
    }
    else if( aItem->IsSelected() && ( aLayer == LAYER_DEVICE_BACKGROUND
                                   || aLayer == LAYER_SHEET_BACKGROUND ) )
    {
        // Selected items will be painted over all other items, so make backgrounds translucent so
        // that non-selected overlapping objects are visible
        color = color.WithAlpha( 0.5 );
    }

    if( m_schSettings.m_ShowDisabled
            || ( m_schSettings.m_ShowGraphicsDisabled && aItem->Type() != SCH_FIELD_T ) )
    {
        color = color.Darken( 0.5f );
    }

    if( aDimmed && !( aItem->IsSelected() && aDrawingShadows ) )
    {
        COLOR4D sheetColour = m_schSettings.GetLayerColor( LAYER_SCHEMATIC_BACKGROUND );
        color.Desaturate();
        color = color.Mix( sheetColour, 0.5f );
    }

    if( aItem->GetForcedTransparency() > 0.0 )
        color = color.WithAlpha( color.a * ( 1.0 - aItem->GetForcedTransparency() ) );

    return color;
}


float SCH_PAINTER::getLineWidth( const SCH_ITEM* aItem, bool aDrawingShadows ) const
{
    wxCHECK( aItem, static_cast<float>( schIUScale.MilsToIU( DEFAULT_LINE_WIDTH_MILS ) ) );

    int   pen = aItem->GetEffectivePenWidth( &m_schSettings );
    float width = pen;

    if( aItem->IsBrightened() || aItem->IsSelected() )
    {
        if( aDrawingShadows && aItem->IsType( g_ScaledSelectionTypes ) )
            width += getShadowWidth( aItem->IsBrightened() );
    }

    return width;
}


float SCH_PAINTER::getTextThickness( const SCH_ITEM* aItem ) const
{
    int pen = m_schSettings.GetDefaultPenWidth();

    switch( aItem->Type() )
    {
    case SCH_FIELD_T:
        pen = static_cast<const SCH_FIELD*>( aItem )->GetEffectiveTextPenWidth( pen );
        break;

    case SCH_TEXT_T:
        pen = static_cast<const SCH_TEXT*>( aItem )->GetEffectiveTextPenWidth( pen );
        break;

    case SCH_LABEL_T:
    case SCH_DIRECTIVE_LABEL_T:
    case SCH_GLOBAL_LABEL_T:
    case SCH_HIER_LABEL_T:
    case SCH_SHEET_PIN_T:
        pen = static_cast<const SCH_LABEL_BASE*>( aItem )->GetEffectiveTextPenWidth( pen );
        break;

    case SCH_TEXTBOX_T:
    case SCH_TABLECELL_T:
        pen = static_cast<const SCH_TEXTBOX*>( aItem )->GetEffectiveTextPenWidth( pen );
        break;

    default:
        UNIMPLEMENTED_FOR( aItem->GetClass() );
    }

    return (float) pen;
}


int SCH_PAINTER::getOperatingPointTextSize() const
{
    int docTextSize = schIUScale.MilsToIU( 50 );
    int screenTextSize = std::abs( (int) m_gal->GetScreenWorldMatrix().GetScale().y * 7 );

    // 66% zoom-relative
    return KiROUND( ( docTextSize + screenTextSize * 2 ) / 3 );
}


static bool isFieldsLayer( int aLayer )
{
    return aLayer == LAYER_REFERENCEPART
        || aLayer == LAYER_VALUEPART
        || aLayer == LAYER_INTERSHEET_REFS
        || aLayer == LAYER_NETCLASS_REFS
        || aLayer == LAYER_FIELDS
        || aLayer == LAYER_SHEETNAME
        || aLayer == LAYER_SHEETFILENAME
        || aLayer == LAYER_SHEETFIELDS;
}


void SCH_PAINTER::strokeText( const wxString& aText, const VECTOR2D& aPosition,
                              const TEXT_ATTRIBUTES& aAttrs,
                              const KIFONT::METRICS& aFontMetrics )
{
    KIFONT::FONT* font = aAttrs.m_Font;

    if( !font )
    {
        font = KIFONT::FONT::GetFont( eeconfig()->m_Appearance.default_font, aAttrs.m_Bold,
                                      aAttrs.m_Italic );
    }

    m_gal->SetIsFill( font->IsOutline() );
    m_gal->SetIsStroke( font->IsStroke() );

    font->Draw( m_gal, aText, aPosition, aAttrs, aFontMetrics );
}


void SCH_PAINTER::bitmapText( const wxString& aText, const VECTOR2D& aPosition,
                              const TEXT_ATTRIBUTES& aAttrs )
{
    // Bitmap font has different metrics from the stroke font so we compensate a bit before
    // stroking
    m_gal->SetGlyphSize( VECTOR2I( aAttrs.m_Size.x, KiROUND( aAttrs.m_Size.y * 1.05 ) ) );
    m_gal->SetLineWidth( (float) aAttrs.m_StrokeWidth * 1.35f );

    m_gal->SetHorizontalJustify( aAttrs.m_Halign );
    m_gal->SetVerticalJustify( aAttrs.m_Valign );

    m_gal->BitmapText( aText, aPosition, aAttrs.m_Angle );
}


void SCH_PAINTER::knockoutText( const wxString& aText, const VECTOR2D& aPosition,
                                const TEXT_ATTRIBUTES& aAttrs, const KIFONT::METRICS& aFontMetrics )
{
    TEXT_ATTRIBUTES attrs( aAttrs );
    KIFONT::FONT*   font = aAttrs.m_Font;

    if( !font )
    {
        font = KIFONT::FONT::GetFont( eeconfig()->m_Appearance.default_font, attrs.m_Bold,
                                      attrs.m_Italic );
    }

    KIGFX::GAL_DISPLAY_OPTIONS empty_opts;
    SHAPE_POLY_SET             knockouts;

    CALLBACK_GAL callback_gal( empty_opts,
            // Polygon callback
            [&]( const SHAPE_LINE_CHAIN& aPoly )
            {
                knockouts.AddOutline( aPoly );
            } );

    callback_gal.SetIsFill( false );
    callback_gal.SetIsStroke( true );
    callback_gal.SetLineWidth( (float) attrs.m_StrokeWidth );
    font->Draw( &callback_gal, aText, aPosition, attrs, aFontMetrics );

    BOX2I          bbox = knockouts.BBox( attrs.m_StrokeWidth * 2 );
    SHAPE_POLY_SET finalPoly;

    finalPoly.NewOutline();
    finalPoly.Append( bbox.GetLeft(),  bbox.GetTop() );
    finalPoly.Append( bbox.GetRight(), bbox.GetTop() );
    finalPoly.Append( bbox.GetRight(), bbox.GetBottom() );
    finalPoly.Append( bbox.GetLeft(),  bbox.GetBottom() );

    finalPoly.BooleanSubtract( knockouts, SHAPE_POLY_SET::PM_FAST );
    finalPoly.Fracture( SHAPE_POLY_SET::PM_FAST );

    m_gal->SetIsStroke( false );
    m_gal->SetIsFill( true );
    m_gal->SetFillColor( attrs.m_Color );
    m_gal->DrawPolygon( finalPoly );
}


void SCH_PAINTER::boxText( const wxString& aText, const VECTOR2D& aPosition,
                           const TEXT_ATTRIBUTES& aAttrs, const KIFONT::METRICS& aFontMetrics )
{
    KIFONT::FONT* font = aAttrs.m_Font;

    if( !font )
    {
        font = KIFONT::FONT::GetFont( eeconfig()->m_Appearance.default_font, aAttrs.m_Bold,
                                      aAttrs.m_Italic );
    }

    VECTOR2I extents = font->StringBoundaryLimits( aText, aAttrs.m_Size, aAttrs.m_StrokeWidth,
                                                   aAttrs.m_Bold, aAttrs.m_Italic, aFontMetrics );
    BOX2I box( aPosition, VECTOR2I( extents.x, aAttrs.m_Size.y ) );

    switch( aAttrs.m_Halign )
    {
    case GR_TEXT_H_ALIGN_LEFT:                                                         break;
    case GR_TEXT_H_ALIGN_CENTER:        box.SetX( box.GetX() - box.GetWidth() / 2 );   break;
    case GR_TEXT_H_ALIGN_RIGHT:         box.SetX( box.GetX() - box.GetWidth() );       break;
    case GR_TEXT_H_ALIGN_INDETERMINATE: wxFAIL_MSG( wxT( "Legal only in dialogs" ) );  break;
    }

    switch(  aAttrs.m_Valign )
    {
    case GR_TEXT_V_ALIGN_TOP:                                                          break;
    case GR_TEXT_V_ALIGN_CENTER:        box.SetY( box.GetY() - box.GetHeight() / 2 );  break;
    case GR_TEXT_V_ALIGN_BOTTOM:        box.SetY( box.GetY() - box.GetHeight() );      break;
    case GR_TEXT_V_ALIGN_INDETERMINATE: wxFAIL_MSG( wxT( "Legal only in dialogs" ) );  break;
    }

    // Give the highlight a bit of margin.
    box.Inflate( 0, aAttrs.m_StrokeWidth * 2 );

    box.Normalize();       // Make h and v sizes always >= 0
    box = box.GetBoundingBoxRotated( aPosition, aAttrs.m_Angle );

    m_gal->SetIsFill( true );
    m_gal->SetIsStroke( false );
    m_gal->DrawRectangle( box.GetOrigin(), box.GetEnd() );
}


void SCH_PAINTER::triLine( const VECTOR2D &a, const VECTOR2D &b, const VECTOR2D &c )
{
    m_gal->DrawLine( a, b );
    m_gal->DrawLine( b, c );
}


void SCH_PAINTER::draw( const LIB_SYMBOL* aSymbol, int aLayer, bool aDrawFields, int aUnit,
                        int aBodyStyle, bool aDimmed )
{
    if( !aUnit )
        aUnit = m_schSettings.m_ShowUnit;

    if( !aBodyStyle )
        aBodyStyle = m_schSettings.m_ShowBodyStyle;

    std::unique_ptr< LIB_SYMBOL > tmpSymbol;
    const LIB_SYMBOL* drawnSymbol = aSymbol;

    if( aSymbol->IsAlias() )
    {
        tmpSymbol = aSymbol->Flatten();
        drawnSymbol = tmpSymbol.get();
    }

    // The parent must exist on the union of all its children's draw layers.  But that doesn't
    // mean we want to draw each child on the union.
    auto childOnLayer =
            []( const SCH_ITEM& item, int layer )
            {
                int layers[512], layers_count;
                item.ViewGetLayers( layers, layers_count );

                for( int ii = 0; ii < layers_count; ++ii )
                {
                    if( layers[ii] == layer )
                        return true;
                }

                return false;
            };

    for( const SCH_ITEM& item : drawnSymbol->GetDrawItems() )
    {
        if( !aDrawFields && item.Type() == SCH_FIELD_T )
            continue;

        if( !childOnLayer( item, aLayer ) )
            continue;

        if( aUnit && item.GetUnit() && aUnit != item.GetUnit() )
            continue;

        if( aBodyStyle && item.GetBodyStyle() && aBodyStyle != item.GetBodyStyle() )
            continue;

        draw( &item, aLayer, aDimmed );
    }
}


bool SCH_PAINTER::setDeviceColors( const SCH_ITEM* aItem, int aLayer, bool aDimmed )
{
    COLOR4D          bg = m_schSettings.GetLayerColor( LAYER_SCHEMATIC_BACKGROUND );
    const EDA_SHAPE* shape = dynamic_cast<const EDA_SHAPE*>( aItem );

    switch( aLayer )
    {
    case LAYER_SELECTION_SHADOWS:
        if( aItem->IsBrightened() || aItem->IsSelected() )
        {
            m_gal->SetIsFill( false );
            m_gal->SetIsStroke( true );
            m_gal->SetLineWidth( getLineWidth( aItem, true ) );
            m_gal->SetStrokeColor( getRenderColor( aItem, LAYER_DEVICE, true, aDimmed ) );
            m_gal->SetFillColor( getRenderColor( aItem, LAYER_DEVICE, true, aDimmed ) );
            return true;
        }

        return false;

    case LAYER_NOTES_BACKGROUND:
    case LAYER_DEVICE_BACKGROUND:
        if( shape )
        {
            if( shape->GetFillMode() != FILL_T::FILLED_WITH_BG_BODYCOLOR )
            {
                return false;   // FILLED_SHAPE and FILLED_WITH_COLOR rendered below on
                                // LAYER_NOTES or LAYER_DEVICE
            }

            m_gal->SetIsFill( true );
            m_gal->SetFillColor( getRenderColor( aItem, LAYER_DEVICE_BACKGROUND, false, aDimmed ) );
            m_gal->SetIsStroke( false );
            return true;
        }

        return false;

    case LAYER_NOTES:
    case LAYER_PRIVATE_NOTES:
    case LAYER_DEVICE:
        m_gal->SetIsFill( shape && (   shape->GetFillMode() == FILL_T::FILLED_SHAPE
                                    || shape->GetFillMode() == FILL_T::FILLED_WITH_COLOR ) );

        if( shape && shape->GetFillMode() == FILL_T::FILLED_SHAPE )
        {
            m_gal->SetFillColor( getRenderColor( aItem, LAYER_DEVICE, false, aDimmed ) );
        }
        else if( shape && shape->GetFillMode() == FILL_T::FILLED_WITH_COLOR )
        {
            COLOR4D fillColour = shape->GetFillColor();
            double  transparency = aItem->GetForcedTransparency();

            if( transparency > 0.0 )
                fillColour = fillColour.WithAlpha( fillColour.a * ( 1.0 - transparency ) );

            if( m_schSettings.m_OverrideItemColors )
            {
                fillColour = getRenderColor( aItem, LAYER_DEVICE_BACKGROUND, false, aDimmed );
            }
            else if( aDimmed )
            {
                fillColour = fillColour.Mix( bg, 0.5f );
                fillColour.Desaturate();
            }

            m_gal->SetFillColor( fillColour );
        }

        if( aItem->GetPenWidth() >= 0 || !shape || !shape->IsFilled() )
        {
            m_gal->SetIsStroke( true );
            m_gal->SetLineWidth( getLineWidth( aItem, false ) );
            m_gal->SetStrokeColor( getRenderColor( aItem, aLayer, false, aDimmed ) );
        }
        else
        {
            m_gal->SetIsStroke( false );
        }

        return true;

    default:
        return false;
    }
}


int SCH_PAINTER::internalPinDecoSize( const SCH_PIN &aPin )
{
    if( m_schSettings.m_PinSymbolSize > 0 )
        return m_schSettings.m_PinSymbolSize;

    return aPin.GetNameTextSize() != 0 ? aPin.GetNameTextSize() / 2 : aPin.GetNumberTextSize() / 2;
}


// Utility for getting the size of the 'external' pin decorators (as a radius)
// i.e. the negation circle, the polarity 'slopes' and the nonlogic marker
int SCH_PAINTER::externalPinDecoSize( const SCH_PIN &aPin )
{
    if( m_schSettings.m_PinSymbolSize > 0 )
        return m_schSettings.m_PinSymbolSize;

    return aPin.GetNumberTextSize() / 2;
}


// Draw the target (an open circle) for a pin which has no connection or is being moved.
void SCH_PAINTER::drawPinDanglingIndicator( const VECTOR2I& aPos, const COLOR4D& aColor,
                                            bool aDrawingShadows, bool aBrightened )
{
    // Dangling symbols must be drawn in a slightly different colour so they can be seen when
    // they overlap with a junction dot.
    m_gal->SetStrokeColor( aColor.Brightened( 0.3 ) );

    m_gal->SetIsFill( false );
    m_gal->SetIsStroke( true );
    m_gal->SetLineWidth( aDrawingShadows ? getShadowWidth( aBrightened )
                                         : m_schSettings.GetDanglingIndicatorThickness() );

    m_gal->DrawCircle( aPos, TARGET_PIN_RADIUS );
}


void SCH_PAINTER::draw( const SCH_PIN* aPin, int aLayer, bool aDimmed )
{
    if( !isUnitAndConversionShown( aPin ) )
        return;

    bool drawingShadows = aLayer == LAYER_SELECTION_SHADOWS;

    if( m_schSettings.IsPrinting() && drawingShadows )
        return;

    bool drawingDangling = aLayer == LAYER_DANGLING;
    bool drawingOP = aLayer == LAYER_OP_CURRENTS;
    bool isDangling = m_schSettings.m_IsSymbolEditor || aPin->HasFlag( IS_DANGLING );

    if( drawingShadows && !( aPin->IsBrightened() || aPin->IsSelected() ) )
        return;

    VECTOR2I pos = aPin->GetPosition();
    COLOR4D  color = getRenderColor( aPin, LAYER_PIN, drawingShadows, aDimmed );

    if( !aPin->IsVisible() )
    {
        if( m_schSettings.IsPrinting() )
            return;

        bool force_show = m_schematic ? eeconfig()->m_Appearance.show_hidden_pins
                                      : m_schSettings.m_ShowHiddenPins;

        if( force_show )
        {
            color = getRenderColor( aPin, LAYER_HIDDEN, drawingShadows, aDimmed );
        }
        else
        {
            if( drawingDangling && isDangling && aPin->IsGlobalPower() )
                drawPinDanglingIndicator( pos, color, drawingShadows, aPin->IsBrightened() );

            return;
        }
    }

    if( drawingDangling )
    {
        if( isDangling )
            drawPinDanglingIndicator( pos, color, drawingShadows, aPin->IsBrightened() );

        return;
    }

    if( m_schSettings.GetDrawBoundingBoxes() )
        drawItemBoundingBox( aPin );

    VECTOR2I p0 = aPin->GetPinRoot();
    VECTOR2I dir( sign( pos.x - p0.x ), sign( pos.y - p0.y ) );
    int      len = aPin->GetLength();

    if( drawingOP && !aPin->GetOperatingPoint().IsEmpty() )
    {
        int             textSize = getOperatingPointTextSize();
        VECTOR2I        mid = ( p0 + pos ) / 2;
        int             textOffset = KiROUND( textSize * 0.22 );
        TEXT_ATTRIBUTES attrs;

        if( len > textSize )
        {
            if( dir.x == 0 )
            {
                mid.x += KiROUND( textOffset * 1.2 );
                attrs.m_Angle = ANGLE_HORIZONTAL;
            }
            else
            {
                mid.y -= KiROUND( textOffset * 1.2 );
                attrs.m_Angle = ANGLE_VERTICAL;
            }

            attrs.m_Halign = GR_TEXT_H_ALIGN_LEFT;
            attrs.m_Valign = GR_TEXT_V_ALIGN_CENTER;

            attrs.m_Font = KIFONT::FONT::GetFont();  // always use stroke font for performance
            attrs.m_Size = VECTOR2I( textSize, textSize );
            attrs.m_StrokeWidth = GetPenSizeForDemiBold( textSize );
            attrs.m_Color = m_schSettings.GetLayerColor( LAYER_OP_CURRENTS );

            knockoutText( aPin->GetOperatingPoint(), mid, attrs, aPin->GetFontMetrics() );
        }
    }

    if( drawingOP )
        return;

    VECTOR2D pc;

    m_gal->SetIsStroke( true );
    m_gal->SetIsFill( false );
    m_gal->SetLineWidth( getLineWidth( aPin, drawingShadows ) );
    m_gal->SetStrokeColor( color );
    m_gal->SetFontBold( false );
    m_gal->SetFontUnderlined( false );
    m_gal->SetFontItalic( false );

    const int radius = externalPinDecoSize( *aPin );
    const int diam = radius*2;
    const int clock_size = internalPinDecoSize( *aPin );

    if( aPin->GetType() == ELECTRICAL_PINTYPE::PT_NC ) // Draw a N.C. symbol
    {
        m_gal->DrawLine( p0, pos );

        m_gal->DrawLine( pos + VECTOR2D( -1, -1 ) * TARGET_PIN_RADIUS,
                         pos + VECTOR2D(  1,  1 ) * TARGET_PIN_RADIUS );
        m_gal->DrawLine( pos + VECTOR2D(  1, -1 ) * TARGET_PIN_RADIUS ,
                         pos + VECTOR2D( -1,  1 ) * TARGET_PIN_RADIUS );
    }
    else
    {
        switch( aPin->GetShape() )
        {
        default:
        case GRAPHIC_PINSHAPE::LINE:
            m_gal->DrawLine( p0, pos );
            break;

        case GRAPHIC_PINSHAPE::INVERTED:
            m_gal->DrawCircle( p0 + dir * radius, radius );
            m_gal->DrawLine( p0 + dir * ( diam ), pos );
            break;

        case GRAPHIC_PINSHAPE::INVERTED_CLOCK:
            pc = p0 - dir * clock_size ;

            triLine( p0 + VECTOR2D( dir.y, -dir.x) * clock_size,
                     pc,
                     p0 + VECTOR2D( -dir.y, dir.x) * clock_size );

            m_gal->DrawCircle( p0 + dir * radius, radius );
            m_gal->DrawLine( p0 + dir * ( diam ), pos );
            break;

        case GRAPHIC_PINSHAPE::CLOCK_LOW:
        case GRAPHIC_PINSHAPE::FALLING_EDGE_CLOCK:
            pc = p0 - dir * clock_size ;

            triLine( p0 + VECTOR2D( dir.y, -dir.x) * clock_size,
                     pc,
                     p0 + VECTOR2D( -dir.y, dir.x) * clock_size );

            if( !dir.y )
            {
                triLine( p0 + VECTOR2D(dir.x, 0) * diam,
                         p0 + VECTOR2D(dir.x, -1) * diam,
                         p0 );
            }
            else    /* MapX1 = 0 */
            {
                triLine( p0 + VECTOR2D( 0, dir.y) * diam,
                         p0 + VECTOR2D(-1, dir.y) * diam,
                         p0 );
            }

            m_gal->DrawLine( p0, pos );
            break;

        case GRAPHIC_PINSHAPE::CLOCK:
            m_gal->DrawLine( p0, pos );

            if( !dir.y )
            {
                triLine( p0 + VECTOR2D( 0, clock_size ),
                         p0 + VECTOR2D( -dir.x * clock_size, 0 ),
                         p0 + VECTOR2D( 0, -clock_size ) );
            }
            else
            {
                triLine( p0 + VECTOR2D( clock_size, 0 ),
                         p0 + VECTOR2D( 0, -dir.y * clock_size ),
                         p0 + VECTOR2D( -clock_size, 0 ) );
            }
            break;

        case GRAPHIC_PINSHAPE::INPUT_LOW:
            m_gal->DrawLine( p0, pos );

            if( !dir.y )
            {
                triLine( p0 + VECTOR2D(dir.x, 0) * diam,
                         p0 + VECTOR2D(dir.x, -1) * diam,
                         p0 );
            }
            else    /* MapX1 = 0 */
            {
                triLine( p0 + VECTOR2D( 0, dir.y) * diam,
                         p0 + VECTOR2D(-1, dir.y) * diam,
                         p0 );
            }
            break;

        case GRAPHIC_PINSHAPE::OUTPUT_LOW: // IEEE symbol "Active Low Output"
            m_gal->DrawLine( p0, pos );

            if( !dir.y )    // Horizontal pin
                m_gal->DrawLine( p0 - VECTOR2D( 0, diam ), p0 + VECTOR2D( dir.x, 0 ) * diam );
            else            // Vertical pin
                m_gal->DrawLine( p0 - VECTOR2D( diam, 0 ), p0 + VECTOR2D( 0, dir.y ) * diam );
            break;

        case GRAPHIC_PINSHAPE::NONLOGIC: // NonLogic pin symbol
            m_gal->DrawLine( p0, pos );

            m_gal->DrawLine( p0 - VECTOR2D( dir.x + dir.y, dir.y - dir.x ) * radius,
                             p0 + VECTOR2D( dir.x + dir.y, dir.y - dir.x ) * radius );
            m_gal->DrawLine( p0 - VECTOR2D( dir.x - dir.y, dir.x + dir.y ) * radius,
                             p0 + VECTOR2D( dir.x - dir.y, dir.x + dir.y ) * radius );
            break;
        }
    }

    if( drawingShadows && !eeconfig()->m_Selection.draw_selected_children )
        return;

    // Draw the labels
    const SYMBOL* symbol = aPin->GetParentSymbol();
    float         penWidth = (float) m_schSettings.GetDefaultPenWidth();
    int           textOffset = symbol->GetPinNameOffset();
    float         nameStrokeWidth = getLineWidth( aPin, false );
    float         numStrokeWidth = getLineWidth( aPin, false );
    bool          showPinNames = symbol->GetShowPinNames();
    bool          showPinNumbers = m_schSettings.m_ShowPinNumbers || symbol->GetShowPinNumbers();

    nameStrokeWidth = Clamp_Text_PenSize( nameStrokeWidth, aPin->GetNameTextSize(), true );
    numStrokeWidth = Clamp_Text_PenSize( numStrokeWidth, aPin->GetNumberTextSize(), true );

    float PIN_TEXT_MARGIN = schIUScale.MilsToIU( KiROUND( 24 * m_schSettings.m_TextOffsetRatio ) );

    // Four locations around a pin where text can be drawn
    enum { INSIDE = 0, OUTSIDE, ABOVE, BELOW };
    int      size[4] = { 0, 0, 0, 0 };
    float    thickness[4] = { numStrokeWidth, numStrokeWidth, numStrokeWidth, numStrokeWidth };
    COLOR4D  colour[4];
    wxString text[4];

    // TextOffset > 0 means pin NAMES on inside, pin NUMBERS above and nothing below
    if( textOffset )
    {
        size     [INSIDE] = showPinNames ? aPin->GetNameTextSize() : 0;
        thickness[INSIDE] = nameStrokeWidth;
        colour   [INSIDE] = getRenderColor( aPin, LAYER_PINNAM, drawingShadows, aDimmed );
        text     [INSIDE] = aPin->GetShownName();

        size     [ABOVE] = showPinNumbers ? aPin->GetNumberTextSize() : 0;
        thickness[ABOVE] = numStrokeWidth;
        colour   [ABOVE] = getRenderColor( aPin, LAYER_PINNUM, drawingShadows, aDimmed );
        text     [ABOVE] = aPin->GetShownNumber();
    }
    // Otherwise if both are shown pin NAMES go above and pin NUMBERS go below
    else if( showPinNames && showPinNumbers )
    {
        size     [ABOVE] = aPin->GetNameTextSize();
        thickness[ABOVE] = nameStrokeWidth;
        colour   [ABOVE] = getRenderColor( aPin, LAYER_PINNAM, drawingShadows, aDimmed );
        text     [ABOVE] = aPin->GetShownName();

        size     [BELOW] = aPin->GetNumberTextSize();
        thickness[BELOW] = numStrokeWidth;
        colour   [BELOW] = getRenderColor( aPin, LAYER_PINNUM, drawingShadows, aDimmed );
        text     [BELOW] = aPin->GetShownNumber();
    }
    else if( showPinNames )
    {
        size     [ABOVE] = aPin->GetNameTextSize();
        thickness[ABOVE] = nameStrokeWidth;
        colour   [ABOVE] = getRenderColor( aPin, LAYER_PINNAM, drawingShadows, aDimmed );
        text     [ABOVE] = aPin->GetShownName();
    }
    else if( showPinNumbers )
    {
        size     [ABOVE] = aPin->GetNumberTextSize();
        thickness[ABOVE] = numStrokeWidth;
        colour   [ABOVE] = getRenderColor( aPin, LAYER_PINNUM, drawingShadows, aDimmed );
        text     [ABOVE] = aPin->GetShownNumber();
    }

    if( m_schSettings.m_ShowPinsElectricalType )
    {
        size     [OUTSIDE] = std::max( aPin->GetNameTextSize() * 3 / 4, schIUScale.mmToIU( 0.7 ) );
        thickness[OUTSIDE] = float( size[OUTSIDE] ) / 8.0f;
        colour   [OUTSIDE] = getRenderColor( aPin, LAYER_PRIVATE_NOTES, drawingShadows, aDimmed );
        text     [OUTSIDE] = aPin->GetElectricalTypeName();
    }

    // Rendering text is expensive (particularly when using outline fonts).  At small effective
    // sizes (ie: zoomed out) the visual differences between outline and/or stroke fonts and the
    // bitmap font becomes immaterial, and there's often more to draw when zoomed out so the
    // performance gain becomes more significant.
    #define BITMAP_FONT_SIZE_THRESHOLD 3.5

    bool renderTextAsBitmap = size[0] * m_gal->GetWorldScale() < BITMAP_FONT_SIZE_THRESHOLD
                           && size[1] * m_gal->GetWorldScale() < BITMAP_FONT_SIZE_THRESHOLD
                           && size[2] * m_gal->GetWorldScale() < BITMAP_FONT_SIZE_THRESHOLD
                           && size[3] * m_gal->GetWorldScale() < BITMAP_FONT_SIZE_THRESHOLD;

    if( !aPin->IsVisible() )
    {
        for( COLOR4D& c : colour )
            c = getRenderColor( aPin, LAYER_HIDDEN, drawingShadows, aDimmed );
    }

    float insideOffset  = (float) textOffset                  - thickness[INSIDE]  / 2.0f;
    float outsideOffset = PIN_TEXT_MARGIN + TARGET_PIN_RADIUS - thickness[OUTSIDE] / 2.0f;
    float aboveOffset   = PIN_TEXT_MARGIN + penWidth / 2.0f   + thickness[ABOVE]   / 2.0f;
    float belowOffset   = PIN_TEXT_MARGIN + penWidth / 2.0f   + thickness[BELOW]   / 2.0f;

    if( isDangling )
        outsideOffset += TARGET_PIN_RADIUS / 2.0f;

    if( drawingShadows )
    {
        float shadowWidth = getShadowWidth( aPin->IsBrightened() );

        for( float& t : thickness )
            t += shadowWidth;

        // Due to the fact a shadow text in position INSIDE or OUTSIDE is drawn left or right aligned,
        // it needs an offset = shadowWidth/2 to be drawn at the same place as normal text
        // texts drawn as GR_TEXT_H_ALIGN_CENTER do not need a specific offset.
        // this offset is shadowWidth/2 but for some reason we need to slightly modify this offset
        // for a better look (better alignment of shadow shape), for KiCad font only
        if( !KIFONT::FONT::GetFont( eeconfig()->m_Appearance.default_font )->IsOutline() )
        {
            const float adjust = 1.2f;      // Value chosen after tests
            float shadowOffset = shadowWidth/2.0f * adjust;
            insideOffset -= shadowOffset;
            outsideOffset -= shadowOffset;
        }
    }

    auto drawText =
            [&]( int i, const VECTOR2D& aPos, GR_TEXT_H_ALIGN_T hAlign, GR_TEXT_V_ALIGN_T vAlign,
                 const EDA_ANGLE& aAngle )
            {
                if( text[i].IsEmpty() )
                    return;

                // Which of these gets used depends on the font technology, so set both
                m_gal->SetStrokeColor( colour[i] );
                m_gal->SetFillColor( colour[i] );

                TEXT_ATTRIBUTES attrs;
                attrs.m_Font = KIFONT::FONT::GetFont( eeconfig()->m_Appearance.default_font );
                attrs.m_Size = VECTOR2I( size[i], size[i] );
                attrs.m_Halign = hAlign;
                attrs.m_Valign = vAlign;
                attrs.m_Angle = aAngle;
                attrs.m_StrokeWidth = KiROUND( thickness[i] );

                if( drawingShadows && !attrs.m_Font->IsOutline() )
                {
                    strokeText( text[i], aPos, attrs, aPin->GetFontMetrics() );
                }
                else if( drawingShadows )
                {
                    boxText( text[i], aPos, attrs, aPin->GetFontMetrics() );
                }
                else if( nonCached( aPin ) && renderTextAsBitmap )
                {
                    bitmapText( text[i], aPos, attrs );
                    const_cast<SCH_PIN*>( aPin )->SetFlags( IS_SHOWN_AS_BITMAP );
                }
                else
                {
                    strokeText( text[i], aPos, attrs, aPin->GetFontMetrics() );
                    const_cast<SCH_PIN*>( aPin )->SetFlags( IS_SHOWN_AS_BITMAP );
                }
            };

    switch( aPin->GetOrientation() )
    {
    case PIN_ORIENTATION::PIN_LEFT:
        if( size[INSIDE] )
        {
            drawText( INSIDE, pos + VECTOR2D( -insideOffset - (float) len, 0 ),
                      GR_TEXT_H_ALIGN_RIGHT, GR_TEXT_V_ALIGN_CENTER, ANGLE_HORIZONTAL );
        }
        if( size[OUTSIDE] )
        {
            drawText( OUTSIDE, pos + VECTOR2D( outsideOffset, 0 ),
                      GR_TEXT_H_ALIGN_LEFT, GR_TEXT_V_ALIGN_CENTER, ANGLE_HORIZONTAL );
        }
        if( size[ABOVE] )
        {
            drawText( ABOVE, pos + VECTOR2D( -len / 2.0, -aboveOffset ),
                      GR_TEXT_H_ALIGN_CENTER, GR_TEXT_V_ALIGN_BOTTOM, ANGLE_HORIZONTAL );
        }
        if( size[BELOW] )
        {
            drawText( BELOW, pos + VECTOR2D( -len / 2.0, belowOffset ),
                      GR_TEXT_H_ALIGN_CENTER, GR_TEXT_V_ALIGN_TOP, ANGLE_HORIZONTAL );
        }
        break;

    case PIN_ORIENTATION::PIN_RIGHT:
        if( size[INSIDE] )
        {
            drawText( INSIDE, pos + VECTOR2D( insideOffset + (float) len, 0 ),
                      GR_TEXT_H_ALIGN_LEFT, GR_TEXT_V_ALIGN_CENTER, ANGLE_HORIZONTAL );
        }
        if( size[OUTSIDE] )
        {
            drawText( OUTSIDE, pos + VECTOR2D( -outsideOffset, 0 ),
                      GR_TEXT_H_ALIGN_RIGHT, GR_TEXT_V_ALIGN_CENTER, ANGLE_HORIZONTAL );
        }
        if( size[ABOVE] )
        {
            drawText( ABOVE, pos + VECTOR2D( len / 2.0, -aboveOffset ),
                      GR_TEXT_H_ALIGN_CENTER, GR_TEXT_V_ALIGN_BOTTOM, ANGLE_HORIZONTAL );
        }
        if( size[BELOW] )
        {
            drawText( BELOW, pos + VECTOR2D( len / 2.0, belowOffset ),
                      GR_TEXT_H_ALIGN_CENTER, GR_TEXT_V_ALIGN_TOP, ANGLE_HORIZONTAL );
        }
        break;

    case PIN_ORIENTATION::PIN_DOWN:
        if( size[INSIDE] )
        {
            drawText( INSIDE, pos + VECTOR2D( 0, insideOffset + (float) len ),
                      GR_TEXT_H_ALIGN_RIGHT, GR_TEXT_V_ALIGN_CENTER, ANGLE_VERTICAL );
        }
        if( size[OUTSIDE] )
        {
            drawText( OUTSIDE, pos + VECTOR2D( 0, -outsideOffset ),
                      GR_TEXT_H_ALIGN_LEFT, GR_TEXT_V_ALIGN_CENTER, ANGLE_VERTICAL );
        }
        if( size[ABOVE] )
        {
            drawText( ABOVE, pos + VECTOR2D( -aboveOffset, len / 2.0 ),
                      GR_TEXT_H_ALIGN_CENTER, GR_TEXT_V_ALIGN_BOTTOM, ANGLE_VERTICAL );
        }
        if( size[BELOW] )
        {
            drawText( BELOW, pos + VECTOR2D( belowOffset, len / 2.0 ),
                      GR_TEXT_H_ALIGN_CENTER, GR_TEXT_V_ALIGN_TOP, ANGLE_VERTICAL );
        }
        break;

    case PIN_ORIENTATION::PIN_UP:
        if( size[INSIDE] )
        {
            drawText( INSIDE, pos + VECTOR2D( 0, -insideOffset - (float) len ),
                      GR_TEXT_H_ALIGN_LEFT, GR_TEXT_V_ALIGN_CENTER, ANGLE_VERTICAL );
        }
        if( size[OUTSIDE] )
        {
            drawText( OUTSIDE, pos + VECTOR2D( 0, outsideOffset ),
                      GR_TEXT_H_ALIGN_RIGHT, GR_TEXT_V_ALIGN_CENTER, ANGLE_VERTICAL );
        }
        if( size[ABOVE] )
        {
            drawText( ABOVE, pos + VECTOR2D( -aboveOffset, -len / 2.0 ),
                      GR_TEXT_H_ALIGN_CENTER, GR_TEXT_V_ALIGN_BOTTOM, ANGLE_VERTICAL );
        }
        if( size[BELOW] )
        {
            drawText( BELOW, pos + VECTOR2D( belowOffset, -len / 2.0 ),
                      GR_TEXT_H_ALIGN_CENTER, GR_TEXT_V_ALIGN_TOP, ANGLE_VERTICAL );
        }
        break;

    default:
        wxFAIL_MSG( "Unknown pin orientation" );
    }
}


// Draw the target (an open square) for a wire or label which has no connection or is
// being moved.
void SCH_PAINTER::drawDanglingIndicator( const VECTOR2I& aPos, const COLOR4D& aColor, int aWidth,
                                         bool aDangling, bool aDrawingShadows, bool aBrightened )
{
    if( m_schSettings.IsPrinting() )
        return;

    int size = aDangling ? DANGLING_SYMBOL_SIZE : UNSELECTED_END_SIZE;

    if( !aDangling )
        aWidth /= 2;

    VECTOR2I radius( aWidth + schIUScale.MilsToIU( size / 2 ),
                     aWidth + schIUScale.MilsToIU( size / 2 ) );

    // Dangling symbols must be drawn in a slightly different colour so they can be seen when
    // they overlap with a junction dot.
    m_gal->SetStrokeColor( aColor.Brightened( 0.3 ) );
    m_gal->SetIsStroke( true );
    m_gal->SetIsFill( false );
    m_gal->SetLineWidth( aDrawingShadows ? getShadowWidth( aBrightened )
                                         : m_schSettings.GetDanglingIndicatorThickness() );

    m_gal->DrawRectangle( aPos - radius, aPos + radius );
}


void SCH_PAINTER::draw( const SCH_JUNCTION* aJct, int aLayer )
{
    bool drawingShadows = aLayer == LAYER_SELECTION_SHADOWS;

    if( m_schSettings.IsPrinting() && drawingShadows )
        return;

    if( drawingShadows && !( aJct->IsBrightened() || aJct->IsSelected() ) )
        return;

    COLOR4D color = getRenderColor( aJct, aJct->GetLayer(), drawingShadows );

    int junctionSize = aJct->GetEffectiveDiameter() / 2;

    if( junctionSize > 1 )
    {
        m_gal->SetIsStroke( drawingShadows );
        m_gal->SetLineWidth( getLineWidth( aJct, drawingShadows ) );
        m_gal->SetStrokeColor( color );
        m_gal->SetIsFill( !drawingShadows );
        m_gal->SetFillColor( color );
        m_gal->DrawCircle( aJct->GetPosition(), junctionSize );
    }
}


void SCH_PAINTER::draw( const SCH_LINE* aLine, int aLayer )
{
    bool drawingShadows = aLayer == LAYER_SELECTION_SHADOWS;

    if( m_schSettings.IsPrinting() && drawingShadows )
        return;

    bool drawingDangling = aLayer == LAYER_DANGLING;
    bool drawingOP = aLayer == LAYER_OP_VOLTAGES;

    if( drawingShadows && !( aLine->IsBrightened() || aLine->IsSelected() ) )
        return;

    // Line end dangling status isn't updated until the line is finished drawing, so don't warn
    // them about ends that are probably connected
    if( aLine->IsNew() && drawingDangling )
        return;

    COLOR4D    color = getRenderColor( aLine, aLine->GetLayer(), drawingShadows );
    float      width = getLineWidth( aLine, drawingShadows );
    LINE_STYLE lineStyle = aLine->GetEffectiveLineStyle();

    if( ( drawingDangling || drawingShadows ) && !aLine->IsNew() )
    {
        if( ( aLine->IsWire() && aLine->IsStartDangling() )
            || ( drawingShadows && aLine->IsSelected() && !aLine->HasFlag( STARTPOINT ) ) )
        {
            COLOR4D indicatorColor( color );

            if( drawingShadows && !aLine->HasFlag( STARTPOINT ) )
                indicatorColor.Invert();

            drawDanglingIndicator( aLine->GetStartPoint(), indicatorColor, KiROUND( width ),
                                   aLine->IsWire() && aLine->IsStartDangling(), drawingShadows,
                                   aLine->IsBrightened() );
        }

        if( ( aLine->IsWire() && aLine->IsEndDangling() )
            || ( drawingShadows && aLine->IsSelected() && !aLine->HasFlag( ENDPOINT ) ) )
        {
            COLOR4D indicatorColor( color );

            if( drawingShadows && !aLine->HasFlag( ENDPOINT ) )
                indicatorColor.Invert();

            drawDanglingIndicator( aLine->GetEndPoint(), indicatorColor, KiROUND( width ),
                                   aLine->IsWire() && aLine->IsEndDangling(), drawingShadows,
                                   aLine->IsBrightened() );
        }
    }

    if( drawingDangling )
        return;

    if( drawingOP && !aLine->GetOperatingPoint().IsEmpty() )
    {
        int             textSize = getOperatingPointTextSize();
        VECTOR2I        pos = aLine->GetMidPoint();
        int             textOffset = KiROUND( textSize * 0.22 );
        TEXT_ATTRIBUTES attrs;

        if( aLine->GetStartPoint().y == aLine->GetEndPoint().y )
        {
            pos.y -= textOffset;
            attrs.m_Halign = GR_TEXT_H_ALIGN_CENTER;
            attrs.m_Valign = GR_TEXT_V_ALIGN_BOTTOM;
        }
        else
        {
            pos.x += KiROUND( textOffset * 1.2 );
            attrs.m_Halign = GR_TEXT_H_ALIGN_LEFT;
            attrs.m_Valign = GR_TEXT_V_ALIGN_CENTER;
        }

        attrs.m_Font = KIFONT::FONT::GetFont();  // always use stroke font for performance
        attrs.m_Size = VECTOR2I( textSize, textSize );
        attrs.m_StrokeWidth = GetPenSizeForDemiBold( textSize );
        attrs.m_Color = m_schSettings.GetLayerColor( LAYER_OP_VOLTAGES );

        knockoutText( aLine->GetOperatingPoint(), pos, attrs, aLine->GetFontMetrics() );
    }

    if( drawingOP )
        return;

    m_gal->SetIsStroke( true );
    m_gal->SetStrokeColor( color );
    m_gal->SetLineWidth( width );

    if( lineStyle <= LINE_STYLE::FIRST_TYPE || drawingShadows )
    {
        m_gal->DrawLine( aLine->GetStartPoint(), aLine->GetEndPoint() );
    }
    else
    {
        SHAPE_SEGMENT line( aLine->GetStartPoint(), aLine->GetEndPoint() );

        STROKE_PARAMS::Stroke( &line, lineStyle, KiROUND( width ), &m_schSettings,
                [&]( const VECTOR2I& a, const VECTOR2I& b )
                {
                    // DrawLine has problem with 0 length lines so enforce minimum
                    if( a == b )
                        m_gal->DrawLine( a+1, b );
                    else
                        m_gal->DrawLine( a, b );
                } );
    }
}


void SCH_PAINTER::draw( const SCH_SHAPE* aShape, int aLayer, bool aDimmed )
{
    if( !isUnitAndConversionShown( aShape ) )
        return;

    if( aShape->IsPrivate() && !m_schSettings.m_IsSymbolEditor )
        return;

    bool drawingShadows = aLayer == LAYER_SELECTION_SHADOWS;

    if( m_schSettings.IsPrinting() && drawingShadows )
        return;

    LINE_STYLE lineStyle = aShape->GetEffectiveLineStyle();
    COLOR4D    color = getRenderColor( aShape, aLayer, drawingShadows );

    if( drawingShadows && !( aShape->IsBrightened() || aShape->IsSelected() ) )
        return;

    auto drawShape =
            [&]( const SCH_SHAPE* shape )
            {
                switch( shape->GetShape() )
                {
                case SHAPE_T::ARC:
                {
                    VECTOR2D start = shape->GetStart();
                    VECTOR2D mid = shape->GetArcMid();
                    VECTOR2D end = shape->GetEnd();
                    VECTOR2D center = CalcArcCenter( start, mid, end );

                    EDA_ANGLE startAngle( start - center );
                    EDA_ANGLE midAngle( mid - center );
                    EDA_ANGLE endAngle( end - center );

                    EDA_ANGLE angle1 = midAngle - startAngle;
                    EDA_ANGLE angle2 = endAngle - midAngle;

                    EDA_ANGLE angle = angle1.Normalize180() + angle2.Normalize180();

                    m_gal->DrawArc( center, ( start - center ).EuclideanNorm(), startAngle, angle );
                    break;
                }

                case SHAPE_T::CIRCLE:
                    m_gal->DrawCircle( shape->GetPosition(), shape->GetRadius() );
                    break;

                case SHAPE_T::RECTANGLE:
                    m_gal->DrawRectangle( shape->GetPosition(), shape->GetEnd() );
                    break;

                case SHAPE_T::POLY:
                {
                    const SHAPE_LINE_CHAIN poly = shape->GetPolyShape().Outline( 0 );
                    std::deque<VECTOR2D>   mappedPts;

                    for( const VECTOR2I& pt : poly.CPoints() )
                        mappedPts.push_back( pt );

                    m_gal->DrawPolygon( mappedPts );
                    break;
                }

                case SHAPE_T::BEZIER:
                {
                    m_gal->DrawCurve( shape->GetStart(), shape->GetBezierC1(),
                                      shape->GetBezierC2(), shape->GetEnd() );
                    break;
                }

                default:
                    UNIMPLEMENTED_FOR( shape->SHAPE_T_asString() );
                }
            };

    if( aLayer == LAYER_SELECTION_SHADOWS )
    {
        if( eeconfig()->m_Selection.fill_shapes )
        {
            // Consider a NAND gate.  We have no idea which side of the arc is "inside"
            // so we can't reliably fill.
            if( aShape->GetShape() == SHAPE_T::ARC )
                m_gal->SetIsFill( aShape->IsFilled() );
            else
                m_gal->SetIsFill( true );

            m_gal->SetIsStroke( false );
            m_gal->SetFillColor( color );
        }
        else
        {
            m_gal->SetIsStroke( true );
            m_gal->SetIsFill( false );
            m_gal->SetLineWidth( getLineWidth( aShape, true ) );
            m_gal->SetStrokeColor( color );
        }

        drawShape( aShape );
    }
    else if( aLayer == LAYER_DEVICE_BACKGROUND || aLayer == LAYER_NOTES_BACKGROUND )
    {
        // Do not fill the shape in B&W print mode, to avoid to visible items inside the shape
        if( aShape->IsFilled() && !m_schSettings.PrintBlackAndWhiteReq() )
        {
            if( aShape->GetFillMode() == FILL_T::FILLED_WITH_BG_BODYCOLOR )
                color = m_schSettings.GetLayerColor( LAYER_DEVICE_BACKGROUND );

            m_gal->SetIsFill( true );
            m_gal->SetIsStroke( false );
            m_gal->SetFillColor( color );

            drawShape( aShape );
        }
    }
    else if( aLayer == LAYER_DEVICE || aLayer == LAYER_NOTES || aLayer == LAYER_PRIVATE_NOTES )
    {
        float lineWidth = getLineWidth( aShape, drawingShadows );

        if( lineWidth > 0 )
        {
            m_gal->SetIsFill( false );
            m_gal->SetIsStroke( true );
            m_gal->SetLineWidth( lineWidth );
            m_gal->SetStrokeColor( color );

            if( lineStyle <= LINE_STYLE::FIRST_TYPE || drawingShadows )
            {
                drawShape( aShape );
            }
            else
            {
                std::vector<SHAPE*> shapes = aShape->MakeEffectiveShapes( true );

                for( SHAPE* shape : shapes )
                {
                    STROKE_PARAMS::Stroke( shape, lineStyle, KiROUND( lineWidth ), &m_schSettings,
                            [this]( const VECTOR2I& a, const VECTOR2I& b )
                            {
                                // DrawLine has problem with 0 length lines so enforce minimum
                                if( a == b )
                                    m_gal->DrawLine( a+1, b );
                                else
                                    m_gal->DrawLine( a, b );
                            } );
                }

                for( SHAPE* shape : shapes )
                    delete shape;
            }
        }
    }
}


void SCH_PAINTER::draw( const SCH_TEXT* aText, int aLayer, bool aDimmed )
{
    if( !isUnitAndConversionShown( aText ) )
        return;

    if( aText->IsPrivate() && !m_schSettings.m_IsSymbolEditor )
        return;

    bool drawingShadows = aLayer == LAYER_SELECTION_SHADOWS;

    if( m_schSettings.IsPrinting() && drawingShadows )
        return;

    if( drawingShadows && !( aText->IsBrightened() || aText->IsSelected() ) )
        return;

    switch( aText->Type() )
    {
    case SCH_SHEET_PIN_T:       aLayer = LAYER_SHEETLABEL;                        break;
    case SCH_HIER_LABEL_T:      aLayer = LAYER_HIERLABEL;                         break;
    case SCH_GLOBAL_LABEL_T:    aLayer = LAYER_GLOBLABEL;                         break;
    case SCH_DIRECTIVE_LABEL_T: aLayer = LAYER_NETCLASS_REFS;                     break;
    case SCH_LABEL_T:           aLayer = LAYER_LOCLABEL;                          break;
    case SCH_TEXT_T:            aLayer = aText->GetParentSymbol() ? LAYER_DEVICE
                                                                  : LAYER_NOTES;  break;
    default:                    aLayer = LAYER_NOTES;                             break;
    }

    COLOR4D color = getRenderColor( aText, aLayer, drawingShadows, aDimmed );

    if( m_schematic )
    {
        SCH_CONNECTION* conn = nullptr;

        if( !aText->IsConnectivityDirty() )
            conn = aText->Connection();

        if( conn && conn->IsBus() )
            color = getRenderColor( aText, LAYER_BUS, drawingShadows );
    }

    if( !( aText->IsVisible() || aText->IsForceVisible() ) )
    {
        if( m_schSettings.m_IsSymbolEditor || eeconfig()->m_Appearance.show_hidden_fields )
            color = getRenderColor( aText, LAYER_HIDDEN, drawingShadows );
        else
            return;
    }

    m_gal->SetStrokeColor( color );
    m_gal->SetFillColor( color );

    wxString        shownText( aText->GetShownText( true ) );
    VECTOR2I        text_offset = aText->GetSchematicTextOffset( &m_schSettings );
    TEXT_ATTRIBUTES attrs = aText->GetAttributes();
    KIFONT::FONT*   font = getFont( aText );

    attrs.m_Angle = aText->GetDrawRotation();
    attrs.m_StrokeWidth = KiROUND( getTextThickness( aText ) );

    if( drawingShadows && font->IsOutline() )
    {
        BOX2I bBox = aText->GetBoundingBox();
        bBox.Inflate( KiROUND( getTextThickness( aText ) * 2 ) );

        m_gal->SetIsStroke( false );
        m_gal->SetIsFill( true );
        m_gal->DrawRectangle( bBox.GetPosition(), bBox.GetEnd() );
    }
    else if( aText->GetLayer() == LAYER_DEVICE )
    {
        BOX2I    bBox = aText->GetBoundingBox();
        VECTOR2D pos = bBox.Centre();

        // Due to the fact a shadow text can be drawn left or right aligned, it needs to be
        // offset by shadowWidth/2 to be drawn at the same place as normal text.
        // For some reason we need to slightly modify this offset for a better look (better
        // alignment of shadow shape), for KiCad font only.
        double shadowOffset = 0.0;

        if( drawingShadows )
        {
            double shadowWidth = getShadowWidth( !aText->IsSelected() );
            attrs.m_StrokeWidth += getShadowWidth( !aText->IsSelected() );

            const double adjust = 1.2f;      // Value chosen after tests
            shadowOffset = shadowWidth/2.0f * adjust;
        }

        if( attrs.m_Angle == ANGLE_VERTICAL )
        {
            switch( attrs.m_Halign )
            {
            case GR_TEXT_H_ALIGN_LEFT:
                pos.y = bBox.GetBottom() + shadowOffset;
                break;
            case GR_TEXT_H_ALIGN_CENTER:
                pos.y = ( bBox.GetTop() + bBox.GetBottom() ) / 2.0;
                break;
            case GR_TEXT_H_ALIGN_RIGHT:
                pos.y = bBox.GetTop() - shadowOffset;
                break;
            case GR_TEXT_H_ALIGN_INDETERMINATE:
                wxFAIL_MSG( wxT( "Indeterminate state legal only in dialogs." ) );
                break;
            }
        }
        else
        {
            switch( attrs.m_Halign )
            {
            case GR_TEXT_H_ALIGN_LEFT:
                pos.x = bBox.GetLeft() - shadowOffset;
                break;
            case GR_TEXT_H_ALIGN_CENTER:
                pos.x = ( bBox.GetLeft() + bBox.GetRight() ) / 2.0;
                break;
            case GR_TEXT_H_ALIGN_RIGHT:
                pos.x = bBox.GetRight() + shadowOffset;
                break;
            case GR_TEXT_H_ALIGN_INDETERMINATE:
                wxFAIL_MSG( wxT( "Indeterminate state legal only in dialogs." ) );
                break;
            }
        }

        // Because the text vertical position is the bounding box center, the text is drawn as
        // vertically centered.
        attrs.m_Valign = GR_TEXT_V_ALIGN_CENTER;

        strokeText( shownText, pos, attrs, aText->GetFontMetrics() );
    }
    else if( drawingShadows )
    {
        m_gal->SetIsFill( false );
        m_gal->SetIsStroke( true );
        attrs.m_StrokeWidth += KiROUND( getShadowWidth( !aText->IsSelected() ) );
        attrs.m_Underlined = false;

        // Fudge factors to match 6.0 positioning
        // New text stroking has width dependent offset but we need to center the shadow on the
        // stroke.  NB this offset is in font.cpp also.
        int fudge = KiROUND( getShadowWidth( !aText->IsSelected() ) / 1.52 );

        if( attrs.m_Halign == GR_TEXT_H_ALIGN_LEFT && attrs.m_Angle == ANGLE_0 )
            text_offset.x -= fudge;
        else if( attrs.m_Halign == GR_TEXT_H_ALIGN_RIGHT && attrs.m_Angle == ANGLE_90 )
            text_offset.y -= fudge;
        else if( attrs.m_Halign == GR_TEXT_H_ALIGN_RIGHT && attrs.m_Angle == ANGLE_0 )
            text_offset.x += fudge;
        else if( attrs.m_Halign == GR_TEXT_H_ALIGN_LEFT && attrs.m_Angle == ANGLE_90 )
            text_offset.y += fudge;

        strokeText( shownText, aText->GetDrawPos() + text_offset, attrs, aText->GetFontMetrics() );
    }
    else
    {
        if( aText->IsHypertext() && aText->IsRollover() )
        {
            m_gal->SetStrokeColor( m_schSettings.GetLayerColor( LAYER_HOVERED ) );
            m_gal->SetFillColor( m_schSettings.GetLayerColor( LAYER_HOVERED ) );
            attrs.m_Underlined = true;
        }

        // Adjust text drawn in an outline font to more closely mimic the positioning of
        // SCH_FIELD text.
        if( font->IsOutline() && aText->Type() == SCH_TEXT_T )
        {
            BOX2I    firstLineBBox = aText->GetTextBox( 0 );
            int      sizeDiff = firstLineBBox.GetHeight() - aText->GetTextSize().y;
            int      adjust = KiROUND( sizeDiff * 0.4 );
            VECTOR2I adjust_offset( 0, - adjust );

            RotatePoint( adjust_offset, aText->GetDrawRotation() );
            text_offset += adjust_offset;
        }

        if( nonCached( aText )
                && aText->RenderAsBitmap( m_gal->GetWorldScale() )
                && !shownText.Contains( wxT( "\n" ) ) )
        {
            bitmapText( shownText, aText->GetDrawPos() + text_offset, attrs );
            const_cast<SCH_TEXT*>( aText )->SetFlags( IS_SHOWN_AS_BITMAP );
        }
        else
        {
            std::vector<std::unique_ptr<KIFONT::GLYPH>>* cache = nullptr;

            if( !aText->IsHypertext() && font->IsOutline() )
                cache = aText->GetRenderCache( font, shownText, text_offset );

            if( cache )
            {
                m_gal->SetLineWidth( attrs.m_StrokeWidth );
                m_gal->DrawGlyphs( *cache );
            }
            else
            {
                strokeText( shownText, aText->GetDrawPos() + text_offset, attrs,
                            aText->GetFontMetrics() );
            }

            const_cast<SCH_TEXT*>( aText )->ClearFlags( IS_SHOWN_AS_BITMAP );
        }
    }
}


void SCH_PAINTER::draw( const SCH_TEXTBOX* aTextBox, int aLayer, bool aDimmed )
{
    if( aTextBox->Type() == SCH_TABLECELL_T )
    {
        const SCH_TABLECELL* cell = static_cast<const SCH_TABLECELL*>( aTextBox );

        if( cell->GetColSpan() == 0 || cell->GetRowSpan() == 0 )
            return;
    }

    if( !isUnitAndConversionShown( aTextBox ) )
        return;

    if( aTextBox->IsPrivate() && !m_schSettings.m_IsSymbolEditor )
        return;

    bool drawingShadows = aLayer == LAYER_SELECTION_SHADOWS;

    if( m_schSettings.IsPrinting() && drawingShadows )
        return;

    COLOR4D       color = getRenderColor( aTextBox, aLayer, drawingShadows, aDimmed );
    COLOR4D       bg = m_schSettings.GetLayerColor( LAYER_SCHEMATIC_BACKGROUND );
    float         borderWidth = getLineWidth( aTextBox, drawingShadows );
    KIFONT::FONT* font = getFont( aTextBox );

    auto drawText =
            [&]()
            {
                wxString        shownText = aTextBox->GetShownText( true );
                TEXT_ATTRIBUTES attrs = aTextBox->GetAttributes();

                attrs.m_Angle = aTextBox->GetDrawRotation();
                attrs.m_StrokeWidth = KiROUND( getTextThickness( aTextBox ) );

                if( aTextBox->IsHypertext() && aTextBox->IsRollover() )
                {
                    m_gal->SetStrokeColor( m_schSettings.GetLayerColor( LAYER_HOVERED ) );
                    m_gal->SetFillColor( m_schSettings.GetLayerColor( LAYER_HOVERED ) );
                    attrs.m_Underlined = true;
                }

                std::vector<std::unique_ptr<KIFONT::GLYPH>>* cache = nullptr;

                if( !aTextBox->IsHypertext() && font->IsOutline() )
                    cache = aTextBox->GetRenderCache( font, shownText );

                if( cache )
                {
                    m_gal->SetLineWidth( attrs.m_StrokeWidth );
                    m_gal->DrawGlyphs( *cache );
                }
                else
                {
                    strokeText( shownText, aTextBox->GetDrawPos(), attrs, aTextBox->GetFontMetrics() );
                }
            };

    if( drawingShadows && !( aTextBox->IsBrightened() || aTextBox->IsSelected() ) )
        return;

    m_gal->SetFillColor( color );
    m_gal->SetStrokeColor( color );

    if( aLayer == LAYER_SELECTION_SHADOWS )
    {
        m_gal->SetIsFill( true );
        m_gal->SetIsStroke( false );
        m_gal->SetLineWidth( borderWidth );

        m_gal->DrawRectangle( aTextBox->GetPosition(), aTextBox->GetEnd() );
    }
    else if( aLayer == LAYER_DEVICE_BACKGROUND || aLayer == LAYER_NOTES_BACKGROUND )
    {
        // Do not fill the shape in B&W print mode, to avoid to visible items
        // inside the shape
        if( aTextBox->IsFilled() && !m_schSettings.PrintBlackAndWhiteReq() )
        {
            m_gal->SetIsFill( true );
            m_gal->SetIsStroke( false );
            m_gal->SetLineWidth( borderWidth );

            m_gal->DrawRectangle( aTextBox->GetPosition(), aTextBox->GetEnd() );
        }
    }
    else if( aLayer == LAYER_DEVICE || aLayer == LAYER_NOTES || aLayer == LAYER_PRIVATE_NOTES )
    {
        drawText();

        if( aTextBox->Type() != SCH_TABLECELL_T && borderWidth > 0 )
        {
            COLOR4D    borderColor = aTextBox->GetStroke().GetColor();
            LINE_STYLE borderStyle = aTextBox->GetEffectiveLineStyle();
            double     transparency = aTextBox->GetForcedTransparency();

            if( m_schSettings.m_OverrideItemColors || aTextBox->IsBrightened()
                    || borderColor == COLOR4D::UNSPECIFIED )
            {
                borderColor = m_schSettings.GetLayerColor( aLayer );
            }

            if( transparency > 0.0 )
                borderColor = borderColor.WithAlpha( borderColor.a * ( 1.0 - transparency ) );

            if( aDimmed )
            {
                borderColor = borderColor.Mix( bg, 0.5f );
                borderColor.Desaturate( );
            }

            m_gal->SetIsFill( false );
            m_gal->SetIsStroke( true );
            m_gal->SetStrokeColor( borderColor );
            m_gal->SetLineWidth( borderWidth );

            if( borderStyle <= LINE_STYLE::FIRST_TYPE || drawingShadows )
            {
                m_gal->DrawRectangle( aTextBox->GetPosition(), aTextBox->GetEnd() );
            }
            else
            {
                std::vector<SHAPE*> shapes = aTextBox->MakeEffectiveShapes( true );

                for( SHAPE* shape : shapes )
                {
                    STROKE_PARAMS::Stroke( shape, borderStyle, KiROUND( borderWidth ),
                            &m_schSettings,
                            [this]( const VECTOR2I& a, const VECTOR2I& b )
                            {
                                // DrawLine has problem with 0 length lines so enforce minimum
                                if( a == b )
                                    m_gal->DrawLine( a+1, b );
                                else
                                    m_gal->DrawLine( a, b );
                            } );
                }

                for( SHAPE* shape : shapes )
                    delete shape;
            }
        }
    }
}


void SCH_PAINTER::draw( const SCH_TABLE* aTable, int aLayer, bool aDimmed )
{
    for( SCH_TABLECELL* cell : aTable->GetCells() )
        draw( cell, aLayer, aDimmed );

    if( aLayer == LAYER_SELECTION_SHADOWS )
        return;

    VECTOR2I pos = aTable->GetPosition();
    VECTOR2I end = aTable->GetEnd();

    int        lineWidth;
    COLOR4D    color;
    LINE_STYLE lineStyle;

    auto setupStroke =
            [&]( const STROKE_PARAMS& stroke )
            {
                lineWidth = stroke.GetWidth();
                color = stroke.GetColor();
                lineStyle = stroke.GetLineStyle();

                if( lineWidth == 0 )
                    lineWidth = m_schSettings.GetDefaultPenWidth();

                if( color == COLOR4D::UNSPECIFIED )
                    color = m_schSettings.GetLayerColor( LAYER_NOTES );

                if( lineStyle == LINE_STYLE::DEFAULT )
                    lineStyle = LINE_STYLE::SOLID;

                m_gal->SetIsFill( false );
                m_gal->SetIsStroke( true );
                m_gal->SetStrokeColor( color );
                m_gal->SetLineWidth( (float) lineWidth );
            };

    auto strokeShape =
            [&]( const SHAPE& shape )
            {
                STROKE_PARAMS::Stroke( &shape, lineStyle, lineWidth, &m_schSettings,
                        [&]( const VECTOR2I& a, const VECTOR2I& b )
                        {
                            // DrawLine has problem with 0 length lines so enforce minimum
                            if( a == b )
                                m_gal->DrawLine( a+1, b );
                            else
                                m_gal->DrawLine( a, b );
                        } );
            };

    auto strokeLine =
            [&]( const VECTOR2I& ptA, const VECTOR2I& ptB )
            {
                if( lineStyle <= LINE_STYLE::FIRST_TYPE )
                {
                    m_gal->DrawLine( ptA, ptB );
                }
                else
                {
                    SHAPE_SEGMENT seg( ptA, ptB );
                    strokeShape( seg );
                }
            };

    auto strokeRect =
            [&]( const VECTOR2I& ptA, const VECTOR2I& ptB )
            {
                if( lineStyle <= LINE_STYLE::FIRST_TYPE )
                {
                    m_gal->DrawRectangle( ptA, ptB );
                }
                else
                {
                    SHAPE_RECT rect( BOX2I( ptA, ptB - ptA ) );
                    strokeShape( rect );
                }
            };

    if( aTable->GetSeparatorsStroke().GetWidth() >= 0 )
    {
        setupStroke( aTable->GetSeparatorsStroke() );

        if( aTable->StrokeColumns() )
        {
            for( int col = 0; col < aTable->GetColCount() - 1; ++col )
            {
                for( int row = 0; row < aTable->GetRowCount(); ++row )
                {
                    SCH_TABLECELL* cell = aTable->GetCell( row, col );
                    VECTOR2I       topRight( cell->GetEndX(), cell->GetStartY() );

                    if( cell->GetColSpan() > 0 && cell->GetRowSpan() > 0 )
                        strokeLine( topRight, cell->GetEnd() );
                }
            }
        }

        if( aTable->StrokeRows() )
        {
            for( int row = 0; row < aTable->GetRowCount() - 1; ++row )
            {
                for( int col = 0; col < aTable->GetColCount(); ++col )
                {
                    SCH_TABLECELL* cell = aTable->GetCell( row, col );
                    VECTOR2I       botLeft( cell->GetStartX(), cell->GetEndY() );

                    if( cell->GetColSpan() > 0 && cell->GetRowSpan() > 0 )
                        strokeLine( botLeft, cell->GetEnd() );
                }
            }
        }
    }

    if( aTable->GetBorderStroke().GetWidth() >= 0 )
    {
        setupStroke( aTable->GetBorderStroke() );

        if( aTable->StrokeHeader() )
        {
            SCH_TABLECELL* cell = aTable->GetCell( 0, 0 );
            strokeLine( VECTOR2I( pos.x, cell->GetEndY() ), VECTOR2I( end.x, cell->GetEndY() ) );
        }

        if( aTable->StrokeExternal() )
            strokeRect( pos, end );
    }
}


static void orientSymbol( LIB_SYMBOL* symbol, int orientation )
{
    struct ORIENT
    {
        int flag;
        int n_rots;
        int mirror_x;
        int mirror_y;
    }
    orientations[] =
    {
        { SYM_ORIENT_0,                  0, 0, 0 },
        { SYM_ORIENT_90,                 1, 0, 0 },
        { SYM_ORIENT_180,                2, 0, 0 },
        { SYM_ORIENT_270,                3, 0, 0 },
        { SYM_MIRROR_X + SYM_ORIENT_0,   0, 1, 0 },
        { SYM_MIRROR_X + SYM_ORIENT_90,  1, 1, 0 },
        { SYM_MIRROR_Y,                  0, 0, 1 },
        { SYM_MIRROR_X + SYM_ORIENT_270, 3, 1, 0 },
        { SYM_MIRROR_Y + SYM_ORIENT_0,   0, 0, 1 },
        { SYM_MIRROR_Y + SYM_ORIENT_90,  1, 0, 1 },
        { SYM_MIRROR_Y + SYM_ORIENT_180, 2, 0, 1 },
        { SYM_MIRROR_Y + SYM_ORIENT_270, 3, 0, 1 }
    };

    ORIENT o = orientations[ 0 ];

    for( ORIENT& i : orientations )
    {
        if( i.flag == orientation )
        {
            o = i;
            break;
        }
    }

    for( SCH_ITEM& item : symbol->GetDrawItems() )
    {
        for( int i = 0; i < o.n_rots; i++ )
            item.Rotate( VECTOR2I( 0, 0 ), true );

        if( o.mirror_x )
            item.MirrorVertically( 0 );

        if( o.mirror_y )
            item.MirrorHorizontally( 0 );
    }
}


wxString SCH_PAINTER::expandLibItemTextVars( const wxString& aSourceText,
                                             const SCH_SYMBOL* aSymbolContext )
{
    std::function<bool( wxString* )> symbolResolver =
            [&]( wxString* token ) -> bool
            {
                return aSymbolContext->ResolveTextVar( &m_schematic->CurrentSheet(), token );
            };

    return ExpandTextVars( aSourceText, &symbolResolver );
}


void SCH_PAINTER::draw( const SCH_SYMBOL* aSymbol, int aLayer )
{
    bool drawingShadows = aLayer == LAYER_SELECTION_SHADOWS;

    if( m_schSettings.IsPrinting() && drawingShadows )
        return;

    if( !drawingShadows || eeconfig()->m_Selection.draw_selected_children )
    {
        for( const SCH_FIELD& field : aSymbol->GetFields() )
            draw( &field, aLayer, aSymbol->GetDNP() );
    }

    if( isFieldsLayer( aLayer ) )
        return;

    if( drawingShadows && !( aSymbol->IsBrightened() || aSymbol->IsSelected() ) )
    {
        // Don't exit here; symbol may still have selected pins
        // return;
    }

    int unit = aSymbol->GetUnitSelection( &m_schematic->CurrentSheet() );
    int bodyStyle = aSymbol->GetBodyStyle();

    // Use dummy symbol if the actual couldn't be found (or couldn't be locked).
    LIB_SYMBOL* originalSymbol = aSymbol->GetLibSymbolRef() ? aSymbol->GetLibSymbolRef().get()
                                                            : dummy();
    std::vector<SCH_PIN*> originalPins = originalSymbol->GetPins( unit, bodyStyle );

    // Copy the source so we can re-orient and translate it.
    LIB_SYMBOL            tempSymbol( *originalSymbol );
    std::vector<SCH_PIN*> tempPins = tempSymbol.GetPins( unit, bodyStyle );

    tempSymbol.SetFlags( aSymbol->GetFlags() );

    orientSymbol( &tempSymbol, aSymbol->GetOrientation() );

    for( SCH_ITEM& tempItem : tempSymbol.GetDrawItems() )
    {
        tempItem.SetFlags( aSymbol->GetFlags() );     // SELECTED, HIGHLIGHTED, BRIGHTENED,
        tempItem.Move( aSymbol->GetPosition() );

        if( tempItem.Type() == SCH_TEXT_T )
        {
            SCH_TEXT* textItem = static_cast<SCH_TEXT*>( &tempItem );

            if( textItem->HasTextVars() )
                textItem->SetText( expandLibItemTextVars( textItem->GetText(), aSymbol ) );
        }
        else if( tempItem.Type() == SCH_TEXTBOX_T )
        {
            SCH_TEXTBOX* textboxItem = static_cast<SCH_TEXTBOX*>( &tempItem );

            if( textboxItem->HasTextVars() )
                textboxItem->SetText( expandLibItemTextVars( textboxItem->GetText(), aSymbol ) );
        }
    }

    // Copy the pin info from the symbol to the temp pins
    for( unsigned i = 0; i < tempPins.size(); ++ i )
    {
        SCH_PIN* symbolPin = aSymbol->GetPin( originalPins[ i ] );
        SCH_PIN* tempPin = tempPins[ i ];

        tempPin->ClearFlags();
        tempPin->SetFlags( symbolPin->GetFlags() );     // SELECTED, HIGHLIGHTED, BRIGHTENED,
                                                        // IS_SHOWN_AS_BITMAP

        tempPin->SetName( expandLibItemTextVars( symbolPin->GetShownName(), aSymbol ) );
        tempPin->SetType( symbolPin->GetType() );
        tempPin->SetShape( symbolPin->GetShape() );

        if( symbolPin->IsDangling() )
            tempPin->SetFlags( IS_DANGLING );
        else
            tempPin->ClearFlags( IS_DANGLING );

        tempPin->SetOperatingPoint( symbolPin->GetOperatingPoint() );
    }

    draw( &tempSymbol, aLayer, false, aSymbol->GetUnit(), aSymbol->GetBodyStyle(), aSymbol->GetDNP() );

    for( unsigned i = 0; i < tempPins.size(); ++i )
    {
        SCH_PIN* symbolPin = aSymbol->GetPin( originalPins[ i ] );
        SCH_PIN* tempPin = tempPins[ i ];

        symbolPin->ClearFlags();
        tempPin->ClearFlags( IS_DANGLING );             // Clear this temporary flag
        symbolPin->SetFlags( tempPin->GetFlags() );     // SELECTED, HIGHLIGHTED, BRIGHTENED,
                                                        // IS_SHOWN_AS_BITMAP
    }

    if( aSymbol->GetDNP() )
    {
        BOX2I    bbox = aSymbol->GetBodyBoundingBox();
        BOX2I    pins = aSymbol->GetBodyAndPinsBoundingBox();
        VECTOR2D margins( std::max( bbox.GetX() - pins.GetX(), pins.GetEnd().x - bbox.GetEnd().x ),
                          std::max( bbox.GetY() - pins.GetY(), pins.GetEnd().y - bbox.GetEnd().y ) );

        margins.x = std::max( margins.x * 0.6, margins.y * 0.3 );
        margins.y = std::max( margins.y * 0.6, margins.x * 0.3 );
        bbox.Inflate( KiROUND( margins.x ), KiROUND( margins.y ) );

        VECTOR2I pt1 = bbox.GetOrigin();
        VECTOR2I pt2 = bbox.GetEnd();

        m_gal->PushDepth();
        m_gal->AdvanceDepth();
        m_gal->SetIsStroke( true );
        m_gal->SetIsFill( true );
        m_gal->SetStrokeColor( m_schSettings.GetLayerColor( LAYER_DNP_MARKER ) );
        m_gal->SetFillColor( m_schSettings.GetLayerColor( LAYER_DNP_MARKER ) );

        m_gal->DrawSegment( pt1, pt2, 3.0 * schIUScale.MilsToIU( DEFAULT_LINE_WIDTH_MILS ) );
        std::swap( pt1.x, pt2.x );
        m_gal->DrawSegment( pt1, pt2, 3.0 * schIUScale.MilsToIU( DEFAULT_LINE_WIDTH_MILS ) );
        m_gal->PopDepth();
    }
}


void SCH_PAINTER::draw( const SCH_FIELD* aField, int aLayer, bool aDimmed )
{
    bool drawingShadows = aLayer == LAYER_SELECTION_SHADOWS;

    if( m_schSettings.IsPrinting() && drawingShadows )
        return;

    if( drawingShadows && !( aField->IsBrightened() || aField->IsSelected() ) )
        return;

    if( !isUnitAndConversionShown( aField ) )
        return;

    // Must check layer as fields are sometimes drawn by their parent rather than directly
    // from the view.
    int  layers[KIGFX::VIEW::VIEW_MAX_LAYERS];
    int  layers_count;
    bool foundLayer = false;

    aField->ViewGetLayers( layers, layers_count );

    for( int i = 0; i < layers_count; ++i )
    {
        if( layers[i] == aLayer )
            foundLayer = true;
    }

    if( !foundLayer )
        return;

    aLayer = aField->GetLayer();

    COLOR4D color = getRenderColor( aField, aLayer, drawingShadows, aDimmed );

    if( !( aField->IsVisible() || aField->IsForceVisible() ) )
    {
        bool force_show = m_schematic ? eeconfig()->m_Appearance.show_hidden_fields
                                      : m_schSettings.m_ShowHiddenFields;

        if( force_show )
            color = getRenderColor( aField, LAYER_HIDDEN, drawingShadows, aDimmed );
        else
            return;
    }

    wxString shownText = aField->GetShownText( true );

    if( shownText.IsEmpty() )
        return;

    // Calculate the text orientation according to the parent orientation.
    EDA_ANGLE orient = aField->GetTextAngle();

    if( aField->GetParent() && aField->GetParent()->Type() == SCH_SYMBOL_T )
    {
        if( static_cast<SCH_SYMBOL*>( aField->GetParent() )->GetTransform().y1 )
        {
            // Rotate symbol 90 degrees.
            if( orient.IsHorizontal() )
                orient = ANGLE_VERTICAL;
            else
                orient = ANGLE_HORIZONTAL;
        }
    }

    /*
     * Calculate the text justification, according to the symbol orientation/mirror.
     * This is a bit complicated due to cumulative calculations:
     * - numerous cases (mirrored or not, rotation)
     * - the DrawGraphicText function recalculate also H and H justifications according to the
     *   text orientation.
     * - when symbol is mirrored, the text is not mirrored and justifications are complicated
     *   to calculate so the easier way is to use no justifications (centered text) and use
     *   GetBoundingBox to know the text coordinate considered as centered
     */
    BOX2I bbox = aField->GetBoundingBox();

    if( aField->GetParent() && aField->GetParent()->Type() == SCH_GLOBAL_LABEL_T )
    {
        SCH_GLOBALLABEL* label = static_cast<SCH_GLOBALLABEL*>( aField->GetParent() );
        bbox.Offset( label->GetSchematicTextOffset( &m_schSettings ) );
    }

    if( m_schSettings.GetDrawBoundingBoxes() )
        drawItemBoundingBox( aField );

    m_gal->SetStrokeColor( color );
    m_gal->SetFillColor( color );

    if( drawingShadows && getFont( aField )->IsOutline() )
    {
        BOX2I shadow_box = bbox;
        shadow_box.Inflate( KiROUND( getTextThickness( aField ) * 2 ) );

        m_gal->SetIsStroke( false );
        m_gal->SetIsFill( true );
        m_gal->DrawRectangle( shadow_box.GetPosition(), shadow_box.GetEnd() );
    }
    else
    {
        VECTOR2I        textpos = bbox.Centre();
        TEXT_ATTRIBUTES attributes = aField->GetAttributes();

        attributes.m_Halign = GR_TEXT_H_ALIGN_CENTER;
        attributes.m_Valign = GR_TEXT_V_ALIGN_CENTER;
        attributes.m_StrokeWidth = KiROUND( getTextThickness( aField ) );
        attributes.m_Angle = orient;

        if( drawingShadows )
            attributes.m_StrokeWidth += getShadowWidth( !aField->IsSelected() );

        if( aField->IsHypertext() && aField->IsRollover() )
        {
            m_gal->SetStrokeColor( m_schSettings.GetLayerColor( LAYER_HOVERED ) );
            m_gal->SetFillColor( m_schSettings.GetLayerColor( LAYER_HOVERED ) );
            attributes.m_Underlined = true;
        }

        if( nonCached( aField ) && aField->RenderAsBitmap( m_gal->GetWorldScale() ) )
        {
            bitmapText( shownText, textpos, attributes );
            const_cast<SCH_FIELD*>( aField )->SetFlags( IS_SHOWN_AS_BITMAP );
        }
        else
        {
            std::vector<std::unique_ptr<KIFONT::GLYPH>>* cache = nullptr;

            if( !aField->IsHypertext() )
                cache = aField->GetRenderCache( shownText, textpos, attributes );

            if( cache )
            {
                m_gal->SetLineWidth( attributes.m_StrokeWidth );
                m_gal->DrawGlyphs( *cache );
            }
            else
            {
                strokeText( shownText, textpos, attributes, aField->GetFontMetrics() );
            }

            const_cast<SCH_FIELD*>( aField )->ClearFlags( IS_SHOWN_AS_BITMAP );
        }
    }

    // Draw the umbilical line
    if( aField->IsMoving() && m_schematic )
    {
        VECTOR2I parentPos = aField->GetParentPosition();

        m_gal->SetLineWidth( m_schSettings.GetOutlineWidth() );
        m_gal->SetStrokeColor( getRenderColor( aField, LAYER_SCHEMATIC_ANCHOR, drawingShadows ) );
        m_gal->DrawLine( bbox.Centre(), parentPos );
    }
}


void SCH_PAINTER::draw( const SCH_GLOBALLABEL* aLabel, int aLayer )
{
    bool drawingShadows = aLayer == LAYER_SELECTION_SHADOWS;

    if( m_schSettings.IsPrinting() && drawingShadows )
        return;

    bool drawingDangling = aLayer == LAYER_DANGLING;

    if( !drawingShadows || eeconfig()->m_Selection.draw_selected_children )
    {
        for( const SCH_FIELD& field : aLabel->GetFields() )
            draw( &field, aLayer, false );
    }

    if( isFieldsLayer( aLayer ) )
        return;

    if( drawingShadows && !( aLabel->IsBrightened() || aLabel->IsSelected() ) )
        return;

    COLOR4D color = getRenderColor( aLabel, LAYER_GLOBLABEL, drawingShadows );

    if( drawingDangling )
    {
        if( aLabel->IsDangling() )
        {
            drawDanglingIndicator( aLabel->GetTextPos(), color,
                                   schIUScale.MilsToIU( DANGLING_SYMBOL_SIZE / 2 ), true,
                                   drawingShadows, aLabel->IsBrightened() );
        }

        return;
    }

    std::vector<VECTOR2I> pts;
    std::deque<VECTOR2D> pts2;

    aLabel->CreateGraphicShape( &m_schSettings, pts, aLabel->GetTextPos() );

    for( const VECTOR2I& p : pts )
        pts2.emplace_back( VECTOR2D( p.x, p.y ) );

    m_gal->SetIsStroke( true );
    m_gal->SetLineWidth( getLineWidth( aLabel, drawingShadows ) );
    m_gal->SetStrokeColor( color );

    if( drawingShadows )
    {
        m_gal->SetIsFill( eeconfig()->m_Selection.fill_shapes );
        m_gal->SetFillColor( color );
        m_gal->DrawPolygon( pts2 );
    }
    else
    {
        m_gal->SetIsFill( false );
        m_gal->DrawPolyline( pts2 );
    }

    draw( static_cast<const SCH_TEXT*>( aLabel ), aLayer, false );
}


void SCH_PAINTER::draw( const SCH_LABEL* aLabel, int aLayer )
{
    bool drawingShadows = aLayer == LAYER_SELECTION_SHADOWS;

    if( m_schSettings.IsPrinting() && drawingShadows )
        return;

    bool drawingDangling = aLayer == LAYER_DANGLING;

    if( !drawingShadows || eeconfig()->m_Selection.draw_selected_children )
    {
        for( const SCH_FIELD& field : aLabel->GetFields() )
            draw( &field, aLayer, false );
    }

    if( isFieldsLayer( aLayer ) )
        return;

    if( drawingShadows && !( aLabel->IsBrightened() || aLabel->IsSelected() ) )
        return;

    COLOR4D color = getRenderColor( aLabel, LAYER_HIERLABEL, drawingShadows );

    if( drawingDangling )
    {
        if( aLabel->IsDangling() )
        {
            drawDanglingIndicator( aLabel->GetTextPos(), color,
                                   schIUScale.MilsToIU( DANGLING_SYMBOL_SIZE / 2 ), true,
                                   drawingShadows, aLabel->IsBrightened() );
        }

        return;
    }

    draw( static_cast<const SCH_TEXT*>( aLabel ), aLayer, false );
}


void SCH_PAINTER::draw( const SCH_HIERLABEL* aLabel, int aLayer )
{
    bool drawingShadows = aLayer == LAYER_SELECTION_SHADOWS;

    if( m_schSettings.IsPrinting() && drawingShadows )
        return;

    bool drawingDangling = aLayer == LAYER_DANGLING;

    if( !( drawingShadows || drawingDangling ) || eeconfig()->m_Selection.draw_selected_children )
    {
        for( const SCH_FIELD& field : aLabel->GetFields() )
            draw( &field, aLayer, false );
    }

    if( isFieldsLayer( aLayer ) )
        return;

    if( drawingShadows && !( aLabel->IsBrightened() || aLabel->IsSelected() ) )
        return;

    COLOR4D color = getRenderColor( aLabel, LAYER_HIERLABEL, drawingShadows );

    if( drawingDangling )
    {
        if( aLabel->IsDangling() )
        {
            drawDanglingIndicator( aLabel->GetTextPos(), color,
                                   schIUScale.MilsToIU( DANGLING_SYMBOL_SIZE / 2 ), true,
                                   drawingShadows, aLabel->IsBrightened() );
        }

        return;
    }

    if( m_schematic )
    {
        SCH_CONNECTION* conn = nullptr;

        if( !aLabel->IsConnectivityDirty() )
            conn = aLabel->Connection();

        if( conn && conn->IsBus() )
            color = getRenderColor( aLabel, LAYER_BUS, drawingShadows );
    }

    std::vector<VECTOR2I> pts;
    std::deque<VECTOR2D> pts2;

    aLabel->CreateGraphicShape( &m_schSettings, pts, (VECTOR2I)aLabel->GetTextPos() );

    for( const VECTOR2I& p : pts )
        pts2.emplace_back( VECTOR2D( p.x, p.y ) );

    m_gal->SetIsFill( true );
    m_gal->SetFillColor( m_schSettings.GetLayerColor( LAYER_SCHEMATIC_BACKGROUND ) );
    m_gal->SetIsStroke( true );
    m_gal->SetLineWidth( getLineWidth( aLabel, drawingShadows ) );
    m_gal->SetStrokeColor( color );
    m_gal->DrawPolyline( pts2 );

    draw( static_cast<const SCH_TEXT*>( aLabel ), aLayer, false );
}


void SCH_PAINTER::draw( const SCH_DIRECTIVE_LABEL* aLabel, int aLayer )
{
    if( !eeconfig()->m_Appearance.show_directive_labels && !aLabel->IsSelected() )
        return;

    bool drawingShadows = aLayer == LAYER_SELECTION_SHADOWS;

    if( m_schSettings.IsPrinting() && drawingShadows )
        return;

    if( !drawingShadows || eeconfig()->m_Selection.draw_selected_children )
    {
        for( const SCH_FIELD& field : aLabel->GetFields() )
            draw( &field, aLayer, false );
    }

    if( isFieldsLayer( aLayer ) )
        return;

    if( drawingShadows && !( aLabel->IsBrightened() || aLabel->IsSelected() ) )
        return;

    COLOR4D color = getRenderColor( aLabel, LAYER_NETCLASS_REFS, drawingShadows );

    if( aLayer == LAYER_DANGLING )
    {
        if( aLabel->IsDangling() )
        {
            drawDanglingIndicator( aLabel->GetTextPos(), color,
                                   schIUScale.MilsToIU( DANGLING_SYMBOL_SIZE / 2 ), true,
                                   drawingShadows, aLabel->IsBrightened() );
        }

        return;
    }

    std::vector<VECTOR2I> pts;
    std::deque<VECTOR2D> pts2;

    aLabel->CreateGraphicShape( &m_schSettings, pts, aLabel->GetTextPos() );

    for( const VECTOR2I& p : pts )
        pts2.emplace_back( VECTOR2D( p.x, p.y ) );

    m_gal->SetIsFill( false );
    m_gal->SetFillColor( color );
    m_gal->SetIsStroke( true );
    m_gal->SetLineWidth( getLineWidth( aLabel, drawingShadows ) );
    m_gal->SetStrokeColor( color );

    if( aLabel->GetShape() == LABEL_FLAG_SHAPE::F_DOT )
    {
        m_gal->DrawLine( pts2[0], pts2[1] );
        m_gal->SetIsFill( true );
        m_gal->DrawCircle( pts2[2], ( pts2[2] - pts2[1] ).EuclideanNorm() );
    }
    else if( aLabel->GetShape() == LABEL_FLAG_SHAPE::F_ROUND )
    {
        m_gal->DrawLine( pts2[0], pts2[1] );
        m_gal->DrawCircle( pts2[2], ( pts2[2] - pts2[1] ).EuclideanNorm() );
    }
    else
    {
        m_gal->DrawPolyline( pts2 );
    }
}


void SCH_PAINTER::draw( const SCH_SHEET* aSheet, int aLayer )
{
    bool drawingShadows = aLayer == LAYER_SELECTION_SHADOWS;

    if( m_schSettings.IsPrinting() && drawingShadows )
        return;

    if( !drawingShadows || eeconfig()->m_Selection.draw_selected_children )
    {
        for( const SCH_FIELD& field : aSheet->GetFields() )
            draw( &field, aLayer, false );

        for( SCH_SHEET_PIN* sheetPin : aSheet->GetPins() )
            draw( static_cast<SCH_HIERLABEL*>( sheetPin ), aLayer );
    }

    if( isFieldsLayer( aLayer ) )
        return;

    VECTOR2D pos  = aSheet->GetPosition();
    VECTOR2D size = aSheet->GetSize();

    if( aLayer == LAYER_SHEET_BACKGROUND )
    {
        // Do not fill the shape in B&W print mode, to avoid to visible items
        // inside the shape
        if( !m_schSettings.PrintBlackAndWhiteReq() )
        {
            m_gal->SetFillColor( getRenderColor( aSheet, LAYER_SHEET_BACKGROUND, true ) );
            m_gal->SetIsFill( true );
            m_gal->SetIsStroke( false );

            m_gal->DrawRectangle( pos, pos + size );
        }
    }

    if( aLayer == LAYER_SHEET || aLayer == LAYER_SELECTION_SHADOWS )
    {
        m_gal->SetStrokeColor( getRenderColor( aSheet, LAYER_SHEET, drawingShadows ) );
        m_gal->SetIsStroke( true );
        m_gal->SetLineWidth( getLineWidth( aSheet, drawingShadows ) );
        m_gal->SetIsFill( false );

        m_gal->DrawRectangle( pos, pos + size );
    }
}


void SCH_PAINTER::draw( const SCH_NO_CONNECT* aNC, int aLayer )
{
    bool drawingShadows = aLayer == LAYER_SELECTION_SHADOWS;

    if( m_schSettings.IsPrinting() && drawingShadows )
        return;

    if( drawingShadows && !( aNC->IsBrightened() || aNC->IsSelected() ) )
        return;

    m_gal->SetIsStroke( true );
    m_gal->SetLineWidth( getLineWidth( aNC, drawingShadows ) );
    m_gal->SetStrokeColor( getRenderColor( aNC, LAYER_NOCONNECT, drawingShadows ) );
    m_gal->SetIsFill( false );

    VECTOR2D p = aNC->GetPosition();
    int      delta = std::max( aNC->GetSize(), m_schSettings.GetDefaultPenWidth() * 3 ) / 2;

    m_gal->DrawLine( p + VECTOR2D( -delta, -delta ), p + VECTOR2D( delta, delta ) );
    m_gal->DrawLine( p + VECTOR2D( -delta, delta ), p + VECTOR2D( delta, -delta ) );
}


void SCH_PAINTER::draw( const SCH_BUS_ENTRY_BASE *aEntry, int aLayer )
{
    SCH_LAYER_ID layer = aEntry->Type() == SCH_BUS_WIRE_ENTRY_T ? LAYER_WIRE : LAYER_BUS;
    SCH_LINE     line( VECTOR2I(), layer );
    bool         drawingShadows = aLayer == LAYER_SELECTION_SHADOWS;

    if( m_schSettings.IsPrinting() && drawingShadows )
        return;

    bool         drawingDangling = aLayer == LAYER_DANGLING;

    if( drawingShadows && !( aEntry->IsBrightened() || aEntry->IsSelected() ) )
        return;

    if( aEntry->IsSelected() )
    {
        line.SetSelected();
        // Never show unselected endpoints on bus entries
        line.SetFlags( STARTPOINT | ENDPOINT );
    }
    else if( aEntry->IsBrightened() )
        line.SetBrightened();

    line.SetStartPoint( aEntry->GetPosition() );
    line.SetEndPoint( aEntry->GetEnd() );
    line.SetStroke( aEntry->GetStroke() );
    line.SetLineWidth( KiROUND( getLineWidth( aEntry, false ) ) );

    COLOR4D color = getRenderColor( aEntry, LAYER_WIRE, drawingShadows );

    if( aEntry->Type() == SCH_BUS_BUS_ENTRY_T )
        color = getRenderColor( aEntry, LAYER_BUS, drawingShadows );

    if( drawingDangling )
    {
        m_gal->SetIsFill( false );
        m_gal->SetIsStroke( true );
        m_gal->SetStrokeColor( color.Brightened( 0.3 ) );
        m_gal->SetLineWidth( m_schSettings.GetDanglingIndicatorThickness() );

        if( aEntry->IsDanglingStart() )
        {
            m_gal->DrawCircle( aEntry->GetPosition(),
                               aEntry->GetPenWidth() + KiROUND( TARGET_BUSENTRY_RADIUS / 2.0 ) );
        }

        if( aEntry->IsDanglingEnd() )
        {
            m_gal->DrawCircle( aEntry->GetEnd(),
                               aEntry->GetPenWidth() + KiROUND( TARGET_BUSENTRY_RADIUS / 2.0 ) );
        }
    }
    else
    {
        line.SetLineColor( color );
        line.SetLineStyle( aEntry->GetLineStyle() );

        draw( &line, aLayer );
    }
}


void SCH_PAINTER::draw( const SCH_BITMAP* aBitmap, int aLayer )
{
    m_gal->Save();
    m_gal->Translate( aBitmap->GetPosition() );

    // When the image scale factor is not 1.0, we need to modify the actual as the image scale
    // factor is similar to a local zoom
    double img_scale = aBitmap->GetImageScale();

    if( img_scale != 1.0 )
        m_gal->Scale( VECTOR2D( img_scale, img_scale ) );

    if( aLayer == LAYER_DRAW_BITMAPS )
    {
        m_gal->DrawBitmap( *aBitmap->GetImage() );
    }

    if( aLayer == LAYER_SELECTION_SHADOWS )
    {
        if( aBitmap->IsSelected() || aBitmap->IsBrightened() )
        {
            COLOR4D color = getRenderColor( aBitmap, LAYER_DRAW_BITMAPS, true );
            m_gal->SetIsStroke( true );
            m_gal->SetStrokeColor( color );
            m_gal->SetLineWidth ( getShadowWidth( aBitmap->IsBrightened() ) );
            m_gal->SetIsFill( false );

            // Draws a bounding box.
            VECTOR2D bm_size( aBitmap->GetSize() );
            // bm_size is the actual image size in UI.
            // but m_gal scale was previously set to img_scale
            // so recalculate size relative to this image size.
            bm_size.x /= img_scale;
            bm_size.y /= img_scale;
            VECTOR2D origin( -bm_size.x / 2.0, -bm_size.y / 2.0 );
            VECTOR2D end = origin + bm_size;

            m_gal->DrawRectangle( origin, end );
        }
    }

    m_gal->Restore();
}


void SCH_PAINTER::draw( const SCH_MARKER* aMarker, int aLayer )
{
    bool drawingShadows = aLayer == LAYER_SELECTION_SHADOWS;

    if( m_schSettings.IsPrinting() && drawingShadows )
        return;

    if( drawingShadows && !( aMarker->IsBrightened() || aMarker->IsSelected() ) )
        return;

    COLOR4D color = getRenderColor( aMarker, aMarker->GetColorLayer(), drawingShadows );

    m_gal->Save();
    m_gal->Translate( aMarker->GetPosition() );
    m_gal->SetIsFill( !drawingShadows );
    m_gal->SetFillColor( color );
    m_gal->SetIsStroke( drawingShadows );
    m_gal->SetLineWidth( getLineWidth( aMarker, drawingShadows ) );
    m_gal->SetStrokeColor( color );

    SHAPE_LINE_CHAIN polygon;
    aMarker->ShapeToPolygon( polygon );

    m_gal->DrawPolygon( polygon );
    m_gal->Restore();
}


}; // namespace KIGFX
