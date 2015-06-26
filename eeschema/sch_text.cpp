/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2015 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 2015 Wayne Stambaugh <stambaughw@verizon.net>
 * Copyright (C) 1992-2015 KiCad Developers, see AUTHORS.txt for contributors.
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
 * @brief Code for handling schematic sheet labels.
 */

#include <fctsys.h>
#include <gr_basic.h>
#include <macros.h>
#include <trigo.h>
#include <eeschema_id.h>
#include <class_drawpanel.h>
#include <drawtxt.h>
#include <schframe.h>
#include <plot_common.h>
#include <base_units.h>
#include <msgpanel.h>

#include <general.h>
#include <protos.h>
#include <sch_text.h>
#include <class_netlist_object.h>


extern void IncrementLabelMember( wxString& name, int aIncrement );


/* Names of sheet label types. */
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
    EDA_TEXT( text )
{
    m_Layer = LAYER_NOTES;
    m_Pos = pos;
    m_shape = 0;
    m_isDangling = false;
    m_MultilineAllowed = true;
    m_schematicOrientation = 0;
}


SCH_TEXT::SCH_TEXT( const SCH_TEXT& aText ) :
    SCH_ITEM( aText ),
    EDA_TEXT( aText )
{
    m_Pos = aText.m_Pos;
    m_shape = aText.m_shape;
    m_MultilineAllowed = aText.m_MultilineAllowed;
    m_schematicOrientation = aText.m_schematicOrientation;
    m_isDangling = false;
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

    // add a small offset (TXTMARGE) to x ( or y) position to allow a text to
    // be on a wire or a line and be readable
    switch( m_schematicOrientation )
    {
    default:
    case 0: /* Horiz Normal Orientation (left justified) */
        text_offset.y = -TXTMARGE;
        break;

    case 1: /* Vert Orientation UP */
        text_offset.x = -TXTMARGE;
        break;

    case 2: /* Horiz Orientation - Right justified */
        text_offset.y = -TXTMARGE;
        break;

    case 3: /*  Vert Orientation BOTTOM */
        text_offset.x = -TXTMARGE;
        break;
    }

    return text_offset;
}


bool SCH_TEXT::Matches( wxFindReplaceData& aSearchData, void* aAuxData, wxPoint * aFindLocation )
{
    wxLogTrace( traceFindItem, wxT( "  item " ) + GetSelectMenuText() );

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
    switch( GetOrientation() )
    {
    case 0: // horizontal text
        SetOrientation( 2 );
        break;

    case 2: // invert horizontal text
        SetOrientation( 0 );
        break;

    case 1: // Vert Orientation UP
    case 3: // Vert Orientation BOTTOM
    default:
        break;
    }

    MIRROR( m_Pos.x, aYaxis_position );
}


void SCH_TEXT::MirrorX( int aXaxis_position )
{
    // Text is NOT really mirrored; it is moved to a suitable vertical position
    switch( GetOrientation() )
    {
    case 1: // Vert Orientation UP
        SetOrientation( 3 );
        break;

    case 3: // Vert Orientation BOTTOM
        SetOrientation( 1 );
        break;

    case 0: // horizontal text
    case 2: // invert horizontal text
    default:
        break;
    }

    MIRROR( m_Pos.y, aXaxis_position );
}


void SCH_TEXT::Rotate( wxPoint aPosition )
{
    int dy;

    RotatePoint( &m_Pos, aPosition, 900 );
    SetOrientation( (GetOrientation() + 1) % 4 );

    switch( GetOrientation() )
    {
    case 0:     // horizontal text
        dy = m_Size.y;
        break;

    case 1:     // Vert Orientation UP
        dy = 0;
        break;

    case 2:     // invert horizontal text
        dy = m_Size.y;
        break;

    case 3:     // Vert Orientation BOTTOM
        dy = 0;
        break;

    default:
        dy = 0;
        break;
    }

    m_Pos.y += dy;
}


void SCH_TEXT::SetOrientation( int aOrientation )
{
    m_schematicOrientation = aOrientation;

    switch( m_schematicOrientation )
    {
    default:
    case 0: /* Horiz Normal Orientation (left justified) */
        m_Orient   = TEXT_ORIENT_HORIZ;
        m_HJustify = GR_TEXT_HJUSTIFY_LEFT;
        m_VJustify = GR_TEXT_VJUSTIFY_BOTTOM;
        break;

    case 1: /* Vert Orientation UP */
        m_Orient   = TEXT_ORIENT_VERT;
        m_HJustify = GR_TEXT_HJUSTIFY_LEFT;
        m_VJustify = GR_TEXT_VJUSTIFY_BOTTOM;
        break;

    case 2: /* Horiz Orientation - Right justified */
        m_Orient   = TEXT_ORIENT_HORIZ;
        m_HJustify = GR_TEXT_HJUSTIFY_RIGHT;
        m_VJustify = GR_TEXT_VJUSTIFY_BOTTOM;
        break;

    case 3: /*  Vert Orientation BOTTOM */
        m_Orient   = TEXT_ORIENT_VERT;
        m_HJustify = GR_TEXT_HJUSTIFY_RIGHT;
        m_VJustify = GR_TEXT_VJUSTIFY_BOTTOM;
        break;
    }
}


void SCH_TEXT::SwapData( SCH_ITEM* aItem )
{
    SCH_TEXT* item = (SCH_TEXT*) aItem;

    std::swap( m_Text, item->m_Text );
    std::swap( m_Pos, item->m_Pos );
    std::swap( m_Size, item->m_Size );
    std::swap( m_Thickness, item->m_Thickness );
    std::swap( m_shape, item->m_shape );
    std::swap( m_Orient, item->m_Orient );

    std::swap( m_Layer, item->m_Layer );
    std::swap( m_HJustify, item->m_HJustify );
    std::swap( m_VJustify, item->m_VJustify );
    std::swap( m_isDangling, item->m_isDangling );
    std::swap( m_schematicOrientation, item->m_schematicOrientation );
}


int SCH_TEXT::GetPenSize() const
{
    int pensize = m_Thickness;

    if( pensize == 0 )   // Use default values for pen size
    {
        if( m_Bold  )
            pensize = GetPenSizeForBold( m_Size.x );
        else
            pensize = GetDefaultLineThickness();
    }

    // Clip pen size for small texts:
    pensize = Clamp_Text_PenSize( pensize, m_Size, m_Bold );
    return pensize;
}


void SCH_TEXT::Draw( EDA_DRAW_PANEL* panel, wxDC* DC, const wxPoint& aOffset,
                     GR_DRAWMODE DrawMode, EDA_COLOR_T Color )
{
    EDA_COLOR_T color;
    int         linewidth = ( m_Thickness == 0 ) ? GetDefaultLineThickness() : m_Thickness;
    EDA_RECT* clipbox = panel? panel->GetClipBox() : NULL;

    linewidth = Clamp_Text_PenSize( linewidth, m_Size, m_Bold );

    if( Color >= 0 )
        color = Color;
    else
        color = GetLayerColor( m_Layer );

    GRSetDrawMode( DC, DrawMode );

    wxPoint text_offset = aOffset + GetSchematicTextOffset();
    std::swap( linewidth, m_Thickness );            // Set the minimum width
    EDA_TEXT::Draw( clipbox, DC, text_offset, color, DrawMode, FILLED, UNSPECIFIED_COLOR );
    std::swap( linewidth, m_Thickness );            // set initial value

    if( m_isDangling && panel)
        DrawDanglingSymbol( panel, DC, m_Pos + aOffset, color );

    // Enable these line to draw the bounding box (debug tests purposes only)
#if 0
    {
        EDA_RECT BoundaryBox = GetBoundingBox();
        GRRect( clipbox, DC, BoundaryBox, 0, BROWN );
    }
#endif
}


bool SCH_TEXT::Save( FILE* aFile ) const
{
    bool        success = true;
    const char* shape   = "~";

    if( m_Italic )
        shape = "Italic";

    // For compatibility reason, the text must be saved in only one text line
    // so we replace all E.O.L. by \\n
    wxString text = m_Text;

    text.Replace( wxT("\n"), wxT( "\\n" ) );

    // Here we should have no CR or LF character in line
    // This is not always the case if a multiline text was copied (using a copy/paste function)
    // from a text that uses E.O.L characters that differs from the current EOL format
    // This is mainly the case under Linux using LF symbol when copying a text from
    // Windows (using CRLF symbol)
    // So we must just remove the extra CR left (or LF left under MacOSX)
    for( unsigned ii = 0; ii < text.Len();  )
    {
        if( text[ii] == 0x0A || text[ii] == 0x0D )
            text.erase( ii, 1 );
        else
            ii++;
    }


    if( fprintf( aFile, "Text Notes %-4d %-4d %-4d %-4d %s %d\n%s\n",
                 m_Pos.x, m_Pos.y, m_schematicOrientation, m_Size.x,
                 shape, m_Thickness, TO_UTF8( text ) ) == EOF )
    {
        success = false;
    }

    return success;
}


bool SCH_TEXT::Load( LINE_READER& aLine, wxString& aErrorMsg )
{
    char      Name1[256];
    char      Name2[256];
    char      Name3[256];
    int       thickness = 0, size = 0, orient = 0;

    Name1[0] = 0; Name2[0] = 0; Name3[0] = 0;

    char*     sline = (char*) aLine;

    while( ( *sline != ' ' ) && *sline )
        sline++;

    // sline points the start of parameters
    int ii = sscanf( sline, "%255s %d %d %d %d %255s %255s %d", Name1, &m_Pos.x, &m_Pos.y,
                     &orient, &size, Name2, Name3, &thickness );

    if( ii < 4 )
    {
        aErrorMsg.Printf( wxT( "Eeschema file text load error at line %d" ),
                          aLine.LineNumber() );
        return false;
    }

    if( !aLine.ReadLine() )
    {
        aErrorMsg.Printf( wxT( "Eeschema file text load error at line %d" ),
                          aLine.LineNumber() );
        return false;
    }

    if( size == 0 )
        size = GetDefaultTextSize();

    char* text = strtok( (char*) aLine, "\n\r" );

    if( text == NULL )
    {
        aErrorMsg.Printf( wxT( "Eeschema file text load error at line %d" ),
                          aLine.LineNumber() );
        return false;
    }

    wxString val = FROM_UTF8( text );

    for( ; ; )
    {
        int i = val.find( wxT( "\\n" ) );

        if( i == wxNOT_FOUND )
            break;

        val.erase( i, 2 );
        val.insert( i, wxT( "\n" ) );
    }

    m_Text = val;
    m_Size.x = m_Size.y = size;
    SetOrientation( orient );

    if( isdigit( Name3[0] ) )
    {
        thickness = atol( Name3 );
        m_Bold = ( thickness != 0 );
        m_Thickness = m_Bold ? GetPenSizeForBold( size ) : 0;
    }

    if( strnicmp( Name2, "Italic", 6 ) == 0 )
        m_Italic = 1;

    return true;
}


void SCH_TEXT::GetEndPoints( std::vector <DANGLING_END_ITEM>& aItemList )
{
    // Normal text labels cannot be tested for dangling ends.
    if( Type() == SCH_TEXT_T )
        return;

    DANGLING_END_ITEM item( LABEL_END, this, m_Pos );
    aItemList.push_back( item );
}


bool SCH_TEXT::IsDanglingStateChanged( std::vector< DANGLING_END_ITEM >& aItemList )
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
            if( m_Pos == item.GetPosition() )
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
            m_isDangling = !IsPointOnSegment( item.GetPosition(), nextItem.GetPosition(), m_Pos );
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

    if( aRect.Contains( m_Pos ) )
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

    aPoints.push_back( m_Pos );
}


const EDA_RECT SCH_TEXT::GetBoundingBox() const
{
    // We must pass the effective text thickness to GetTextBox
    // when calculating the bounding box
    int linewidth = ( m_Thickness == 0 ) ? GetDefaultLineThickness() : m_Thickness;

    linewidth = Clamp_Text_PenSize( linewidth, m_Size, m_Bold );

    EDA_RECT rect = GetTextBox( -1, linewidth );

    if( m_Orient )                          // Rotate rect
    {
        wxPoint pos = rect.GetOrigin();
        wxPoint end = rect.GetEnd();
        RotatePoint( &pos, m_Pos, m_Orient );
        RotatePoint( &end, m_Pos, m_Orient );
        rect.SetOrigin( pos );
        rect.SetEnd( end );
    }

    rect.Normalize();
    return rect;
}


wxString SCH_TEXT::GetSelectMenuText() const
{
    wxString msg;
    msg.Printf( _( "Graphic Text %s" ), GetChars( ShortenedShownText() ) );
    return msg;
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
    item->m_Start = item->m_End = m_Pos;

    aNetListItems.push_back( item );

    /* If a bus connects to label */
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

    EDA_COLOR_T color = GetLayerColor( GetLayer() );
    wxPoint     textpos   = m_Pos + GetSchematicTextOffset();
    int         thickness = GetPenSize();

    aPlotter->SetCurrentLineWidth( thickness );

    if( m_MultilineAllowed )
    {
        std::vector<wxPoint> positions;
        wxArrayString strings_list;
        wxStringSplit( GetShownText(), strings_list, '\n' );
        positions.reserve( strings_list.Count() );

        GetPositionsOfLinesOfMultilineText(positions, strings_list.Count() );

        for( unsigned ii = 0; ii < strings_list.Count(); ii++ )
        {
            wxString& txt = strings_list.Item( ii );
            aPlotter->Text( positions[ii], color, txt, m_Orient, m_Size, m_HJustify,
                            m_VJustify, thickness, m_Italic, m_Bold );
        }
    }
    else
    {
        aPlotter->Text( textpos, color, GetShownText(), m_Orient, m_Size, m_HJustify,
                        m_VJustify, thickness, m_Italic, m_Bold );
    }

    /* Draw graphic symbol for global or hierarchical labels */
    CreateGraphicShape( Poly, m_Pos );

    aPlotter->SetCurrentLineWidth( GetPenSize() );

    if( Poly.size() )
        aPlotter->PlotPoly( Poly, NO_FILL );
}


/*
 * Display the type, shape, size and some other props to the Message panel
 */
void SCH_TEXT::GetMsgPanelInfo( MSG_PANEL_ITEMS& aList )
{
    wxString msg;

    switch( Type() )
    {
    case SCH_TEXT_T:
        msg = _( "Graphic Text" );
        break;

    case SCH_LABEL_T:
        msg = _( "Label" );
        break;

    case SCH_GLOBAL_LABEL_T:
        msg = _( "Global Label" );
        break;

    case SCH_HIERARCHICAL_LABEL_T:
        msg = _( "Hierarchical Label" );
        break;

    case SCH_SHEET_PIN_T:
        msg = _( "Hierarchical Sheet Pin" );
        break;

    default:
        return;
    }

    aList.push_back( MSG_PANEL_ITEM( msg, GetShownText(), DARKCYAN ) );

    switch( GetOrientation() )
    {
    case 0:     // horizontal text
        msg = _( "Horizontal" );
        break;

    case 1:     // Vert Orientation UP
        msg = _( "Vertical up" );
        break;

    case 2:     // invert horizontal text
        msg = _( "Horizontal invert" );
        break;

    case 3:     // Vert Orientation Down
        msg = _( "Vertical down" );
        break;

    default:
        msg = wxT( "???" );
        break;
    }

    aList.push_back( MSG_PANEL_ITEM( _( "Orientation" ), msg, BROWN ) );

    wxString textStyle[] = { _( "Normal" ), _( "Italic" ), _( "Bold" ), _( "Bold Italic" ) };
    int style = 0;

    if( m_Italic )
        style = 1;

    if( m_Bold )
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
    msg = StringFromValue( g_UserUnit, m_Size.x, true );
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
    m_MultilineAllowed = false;
}


EDA_ITEM* SCH_LABEL::Clone() const
{
    return new SCH_LABEL( *this );
}


wxPoint SCH_LABEL::GetSchematicTextOffset() const
{
    return SCH_TEXT::GetSchematicTextOffset();
}


void SCH_LABEL::SetOrientation( int aOrientation )
{
    SCH_TEXT::SetOrientation( aOrientation );
}


void SCH_LABEL::MirrorX( int aXaxis_position )
{
    // Text is NOT really mirrored; it is moved to a suitable position
    switch( GetOrientation() )
    {
    case 1: // Vert Orientation UP
        SetOrientation( 3 );
        break;

    case 3: // Vert Orientation BOTTOM
        SetOrientation( 1 );
        break;

    case 0: // horizontal text
    case 2: // invert horizontal text
    default:
        break;
    }

    MIRROR( m_Pos.y, aXaxis_position );
}


void SCH_LABEL::Rotate( wxPoint aPosition )
{
    RotatePoint( &m_Pos, aPosition, 900 );
    SetOrientation( (GetOrientation() + 1) % 4 );
}


bool SCH_LABEL::Save( FILE* aFile ) const
{
    bool        success = true;
    const char* shape   = "~";

    if( m_Italic )
        shape = "Italic";

    if( fprintf( aFile, "Text Label %-4d %-4d %-4d %-4d %s %d\n%s\n",
                 m_Pos.x, m_Pos.y, m_schematicOrientation, m_Size.x, shape,
                 m_Thickness, TO_UTF8( m_Text ) ) == EOF )
    {
        success = false;
    }

    return success;
}


bool SCH_LABEL::Load( LINE_READER& aLine, wxString& aErrorMsg )
{
    char      Name1[256];
    char      Name2[256];
    char      Name3[256];
    int       thickness = 0, size = 0, orient = 0;

    Name1[0] = 0; Name2[0] = 0; Name3[0] = 0;

    char*     sline = (char*) aLine;

    while( ( *sline != ' ' ) && *sline )
        sline++;

    // sline points the start of parameters
    int ii = sscanf( sline, "%255s %d %d %d %d %255s %255s %d", Name1, &m_Pos.x, &m_Pos.y,
                     &orient, &size, Name2, Name3, &thickness );

    if( ii < 4 )
    {
        aErrorMsg.Printf( wxT( "Eeschema file label load error at line %d" ),
                          aLine.LineNumber() );
        return false;
    }

    if( !aLine.ReadLine() )
    {
        aErrorMsg.Printf( wxT( "Eeschema file label load error atline %d" ),
                          aLine.LineNumber() );
        return false;
    }

    if( size == 0 )
        size = GetDefaultTextSize();

    char* text = strtok( (char*) aLine, "\n\r" );

    if( text == NULL )
    {
        aErrorMsg.Printf( wxT( "Eeschema file label load error at line %d" ),
                          aLine.LineNumber() );
        return false;
    }

    m_Text = FROM_UTF8( text );
    m_Size.x = m_Size.y = size;
    SetOrientation( orient );

    if( isdigit( Name3[0] ) )
    {
        thickness = atol( Name3 );
        m_Bold = ( thickness != 0 );
        m_Thickness = m_Bold ? GetPenSizeForBold( size ) : 0;
    }

    if( stricmp( Name2, "Italic" ) == 0 )
        m_Italic = 1;

    return true;
}


void SCH_LABEL::Draw( EDA_DRAW_PANEL* panel, wxDC* DC, const wxPoint& offset,
                      GR_DRAWMODE DrawMode, EDA_COLOR_T Color )
{
    SCH_TEXT::Draw( panel, DC, offset, DrawMode, Color );
}


const EDA_RECT SCH_LABEL::GetBoundingBox() const
{
    int x, y, dx, dy, length, height;

    x = m_Pos.x;
    y = m_Pos.y;
    int width = (m_Thickness == 0) ? GetDefaultLineThickness() : m_Thickness;
    length = LenSize( GetShownText() );
    height = m_Size.y + width;
    dx     = dy = 0;

    switch( m_schematicOrientation )
    {
    case 0:             /* Horiz Normal Orientation (left justified) */
        dx = 2 * DANGLING_SYMBOL_SIZE + length;
        dy = -2 * DANGLING_SYMBOL_SIZE - height - TXTMARGE;
        x -= DANGLING_SYMBOL_SIZE;
        y += DANGLING_SYMBOL_SIZE;
        break;

    case 1:     /* Vert Orientation UP */
        dx = -2 * DANGLING_SYMBOL_SIZE - height - TXTMARGE;
        dy = -2 * DANGLING_SYMBOL_SIZE - length;
        x += DANGLING_SYMBOL_SIZE;
        y += DANGLING_SYMBOL_SIZE;
        break;

    case 2:     /* Horiz Orientation - Right justified */
        dx = -2 * DANGLING_SYMBOL_SIZE - length;
        dy = -2 * DANGLING_SYMBOL_SIZE - height - TXTMARGE;
        x += DANGLING_SYMBOL_SIZE;
        y += DANGLING_SYMBOL_SIZE;
        break;

    case 3:     /*  Vert Orientation BOTTOM */
        dx = -2 * DANGLING_SYMBOL_SIZE - height - TXTMARGE;
        dy = 2 * DANGLING_SYMBOL_SIZE + length;
        x += DANGLING_SYMBOL_SIZE;
        y -= DANGLING_SYMBOL_SIZE;
        break;
    }

    EDA_RECT box( wxPoint( x, y ), wxSize( dx, dy ) );
    box.Normalize();
    return box;
}


wxString SCH_LABEL::GetSelectMenuText() const
{
    wxString msg;
    msg.Printf( _( "Label %s" ), GetChars( ShortenedShownText() ) );
    return msg;
}



SCH_GLOBALLABEL::SCH_GLOBALLABEL( const wxPoint& pos, const wxString& text ) :
    SCH_TEXT( pos, text, SCH_GLOBAL_LABEL_T )
{
    m_Layer = LAYER_GLOBLABEL;
    m_shape = NET_BIDI;
    m_isDangling = true;
    m_MultilineAllowed = false;
}


EDA_ITEM* SCH_GLOBALLABEL::Clone() const
{
    return new SCH_GLOBALLABEL( *this );
}


bool SCH_GLOBALLABEL::Save( FILE* aFile ) const
{
    bool        success = true;
    const char* shape   = "~";

    if( m_Italic )
        shape = "Italic";

    if( fprintf( aFile, "Text GLabel %-4d %-4d %-4d %-4d %s %s %d\n%s\n",
                 m_Pos.x, m_Pos.y, m_schematicOrientation, m_Size.x,
                 SheetLabelType[m_shape], shape, m_Thickness, TO_UTF8( m_Text ) ) == EOF )
    {
        success = false;
    }

    return success;
}


bool SCH_GLOBALLABEL::Load( LINE_READER& aLine, wxString& aErrorMsg )
{
    char      Name1[256];
    char      Name2[256];
    char      Name3[256];
    int       thickness = 0, size = 0, orient = 0;

    Name1[0] = 0; Name2[0] = 0; Name3[0] = 0;

    char*     sline = (char*) aLine;

    while( (*sline != ' ' ) && *sline )
        sline++;

    // sline points the start of parameters
    int ii = sscanf( sline, "%255s %d %d %d %d %255s %255s %d", Name1, &m_Pos.x, &m_Pos.y,
                     &orient, &size, Name2, Name3, &thickness );

    if( ii < 4 )
    {
        aErrorMsg.Printf( wxT( "Eeschema file global label load error at line %d" ),
                          aLine.LineNumber() );
        return false;
    }

    if( !aLine.ReadLine() )
    {
        aErrorMsg.Printf( wxT( "Eeschema file global label load  error at line %d" ),
                          aLine.LineNumber() );
        return false;
    }

    if( size == 0 )
        size = GetDefaultTextSize();

    char* text = strtok( (char*) aLine, "\n\r" );

    if( text == NULL )
    {
        aErrorMsg.Printf( wxT( "Eeschema file global label load error at line %d" ),
                          aLine.LineNumber() );
        return false;
    }

    m_Text = FROM_UTF8( text );
    m_Size.x = m_Size.y = size;
    SetOrientation( orient );
    m_shape  = NET_INPUT;
    m_Bold = ( thickness != 0 );
    m_Thickness = m_Bold ? GetPenSizeForBold( size ) : 0;

    if( stricmp( Name2, SheetLabelType[NET_OUTPUT] ) == 0 )
        m_shape = NET_OUTPUT;

    if( stricmp( Name2, SheetLabelType[NET_BIDI] ) == 0 )
        m_shape = NET_BIDI;

    if( stricmp( Name2, SheetLabelType[NET_TRISTATE] ) == 0 )
        m_shape = NET_TRISTATE;

    if( stricmp( Name2, SheetLabelType[NET_UNSPECIFIED] ) == 0 )
        m_shape = NET_UNSPECIFIED;

    if( stricmp( Name3, "Italic" ) == 0 )
        m_Italic = 1;

    return true;
}


void SCH_GLOBALLABEL::MirrorY( int aYaxis_position )
{
    /* The global label is NOT really mirrored.
     *  for an horizontal label, the schematic orientation is changed.
     *  for a vertical label, the schematic orientation is not changed.
     *  and the label is moved to a suitable position
     */
    switch( GetOrientation() )
    {
    case 0:             /* horizontal text */
        SetOrientation( 2 );
        break;

    case 2:        /* invert horizontal text*/
        SetOrientation( 0 );
        break;
    }

    MIRROR( m_Pos.x, aYaxis_position );
}


void SCH_GLOBALLABEL::MirrorX( int aXaxis_position )
{
    switch( GetOrientation() )
    {
    case 1:             /* vertical text */
        SetOrientation( 3 );
        break;

    case 3:        /* invert vertical text*/
        SetOrientation( 1 );
        break;
    }

    MIRROR( m_Pos.y, aXaxis_position );
}


void SCH_GLOBALLABEL::Rotate( wxPoint aPosition )
{
    RotatePoint( &m_Pos, aPosition, 900 );
    SetOrientation( (GetOrientation() + 3) % 4 );
}


wxPoint SCH_GLOBALLABEL::GetSchematicTextOffset() const
{
    wxPoint text_offset;
    int     width = (m_Thickness == 0) ? GetDefaultLineThickness() : m_Thickness;

    width = Clamp_Text_PenSize( width, m_Size, m_Bold );
    int     HalfSize = m_Size.x / 2;
    int     offset   = width;

    switch( m_shape )
    {
    case NET_INPUT:
    case NET_BIDI:
    case NET_TRISTATE:
        offset += HalfSize;
        break;

    case NET_OUTPUT:
    case NET_UNSPECIFIED:
        offset += TXTMARGE;
        break;

    default:
        break;
    }

    switch( m_schematicOrientation )
    {
    case 0:             /* Orientation horiz normal */
        text_offset.x -= offset;
        break;

    case 1:             /* Orientation vert UP */
        text_offset.y -= offset;
        break;

    case 2:             /* Orientation horiz inverse */
        text_offset.x += offset;
        break;

    case 3:             /* Orientation vert BOTTOM */
        text_offset.y += offset;
        break;
    }

    return text_offset;
}


void SCH_GLOBALLABEL::SetOrientation( int aOrientation )
{
    m_schematicOrientation = aOrientation;

    switch( m_schematicOrientation )
    {
    default:
    case 0: /* Horiz Normal Orientation */
        m_Orient   = TEXT_ORIENT_HORIZ;
        m_HJustify = GR_TEXT_HJUSTIFY_RIGHT;
        m_VJustify = GR_TEXT_VJUSTIFY_CENTER;
        break;

    case 1: /* Vert Orientation UP */
        m_Orient   = TEXT_ORIENT_VERT;
        m_HJustify = GR_TEXT_HJUSTIFY_LEFT;
        m_VJustify = GR_TEXT_VJUSTIFY_CENTER;
        break;

    case 2: /* Horiz Orientation */
        m_Orient   = TEXT_ORIENT_HORIZ;
        m_HJustify = GR_TEXT_HJUSTIFY_LEFT;
        m_VJustify = GR_TEXT_VJUSTIFY_CENTER;
        break;

    case 3: /*  Vert Orientation BOTTOM */
        m_Orient   = TEXT_ORIENT_VERT;
        m_HJustify = GR_TEXT_HJUSTIFY_RIGHT;
        m_VJustify = GR_TEXT_VJUSTIFY_CENTER;
        break;
    }
}


void SCH_GLOBALLABEL::Draw( EDA_DRAW_PANEL* panel,
                            wxDC*           DC,
                            const wxPoint&  aOffset,
                            GR_DRAWMODE     DrawMode,
                            EDA_COLOR_T     Color )
{
    static std::vector <wxPoint> Poly;
    EDA_COLOR_T color;
    wxPoint     text_offset = aOffset + GetSchematicTextOffset();

    if( Color >= 0 )
        color = Color;
    else
        color = GetLayerColor( m_Layer );

    GRSetDrawMode( DC, DrawMode );

    int linewidth = (m_Thickness == 0) ? GetDefaultLineThickness() : m_Thickness;
    linewidth = Clamp_Text_PenSize( linewidth, m_Size, m_Bold );
    std::swap( linewidth, m_Thickness );            // Set the minimum width
    EDA_RECT* clipbox = panel? panel->GetClipBox() : NULL;
    EDA_TEXT::Draw( clipbox, DC, text_offset, color, DrawMode, FILLED, UNSPECIFIED_COLOR );
    std::swap( linewidth, m_Thickness );            // set initial value

    CreateGraphicShape( Poly, m_Pos + aOffset );
    GRPoly( clipbox, DC, Poly.size(), &Poly[0], 0, linewidth, color, color );

    if( m_isDangling && panel )
        DrawDanglingSymbol( panel, DC, m_Pos + aOffset, color );

    // Enable these line to draw the bounding box (debug tests purposes only)
#if 0
    {
        EDA_RECT BoundaryBox = GetBoundingBox();
        GRRect( clipbox, DC, BoundaryBox, 0, BROWN );
    }
#endif
}


void SCH_GLOBALLABEL::CreateGraphicShape( std::vector <wxPoint>& aPoints, const wxPoint& Pos )
{
    int HalfSize  = m_Size.y / 2;
    int linewidth = (m_Thickness == 0) ? GetDefaultLineThickness() : m_Thickness;

    linewidth = Clamp_Text_PenSize( linewidth, m_Size, m_Bold );

    aPoints.clear();

    int symb_len = LenSize( GetShownText() ) + ( TXTMARGE * 2 );

    // Create outline shape : 6 points
    int x = symb_len + linewidth + 3;

    // Use negation bar Y position to calculate full vertical size
    #define Y_CORRECTION 1.3
    // Note: this factor is due to the fact the negation bar Y position
    // does not give exactly the full Y size of text
    // and is experimentally set  to this value
    int y = KiROUND( OverbarPositionY( HalfSize ) * Y_CORRECTION );
    // add room for line thickness and space between top of text and graphic shape
    y += linewidth;

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
        x_offset = -HalfSize;
        aPoints[0].x += HalfSize;
        break;

    case NET_OUTPUT:
        aPoints[3].x -= HalfSize;
        break;

    case NET_BIDI:
    case NET_TRISTATE:
        x_offset = -HalfSize;
        aPoints[0].x += HalfSize;
        aPoints[3].x -= HalfSize;
        break;

    case NET_UNSPECIFIED:
    default:
        break;
    }

    int angle = 0;

    switch( m_schematicOrientation )
    {
    case 0:             /* Orientation horiz normal */
        break;

    case 1:             /* Orientation vert UP */
        angle = -900;
        break;

    case 2:             /* Orientation horiz inverse */
        angle = 1800;
        break;

    case 3:             /* Orientation vert BOTTOM */
        angle = 900;
        break;
    }

    // Rotate outlines and move corners in real position
    for( unsigned ii = 0; ii < aPoints.size(); ii++ )
    {
        aPoints[ii].x += x_offset;

        if( angle )
            RotatePoint( &aPoints[ii], angle );

        aPoints[ii] += Pos;
    }

    aPoints.push_back( aPoints[0] ); // closing
}


const EDA_RECT SCH_GLOBALLABEL::GetBoundingBox() const
{
    int x, y, dx, dy, length, height;

    x  = m_Pos.x;
    y  = m_Pos.y;
    dx = dy = 0;

    int width = (m_Thickness == 0) ? GetDefaultLineThickness() : m_Thickness;
    height = ( (m_Size.y * 15) / 10 ) + width + 2 * TXTMARGE;

    // text X size add height for triangular shapes(bidirectional)
    length = LenSize( GetShownText() ) + height + DANGLING_SYMBOL_SIZE;

    switch( m_schematicOrientation )    // respect orientation
    {
    case 0:                             /* Horiz Normal Orientation (left justified) */
        dx = -length;
        dy = height;
        x += DANGLING_SYMBOL_SIZE;
        y -= height / 2;
        break;

    case 1:     /* Vert Orientation UP */
        dx = height;
        dy = -length;
        x -= height / 2;
        y += DANGLING_SYMBOL_SIZE;
        break;

    case 2:     /* Horiz Orientation - Right justified */
        dx = length;
        dy = height;
        x -= DANGLING_SYMBOL_SIZE;
        y -= height / 2;
        break;

    case 3:     /*  Vert Orientation BOTTOM */
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


wxString SCH_GLOBALLABEL::GetSelectMenuText() const
{
    wxString msg;
    msg.Printf( _( "Global Label %s" ), GetChars( ShortenedShownText() ) );
    return msg;
}



SCH_HIERLABEL::SCH_HIERLABEL( const wxPoint& pos, const wxString& text, KICAD_T aType ) :
    SCH_TEXT( pos, text, aType )
{
    m_Layer = LAYER_HIERLABEL;
    m_shape = NET_INPUT;
    m_isDangling = true;
    m_MultilineAllowed = false;
}


EDA_ITEM* SCH_HIERLABEL::Clone() const
{
    return new SCH_HIERLABEL( *this );
}


bool SCH_HIERLABEL::Save( FILE* aFile ) const
{
    bool        success = true;
    const char* shape   = "~";

    if( m_Italic )
        shape = "Italic";

    if( fprintf( aFile, "Text HLabel %-4d %-4d %-4d %-4d %s %s %d\n%s\n",
                 m_Pos.x, m_Pos.y, m_schematicOrientation, m_Size.x,
                 SheetLabelType[m_shape], shape, m_Thickness, TO_UTF8( m_Text ) ) == EOF )
    {
        success = false;
    }

    return success;
}


bool SCH_HIERLABEL::Load( LINE_READER& aLine, wxString& aErrorMsg )
{
    char      Name1[256];
    char      Name2[256];
    char      Name3[256];
    int       thickness = 0, size = 0, orient = 0;

    Name1[0] = 0; Name2[0] = 0; Name3[0] = 0;

    char*     sline = (char*) aLine;

    while( (*sline != ' ' ) && *sline )
        sline++;

    // sline points the start of parameters
    int ii = sscanf( sline, "%255s %d %d %d %d %255s %255s %d", Name1, &m_Pos.x, &m_Pos.y,
                     &orient, &size, Name2, Name3, &thickness );

    if( ii < 4 )
    {
        aErrorMsg.Printf( wxT( "Eeschema file hierarchical label load error at line %d" ),
                          aLine.LineNumber() );
        return false;
    }

    if( !aLine.ReadLine() )
    {
        aErrorMsg.Printf( wxT( "Eeschema file hierarchical label load  error at line %d" ),
                          aLine.LineNumber() );
        return false;
    }

    if( size == 0 )
        size = GetDefaultTextSize();

    char* text = strtok( (char*) aLine, "\n\r" );

    if( text == NULL )
    {
        aErrorMsg.Printf( wxT( "Eeschema file hierarchical label load error at line %d" ),
                          aLine.LineNumber() );
        return false;
    }

    m_Text = FROM_UTF8( text );
    m_Size.x = m_Size.y = size;
    SetOrientation( orient );
    m_shape  = NET_INPUT;
    m_Bold = ( thickness != 0 );
    m_Thickness = m_Bold ? GetPenSizeForBold( size ) : 0;

    if( stricmp( Name2, SheetLabelType[NET_OUTPUT] ) == 0 )
        m_shape = NET_OUTPUT;

    if( stricmp( Name2, SheetLabelType[NET_BIDI] ) == 0 )
        m_shape = NET_BIDI;

    if( stricmp( Name2, SheetLabelType[NET_TRISTATE] ) == 0 )
        m_shape = NET_TRISTATE;

    if( stricmp( Name2, SheetLabelType[NET_UNSPECIFIED] ) == 0 )
        m_shape = NET_UNSPECIFIED;

    if( stricmp( Name3, "Italic" ) == 0 )
        m_Italic = 1;

    return true;
}


void SCH_HIERLABEL::SetOrientation( int aOrientation )
{
    m_schematicOrientation = aOrientation;

    switch( m_schematicOrientation )
    {
    default:
    case 0: /* Horiz Normal Orientation */
        m_Orient   = TEXT_ORIENT_HORIZ;
        m_HJustify = GR_TEXT_HJUSTIFY_RIGHT;
        m_VJustify = GR_TEXT_VJUSTIFY_CENTER;
        break;

    case 1: /* Vert Orientation UP */
        m_Orient   = TEXT_ORIENT_VERT;
        m_HJustify = GR_TEXT_HJUSTIFY_LEFT;
        m_VJustify = GR_TEXT_VJUSTIFY_CENTER;
        break;

    case 2: /* Horiz Orientation */
        m_Orient   = TEXT_ORIENT_HORIZ;
        m_HJustify = GR_TEXT_HJUSTIFY_LEFT;
        m_VJustify = GR_TEXT_VJUSTIFY_CENTER;
        break;

    case 3: /*  Vert Orientation BOTTOM */
        m_Orient   = TEXT_ORIENT_VERT;
        m_HJustify = GR_TEXT_HJUSTIFY_RIGHT;
        m_VJustify = GR_TEXT_VJUSTIFY_CENTER;
        break;
    }
}


void SCH_HIERLABEL::Draw( EDA_DRAW_PANEL* panel,
                          wxDC*           DC,
                          const wxPoint&  offset,
                          GR_DRAWMODE     DrawMode,
                          EDA_COLOR_T     Color )
{
    static std::vector <wxPoint> Poly;
    EDA_COLOR_T color;
    int         linewidth = m_Thickness == 0 ?
                            GetDefaultLineThickness() : m_Thickness;
    EDA_RECT* clipbox = panel? panel->GetClipBox() : NULL;

    linewidth = Clamp_Text_PenSize( linewidth, m_Size, m_Bold );

    if( Color >= 0 )
        color = Color;
    else
        color = GetLayerColor( m_Layer );

    GRSetDrawMode( DC, DrawMode );

    std::swap( linewidth, m_Thickness );            // Set the minimum width
    wxPoint text_offset = offset + GetSchematicTextOffset();
    EDA_TEXT::Draw( clipbox, DC, text_offset, color, DrawMode, FILLED, UNSPECIFIED_COLOR );
    std::swap( linewidth, m_Thickness );            // set initial value

    CreateGraphicShape( Poly, m_Pos + offset );
    GRPoly( clipbox, DC, Poly.size(), &Poly[0], 0, linewidth, color, color );

    if( m_isDangling && panel )
        DrawDanglingSymbol( panel, DC, m_Pos + offset, color );

    // Enable these line to draw the bounding box (debug tests purposes only)
#if 0
    {
        EDA_RECT BoundaryBox = GetBoundingBox();
        GRRect( clipbox, DC, BoundaryBox, 0, BROWN );
    }
#endif
}


void SCH_HIERLABEL::CreateGraphicShape( std::vector <wxPoint>& aPoints, const wxPoint& Pos )
{
    int* Template = TemplateShape[m_shape][m_schematicOrientation];
    int  HalfSize = m_Size.x / 2;

    int  imax = *Template; Template++;

    aPoints.clear();

    for( int ii = 0; ii < imax; ii++ )
    {
        wxPoint corner;
        corner.x = ( HalfSize * (*Template) ) + Pos.x;
        Template++;

        corner.y = ( HalfSize * (*Template) ) + Pos.y;
        Template++;

        aPoints.push_back( corner );
    }
}


const EDA_RECT SCH_HIERLABEL::GetBoundingBox() const
{
    int x, y, dx, dy, length, height;

    x  = m_Pos.x;
    y  = m_Pos.y;
    dx = dy = 0;

    int width = (m_Thickness == 0) ? GetDefaultLineThickness() : m_Thickness;
    height = m_Size.y + width + 2 * TXTMARGE;
    length = LenSize( GetShownText() )
             + height                 // add height for triangular shapes
             + 2 * DANGLING_SYMBOL_SIZE;

    switch( m_schematicOrientation )    // respect orientation
    {
    case 0:                             /* Horiz Normal Orientation (left justified) */
        dx = -length;
        dy = height;
        x += DANGLING_SYMBOL_SIZE;
        y -= height / 2;
        break;

    case 1:     /* Vert Orientation UP */
        dx = height;
        dy = -length;
        x -= height / 2;
        y += DANGLING_SYMBOL_SIZE;
        break;

    case 2:     /* Horiz Orientation - Right justified */
        dx = length;
        dy = height;
        x -= DANGLING_SYMBOL_SIZE;
        y -= height / 2;
        break;

    case 3:     /*  Vert Orientation BOTTOM */
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

    int     width = std::max( m_Thickness, GetDefaultLineThickness() );

    int     ii = m_Size.x + TXTMARGE + width;

    switch( m_schematicOrientation )
    {
    case 0:             /* Orientation horiz normale */
        text_offset.x = -ii;
        break;

    case 1:             /* Orientation vert UP */
        text_offset.y = -ii;
        break;

    case 2:             /* Orientation horiz inverse */
        text_offset.x = ii;
        break;

    case 3:             /* Orientation vert BOTTOM */
        text_offset.y = ii;
        break;
    }

    return text_offset;
}


void SCH_HIERLABEL::MirrorY( int aYaxis_position )
{
    /* The hierarchical label is NOT really mirrored for an horizontal label, the schematic
     * orientation is changed.  For a vertical label, the schematic orientation is not changed
     * and the label is moved to a suitable position.
     */

    switch( GetOrientation() )
    {
    case 0:             /* horizontal text */
        SetOrientation( 2 );
        break;

    case 2:             /* invert horizontal text*/
        SetOrientation( 0 );
        break;
    }

    MIRROR( m_Pos.x, aYaxis_position );
}


void SCH_HIERLABEL::MirrorX( int aXaxis_position )
{
    switch( GetOrientation() )
    {
    case 1:             /* vertical text */
        SetOrientation( 3 );
        break;

    case 3:        /* invert vertical text*/
        SetOrientation( 1 );
        break;
    }

    MIRROR( m_Pos.y, aXaxis_position );
}


void SCH_HIERLABEL::Rotate( wxPoint aPosition )
{
    RotatePoint( &m_Pos, aPosition, 900 );
    SetOrientation( (GetOrientation() + 3) % 4 );
}


wxString SCH_HIERLABEL::GetSelectMenuText() const
{
    wxString msg;
    msg.Printf( _( "Hierarchical Label %s" ), GetChars( ShortenedShownText() ) );
    return msg;
}
