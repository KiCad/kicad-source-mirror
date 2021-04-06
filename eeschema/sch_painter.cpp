/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2014 CERN
 * Copyright (C) 2019-2021 KiCad Developers, see AUTHORS.txt for contributors.
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


#include <sch_item.h>

#include <bezier_curves.h>
#include <class_library.h>
#include <connection_graph.h>
#include <gal/graphics_abstraction_layer.h>
#include <geometry/geometry_utils.h>
#include <geometry/shape_line_chain.h>
#include <gr_text.h>
#include <lib_arc.h>
#include <lib_bezier.h>
#include <lib_circle.h>
#include <lib_field.h>
#include <lib_item.h>
#include <lib_pin.h>
#include <lib_polyline.h>
#include <lib_rectangle.h>
#include <lib_text.h>
#include <math/util.h>
#include <plotter.h>
#include <sch_bitmap.h>
#include <sch_bus_entry.h>
#include <sch_symbol.h>
#include <sch_edit_frame.h>
#include <sch_field.h>
#include <sch_junction.h>
#include <sch_line.h>
#include <sch_marker.h>
#include <sch_no_connect.h>
#include <sch_sheet.h>
#include <sch_sheet_pin.h>
#include <sch_text.h>
#include <schematic.h>
#include <settings/color_settings.h>
#include <view/view.h>
#include <kiface_i.h>
#include <default_values.h>
#include <advanced_config.h>
#include "sch_painter.h"

namespace KIGFX
{

SCH_RENDER_SETTINGS::SCH_RENDER_SETTINGS() :
        m_ShowUnit( 0 ),
        m_ShowConvert( 0 ),
        m_ShowHiddenText( true ),
        m_ShowHiddenPins( true ),
        m_ShowPinsElectricalType( true ),
        m_ShowDisabled( false ),
        m_ShowGraphicsDisabled( false ),
        m_ShowUmbilicals( true ),
        m_OverrideItemColors( false ),
        m_TextOffsetRatio( 0.08 ),
        m_DefaultWireThickness( DEFAULT_WIRE_THICKNESS * IU_PER_MILS ),
        m_DefaultBusThickness( DEFAULT_BUS_THICKNESS * IU_PER_MILS ),
        m_PinSymbolSize( DEFAULT_TEXT_SIZE * IU_PER_MILS / 2 ),
        m_JunctionSize( DEFAULT_JUNCTION_DIAM * IU_PER_MILS )
{
    SetDefaultPenWidth( DEFAULT_LINE_THICKNESS * IU_PER_MILS );

    m_minPenWidth = ADVANCED_CFG::GetCfg().m_MinPlotPenWidth * IU_PER_MM;
}


void SCH_RENDER_SETTINGS::LoadColors( const COLOR_SETTINGS* aSettings )
{
    for( int layer = SCH_LAYER_ID_START; layer < SCH_LAYER_ID_END; layer ++)
        m_layerColors[ layer ] = aSettings->GetColor( layer );

    for( int layer = GAL_LAYER_ID_START; layer < GAL_LAYER_ID_END; layer ++)
        m_layerColors[ layer ] = aSettings->GetColor( layer );

    m_backgroundColor = aSettings->GetColor( LAYER_SCHEMATIC_BACKGROUND );

    m_layerColors[LAYER_AUX_ITEMS] = m_layerColors[LAYER_SCHEMATIC_AUX_ITEMS];

    m_OverrideItemColors = aSettings->GetOverrideSchItemColors();
}


COLOR4D SCH_RENDER_SETTINGS::GetColor( const VIEW_ITEM* aItem, int aLayer ) const
{
    return m_layerColors[ aLayer ];
}


EESCHEMA_SETTINGS* eeconfig()
{
    return dynamic_cast<EESCHEMA_SETTINGS*>( Kiface().KifaceSettings() );
}


/**
 * Used when a LIB_PART is not found in library to draw a dummy shape.
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
static LIB_PART* dummy()
{
    static LIB_PART* part;

    if( !part )
    {
        part = new LIB_PART( wxEmptyString );

        LIB_RECTANGLE* square = new LIB_RECTANGLE( part );

        square->MoveTo( wxPoint( Mils2iu( -200 ), Mils2iu( 200 ) ) );
        square->SetEndPosition( wxPoint( Mils2iu( 200 ), Mils2iu( -200 ) ) );

        LIB_TEXT* text = new LIB_TEXT( part );

        text->SetTextSize( wxSize( Mils2iu( 150 ), Mils2iu( 150 ) ) );
        text->SetText( wxString( wxT( "??" ) ) );

        part->AddDrawItem( square );
        part->AddDrawItem( text );
    }

    return part;
}


SCH_PAINTER::SCH_PAINTER( GAL* aGal ) :
    KIGFX::PAINTER( aGal ),
    m_schematic( nullptr )
{ }


#define HANDLE_ITEM( type_id, type_name ) \
    case type_id: draw( (type_name *) item, aLayer ); break


bool SCH_PAINTER::Draw( const VIEW_ITEM *aItem, int aLayer )
{
    const auto item = dynamic_cast<const EDA_ITEM*>( aItem );

    if( !item )
        return false;

#ifdef CONNECTIVITY_DEBUG

    auto sch_item = dynamic_cast<const SCH_ITEM*>( item );
    auto conn = sch_item ? sch_item->Connection( *g_CurrentSheet ) : nullptr;

    if( conn )
    {
        auto pos = item->GetBoundingBox().Centre();
        auto label = conn->Name( true );

        m_gal->SetHorizontalJustify( GR_TEXT_HJUSTIFY_CENTER );
        m_gal->SetVerticalJustify( GR_TEXT_VJUSTIFY_CENTER );
        m_gal->SetStrokeColor( COLOR4D( LIGHTRED ) );
        m_gal->SetLineWidth( Mils2ui( 2 ) );
        m_gal->SetGlyphSize( VECTOR2D( Mils2ui( 20 ), Mils2ui( 20 ) ) );
        m_gal->StrokeText( conn->Name( true ), pos, 0.0, 0 );
    }

#endif

    if( ADVANCED_CFG::GetCfg().m_DrawBoundingBoxes )
    {
        BOX2I box = item->GetBoundingBox();

        if( item->Type() == SCH_COMPONENT_T )
            box = static_cast<const SCH_COMPONENT*>( item )->GetBodyBoundingBox();

        m_gal->SetIsFill( false );
        m_gal->SetIsStroke( true );
        m_gal->SetStrokeColor( item->IsSelected() ? COLOR4D( 1.0, 0.2, 0.2, 1 ) :
                               COLOR4D( 0.2, 0.2, 0.2, 1 ) );
        m_gal->SetLineWidth( Mils2iu( 3 ) );
        m_gal->DrawRectangle( box.GetOrigin(), box.GetEnd() );
    }

    switch( item->Type() )
    {
    HANDLE_ITEM( LIB_PART_T, LIB_PART );
    HANDLE_ITEM( LIB_RECTANGLE_T, LIB_RECTANGLE );
    HANDLE_ITEM( LIB_POLYLINE_T, LIB_POLYLINE );
    HANDLE_ITEM( LIB_CIRCLE_T, LIB_CIRCLE );
    HANDLE_ITEM( LIB_PIN_T, LIB_PIN );
    HANDLE_ITEM( LIB_ARC_T, LIB_ARC );
    HANDLE_ITEM( LIB_FIELD_T, LIB_FIELD );
    HANDLE_ITEM( LIB_TEXT_T, LIB_TEXT );
    HANDLE_ITEM( LIB_BEZIER_T, LIB_BEZIER );
    HANDLE_ITEM( SCH_COMPONENT_T, SCH_COMPONENT );
    HANDLE_ITEM( SCH_JUNCTION_T, SCH_JUNCTION );
    HANDLE_ITEM( SCH_LINE_T, SCH_LINE );
    HANDLE_ITEM( SCH_TEXT_T, SCH_TEXT );
    HANDLE_ITEM( SCH_LABEL_T, SCH_TEXT );
    HANDLE_ITEM( SCH_FIELD_T, SCH_FIELD );
    HANDLE_ITEM( SCH_HIER_LABEL_T, SCH_HIERLABEL );
    HANDLE_ITEM( SCH_GLOBAL_LABEL_T, SCH_GLOBALLABEL );
    HANDLE_ITEM( SCH_SHEET_T, SCH_SHEET );
    HANDLE_ITEM( SCH_SHEET_PIN_T, SCH_HIERLABEL );
    HANDLE_ITEM( SCH_NO_CONNECT_T, SCH_NO_CONNECT );
    HANDLE_ITEM( SCH_BUS_WIRE_ENTRY_T, SCH_BUS_ENTRY_BASE );
    HANDLE_ITEM( SCH_BUS_BUS_ENTRY_T, SCH_BUS_ENTRY_BASE );
    HANDLE_ITEM( SCH_BITMAP_T, SCH_BITMAP );
    HANDLE_ITEM( SCH_MARKER_T, SCH_MARKER );

    default: return false;
    }

    return false;
}


bool SCH_PAINTER::isUnitAndConversionShown( const LIB_ITEM* aItem ) const
{
    if( m_schSettings.m_ShowUnit            // showing a specific unit
            && aItem->GetUnit()             // item is unit-specific
            && aItem->GetUnit() != m_schSettings.m_ShowUnit )
    {
        return false;
    }

    if( m_schSettings.m_ShowConvert         // showing a specific conversion
            && aItem->GetConvert()          // item is conversion-specific
            && aItem->GetConvert() != m_schSettings.m_ShowConvert )
    {
        return false;
    }

    return true;
}


float SCH_PAINTER::getShadowWidth() const
{
    const MATRIX3x3D& matrix = m_gal->GetScreenWorldMatrix();

    // For best visuals the selection width must be a cross between the zoom level and the
    // default line width.
    return (float) std::fabs( matrix.GetScale().x * 2.75 ) + Mils2iu( eeconfig()->m_Selection.thickness );
}


COLOR4D SCH_PAINTER::getRenderColor( const EDA_ITEM* aItem, int aLayer, bool aDrawingShadows ) const
{
    COLOR4D color = m_schSettings.GetLayerColor( aLayer );

    if( aItem->Type() == SCH_LINE_T )
    {
        COLOR4D lineColor = static_cast<const SCH_LINE*>( aItem )->GetLineColor();

        if( lineColor != COLOR4D::UNSPECIFIED )
            color = lineColor;
    }
    else if( aItem->Type() == SCH_BUS_WIRE_ENTRY_T )
    {
        COLOR4D busEntryColor = static_cast<const SCH_BUS_WIRE_ENTRY*>( aItem )->GetStrokeColor();

        if( busEntryColor != COLOR4D::UNSPECIFIED )
            color = busEntryColor;
    }
    else if( aItem->Type() == SCH_JUNCTION_T )
    {
        COLOR4D junctionColor = static_cast<const SCH_JUNCTION*>( aItem )->GetJunctionColor();

        if( junctionColor != COLOR4D::UNSPECIFIED )
            color = junctionColor;
    }
    else if( aItem->Type() == SCH_SHEET_T )
    {
        SCH_SHEET* sheet = (SCH_SHEET*) aItem;

        if( m_schSettings.m_OverrideItemColors )
            color = m_schSettings.GetLayerColor( aLayer );
        else if( aLayer == LAYER_SHEET )
            color = sheet->GetBorderColor();
        else if( aLayer == LAYER_SHEET_BACKGROUND )
            color = sheet->GetBackgroundColor();

        if( color == COLOR4D::UNSPECIFIED )
            color = m_schSettings.GetLayerColor( aLayer );
    }

    if( aItem->IsBrightened() && !aDrawingShadows ) // Selection disambiguation, etc.
    {
        color = m_schSettings.GetLayerColor( LAYER_BRIGHTENED );

        if( aLayer == LAYER_DEVICE_BACKGROUND || aLayer == LAYER_SHEET_BACKGROUND )
            color = color.WithAlpha( 0.2 );
    }
    else if( aItem->IsSelected() )
    {
        if( aDrawingShadows )
            color = m_schSettings.GetLayerColor( LAYER_SELECTION_SHADOWS );
    }

    if( m_schSettings.m_ShowDisabled
            || ( m_schSettings.m_ShowGraphicsDisabled && aItem->Type() != LIB_FIELD_T ) )
    {
        color = color.Darken( 0.5f );
    }

    return color;
}


float SCH_PAINTER::getLineWidth( const LIB_ITEM* aItem, bool aDrawingShadows ) const
{
    float width = (float) std::max( aItem->GetPenWidth(), m_schSettings.GetDefaultPenWidth() );

    if( aItem->IsSelected() && aDrawingShadows )
        width += getShadowWidth();

    return width;
}


float SCH_PAINTER::getLineWidth( const SCH_ITEM* aItem, bool aDrawingShadows ) const
{
    wxCHECK( aItem, static_cast<float>( m_schSettings.m_DefaultWireThickness ) );

    float width = (float) aItem->GetPenWidth();

    if( aItem->IsSelected() && aDrawingShadows )
        width += getShadowWidth();

    return std::max( width, 1.0f );
}


float SCH_PAINTER::getTextThickness( const SCH_TEXT* aItem, bool aDrawingShadows ) const
{
    float width = (float) aItem->GetEffectiveTextPenWidth( m_schSettings.GetDefaultPenWidth() );

    if( aItem->IsSelected() && aDrawingShadows )
        width += getShadowWidth();

    return width;
}


float SCH_PAINTER::getTextThickness( const SCH_FIELD* aItem, bool aDrawingShadows ) const
{
    float width = (float) aItem->GetEffectiveTextPenWidth( m_schSettings.GetDefaultPenWidth() );

    if( aItem->IsSelected() && aDrawingShadows )
        width += getShadowWidth();

    return width;
}


float SCH_PAINTER::getTextThickness( const LIB_FIELD* aItem, bool aDrawingShadows ) const
{
    float width = (float) std::max( aItem->GetEffectiveTextPenWidth(),
                                    m_schSettings.GetDefaultPenWidth() );

    if( aItem->IsSelected() && aDrawingShadows )
        width += getShadowWidth();

    return width;
}


float SCH_PAINTER::getTextThickness( const LIB_TEXT* aItem, bool aDrawingShadows ) const
{
    float width = (float) std::max( aItem->GetEffectiveTextPenWidth(),
                                    m_schSettings.GetDefaultPenWidth() );

    if( aItem->IsSelected() && aDrawingShadows )
        width += getShadowWidth();

    return width;
}


void SCH_PAINTER::strokeText( const wxString& aText, const VECTOR2D& aPosition, double aAngle )
{
    m_gal->StrokeText( aText, aPosition, aAngle );
}


void SCH_PAINTER::draw( const LIB_PART *aPart, int aLayer, bool aDrawFields, int aUnit, int aConvert )
{
    if( !aUnit )
        aUnit = m_schSettings.m_ShowUnit;

    if( !aConvert )
        aConvert = m_schSettings.m_ShowConvert;

    std::unique_ptr< LIB_PART > tmpPart;
    const LIB_PART* drawnPart = aPart;

    if( aPart->IsAlias() )
    {
        tmpPart = aPart->Flatten();
        drawnPart = tmpPart.get();
    }

    for( const LIB_ITEM& item : drawnPart->GetDrawItems() )
    {
        if( !aDrawFields && item.Type() == LIB_FIELD_T )
            continue;

        if( aUnit && item.GetUnit() && aUnit != item.GetUnit() )
            continue;

        if( aConvert && item.GetConvert() && aConvert != item.GetConvert() )
            continue;

        Draw( &item, aLayer );
    }
}


static VECTOR2D mapCoords( const wxPoint& aCoord )
{
    return VECTOR2D( aCoord.x, -aCoord.y );
}


void SCH_PAINTER::triLine( const VECTOR2D &a, const VECTOR2D &b, const VECTOR2D &c )
{
    m_gal->DrawLine( a, b );
    m_gal->DrawLine( b, c );
}


bool SCH_PAINTER::setDeviceColors( const LIB_ITEM* aItem, int aLayer )
{
    switch( aLayer )
    {
    case LAYER_SELECTION_SHADOWS:
        if( aItem->IsSelected() )
        {
            m_gal->SetIsFill( false );
            m_gal->SetIsStroke( true );
            m_gal->SetLineWidth( getLineWidth( aItem, true ) );
            m_gal->SetStrokeColor( getRenderColor( aItem, LAYER_DEVICE, true ) );
            m_gal->SetFillColor( getRenderColor( aItem, LAYER_DEVICE, true ) );
            return true;
        }

        return false;

    case LAYER_DEVICE_BACKGROUND:
        if( aItem->GetFillMode() == FILL_TYPE::FILLED_WITH_BG_BODYCOLOR )
        {
            COLOR4D fillColor = getRenderColor( aItem, LAYER_DEVICE_BACKGROUND, false );

            m_gal->SetIsFill( aItem->GetFillMode() == FILL_TYPE::FILLED_WITH_BG_BODYCOLOR );
            m_gal->SetFillColor( fillColor );
            m_gal->SetIsStroke( false );
            return true;
        }

        return false;

    case LAYER_DEVICE:
        m_gal->SetIsFill( aItem->GetFillMode() == FILL_TYPE::FILLED_SHAPE );
        m_gal->SetFillColor( getRenderColor( aItem, LAYER_DEVICE, false ) );

        if( aItem->GetPenWidth() > 0 || aItem->GetFillMode() == FILL_TYPE::NO_FILL )
        {
            m_gal->SetIsStroke( true );
            m_gal->SetLineWidth( getLineWidth( aItem, false ) );
            m_gal->SetStrokeColor( getRenderColor( aItem, LAYER_DEVICE, false ) );
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


void SCH_PAINTER::fillIfSelection( int aLayer )
{
    if( aLayer == LAYER_SELECTION_SHADOWS && eeconfig()->m_Selection.fill_shapes )
        m_gal->SetIsFill( true );
}


void SCH_PAINTER::draw( const LIB_RECTANGLE *aRect, int aLayer )
{
    if( !isUnitAndConversionShown( aRect ) )
        return;

    if( setDeviceColors( aRect, aLayer ) )
    {
        fillIfSelection( aLayer );
        m_gal->DrawRectangle( mapCoords( aRect->GetPosition() ), mapCoords( aRect->GetEnd() ) );
    }
}


void SCH_PAINTER::draw( const LIB_CIRCLE *aCircle, int aLayer )
{
    if( !isUnitAndConversionShown( aCircle ) )
        return;

    if( setDeviceColors( aCircle, aLayer ) )
    {
        fillIfSelection( aLayer );
        m_gal->DrawCircle( mapCoords( aCircle->GetPosition() ), aCircle->GetRadius() );
    }
}


void SCH_PAINTER::draw( const LIB_ARC *aArc, int aLayer )
{
    if( !isUnitAndConversionShown( aArc ) )
        return;

    if( setDeviceColors( aArc, aLayer ) )
    {
        int sai = aArc->GetFirstRadiusAngle();
        int eai = aArc->GetSecondRadiusAngle();

        /**
         * This accounts for an oddity in the old library format, where the symbol
         * is overdefined.  The previous draw (based on wxwidgets) used start point and end
         * point and always drew counter-clockwise.  The new GAL draw takes center, radius and
         * start/end angles.  All of these points were stored in the file, so we need to mimic the
         * swapping of start/end points rather than using the stored angles in order to properly map
         * edge cases.
         *
         * todo(v6): Remove this hack when we update the file format and do translation on loading.
         */
        if( !TRANSFORM().MapAngles( &sai, &eai ) )
        {
            LIB_ARC new_arc( *aArc );

            new_arc.SetStart( aArc->GetEnd() );
            new_arc.SetEnd( aArc->GetStart() );
            new_arc.CalcRadiusAngles();
            sai = new_arc.GetFirstRadiusAngle();
            eai = new_arc.GetSecondRadiusAngle();
            TRANSFORM().MapAngles( &sai, &eai );
        }

        double sa = (double) sai * M_PI / 1800.0;
        double ea = (double) eai * M_PI / 1800.0 ;

        VECTOR2D pos = mapCoords( aArc->GetPosition() );

        m_gal->DrawArc( pos, aArc->GetRadius(), sa, ea );
    }
}


void SCH_PAINTER::draw( const LIB_POLYLINE *aLine, int aLayer )
{
    if( !isUnitAndConversionShown( aLine ) )
        return;

    if( setDeviceColors( aLine, aLayer ) )
    {
        const std::vector<wxPoint>& pts = aLine->GetPolyPoints();
        std::deque<VECTOR2D> vtx;

        for( auto p : pts )
            vtx.push_back( mapCoords( p ) );

        fillIfSelection( aLayer );
        m_gal->DrawPolygon( vtx );
    }
}


void SCH_PAINTER::draw( const LIB_FIELD *aField, int aLayer )
{
    bool drawingShadows = aLayer == LAYER_SELECTION_SHADOWS;

    if( drawingShadows && !aField->IsSelected() )
        return;

    if( !isUnitAndConversionShown( aField ) )
        return;

    // Must check layer as fields are sometimes drawn by their parent rather than
    // directly from the view.
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

    COLOR4D color = getRenderColor( aField, aLayer, drawingShadows );

    if( !( aField->IsVisible() || aField->IsForceVisible() ) )
    {
        if( m_schSettings.m_ShowHiddenText )
            color = getRenderColor( aField, LAYER_HIDDEN, drawingShadows );
        else
            return;
    }

    m_gal->SetLineWidth( getTextThickness( aField, drawingShadows ) );
    m_gal->SetIsFill( false );
    m_gal->SetIsStroke( true );
    m_gal->SetStrokeColor( color );

    auto pos = mapCoords( aField->GetPosition() );

    if( drawingShadows && eeconfig()->m_Selection.text_as_box )
    {
        EDA_RECT boundaryBox = aField->GetBoundingBox();

        m_gal->SetIsFill( true );
        m_gal->SetFillColor( color );
        m_gal->SetLineWidth( m_gal->GetLineWidth() * 0.5 );
        boundaryBox.RevertYAxis();

        m_gal->DrawRectangle( mapCoords( boundaryBox.GetPosition() ),
                              mapCoords( boundaryBox.GetEnd() ) );
    }
    else
    {
        m_gal->SetGlyphSize( VECTOR2D( aField->GetTextSize() ) );
        m_gal->SetFontItalic( aField->IsItalic() );

        m_gal->SetHorizontalJustify( aField->GetHorizJustify() );
        m_gal->SetVerticalJustify( aField->GetVertJustify() );

        double orient = aField->GetTextAngleRadians();

        strokeText( aField->GetText(), pos, orient );
    }

    // Draw the umbilical line
    if( aField->IsMoving() && m_schSettings.m_ShowUmbilicals )
    {
        m_gal->SetLineWidth( m_schSettings.m_outlineWidth );
        m_gal->SetStrokeColor( COLOR4D( 0.0, 0.0, 1.0, 1.0 ) );
        m_gal->DrawLine( pos, wxPoint( 0, 0 ) );
    }
}


void SCH_PAINTER::draw( const LIB_TEXT *aText, int aLayer )
{
    if( !isUnitAndConversionShown( aText ) )
        return;

    bool drawingShadows = aLayer == LAYER_SELECTION_SHADOWS;

    if( drawingShadows && !aText->IsSelected() )
        return;

    COLOR4D color = getRenderColor( aText, LAYER_DEVICE, drawingShadows );

    if( !aText->IsVisible() )
    {
        if( m_schSettings.m_ShowHiddenText )
            color = getRenderColor( aText, LAYER_HIDDEN, drawingShadows );
        else
            return;
    }

    EDA_RECT bBox = aText->GetBoundingBox();
    bBox.RevertYAxis();
    VECTOR2D pos = mapCoords( bBox.Centre() );
    double orient = aText->GetTextAngleRadians();

    m_gal->SetHorizontalJustify( GR_TEXT_HJUSTIFY_CENTER );
    m_gal->SetVerticalJustify( GR_TEXT_VJUSTIFY_CENTER );
    m_gal->SetLineWidth( getTextThickness( aText, drawingShadows ) );
    m_gal->SetIsFill( false );
    m_gal->SetIsStroke( true );
    m_gal->SetStrokeColor( color );
    m_gal->SetGlyphSize( VECTOR2D( aText->GetTextSize() ) );
    m_gal->SetFontBold( aText->IsBold() );
    m_gal->SetFontItalic( aText->IsItalic() );
    m_gal->SetFontUnderlined( false );
    strokeText( aText->GetText(), pos, orient );
}


int SCH_PAINTER::internalPinDecoSize( const LIB_PIN &aPin )
{
    if( m_schSettings.m_PinSymbolSize > 0 )
        return m_schSettings.m_PinSymbolSize;

    return aPin.GetNameTextSize() != 0 ? aPin.GetNameTextSize() / 2 : aPin.GetNumberTextSize() / 2;
}


// Utility for getting the size of the 'external' pin decorators (as a radius)
// i.e. the negation circle, the polarity 'slopes' and the nonlogic marker
int SCH_PAINTER::externalPinDecoSize( const LIB_PIN &aPin )
{
    if( m_schSettings.m_PinSymbolSize > 0 )
        return m_schSettings.m_PinSymbolSize;

    return aPin.GetNumberTextSize() / 2;
}


// Draw the target (an open circle) for a pin which has no connection or is being moved.
void SCH_PAINTER::drawPinDanglingSymbol( const VECTOR2I& aPos, bool aDrawingShadows )
{
    m_gal->SetIsFill( false );
    m_gal->SetIsStroke( true );
    m_gal->SetLineWidth( aDrawingShadows ? getShadowWidth()
                                         : m_schSettings.GetDanglineSymbolThickness() );

    m_gal->DrawCircle( aPos, TARGET_PIN_RADIUS );
}


void SCH_PAINTER::draw( LIB_PIN *aPin, int aLayer )
{
    if( !isUnitAndConversionShown( aPin ) )
        return;

    bool drawingShadows = aLayer == LAYER_SELECTION_SHADOWS;

    if( drawingShadows && !aPin->IsSelected() )
        return;

    VECTOR2I pos = mapCoords( aPin->GetPosition() );
    COLOR4D  color = getRenderColor( aPin, LAYER_PIN, drawingShadows );

    if( !aPin->IsVisible() )
    {
        if( m_schSettings.m_ShowHiddenPins )
        {
            color = getRenderColor( aPin, LAYER_HIDDEN, drawingShadows );
        }
        else
        {
            if( aPin->HasFlag( IS_DANGLING ) && aPin->IsPowerConnection() )
                drawPinDanglingSymbol( pos, drawingShadows );

            return;
        }
    }

    VECTOR2I p0;
    VECTOR2I dir;
    int len = aPin->GetLength();
    int orient = aPin->GetOrientation();

    switch( orient )
    {
    case PIN_UP:
        p0 = VECTOR2I( pos.x, pos.y - len );
        dir = VECTOR2I( 0, 1 );
        break;

    case PIN_DOWN:
        p0 = VECTOR2I( pos.x, pos.y + len );
        dir = VECTOR2I( 0, -1 );
        break;

    case PIN_LEFT:
        p0 = VECTOR2I( pos.x - len, pos.y );
        dir = VECTOR2I( 1, 0 );
        break;

    default:
    case PIN_RIGHT:
        p0 = VECTOR2I( pos.x + len, pos.y );
        dir = VECTOR2I( -1, 0 );
        break;
    }

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

        aPin->ClearFlags( IS_DANGLING ); // PIN_NC pin type is always not connected and dangling.
    }
    else
    {
        switch( aPin->GetShape() )
        {
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


    if( aPin->HasFlag( IS_DANGLING ) && ( aPin->IsVisible() || aPin->IsPowerConnection() ) )
        drawPinDanglingSymbol( pos, drawingShadows );

    LIB_PART* libEntry = aPin->GetParent();

    // Draw the labels
    if( drawingShadows && ( libEntry->Type() == LIB_PART_T || libEntry->IsSelected() )
            && !eeconfig()->m_Selection.draw_selected_children )
        return;

    int textOffset = libEntry->GetPinNameOffset();

    float nameLineWidth = getLineWidth( aPin, drawingShadows );
    nameLineWidth = Clamp_Text_PenSize( nameLineWidth, aPin->GetNameTextSize(), false );
    float numLineWidth = getLineWidth( aPin, drawingShadows );
    numLineWidth = Clamp_Text_PenSize( numLineWidth, aPin->GetNumberTextSize(), false );

    #define PIN_TEXT_MARGIN 4.0

    // Four locations around a pin where text can be drawn
    enum { INSIDE = 0, OUTSIDE, ABOVE, BELOW };
    int size[4] = { 0, 0, 0, 0 };
    float thickness[4] = { numLineWidth, numLineWidth, numLineWidth, numLineWidth };
    COLOR4D colour[4];
    wxString text[4];

    // TextOffset > 0 means pin NAMES on inside, pin NUMBERS above and nothing below
    if( textOffset )
    {
        size     [INSIDE] = libEntry->ShowPinNames() ? aPin->GetNameTextSize() : 0;
        thickness[INSIDE] = nameLineWidth;
        colour   [INSIDE] = getRenderColor( aPin, LAYER_PINNAM, drawingShadows );
        text     [INSIDE] = aPin->GetName();

        size     [ABOVE] = libEntry->ShowPinNumbers() ? aPin->GetNumberTextSize() : 0;
        thickness[ABOVE] = numLineWidth;
        colour   [ABOVE] = getRenderColor( aPin, LAYER_PINNUM, drawingShadows );
        text     [ABOVE] = aPin->GetNumber();
    }
    // Otherwise pin NAMES go above and pin NUMBERS go below
    else
    {
        size     [ABOVE] = libEntry->ShowPinNames() ? aPin->GetNameTextSize() : 0;
        thickness[ABOVE] = nameLineWidth;
        colour   [ABOVE] = getRenderColor( aPin, LAYER_PINNAM, drawingShadows );
        text     [ABOVE] = aPin->GetName();

        size     [BELOW] = libEntry->ShowPinNumbers() ? aPin->GetNumberTextSize() : 0;
        thickness[BELOW] = numLineWidth;
        colour   [BELOW] = getRenderColor( aPin, LAYER_PINNUM, drawingShadows );
        text     [BELOW] = aPin->GetNumber();
    }

    if( m_schSettings.m_ShowPinsElectricalType )
    {
        size     [OUTSIDE] = std::max( aPin->GetNameTextSize() * 3 / 4, Millimeter2iu( 0.7 ) );
        thickness[OUTSIDE] = float( size[OUTSIDE] ) / 6.0F;
        colour   [OUTSIDE] = getRenderColor( aPin, LAYER_NOTES, drawingShadows );
        text     [OUTSIDE] = aPin->GetElectricalTypeName();
    }

    if( !aPin->IsVisible() )
    {
        for( COLOR4D& c : colour )
            c = getRenderColor( aPin, LAYER_HIDDEN, drawingShadows );
    }

    int   insideOffset = textOffset;
    int   outsideOffset = 10;
    float lineThickness = (float) m_schSettings.GetDefaultPenWidth();
    float aboveOffset = Mils2iu( PIN_TEXT_MARGIN ) + ( thickness[ABOVE] + lineThickness ) / 2.0;
    float belowOffset = Mils2iu( PIN_TEXT_MARGIN ) + ( thickness[BELOW] + lineThickness ) / 2.0;

    if( drawingShadows )
    {
        for( float& t : thickness )
            t += getShadowWidth();

        insideOffset -= KiROUND( getShadowWidth() / 2 );
        outsideOffset -= KiROUND( getShadowWidth() / 2 );
    }

    #define SET_DC( i ) \
        m_gal->SetGlyphSize( VECTOR2D( size[i], size[i] ) ); \
        m_gal->SetLineWidth( thickness[i] ); \
        m_gal->SetStrokeColor( colour[i] )

    switch( orient )
    {
    case PIN_LEFT:
        if( size[INSIDE] )
        {
            SET_DC( INSIDE );
            m_gal->SetHorizontalJustify( GR_TEXT_HJUSTIFY_RIGHT );
            m_gal->SetVerticalJustify( GR_TEXT_VJUSTIFY_CENTER );
            strokeText( text[INSIDE], pos + VECTOR2D( -insideOffset - len, 0 ), 0 );
        }
        if( size[OUTSIDE] )
        {
            SET_DC( OUTSIDE );
            m_gal->SetHorizontalJustify( GR_TEXT_HJUSTIFY_LEFT );
            m_gal->SetVerticalJustify( GR_TEXT_VJUSTIFY_CENTER );
            strokeText( text[OUTSIDE], pos + VECTOR2D( outsideOffset, 0 ), 0 );
        }
        if( size[ABOVE] )
        {
            SET_DC( ABOVE );
            m_gal->SetHorizontalJustify( GR_TEXT_HJUSTIFY_CENTER );
            m_gal->SetVerticalJustify( GR_TEXT_VJUSTIFY_BOTTOM );
            strokeText( text[ABOVE], pos + VECTOR2D( -len / 2.0, -aboveOffset ), 0 );
        }
        if( size[BELOW] )
        {
            SET_DC( BELOW );
            m_gal->SetHorizontalJustify( GR_TEXT_HJUSTIFY_CENTER );
            m_gal->SetVerticalJustify( GR_TEXT_VJUSTIFY_TOP );
            strokeText( text[BELOW], pos + VECTOR2D( -len / 2.0, belowOffset ), 0 );
        }
        break;

    case PIN_RIGHT:
        if( size[INSIDE] )
        {
            SET_DC( INSIDE );
            m_gal->SetHorizontalJustify( GR_TEXT_HJUSTIFY_LEFT );
            m_gal->SetVerticalJustify( GR_TEXT_VJUSTIFY_CENTER );
            m_gal->SetHorizontalJustify( GR_TEXT_HJUSTIFY_LEFT );
            strokeText( text[INSIDE], pos + VECTOR2D( insideOffset + len, 0 ), 0 );
        }
        if( size[OUTSIDE] )
        {
            SET_DC( OUTSIDE );
            m_gal->SetHorizontalJustify( GR_TEXT_HJUSTIFY_RIGHT );
            m_gal->SetVerticalJustify( GR_TEXT_VJUSTIFY_CENTER );
            strokeText( text[OUTSIDE], pos + VECTOR2D( -outsideOffset, 0 ), 0 );
        }
        if( size[ABOVE] )
        {
            SET_DC( ABOVE );
            m_gal->SetHorizontalJustify( GR_TEXT_HJUSTIFY_CENTER );
            m_gal->SetVerticalJustify( GR_TEXT_VJUSTIFY_BOTTOM );
            strokeText( text[ABOVE], pos + VECTOR2D( len / 2.0, -aboveOffset ), 0 );
        }
        if( size[BELOW] )
        {
            SET_DC( BELOW );
            m_gal->SetHorizontalJustify( GR_TEXT_HJUSTIFY_CENTER );
            m_gal->SetVerticalJustify( GR_TEXT_VJUSTIFY_TOP );
            strokeText( text[BELOW], pos + VECTOR2D( len / 2.0, belowOffset ), 0 );
        }
        break;

    case PIN_DOWN:
        if( size[INSIDE] )
        {
            SET_DC( INSIDE );
            m_gal->SetHorizontalJustify( GR_TEXT_HJUSTIFY_RIGHT );
            m_gal->SetVerticalJustify( GR_TEXT_VJUSTIFY_CENTER );
            strokeText( text[INSIDE], pos + VECTOR2D( 0, insideOffset + len ), M_PI / 2 );
        }
        if( size[OUTSIDE] )
        {
            SET_DC( OUTSIDE );
            m_gal->SetHorizontalJustify( GR_TEXT_HJUSTIFY_LEFT );
            m_gal->SetVerticalJustify( GR_TEXT_VJUSTIFY_CENTER );
            strokeText( text[OUTSIDE], pos + VECTOR2D( 0, -outsideOffset ), M_PI / 2 );
        }
        if( size[ABOVE] )
        {
            SET_DC( ABOVE );
            m_gal->SetHorizontalJustify( GR_TEXT_HJUSTIFY_CENTER );
            m_gal->SetVerticalJustify( GR_TEXT_VJUSTIFY_BOTTOM );
            strokeText( text[ABOVE], pos + VECTOR2D( -aboveOffset, len / 2.0 ), M_PI / 2 );
        }
        if( size[BELOW] )
        {
            SET_DC( BELOW );
            m_gal->SetHorizontalJustify( GR_TEXT_HJUSTIFY_CENTER );
            m_gal->SetVerticalJustify( GR_TEXT_VJUSTIFY_TOP );
            strokeText( text[BELOW], pos + VECTOR2D( belowOffset, len / 2.0 ), M_PI / 2 );
        }
        break;

    case PIN_UP:
        if( size[INSIDE] )
        {
            SET_DC( INSIDE );
            m_gal->SetHorizontalJustify( GR_TEXT_HJUSTIFY_LEFT );
            m_gal->SetVerticalJustify( GR_TEXT_VJUSTIFY_CENTER );
            strokeText( text[INSIDE], pos + VECTOR2D( 0, -insideOffset - len ), M_PI / 2 );
        }
        if( size[OUTSIDE] )
        {
            SET_DC( OUTSIDE );
            m_gal->SetHorizontalJustify( GR_TEXT_HJUSTIFY_RIGHT );
            m_gal->SetVerticalJustify( GR_TEXT_VJUSTIFY_CENTER );
            strokeText( text[OUTSIDE], pos + VECTOR2D( 0, outsideOffset ), M_PI / 2 );
        }
        if( size[ABOVE] )
        {
            SET_DC( ABOVE );
            m_gal->SetHorizontalJustify( GR_TEXT_HJUSTIFY_CENTER );
            m_gal->SetVerticalJustify( GR_TEXT_VJUSTIFY_BOTTOM );
            strokeText( text[ABOVE], pos + VECTOR2D( -aboveOffset, -len / 2.0 ), M_PI / 2 );
        }
        if( size[BELOW] )
        {
            SET_DC( BELOW );
            m_gal->SetHorizontalJustify( GR_TEXT_HJUSTIFY_CENTER );
            m_gal->SetVerticalJustify( GR_TEXT_VJUSTIFY_TOP );
            strokeText( text[BELOW], pos + VECTOR2D( belowOffset, -len / 2.0 ), M_PI / 2 );
        }
        break;

    default:
        wxFAIL_MSG( "Unknown pin orientation" );
    }
}


void SCH_PAINTER::draw( const LIB_BEZIER *aCurve, int aLayer )
{
    if( !isUnitAndConversionShown( aCurve ) )
        return;

    if( setDeviceColors( aCurve, aLayer ) )
    {
        BEZIER_POLY poly ( aCurve->GetPoints() );
        std::vector<wxPoint> pts;
        std::deque<VECTOR2D> pts_xformed;
        poly.GetPoly( pts );

        for( const wxPoint &p : pts )
            pts_xformed.push_back( mapCoords( p ) );

        m_gal->DrawPolygon( pts_xformed );
    }
}


// Draw the target (an open square) for a wire or label which has no connection or is
// being moved.
void SCH_PAINTER::drawDanglingSymbol( const wxPoint& aPos, int aWidth, bool aDrawingShadows )
{
    wxPoint radius( aWidth + Mils2iu( DANGLING_SYMBOL_SIZE / 2 ),
                    aWidth + Mils2iu( DANGLING_SYMBOL_SIZE /2 ) );

    m_gal->SetIsStroke( true );
    m_gal->SetIsFill( false );
    m_gal->SetLineWidth( aDrawingShadows ? getShadowWidth()
                                         : m_schSettings.GetDanglineSymbolThickness() );

    m_gal->DrawRectangle( aPos - radius, aPos + radius );
}


void SCH_PAINTER::draw( const SCH_JUNCTION *aJct, int aLayer )
{
    bool drawingShadows = aLayer == LAYER_SELECTION_SHADOWS;

    if( drawingShadows && !aJct->IsSelected() )
        return;

    COLOR4D color = getRenderColor( aJct, aJct->GetLayer(), drawingShadows );

    int junctionSize = aJct->GetDiameter() / 2;

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


void SCH_PAINTER::draw( const SCH_LINE *aLine, int aLayer )
{
    bool drawingShadows = aLayer == LAYER_SELECTION_SHADOWS;

    if( drawingShadows && !aLine->IsSelected() )
        return;

    COLOR4D        color = getRenderColor( aLine, aLine->GetLayer(), drawingShadows );
    float          width = getLineWidth( aLine, drawingShadows );
    PLOT_DASH_TYPE lineStyle = aLine->GetEffectiveLineStyle();

    m_gal->SetIsStroke( true );
    m_gal->SetStrokeColor( color );
    m_gal->SetLineWidth( width );

    if( lineStyle <= PLOT_DASH_TYPE::FIRST_TYPE || drawingShadows )
    {
        m_gal->DrawLine( aLine->GetStartPoint(), aLine->GetEndPoint() );
    }
    else
    {
        VECTOR2D start = aLine->GetStartPoint();
        VECTOR2D end = aLine->GetEndPoint();

        EDA_RECT clip( (wxPoint)start, wxSize( end.x - start.x, end.y - start.y ) );
        clip.Normalize();

        double theta = atan2( end.y - start.y, end.x - start.x );
        double strokes[] = { 1.0, DASH_GAP_LEN( width ), 1.0, DASH_GAP_LEN( width ) };

        switch( lineStyle )
        {
        default:
        case PLOT_DASH_TYPE::DASH:
            strokes[0] = strokes[2] = DASH_MARK_LEN( width );
            break;
        case PLOT_DASH_TYPE::DOT:
            strokes[0] = strokes[2] = DOT_MARK_LEN( width );
            break;
        case PLOT_DASH_TYPE::DASHDOT:
            strokes[0] = DASH_MARK_LEN( width );
            strokes[2] = DOT_MARK_LEN( width );
            break;
        }

        for( size_t i = 0; i < 10000; ++i )
        {
            // Calculations MUST be done in doubles to keep from accumulating rounding
            // errors as we go.
            VECTOR2D next( start.x + strokes[ i % 4 ] * cos( theta ),
                           start.y + strokes[ i % 4 ] * sin( theta ) );

            // Drawing each segment can be done rounded to ints.
            wxPoint segStart( KiROUND( start.x ), KiROUND( start.y ) );
            wxPoint segEnd( KiROUND( next.x ), KiROUND( next.y ) );

            if( ClipLine( &clip, segStart.x, segStart.y, segEnd.x, segEnd.y ) )
                break;
            else if( i % 2 == 0 )
                m_gal->DrawLine( segStart, segEnd );

            start = next;
        }
    }

    if( aLine->IsStartDangling() && aLine->IsWire() )
    {
        drawDanglingSymbol( aLine->GetStartPoint(), getLineWidth( aLine, drawingShadows ),
                            drawingShadows );
    }

    if( aLine->IsEndDangling() && aLine->IsWire() )
    {
        drawDanglingSymbol( aLine->GetEndPoint(), getLineWidth( aLine, drawingShadows ),
                            drawingShadows );
    }
}


void SCH_PAINTER::draw( const SCH_TEXT *aText, int aLayer )
{
    bool drawingShadows = aLayer == LAYER_SELECTION_SHADOWS;

    if( drawingShadows && !aText->IsSelected() )
        return;

    switch( aText->Type() )
    {
    case SCH_SHEET_PIN_T:     aLayer = LAYER_SHEETLABEL; break;
    case SCH_HIER_LABEL_T:    aLayer = LAYER_HIERLABEL;  break;
    case SCH_GLOBAL_LABEL_T:  aLayer = LAYER_GLOBLABEL;  break;
    case SCH_LABEL_T:         aLayer = LAYER_LOCLABEL;   break;
    default:                  aLayer = LAYER_NOTES;      break;
    }

    COLOR4D color = getRenderColor( aText, aLayer, drawingShadows );

    if( m_schematic )
    {
        SCH_CONNECTION* conn = aText->Connection();

        if( conn && conn->IsBus() )
            color = getRenderColor( aText, LAYER_BUS, drawingShadows );
    }

    if( !( aText->IsVisible() || aText->IsForceVisible() ) )
    {
        if( m_schSettings.m_ShowHiddenText )
            color = getRenderColor( aText, LAYER_HIDDEN, drawingShadows );
        else
            return;
    }

    m_gal->SetIsFill( false );
    m_gal->SetIsStroke( true );
    m_gal->SetLineWidth( getTextThickness( aText, drawingShadows ) );
    m_gal->SetStrokeColor( color );
    m_gal->SetTextAttributes( aText );
    m_gal->SetFontUnderlined( false );

    VECTOR2D text_offset = aText->GetTextPos() + aText->GetSchematicTextOffset( &m_schSettings );
    wxString shownText( aText->GetShownText() );

    if( drawingShadows )
    {
        if( eeconfig()->m_Selection.text_as_box )
        {
            EDA_RECT bBox = aText->GetBoundingBox();

            m_gal->SetIsFill( true );
            m_gal->SetFillColor( color );
            m_gal->SetLineWidth( m_gal->GetLineWidth() * 0.5 );
            bBox.RevertYAxis();

            m_gal->DrawRectangle( mapCoords( bBox.GetPosition() ), mapCoords( bBox.GetEnd() ) );
            return;
        }

        switch( aText->GetLabelSpinStyle() )
        {
        case LABEL_SPIN_STYLE::LEFT:   text_offset.x += getShadowWidth() / 2; break;
        case LABEL_SPIN_STYLE::UP:     text_offset.y += getShadowWidth() / 2; break;
        case LABEL_SPIN_STYLE::RIGHT:  text_offset.x -= getShadowWidth() / 2; break;
        case LABEL_SPIN_STYLE::BOTTOM: text_offset.y -= getShadowWidth() / 2; break;
        }
    }

    if( !shownText.IsEmpty() )
    {
        strokeText( shownText, text_offset, aText->GetTextAngleRadians() );
    }

    if( aText->IsDangling() )
    {
        drawDanglingSymbol( aText->GetTextPos(), Mils2iu( DANGLING_SYMBOL_SIZE / 2 ),
                            drawingShadows );
    }

}


static void orientPart( LIB_PART* part, int orientation )
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
        { CMP_ORIENT_0,                  0, 0, 0 },
        { CMP_ORIENT_90,                 1, 0, 0 },
        { CMP_ORIENT_180,                2, 0, 0 },
        { CMP_ORIENT_270,                3, 0, 0 },
        { CMP_MIRROR_X + CMP_ORIENT_0,   0, 1, 0 },
        { CMP_MIRROR_X + CMP_ORIENT_90,  1, 1, 0 },
        { CMP_MIRROR_Y,                  0, 0, 1 },
        { CMP_MIRROR_X + CMP_ORIENT_270, 3, 1, 0 },
        { CMP_MIRROR_Y + CMP_ORIENT_0,   0, 0, 1 },
        { CMP_MIRROR_Y + CMP_ORIENT_90,  1, 0, 1 },
        { CMP_MIRROR_Y + CMP_ORIENT_180, 2, 0, 1 },
        { CMP_MIRROR_Y + CMP_ORIENT_270, 3, 0, 1 }
    };

    ORIENT o = orientations[ 0 ];

    for( auto& i : orientations )
    {
        if( i.flag == orientation )
        {
            o = i;
            break;
        }
    }

    for( auto& item : part->GetDrawItems() )
    {
        for( int i = 0; i < o.n_rots; i++ )
            item.Rotate( wxPoint(0, 0 ), true );

        if( o.mirror_x )
            item.MirrorVertical( wxPoint( 0, 0 ) );

        if( o.mirror_y )
            item.MirrorHorizontal( wxPoint( 0, 0 ) );
    }
}


void SCH_PAINTER::draw( SCH_COMPONENT *aSymbol, int aLayer )
{
    int unit = aSymbol->GetUnitSelection( &m_schematic->CurrentSheet() );
    int convert = aSymbol->GetConvert();

    // Use dummy part if the actual couldn't be found (or couldn't be locked).
    LIB_PART* originalPart = aSymbol->GetPartRef() ? aSymbol->GetPartRef().get() : dummy();
    LIB_PINS  originalPins;
    originalPart->GetPins( originalPins, unit, convert );

    // Copy the source so we can re-orient and translate it.
    LIB_PART tempPart( *originalPart );
    LIB_PINS tempPins;
    tempPart.GetPins( tempPins, unit, convert );

    tempPart.SetFlags( aSymbol->GetFlags() );

    orientPart( &tempPart, aSymbol->GetOrientation() );

    for( auto& tempItem : tempPart.GetDrawItems() )
    {
        tempItem.SetFlags( aSymbol->GetFlags() );     // SELECTED, HIGHLIGHTED, BRIGHTENED
        tempItem.MoveTo( tempItem.GetPosition() + (wxPoint) mapCoords( aSymbol->GetPosition() ) );
    }

    // Copy the pin info from the symbol to the temp pins
    for( unsigned i = 0; i < tempPins.size(); ++ i )
    {
        SCH_PIN* symbolPin = aSymbol->GetPin( originalPins[ i ] );
        LIB_PIN* tempPin = tempPins[ i ];

        tempPin->ClearFlags();
        tempPin->SetFlags( symbolPin->GetFlags() );     // SELECTED, HIGHLIGHTED, BRIGHTENED

        tempPin->SetName( symbolPin->GetName() );
        tempPin->SetType( symbolPin->GetType() );
        tempPin->SetShape( symbolPin->GetShape() );

        if( symbolPin->IsDangling() )
            tempPin->SetFlags( IS_DANGLING );
    }

    draw( &tempPart, aLayer, false, aSymbol->GetUnit(), aSymbol->GetConvert() );

    // The fields are SCH_COMPONENT-specific so don't need to be copied/oriented/translated
    for( const SCH_FIELD& field : aSymbol->GetFields() )
        draw( &field, aLayer );
}


void SCH_PAINTER::draw( const SCH_FIELD *aField, int aLayer )
{
    bool drawingShadows = aLayer == LAYER_SELECTION_SHADOWS;

    if( drawingShadows && !aField->IsSelected() )
        return;

    aLayer = aField->GetLayer();

    COLOR4D color = getRenderColor( aField, aLayer, drawingShadows );

    if( !( aField->IsVisible() || aField->IsForceVisible() ) )
    {
        if( m_schSettings.m_ShowHiddenText )
            color = getRenderColor( aField, LAYER_HIDDEN, drawingShadows );
        else
            return;
    }

    if( aField->IsVoid() )
        return;

    if( drawingShadows && aField->GetParent()->IsSelected()
            && !eeconfig()->m_Selection.draw_selected_children )
    {
        return;
    }

    bool underline = false;

    if( aField->IsHypertext() && ( aField->GetFlags() & IS_ROLLOVER ) > 0
            && !drawingShadows && !aField->IsMoving() )
    {
        color = PUREBLUE;
        underline = true;
    }

    // Calculate the text orientation according to the parent orientation.
    int orient = (int) aField->GetTextAngle();

    if( aField->GetParent() && aField->GetParent()->Type() == SCH_COMPONENT_T )
    {
        if( static_cast<SCH_COMPONENT*>( aField->GetParent() )->GetTransform().y1 )
        {
        // Rotate symbol 90 degrees.
        if( orient == TEXT_ANGLE_HORIZ )
            orient = TEXT_ANGLE_VERT;
        else
            orient = TEXT_ANGLE_HORIZ;
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
    EDA_RECT boundaryBox = aField->GetBoundingBox();
    wxPoint textpos = boundaryBox.Centre();

    m_gal->SetStrokeColor( color );
    m_gal->SetIsStroke( true );

    if( drawingShadows && eeconfig()->m_Selection.text_as_box )
    {
        m_gal->SetIsFill( true );
        m_gal->SetFillColor( color );
        m_gal->SetLineWidth( m_gal->GetLineWidth() * 0.5 );
        boundaryBox.RevertYAxis();

        m_gal->DrawRectangle( mapCoords( boundaryBox.GetPosition() ),
                              mapCoords( boundaryBox.GetEnd() ) );
    }
    else
    {
        m_gal->SetHorizontalJustify( GR_TEXT_HJUSTIFY_CENTER );
        m_gal->SetVerticalJustify( GR_TEXT_VJUSTIFY_CENTER );
        m_gal->SetIsFill( false );
        m_gal->SetGlyphSize( VECTOR2D( aField->GetTextSize() ) );
        m_gal->SetFontBold( aField->IsBold() );
        m_gal->SetFontItalic( aField->IsItalic() );
        m_gal->SetFontUnderlined( underline );
        m_gal->SetTextMirrored( aField->IsMirrored() );
        m_gal->SetLineWidth( getTextThickness( aField, drawingShadows ) );

        strokeText( aField->GetShownText(), textpos, orient == TEXT_ANGLE_VERT ? M_PI / 2 : 0 );
    }

    // Draw the umbilical line
    if( aField->IsMoving() )
    {
        wxPoint parentPos = aField->GetParentPosition();

        m_gal->SetLineWidth( m_schSettings.m_outlineWidth );
        m_gal->SetStrokeColor( COLOR4D( 0.0, 0.0, 1.0, 1.0 ) );
        m_gal->DrawLine( textpos, parentPos );
    }
}


void SCH_PAINTER::draw( SCH_GLOBALLABEL *aLabel, int aLayer )
{
    bool drawingShadows = aLayer == LAYER_SELECTION_SHADOWS;

    if( !drawingShadows || aLabel->IsSelected() )
    {
        COLOR4D color = getRenderColor( aLabel, LAYER_GLOBLABEL, drawingShadows );

        std::vector<wxPoint> pts;
        std::deque<VECTOR2D> pts2;

        aLabel->CreateGraphicShape( &m_schSettings, pts, aLabel->GetTextPos() );

        for( const wxPoint& p : pts )
            pts2.emplace_back( VECTOR2D( p.x, p.y ) );

        // The text is drawn inside the graphic shape.
        // On Cairo the graphic shape is filled by the background before drawing the text.
        // However if the text is selected, it is draw twice: first on LAYER_SELECTION_SHADOWS
        // and second on the text layer.  The second must not erase the first drawing.
        bool fillBg = ( aLayer == LAYER_SELECTION_SHADOWS ) || !aLabel->IsSelected();
        m_gal->SetIsFill( fillBg );
        m_gal->SetFillColor( m_schSettings.GetLayerColor( LAYER_SCHEMATIC_BACKGROUND ) );
        m_gal->SetIsStroke( true );
        m_gal->SetLineWidth( getTextThickness( aLabel, drawingShadows ) );
        m_gal->SetStrokeColor( color );
        m_gal->DrawPolyline( pts2 );

        draw( static_cast<SCH_TEXT*>( aLabel ), aLayer );
    }

    if( !drawingShadows || eeconfig()->m_Selection.draw_selected_children || !aLabel->IsSelected() )
    {
        draw( aLabel->GetIntersheetRefs(), aLayer );
    }
}


void SCH_PAINTER::draw( SCH_HIERLABEL *aLabel, int aLayer )
{
    bool drawingShadows = aLayer == LAYER_SELECTION_SHADOWS;

    if( drawingShadows && !aLabel->IsSelected() )
        return;

    COLOR4D color = getRenderColor( aLabel, LAYER_SHEETLABEL, drawingShadows );

    if( m_schematic )
    {
        SCH_CONNECTION* conn = aLabel->Connection();

        if( conn && conn->IsBus() )
            color = getRenderColor( aLabel, LAYER_BUS, drawingShadows );
    }

    std::vector<wxPoint> pts;
    std::deque<VECTOR2D> pts2;

    aLabel->CreateGraphicShape( &m_schSettings, pts, aLabel->GetTextPos() );

    for( auto p : pts )
        pts2.emplace_back( VECTOR2D( p.x, p.y ) );

    m_gal->SetIsFill( true );
    m_gal->SetFillColor( m_schSettings.GetLayerColor( LAYER_SCHEMATIC_BACKGROUND ) );
    m_gal->SetIsStroke( true );
    m_gal->SetLineWidth( getTextThickness( aLabel, drawingShadows ) );
    m_gal->SetStrokeColor( color );
    m_gal->DrawPolyline( pts2 );

    draw( static_cast<const SCH_TEXT*>( aLabel ), aLayer );
}


void SCH_PAINTER::draw( const SCH_SHEET *aSheet, int aLayer )
{
    bool drawingShadows = aLayer == LAYER_SELECTION_SHADOWS;

    if( aLayer == LAYER_HIERLABEL || aLayer == LAYER_SELECTION_SHADOWS )
    {
        for( SCH_SHEET_PIN* sheetPin : aSheet->GetPins() )
        {
            if( drawingShadows && !aSheet->IsSelected() && !sheetPin->IsSelected() )
                continue;

            if( drawingShadows && aSheet->IsSelected()
                    && !eeconfig()->m_Selection.draw_selected_children )
            {
                break;
            }

            int     width = std::max( aSheet->GetPenWidth(), m_schSettings.GetDefaultPenWidth() );
            wxPoint initial_pos = sheetPin->GetTextPos();
            wxPoint offset_pos = initial_pos;

            // For aesthetic reasons, the SHEET_PIN is drawn with a small offset of width / 2
            switch( sheetPin->GetEdge() )
            {
            case SHEET_TOP_SIDE:    offset_pos.y += KiROUND( width / 2.0 ); break;
            case SHEET_BOTTOM_SIDE: offset_pos.y -= KiROUND( width / 2.0 ); break;
            case SHEET_RIGHT_SIDE:  offset_pos.x -= KiROUND( width / 2.0 ); break;
            case SHEET_LEFT_SIDE:   offset_pos.x += KiROUND( width / 2.0 ); break;
            default: break;
            }

            sheetPin->SetTextPos( offset_pos );
            draw( static_cast<SCH_HIERLABEL*>( sheetPin ), aLayer );
            m_gal->DrawLine( offset_pos, initial_pos );
            sheetPin->SetTextPos( initial_pos );
        }
    }

    VECTOR2D pos  = aSheet->GetPosition();
    VECTOR2D size = aSheet->GetSize();

    if( aLayer == LAYER_SHEET_BACKGROUND )
    {
        m_gal->SetFillColor( getRenderColor( aSheet, LAYER_SHEET_BACKGROUND, true ) );
        m_gal->SetIsFill( true );
        m_gal->SetIsStroke( false );

        m_gal->DrawRectangle( pos, pos + size );
    }

    if( aLayer == LAYER_SHEET || aLayer == LAYER_SELECTION_SHADOWS )
    {
        m_gal->SetStrokeColor( getRenderColor( aSheet, LAYER_SHEET, drawingShadows ) );
        m_gal->SetIsStroke( true );
        m_gal->SetLineWidth( getLineWidth( aSheet, drawingShadows ) );
        m_gal->SetIsFill( false );

        m_gal->DrawRectangle( pos, pos + size );

        if( drawingShadows && !eeconfig()->m_Selection.draw_selected_children && aSheet->IsSelected() )
            return;

        for( const SCH_FIELD& field : aSheet->GetFields() )
            draw( &field, aLayer );
    }
}


void SCH_PAINTER::draw( const SCH_NO_CONNECT *aNC, int aLayer )
{
    bool drawingShadows = aLayer == LAYER_SELECTION_SHADOWS;

    if( drawingShadows && !aNC->IsSelected() )
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
    SCH_LINE line;
    bool drawingShadows = aLayer == LAYER_SELECTION_SHADOWS;

    if( drawingShadows && !aEntry->IsSelected() )
        return;

    line.SetLayer( aEntry->Type() == SCH_BUS_WIRE_ENTRY_T ? LAYER_WIRE : LAYER_BUS );

    if( aEntry->IsSelected() )
        line.SetSelected();
    else if( aEntry->IsBrightened() )
        line.SetBrightened();

    line.SetStartPoint( aEntry->GetPosition() );
    line.SetEndPoint( aEntry->GetEnd() );
    line.SetStroke( aEntry->GetStroke() );

    COLOR4D color = getRenderColor( aEntry, LAYER_WIRE, drawingShadows );

    if( aEntry->Type() == SCH_BUS_BUS_ENTRY_T )
        color = getRenderColor( aEntry, LAYER_BUS, drawingShadows );

    line.SetLineColor( color );
    line.SetLineStyle( aEntry->GetStrokeStyle() );

    draw( &line, aLayer );

    m_gal->SetIsFill( false );
    m_gal->SetIsStroke( true );
    m_gal->SetLineWidth( drawingShadows ? getShadowWidth() : 1.0F );

    if( aEntry->IsDanglingStart() )
        m_gal->DrawCircle( aEntry->GetPosition(),
                           aEntry->GetPenWidth() + ( TARGET_BUSENTRY_RADIUS / 2 ) );

    if( aEntry->IsDanglingEnd() )
        m_gal->DrawCircle( aEntry->GetEnd(),
                           aEntry->GetPenWidth() + ( TARGET_BUSENTRY_RADIUS / 2 ) );
}


void SCH_PAINTER::draw( const SCH_BITMAP *aBitmap, int aLayer )
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
            m_gal->SetLineWidth ( getShadowWidth() );
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


void SCH_PAINTER::draw( const SCH_MARKER *aMarker, int aLayer )
{
    bool drawingShadows = aLayer == LAYER_SELECTION_SHADOWS;

    if( drawingShadows && !aMarker->IsSelected() )
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
