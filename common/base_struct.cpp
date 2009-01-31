/********************************************************/
/* methodes pour les structures de base:				*/
/*		EDA_BaseStruct (classe non utilisable seule)	*/
/*		EDA_TextStruct (classe non utilisable seule)	*/
/********************************************************/

/* Fichier base_struct.cpp */

#include "fctsys.h"
#include "gr_basic.h"
#include "trigo.h"
#include "common.h"
#include "wxstruct.h"
#include "base_struct.h"
#include "grfonte.h"
#include "macros.h"

enum textbox {
    ID_TEXTBOX_LIST = 8010
};


/******************************************************************************/
EDA_BaseStruct::EDA_BaseStruct( EDA_BaseStruct* parent, KICAD_T idType )
/******************************************************************************/
{
    InitVars();
    m_StructType = idType;
    m_Parent = parent;  /* Chainage hierarchique sur struct racine */
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

#if 0 && defined (DEBUG)
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


#if defined (DEBUG)

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
    m_Width  = 0;                                   /* thickness */
    m_Italic = false;                               /* true = italic shape */
    m_Text   = text;
}


EDA_TextStruct::~EDA_TextStruct()
{
}


/********************************/
int EDA_TextStruct::Len_Size()
/********************************/

// Return the text lenght in internal units
{
    int nbchar = m_Text.Len();
    int len;

    if( nbchar == 0 )
        return 0;

    len = (( (10 * m_Size.x ) / 9 ) + m_Width) * nbchar;
    return len;
}


/*************************************************/
bool EDA_TextStruct::HitTest( const wxPoint& posref )
/*************************************************/

/* locate function
 *  return:
 *      true  if posref is inside the text area.
 *      false else.
 */
{
    int dx, dy;
    wxPoint location;

    dx = (int) (( Pitch() * GetLength() ) / 2);
    dy = m_Size.y / 2;

    /* Is the ref point inside the text area ?  */
    location = posref - m_Pos;

    RotatePoint( &location, -m_Orient );

    if( ( abs( location.x ) <= abs( dx ) ) && ( abs( location.y ) <= abs( dy ) ) )
        return true;

    return false;
}


/**
 * Function HitTest (overlayed)
 * tests if the given EDA_Rect intersect this object.
 * @param refArea the given EDA_Rect to test
 * @return bool - true if a hit, else false
 */
/*********************************************************/
bool EDA_TextStruct::HitTest( EDA_Rect& refArea )
/*********************************************************/
{
    if( refArea.Inside( m_Pos ) )
        return true;
    return false;
}


/*******************************/
int EDA_TextStruct::Pitch(int aMinTickness)
/*******************************/
/**
 * Function Pitch
 * @return distance between 2 characters
 * @param aMinTickness = min segments tickness
 */
{
    return ((m_Size.x * 10)/9) + MAX( m_Width, aMinTickness);
}


/***************************************************************/
void EDA_TextStruct::Draw( WinEDA_DrawPanel* aPanel, wxDC* aDC,
                           const wxPoint& aOffset, EDA_Colors aColor,
                           int aDrawMode,
                           GRFillMode aDisplayMode, EDA_Colors aAnchor_color )
/***************************************************************/

/** Function Draw
 *  @param aPanel = the current DrawPanel
 *  @param aDC = the current Device Context
 *  @param aOffset = draw offset (usually (0,0))
 *  @param EDA_Colors aColor = text color
 *  @param aDrawMode = GR_OR, GR_XOR.., -1 to use the current mode.
 *  @param aDisplayMode = FILAIRE, FILLED or SKETCH
 *  @param EDA_Colors aAnchor_color = anchor color ( UNSPECIFIED_COLOR = do not draw anchor ).
 */
{
    int width;

    width = m_Width;
    if( aDisplayMode == FILAIRE )
        width = 0;

    if( aDrawMode != -1 )
        GRSetDrawMode( aDC, aDrawMode );

    /* Draw text anchor, if allowed */
    if( aAnchor_color != UNSPECIFIED_COLOR )
    {
        int anchor_size = aPanel->GetScreen()->Unscale( 2 );
        aAnchor_color = (EDA_Colors) (aAnchor_color & MASKCOLOR);

        int cX = m_Pos.x + aOffset.x;
        int cY = m_Pos.y + aOffset.y;

        GRLine( &aPanel->m_ClipBox, aDC, cX - anchor_size, cY,
                cX + anchor_size, cY, 0, aAnchor_color );

        GRLine( &aPanel->m_ClipBox, aDC, cX, cY - anchor_size,
                cX, cY + anchor_size, 0, aAnchor_color );
    }

    if( aDisplayMode == SKETCH )
        width = -width;
	wxSize size = m_Size;
	if ( m_Mirror )
		size.x = -size.x;

    DrawGraphicText( aPanel, aDC,
                     aOffset + m_Pos, aColor, m_Text,
                     m_Orient, size,
                     m_HJustify, m_VJustify, width, m_Italic );
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
    wxSize size = m_Size;

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

    int left   = MAX( m_Pos.x, aRect.m_Pos.x );
    int right  = MIN( m_Pos.x + m_Size.x, aRect.m_Pos.x + aRect.m_Size.x );
    int top    = MAX( m_Pos.y, aRect.m_Pos.y );
    int bottom = MIN( m_Pos.y + m_Size.y, aRect.m_Pos.y + aRect.m_Size.y );

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
    wxPoint  end      = GetEnd();
    wxPoint  rect_end = rect.GetEnd();

    // Change origin and size in order to contain the given rect
    m_Pos.x = MIN( m_Pos.x, rect.m_Pos.x );
    m_Pos.y = MIN( m_Pos.y, rect.m_Pos.y );
    end.x   = MAX( end.x, rect_end.x );
    end.y   = MAX( end.y, rect_end.y );
    SetEnd( end );
}
