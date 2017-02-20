/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2012 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 2004-2017 KiCad Developers, see AUTHORS.txt for contributors.
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
 * @file lib_field.cpp
 */

#include <fctsys.h>
#include <pgm_base.h>
#include <gr_basic.h>
#include <macros.h>
#include <base_struct.h>
#include <drawtxt.h>
#include <kicad_string.h>
#include <class_drawpanel.h>
#include <plot_common.h>
#include <trigo.h>
#include <base_units.h>
#include <msgpanel.h>
#include <bitmaps.h>

#include <general.h>
#include <class_libentry.h>
#include <transform.h>
#include <lib_field.h>
#include <template_fieldnames.h>


LIB_FIELD::LIB_FIELD(LIB_PART * aParent, int idfield ) :
    LIB_ITEM( LIB_FIELD_T, aParent )
{
    Init( idfield );
}


LIB_FIELD::LIB_FIELD( int idfield ) :
    LIB_ITEM( LIB_FIELD_T, NULL )
{
    Init( idfield );
}


LIB_FIELD::~LIB_FIELD()
{
}


void LIB_FIELD::operator=( const LIB_FIELD& field )
{
    m_id = field.m_id;
    m_Text = field.m_Text;
    m_name = field.m_name;
    m_Parent = field.m_Parent;

    SetEffects( field );
}


void LIB_FIELD::Init( int id )
{
    m_id = id;

    SetTextWidth( GetDefaultTextSize() );
    SetTextHeight( GetDefaultTextSize() );

    m_typeName = _( "Field" );

    SetTextAngle( TEXT_ANGLE_HORIZ );    // constructor already did this.

    m_rotate = false;
    m_updateText = false;

    // fields in RAM must always have names, because we are trying to get
    // less dependent on field ids and more dependent on names.
    // Plus assumptions are made in the field editors.
    m_name = TEMPLATE_FIELDNAME::GetDefaultFieldName( id );

    switch( id )
    {
    case DATASHEET:
    case FOOTPRINT:
        // by contrast, VALUE and REFERENCE are are always constructed as
        // initially visible, and template fieldsnames' initial visibility
        // is controlled by the template fieldname configuration record.
        SetVisible( false );
        break;
    }
}


bool LIB_FIELD::Save( OUTPUTFORMATTER& aFormatter )
{
    int      hjustify, vjustify;
    wxString text = m_Text;

    hjustify = 'C';

    if( GetHorizJustify() == GR_TEXT_HJUSTIFY_LEFT )
        hjustify = 'L';
    else if( GetHorizJustify() == GR_TEXT_HJUSTIFY_RIGHT )
        hjustify = 'R';

    vjustify = 'C';

    if( GetVertJustify() == GR_TEXT_VJUSTIFY_BOTTOM )
        vjustify = 'B';
    else if( GetVertJustify() == GR_TEXT_VJUSTIFY_TOP )
        vjustify = 'T';

    aFormatter.Print( 0, "F%d %s %d %d %d %c %c %c %c%c%c",
                      m_id,
                      EscapedUTF8( text ).c_str(),       // wraps in quotes
                      GetTextPos().x, GetTextPos().y, GetTextWidth(),
                      GetTextAngle() == 0 ? 'H' : 'V',
                      IsVisible() ? 'V' : 'I',
                      hjustify, vjustify,
                      IsItalic() ? 'I' : 'N',
                      IsBold() ? 'B' : 'N' );

    /* Save field name, if necessary
     * Field name is saved only if it is not the default name.
     * Just because default name depends on the language and can change from
     * a country to an other
     */
    wxString defName = TEMPLATE_FIELDNAME::GetDefaultFieldName( m_id );

    if( m_id >= FIELD1 && !m_name.IsEmpty() && m_name != defName )
        aFormatter.Print( 0, " %s", EscapedUTF8( m_name ).c_str() );

    aFormatter.Print( 0, "\n" );

    return true;
}


bool LIB_FIELD::Load( LINE_READER& aLineReader, wxString& errorMsg )
{
    int     cnt;
    int     x, y, size;
    char    textOrient;
    char    textVisible;
    char    textHJustify;
    char    textVJustify[256];

    char*   line = (char*) aLineReader;
    char*   limit = line + aLineReader.Length();

    if( sscanf( line + 1, "%d", &m_id ) != 1 || m_id < 0 )
    {
        errorMsg = wxT( "invalid field header" );
        return false;
    }

    // Caller did a strtok(), which inserts a nul, so next few bytes are ugly:
    // digit(s), a nul, some whitespace, then a double quote.
    while( line < limit && *line != '"' )
        line++;

    if( line == limit )
        return false;

    line += ReadDelimitedText( &m_Text, line );

    // Doctor the *.lib file field which has a "~" in blank fields.  New saves will
    // not save like this, and eventually these two lines can be removed.
    if( m_Text.size() == 1 && m_Text[0] == wxChar( '~' ) )
        m_Text.clear();

    memset( textVJustify, 0, sizeof( textVJustify ) );

    cnt = sscanf( line, " %d %d %d %c %c %c %255s", &x, &y, &size,
                  &textOrient, &textVisible, &textHJustify, textVJustify );

    if( cnt < 5 )
    {
        errorMsg.Printf( wxT( "field %d does not have the correct number of parameters" ),
                         m_id );
        return false;
    }

    SetTextPos( wxPoint( x, y ) );
    SetTextSize( wxSize( size, size ) );

    if( textOrient == 'H' )
        SetTextAngle( TEXT_ANGLE_HORIZ );
    else if( textOrient == 'V' )
        SetTextAngle( TEXT_ANGLE_VERT );
    else
    {
        errorMsg.Printf( wxT( "field %d text orientation parameter <%c> is not valid" ),
                         textOrient );
        return false;
    }

    if( textVisible == 'V' )
        SetVisible( true );
    else if( textVisible == 'I' )
        SetVisible( false );
    else
    {
        errorMsg.Printf( wxT( "field %d text visible parameter <%c> is not valid" ),
                         textVisible );
        return false;
    }

    SetHorizJustify( GR_TEXT_HJUSTIFY_CENTER );
    SetVertJustify( GR_TEXT_VJUSTIFY_CENTER );

    if( cnt >= 6 )
    {
        if( textHJustify == 'C' )
            SetHorizJustify( GR_TEXT_HJUSTIFY_CENTER );
        else if( textHJustify == 'L' )
            SetHorizJustify( GR_TEXT_HJUSTIFY_LEFT );
        else if( textHJustify == 'R' )
            SetHorizJustify( GR_TEXT_HJUSTIFY_RIGHT );
        else
        {
            errorMsg.Printf(
                wxT( "field %d text horizontal justification parameter <%c> is not valid" ),
                textHJustify );
            return false;
        }

        if( textVJustify[0] == 'C' )
            SetVertJustify( GR_TEXT_VJUSTIFY_CENTER );
        else if( textVJustify[0] == 'B' )
            SetVertJustify( GR_TEXT_VJUSTIFY_BOTTOM );
        else if( textVJustify[0] == 'T' )
            SetVertJustify( GR_TEXT_VJUSTIFY_TOP );
        else
        {
            errorMsg.Printf(
                wxT( "field %d text vertical justification parameter <%c> is not valid" ),
                textVJustify[0] );
            return false;
        }

        if( textVJustify[1] == 'I' )  // Italic
            SetItalic( true );
        if( textVJustify[2] == 'B' )  // Bold
            SetBold( true );
    }

    // fields in RAM must always have names.
    if( m_id < MANDATORY_FIELDS )
    {
        // Fields in RAM must always have names, because we are trying to get
        // less dependent on field ids and more dependent on names.
        // Plus assumptions are made in the field editors.
        m_name = TEMPLATE_FIELDNAME::GetDefaultFieldName( m_id );
    }
    else
    {
        ReadDelimitedText( &m_name, line );
    }

    return true;
}


int LIB_FIELD::GetPenSize() const
{
    return GetThickness() == 0 ? GetDefaultLineThickness() : GetThickness();
}


void LIB_FIELD::drawGraphic( EDA_DRAW_PANEL* aPanel, wxDC* aDC, const wxPoint& aOffset,
                             COLOR4D aColor, GR_DRAWMODE aDrawMode, void* aData,
                             const TRANSFORM& aTransform )
{
    wxPoint  text_pos;
    COLOR4D color = COLOR4D::UNSPECIFIED;
    int      linewidth = GetPenSize();

    if( IsBold() )
        linewidth = GetPenSizeForBold( GetTextWidth() );
    else
        linewidth = Clamp_Text_PenSize( linewidth, GetTextSize(), IsBold() );

    if( !IsVisible() && ( aColor == COLOR4D::UNSPECIFIED ) )
    {
        color = GetInvisibleItemColor();
    }
    else if( IsSelected() && ( aColor == COLOR4D::UNSPECIFIED ) )
    {
        color = GetItemSelectedColor();
    }
    else
    {
        color = aColor;
    }

    if( color == COLOR4D::UNSPECIFIED )
        color = GetDefaultColor();

    text_pos = aTransform.TransformCoordinate( GetTextPos() ) + aOffset;

    wxString text;

    if( aData )
        text = *(wxString*)aData;
    else
        text = m_Text;

    GRSetDrawMode( aDC, aDrawMode );
    EDA_RECT* clipbox = aPanel? aPanel->GetClipBox() : NULL;

    DrawGraphicText( clipbox, aDC, text_pos, color, text,
                     GetTextAngle(), GetTextSize(),
                     GetHorizJustify(), GetVertJustify(),
                     linewidth, IsItalic(), IsBold() );

    /* Set to one (1) to draw bounding box around field text to validate
     * bounding box calculation. */
#if 0
    EDA_RECT bBox = GetBoundingBox();
    bBox.RevertYAxis();
    bBox = aTransform.TransformCoordinate( bBox );
    bBox.Move( aOffset );
    GRRect( clipbox, aDC, bBox, 0, LIGHTMAGENTA );
#endif
}


bool LIB_FIELD::HitTest( const wxPoint& aPosition ) const
{
    // Because HitTest is mainly used to select the field
    // return always false if this field is void
    if( IsVoid() )
        return false;

    return HitTest( aPosition, 0, DefaultTransform );
}


bool LIB_FIELD::HitTest( const wxPoint &aPosition, int aThreshold, const TRANSFORM& aTransform ) const
{
    if( aThreshold < 0 )
        aThreshold = 0;

    // Build a temporary copy of the text for hit testing
    EDA_TEXT tmp_text( *this );

    // Reference designator text has one or 2 additional character (displays
    // U? or U?A)
    if( m_id == REFERENCE )
    {
        wxString extended_text = tmp_text.GetText();
        extended_text.Append('?');
        const LIB_PART*      parent = static_cast<const LIB_PART*>( m_Parent );

        if ( parent && ( parent->GetUnitCount() > 1 ) )
            extended_text.Append('A');
        tmp_text.SetText( extended_text );
    }

    tmp_text.SetTextPos( aTransform.TransformCoordinate( GetTextPos() ) );

    /* The text orientation may need to be flipped if the
     *  transformation matrix causes xy axes to be flipped.
     * this simple algo works only for schematic matrix (rot 90 or/and mirror)
     */
    bool t1 = ( aTransform.x1 != 0 ) ^ ( GetTextAngle() != 0 );
    tmp_text.SetTextAngle( t1 ? TEXT_ANGLE_HORIZ : TEXT_ANGLE_VERT );

    return tmp_text.TextHitTest( aPosition );
}


EDA_ITEM* LIB_FIELD::Clone() const
{
    LIB_FIELD* newfield = new LIB_FIELD( m_id );

    Copy( newfield );

    return (EDA_ITEM*) newfield;
}


void LIB_FIELD::Copy( LIB_FIELD* aTarget ) const
{
    aTarget->m_Text = m_Text;
    aTarget->m_name = m_name;

    aTarget->SetEffects( *this );
    aTarget->SetParent( m_Parent );
}


int LIB_FIELD::compare( const LIB_ITEM& other ) const
{
    wxASSERT( other.Type() == LIB_FIELD_T );

    const LIB_FIELD* tmp = ( LIB_FIELD* ) &other;

    if( m_id != tmp->m_id )
        return m_id - tmp->m_id;

    int result = m_Text.CmpNoCase( tmp->m_Text );

    if( result != 0 )
        return result;

    if( GetTextPos().x != tmp->GetTextPos().x )
        return GetTextPos().x - tmp->GetTextPos().x;

    if( GetTextPos().y != tmp->GetTextPos().y )
        return GetTextPos().y - tmp->GetTextPos().y;

    if( GetTextWidth() != tmp->GetTextWidth() )
        return GetTextWidth() - tmp->GetTextWidth();

    if( GetTextHeight() != tmp->GetTextHeight() )
        return GetTextHeight() - tmp->GetTextHeight();

    return 0;
}


void LIB_FIELD::SetOffset( const wxPoint& aOffset )
{
    EDA_TEXT::Offset( aOffset );
}


bool LIB_FIELD::Inside( EDA_RECT& rect ) const
{
    /*
     * FIXME: This fails to take into account the size and/or orientation of
     *        the text.
     */
    return rect.Contains( GetTextPos().x, -GetTextPos().y );
}


void LIB_FIELD::Move( const wxPoint& newPosition )
{
    EDA_TEXT::SetTextPos( newPosition );
}


void LIB_FIELD::MirrorHorizontal( const wxPoint& center )
{
    int x = GetTextPos().x;

    x -= center.x;
    x *= -1;
    x += center.x;

    SetTextX( x );
}


void LIB_FIELD::MirrorVertical( const wxPoint& center )
{
    int y = GetTextPos().y;

    y -= center.y;
    y *= -1;
    y += center.y;

    SetTextY( y );
}


void LIB_FIELD::Rotate( const wxPoint& center, bool aRotateCCW )
{
    int rot_angle = aRotateCCW ? -900 : 900;

    wxPoint pt = GetTextPos();
    RotatePoint( &pt, center, rot_angle );
    SetTextPos( pt );

    SetTextAngle( GetTextAngle() != 0.0 ? 0 : 900 );
}


void LIB_FIELD::Plot( PLOTTER* aPlotter, const wxPoint& aOffset, bool aFill,
                      const TRANSFORM& aTransform )
{
    if( IsVoid() )
        return;

    /* Calculate the text orientation, according to the component
     * orientation/mirror */
    int orient = GetTextAngle();

    if( aTransform.y1 )  // Rotate component 90 deg.
    {
        if( orient == TEXT_ANGLE_HORIZ )
            orient = TEXT_ANGLE_VERT;
        else
            orient = TEXT_ANGLE_HORIZ;
    }

    EDA_RECT BoundaryBox = GetBoundingBox();
    BoundaryBox.RevertYAxis();

    EDA_TEXT_HJUSTIFY_T hjustify = GR_TEXT_HJUSTIFY_CENTER;
    EDA_TEXT_VJUSTIFY_T vjustify = GR_TEXT_VJUSTIFY_CENTER;
    wxPoint textpos = aTransform.TransformCoordinate( BoundaryBox.Centre() )
                      + aOffset;

    aPlotter->Text( textpos, GetDefaultColor(), GetShownText(),
                    orient, GetTextSize(),
                    hjustify, vjustify,
                    GetPenSize(), IsItalic(), IsBold() );
}


wxString LIB_FIELD::GetFullText( int unit ) const
{
    if( m_id != REFERENCE )
        return GetText();

    wxString text = GetText();
    text << wxT( "?" );

    if( GetParent()->IsMulti() )
        text << LIB_PART::SubReference( unit );

    return text;
}


const EDA_RECT LIB_FIELD::GetBoundingBox() const
{
    /* Y coordinates for LIB_ITEMS are bottom to top, so we must invert the Y position when
     * calling GetTextBox() that works using top to bottom Y axis orientation.
     */
    EDA_RECT rect = GetTextBox( -1, -1, true );
    rect.RevertYAxis();

    // We are using now a bottom to top Y axis.
    wxPoint orig = rect.GetOrigin();
    wxPoint end = rect.GetEnd();

    RotatePoint( &orig, GetTextPos(), -GetTextAngle() );
    RotatePoint( &end, GetTextPos(), -GetTextAngle() );

    rect.SetOrigin( orig );
    rect.SetEnd( end );

    // We are using now a top to bottom Y axis:
    rect.RevertYAxis();

    return rect;
}


COLOR4D LIB_FIELD::GetDefaultColor()
{
    COLOR4D color;

    switch( m_id )
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

    return color;
}


void LIB_FIELD::Rotate()
{
    if( InEditMode() )
    {
        m_rotate = true;
    }
    else
    {
        SetTextAngle( GetTextAngle() == TEXT_ANGLE_VERT ? TEXT_ANGLE_HORIZ : TEXT_ANGLE_VERT );
    }
}


wxString LIB_FIELD::GetName( bool aTranslate ) const
{
    wxString name;

    switch( m_id )
    {
    case REFERENCE:
        if( aTranslate )
            name = _( "Reference" );
        else
            name = wxT( "Reference" );
        break;

    case VALUE:
        if( aTranslate )
            name = _( "Value" );
        else
            name = wxT( "Value" );
        break;

    case FOOTPRINT:
        if( aTranslate )
            name = _( "Footprint" );
        else
            name = wxT( "Footprint" );
        break;

    case DATASHEET:
        if( aTranslate )
           name = _( "Datasheet" );
        else
           name = wxT( "Datasheet" );
        break;

    default:
        if( m_name.IsEmpty() )
        {
            if( aTranslate )
                name.Printf( _( "Field%d" ), m_id );
            else
                name.Printf( wxT( "Field%d" ), m_id );
        }
        else
            name = m_name;
    }

    return name;
}


void LIB_FIELD::SetName( const wxString& aName )
{
    // Mandatory field names are fixed.

    // So what?  Why should the low level code be in charge of such a policy issue?
    // Besides, m_id is a relic that is untrustworthy now.
    if( m_id >=0 && m_id < MANDATORY_FIELDS )
    {
        DBG(printf( "trying to set a MANDATORY_FIELD's name\n" );)
        return;
    }

    if( m_name != aName )
    {
        m_name = aName;
        SetModified();
    }
}


void LIB_FIELD::SetText( const wxString& aText )
{
    if( aText == GetText() )
        return;

    wxString oldName = m_Text;

    if( m_id == VALUE && m_Parent != NULL )
    {
        LIB_PART*      parent = GetParent();

        // Set the parent component and root alias to the new name.
        if( parent->GetName().CmpNoCase( aText ) != 0 )
            parent->SetName( aText );
    }

    if( InEditMode() )
    {
        m_Text = oldName;
        m_savedText = aText;
        m_updateText = true;
    }
    else
    {
        m_Text = aText;
    }
}


wxString LIB_FIELD::GetSelectMenuText() const
{
    return wxString::Format( _( "Field %s %s" ),
                             GetChars( GetName() ),
                             GetChars( ShortenedShownText() ) );
}


void LIB_FIELD::BeginEdit( STATUS_FLAGS aEditMode, const wxPoint aPosition )
{
    wxCHECK_RET( ( aEditMode & ( IS_NEW | IS_MOVED ) ) != 0,
                 wxT( "Invalid edit mode for LIB_FIELD object." ) );

    if( aEditMode == IS_MOVED )
    {
        m_initialPos = GetTextPos();
        m_initialCursorPos = aPosition;
        SetEraseLastDrawItem();
    }
    else
    {
        SetTextPos( aPosition );
    }

    m_Flags = aEditMode;
}


bool LIB_FIELD::ContinueEdit( const wxPoint aPosition )
{
    wxCHECK_MSG( ( m_Flags & ( IS_NEW | IS_MOVED ) ) != 0, false,
                   wxT( "Bad call to ContinueEdit().  Text is not being edited." ) );

    return false;
}


void LIB_FIELD::EndEdit( const wxPoint& aPosition, bool aAbort )
{
    wxCHECK_RET( ( m_Flags & ( IS_NEW | IS_MOVED ) ) != 0,
                   wxT( "Bad call to EndEdit().  Text is not being edited." ) );

    m_Flags = 0;
    m_rotate = false;
    m_updateText = false;
    SetEraseLastDrawItem( false );
}


void LIB_FIELD::calcEdit( const wxPoint& aPosition )
{
    if( m_rotate )
    {
        SetTextAngle( GetTextAngle() == TEXT_ANGLE_VERT ? TEXT_ANGLE_HORIZ : TEXT_ANGLE_VERT );
        m_rotate = false;
    }

    if( m_updateText )
    {
        std::swap( m_Text, m_savedText );
        m_updateText = false;
    }

    if( m_Flags == IS_NEW )
    {
        SetTextPos( aPosition );
    }
    else if( m_Flags == IS_MOVED )
    {
        Move( m_initialPos + aPosition - m_initialCursorPos );
    }
}


void LIB_FIELD::GetMsgPanelInfo( MSG_PANEL_ITEMS& aList )
{
    wxString msg;

    LIB_ITEM::GetMsgPanelInfo( aList );

    // Display style:
    msg = GetTextStyleName();
    aList.push_back( MSG_PANEL_ITEM( _( "Style" ), msg, MAGENTA ) );

    msg = StringFromValue( g_UserUnit, GetTextWidth(), true );
    aList.push_back( MSG_PANEL_ITEM( _( "Width" ), msg, BLUE ) );

    msg = StringFromValue( g_UserUnit, GetTextHeight(), true );
    aList.push_back( MSG_PANEL_ITEM( _( "Height" ), msg, BLUE ) );

    // Display field name (ref, value ...)
    msg = GetName();
    aList.push_back( MSG_PANEL_ITEM( _( "Field" ), msg, BROWN ) );

    // Display field text:
    aList.push_back( MSG_PANEL_ITEM( _( "Value" ), GetShownText(), BROWN ) );
}


BITMAP_DEF LIB_FIELD::GetMenuImage() const
{
    return move_field_xpm;
}
