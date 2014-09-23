/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2004 Jean-Pierre Charras, jaen-pierre.charras@gipsa-lab.inpg.com
 * Copyright (C) 2004-2011 KiCad Developers, see change_log.txt for contributors.
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
 * @file sch_field.cpp
 * @brief Implementation of the SCH_FIELD class.
 */

/* Fields are texts attached to a component, having a special meaning
 * Fields 0 and 1 are very important: reference and value
 * Field 2 is used as default footprint name.
 * Field 3 is reserved (not currently used
 * Fields 4 and more are user fields.
 * They can be renamed and can appear in reports
 */

#include <fctsys.h>
#include <class_drawpanel.h>
#include <base_struct.h>
#include <gr_basic.h>
#include <drawtxt.h>
#include <macros.h>
#include <trigo.h>
#include <wxEeschemaStruct.h>
#include <plot_common.h>

#include <general.h>
#include <protos.h>
#include <class_library.h>
#include <sch_component.h>
#include <sch_field.h>
#include <kicad_string.h>


SCH_FIELD::SCH_FIELD( const wxPoint& aPos, int aFieldId, SCH_COMPONENT* aParent, wxString aName ) :
    SCH_ITEM( aParent, SCH_FIELD_T ),
    EDA_TEXT()
{
    m_Pos = aPos;
    m_id = aFieldId;
    m_Attributs = TEXT_NO_VISIBLE;
    m_name = aName;

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
    wxString text = m_Text;

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
    int pensize = m_Thickness;

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


void SCH_FIELD::Draw( EDA_DRAW_PANEL* panel, wxDC* DC,
                      const wxPoint& offset, GR_DRAWMODE DrawMode, EDA_COLOR_T Color )
{
    int            orient;
    EDA_COLOR_T    color;
    wxPoint        textpos;
    SCH_COMPONENT* parentComponent = (SCH_COMPONENT*) m_Parent;
    int            LineWidth = m_Thickness;

    if( LineWidth == 0 )   // Use default values for pen size
    {
        if( m_Bold  )
            LineWidth = GetPenSizeForBold( m_Size.x );
        else
            LineWidth = GetDefaultLineThickness();
    }


    // Clip pen size for small texts:
    LineWidth = Clamp_Text_PenSize( LineWidth, m_Size, m_Bold );

    if( ((m_Attributs & TEXT_NO_VISIBLE) && !m_forceVisible) || IsVoid() )
        return;

    GRSetDrawMode( DC, DrawMode );

    // Calculate the text orientation according to the component orientation.
    orient = m_Orient;

    if( parentComponent->GetTransform().y1 )  // Rotate component 90 degrees.
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
    EDA_RECT boundaryBox = GetBoundingBox();
    textpos = boundaryBox.Centre();

    if( m_forceVisible )
    {
        color = DARKGRAY;
    }
    else
    {
        if( m_id == REFERENCE )
            color = GetLayerColor( LAYER_REFERENCEPART );
        else if( m_id == VALUE )
            color = GetLayerColor( LAYER_VALUEPART );
        else
            color = GetLayerColor( LAYER_FIELDS );
    }

    EDA_RECT* clipbox = panel? panel->GetClipBox() : NULL;
    DrawGraphicText( clipbox, DC, textpos, color, GetFullyQualifiedText(), orient, m_Size,
                     GR_TEXT_HJUSTIFY_CENTER, GR_TEXT_VJUSTIFY_CENTER,
                     LineWidth, m_Italic, m_Bold );

    // While moving: don't loose visual contact to which component this label belongs.
    if ( IsWireImage() )
    {
        const wxPoint origin = parentComponent->GetPosition();
        textpos  = m_Pos - origin;
        textpos  = parentComponent->GetScreenCoord( textpos );
        textpos += parentComponent->GetPosition();
        GRLine( clipbox, DC, origin, textpos, 2, DARKGRAY );
    }

    /* Enable this to draw the bounding box around the text field to validate
     * the bounding box calculations.
     */
#if 0

    // Draw boundary box:
    GRRect( panel->GetClipBox(), DC, boundaryBox, 0, BROWN );

    // Draw the text anchor point

    /* Calculate the text position, according to the component
     * orientation/mirror */
    textpos  = m_Pos - parentComponent->GetPosition();
    textpos  = parentComponent->GetScreenCoord( textpos );
    textpos += parentComponent->GetPosition();
    const int len = 10;
    GRLine( clipbox, DC,
            textpos.x - len, textpos.y, textpos.x + len, textpos.y, 0, BLUE );
    GRLine( clipbox, DC,
            textpos.x, textpos.y - len, textpos.x, textpos.y + len, 0, BLUE );
#endif
}


void SCH_FIELD::ImportValues( const LIB_FIELD& aSource )
{
    m_Orient    = aSource.GetOrientation();
    m_Size      = aSource.GetSize();
    m_HJustify  = aSource.GetHorizJustify();
    m_VJustify  = aSource.GetVertJustify();
    m_Italic    = aSource.IsItalic();
    m_Bold      = aSource.IsBold();
    m_Thickness = aSource.GetThickness();
    m_Attributs = aSource.GetAttributes();
    m_Mirror    = aSource.IsMirrored();
}


void SCH_FIELD::SwapData( SCH_ITEM* aItem )
{
    wxCHECK_RET( (aItem != NULL) && (aItem->Type() == SCH_FIELD_T),
                 wxT( "Cannot swap field data with invalid item." ) );

    SCH_FIELD* item = (SCH_FIELD*) aItem;

    EXCHG( m_Text, item->m_Text );
    EXCHG( m_Layer, item->m_Layer );
    EXCHG( m_Pos, item->m_Pos );
    EXCHG( m_Size, item->m_Size );
    EXCHG( m_Thickness, item->m_Thickness );
    EXCHG( m_Orient, item->m_Orient );
    EXCHG( m_Mirror, item->m_Mirror );
    EXCHG( m_Attributs, item->m_Attributs );
    EXCHG( m_Italic, item->m_Italic );
    EXCHG( m_Bold, item->m_Bold );
    EXCHG( m_HJustify, item->m_HJustify );
    EXCHG( m_VJustify, item->m_VJustify );
}


const EDA_RECT SCH_FIELD::GetBoundingBox() const
{
    SCH_COMPONENT* parentComponent = (SCH_COMPONENT*) m_Parent;
    int linewidth = ( m_Thickness == 0 ) ? GetDefaultLineThickness() : m_Thickness;

    // We must pass the effective text thickness to GetTextBox
    // when calculating the bounding box
    linewidth = Clamp_Text_PenSize( linewidth, m_Size, m_Bold );

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
    wxPoint pos = m_Pos - origin;
    wxPoint begin = rect.GetOrigin() - origin;
    wxPoint end = rect.GetEnd() - origin;
    RotatePoint( &begin, pos, m_Orient );
    RotatePoint( &end, pos, m_Orient );

    // Due to the Y axis direction, we must mirror the bounding box,
    // relative to the text position:
    begin.y -= pos.y;
    end.y -= pos.y;
    NEGATE( begin.y );
    NEGATE( end.y );
    begin.y += pos.y;
    end.y += pos.y;

    // Now, apply the component transform (mirror/rot)
    begin = parentComponent->GetTransform().TransformCoordinate( begin );
    end = parentComponent->GetTransform().TransformCoordinate( end );
    rect.SetOrigin( begin);
    rect.SetEnd( end);
    rect.Move( origin );
    rect.Normalize();
    return rect;
}


bool SCH_FIELD::Save( FILE* aFile ) const
{
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

    if( fprintf( aFile, "F %d %s %c %-3d %-3d %-3d %4.4X %c %c%c%c",
                 m_id,
                 EscapedUTF8( m_Text ).c_str(),     // wraps in quotes too
                 m_Orient == TEXT_ORIENT_HORIZ ? 'H' : 'V',
                 m_Pos.x, m_Pos.y,
                 m_Size.x,
                 m_Attributs,
                 hjustify, vjustify,
                 m_Italic ? 'I' : 'N',
                 m_Bold ? 'B' : 'N' ) == EOF )
    {
        return false;
    }

    // Save field name, if the name is user definable
    if( m_id >= FIELD1 )
    {
        if( fprintf( aFile, " %s", EscapedUTF8( m_name ).c_str() ) == EOF )
        {
            return false;
        }
    }

    if( fprintf( aFile, "\n" ) == EOF )
    {
        return false;
    }

    return true;
}


void SCH_FIELD::Place( SCH_EDIT_FRAME* frame, wxDC* DC )
{
    frame->GetCanvas()->SetMouseCapture( NULL, NULL );

    SCH_COMPONENT* component = (SCH_COMPONENT*) GetParent();

    // save old cmp in undo list
    frame->SaveUndoItemInUndoList( component );

    Draw( frame->GetCanvas(), DC, wxPoint( 0, 0 ), GR_DEFAULT_DRAWMODE );
    ClearFlags();
    frame->GetScreen()->SetCurItem( NULL );
    frame->OnModify();
}


bool SCH_FIELD::Matches( wxFindReplaceData& aSearchData, void* aAuxData, wxPoint* aFindLocation )
{
    bool match;
    wxString text = GetFullyQualifiedText();

    // User defined fields have an ID of -1.
    if( ((m_id > VALUE || m_id < REFERENCE) && !(aSearchData.GetFlags() & FR_SEARCH_ALL_FIELDS))
        || ((m_id == REFERENCE) && !(aSearchData.GetFlags() & FR_REPLACE_REFERENCES)) )
        return false;

    wxLogTrace( traceFindItem, wxT( "    child item " ) + GetSelectMenuText() );

    // Take sheet path into account which effects the reference field and the unit for
    // components with multiple parts.
    if( m_id == REFERENCE && aAuxData != NULL )
    {
        SCH_COMPONENT* component = (SCH_COMPONENT*) m_Parent;

        wxCHECK_MSG( component != NULL, false,
                     wxT( "No component associated with field" ) + text );

        text = component->GetRef( (SCH_SHEET_PATH*) aAuxData );

        if( component->GetUnitCount() > 1 )
            text << LIB_PART::SubReference( component->GetUnit() );
    }

    match = SCH_ITEM::Matches( text, aSearchData );

    if( match )
    {
        if( aFindLocation )
            *aFindLocation = GetBoundingBox().Centre();

        return true;
    }

    return false;
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
                     wxT( "Invalid replace component reference field call." ) ) ;

        SCH_COMPONENT* component = (SCH_COMPONENT*) m_Parent;

        wxCHECK_MSG( component != NULL, false,
                     wxT( "No component associated with field" ) + text );

        text = component->GetRef( (SCH_SHEET_PATH*) aAuxData );

        // if( component->GetUnitCount() > 1 )
        //     text << LIB_PART::SubReference( component->GetUnit() );

        isReplaced = EDA_ITEM::Replace( aSearchData, text );

        if( isReplaced )
            component->SetRef( (SCH_SHEET_PATH*) aAuxData, text );
    }
    else
    {
        isReplaced = EDA_ITEM::Replace( aSearchData, m_Text );
    }

    return isReplaced;
}


void SCH_FIELD::Rotate( wxPoint aPosition )
{
    RotatePoint( &m_Pos, aPosition, 900 );
}


wxString SCH_FIELD::GetSelectMenuText() const
{
    wxString tmp;
    tmp.Printf( _( "Field %s" ), GetChars( GetName() ) );

    return tmp;
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

    EDA_COLOR_T color = GetLayerColor( GetLayer() );

    if( m_Attributs & TEXT_NO_VISIBLE )
        return;

    if( IsVoid() )
        return;

    /* Calculate the text orientation, according to the component
     * orientation/mirror */
    int orient = m_Orient;

    if( parent->GetTransform().y1 )  // Rotate component 90 deg.
    {
        if( orient == TEXT_ORIENT_HORIZ )
            orient = TEXT_ORIENT_VERT;
        else
            orient = TEXT_ORIENT_HORIZ;
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

    aPlotter->Text( textpos, color, GetFullyQualifiedText(), orient, m_Size, hjustify, vjustify,
            thickness, m_Italic, m_Bold );
}


void SCH_FIELD::SetPosition( const wxPoint& aPosition )
{
    SCH_COMPONENT* component = (SCH_COMPONENT*) GetParent();

    wxPoint pos = ( (SCH_COMPONENT*) GetParent() )->GetPosition();

    // Actual positions are calculated by the rotation/mirror transform of the
    // parent component of the field.  The inverse transfrom is used to calculate
    // the position relative to the parent component.
    wxPoint pt = aPosition - pos;

    m_Pos = pos + component->GetTransform().InverseTransform().TransformCoordinate( pt );
}


wxPoint SCH_FIELD::GetPosition() const
{

    SCH_COMPONENT* component = (SCH_COMPONENT*) GetParent();

    wxPoint pos = m_Pos - component->GetPosition();

    return component->GetTransform().TransformCoordinate( pos ) + component->GetPosition();
}
