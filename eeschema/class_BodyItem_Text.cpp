/***************************/
/* class_BodyItem_Text.cpp */
/***************************/

/**
* class LibDrawText : describes a graphic text used to draw component shapes
* This is only a graphic item
*/

#include "fctsys.h"
#include "gr_basic.h"
#include "common.h"
#include "class_drawpanel.h"
#include "drawtxt.h"
#include "trigo.h"

#include "program.h"
#include "libcmp.h"
#include "general.h"
#include "protos.h"




LibDrawText::LibDrawText(EDA_LibComponentStruct * aParent) :
    LibEDA_BaseStruct( COMPONENT_GRAPHIC_TEXT_DRAW_TYPE, aParent ),
    EDA_TextStruct()
{
    m_Size     = wxSize( 50, 50 );
    m_typeName = _( "Text" );
}


bool LibDrawText::Save( FILE* ExportFile ) const
{
    wxString text = m_Text;

    // Spaces are not allowed in text because it is not double quoted:
    // changed to '~'
    text.Replace( wxT( " " ), wxT( "~" ) );

    if( fprintf( ExportFile, "T %d %d %d %d %d %d %d %s ", m_Orient,
                 m_Pos.x, m_Pos.y, m_Size.x, m_Attributs, m_Unit, m_Convert,
                 CONV_TO_UTF8( text ) ) < 0 )
        return false;
    if( fprintf( ExportFile, " %s %d", m_Italic ? "Italic" : "Normal",
                 ( m_Bold > 0 ) ? 1 : 0 ) < 0 )
        return false;

    char hjustify = 'C';
    if( m_HJustify == GR_TEXT_HJUSTIFY_LEFT )
        hjustify = 'L';
    else if( m_HJustify == GR_TEXT_HJUSTIFY_RIGHT )
        hjustify = 'R';

    char vjustify = 'C';
    if( m_VJustify == GR_TEXT_VJUSTIFY_BOTTOM )
        vjustify = 'B';
    else if( m_VJustify == GR_TEXT_VJUSTIFY_TOP )
        vjustify = 'T';

    if( fprintf( ExportFile, " %c %c\n", hjustify, vjustify ) < 0 )
        return false;

    return true;
}


bool LibDrawText::Load( char* line, wxString& errorMsg )
{
    int  cnt, thickness;
    char hjustify = 'C', vjustify = 'C';
    char buf[256];
    char tmp[256];

    buf[0] = 0;
    tmp[0] = 0;         // For italic option, Not in old versions

    cnt = sscanf( &line[2], "%d %d %d %d %d %d %d %s %s %d %c %c",
                  &m_Orient, &m_Pos.x, &m_Pos.y, &m_Size.x, &m_Attributs,
                  &m_Unit, &m_Convert, buf, tmp, &thickness, &hjustify,
                  &vjustify );

    if( cnt < 8 )
    {
        errorMsg.Printf( _( "text only had %d parameters of the required 8" ),
                         cnt );
        return false;
    }

    m_Size.y = m_Size.x;

    if( strnicmp( tmp, "Italic", 6 ) == 0 )
        m_Italic = true;
    if( thickness > 0 )
    {
        m_Bold = true;
    }

    switch( hjustify )
    {
    case 'L':
        m_HJustify = GR_TEXT_HJUSTIFY_LEFT;
        break;

    case 'C':
        m_HJustify = GR_TEXT_HJUSTIFY_CENTER;
        break;

    case 'R':
        m_HJustify = GR_TEXT_HJUSTIFY_RIGHT;
        break;
    }

    switch( vjustify )
    {
    case 'T':
        m_VJustify = GR_TEXT_VJUSTIFY_TOP;
        break;

    case 'C':
        m_VJustify = GR_TEXT_VJUSTIFY_CENTER;
        break;

    case 'B':
        m_VJustify = GR_TEXT_VJUSTIFY_BOTTOM;
        break;
    }

    /* Convert '~' to spaces. */
    m_Text = CONV_FROM_UTF8( buf );
    m_Text.Replace( wxT( "~" ), wxT( " " ) );

    return true;
}

/**
 * Function HitTest
 * tests if the given wxPoint is within the bounds of this object.
 * @param refPos A wxPoint to test
 * @return bool - true if a hit, else false
 */
bool LibDrawText::HitTest( const wxPoint& refPos )
{
    return HitTest( refPos, 0, DefaultTransformMatrix );
}


/** Function HitTest
 * @return true if the point aPosRef is near this item
 * @param aPosRef = a wxPoint to test, in eeschema space
 * @param aThreshold = unused here (TextHitTest calculates its threshold )
 * @param aTransMat = the transform matrix
 */
bool LibDrawText::HitTest( wxPoint aPosRef, int aThreshold,
                           const int aTransMat[2][2] )
{
    wxPoint physicalpos = TransformCoordinate( aTransMat, m_Pos );
    wxPoint tmp = m_Pos;
    m_Pos = physicalpos;
    /* The text orientation may need to be flipped if the
     *  transformation matrix causes xy axes to be flipped.
     * this simple algo works only for schematic matrix (rot 90 or/and mirror)
    */
    int t1 = ( aTransMat[0][0] != 0 ) ^ ( m_Orient != 0 );
    int orient = t1 ? TEXT_ORIENT_HORIZ : TEXT_ORIENT_VERT;
    EXCHG( m_Orient, orient );
    bool hit = TextHitTest(aPosRef);
    EXCHG( m_Orient, orient );
    m_Pos = tmp;
    return hit;
}


LibEDA_BaseStruct* LibDrawText::DoGenCopy()
{
    LibDrawText* newitem = new LibDrawText(NULL);

    newitem->m_Pos = m_Pos;
    newitem->m_Orient    = m_Orient;
    newitem->m_Size      = m_Size;
    newitem->m_Attributs = m_Attributs;
    newitem->m_Unit      = m_Unit;
    newitem->m_Convert   = m_Convert;
    newitem->m_Flags     = m_Flags;
    newitem->m_Text      = m_Text;
    newitem->m_Width     = m_Width;
    newitem->m_Italic    = m_Italic;
    newitem->m_Bold      = m_Bold;
    newitem->m_HJustify  = m_HJustify;
    newitem->m_VJustify  = m_VJustify;
    return (LibEDA_BaseStruct*) newitem;
}


bool LibDrawText::DoCompare( const LibEDA_BaseStruct& other ) const
{
    wxASSERT( other.Type() == COMPONENT_GRAPHIC_TEXT_DRAW_TYPE );

    const LibDrawText* tmp = ( LibDrawText* ) &other;

    return ( ( m_Pos == tmp->m_Pos ) && ( m_Size == tmp->m_Size )
             && ( m_Text == tmp->m_Text ) );
}


void LibDrawText::DoOffset( const wxPoint& offset )
{
    m_Pos += offset;
}


bool LibDrawText::DoTestInside( EDA_Rect& rect )
{
    /*
     * FIXME: This should calculate the text size and justification and
     *        use rectangle instect.
     */
    return rect.Inside( m_Pos.x, -m_Pos.y );
}


/** Function GetPenSize
 * @return the size of the "pen" that be used to draw or plot this item
 */
int LibDrawText::GetPenSize( )
{
    int     pensize = m_Width;

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

void LibDrawText::Draw( WinEDA_DrawPanel* aPanel, wxDC* aDC,
                        const wxPoint& aOffset, int aColor, int aDrawMode,
                        void* aData, const int aTransformMatrix[2][2] )
{
    wxPoint pos1, pos2;

    int     color     = ReturnLayerColor( LAYER_DEVICE );
    if( aColor < 0 )       // Used normal color or selected color
    {
        if( ( m_Selected & IS_SELECTED ) )
            color = g_ItemSelectetColor;
    }
    else
        color = aColor;

    pos1 = TransformCoordinate( aTransformMatrix, m_Pos ) + aOffset;

    /* The text orientation may need to be flipped if the
     *  transformation matrix causes xy axes to be flipped. */
    int t1 = ( aTransformMatrix[0][0] != 0 ) ^ ( m_Orient != 0 );

    GRSetDrawMode( aDC, aDrawMode );

    DrawGraphicText( aPanel, aDC, pos1, (EDA_Colors) color, m_Text,
                     t1 ? TEXT_ORIENT_HORIZ : TEXT_ORIENT_VERT,
                     m_Size, m_HJustify, m_VJustify,
                     GetPenSize( ), m_Italic, m_Bold );
}


void LibDrawText::DisplayInfo( WinEDA_DrawFrame* frame )
{
    wxString msg;

    LibEDA_BaseStruct::DisplayInfo( frame );

    msg = ReturnStringFromValue( g_UnitMetric, m_Width,
                                 EESCHEMA_INTERNAL_UNIT, true );

    frame->MsgPanel->Affiche_1_Parametre( 20, _( "Line width" ), msg, BLUE );
}
