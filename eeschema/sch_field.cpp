/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2015 Jean-Pierre Charras, jp.charras at wanadoo.fr
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

/*
 * Fields are texts attached to a component, having a special meaning
 * Fields 0 and 1 are very important: reference and value
 * Field 2 is used as default footprint name.
 * Field 3 is reserved (not currently used
 * Fields 4 and more are user fields.
 * They can be renamed and can appear in reports
 */

#include <wx/menu.h>
#include <eda_item.h>
#include <gr_text.h>
#include <sch_edit_frame.h>
#include <plotter.h>
#include <bitmaps.h>
#include <core/mirror.h>
#include <kiway.h>
#include <general.h>
#include <class_library.h>
#include <sch_symbol.h>
#include <sch_field.h>
#include <schematic.h>
#include <settings/color_settings.h>
#include <kicad_string.h>
#include <trace_helpers.h>
#include <trigo.h>
#include <eeschema_id.h>
#include <tool/tool_manager.h>
#include <tools/ee_actions.h>

SCH_FIELD::SCH_FIELD( const wxPoint& aPos, int aFieldId, SCH_ITEM* aParent,
                      const wxString& aName ) :
    SCH_ITEM( aParent, SCH_FIELD_T ),
    EDA_TEXT( wxEmptyString ),
    m_id( 0 ),
    m_name( aName )
{
    SetTextPos( aPos );
    SetId( aFieldId );  // will also set the layer
    SetVisible( false );
}


SCH_FIELD::~SCH_FIELD()
{
}


EDA_ITEM* SCH_FIELD::Clone() const
{
    return new SCH_FIELD( *this );
}


void SCH_FIELD::SetId( int aId )
{
    m_id = aId;

    if( m_parent && m_parent->Type() == SCH_SHEET_T )
    {
        switch( m_id )
        {
        case SHEETNAME:     SetLayer( LAYER_SHEETNAME );     break;
        case SHEETFILENAME: SetLayer( LAYER_SHEETFILENAME ); break;
        default:            SetLayer( LAYER_SHEETFIELDS );   break;
        }
    }
    else
    {
        switch( m_id )
        {
        case REFERENCE_FIELD: SetLayer( LAYER_REFERENCEPART ); break;
        case VALUE_FIELD:     SetLayer( LAYER_VALUEPART );     break;
        default:              SetLayer( LAYER_FIELDS );        break;
        }
    }
}


wxString SCH_FIELD::GetShownText( int aDepth ) const
{
    std::function<bool( wxString* )> symbolResolver =
            [&]( wxString* token ) -> bool
            {
                if( token->Contains( ':' ) )
                {
                    if( Schematic()->ResolveCrossReference( token, aDepth ) )
                        return true;
                }
                else
                {
                    SCH_COMPONENT* parentSymbol = static_cast<SCH_COMPONENT*>( m_parent );

                    if( parentSymbol->ResolveTextVar( token, aDepth + 1 ) )
                        return true;

                    SCHEMATIC* schematic = parentSymbol->Schematic();
                    SCH_SHEET* sheet = schematic ? schematic->CurrentSheet().Last() : nullptr;

                    if( sheet && sheet->ResolveTextVar( token, aDepth + 1 ) )
                        return true;
                }

                return false;
            };

    std::function<bool( wxString* )> sheetResolver =
            [&]( wxString* token ) -> bool
            {
                SCH_SHEET* sheet = static_cast<SCH_SHEET*>( m_parent );
                return sheet->ResolveTextVar( token, aDepth + 1 );
            };

    std::function<bool( wxString* )> globalLabelResolver =
            [&]( wxString* token ) -> bool
            {
                SCH_GLOBALLABEL* globalLabel = static_cast<SCH_GLOBALLABEL*>( m_parent );
                return globalLabel->ResolveTextVar( token, aDepth + 1 );
            };

    PROJECT*  project = nullptr;
    bool      processTextVars = false;
    wxString  text = EDA_TEXT::GetShownText( &processTextVars );

    if( processTextVars )
    {
        if( Schematic() )
            project = &Schematic()->Prj();

        if( aDepth < 10 )
        {
            if( m_parent && m_parent->Type() == SCH_COMPONENT_T )
                text = ExpandTextVars( text, &symbolResolver, nullptr, project );
            else if( m_parent && m_parent->Type() == SCH_SHEET_T )
                text = ExpandTextVars( text, &sheetResolver, nullptr, project );
            else if( m_parent && m_parent->Type() == SCH_GLOBAL_LABEL_T )
                text = ExpandTextVars( text, &globalLabelResolver, nullptr, project );
            else
                text = ExpandTextVars( text, project );
        }
    }

    // WARNING: the IDs of FIELDS and SHEETS overlap, so one must check *both* the
    // id and the parent's type.

    if( m_parent && m_parent->Type() == SCH_COMPONENT_T )
    {
        SCH_COMPONENT* parentSymbol = static_cast<SCH_COMPONENT*>( m_parent );

        if( m_id == REFERENCE_FIELD )
        {
            // For more than one part per package, we must add the part selection
            // A, B, ... or 1, 2, .. to the reference.
            if( parentSymbol->GetUnitCount() > 1 )
                text << LIB_PART::SubReference( parentSymbol->GetUnit() );
        }
    }
    else if( m_parent && m_parent->Type() == SCH_SHEET_T )
    {
        if( m_id == SHEETFILENAME )
            text = _( "File:" ) + wxS( " " )+ text;
    }

    return text;
}


int SCH_FIELD::GetPenWidth() const
{
    return GetEffectiveTextPenWidth();
}


void SCH_FIELD::Print( const RENDER_SETTINGS* aSettings, const wxPoint& aOffset )
{
    wxDC*    DC = aSettings->GetPrintDC();
    COLOR4D  color = aSettings->GetLayerColor( IsForceVisible() ? LAYER_HIDDEN : m_layer );
    int      orient;
    wxPoint  textpos;
    int      penWidth = GetEffectiveTextPenWidth( aSettings->GetDefaultPenWidth() );

    if( ( !IsVisible() && !IsForceVisible() ) || IsVoid() )
        return;

    // Calculate the text orientation according to the symbol orientation.
    orient = GetTextAngle();

    if( m_parent && m_parent->Type() == SCH_COMPONENT_T )
    {
        SCH_COMPONENT* parentSymbol = static_cast<SCH_COMPONENT*>( m_parent );

        if( parentSymbol && parentSymbol->GetTransform().y1 )  // Rotate symbol 90 degrees.
        {
            if( orient == TEXT_ANGLE_HORIZ )
                orient = TEXT_ANGLE_VERT;
            else
                orient = TEXT_ANGLE_HORIZ;
        }
    }

    /*
     * Calculate the text justification, according to the symbol orientation/mirror.
     * This is a bit complicated due to cumulative calculations:
     * - numerous cases (mirrored or not, rotation)
     * - the GRText function will also recalculate H and V justifications according to the text
     *   orientation.
     * - When a symbol is mirrored, the text is not mirrored and justifications are complicated
     *   to calculate so the more easily way is to use no justifications (centered text) and use
     *   GetBoundingBox to know the text coordinate considered as centered
     */
    EDA_RECT boundaryBox = GetBoundingBox();
    textpos = boundaryBox.Centre() + aOffset;

    GRText( DC, textpos, color, GetShownText(), orient, GetTextSize(), GR_TEXT_HJUSTIFY_CENTER,
            GR_TEXT_VJUSTIFY_CENTER, penWidth, IsItalic(), IsBold() );
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

    std::swap( m_layer, item->m_layer );
    SwapText( *item );
    SwapEffects( *item );
}


const EDA_RECT SCH_FIELD::GetBoundingBox() const
{
    // Calculate the text bounding box:
    EDA_RECT rect = GetTextBox();

    // Calculate the bounding box position relative to the parent:
    wxPoint origin = GetParentPosition();
    wxPoint pos = GetTextPos() - origin;
    wxPoint begin = rect.GetOrigin() - origin;
    wxPoint end = rect.GetEnd() - origin;
    RotatePoint( &begin, pos, GetTextAngle() );
    RotatePoint( &end, pos, GetTextAngle() );

    // Now, apply the symbol transform (mirror/rot)
    TRANSFORM transform;

    if( m_parent && m_parent->Type() == SCH_COMPONENT_T )
    {
        SCH_COMPONENT* parentSymbol = static_cast<SCH_COMPONENT*>( m_parent );

        // Due to the Y axis direction, we must mirror the bounding box,
        // relative to the text position:
        MIRROR( begin.y, pos.y );
        MIRROR( end.y,   pos.y );

        transform = parentSymbol->GetTransform();
    }
    else
    {
        transform = TRANSFORM( 1, 0, 0, 1 );  // identity transform
    }

    rect.SetOrigin( transform.TransformCoordinate( begin ) );
    rect.SetEnd( transform.TransformCoordinate( end ) );

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


bool SCH_FIELD::Matches( const wxFindReplaceData& aSearchData, void* aAuxData ) const
{
    wxString text = GetShownText();
    int      flags = aSearchData.GetFlags();
    bool     searchHiddenFields = flags & FR_SEARCH_ALL_FIELDS;
    bool     searchAndReplace = flags & FR_SEARCH_REPLACE;
    bool     replaceReferences = flags & FR_REPLACE_REFERENCES;

    wxLogTrace( traceFindItem, wxT( "    child item " )
                    + GetSelectMenuText( EDA_UNITS::MILLIMETRES ) );

    if( !IsVisible() && !searchHiddenFields )
        return false;

    if( m_parent && m_parent->Type() == SCH_COMPONENT_T && m_id == REFERENCE_FIELD )
    {
        if( searchAndReplace && !replaceReferences )
            return false;

        SCH_COMPONENT* parentSymbol = static_cast<SCH_COMPONENT*>( m_parent );
        wxASSERT( aAuxData );

        // Take sheet path into account which effects the reference field and the unit for
        // symbols with multiple parts.
        if( aAuxData )
        {
            text = parentSymbol->GetRef((SCH_SHEET_PATH*) aAuxData );

            if( SCH_ITEM::Matches( text, aSearchData ) )
                return true;

            if( parentSymbol->GetUnitCount() > 1 )
                text << LIB_PART::SubReference( parentSymbol->GetUnit() );
        }
    }

    return SCH_ITEM::Matches( text, aSearchData );
}


bool SCH_FIELD::IsReplaceable() const
{
    if( m_parent && m_parent->Type() == SCH_COMPONENT_T )
    {
        SCH_COMPONENT* parentSymbol = static_cast<SCH_COMPONENT*>( m_parent );

        if( m_id == VALUE_FIELD )
        {
            if( parentSymbol->GetPartRef() && parentSymbol->GetPartRef()->IsPower() )
                return false;
        }
    }
    else if( m_parent && m_parent->Type() == SCH_SHEET_T )
    {
        // See comments in SCH_FIELD::Replace(), below.
        if( m_id == SHEETFILENAME )
            return false;
    }
    else if( m_parent && m_parent->Type() == SCH_GLOBAL_LABEL_T )
    {
        return false;
    }

    return true;
}


bool SCH_FIELD::Replace( const wxFindReplaceData& aSearchData, void* aAuxData )
{
    bool isReplaced = false;

    if( m_parent && m_parent->Type() == SCH_COMPONENT_T )
    {
        SCH_COMPONENT* parentSymbol = static_cast<SCH_COMPONENT*>( m_parent );

        if( m_id == REFERENCE_FIELD )
        {
            wxCHECK_MSG( aAuxData != NULL, false,
                         wxT( "Cannot replace reference designator without valid sheet path." ) );

            if( !( aSearchData.GetFlags() & FR_REPLACE_REFERENCES ) )
                return false;

            wxString text = parentSymbol->GetRef((SCH_SHEET_PATH*) aAuxData );

            isReplaced = EDA_ITEM::Replace( aSearchData, text );

            if( isReplaced )
                parentSymbol->SetRef((SCH_SHEET_PATH*) aAuxData, text );
        }
        else
        {
            isReplaced = EDA_TEXT::Replace( aSearchData );
        }
    }
    else if( m_parent && m_parent->Type() == SCH_SHEET_T )
    {
        isReplaced = EDA_TEXT::Replace( aSearchData );

        if( m_id == SHEETFILENAME && isReplaced )
        {
            // If we allowed this we'd have a bunch of work to do here, including warning
            // about it not being undoable, checking for recursive hierarchies, reloading
            // sheets, etc.  See DIALOG_SHEET_PROPERTIES::TransferDataFromWindow().
        }
    }

    return isReplaced;
}


void SCH_FIELD::Rotate( wxPoint aPosition )
{
    wxPoint pt = GetTextPos();
    RotatePoint( &pt, aPosition, 900 );
    SetTextPos( pt );
}


wxString SCH_FIELD::GetSelectMenuText( EDA_UNITS aUnits ) const
{
    return wxString::Format( "%s '%s'",
                             GetName(),
                             ShortenedShownText() );
}


void SCH_FIELD::DoHypertextMenu( EDA_DRAW_FRAME* aFrame )
{
    static wxString back = "HYPERTEXT_BACK";
    wxMenu          menu;
    SCH_TEXT*       label = dynamic_cast<SCH_TEXT*>( m_parent );

    if( label && Schematic() )
    {
        auto it = Schematic()->GetPageRefsMap().find( label->GetText() );

        if( it != Schematic()->GetPageRefsMap().end() )
        {
            std::map<wxString, wxString> sheetNames;
            std::vector<wxString>        pageListCopy;

            pageListCopy.insert( pageListCopy.end(), it->second.begin(), it->second.end() );
            std::sort( pageListCopy.begin(), pageListCopy.end() );

            for( const SCH_SHEET_PATH& sheet : Schematic()->GetSheets() )
            {
                if( sheet.size() == 1 )
                    sheetNames[ sheet.GetPageNumber() ] = _( "<root sheet>" );
                else
                    sheetNames[ sheet.GetPageNumber() ] = sheet.Last()->GetName();
            }

            for( int i = 0; i < (int) pageListCopy.size(); ++i )
            {
                menu.Append( i, wxString::Format( _( "Go to Page %s (%s)" ),
                                                  pageListCopy[i],
                                                  sheetNames[ pageListCopy[i] ] ) );
            }

            menu.AppendSeparator();
            menu.Append( 999, _( "Back" ) );

            int   sel = aFrame->GetPopupMenuSelectionFromUser( menu );
            void* param = nullptr;

            if( sel >= 0 && sel < (int) pageListCopy.size() )
                param = (void*) &pageListCopy[ sel ];
            else if( sel == 999 )
                param = (void*) &back;

            if( param )
                aFrame->GetToolManager()->RunAction( EE_ACTIONS::hypertextCommand, true, param );
        }
    }
}


wxString SCH_FIELD::GetName( bool aUseDefaultName ) const
{
    if( !m_name.IsEmpty() )
        return m_name;
    else if( aUseDefaultName )
    {
        if( m_parent && m_parent->Type() == SCH_COMPONENT_T )
            return TEMPLATE_FIELDNAME::GetDefaultFieldName( m_id );
        else if( m_parent && m_parent->Type() == SCH_SHEET_T )
            return SCH_SHEET::GetDefaultFieldName( m_id );
        else if( m_parent && m_parent->Type() == SCH_GLOBAL_LABEL_T )
            return _( "Intersheet References" );
    }

    return wxEmptyString;
}


wxString SCH_FIELD::GetCanonicalName() const
{
    if( m_parent && m_parent->Type() == SCH_COMPONENT_T )
    {
        switch( m_id )
        {
        case  REFERENCE_FIELD: return wxT( "Reference" );
        case  VALUE_FIELD:     return wxT( "Value" );
        case  FOOTPRINT_FIELD: return wxT( "Footprint" );
        case  DATASHEET_FIELD: return wxT( "Datasheet" );
        }
    }
    else if( m_parent && m_parent->Type() == SCH_SHEET_T )
    {
        switch( m_id )
        {
        case  SHEETNAME:     return wxT( "Sheetname" );
        case  SHEETFILENAME: return wxT( "Sheetfile" );
        }
    }
    else if( m_parent && m_parent->Type() == SCH_GLOBAL_LABEL_T )
    {
        return wxT( "Intersheet References" );
    }

    return m_name;
}


BITMAP_DEF SCH_FIELD::GetMenuImage() const
{
    if( m_parent && m_parent->Type() == SCH_COMPONENT_T )
    {
        switch( m_id )
        {
        case REFERENCE_FIELD: return edit_comp_ref_xpm;
        case VALUE_FIELD:     return edit_comp_value_xpm;
        case FOOTPRINT_FIELD: return edit_comp_footprint_xpm;
        default:              return text_xpm;
        }
    }

    return text_xpm;
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
    RENDER_SETTINGS* settings = aPlotter->RenderSettings();
    COLOR4D          color = settings->GetLayerColor( GetLayer() );
    int              penWidth = GetEffectiveTextPenWidth( settings->GetDefaultPenWidth() );

    penWidth = std::max( penWidth, settings->GetMinPenWidth() );

    if( !IsVisible() )
        return;

    if( IsVoid() )
        return;

    // Calculate the text orientation, according to the symbol orientation/mirror
    int orient = GetTextAngle();

    if( m_parent && m_parent->Type() == SCH_COMPONENT_T )
    {
        SCH_COMPONENT* parentSymbol = static_cast<SCH_COMPONENT*>( m_parent );

        if( parentSymbol->GetTransform().y1 )  // Rotate symbol 90 deg.
        {
            if( orient == TEXT_ANGLE_HORIZ )
                orient = TEXT_ANGLE_VERT;
            else
                orient = TEXT_ANGLE_HORIZ;
        }
    }

    /*
     * Calculate the text justification, according to the symbol orientation/mirror.
     * This is a bit complicated due to cumulative calculations:
     * - numerous cases (mirrored or not, rotation)
     * - the plotter's Text function will also recalculate H and V justifications according to
     *   the text orientation.
     * - When a symbol is mirrored, the text is not mirrored and justifications are complicated
     *   to calculate so the easier way is to use no justifications (centered text) and use
     *   GetBoundingBox to know the text coordinate considered as centered
     */
    EDA_RECT BoundaryBox = GetBoundingBox();
    EDA_TEXT_HJUSTIFY_T hjustify = GR_TEXT_HJUSTIFY_CENTER;
    EDA_TEXT_VJUSTIFY_T vjustify = GR_TEXT_VJUSTIFY_CENTER;
    wxPoint  textpos = BoundaryBox.Centre();

    aPlotter->Text( textpos, color, GetShownText(), orient, GetTextSize(),  hjustify, vjustify,
                    penWidth, IsItalic(), IsBold() );
}


void SCH_FIELD::SetPosition( const wxPoint& aPosition )
{
    // Actual positions are calculated by the rotation/mirror transform of the
    // parent component of the field.  The inverse transform is used to calculate
    // the position relative to the parent component.
    if( m_parent && m_parent->Type() == SCH_COMPONENT_T )
    {
        SCH_COMPONENT* parentComponent = static_cast<SCH_COMPONENT*>( m_parent );
        wxPoint        relativePos = aPosition - parentComponent->GetPosition();

        relativePos = parentComponent->GetTransform().
                            InverseTransform().TransformCoordinate( relativePos );

        SetTextPos( relativePos + parentComponent->GetPosition() );
        return;
    }

    SetTextPos( aPosition );
}


wxPoint SCH_FIELD::GetPosition() const
{
    if( m_parent && m_parent->Type() == SCH_COMPONENT_T )
    {
        SCH_COMPONENT* parentComponent = static_cast<SCH_COMPONENT*>( m_parent );
        wxPoint        relativePos = GetTextPos() - parentComponent->GetPosition();

        relativePos = parentComponent->GetTransform().TransformCoordinate( relativePos );

        return relativePos + parentComponent->GetPosition();
    }

    return GetTextPos();
}


wxPoint SCH_FIELD::GetParentPosition() const
{
    return m_parent ? m_parent->GetPosition() : wxPoint( 0, 0 );
}


bool SCH_FIELD::operator <( const SCH_ITEM& aItem ) const
{
    if( Type() != aItem.Type() )
        return Type() < aItem.Type();

    auto field = static_cast<const SCH_FIELD*>( &aItem );

    if( GetId() != field->GetId() )
        return GetId() < field->GetId();

    if( GetText() != field->GetText() )
        return GetText() < field->GetText();

    if( GetLibPosition().x != field->GetLibPosition().x )
        return GetLibPosition().x < field->GetLibPosition().x;

    if( GetLibPosition().y != field->GetLibPosition().y )
        return GetLibPosition().y < field->GetLibPosition().y;

    return GetName() < field->GetName();
}
