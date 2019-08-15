/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2014 CERN
 * @author Tomasz Wlostowski <tomasz.wlostowski@cern.ch>
 * Copyright (C) 2019 KiCad Developers, see AUTHORS.txt for contributors.
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

#include <lib_item.h>
#include <lib_rectangle.h>
#include <lib_pin.h>
#include <lib_circle.h>
#include <lib_polyline.h>
#include <lib_arc.h>
#include <lib_field.h>
#include <lib_text.h>
#include <lib_bezier.h>
#include <sch_line.h>
#include <sch_component.h>
#include <sch_field.h>
#include <sch_junction.h>
#include <sch_text.h>
#include <sch_no_connect.h>
#include <sch_bus_entry.h>
#include <sch_bitmap.h>
#include <sch_sheet.h>
#include <gr_text.h>
#include <geometry/geometry_utils.h>
#include <lib_edit_frame.h>
#include <plotter.h>
#include <template_fieldnames.h>
#include <class_libentry.h>
#include <class_library.h>
#include <sch_edit_frame.h>
#include <view/view.h>
#include <gal/graphics_abstraction_layer.h>
#include <colors_design_settings.h>
#include <connection_graph.h>
#include <geometry/shape_line_chain.h>
#include <bezier_curves.h>

#include "sch_painter.h"

namespace KIGFX
{

SCH_RENDER_SETTINGS::SCH_RENDER_SETTINGS() :
    m_ShowUnit( 0 ), m_ShowConvert( 0 )
{
    ImportLegacyColors( nullptr );

    m_ShowHiddenText = true;
    m_ShowHiddenPins = true;
    m_ShowPinsElectricalType = true;
    m_ShowUmbilicals = true;
}


void SCH_RENDER_SETTINGS::ImportLegacyColors( const COLORS_DESIGN_SETTINGS* aSettings )
{
    for( int layer = SCH_LAYER_ID_START; layer < SCH_LAYER_ID_END; layer ++)
        m_layerColors[ layer ] = ::GetLayerColor( static_cast<SCH_LAYER_ID>( layer ) );

    for( int layer = GAL_LAYER_ID_START; layer < GAL_LAYER_ID_END; layer ++)
        m_layerColors[ layer ] = ::GetLayerColor( static_cast<SCH_LAYER_ID>( layer ) );

    m_backgroundColor = ::GetLayerColor( LAYER_SCHEMATIC_BACKGROUND );
}


const COLOR4D& SCH_RENDER_SETTINGS::GetColor( const VIEW_ITEM* aItem, int aLayer ) const
{
    return m_layerColors[ aLayer ];
}


/**
 * Used when a LIB_PART is not found in library to draw a dummy shape.
 * This component is a 400 mils square with the text "??"
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

        square->MoveTo( wxPoint( -200, 200 ));
        square->SetEndPosition( wxPoint( 200, -200 ) );

        LIB_TEXT* text = new LIB_TEXT( part );

        text->SetTextSize( wxSize( 150, 150 ) );
        text->SetText( wxString( wxT( "??" ) ) );

        part->AddDrawItem( square );
        part->AddDrawItem( text );
    }

    return part;
}


SCH_PAINTER::SCH_PAINTER( GAL* aGal ) :
    KIGFX::PAINTER (aGal)
{ }


#define HANDLE_ITEM( type_id, type_name ) \
    case type_id: draw( (type_name *) item, aLayer ); break


bool SCH_PAINTER::Draw( const VIEW_ITEM *aItem, int aLayer )
{
	auto item2 = static_cast<const EDA_ITEM*>( aItem );
    auto item = const_cast<EDA_ITEM*>( item2 );

    m_schSettings.ImportLegacyColors( nullptr );

#ifdef CONNECTIVITY_DEBUG

    auto sch_item = dynamic_cast<SCH_ITEM*>( item );
    auto conn = sch_item ? sch_item->Connection( *g_CurrentSheet ) : nullptr;

    if( conn )
    {
        auto pos = item->GetBoundingBox().Centre();
        auto label = conn->Name( true );

        m_gal->SetHorizontalJustify( GR_TEXT_HJUSTIFY_CENTER );
        m_gal->SetVerticalJustify( GR_TEXT_VJUSTIFY_CENTER );
        m_gal->SetStrokeColor( COLOR4D( LIGHTRED ) );
        m_gal->SetLineWidth( 2 );
        m_gal->SetGlyphSize( VECTOR2D( 20, 20 ) );
        m_gal->StrokeText( conn->Name( true ), pos, 0.0 );
    }

#endif

	switch( item->Type() )
	{
	HANDLE_ITEM(LIB_ALIAS_T, LIB_ALIAS);
	HANDLE_ITEM(LIB_PART_T, LIB_PART);
	HANDLE_ITEM(LIB_RECTANGLE_T, LIB_RECTANGLE);
    HANDLE_ITEM(LIB_POLYLINE_T, LIB_POLYLINE);
    HANDLE_ITEM(LIB_CIRCLE_T, LIB_CIRCLE);
    HANDLE_ITEM(LIB_PIN_T, LIB_PIN);
    HANDLE_ITEM(LIB_ARC_T, LIB_ARC);
    HANDLE_ITEM(LIB_FIELD_T, LIB_FIELD);
    HANDLE_ITEM(LIB_TEXT_T, LIB_TEXT);
    HANDLE_ITEM(LIB_BEZIER_T, LIB_BEZIER);
    HANDLE_ITEM(SCH_COMPONENT_T, SCH_COMPONENT);
    HANDLE_ITEM(SCH_JUNCTION_T, SCH_JUNCTION);
    HANDLE_ITEM(SCH_LINE_T, SCH_LINE);
    HANDLE_ITEM(SCH_TEXT_T, SCH_TEXT);
    HANDLE_ITEM(SCH_LABEL_T, SCH_TEXT);
    HANDLE_ITEM(SCH_FIELD_T, SCH_FIELD);
    HANDLE_ITEM(SCH_HIER_LABEL_T, SCH_HIERLABEL);
    HANDLE_ITEM(SCH_GLOBAL_LABEL_T, SCH_GLOBALLABEL);
    HANDLE_ITEM(SCH_SHEET_T, SCH_SHEET);
    HANDLE_ITEM(SCH_SHEET_PIN_T, SCH_HIERLABEL);
    HANDLE_ITEM(SCH_NO_CONNECT_T, SCH_NO_CONNECT);
    HANDLE_ITEM(SCH_BUS_WIRE_ENTRY_T, SCH_BUS_ENTRY_BASE);
    HANDLE_ITEM(SCH_BUS_BUS_ENTRY_T, SCH_BUS_ENTRY_BASE);
    HANDLE_ITEM(SCH_BITMAP_T, SCH_BITMAP);
    HANDLE_ITEM(SCH_MARKER_T, SCH_MARKER);

    default: return false;
	}

	return false;
}


bool SCH_PAINTER::isUnitAndConversionShown( const LIB_ITEM* aItem )
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


float SCH_PAINTER::getShadowWidth()
{
    const MATRIX3x3D& matrix = m_gal->GetScreenWorldMatrix();

    // For best visuals the selection width must be a cross between the zoom level and the
    // default line width.
    return (float) ( ( fabs( matrix.GetScale().x * 5.5 ) + GetDefaultLineThickness() ) / 2.0 );
}


COLOR4D SCH_PAINTER::getRenderColor( const EDA_ITEM* aItem, int aLayer, bool aDrawingShadows )
{
    static COLOR4D highlightColor( 1.0, 0.3, 0.3, 1.0 );
    static COLOR4D selectionColor = wxSystemSettings::GetColour( wxSYS_COLOUR_HIGHLIGHT );

    COLOR4D color = m_schSettings.GetLayerColor( aLayer );

    if( aItem->Type() == SCH_LINE_T )
        color = static_cast<const SCH_LINE*>( aItem )->GetLineColor();

    if( aItem->IsBrightened() && !aDrawingShadows ) // Selection disambiguation, etc.
    {
        color = m_schSettings.GetLayerColor( LAYER_BRIGHTENED );

        if( aLayer == LAYER_DEVICE_BACKGROUND || aLayer == LAYER_SHEET_BACKGROUND )
            color = color.WithAlpha( 0.2 );
    }
    else if( aItem->IsSelected() )
    {
        if( aDrawingShadows )
            color = selectionColor.WithAlpha( 0.8 );
    }
    else if( aItem->IsHighlighted() )               // Cross-probing
    {
        color = highlightColor;
    }

    return color;
}


float SCH_PAINTER::getLineWidth( const LIB_ITEM* aItem, bool aDrawingShadows )
{
    float width = (float) aItem->GetPenSize();

    if( aItem->IsSelected() && aDrawingShadows )
        width += getShadowWidth();

    return width;
}


float SCH_PAINTER::getLineWidth( const SCH_ITEM* aItem, bool aDrawingShadows )
{
   float width = (float) aItem->GetPenSize();

   if( aItem->IsSelected() && aDrawingShadows )
       width += getShadowWidth();

   return width;
}


float SCH_PAINTER::getTextThickness( const SCH_TEXT* aItem, bool aDrawingShadows )
{
    float width = (float) aItem->GetThickness();

    if( width == 0 )
        width = (float) GetDefaultLineThickness();

    if( aItem->IsSelected() && aDrawingShadows )
        width += getShadowWidth();

    return width;
}


void SCH_PAINTER::draw( LIB_PART *aPart, int aLayer, bool aDrawFields, int aUnit, int aConvert )
{
    if( !aUnit )
        aUnit = m_schSettings.m_ShowUnit;

    if( !aConvert )
        aConvert = m_schSettings.m_ShowConvert;

    for( auto& item : aPart->GetDrawItems() )
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


void SCH_PAINTER::draw( LIB_ALIAS *aAlias, int aLayer )
{
    LIB_PART* comp = aAlias->GetPart();

    draw( comp, aLayer, false );

    LIB_FIELDS fields;
    comp->GetFields( fields );

    if( !aAlias->IsRoot() )
    {
        fields[ VALUE ].SetText( aAlias->GetName() );
        fields[ DATASHEET ].SetText( aAlias->GetDocFileName() );
    }

    for( LIB_FIELD& field : fields )
        draw( &field, aLayer );
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
            return true;
        }

        return false;

    case LAYER_DEVICE_BACKGROUND:
        if( aItem->GetFillMode() == FILLED_WITH_BG_BODYCOLOR )
        {
            COLOR4D fillColor = getRenderColor( aItem, LAYER_DEVICE_BACKGROUND, false );

            // These actions place the item over others, so allow a modest transparency here
            if( aItem->IsMoving() || aItem->IsDragging() || aItem->IsResized() )
                fillColor = fillColor.WithAlpha( 0.75 );

            m_gal->SetIsFill( aItem->GetFillMode() == FILLED_WITH_BG_BODYCOLOR );
            m_gal->SetFillColor( fillColor );
            m_gal->SetIsStroke( false );
            return true;
        }

        return false;

    case LAYER_DEVICE:
        m_gal->SetIsFill( aItem->GetFillMode() == FILLED_SHAPE );
        m_gal->SetFillColor( getRenderColor( aItem, LAYER_DEVICE, false ) );
        m_gal->SetIsStroke( aItem->GetPenSize() > 0 );
        m_gal->SetLineWidth( getLineWidth( aItem, false ) );
        m_gal->SetStrokeColor( getRenderColor( aItem, LAYER_DEVICE, false ) );

        return true;

    default:
        return false;
    }
}


void SCH_PAINTER::draw( LIB_RECTANGLE *aRect, int aLayer )
{
    if( !isUnitAndConversionShown( aRect ) )
        return;

    if( setDeviceColors( aRect, aLayer ) )
        m_gal->DrawRectangle( mapCoords( aRect->GetPosition() ), mapCoords( aRect->GetEnd() ) );

}


void SCH_PAINTER::draw( LIB_CIRCLE *aCircle, int aLayer )
{
    if( !isUnitAndConversionShown( aCircle ) )
        return;

    if( setDeviceColors( aCircle, aLayer ) )
        m_gal->DrawCircle( mapCoords( aCircle->GetPosition() ), aCircle->GetRadius() );
}


void SCH_PAINTER::draw( LIB_ARC *aArc, int aLayer )
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


void SCH_PAINTER::draw( LIB_POLYLINE *aLine, int aLayer )
{
    if( !isUnitAndConversionShown( aLine ) )
        return;

    if( setDeviceColors( aLine, aLayer ) )
    {
        const std::vector<wxPoint>& pts = aLine->GetPolyPoints();
        std::deque<VECTOR2D> vtx;

        for( auto p : pts )
            vtx.push_back( mapCoords( p ) );

        m_gal->DrawPolygon( vtx );
    }
}


void SCH_PAINTER::draw( LIB_FIELD *aField, int aLayer )
{
    bool drawingShadows = aLayer == LAYER_SELECTION_SHADOWS;

    if( drawingShadows && !aField->IsSelected() )
        return;

    // Must check layer as fields are sometimes drawn by their parent rather than
    // directly from the view.
    int layers[KIGFX::VIEW::VIEW_MAX_LAYERS];
    int layers_count;
    aField->ViewGetLayers( layers, layers_count );

    if( aLayer != layers[0] )
        return;

    if( !isUnitAndConversionShown( aField ) )
        return;

    COLOR4D color = getRenderColor( aField, aLayer, drawingShadows );

    if( !aField->IsVisible() )
    {
        if( m_schSettings.m_ShowHiddenText )
            color = getRenderColor( aField, LAYER_HIDDEN, drawingShadows );
        else
            return;
    }

    m_gal->SetLineWidth( getLineWidth( aField, drawingShadows ) );
    m_gal->SetIsFill( false );
    m_gal->SetIsStroke( true );
    m_gal->SetStrokeColor( color );
    m_gal->SetGlyphSize( VECTOR2D( aField->GetTextSize() ) );
    m_gal->SetFontItalic( aField->IsItalic() );

    m_gal->SetHorizontalJustify( aField->GetHorizJustify( ) );
    m_gal->SetVerticalJustify( aField->GetVertJustify( ) );

    auto pos = mapCoords( aField->GetPosition() );
    double orient = aField->GetTextAngleRadians();

    m_gal->StrokeText( aField->GetText(), pos, orient );

    // Draw the umbilical line
    if( aField->IsMoving() && m_schSettings.m_ShowUmbilicals )
    {
        m_gal->SetLineWidth( m_schSettings.m_outlineWidth );
        m_gal->SetStrokeColor( COLOR4D( 0.0, 0.0, 1.0, 1.0 ) );
        m_gal->DrawLine( pos, wxPoint( 0, 0 ) );
    }
}


void SCH_PAINTER::draw( LIB_TEXT *aText, int aLayer )
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
    m_gal->SetLineWidth( getLineWidth( aText, drawingShadows ) );
    m_gal->SetIsFill( false );
    m_gal->SetIsStroke( true );
    m_gal->SetStrokeColor( color );
    m_gal->SetGlyphSize( VECTOR2D( aText->GetTextSize() ) );
    m_gal->SetFontBold( aText->IsBold() );
    m_gal->SetFontItalic( aText->IsItalic() );
    m_gal->StrokeText( aText->GetText(), pos, orient );
}


static int InternalPinDecoSize( const LIB_PIN &aPin )
{
    return aPin.GetNameTextSize() != 0 ? aPin.GetNameTextSize() / 2 : aPin.GetNumberTextSize() / 2;
}


// Utility for getting the size of the 'external' pin decorators (as a radius)
// i.e. the negation circle, the polarity 'slopes' and the nonlogic marker
static int ExternalPinDecoSize( const LIB_PIN &aPin )
{
    return aPin.GetNumberTextSize() / 2;
}


// Draw the target (an open circle) for a pin which has no connection or is being moved.
void SCH_PAINTER::drawPinDanglingSymbol( const VECTOR2I& aPos, bool aDrawingShadows )
{
    m_gal->SetIsFill( false );
    m_gal->SetIsStroke( true );
    m_gal->SetLineWidth( aDrawingShadows ? getShadowWidth() : 1.0F );

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
            if( ( aPin->GetFlags() & IS_DANGLING ) && aPin->IsPowerConnection() )
                drawPinDanglingSymbol( pos, drawingShadows );

            return;
        }
    }

    VECTOR2I p0;
    VECTOR2I dir;
	int len = aPin->GetLength();
	int shape = aPin->GetShape();
    int orient = aPin->GetOrientation();

    switch( orient )
	{
    case PIN_UP:
        p0 = VECTOR2I( pos.x, pos.y - len );
        dir = VECTOR2I(0, 1);
        break;
    case PIN_DOWN:
        p0 = VECTOR2I( pos.x, pos.y + len );
        dir = VECTOR2I(0, -1);
        break;
    case PIN_LEFT:
        p0 = VECTOR2I( pos.x - len, pos.y );
        dir = VECTOR2I(1, 0);
        break;
	default:
    case PIN_RIGHT:
        p0 = VECTOR2I( pos.x + len, pos.y );
        dir = VECTOR2I(-1, 0);
        break;
	}

    VECTOR2D pc;

    m_gal->SetIsStroke( true );
    m_gal->SetIsFill( false );
    m_gal->SetLineWidth( getLineWidth( aPin, drawingShadows ) );
    m_gal->SetStrokeColor( color );
    m_gal->SetFontBold( false );
    m_gal->SetFontItalic( false );

    const int radius = ExternalPinDecoSize( *aPin );
    const int diam = radius*2;
    const int clock_size = InternalPinDecoSize( *aPin );

	if( shape == PINSHAPE_INVERTED )
	{
		m_gal->DrawCircle( p0 + dir * radius, radius );
		m_gal->DrawLine( p0 + dir * ( diam ), pos );
	}
	else if( shape == PINSHAPE_FALLING_EDGE_CLOCK )
    {
        pc = p0 + dir * clock_size ;

        triLine( p0 + VECTOR2D( dir.y, -dir.x) * clock_size,
                 pc,
                 p0 + VECTOR2D( -dir.y, dir.x) * clock_size );

        m_gal->DrawLine( pos, pc );
    }
    else
    {
        m_gal->DrawLine( p0, pos );
    }

    if( shape == PINSHAPE_CLOCK )
    {
        if (!dir.y)
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
    }

    if( shape == PINSHAPE_INPUT_LOW )
    {
        if(!dir.y)
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
    }

    if( shape == PINSHAPE_OUTPUT_LOW )    /* IEEE symbol "Active Low Output" */
    {
        if( !dir.y )    // Horizontal pin
            m_gal->DrawLine( p0 - VECTOR2D( 0, diam ), p0 + VECTOR2D( dir.x, 0 ) * diam );
        else            // Vertical pin
            m_gal->DrawLine( p0 - VECTOR2D( diam, 0 ), p0 + VECTOR2D( 0, dir.y ) * diam );
    }

    if( shape == PINSHAPE_NONLOGIC ) /* NonLogic pin symbol */
    {
        m_gal->DrawLine( p0 - VECTOR2D( dir.x + dir.y, dir.y - dir.x ) * radius,
                         p0 + VECTOR2D( dir.x + dir.y, dir.y - dir.x ) * radius );
        m_gal->DrawLine( p0 - VECTOR2D( dir.x - dir.y, dir.x + dir.y ) * radius,
                         p0 + VECTOR2D( dir.x - dir.y, dir.x + dir.y ) * radius );
    }

    if( aPin->GetType() == PIN_NC )   // Draw a N.C. symbol
    {
        m_gal->DrawLine( pos + VECTOR2D( -1, -1 ) * TARGET_PIN_RADIUS,
                         pos + VECTOR2D(  1,  1 ) * TARGET_PIN_RADIUS );
        m_gal->DrawLine( pos + VECTOR2D(  1, -1 ) * TARGET_PIN_RADIUS ,
                         pos + VECTOR2D( -1,  1 ) * TARGET_PIN_RADIUS );

        aPin->ClearFlags( IS_DANGLING ); // PIN_NC pin type is always not connected and dangling.
    }

    if( ( aPin->GetFlags() & IS_DANGLING ) && ( aPin->IsVisible() || aPin->IsPowerConnection() ) )
        drawPinDanglingSymbol( pos, drawingShadows );

    // Draw the labels

    LIB_PART* libEntry = aPin->GetParent();
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
    float lineThickness = (float) GetDefaultLineThickness();
    float aboveOffset = PIN_TEXT_MARGIN + ( thickness[ABOVE] + lineThickness ) / 2.0;
    float belowOffset = PIN_TEXT_MARGIN + ( thickness[BELOW] + lineThickness ) / 2.0;

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
            m_gal->StrokeText( text[INSIDE], pos + VECTOR2D( -insideOffset - len, 0 ), 0 );
        }
        if( size[OUTSIDE] )
        {
            SET_DC( OUTSIDE );
            m_gal->SetHorizontalJustify( GR_TEXT_HJUSTIFY_LEFT );
            m_gal->SetVerticalJustify( GR_TEXT_VJUSTIFY_CENTER );
            m_gal->StrokeText( text[OUTSIDE], pos + VECTOR2D( outsideOffset, 0 ), 0 );
        }
        if( size[ABOVE] )
        {
            SET_DC( ABOVE );
            m_gal->SetHorizontalJustify( GR_TEXT_HJUSTIFY_CENTER );
            m_gal->SetVerticalJustify( GR_TEXT_VJUSTIFY_BOTTOM );
            m_gal->StrokeText( text[ABOVE], pos + VECTOR2D( -len / 2.0, -aboveOffset ), 0 );
        }
        if( size[BELOW] )
        {
            SET_DC( BELOW );
            m_gal->SetHorizontalJustify( GR_TEXT_HJUSTIFY_CENTER );
            m_gal->SetVerticalJustify( GR_TEXT_VJUSTIFY_TOP );
            m_gal->StrokeText( text[BELOW], pos + VECTOR2D( -len / 2.0, belowOffset ), 0 );
        }
        break;

    case PIN_RIGHT:
        if( size[INSIDE] )
        {
            SET_DC( INSIDE );
            m_gal->SetHorizontalJustify( GR_TEXT_HJUSTIFY_LEFT );
            m_gal->SetVerticalJustify( GR_TEXT_VJUSTIFY_CENTER );
            m_gal->SetHorizontalJustify( GR_TEXT_HJUSTIFY_LEFT );
            m_gal->StrokeText( text[INSIDE], pos + VECTOR2D( insideOffset + len, 0 ), 0 );
        }
        if( size[OUTSIDE] )
        {
            SET_DC( OUTSIDE );
            m_gal->SetHorizontalJustify( GR_TEXT_HJUSTIFY_RIGHT );
            m_gal->SetVerticalJustify( GR_TEXT_VJUSTIFY_CENTER );
            m_gal->StrokeText( text[OUTSIDE], pos + VECTOR2D( -outsideOffset, 0 ), 0 );
        }
        if( size[ABOVE] )
        {
            SET_DC( ABOVE );
            m_gal->SetHorizontalJustify( GR_TEXT_HJUSTIFY_CENTER );
            m_gal->SetVerticalJustify( GR_TEXT_VJUSTIFY_BOTTOM );
            m_gal->StrokeText( text[ABOVE], pos + VECTOR2D( len / 2.0, -aboveOffset ), 0 );
        }
        if( size[BELOW] )
        {
            SET_DC( BELOW );
            m_gal->SetHorizontalJustify( GR_TEXT_HJUSTIFY_CENTER );
            m_gal->SetVerticalJustify( GR_TEXT_VJUSTIFY_TOP );
            m_gal->StrokeText( text[BELOW], pos + VECTOR2D( len / 2.0, belowOffset ), 0 );
        }
        break;

    case PIN_DOWN:
        if( size[INSIDE] )
        {
            SET_DC( INSIDE );
            m_gal->SetHorizontalJustify( GR_TEXT_HJUSTIFY_RIGHT );
            m_gal->SetVerticalJustify( GR_TEXT_VJUSTIFY_CENTER );
            m_gal->StrokeText ( text[INSIDE], pos + VECTOR2D( 0, insideOffset + len ), M_PI / 2);
        }
        if( size[OUTSIDE] )
        {
            SET_DC( OUTSIDE );
            m_gal->SetHorizontalJustify( GR_TEXT_HJUSTIFY_LEFT );
            m_gal->SetVerticalJustify( GR_TEXT_VJUSTIFY_CENTER );
            m_gal->StrokeText ( text[OUTSIDE], pos + VECTOR2D( 0, -outsideOffset ), M_PI / 2);
        }
        if( size[ABOVE] )
        {
            SET_DC( ABOVE );
            m_gal->SetHorizontalJustify( GR_TEXT_HJUSTIFY_CENTER );
            m_gal->SetVerticalJustify( GR_TEXT_VJUSTIFY_BOTTOM );
            m_gal->StrokeText( text[ABOVE], pos + VECTOR2D( -aboveOffset, len / 2.0 ), M_PI / 2 );
        }
        if( size[BELOW] )
        {
            SET_DC( BELOW );
            m_gal->SetHorizontalJustify( GR_TEXT_HJUSTIFY_CENTER );
            m_gal->SetVerticalJustify( GR_TEXT_VJUSTIFY_TOP );
            m_gal->StrokeText( text[BELOW], pos + VECTOR2D( belowOffset, len / 2.0 ), M_PI / 2 );
        }
        break;

    case PIN_UP:
        if( size[INSIDE] )
        {
            SET_DC( INSIDE );
            m_gal->SetHorizontalJustify( GR_TEXT_HJUSTIFY_LEFT );
            m_gal->SetVerticalJustify( GR_TEXT_VJUSTIFY_CENTER );
            m_gal->StrokeText ( text[INSIDE], pos + VECTOR2D( 0, -insideOffset - len ), M_PI / 2);
        }
        if( size[OUTSIDE] )
        {
            SET_DC( OUTSIDE );
            m_gal->SetHorizontalJustify( GR_TEXT_HJUSTIFY_RIGHT );
            m_gal->SetVerticalJustify( GR_TEXT_VJUSTIFY_CENTER );
            m_gal->StrokeText ( text[OUTSIDE], pos + VECTOR2D( 0, outsideOffset ), M_PI / 2);
        }
        if( size[ABOVE] )
        {
            SET_DC( ABOVE );
            m_gal->SetHorizontalJustify( GR_TEXT_HJUSTIFY_CENTER );
            m_gal->SetVerticalJustify( GR_TEXT_VJUSTIFY_BOTTOM );
            m_gal->StrokeText( text[ABOVE], pos + VECTOR2D( -aboveOffset, -len / 2.0 ), M_PI / 2 );
        }
        if( size[BELOW] )
        {
            SET_DC( BELOW );
            m_gal->SetHorizontalJustify( GR_TEXT_HJUSTIFY_CENTER );
            m_gal->SetVerticalJustify( GR_TEXT_VJUSTIFY_TOP );
            m_gal->StrokeText( text[BELOW], pos + VECTOR2D( belowOffset, -len / 2.0 ), M_PI / 2 );
        }
        break;

    default:
        wxFAIL_MSG( "Unknown pin orientation" );
    }
}


void SCH_PAINTER::draw( LIB_BEZIER *aCurve, int aLayer )
{
    if( !isUnitAndConversionShown( aCurve ) )
        return;

    if( setDeviceColors( aCurve, aLayer ) )
    {
        BEZIER_POLY poly ( aCurve->GetPoints() );
        std::vector<wxPoint> pts;
        std::deque<VECTOR2D> pts_xformed;
        poly.GetPoly( pts );

        for( const auto &p : pts )
            pts_xformed.push_back( mapCoords( p ) );

        m_gal->DrawPolygon( pts_xformed );
    }
}


// Draw the target (an open square) for a wire or label which has no connection or is
// being moved.
void SCH_PAINTER::drawDanglingSymbol( const wxPoint& aPos, bool aDrawingShadows )
{
    wxPoint radius( DANGLING_SYMBOL_SIZE, DANGLING_SYMBOL_SIZE );

    m_gal->SetIsStroke( true );
    m_gal->SetIsFill( false );
    m_gal->SetLineWidth ( aDrawingShadows ? getShadowWidth() : 1.0F );

    m_gal->DrawRectangle( aPos - radius, aPos + radius );
}


void SCH_PAINTER::draw( SCH_JUNCTION *aJct, int aLayer )
{
    bool drawingShadows = aLayer == LAYER_SELECTION_SHADOWS;

    if( drawingShadows && !aJct->IsSelected() )
        return;

    COLOR4D color;
    auto    conn = aJct->Connection( *g_CurrentSheet );

    if( conn && conn->IsBus() )
        color = getRenderColor( aJct, LAYER_BUS, drawingShadows );
    else
        color = getRenderColor( aJct, LAYER_JUNCTION, drawingShadows );

    m_gal->SetIsStroke( drawingShadows );
    m_gal->SetLineWidth( getLineWidth( aJct, drawingShadows ) );
    m_gal->SetStrokeColor( color );
    m_gal->SetIsFill( !drawingShadows );
    m_gal->SetFillColor( color );
    m_gal->DrawCircle( aJct->GetPosition(), SCH_JUNCTION::GetEffectiveSymbolSize() / 2.0 );
}


void SCH_PAINTER::draw( SCH_LINE *aLine, int aLayer )
{
    bool drawingShadows = aLayer == LAYER_SELECTION_SHADOWS;

    if( drawingShadows && !aLine->IsSelected() )
        return;

    COLOR4D color = getRenderColor( aLine, aLine->GetLayer(), drawingShadows );
    float   width = getLineWidth( aLine, drawingShadows );

    m_gal->SetIsStroke( true );
    m_gal->SetStrokeColor( color );
    m_gal->SetLineWidth( width );

    if( aLine->GetLineStyle() <= PLOTDASHTYPE_SOLID || drawingShadows )
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

        switch( aLine->GetLineStyle() )
        {
        default:
        case PLOTDASHTYPE_DASH:
            strokes[0] = strokes[2] = DASH_MARK_LEN( width );
            break;
        case PLOTDASHTYPE_DOT:
            strokes[0] = strokes[2] = DOT_MARK_LEN( width );
            break;
        case PLOTDASHTYPE_DASHDOT:
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

    if( aLine->IsStartDangling() )
        drawDanglingSymbol( aLine->GetStartPoint(), drawingShadows );

    if( aLine->IsEndDangling() )
        drawDanglingSymbol( aLine->GetEndPoint(), drawingShadows );
}


void SCH_PAINTER::draw( SCH_TEXT *aText, int aLayer )
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

    COLOR4D         color = getRenderColor( aText, aLayer, drawingShadows );
    SCH_CONNECTION* conn = aText->Connection( *g_CurrentSheet );

    if( conn && conn->IsBus() )
        color = getRenderColor( aText, LAYER_BUS, drawingShadows );

    if( !aText->IsVisible() )
    {
        if( m_schSettings.m_ShowHiddenText )
            color = getRenderColor( aText, LAYER_HIDDEN, drawingShadows );
        else
            return;
    }

    m_gal->SetIsFill( false );
    m_gal->SetIsStroke( true );
    m_gal->SetLineWidth( getLineWidth( aText, drawingShadows ) );
    m_gal->SetStrokeColor( color );
    m_gal->SetTextAttributes( aText );

    VECTOR2D text_offset = aText->GetTextPos() + aText->GetSchematicTextOffset();
    wxString shownText( aText->GetShownText() );

    if( drawingShadows )
    {
        switch( aText->GetLabelSpinStyle() )
        {
        case 0:
            if( aText->Type() == SCH_LABEL_T || aText->Type() == SCH_TEXT_T )
                text_offset.x -= getShadowWidth() / 2;
            else
                text_offset.x += getShadowWidth() / 2;
            break;
        case 1:
            text_offset.y += getShadowWidth() / 2;
            break;
        case 2:
            if( aText->Type() == SCH_LABEL_T || aText->Type() == SCH_TEXT_T )
                text_offset.x += getShadowWidth() / 2;
            else
                text_offset.x -= getShadowWidth() / 2;
            break;
        case 3:
            text_offset.y -= getShadowWidth() / 2;
            break;
        }
    }

    if( !shownText.IsEmpty() )
        m_gal->StrokeText( shownText, text_offset, aText->GetTextAngleRadians() );

    if( aText->IsDangling() )
        drawDanglingSymbol( aText->GetTextPos(), drawingShadows );
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


void SCH_PAINTER::draw( SCH_COMPONENT *aComp, int aLayer )
{
    PART_SPTR originalPartSptr = aComp->GetPartRef().lock();

    // Use dummy part if the actual couldn't be found (or couldn't be locked).
    LIB_PART* originalPart = originalPartSptr ? originalPartSptr.get() : dummy();

    // Copy the source so we can re-orient and translate it.
    LIB_PART tempPart( *originalPart );

    tempPart.SetFlags( aComp->GetFlags() );

    orientPart( &tempPart, aComp->GetOrientation());

    for( auto& tempItem : tempPart.GetDrawItems() )
    {
        tempItem.SetFlags( aComp->GetFlags() );     // SELECTED, HIGHLIGHTED, BRIGHTENED
        tempItem.MoveTo( tempItem.GetPosition() + (wxPoint) mapCoords( aComp->GetPosition()));
    }

    // Copy the pin info from the component to the temp pins
    LIB_PINS tempPins;
    tempPart.GetPins( tempPins, aComp->GetUnit(), aComp->GetConvert() );
    const SCH_PINS& compPins = aComp->GetPins();

    for( unsigned i = 0; i < tempPins.size() && i < compPins.size(); ++ i )
    {
        LIB_PIN* tempPin = tempPins[ i ];
        const SCH_PIN& compPin = compPins[ i ];

        tempPin->ClearFlags();
        tempPin->SetFlags( compPin.GetFlags() );     // SELECTED, HIGHLIGHTED, BRIGHTENED

        if( compPin.IsDangling() )
            tempPin->SetFlags( IS_DANGLING );
    }

    draw( &tempPart, aLayer, false, aComp->GetUnit(), aComp->GetConvert() );

    // The fields are SCH_COMPONENT-specific so don't need to be copied/oriented/translated
    std::vector<SCH_FIELD*> fields;
    aComp->GetFields( fields, false );

    for( SCH_FIELD* field : fields )
        draw( field, aLayer );
}


void SCH_PAINTER::draw( SCH_FIELD *aField, int aLayer )
{
    bool drawingShadows = aLayer == LAYER_SELECTION_SHADOWS;

    if( drawingShadows && !aField->IsSelected() )
        return;

    switch( aField->GetId() )
    {
    case REFERENCE: aLayer = LAYER_REFERENCEPART; break;
    case VALUE:     aLayer = LAYER_VALUEPART;     break;
    default:        aLayer = LAYER_FIELDS;        break;
    }

    COLOR4D color = getRenderColor( aField, aLayer, drawingShadows );

    if( !aField->IsVisible() )
    {
        if( m_schSettings.m_ShowHiddenText )
            color = getRenderColor( aField, LAYER_HIDDEN, drawingShadows );
        else
            return;
    }

    if( aField->IsVoid() )
        return;

    // Calculate the text orientation according to the component orientation.
    SCH_COMPONENT* parentComponent = (SCH_COMPONENT*) aField->GetParent();
    int            orient = (int) aField->GetTextAngle();

    if( parentComponent->GetTransform().y1 )  // Rotate component 90 degrees.
    {
        if( orient == TEXT_ANGLE_HORIZ )
            orient = TEXT_ANGLE_VERT;
        else
            orient = TEXT_ANGLE_HORIZ;
    }

    /* Calculate the text justification, according to the component orientation/mirror.
     * Tthis is a bit complicated due to cumulative calculations:
     * - numerous cases (mirrored or not, rotation)
     * - the DrawGraphicText function recalculate also H and H justifications according to the
     *   text orientation.
     * - When a component is mirrored, the text is not mirrored and justifications are
     *   complicated to calculate
     * so the easier way is to use no justifications (centered text) and use GetBoundingBox
     * to know the text coordinate considered as centered
     */
    EDA_RECT boundaryBox = aField->GetBoundingBox();
    wxPoint textpos = boundaryBox.Centre();

    m_gal->SetHorizontalJustify( GR_TEXT_HJUSTIFY_CENTER );
    m_gal->SetVerticalJustify( GR_TEXT_VJUSTIFY_CENTER );
    m_gal->SetStrokeColor( color );
    m_gal->SetIsFill( false );
    m_gal->SetIsStroke( true );
    m_gal->SetGlyphSize( VECTOR2D( aField->GetTextSize() ) );
    m_gal->SetFontBold( aField->IsBold() );
    m_gal->SetFontItalic( aField->IsItalic() );
    m_gal->SetTextMirrored( aField->IsMirrored() );
    m_gal->SetLineWidth( getLineWidth( aField, drawingShadows ) );
    m_gal->StrokeText( aField->GetFullyQualifiedText(), textpos, orient == TEXT_ANGLE_VERT ? M_PI/2 : 0 );

    // Draw the umbilical line
    if( aField->IsMoving() )
    {
        m_gal->SetLineWidth( m_schSettings.m_outlineWidth );
        m_gal->SetStrokeColor( COLOR4D( 0.0, 0.0, 1.0, 1.0 ) );
        m_gal->DrawLine( textpos, parentComponent->GetPosition() );
    }
}


void SCH_PAINTER::draw( SCH_GLOBALLABEL *aLabel, int aLayer )
{
    bool drawingShadows = aLayer == LAYER_SELECTION_SHADOWS;

    if( drawingShadows && !aLabel->IsSelected() )
        return;

    COLOR4D color = getRenderColor( aLabel, LAYER_GLOBLABEL, drawingShadows );

    std::vector<wxPoint> pts;
    std::deque<VECTOR2D> pts2;

    aLabel->CreateGraphicShape( pts, aLabel->GetTextPos() );

    for( auto p : pts )
        pts2.emplace_back( VECTOR2D( p.x, p.y ) );

    m_gal->SetIsFill( true );
    m_gal->SetFillColor( m_schSettings.GetLayerColor( LAYER_SCHEMATIC_BACKGROUND ) );
    m_gal->SetIsStroke( true );
    m_gal->SetLineWidth( getTextThickness( aLabel, drawingShadows ) );
    m_gal->SetStrokeColor( color );
    m_gal->DrawPolyline( pts2 );

    draw( static_cast<SCH_TEXT*>( aLabel ), aLayer );
}


void SCH_PAINTER::draw( SCH_HIERLABEL *aLabel, int aLayer )
{
    bool drawingShadows = aLayer == LAYER_SELECTION_SHADOWS;

    if( drawingShadows && !aLabel->IsSelected() )
        return;

    COLOR4D color = getRenderColor( aLabel, LAYER_SHEETLABEL, drawingShadows );

    SCH_CONNECTION* conn = aLabel->Connection( *g_CurrentSheet );

    if( conn && conn->IsBus() )
        color = getRenderColor( aLabel, LAYER_BUS, drawingShadows );

    std::vector<wxPoint> pts;
    std::deque<VECTOR2D> pts2;

    aLabel->CreateGraphicShape( pts, aLabel->GetTextPos() );

    for( auto p : pts )
        pts2.emplace_back( VECTOR2D( p.x, p.y ) );

    m_gal->SetIsFill( true );
    m_gal->SetFillColor( m_schSettings.GetLayerColor( LAYER_SCHEMATIC_BACKGROUND ) );
    m_gal->SetIsStroke( true );
    m_gal->SetLineWidth( getTextThickness( aLabel, drawingShadows ) );
    m_gal->SetStrokeColor( color );
    m_gal->DrawPolyline( pts2 );

    draw( static_cast<SCH_TEXT*>( aLabel ), aLayer );
}

void SCH_PAINTER::draw( SCH_SHEET *aSheet, int aLayer )
{
    bool drawingShadows = aLayer == LAYER_SELECTION_SHADOWS;

    if( aLayer == LAYER_HIERLABEL || drawingShadows )
    {
        for( auto& sheetPin : aSheet->GetPins() )
        {
            if( drawingShadows && !aSheet->IsSelected() && !sheetPin.IsSelected() )
                continue;

            int     width = aSheet->GetPenSize();
            wxPoint initial_pos = sheetPin.GetTextPos();
            wxPoint offset_pos = initial_pos;

            // For aesthetic reasons, the SHEET_PIN is drawn with a small offset of width / 2
            switch( sheetPin.GetEdge() )
            {
            case SHEET_TOP_SIDE:    offset_pos.y += KiROUND( width / 2.0 ); break;
            case SHEET_BOTTOM_SIDE: offset_pos.y -= KiROUND( width / 2.0 ); break;
            case SHEET_RIGHT_SIDE:  offset_pos.x -= KiROUND( width / 2.0 ); break;
            case SHEET_LEFT_SIDE:   offset_pos.x += KiROUND( width / 2.0 ); break;
            default: break;
            }

            sheetPin.SetTextPos( offset_pos );
            draw( static_cast<SCH_HIERLABEL*>( &sheetPin ), aLayer );
            m_gal->DrawLine( offset_pos, initial_pos );
            sheetPin.SetTextPos( initial_pos );
        }
    }

    if( drawingShadows && !aSheet->IsSelected() )
        return;

    VECTOR2D pos = aSheet->GetPosition();
    VECTOR2D size = aSheet->GetSize();

    if( aLayer == LAYER_SHEET_BACKGROUND )
    {
        m_gal->SetIsStroke( aSheet->IsSelected() );
        m_gal->SetLineWidth( getShadowWidth() );
        m_gal->SetStrokeColor( getRenderColor( aSheet, LAYER_SHEET_BACKGROUND, true ) );

        if( aSheet->IsMoving() )    // Gives a filled background when moving for a better look
        {
            // Select a fill color working well with black and white background color,
            // both in Opengl and Cairo
            m_gal->SetFillColor( COLOR4D( 0.1, 0.5, 0.5, 0.3 ) );
            m_gal->SetIsFill( true );
        }
        else
        {
            // Could be modified later, when sheets can have their own fill color
            m_gal->SetIsFill( false );
        }

        m_gal->DrawRectangle( pos, pos + size );
    }

    if( aLayer == LAYER_SHEET || drawingShadows )
    {
        m_gal->SetStrokeColor( getRenderColor( aSheet, LAYER_SHEET, drawingShadows ) );
        m_gal->SetIsStroke( true );
        m_gal->SetLineWidth( getLineWidth( aSheet, drawingShadows ) );
        m_gal->SetIsFill( false );

        m_gal->DrawRectangle( pos, pos + size );

        VECTOR2D pos_sheetname = aSheet->GetSheetNamePosition();
        VECTOR2D pos_filename = aSheet->GetFileNamePosition();
        double   nameAngle = 0.0;

        if( aSheet->IsVerticalOrientation() )
            nameAngle = M_PI/2;

        if( drawingShadows )
        {
            if( aSheet->IsVerticalOrientation() )
            {
                pos_sheetname.y += getShadowWidth() / 2;
                pos_filename.y += getShadowWidth() / 2;
            }
            else
            {
                pos_sheetname.x -= getShadowWidth() / 2;
                pos_filename.x -= getShadowWidth() / 2;
            }
        }

        m_gal->SetStrokeColor( getRenderColor( aSheet, LAYER_SHEETNAME, drawingShadows ) );

        auto text = wxT( "Sheet: " ) + aSheet->GetName();

        m_gal->SetHorizontalJustify( GR_TEXT_HJUSTIFY_LEFT );
        m_gal->SetVerticalJustify( GR_TEXT_VJUSTIFY_BOTTOM );

        auto txtSize = aSheet->GetSheetNameSize();

        m_gal->SetGlyphSize( VECTOR2D( txtSize, txtSize ) );
        m_gal->SetFontBold( false );
        m_gal->SetFontItalic( false );

        m_gal->StrokeText( text, pos_sheetname, nameAngle );

        txtSize = aSheet->GetFileNameSize();
        m_gal->SetGlyphSize( VECTOR2D( txtSize, txtSize ) );
        m_gal->SetStrokeColor( getRenderColor( aSheet, LAYER_SHEETFILENAME, drawingShadows ) );
        m_gal->SetVerticalJustify( GR_TEXT_VJUSTIFY_TOP );

        text = wxT( "File: " ) + aSheet->GetFileName();
        m_gal->StrokeText( text, pos_filename, nameAngle );
    }
}


void SCH_PAINTER::draw( SCH_NO_CONNECT *aNC, int aLayer )
{
    bool drawingShadows = aLayer == LAYER_SELECTION_SHADOWS;

    if( drawingShadows && !aNC->IsSelected() )
        return;

    m_gal->SetIsStroke( true );
    m_gal->SetLineWidth( getLineWidth( aNC, drawingShadows ) );
    m_gal->SetStrokeColor( getRenderColor( aNC, LAYER_NOCONNECT, drawingShadows ) );
    m_gal->SetIsFill( false );

    VECTOR2D p = aNC->GetPosition();
    int      delta = aNC->GetSize() / 2;

    m_gal->DrawLine( p + VECTOR2D( -delta, -delta ), p + VECTOR2D( delta, delta ) );
    m_gal->DrawLine( p + VECTOR2D( -delta, delta ), p + VECTOR2D( delta, -delta ) );
}


void SCH_PAINTER::draw( SCH_BUS_ENTRY_BASE *aEntry, int aLayer )
{
    bool drawingShadows = aLayer == LAYER_SELECTION_SHADOWS;

    if( drawingShadows && !aEntry->IsSelected() )
        return;

    COLOR4D color = getRenderColor( aEntry, LAYER_WIRE, drawingShadows );

    if( aEntry->Type() == SCH_BUS_BUS_ENTRY_T )
        color = getRenderColor( aEntry, LAYER_BUS, drawingShadows );

    m_gal->SetIsStroke( true );
    m_gal->SetLineWidth( getLineWidth( aEntry, drawingShadows ) );
    m_gal->SetStrokeColor( color );
    m_gal->SetIsFill( false );

    VECTOR2D pos = aEntry->GetPosition();
    VECTOR2D endPos = aEntry->m_End();

    m_gal->DrawLine( pos, endPos );

    // Draw dangling symbols:
    m_gal->SetLineWidth ( getLineWidth( aEntry, drawingShadows ) );

    if( aEntry->IsDanglingStart() )
        m_gal->DrawCircle( pos, TARGET_BUSENTRY_RADIUS );

    if( aEntry->IsDanglingEnd() )
        m_gal->DrawCircle( endPos, TARGET_BUSENTRY_RADIUS );
}


void SCH_PAINTER::draw( SCH_BITMAP *aBitmap, int aLayer )
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
        if( aBitmap->IsSelected() || aBitmap->IsBrightened() || aBitmap->IsHighlighted() )
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


void SCH_PAINTER::draw( SCH_MARKER *aMarker, int aLayer )
{
    bool drawingShadows = aLayer == LAYER_SELECTION_SHADOWS;

    if( drawingShadows && !aMarker->IsSelected() )
        return;

    if( aMarker->GetErrorLevel() == MARKER_BASE::MARKER_SEVERITY_ERROR )
        aLayer = LAYER_ERC_ERR;
    else
        aLayer = LAYER_ERC_WARN;

    COLOR4D color = getRenderColor( aMarker, aLayer, drawingShadows );

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
