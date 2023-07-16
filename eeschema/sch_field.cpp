/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2015 Jean-Pierre Charras, jp.charras at wanadoo.fr
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

/*
 * Fields are texts attached to a symbol, some of which have a special meaning.
 * Fields 0 and 1 are very important: reference and value.
 * Field 2 is used as default footprint name.
 * Field 3 is used to point to a datasheet (usually a URL).
 * Fields 4+ are user fields.  They can be renamed and can appear in reports.
 */

#include <wx/log.h>
#include <wx/menu.h>
#include <base_units.h>
#include <common.h>     // for ExpandTextVars
#include <eda_item.h>
#include <sch_edit_frame.h>
#include <plotters/plotter.h>
#include <bitmaps.h>
#include <core/kicad_algo.h>
#include <core/mirror.h>
#include <kiway.h>
#include <general.h>
#include <symbol_library.h>
#include <sch_symbol.h>
#include <sch_field.h>
#include <sch_label.h>
#include <schematic.h>
#include <settings/color_settings.h>
#include <string_utils.h>
#include <trace_helpers.h>
#include <trigo.h>
#include <eeschema_id.h>
#include <tool/tool_manager.h>
#include <tools/sch_navigate_tool.h>
#include <font/outline_font.h>

SCH_FIELD::SCH_FIELD( const VECTOR2I& aPos, int aFieldId, SCH_ITEM* aParent,
                      const wxString& aName ) :
    SCH_ITEM( aParent, SCH_FIELD_T ),
    EDA_TEXT( schIUScale, wxEmptyString ),
    m_id( 0 ),
    m_showName( false ),
    m_allowAutoPlace( true ),
    m_renderCacheValid( false ),
    m_lastResolvedColor( COLOR4D::UNSPECIFIED )
{
    SetName( aName );
    SetTextPos( aPos );
    SetId( aFieldId );  // will also set the layer
    SetVisible( false );
}


SCH_FIELD::SCH_FIELD( SCH_ITEM* aParent, int aFieldId, const wxString& aName ) :
    SCH_FIELD( VECTOR2I(), aFieldId, aParent, aName )
{
}


SCH_FIELD::SCH_FIELD( const SCH_FIELD& aField ) :
    SCH_ITEM( aField ),
    EDA_TEXT( aField )
{
    m_id             = aField.m_id;
    m_name           = aField.m_name;
    m_showName       = aField.m_showName;
    m_allowAutoPlace = aField.m_allowAutoPlace;
    m_isNamedVariable = aField.m_isNamedVariable;

    m_renderCache.clear();

    for( const std::unique_ptr<KIFONT::GLYPH>& glyph : aField.m_renderCache )
    {
        KIFONT::OUTLINE_GLYPH* outline_glyph = static_cast<KIFONT::OUTLINE_GLYPH*>( glyph.get() );
        m_renderCache.emplace_back( std::make_unique<KIFONT::OUTLINE_GLYPH>( *outline_glyph ) );
    }

    m_renderCacheValid = aField.m_renderCacheValid;
    m_renderCachePos = aField.m_renderCachePos;

    m_lastResolvedColor = aField.m_lastResolvedColor;
}


SCH_FIELD& SCH_FIELD::operator=( const SCH_FIELD& aField )
{
    EDA_TEXT::operator=( aField );

    m_id              = aField.m_id;
    m_name            = aField.m_name;
    m_showName        = aField.m_showName;
    m_allowAutoPlace  = aField.m_allowAutoPlace;
    m_isNamedVariable = aField.m_isNamedVariable;

    m_renderCache.clear();

    for( const std::unique_ptr<KIFONT::GLYPH>& glyph : aField.m_renderCache )
    {
        KIFONT::OUTLINE_GLYPH* outline_glyph = static_cast<KIFONT::OUTLINE_GLYPH*>( glyph.get() );
        m_renderCache.emplace_back( std::make_unique<KIFONT::OUTLINE_GLYPH>( *outline_glyph ) );
    }

    m_renderCacheValid = aField.m_renderCacheValid;
    m_renderCachePos = aField.m_renderCachePos;

    m_lastResolvedColor = aField.m_lastResolvedColor;

    return *this;
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
    else if( m_parent && m_parent->Type() == SCH_SYMBOL_T )
    {
        switch( m_id )
        {
        case REFERENCE_FIELD: SetLayer( LAYER_REFERENCEPART ); break;
        case VALUE_FIELD:     SetLayer( LAYER_VALUEPART );     break;
        default:              SetLayer( LAYER_FIELDS );        break;
        }
    }
    else if( m_parent && m_parent->IsType( { SCH_LABEL_LOCATE_ANY_T } ) )
    {
        // We can't use defined IDs for labels because there can be multiple net class
        // assignments.

        if( GetCanonicalName() == wxT( "Netclass" ) )
            SetLayer( LAYER_NETCLASS_REFS );
        else if( GetCanonicalName() == wxT( "Intersheetrefs" ) )
            SetLayer( LAYER_INTERSHEET_REFS );
        else
            SetLayer( LAYER_FIELDS );
    }
}


wxString SCH_FIELD::GetShownName() const
{
    return m_isNamedVariable ? GetTextVars( GetName() ) : GetName();
}


wxString SCH_FIELD::GetShownText( const SCH_SHEET_PATH* aPath, bool aAllowExtraText,
                                  int aDepth ) const
{
    std::function<bool( wxString* )> symbolResolver =
            [&]( wxString* token ) -> bool
            {
                return static_cast<SCH_SYMBOL*>( m_parent )->ResolveTextVar( aPath, token, aDepth + 1 );
            };

    std::function<bool( wxString* )> sheetResolver =
            [&]( wxString* token ) -> bool
            {
                return static_cast<SCH_SHEET*>( m_parent )->ResolveTextVar( token, aDepth + 1 );
            };

    std::function<bool( wxString* )> labelResolver =
            [&]( wxString* token ) -> bool
            {
                return static_cast<SCH_LABEL_BASE*>( m_parent )->ResolveTextVar( aPath, token,
                                                                                 aDepth + 1 );
            };

    wxString text = EDA_TEXT::GetShownText( aAllowExtraText, aDepth );

    if( IsNameShown() && aAllowExtraText )
        text = GetShownName() << wxS( ": " ) << text;

    if( text == wxS( "~" ) ) // Legacy placeholder for empty string
    {
        text = wxS( "" );
    }
    else if( HasTextVars() )
    {
        if( aDepth < 10 )
        {
            if( m_parent && m_parent->Type() == SCH_SYMBOL_T )
                text = ExpandTextVars( text, &symbolResolver );
            else if( m_parent && m_parent->Type() == SCH_SHEET_T )
                text = ExpandTextVars( text, &sheetResolver );
            else if( m_parent && m_parent->IsType( { SCH_LABEL_LOCATE_ANY_T } ) )
                text = ExpandTextVars( text, &labelResolver );
            else if( Schematic() )
                text = ExpandTextVars( text, &Schematic()->Prj() );
        }
    }

    // WARNING: the IDs of FIELDS and SHEETS overlap, so one must check *both* the
    // id and the parent's type.

    if( m_parent && m_parent->Type() == SCH_SYMBOL_T )
    {
        SCH_SYMBOL* parentSymbol = static_cast<SCH_SYMBOL*>( m_parent );

        if( m_id == REFERENCE_FIELD )
        {
            // For more than one part per package, we must add the part selection
            // A, B, ... or 1, 2, .. to the reference.
            if( parentSymbol->GetUnitCount() > 1 )
                text << LIB_SYMBOL::SubReference( parentSymbol->GetUnit() );
        }
    }
    else if( m_parent && m_parent->Type() == SCH_SHEET_T )
    {
        if( m_id == SHEETFILENAME && aAllowExtraText )
            text = _( "File:" ) + wxS( " " )+ text;
    }

    return text;
}


wxString SCH_FIELD::GetShownText( bool aAllowExtraText, int aDepth ) const
{
    if( SCHEMATIC* schematic = Schematic() )
        return GetShownText( &schematic->CurrentSheet(), aAllowExtraText, aDepth );
    else
        return EDA_TEXT::GetShownText( aAllowExtraText, aDepth );
}



int SCH_FIELD::GetPenWidth() const
{
    return GetEffectiveTextPenWidth();
}


KIFONT::FONT* SCH_FIELD::getDrawFont() const
{
    KIFONT::FONT* font = EDA_TEXT::GetFont();

    if( !font )
        font = KIFONT::FONT::GetFont( GetDefaultFont(), IsBold(), IsItalic() );

    return font;
}


void SCH_FIELD::ClearCaches()
{
    ClearRenderCache();
    EDA_TEXT::ClearBoundingBoxCache();
}


void SCH_FIELD::ClearRenderCache()
{
    EDA_TEXT::ClearRenderCache();
    m_renderCacheValid = false;
}


std::vector<std::unique_ptr<KIFONT::GLYPH>>*
SCH_FIELD::GetRenderCache( const wxString& forResolvedText, const VECTOR2I& forPosition,
                           TEXT_ATTRIBUTES& aAttrs ) const
{
    KIFONT::FONT* font = GetFont();

    if( !font )
        font = KIFONT::FONT::GetFont( GetDefaultFont(), IsBold(), IsItalic() );

    if( font->IsOutline() )
    {
        KIFONT::OUTLINE_FONT* outlineFont = static_cast<KIFONT::OUTLINE_FONT*>( font );

        if( m_renderCache.empty() || !m_renderCacheValid )
        {
            m_renderCache.clear();

            outlineFont->GetLinesAsGlyphs( &m_renderCache, forResolvedText, forPosition, aAttrs );

            m_renderCachePos = forPosition;
            m_renderCacheValid = true;
        }

        if( m_renderCachePos != forPosition )
        {
            VECTOR2I delta = forPosition - m_renderCachePos;

            for( std::unique_ptr<KIFONT::GLYPH>& glyph : m_renderCache )
                static_cast<KIFONT::OUTLINE_GLYPH*>( glyph.get() )->Move( delta );

            m_renderCachePos = forPosition;
        }

        return &m_renderCache;
    }

    return nullptr;
}


void SCH_FIELD::Print( const RENDER_SETTINGS* aSettings, const VECTOR2I& aOffset )
{
    wxDC*    DC = aSettings->GetPrintDC();
    COLOR4D  color = aSettings->GetLayerColor( IsForceVisible() ? LAYER_HIDDEN : m_layer );
    bool     blackAndWhiteMode = GetGRForceBlackPenState();
    VECTOR2I textpos;
    int      penWidth = GetEffectiveTextPenWidth( aSettings->GetDefaultPenWidth() );

    if( ( !IsVisible() && !IsForceVisible() ) || GetShownText( true ).IsEmpty() )
        return;

    COLOR4D bg = aSettings->GetBackgroundColor();

    if( bg == COLOR4D::UNSPECIFIED || GetGRForceBlackPenState() )
        bg = COLOR4D::WHITE;

    if( IsForceVisible() )
        bg = aSettings->GetLayerColor( LAYER_HIDDEN );

    if( !blackAndWhiteMode && GetTextColor() != COLOR4D::UNSPECIFIED )
        color = GetTextColor();

    // Calculate the text orientation according to the symbol orientation.
    EDA_ANGLE orient = GetTextAngle();

    if( m_parent && m_parent->Type() == SCH_SYMBOL_T )
    {
        SCH_SYMBOL* parentSymbol = static_cast<SCH_SYMBOL*>( m_parent );

        if( parentSymbol && parentSymbol->GetTransform().y1 )  // Rotate symbol 90 degrees.
        {
            if( orient == ANGLE_HORIZONTAL )
                orient = ANGLE_VERTICAL;
            else
                orient = ANGLE_HORIZONTAL;
        }

        if( parentSymbol && parentSymbol->GetDNP() )
        {
            color.Desaturate();
            color = color.Mix( bg, 0.5f );
        }
    }

    KIFONT::FONT* font = GetFont();

    if( !font )
        font = KIFONT::FONT::GetFont( aSettings->GetDefaultFont(), IsBold(), IsItalic() );

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
    textpos = GetBoundingBox().Centre() + aOffset;

    if( GetParent() && GetParent()->Type() == SCH_GLOBAL_LABEL_T )
    {
        SCH_GLOBALLABEL* label = static_cast<SCH_GLOBALLABEL*>( GetParent() );
        textpos += label->GetSchematicTextOffset( aSettings );
    }

    GRPrintText( DC, textpos, color, GetShownText( true ), orient, GetTextSize(),
                 GR_TEXT_H_ALIGN_CENTER, GR_TEXT_V_ALIGN_CENTER, penWidth, IsItalic(), IsBold(),
                 font );
}


void SCH_FIELD::ImportValues( const LIB_FIELD& aSource )
{
    SetAttributes( aSource );
    SetNameShown( aSource.IsNameShown() );
    SetCanAutoplace( aSource.CanAutoplace() );
}


void SCH_FIELD::SwapData( SCH_ITEM* aItem )
{
    SCH_ITEM::SwapFlags( aItem );

    wxCHECK_RET( ( aItem != nullptr ) && ( aItem->Type() == SCH_FIELD_T ),
                 wxT( "Cannot swap field data with invalid item." ) );

    SCH_FIELD* item = (SCH_FIELD*) aItem;

    std::swap( m_layer, item->m_layer );
    std::swap( m_showName, item->m_showName );
    std::swap( m_allowAutoPlace, item->m_allowAutoPlace );
    std::swap( m_isNamedVariable, item->m_isNamedVariable );
    SwapText( *item );
    SwapAttributes( *item );

    std::swap( m_lastResolvedColor, item->m_lastResolvedColor );
}


COLOR4D SCH_FIELD::GetFieldColor() const
{
    if( GetTextColor() != COLOR4D::UNSPECIFIED )
    {
        m_lastResolvedColor = GetTextColor();
    }
    else
    {
        SCH_LABEL_BASE* parentLabel = dynamic_cast<SCH_LABEL_BASE*>( GetParent() );

        if( parentLabel && !parentLabel->IsConnectivityDirty() )
            m_lastResolvedColor = parentLabel->GetEffectiveNetClass()->GetSchematicColor();
        else
            m_lastResolvedColor = GetTextColor();
    }

    return m_lastResolvedColor;
}


void SCH_FIELD::ViewGetLayers( int aLayers[], int& aCount ) const
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


EDA_ANGLE SCH_FIELD::GetDrawRotation() const
{
    // Calculate the text orientation according to the symbol orientation.
    EDA_ANGLE orient = GetTextAngle();

    if( m_parent && m_parent->Type() == SCH_SYMBOL_T )
    {
        SCH_SYMBOL* parentSymbol = static_cast<SCH_SYMBOL*>( m_parent );

        if( parentSymbol && parentSymbol->GetTransform().y1 )  // Rotate symbol 90 degrees.
        {
            if( orient.IsHorizontal() )
                orient = ANGLE_VERTICAL;
            else
                orient = ANGLE_HORIZONTAL;
        }
    }

    return orient;
}


const BOX2I SCH_FIELD::GetBoundingBox() const
{
    // Calculate the text bounding box:
    BOX2I bbox = GetTextBox();

    // Calculate the bounding box position relative to the parent:
    VECTOR2I origin = GetParentPosition();
    VECTOR2I pos = GetTextPos() - origin;
    VECTOR2I begin = bbox.GetOrigin() - origin;
    VECTOR2I end = bbox.GetEnd() - origin;
    RotatePoint( begin, pos, GetTextAngle() );
    RotatePoint( end, pos, GetTextAngle() );

    // Now, apply the symbol transform (mirror/rot)
    TRANSFORM transform;

    if( m_parent && m_parent->Type() == SCH_SYMBOL_T )
    {
        SCH_SYMBOL* parentSymbol = static_cast<SCH_SYMBOL*>( m_parent );

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

    bbox.SetOrigin( transform.TransformCoordinate( begin ) );
    bbox.SetEnd( transform.TransformCoordinate( end ) );

    bbox.Move( origin );
    bbox.Normalize();

    return bbox;
}


bool SCH_FIELD::IsHorizJustifyFlipped() const
{
    VECTOR2I render_center = GetBoundingBox().Centre();
    VECTOR2I pos = GetPosition();

    switch( GetHorizJustify() )
    {
    case GR_TEXT_H_ALIGN_LEFT:
        if( GetDrawRotation().IsVertical() )
            return render_center.y > pos.y;
        else
            return render_center.x < pos.x;
    case GR_TEXT_H_ALIGN_RIGHT:
        if( GetDrawRotation().IsVertical() )
            return render_center.y < pos.y;
        else
            return render_center.x > pos.x;
    default:
        return false;
    }
}


GR_TEXT_H_ALIGN_T SCH_FIELD::GetEffectiveHorizJustify() const
{
    switch( GetHorizJustify() )
    {
    case GR_TEXT_H_ALIGN_LEFT:
        return IsHorizJustifyFlipped() ? GR_TEXT_H_ALIGN_RIGHT : GR_TEXT_H_ALIGN_LEFT;
    case GR_TEXT_H_ALIGN_RIGHT:
        return IsHorizJustifyFlipped() ? GR_TEXT_H_ALIGN_LEFT : GR_TEXT_H_ALIGN_RIGHT;
    default:
        return GR_TEXT_H_ALIGN_CENTER;
    }
}


bool SCH_FIELD::IsVertJustifyFlipped() const
{
    VECTOR2I render_center = GetBoundingBox().Centre();
    VECTOR2I pos = GetPosition();

    switch( GetVertJustify() )
    {
    case GR_TEXT_V_ALIGN_TOP:
        if( GetDrawRotation().IsVertical() )
            return render_center.x < pos.x;
        else
            return render_center.y < pos.y;
    case GR_TEXT_V_ALIGN_BOTTOM:
        if( GetDrawRotation().IsVertical() )
            return render_center.x > pos.x;
        else
            return render_center.y > pos.y;
    default:
        return false;
    }
}


GR_TEXT_V_ALIGN_T SCH_FIELD::GetEffectiveVertJustify() const
{
    switch( GetVertJustify() )
    {
    case GR_TEXT_V_ALIGN_TOP:
        return IsVertJustifyFlipped() ? GR_TEXT_V_ALIGN_BOTTOM : GR_TEXT_V_ALIGN_TOP;
    case GR_TEXT_V_ALIGN_BOTTOM:
        return IsVertJustifyFlipped() ? GR_TEXT_V_ALIGN_TOP : GR_TEXT_V_ALIGN_BOTTOM;
    default:
        return GR_TEXT_V_ALIGN_CENTER;
    }
}


bool SCH_FIELD::Matches( const EDA_SEARCH_DATA& aSearchData, void* aAuxData ) const
{
    bool searchHiddenFields = false;
    bool searchAndReplace = false;
    bool replaceReferences = false;

    try
    {
        const SCH_SEARCH_DATA& schSearchData = dynamic_cast<const SCH_SEARCH_DATA&>( aSearchData ); // downcast
        searchHiddenFields = schSearchData.searchAllFields;
        searchAndReplace = schSearchData.searchAndReplace;
        replaceReferences = schSearchData.replaceReferences;
    }
    catch( const std::bad_cast& )
    {
    }

    wxString text = UnescapeString( GetText() );

    if( !IsVisible() && !searchHiddenFields )
        return false;

    if( m_parent && m_parent->Type() == SCH_SYMBOL_T && m_id == REFERENCE_FIELD )
    {
        if( searchAndReplace && !replaceReferences )
            return false;

        SCH_SYMBOL* parentSymbol = static_cast<SCH_SYMBOL*>( m_parent );
        wxASSERT( aAuxData );

        // Take sheet path into account which effects the reference field and the unit for
        // symbols with multiple parts.
        if( aAuxData )
        {
            text = parentSymbol->GetRef( (SCH_SHEET_PATH*) aAuxData );

            if( SCH_ITEM::Matches( text, aSearchData ) )
                return true;

            if( parentSymbol->GetUnitCount() > 1 )
                text << LIB_SYMBOL::SubReference( parentSymbol->GetUnit() );
        }
    }

    return SCH_ITEM::Matches( text, aSearchData );
}


bool SCH_FIELD::IsReplaceable() const
{
    if( m_parent && m_parent->Type() == SCH_SHEET_T )
    {
        // See comments in SCH_FIELD::Replace(), below.
        if( m_id == SHEETFILENAME )
            return false;
    }
    else if( m_parent && m_parent->Type() == SCH_GLOBAL_LABEL_T )
    {
        if( m_id == 0 /* IntersheetRefs */ )
            return false;
    }

    return true;
}


bool SCH_FIELD::Replace( const EDA_SEARCH_DATA& aSearchData, void* aAuxData )
{
    bool replaceReferences = false;

    try
    {
        const SCH_SEARCH_DATA& schSearchData = dynamic_cast<const SCH_SEARCH_DATA&>( aSearchData );
        replaceReferences = schSearchData.replaceReferences;
    }
    catch( const std::bad_cast& )
    {
    }

    wxString text;
    bool     isReplaced = false;

    if( m_parent && m_parent->Type() == SCH_SYMBOL_T )
    {
        SCH_SYMBOL* parentSymbol = static_cast<SCH_SYMBOL*>( m_parent );

        switch( m_id )
        {
        case REFERENCE_FIELD:
            wxCHECK_MSG( aAuxData, false, wxT( "Need sheetpath to replace in refdes." ) );

            if( !replaceReferences )
                return false;

            text = parentSymbol->GetRef( (SCH_SHEET_PATH*) aAuxData );
            isReplaced = EDA_ITEM::Replace( aSearchData, text );

            if( isReplaced )
                parentSymbol->SetRef( (SCH_SHEET_PATH*) aAuxData, text );

            break;

        case VALUE_FIELD:
            wxCHECK_MSG( aAuxData, false, wxT( "Need sheetpath to replace in value field." ) );

            text = parentSymbol->GetField( VALUE_FIELD )->GetText();
            isReplaced = EDA_ITEM::Replace( aSearchData, text );

            if( isReplaced )
                parentSymbol->SetValueFieldText( text );

            break;

        case FOOTPRINT_FIELD:
            wxCHECK_MSG( aAuxData, false, wxT( "Need sheetpath to replace in footprint field." ) );

            text = parentSymbol->GetField( FOOTPRINT_FIELD )->GetText();
            isReplaced = EDA_ITEM::Replace( aSearchData, text );

            if( isReplaced )
                parentSymbol->SetFootprintFieldText( text );

            break;

        default:
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
    else
    {
        isReplaced = EDA_TEXT::Replace( aSearchData );
    }

    return isReplaced;
}


void SCH_FIELD::Rotate( const VECTOR2I& aCenter )
{
    VECTOR2I pt = GetPosition();
    RotatePoint( pt, aCenter, ANGLE_90 );
    SetPosition( pt );
}


wxString SCH_FIELD::GetItemDescription( UNITS_PROVIDER* aUnitsProvider ) const
{
    return wxString::Format( "%s '%s'", GetName(), KIUI::EllipsizeMenuText( GetText() ) );
}


void SCH_FIELD::GetMsgPanelInfo( EDA_DRAW_FRAME* aFrame, std::vector<MSG_PANEL_ITEM>& aList )
{
    wxString msg;

    aList.emplace_back( _( "Symbol Field" ), UnescapeString( GetName() ) );

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


void SCH_FIELD::DoHypertextAction( EDA_DRAW_FRAME* aFrame ) const
{
    constexpr int START_ID = 1;

    if( IsHypertext() )
    {
        SCH_LABEL_BASE*                            label = static_cast<SCH_LABEL_BASE*>( m_parent );
        std::vector<std::pair<wxString, wxString>> pages;
        wxMenu                                     menu;
        wxString                                   href;

        label->GetIntersheetRefs( &pages );

        for( int i = 0; i < (int) pages.size(); ++i )
        {
            menu.Append( i + START_ID, wxString::Format( _( "Go to Page %s (%s)" ),
                                                         pages[i].first,
                                                         pages[i].second ) );
        }

        menu.AppendSeparator();
        menu.Append( 999 + START_ID, _( "Back to Previous Selected Sheet" ) );

        int sel = aFrame->GetPopupMenuSelectionFromUser( menu ) - START_ID;

        if( sel >= 0 && sel < (int) pages.size() )
            href = wxT( "#" ) + pages[ sel ].first;
        else if( sel == 999 )
            href = SCH_NAVIGATE_TOOL::g_BackLink;

        if( !href.IsEmpty() )
        {
            SCH_NAVIGATE_TOOL* navTool = aFrame->GetToolManager()->GetTool<SCH_NAVIGATE_TOOL>();
            navTool->HypertextCommand( href );
        }
    }
}


void SCH_FIELD::SetName( const wxString& aName )
{
    m_name = aName;
    m_isNamedVariable = m_name.StartsWith( wxT( "${" ) );

    if( m_isNamedVariable )
        EDA_TEXT::SetText( aName );
}


void SCH_FIELD::SetText( const wxString& aText )
{
    // Don't allow modification of text value when using named variables
    // as field name.
    if( m_isNamedVariable )
        return;

    EDA_TEXT::SetText( aText );
}


wxString SCH_FIELD::GetName( bool aUseDefaultName ) const
{
    if( m_parent && m_parent->Type() == SCH_SYMBOL_T )
    {
        if( m_id >= 0 && m_id < MANDATORY_FIELDS )
            return TEMPLATE_FIELDNAME::GetDefaultFieldName( m_id );
        else if( m_name.IsEmpty() && aUseDefaultName )
            return TEMPLATE_FIELDNAME::GetDefaultFieldName( m_id );
        else
            return m_name;
    }
    else if( m_parent && m_parent->Type() == SCH_SHEET_T )
    {
        if( m_id >= 0 && m_id < SHEET_MANDATORY_FIELDS )
            return SCH_SHEET::GetDefaultFieldName( m_id );
        else if( m_name.IsEmpty() && aUseDefaultName )
            return SCH_SHEET::GetDefaultFieldName( m_id );
        else
            return m_name;
    }
    else if( m_parent && m_parent->IsType( { SCH_LABEL_LOCATE_ANY_T } ) )
    {
        return SCH_LABEL_BASE::GetDefaultFieldName( m_name, aUseDefaultName );
    }
    else
    {
        wxFAIL_MSG( "Unhandled field owner type." );
        return m_name;
    }
}


wxString SCH_FIELD::GetCanonicalName() const
{
    if( m_parent && m_parent->Type() == SCH_SYMBOL_T )
    {
        if( m_id < MANDATORY_FIELDS )
            return TEMPLATE_FIELDNAME::GetDefaultFieldName( m_id, false );
        else
            return m_name;
    }
    else if( m_parent && m_parent->Type() == SCH_SHEET_T )
    {
        switch( m_id )
        {
        case  SHEETNAME:     return wxT( "Sheetname" );
        case  SHEETFILENAME: return wxT( "Sheetfile" );
        default:             return m_name;
        }
    }
    else if( m_parent && m_parent->IsType( { SCH_LABEL_LOCATE_ANY_T } ) )
    {
        // These should be stored in canonical format, but just in case:
        if( m_name == _( "Net Class" ) || m_name == wxT( "Net Class" ) )
            return wxT( "Netclass" );
        else if( m_name == _( "Sheet References" )
                 || m_name == wxT( "Sheet References" )
                 || m_name == wxT( "Intersheet References" ) )
            return wxT( "Intersheetrefs" );
        else
            return m_name;
    }
    else
    {
        if( m_parent )
        {
            wxFAIL_MSG( wxString::Format( "Unhandled field owner type (id %d, parent type %d).",
                                           m_id, m_parent->Type() ) );
        }

        return m_name;
    }
}


BITMAPS SCH_FIELD::GetMenuImage() const
{
    if( m_parent && m_parent->Type() == SCH_SYMBOL_T )
    {
        switch( m_id )
        {
        case REFERENCE_FIELD: return BITMAPS::edit_comp_ref;
        case VALUE_FIELD:     return BITMAPS::edit_comp_value;
        case FOOTPRINT_FIELD: return BITMAPS::edit_comp_footprint;
        default:              return BITMAPS::text;
        }
    }

    return BITMAPS::text;
}


bool SCH_FIELD::HitTest( const VECTOR2I& aPosition, int aAccuracy ) const
{
    // Do not hit test hidden or empty fields.
    if( !IsVisible() || GetShownText( true ).IsEmpty() )
        return false;

    BOX2I rect = GetBoundingBox();

    rect.Inflate( aAccuracy );

    if( GetParent() && GetParent()->Type() == SCH_GLOBAL_LABEL_T )
    {
        SCH_GLOBALLABEL* label = static_cast<SCH_GLOBALLABEL*>( GetParent() );
        rect.Offset( label->GetSchematicTextOffset( nullptr ) );
    }

    return rect.Contains( aPosition );
}


bool SCH_FIELD::HitTest( const BOX2I& aRect, bool aContained, int aAccuracy ) const
{
    // Do not hit test hidden fields.
    if( !IsVisible() || GetShownText( true ).IsEmpty() )
        return false;

    BOX2I rect = aRect;

    rect.Inflate( aAccuracy );

    if( GetParent() && GetParent()->Type() == SCH_GLOBAL_LABEL_T )
    {
        SCH_GLOBALLABEL* label = static_cast<SCH_GLOBALLABEL*>( GetParent() );
        rect.Offset( label->GetSchematicTextOffset( nullptr ) );
    }

    if( aContained )
        return rect.Contains( GetBoundingBox() );

    return rect.Intersects( GetBoundingBox() );
}


void SCH_FIELD::Plot( PLOTTER* aPlotter, bool aBackground ) const
{
    if( GetShownText( true ).IsEmpty() || aBackground )
        return;

    RENDER_SETTINGS* settings = aPlotter->RenderSettings();
    COLOR4D          color = settings->GetLayerColor( GetLayer() );
    int              penWidth = GetEffectiveTextPenWidth( settings->GetDefaultPenWidth() );

    COLOR4D bg = settings->GetBackgroundColor();;

    if( bg == COLOR4D::UNSPECIFIED || !aPlotter->GetColorMode() )
        bg = COLOR4D::WHITE;

    if( aPlotter->GetColorMode() && GetTextColor() != COLOR4D::UNSPECIFIED )
        color = GetTextColor();

    penWidth = std::max( penWidth, settings->GetMinPenWidth() );

    // clamp the pen width to be sure the text is readable
    penWidth = std::min( penWidth, std::min( GetTextSize().x, GetTextSize().y ) / 4 );

    if( !IsVisible() )
        return;

    // Calculate the text orientation, according to the symbol orientation/mirror
    EDA_ANGLE         orient = GetTextAngle();
    VECTOR2I          textpos = GetTextPos();
    GR_TEXT_H_ALIGN_T hjustify = GetHorizJustify();
    GR_TEXT_V_ALIGN_T vjustify = GetVertJustify();

    if( m_parent && m_parent->Type() == SCH_SYMBOL_T )
    {
        SCH_SYMBOL* parentSymbol = static_cast<SCH_SYMBOL*>( m_parent );

        if( parentSymbol->GetDNP() )
        {
            color.Desaturate();
            color = color.Mix( bg, 0.5f );
        }

        if( parentSymbol->GetTransform().y1 )  // Rotate symbol 90 deg.
        {
            if( orient.IsHorizontal() )
                orient = ANGLE_VERTICAL;
            else
                orient = ANGLE_HORIZONTAL;
        }

        /*
         * Calculate the text justification, according to the symbol orientation/mirror.  This is
         * a bit complicated due to cumulative calculations:
         *  - numerous cases (mirrored or not, rotation)
         *  - the plotter's Text() function will also recalculate H and V justifications according
         *    to the text orientation
         *  - when a symbol is mirrored the text is not, and justifications become a nightmare
         *
         *  So the easier way is to use no justifications (centered text) and use GetBoundingBox to
         *  know the text coordinate considered as centered.
         */
        hjustify = GR_TEXT_H_ALIGN_CENTER;
        vjustify = GR_TEXT_V_ALIGN_CENTER;
        textpos = GetBoundingBox().Centre();
    }
    else if( m_parent && m_parent->Type() == SCH_GLOBAL_LABEL_T )
    {
        SCH_GLOBALLABEL* label = static_cast<SCH_GLOBALLABEL*>( m_parent );
        textpos += label->GetSchematicTextOffset( settings );
    }

    KIFONT::FONT* font = GetFont();

    if( !font )
        font = KIFONT::FONT::GetFont( settings->GetDefaultFont(), IsBold(), IsItalic() );

    TEXT_ATTRIBUTES attrs = GetAttributes();
    attrs.m_StrokeWidth = penWidth;
    attrs.m_Halign = hjustify;
    attrs.m_Valign = vjustify;
    attrs.m_Angle = orient;
    attrs.m_Multiline = false;

    aPlotter->PlotText( textpos, color, GetShownText( true ), attrs, font );

    if( IsHypertext() )
    {
        SCH_LABEL_BASE*                            label = static_cast<SCH_LABEL_BASE*>( m_parent );
        std::vector<std::pair<wxString, wxString>> pages;
        std::vector<wxString>                      pageHrefs;
        BOX2I                                      bbox = GetBoundingBox();

        wxCHECK( label, /* void */ );

        label->GetIntersheetRefs( &pages );

        for( const std::pair<wxString, wxString>& page : pages )
            pageHrefs.push_back( wxT( "#" ) + page.first );

        bbox.Offset( label->GetSchematicTextOffset( settings ) );

        aPlotter->HyperlinkMenu( bbox, pageHrefs );
    }
}


void SCH_FIELD::SetPosition( const VECTOR2I& aPosition )
{
    // Actual positions are calculated by the rotation/mirror transform of the parent symbol
    // of the field.  The inverse transform is used to calculate the position relative to the
    // parent symbol.
    if( m_parent && m_parent->Type() == SCH_SYMBOL_T )
    {
        SCH_SYMBOL* parentSymbol = static_cast<SCH_SYMBOL*>( m_parent );
        VECTOR2I    relPos = aPosition - parentSymbol->GetPosition();

        relPos = parentSymbol->GetTransform().InverseTransform().TransformCoordinate( relPos );

        SetTextPos( relPos + parentSymbol->GetPosition() );
        return;
    }

    SetTextPos( aPosition );
}


VECTOR2I SCH_FIELD::GetPosition() const
{
    if( m_parent && m_parent->Type() == SCH_SYMBOL_T )
    {
        SCH_SYMBOL* parentSymbol = static_cast<SCH_SYMBOL*>( m_parent );
        VECTOR2I    relativePos = GetTextPos() - parentSymbol->GetPosition();

        relativePos = parentSymbol->GetTransform().TransformCoordinate( relativePos );

        return relativePos + parentSymbol->GetPosition();
    }

    return GetTextPos();
}


VECTOR2I SCH_FIELD::GetParentPosition() const
{
    return m_parent ? m_parent->GetPosition() : VECTOR2I( 0, 0 );
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


static struct SCH_FIELD_DESC
{
    SCH_FIELD_DESC()
    {
        PROPERTY_MANAGER& propMgr = PROPERTY_MANAGER::Instance();
        REGISTER_TYPE( SCH_FIELD );
        propMgr.AddTypeCast( new TYPE_CAST<SCH_FIELD, SCH_ITEM> );
        propMgr.AddTypeCast( new TYPE_CAST<SCH_FIELD, EDA_TEXT> );
        propMgr.InheritsAfter( TYPE_HASH( SCH_FIELD ), TYPE_HASH( SCH_ITEM ) );
        propMgr.InheritsAfter( TYPE_HASH( SCH_FIELD ), TYPE_HASH( EDA_TEXT ) );

        propMgr.AddProperty( new PROPERTY<SCH_FIELD, bool>( _HKI( "Show Field Name" ),
                &SCH_FIELD::SetNameShown, &SCH_FIELD::IsNameShown ) );

        propMgr.AddProperty( new PROPERTY<SCH_FIELD, bool>( _HKI( "Allow Autoplacement" ),
                &SCH_FIELD::SetCanAutoplace, &SCH_FIELD::CanAutoplace ) );

        propMgr.Mask( TYPE_HASH( SCH_FIELD ), TYPE_HASH( EDA_TEXT ), _HKI( "Hyperlink" ) );
        propMgr.Mask( TYPE_HASH( SCH_FIELD ), TYPE_HASH( EDA_TEXT ), _HKI( "Thickness" ) );
        propMgr.Mask( TYPE_HASH( SCH_FIELD ), TYPE_HASH( EDA_TEXT ), _HKI( "Mirrored" ) );
        propMgr.Mask( TYPE_HASH( SCH_FIELD ), TYPE_HASH( EDA_TEXT ), _HKI( "Visible" ) );
        propMgr.Mask( TYPE_HASH( SCH_FIELD ), TYPE_HASH( EDA_TEXT ), _HKI( "Width" ) );
        propMgr.Mask( TYPE_HASH( SCH_FIELD ), TYPE_HASH( EDA_TEXT ), _HKI( "Height" ) );

        propMgr.Mask( TYPE_HASH( SCH_FIELD ), TYPE_HASH( EDA_TEXT ), _HKI( "Orientation" ) );

        auto isNotNamedVariable =
                []( INSPECTABLE* aItem ) -> bool
                {
                    if( SCH_FIELD* field = dynamic_cast<SCH_FIELD*>( aItem ) )
                        return !field->IsNamedVariable();

                    return true;
                };

        propMgr.OverrideWriteability( TYPE_HASH( SCH_FIELD ), TYPE_HASH( EDA_TEXT ), _HKI( "Text" ),
                                      isNotNamedVariable );
    }
} _SCH_FIELD_DESC;
