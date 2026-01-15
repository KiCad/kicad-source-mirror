/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2014 CERN
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
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
#include <chrono>
#include <bitmap_base.h>
#include <connection_graph.h>
#include <gal/graphics_abstraction_layer.h>
#include <callback_gal.h>
#include <geometry/shape_segment.h>
#include <geometry/shape_rect.h>
#include <geometry/roundrect.h>
#include <geometry/shape_poly_set.h>
#include <geometry/shape_utils.h>
#include <gr_text.h>
#include <sch_pin.h>
#include <math/util.h>
#include <pin_layout_cache.h>
#include <pgm_base.h>
#include <sch_bitmap.h>
#include <sch_bus_entry.h>
#include <sch_symbol.h>
#include <sch_edit_frame.h>
#include <sch_field.h>
#include <sch_group.h>
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
#include <trace_helpers.h>
#include <view/view.h>
#include <kiface_base.h>
#include <default_values.h>
#include <advanced_config.h>
#include <settings/settings_manager.h>
#include <stroke_params.h>
#include <string_utils.h>
#include "sch_painter.h"
#include "common.h"

#include "symb_transforms_utils.h"

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
    SCH_RULE_AREA_T,
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

        m_canvas->SetHorizontalJustify( GR_TEXT_H_ALIGN_CENTER );
        m_canvas->SetVerticalJustify( GR_TEXT_V_ALIGN_CENTER );
        m_canvas->SetStrokeColor( COLOR4D( LIGHTRED ) );
        m_canvas->SetLineWidth( Mils2ui( 2 ) );
        m_canvas->SetGlyphSize( VECTOR2D( Mils2ui( 20 ), Mils2ui( 20 ) ) );
        m_canvas->StrokeText( *m_canvas, conn->Name( true ), pos, 0.0, 0 );
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
    case SCH_RULE_AREA_T:
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
        draw( static_cast<const SCH_LABEL*>( aItem ), aLayer, aDimmed );
        break;
    case SCH_DIRECTIVE_LABEL_T:
        draw( static_cast<const SCH_DIRECTIVE_LABEL*>( aItem ), aLayer, aDimmed );
        break;
    case SCH_FIELD_T:
        draw( static_cast<const SCH_FIELD*>( aItem ), aLayer, aDimmed );
        break;
    case SCH_HIER_LABEL_T:
        draw( static_cast<const SCH_HIERLABEL*>( aItem ), aLayer, aDimmed );
        break;
    case SCH_GLOBAL_LABEL_T:
        draw( static_cast<const SCH_GLOBALLABEL*>( aItem ), aLayer, aDimmed );
        break;
    case SCH_SHEET_T:
        draw( static_cast<const SCH_SHEET*>( aItem ), aLayer );
        break;
    case SCH_SHEET_PIN_T:
        draw( static_cast<const SCH_HIERLABEL*>( aItem ), aLayer, aDimmed );
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
    case SCH_GROUP_T:
        draw( static_cast<const SCH_GROUP*>( aItem ), aLayer );
        break;
    default:
        return;
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
    if( KIFONT::FONT* font = aItem->GetDrawFont( &m_schSettings ) )
        return font;

    return KIFONT::FONT::GetFont( m_schSettings.GetDefaultFont(), aItem->IsBold(), aItem->IsItalic() );
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
                                     bool aDimmed, bool aIgnoreNets ) const
{
    auto isBackgroundLayer =
            []( int layer )
            {
                return layer == LAYER_DEVICE_BACKGROUND || layer == LAYER_NOTES_BACKGROUND
                    || layer == LAYER_SHAPES_BACKGROUND || layer == LAYER_SHEET_BACKGROUND;
            };

    COLOR4D color = m_schSettings.GetLayerColor( aLayer );

    // Graphic items of a SYMBOL frequently use the LAYER_DEVICE layer color
    // (i.e. when no specific color is set)
    bool isSymbolChild = aItem->GetParentSymbol() != nullptr;

    if( !m_schSettings.m_OverrideItemColors )
    {
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
        else if( aItem->Type() == SCH_SHEET_T )
        {
            const SCH_SHEET* sheet = static_cast<const SCH_SHEET*>( aItem );

            if( isBackgroundLayer( aLayer ) )
                color = sheet->GetBackgroundColor();
            else
                color = sheet->GetBorderColor();
        }
        else if( aItem->Type() == SCH_SHAPE_T || aItem->Type() == SCH_RULE_AREA_T )
        {
            const SCH_SHAPE* shape = static_cast<const SCH_SHAPE*>( aItem );

            if( isBackgroundLayer( aLayer ) )
            {
                switch( shape->GetFillMode() )
                {
                case FILL_T::NO_FILL:
                    break;

                case FILL_T::FILLED_SHAPE:
                    color = shape->GetStroke().GetColor();
                    break;

                case FILL_T::HATCH:
                case FILL_T::REVERSE_HATCH:
                case FILL_T::CROSS_HATCH:
                case FILL_T::FILLED_WITH_COLOR:
                    color = shape->GetFillColor();
                    break;

                case FILL_T::FILLED_WITH_BG_BODYCOLOR:
                    color = m_schSettings.GetLayerColor( LAYER_DEVICE_BACKGROUND );
                    break;

                default:
                    wxFAIL_MSG( wxT( "Unsupported fill type" ) );
                }

                // A filled shape means filled; if they didn't specify a fill colour then use
                // the border colour.
                if( shape->GetFillMode() != FILL_T::NO_FILL && color == COLOR4D::UNSPECIFIED )
                {
                    if( aItem->Type() == SCH_RULE_AREA_T )
                        color = m_schSettings.GetLayerColor( LAYER_RULE_AREAS );
                    else if( isSymbolChild )
                        color = m_schSettings.GetLayerColor( LAYER_DEVICE );
                    else
                        color = m_schSettings.GetLayerColor( LAYER_NOTES );
                }
            }
            else
            {
                color = shape->GetStroke().GetColor();
            }
        }
        else if( aItem->IsType( { SCH_LABEL_LOCATE_ANY_T } ) )
        {
            const SCH_LABEL_BASE* label = static_cast<const SCH_LABEL_BASE*>( aItem );

            if( label->GetTextColor() != COLOR4D::UNSPECIFIED )
                color = label->GetTextColor();                      // override color
            else if( aIgnoreNets )
                color = m_schSettings.GetLayerColor( aLayer );      // layer color
            else
                color = label->GetLabelColor();                     // net/netclass color
        }
        else if( aItem->Type() == SCH_FIELD_T )
        {
            color = static_cast<const SCH_FIELD*>( aItem )->GetFieldColor();
        }
        else if( aItem->Type() == SCH_TEXTBOX_T || aItem->Type() == SCH_TABLECELL_T )
        {
            const SCH_TEXTBOX* textBox = static_cast<const SCH_TEXTBOX*>( aItem );

            if( isBackgroundLayer( aLayer ) )
                color = textBox->GetFillColor();
            else if( !isSymbolChild || textBox->GetTextColor() != COLOR4D::UNSPECIFIED )
                color = textBox->GetTextColor();
        }
        else if( const EDA_TEXT* otherTextItem = dynamic_cast<const EDA_TEXT*>( aItem ) )
        {
            if( !isSymbolChild || otherTextItem->GetTextColor() != COLOR4D::UNSPECIFIED )
                color = otherTextItem->GetTextColor();
        }

        if( color.m_text.has_value() )
            color = COLOR4D( aItem->ResolveText( color.m_text.value(), &m_schematic->CurrentSheet() ) );
    }
    else  /* overrideItemColors */
    {
        // If we ARE overriding the item colors, what do we do with non-item-color fills?
        // There are two theories: we should leave them untouched, or we should drop them entirely.
        // We currently implment the first.
        if( isBackgroundLayer( aLayer) )
        {
            if( aItem->Type() == SCH_SHAPE_T || aItem->Type() == SCH_RULE_AREA_T )
            {
                const SCH_SHAPE* shape = static_cast<const SCH_SHAPE*>( aItem );

                if( shape->GetFillMode() == FILL_T::FILLED_WITH_COLOR )
                    color = shape->GetFillColor();
            }
            else if( aItem->Type() == SCH_SHEET_T )
            {
                const SCH_SHEET* sheet = static_cast<const SCH_SHEET*>( aItem );

                color = sheet->GetBackgroundColor();
            }
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
        else if( isBackgroundLayer( aLayer ) )
        {
            color = color.WithAlpha( 0.2 );
        }
    }
    else if( aItem->IsSelected() && aDrawingShadows )
    {
        color = m_schSettings.GetLayerColor( LAYER_SELECTION_SHADOWS );
    }
    else if( aItem->IsSelected() && isBackgroundLayer( aLayer ) )
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


float SCH_PAINTER::getLineWidth( const SCH_ITEM* aItem, bool aDrawingShadows,
                                 bool aDrawingWireColorHighlights ) const
{
    wxCHECK( aItem, static_cast<float>( schIUScale.MilsToIU( DEFAULT_LINE_WIDTH_MILS ) ) );

    int   pen = aItem->GetEffectivePenWidth( &m_schSettings );
    float width = pen;

    if( aItem->IsBrightened() || aItem->IsSelected() )
    {
        if( aDrawingShadows && aItem->IsType( g_ScaledSelectionTypes ) )
            width += getShadowWidth( aItem->IsBrightened() );
    }

    if( aDrawingWireColorHighlights )
    {
        float              colorHighlightWidth = schIUScale.MilsToIU( 15.0 );
        EESCHEMA_SETTINGS* eeschemaCfg = eeconfig();

        if( eeschemaCfg )
            colorHighlightWidth = schIUScale.MilsToIU( eeschemaCfg->m_Selection.highlight_netclass_colors_thickness );

        width += colorHighlightWidth;
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
    return KiROUND( ( docTextSize + screenTextSize * 2 ) / 3.0 );
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


static BOX2I GetTextExtents( const wxString& aText, const VECTOR2D& aPosition, KIFONT::FONT& aFont,
                             const TEXT_ATTRIBUTES& aAttrs, const KIFONT::METRICS& aFontMetrics )
{
    const VECTOR2I extents = aFont.StringBoundaryLimits( aText, aAttrs.m_Size, aAttrs.m_StrokeWidth,
                                                         aAttrs.m_Bold, aAttrs.m_Italic, aFontMetrics );
    BOX2I box( aPosition, VECTOR2I( extents.x, aAttrs.m_Size.y ) );

    switch( aAttrs.m_Halign )
    {
    case GR_TEXT_H_ALIGN_LEFT:                                                        break;
    case GR_TEXT_H_ALIGN_CENTER:        box.SetX( box.GetX() - box.GetWidth() / 2 );  break;
    case GR_TEXT_H_ALIGN_RIGHT:         box.SetX( box.GetX() - box.GetWidth() );      break;
    case GR_TEXT_H_ALIGN_INDETERMINATE: wxFAIL_MSG( wxT( "Legal only in dialogs" ) ); break;
    }

    switch( aAttrs.m_Valign )
    {
    case GR_TEXT_V_ALIGN_TOP:                                                         break;
    case GR_TEXT_V_ALIGN_CENTER:        box.SetY( box.GetY() - box.GetHeight() / 2 ); break;
    case GR_TEXT_V_ALIGN_BOTTOM:        box.SetY( box.GetY() - box.GetHeight() );     break;
    case GR_TEXT_V_ALIGN_INDETERMINATE: wxFAIL_MSG( wxT( "Legal only in dialogs" ) ); break;
    }

    box.Normalize(); // Make h and v sizes always >= 0
    box = box.GetBoundingBoxRotated( aPosition, aAttrs.m_Angle );

    return box;
}


static void strokeText( KIGFX::GAL& aGal, const wxString& aText, const VECTOR2D& aPosition,
                        const TEXT_ATTRIBUTES& aAttrs, const KIFONT::METRICS& aFontMetrics,
                        std::optional<VECTOR2I> aMousePos = std::nullopt, wxString* aActiveUrl = nullptr )
{
    KIFONT::FONT* font = aAttrs.m_Font;

    if( !font )
        font = KIFONT::FONT::GetFont( eeconfig()->m_Appearance.default_font, aAttrs.m_Bold, aAttrs.m_Italic );

    aGal.SetIsFill( font->IsOutline() );
    aGal.SetIsStroke( font->IsStroke() );

    font->Draw( &aGal, aText, aPosition, aAttrs, aFontMetrics, aMousePos, aActiveUrl );
}


static void bitmapText( KIGFX::GAL& aGal, const wxString& aText, const VECTOR2D& aPosition,
                        const TEXT_ATTRIBUTES& aAttrs )
{
    // Bitmap font has different metrics from the stroke font so we compensate a bit before
    // stroking
    aGal.SetGlyphSize( VECTOR2I( aAttrs.m_Size.x, KiROUND( aAttrs.m_Size.y * 1.05 ) ) );
    aGal.SetLineWidth( (float) aAttrs.m_StrokeWidth * 1.35f );

    aGal.SetHorizontalJustify( aAttrs.m_Halign );
    aGal.SetVerticalJustify( aAttrs.m_Valign );

    aGal.BitmapText( aText, aPosition, aAttrs.m_Angle );
}


static void knockoutText( KIGFX::GAL& aGal, const wxString& aText, const VECTOR2D& aPosition,
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

    finalPoly.BooleanSubtract( knockouts );
    finalPoly.Fracture();

    aGal.SetIsStroke( false );
    aGal.SetIsFill( true );
    aGal.SetFillColor( attrs.m_Color );
    aGal.DrawPolygon( finalPoly );
}


static void boxText( KIGFX::GAL& aGal, const wxString& aText, const VECTOR2D& aPosition,
                     const TEXT_ATTRIBUTES& aAttrs, const KIFONT::METRICS& aFontMetrics )
{
    KIFONT::FONT* font = aAttrs.m_Font;

    if( !font )
        font = KIFONT::FONT::GetFont( eeconfig()->m_Appearance.default_font, aAttrs.m_Bold, aAttrs.m_Italic );

    BOX2I box = GetTextExtents( aText, aPosition, *font, aAttrs, aFontMetrics );

    // Give the highlight a bit of margin.
    box.Inflate( 0, aAttrs.m_StrokeWidth * 2 );

    aGal.SetIsFill( true );
    aGal.SetIsStroke( false );
    aGal.DrawRectangle( box.GetOrigin(), box.GetEnd() );
}


void SCH_PAINTER::triLine( const VECTOR2D& a, const VECTOR2D& b, const VECTOR2D& c )
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

    if( aSymbol->IsDerived() )
    {
        tmpSymbol = aSymbol->Flatten();
        drawnSymbol = tmpSymbol.get();
    }

    // The parent must exist on the union of all its children's draw layers.  But that doesn't
    // mean we want to draw each child on the union.
    auto childOnLayer =
            []( const SCH_ITEM& item, int layer )
            {
                return alg::contains( item.ViewGetLayers(), layer );
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
void SCH_PAINTER::drawPinDanglingIndicator( const SCH_PIN& aPin, const COLOR4D& aColor,
                                            bool aDrawingShadows, bool aBrightened )
{
    const PIN_LAYOUT_CACHE& plc = aPin.GetLayoutCache();
    const CIRCLE            c = plc.GetDanglingIndicator();

    float lineWidth = aDrawingShadows ? getShadowWidth( aBrightened )
                                      : m_schSettings.GetDanglingIndicatorThickness();

    // Dangling symbols must be drawn in a slightly different colour so they can be seen when
    // they overlap with a junction dot.
    m_gal->SetStrokeColor( aColor.Brightened( 0.3 ) );

    m_gal->SetIsFill( false );
    m_gal->SetIsStroke( true );
    m_gal->SetLineWidth( lineWidth );
    m_gal->DrawCircle( c.Center, c.Radius );
}


/**
 * Draw an local power pin indicator icon.
 */
void SCH_PAINTER::drawLocalPowerIcon( const VECTOR2D& aPos, double aSize, bool aRotate,
                                      const COLOR4D& aColor, bool aDrawingShadows,
                                      bool aBrightened )
{
    double lineWidth = aSize / 10.0;

    if( aDrawingShadows )
        lineWidth += getShadowWidth( aBrightened );

    std::vector<SCH_SHAPE> shapeList;
    SCH_SYMBOL::BuildLocalPowerIconShape( shapeList, aPos, aSize, lineWidth, aRotate );

    m_gal->SetLineWidth( lineWidth );
    m_gal->SetIsStroke( true );
    m_gal->SetStrokeColor( aColor );
    m_gal->SetFillColor( aColor );

    for( const SCH_SHAPE& shape : shapeList )
    {
        // Currently there are only 2 shapes: BEZIER and CIRCLE
        m_gal->SetIsFill( shape.GetFillMode() != FILL_T::NO_FILL );

        if( shape.GetShape() == SHAPE_T::BEZIER )
            m_gal->DrawCurve( shape.GetStart(), shape.GetBezierC1(), shape.GetBezierC2(), shape.GetEnd() );
        else if( shape.GetShape() == SHAPE_T::CIRCLE )
            m_gal->DrawCircle( shape.getCenter(), shape.GetRadius() );
    }
}


/**
 * Draw an alternate pin mode indicator icon.
 */
static void drawAltPinModesIcon( GAL& aGal, const VECTOR2D& aPos, double aSize, bool aBaseSelected,
                                 bool aRotate, int aExtraLineWidth, const COLOR4D& aColor )
{
    aGal.Save();

    aGal.Translate( aPos );

    if( aRotate )
    {
        aGal.Rotate( ANGLE_270.AsRadians() );
    }

    aGal.SetIsFill( false );
    aGal.SetIsStroke( true );
    aGal.SetLineWidth( aSize / 10.0 + aExtraLineWidth );
    aGal.SetStrokeColor( aColor );

    /*
     *  ----------->
     *      + <--center
     *     \------->
     *
     * or
     *
     *  -----  ---->
     *     \
     *      \------>
     */

    const double lineYOffset = aSize / 4;
    const double arrowHead = aSize / 8;

    const VECTOR2D topLineREnd = VECTOR2D{ aSize / 2, -lineYOffset };
    const VECTOR2D btmLineREnd = VECTOR2D{ aSize / 2, lineYOffset };

    // Top line and arrowhead
    if( aBaseSelected )
    {
        // Full top line
        aGal.DrawLine( topLineREnd, topLineREnd - VECTOR2D{ aSize, 0 } );
    }
    else
    {
        // Line with a gap
        aGal.DrawLine( topLineREnd, topLineREnd - VECTOR2D{ aSize / 2, 0 } );
        aGal.DrawLine( topLineREnd - VECTOR2D{ aSize, 0 },
                       topLineREnd - VECTOR2D{ aSize * 0.7, 0 } );
    }

    aGal.DrawLine( topLineREnd, topLineREnd - VECTOR2D{ arrowHead * 1.2, arrowHead } );
    aGal.DrawLine( topLineREnd, topLineREnd - VECTOR2D{ arrowHead * 1.2, -arrowHead } );

    // Bottom line and arrowhead
    aGal.DrawLine( btmLineREnd, btmLineREnd - VECTOR2D{ aSize / 2, 0 } );
    aGal.DrawLine( btmLineREnd, btmLineREnd - VECTOR2D{ arrowHead * 1.2, arrowHead } );
    aGal.DrawLine( btmLineREnd, btmLineREnd - VECTOR2D{ arrowHead * 1.2, -arrowHead } );

    // Top and bottom 'S' arcs
    if( !aBaseSelected )
    {
        aGal.DrawArc( topLineREnd - VECTOR2D{ aSize, -lineYOffset },
                      lineYOffset, ANGLE_0, -ANGLE_90 );
    }

    aGal.DrawArc( topLineREnd - VECTOR2D{ aSize - lineYOffset * 2, -lineYOffset },
                  lineYOffset, ANGLE_180, -ANGLE_90 );

    aGal.Restore();
};


void SCH_PAINTER::draw( const SCH_PIN* aPin, int aLayer, bool aDimmed )
{
    // Don't draw pins from a selection view-group.  Pins in a schematic must always be drawn
    // from their parent symbol's m_part.
    if( dynamic_cast<const SCH_SYMBOL*>( aPin->GetParentSymbol() ) )
        return;

    if( !isUnitAndConversionShown( aPin ) )
        return;

    const bool drawingShadows = aLayer == LAYER_SELECTION_SHADOWS;
    const bool drawingDangling = aLayer == LAYER_DANGLING;
    const bool drawingOP = aLayer == LAYER_OP_CURRENTS;

    if( m_schSettings.IsPrinting() && ( drawingShadows || drawingDangling ) )
        return;

    const bool isDangling = m_schSettings.m_IsSymbolEditor || aPin->HasFlag( IS_DANGLING );

    if( drawingShadows && !( aPin->IsBrightened() || aPin->IsSelected() ) )
        return;

    const VECTOR2I pos = aPin->GetPosition();
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
                drawPinDanglingIndicator( *aPin, color, drawingShadows, aPin->IsBrightened() );

            return;
        }
    }

    if( drawingDangling )
    {
        if( isDangling )
            drawPinDanglingIndicator( *aPin, color, drawingShadows, aPin->IsBrightened() );

        return;
    }

    if( m_schSettings.GetDrawBoundingBoxes() )
        drawItemBoundingBox( aPin );

    const VECTOR2I p0 = aPin->GetPinRoot();
    const VECTOR2I dir( sign( pos.x - p0.x ), sign( pos.y - p0.y ) );
    const int      len = aPin->GetLength();

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

            knockoutText( *m_gal, aPin->GetOperatingPoint(), mid, attrs, aPin->GetFontMetrics() );
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
    float         nameStrokeWidth = getLineWidth( aPin, false );
    float         numStrokeWidth = getLineWidth( aPin, false );

    nameStrokeWidth = ClampTextPenSize( nameStrokeWidth, aPin->GetNameTextSize(), true );
    numStrokeWidth = ClampTextPenSize( numStrokeWidth, aPin->GetNumberTextSize(), true );

    float shadowWidth = 0.0f;

    if( drawingShadows )
    {
        shadowWidth = getShadowWidth( aPin->IsBrightened() );
    }

    PIN_LAYOUT_CACHE& cache = aPin->GetLayoutCache();
    cache.SetRenderParameters( nameStrokeWidth, numStrokeWidth, m_schSettings.m_ShowPinsElectricalType,
                               m_schSettings.m_ShowPinAltIcons );

    const auto textRendersAsBitmap =
            [&]( KIGFX::GAL& aGal, int aTextSize )
            {
                // Rendering text is expensive (particularly when using outline fonts).  At small effective
                // sizes (ie: zoomed out) the visual differences between outline and/or stroke fonts and the
                // bitmap font becomes immaterial, and there's often more to draw when zoomed out so the
                // performance gain becomes more significant.
                static const float BITMAP_FONT_SIZE_THRESHOLD = 3.5;

                // Any text non bitmappable?
                return aTextSize * aGal.GetWorldScale() < BITMAP_FONT_SIZE_THRESHOLD;
            };

    // Helper function for drawing braces around multi-line text
    const auto drawBrace =
            [&]( KIGFX::GAL& aGal, const VECTOR2D& aTop, const VECTOR2D& aBottom,
                 int aBraceWidth, bool aLeftBrace, const TEXT_ATTRIBUTES& aAttrs )
            {
                // Draw a simple brace using line segments, accounting for text rotation
                VECTOR2D mid = ( aTop + aBottom ) / 2.0;

                aGal.SetLineWidth( aAttrs.m_StrokeWidth );
                aGal.SetIsFill( false );
                aGal.SetIsStroke( true );

                // Calculate brace points in text coordinate system
                VECTOR2D p1 = aTop;
                VECTOR2D p2 = aTop;
                VECTOR2D p3 = mid;
                VECTOR2D p4 = aBottom;
                VECTOR2D p5 = aBottom;

                // Apply brace offset based on text orientation
                if( aAttrs.m_Angle == ANGLE_VERTICAL )
                {
                    // For vertical text, braces extend in the Y direction
                    // "Left" brace is actually towards negative Y, "right" towards positive Y
                    double braceOffset = aLeftBrace ? -aBraceWidth : aBraceWidth;
                    p2.y += braceOffset / 2;
                    p3.y += braceOffset;
                    p4.y += braceOffset / 2;
                }
                else
                {
                    // For horizontal text, braces extend in the X direction
                    double braceOffset = aLeftBrace ? -aBraceWidth : aBraceWidth;
                    p2.x += braceOffset / 2;
                    p3.x += braceOffset;
                    p4.x += braceOffset / 2;
                }

                // Draw the brace segments
                aGal.DrawLine( p1, p2 );
                aGal.DrawLine( p2, p3 );
                aGal.DrawLine( p3, p4 );
                aGal.DrawLine( p4, p5 );
            };

    const auto drawBracesAroundText =
            [&]( KIGFX::GAL& aGal, const wxArrayString& aLines, const VECTOR2D& aStartPos,
                 int aLineSpacing, const TEXT_ATTRIBUTES& aAttrs )
            {
                if( aLines.size() <= 1 )
                    return;

                // Calculate brace dimensions
                int braceWidth = aAttrs.m_Size.x / 3;  // Make braces a bit larger

                // Find the maximum line width to position braces
                int maxLineWidth = 0;
                KIFONT::FONT* font = aAttrs.m_Font;

                if( !font )
                    font = KIFONT::FONT::GetFont( eeconfig()->m_Appearance.default_font );

                for( const wxString& line : aLines )
                {
                    wxString trimmedLine = line;
                    trimmedLine.Trim( true ).Trim( false );
                    VECTOR2I lineExtents = font->StringBoundaryLimits( trimmedLine, aAttrs.m_Size,
                                                                       aAttrs.m_StrokeWidth, false, false,
                                                                       KIFONT::METRICS() );
                    maxLineWidth = std::max( maxLineWidth, lineExtents.x );
                }

                // Calculate brace positions based on text vertical alignment and rotation
                VECTOR2D braceStart = aStartPos;
                VECTOR2D braceEnd = aStartPos;

                // Extend braces beyond the text bounds
                int textHeight = aAttrs.m_Size.y;
                int extraHeight = textHeight / 3;  // Extend braces by 1/3 of text height beyond text

                if( aAttrs.m_Angle == ANGLE_VERTICAL )
                {
                    // For vertical text, lines are spaced horizontally and braces are horizontal
                    braceEnd.x += ( (int) aLines.size() - 1 ) * aLineSpacing;

                    // Extend braces horizontally to encompass all lines plus extra space
                    braceStart.x -= 2 * extraHeight;

                    // Position braces in the perpendicular direction (Y) with proper spacing
                    int braceSpacing = maxLineWidth / 2 + braceWidth;

                    VECTOR2D topBraceStart = braceStart;
                    topBraceStart.y -= braceSpacing;

                    VECTOR2D topBraceEnd = braceEnd;
                    topBraceEnd.y -= braceSpacing;

                    drawBrace( aGal, topBraceStart, topBraceEnd, braceWidth, true, aAttrs );

                    VECTOR2D bottomBraceStart = braceStart;
                    bottomBraceStart.y += braceSpacing;

                    VECTOR2D bottomBraceEnd = braceEnd;
                    bottomBraceEnd.y += braceSpacing;

                    drawBrace( aGal, bottomBraceStart, bottomBraceEnd, braceWidth, false, aAttrs );
                }
                else
                {
                    // For horizontal text, lines are spaced vertically and braces are vertical
                    braceEnd.y += ( (int) aLines.size() - 1 ) * aLineSpacing;

                    // Extend braces vertically to encompass all lines plus extra space
                    braceStart.y -= 2 * extraHeight;

                    // Position braces in the perpendicular direction (X) with proper spacing
                    int braceSpacing = maxLineWidth / 2 + braceWidth;

                    // Draw left brace
                    VECTOR2D leftTop = braceStart;
                    leftTop.x -= braceSpacing;

                    VECTOR2D leftBottom = braceEnd;
                    leftBottom.x -= braceSpacing;

                    drawBrace( aGal, leftTop, leftBottom, braceWidth, true, aAttrs );

                    // Draw right brace
                    VECTOR2D rightTop = braceStart;
                    rightTop.x += braceSpacing;

                    VECTOR2D rightBottom = braceEnd;
                    rightBottom.x += braceSpacing;

                    drawBrace( aGal, rightTop, rightBottom, braceWidth, false, aAttrs );
                }
            };

    const auto drawBracesAroundTextBitmap =
            [&]( KIGFX::GAL& aGal, const wxArrayString& aLines, const VECTOR2D& aStartPos,
                 int aLineSpacing, const TEXT_ATTRIBUTES& aAttrs )
            {
                // Simplified brace drawing for bitmap text
                if( aLines.size() <= 1 )
                    return;

                int braceWidth = aAttrs.m_Size.x / 4;

                // Estimate max line width (less precise for bitmap text)
                int maxLineWidth = aAttrs.m_Size.x * 4;  // Conservative estimate

                // Calculate brace positions based on rotation
                VECTOR2D braceStart = aStartPos;
                VECTOR2D braceEnd = aStartPos;

                int textHalfHeight = aAttrs.m_Size.y / 2;

                if( aAttrs.m_Angle == ANGLE_VERTICAL )
                {
                    // For vertical text, lines are spaced horizontally
                    braceEnd.x += ( (int) aLines.size() - 1 ) * aLineSpacing;

                    VECTOR2D leftStart = braceStart;
                    leftStart.y -= maxLineWidth / 2.0 + braceWidth / 2.0;

                    VECTOR2D leftEnd = braceEnd;
                    leftEnd.y -= maxLineWidth / 2.0 + braceWidth / 2.0;

                    drawBrace( aGal, leftStart, leftEnd, braceWidth, true, aAttrs );

                    VECTOR2D rightStart = braceStart;
                    rightStart.y += maxLineWidth / 2.0 + braceWidth / 2.0;

                    VECTOR2D rightEnd = braceEnd;
                    rightEnd.y += maxLineWidth / 2.0 + braceWidth / 2.0;

                    drawBrace( aGal, rightStart, rightEnd, braceWidth, false, aAttrs );
                }
                else
                {
                    // For horizontal text, lines are spaced vertically
                    braceEnd.y += ( (int) aLines.size() - 1 ) * aLineSpacing;

                    VECTOR2D braceTop = braceStart;
                    braceTop.y -= textHalfHeight;

                    VECTOR2D braceBottom = braceEnd;
                    braceBottom.y += textHalfHeight;

                    VECTOR2D leftTop = braceTop;
                    leftTop.x -= maxLineWidth / 2.0 + braceWidth / 2.0;

                    VECTOR2D leftBottom = braceBottom;
                    leftBottom.x -= maxLineWidth / 2.0 + braceWidth / 2.0;

                    drawBrace( aGal, leftTop, leftBottom, braceWidth, true, aAttrs );

                    VECTOR2D rightTop = braceTop;
                    rightTop.x += maxLineWidth / 2.0 + braceWidth / 2.0;

                    VECTOR2D rightBottom = braceBottom;
                    rightBottom.x += maxLineWidth / 2.0 + braceWidth / 2.0;

                    drawBrace( aGal, rightTop, rightBottom, braceWidth, false, aAttrs );
                }
            };

    // Helper functions for drawing multi-line pin text with braces
    const auto drawMultiLineText =
            [&]( KIGFX::GAL& aGal, const wxString& aText, const VECTOR2D& aPosition,
                 const TEXT_ATTRIBUTES& aAttrs, const KIFONT::METRICS& aFontMetrics )
            {
                // Check if this is multi-line stacked pin text with braces
                if( aText.StartsWith( "[" ) && aText.EndsWith( "]" ) && aText.Contains( "\n" ) )
                {
                    // Extract content between braces and split into lines
                    wxString content = aText.Mid( 1, aText.Length() - 2 );
                    wxArrayString lines;
                    wxStringSplit( content, lines, '\n' );

                    if( lines.size() > 1 )
                    {
                        // Calculate line spacing (similar to EDA_TEXT::GetInterline)
                        int lineSpacing = KiROUND( aAttrs.m_Size.y * 1.3 );  // 130% of text height

                        // Calculate positioning based on text alignment and rotation
                        VECTOR2D startPos = aPosition;

                        if( aAttrs.m_Angle == ANGLE_VERTICAL )
                        {
                            // For vertical text, lines are spaced horizontally
                            // Adjust start position based on horizontal alignment
                            if( aAttrs.m_Halign == GR_TEXT_H_ALIGN_RIGHT )
                            {
                                int totalWidth = ( (int) lines.size() - 1 ) * lineSpacing;
                                startPos.x -= totalWidth;
                            }
                            else if( aAttrs.m_Halign == GR_TEXT_H_ALIGN_CENTER )
                            {
                                int totalWidth = ( (int) lines.size() - 1 ) * lineSpacing;
                                startPos.x -= totalWidth / 2.0;
                            }

                            // Draw each line
                            for( size_t i = 0; i < lines.size(); i++ )
                            {
                                VECTOR2D linePos = startPos;
                                linePos.x += i * lineSpacing;

                                wxString line = lines[i];
                                line.Trim( true ).Trim( false );

                                strokeText( aGal, line, linePos, aAttrs, aFontMetrics );
                            }
                        }
                        else
                        {
                            // For horizontal text, lines are spaced vertically
                            // Adjust start position based on vertical alignment
                            if( aAttrs.m_Valign == GR_TEXT_V_ALIGN_BOTTOM )
                            {
                                int totalHeight = ( (int) lines.size() - 1 ) * lineSpacing;
                                startPos.y -= totalHeight;
                            }
                            else if( aAttrs.m_Valign == GR_TEXT_V_ALIGN_CENTER )
                            {
                                int totalHeight = ( (int) lines.size() - 1 ) * lineSpacing;
                                startPos.y -= totalHeight / 2.0;
                            }

                            // Draw each line
                            for( size_t i = 0; i < lines.size(); i++ )
                            {
                                VECTOR2D linePos = startPos;
                                linePos.y += (int) i * lineSpacing;

                                wxString line = lines[i];
                                line.Trim( true ).Trim( false );

                                strokeText( aGal, line, linePos, aAttrs, aFontMetrics );
                            }
                        }

                        // Draw braces around the text
                        drawBracesAroundText( aGal, lines, startPos, lineSpacing, aAttrs );
                        return;
                    }
                }

                // Fallback to regular single-line text
                strokeText( aGal, aText, aPosition, aAttrs, aFontMetrics );
            };

    const auto drawMultiLineTextBox =
            [&]( KIGFX::GAL& aGal, const wxString& aText, const VECTOR2D& aPosition,
                 const TEXT_ATTRIBUTES& aAttrs, const KIFONT::METRICS& aFontMetrics )
            {
                // Similar to drawMultiLineText but uses boxText for outline fonts
                if( aText.StartsWith( "[" ) && aText.EndsWith( "]" ) && aText.Contains( "\n" ) )
                {
                    wxString content = aText.Mid( 1, aText.Length() - 2 );
                    wxArrayString lines;
                    wxStringSplit( content, lines, '\n' );

                    if( lines.size() > 1 )
                    {
                        int lineSpacing = KiROUND( aAttrs.m_Size.y * 1.3 );
                        VECTOR2D startPos = aPosition;

                        if( aAttrs.m_Angle == ANGLE_VERTICAL )
                        {
                            // For vertical text, lines are spaced horizontally
                            if( aAttrs.m_Halign == GR_TEXT_H_ALIGN_RIGHT )
                            {
                                int totalWidth = ( (int) lines.size() - 1 ) * lineSpacing;
                                startPos.x -= totalWidth;
                            }
                            else if( aAttrs.m_Halign == GR_TEXT_H_ALIGN_CENTER )
                            {
                                int totalWidth = ( (int) lines.size() - 1 ) * lineSpacing;
                                startPos.x -= totalWidth / 2.0;
                            }

                            for( size_t i = 0; i < lines.size(); i++ )
                            {
                                VECTOR2D linePos = startPos;
                                linePos.x += (int) i * lineSpacing;

                                wxString line = lines[i];
                                line.Trim( true ).Trim( false );

                                boxText( aGal, line, linePos, aAttrs, aFontMetrics );
                            }
                        }
                        else
                        {
                            // For horizontal text, lines are spaced vertically
                            if( aAttrs.m_Valign == GR_TEXT_V_ALIGN_BOTTOM )
                            {
                                int totalHeight = ( (int) lines.size() - 1 ) * lineSpacing;
                                startPos.y -= totalHeight;
                            }
                            else if( aAttrs.m_Valign == GR_TEXT_V_ALIGN_CENTER )
                            {
                                int totalHeight = ( (int) lines.size() - 1 ) * lineSpacing;
                                startPos.y -= totalHeight / 2.0;
                            }

                            for( size_t i = 0; i < lines.size(); i++ )
                            {
                                VECTOR2D linePos = startPos;
                                linePos.y += (int) i * lineSpacing;

                                wxString line = lines[i];
                                line.Trim( true ).Trim( false );

                                boxText( aGal, line, linePos, aAttrs, aFontMetrics );
                            }
                        }

                        drawBracesAroundText( aGal, lines, startPos, lineSpacing, aAttrs );
                        return;
                    }
                }

                boxText( aGal, aText, aPosition, aAttrs, aFontMetrics );
            };

    const auto drawMultiLineBitmapText =
            [&]( KIGFX::GAL& aGal, const wxString& aText, const VECTOR2D& aPosition,
                 const TEXT_ATTRIBUTES& aAttrs )
            {
                // Similar to drawMultiLineText but uses bitmapText
                if( aText.StartsWith( "[" ) && aText.EndsWith( "]" ) && aText.Contains( "\n" ) )
                {
                    wxString content = aText.Mid( 1, aText.Length() - 2 );
                    wxArrayString lines;
                    wxStringSplit( content, lines, '\n' );

                    if( lines.size() > 1 )
                    {
                        int lineSpacing = KiROUND( aAttrs.m_Size.y * 1.3 );
                        VECTOR2D startPos = aPosition;

                        if( aAttrs.m_Angle == ANGLE_VERTICAL )
                        {
                            // For vertical text, lines are spaced horizontally
                            if( aAttrs.m_Halign == GR_TEXT_H_ALIGN_RIGHT )
                            {
                                int totalWidth = ( (int) lines.size() - 1 ) * lineSpacing;
                                startPos.x -= totalWidth;
                            }
                            else if( aAttrs.m_Halign == GR_TEXT_H_ALIGN_CENTER )
                            {
                                int totalWidth = ( (int) lines.size() - 1 ) * lineSpacing;
                                startPos.x -= totalWidth / 2.0;
                            }

                            for( size_t i = 0; i < lines.size(); i++ )
                            {
                                VECTOR2D linePos = startPos;
                                linePos.x += (int) i * lineSpacing;

                                wxString line = lines[i];
                                line.Trim( true ).Trim( false );

                                bitmapText( aGal, line, linePos, aAttrs );
                            }
                        }
                        else
                        {
                            // For horizontal text, lines are spaced vertically
                            if( aAttrs.m_Valign == GR_TEXT_V_ALIGN_BOTTOM )
                            {
                                int totalHeight = ( (int) lines.size() - 1 ) * lineSpacing;
                                startPos.y -= totalHeight;
                            }
                            else if( aAttrs.m_Valign == GR_TEXT_V_ALIGN_CENTER )
                            {
                                int totalHeight = ( (int) lines.size() - 1 ) * lineSpacing;
                                startPos.y -= totalHeight / 2.0;
                            }

                            for( size_t i = 0; i < lines.size(); i++ )
                            {
                                VECTOR2D linePos = startPos;
                                linePos.y += (int) i * lineSpacing;

                                wxString line = lines[i];
                                line.Trim( true ).Trim( false );

                                bitmapText( aGal, line, linePos, aAttrs );
                            }
                        }

                        // Draw braces with bitmap text (simplified version)
                        drawBracesAroundTextBitmap( aGal, lines, startPos, lineSpacing, aAttrs );
                        return;
                    }
                }

                bitmapText( aGal, aText, aPosition, aAttrs );
            };

    const auto drawTextInfo =
            [&]( const PIN_LAYOUT_CACHE::TEXT_INFO& aTextInfo, const COLOR4D& aColor )
            {
                // const double iconSize = std::min( aPin->GetNameTextSize(), schIUScale.mmToIU( 1.5 ) );
                const bool renderTextAsBitmap = textRendersAsBitmap( *m_gal, aTextInfo.m_TextSize );

                // Which of these gets used depends on the font technology, so set both
                m_gal->SetStrokeColor( aColor );
                m_gal->SetFillColor( aColor );

                TEXT_ATTRIBUTES attrs;
                attrs.m_Font = KIFONT::FONT::GetFont( eeconfig()->m_Appearance.default_font );
                attrs.m_Size = VECTOR2I( aTextInfo.m_TextSize, aTextInfo.m_TextSize );
                attrs.m_Halign = aTextInfo.m_HAlign;
                attrs.m_Valign = aTextInfo.m_VAlign;
                attrs.m_Angle = aTextInfo.m_Angle;
                attrs.m_StrokeWidth = aTextInfo.m_Thickness;

                if( drawingShadows )
                {
                    attrs.m_StrokeWidth += KiROUND( shadowWidth );

                    if( !attrs.m_Font->IsOutline() )
                    {
                        drawMultiLineText( *m_gal, aTextInfo.m_Text, aTextInfo.m_TextPosition, attrs,
                                         aPin->GetFontMetrics() );
                    }
                    else
                    {
                        drawMultiLineTextBox( *m_gal, aTextInfo.m_Text, aTextInfo.m_TextPosition, attrs,
                                            aPin->GetFontMetrics() );
                    }
                }
                else if( nonCached( aPin ) && renderTextAsBitmap )
                {
                    drawMultiLineBitmapText( *m_gal, aTextInfo.m_Text, aTextInfo.m_TextPosition, attrs );
                    const_cast<SCH_PIN*>( aPin )->SetFlags( IS_SHOWN_AS_BITMAP );
                }
                else
                {
                    drawMultiLineText( *m_gal, aTextInfo.m_Text, aTextInfo.m_TextPosition, attrs,
                                       aPin->GetFontMetrics() );
                    const_cast<SCH_PIN*>( aPin )->SetFlags( IS_SHOWN_AS_BITMAP );
                }
            };

    const auto getColorForLayer =
            [&]( int aDrawnLayer )
            {
                if( !aPin->IsVisible() )
                    return getRenderColor( aPin, LAYER_HIDDEN, drawingShadows, aDimmed );

                return getRenderColor( aPin, aDrawnLayer, drawingShadows, aDimmed );
            };

    // Request text layout info and draw it

    if( std::optional<PIN_LAYOUT_CACHE::TEXT_INFO> numInfo = cache.GetPinNumberInfo( shadowWidth ) )
        drawTextInfo( *numInfo, getColorForLayer( LAYER_PINNUM ) );

    if( std::optional<PIN_LAYOUT_CACHE::TEXT_INFO> nameInfo = cache.GetPinNameInfo( shadowWidth ) )
    {
        drawTextInfo( *nameInfo, getColorForLayer( LAYER_PINNAM ) );

        if( OPT_BOX2I altIconBox = cache.GetAltIconBBox() )
        {
            drawAltPinModesIcon( *m_gal, altIconBox->GetCenter(), altIconBox->GetWidth(),
                                 // Icon style doesn't work due to the tempPin having no alt
                                 // but maybe it's better with just one style anyway.
                                 true, nameInfo->m_Angle == ANGLE_VERTICAL, shadowWidth,
                                 getColorForLayer( LAYER_PINNAM ) );
        }
    }

    if( std::optional<PIN_LAYOUT_CACHE::TEXT_INFO> elecTypeInfo = cache.GetPinElectricalTypeInfo( shadowWidth ) )
        drawTextInfo( *elecTypeInfo, getColorForLayer( LAYER_PRIVATE_NOTES ) );
}


void SCH_PAINTER::drawAnchor( const VECTOR2I& aPos, bool aDrawingShadows )
{
    if( m_schSettings.IsPrinting() )
        return;

    // In order for the anchors to be visible but unobtrusive, their size must factor in the
    // current zoom level.
    const MATRIX3x3D& matrix = m_gal->GetScreenWorldMatrix();
    int radius = KiROUND( std::fabs( matrix.GetScale().x * TEXT_ANCHOR_SIZE ) / 25.0 )
                     + schIUScale.MilsToIU( TEXT_ANCHOR_SIZE );

    COLOR4D color = aDrawingShadows ? m_schSettings.GetLayerColor( LAYER_SELECTION_SHADOWS )
                                    : m_schSettings.GetLayerColor( LAYER_SCHEMATIC_ANCHOR );

    m_gal->SetStrokeColor( color );
    m_gal->SetIsStroke( true );
    m_gal->SetLineWidth( aDrawingShadows ? getShadowWidth( false )
                                         : m_schSettings.GetDanglingIndicatorThickness() );

    m_gal->DrawLine( aPos - VECTOR2I( radius, 0 ), aPos + VECTOR2I( radius, 0 ) );
    m_gal->DrawLine( aPos - VECTOR2I( 0, radius ), aPos + VECTOR2I( 0, radius ) );
}


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
    bool highlightNetclassColors = false;
    EESCHEMA_SETTINGS* eeschemaCfg = eeconfig();

    if( eeschemaCfg )
    {
        highlightNetclassColors = eeschemaCfg->m_Selection.highlight_netclass_colors;
    }

    bool drawingShadows = aLayer == LAYER_SELECTION_SHADOWS;

    if( m_schSettings.IsPrinting() && drawingShadows )
        return;

    if( drawingShadows && !( aJct->IsBrightened() || aJct->IsSelected() ) )
        return;

    COLOR4D color;

    if( highlightNetclassColors && aLayer == aJct->GetLayer() )
        color = m_schSettings.GetLayerColor( aJct->GetLayer() );
    else
        color = getRenderColor( aJct, aJct->GetLayer(), drawingShadows );

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
    bool drawingNetColorHighlights = aLayer == LAYER_NET_COLOR_HIGHLIGHT;
    bool drawingWires = aLayer == LAYER_WIRE;
    bool drawingBusses = aLayer == LAYER_BUS;
    bool drawingDangling = aLayer == LAYER_DANGLING;
    bool drawingOP = aLayer == LAYER_OP_VOLTAGES;

    bool highlightNetclassColors = false;
    double             highlightAlpha = 0.6;
    EESCHEMA_SETTINGS* eeschemaCfg = eeconfig();
    double             hopOverScale = 0.0;

    if( aLine->Schematic() )    // Can be nullptr when run from the color selection panel
        hopOverScale = aLine->Schematic()->Settings().m_HopOverScale;

    if( eeschemaCfg )
    {
        highlightNetclassColors = eeschemaCfg->m_Selection.highlight_netclass_colors;
        highlightAlpha = eeschemaCfg->m_Selection.highlight_netclass_colors_alpha;
    }

    if( !highlightNetclassColors && drawingNetColorHighlights )
        return;

    if( drawingNetColorHighlights && !( aLine->IsWire() || aLine->IsBus() ) )
        return;

    if( m_schSettings.m_OverrideItemColors && drawingNetColorHighlights )
        return;

    if( m_schSettings.IsPrinting() && drawingShadows )
        return;

    if( drawingShadows && !( aLine->IsBrightened() || aLine->IsSelected() ) )
        return;

    // Line end dangling status isn't updated until the line is finished drawing, so don't warn
    // them about ends that are probably connected
    if( aLine->IsNew() && drawingDangling )
        return;

    COLOR4D    color = getRenderColor( aLine, aLine->GetLayer(), drawingShadows );
    float      width = getLineWidth( aLine, drawingShadows, drawingNetColorHighlights );
    LINE_STYLE lineStyle = aLine->GetEffectiveLineStyle();

    if( highlightNetclassColors )
    {
        // Force default color for nets we are going to highlight
        if( drawingWires )
            color = m_schSettings.GetLayerColor( LAYER_WIRE );
        else if( drawingBusses )
            color = m_schSettings.GetLayerColor( LAYER_BUS );
    }

    if( drawingNetColorHighlights )
    {
        // Don't draw highlights for default-colored nets
        if( ( aLine->IsWire() && color == m_schSettings.GetLayerColor( LAYER_WIRE ) )
            || ( aLine->IsBus() && color == m_schSettings.GetLayerColor( LAYER_BUS ) ) )
        {
            return;
        }

        color = color.WithAlpha( color.a * highlightAlpha );
    }

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

        knockoutText( *m_gal, aLine->GetOperatingPoint(), pos, attrs, aLine->GetFontMetrics() );
    }

    if( drawingOP )
        return;

    m_gal->SetIsStroke( true );
    m_gal->SetIsFill( false );
    m_gal->SetStrokeColor( color );
    m_gal->SetLineWidth( width );

    std::vector<VECTOR3I> curr_wire_shape;

    if( aLine->IsWire() && hopOverScale > 0.0 )
    {
        double   lineWidth = getLineWidth( aLine, false, drawingNetColorHighlights );
        double   arcRadius = lineWidth * hopOverScale;
        curr_wire_shape = aLine->BuildWireWithHopShape( m_schematic->GetCurrentScreen(), arcRadius );
    }
    else
    {
        curr_wire_shape.emplace_back( aLine->GetStartPoint().x, aLine->GetStartPoint().y, 0 );
        curr_wire_shape.emplace_back( aLine->GetEndPoint().x, aLine->GetEndPoint().y, 0 );
    }

    for( size_t ii = 1; ii < curr_wire_shape.size(); ii++ )
    {
        VECTOR2I start( curr_wire_shape[ii-1].x, curr_wire_shape[ii-1].y );

        if( curr_wire_shape[ii-1].z == 0 )  // This is the start point of a segment
                                            // there are always 2 points in list for a segment
        {
            VECTOR2I end( curr_wire_shape[ii].x, curr_wire_shape[ii].y );
            drawLine( start, end, lineStyle,
                      ( lineStyle <= LINE_STYLE::FIRST_TYPE || drawingShadows ), width );
        }
        else   // This is the start point of a arc. there are always 3 points in list for an arc
        {
            // Hop are a small arc, so use a solid line style gives best results
            VECTOR2I arc_middle( curr_wire_shape[ii].x, curr_wire_shape[ii].y );
            ii++;
            VECTOR2I arc_end( curr_wire_shape[ii].x, curr_wire_shape[ii].y );
            ii++;

            VECTOR2D dstart = start;
            VECTOR2D dmid = arc_middle;
            VECTOR2D dend = arc_end;
            VECTOR2D center = CalcArcCenter( dstart, dmid, dend );

            EDA_ANGLE startAngle( dstart - center );
            EDA_ANGLE midAngle( dmid - center );
            EDA_ANGLE endAngle( dend - center );

            EDA_ANGLE angle1 = midAngle - startAngle;
            EDA_ANGLE angle2 = endAngle - midAngle;

            EDA_ANGLE angle = angle1.Normalize180() + angle2.Normalize180();

            m_gal->DrawArc( center, ( dstart - center ).EuclideanNorm(), startAngle, angle );
        }
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
    COLOR4D    color = getRenderColor( aShape, aLayer, drawingShadows, aDimmed );

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
                    if( shape->GetCornerRadius() > 0 )
                    {
                        // Creates a normalized ROUNDRECT item
                        // (GetRectangleWidth() and GetRectangleHeight() can be < 0 with transforms
                        ROUNDRECT rr( SHAPE_RECT( shape->GetPosition(),
                                                  shape->GetRectangleWidth(),
                                                  shape->GetRectangleHeight() ),
                                      shape->GetCornerRadius(), true /* normalize */ );
                        SHAPE_POLY_SET poly;
                        rr.TransformToPolygon( poly, shape->GetMaxError() );
                        m_gal->DrawPolygon( poly );
                    }
                    else
                    {
                        m_gal->DrawRectangle( shape->GetPosition(), shape->GetEnd() );
                    }
                    break;

                case SHAPE_T::POLY:
                {
                    const std::vector<SHAPE*> polySegments = shape->MakeEffectiveShapes( true );

                    if( !polySegments.empty() )
                    {
                        std::deque<VECTOR2D> pts;

                        for( SHAPE* polySegment : polySegments )
                            pts.push_back( static_cast<SHAPE_SEGMENT*>( polySegment )->GetSeg().A );

                        pts.push_back( static_cast<SHAPE_SEGMENT*>( polySegments.back() )->GetSeg().B );

                        for( SHAPE* polySegment : polySegments )
                            delete polySegment;

                        m_gal->DrawPolygon( pts );
                    }
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
                m_gal->SetIsFill( aShape->IsSolidFill() );
            else
                m_gal->SetIsFill( true );

            m_gal->SetFillColor( color );
        }
        else
        {
            m_gal->SetIsFill( false );
        }

        // We still always draw the stroke, as otherwise single-segment shapes
        // (like a line) don't get a shadow, and special-casing them looks inconsistent.
        m_gal->SetIsStroke( true );
        m_gal->SetLineWidth( getLineWidth( aShape, true ) );
        m_gal->SetStrokeColor( color );

        drawShape( aShape );
    }
    else if( aLayer == LAYER_DEVICE_BACKGROUND || aLayer == LAYER_NOTES_BACKGROUND
             || aLayer == LAYER_SHAPES_BACKGROUND )
    {
        switch( aShape->GetFillMode() )
        {
        case FILL_T::NO_FILL:
            break;

        case FILL_T::FILLED_SHAPE:
            // Fill in the foreground layer
            break;

        case FILL_T::HATCH:
        case FILL_T::REVERSE_HATCH:
        case FILL_T::CROSS_HATCH:
            aShape->UpdateHatching();
            m_gal->SetIsFill( false );
            m_gal->SetIsStroke( true );
            m_gal->SetStrokeColor( color );
            m_gal->SetLineWidth( aShape->GetHatchLineWidth() );

            for( const SEG& seg : aShape->GetHatchLines() )
                m_gal->DrawLine( seg.A, seg.B );

            break;

        case FILL_T::FILLED_WITH_COLOR:
        case FILL_T::FILLED_WITH_BG_BODYCOLOR:
            // Do not fill the shape in B&W print mode, to avoid to visible items inside the shape
            if( !m_schSettings.PrintBlackAndWhiteReq() )
            {
                m_gal->SetIsFill( true );
                m_gal->SetIsStroke( false );
                m_gal->SetFillColor( color );

                drawShape( aShape );
            }
            break;

        default:
            wxFAIL_MSG( wxT( "Unsupported fill type" ) );
        }
    }
    else if( aLayer == LAYER_DEVICE || aLayer == LAYER_NOTES || aLayer == LAYER_PRIVATE_NOTES
             || aLayer == LAYER_RULE_AREAS )
    {
        // Shapes filled with the device colour must be filled in the foreground
        if( aShape->GetFillMode() == FILL_T::FILLED_SHAPE )
        {
            m_gal->SetIsFill( true );
            m_gal->SetIsStroke( false );
            m_gal->SetFillColor( color );

            drawShape( aShape );
        }

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
    m_gal->SetHoverColor( color );

    wxString        shownText( aText->GetShownText( true ) );
    VECTOR2I        text_offset = aText->GetSchematicTextOffset( &m_schSettings );
    TEXT_ATTRIBUTES attrs = aText->GetAttributes();
    KIFONT::FONT*   font = getFont( aText );

    attrs.m_Angle = aText->GetDrawRotation();
    attrs.m_StrokeWidth = KiROUND( getTextThickness( aText ) );

    // Adjust text drawn in an outline font to more closely mimic the positioning of
    // SCH_FIELD text.
    if( font->IsOutline() && aText->Type() == SCH_TEXT_T )
    {
        BOX2I    firstLineBBox = aText->GetTextBox( nullptr, 0 );
        int      sizeDiff = firstLineBBox.GetHeight() - aText->GetTextSize().y;
        int      adjust = KiROUND( sizeDiff * 0.35 );
        VECTOR2I adjust_offset( 0, adjust );

        RotatePoint( adjust_offset, aText->GetDrawRotation() );
        text_offset += adjust_offset;
    }

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

        strokeText( *m_gal, shownText, pos, attrs, aText->GetFontMetrics() );
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

        strokeText( *m_gal, shownText, aText->GetDrawPos() + text_offset, attrs,
                    aText->GetFontMetrics() );
    }
    else
    {
        wxString activeUrl;

        if( aText->IsRollover() && !aText->IsMoving() )
        {
            // Highlight any urls found within the text
            m_gal->SetHoverColor( m_schSettings.GetLayerColor( LAYER_HOVERED ) );

            // Highlight the whole text if it has a link definition
            if( aText->HasHyperlink() )
            {
                attrs.m_Hover = true;
                attrs.m_Underlined = true;
                activeUrl = aText->GetHyperlink();
            }
        }

        if( nonCached( aText ) && aText->RenderAsBitmap( m_gal->GetWorldScale() )
                               && !shownText.Contains( wxT( "\n" ) ) )
        {
            bitmapText( *m_gal, shownText, aText->GetDrawPos() + text_offset, attrs );
            const_cast<SCH_TEXT*>( aText )->SetFlags( IS_SHOWN_AS_BITMAP );
        }
        else
        {
            std::vector<std::unique_ptr<KIFONT::GLYPH>>* cache = nullptr;

            if( !aText->IsRollover() && font->IsOutline() )
                cache = aText->GetRenderCache( font, shownText, text_offset );

            if( cache )
            {
                m_gal->SetLineWidth( attrs.m_StrokeWidth );
                m_gal->DrawGlyphs( *cache );
            }
            else
            {
                strokeText( *m_gal, shownText, aText->GetDrawPos() + text_offset, attrs,
                            aText->GetFontMetrics(), aText->GetRolloverPos(), &activeUrl );
            }

            const_cast<SCH_TEXT*>( aText )->ClearFlags( IS_SHOWN_AS_BITMAP );
        }

        aText->SetActiveUrl( activeUrl );
    }

    // Draw anchor
    if( aText->IsSelected() )
    {
        bool showAnchor;

        switch( aText->Type() )
        {
        case SCH_TEXT_T:
            showAnchor = true;
            break;

        case SCH_LABEL_T:
            // Don't clutter things up if we're already showing a dangling indicator
            showAnchor = !static_cast<const SCH_LABEL*>( aText )->IsDangling();
            break;

        case SCH_DIRECTIVE_LABEL_T:
        case SCH_HIER_LABEL_T:
        case SCH_GLOBAL_LABEL_T:
        case SCH_SHEET_PIN_T:
            // These all have shapes and so don't need anchors
            showAnchor = false;
            break;

        default:
            showAnchor = false;
            break;
        }

        if( showAnchor )
            drawAnchor( aText->GetPosition(), drawingShadows );
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
                wxString        activeUrl;

                attrs.m_Angle = aTextBox->GetDrawRotation();
                attrs.m_StrokeWidth = KiROUND( getTextThickness( aTextBox ) );

                if( aTextBox->IsRollover() && !aTextBox->IsMoving() )
                {
                    // Highlight any urls found within the text
                    m_gal->SetHoverColor( m_schSettings.GetLayerColor( LAYER_HOVERED ) );

                    // Highlight the whole text if it has a link definition
                    if( aTextBox->HasHyperlink() )
                    {
                        attrs.m_Hover = true;
                        attrs.m_Underlined = true;
                        activeUrl = aTextBox->GetHyperlink();
                    }
                }

                std::vector<std::unique_ptr<KIFONT::GLYPH>>* cache = nullptr;

                if( !aTextBox->IsRollover() && font->IsOutline() )
                    cache = aTextBox->GetRenderCache( font, shownText );

                if( cache )
                {
                    m_gal->SetLineWidth( attrs.m_StrokeWidth );
                    m_gal->DrawGlyphs( *cache );
                }
                else
                {
                    strokeText( *m_gal, shownText, aTextBox->GetDrawPos(), attrs,
                                aTextBox->GetFontMetrics(), aTextBox->GetRolloverPos(), &activeUrl );
                }

                aTextBox->SetActiveUrl( activeUrl );
            };

    if( drawingShadows && !( aTextBox->IsBrightened() || aTextBox->IsSelected() ) )
        return;

    m_gal->SetFillColor( color );
    m_gal->SetStrokeColor( color );
    m_gal->SetHoverColor( color );

    if( aLayer == LAYER_SELECTION_SHADOWS )
    {
        m_gal->SetIsFill( true );
        m_gal->SetIsStroke( false );
        m_gal->SetLineWidth( borderWidth );

        m_gal->DrawRectangle( aTextBox->GetPosition(), aTextBox->GetEnd() );
    }
    else if( aLayer == LAYER_DEVICE_BACKGROUND || aLayer == LAYER_NOTES_BACKGROUND
             || aLayer == LAYER_SHAPES_BACKGROUND )
    {
        // Do not fill the shape in B&W print mode, to avoid to visible items
        // inside the shape
        if( aTextBox->IsSolidFill() && !m_schSettings.PrintBlackAndWhiteReq() )
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
    if( aTable->GetCells().empty() )
        return;

    for( SCH_TABLECELL* cell : aTable->GetCells() )
        draw( cell, aLayer, aDimmed );

    if( aLayer == LAYER_SELECTION_SHADOWS )
        return;

    aTable->DrawBorders(
            [&]( const VECTOR2I& ptA, const VECTOR2I& ptB, const STROKE_PARAMS& stroke )
            {
                int        lineWidth = stroke.GetWidth();
                COLOR4D    color = stroke.GetColor();
                LINE_STYLE lineStyle = stroke.GetLineStyle();

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

                if( lineStyle <= LINE_STYLE::FIRST_TYPE )
                {
                    m_gal->DrawLine( ptA, ptB );
                }
                else
                {
                    SHAPE_SEGMENT seg( ptA, ptB );
                    STROKE_PARAMS::Stroke( &seg, lineStyle, lineWidth, &m_schSettings,
                            [&]( const VECTOR2I& a, const VECTOR2I& b )
                            {
                                // DrawLine has problem with 0 length lines so enforce minimum
                                if( a == b )
                                    m_gal->DrawLine( a+1, b );
                                else
                                    m_gal->DrawLine( a, b );
                            } );
                }
            } );
}


wxString SCH_PAINTER::expandLibItemTextVars( const wxString& aSourceText,
                                             const SCH_SYMBOL* aSymbolContext )
{
    std::function<bool( wxString* )> symbolResolver =
            [&]( wxString* token ) -> bool
            {
                if( !m_schematic )
                    return false;

                return aSymbolContext->ResolveTextVar( &m_schematic->CurrentSheet(), token );
            };

    return ExpandTextVars( aSourceText, &symbolResolver );
}


void SCH_PAINTER::draw( const SCH_SYMBOL* aSymbol, int aLayer )
{
    auto t1 = std::chrono::high_resolution_clock::now();
    bool drawingShadows = aLayer == LAYER_SELECTION_SHADOWS;

    std::optional<SCH_SHEET_PATH> optSheetPath;

    wxString variantName;

    if( m_schematic )
    {
        optSheetPath = m_schematic->CurrentSheet();
        variantName = m_schematic->GetCurrentVariant();
        wxLogTrace( traceSchPainter,
                    "SCH_PAINTER::draw symbol %s: Current sheet path='%s', variant='%s', size=%zu, empty=%d",
                    aSymbol->m_Uuid.AsString(),
                    variantName.IsEmpty() ? GetDefaultVariantName() : variantName,
                    optSheetPath->Path().AsString(),
                    optSheetPath->size(),
                    optSheetPath->empty() ? 1 : 0 );
    }

    SCH_SHEET_PATH* sheetPath = optSheetPath ? &optSheetPath.value() : nullptr;
    bool DNP = aSymbol->GetDNP( sheetPath, variantName );
    bool markExclusion = eeconfig()->m_Appearance.mark_sim_exclusions && aSymbol->GetExcludedFromSim( sheetPath,
                                                                                                      variantName );

    if( m_schSettings.IsPrinting() && drawingShadows )
        return;

    if( !drawingShadows || eeconfig()->m_Selection.draw_selected_children )
    {
        for( const SCH_FIELD& field : aSymbol->GetFields() )
            draw( &field, aLayer, DNP );
    }

    if( isFieldsLayer( aLayer ) )
        return;

    if( drawingShadows && !( aSymbol->IsBrightened() || aSymbol->IsSelected() ) )
    {
        // Don't exit here; symbol may still have selected pins
        // return;
    }

    int unit = m_schematic ? aSymbol->GetUnitSelection( &m_schematic->CurrentSheet() ) : 1;
    int bodyStyle = aSymbol->GetBodyStyle();

    // Use dummy symbol if the actual couldn't be found (or couldn't be locked).
    LIB_SYMBOL* originalSymbol =
            aSymbol->GetLibSymbolRef() ? aSymbol->GetLibSymbolRef().get() : LIB_SYMBOL::GetDummy();
    std::vector<SCH_PIN*> originalPins = originalSymbol->GetGraphicalPins( unit, bodyStyle );

    // Copy the source so we can re-orient and translate it.
    auto       tCopy1 = std::chrono::high_resolution_clock::now();
    LIB_SYMBOL tempSymbol( *originalSymbol, nullptr, false );
    auto       tCopy2 = std::chrono::high_resolution_clock::now();

    if( std::chrono::duration_cast<std::chrono::microseconds>( tCopy2 - tCopy1 ).count() > 100 )
    {
        wxLogTrace( traceSchPainter, "SCH_PAINTER::draw symbol copy %s: %lld us", aSymbol->m_Uuid.AsString(),
                    std::chrono::duration_cast<std::chrono::microseconds>( tCopy2 - tCopy1 ).count() );
    }

    std::vector<SCH_PIN*> tempPins = tempSymbol.GetGraphicalPins( unit, bodyStyle );

    tempSymbol.SetFlags( aSymbol->GetFlags() );

    OrientAndMirrorSymbolItems( &tempSymbol, aSymbol->GetOrientation() );

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

        if( !symbolPin )
            continue;

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

    draw( &tempSymbol, aLayer, false, aSymbol->GetUnit(), aSymbol->GetBodyStyle(), DNP );

    for( unsigned i = 0; i < tempPins.size(); ++i )
    {
        SCH_PIN* symbolPin = aSymbol->GetPin( originalPins[ i ] );
        SCH_PIN* tempPin = tempPins[ i ];

        if( !symbolPin )
            continue;

        symbolPin->ClearFlags();
        tempPin->ClearFlags( IS_DANGLING );             // Clear this temporary flag
        symbolPin->SetFlags( tempPin->GetFlags() );     // SELECTED, HIGHLIGHTED, BRIGHTENED,
                                                        // IS_SHOWN_AS_BITMAP
    }

    // Draw DNP and EXCLUDE from SIM markers.
    // These drawings are associated to the symbol body, so draw them only when the LAYER_DEVICE
    // is drawn (to avoid draw artifacts).
    if( DNP && aLayer == LAYER_DEVICE )
    {
        COLOR4D  marker_color = m_schSettings.GetLayerColor( LAYER_DNP_MARKER );
        BOX2I    bbox = aSymbol->GetBodyBoundingBox();
        BOX2I    pins = aSymbol->GetBodyAndPinsBoundingBox();
        VECTOR2D margins( std::max( bbox.GetX() - pins.GetX(), pins.GetEnd().x - bbox.GetEnd().x ),
                          std::max( bbox.GetY() - pins.GetY(),
                                    pins.GetEnd().y - bbox.GetEnd().y ) );
        int      strokeWidth = 3 * schIUScale.MilsToIU( DEFAULT_LINE_WIDTH_MILS );

        margins.x = std::max( margins.x * 0.6, margins.y * 0.3 );
        margins.y = std::max( margins.y * 0.6, margins.x * 0.3 );
        bbox.Inflate( KiROUND( margins.x ), KiROUND( margins.y ) );

        VECTOR2I pt1 = bbox.GetOrigin();
        VECTOR2I pt2 = bbox.GetEnd();

        GAL_SCOPED_ATTRS scopedAttrs( *m_gal, GAL_SCOPED_ATTRS::ALL_ATTRS );
        m_gal->AdvanceDepth();
        m_gal->SetIsStroke( true );
        m_gal->SetIsFill( true );
        m_gal->SetStrokeColor( marker_color );
        m_gal->SetFillColor( marker_color );

        m_gal->DrawSegment( pt1, pt2, strokeWidth );
        std::swap( pt1.x, pt2.x );
        m_gal->DrawSegment( pt1, pt2, strokeWidth );
    }

    if( markExclusion && aLayer == LAYER_DEVICE )
    {
        COLOR4D marker_color = m_schSettings.GetLayerColor( LAYER_EXCLUDED_FROM_SIM );
        BOX2I bbox = aSymbol->GetBodyBoundingBox();
        int   strokeWidth = schIUScale.MilsToIU( ADVANCED_CFG::GetCfg().m_ExcludeFromSimulationLineWidth );

        bbox.Inflate( KiROUND( strokeWidth * 0.5 ) );

        GAL_SCOPED_ATTRS scopedAttrs( *m_gal, GAL_SCOPED_ATTRS::ALL_ATTRS );
        m_gal->AdvanceDepth();
        m_gal->SetIsStroke( true );
        m_gal->SetIsFill( true );
        m_gal->SetStrokeColor( marker_color );
        m_gal->SetFillColor( marker_color );

        m_gal->DrawSegment( bbox.GetPosition(), VECTOR2D( bbox.GetEnd().x, bbox.GetY() ), strokeWidth );
        m_gal->DrawSegment( VECTOR2D( bbox.GetEnd().x, bbox.GetY() ), bbox.GetEnd(), strokeWidth );
        m_gal->DrawSegment( bbox.GetEnd(), VECTOR2D( bbox.GetX(), bbox.GetEnd().y ), strokeWidth );
        m_gal->DrawSegment( VECTOR2D( bbox.GetX(), bbox.GetEnd().y ), bbox.GetPosition(), strokeWidth );

        int offset = 2 * strokeWidth;
        VECTOR2D center = bbox.GetEnd() + VECTOR2D( offset + strokeWidth, -offset );
        VECTOR2D left = center + VECTOR2D( -offset, 0 );
        VECTOR2D right = center + VECTOR2D( offset, 0 );
        VECTOR2D top = center + VECTOR2D( 0, offset );
        VECTOR2D bottom = center + VECTOR2D( 0, -offset );

        m_gal->SetFillColor( marker_color.WithAlpha( 0.1 ) );
        m_gal->DrawCircle( center, offset );
        m_gal->AdvanceDepth();
        m_gal->SetFillColor( marker_color );
        m_gal->DrawCurve( left, top, bottom, right, 1 );
    }

    auto t2 = std::chrono::high_resolution_clock::now();

    if( std::chrono::duration_cast<std::chrono::microseconds>( t2 - t1 ).count() > 100 )
    {
        wxLogTrace( traceSchPainter, "SCH_PAINTER::draw symbol %s: %lld us", aSymbol->m_Uuid.AsString(),
                    std::chrono::duration_cast<std::chrono::microseconds>( t2 - t1 ).count() );
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

    if( aField->IsPrivate() && !m_schSettings.m_IsSymbolEditor )
        return;

    // Must check layer as fields are sometimes drawn by their parent rather than directly
    // from the view.
    std::vector<int> layers = aField->ViewGetLayers();

    if( std::find( layers.begin(), layers.end(), aLayer ) == layers.end() )
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
    m_gal->SetHoverColor( color );

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

        if( aField->IsRollover() && !aField->IsMoving() )
        {
            // Highlight any urls found within the text
            m_gal->SetHoverColor( m_schSettings.GetLayerColor( LAYER_HOVERED ) );

            // Highlight the whole text if it has a link definition
            if( aField->HasHyperlink() )
            {
                attributes.m_Hover = true;
                attributes.m_Underlined = true;
            }
        }

        if( nonCached( aField ) && aField->RenderAsBitmap( m_gal->GetWorldScale() ) )
        {
            bitmapText( *m_gal, shownText, textpos, attributes );
            const_cast<SCH_FIELD*>( aField )->SetFlags( IS_SHOWN_AS_BITMAP );
        }
        else
        {
            std::vector<std::unique_ptr<KIFONT::GLYPH>>* cache = nullptr;

            if( !aField->IsRollover() )
                cache = aField->GetRenderCache( shownText, textpos, attributes );

            if( cache )
            {
                m_gal->SetLineWidth( attributes.m_StrokeWidth );
                m_gal->DrawGlyphs( *cache );
            }
            else
            {
                strokeText( *m_gal, shownText, textpos, attributes, aField->GetFontMetrics(),
                            aField->GetRolloverPos() );
            }

            const_cast<SCH_FIELD*>( aField )->ClearFlags( IS_SHOWN_AS_BITMAP );
        }
    }

    if( aField->GetParent() && aField->GetParent()->Type() == SCH_SYMBOL_T )
    {
        SCH_SYMBOL* parent = static_cast<SCH_SYMBOL*>( aField->GetParent() );
        bool rotated = !orient.IsHorizontal();

        VECTOR2D    pos;
        double      size = bbox.GetHeight() / 1.5;

        if( rotated )
        {
            pos = VECTOR2D( bbox.GetRight() - bbox.GetWidth() / 6.0,
                            bbox.GetBottom() + bbox.GetWidth() / 2.0 );
            size = bbox.GetWidth() / 1.5;
        }
        else
        {
            pos = VECTOR2D( bbox.GetLeft() - bbox.GetHeight() / 2.0,
                            bbox.GetBottom() - bbox.GetHeight() / 6.0 );
        }

        if( parent->IsSymbolLikePowerLocalLabel() && aField->GetId() == FIELD_T::VALUE )
            drawLocalPowerIcon( pos, size, rotated, color, drawingShadows, aField->IsBrightened() );
    }

    // Draw anchor or umbilical line
    if( aField->IsMoving() && m_schematic )
    {
        VECTOR2I parentPos = aField->GetParentPosition();

        m_gal->SetLineWidth( m_schSettings.GetOutlineWidth() );
        m_gal->SetStrokeColor( getRenderColor( aField, LAYER_SCHEMATIC_ANCHOR, drawingShadows ) );
        m_gal->DrawLine( aField->GetPosition(), parentPos );
    }
    else if( aField->IsSelected() )
    {
        drawAnchor( aField->GetPosition(), drawingShadows );
    }
}


void SCH_PAINTER::draw( const SCH_GLOBALLABEL* aLabel, int aLayer, bool aDimmed )
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

    COLOR4D color = getRenderColor( aLabel, LAYER_GLOBLABEL, drawingShadows, aDimmed, true );

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


void SCH_PAINTER::draw( const SCH_LABEL* aLabel, int aLayer, bool aDimmed )
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

    COLOR4D color = getRenderColor( aLabel, LAYER_HIERLABEL, drawingShadows, aDimmed, true );

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


void SCH_PAINTER::draw( const SCH_HIERLABEL* aLabel, int aLayer, bool aDimmed )
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

    COLOR4D color = getRenderColor( aLabel, LAYER_HIERLABEL, drawingShadows, aDimmed, true );

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

    std::vector<VECTOR2I> i_pts;
    std::deque<VECTOR2D>  d_pts;

    aLabel->CreateGraphicShape( &m_schSettings, i_pts, (VECTOR2I)aLabel->GetTextPos() );

    for( const VECTOR2I& i_pt : i_pts )
        d_pts.emplace_back( VECTOR2D( i_pt.x, i_pt.y ) );

    m_gal->SetIsFill( true );
    m_gal->SetFillColor( m_schSettings.GetLayerColor( LAYER_SCHEMATIC_BACKGROUND ) );
    m_gal->SetIsStroke( true );
    m_gal->SetLineWidth( getLineWidth( aLabel, drawingShadows ) );
    m_gal->SetStrokeColor( color );
    m_gal->DrawPolyline( d_pts );

    draw( static_cast<const SCH_TEXT*>( aLabel ), aLayer, false );
}


void SCH_PAINTER::draw( const SCH_DIRECTIVE_LABEL* aLabel, int aLayer, bool aDimmed )
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

    COLOR4D color = getRenderColor( aLabel, LAYER_NETCLASS_REFS, drawingShadows, aDimmed, true );

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
    bool DNP = false;

    if( m_schematic )
        DNP = aSheet->GetDNP( &m_schematic->CurrentSheet(), m_schematic->GetCurrentVariant() );

    bool drawingShadows = aLayer == LAYER_SELECTION_SHADOWS;
    bool markExclusion = eeconfig()->m_Appearance.mark_sim_exclusions && aSheet->GetExcludedFromSim();

    if( m_schSettings.IsPrinting() && drawingShadows )
        return;

    if( !drawingShadows || eeconfig()->m_Selection.draw_selected_children )
    {
        for( const SCH_FIELD& field : aSheet->GetFields() )
            draw( &field, aLayer, DNP );

        for( SCH_SHEET_PIN* sheetPin : aSheet->GetPins() )
            draw( static_cast<SCH_HIERLABEL*>( sheetPin ), aLayer, DNP );
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

    if( DNP && aLayer == LAYER_SHEET )
    {
        int      layer = LAYER_DNP_MARKER;
        BOX2I    bbox = aSheet->GetBodyBoundingBox();
        BOX2I    pins = aSheet->GetBoundingBox();
        VECTOR2D margins( std::max( bbox.GetX() - pins.GetX(), pins.GetEnd().x - bbox.GetEnd().x ),
                          std::max( bbox.GetY() - pins.GetY(), pins.GetEnd().y - bbox.GetEnd().y ) );
        int      strokeWidth = 3 * schIUScale.MilsToIU( DEFAULT_LINE_WIDTH_MILS );

        margins.x = std::max( margins.x * 0.6, margins.y * 0.3 );
        margins.y = std::max( margins.y * 0.6, margins.x * 0.3 );
        bbox.Inflate( KiROUND( margins.x ), KiROUND( margins.y ) );

        VECTOR2I pt1 = bbox.GetOrigin();
        VECTOR2I pt2 = bbox.GetEnd();

        GAL_SCOPED_ATTRS scopedAttrs( *m_gal, GAL_SCOPED_ATTRS::ALL_ATTRS );
        m_gal->AdvanceDepth();
        m_gal->SetIsStroke( true );
        m_gal->SetIsFill( true );
        m_gal->SetStrokeColor( m_schSettings.GetLayerColor( layer ) );
        m_gal->SetFillColor( m_schSettings.GetLayerColor( layer ) );

        m_gal->DrawSegment( pt1, pt2, strokeWidth );
        std::swap( pt1.x, pt2.x );
        m_gal->DrawSegment( pt1, pt2, strokeWidth );
    }

    if( markExclusion )
    {
        int   layer = LAYER_EXCLUDED_FROM_SIM;
        BOX2I bbox = aSheet->GetBodyBoundingBox();
        int   strokeWidth = schIUScale.MilsToIU( ADVANCED_CFG::GetCfg().m_ExcludeFromSimulationLineWidth );

        bbox.Inflate( KiROUND( strokeWidth * 0.5 ) );

        GAL_SCOPED_ATTRS scopedAttrs( *m_gal, GAL_SCOPED_ATTRS::ALL_ATTRS );
        m_gal->AdvanceDepth();
        m_gal->SetIsStroke( true );
        m_gal->SetIsFill( true );
        m_gal->SetStrokeColor( m_schSettings.GetLayerColor( layer ).WithAlpha( 0.5 ) );
        m_gal->SetFillColor( m_schSettings.GetLayerColor( layer ).WithAlpha( 0.5 ) );

        m_gal->DrawSegment( bbox.GetPosition(), VECTOR2D( bbox.GetEnd().x, bbox.GetY() ), strokeWidth );
        m_gal->DrawSegment( VECTOR2D( bbox.GetEnd().x, bbox.GetY() ), bbox.GetEnd(), strokeWidth );
        m_gal->DrawSegment( bbox.GetEnd(), VECTOR2D( bbox.GetX(), bbox.GetEnd().y ), strokeWidth );
        m_gal->DrawSegment( VECTOR2D( bbox.GetX(), bbox.GetEnd().y ), bbox.GetPosition(), strokeWidth );

        int offset = 2 * strokeWidth;
        VECTOR2D center = bbox.GetEnd() + VECTOR2D( offset + strokeWidth, -offset );
        VECTOR2D left = center + VECTOR2D( -offset, 0 );
        VECTOR2D right = center + VECTOR2D( offset, 0 );
        VECTOR2D top = center + VECTOR2D( 0, offset );
        VECTOR2D bottom = center + VECTOR2D( 0, -offset );

        m_gal->SetFillColor( m_schSettings.GetLayerColor( layer ).WithAlpha( 0.1 ) );
        m_gal->DrawCircle( center, offset );
        m_gal->AdvanceDepth();
        m_gal->SetFillColor( m_schSettings.GetLayerColor( layer ).WithAlpha( 0.5 ) );
        m_gal->DrawCurve( left, top, bottom, right, 1 );
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
    bool         drawingNetColorHighlights = aLayer == LAYER_NET_COLOR_HIGHLIGHT;
    bool         drawingDangling = aLayer == LAYER_DANGLING;
    bool         drawingWires = aLayer == LAYER_WIRE;
    bool         drawingBusses = aLayer == LAYER_BUS;

    if( m_schSettings.IsPrinting() && drawingShadows )
        return;

    bool highlightNetclassColors = false;
    EESCHEMA_SETTINGS* eeschemaCfg = eeconfig();

    if( eeschemaCfg )
    {
        highlightNetclassColors = eeschemaCfg->m_Selection.highlight_netclass_colors;
    }

    if( !highlightNetclassColors && drawingNetColorHighlights )
        return;

    if( m_schSettings.m_OverrideItemColors && drawingNetColorHighlights )
        return;

    if( drawingShadows && !( aEntry->IsBrightened() || aEntry->IsSelected() ) )
        return;

    if( aEntry->IsSelected() )
    {
        line.SetSelected();

        // Never show unselected endpoints on bus entries
        line.SetFlags( STARTPOINT | ENDPOINT );
    }
    else if( aEntry->IsBrightened() )
    {
        line.SetBrightened();
    }

    line.SetStartPoint( aEntry->GetPosition() );
    line.SetEndPoint( aEntry->GetEnd() );
    line.SetStroke( aEntry->GetStroke() );
    line.SetLineWidth( KiROUND( getLineWidth( aEntry, false ) ) );

    COLOR4D color = getRenderColor( aEntry, LAYER_WIRE, drawingShadows );

    if( aEntry->Type() == SCH_BUS_BUS_ENTRY_T )
        color = getRenderColor( aEntry, LAYER_BUS, drawingShadows );

    if( highlightNetclassColors )
    {
        // Force default color for nets we are going to highlight
        if( drawingWires )
            color = m_schSettings.GetLayerColor( LAYER_WIRE );
        else if( drawingBusses )
            color = m_schSettings.GetLayerColor( LAYER_BUS );
    }

    if( drawingNetColorHighlights )
    {
        // Don't draw highlights for default-colored nets
        if( ( aEntry->Type() == SCH_BUS_WIRE_ENTRY_T
              && color == m_schSettings.GetLayerColor( LAYER_WIRE ) )
            || ( aEntry->Type() == SCH_BUS_BUS_ENTRY_T
                 && color == m_schSettings.GetLayerColor( LAYER_BUS ) ) )
        {
            return;
        }
    }

    if( drawingDangling )
    {
        m_gal->SetIsFill( false );
        m_gal->SetIsStroke( true );
        m_gal->SetStrokeColor( color.Brightened( 0.3 ) );
        m_gal->SetLineWidth( m_schSettings.GetDanglingIndicatorThickness() );

        if( aEntry->IsStartDangling() )
        {
            m_gal->DrawCircle( aEntry->GetPosition(),
                               aEntry->GetPenWidth() + KiROUND( TARGET_BUSENTRY_RADIUS / 2.0 ) );
        }

        if( aEntry->IsEndDangling() )
        {
            m_gal->DrawCircle( aEntry->GetEnd(),
                               aEntry->GetPenWidth() + KiROUND( TARGET_BUSENTRY_RADIUS / 2.0 ) );
        }
    }
    else
    {
        line.SetLineColor( color );
        line.SetLineStyle( aEntry->GetEffectiveLineStyle() );

        draw( &line, aLayer );
    }
}


void SCH_PAINTER::draw( const SCH_BITMAP* aBitmap, int aLayer )
{
    auto t1 = std::chrono::high_resolution_clock::now();
    m_gal->Save();
    m_gal->Translate( aBitmap->GetPosition() );

    const REFERENCE_IMAGE& refImage = aBitmap->GetReferenceImage();

    // When the image scale factor is not 1.0, we need to modify the actual as the image scale
    // factor is similar to a local zoom
    const double img_scale = refImage.GetImageScale();

    if( img_scale != 1.0 )
        m_gal->Scale( VECTOR2D( img_scale, img_scale ) );

    if( aLayer == LAYER_DRAW_BITMAPS )
    {
        m_gal->DrawBitmap( refImage.GetImage() );
    }

    if( aLayer == LAYER_SELECTION_SHADOWS )
    {
        if( aBitmap->IsSelected() || aBitmap->IsBrightened() )
        {
            const COLOR4D color = getRenderColor( aBitmap, LAYER_DRAW_BITMAPS, true );
            m_gal->SetIsStroke( true );
            m_gal->SetStrokeColor( color );
            m_gal->SetLineWidth ( getShadowWidth( aBitmap->IsBrightened() ) );
            m_gal->SetIsFill( false );

            // Draws a bounding box.
            VECTOR2D bm_size( refImage.GetSize() );

            // bm_size is the actual image size in UI.
            // but m_canvas scale was previously set to img_scale
            // so recalculate size relative to this image size.
            bm_size.x /= img_scale;
            bm_size.y /= img_scale;
            const VECTOR2D origin( -bm_size.x / 2.0, -bm_size.y / 2.0 );
            const VECTOR2D end = origin + bm_size;

            m_gal->DrawRectangle( origin, end );
        }
    }

    m_gal->Restore();
    auto t2 = std::chrono::high_resolution_clock::now();

    if( std::chrono::duration_cast<std::chrono::microseconds>( t2 - t1 ).count() > 100 )
    {
        wxLogTrace( traceSchPainter, "SCH_PAINTER::draw bitmap %s: %lld us", aBitmap->m_Uuid.AsString(),
                    std::chrono::duration_cast<std::chrono::microseconds>( t2 - t1 ).count() );
    }
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


void SCH_PAINTER::draw( const SCH_GROUP* aGroup, int aLayer )
{
    const bool drawingShadows = false;

    if( aLayer == LAYER_SCHEMATIC_ANCHOR )
    {
        if( aGroup->IsSelected() && !( aGroup->GetParent() && aGroup->GetParent()->IsSelected() ) )
        {
            // Selected on our own; draw enclosing box
        }
        else if( aGroup->IsEntered() )
        {
            // Entered group; draw enclosing box
        }
        else
        {
            // Neither selected nor entered; draw nothing at the group level (ie: only draw
            // its members)
            return;
        }

        const COLOR4D color = getRenderColor( aGroup, LAYER_SCHEMATIC_ANCHOR, drawingShadows );

        m_gal->SetStrokeColor( color );
        m_gal->SetLineWidth( m_schSettings.GetOutlineWidth() * 2.0f );

        BOX2I    bbox = aGroup->GetBoundingBox();
        VECTOR2I topLeft = bbox.GetPosition();
        VECTOR2I width = VECTOR2I( bbox.GetWidth(), 0 );
        VECTOR2I height = VECTOR2I( 0, bbox.GetHeight() );

        m_gal->DrawLine( topLeft, topLeft + width );
        m_gal->DrawLine( topLeft + width, topLeft + width + height );
        m_gal->DrawLine( topLeft + width + height, topLeft + height );
        m_gal->DrawLine( topLeft + height, topLeft );

        wxString name = aGroup->GetName();

        if( name.IsEmpty() )
            return;

        int ptSize = 12;
        int scaledSize = abs( KiROUND( m_gal->GetScreenWorldMatrix().GetScale().x * ptSize ) );
        int unscaledSize = schIUScale.MilsToIU( ptSize );

        // Scale by zoom a bit, but not too much
        int      textSize = ( scaledSize + ( unscaledSize * 2 ) ) / 3;
        VECTOR2I textOffset = KiROUND( width.x / 2.0, -textSize * 0.5 );
        VECTOR2I titleHeight = KiROUND( 0.0, textSize * 2.0 );

        if( PrintableCharCount( name ) * textSize < bbox.GetWidth() )
        {
            m_gal->DrawLine( topLeft, topLeft - titleHeight );
            m_gal->DrawLine( topLeft - titleHeight, topLeft + width - titleHeight );
            m_gal->DrawLine( topLeft + width - titleHeight, topLeft + width );

            TEXT_ATTRIBUTES attrs;
            attrs.m_Italic = true;
            attrs.m_Halign = GR_TEXT_H_ALIGN_CENTER;
            attrs.m_Valign = GR_TEXT_V_ALIGN_BOTTOM;
            attrs.m_Size = VECTOR2I( textSize, textSize );
            attrs.m_StrokeWidth = GetPenSizeForNormal( textSize );

            KIFONT::FONT::GetFont()->Draw( m_gal, aGroup->GetName(), topLeft + textOffset, attrs,
                                           aGroup->GetFontMetrics() );
        }
    }
}


void SCH_PAINTER::drawLine( const VECTOR2I& aStartPoint, const VECTOR2I& aEndPoint,
                            LINE_STYLE aLineStyle, bool aDrawDirectLine, int aWidth )
{
    if( aDrawDirectLine )
    {
        m_gal->DrawLine( aStartPoint, aEndPoint );
    }
    else
    {
        SHAPE_SEGMENT segment( aStartPoint, aEndPoint );

        STROKE_PARAMS::Stroke( &segment, aLineStyle, KiROUND( aWidth ), &m_schSettings,
                               [&]( const VECTOR2I& start, const VECTOR2I& end )
                               {
                                   if( start == end )
                                       m_gal->DrawLine( start + 1, end );
                                   else
                                       m_gal->DrawLine( start, end );
                               } );
    }
}

}; // namespace KIGFX
