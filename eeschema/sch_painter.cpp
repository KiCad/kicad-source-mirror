/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2014 CERN
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


/// @ todo: this is work in progress. Cleanup will follow.

#include <sch_item_struct.h>

#include <lib_draw_item.h>
#include <lib_rectangle.h>
#include <lib_pin.h>
#include <lib_circle.h>
#include <lib_polyline.h>
#include <lib_arc.h>
#include <lib_field.h>
#include <lib_text.h>
#include <sch_line.h>
#include <sch_component.h>
#include <sch_field.h>
#include <sch_junction.h>
#include <sch_text.h>
#include <sch_no_connect.h>
#include <sch_bus_entry.h>
#include <sch_bitmap.h>
#include <draw_graphic_text.h>

#include <lib_edit_frame.h>

#include <template_fieldnames.h>
#include <class_libentry.h>
#include <class_library.h>
#include <sch_edit_frame.h>

#include <gal/graphics_abstraction_layer.h>
#include <colors_design_settings.h>

#include "sch_painter.h"

#include <draw_graphic_text.h>

namespace KIGFX
{

SCH_RENDER_SETTINGS::SCH_RENDER_SETTINGS() :
    m_ShowUnit( 0 ),
    m_ShowConvert( 0 )
{
    ImportLegacyColors( nullptr );
}

void SCH_RENDER_SETTINGS::ImportLegacyColors( const COLORS_DESIGN_SETTINGS* aSettings )
{
    for( int layer = SCH_LAYER_ID_START; layer < SCH_LAYER_ID_END; layer ++)
    {
        m_layerColors[ layer ] = ::GetLayerColor( static_cast<SCH_LAYER_ID>( layer ) );
    }

    for( int layer = GAL_LAYER_ID_START; layer < GAL_LAYER_ID_END; layer ++)
    {
        m_layerColors[ layer ] = ::GetLayerColor( static_cast<SCH_LAYER_ID>( layer ) );
    }

    m_backgroundColor = ::GetLayerColor( LAYER_SCHEMATIC_BACKGROUND );
}

const COLOR4D& SCH_RENDER_SETTINGS::GetColor( const VIEW_ITEM* aItem, int aLayer ) const
{
  return m_layerColors[ aLayer ];
}


static COLOR4D selectedBrightening( const COLOR4D& aColor )
{
    return aColor.Brightened( 0.5 );
}


/**
 * Used when a LIB_PART is not found in library to draw a dummy shape.
 * This component is a 400 mils square with the text "??"
 * DEF DUMMY U 0 40 Y Y 1 0 N
 * F0 "U" 0 -350 60 H V
 * F1 "DUMMY" 0 350 60 H V
 * DRAW
 * T 0 0 0 150 0 0 0 ??
 * S -200 200 200 -200 0 1 0
 * ENDDRAW
 * ENDDEF
 */
static LIB_PART* dummy()
{
    static LIB_PART* part;

    if( !part )
    {
        part = new LIB_PART( wxEmptyString );

        LIB_RECTANGLE* square = new LIB_RECTANGLE( part );

        square->Move( wxPoint( -200, 200 ) );
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


#define HANDLE_ITEM(type_id, type_name) \
  case type_id: \
    draw( (type_name *) item, aLayer ); \
    break;

bool SCH_PAINTER::Draw( const VIEW_ITEM *aItem, int aLayer )
{
	auto item2 = static_cast<const EDA_ITEM *>(aItem);
    auto item = const_cast<EDA_ITEM*>(item2);


    //printf("Import\n");
    m_schSettings.ImportLegacyColors( nullptr );

    //auto c =GetLayerColor( LAYER_SCHEMATIC_BACKGROUND );
    //printf("bkgd %.02f %.02f %.02f %.02f\n", c.r, c.g, c.b, c.a);
    //auto c2 = m_schSettings.GetLayerColor ( LAYER_SCHEMATIC_BACKGROUND );
    //printf("bkgd2 %.02f %.02f %.02f %.02f\n", c2.r, c2.g, c2.b, c2.a);

    m_gal->EnableDepthTest( false );

/*    m_gal->SetLineWidth( 10 );
    m_gal->SetIsFill( false );
    m_gal->SetIsStroke (true);
    m_gal->SetStrokeColor( COLOR4D(0.0, 0.0, 0.0, 1.0) );
    m_gal->SetGlyphSize ( VECTOR2D(100,100) );

    m_gal->SetHorizontalJustify( GR_TEXT_HJUSTIFY_CENTER );
    m_gal->SetVerticalJustify( GR_TEXT_VJUSTIFY_CENTER );

    m_gal->StrokeText( wxT("Test"), VECTOR2D(0, 0), 0.0 );
*/
	switch(item->Type())
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
    HANDLE_ITEM(SCH_COMPONENT_T, SCH_COMPONENT);
    HANDLE_ITEM(SCH_JUNCTION_T, SCH_JUNCTION);
    HANDLE_ITEM(SCH_LINE_T, SCH_LINE);
    HANDLE_ITEM(SCH_TEXT_T, SCH_TEXT);
    HANDLE_ITEM(SCH_LABEL_T, SCH_TEXT);
    HANDLE_ITEM(SCH_FIELD_T, SCH_FIELD);
    HANDLE_ITEM(SCH_HIERARCHICAL_LABEL_T, SCH_HIERLABEL);
    HANDLE_ITEM(SCH_GLOBAL_LABEL_T, SCH_GLOBALLABEL);
    HANDLE_ITEM(SCH_SHEET_T, SCH_SHEET);
    HANDLE_ITEM(SCH_NO_CONNECT_T, SCH_NO_CONNECT);
    HANDLE_ITEM(SCH_BUS_WIRE_ENTRY_T, SCH_BUS_ENTRY_BASE);
    HANDLE_ITEM(SCH_BUS_BUS_ENTRY_T, SCH_BUS_ENTRY_BASE);
    HANDLE_ITEM(SCH_BITMAP_T, SCH_BITMAP);
    HANDLE_ITEM(SCH_MARKER_T, SCH_MARKER);

    default:
        return false;
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


void SCH_PAINTER::draw( LIB_PART *aComp, int aLayer, bool aDrawFields, int aUnit, int aConvert,
                       std::vector<bool>* danglingPinFlags )
{
    if( !aUnit )
        aUnit = m_schSettings.m_ShowUnit;

    if( !aConvert )
        aConvert = m_schSettings.m_ShowConvert;

    size_t pinIndex = 0;

    auto visitItem = [&]( LIB_ITEM& item, bool aBackground )
    {
        if( aBackground != ( item.GetFillMode() == FILLED_WITH_BG_BODYCOLOR ) )
            return;

        if( !aDrawFields && item.Type() == LIB_FIELD_T )
            return;

        if( aUnit && item.GetUnit() && aUnit != item.GetUnit() )
            return;

        if( aConvert && item.GetConvert() && aConvert != item.GetConvert() )
            return;

        if( item.Type() == LIB_PIN_T )
        {
            auto pin = static_cast<LIB_PIN*>( &item );
            bool dangling = true;

            if( danglingPinFlags && pinIndex < danglingPinFlags->size() )
                dangling = (*danglingPinFlags)[ pinIndex ];

            draw( pin, aLayer, dangling, aComp->IsMoving() );
            pinIndex++;
        }
        else
            Draw( &item, aLayer );
    };

    // Apply a z-order heuristic (because we don't yet let the user edit it):
    // draw body-filled objects first.

    for( auto& item : aComp->GetDrawItems() )
        visitItem( item, true );

    for( auto& item : aComp->GetDrawItems() )
        visitItem( item, false );
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


void SCH_PAINTER::draw( LIB_RECTANGLE *aRect, int aLayer )
{
    if( !isUnitAndConversionShown( aRect ) )
        return;

    defaultColors(aRect);

    //m_gal->SetIsStroke( true );
    m_gal->SetLineWidth( aRect->GetPenSize() );
    m_gal->DrawRectangle( mapCoords( aRect->GetPosition() ), mapCoords( aRect->GetEnd() ) );

}

void SCH_PAINTER::triLine ( const VECTOR2D &a, const VECTOR2D &b, const VECTOR2D &c )
{
  m_gal->DrawLine ( a, b );
  m_gal->DrawLine ( b, c );
}


void SCH_PAINTER::defaultColors( const LIB_ITEM *aItem )
{
    COLOR4D fg = m_schSettings.GetLayerColor( LAYER_DEVICE );
    COLOR4D bg = m_schSettings.GetLayerColor( LAYER_DEVICE_BACKGROUND );

    if( aItem->IsMoving() )
    {
        fg = selectedBrightening( fg );
        bg = bg.Saturate( 0.7 ).WithAlpha( 0.66 );
    }

    m_gal->SetIsStroke( true );
    m_gal->SetStrokeColor( fg );
    m_gal->SetLineWidth( aItem->GetPenSize() );

    switch( aItem->GetFillMode() )
    {
    case FILLED_WITH_BG_BODYCOLOR:
        m_gal->SetIsFill( true );
        m_gal->SetFillColor( bg );
        break;

    case FILLED_SHAPE:
        m_gal->SetIsFill( true );
        m_gal->SetFillColor( fg );
        break;

    default:
        m_gal->SetIsFill( false );
    }
}


void SCH_PAINTER::draw( LIB_CIRCLE *aCircle, int aLayer )
{
    if( !isUnitAndConversionShown( aCircle ) )
        return;

    defaultColors(aCircle);

    m_gal->DrawCircle( mapCoords( aCircle->GetPosition() ), aCircle->GetRadius() );
}

void SCH_PAINTER::draw( LIB_ARC *aArc, int aLayer )
{
    if( !isUnitAndConversionShown( aArc ) )
        return;

    defaultColors(aArc);

    int sai = aArc->GetFirstRadiusAngle();
    int eai = aArc->GetSecondRadiusAngle();

    if (TRANSFORM().MapAngles( &sai, &eai ))
        std::swap(sai, eai);

    double sa = (double) sai * M_PI / 1800.0;
    double ea = (double) eai * M_PI / 1800.0 ;

    VECTOR2D pos = mapCoords( aArc->GetPosition() );

    m_gal->DrawArc( pos, aArc->GetRadius(), sa, ea);
    /*m_gal->SetStrokeColor(COLOR4D(1.0,0,0,1.0));
    m_gal->DrawLine ( pos - VECTOR2D(20, 20), pos + VECTOR2D(20, 20));
    m_gal->DrawLine ( pos - VECTOR2D(-20, 20), pos + VECTOR2D(-20, 20));*/
}


void SCH_PAINTER::draw( LIB_FIELD *aField, int aLayer )
{
    if( !isUnitAndConversionShown( aField ) )
        return;

    COLOR4D color;

    switch( aField->GetId() )
    {
    case REFERENCE: color = m_schSettings.GetLayerColor( LAYER_REFERENCEPART ); break;
    case VALUE:     color = m_schSettings.GetLayerColor( LAYER_VALUEPART );     break;
    default:        color = m_schSettings.GetLayerColor( LAYER_FIELDS );        break;
    }

    if( !aField->IsVisible() )
    {
        if( m_schSettings.m_ShowHiddenText )
            color = m_schSettings.GetLayerColor( LAYER_HIDDEN );
        else
          return;
    }
    else if( aField->IsMoving() )
    {
        color = selectedBrightening( color );
    }

    int linewidth = aField->GetPenSize();

    if( aField->IsBold() )
        linewidth = GetPenSizeForBold( aField->GetTextWidth() );

    Clamp_Text_PenSize( linewidth, aField->GetTextSize(), aField->IsBold() );

    m_gal->SetLineWidth( linewidth );
    m_gal->SetIsFill( false );
    m_gal->SetIsStroke( true );
    m_gal->SetStrokeColor( color );
    m_gal->SetGlyphSize( VECTOR2D( aField->GetTextSize() ) );

    m_gal->SetHorizontalJustify( aField->GetHorizJustify( ) );
    m_gal->SetVerticalJustify( aField->GetVertJustify( ) );

    auto pos = mapCoords( aField->GetPosition() );
    double orient = aField->GetTextAngleRadians();

    m_gal->StrokeText( aField->GetText(), pos, orient );
}


void SCH_PAINTER::draw( LIB_POLYLINE *aLine, int aLayer )
{
    if( !isUnitAndConversionShown( aLine ) )
        return;

    defaultColors( aLine );

    std::deque<VECTOR2D> vtx;

    for( auto p : aLine->GetPolyPoints() )
        vtx.push_back ( mapCoords( p ) );

    if( aLine->GetFillMode() == FILLED_WITH_BG_BODYCOLOR || aLine->GetFillMode() == FILLED_SHAPE )
        vtx.push_back ( vtx[0] );

    m_gal->DrawPolygon ( vtx );
}


void SCH_PAINTER::draw( LIB_TEXT *aText, int aLayer )
{
    if( !isUnitAndConversionShown( aText ) )
        return;

    COLOR4D color = m_schSettings.GetLayerColor( LAYER_NOTES );

    if( !aText->IsVisible() )
    {
        color = m_schSettings.GetLayerColor( LAYER_HIDDEN );

        if( !m_schSettings.m_ShowHiddenText )
            return;
    }
    else if( aText->IsMoving() )
    {
        color = selectedBrightening( color );
    }

    int w = aText->GetPenSize();
    EDA_RECT bBox = aText->GetBoundingBox();
    bBox.RevertYAxis();
    VECTOR2D pos = mapCoords( bBox.Centre() );
    double orient = aText->GetTextAngleRadians();

    m_gal->SetHorizontalJustify( GR_TEXT_HJUSTIFY_CENTER );
    m_gal->SetVerticalJustify( GR_TEXT_VJUSTIFY_CENTER );
    m_gal->SetLineWidth( w );
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

/// Utility for getting the size of the 'external' pin decorators (as a radius)
// i.e. the negation circle, the polarity 'slopes' and the nonlogic
// marker
static int ExternalPinDecoSize( const LIB_PIN &aPin )
{
    return aPin.GetNumberTextSize() / 2;
}


void SCH_PAINTER::draw( LIB_PIN *aPin, int aLayer, bool isDangling, bool isMoving )
{
    if( !isUnitAndConversionShown( aPin ) )
        return;

    COLOR4D color = m_schSettings.GetLayerColor( LAYER_PIN );

    if( !aPin->IsVisible() )
    {
        color = m_schSettings.GetLayerColor( LAYER_HIDDEN );

        if( !m_schSettings.m_ShowHiddenPins )
            return;
    }
    else if( isMoving )
    {
        color = selectedBrightening( color );
    }

	VECTOR2I pos = mapCoords( aPin->GetPosition() );
    VECTOR2I p0, dir;
	int len = aPin->GetLength();
	int width = aPin->GetPenSize();
	int shape = aPin->GetShape();
    int orient = aPin->GetOrientation();

    switch( orient )
	{
		case PIN_UP:
            //printf("pinUp\n");
			p0 = VECTOR2I( pos.x, pos.y - len );
			dir = VECTOR2I(0, 1);
			break;
		case PIN_DOWN:
            //printf("pinDown\n");
			p0 = VECTOR2I( pos.x, pos.y + len );
			dir = VECTOR2I(0, -1);
			break;
		case PIN_LEFT:
            //printf("pinLeft\n");
			p0 = VECTOR2I( pos.x - len, pos.y );
			dir = VECTOR2I(1, 0);
			break;
		case PIN_RIGHT:
            //printf("pinRight\n");
            p0 = VECTOR2I( pos.x + len, pos.y );
			dir = VECTOR2I(-1, 0);
			break;
	}

//printf("pin %p p0 %d %d pos %d %d len %d\n", aPin, p0.x, p0.y, pos.x, pos.y, len);

    VECTOR2D pc;

    m_gal->SetIsStroke( true );
    m_gal->SetIsFill( false );
    m_gal->SetLineWidth( width );
    m_gal->SetStrokeColor( color );
    m_gal->SetFontBold( false );
    m_gal->SetFontItalic( false );

    const int radius = ExternalPinDecoSize( *aPin );
    const int clock_size = InternalPinDecoSize( *aPin );

	if( shape == PINSHAPE_INVERTED )
	{
		m_gal->DrawCircle( p0 + dir * radius, radius );
		m_gal->DrawLine( p0 + dir * ( 2 * radius ), pos );
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
        //printf("DrawLPin\n");
        m_gal->DrawLine( p0, pos );
        //m_gal->DrawLine( p0, pos+dir.Perpendicular() * radius);
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
            triLine ( p0 + VECTOR2D(dir.x, 0) * radius * 2,
                      p0 + VECTOR2D(dir.x, -1) * radius * 2,
                      p0 );
        }
        else    /* MapX1 = 0 */
        {
            triLine ( p0 + VECTOR2D( 0, dir.y) * radius * 2,
                      p0 + VECTOR2D(-1, dir.y) * radius * 2,
                      p0 );
        }
    }

    if( shape == PINSHAPE_OUTPUT_LOW )    /* IEEE symbol "Active Low Output" */
    {
        if( !dir.y )
            m_gal->DrawLine( p0 - VECTOR2D( 0, radius ), p0 + VECTOR2D( dir.x, 1 ) * radius * 2 );
        else
            m_gal->DrawLine( p0 - VECTOR2D( radius, 0 ), p0 + VECTOR2D( 0, dir.y ) * radius * 2 );
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
    }

    if( ( isDangling || isMoving ) && ( aPin->IsVisible() || aPin->IsPowerConnection() ) )
    {
        m_gal->SetLineWidth ( 1.0 );
        m_gal->DrawCircle( pos, TARGET_PIN_RADIUS );
    }

// Draw the labels

    LIB_PART* libEntry = aPin->GetParent();
    int textOffset = libEntry->GetPinNameOffset();

    int nameLineWidth = aPin->GetPenSize();
    nameLineWidth = Clamp_Text_PenSize( nameLineWidth, aPin->GetNameTextSize(), false );
    int numLineWidth = aPin->GetPenSize();
    numLineWidth = Clamp_Text_PenSize( numLineWidth, aPin->GetNumberTextSize(), false );

    #define PIN_TEXT_MARGIN 4

    // Four locations around a pin where text can be drawn
    enum { INSIDE = 0, OUTSIDE, ABOVE, BELOW };
    int size[4] = { 0, 0, 0, 0 };
    int thickness[4];
    COLOR4D colour[4];
    wxString text[4];

    // TextOffset > 0 means pin NAMES on inside, pin NUMBERS above and nothing below
    if( textOffset )
    {
        size     [INSIDE] = libEntry->ShowPinNames() ? aPin->GetNameTextSize() : 0;
        thickness[INSIDE] = nameLineWidth;
        colour   [INSIDE] = m_schSettings.GetLayerColor( LAYER_PINNAM );
        text     [INSIDE] = aPin->GetName();

        size     [ABOVE] = libEntry->ShowPinNumbers() ? aPin->GetNumberTextSize() : 0;
        thickness[ABOVE] = numLineWidth;
        colour   [ABOVE] = m_schSettings.GetLayerColor( LAYER_PINNUM );
        text     [ABOVE] = aPin->GetNumber();
    }
    // Otherwise pin NAMES go above and pin NUMBERS go below
    else
    {
        size     [ABOVE] = libEntry->ShowPinNames() ? aPin->GetNameTextSize() : 0;
        thickness[ABOVE] = nameLineWidth;
        colour   [ABOVE] = m_schSettings.GetLayerColor( LAYER_PINNAM );
        text     [ABOVE] = aPin->GetName();

        size     [BELOW] = libEntry->ShowPinNumbers() ? aPin->GetNumberTextSize() : 0;
        thickness[BELOW] = numLineWidth;
        colour   [BELOW] = m_schSettings.GetLayerColor( LAYER_PINNUM );
        text     [BELOW] = aPin->GetNumber();
    }

    if( m_schSettings.m_ShowPinsElectricalType )
    {
        size     [OUTSIDE] = std::max( aPin->GetNameTextSize() * 3 / 4, Millimeter2iu( 0.7 ) );
        thickness[OUTSIDE] = size[OUTSIDE] / 6;
        colour   [OUTSIDE] = m_schSettings.GetLayerColor( LAYER_NOTES );
        text     [OUTSIDE] = aPin->GetElectricalTypeName();
    }

    if( !aPin->IsVisible() )
    {
        for( COLOR4D& c : colour )
            c = m_schSettings.GetLayerColor( LAYER_HIDDEN );
    }

    int insideOffset = textOffset;
    int outsideOffset = 10;
    int aboveOffset = PIN_TEXT_MARGIN + ( thickness[ABOVE] + GetDefaultLineThickness() ) / 2;
    int belowOffset = PIN_TEXT_MARGIN + ( thickness[BELOW] + GetDefaultLineThickness() ) / 2;

    #define SET_DC( i ) \
        m_gal->SetGlyphSize( VECTOR2D( size[i], size[i] ) ); \
        m_gal->SetLineWidth( thickness[i] ); \
        m_gal->SetStrokeColor( colour[i] );

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
            m_gal->StrokeText( text[ABOVE], pos + VECTOR2D( -len / 2, -aboveOffset ), 0 );
        }
        if( size[BELOW] )
        {
            SET_DC( BELOW );
            m_gal->SetHorizontalJustify( GR_TEXT_HJUSTIFY_CENTER );
            m_gal->SetVerticalJustify( GR_TEXT_VJUSTIFY_TOP );
            m_gal->StrokeText( text[BELOW], pos + VECTOR2D( -len / 2, belowOffset ), 0 );
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
            m_gal->StrokeText( text[ABOVE], pos + VECTOR2D( len / 2, -aboveOffset ), 0 );
        }
        if( size[BELOW] )
        {
            SET_DC( BELOW );
            m_gal->SetHorizontalJustify( GR_TEXT_HJUSTIFY_CENTER );
            m_gal->SetVerticalJustify( GR_TEXT_VJUSTIFY_TOP );
            m_gal->StrokeText( text[BELOW], pos + VECTOR2D( len / 2, belowOffset ), 0 );
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
            m_gal->StrokeText( text[ABOVE], pos + VECTOR2D( -aboveOffset, len / 2 ), M_PI / 2 );
        }
        if( size[BELOW] )
        {
            SET_DC( BELOW );
            m_gal->SetHorizontalJustify( GR_TEXT_HJUSTIFY_CENTER );
            m_gal->SetVerticalJustify( GR_TEXT_VJUSTIFY_TOP );
            m_gal->StrokeText( text[BELOW], pos + VECTOR2D( belowOffset, len / 2 ), M_PI / 2 );
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
            m_gal->StrokeText( text[ABOVE], pos + VECTOR2D( -aboveOffset, -len / 2 ), M_PI / 2 );
        }
        if( size[BELOW] )
        {
            SET_DC( BELOW );
            m_gal->SetHorizontalJustify( GR_TEXT_HJUSTIFY_CENTER );
            m_gal->SetVerticalJustify( GR_TEXT_VJUSTIFY_TOP );
            m_gal->StrokeText( text[BELOW], pos + VECTOR2D( belowOffset, -len / 2 ), M_PI / 2 );
        }
        break;
    }
}


static void drawDanglingSymbol( GAL* aGal, const wxPoint& aPos )
{
    wxPoint radius( DANGLING_SYMBOL_SIZE, DANGLING_SYMBOL_SIZE );
    aGal->SetLineWidth ( 1.0 );
    aGal->DrawRectangle( aPos - radius, aPos + radius );
}


void SCH_PAINTER::draw( SCH_JUNCTION *aJct, int aLayer )
{
    COLOR4D color = m_schSettings.GetLayerColor( LAYER_JUNCTION );

    if( aJct->IsMoving() )
        color = selectedBrightening( color );

    m_gal->SetIsStroke(true);
    m_gal->SetIsFill(true);
    m_gal->SetStrokeColor( color );
    m_gal->SetFillColor( color );
    m_gal->DrawCircle( aJct->GetPosition(),  aJct->GetSymbolSize() / 2 );
}


void SCH_PAINTER::draw( SCH_LINE *aLine, int aLayer )
{
    COLOR4D color = m_schSettings.GetLayerColor( LAYER_WIRE );

    if( aLine->GetState( BRIGHTENED ) )
        color = m_schSettings.GetLayerColor( LAYER_BRIGHTENED );
    else if( aLine->IsMoving() )
        color = selectedBrightening( color );

    int width = aLine->GetPenSize();

    m_gal->SetIsStroke(true);
    m_gal->SetStrokeColor(color);
    m_gal->SetLineWidth( width );
    m_gal->DrawLine( aLine->GetStartPoint(), aLine->GetEndPoint() );

    if( aLine->IsStartDangling() )
        drawDanglingSymbol( m_gal, aLine->GetStartPoint());

    if( aLine->IsEndDangling() )
        drawDanglingSymbol( m_gal, aLine->GetEndPoint());
}


void SCH_PAINTER::draw( SCH_TEXT *aText, int aLayer )
{
    COLOR4D color;

    switch( aText->Type() )
    {
    case SCH_HIERARCHICAL_LABEL_T: color = m_schSettings.GetLayerColor( LAYER_SHEETLABEL ); break;
    case SCH_GLOBAL_LABEL_T:       color = m_schSettings.GetLayerColor( LAYER_GLOBLABEL );  break;
    default:                       color = m_schSettings.GetLayerColor( LAYER_NOTES );      break;
    }

    if( !aText->IsVisible() )
    {
        color = m_schSettings.GetLayerColor( LAYER_HIDDEN );

        if( !m_schSettings.m_ShowHiddenText )
            return;
    }
    else if( aText->IsMoving() )
    {
        color = selectedBrightening( color );
    }

    if( aText->IsDangling() )
        drawDanglingSymbol( m_gal, aText->GetTextPos());

    wxPoint  text_offset = aText->GetTextPos() + aText->GetSchematicTextOffset();
    int      linewidth = aText->GetThickness() ? aText->GetThickness() : GetDefaultLineThickness();
    wxString shownText( aText->GetShownText() );

    linewidth = Clamp_Text_PenSize( linewidth, aText->GetTextSize(), aText->IsBold() );

    if( !shownText.IsEmpty() )
    {
        m_gal->SetIsFill( false );
        m_gal->SetIsStroke( true );
        m_gal->SetStrokeColor( color );
        m_gal->SetTextAttributes( aText );
        m_gal->SetLineWidth( linewidth );
        m_gal->StrokeText( shownText, text_offset, aText->GetTextAngleRadians() );
    }
}


static void orientComponent( LIB_PART *part, int orientation )
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

    //printf("orient %d %d %d\n", o.n_rots, o.mirror_x, o.mirror_y );

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
    PART_SPTR part = aComp->GetPartRef().lock();

    // Use dummy part if the actual couldn't be found (or couldn't be locked).
    // In either case copy it so we can re-orient and translate it.
    std::unique_ptr<LIB_PART> ptrans( new LIB_PART( part ? *part.get() : *dummy() ) );

    if( aComp->IsMoving() )
        ptrans->SetFlags( IS_MOVED );

    orientComponent( ptrans.get(), aComp->GetOrientation() );

    for( auto& item : ptrans->GetDrawItems() )
    {
        if( aComp->IsMoving() )
            item.SetFlags( IS_MOVED );

        auto rp = aComp->GetPosition();
        auto ip = item.GetPosition();
        item.Move( wxPoint( rp.x + ip.x, ip.y - rp.y ) );
    }

    draw( ptrans.get(), aLayer, false,
          aComp->GetUnit(), aComp->GetConvert(), aComp->GetDanglingPinFlags() );

    // The fields are SCH_COMPONENT-specific and so don't need to be copied/
    // oriented/translated.
    std::vector<SCH_FIELD*> fields;
    aComp->GetFields( fields, false );

    for( SCH_FIELD* field : fields )
    {
        if( field->GetId() == REFERENCE || !field->IsMoving() )
            draw( field, aLayer );
    }
}


void SCH_PAINTER::draw( SCH_FIELD *aField, int aLayer )
{
    int            orient;
    COLOR4D        color;
    wxPoint        textpos;
    SCH_COMPONENT* parentComponent = (SCH_COMPONENT*) aField->GetParent();
    int            lineWidth = aField->GetPenSize();

    switch( aField->GetId() )
    {
    case REFERENCE: color = m_schSettings.GetLayerColor( LAYER_REFERENCEPART ); break;
    case VALUE:     color = m_schSettings.GetLayerColor( LAYER_VALUEPART );     break;
    default:        color = m_schSettings.GetLayerColor( LAYER_FIELDS );        break;
    }

    if( !aField->IsVisible() )
    {
        color = m_schSettings.GetLayerColor( LAYER_HIDDEN );

        if( !m_schSettings.m_ShowHiddenText )
            return;
    }
    else if( aField->IsMoving() )
    {
        color = selectedBrightening( color );
    }

    if( aField->IsVoid() )
        return;

    if( aField->IsBold() )
        lineWidth = GetPenSizeForBold( aField->GetTextWidth() );

    // Clip pen size for small texts:
    lineWidth = Clamp_Text_PenSize( lineWidth, aField->GetTextSize(), aField->IsBold() );

    // Calculate the text orientation according to the component orientation.
    orient = aField->GetTextAngle();

    if( parentComponent->GetTransform().y1 )  // Rotate component 90 degrees.
    {
        if( orient == TEXT_ANGLE_HORIZ )
            orient = TEXT_ANGLE_VERT;
        else
            orient = TEXT_ANGLE_HORIZ;
    }

    /* Calculate the text justification, according to the component
     * orientation/mirror this is a bit complicated due to cumulative
     * calculations:
     * - numerous cases (mirrored or not, rotation)
     * - the DrawGraphicText function recalculate also H and H justifications
     *      according to the text orientation.
     * - When a component is mirrored, the text is not mirrored and
     *   justifications are complicated to calculate
     * so the more easily way is to use no justifications ( Centered text )
     * and use GetBoundaryBox to know the text coordinate considered as centered
     */
    EDA_RECT boundaryBox = aField->GetBoundingBox();
    textpos = boundaryBox.Centre();

    m_gal->SetHorizontalJustify( GR_TEXT_HJUSTIFY_CENTER );
    m_gal->SetVerticalJustify( GR_TEXT_VJUSTIFY_CENTER );
    m_gal->SetStrokeColor( color );
    m_gal->SetIsFill( false );
    m_gal->SetIsStroke( true );
    m_gal->SetGlyphSize( VECTOR2D( aField->GetTextSize() ) );
    m_gal->SetFontBold( aField->IsBold() );
    m_gal->SetFontItalic( aField->IsItalic() );
    m_gal->SetTextMirrored( aField->IsMirrored() );
    m_gal->SetLineWidth( lineWidth );
    m_gal->StrokeText( aField->GetFullyQualifiedText(), textpos, orient == TEXT_ANGLE_VERT ? M_PI/2 : 0 );
}


void SCH_PAINTER::draw( SCH_GLOBALLABEL *aLabel, int aLayer )
{
    COLOR4D color = m_schSettings.GetLayerColor( LAYER_GLOBLABEL );

    if( aLabel->IsMoving() )
        color = selectedBrightening( color );

    std::vector<wxPoint> pts;
    std::deque<VECTOR2D> pts2;

    aLabel->CreateGraphicShape( pts, aLabel->GetTextPos() );

    for( auto p : pts )
        pts2.push_back( VECTOR2D( p.x, p.y ) );

    m_gal->SetIsFill( false );
    m_gal->SetIsStroke( true );
    m_gal->SetLineWidth( aLabel->GetThickness() );
    m_gal->SetStrokeColor( color );
    m_gal->DrawPolyline( pts2 );

    draw( static_cast<SCH_TEXT*>( aLabel ), aLayer );
}


void SCH_PAINTER::draw( SCH_HIERLABEL *aLabel, int aLayer )
{
    COLOR4D color = m_schSettings.GetLayerColor( LAYER_SHEETLABEL );

    if( aLabel->IsMoving() )
        color = selectedBrightening( color );

    std::vector<wxPoint> pts;
    std::deque<VECTOR2D> pts2;

    aLabel->CreateGraphicShape( pts, aLabel->GetTextPos() );

    for( auto p : pts )
        pts2.push_back( VECTOR2D( p.x, p.y ) );

    m_gal->SetIsFill( false );
    m_gal->SetIsStroke( true );
    m_gal->SetLineWidth( aLabel->GetThickness() );
    m_gal->SetStrokeColor( color );
    m_gal->DrawPolyline( pts2 );

    draw( static_cast<SCH_TEXT*>( aLabel ), aLayer );
}

void SCH_PAINTER::draw( SCH_SHEET *aSheet, int aLayer )
{
    VECTOR2D pos_sheetname = aSheet->GetSheetNamePosition();
    VECTOR2D pos_filename = aSheet->GetFileNamePosition();
    VECTOR2D pos = aSheet->GetPosition();
    VECTOR2D size = aSheet->GetSize();

    m_gal->SetStrokeColor( m_schSettings.GetLayerColor( LAYER_SHEET ) );
    m_gal->SetFillColor ( COLOR4D(1.0, 1.0, 1.0, 0.5) );
    m_gal->SetIsStroke ( true );
    m_gal->SetIsFill ( true );
    m_gal->DrawRectangle( pos, pos + size );

    auto nameAngle = 0.0;

    if( aSheet->IsVerticalOrientation() )
        nameAngle = -M_PI/2;

    m_gal->SetStrokeColor( m_schSettings.GetLayerColor( LAYER_SHEETNAME ) );

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
    m_gal->SetStrokeColor( m_schSettings.GetLayerColor( LAYER_SHEETFILENAME ) );
    m_gal->SetVerticalJustify( GR_TEXT_VJUSTIFY_TOP );

    text = wxT( "File: " ) + aSheet->GetFileName();
    m_gal->StrokeText( text, pos_filename, nameAngle );

    for( auto& sheetPin : aSheet->GetPins() )
        draw( static_cast<SCH_HIERLABEL*>( &sheetPin ), aLayer );
}


void SCH_PAINTER::draw( SCH_NO_CONNECT *aNC, int aLayer )
{
    int delta = aNC->GetSize().x / 2;
    int width = GetDefaultLineThickness();

    m_gal->SetStrokeColor( m_schSettings.GetLayerColor( LAYER_NOCONNECT ) );
    m_gal->SetIsStroke( true );
    m_gal->SetIsFill( false );
    m_gal->SetLineWidth( width );

    VECTOR2D p = aNC->GetPosition();

    m_gal->DrawLine( p + VECTOR2D(-delta, -delta), p+VECTOR2D( delta, delta ) );
    m_gal->DrawLine( p + VECTOR2D(-delta, delta), p+VECTOR2D( delta, -delta ) );
}


void SCH_PAINTER::draw( SCH_BUS_ENTRY_BASE *aEntry, int aLayer )
{
    COLOR4D color = m_schSettings.GetLayerColor( LAYER_BUS );

    if( aEntry->IsMoving() )
        color = selectedBrightening( color );

    m_gal->SetStrokeColor( color );
    m_gal->SetIsStroke( true );
    m_gal->SetLineWidth( aEntry->GetPenSize() );
    m_gal->SetIsFill( false );

    VECTOR2D pos = aEntry->GetPosition();
    VECTOR2D endPos = aEntry->m_End();

    m_gal->DrawLine( pos, endPos );

    m_gal->SetStrokeColor( m_schSettings.GetLayerColor( LAYER_BUS ) );

    if( aEntry->IsDanglingStart() )
        m_gal->DrawCircle( pos, TARGET_BUSENTRY_RADIUS );

    if( aEntry->IsDanglingEnd() )
        m_gal->DrawCircle( endPos, TARGET_BUSENTRY_RADIUS );
}


void SCH_PAINTER::draw( SCH_BITMAP *aBitmap, int aLayer )
{
    m_gal->Save();
    m_gal->Translate( aBitmap->GetPosition() );
    m_gal->DrawBitmap( *aBitmap->GetImage() );
    m_gal->Restore();
}


void SCH_PAINTER::draw( SCH_MARKER *aMarker, int aLayer )
{
    const int scale = aMarker->m_ScalingFactor;

    // If you are changing this, update the bounding box as well
    const VECTOR2D arrow[] = {
            VECTOR2D(  0 * scale,   0 * scale ),
            VECTOR2D(  8 * scale,   1 * scale ),
            VECTOR2D(  4 * scale,   3 * scale ),
            VECTOR2D( 13 * scale,   8 * scale ),
            VECTOR2D(  9 * scale,   9 * scale ),
            VECTOR2D(  8 * scale,  13 * scale ),
            VECTOR2D(  3 * scale,   4 * scale ),
            VECTOR2D(  1 * scale,   8 * scale ),
            VECTOR2D(  0 * scale,   0 * scale )
    };

    COLOR4D color = m_schSettings.GetLayerColor( LAYER_ERC_WARN );

    if( aMarker->GetErrorLevel() == MARKER_BASE::MARKER_SEVERITY_ERROR )
        color = m_schSettings.GetLayerColor( LAYER_ERC_ERR );

    m_gal->Save();
    m_gal->Translate( aMarker->GetPosition() );
    m_gal->SetFillColor( color );
    m_gal->SetIsFill( true );
    m_gal->SetIsStroke( false );
    m_gal->DrawPolygon( arrow, sizeof( arrow ) / sizeof( VECTOR2D ) );
    m_gal->Restore();
}


}; // namespace KIGFX
