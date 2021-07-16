/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2018 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 2004-2021 KiCad Developers, see AUTHORS.txt for contributors.
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

#include <eda_item.h>
#include <gr_text.h>
#include <kicad_string.h>
#include <sch_draw_panel.h>
#include <eda_draw_frame.h>
#include <plotter.h>
#include <trigo.h>
#include <base_units.h>
#include <widgets/msgpanel.h>
#include <bitmaps.h>
#include <general.h>
#include <lib_symbol.h>
#include <transform.h>
#include <lib_field.h>
#include <template_fieldnames.h>
#include <settings/color_settings.h>


LIB_FIELD::LIB_FIELD( LIB_SYMBOL* aParent, int idfield ) :
    LIB_ITEM( LIB_FIELD_T, aParent )
{
    Init( idfield );
}


LIB_FIELD::LIB_FIELD( int idfield ) :
    LIB_ITEM( LIB_FIELD_T, nullptr )
{
    Init( idfield );
}


LIB_FIELD::LIB_FIELD( int aID, const wxString& aName ) :
    LIB_ITEM( LIB_FIELD_T, nullptr )
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
    m_parent = field.m_parent;

    SetText( field.GetText() );
    SetEffects( field );

    return *this;
}


void LIB_FIELD::Init( int id )
{
    m_id = id;

    SetTextAngle( TEXT_ANGLE_HORIZ );    // constructor already did this.

    // Fields in RAM must always have names, because we are trying to get less dependent on
    // field ids and more dependent on names. Plus assumptions are made in the field editors.
    m_name = TEMPLATE_FIELDNAME::GetDefaultFieldName( id );

    // By contrast, VALUE and REFERENCE are are always constructed as initially visible, and
    // template fieldsnames' initial visibility is controlled by the template fieldname config.
    if( id == DATASHEET_FIELD || id == FOOTPRINT_FIELD )
        SetVisible( false );
}


int LIB_FIELD::GetPenWidth() const
{
    return GetEffectiveTextPenWidth();
}


void LIB_FIELD::print( const RENDER_SETTINGS* aSettings, const wxPoint& aOffset, void* aData,
                       const TRANSFORM& aTransform )
{
    wxDC*    DC = aSettings->GetPrintDC();
    COLOR4D  color = aSettings->GetLayerColor( IsVisible() ? GetDefaultLayer() : LAYER_HIDDEN );
    int      penWidth = std::max( GetPenWidth(), aSettings->GetDefaultPenWidth() );
    wxPoint  text_pos = aTransform.TransformCoordinate( GetTextPos() ) + aOffset;
    wxString text = aData ? *static_cast<wxString*>( aData ) : GetText();

    GRText( DC, text_pos, color, text, GetTextAngle(), GetTextSize(), GetHorizJustify(),
            GetVertJustify(), penWidth, IsItalic(), IsBold() );
}


bool LIB_FIELD::HitTest( const wxPoint& aPosition, int aAccuracy ) const
{
    // Because HitTest is mainly used to select the field return false if it is empty
    if( GetText().IsEmpty() )
        return false;

    // Build a temporary copy of the text for hit testing
    EDA_TEXT tmp_text( *this );

    // Reference designator text has one or 2 additional character (displays U? or U?A)
    if( m_id == REFERENCE_FIELD )
    {
        const LIB_SYMBOL* parent = dynamic_cast<const LIB_SYMBOL*>( m_parent );

        wxString extended_text = tmp_text.GetText();
        extended_text.Append('?');

        if ( parent && parent->GetUnitCount() > 1 )
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

    aTarget->CopyText( *this );
    aTarget->SetEffects( *this );
    aTarget->SetParent( m_parent );
}


int LIB_FIELD::compare( const LIB_ITEM& aOther, LIB_ITEM::COMPARE_FLAGS aCompareFlags ) const
{
    wxASSERT( aOther.Type() == LIB_FIELD_T );

    int retv = LIB_ITEM::compare( aOther, aCompareFlags );

    if( retv )
        return retv;

    const LIB_FIELD* tmp = ( LIB_FIELD* ) &aOther;

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
                      const TRANSFORM& aTransform ) const
{
    if( GetText().IsEmpty() )
        return;

    // Calculate the text orientation, according to the symbol orientation/mirror.
    int orient = (int) GetTextAngle();

    if( aTransform.y1 )  // Rotate symbol 90 deg.
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

    COLOR4D color;

    if( aPlotter->GetColorMode() )
        color = aPlotter->RenderSettings()->GetLayerColor( GetDefaultLayer() );
    else
        color = COLOR4D::BLACK;

    int penWidth = std::max( GetPenWidth(),aPlotter->RenderSettings()->GetMinPenWidth() );

    aPlotter->Text( textpos, color, GetShownText(), orient, GetTextSize(), hjustify, vjustify,
                    penWidth, IsItalic(), IsBold() );
}


wxString LIB_FIELD::GetFullText( int unit ) const
{
    if( m_id != REFERENCE_FIELD )
        return GetText();

    wxString text = GetText();
    text << wxT( "?" );

    wxCHECK( GetParent(), text );

    if( GetParent()->IsMulti() )
        text << LIB_SYMBOL::SubReference( unit );

    return text;
}


const EDA_RECT LIB_FIELD::GetBoundingBox() const
{
    /* Y coordinates for LIB_ITEMS are bottom to top, so we must invert the Y position when
     * calling GetTextBox() that works using top to bottom Y axis orientation.
     */
    EDA_RECT rect = GetTextBox( -1, true );
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
    case REFERENCE_FIELD: aLayers[0] = LAYER_REFERENCEPART; break;
    case VALUE_FIELD:     aLayers[0] = LAYER_VALUEPART;     break;
    default:              aLayers[0] = LAYER_FIELDS;        break;
    }

    aLayers[1] = LAYER_SELECTION_SHADOWS;
}


SCH_LAYER_ID LIB_FIELD::GetDefaultLayer() const
{
    switch( m_id )
    {
    case REFERENCE_FIELD: return LAYER_REFERENCEPART;
    case VALUE_FIELD:     return LAYER_VALUEPART;
    default:              return LAYER_FIELDS;
    }
}


wxString LIB_FIELD::GetName( bool aUseDefaultName ) const
{
    if( m_name.IsEmpty() && aUseDefaultName )
        return TEMPLATE_FIELDNAME::GetDefaultFieldName( m_id );

    return m_name;
}


wxString LIB_FIELD::GetCanonicalName() const
{
    switch( m_id )
    {
    case  REFERENCE_FIELD: return wxT( "Reference" );
    case  VALUE_FIELD:     return wxT( "Value" );
    case  FOOTPRINT_FIELD: return wxT( "Footprint" );
    case  DATASHEET_FIELD: return wxT( "Datasheet" );
    }

    return m_name;
}


void LIB_FIELD::SetName( const wxString& aName )
{
    // Mandatory field names are fixed.
    if( IsMandatory() )
    {
        wxFAIL_MSG( "trying to set a MANDATORY_FIELD's name\n" );
        return;
    }

    if( m_name != aName )
    {
        m_name = aName;
        SetModified();
    }
}


wxString LIB_FIELD::GetSelectMenuText( EDA_UNITS aUnits ) const
{
    return wxString::Format( "%s '%s'", GetName(), ShortenedShownText() );
}


void LIB_FIELD::BeginEdit( const wxPoint& aPosition )
{
    SetTextPos( aPosition );
}


void LIB_FIELD::CalcEdit( const wxPoint& aPosition )
{
    SetTextPos( aPosition );
}


void LIB_FIELD::GetMsgPanelInfo( EDA_DRAW_FRAME* aFrame, MSG_PANEL_ITEMS& aList )
{
    wxString msg;

    LIB_ITEM::GetMsgPanelInfo( aFrame, aList );

    // Display style:
    msg = GetTextStyleName();
    aList.push_back( MSG_PANEL_ITEM( _( "Style" ), msg ) );

    msg = MessageTextFromValue( aFrame->GetUserUnits(), GetTextWidth() );
    aList.push_back( MSG_PANEL_ITEM( _( "Width" ), msg ) );

    msg = MessageTextFromValue( aFrame->GetUserUnits(), GetTextHeight() );
    aList.push_back( MSG_PANEL_ITEM( _( "Height" ), msg ) );

    // Display field name (ref, value ...)
    aList.push_back( MSG_PANEL_ITEM( _( "Field" ), GetName() ) );

    // Display field text:
    aList.push_back( MSG_PANEL_ITEM( _( "Value" ), GetShownText() ) );
}


BITMAPS LIB_FIELD::GetMenuImage() const
{
    return BITMAPS::move;
}


bool LIB_FIELD::IsMandatory() const
{
    return m_id >= 0 && m_id < MANDATORY_FIELDS;
}
