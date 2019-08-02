/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2018 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 2004-2019 KiCad Developers, see AUTHORS.txt for contributors.
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

#include <fctsys.h>
#include <pgm_base.h>
#include <gr_basic.h>
#include <base_struct.h>
#include <gr_text.h>
#include <kicad_string.h>
#include <sch_draw_panel.h>
#include <plotter.h>
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


LIB_FIELD::LIB_FIELD( int aID, wxString& aName ) :
    LIB_ITEM( LIB_FIELD_T, NULL )
{
    Init( aID );
    m_name = aName;
}


LIB_FIELD::~LIB_FIELD()
{
}


LIB_FIELD& LIB_FIELD::operator=( const LIB_FIELD& field )
{
    m_id = field.m_id;
    m_name = field.m_name;
    m_Parent = field.m_Parent;

    SetText( field.GetText() );
    SetEffects( field );

    return *this;
}


void LIB_FIELD::Init( int id )
{
    m_id = id;

    SetTextWidth( GetDefaultTextSize() );
    SetTextHeight( GetDefaultTextSize() );

    SetTextAngle( TEXT_ANGLE_HORIZ );    // constructor already did this.

    // Fields in RAM must always have names, because we are trying to get less dependent on
    // field ids and more dependent on names. Plus assumptions are made in the field editors.
    m_name = TEMPLATE_FIELDNAME::GetDefaultFieldName( id );

    // By contrast, VALUE and REFERENCE are are always constructed as initially visible, and
    // template fieldsnames' initial visibility is controlled by the template fieldname config.
    if( id == DATASHEET || id == FOOTPRINT )
        SetVisible( false );

}


int LIB_FIELD::GetPenSize() const
{
    int pensize = GetThickness();

    if( pensize == 0 )   // Use default values for pen size
    {
        if( IsBold() )
            pensize = GetPenSizeForBold( GetTextWidth() );
        else
            pensize = GetDefaultLineThickness();
    }

    // Clip pen size for small texts:
    pensize = Clamp_Text_PenSize( pensize, GetTextSize(), IsBold() );
    return pensize;
}


void LIB_FIELD::print( wxDC* aDC, const wxPoint& aOffset, void* aData,
                       const TRANSFORM& aTransform )
{
    COLOR4D  color = IsVisible() ? GetDefaultColor() : GetInvisibleItemColor();
    int      linewidth = GetPenSize();
    wxPoint  text_pos = aTransform.TransformCoordinate( GetTextPos() ) + aOffset;
    wxString text = aData ? *static_cast<wxString*>( aData ) : GetText();

    GRText( aDC, text_pos, color, text, GetTextAngle(), GetTextSize(), GetHorizJustify(),
            GetVertJustify(), linewidth, IsItalic(), IsBold() );
}


bool LIB_FIELD::HitTest( const wxPoint& aPosition, int aAccuracy ) const
{
    // Because HitTest is mainly used to select the field return false if it is empty
    if( GetText().IsEmpty() )
        return false;

    // Build a temporary copy of the text for hit testing
    EDA_TEXT tmp_text( *this );

    // Reference designator text has one or 2 additional character (displays U? or U?A)
    if( m_id == REFERENCE )
    {
        wxString extended_text = tmp_text.GetText();
        extended_text.Append('?');
        const LIB_PART*      parent = static_cast<const LIB_PART*>( m_Parent );

        if ( parent && ( parent->GetUnitCount() > 1 ) )
            extended_text.Append('A');
        tmp_text.SetText( extended_text );
    }

    tmp_text.SetTextPos( DefaultTransform.TransformCoordinate( GetTextPos() ) );

    // The text orientation may need to be flipped if the transformation matrix causes xy axes
    // to be flipped.  This simple algo works only for schematic matrix (rot 90 or/and mirror)
    bool t1 = ( DefaultTransform.x1 != 0 ) ^ ( GetTextAngle() != 0 );
    tmp_text.SetTextAngle( t1 ? TEXT_ANGLE_HORIZ : TEXT_ANGLE_VERT );

    return tmp_text.TextHitTest( aPosition, aAccuracy );
}


EDA_ITEM* LIB_FIELD::Clone() const
{
    LIB_FIELD* newfield = new LIB_FIELD( m_id );

    Copy( newfield );

    return (EDA_ITEM*) newfield;
}


void LIB_FIELD::Copy( LIB_FIELD* aTarget ) const
{
    aTarget->m_name = m_name;

    aTarget->SetText( GetText() );
    aTarget->SetEffects( *this );
    aTarget->SetParent( m_Parent );
}


int LIB_FIELD::compare( const LIB_ITEM& other ) const
{
    wxASSERT( other.Type() == LIB_FIELD_T );

    const LIB_FIELD* tmp = ( LIB_FIELD* ) &other;

    if( m_id != tmp->m_id )
        return m_id - tmp->m_id;

    int result = GetText().CmpNoCase( tmp->GetText() );

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


void LIB_FIELD::Offset( const wxPoint& aOffset )
{
    EDA_TEXT::Offset( aOffset );
}


bool LIB_FIELD::Inside( EDA_RECT& rect ) const
{
    return rect.Intersects( GetBoundingBox() );
}


void LIB_FIELD::MoveTo( const wxPoint& newPosition )
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
    if( GetText().IsEmpty() )
        return;

    // Calculate the text orientation, according to the component orientation/mirror
    int orient = (int) GetTextAngle();

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
    wxPoint textpos = aTransform.TransformCoordinate( BoundaryBox.Centre() ) + aOffset;

    aPlotter->Text( textpos, GetDefaultColor(), GetShownText(), orient, GetTextSize(),
                    hjustify, vjustify, GetPenSize(), IsItalic(), IsBold() );
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


void LIB_FIELD::ViewGetLayers( int aLayers[], int& aCount ) const
{
    aCount      = 2;

    switch( m_id )
    {
    case REFERENCE: aLayers[0] = LAYER_REFERENCEPART; break;
    case VALUE:     aLayers[0] = LAYER_VALUEPART;     break;
    default:        aLayers[0] = LAYER_FIELDS;        break;
    }

    aLayers[1] = LAYER_SELECTION_SHADOWS;
}


COLOR4D LIB_FIELD::GetDefaultColor()
{
    switch( m_id )
    {
    case REFERENCE: return GetLayerColor( LAYER_REFERENCEPART );
    case VALUE:     return GetLayerColor( LAYER_VALUEPART );
    default:        return GetLayerColor( LAYER_FIELDS );
    }
}


wxString LIB_FIELD::GetName( bool aTranslate ) const
{
    wxString name;

    switch( m_id )
    {
    case REFERENCE: return aTranslate ? _( "Reference" ) : wxT( "Reference" );
    case VALUE:     return aTranslate ? _( "Value" ) : wxT( "Value" );
    case FOOTPRINT: return aTranslate ? _( "Footprint" ) : wxT( "Footprint" );
    case DATASHEET: return aTranslate ? _( "Datasheet" ) : wxT( "Datasheet" );

    default:
        if( m_name.IsEmpty() )
        {
            return aTranslate ? wxString::Format( _( "Field%d" ), m_id )
                              : wxString::Format( wxT( "Field%d" ), m_id );
        }
        else
        {
            return m_name;
        }
    }
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


wxString LIB_FIELD::GetSelectMenuText( EDA_UNITS_T aUnits ) const
{
    return wxString::Format( _( "Field %s \"%s\"" ), GetName(), ShortenedShownText() );
}


void LIB_FIELD::BeginEdit( const wxPoint aPosition )
{
    SetTextPos( aPosition );
}


void LIB_FIELD::CalcEdit( const wxPoint& aPosition )
{
    SetTextPos( aPosition );
}


void LIB_FIELD::GetMsgPanelInfo( EDA_UNITS_T aUnits, MSG_PANEL_ITEMS& aList )
{
    wxString msg;

    LIB_ITEM::GetMsgPanelInfo( aUnits, aList );

    // Display style:
    msg = GetTextStyleName();
    aList.push_back( MSG_PANEL_ITEM( _( "Style" ), msg, MAGENTA ) );

    msg = MessageTextFromValue( aUnits, GetTextWidth(), true );
    aList.push_back( MSG_PANEL_ITEM( _( "Width" ), msg, BLUE ) );

    msg = MessageTextFromValue( aUnits, GetTextHeight(), true );
    aList.push_back( MSG_PANEL_ITEM( _( "Height" ), msg, BLUE ) );

    // Display field name (ref, value ...)
    aList.push_back( MSG_PANEL_ITEM( _( "Field" ), GetName(), BROWN ) );

    // Display field text:
    aList.push_back( MSG_PANEL_ITEM( _( "Value" ), GetShownText(), BROWN ) );
}


BITMAP_DEF LIB_FIELD::GetMenuImage() const
{
    return move_xpm;
}
