/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2015 Jean-Pierre Charras, jp.charras at wanadoo.fr
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

/* Fields are texts attached to a component, having a special meaning
 * Fields 0 and 1 are very important: reference and value
 * Field 2 is used as default footprint name.
 * Field 3 is reserved (not currently used
 * Fields 4 and more are user fields.
 * They can be renamed and can appear in reports
 */

#include <fctsys.h>
#include <sch_draw_panel.h>
#include <base_struct.h>
#include <gr_basic.h>
#include <gr_text.h>
#include <macros.h>
#include <sch_edit_frame.h>
#include <plotter.h>
#include <bitmaps.h>

#include <general.h>
#include <class_library.h>
#include <sch_component.h>
#include <sch_field.h>
#include <kicad_string.h>
#include <trace_helpers.h>


SCH_FIELD::SCH_FIELD( const wxPoint& aPos, int aFieldId, SCH_COMPONENT* aParent, const wxString& aName ) :
    SCH_ITEM( aParent, SCH_FIELD_T ),
    EDA_TEXT()
{
    SetTextPos( aPos );
    m_id = aFieldId;
    m_name = aName;

    SetVisible( false );
    SetLayer( LAYER_FIELDS );
}


SCH_FIELD::~SCH_FIELD()
{
}


EDA_ITEM* SCH_FIELD::Clone() const
{
    return new SCH_FIELD( *this );
}


const wxString SCH_FIELD::GetFullyQualifiedText() const
{
    wxString text = GetText();

    /* For more than one part per package, we must add the part selection
     * A, B, ... or 1, 2, .. to the reference. */
    if( m_id == REFERENCE )
    {
        SCH_COMPONENT* component = (SCH_COMPONENT*) m_Parent;

        wxCHECK_MSG( component != NULL, text,
                     wxT( "No component associated with field" ) + text );

        if( component->GetUnitCount() > 1 )
            text << LIB_PART::SubReference( component->GetUnit() );
    }

    return text;
}


int SCH_FIELD::GetPenSize() const
{
    int pensize = GetThickness();

    if( pensize == 0 )   // Use default values for pen size
    {
        if( IsBold()  )
            pensize = GetPenSizeForBold( GetTextWidth() );
        else
            pensize = GetDefaultLineThickness();
    }

    // Clip pen size for small texts:
    pensize = Clamp_Text_PenSize( pensize, GetTextSize(), IsBold() );
    return pensize;
}


void SCH_FIELD::Print( wxDC* aDC, const wxPoint& aOffset )
{
    int            orient;
    COLOR4D        color;
    wxPoint        textpos;
    SCH_COMPONENT* parentComponent = (SCH_COMPONENT*) m_Parent;
    int            lineWidth = GetThickness();

    if( lineWidth == 0 )   // Use default values for pen size
    {
        if( IsBold() )
            lineWidth = GetPenSizeForBold( GetTextWidth() );
        else
            lineWidth = GetDefaultLineThickness();
    }

    // Clip pen size for small texts:
    lineWidth = Clamp_Text_PenSize( lineWidth, GetTextSize(), IsBold() );

    if( ( !IsVisible() && !m_forceVisible) || IsVoid() )
        return;

    // Calculate the text orientation according to the component orientation.
    orient = GetTextAngle();

    if( parentComponent->GetTransform().y1 )  // Rotate component 90 degrees.
    {
        if( orient == TEXT_ANGLE_HORIZ )
            orient = TEXT_ANGLE_VERT;
        else
            orient = TEXT_ANGLE_HORIZ;
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
    EDA_RECT boundaryBox = GetBoundingBox();
    textpos = boundaryBox.Centre() + aOffset;

    if( m_forceVisible )
        color = COLOR4D( DARKGRAY );
    else if( m_id == REFERENCE )
        color = GetLayerColor( LAYER_REFERENCEPART );
    else if( m_id == VALUE )
        color = GetLayerColor( LAYER_VALUEPART );
    else
        color = GetLayerColor( LAYER_FIELDS );

    GRText( aDC, textpos, color, GetFullyQualifiedText(), orient, GetTextSize(),
            GR_TEXT_HJUSTIFY_CENTER, GR_TEXT_VJUSTIFY_CENTER, lineWidth, IsItalic(), IsBold() );
}


void SCH_FIELD::ImportValues( const LIB_FIELD& aSource )
{
    SetEffects( aSource );
}


void SCH_FIELD::SwapData( SCH_ITEM* aItem )
{
    wxCHECK_RET( (aItem != NULL) && (aItem->Type() == SCH_FIELD_T),
                 wxT( "Cannot swap field data with invalid item." ) );

    SCH_FIELD* item = (SCH_FIELD*) aItem;

    std::swap( m_Layer, item->m_Layer );
    SwapText( *item );
    SwapEffects( *item );
}


const EDA_RECT SCH_FIELD::GetBoundingBox() const
{
    SCH_COMPONENT* parentComponent = (SCH_COMPONENT*) m_Parent;
    int linewidth = GetThickness() == 0 ? GetDefaultLineThickness() : GetThickness();

    // We must pass the effective text thickness to GetTextBox
    // when calculating the bounding box
    linewidth = Clamp_Text_PenSize( linewidth, GetTextSize(), IsBold() );

    // Calculate the text bounding box:
    EDA_RECT rect;

    if( m_id == REFERENCE )     // multi units have one letter or more added to reference
    {
        SCH_FIELD text( *this );    // Make a local copy to change text
                                    // because GetBoundingBox() is const
        text.SetText( GetFullyQualifiedText() );
        rect = text.GetTextBox( -1, linewidth );
    }
    else
        rect = GetTextBox( -1, linewidth );

    // Calculate the bounding box position relative to the component:
    wxPoint origin = parentComponent->GetPosition();
    wxPoint pos = GetTextPos() - origin;
    wxPoint begin = rect.GetOrigin() - origin;
    wxPoint end = rect.GetEnd() - origin;
    RotatePoint( &begin, pos, GetTextAngle() );
    RotatePoint( &end, pos, GetTextAngle() );

    // Due to the Y axis direction, we must mirror the bounding box,
    // relative to the text position:
    MIRROR( begin.y, pos.y );
    MIRROR( end.y,   pos.y );

    // Now, apply the component transform (mirror/rot)
    begin = parentComponent->GetTransform().TransformCoordinate( begin );
    end = parentComponent->GetTransform().TransformCoordinate( end );
    rect.SetOrigin( begin);
    rect.SetEnd( end);
    rect.Move( origin );
    rect.Normalize();
    return rect;
}


bool SCH_FIELD::IsHorizJustifyFlipped() const
{
    wxPoint render_center = GetBoundingBox().Centre();
    wxPoint pos = GetPosition();

    switch( GetHorizJustify() )
    {
    case GR_TEXT_HJUSTIFY_LEFT:
        return render_center.x < pos.x;
    case GR_TEXT_HJUSTIFY_RIGHT:
        return render_center.x > pos.x;
    default:
        return false;
    }
}


bool SCH_FIELD::IsVoid() const
{
    return GetText().Len() == 0;
}


void SCH_FIELD::Place( SCH_EDIT_FRAME* frame, wxDC* DC )
{
    SCH_COMPONENT* component = (SCH_COMPONENT*) GetParent();

    // save old cmp in undo list
    frame->SaveUndoItemInUndoList( component );

    ClearEditFlags();
    frame->OnModify();
}


bool SCH_FIELD::Matches( wxFindReplaceData& aSearchData, void* aAuxData )
{
    wxString text = GetFullyQualifiedText();
    int      flags = aSearchData.GetFlags();

    // User defined fields have an ID of -1.
    if( m_id != REFERENCE && m_id != VALUE && !( flags & FR_SEARCH_ALL_FIELDS ) )
        return false;

    if( ( flags & FR_SEARCH_REPLACE ) && m_id == REFERENCE && !( flags & FR_REPLACE_REFERENCES ) )
        return false;

    wxLogTrace( traceFindItem, wxT( "    child item " ) + GetSelectMenuText( MILLIMETRES ) );

    // Take sheet path into account which effects the reference field and the unit for
    // components with multiple parts.
    if( m_id == REFERENCE && aAuxData != NULL )
    {
        SCH_COMPONENT* component = (SCH_COMPONENT*) m_Parent;

        wxCHECK_MSG( component != NULL, false, wxT( "No symbol associated with field" ) + text );

        text = component->GetRef( (SCH_SHEET_PATH*) aAuxData );

        if( component->GetUnitCount() > 1 )
            text << LIB_PART::SubReference( component->GetUnit() );
    }

    return SCH_ITEM::Matches( text, aSearchData );
}


bool SCH_FIELD::IsReplaceable() const
{
    if( m_id != VALUE )
        return true;

    SCH_COMPONENT* component = dynamic_cast<SCH_COMPONENT*>( GetParent() );
    LIB_PART*      part = component ? component->GetPartRef().lock().get() : nullptr;
    bool           isPower = part ? part->IsPower() : false;

    return !isPower;
}


bool SCH_FIELD::Replace( wxFindReplaceData& aSearchData, void* aAuxData )
{
    bool isReplaced;
    wxString text = GetFullyQualifiedText();

    if( m_id == REFERENCE )
    {
        wxCHECK_MSG( aAuxData != NULL, false,
                     wxT( "Cannot replace reference designator without valid sheet path." ) );

        wxCHECK_MSG( aSearchData.GetFlags() & FR_REPLACE_REFERENCES, false,
                     wxT( "Invalid replace symbol reference field call." ) ) ;

        SCH_COMPONENT* component = (SCH_COMPONENT*) m_Parent;

        wxCHECK_MSG( component != NULL, false, wxT( "No symbol associated with field" ) + text );

        text = component->GetRef( (SCH_SHEET_PATH*) aAuxData );

        // if( component->GetUnitCount() > 1 )
        //     text << LIB_PART::SubReference( component->GetUnit() );

        isReplaced = EDA_ITEM::Replace( aSearchData, text );

        if( isReplaced )
            component->SetRef( (SCH_SHEET_PATH*) aAuxData, text );
    }
    else
    {
        isReplaced = EDA_TEXT::Replace( aSearchData );
    }

    return isReplaced;
}


void SCH_FIELD::Rotate( wxPoint aPosition )
{
    wxPoint pt = GetTextPos();
    RotatePoint( &pt, aPosition, 900 );
    SetTextPos( pt );
}


wxString SCH_FIELD::GetSelectMenuText( EDA_UNITS_T aUnits ) const
{
    return wxString::Format( _( "Field %s" ), GetName() );
}


wxString SCH_FIELD::GetName( bool aUseDefaultName ) const
{
    if( !m_name.IsEmpty() )
        return m_name;
    else if( aUseDefaultName )
        return TEMPLATE_FIELDNAME::GetDefaultFieldName( m_id );

    return wxEmptyString;
}


BITMAP_DEF SCH_FIELD::GetMenuImage() const
{
    if( m_id == REFERENCE )
        return edit_comp_ref_xpm;

    if( m_id == VALUE )
        return edit_comp_value_xpm;

    if( m_id == FOOTPRINT )
        return edit_comp_footprint_xpm;

    return edit_text_xpm;
}


bool SCH_FIELD::HitTest( const wxPoint& aPosition, int aAccuracy ) const
{
    // Do not hit test hidden or empty fields.
    if( !IsVisible() || IsVoid() )
        return false;

    EDA_RECT rect = GetBoundingBox();

    rect.Inflate( aAccuracy );

    return rect.Contains( aPosition );
}


bool SCH_FIELD::HitTest( const EDA_RECT& aRect, bool aContained, int aAccuracy ) const
{
    // Do not hit test hidden fields.
    if( !IsVisible() || IsVoid() )
        return false;

    EDA_RECT rect = aRect;

    rect.Inflate( aAccuracy );

    if( aContained )
        return rect.Contains( GetBoundingBox() );

    return rect.Intersects( GetBoundingBox() );
}


void SCH_FIELD::Plot( PLOTTER* aPlotter )
{
    SCH_COMPONENT* parent = ( SCH_COMPONENT* ) GetParent();

    wxCHECK_RET( parent != NULL && parent->Type() == SCH_COMPONENT_T,
                 wxT( "Cannot plot field with invalid parent." ) );

    COLOR4D color = GetLayerColor( GetLayer() );

    if( !IsVisible() )
        return;

    if( IsVoid() )
        return;

    /* Calculate the text orientation, according to the component
     * orientation/mirror */
    int orient = GetTextAngle();

    if( parent->GetTransform().y1 )  // Rotate component 90 deg.
    {
        if( orient == TEXT_ANGLE_HORIZ )
            orient = TEXT_ANGLE_VERT;
        else
            orient = TEXT_ANGLE_HORIZ;
    }

    /* Calculate the text justification, according to the component
     * orientation/mirror
     * this is a bit complicated due to cumulative calculations:
     * - numerous cases (mirrored or not, rotation)
     * - the DrawGraphicText function recalculate also H and H justifications
     *      according to the text orientation.
     * - When a component is mirrored, the text is not mirrored and
     *   justifications are complicated to calculate
     * so the more easily way is to use no justifications ( Centered text )
     * and use GetBoundaryBox to know the text coordinate considered as centered
     */
    EDA_RECT BoundaryBox = GetBoundingBox();
    EDA_TEXT_HJUSTIFY_T hjustify = GR_TEXT_HJUSTIFY_CENTER;
    EDA_TEXT_VJUSTIFY_T vjustify = GR_TEXT_VJUSTIFY_CENTER;
    wxPoint  textpos = BoundaryBox.Centre();

    int      thickness = GetPenSize();

    aPlotter->Text( textpos, color, GetFullyQualifiedText(), orient, GetTextSize(),
            hjustify, vjustify,
            thickness, IsItalic(), IsBold() );
}


void SCH_FIELD::SetPosition( const wxPoint& aPosition )
{
    SCH_COMPONENT* component = (SCH_COMPONENT*) GetParent();

    wxPoint pos = ( (SCH_COMPONENT*) GetParent() )->GetPosition();

    // Actual positions are calculated by the rotation/mirror transform of the
    // parent component of the field.  The inverse transform is used to calculate
    // the position relative to the parent component.
    wxPoint pt = aPosition - pos;

    SetTextPos( pos + component->GetTransform().InverseTransform().TransformCoordinate( pt ) );
}


wxPoint SCH_FIELD::GetPosition() const
{
    SCH_COMPONENT* component = (SCH_COMPONENT*) GetParent();
    wxPoint        pos = GetTextPos() - component->GetPosition();

    return component->GetTransform().TransformCoordinate( pos ) + component->GetPosition();
}
