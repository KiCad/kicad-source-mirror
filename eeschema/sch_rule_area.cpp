/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
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

#include <algorithm>
#include <iterator>
#include <map>
#include <vector>

#include <eda_draw_frame.h>
#include <erc/erc_item.h>
#include <erc/erc_settings.h>
#include <geometry/shape_segment.h>
#include <geometry/shape_simple.h>
#include <sch_line.h>
#include <sch_marker.h>
#include <sch_rtree.h>
#include <sch_rule_area.h>
#include <sch_screen.h>
#include <sch_sheet_path.h>
#include <geometry/shape_rect.h>


wxString SCH_RULE_AREA::GetClass() const
{
    return wxT( "SCH_RULE_AREA" );
}


wxString SCH_RULE_AREA::GetFriendlyName() const
{
    return _( "Rule Area" );
}


EDA_ITEM* SCH_RULE_AREA::Clone() const
{
    return new SCH_RULE_AREA( *this );
}


std::vector<int> SCH_RULE_AREA::ViewGetLayers() const
{
    return { LAYER_RULE_AREAS, LAYER_NOTES_BACKGROUND, LAYER_SELECTION_SHADOWS };
}


std::vector<SHAPE*> SCH_RULE_AREA::MakeEffectiveShapes( bool aEdgeOnly ) const
{
    std::vector<SHAPE*> effectiveShapes;
    int                 width = GetEffectiveWidth();

    switch( m_shape )
    {
    case SHAPE_T::POLY:
        if( GetPolyShape().OutlineCount() == 0 ) // malformed/empty polygon
            break;

        for( int ii = 0; ii < GetPolyShape().OutlineCount(); ++ii )
        {
            const SHAPE_LINE_CHAIN& l = GetPolyShape().COutline( ii );

            if( IsSolidFill() && !aEdgeOnly )
                effectiveShapes.emplace_back( new SHAPE_SIMPLE( l ) );

            if( width > 0 || !IsSolidFill() || aEdgeOnly )
            {
                int segCount = l.SegmentCount();

                for( int jj = 0; jj < segCount; jj++ )
                    effectiveShapes.emplace_back( new SHAPE_SEGMENT( l.CSegment( jj ), width ) );
            }
        }

        break;

    default:
        return SCH_SHAPE::MakeEffectiveShapes( aEdgeOnly );
    }

    return effectiveShapes;
}


void SCH_RULE_AREA::Plot( PLOTTER* aPlotter, bool aBackground, const SCH_PLOT_OPTS& aPlotOpts,
                          int aUnit, int aBodyStyle, const VECTOR2I& aOffset, bool aDimmed )
{
    if( IsPrivate() )
        return;

    SCH_RENDER_SETTINGS* renderSettings = getRenderSettings( aPlotter );
    int                  pen_size = GetEffectivePenWidth( renderSettings );

    if( GetShape() != SHAPE_T::POLY )
        SCH_SHAPE::Plot( aPlotter, aBackground, aPlotOpts, aUnit, aBodyStyle, aOffset, aDimmed );

    static std::vector<VECTOR2I> ptList;

    ptList.clear();

    const std::vector<VECTOR2I>& polyPoints = m_poly.Outline( 0 ).CPoints();

    for( const VECTOR2I& pt : polyPoints )
        ptList.push_back( pt );

    ptList.push_back( polyPoints[0] );

    COLOR4D    color = GetStroke().GetColor();
    COLOR4D    bg = renderSettings->GetBackgroundColor();
    LINE_STYLE lineStyle = GetStroke().GetLineStyle();
    FILL_T     fill = m_fill;

    if( aBackground )
    {
        if( !aPlotter->GetColorMode() )
            return;

        switch( m_fill )
        {
        case FILL_T::FILLED_SHAPE:
            return;

        case FILL_T::FILLED_WITH_COLOR:
            color = GetFillColor();
            break;

        case FILL_T::FILLED_WITH_BG_BODYCOLOR:
            color = renderSettings->GetLayerColor( LAYER_DEVICE_BACKGROUND );
            break;

        default:
            return;
        }

        pen_size = 0;
        lineStyle = LINE_STYLE::SOLID;
    }
    else /* if( aForeground ) */
    {
        if( !aPlotter->GetColorMode() || color == COLOR4D::UNSPECIFIED )
            color = renderSettings->GetLayerColor( m_layer );

        if( lineStyle == LINE_STYLE::DEFAULT )
            lineStyle = LINE_STYLE::SOLID;

        if( m_fill == FILL_T::FILLED_SHAPE )
            fill = m_fill;
        else
            fill = FILL_T::NO_FILL;

        pen_size = GetEffectivePenWidth( renderSettings );
    }

    if( bg == COLOR4D::UNSPECIFIED || !aPlotter->GetColorMode() )
        bg = COLOR4D::WHITE;

    if( color.m_text.has_value() && Schematic() )
        color = COLOR4D( ResolveText( color.m_text.value(), &Schematic()->CurrentSheet() ) );

    if( aDimmed )
    {
        color.Desaturate();
        color = color.Mix( bg, 0.5f );
    }

    aPlotter->SetColor( color );
    aPlotter->SetCurrentLineWidth( pen_size );
    aPlotter->SetDash( pen_size, lineStyle );

    aPlotter->PlotPoly( ptList, fill, pen_size, nullptr );

    aPlotter->SetDash( pen_size, LINE_STYLE::SOLID );
}


wxString SCH_RULE_AREA::GetItemDescription( UNITS_PROVIDER* aUnitsProvider, bool aFull ) const
{
    return _( "Schematic rule area" );
}


void SCH_RULE_AREA::resetCaches()
{
    // Save the current state
    m_prev_items = m_itemIDs;
    m_prev_directives = m_directiveIDs;

    // Reset the rule area
    // Do NOT assume these pointers are valid.
    m_items.clear();
    m_itemIDs.clear();
    m_directives.clear();
    m_directiveIDs.clear();
}


void SCH_RULE_AREA::RefreshContainedItemsAndDirectives( SCH_SCREEN* screen )
{
    EE_RTREE&   items = screen->Items();
    const BOX2I boundingBox = GetBoundingBox();

    // Get any SCH_DIRECTIVE_LABELs which are attached to the rule area border
    std::unordered_set<SCH_DIRECTIVE_LABEL*> attachedDirectives;
    EE_RTREE::EE_TYPE candidateDirectives = items.Overlapping( SCH_DIRECTIVE_LABEL_T, boundingBox );

    for( SCH_ITEM* candidateDirective : candidateDirectives )
    {
        SCH_DIRECTIVE_LABEL*        label = static_cast<SCH_DIRECTIVE_LABEL*>( candidateDirective );
        const std::vector<VECTOR2I> labelConnectionPoints = label->GetConnectionPoints();
        assert( labelConnectionPoints.size() == 1 );

        if( GetPolyShape().CollideEdge( labelConnectionPoints[0], nullptr, 5 ) )
            addDirective( label );
    }

    // Next find any connectable items which lie within the rule area
    EE_RTREE::EE_TYPE ruleAreaItems = items.Overlapping( boundingBox );

    for( SCH_ITEM* areaItem : ruleAreaItems )
    {
        if( areaItem->IsType( { SCH_ITEM_LOCATE_WIRE_T, SCH_ITEM_LOCATE_BUS_T } ) )
        {
            SCH_LINE*     lineItem = static_cast<SCH_LINE*>( areaItem );
            SHAPE_SEGMENT lineSeg( lineItem->GetStartPoint(), lineItem->GetEndPoint(),
                                   lineItem->GetLineWidth() );

            if( GetPolyShape().Collide( &lineSeg ) )
                addContainedItem( areaItem );
        }
        else if( areaItem->IsType( { SCH_PIN_T, SCH_LABEL_T, SCH_GLOBAL_LABEL_T, SCH_HIER_LABEL_T } ) )
        {
            std::vector<VECTOR2I> connectionPoints = areaItem->GetConnectionPoints();
            wxASSERT( connectionPoints.size() == 1 );

            if( GetPolyShape().Collide( connectionPoints[0] ) )
                addContainedItem( areaItem );
        }
        else if( areaItem->IsType( { SCH_SYMBOL_T } ) )
        {
            SCH_SYMBOL*      symbol = static_cast<SCH_SYMBOL*>( areaItem );
            const BOX2I      symbolBb = symbol->GetBoundingBox();
            const SHAPE_RECT rect( symbolBb );

            if( GetPolyShape().Collide( &rect ) )
            {
                addContainedItem( areaItem );

                // Add child pins which are within the rule area
                for( SCH_PIN* pin : symbol->GetPins() )
                {
                    if( GetPolyShape().Collide( pin->GetPosition() ) )
                        addContainedItem( pin );
                }
            }
        }
        else if( areaItem->IsType( { SCH_SHEET_T } ) )
        {
            const BOX2I      sheetBb = areaItem->GetBoundingBox();
            const SHAPE_RECT rect( sheetBb );

            if( GetPolyShape().Collide( &rect ) )
            {
                addContainedItem( areaItem );
            }
        }
    }
}


std::vector<std::pair<SCH_RULE_AREA*, SCH_SCREEN*>>
SCH_RULE_AREA::UpdateRuleAreasInScreens( std::unordered_set<SCH_SCREEN*>& screens, KIGFX::SCH_VIEW* view )
{
    std::vector<std::pair<SCH_RULE_AREA*, SCH_SCREEN*>> forceUpdateRuleAreas;

    for( SCH_SCREEN* screen : screens )
    {
        // First reset all item caches - must be done first to ensure two rule areas
        // on the same item don't overwrite each other's caches
        for( SCH_ITEM* item : screen->Items() )
        {
            if( item->Type() == SCH_RULE_AREA_T )
                static_cast<SCH_RULE_AREA*>( item )->resetCaches();

            if( item->Type() == SCH_DIRECTIVE_LABEL_T && view )
                view->Update( item, KIGFX::REPAINT );

            item->ClearRuleAreasCache();
        }

        // Secondly refresh the contained items
        for( SCH_ITEM* ruleAreaAsItem : screen->Items().OfType( SCH_RULE_AREA_T ) )
        {
            SCH_RULE_AREA* ruleArea = static_cast<SCH_RULE_AREA*>( ruleAreaAsItem );

            ruleArea->RefreshContainedItemsAndDirectives( screen );

            if( ruleArea->m_directiveIDs != ruleArea->m_prev_directives )
                forceUpdateRuleAreas.push_back( { ruleArea, screen } );
        }
    }

    return forceUpdateRuleAreas;
}


const std::unordered_set<SCH_ITEM*>& SCH_RULE_AREA::GetContainedItems() const
{
    return m_items;
}


const std::unordered_set<SCH_DIRECTIVE_LABEL*>& SCH_RULE_AREA::GetDirectives() const
{
    return m_directives;
}


const std::unordered_set<KIID>& SCH_RULE_AREA::GetPastContainedItems() const
{
    return m_prev_items;
}


const std::vector<std::pair<wxString, SCH_ITEM*>>
SCH_RULE_AREA::GetResolvedNetclasses( const SCH_SHEET_PATH* aSheetPath ) const
{
    std::vector<std::pair<wxString, SCH_ITEM*>> resolvedNetclasses;

    for( SCH_DIRECTIVE_LABEL* directive : m_directives )
    {
        directive->RunOnChildren(
                [&]( SCH_ITEM* aChild )
                {
                    if( aChild->Type() == SCH_FIELD_T )
                    {
                        SCH_FIELD* field = static_cast<SCH_FIELD*>( aChild );

                        if( field->GetCanonicalName() == wxT( "Netclass" ) )
                        {
                            wxString netclass = field->GetShownText( aSheetPath, false );

                            if( netclass != wxEmptyString )
                                resolvedNetclasses.push_back( { netclass, directive } );
                        }
                    }

                    return true;
                },
                RECURSE_MODE::NO_RECURSE );
    }

    return resolvedNetclasses;
}


void SCH_RULE_AREA::GetMsgPanelInfo( EDA_DRAW_FRAME* aFrame, std::vector<MSG_PANEL_ITEM>& aList )
{
    aList.emplace_back( _( "Rule Area" ), wxEmptyString );

    wxString msg;
    msg.Printf( wxS( "%d" ), GetPolyShape().Outline( 0 ).PointCount() );
    aList.emplace_back( _( "Points" ), msg );

    m_stroke.GetMsgPanelInfo( aFrame, aList );

    const std::vector<std::pair<wxString, SCH_ITEM*>> netclasses = SCH_RULE_AREA::GetResolvedNetclasses( nullptr );
    wxString resolvedNetclass = _( "<None>" );

    if( netclasses.size() > 0 )
        resolvedNetclass = netclasses[0].first;

    aList.emplace_back( _( "Resolved netclass" ), resolvedNetclass );
}


void SCH_RULE_AREA::addDirective( SCH_DIRECTIVE_LABEL* label )
{
    label->AddConnectedRuleArea( this );
    m_directives.insert( label );
    m_directiveIDs.insert( label->m_Uuid );
}


void SCH_RULE_AREA::addContainedItem( SCH_ITEM* item )
{
    item->AddRuleAreaToCache( this );
    m_items.insert( item );
    m_itemIDs.insert( item->m_Uuid );
}


static struct SCH_RULE_AREA_DESC
{
    SCH_RULE_AREA_DESC()
    {
        PROPERTY_MANAGER& propMgr = PROPERTY_MANAGER::Instance();
        REGISTER_TYPE( SCH_RULE_AREA );
        propMgr.AddTypeCast( new TYPE_CAST<SCH_RULE_AREA, SCH_SHAPE> );
        propMgr.AddTypeCast( new TYPE_CAST<SCH_RULE_AREA, SCH_ITEM> );
        propMgr.AddTypeCast( new TYPE_CAST<SCH_RULE_AREA, EDA_SHAPE> );
        propMgr.InheritsAfter( TYPE_HASH( SCH_RULE_AREA ), TYPE_HASH( SCH_SHAPE ) );
        propMgr.InheritsAfter( TYPE_HASH( SCH_RULE_AREA ), TYPE_HASH( SCH_ITEM ) );
        propMgr.InheritsAfter( TYPE_HASH( SCH_RULE_AREA ), TYPE_HASH( EDA_SHAPE ) );
    }
} _SCH_RULE_AREA_DESC;
