/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2004-2012 KiCad Developers, see change_log.txt for contributors.
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
 * @file lib_text.cpp
 */

#include <fctsys.h>
#include <gr_basic.h>
#include <macros.h>
#include <class_drawpanel.h>
#include <plot_common.h>
#include <drawtxt.h>
#include <trigo.h>
#include <wxstruct.h>
#include <richio.h>
#include <base_units.h>
#include <msgpanel.h>

#include <lib_draw_item.h>
#include <general.h>
#include <transform.h>
#include <lib_text.h>


LIB_TEXT::LIB_TEXT( LIB_PART * aParent ) :
    LIB_ITEM( LIB_TEXT_T, aParent ),
    EDA_TEXT()
{
    m_Size       = wxSize( 50, 50 );
    m_typeName   = _( "Text" );
    m_rotate     = false;
    m_updateText = false;
}


bool LIB_TEXT::Save( OUTPUTFORMATTER& aFormatter )
{
    wxString text = m_Text;

    if( text.Contains( wxT( "~" ) ) || text.Contains( wxT( "\"" ) ) )
    {
        // convert double quote to similar-looking two apostrophes
        text.Replace( wxT( "\"" ), wxT( "''" ) );
        text = wxT( "\"" ) + text + wxT( "\"" );
    }
    else
    {
        // Spaces are not allowed in text because it is not double quoted:
        // changed to '~'
        text.Replace( wxT( " " ), wxT( "~" ) );
    }

    aFormatter.Print( 0, "T %g %d %d %d %d %d %d %s ", GetOrientation(), m_Pos.x, m_Pos.y,
                      m_Size.x, m_Attributs, m_Unit, m_Convert, TO_UTF8( text ) );

    aFormatter.Print( 0, " %s %d", m_Italic ? "Italic" : "Normal", ( m_Bold > 0 ) ? 1 : 0 );

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

    aFormatter.Print( 0, " %c %c\n", hjustify, vjustify );

    return true;
}


bool LIB_TEXT::Load( LINE_READER& aLineReader, wxString& errorMsg )
{
    int     cnt, thickness = 0;
    char    hjustify = 'C', vjustify = 'C';
    char    buf[256];
    char    tmp[256];
    char*   line = (char*) aLineReader;
    double  angle;

    buf[0] = 0;
    tmp[0] = 0;         // For italic option, Not in old versions

    cnt = sscanf( line + 2, "%lf %d %d %d %d %d %d \"%[^\"]\" %255s %d %c %c",
                  &angle, &m_Pos.x, &m_Pos.y, &m_Size.x, &m_Attributs,
                  &m_Unit, &m_Convert, buf, tmp, &thickness, &hjustify,
                  &vjustify );

    if( cnt >= 8 ) // if quoted loading failed, load as not quoted
    {
        m_Text = FROM_UTF8( buf );

        // convert two apostrophes back to double quote
        m_Text.Replace( wxT( "''" ), wxT( "\"" ) );
    }
    else
    {
        cnt = sscanf( line + 2, "%lf %d %d %d %d %d %d %255s %255s %d %c %c",
                      &angle, &m_Pos.x, &m_Pos.y, &m_Size.x, &m_Attributs,
                      &m_Unit, &m_Convert, buf, tmp, &thickness, &hjustify,
                      &vjustify );

        if( cnt < 8 )
        {
            errorMsg.Printf( _( "Text only had %d parameters of the required 8" ), cnt );
            return false;
        }

        /* Convert '~' to spaces (only if text is not quoted). */
        m_Text = FROM_UTF8( buf );
        m_Text.Replace( wxT( "~" ), wxT( " " ) );
    }

    SetOrientation( angle );

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


    return true;
}


bool LIB_TEXT::HitTest( const wxPoint& aPosition ) const
{
    return HitTest( aPosition, 0, DefaultTransform );
}


bool LIB_TEXT::HitTest( const wxPoint &aPosition, int aThreshold, const TRANSFORM& aTransform ) const
{
    if( aThreshold < 0 )
        aThreshold = 0;

    EDA_TEXT tmp_text( *this );
    tmp_text.SetTextPosition( aTransform.TransformCoordinate( m_Pos ) );

    /* The text orientation may need to be flipped if the
     *  transformation matrix causes xy axes to be flipped.
     * this simple algo works only for schematic matrix (rot 90 or/and mirror)
     */
    int t1 = ( aTransform.x1 != 0 ) ^ ( m_Orient != 0 );
    tmp_text.SetOrientation( t1 ? TEXT_ORIENT_HORIZ : TEXT_ORIENT_VERT );
    return tmp_text.TextHitTest( aPosition );
}


EDA_ITEM* LIB_TEXT::Clone() const
{
    LIB_TEXT* newitem = new LIB_TEXT(NULL);

    newitem->m_Pos       = m_Pos;
    newitem->m_Orient    = m_Orient;
    newitem->m_Size      = m_Size;
    newitem->m_Attributs = m_Attributs;
    newitem->m_Unit      = m_Unit;
    newitem->m_Convert   = m_Convert;
    newitem->m_Flags     = m_Flags;
    newitem->m_Text      = m_Text;
    newitem->m_Thickness = m_Thickness;
    newitem->m_Italic    = m_Italic;
    newitem->m_Bold      = m_Bold;
    newitem->m_HJustify  = m_HJustify;
    newitem->m_VJustify  = m_VJustify;
    return newitem;
}


int LIB_TEXT::compare( const LIB_ITEM& other ) const
{
    wxASSERT( other.Type() == LIB_TEXT_T );

    const LIB_TEXT* tmp = ( LIB_TEXT* ) &other;

    int result = m_Text.CmpNoCase( tmp->m_Text );

    if( result != 0 )
        return result;

    if( m_Pos.x != tmp->m_Pos.x )
        return m_Pos.x - tmp->m_Pos.x;

    if( m_Pos.y != tmp->m_Pos.y )
        return m_Pos.y - tmp->m_Pos.y;

    if( m_Size.x != tmp->m_Size.x )
        return m_Size.x - tmp->m_Size.x;

    if( m_Size.y != tmp->m_Size.y )
        return m_Size.y - tmp->m_Size.y;

    return 0;
}


void LIB_TEXT::SetOffset( const wxPoint& aOffset )
{
    m_Pos += aOffset;
}


bool LIB_TEXT::Inside( EDA_RECT& rect ) const
{
    /*
     * FIXME: This should calculate the text size and justification and
     *        use rectangle intersect.
     */
    return rect.Contains( m_Pos.x, -m_Pos.y );
}


void LIB_TEXT::Move( const wxPoint& newPosition )
{
    m_Pos = newPosition;
}


void LIB_TEXT::MirrorHorizontal( const wxPoint& center )
{
    m_Pos.x -= center.x;
    m_Pos.x *= -1;
    m_Pos.x += center.x;
}

void LIB_TEXT::MirrorVertical( const wxPoint& center )
{
    m_Pos.y -= center.y;
    m_Pos.y *= -1;
    m_Pos.y += center.y;
}

void LIB_TEXT::Rotate( const wxPoint& center, bool aRotateCCW )
{
    int rot_angle = aRotateCCW ? -900 : 900;

    RotatePoint( &m_Pos, center, rot_angle );
    m_Orient = m_Orient ? 0 : 900;
}


void LIB_TEXT::Plot( PLOTTER* plotter, const wxPoint& offset, bool fill,
                     const TRANSFORM& aTransform )
{
    wxASSERT( plotter != NULL );

    /* The text orientation may need to be flipped if the
     * transformation matrix causes xy axes to be flipped. */
    int t1  = ( aTransform.x1 != 0 ) ^ ( m_Orient != 0 );
    wxPoint pos = aTransform.TransformCoordinate( m_Pos ) + offset;

    // Get color
    EDA_COLOR_T     color;

    if( plotter->GetColorMode() )       // Used normal color or selected color
        color = IsSelected() ? GetItemSelectedColor() : GetDefaultColor();
    else
        color = BLACK;

    plotter->Text( pos, color, GetShownText(),
                   t1 ? TEXT_ORIENT_HORIZ : TEXT_ORIENT_VERT,
                   m_Size, GR_TEXT_HJUSTIFY_CENTER, GR_TEXT_VJUSTIFY_CENTER,
                   GetPenSize(), m_Italic, m_Bold );
}


int LIB_TEXT::GetPenSize() const
{
    int     pensize = m_Thickness;

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


void LIB_TEXT::drawGraphic( EDA_DRAW_PANEL* aPanel, wxDC* aDC, const wxPoint& aOffset,
                            EDA_COLOR_T aColor, GR_DRAWMODE aDrawMode, void* aData,
                            const TRANSFORM& aTransform )
{
    EDA_COLOR_T color = GetDefaultColor();

    if( aColor < 0 )       // Used normal color or selected color
    {
        if( IsSelected() )
            color = GetItemSelectedColor();
    }
    else
    {
        color = aColor;
    }

    GRSetDrawMode( aDC, aDrawMode );

    /* Calculate the text orientation, according to the component
     * orientation/mirror (needed when draw text in schematic)
     */
    int orient = m_Orient;

    if( aTransform.y1 )  // Rotate component 90 degrees.
    {
        if( orient == TEXT_ORIENT_HORIZ )
            orient = TEXT_ORIENT_VERT;
        else
            orient = TEXT_ORIENT_HORIZ;
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
    EDA_RECT bBox = GetBoundingBox();
    // convert coordinates from draw Y axis to libedit Y axis:
    bBox.RevertYAxis();
    wxPoint txtpos = bBox.Centre();

    // Calculate pos according to mirror/rotation.
    txtpos = aTransform.TransformCoordinate( txtpos ) + aOffset;

    EDA_RECT* clipbox = aPanel? aPanel->GetClipBox() : NULL;
    DrawGraphicText( clipbox, aDC, txtpos, color, GetShownText(), orient, m_Size,
                     GR_TEXT_HJUSTIFY_CENTER, GR_TEXT_VJUSTIFY_CENTER, GetPenSize(),
                     m_Italic, m_Bold );


    /* Enable this to draw the bounding box around the text field to validate
     * the bounding box calculations.
     */
#if 0
    // bBox already uses libedit Y axis.
    bBox = aTransform.TransformCoordinate( bBox );
    bBox.Move( aOffset );
    GRRect( clipbox, aDC, bBox, 0, LIGHTMAGENTA );
#endif
}


void LIB_TEXT::GetMsgPanelInfo( MSG_PANEL_ITEMS& aList )
{
    wxString msg;

    LIB_ITEM::GetMsgPanelInfo( aList );

    msg = StringFromValue( g_UserUnit, m_Thickness, true );

    aList.push_back( MSG_PANEL_ITEM( _( "Line Width" ), msg, BLUE ) );
}


const EDA_RECT LIB_TEXT::GetBoundingBox() const
{
    /* Y coordinates for LIB_ITEMS are bottom to top, so we must invert the Y position when
     * calling GetTextBox() that works using top to bottom Y axis orientation.
     */
    EDA_RECT rect = GetTextBox( -1, -1, true );
    rect.RevertYAxis();

    // We are using now a bottom to top Y axis.
    wxPoint orig = rect.GetOrigin();
    wxPoint end = rect.GetEnd();
    RotatePoint( &orig, m_Pos, -m_Orient );
    RotatePoint( &end, m_Pos, -m_Orient );

    rect.SetOrigin( orig );
    rect.SetEnd( end );

    // We are using now a top to bottom Y axis:
    rect.RevertYAxis();

    return rect;
}


void LIB_TEXT::Rotate()
{
    if( InEditMode() )
    {
        m_rotate = true;
    }
    else
    {
        m_Orient = ( m_Orient == TEXT_ORIENT_VERT ) ? TEXT_ORIENT_HORIZ : TEXT_ORIENT_VERT;
    }
}


void LIB_TEXT::SetText( const wxString& aText )
{
    if( aText == m_Text )
        return;

    if( InEditMode() )
    {
        m_savedText = aText;
        m_updateText = true;
    }
    else
    {
        m_Text = aText;
    }
}


wxString LIB_TEXT::GetSelectMenuText() const
{
    wxString msg;
    msg.Printf( _( "Graphic Text %s" ), GetChars( ShortenedShownText() ) );
    return msg;
}


void LIB_TEXT::BeginEdit( STATUS_FLAGS aEditMode, const wxPoint aPosition )
{
    wxCHECK_RET( ( aEditMode & ( IS_NEW | IS_MOVED ) ) != 0,
                 wxT( "Invalid edit mode for LIB_TEXT object." ) );

    if( aEditMode == IS_MOVED )
    {
        m_initialPos = m_Pos;
        m_initialCursorPos = aPosition;
        SetEraseLastDrawItem();
    }
    else
    {
        m_Pos = aPosition;
    }

    m_Flags = aEditMode;
}


bool LIB_TEXT::ContinueEdit( const wxPoint aPosition )
{
    wxCHECK_MSG( ( m_Flags & ( IS_NEW | IS_MOVED ) ) != 0, false,
                   wxT( "Bad call to ContinueEdit().  Text is not being edited." ) );

    return false;
}


void LIB_TEXT::EndEdit( const wxPoint& aPosition, bool aAbort )
{
    wxCHECK_RET( ( m_Flags & ( IS_NEW | IS_MOVED ) ) != 0,
                   wxT( "Bad call to EndEdit().  Text is not being edited." ) );

    m_Flags = 0;
    m_rotate = false;
    m_updateText = false;
    SetEraseLastDrawItem( false );
}


void LIB_TEXT::calcEdit( const wxPoint& aPosition )
{
    if( m_rotate )
    {
        m_Orient = ( m_Orient == TEXT_ORIENT_VERT ) ? TEXT_ORIENT_HORIZ : TEXT_ORIENT_VERT;
        m_rotate = false;
    }

    if( m_updateText )
    {
        EXCHG( m_Text, m_savedText );
        m_updateText = false;
    }

    if( m_Flags == IS_NEW )
    {
        SetEraseLastDrawItem();
        m_Pos = aPosition;
    }
    else if( m_Flags == IS_MOVED )
    {
        Move( m_initialPos + aPosition - m_initialCursorPos );
    }
}
