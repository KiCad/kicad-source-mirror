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

namespace KIGFX {
    struct COLOR_DEF
    {
        std::string configName;
        SCH_LAYER_ID layer;
        COLOR4D color;

        COLOR_DEF( std::string name, SCH_LAYER_ID layer_, COLOR4D color_ )
        {
            configName = name;
            layer = layer_;
            color = color_;
        }
    };


SCH_RENDER_SETTINGS::SCH_RENDER_SETTINGS()
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

SCH_PAINTER::SCH_PAINTER( GAL* aGal ) :
	KIGFX::PAINTER (aGal)
	{

	}

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
    m_gal->AdvanceDepth();

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

		default:
			return false;
	}
	return false;
}

void SCH_PAINTER::draw ( LIB_PART *aComp, int aLayer, bool aDrawFields, int aUnit, int aConvert )
{
    auto comp = const_cast<LIB_PART*>(aComp);
    for ( auto& item : comp->GetDrawItems() )
    {
		if( !aDrawFields && item.Type() == LIB_FIELD_T)
            continue;

        if ( aUnit && item.GetUnit() && aUnit != item.GetUnit() )
            continue;

        if ( aConvert && item.GetConvert() && aConvert != item.GetConvert() )
            continue;

        Draw ( &item, aLayer );
    }
}

static VECTOR2D mapCoords( const wxPoint& aCoord )
{
    return VECTOR2D( aCoord.x, -aCoord.y );
}


void SCH_PAINTER::draw ( LIB_RECTANGLE *aComp, int aLayer )
{
	defaultColors(aComp);
    //m_gal->SetIsStroke( true );
    m_gal->SetLineWidth( aComp->GetPenSize() );
    m_gal->DrawRectangle( mapCoords( aComp->GetPosition() ),
                          mapCoords( aComp->GetEnd() ) );

}

void SCH_PAINTER::triLine ( const VECTOR2D &a, const VECTOR2D &b, const VECTOR2D &c )
{
  m_gal->DrawLine ( a, b );
  m_gal->DrawLine ( b, c );
}


void SCH_PAINTER::defaultColors ( const LIB_ITEM *aItem )
{
  const COLOR4D& fg = m_schSettings.GetLayerColor ( LAYER_DEVICE );
  const COLOR4D& bg = m_schSettings.GetLayerColor ( LAYER_DEVICE_BACKGROUND );

  m_gal->SetIsStroke (true);
  m_gal->SetStrokeColor( fg );
  m_gal->SetLineWidth ( aItem->GetPenSize() );
  switch(aItem->GetFillMode())
  {
    case FILLED_WITH_BG_BODYCOLOR:
      m_gal->SetIsFill(true);
      m_gal->SetFillColor ( bg );
      break;

    case FILLED_SHAPE:
      m_gal->SetIsFill(true);
      m_gal->SetFillColor ( fg );
      break;
    default:
      m_gal->SetIsFill(false);
  }
}

void SCH_PAINTER::draw ( LIB_CIRCLE *aCircle, int aLayer )
{
  defaultColors(aCircle);
  m_gal->DrawCircle( mapCoords( aCircle->GetPosition() ), aCircle->GetRadius() );
}

void SCH_PAINTER::draw ( LIB_ARC *aArc, int aLayer )
{
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


void SCH_PAINTER::draw ( LIB_FIELD *aField, int aLayer )
{
    if(!aField->IsVisible())
      return;

    int w;

   if( aField->IsBold() )
      w = GetPenSizeForBold( aField->GetWidth() );
    else
      w = aField->GetPenSize();


    COLOR4D color;

        switch( aField->GetId() )
        {
        case REFERENCE:
            color = m_schSettings.GetLayerColor( LAYER_REFERENCEPART );
            break;

        case VALUE:
            color = m_schSettings.GetLayerColor( LAYER_VALUEPART );
            break;

        default:
            color = m_schSettings.GetLayerColor( LAYER_FIELDS );
        break;
    }

    m_gal->SetLineWidth(w);
    m_gal->SetIsFill( false );
    m_gal->SetIsStroke (true);
    m_gal->SetStrokeColor( color );
    m_gal->SetGlyphSize ( aField->GetTextSize() );

    m_gal->SetHorizontalJustify( aField->GetHorizJustify( ) );
    m_gal->SetVerticalJustify( aField->GetVertJustify( ) );


    auto pos = mapCoords( aField->GetPosition() );
    double orient = aField->GetTextAngleRadians();

    m_gal->StrokeText( aField->GetText(), pos, orient );
}

#if 0
void SCH_PAINTER::draw ( SCH_FIELD *aField, int aLayer )
{

    if(!aField->IsVisible())
      return;

    int w;

   if( aField->IsBold() )
      w =  aField->GetPenSize() * 1.5; //GetPenSizeForBold( aField->GetWidth() );
    else
      w = aField->GetPenSize();

    m_gal->SetLineWidth(w);
    m_gal->SetIsFill( false );
    m_gal->SetIsStroke (true);

    COLOR4D color;

    switch(aField->GetId())
    {
      case REFERENCE:
        color = GetLayerColor( LAYER_REFERENCEPART );
        break;
      case VALUE:
        color = GetLayerColor( LAYER_VALUEPART );
        break;
      default:
        color = GetLayerColor( LAYER_FIELDS );
        break;
    }

    m_gal->SetStrokeColor( color );
    m_gal->SetGlyphSize ( aField->GetTextSize() );

    m_gal->SetHorizontalJustify( aField->GetHorizJustify( ));
    m_gal->SetVerticalJustify( aField->GetVertJustify( ));


    const VECTOR2D pos = aField->GetPosition();
    double orient = aField->GetTextAngleRadians() + M_PI;

    m_gal->StrokeText( aField->GetFullyQualifiedText(), pos, orient );
}
#endif


void SCH_PAINTER::draw ( LIB_POLYLINE *aLine, int aLayer )
{
  defaultColors(aLine);
  std::deque<VECTOR2D> vtx;

  for ( auto p : aLine->GetPolyPoints() )
    vtx.push_back ( mapCoords( p ) );

  if( aLine->GetFillMode() == FILLED_WITH_BG_BODYCOLOR || aLine->GetFillMode() == FILLED_SHAPE)
    vtx.push_back ( vtx[0] );

  m_gal->DrawPolygon ( vtx );
}

void SCH_PAINTER::draw ( LIB_TEXT *aText, int aLayer )
{
  if(!aText->IsVisible())
      return;

    int w = aText->GetPenSize();

    m_gal->SetLineWidth(w);
    m_gal->SetIsFill( false );
    m_gal->SetIsStroke (true);
    //m_gal->SetStrokeColor(  );
    m_gal->SetGlyphSize ( aText->GetTextSize() );

    m_gal->SetHorizontalJustify( GR_TEXT_HJUSTIFY_CENTER );
    m_gal->SetVerticalJustify( GR_TEXT_VJUSTIFY_CENTER );

    EDA_RECT bBox = aText->GetBoundingBox();
    bBox.RevertYAxis();
    
    auto pos = mapCoords( bBox.Centre() );

    double orient = aText->GetTextAngleRadians();

    m_gal->SetFontBold ( aText->IsBold() );
    m_gal->SetFontItalic ( aText->IsItalic() );
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


void SCH_PAINTER::draw ( LIB_PIN *aPin, int aLayer )
{
    if( !aPin->IsVisible() )
      return;

    const COLOR4D& color = m_schSettings.GetLayerColor( LAYER_PIN );

	VECTOR2I pos = mapCoords( aPin->GetPosition() ), p0, dir;
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

  m_gal->SetIsStroke ( true );
  m_gal->SetIsFill ( false );
  m_gal->SetLineWidth ( width );
  m_gal->SetStrokeColor ( color );
  m_gal->SetFontBold ( false );
  m_gal->SetFontItalic ( false );

  const int radius = ExternalPinDecoSize( *aPin );
  const int clock_size = InternalPinDecoSize( *aPin );

	if(shape == PINSHAPE_INVERTED)
	{

		m_gal->DrawCircle ( p0 + dir * radius, radius );
		m_gal->DrawLine ( p0 + dir * ( 2 * radius ), pos );
	} else if (shape == PINSHAPE_FALLING_EDGE_CLOCK )
  {

    pc = p0 + dir * clock_size ;

    triLine( p0 + VECTOR2D ( dir.y, -dir.x) * clock_size,
            pc,
            p0 + VECTOR2D ( -dir.y, dir.x) * clock_size
            );

    m_gal->DrawLine ( pos, pc );
  }
  else {
      //printf("DrawLPin\n");
    m_gal->DrawLine ( p0, pos );
    //m_gal->DrawLine ( p0, pos+dir.Perpendicular() * radius);
  }

  if(shape == PINSHAPE_CLOCK)
  {
    if (!dir.y)
    {
      triLine (p0 + VECTOR2D( 0, clock_size ),
                p0 + VECTOR2D( -dir.x * clock_size, 0),
                p0 + VECTOR2D( 0, -clock_size ));

    } else {
      triLine ( p0 + VECTOR2D ( clock_size, 0 ),
                p0 + VECTOR2D ( 0, -dir.y * clock_size ),
                p0 + VECTOR2D ( -clock_size, 0 ));
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
        if(!dir.y)
          m_gal->DrawLine( p0 - VECTOR2D(0, radius), p0 + VECTOR2D(dir.x, 1) * radius * 2);
        else
          m_gal->DrawLine (p0 - VECTOR2D(radius, 0), p0 + VECTOR2D(0, dir.y) * radius * 2);
    }

    if( shape == PINSHAPE_NONLOGIC ) /* NonLogic pin symbol */
    {
        m_gal->DrawLine ( p0 - VECTOR2D(dir.x + dir.y, dir.y - dir.x) * radius,
                          p0 + VECTOR2D(dir.x + dir.y, dir.y - dir.x) * radius);
        m_gal->DrawLine ( p0 - VECTOR2D(dir.x - dir.y, dir.x + dir.y) * radius,
                          p0 + VECTOR2D(dir.x - dir.y, dir.x + dir.y) * radius);
    }

    #define NCSYMB_PIN_DIM TARGET_PIN_RADIUS

    if( aPin->GetType() == PIN_NC )   // Draw a N.C. symbol
    {
        m_gal->DrawLine ( pos + VECTOR2D(-1, -1) * NCSYMB_PIN_DIM,
                          pos + VECTOR2D(1, 1) * NCSYMB_PIN_DIM);
        m_gal->DrawLine ( pos + VECTOR2D(1, -1) * NCSYMB_PIN_DIM,
                          pos + VECTOR2D(-1, 1) * NCSYMB_PIN_DIM);
    }

    m_gal->SetLineWidth ( 0.0 );
    m_gal->DrawCircle( pos, TARGET_PIN_RADIUS );

// Draw the labels

    int labelWidth = std::min ( GetDefaultLineThickness(), width );
    LIB_PART* libEntry = (const_cast<LIB_PIN *> (aPin)) ->GetParent();
    m_gal->SetLineWidth ( labelWidth );
    wxString    stringPinNum;

    /* Get the num and name colors */
    COLOR4D nameColor = m_schSettings.GetLayerColor( LAYER_PINNAM );
    COLOR4D numColor  = m_schSettings.GetLayerColor( LAYER_PINNUM );

    /* Create the pin num string */
    stringPinNum = aPin->GetNumber();
    int textOffset = libEntry->GetPinNameOffset();

    bool showNums = libEntry->ShowPinNumbers();
    bool showNames = libEntry->ShowPinNames();

    //m_gal->SetTextMirrored ( true ); // don't know why yet...
    m_gal->SetStrokeColor ( nameColor );
    m_gal->SetVerticalJustify( GR_TEXT_VJUSTIFY_CENTER );

    int         nameLineWidth = aPin->GetPenSize();
    nameLineWidth = Clamp_Text_PenSize( nameLineWidth, aPin->GetNameTextSize(), false );
    int         numLineWidth = aPin->GetPenSize();
    numLineWidth = Clamp_Text_PenSize( numLineWidth, aPin->GetNumberTextSize(), false );

    #define PIN_TEXT_MARGIN 4


    int         name_offset = PIN_TEXT_MARGIN +
                              ( nameLineWidth + GetDefaultLineThickness() ) / 2;
    int         num_offset = - PIN_TEXT_MARGIN -
                             ( numLineWidth + GetDefaultLineThickness() ) / 2;

    //printf("numoffs %d w %d s %d\n", num_offset, numLineWidth,aPin->GetNumberTextSize() );

    int nameSize = aPin->GetNameTextSize();


    if( textOffset )  /* Draw the text inside, but the pin numbers outside. */
    {
        m_gal->SetGlyphSize ( VECTOR2D ( nameSize, nameSize ) );

        if(showNames && (nameSize > 0))
        {
        switch ( orient )
        {
          case PIN_LEFT:
            m_gal->SetHorizontalJustify( GR_TEXT_HJUSTIFY_RIGHT );
            m_gal->StrokeText( aPin->GetName(), pos + VECTOR2D ( -textOffset - len, 0 ), 0 );
            break;
          case PIN_RIGHT:
            m_gal->SetHorizontalJustify( GR_TEXT_HJUSTIFY_LEFT );
            m_gal->StrokeText( aPin->GetName(), pos + VECTOR2D ( textOffset + len, 0 ), 0 );
            break;
          case PIN_DOWN:
            m_gal->SetHorizontalJustify ( GR_TEXT_HJUSTIFY_RIGHT );
            m_gal->StrokeText ( aPin->GetName(), pos + VECTOR2D ( 0, textOffset + len), M_PI / 2);
            break;
          case PIN_UP:
            m_gal->SetHorizontalJustify ( GR_TEXT_HJUSTIFY_LEFT );
            m_gal->StrokeText ( aPin->GetName(), pos + VECTOR2D ( 0, - textOffset - len), M_PI / 2);
            break;
        }
    }

    #define TXTMARGE 10

    int numSize = aPin->GetNumberTextSize();

      if(showNums && numSize > 0)
      {

        m_gal->SetGlyphSize ( VECTOR2D ( numSize, numSize ) );


        m_gal->SetStrokeColor( numColor );
        m_gal->SetGlyphSize ( VECTOR2D ( numSize, numSize ) );
        m_gal->SetHorizontalJustify( GR_TEXT_HJUSTIFY_CENTER );
        m_gal->SetVerticalJustify( GR_TEXT_VJUSTIFY_BOTTOM );

        switch(orient)
        {
          case PIN_LEFT:
          case PIN_RIGHT:
            m_gal->StrokeText (stringPinNum, VECTOR2D( (p0.x + pos.x) / 2, p0.y + num_offset ), 0 );
            break;
          case PIN_DOWN:
          case PIN_UP:
            m_gal->StrokeText (stringPinNum, VECTOR2D ( p0.x - num_offset, (p0.y + pos.y) / 2), M_PI / 2);
           break;
        }
      }
    }
}

void SCH_PAINTER::draw ( SCH_JUNCTION *aJct, int aLayer )
{
    const COLOR4D& color = m_schSettings.GetLayerColor( LAYER_JUNCTION );


    m_gal->SetIsStroke(true);
    m_gal->SetIsFill(true);
    m_gal->SetStrokeColor( color );
    m_gal->SetFillColor( color );
    m_gal->DrawCircle( aJct->GetPosition(),  aJct->GetSymbolSize() / 2 );
}

void SCH_PAINTER::draw ( SCH_LINE *aLine, int aLayer )
{
    COLOR4D color = GetLayerColor( LAYER_WIRE );
    int width = aLine->GetPenSize();

    /*if( Color != COLOR4D::UNSPECIFIED )
        color = Color;
    else if( m_color != COLOR4D::UNSPECIFIED )
        color = m_color;
    else
        color = GetLayerColor( GetState( BRIGHTENED ) ? LAYER_BRIGHTENED : m_Layer );

    GRSetDrawMode( DC, DrawMode );

    wxPoint start = m_start;
    wxPoint end = m_end;

    if( ( m_Flags & STARTPOINT ) == 0 )
        start += offset;

    if( ( m_Flags & ENDPOINT ) == 0 )
        end += offset;*/

    m_gal->SetIsStroke(true);
    m_gal->SetStrokeColor(color);
    m_gal->SetLineWidth( width );
    m_gal->DrawLine( aLine->GetStartPoint(), aLine->GetEndPoint() );

    //GRLine( panel->GetClipBox(), DC, start.x, start.y, end.x, end.y, width, color,
    //        getwxPenStyle( (PlotDashType) GetLineStyle() ) );

    /*if( m_startIsDangling )
        DrawDanglingSymbol( panel, DC, start, color );

    if( m_endIsDangling )
        DrawDanglingSymbol( panel, DC, end, color );*/
}

void SCH_PAINTER::draw ( SCH_TEXT *aText, int aLayer )
{
    COLOR4D     color;
    int         linewidth = aText->GetThickness() == 0 ? GetDefaultLineThickness() : aText->GetThickness();

    switch ( aText->Type() )
    {
        case SCH_HIERARCHICAL_LABEL_T:
            color = m_schSettings.GetLayerColor( LAYER_SHEETLABEL );
            break;
        default:
            color = m_schSettings.GetLayerColor( LAYER_NOTES );
            break;
    }

    linewidth = Clamp_Text_PenSize( linewidth, aText->GetTextSize(), aText->IsBold() );

    wxPoint text_offset = aText->GetTextPos() + aText->GetSchematicTextOffset();

    int savedWidth = aText->GetThickness();

    //if( m_isDangling && panel)
        //DrawDanglingSymbol( panel, DC, GetTextPos() + aOffset, color );

        wxString shownText( aText->GetShownText() );
        if( shownText.Length() == 0 )
            return;


        m_gal->SetStrokeColor( color );
        m_gal->SetIsFill( false );
        m_gal->SetIsStroke( true );
        m_gal->SetTextAttributes( aText );
        m_gal->SetLineWidth( linewidth );
        m_gal->StrokeText( shownText, text_offset, aText->GetTextAngleRadians() );


}

static LIB_PART* clone( LIB_PART *part )
{
    LIB_PART *rv = new LIB_PART ( *part );
    for( auto& item : part->GetDrawItems() )
    {
        rv->AddDrawItem( static_cast<LIB_ITEM*>(item.Clone() ) );
    }
    return rv;
}

static void orientComponent( LIB_PART *part, int orientation )
{

#define N_ORIENTATIONS 8

    struct ORIENT {
        int flag;
        int n_rots;
        int mirror_x, mirror_y;
    } orientations[N_ORIENTATIONS] = {
        {    CMP_ORIENT_0,       0, 0, 0},
        {    CMP_ORIENT_90,      1, 0, 0},
        {    CMP_ORIENT_180,     2, 0, 0},
        {    CMP_ORIENT_270,     3, 0, 0},
        { CMP_MIRROR_X + CMP_ORIENT_0, 0, 1, 0 },
        { CMP_MIRROR_X + CMP_ORIENT_90, 1, 1, 0},
        { CMP_MIRROR_Y, 0, 0, 1},
        { CMP_MIRROR_X + CMP_ORIENT_270, 3, 1, 0 }
    };
    //        ,
    //        CMP_MIRROR_Y + CMP_ORIENT_0,   CMP_MIRROR_Y + CMP_ORIENT_90,
    //        CMP_MIRROR_Y + CMP_ORIENT_180, CMP_MIRROR_Y + CMP_ORIENT_270*/
        //}

    ORIENT o;

    for(int i = 0; i < N_ORIENTATIONS;i++)
    if(orientations[i].flag == orientation)
    {
        o = orientations[i];
        break;
    }

    //printf("orient %d %d %d\n", o.n_rots, o.mirror_x, o.mirror_y );

    for(int i = 0; i < o.n_rots; i++)
    {
        for( auto& item : part->GetDrawItems() )
        {
            item.Rotate( wxPoint(0, 0 ), true );
        }
    }

    for( auto& item : part->GetDrawItems() )
    {
        if( orientation & CMP_MIRROR_X )
            item.MirrorVertical( wxPoint(0, 0 ) );
        if( orientation & CMP_MIRROR_Y )
            item.MirrorHorizontal( wxPoint(0, 0 ) );
    }
}

void SCH_PAINTER::draw ( SCH_COMPONENT *aComp, int aLayer )
{
    PART_SPTR part = aComp->GetPartRef().lock();

    if (part)
    {
        std::unique_ptr<LIB_PART> ptrans ( clone(part.get()) );

        auto orient = aComp->GetOrientation();

        orientComponent( ptrans.get(), orient );

        for( auto& item : ptrans->GetDrawItems() )
        {
//            item.MirrorVertical( wxPoint(0, 0 ) );
        }

        for( auto& item : ptrans->GetDrawItems() )
        {
            auto rp = aComp->GetPosition();
            auto ip = item.GetPosition();
            item.Move( wxPoint(rp.x+ip.x, ip.y-rp.y) );
        }

        draw( ptrans.get(), aLayer, false, aComp->GetUnit(), aComp->GetConvert() );
    }
    else    // Use dummy() part if the actual cannot be found.
    {
        //printf("drawDummy\n");
//        dummy()->Draw( aPanel, aDC, m_Pos + aOffset, 0, 0, opts );
    }

    SCH_FIELD* field = aComp->GetField( REFERENCE );

    draw( field, aLayer );

    for( int ii = VALUE; ii < aComp->GetFieldCount(); ii++ )
    {
        field = aComp->GetField( ii );

        if( field->IsMoving() )
            continue;

        draw( field, aLayer );
    }

}

void SCH_PAINTER::draw ( SCH_FIELD *aField, int aLayer )
{
    int            orient;
    COLOR4D        color;
    wxPoint        textpos;

    SCH_COMPONENT* parentComponent = (SCH_COMPONENT*) aField->GetParent();
    int            lineWidth = aField->GetThickness();

    if( lineWidth == 0 )   // Use default values for pen size
    {
        if( aField->IsBold() )
            lineWidth = GetPenSizeForBold( aField->GetTextWidth() );
        else
            lineWidth = GetDefaultLineThickness();
    }

    // Clip pen size for small texts:
    lineWidth = Clamp_Text_PenSize( lineWidth, aField->GetTextSize(), aField->IsBold() );

    if( !aField->IsVisible() || aField->IsVoid() )
        return;


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

    if( aField->GetId() == REFERENCE )
        color = GetLayerColor( LAYER_REFERENCEPART );
    else if( aField->GetId() == VALUE )
        color = GetLayerColor( LAYER_VALUEPART );
    else
        color = GetLayerColor( LAYER_FIELDS );

    m_gal->SetHorizontalJustify( GR_TEXT_HJUSTIFY_CENTER );
    m_gal->SetVerticalJustify( GR_TEXT_VJUSTIFY_CENTER );
    m_gal->SetStrokeColor( color );
    m_gal->SetIsFill( false );
    m_gal->SetIsStroke( true );
    m_gal->SetTextAttributes( aField );
    m_gal->SetLineWidth( lineWidth );
    m_gal->StrokeText( aField->GetFullyQualifiedText(), textpos, orient == TEXT_ANGLE_VERT ? -M_PI/2 : 0 );
}

void SCH_PAINTER::draw ( SCH_GLOBALLABEL *aLabel, int aLayer )
{

}


void SCH_PAINTER::draw ( SCH_HIERLABEL *aLabel, int aLayer )
{
    std::vector<wxPoint> pts;
    std::deque<VECTOR2D> pts2;

    aLabel->CreateGraphicShape( pts, aLabel->GetTextPos() );

    for( auto p : pts )
        pts2.push_back( VECTOR2D(p.x, p.y ) );

    m_gal->SetIsFill( false );
    m_gal->SetIsStroke( true );
    m_gal->SetStrokeColor( m_schSettings.GetLayerColor( LAYER_SHEETLABEL ) );
    m_gal->DrawPolyline( pts2 );
    m_gal->AdvanceDepth(); // fixme

    draw( static_cast<SCH_TEXT*>( aLabel ), aLayer );
}

void SCH_PAINTER::draw ( SCH_SHEET *aSheet, int aLayer )
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
    {
        m_gal->AdvanceDepth();
        draw( static_cast<SCH_HIERLABEL*>( &sheetPin ), aLayer );
    }

}

void SCH_PAINTER::draw ( SCH_NO_CONNECT *aNC, int aLayer )
{
    int delta = aNC->GetSize().x / 2;
    int width = GetDefaultLineThickness();

    m_gal->SetStrokeColor( GetLayerColor( LAYER_NOCONNECT ) );
    m_gal->SetIsStroke( true );
    m_gal->SetIsFill( false );
    m_gal->SetLineWidth( width );

    VECTOR2D p = aNC->GetPosition();

    m_gal->DrawLine( p + VECTOR2D(-delta, -delta), p+VECTOR2D( delta, delta ) );
    m_gal->DrawLine( p + VECTOR2D(-delta, delta), p+VECTOR2D( delta, -delta ) );
}

void SCH_PAINTER::draw ( SCH_BUS_ENTRY_BASE *aEntry, int aLayer )
{
    m_gal->SetStrokeColor( GetLayerColor( LAYER_BUS ) );
    m_gal->SetIsStroke( true );
    m_gal->SetLineWidth( aEntry->GetPenSize() );
    m_gal->SetIsFill( false );

    VECTOR2D pos = aEntry->GetPosition();
    VECTOR2D endPos = aEntry->m_End();

    m_gal->DrawLine( pos, endPos );

    if( aEntry->IsDanglingStart() )
        m_gal->DrawCircle( pos, TARGET_BUSENTRY_RADIUS );

    if( aEntry->IsDanglingEnd() )
        m_gal->DrawCircle( endPos, TARGET_BUSENTRY_RADIUS );


}


}; // namespace KIGFX
