/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2024 KiCad Developers, see AUTHORS.txt for contributors.
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


void SCH_RULE_AREA::ViewGetLayers( int aLayers[], int& aCount ) const
{
    aCount = 3;
    aLayers[0] = LAYER_RULE_AREAS;
    aLayers[1] = LAYER_NOTES_BACKGROUND;
    aLayers[2] = LAYER_SELECTION_SHADOWS;
}


std::vector<SHAPE*> SCH_RULE_AREA::MakeEffectiveShapes( bool aEdgeOnly ) const
{
    std::vector<SHAPE*> effectiveShapes;
    int                 width = GetEffectiveWidth();

    switch( m_shape )
    {
    case SHAPE_T::POLY:
    {
        if( GetPolyShape().OutlineCount() == 0 ) // malformed/empty polygon
            break;

        for( int ii = 0; ii < GetPolyShape().OutlineCount(); ++ii )
        {
            const SHAPE_LINE_CHAIN& l = GetPolyShape().COutline( ii );

            if( IsFilled() && !aEdgeOnly )
                effectiveShapes.emplace_back( new SHAPE_SIMPLE( l ) );

            if( width > 0 || !IsFilled() || aEdgeOnly )
            {
                int segCount = l.SegmentCount();

                for( int jj = 0; jj < segCount; jj++ )
                    effectiveShapes.emplace_back( new SHAPE_SEGMENT( l.CSegment( jj ), width ) );
            }
        }
    }
    break;

    default:
        return SCH_SHAPE::MakeEffectiveShapes( aEdgeOnly );
        break;
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
    {
        ptList.push_back( pt );
    }

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

    if( aDimmed )
    {
        color.Desaturate();
        color = color.Mix( bg, 0.5f );
    }

    aPlotter->SetColor( color );
    aPlotter->SetCurrentLineWidth( pen_size );
    aPlotter->SetDash( pen_size, lineStyle );

    aPlotter->PlotPoly( ptList, fill, pen_size );

    aPlotter->SetDash( pen_size, LINE_STYLE::SOLID );
}


wxString SCH_RULE_AREA::GetItemDescription( UNITS_PROVIDER* aUnitsProvider ) const
{
    return _( "Schematic rule area" );
}


void SCH_RULE_AREA::ResetCaches( KIGFX::SCH_VIEW* view )
{
    // Save the current state
    m_prev_items = m_items;
    m_prev_directives = m_directives;

    // Reset the rule area
    clearContainedItems();
    clearDirectives( view );
}


void SCH_RULE_AREA::RefreshContainedItemsAndDirectives(
        SCH_SCREEN* screen, KIGFX::SCH_VIEW* view,
        std::vector<std::pair<SCH_RULE_AREA*, SCH_SCREEN*>>& forceUpdateRuleAreas )
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
        {
            addDirective( label, view );
        }
    }

    // If directives have changed, we need to force an update of the contained items connectivity
    if( m_directives != m_prev_directives )
        forceUpdateRuleAreas.push_back( { this, screen } );

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
            {
                addContainedItem( areaItem );
            }
        }
        else if( areaItem->IsType(
                         { SCH_PIN_T, SCH_LABEL_T, SCH_GLOBAL_LABEL_T, SCH_HIER_LABEL_T } ) )
        {
            std::vector<VECTOR2I> connectionPoints = areaItem->GetConnectionPoints();
            assert( connectionPoints.size() == 1 );

            if( GetPolyShape().Collide( connectionPoints[0] ) )
            {
                addContainedItem( areaItem );
            }
        }
    }
}


std::unordered_set<SCH_ITEM*> SCH_RULE_AREA::GetPastAndPresentContainedItems() const
{
    std::unordered_set<SCH_ITEM*> items = m_items;

    for( SCH_ITEM* item : m_prev_items )
        items.insert( item );

    return items;
}


std::vector<std::pair<SCH_RULE_AREA*, SCH_SCREEN*>>
SCH_RULE_AREA::UpdateRuleAreasInScreens( std::unordered_set<SCH_SCREEN*>& screens,
                                         KIGFX::SCH_VIEW*                 view )
{
    std::vector<std::pair<SCH_RULE_AREA*, SCH_SCREEN*>> forceUpdateRuleAreas;

    for( SCH_SCREEN* screen : screens )
    {
        // First reset all item caches - must be done first to ensure two rule areas
        // on the same item don't overwrite each other's caches
        for( SCH_ITEM* ruleAreaAsItem : screen->Items().OfType( SCH_RULE_AREA_T ) )
        {
            SCH_RULE_AREA* ruleArea = static_cast<SCH_RULE_AREA*>( ruleAreaAsItem );
            ruleArea->ResetCaches( view );
        }

        // Secondly refresh the contained items
        for( SCH_ITEM* ruleAreaAsItem : screen->Items().OfType( SCH_RULE_AREA_T ) )
        {
            SCH_RULE_AREA* ruleArea = static_cast<SCH_RULE_AREA*>( ruleAreaAsItem );
            ruleArea->RefreshContainedItemsAndDirectives( screen, view, forceUpdateRuleAreas );
        }
    }

    return forceUpdateRuleAreas;
}


const std::unordered_set<SCH_ITEM*>& SCH_RULE_AREA::GetContainedItems() const
{
    return m_items;
}


const std::unordered_set<SCH_DIRECTIVE_LABEL*> SCH_RULE_AREA::GetDirectives() const
{
    return m_directives;
}


const std::vector<std::pair<wxString, SCH_ITEM*>> SCH_RULE_AREA::GetResolvedNetclasses() const
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
                            wxString netclass = field->GetText();

                            if( netclass != wxEmptyString )
                                resolvedNetclasses.push_back( { netclass, directive } );
                        }
                    }

                    return true;
                } );
    }

    return resolvedNetclasses;
}


void SCH_RULE_AREA::ResetDirectivesAndItems( KIGFX::SCH_VIEW* view )
{
    for( SCH_DIRECTIVE_LABEL* label : m_directives )
    {
        label->ClearConnectedRuleAreas();
        view->Update( label, KIGFX::REPAINT );
    }

    for( SCH_ITEM* item : m_items )
        item->ClearRuleAreasCache();
}


void SCH_RULE_AREA::GetMsgPanelInfo( EDA_DRAW_FRAME* aFrame, std::vector<MSG_PANEL_ITEM>& aList )
{
    aList.emplace_back( _( "Rule Area" ), wxEmptyString );

    wxString msg;
    msg.Printf( wxS( "%d" ), GetPolyShape().Outline( 0 ).PointCount() );
    aList.emplace_back( _( "Points" ), msg );

    m_stroke.GetMsgPanelInfo( aFrame, aList );

    const std::vector<std::pair<wxString, SCH_ITEM*>> netclasses =
            SCH_RULE_AREA::GetResolvedNetclasses();
    wxString resolvedNetclass = _( "<None>" );

    if( netclasses.size() > 0 )
        resolvedNetclass = netclasses[0].first;

    aList.emplace_back( _( "Resolved netclass" ), resolvedNetclass );
}


void SCH_RULE_AREA::addDirective( SCH_DIRECTIVE_LABEL* label, KIGFX::SCH_VIEW* view )
{
    label->AddConnectedRuleArea( this );
    m_directives.insert( label );

    if( view )
        view->Update( label, KIGFX::REPAINT );
}


void SCH_RULE_AREA::clearDirectives( KIGFX::SCH_VIEW* view )
{
    for( SCH_DIRECTIVE_LABEL* label : m_directives )
    {
        label->ClearConnectedRuleAreas();

        if( view )
            view->Update( label, KIGFX::REPAINT );
    }

    m_directives.clear();
}


void SCH_RULE_AREA::addContainedItem( SCH_ITEM* item )
{
    item->AddRuleAreaToCache( this );
    m_items.insert( item );
}


void SCH_RULE_AREA::clearContainedItems()
{
    for( SCH_ITEM* item : m_items )
        item->ClearRuleAreasCache();

    m_items.clear();
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
