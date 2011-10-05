/*********************************************/
/* Code for handling schematic sheet labels. */
/*********************************************/

#include "fctsys.h"
#include "gr_basic.h"
#include "macros.h"
#include "trigo.h"
#include "eeschema_id.h"
#include "class_drawpanel.h"
#include "drawtxt.h"
#include "wxEeschemaStruct.h"
#include "plot_common.h"

#include "general.h"
#include "protos.h"
#include "sch_text.h"

extern void IncrementLabelMember( wxString& name );


/************************/
/* class SCH_TEXT */
/* class SCH_LABEL */
/* class SCH_GLOBALLABEL */
/* class SCH_HIERLABEL */
/************************/

/* Names of sheet label types. */
const char* SheetLabelType[] =
{
    "Input",
    "Output",
    "BiDi",
    "3State",
    "UnSpc",
    "?????"
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
    m_Shape = 0;
    m_IsDangling = false;
    m_MultilineAllowed = true;
    m_SchematicOrientation = 0;
}


SCH_TEXT::SCH_TEXT( const SCH_TEXT& aText ) :
    SCH_ITEM( aText ),
    EDA_TEXT( aText )
{
    m_Pos = aText.m_Pos;
    m_Shape = aText.m_Shape;
    m_MultilineAllowed = aText.m_MultilineAllowed;
    m_SchematicOrientation = aText.m_SchematicOrientation;
    m_IsDangling = false;
}


EDA_ITEM* SCH_TEXT::doClone() const
{
    return new SCH_TEXT( *this );
}


void SCH_TEXT::IncrementLabel()
{
    IncrementLabelMember( m_Text );
}


wxPoint SCH_TEXT::GetSchematicTextOffset() const
{
    wxPoint text_offset;

    // add a small offset (TXTMARGE) to x ( or y) position to allow a text to
    // be on a wire or a line and be readable
    switch( m_SchematicOrientation )
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
    if( SCH_ITEM::Matches( m_Text, aSearchData ) )
    {
        EDA_RECT BoundaryBox = GetBoundingBox();
        if( aFindLocation )
            *aFindLocation = BoundaryBox.Centre();
        return true;
    }

    return false;
}


void SCH_TEXT::Mirror_Y( int aYaxis_position )
{
    // Text is NOT really mirrored; it is moved to a suitable position
    // which is the closest position for a true mirrored text
    // The center position is mirrored and the text is moved for half
    // horizontal len
    int px = m_Pos.x;
    int dx;

    switch( GetOrientation() )
    {
    case 0:             /* horizontal text */
        dx = LenSize( m_Text ) / 2;
        break;

    case 1: /* Vert Orientation UP */
        dx = -m_Size.x / 2;
        break;

    case 2:        /* invert horizontal text*/
        dx = -LenSize( m_Text ) / 2;
        break;

    case 3: /*  Vert Orientation BOTTOM */
        dx = m_Size.x / 2;
        break;

    default:
        dx = 0;
        break;
    }

    px += dx;
    px -= aYaxis_position;
    NEGATE( px );
    px += aYaxis_position;
    px -= dx;
    m_Pos.x = px;
}


void SCH_TEXT::Mirror_X( int aXaxis_position )
{
    // Text is NOT really mirrored; it is moved to a suitable position
    // which is the closest position for a true mirrored text
    // The center position is mirrored and the text is moved for half
    // horizontal len
    int py = m_Pos.y;
    int dy;

    switch( GetOrientation() )
    {
    case 0:             /* horizontal text */
        dy = -m_Size.y / 2;
        break;

    case 1: /* Vert Orientation UP */
        dy = -LenSize( m_Text ) / 2;
        break;

    case 2:                 /* invert horizontal text*/
        dy = m_Size.y / 2;  // how to calculate text height?
        break;

    case 3: /*  Vert Orientation BOTTOM */
        dy = LenSize( m_Text ) / 2;
        break;

    default:
        dy = 0;
        break;
    }

    py += dy;
    py -= aXaxis_position;
    NEGATE( py );
    py += aXaxis_position;
    py -= dy;
    m_Pos.y = py;
}


void SCH_TEXT::Rotate( wxPoint rotationPoint )
{
    int dy;

    RotatePoint( &m_Pos, rotationPoint, 900 );
    SetOrientation( (GetOrientation() + 1) % 4 );

    switch( GetOrientation() )
    {
    case 0:             /* horizontal text */
        dy = m_Size.y;
        break;

    case 1: /* Vert Orientation UP */
        dy = 0;
        break;

    case 2:        /* invert horizontal text*/
        dy = m_Size.y;
        break;

    case 3: /*  Vert Orientation BOTTOM */
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
    m_SchematicOrientation = aOrientation;

    switch( m_SchematicOrientation )
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


void SCH_TEXT::SwapData( SCH_TEXT* copyitem )
{
    EXCHG( m_Text, copyitem->m_Text );
    EXCHG( m_Pos, copyitem->m_Pos );
    EXCHG( m_Size, copyitem->m_Size );
    EXCHG( m_Thickness, copyitem->m_Thickness );
    EXCHG( m_Shape, copyitem->m_Shape );
    EXCHG( m_Orient, copyitem->m_Orient );

    EXCHG( m_Layer, copyitem->m_Layer );
    EXCHG( m_HJustify, copyitem->m_HJustify );
    EXCHG( m_VJustify, copyitem->m_VJustify );
    EXCHG( m_IsDangling, copyitem->m_IsDangling );
    EXCHG( m_SchematicOrientation, copyitem->m_SchematicOrientation );
}


void SCH_TEXT::Place( SCH_EDIT_FRAME* frame, wxDC* DC )
{
    if( !IsNew() )
    {
        // For existing (not new texts: save in undo list the old text:
        ClearFlags();
        PICKED_ITEMS_LIST pickList;
        ITEM_PICKER picker( this, UR_CHANGED); //UR_EXCHANGE_T );
        SCH_ITEM* undoItem = frame->GetUndoItem();

        wxCHECK_RET( undoItem != NULL && undoItem->Type() == Type(),
                    wxT( "Invalid text undo item." ) );

        undoItem->ClearFlags();
        picker.SetLink( undoItem );
        // the owner of undoItem is no more frame, this is picker:
        frame->SetUndoItem( NULL );

        pickList.PushItem( picker );
        frame->SaveCopyInUndoList( pickList, UR_CHANGED); //UR_EXCHANGE_T );
    }

    SCH_ITEM::Place( frame, DC );
}


int SCH_TEXT::GetPenSize() const
{
    int pensize = m_Thickness;

    if( pensize == 0 )   // Use default values for pen size
    {
        if( m_Bold  )
            pensize = GetPenSizeForBold( m_Size.x );
        else
            pensize = g_DrawDefaultLineThickness;
    }

    // Clip pen size for small texts:
    pensize = Clamp_Text_PenSize( pensize, m_Size, m_Bold );
    return pensize;
}


void SCH_TEXT::Draw( EDA_DRAW_PANEL* panel, wxDC* DC, const wxPoint& aOffset,
                     int DrawMode, int Color )
{
    EDA_Colors color;
    int        linewidth = ( m_Thickness == 0 ) ? g_DrawDefaultLineThickness : m_Thickness;

    linewidth = Clamp_Text_PenSize( linewidth, m_Size, m_Bold );

    if( Color >= 0 )
        color = (EDA_Colors) Color;
    else
        color = ReturnLayerColor( m_Layer );

    GRSetDrawMode( DC, DrawMode );

    wxPoint text_offset = aOffset + GetSchematicTextOffset();
    EXCHG( linewidth, m_Thickness );            // Set the minimum width
    EDA_TEXT::Draw( panel, DC, text_offset, color, DrawMode, FILLED, UNSPECIFIED_COLOR );
    EXCHG( linewidth, m_Thickness );            // set initial value
    if( m_IsDangling )
        DrawDanglingSymbol( panel, DC, m_Pos + aOffset, color );

    // Enable these line to draw the bounding box (debug tests purposes only)
#if 0
    {
        EDA_RECT BoundaryBox = GetBoundingBox();
        GRRect( &panel->m_ClipBox, DC, BoundaryBox, 0, BROWN );
    }
#endif
}


/**
 * Function Save
 * writes the data structures for this object out to a FILE in "*.brd" format.
 * @param aFile The FILE to write to.
 * @return bool - true if success writing else false.
 */
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
                 m_Pos.x, m_Pos.y, m_SchematicOrientation, m_Size.x,
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
    int ii = sscanf( sline, "%s %d %d %d %d %s %s %d", Name1, &m_Pos.x, &m_Pos.y,
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
        size = DEFAULT_SIZE_TEXT;

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

    DANGLING_END_ITEM item( LABEL_END, this );
    item.m_Pos = m_Pos;
    aItemList.push_back( item );
}


bool SCH_TEXT::IsDanglingStateChanged( std::vector< DANGLING_END_ITEM >& aItemList )
{
    // Normal text labels cannot be tested for dangling ends.
    if( Type() == SCH_TEXT_T )
        return false;

    bool previousState = m_IsDangling;
    m_IsDangling = true;

    for( unsigned ii = 0; ii < aItemList.size(); ii++ )
    {
        DANGLING_END_ITEM& item = aItemList[ii];

        if( item.m_Item == this )
            continue;

        switch( item.m_Type )
        {
        case PIN_END:
        case LABEL_END:
        case SHEET_LABEL_END:
            if( m_Pos == item.m_Pos )
                m_IsDangling = false;
            break;

        case WIRE_START_END:
        case BUS_START_END:
        {
            // These schematic items have created 2 DANGLING_END_ITEM one per end.  But being
            // a paranoid programmer, I'll check just in case.
            ii++;

            wxCHECK_MSG( ii < aItemList.size(), previousState != m_IsDangling,
                         wxT( "Dangling end type list overflow.  Bad programmer!" ) );

            DANGLING_END_ITEM & nextItem = aItemList[ii];
            m_IsDangling = !SegmentIntersect( item.m_Pos, nextItem.m_Pos, m_Pos );
        }
            break;

        default:
            break;
        }

        if( m_IsDangling == false )
            break;
    }

    return previousState != m_IsDangling;
}


bool SCH_TEXT::IsSelectStateChanged( const wxRect& aRect )
{
    bool previousState = IsSelected();

    if( aRect.Contains( m_Pos ) )
        m_Flags |= SELECTED;
    else
        m_Flags &= ~SELECTED;

    return previousState != IsSelected();
}


void SCH_TEXT::GetConnectionPoints( vector< wxPoint >& aPoints ) const
{
    // Normal text labels do not have connection points.  All others do.
    if( Type() == SCH_TEXT_T )
        return;

    aPoints.push_back( m_Pos );
}


EDA_RECT SCH_TEXT::GetBoundingBox() const
{
    // We must pass the effective text thickness to GetTextBox
    // when calculating the bounding box
    int linewidth = ( m_Thickness == 0 ) ? g_DrawDefaultLineThickness : m_Thickness;

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
    wxString tmp = GetText();
    tmp.Replace( wxT( "\n" ), wxT( " " ) );
    tmp.Replace( wxT( "\r" ), wxT( " " ) );
    tmp.Replace( wxT( "\t" ), wxT( " " ) );
    tmp =( tmp.Length() > 15 ) ? tmp.Left( 12 ) + wxT( "..." ) : tmp;

    wxString msg;
    msg.Printf( _( "Graphic Text %s" ), GetChars( tmp ) );
    return msg;
}


bool SCH_TEXT::doHitTest( const wxPoint& aPoint, int aAccuracy ) const
{
    return TextHitTest( aPoint, aAccuracy );
}


bool SCH_TEXT::doHitTest( const EDA_RECT& aRect, bool aContained, int aAccuracy ) const
{
    return TextHitTest( aRect, aContained, aAccuracy );
}


void SCH_TEXT::doPlot( PLOTTER* aPlotter )
{
    static std::vector <wxPoint> Poly;

    EDA_Colors color = ReturnLayerColor( GetLayer() );
    wxPoint    textpos   = m_Pos + GetSchematicTextOffset();
    int        thickness = GetPenSize();

    aPlotter->set_current_line_width( thickness );

    if( m_MultilineAllowed )
    {
        wxPoint        pos  = textpos;
        wxArrayString* list = wxStringSplit( m_Text, '\n' );
        wxPoint        offset;

        offset.y = GetInterline();

        RotatePoint( &offset, m_Orient );

        for( unsigned i = 0; i<list->Count(); i++ )
        {
            wxString txt = list->Item( i );
            aPlotter->text( pos, color, txt, m_Orient, m_Size, m_HJustify,
                            m_VJustify, thickness, m_Italic, m_Bold );
            pos += offset;
        }

        delete (list);
    }
    else
    {
        aPlotter->text( textpos, color, m_Text, m_Orient, m_Size, m_HJustify,
                        m_VJustify, thickness, m_Italic, m_Bold );
    }

    /* Draw graphic symbol for global or hierarchical labels */
    CreateGraphicShape( Poly, m_Pos );

    aPlotter->set_current_line_width( GetPenSize() );

    if( Poly.size() )
        aPlotter->PlotPoly( Poly, NO_FILL );
}


#if defined(DEBUG)

void SCH_TEXT::Show( int nestLevel, std::ostream& os )
{
    // XML output:
    wxString s = GetClass();

    NestedSpace( nestLevel, os ) << '<' << s.Lower().mb_str()
                                 << " layer=\"" << m_Layer << '"'
                                 << " shape=\"" << m_Shape << '"'
                                 << " dangling=\"" << m_IsDangling << '"'
                                 << '>'
                                 << TO_UTF8( m_Text )
                                 << "</" << s.Lower().mb_str() << ">\n";
}

#endif


SCH_LABEL::SCH_LABEL( const wxPoint& pos, const wxString& text ) :
    SCH_TEXT( pos, text, SCH_LABEL_T )
{
    m_Layer = LAYER_LOCLABEL;
    m_Shape = NET_INPUT;
    m_IsDangling = TRUE;
    m_MultilineAllowed = false;
}


SCH_LABEL::SCH_LABEL( const SCH_LABEL& aLabel ) :
    SCH_TEXT( aLabel )
{
}


EDA_ITEM* SCH_LABEL::doClone() const
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


void SCH_LABEL::Mirror_X( int aXaxis_position )
{
    // Text is NOT really mirrored; it is moved to a suitable position
    // which is the closest position for a true mirrored text
    // The center position is mirrored and the text is moved for half
    // horizontal len
    int py = m_Pos.y;

    py -= aXaxis_position;
    NEGATE( py );
    py += aXaxis_position;
    m_Pos.y = py;
}


void SCH_LABEL::Rotate( wxPoint rotationPoint )
{
    RotatePoint( &m_Pos, rotationPoint, 900 );
    SetOrientation( (GetOrientation() + 1) % 4 );
}


bool SCH_LABEL::Save( FILE* aFile ) const
{
    bool        success = true;
    const char* shape   = "~";

    if( m_Italic )
        shape = "Italic";

    if( fprintf( aFile, "Text Label %-4d %-4d %-4d %-4d %s %d\n%s\n",
                 m_Pos.x, m_Pos.y, m_SchematicOrientation, m_Size.x, shape,
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
    int ii = sscanf( sline, "%s %d %d %d %d %s %s %d", Name1, &m_Pos.x, &m_Pos.y,
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
        size = DEFAULT_SIZE_TEXT;

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
                      int DrawMode, int Color )
{
    SCH_TEXT::Draw( panel, DC, offset, DrawMode, Color );
}


EDA_RECT SCH_LABEL::GetBoundingBox() const
{
    int x, y, dx, dy, length, height;

    x = m_Pos.x;
    y = m_Pos.y;
    int width = (m_Thickness == 0) ? g_DrawDefaultLineThickness : m_Thickness;
    length = LenSize( m_Text );
    height = m_Size.y + width;
    dx     = dy = 0;

    switch( m_SchematicOrientation )
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
    wxString tmp = ( GetText().Length() > 15 ) ? GetText().Left( 12 ) + wxT( "..." ) : GetText();

    wxString msg;
    msg.Printf( _( "Label %s" ), GetChars(tmp) );
    return msg;
}


bool SCH_LABEL::doHitTest( const wxPoint& aPoint, int aAccuracy ) const
{
    return TextHitTest( aPoint, aAccuracy );
}


SCH_GLOBALLABEL::SCH_GLOBALLABEL( const wxPoint& pos, const wxString& text ) :
    SCH_TEXT( pos, text, SCH_GLOBAL_LABEL_T )
{
    m_Layer = LAYER_GLOBLABEL;
    m_Shape = NET_BIDI;
    m_IsDangling = TRUE;
    m_MultilineAllowed = false;
}


SCH_GLOBALLABEL::SCH_GLOBALLABEL( const SCH_GLOBALLABEL& aGlobalLabel ) :
    SCH_TEXT( aGlobalLabel )
{
}


EDA_ITEM* SCH_GLOBALLABEL::doClone() const
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
                 m_Pos.x, m_Pos.y, m_SchematicOrientation, m_Size.x,
                 SheetLabelType[m_Shape], shape, m_Thickness, TO_UTF8( m_Text ) ) == EOF )
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
    int ii = sscanf( sline, "%s %d %d %d %d %s %s %d", Name1, &m_Pos.x, &m_Pos.y,
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
        size = DEFAULT_SIZE_TEXT;

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
    m_Shape  = NET_INPUT;
    m_Bold = ( thickness != 0 );
    m_Thickness = m_Bold ? GetPenSizeForBold( size ) : 0;

    if( stricmp( Name2, SheetLabelType[NET_OUTPUT] ) == 0 )
        m_Shape = NET_OUTPUT;

    if( stricmp( Name2, SheetLabelType[NET_BIDI] ) == 0 )
        m_Shape = NET_BIDI;

    if( stricmp( Name2, SheetLabelType[NET_TRISTATE] ) == 0 )
        m_Shape = NET_TRISTATE;

    if( stricmp( Name2, SheetLabelType[NET_UNSPECIFIED] ) == 0 )
        m_Shape = NET_UNSPECIFIED;

    if( stricmp( Name3, "Italic" ) == 0 )
        m_Italic = 1;

    return true;
}


void SCH_GLOBALLABEL::Mirror_Y( int aYaxis_position )
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

    m_Pos.x -= aYaxis_position;
    NEGATE( m_Pos.x );
    m_Pos.x += aYaxis_position;
}


void SCH_GLOBALLABEL::Mirror_X( int aXaxis_position )
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

    m_Pos.y -= aXaxis_position;
    NEGATE( m_Pos.y );
    m_Pos.y += aXaxis_position;
}


void SCH_GLOBALLABEL::Rotate( wxPoint rotationPoint )
{
    RotatePoint( &m_Pos, rotationPoint, 900 );
    SetOrientation( (GetOrientation() + 3) % 4 );
}


wxPoint SCH_GLOBALLABEL::GetSchematicTextOffset() const
{
    wxPoint text_offset;
    int     width = (m_Thickness == 0) ? g_DrawDefaultLineThickness : m_Thickness;

    width = Clamp_Text_PenSize( width, m_Size, m_Bold );
    int     HalfSize = m_Size.x / 2;
    int     offset   = width;

    switch( m_Shape )
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

    switch( m_SchematicOrientation )
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
    m_SchematicOrientation = aOrientation;

    switch( m_SchematicOrientation )
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
                            int             DrawMode,
                            int             Color )
{
    static std::vector <wxPoint> Poly;
    EDA_Colors color;
    wxPoint    text_offset = aOffset + GetSchematicTextOffset();

    if( Color >= 0 )
        color = (EDA_Colors) Color;
    else
        color = ReturnLayerColor( m_Layer );

    GRSetDrawMode( DC, DrawMode );

    int linewidth = (m_Thickness == 0) ? g_DrawDefaultLineThickness : m_Thickness;
    linewidth = Clamp_Text_PenSize( linewidth, m_Size, m_Bold );
    EXCHG( linewidth, m_Thickness );            // Set the minimum width
    EDA_TEXT::Draw( panel, DC, text_offset, color, DrawMode, FILLED, UNSPECIFIED_COLOR );
    EXCHG( linewidth, m_Thickness );            // set initial value

    CreateGraphicShape( Poly, m_Pos + aOffset );
    GRPoly( &panel->m_ClipBox, DC, Poly.size(), &Poly[0], 0, linewidth, color, color );

    if( m_IsDangling )
        DrawDanglingSymbol( panel, DC, m_Pos + aOffset, color );

    // Enable these line to draw the bounding box (debug tests purposes only)
#if 0
    {
        EDA_RECT BoundaryBox = GetBoundingBox();
        GRRect( &panel->m_ClipBox, DC, BoundaryBox, 0, BROWN );
    }
#endif
}


void SCH_GLOBALLABEL::CreateGraphicShape( std::vector <wxPoint>& aPoints, const wxPoint& Pos )
{
    int HalfSize  = m_Size.y / 2;
    int linewidth = (m_Thickness == 0) ? g_DrawDefaultLineThickness : m_Thickness;

    linewidth = Clamp_Text_PenSize( linewidth, m_Size, m_Bold );

    aPoints.clear();

    int symb_len = LenSize( m_Text ) + ( TXTMARGE * 2 );

    // Create outline shape : 6 points
    int x = symb_len + linewidth + 3;

    // 50% more for negation bar
    int y = wxRound( (double) HalfSize * 1.5 + (double) linewidth + 3.0 );

    // Starting point(anchor)
    aPoints.push_back( wxPoint( 0, 0 ) );
    aPoints.push_back( wxPoint( 0, -y ) );     // Up
    aPoints.push_back( wxPoint( -x, -y ) );    // left
    aPoints.push_back( wxPoint( -x, 0 ) );     // Up left
    aPoints.push_back( wxPoint( -x, y ) );     // left down
    aPoints.push_back( wxPoint( 0, y ) );      // down

    int x_offset = 0;

    switch( m_Shape )
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

    switch( m_SchematicOrientation )
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


EDA_RECT SCH_GLOBALLABEL::GetBoundingBox() const
{
    int x, y, dx, dy, length, height;

    x  = m_Pos.x;
    y  = m_Pos.y;
    dx = dy = 0;

    int width = (m_Thickness == 0) ? g_DrawDefaultLineThickness : m_Thickness;
    height = ( (m_Size.y * 15) / 10 ) + width + 2 * TXTMARGE;

    // text X size add height for triangular shapes(bidirectional)
    length = LenSize( m_Text ) + height + DANGLING_SYMBOL_SIZE;

    switch( m_SchematicOrientation )    // respect orientation
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
    wxString tmp = ( GetText().Length() > 15 ) ? GetText().Left( 12 ) + wxT( "..." ) : GetText();

    wxString msg;
    msg.Printf( _( "Global Label %s" ), GetChars(tmp) );
    return msg;
}


bool SCH_GLOBALLABEL::doHitTest( const wxPoint& aPoint, int aAccuracy ) const
{
    return TextHitTest( aPoint, aAccuracy );
}


SCH_HIERLABEL::SCH_HIERLABEL( const wxPoint& pos, const wxString& text, KICAD_T aType ) :
    SCH_TEXT( pos, text, aType )
{
    m_Layer = LAYER_HIERLABEL;
    m_Shape = NET_INPUT;
    m_IsDangling = TRUE;
    m_MultilineAllowed = false;
}


SCH_HIERLABEL::SCH_HIERLABEL( const SCH_HIERLABEL& aHierLabel ) :
    SCH_TEXT( aHierLabel )
{
}


EDA_ITEM* SCH_HIERLABEL::doClone() const
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
                 m_Pos.x, m_Pos.y, m_SchematicOrientation, m_Size.x,
                 SheetLabelType[m_Shape], shape, m_Thickness, TO_UTF8( m_Text ) ) == EOF )
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
    int ii = sscanf( sline, "%s %d %d %d %d %s %s %d", Name1, &m_Pos.x, &m_Pos.y,
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
        size = DEFAULT_SIZE_TEXT;

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
    m_Shape  = NET_INPUT;
    m_Bold = ( thickness != 0 );
    m_Thickness = m_Bold ? GetPenSizeForBold( size ) : 0;

    if( stricmp( Name2, SheetLabelType[NET_OUTPUT] ) == 0 )
        m_Shape = NET_OUTPUT;

    if( stricmp( Name2, SheetLabelType[NET_BIDI] ) == 0 )
        m_Shape = NET_BIDI;

    if( stricmp( Name2, SheetLabelType[NET_TRISTATE] ) == 0 )
        m_Shape = NET_TRISTATE;

    if( stricmp( Name2, SheetLabelType[NET_UNSPECIFIED] ) == 0 )
        m_Shape = NET_UNSPECIFIED;

    if( stricmp( Name3, "Italic" ) == 0 )
        m_Italic = 1;

    return true;
}


void SCH_HIERLABEL::SetOrientation( int aOrientation )
{
    m_SchematicOrientation = aOrientation;

    switch( m_SchematicOrientation )
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
                          int             DrawMode,
                          int             Color )
{
    static std::vector <wxPoint> Poly;
    EDA_Colors color;
    int        linewidth = ( m_Thickness == 0 ) ? g_DrawDefaultLineThickness : m_Thickness;

    linewidth = Clamp_Text_PenSize( linewidth, m_Size, m_Bold );

    if( Color >= 0 )
        color = (EDA_Colors) Color;
    else
        color = ReturnLayerColor( m_Layer );

    GRSetDrawMode( DC, DrawMode );

    EXCHG( linewidth, m_Thickness );            // Set the minimum width
    wxPoint text_offset = offset + GetSchematicTextOffset();
    EDA_TEXT::Draw( panel, DC, text_offset, color, DrawMode, FILLED, UNSPECIFIED_COLOR );
    EXCHG( linewidth, m_Thickness );            // set initial value

    CreateGraphicShape( Poly, m_Pos + offset );
    GRPoly( &panel->m_ClipBox, DC, Poly.size(), &Poly[0], 0, linewidth, color, color );

    if( m_IsDangling )
        DrawDanglingSymbol( panel, DC, m_Pos + offset, color );

    // Enable these line to draw the bounding box (debug tests purposes only)
#if 0
    {
        EDA_RECT BoundaryBox = GetBoundingBox();
        GRRect( &panel->m_ClipBox, DC, BoundaryBox, 0, BROWN );
    }
#endif
}


void SCH_HIERLABEL::CreateGraphicShape( std::vector <wxPoint>& aPoints, const wxPoint& Pos )
{
    int* Template = TemplateShape[m_Shape][m_SchematicOrientation];
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


EDA_RECT SCH_HIERLABEL::GetBoundingBox() const
{
    int x, y, dx, dy, length, height;

    x  = m_Pos.x;
    y  = m_Pos.y;
    dx = dy = 0;

    int width = (m_Thickness == 0) ? g_DrawDefaultLineThickness : m_Thickness;
    height = m_Size.y + width + 2 * TXTMARGE;
    length = LenSize( m_Text )
             + height                 // add height for triangular shapes
             + 2 * DANGLING_SYMBOL_SIZE;

    switch( m_SchematicOrientation )    // respect orientation
    {
    case 0:                             /* Horiz Normal Orientation (left
                                         *justified) */
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

    int     width = MAX( m_Thickness, g_DrawDefaultLineThickness );

    int     ii = m_Size.x + TXTMARGE + width;

    switch( m_SchematicOrientation )
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


void SCH_HIERLABEL::Mirror_Y( int aYaxis_position )
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

    m_Pos.x -= aYaxis_position;
    NEGATE( m_Pos.x );
    m_Pos.x += aYaxis_position;
}


void SCH_HIERLABEL::Mirror_X( int aXaxis_position )
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

    m_Pos.y -= aXaxis_position;
    NEGATE( m_Pos.y );
    m_Pos.y += aXaxis_position;
}


void SCH_HIERLABEL::Rotate( wxPoint rotationPoint )
{
    RotatePoint( &m_Pos, rotationPoint, 900 );
    SetOrientation( (GetOrientation() + 3) % 4 );
}


wxString SCH_HIERLABEL::GetSelectMenuText() const
{
    wxString tmp = ( GetText().Length() > 15 ) ? GetText().Left( 12 ) + wxT( "..." ) : GetText();

    wxString msg;
    msg.Printf( _( "Hierarchical Label %s" ), GetChars( tmp ) );
    return msg;
}


bool SCH_HIERLABEL::doHitTest( const wxPoint& aPoint, int aAccuracy ) const
{
    return TextHitTest( aPoint, aAccuracy );
}
