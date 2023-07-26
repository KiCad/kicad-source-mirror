/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2018 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 2022 CERN
 * Copyright (C) 2004-2023 KiCad Developers, see AUTHORS.txt for contributors.
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
#include <string_utils.h>
#include <sch_draw_panel.h>
#include <eda_draw_frame.h>
#include <plotters/plotter.h>
#include <trigo.h>
#include <base_units.h>
#include <widgets/msgpanel.h>
#include <bitmaps.h>
#include <lib_symbol.h>
#include <transform.h>
#include <template_fieldnames.h>
#include <settings/color_settings.h>


LIB_FIELD::LIB_FIELD( LIB_SYMBOL* aParent, int aId, const wxString& aName ) :
    LIB_ITEM( LIB_FIELD_T, aParent ),
    EDA_TEXT( schIUScale )
{
    Init( aId );
    m_name = aName;
}


LIB_FIELD::LIB_FIELD( int aId ) :
    LIB_ITEM( LIB_FIELD_T, nullptr ),
    EDA_TEXT( schIUScale )
{
    Init( aId );
}


LIB_FIELD::LIB_FIELD( int aId, const wxString& aName ) :
    LIB_ITEM( LIB_FIELD_T, nullptr ),
    EDA_TEXT( schIUScale )
{
    Init( aId );
    m_name = aName;
}


LIB_FIELD& LIB_FIELD::operator=( const LIB_FIELD& field )
{
    m_id             = field.m_id;
    m_name           = field.m_name;
    m_parent         = field.m_parent;
    m_autoAdded      = field.m_autoAdded;
    m_showName       = field.m_showName;
    m_allowAutoPlace = field.m_allowAutoPlace;
    m_showInChooser  = field.m_showInChooser;

    SetText( field.GetText() );
    SetAttributes( field );

    return *this;
}


void LIB_FIELD::Init( int aId )
{
    m_id = aId;

    SetTextAngle( ANGLE_HORIZONTAL );    // constructor already did this.

    // Fields in RAM must always have names, because we are trying to get less dependent on
    // field ids and more dependent on names. Plus assumptions are made in the field editors.
    m_name = TEMPLATE_FIELDNAME::GetDefaultFieldName( aId );

    // By contrast, VALUE and REFERENCE are are always constructed as initially visible, and
    // template fieldsnames' initial visibility is controlled by the template fieldname config.
    if( aId != VALUE_FIELD && aId != REFERENCE_FIELD && aId < MANDATORY_FIELDS )
        SetVisible( false );

    m_autoAdded      = false;
    m_showName       = false;
    m_allowAutoPlace = true;
    m_showInChooser  = true;
}


void LIB_FIELD::SetId( int aId )
{
    m_id = aId;
}


int LIB_FIELD::GetPenWidth() const
{
    return GetEffectiveTextPenWidth();
}


KIFONT::FONT* LIB_FIELD::getDrawFont() const
{
    KIFONT::FONT* font = EDA_TEXT::GetFont();

    if( !font )
        font = KIFONT::FONT::GetFont( GetDefaultFont(), IsBold(), IsItalic() );

    return font;
}


void LIB_FIELD::print( const RENDER_SETTINGS* aSettings, const VECTOR2I& aOffset, void* aData,
                       const TRANSFORM& aTransform, bool aDimmed )
{
    wxDC*    DC = aSettings->GetPrintDC();
    COLOR4D  color = aSettings->GetLayerColor( IsVisible() ? GetDefaultLayer() : LAYER_HIDDEN );
    COLOR4D  bg = aSettings->GetBackgroundColor();
    bool     blackAndWhiteMode = GetGRForceBlackPenState();
    int      penWidth = GetEffectivePenWidth( aSettings );
    VECTOR2I text_pos = aTransform.TransformCoordinate( GetTextPos() ) + aOffset;
    wxString text = aData ? *static_cast<wxString*>( aData ) : GetText();

    if( blackAndWhiteMode || bg == COLOR4D::UNSPECIFIED )
        bg = COLOR4D::WHITE;

    if( !blackAndWhiteMode && GetTextColor() != COLOR4D::UNSPECIFIED )
        color = GetTextColor();

    if( aDimmed )
    {
        color.Desaturate( );
        color = color.Mix( bg, 0.5f );
    }

    KIFONT::FONT* font = GetFont();

    if( !font )
        font = KIFONT::FONT::GetFont( aSettings->GetDefaultFont(), IsBold(), IsItalic() );

    GRPrintText( DC, text_pos, color, text, GetTextAngle(), GetTextSize(), GetHorizJustify(),
                 GetVertJustify(), penWidth, IsItalic(), IsBold(), font );
}


bool LIB_FIELD::HitTest( const VECTOR2I& aPosition, int aAccuracy ) const
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
    bool t1 = ( DefaultTransform.x1 != 0 ) ^ ( GetTextAngle() != ANGLE_HORIZONTAL );
    tmp_text.SetTextAngle( t1 ? ANGLE_HORIZONTAL : ANGLE_VERTICAL );

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
    aTarget->m_name           = m_name;
    aTarget->m_showName       = m_showName;
    aTarget->m_allowAutoPlace = m_allowAutoPlace;
    aTarget->m_showInChooser  = m_showInChooser;

    aTarget->CopyText( *this );
    aTarget->SetAttributes( *this );
    aTarget->SetParent( m_parent );
    aTarget->SetAutoAdded( IsAutoAdded() );
}


int LIB_FIELD::compare( const LIB_ITEM& aOther, int aCompareFlags ) const
{
    wxASSERT( aOther.Type() == LIB_FIELD_T );

    int retv = LIB_ITEM::compare( aOther, aCompareFlags );

    if( retv )
        return retv;

    const LIB_FIELD* tmp = ( LIB_FIELD* ) &aOther;

    // Equality test will vary depending whether or not the field is mandatory.  Otherwise,
    // sorting is done by ordinal.
    if( aCompareFlags & LIB_ITEM::COMPARE_FLAGS::EQUALITY )
    {
        // Mandatory fields have fixed ordinals and their names can vary due to translated field
        // names.  Optional fields have fixed names and their ordinals can vary.
        if( IsMandatory() )
        {
            if( m_id != tmp->m_id )
                return m_id - tmp->m_id;
        }
        else
        {
            retv = m_name.Cmp( tmp->m_name );

            if( retv )
                return retv;
        }
    }
    else    // assume we're sorting
    {
        if( m_id != tmp->m_id )
            return m_id - tmp->m_id;
    }

    bool ignoreFieldText = false;

    if( m_id == REFERENCE_FIELD && ( aCompareFlags & COMPARE_FLAGS::EQUALITY ) )
        ignoreFieldText = true;

    if( m_id == VALUE_FIELD && ( aCompareFlags & COMPARE_FLAGS::ERC ) )
        ignoreFieldText = true;

    if( !ignoreFieldText )
    {
        retv = GetText().CmpNoCase( tmp->GetText() );

        if( retv != 0 )
            return retv;
    }

    if( aCompareFlags & LIB_ITEM::COMPARE_FLAGS::EQUALITY )
    {
        if( GetTextPos().x != tmp->GetTextPos().x )
            return GetTextPos().x - tmp->GetTextPos().x;

        if( GetTextPos().y != tmp->GetTextPos().y )
            return GetTextPos().y - tmp->GetTextPos().y;
    }

    if( GetTextWidth() != tmp->GetTextWidth() )
        return GetTextWidth() - tmp->GetTextWidth();

    if( GetTextHeight() != tmp->GetTextHeight() )
        return GetTextHeight() - tmp->GetTextHeight();

    return 0;
}


void LIB_FIELD::Offset( const VECTOR2I& aOffset )
{
    EDA_TEXT::Offset( aOffset );
}


void LIB_FIELD::MoveTo( const VECTOR2I& newPosition )
{
    EDA_TEXT::SetTextPos( newPosition );
}


void LIB_FIELD::MirrorHorizontal( const VECTOR2I& center )
{
    int x = GetTextPos().x;

    x -= center.x;
    x *= -1;
    x += center.x;

    SetTextX( x );
}


void LIB_FIELD::MirrorVertical( const VECTOR2I& center )
{
    int y = GetTextPos().y;

    y -= center.y;
    y *= -1;
    y += center.y;

    SetTextY( y );
}


void LIB_FIELD::Rotate( const VECTOR2I& center, bool aRotateCCW )
{
    EDA_ANGLE rot_angle = aRotateCCW ? -ANGLE_90 : ANGLE_90;

    VECTOR2I pt = GetTextPos();
    RotatePoint( pt, center, rot_angle );
    SetTextPos( pt );

    SetTextAngle( GetTextAngle() != ANGLE_HORIZONTAL ? ANGLE_HORIZONTAL  : ANGLE_VERTICAL );
}


void LIB_FIELD::Plot( PLOTTER* aPlotter, bool aBackground, const VECTOR2I& aOffset,
                      const TRANSFORM& aTransform, bool aDimmed ) const
{
    if( GetText().IsEmpty() || aBackground )
        return;

    RENDER_SETTINGS* renderSettings = aPlotter->RenderSettings();

    // Calculate the text orientation, according to the symbol orientation/mirror.
    EDA_ANGLE orient = GetTextAngle();

    if( aTransform.y1 )  // Rotate symbol 90 deg.
    {
        if( orient.IsHorizontal() )
            orient = ANGLE_VERTICAL;
        else
            orient = ANGLE_HORIZONTAL;
    }

    BOX2I bbox = GetBoundingBox();
    bbox.RevertYAxis();

    GR_TEXT_H_ALIGN_T hjustify = GR_TEXT_H_ALIGN_CENTER;
    GR_TEXT_V_ALIGN_T vjustify = GR_TEXT_V_ALIGN_CENTER;
    VECTOR2I          textpos = aTransform.TransformCoordinate( bbox.Centre() ) + aOffset;

    COLOR4D color;
    COLOR4D bg;

    if( aPlotter->GetColorMode() )
    {
        if( GetTextColor() != COLOR4D::UNSPECIFIED )
            color = GetTextColor();
        else
            color = renderSettings->GetLayerColor( GetDefaultLayer() );

        bg = renderSettings->GetBackgroundColor();

        if( bg == COLOR4D::UNSPECIFIED )
            bg = COLOR4D::WHITE;
    }
    else
    {
        color = COLOR4D::BLACK;
        bg = COLOR4D::WHITE;
    }

    if( aDimmed )
    {
        color.Desaturate( );
        color = color.Mix( bg, 0.5f );
    }

    int           penWidth = GetEffectivePenWidth( renderSettings );
    KIFONT::FONT* font = GetFont();

    if( !font )
        font = KIFONT::FONT::GetFont( renderSettings->GetDefaultFont(), IsBold(), IsItalic() );

    TEXT_ATTRIBUTES attrs = GetAttributes();
    attrs.m_StrokeWidth = penWidth;
    attrs.m_Halign = hjustify;
    attrs.m_Valign = vjustify;
    attrs.m_Angle = orient;
    attrs.m_Multiline = false;

    aPlotter->PlotText( textpos, color, GetShownText( true ), attrs, font );
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


wxString LIB_FIELD::GetShownText( bool aAllowExtraText, int aDepth ) const
{
    wxString text = EDA_TEXT::GetShownText( aAllowExtraText, aDepth );

    if( IsNameShown() )
        text = GetName() << wxT( ": " ) << text;

    return text;
}


const BOX2I LIB_FIELD::GetBoundingBox() const
{
    /* Y coordinates for LIB_ITEMS are bottom to top, so we must invert the Y position when
     * calling GetTextBox() that works using top to bottom Y axis orientation.
     */
    BOX2I bbox = GetTextBox( -1, true );
    bbox.RevertYAxis();

    // We are using now a bottom to top Y axis.
    VECTOR2I orig = bbox.GetOrigin();
    VECTOR2I end = bbox.GetEnd();

    RotatePoint( orig, GetTextPos(), -GetTextAngle() );
    RotatePoint( end, GetTextPos(), -GetTextAngle() );

    bbox.SetOrigin( orig );
    bbox.SetEnd( end );

    // We are using now a top to bottom Y axis:
    bbox.RevertYAxis();

    return bbox;
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
    if( m_id < MANDATORY_FIELDS )
        return TEMPLATE_FIELDNAME::GetDefaultFieldName( m_id );

    return m_name;
}


void LIB_FIELD::SetName( const wxString& aName )
{
    // Mandatory field names are fixed.
    if( IsMandatory() )
    {
        wxFAIL_MSG( wxS( "trying to set a MANDATORY_FIELD's name\n" ) );
        return;
    }

    m_name = aName;
}


wxString LIB_FIELD::GetItemDescription( UNITS_PROVIDER* aUnitsProvider ) const
{
    return wxString::Format( "%s '%s'",
                             UnescapeString( GetName() ),
                             KIUI::EllipsizeMenuText( GetText() ) );
}


void LIB_FIELD::BeginEdit( const VECTOR2I& aPosition )
{
    SetTextPos( aPosition );
}


void LIB_FIELD::CalcEdit( const VECTOR2I& aPosition )
{
    SetTextPos( aPosition );
}


void LIB_FIELD::GetMsgPanelInfo( EDA_DRAW_FRAME* aFrame, std::vector<MSG_PANEL_ITEM>& aList )
{
    wxString msg;

    LIB_ITEM::GetMsgPanelInfo( aFrame, aList );

    // Don't use GetShownText(); we want to see the variable references here
    aList.emplace_back( _( "Field" ), UnescapeString( GetName() ) );

    // Don't use GetShownText() here; we want to show the user the variable references
    aList.emplace_back( _( "Text" ), KIUI::EllipsizeStatusText( aFrame, GetText() ) );

    aList.emplace_back( _( "Visible" ), IsVisible() ? _( "Yes" ) : _( "No" ) );

    aList.emplace_back( _( "Font" ), GetFont() ? GetFont()->GetName() : _( "Default" ) );

    aList.emplace_back( _( "Style" ), GetTextStyleName() );

    aList.emplace_back( _( "Text Size" ), aFrame->MessageTextFromValue( GetTextWidth() ) );

    switch ( GetHorizJustify() )
    {
    case GR_TEXT_H_ALIGN_LEFT:   msg = _( "Left" );   break;
    case GR_TEXT_H_ALIGN_CENTER: msg = _( "Center" ); break;
    case GR_TEXT_H_ALIGN_RIGHT:  msg = _( "Right" );  break;
    }

    aList.emplace_back( _( "H Justification" ), msg );

    switch ( GetVertJustify() )
    {
    case GR_TEXT_V_ALIGN_TOP:    msg = _( "Top" );    break;
    case GR_TEXT_V_ALIGN_CENTER: msg = _( "Center" ); break;
    case GR_TEXT_V_ALIGN_BOTTOM: msg = _( "Bottom" ); break;
    }

    aList.emplace_back( _( "V Justification" ), msg );
}


BITMAPS LIB_FIELD::GetMenuImage() const
{
    return BITMAPS::move;
}


bool LIB_FIELD::IsMandatory() const
{
    return m_id >= 0 && m_id < MANDATORY_FIELDS;
}


static struct LIB_FIELD_DESC
{
    LIB_FIELD_DESC()
    {
        PROPERTY_MANAGER& propMgr = PROPERTY_MANAGER::Instance();
        REGISTER_TYPE( LIB_FIELD );
        propMgr.AddTypeCast( new TYPE_CAST<LIB_FIELD, LIB_ITEM> );
        propMgr.AddTypeCast( new TYPE_CAST<LIB_FIELD, EDA_TEXT> );
        propMgr.InheritsAfter( TYPE_HASH( LIB_FIELD ), TYPE_HASH( LIB_ITEM ) );
        propMgr.InheritsAfter( TYPE_HASH( LIB_FIELD ), TYPE_HASH( EDA_TEXT ) );

        propMgr.AddProperty( new PROPERTY<LIB_FIELD, bool>( _HKI( "Show Field Name" ),
                &LIB_FIELD::SetNameShown, &LIB_FIELD::IsNameShown ) );

        propMgr.AddProperty( new PROPERTY<LIB_FIELD, bool>( _HKI( "Allow Autoplacement" ),
                &LIB_FIELD::SetCanAutoplace, &LIB_FIELD::CanAutoplace ) );

        propMgr.Mask( TYPE_HASH( LIB_FIELD ), TYPE_HASH( EDA_TEXT ), _HKI( "Hyperlink" ) );
        propMgr.Mask( TYPE_HASH( LIB_FIELD ), TYPE_HASH( EDA_TEXT ), _HKI( "Thickness" ) );
        propMgr.Mask( TYPE_HASH( LIB_FIELD ), TYPE_HASH( EDA_TEXT ), _HKI( "Mirrored" ) );
        propMgr.Mask( TYPE_HASH( LIB_FIELD ), TYPE_HASH( EDA_TEXT ), _HKI( "Visible" ) );
        propMgr.Mask( TYPE_HASH( LIB_FIELD ), TYPE_HASH( EDA_TEXT ), _HKI( "Width" ) );
        propMgr.Mask( TYPE_HASH( LIB_FIELD ), TYPE_HASH( EDA_TEXT ), _HKI( "Height" ) );

        propMgr.Mask( TYPE_HASH( LIB_FIELD ), TYPE_HASH( EDA_TEXT ), _HKI( "Orientation" ) );
    }
} _LIB_FIELD_DESC;
