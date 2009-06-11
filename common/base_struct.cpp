/****************************************/
/* Basic classes for Kicad:				*/
/*		EDA_BaseStruct                  */
/*		EDA_TextStruct                  */
/****************************************/

/* Fichier base_struct.cpp */

#include "fctsys.h"
#include "gr_basic.h"
#include "trigo.h"
#include "common.h"
#include "macros.h"
#include "wxstruct.h"
#include "class_drawpanel.h"
#include "class_base_screen.h"
#include "drawtxt.h"


enum textbox {
    ID_TEXTBOX_LIST = 8010
};


/******************************************************************************/
EDA_BaseStruct::EDA_BaseStruct( EDA_BaseStruct* parent, KICAD_T idType )
/******************************************************************************/
{
    InitVars();
    m_StructType = idType;
    m_Parent     = parent; /* Chainage hierarchique sur struct racine */
}


/********************************************/
EDA_BaseStruct::EDA_BaseStruct( KICAD_T idType )
/********************************************/
{
    InitVars();
    m_StructType = idType;
}


/********************************************/
void EDA_BaseStruct::InitVars()
/********************************************/
{
    m_StructType = TYPE_NOT_INIT;
    Pnext       = NULL;     // Linked list: Link (next struct)
    Pback       = NULL;     // Linked list: Link (previous struct)
    m_Parent    = NULL;     // Linked list: Link (parent struct)
    m_Son       = NULL;     // Linked list: Link (son struct)
    m_List      = NULL;     // I am not on any list yet
    m_Image     = NULL;     // Link to an image copy for undelete or abort command
    m_Flags     = 0;        // flags for editions and other
    m_TimeStamp = 0;        // Time stamp used for logical links
    m_Status    = 0;
    m_Selected  = 0;        // Used by block commands, and selective editing
}


// see base_struct.h
SEARCH_RESULT EDA_BaseStruct::IterateForward( EDA_BaseStruct* listStart,
                                              INSPECTOR*      inspector,
                                              const void*     testData,
                                              const KICAD_T   scanTypes[] )
{
    EDA_BaseStruct* p = listStart;

    for( ; p; p = p->Pnext )
    {
        if( SEARCH_QUIT == p->Visit( inspector, testData, scanTypes ) )
            return SEARCH_QUIT;
    }

    return SEARCH_CONTINUE;
}


// see base_struct.h
// many classes inherit this method, be careful:
SEARCH_RESULT EDA_BaseStruct::Visit( INSPECTOR* inspector, const void* testData,
                                     const KICAD_T scanTypes[] )
{
    KICAD_T stype;

#if 0 && defined(DEBUG)
    std::cout << GetClass().mb_str() << ' ';
#endif

    for( const KICAD_T* p = scanTypes;  (stype = *p) != EOT;   ++p )
    {
        // If caller wants to inspect my type
        if( stype == Type() )
        {
            if( SEARCH_QUIT == inspector->Inspect( this, testData ) )
                return SEARCH_QUIT;

            break;
        }
    }

    return SEARCH_CONTINUE;
}


#if defined(DEBUG)

// A function that should have been in wxWidgets
std::ostream& operator<<( std::ostream& out, const wxSize& size )
{
    out << " width=\"" << size.GetWidth() << "\" height=\"" << size.GetHeight() << "\"";
    return out;
}


// A function that should have been in wxWidgets
std::ostream& operator<<( std::ostream& out, const wxPoint& pt )
{
    out << " x=\"" << pt.x << "\" y=\"" << pt.y << "\"";
    return out;
}


/**
 * Function Show
 * is used to output the object tree, currently for debugging only.
 * @param nestLevel An aid to prettier tree indenting, and is the level
 *          of nesting of this object within the overall tree.
 * @param os The ostream& to output to.
 */
void EDA_BaseStruct::Show( int nestLevel, std::ostream& os )
{
    // XML output:
    wxString s = GetClass();

    NestedSpace( nestLevel, os ) << '<' << s.Lower().mb_str() << ">"
                                 << " Need ::Show() override for this class "
                                 << "</" << s.Lower().mb_str() << ">\n";
}


/**
 * Function NestedSpace
 * outputs nested space for pretty indenting.
 * @param nestLevel The nest count
 * @param os The ostream&, where to output
 * @return std::ostream& - for continuation.
 **/
std::ostream& EDA_BaseStruct::NestedSpace( int nestLevel, std::ostream& os )
{
    for( int i = 0; i<nestLevel; ++i )
        os << "  ";

    // number of spaces here controls indent per nest level

    return os;
}


#endif


/**************************************************/
/* EDA_TextStruct (basic class, not directly used */
/**************************************************/
EDA_TextStruct::EDA_TextStruct( const wxString& text )
{
    m_Size.x    = m_Size.y = DEFAULT_SIZE_TEXT;     /* XY size of font */
    m_Orient    = 0;                                /* Orient in 0.1 degrees */
    m_Attributs = 0;
    m_Mirror    = false;                            // display mirror if true
    m_HJustify  = GR_TEXT_HJUSTIFY_CENTER;
    m_VJustify  = GR_TEXT_VJUSTIFY_CENTER;          /* Justifications Horiz et Vert du texte */
    m_Width     = 0;                                /* thickness */
    m_Italic    = false;                            /* true = italic shape */
    m_Bold      = false;
    m_MultilineAllowed = false;                     // Set to true only for texts that can use multiline.
    m_Text = text;
}


EDA_TextStruct::~EDA_TextStruct()
{
}


/**
 * Function LenSize
 * @return the text lenght in internal units
 * @param aLine : the line of text to consider.
 * For single line text, this parameter is always m_Text
 */
int EDA_TextStruct::LenSize( const wxString& aLine ) const
{
    return ReturnGraphicTextWidth(aLine, m_Size.x, m_Italic, m_Bold ) + m_Width;
}


/** Function GetTextBox
 * useful in multiline texts to calculate the full text or a line area (for zones filling, locate functions....)
 * @return the rect containing the line of text (i.e. the position and the size of one line)
 * this rectangle is calculated for 0 orient text. if orient is not 0 the rect must be rotated to match the physical area
 * @param aLine : the line of text to consider.
 * for single line text, aLine is unused
 * If aLine == -1, the full area (considering all lines) is returned
 */
EDA_Rect EDA_TextStruct::GetTextBox( int aLine )
{
    EDA_Rect       rect;
    wxPoint        pos;
    wxArrayString* list = NULL;

    wxString*      text = &m_Text;

    if( m_MultilineAllowed )
    {
        list = wxStringSplit( m_Text, '\n' );
        if( aLine >= 0 && (aLine < (int)list->GetCount()) )
            text = &list->Item( aLine );
        else
            text = &list->Item( 0 );
    }


    // calculate the H and V size
    int    dx = LenSize( *text );
    int    dy = m_Size.y + m_Width;
    int extra_dy = (m_Size.y * 3)/10;      // extra dy value for letters like j and y

    /* Creates bounding box (rectangle) for an horizontal text */
    wxSize textsize = wxSize( dx, dy );
    rect.SetOrigin( m_Pos );

    // for multiline texts ans aLine < 0, merge all rectangles
    if( m_MultilineAllowed && aLine < 0 )
    {
        dy = GetInterline();
        for( unsigned ii = 1; ii < list->GetCount(); ii++ )
        {
            text = &list->Item( ii );
            dx   = LenSize( *text );
            textsize.x  = MAX( textsize.x, dx );
            textsize.y += dy;
        }
    }
    delete list;

    textsize.y += extra_dy;
    rect.SetSize( textsize );

    /* Now, calculate the rect origin, according to text justification
     * At this point the rectangle origin is the text origin (m_Pos).
     * This is true only for left and top text justified texts (using top to bottom Y axis orientation).
     * and must be recalculated for others justifications
     * also, note the V justification is relative to the first line
     */
    switch( m_HJustify )
    {
    case GR_TEXT_HJUSTIFY_LEFT:
        break;

    case GR_TEXT_HJUSTIFY_CENTER:
        rect.SetX( rect.GetX() - (rect.GetWidth() / 2) );
        break;

    case GR_TEXT_HJUSTIFY_RIGHT:
        rect.SetX( rect.GetX() - rect.GetWidth() );
        break;
    }

    dy = m_Size.y + m_Width;
    switch( m_VJustify )
    {
    case GR_TEXT_VJUSTIFY_TOP:
        break;

    case GR_TEXT_VJUSTIFY_CENTER:
        rect.SetY( rect.GetY() - (dy / 2) );
        break;

    case GR_TEXT_VJUSTIFY_BOTTOM:
        rect.SetY( rect.GetY() - dy );
        break;
    }

    rect.Normalize();       // Make h and v sizes always >= 0
    return rect;
}


/*************************************************/
bool EDA_TextStruct::TextHitTest( const wxPoint& posref )
/*************************************************/

/**
 * Function TextHitTest (overlayed)
 * tests if the given point is inside this object.
 * @param posref point to test
 * @return bool - true if a hit, else false
 */
{
    EDA_Rect rect = GetTextBox( -1 );   // Get the full text area.

    /* Is the ref point inside the text area ?  */
    wxPoint location = posref;
    RotatePoint( &location, m_Pos, -m_Orient );

    return rect.Inside ( location);
}


/**
 * Function TextHitTest (overlayed)
 * tests if the given EDA_Rect intersect this object.
 * @param refArea the given EDA_Rect to test
 * @return bool - true if a hit, else false
 */
/*********************************************************/
bool EDA_TextStruct::TextHitTest( EDA_Rect& refArea )
/*********************************************************/
{
    if( refArea.Inside( m_Pos ) )
        return true;
    return false;
}

/***************************************************************/
void EDA_TextStruct::Draw( WinEDA_DrawPanel* aPanel, wxDC* aDC,
                           const wxPoint& aOffset, EDA_Colors aColor,
                           int aDrawMode,
                           GRFillMode aFillMode, EDA_Colors aAnchor_color )
/***************************************************************/

/** Function Draw
 * Draws this, that can be a multiline text
 *  @param aPanel = the current DrawPanel
 *  @param aDC = the current Device Context
 *  @param aOffset = draw offset (usually (0,0))
 *  @param EDA_Colors aColor = text color
 *  @param aDrawMode = GR_OR, GR_XOR.., -1 to use the current mode.
 *  @param aFillMode = FILAIRE, FILLED or SKETCH
 *  @param EDA_Colors aAnchor_color = anchor color ( UNSPECIFIED_COLOR = do not draw anchor ).
 */

{
    if( m_MultilineAllowed )
    {
        wxPoint        pos  = m_Pos;
        wxArrayString* list = wxStringSplit( m_Text, '\n' );
        wxPoint        offset;

        offset.y = GetInterline();

        RotatePoint( &offset, m_Orient );
        for( unsigned i = 0; i<list->Count(); i++ )
        {
            wxString txt = list->Item( i );
            DrawOneLineOfText( aPanel,
                               aDC,
                               aOffset,
                               aColor,
                               aDrawMode,
                               aFillMode,
                               aAnchor_color,
                               txt,
                               pos );
            pos += offset;
        }

        delete (list);
    }
    else
        DrawOneLineOfText( aPanel,
                           aDC,
                           aOffset,
                           aColor,
                           aDrawMode,
                           aFillMode,
                           aAnchor_color,
                           m_Text,
                           m_Pos );
}


/** Function DrawOneLineOfText
 * Draw a single text line.
 * Used to draw each line of this EDA_TextStruct, that can be multiline
 *  @param aPanel = the current DrawPanel
 *  @param aDC = the current Device Context
 *  @param aOffset = draw offset (usually (0,0))
 *  @param EDA_Colors aColor = text color
 *  @param aDrawMode = GR_OR, GR_XOR.., -1 to use the current mode.
 *  @param aFillMode = FILAIRE, FILLED or SKETCH
 *  @param EDA_Colors aAnchor_color = anchor color ( UNSPECIFIED_COLOR = do not draw anchor ).
 *  @param EDA_Colors aText = the single line of text to draw.
 *  @param EDA_Colors aPos = the position of this line ).
 */
void EDA_TextStruct::DrawOneLineOfText( WinEDA_DrawPanel* aPanel, wxDC* aDC,
                                        const wxPoint& aOffset, EDA_Colors aColor,
                                        int aDrawMode,
                                        GRFillMode aFillMode, EDA_Colors aAnchor_color,
                                        wxString& aText, wxPoint aPos )
{
    int width = m_Width;

    if( aFillMode == FILAIRE )
        width = 0;

    if( aDrawMode != -1 )
        GRSetDrawMode( aDC, aDrawMode );

    /* Draw text anchor, if allowed */
    if( aAnchor_color != UNSPECIFIED_COLOR )
    {
        int anchor_size = aPanel->GetScreen()->Unscale( 2 );
        aAnchor_color = (EDA_Colors) ( aAnchor_color & MASKCOLOR );

        int cX = aPos.x + aOffset.x;
        int cY = aPos.y + aOffset.y;

        GRLine( &aPanel->m_ClipBox, aDC, cX - anchor_size, cY,
                cX + anchor_size, cY, 0, aAnchor_color );

        GRLine( &aPanel->m_ClipBox, aDC, cX, cY - anchor_size,
                cX, cY + anchor_size, 0, aAnchor_color );
    }

    if( aFillMode == SKETCH )
        width = -width;

    wxSize size = m_Size;

    if( m_Mirror )
        size.x = -size.x;

    DrawGraphicText( aPanel, aDC,
                     aOffset + aPos, aColor, aText,
                     m_Orient, size,
                     m_HJustify, m_VJustify, width, m_Italic, m_Bold );
}


/******************/
/* Class EDA_Rect */
/******************/

/******************************/
void EDA_Rect::Normalize()
/******************************/

// Ensure the height ant width are >= 0
{
    if( m_Size.y < 0 )
    {
        m_Size.y = -m_Size.y;
        m_Pos.y -= m_Size.y;
    }
    if( m_Size.x < 0 )
    {
        m_Size.x = -m_Size.x;
        m_Pos.x -= m_Size.x;
    }
}


/*******************************************/
bool EDA_Rect::Inside( const wxPoint& point )
/*******************************************/

/* Return TRUE if point is in Rect
 *  Accept rect size < 0
 */
{
    int    rel_posx = point.x - m_Pos.x;
    int    rel_posy = point.y - m_Pos.y;
    wxSize size     = m_Size;

    if( size.x < 0 )
    {
        size.x    = -size.x;
        rel_posx += size.x;
    }

    if( size.y < 0 )
    {
        size.y    = -size.y;
        rel_posy += size.y;
    }

    return (rel_posx >= 0) && (rel_posy >= 0)
           && ( rel_posy <= size.y)
           && ( rel_posx <= size.x)
    ;
}


bool EDA_Rect::Intersects( const EDA_Rect aRect ) const
{
    // this logic taken from wxWidgets' geometry.cpp file:
    bool rc;

    int  left   = MAX( m_Pos.x, aRect.m_Pos.x );
    int  right  = MIN( m_Pos.x + m_Size.x, aRect.m_Pos.x + aRect.m_Size.x );
    int  top    = MAX( m_Pos.y, aRect.m_Pos.y );
    int  bottom = MIN( m_Pos.y + m_Size.y, aRect.m_Pos.y + aRect.m_Size.y );

    if( left < right && top < bottom )
        rc = true;
    else
        rc = false;

    return rc;
}


/**************************************************/
EDA_Rect& EDA_Rect::Inflate( wxCoord dx, wxCoord dy )
/**************************************************/

/** Function Inflate
 * Inflate "this": move each horizontal edge by dx and each vertical edge by dy
 * toward rect outside
 * if dx and/or dy is negative, move toward rect inside (deflate)
 * Works for positive and negative rect size
 *
 */
{
    if( m_Size.x >= 0 )
    {
        if( m_Size.x < -2 * dx )
        {
            // Don't allow deflate to eat more width than we have,
            m_Pos.x += m_Size.x / 2;
            m_Size.x = 0;
        }
        else
        {
            // The inflate is valid.
            m_Pos.x  -= dx;
            m_Size.x += 2 * dx;
        }
    }
    else    // size.x < 0:
    {
        if( m_Size.x > -2 * dx )
        {
            // Don't allow deflate to eat more width than we have,
            m_Pos.x -= m_Size.x / 2;
            m_Size.x = 0;
        }
        else
        {
            // The inflate is valid.
            m_Pos.x  += dx;
            m_Size.x -= 2 * dx; // m_Size.x <0: inflate when dx > 0
        }
    }


    if( m_Size.y >= 0 )
    {
        if( m_Size.y < -2 * dy )
        {
            // Don't allow deflate to eat more height than we have,
            m_Pos.y += m_Size.y / 2;
            m_Size.y = 0;
        }
        else
        {
            // The inflate is valid.
            m_Pos.y  -= dy;
            m_Size.y += 2 * dy;
        }
    }
    else    // size.y < 0:
    {
        if( m_Size.y > 2 * dy )
        {
            // Don't allow deflate to eat more height than we have,
            m_Pos.y -= m_Size.y / 2;
            m_Size.y = 0;
        }
        else
        {
            // The inflate is valid.
            m_Pos.y  += dy;
            m_Size.y -= 2 * dy; // m_Size.y <0: inflate when dy > 0
        }
    }

    return *this;
}


/**
 * Function Merge
 * modifies Position and Size of this in order to contain the given rect
 * mainly used to calculate bounding boxes
 * @param aRect = given rect to merge with this
 */
void EDA_Rect::Merge( const EDA_Rect& aRect )
{
    Normalize();        // ensure width and height >= 0
    EDA_Rect rect = aRect;
    rect.Normalize();   // ensure width and height >= 0
    wxPoint  end = GetEnd();
    wxPoint  rect_end = rect.GetEnd();

    // Change origin and size in order to contain the given rect
    m_Pos.x = MIN( m_Pos.x, rect.m_Pos.x );
    m_Pos.y = MIN( m_Pos.y, rect.m_Pos.y );
    end.x   = MAX( end.x, rect_end.x );
    end.y   = MAX( end.y, rect_end.y );
    SetEnd( end );
}
