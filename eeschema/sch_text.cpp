/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2016 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 2015 Wayne Stambaugh <stambaughw@verizon.net>
 * Copyright (C) 1992-2017 KiCad Developers, see AUTHORS.txt for contributors.
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

/**
 * @file sch_text.cpp
 * @brief Code for handling schematic texts (texts, labels, hlabels and global labels).
 */

#include <fctsys.h>
#include <gr_basic.h>
#include <macros.h>
#include <trigo.h>
#include <sch_draw_panel.h>
#include <draw_graphic_text.h>
#include <sch_edit_frame.h>
#include <plotter.h>
#include <msgpanel.h>
#include <gal/stroke_font.h>
#include <bitmaps.h>

#include <list_operations.h>
#include <sch_text.h>
#include <netlist_object.h>
#include <trace_helpers.h>


extern void IncrementLabelMember( wxString& name, int aIncrement );

// Only for tests: set DRAW_BBOX to 1 to draw the bounding box of labels
#define DRAW_BBOX 0

// Margin in internal units (mils) between labels and wires
#define TXT_MARGIN 4

// Names of sheet label types.
const char* SheetLabelType[] =
{
    "Input",
    "Output",
    "BiDi",
    "3State",
    "UnSpc",
    "???"
};

/* Coding polygons for global symbol graphic shapes.
 *  the first parml is the number of corners
 *  others are the corners coordinates in reduced units
 *  the real coordinate is the reduced coordinate * text half size
 */
static int  TemplateIN_HN[] = { 6, 0, 0, -1, -1, -2, -1, -2, 1, -1, 1, 0, 0 };
static int  TemplateIN_HI[] = { 6, 0, 0, 1, 1, 2, 1, 2, -1, 1, -1, 0, 0 };
static int  TemplateIN_UP[] = { 6, 0, 0, 1, -1, 1, -2, -1, -2, -1, -1, 0, 0 };
static int  TemplateIN_BOTTOM[] = { 6, 0, 0, 1, 1, 1, 2, -1, 2, -1, 1, 0, 0 };

static int  TemplateOUT_HN[] = { 6, -2, 0, -1, 1, 0, 1, 0, -1, -1, -1, -2, 0 };
static int  TemplateOUT_HI[] = { 6, 2, 0, 1, -1, 0, -1, 0, 1, 1, 1, 2, 0 };
static int  TemplateOUT_UP[] = { 6, 0, -2, 1, -1, 1, 0, -1, 0, -1, -1, 0, -2 };
static int  TemplateOUT_BOTTOM[] = { 6, 0, 2, 1, 1, 1, 0, -1, 0, -1, 1, 0, 2 };

static int  TemplateUNSPC_HN[] = { 5, 0, -1, -2, -1, -2, 1, 0, 1, 0, -1 };
static int  TemplateUNSPC_HI[] = { 5, 0, -1, 2, -1, 2, 1, 0, 1, 0, -1 };
static int  TemplateUNSPC_UP[] = { 5, 1, 0, 1, -2, -1, -2, -1, 0, 1, 0 };
static int  TemplateUNSPC_BOTTOM[] = { 5, 1, 0, 1, 2, -1, 2, -1, 0, 1, 0 };

static int  TemplateBIDI_HN[] = { 5, 0, 0, -1, -1, -2, 0, -1, 1, 0, 0 };
static int  TemplateBIDI_HI[] = { 5, 0, 0, 1, -1, 2, 0, 1, 1, 0, 0 };
static int  TemplateBIDI_UP[] = { 5, 0, 0, -1, -1, 0, -2, 1, -1, 0, 0 };
static int  TemplateBIDI_BOTTOM[] = { 5, 0, 0, -1, 1, 0, 2, 1, 1, 0, 0 };

static int  Template3STATE_HN[] = { 5, 0, 0, -1, -1, -2, 0, -1, 1, 0, 0 };
static int  Template3STATE_HI[] = { 5, 0, 0, 1, -1, 2, 0, 1, 1, 0, 0 };
static int  Template3STATE_UP[] = { 5, 0, 0, -1, -1, 0, -2, 1, -1, 0, 0 };
static int  Template3STATE_BOTTOM[] = { 5, 0, 0, -1, 1, 0, 2, 1, 1, 0, 0 };

static int* TemplateShape[5][4] =
{
    { TemplateIN_HN,     TemplateIN_UP,     TemplateIN_HI,     TemplateIN_BOTTOM     },
    { TemplateOUT_HN,    TemplateOUT_UP,    TemplateOUT_HI,    TemplateOUT_BOTTOM    },
    { TemplateBIDI_HN,   TemplateBIDI_UP,   TemplateBIDI_HI,   TemplateBIDI_BOTTOM   },
    { Template3STATE_HN, Template3STATE_UP, Template3STATE_HI, Template3STATE_BOTTOM },
    { TemplateUNSPC_HN,  TemplateUNSPC_UP,  TemplateUNSPC_HI,  TemplateUNSPC_BOTTOM  }
};


SCH_TEXT::SCH_TEXT( const wxPoint& pos, const wxString& text, KICAD_T aType ) :
    SCH_ITEM( NULL, aType ),
    EDA_TEXT( text ),
    m_shape( NET_INPUT )
{
    m_Layer = LAYER_NOTES;
    SetTextPos( pos );
    m_isDangling = false;
    m_spin_style = 0;

    SetMultilineAllowed( true );
}


SCH_TEXT::SCH_TEXT( const SCH_TEXT& aText ) :
    SCH_ITEM( aText ),
    EDA_TEXT( aText )
{
    m_shape = aText.m_shape;
    m_isDangling = aText.m_isDangling;
    m_spin_style = aText.m_spin_style;
}


EDA_ITEM* SCH_TEXT::Clone() const
{
    return new SCH_TEXT( *this );
}


void SCH_TEXT::IncrementLabel( int aIncrement )
{
    IncrementLabelMember( m_Text, aIncrement );
}


wxPoint SCH_TEXT::GetSchematicTextOffset() const
{
    wxPoint text_offset;

    // add an offset to x (or y) position to aid readability of text on a wire or line
    int thick_offset = TXT_MARGIN + ( GetPenSize() + GetDefaultLineThickness() ) / 2;

    switch( GetLabelSpinStyle() )
    {
    default:
    case 0: text_offset.y = -thick_offset; break;  // Horiz Normal Orientation (left justified)
    case 1: text_offset.x = -thick_offset; break;  // Vert Orientation UP
    case 2: text_offset.y = -thick_offset; break;  // Horiz Orientation - Right justified
    case 3: text_offset.x = -thick_offset; break;  // Vert Orientation BOTTOM
    }

    return text_offset;
}


bool SCH_TEXT::Matches( wxFindReplaceData& aSearchData, void* aAuxData, wxPoint * aFindLocation )
{
    wxLogTrace( traceFindItem, wxT( "  item " ) + GetSelectMenuText( MILLIMETRES ) );

    if( SCH_ITEM::Matches( m_Text, aSearchData ) )
    {
        EDA_RECT BoundaryBox = GetBoundingBox();

        if( aFindLocation )
            *aFindLocation = BoundaryBox.Centre();

        return true;
    }

    return false;
}


void SCH_TEXT::MirrorY( int aYaxis_position )
{
    // Text is NOT really mirrored; it is moved to a suitable horizontal position
    switch( GetLabelSpinStyle() )
    {
    default:
    case 0: SetLabelSpinStyle( 2 ); break;  // horizontal text
    case 1:                         break;  // Vert Orientation UP
    case 2: SetLabelSpinStyle( 0 ); break;  // invert horizontal text
    case 3:                         break;  // Vert Orientation BOTTOM
    }

    SetTextX( Mirror( GetTextPos().x, aYaxis_position ) );
}


void SCH_TEXT::MirrorX( int aXaxis_position )
{
    // Text is NOT really mirrored; it is moved to a suitable vertical position
    switch( GetLabelSpinStyle() )
    {
    default:
    case 0:                         break;  // horizontal text
    case 1: SetLabelSpinStyle( 3 ); break;  // Vert Orientation UP
    case 2:                         break;  // invert horizontal text
    case 3: SetLabelSpinStyle( 1 ); break;  // Vert Orientation BOTTOM
    }

    SetTextY( Mirror( GetTextPos().y, aXaxis_position ) );
}


void SCH_TEXT::Rotate( wxPoint aPosition )
{
    int dy;

    wxPoint pt = GetTextPos();
    RotatePoint( &pt, aPosition, 900 );
    SetTextPos( pt );

    SetLabelSpinStyle( (GetLabelSpinStyle() + 1) % 4 );

    if( this->Type() == SCH_TEXT_T )
    {
        switch( GetLabelSpinStyle() )
        {
        case 0:  dy = GetTextHeight(); break;     // horizontal text
        case 1:  dy = 0;               break;     // Vert Orientation UP
        case 2:  dy = GetTextHeight(); break;     // invert horizontal text
        case 3:  dy = 0;               break;     // Vert Orientation BOTTOM
        default: dy = 0;               break;
        }

        SetTextY( GetTextPos().y + dy );
    }
}


void SCH_TEXT::SetLabelSpinStyle( int aSpinStyle )
{
    m_spin_style = aSpinStyle;

    switch( aSpinStyle )
    {
    default:
    case 0: // Horiz Normal Orientation (left justified)
        SetTextAngle( TEXT_ANGLE_HORIZ );
        SetHorizJustify( GR_TEXT_HJUSTIFY_LEFT );
        SetVertJustify( GR_TEXT_VJUSTIFY_BOTTOM );
        break;

    case 1: // Vert Orientation UP
        SetTextAngle( TEXT_ANGLE_VERT );
        SetHorizJustify( GR_TEXT_HJUSTIFY_LEFT );
        SetVertJustify( GR_TEXT_VJUSTIFY_BOTTOM );
        break;

    case 2: // Horiz Orientation - Right justified
        SetTextAngle( TEXT_ANGLE_HORIZ );
        SetHorizJustify( GR_TEXT_HJUSTIFY_RIGHT );
        SetVertJustify( GR_TEXT_VJUSTIFY_BOTTOM );
        break;

    case 3: //  Vert Orientation BOTTOM
        SetTextAngle( TEXT_ANGLE_VERT );
        SetHorizJustify( GR_TEXT_HJUSTIFY_RIGHT );
        SetVertJustify( GR_TEXT_VJUSTIFY_BOTTOM );
        break;
    }
}


void SCH_TEXT::SwapData( SCH_ITEM* aItem )
{
    SCH_TEXT* item = (SCH_TEXT*) aItem;

    std::swap( m_Text, item->m_Text );
    std::swap( m_Layer, item->m_Layer );

    std::swap( m_shape, item->m_shape );
    std::swap( m_isDangling, item->m_isDangling );
    std::swap( m_spin_style, item->m_spin_style );

    SwapEffects( *item );
}


int SCH_TEXT::GetPenSize() const
{
    int pensize = GetThickness();

    if( pensize == 0 )   // Use default values for pen size
    {
        if( IsBold()  )
            pensize = GetPenSizeForBold( GetTextWidth() );
        else
            pensize = GetDefaultLineThickness();
    }

    // Clip pen size for small texts:
    pensize = Clamp_Text_PenSize( pensize, GetTextSize(), IsBold() );
    return pensize;
}


void SCH_TEXT::Draw( EDA_DRAW_PANEL* panel, wxDC* DC, const wxPoint& aOffset,
                     GR_DRAWMODE DrawMode, COLOR4D Color )
{
    COLOR4D     color;
    int         linewidth = GetThickness() == 0 ? GetDefaultLineThickness() : GetThickness();
    EDA_RECT*   clipbox = panel? panel->GetClipBox() : NULL;

    linewidth = Clamp_Text_PenSize( linewidth, GetTextSize(), IsBold() );

    if( Color != COLOR4D::UNSPECIFIED )
        color = Color;
    else
        color = GetLayerColor( GetState( BRIGHTENED ) ? LAYER_BRIGHTENED : m_Layer );

    GRSetDrawMode( DC, DrawMode );

    wxPoint text_offset = aOffset + GetSchematicTextOffset();

    int savedWidth = GetThickness();
    SetThickness( linewidth );              // Set the minimum width

    EDA_TEXT::Draw( clipbox, DC, text_offset, color, DrawMode, FILLED, COLOR4D::UNSPECIFIED );

    SetThickness( savedWidth );

    if( m_isDangling && panel)
        DrawDanglingSymbol( panel, DC, GetTextPos() + aOffset, color );

    // Enable these line to draw the bounding box (debug tests purposes only)
#if DRAW_BBOX
    {
        EDA_RECT BoundaryBox = GetBoundingBox();
        GRRect( clipbox, DC, BoundaryBox, 0, BROWN );
    }
#endif
}


void SCH_TEXT::GetEndPoints( std::vector <DANGLING_END_ITEM>& aItemList )
{
    // Normal text labels cannot be tested for dangling ends.
    if( Type() == SCH_TEXT_T )
        return;

    DANGLING_END_ITEM item( LABEL_END, this, GetTextPos() );
    aItemList.push_back( item );
}


bool SCH_TEXT::UpdateDanglingState( std::vector<DANGLING_END_ITEM>& aItemList )
{
    // Normal text labels cannot be tested for dangling ends.
    if( Type() == SCH_TEXT_T )
        return false;

    bool previousState = m_isDangling;
    m_isDangling = true;

    for( unsigned ii = 0; ii < aItemList.size(); ii++ )
    {
        DANGLING_END_ITEM& item = aItemList[ii];

        if( item.GetItem() == this )
            continue;

        switch( item.GetType() )
        {
        case PIN_END:
        case LABEL_END:
        case SHEET_LABEL_END:
        case NO_CONNECT_END:
            if( GetTextPos() == item.GetPosition() )
                m_isDangling = false;

            break;

        case WIRE_START_END:
        case BUS_START_END:
        {
            // These schematic items have created 2 DANGLING_END_ITEM one per end.  But being
            // a paranoid programmer, I'll check just in case.
            ii++;

            wxCHECK_MSG( ii < aItemList.size(), previousState != m_isDangling,
                         wxT( "Dangling end type list overflow.  Bad programmer!" ) );

            DANGLING_END_ITEM & nextItem = aItemList[ii];
            m_isDangling = !IsPointOnSegment( item.GetPosition(), nextItem.GetPosition(), GetTextPos() );
        }
            break;

        default:
            break;
        }

        if( !m_isDangling )
            break;
    }

    return previousState != m_isDangling;
}


bool SCH_TEXT::IsSelectStateChanged( const wxRect& aRect )
{
    bool previousState = IsSelected();

    if( aRect.Contains( GetTextPos() ) )
        SetFlags( SELECTED );
    else
        ClearFlags( SELECTED );

    return previousState != IsSelected();
}


void SCH_TEXT::GetConnectionPoints( std::vector< wxPoint >& aPoints ) const
{
    // Normal text labels do not have connection points.  All others do.
    if( Type() == SCH_TEXT_T )
        return;

    aPoints.push_back( GetTextPos() );
}


const EDA_RECT SCH_TEXT::GetBoundingBox() const
{
    // We must pass the effective text thickness to GetTextBox
    // when calculating the bounding box
    int linewidth = GetThickness() == 0 ? GetDefaultLineThickness() : GetThickness();

    linewidth = Clamp_Text_PenSize( linewidth, GetTextSize(), IsBold() );

    EDA_RECT rect = GetTextBox( -1, linewidth );

    if( GetTextAngle() != 0 )      // Rotate rect
    {
        wxPoint pos = rect.GetOrigin();
        wxPoint end = rect.GetEnd();

        RotatePoint( &pos, GetTextPos(), GetTextAngle() );
        RotatePoint( &end, GetTextPos(), GetTextAngle() );

        rect.SetOrigin( pos );
        rect.SetEnd( end );
    }

    rect.Normalize();
    return rect;
}


wxString SCH_TEXT::GetSelectMenuText( EDA_UNITS_T aUnits ) const
{
    return wxString::Format( _( "Graphic Text \"%s\"" ), GetChars( ShortenedShownText() ) );
}


BITMAP_DEF SCH_TEXT::GetMenuImage() const
{
    return text_xpm;
}


void SCH_TEXT::GetNetListItem( NETLIST_OBJECT_LIST& aNetListItems,
                               SCH_SHEET_PATH*      aSheetPath )
{
    if( GetLayer() == LAYER_NOTES || GetLayer() == LAYER_SHEETLABEL )
        return;

    NETLIST_OBJECT* item = new NETLIST_OBJECT();
    item->m_SheetPath = *aSheetPath;
    item->m_SheetPathInclude = *aSheetPath;
    item->m_Comp = (SCH_ITEM*) this;
    item->m_Type = NET_LABEL;

    if( GetLayer() == LAYER_GLOBLABEL )
        item->m_Type = NET_GLOBLABEL;
    else if( GetLayer() == LAYER_HIERLABEL )
        item->m_Type = NET_HIERLABEL;

    item->m_Label = m_Text;
    item->m_Start = item->m_End = GetTextPos();

    aNetListItems.push_back( item );

    // If a bus connects to label
    if( IsBusLabel( m_Text ) )
        item->ConvertBusToNetListItems( aNetListItems );
}


bool SCH_TEXT::HitTest( const wxPoint& aPosition, int aAccuracy ) const
{
    EDA_RECT bBox = GetBoundingBox();
    bBox.Inflate( aAccuracy );
    return bBox.Contains( aPosition );
}


bool SCH_TEXT::HitTest( const EDA_RECT& aRect, bool aContained, int aAccuracy ) const
{
    EDA_RECT bBox = GetBoundingBox();
    bBox.Inflate( aAccuracy );

    if( aContained )
        return aRect.Contains( bBox );

    return aRect.Intersects( bBox );
}


void SCH_TEXT::Plot( PLOTTER* aPlotter )
{
    static std::vector <wxPoint> Poly;
    COLOR4D  color = GetLayerColor( GetLayer() );
    int      thickness = GetPenSize();

    // Two thicknesses are set here:
    // The first is for EDA_TEXT, which controls the interline spacing based on text thickness
    // The second is for the output that sets the actual stroke size
    SetThickness( thickness );
    aPlotter->SetCurrentLineWidth( thickness );

    if( IsMultilineAllowed() )
    {
        std::vector<wxPoint> positions;
        wxArrayString strings_list;
        wxStringSplit( GetShownText(), strings_list, '\n' );
        positions.reserve( strings_list.Count() );

        GetPositionsOfLinesOfMultilineText(positions, (int) strings_list.Count() );

        for( unsigned ii = 0; ii < strings_list.Count(); ii++ )
        {
            wxPoint textpos = positions[ii] + GetSchematicTextOffset();
            wxString& txt = strings_list.Item( ii );
            aPlotter->Text( textpos, color, txt, GetTextAngle(), GetTextSize(),
                            GetHorizJustify(), GetVertJustify(),
                            thickness, IsItalic(), IsBold() );
        }
    }
    else
    {
        wxPoint textpos = GetTextPos() + GetSchematicTextOffset();

        aPlotter->Text( textpos, color, GetShownText(), GetTextAngle(), GetTextSize(),
                        GetHorizJustify(), GetVertJustify(),
                        thickness, IsItalic(), IsBold() );
    }

    // Draw graphic symbol for global or hierarchical labels
    CreateGraphicShape( Poly, GetTextPos() );

    aPlotter->SetCurrentLineWidth( GetPenSize() );

    if( Poly.size() )
        aPlotter->PlotPoly( Poly, NO_FILL );
}


/*
 * Display the type, shape, size and some other props to the Message panel
 */
void SCH_TEXT::GetMsgPanelInfo( EDA_UNITS_T aUnits, MSG_PANEL_ITEMS& aList )
{
    wxString msg;

    switch( Type() )
    {
    case SCH_TEXT_T:               msg = _( "Graphic Text" );           break;
    case SCH_LABEL_T:              msg = _( "Label" );                  break;
    case SCH_GLOBAL_LABEL_T:       msg = _( "Global Label" );           break;
    case SCH_HIERARCHICAL_LABEL_T: msg = _( "Hierarchical Label" );     break;
    case SCH_SHEET_PIN_T:          msg = _( "Hierarchical Sheet Pin" ); break;
    default: return;
    }

    aList.push_back( MSG_PANEL_ITEM( msg, GetShownText(), DARKCYAN ) );

    switch( GetLabelSpinStyle() )
    {
    case 0:  msg = _( "Horizontal" );        break;
    case 1:  msg = _( "Vertical up" );       break;
    case 2:  msg = _( "Horizontal invert" ); break;
    case 3:  msg = _( "Vertical down" );     break;
    default: msg = wxT( "???" );             break;
    }

    aList.push_back( MSG_PANEL_ITEM( _( "Orientation" ), msg, BROWN ) );

    wxString textStyle[] = { _( "Normal" ), _( "Italic" ), _( "Bold" ), _( "Bold Italic" ) };
    int style = 0;

    if( IsItalic() )
        style = 1;

    if( IsBold() )
        style += 2;

    aList.push_back( MSG_PANEL_ITEM( _( "Style" ), textStyle[style], BROWN ) );


    // Display electricat type if it is relevant
    if( (Type() == SCH_GLOBAL_LABEL_T) ||
        (Type() == SCH_HIERARCHICAL_LABEL_T ) ||
        (Type() == SCH_SHEET_PIN_T ) )
    {
        switch( GetShape() )
        {
        case NET_INPUT:        msg = _( "Input" );           break;
        case NET_OUTPUT:       msg = _( "Output" );          break;
        case NET_BIDI:         msg = _( "Bidirectional" );   break;
        case NET_TRISTATE:     msg = _( "Tri-State" );       break;
        case NET_UNSPECIFIED:  msg = _( "Passive" );         break;
        default:               msg = wxT( "???" );           break;
        }

        aList.push_back( MSG_PANEL_ITEM( _( "Type" ), msg, BLUE ) );
    }

    // Display text size (X or Y value, with are the same value in Eeschema)
    msg = MessageTextFromValue( aUnits, GetTextWidth(), true );
    aList.push_back( MSG_PANEL_ITEM( _( "Size" ), msg, RED ) );
}

#if defined(DEBUG)

void SCH_TEXT::Show( int nestLevel, std::ostream& os ) const
{
    // XML output:
    wxString s = GetClass();

    NestedSpace( nestLevel, os ) << '<' << s.Lower().mb_str()
                                 << " layer=\"" << m_Layer << '"'
                                 << " shape=\"" << m_shape << '"'
                                 << " dangling=\"" << m_isDangling << '"'
                                 << '>'
                                 << TO_UTF8( m_Text )
                                 << "</" << s.Lower().mb_str() << ">\n";
}

#endif


SCH_LABEL::SCH_LABEL( const wxPoint& pos, const wxString& text ) :
    SCH_TEXT( pos, text, SCH_LABEL_T )
{
    m_Layer = LAYER_LOCLABEL;
    m_shape = NET_INPUT;
    m_isDangling = true;
    SetMultilineAllowed( false );
}


EDA_ITEM* SCH_LABEL::Clone() const
{
    return new SCH_LABEL( *this );
}


const EDA_RECT SCH_LABEL::GetBoundingBox() const
{
    int         linewidth = GetThickness() == 0 ? GetDefaultLineThickness() : GetThickness();
    EDA_RECT    rect = GetTextBox( -1, linewidth );

    if( GetTextAngle() != 0.0 )
    {
        // Rotate rect
        wxPoint pos = rect.GetOrigin();
        wxPoint end = rect.GetEnd();

        RotatePoint( &pos, GetTextPos(), GetTextAngle() );
        RotatePoint( &end, GetTextPos(), GetTextAngle() );

        rect.SetOrigin( pos );
        rect.SetEnd( end );

        rect.Normalize();
    }

    return rect;
}


wxString SCH_LABEL::GetSelectMenuText( EDA_UNITS_T aUnits ) const
{
    return wxString::Format( _( "Label %s" ), ShortenedShownText() );
}


BITMAP_DEF SCH_LABEL::GetMenuImage() const
{
    return add_line_label_xpm;
}


SCH_GLOBALLABEL::SCH_GLOBALLABEL( const wxPoint& pos, const wxString& text ) :
    SCH_TEXT( pos, text, SCH_GLOBAL_LABEL_T )
{
    m_Layer = LAYER_GLOBLABEL;
    m_shape = NET_BIDI;
    m_isDangling = true;
    SetMultilineAllowed( false );
}


EDA_ITEM* SCH_GLOBALLABEL::Clone() const
{
    return new SCH_GLOBALLABEL( *this );
}


wxPoint SCH_GLOBALLABEL::GetSchematicTextOffset() const
{
    wxPoint text_offset;
    int     width = GetThickness() == 0 ? GetDefaultLineThickness() : GetThickness();

    width = Clamp_Text_PenSize( width, GetTextSize(), IsBold() );
    int     halfSize = GetTextWidth() / 2;
    int     offset   = width;

    switch( m_shape )
    {
    case NET_INPUT:
    case NET_BIDI:
    case NET_TRISTATE:
        offset += halfSize;
        break;

    case NET_OUTPUT:
    case NET_UNSPECIFIED:
        offset += TXT_MARGIN;
        break;

    default:
        break;
    }

    switch( GetLabelSpinStyle() )
    {
    default:
    case 0: text_offset.x -= offset; break;    // Orientation horiz normal
    case 1: text_offset.y -= offset; break;    // Orientation vert UP
    case 2: text_offset.x += offset; break;    // Orientation horiz inverse
    case 3: text_offset.y += offset; break;    // Orientation vert BOTTOM
    }

    return text_offset;
}


void SCH_GLOBALLABEL::SetLabelSpinStyle( int aSpinStyle )
{
    m_spin_style = aSpinStyle;

    switch( aSpinStyle )
    {
    default:
    case 0: // Horiz Normal Orientation
        SetTextAngle( TEXT_ANGLE_HORIZ );
        SetHorizJustify( GR_TEXT_HJUSTIFY_RIGHT );
        SetVertJustify( GR_TEXT_VJUSTIFY_CENTER );
        break;

    case 1: // Vert Orientation UP
        SetTextAngle( TEXT_ANGLE_VERT );
        SetHorizJustify( GR_TEXT_HJUSTIFY_LEFT );
        SetVertJustify( GR_TEXT_VJUSTIFY_CENTER );
        break;

    case 2: // Horiz Orientation
        SetTextAngle( TEXT_ANGLE_HORIZ );
        SetHorizJustify( GR_TEXT_HJUSTIFY_LEFT );
        SetVertJustify( GR_TEXT_VJUSTIFY_CENTER );
        break;

    case 3: //  Vert Orientation BOTTOM
        SetTextAngle( TEXT_ANGLE_VERT );
        SetHorizJustify( GR_TEXT_HJUSTIFY_RIGHT );
        SetVertJustify( GR_TEXT_VJUSTIFY_CENTER );
        break;
    }
}


void SCH_GLOBALLABEL::Draw( EDA_DRAW_PANEL* panel,
                            wxDC*           DC,
                            const wxPoint&  aOffset,
                            GR_DRAWMODE     DrawMode,
                            COLOR4D        Color )
{
    static std::vector <wxPoint> Poly;
    COLOR4D color;
    wxPoint     text_offset = aOffset + GetSchematicTextOffset();

    if( Color != COLOR4D::UNSPECIFIED )
        color = Color;
    else
        color = GetLayerColor( GetState( BRIGHTENED ) ? LAYER_BRIGHTENED : m_Layer );

    GRSetDrawMode( DC, DrawMode );

    int linewidth = GetThickness() == 0 ? GetDefaultLineThickness() : GetThickness();

    linewidth = Clamp_Text_PenSize( linewidth, GetTextSize(), IsBold() );

    int save_width = GetThickness();
    SetThickness( linewidth );

    EDA_RECT* clipbox = panel? panel->GetClipBox() : NULL;
    EDA_TEXT::Draw( clipbox, DC, text_offset, color, DrawMode, FILLED, COLOR4D::UNSPECIFIED );

    SetThickness( save_width );   // restore initial value

    CreateGraphicShape( Poly, GetTextPos() + aOffset );
    GRPoly( clipbox, DC, Poly.size(), &Poly[0], 0, linewidth, color, color );

    if( m_isDangling && panel )
        DrawDanglingSymbol( panel, DC, GetTextPos() + aOffset, color );

    // Enable these line to draw the bounding box (debug tests purposes only)
#if DRAW_BBOX
    {
        EDA_RECT BoundaryBox = GetBoundingBox();
        GRRect( clipbox, DC, BoundaryBox, 0, BROWN );
    }
#endif
}


void SCH_GLOBALLABEL::CreateGraphicShape( std::vector <wxPoint>& aPoints, const wxPoint& Pos )
{
    int halfSize  = GetTextHeight() / 2;
    int linewidth = GetThickness() == 0 ? GetDefaultLineThickness() : GetThickness();

    linewidth = Clamp_Text_PenSize( linewidth, GetTextSize(), IsBold() );

    aPoints.clear();

    int symb_len = LenSize( GetShownText(), linewidth ) + ( TXT_MARGIN * 2 );

    // Create outline shape : 6 points
    int x = symb_len + linewidth + 3;

    // Use negation bar Y position to calculate full vertical size
    // Search for overbar symbol
    bool hasOverBar = false;

    for( unsigned ii = 1; ii < m_Text.size(); ii++ )
    {
        if( m_Text[ii-1] == '~' && m_Text[ii] != '~' )
        {
            hasOverBar = true;
            break;
        }
    }

    #define Y_CORRECTION 1.40
    // Note: this factor is due to the fact the Y size of a few letters like [
    // are bigger than the y size value, and we need a margin for the graphic symbol.
    int y = KiROUND( halfSize * Y_CORRECTION );

    // Note: this factor is due to the fact we need a margin for the graphic symbol.
    #define Y_OVERBAR_CORRECTION 1.2
    if( hasOverBar )
        y = KiROUND( KIGFX::STROKE_FONT::GetInterline( halfSize, linewidth )
                     * Y_OVERBAR_CORRECTION );

    // Gives room for line thickess and margin
    y += linewidth          // for line thickess
         + linewidth/2;     // for margin

    // Starting point(anchor)
    aPoints.push_back( wxPoint( 0, 0 ) );
    aPoints.push_back( wxPoint( 0, -y ) );     // Up
    aPoints.push_back( wxPoint( -x, -y ) );    // left
    aPoints.push_back( wxPoint( -x, 0 ) );     // Up left
    aPoints.push_back( wxPoint( -x, y ) );     // left down
    aPoints.push_back( wxPoint( 0, y ) );      // down

    int x_offset = 0;

    switch( m_shape )
    {
    case NET_INPUT:
        x_offset = -halfSize;
        aPoints[0].x += halfSize;
        break;

    case NET_OUTPUT:
        aPoints[3].x -= halfSize;
        break;

    case NET_BIDI:
    case NET_TRISTATE:
        x_offset = -halfSize;
        aPoints[0].x += halfSize;
        aPoints[3].x -= halfSize;
        break;

    case NET_UNSPECIFIED:
    default:
        break;
    }

    int angle = 0;

    switch( GetLabelSpinStyle() )
    {
    default:
    case 0:               break;   // Orientation horiz normal
    case 1: angle = -900; break;   // Orientation vert UP
    case 2: angle = 1800; break;   // Orientation horiz inverse
    case 3: angle = 900;  break;   // Orientation vert BOTTOM
    }

    // Rotate outlines and move corners in real position
    for( wxPoint& aPoint : aPoints )
    {
        aPoint.x += x_offset;

        if( angle )
            RotatePoint( &aPoint, angle );

        aPoint += Pos;
    }

    aPoints.push_back( aPoints[0] ); // closing
}


const EDA_RECT SCH_GLOBALLABEL::GetBoundingBox() const
{
    int x, y, dx, dy, length, height;

    x  = GetTextPos().x;
    y  = GetTextPos().y;
    dx = dy = 0;

    int width = GetThickness() == 0 ? GetDefaultLineThickness() : GetThickness();

    height = ( (GetTextHeight() * 15) / 10 ) + width + 2 * TXT_MARGIN;

    // text X size add height for triangular shapes(bidirectional)
    length = LenSize( GetShownText(), width ) + height + DANGLING_SYMBOL_SIZE;

    switch( GetLabelSpinStyle() )    // respect orientation
    {
    default:
    case 0:                             // Horiz Normal Orientation (left justified)
        dx = -length;
        dy = height;
        x += DANGLING_SYMBOL_SIZE;
        y -= height / 2;
        break;

    case 1:     // Vert Orientation UP
        dx = height;
        dy = -length;
        x -= height / 2;
        y += DANGLING_SYMBOL_SIZE;
        break;

    case 2:     // Horiz Orientation - Right justified
        dx = length;
        dy = height;
        x -= DANGLING_SYMBOL_SIZE;
        y -= height / 2;
        break;

    case 3:     //  Vert Orientation BOTTOM
        dx = height;
        dy = length;
        x -= height / 2;
        y -= DANGLING_SYMBOL_SIZE;
        break;
    }

    EDA_RECT box( wxPoint( x, y ), wxSize( dx, dy ) );
    box.Normalize();
    return box;
}


wxString SCH_GLOBALLABEL::GetSelectMenuText( EDA_UNITS_T aUnits ) const
{
    return wxString::Format( _( "Global Label %s" ), ShortenedShownText() );
}


BITMAP_DEF SCH_GLOBALLABEL::GetMenuImage() const
{
    return add_glabel_xpm;
}



SCH_HIERLABEL::SCH_HIERLABEL( const wxPoint& pos, const wxString& text, KICAD_T aType ) :
    SCH_TEXT( pos, text, aType )
{
    m_Layer = LAYER_HIERLABEL;
    m_shape = NET_INPUT;
    m_isDangling = true;
    SetMultilineAllowed( false );
}


EDA_ITEM* SCH_HIERLABEL::Clone() const
{
    return new SCH_HIERLABEL( *this );
}


void SCH_HIERLABEL::SetLabelSpinStyle( int aSpinStyle )
{
    m_spin_style = aSpinStyle;

    switch( aSpinStyle )
    {
    default:
    case 0: // Horiz Normal Orientation
        SetTextAngle( TEXT_ANGLE_HORIZ );
        SetHorizJustify( GR_TEXT_HJUSTIFY_RIGHT );
        SetVertJustify( GR_TEXT_VJUSTIFY_CENTER );
        break;

    case 1: // Vert Orientation UP
        SetTextAngle( TEXT_ANGLE_VERT );
        SetHorizJustify( GR_TEXT_HJUSTIFY_LEFT );
        SetVertJustify( GR_TEXT_VJUSTIFY_CENTER );
        break;

    case 2: // Horiz Orientation
        SetTextAngle( TEXT_ANGLE_HORIZ );
        SetHorizJustify( GR_TEXT_HJUSTIFY_LEFT );
        SetVertJustify( GR_TEXT_VJUSTIFY_CENTER );
        break;

    case 3: //  Vert Orientation BOTTOM
        SetTextAngle( TEXT_ANGLE_VERT );
        SetHorizJustify( GR_TEXT_HJUSTIFY_RIGHT );
        SetVertJustify( GR_TEXT_VJUSTIFY_CENTER );
        break;
    }
}


void SCH_HIERLABEL::Draw( EDA_DRAW_PANEL* panel,
                          wxDC*           DC,
                          const wxPoint&  offset,
                          GR_DRAWMODE     DrawMode,
                          COLOR4D         Color )
{
    static std::vector <wxPoint> Poly;
    COLOR4D color;
    int         linewidth = GetThickness() == 0 ? GetDefaultLineThickness() : GetThickness();
    EDA_RECT*   clipbox = panel? panel->GetClipBox() : NULL;

    linewidth = Clamp_Text_PenSize( linewidth, GetTextSize(), IsBold() );

    if( Color != COLOR4D::UNSPECIFIED )
        color = Color;
    else
        color = GetLayerColor( GetState( BRIGHTENED ) ? LAYER_BRIGHTENED : m_Layer );

    GRSetDrawMode( DC, DrawMode );

    int save_width = GetThickness();
    SetThickness( linewidth );

    wxPoint text_offset = offset + GetSchematicTextOffset();
    EDA_TEXT::Draw( clipbox, DC, text_offset, color, DrawMode, FILLED, COLOR4D::UNSPECIFIED );

    SetThickness( save_width );         // restore initial value

    CreateGraphicShape( Poly, GetTextPos() + offset );
    GRPoly( clipbox, DC, Poly.size(), &Poly[0], 0, linewidth, color, color );

    if( m_isDangling && panel )
        DrawDanglingSymbol( panel, DC, GetTextPos() + offset, color );

    // Enable these line to draw the bounding box (debug tests purposes only)
#if DRAW_BBOX
    {
        EDA_RECT BoundaryBox = GetBoundingBox();
        GRRect( clipbox, DC, BoundaryBox, 0, BROWN );
    }
#endif
}


void SCH_HIERLABEL::CreateGraphicShape( std::vector <wxPoint>& aPoints, const wxPoint& Pos )
{
    int* Template = TemplateShape[m_shape][m_spin_style];
    int  halfSize = GetTextWidth() / 2;
    int  imax = *Template; Template++;

    aPoints.clear();

    for( int ii = 0; ii < imax; ii++ )
    {
        wxPoint corner;
        corner.x = ( halfSize * (*Template) ) + Pos.x;
        Template++;

        corner.y = ( halfSize * (*Template) ) + Pos.y;
        Template++;

        aPoints.push_back( corner );
    }
}


const EDA_RECT SCH_HIERLABEL::GetBoundingBox() const
{
    int x, y, dx, dy, length, height;

    x  = GetTextPos().x;
    y  = GetTextPos().y;
    dx = dy = 0;

    int width = GetThickness() == 0 ? GetDefaultLineThickness() : GetThickness();

    height = GetTextHeight() + width + 2 * TXT_MARGIN;
    length = LenSize( GetShownText(), width )
             + height                 // add height for triangular shapes
             + 2 * DANGLING_SYMBOL_SIZE;

    switch( GetLabelSpinStyle() )
    {
    default:
    case 0:                             // Horiz Normal Orientation (left justified)
        dx = -length;
        dy = height;
        x += DANGLING_SYMBOL_SIZE;
        y -= height / 2;
        break;

    case 1:     // Vert Orientation UP
        dx = height;
        dy = -length;
        x -= height / 2;
        y += DANGLING_SYMBOL_SIZE;
        break;

    case 2:     // Horiz Orientation - Right justified
        dx = length;
        dy = height;
        x -= DANGLING_SYMBOL_SIZE;
        y -= height / 2;
        break;

    case 3:     //  Vert Orientation BOTTOM
        dx = height;
        dy = length;
        x -= height / 2;
        y -= DANGLING_SYMBOL_SIZE;
        break;
    }

    EDA_RECT box( wxPoint( x, y ), wxSize( dx, dy ) );
    box.Normalize();
    return box;
}


wxPoint SCH_HIERLABEL::GetSchematicTextOffset() const
{
    wxPoint text_offset;
    int     width = std::max( GetThickness(), GetDefaultLineThickness() );
    int     ii = GetTextWidth() + TXT_MARGIN + width;

    switch( GetLabelSpinStyle() )
    {
    default:
    case 0: text_offset.x = -ii; break;  // Orientation horiz normale
    case 1: text_offset.y = -ii; break;  // Orientation vert UP
    case 2: text_offset.x =  ii; break;  // Orientation horiz inverse
    case 3: text_offset.y =  ii; break;  // Orientation vert BOTTOM
    }

    return text_offset;
}


wxString SCH_HIERLABEL::GetSelectMenuText( EDA_UNITS_T aUnits ) const
{
    return wxString::Format( _( "Hierarchical Label %s" ), ShortenedShownText() );
}


BITMAP_DEF SCH_HIERLABEL::GetMenuImage() const
{
    return add_hierarchical_label_xpm;
}
