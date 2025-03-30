/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2015 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
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

#include <wx/log.h>
#include <wx/menu.h>

#include <advanced_config.h>
#include <base_units.h>
#include <common.h>     // for ExpandTextVars
#include <ctl_flags.h>
#include <sch_edit_frame.h>
#include <plotters/plotter.h>
#include <richio.h>
#include <io/kicad/kicad_io_utils.h>
#include <bitmaps.h>
#include <kiway.h>
#include <symbol_library.h>
#include <settings/color_settings.h>
#include <string_utils.h>
#include <trace_helpers.h>
#include <tool/tool_manager.h>
#include <tools/sch_navigate_tool.h>
#include <font/outline_font.h>
#include "sim/sim_lib_mgr.h"

static const std::vector<KICAD_T> labelTypes = { SCH_LABEL_LOCATE_ANY_T };


SCH_FIELD::SCH_FIELD( const VECTOR2I& aPos, int aFieldId, SCH_ITEM* aParent,
                      const wxString& aName ) :
        SCH_ITEM( aParent, SCH_FIELD_T ),
        EDA_TEXT( schIUScale, wxEmptyString ),
        m_id( 0 ),
        m_showName( false ),
        m_allowAutoPlace( true ),
        m_isNamedVariable( false ),
        m_autoAdded( false ),
        m_showInChooser( true ),
        m_renderCacheValid( false ),
        m_lastResolvedColor( COLOR4D::UNSPECIFIED )
{
    if( !aName.IsEmpty() )
        SetName( aName );
    else if( aParent && aParent->Type() == SCH_SYMBOL_T )
        SetName( GetDefaultFieldName( aFieldId, DO_TRANSLATE ) );
    else if( aParent && aParent->Type() == SCH_SHEET_T )
        SetName( SCH_SHEET::GetDefaultFieldName( aFieldId, DO_TRANSLATE ) );

    SetTextPos( aPos );
    SetId( aFieldId );  // will also set the layer
    SetVisible( true );
}


SCH_FIELD::SCH_FIELD( SCH_ITEM* aParent, int aFieldId, const wxString& aName ) :
        SCH_FIELD( VECTOR2I(), aFieldId, aParent, aName )
{
}


SCH_FIELD::SCH_FIELD( SCH_TEXT* aText ) :
        SCH_FIELD( VECTOR2I(), INVALID_FIELD, nullptr, wxEmptyString )
{
    SCH_ITEM::operator=( *aText );
    EDA_TEXT::operator=( *aText );
}


SCH_FIELD::SCH_FIELD( const SCH_FIELD& aField ) :
        SCH_ITEM( aField ),
        EDA_TEXT( aField )
{
    m_private         = aField.m_private;
    SetId( aField.m_id );  // will also set the layer
    m_name            = aField.m_name;
    m_showName        = aField.m_showName;
    m_allowAutoPlace  = aField.m_allowAutoPlace;
    m_isNamedVariable = aField.m_isNamedVariable;
    m_autoAdded       = aField.m_autoAdded;
    m_showInChooser   = aField.m_showInChooser;

    m_renderCache.clear();

    for( const std::unique_ptr<KIFONT::GLYPH>& glyph : aField.m_renderCache )
    {
        if( KIFONT::OUTLINE_GLYPH* outline = dynamic_cast<KIFONT::OUTLINE_GLYPH*>( glyph.get() ) )
            m_renderCache.emplace_back( std::make_unique<KIFONT::OUTLINE_GLYPH>( *outline ) );
        else if( KIFONT::STROKE_GLYPH* stroke = dynamic_cast<KIFONT::STROKE_GLYPH*>( glyph.get() ) )
            m_renderCache.emplace_back( std::make_unique<KIFONT::STROKE_GLYPH>( *stroke ) );
    }

    m_renderCacheValid = aField.m_renderCacheValid;
    m_renderCachePos = aField.m_renderCachePos;

    m_lastResolvedColor = aField.m_lastResolvedColor;
}


SCH_FIELD& SCH_FIELD::operator=( const SCH_FIELD& aField )
{
    EDA_TEXT::operator=( aField );

    m_private         = aField.m_private;
    SetId( aField.m_id );  // will also set the layer
    m_name            = aField.m_name;
    m_showName        = aField.m_showName;
    m_allowAutoPlace  = aField.m_allowAutoPlace;
    m_isNamedVariable = aField.m_isNamedVariable;

    m_renderCache.clear();

    for( const std::unique_ptr<KIFONT::GLYPH>& glyph : aField.m_renderCache )
    {
        if( KIFONT::OUTLINE_GLYPH* outline = dynamic_cast<KIFONT::OUTLINE_GLYPH*>( glyph.get() ) )
            m_renderCache.emplace_back( std::make_unique<KIFONT::OUTLINE_GLYPH>( *outline ) );
        else if( KIFONT::STROKE_GLYPH* stroke = dynamic_cast<KIFONT::STROKE_GLYPH*>( glyph.get() ) )
            m_renderCache.emplace_back( std::make_unique<KIFONT::STROKE_GLYPH>( *stroke ) );
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


void SCH_FIELD::Copy( SCH_FIELD* aTarget ) const
{
    *aTarget = *this;
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
    else if( m_parent && ( m_parent->Type() == SCH_SYMBOL_T || m_parent->Type() == LIB_SYMBOL_T ) )
    {
        switch( m_id )
        {
        case REFERENCE_FIELD: SetLayer( LAYER_REFERENCEPART ); break;
        case VALUE_FIELD:     SetLayer( LAYER_VALUEPART );     break;
        default:              SetLayer( LAYER_FIELDS );        break;
        }
    }
    else if( m_parent && m_parent->IsType( labelTypes ) )
    {
        // We can't use defined IDs for labels because there can be multiple net class
        // assignments.

        if( GetCanonicalName() == wxT( "Netclass" )
            || GetCanonicalName() == wxT( "Component Class" ) )
        {
            SetLayer( LAYER_NETCLASS_REFS );
        }
        else if( GetCanonicalName() == wxT( "Intersheetrefs" ) )
        {
            SetLayer( LAYER_INTERSHEET_REFS );
        }
        else
        {
            SetLayer( LAYER_FIELDS );
        }
    }
}


wxString SCH_FIELD::GetShownName() const
{
    return m_isNamedVariable ? GetTextVars( GetName() ) : GetName();
}


wxString SCH_FIELD::GetShownText( const SCH_SHEET_PATH* aPath, bool aAllowExtraText,
                                  int aDepth ) const
{
    std::function<bool( wxString* )> libSymbolResolver =
            [&]( wxString* token ) -> bool
            {
                LIB_SYMBOL* symbol = static_cast<LIB_SYMBOL*>( m_parent );
                return symbol->ResolveTextVar( token, aDepth + 1 );
            };

    std::function<bool( wxString* )> symbolResolver =
            [&]( wxString* token ) -> bool
            {
                SCH_SYMBOL* symbol = static_cast<SCH_SYMBOL*>( m_parent );
                return symbol->ResolveTextVar( aPath, token, aDepth + 1 );
            };

    std::function<bool( wxString* )> schematicResolver =
            [&]( wxString* token ) -> bool
            {
                if( !aPath )
                    return false;

                if( SCHEMATIC* schematic = Schematic() )
                    return schematic->ResolveTextVar( aPath, token, aDepth + 1 );

                return false;
            };

    std::function<bool( wxString* )> sheetResolver =
            [&]( wxString* token ) -> bool
            {
                if( !aPath )
                    return false;

                SCH_SHEET* sheet = static_cast<SCH_SHEET*>( m_parent );

                SCHEMATIC* schematic = Schematic();
                SCH_SHEET_PATH path = *aPath;
                path.push_back( sheet );

                bool retval = sheet->ResolveTextVar( &path, token, aDepth + 1 );

                if( schematic )
                    retval |= schematic->ResolveTextVar( &path, token, aDepth + 1 );

                return retval;
            };

    std::function<bool( wxString* )> labelResolver =
            [&]( wxString* token ) -> bool
            {
                if( !aPath )
                    return false;

                SCH_LABEL_BASE* label = static_cast<SCH_LABEL_BASE*>( m_parent );
                return label->ResolveTextVar( aPath, token, aDepth + 1 );
            };

    wxString text = EDA_TEXT::GetShownText( aAllowExtraText, aDepth );

    if( IsNameShown() && aAllowExtraText )
        text = GetShownName() << wxS( ": " ) << text;

    if( text == wxS( "~" ) ) // Legacy placeholder for empty string
        text = wxS( "" );

    // The iteration here it to allow for nested variables in the
    // text strings (e.g. ${${VAR}}).  Although the symbols and sheets
    // and labels recurse, text that is none of those types such as text
    // boxes and labels do not.  This only loops if there is still a
    // variable to resolve.
    for( int ii = aDepth;
         ii < ADVANCED_CFG::GetCfg().m_ResolveTextRecursionDepth && text.Contains( wxT( "${" ) );
         ++ii )
    {
        if( m_parent && m_parent->Type() == LIB_SYMBOL_T )
            text = ExpandTextVars( text, &libSymbolResolver );
        else if( m_parent && m_parent->Type() == SCH_SYMBOL_T )
            text = ExpandTextVars( text, &symbolResolver );
        else if( m_parent && m_parent->Type() == SCH_SHEET_T )
            text = ExpandTextVars( text, &sheetResolver );
        else if( m_parent && m_parent->IsType( labelTypes ) )
            text = ExpandTextVars( text, &labelResolver );
        else if( Schematic() )
        {
            text = ExpandTextVars( text, &Schematic()->Prj() );
            text = ExpandTextVars( text, &schematicResolver );
        }
    }

    // WARNING: the IDs of FIELDS and SHEETS overlap, so one must check *both* the
    // id and the parent's type.

    if( m_parent && m_parent->Type() == SCH_SYMBOL_T )
    {
        SCH_SYMBOL* parentSymbol = static_cast<SCH_SYMBOL*>( m_parent );

        if( m_id == REFERENCE_FIELD && aPath )
        {
            // For more than one part per package, we must add the part selection
            // A, B, ... or 1, 2, .. to the reference.
            if( parentSymbol->GetUnitCount() > 1 )
                text << parentSymbol->SubReference( parentSymbol->GetUnitSelection( aPath ) );
        }
    }
    else if( m_parent && m_parent->Type() == SCH_SHEET_T )
    {
        if( m_id == SHEETFILENAME && aAllowExtraText && !IsNameShown() )
            text = _( "File:" ) + wxS( " " ) + text;
    }

    return text;
}


wxString SCH_FIELD::GetShownText( bool aAllowExtraText, int aDepth ) const
{
    if( SCHEMATIC* schematic = Schematic() )
        return GetShownText( &schematic->CurrentSheet(), aAllowExtraText, aDepth );
    else
        return GetShownText( nullptr, aAllowExtraText, aDepth );
}


wxString SCH_FIELD::GetFullText( int unit ) const
{
    if( GetId() != REFERENCE_FIELD )
        return GetText();

    wxString text = GetText();
    text << wxT( "?" );

    if( GetParentSymbol() && GetParentSymbol()->IsMulti() )
        text << LIB_SYMBOL::LetterSubReference( unit, 'A' );

    return text;
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

            outlineFont->GetLinesAsGlyphs( &m_renderCache, forResolvedText, forPosition, aAttrs,
                                           GetFontMetrics() );

            m_renderCachePos = forPosition;
            m_renderCacheValid = true;
        }

        if( m_renderCachePos != forPosition )
        {
            VECTOR2I delta = forPosition - m_renderCachePos;

            for( std::unique_ptr<KIFONT::GLYPH>& glyph : m_renderCache )
            {
                if( glyph->IsOutline() )
                    static_cast<KIFONT::OUTLINE_GLYPH*>( glyph.get() )->Move( delta );
                else
                    static_cast<KIFONT::STROKE_GLYPH*>( glyph.get() )->Move( delta );
            }

            m_renderCachePos = forPosition;
        }

        return &m_renderCache;
    }

    return nullptr;
}


void SCH_FIELD::Print( const SCH_RENDER_SETTINGS* aSettings, int aUnit, int aBodyStyle,
                       const VECTOR2I& aOffset, bool aForceNoFill, bool aDimmed )
{
    wxString text;

    if( Schematic() )
        text = GetShownText( &Schematic()->CurrentSheet(), true );
    else
        text = GetShownText( true );

    if( ( !IsVisible() && !IsForceVisible() ) || text.IsEmpty() )
        return;

    wxDC*   DC = aSettings->GetPrintDC();
    COLOR4D color = aSettings->GetLayerColor( IsForceVisible() ? LAYER_HIDDEN : m_layer );
    bool    blackAndWhiteMode = GetGRForceBlackPenState();
    int     penWidth = GetEffectiveTextPenWidth( aSettings->GetDefaultPenWidth() );

    COLOR4D bg = aSettings->GetBackgroundColor();

    if( bg == COLOR4D::UNSPECIFIED || GetGRForceBlackPenState() )
        bg = COLOR4D::WHITE;

    if( IsForceVisible() )
        bg = aSettings->GetLayerColor( LAYER_HIDDEN );

    if( !blackAndWhiteMode && GetTextColor() != COLOR4D::UNSPECIFIED )
        color = GetTextColor();

    if( aDimmed )
    {
        color.Desaturate( );
        color = color.Mix( bg, 0.5f );
    }

    // Calculate the text orientation according to the symbol orientation.
    EDA_ANGLE         orient = GetTextAngle();
    VECTOR2I          textpos = GetTextPos();
    GR_TEXT_H_ALIGN_T hjustify = GetHorizJustify();
    GR_TEXT_V_ALIGN_T vjustify = GetVertJustify();
    KIFONT::FONT*     font = GetFont();

    if( !font )
        font = KIFONT::FONT::GetFont( aSettings->GetDefaultFont(), IsBold(), IsItalic() );

    if( m_parent && m_parent->Type() == LIB_SYMBOL_T )
    {
        textpos = aSettings->TransformCoordinate( GetTextPos() ) + aOffset;
    }
    else if( m_parent && m_parent->Type() == SCH_SYMBOL_T )
    {
        /*
         * Calculate the text justification, according to the symbol orientation/mirror.
         * This is a bit complicated due to cumulative calculations:
         * - numerous cases (mirrored or not, rotation)
         * - the GRText function will also recalculate H and V justifications according to the
         *   text orientation.
         * - when symbol is mirrored, the text is not mirrored and justifications are complicated
         *   to calculate so the easier way is to use no justifications (centered text) and use
         *   GetBoundingBox to know the text coordinate considered as centered
         */
        hjustify = GR_TEXT_H_ALIGN_CENTER;
        vjustify = GR_TEXT_V_ALIGN_CENTER;
        textpos = GetBoundingBox().Centre() + aOffset;

        if( aSettings->m_Transform.y1 )  // Rotate symbol 90 degrees.
        {
            if( orient == ANGLE_HORIZONTAL )
                orient = ANGLE_VERTICAL;
            else
                orient = ANGLE_HORIZONTAL;
        }
    }
    else if( m_parent && m_parent->Type() == SCH_GLOBAL_LABEL_T )
    {
        SCH_GLOBALLABEL* label = static_cast<SCH_GLOBALLABEL*>( GetParent() );
        textpos += label->GetSchematicTextOffset( aSettings );
    }

    GRPrintText( DC, textpos, color, text, orient, GetTextSize(), hjustify, vjustify, penWidth,
                 IsItalic(), IsBold(), font, GetFontMetrics() );
}


// The following is taken from EDA_TEXT::Format because after 9.0.0, the `(hide yes)` property
// was removed from EDA_TEXT but not EDA_FIELD, but to preserve compatibility with 9.0.0, it
// needs to be written inside the effects block.
void SCH_FIELD::Format( OUTPUTFORMATTER* aFormatter, int aControlBits ) const
{
    aFormatter->Print( "(effects" );

    aFormatter->Print( "(font" );

    if( GetFont() && !GetFont()->GetName().IsEmpty() )
        aFormatter->Print( "(face %s)", aFormatter->Quotew( GetFont()->NameAsToken() ).c_str() );

    // Text size
    aFormatter->Print( "(size %s %s)",
                       EDA_UNIT_UTILS::FormatInternalUnits( m_IuScale, GetTextHeight() ).c_str(),
                       EDA_UNIT_UTILS::FormatInternalUnits( m_IuScale, GetTextWidth() ).c_str() );

    if( GetLineSpacing() != 1.0 )
    {
        aFormatter->Print( "(line_spacing %s)",
                           FormatDouble2Str( GetLineSpacing() ).c_str() );
    }

    if( GetTextThickness() )
    {
        aFormatter->Print( "(thickness %s)",
                EDA_UNIT_UTILS::FormatInternalUnits( m_IuScale, GetTextThickness() ).c_str() );
    }

    if( IsBold() )
        KICAD_FORMAT::FormatBool( aFormatter, "bold", true );

    if( IsItalic() )
        KICAD_FORMAT::FormatBool( aFormatter, "italic", true );

    if( !( aControlBits & CTL_OMIT_COLOR ) && GetTextColor() != COLOR4D::UNSPECIFIED )
    {
        aFormatter->Print( "(color %d %d %d %s)",
                           KiROUND( GetTextColor().r * 255.0 ),
                           KiROUND( GetTextColor().g * 255.0 ),
                           KiROUND( GetTextColor().b * 255.0 ),
                           FormatDouble2Str( GetTextColor().a ).c_str() );
    }

    aFormatter->Print( ")"); // (font

    if( IsMirrored() || GetHorizJustify() != GR_TEXT_H_ALIGN_CENTER
                     || GetVertJustify() != GR_TEXT_V_ALIGN_CENTER )
    {
        aFormatter->Print( "(justify");

        if( GetHorizJustify() != GR_TEXT_H_ALIGN_CENTER )
            aFormatter->Print( GetHorizJustify() == GR_TEXT_H_ALIGN_LEFT ? " left" : " right" );

        if( GetVertJustify() != GR_TEXT_V_ALIGN_CENTER )
            aFormatter->Print( GetVertJustify() == GR_TEXT_V_ALIGN_TOP ? " top" : " bottom" );

        if( IsMirrored() )
            aFormatter->Print( " mirror" );

        aFormatter->Print( ")" ); // (justify
    }

    // The relevant difference from EDA_TEXT::Format
    if( !IsVisible() )
        KICAD_FORMAT::FormatBool( aFormatter, "hide", true );

    if( !( aControlBits & CTL_OMIT_HYPERLINK ) && HasHyperlink() )
        aFormatter->Print( "(href %s)", aFormatter->Quotew( GetHyperlink() ).c_str() );

    aFormatter->Print( ")" ); // (effects
}


bool SCH_FIELD::IsDefaultFormatting() const
{
    return ( IsVisible()
             && EDA_TEXT::IsDefaultFormatting()
           );
}


void SCH_FIELD::ImportValues( const SCH_FIELD& aSource )
{
    SetAttributes( aSource );
    SetVisible( aSource.IsVisible() );
    SetNameShown( aSource.IsNameShown() );
    SetCanAutoplace( aSource.CanAutoplace() );
}


void SCH_FIELD::SwapData( SCH_ITEM* aItem )
{
    wxCHECK_RET( aItem && aItem->Type() == SCH_FIELD_T, wxT( "Cannot swap with invalid item." ) );

    SCH_ITEM::SwapFlags( aItem );

    SCH_FIELD* item = static_cast<SCH_FIELD*>( aItem );

    std::swap( m_layer, item->m_layer );
    std::swap( m_showName, item->m_showName );
    std::swap( m_allowAutoPlace, item->m_allowAutoPlace );
    std::swap( m_isNamedVariable, item->m_isNamedVariable );
    std::swap( m_private, item->m_private );
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


std::vector<int> SCH_FIELD::ViewGetLayers() const
{
    return { GetDefaultLayer(), LAYER_SELECTION_SHADOWS };
}


SCH_LAYER_ID SCH_FIELD::GetDefaultLayer() const
{
    if( m_parent && ( m_parent->Type() == LIB_SYMBOL_T || m_parent->Type() == SCH_SYMBOL_T ) )
    {
        if( m_id == REFERENCE_FIELD )
            return LAYER_REFERENCEPART;
        else if( m_id == VALUE_FIELD )
            return LAYER_VALUEPART;
    }
    else if( m_parent && m_parent->Type() == SCH_SHEET_T )
    {
        if( m_id == SHEETNAME )
            return LAYER_SHEETNAME;
        else if( m_id == SHEETFILENAME )
            return LAYER_SHEETFILENAME;
        else
            return LAYER_SHEETFIELDS;
    }
    else if( m_parent && m_parent->Type() == SCH_LABEL_T )
    {
        if( GetCanonicalName() == wxT( "Netclass" ) )
            return LAYER_NETCLASS_REFS;
    }

    return LAYER_FIELDS;
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
        transform = static_cast<SCH_SYMBOL*>( m_parent )->GetTransform();

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


void SCH_FIELD::SetEffectiveHorizJustify( GR_TEXT_H_ALIGN_T aJustify )
{
    GR_TEXT_H_ALIGN_T actualJustify;

    switch( aJustify )
    {
    case GR_TEXT_H_ALIGN_LEFT:
        actualJustify = IsHorizJustifyFlipped() ? GR_TEXT_H_ALIGN_RIGHT : GR_TEXT_H_ALIGN_LEFT;
        break;
    case GR_TEXT_H_ALIGN_RIGHT:
        actualJustify = IsHorizJustifyFlipped() ? GR_TEXT_H_ALIGN_LEFT : GR_TEXT_H_ALIGN_RIGHT;
        break;
    default:
        actualJustify = aJustify;
    }

    SetHorizJustify( actualJustify );
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


void SCH_FIELD::SetEffectiveVertJustify( GR_TEXT_V_ALIGN_T aJustify )
{
    GR_TEXT_V_ALIGN_T actualJustify;

    switch( aJustify )
    {
    case GR_TEXT_V_ALIGN_TOP:
        actualJustify = IsVertJustifyFlipped() ? GR_TEXT_V_ALIGN_BOTTOM : GR_TEXT_V_ALIGN_TOP;
        break;
    case GR_TEXT_V_ALIGN_BOTTOM:
        actualJustify = IsVertJustifyFlipped() ? GR_TEXT_V_ALIGN_TOP : GR_TEXT_V_ALIGN_BOTTOM;
        break;
    default:
        actualJustify = aJustify;
    }

    SetVertJustify( actualJustify );
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
        // downcast
        const SCH_SEARCH_DATA& schSearchData = dynamic_cast<const SCH_SEARCH_DATA&>( aSearchData );
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
            SCH_SHEET_PATH* sheet = (SCH_SHEET_PATH*) aAuxData;
            text = parentSymbol->GetRef( sheet );

            if( SCH_ITEM::Matches( text, aSearchData ) )
                return true;

            if( parentSymbol->GetUnitCount() > 1 )
                text << parentSymbol->SubReference( parentSymbol->GetUnitSelection( sheet ) );
        }
    }

    return SCH_ITEM::Matches( text, aSearchData );
}


void SCH_FIELD::OnScintillaCharAdded( SCINTILLA_TRICKS* aScintillaTricks,
                                      wxStyledTextEvent &aEvent ) const
{
    SCH_ITEM*  parent = dynamic_cast<SCH_ITEM*>( GetParent() );
    SCHEMATIC* schematic = parent ? parent->Schematic() : nullptr;

    if( !schematic )
        return;

    wxStyledTextCtrl* scintilla = aScintillaTricks->Scintilla();
    int               key = aEvent.GetKey();

    wxArrayString autocompleteTokens;
    int           pos = scintilla->GetCurrentPos();
    int           start = scintilla->WordStartPosition( pos, true );
    wxString      partial;

    // Multi-line fields are not allowed. So remove '\n' if entered.
    if( key == '\n' )
    {
        wxString text = scintilla->GetText();
        int currpos = scintilla->GetCurrentPos();
        text.Replace( wxS( "\n" ), wxS( "" ) );
        scintilla->SetText( text );
        scintilla->GotoPos( currpos-1 );
        return;
    }

    auto textVarRef =
            [&]( int pt )
            {
                return pt >= 2
                        && scintilla->GetCharAt( pt - 2 ) == '$'
                        && scintilla->GetCharAt( pt - 1 ) == '{';
            };

    // Check for cross-reference
    if( start > 1 && scintilla->GetCharAt( start - 1 ) == ':' )
    {
        int refStart = scintilla->WordStartPosition( start - 1, true );

        if( textVarRef( refStart ) )
        {
            partial = scintilla->GetRange( start, pos );

            wxString ref = scintilla->GetRange( refStart, start - 1 );

            if( ref == wxS( "OP" ) )
            {
                // SPICE operating points use ':' syntax for ports
                if( SCH_SYMBOL* symbol = dynamic_cast<SCH_SYMBOL*>( parent ) )
                {
                    NULL_REPORTER   devnull;
                    SCH_SHEET_PATH& sheet = schematic->CurrentSheet();
                    SIM_LIB_MGR     mgr( &schematic->Prj(), schematic );
                    SIM_MODEL&      model = mgr.CreateModel( &sheet, *symbol, devnull ).model;

                    for( wxString pin : model.GetPinNames() )
                    {
                        if( pin.StartsWith( '<' ) && pin.EndsWith( '>' ) )
                            autocompleteTokens.push_back( pin.Mid( 1, pin.Length() - 2 ) );
                        else
                            autocompleteTokens.push_back( pin );
                    }
                }
            }
            else
            {
                SCH_REFERENCE_LIST refs;
                SCH_SYMBOL*        refSymbol = nullptr;

                schematic->Hierarchy().GetSymbols( refs );

                for( size_t jj = 0; jj < refs.GetCount(); jj++ )
                {
                    if( refs[ jj ].GetSymbol()->GetRef( &refs[ jj ].GetSheetPath(), true ) == ref )
                    {
                        refSymbol = refs[ jj ].GetSymbol();
                        break;
                    }
                }

                if( refSymbol )
                    refSymbol->GetContextualTextVars( &autocompleteTokens );
            }
        }
    }
    else if( textVarRef( start ) )
    {
        partial = scintilla->GetTextRange( start, pos );

        SCH_SYMBOL*     symbol = dynamic_cast<SCH_SYMBOL*>( parent );
        SCH_SHEET*      sheet = dynamic_cast<SCH_SHEET*>( parent );
        SCH_LABEL_BASE* label = dynamic_cast<SCH_LABEL_BASE*>( parent );

        if( symbol )
        {
            symbol->GetContextualTextVars( &autocompleteTokens );

            if( schematic->CurrentSheet().Last() )
                schematic->CurrentSheet().Last()->GetContextualTextVars( &autocompleteTokens );
        }

        if( sheet )
            sheet->GetContextualTextVars( &autocompleteTokens );

        if( label )
            label->GetContextualTextVars( &autocompleteTokens );

        for( std::pair<wxString, wxString> entry : schematic->Prj().GetTextVars() )
            autocompleteTokens.push_back( entry.first );
    }

    aScintillaTricks->DoAutocomplete( partial, autocompleteTokens );
    scintilla->SetFocus();
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


void SCH_FIELD::Rotate( const VECTOR2I& aCenter, bool aRotateCCW )
{
    const GR_TEXT_H_ALIGN_T horizJustify = GetHorizJustify();

    if( GetTextAngle().IsVertical() )
    {
        switch( horizJustify )
        {
        case GR_TEXT_H_ALIGN_LEFT:
            if( aRotateCCW )
                SetHorizJustify( GR_TEXT_H_ALIGN_RIGHT );

            break;
        case GR_TEXT_H_ALIGN_RIGHT:
            if( aRotateCCW )
                SetHorizJustify( GR_TEXT_H_ALIGN_LEFT );
            break;
        case GR_TEXT_H_ALIGN_CENTER:
        case GR_TEXT_H_ALIGN_INDETERMINATE: break;
        }

        SetTextAngle( ANGLE_HORIZONTAL );
    }
    else if( GetTextAngle().IsHorizontal() )
    {
        switch( horizJustify )
        {
        case GR_TEXT_H_ALIGN_LEFT:
            if( !aRotateCCW )
                SetHorizJustify( GR_TEXT_H_ALIGN_RIGHT );
            break;
        case GR_TEXT_H_ALIGN_RIGHT:
            if( !aRotateCCW )
                SetHorizJustify( GR_TEXT_H_ALIGN_LEFT );
            break;
        case GR_TEXT_H_ALIGN_CENTER:
        case GR_TEXT_H_ALIGN_INDETERMINATE: break;
        }

        SetTextAngle( ANGLE_VERTICAL );
    }
    else
    {
        wxASSERT_MSG(
                false,
                wxString::Format( wxT( "SCH_FIELD text angle is not horizontal or vertical: %d" ),
                                  GetTextAngle().AsDegrees() ) );
    }

    VECTOR2I pt = GetPosition();
    RotatePoint( pt, aCenter, aRotateCCW ? ANGLE_90 : ANGLE_270 );
    SetPosition( pt );
}


void SCH_FIELD::MirrorHorizontally( int aCenter )
{
    int x = GetTextPos().x;

    x -= aCenter;
    x *= -1;
    x += aCenter;

    SetTextX( x );
}


void SCH_FIELD::MirrorVertically( int aCenter )
{
    int y = GetTextPos().y;

    y -= aCenter;
    y *= -1;
    y += aCenter;

    SetTextY( y );
}


void SCH_FIELD::BeginEdit( const VECTOR2I& aPosition )
{
    SetTextPos( aPosition );
}


void SCH_FIELD::CalcEdit( const VECTOR2I& aPosition )
{
    SetTextPos( aPosition );
}


wxString SCH_FIELD::GetItemDescription( UNITS_PROVIDER* aUnitsProvider, bool aFull ) const
{
    return wxString::Format( _( "Field %s '%s'" ),
                             UnescapeString( GetName() ),
                             aFull ? GetShownText( false ) : KIUI::EllipsizeMenuText( GetText() ) );
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
    case GR_TEXT_H_ALIGN_LEFT:          msg = _( "Left" );         break;
    case GR_TEXT_H_ALIGN_CENTER:        msg = _( "Center" );       break;
    case GR_TEXT_H_ALIGN_RIGHT:         msg = _( "Right" );        break;
    case GR_TEXT_H_ALIGN_INDETERMINATE: msg = INDETERMINATE_STATE; break;
    }

    aList.emplace_back( _( "H Justification" ), msg );

    switch ( GetVertJustify() )
    {
    case GR_TEXT_V_ALIGN_TOP:           msg = _( "Top" );          break;
    case GR_TEXT_V_ALIGN_CENTER:        msg = _( "Center" );       break;
    case GR_TEXT_V_ALIGN_BOTTOM:        msg = _( "Bottom" );       break;
    case GR_TEXT_V_ALIGN_INDETERMINATE: msg = INDETERMINATE_STATE; break;
    }

    aList.emplace_back( _( "V Justification" ), msg );
}


void SCH_FIELD::DoHypertextAction( EDA_DRAW_FRAME* aFrame ) const
{
    constexpr int START_ID = 1;

    if( IsHypertext() )
    {
        wxString         href;
        SCH_GLOBALLABEL* global = dynamic_cast<SCH_GLOBALLABEL*>( m_parent );

        if( global && m_id == INTERSHEET_REFS )
        {
            SCH_SHEET_PATH* sheet = &global->Schematic()->CurrentSheet();
            wxMenu          menu;

            std::vector<std::pair<wxString, wxString>> pages;

            global->GetIntersheetRefs( sheet, &pages );

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
        }
        else if( IsURL( GetShownText( false ) ) )
        {
            href = GetShownText( false );
        }

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

    // Mandatory fields should not have leading or trailing whitespace.
    if( IsMandatory() )
        EDA_TEXT::SetText( aText.Strip( wxString::both ) );
    else
        EDA_TEXT::SetText( aText );
}


wxString SCH_FIELD::GetName( bool aUseDefaultName ) const
{
    if( m_parent && ( m_parent->Type() == SCH_SYMBOL_T || m_parent->Type() == LIB_SYMBOL_T ) )
    {
        if( IsMandatory() )
            return GetCanonicalFieldName( m_id );
        else if( m_name.IsEmpty() && aUseDefaultName )
            return GetDefaultFieldName( m_id, !DO_TRANSLATE );
    }
    else if( m_parent && m_parent->Type() == SCH_SHEET_T )
    {
        if( IsMandatory() )
            return SCH_SHEET::GetDefaultFieldName( m_id, !DO_TRANSLATE );
        else if( m_name.IsEmpty() && aUseDefaultName )
            return SCH_SHEET::GetDefaultFieldName( m_id, !DO_TRANSLATE );
    }
    else if( m_parent && m_parent->IsType( labelTypes ) )
    {
        return SCH_LABEL_BASE::GetDefaultFieldName( m_name, aUseDefaultName );
    }

    return m_name;
}


wxString SCH_FIELD::GetCanonicalName() const
{
    if( m_parent && ( m_parent->Type() == SCH_SYMBOL_T || m_parent->Type() == LIB_SYMBOL_T ) )
    {
        if( IsMandatory() )
            return GetCanonicalFieldName( m_id );
    }
    else if( m_parent && m_parent->Type() == SCH_SHEET_T )
    {
        if( IsMandatory() )
            return SCH_SHEET::GetDefaultFieldName( m_id, !DO_TRANSLATE );
    }
    else if( m_parent && m_parent->IsType( labelTypes ) )
    {
        // These should be stored in canonical format, but just in case:
        if( m_name == _( "Net Class" ) || m_name == wxT( "Net Class" ) )
        {
            return wxT( "Netclass" );
        }
        else if( m_name == _( "Sheet References" )
                 || m_name == wxT( "Sheet References" )
                 || m_name == wxT( "Intersheet References" ) )
        {
            return wxT( "Intersheetrefs" );
        }
    }

    return m_name;
}


BITMAPS SCH_FIELD::GetMenuImage() const
{
    if( m_parent && ( m_parent->Type() == SCH_SYMBOL_T || m_parent->Type() == LIB_SYMBOL_T ) )
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
    if( GetShownText( true ).IsEmpty() )
        return false;

    BOX2I rect = GetBoundingBox();

    // Text in symbol editor can have additional chars (ie: reference designators U? or U?A)
    if( m_parent && m_parent->Type() == LIB_SYMBOL_T )
    {
        SCH_FIELD temp( *this );
        temp.SetText( GetFullText() );
        rect = temp.GetBoundingBox();
    }

    rect.Inflate( aAccuracy );

    if( m_parent && m_parent->Type() == SCH_GLOBAL_LABEL_T )
    {
        SCH_GLOBALLABEL* label = static_cast<SCH_GLOBALLABEL*>( GetParent() );
        rect.Offset( label->GetSchematicTextOffset( nullptr ) );
    }

    return rect.Contains( aPosition );
}


bool SCH_FIELD::HitTest( const BOX2I& aRect, bool aContained, int aAccuracy ) const
{
    if( GetShownText( true ).IsEmpty() )
        return false;

    if( m_flags & (STRUCT_DELETED | SKIP_STRUCT ) )
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


void SCH_FIELD::Plot( PLOTTER* aPlotter, bool aBackground, const SCH_PLOT_OPTS& aPlotOpts,
                      int aUnit, int aBodyStyle, const VECTOR2I& aOffset, bool aDimmed )
{
    wxString text;

    if( Schematic() )
        text = GetShownText( &Schematic()->CurrentSheet(), true );
    else
        text = GetShownText( true );

    if( ( !IsVisible() && !IsForceVisible() ) || text.IsEmpty() || aBackground )
        return;

    SCH_RENDER_SETTINGS* renderSettings = getRenderSettings( aPlotter );
    COLOR4D color = renderSettings->GetLayerColor( GetLayer() );
    int penWidth = GetEffectiveTextPenWidth( renderSettings->GetDefaultPenWidth() );

    COLOR4D bg = renderSettings->GetBackgroundColor();;

    if( bg == COLOR4D::UNSPECIFIED || !aPlotter->GetColorMode() )
        bg = COLOR4D::WHITE;

    if( aPlotter->GetColorMode() && GetTextColor() != COLOR4D::UNSPECIFIED )
        color = GetTextColor();

    if( aDimmed )
    {
        color.Desaturate( );
        color = color.Mix( bg, 0.5f );
    }

    penWidth = std::max( penWidth, renderSettings->GetMinPenWidth() );

    // clamp the pen width to be sure the text is readable
    penWidth = std::min( penWidth, std::min( GetTextSize().x, GetTextSize().y ) / 4 );

    if( !IsVisible() && !renderSettings->m_ShowHiddenFields )
        return;

    // Calculate the text orientation, according to the symbol orientation/mirror
    EDA_ANGLE         orient = GetTextAngle();
    VECTOR2I          textpos = GetTextPos();
    GR_TEXT_H_ALIGN_T hjustify = GetHorizJustify();
    GR_TEXT_V_ALIGN_T vjustify = GetVertJustify();

    if( renderSettings->m_Transform.y1 )  // Rotate symbol 90 deg.
    {
        if( orient.IsHorizontal() )
            orient = ANGLE_VERTICAL;
        else
            orient = ANGLE_HORIZONTAL;
    }

    if( m_parent && m_parent->Type() == SCH_SYMBOL_T )
    {
        /*
         * Calculate the text justification, according to the symbol orientation/mirror.  This is
         * a bit complicated due to cumulative calculations:
         *  - numerous cases (mirrored or not, rotation)
         *  - the plotter's Text() function will also recalculate H and V justifications according
         *    to the text orientation
         *  - when a symbol is mirrored the text is not, and justifications become a nightmare
         *
         *  So the easier way is to use no justifications (centered text) and use GetBoundingBox
         *  to know the text coordinate considered as centered.
         */
        hjustify = GR_TEXT_H_ALIGN_CENTER;
        vjustify = GR_TEXT_V_ALIGN_CENTER;
        textpos = GetBoundingBox().Centre();
    }
    else if( m_parent && m_parent->Type() == SCH_GLOBAL_LABEL_T )
    {
        SCH_GLOBALLABEL* label = static_cast<SCH_GLOBALLABEL*>( m_parent );
        textpos += label->GetSchematicTextOffset( renderSettings );
    }

    KIFONT::FONT* font = GetFont();

    if( !font )
        font = KIFONT::FONT::GetFont( renderSettings->GetDefaultFont(), IsBold(), IsItalic() );

    TEXT_ATTRIBUTES attrs = GetAttributes();
    attrs.m_StrokeWidth = penWidth;
    attrs.m_Halign = hjustify;
    attrs.m_Valign = vjustify;
    attrs.m_Angle = orient;
    attrs.m_Multiline = false;

    aPlotter->PlotText( textpos, color, text, attrs, font, GetFontMetrics() );

    if( IsHypertext() && Schematic() )
    {
        SCH_LABEL_BASE*                            label = static_cast<SCH_LABEL_BASE*>( m_parent );
        std::vector<std::pair<wxString, wxString>> pages;
        std::vector<wxString>                      pageHrefs;
        BOX2I                                      bbox = GetBoundingBox();

        wxCHECK( label, /* void */ );

        label->GetIntersheetRefs( &Schematic()->CurrentSheet(), &pages );

        for( const auto& [ pageNumber, sheetName ] : pages )
            pageHrefs.push_back( wxT( "#" ) + pageNumber );

        bbox.Offset( label->GetSchematicTextOffset( renderSettings ) );

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


bool SCH_FIELD::IsMandatory() const
{
    if( !m_parent )
        return false;

    switch( m_parent->Type() )
    {
    case SCH_SHEET_T:
        return m_id == SHEETNAME
            || m_id == SHEETFILENAME;

    case SCH_SYMBOL_T:
    case LIB_SYMBOL_T:
        return m_id == REFERENCE_FIELD
            || m_id == VALUE_FIELD
            || m_id == FOOTPRINT_FIELD
            || m_id == DATASHEET_FIELD
            || m_id == DESCRIPTION_FIELD;

    case SCH_GLOBAL_LABEL_T:
        return m_id == INTERSHEET_REFS;

    default:
        return false;
    }
}


bool SCH_FIELD::operator<( const SCH_ITEM& aItem ) const
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


bool SCH_FIELD::operator==(const SCH_ITEM& aOther) const
{
    if( Type() != aOther.Type() )
        return false;

    const SCH_FIELD& field = static_cast<const SCH_FIELD&>( aOther );

    return *this == field;
}


bool SCH_FIELD::operator==( const SCH_FIELD& aOther ) const
{
    // Identical fields of different symbols are not equal.
    if( !GetParentSymbol() || !aOther.GetParentSymbol()
        || GetParentSymbol()->m_Uuid != aOther.GetParentSymbol()->m_Uuid )
    {
        return false;
    }

    if( GetId() != aOther.GetId() )
        return false;

    if( GetPosition() != aOther.GetPosition() )
        return false;

    if( IsNamedVariable() != aOther.IsNamedVariable() )
        return false;

    if( IsNameShown() != aOther.IsNameShown() )
        return false;

    if( CanAutoplace() != aOther.CanAutoplace() )
        return false;

    return EDA_TEXT::operator==( aOther );
}


double SCH_FIELD::Similarity( const SCH_ITEM& aOther ) const
{
    if( Type() != aOther.Type() )
        return 0.0;

    if( m_Uuid == aOther.m_Uuid )
        return 1.0;

    const SCH_FIELD& field = static_cast<const SCH_FIELD&>( aOther );

    double similarity = 0.99; // The UUIDs are different, so we start with non-identity

    if( GetId() != field.GetId() )
    {
        // We don't allow swapping of mandatory fields, so these cannot be the same item
        if( IsMandatory() || field.IsMandatory() )
            return 0.0;
        else
            similarity *= 0.5;
    }

    similarity *= SimilarityBase( aOther );

    similarity *= EDA_TEXT::Similarity( field );

    if( GetPosition() != field.GetPosition() )
        similarity *= 0.5;

    if( IsNamedVariable() != field.IsNamedVariable() )
        similarity *= 0.5;

    if( IsNameShown() != field.IsNameShown() )
        similarity *= 0.5;

    if( CanAutoplace() != field.CanAutoplace() )
        similarity *= 0.5;

    return similarity;
}


int SCH_FIELD::compare( const SCH_ITEM& aOther, int aCompareFlags ) const
{
    wxASSERT( aOther.Type() == SCH_FIELD_T );

    int compareFlags = aCompareFlags;

    // For ERC tests, the field position has no matter, so do not test it
    if( aCompareFlags & SCH_ITEM::COMPARE_FLAGS::ERC )
        compareFlags |= SCH_ITEM::COMPARE_FLAGS::SKIP_TST_POS;

    int retv = SCH_ITEM::compare( aOther, compareFlags );

    if( retv )
        return retv;

    const SCH_FIELD* tmp = static_cast<const SCH_FIELD*>( &aOther );

    // Equality test will vary depending whether or not the field is mandatory.  Otherwise,
    // sorting is done by ordinal.
    if( aCompareFlags & SCH_ITEM::COMPARE_FLAGS::EQUALITY )
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

    if( m_id == REFERENCE_FIELD && !( aCompareFlags & SCH_ITEM::COMPARE_FLAGS::EQUALITY ) )
        ignoreFieldText = true;

    if( m_id == VALUE_FIELD && ( aCompareFlags & SCH_ITEM::COMPARE_FLAGS::ERC ) )
        ignoreFieldText = true;

    if( !ignoreFieldText )
    {
        retv = GetText().CmpNoCase( tmp->GetText() );

        if( retv != 0 )
            return retv;
    }

    if( aCompareFlags & SCH_ITEM::COMPARE_FLAGS::EQUALITY )
    {
        if( GetTextPos().x != tmp->GetTextPos().x )
            return GetTextPos().x - tmp->GetTextPos().x;

        if( GetTextPos().y != tmp->GetTextPos().y )
            return GetTextPos().y - tmp->GetTextPos().y;
    }

    // For ERC tests, the field size has no matter, so do not test it
    if( !( aCompareFlags & SCH_ITEM::COMPARE_FLAGS::ERC ) )
    {
        if( GetTextWidth() != tmp->GetTextWidth() )
            return GetTextWidth() - tmp->GetTextWidth();

        if( GetTextHeight() != tmp->GetTextHeight() )
            return GetTextHeight() - tmp->GetTextHeight();
    }

    return 0;
}


static struct SCH_FIELD_DESC
{
    SCH_FIELD_DESC()
    {
        // These are defined in EDA_TEXT as well but initialization order is
        // not defined, so this needs to be conditional.  Defining in both
        // places leads to duplicate symbols.
        auto& h_inst = ENUM_MAP<GR_TEXT_H_ALIGN_T>::Instance();

        if( h_inst.Choices().GetCount() == 0)
        {
            h_inst.Map( GR_TEXT_H_ALIGN_LEFT,   _( "Left" ) );
            h_inst.Map( GR_TEXT_H_ALIGN_CENTER, _( "Center" ) );
            h_inst.Map( GR_TEXT_H_ALIGN_RIGHT,  _( "Right" ) );
        }

        auto& v_inst = ENUM_MAP<GR_TEXT_V_ALIGN_T>::Instance();

        if( v_inst.Choices().GetCount() == 0)
        {
            v_inst.Map( GR_TEXT_V_ALIGN_TOP,    _( "Top" ) );
            v_inst.Map( GR_TEXT_V_ALIGN_CENTER, _( "Center" ) );
            v_inst.Map( GR_TEXT_V_ALIGN_BOTTOM, _( "Bottom" ) );
        }

        PROPERTY_MANAGER& propMgr = PROPERTY_MANAGER::Instance();
        REGISTER_TYPE( SCH_FIELD );
        propMgr.AddTypeCast( new TYPE_CAST<SCH_FIELD, SCH_ITEM> );
        propMgr.AddTypeCast( new TYPE_CAST<SCH_FIELD, EDA_TEXT> );
        propMgr.InheritsAfter( TYPE_HASH( SCH_FIELD ), TYPE_HASH( SCH_ITEM ) );
        propMgr.InheritsAfter( TYPE_HASH( SCH_FIELD ), TYPE_HASH( EDA_TEXT ) );

        const wxString textProps = _HKI( "Text Properties" );

        auto horiz = new PROPERTY_ENUM<SCH_FIELD, GR_TEXT_H_ALIGN_T>(
                _HKI( "Horizontal Justification" ), &SCH_FIELD::SetEffectiveHorizJustify,
                &SCH_FIELD::GetEffectiveHorizJustify );

        propMgr.ReplaceProperty( TYPE_HASH( EDA_TEXT ), _HKI( "Horizontal Justification" ), horiz,
                                 textProps );

        auto vert = new PROPERTY_ENUM<SCH_FIELD, GR_TEXT_V_ALIGN_T>(
                _HKI( "Vertical Justification" ), &SCH_FIELD::SetEffectiveVertJustify,
                &SCH_FIELD::GetEffectiveVertJustify );

        propMgr.ReplaceProperty( TYPE_HASH( EDA_TEXT ), _HKI( "Vertical Justification" ), vert,
                                 textProps );

        propMgr.AddProperty( new PROPERTY<SCH_FIELD, bool>( _HKI( "Show Field Name" ),
                &SCH_FIELD::SetNameShown, &SCH_FIELD::IsNameShown ) );

        propMgr.AddProperty( new PROPERTY<SCH_FIELD, bool>( _HKI( "Allow Autoplacement" ),
                &SCH_FIELD::SetCanAutoplace, &SCH_FIELD::CanAutoplace ) );

        propMgr.Mask( TYPE_HASH( SCH_FIELD ), TYPE_HASH( EDA_TEXT ), _HKI( "Hyperlink" ) );
        propMgr.Mask( TYPE_HASH( SCH_FIELD ), TYPE_HASH( EDA_TEXT ), _HKI( "Thickness" ) );
        propMgr.Mask( TYPE_HASH( SCH_FIELD ), TYPE_HASH( EDA_TEXT ), _HKI( "Mirrored" ) );
        propMgr.Mask( TYPE_HASH( SCH_FIELD ), TYPE_HASH( EDA_TEXT ), _HKI( "Width" ) );
        propMgr.Mask( TYPE_HASH( SCH_FIELD ), TYPE_HASH( EDA_TEXT ), _HKI( "Height" ) );


        propMgr.AddProperty( new PROPERTY<SCH_FIELD, int>( _HKI( "Text Size" ),
                &SCH_FIELD::SetSchTextSize, &SCH_FIELD::GetSchTextSize, PROPERTY_DISPLAY::PT_SIZE ),
                _HKI( "Text Properties" ) );

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


        auto isNonMandatoryField =
                []( INSPECTABLE* aItem ) -> bool
                {
                    if( SCH_FIELD* field = dynamic_cast<SCH_FIELD*>( aItem ) )
                        return !field->IsMandatory();

                    return false;
                };

        propMgr.OverrideAvailability( TYPE_HASH( SCH_FIELD ), TYPE_HASH( SCH_ITEM ),
                                      _HKI( "Private" ), isNonMandatoryField );
    }
} _SCH_FIELD_DESC;


DECLARE_ENUM_TO_WXANY( GR_TEXT_H_ALIGN_T )
DECLARE_ENUM_TO_WXANY( GR_TEXT_V_ALIGN_T )
