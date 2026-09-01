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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include <algorithm>
#include <cmath>
#include <map>
#include <set>
#include <tuple>
#include <vector>

#include <geometry/geometry_utils.h>
#include <math/util.h>

#include <board.h>
#include <board_commit.h>
#include <board_design_settings.h>
#include <footprint.h>
#include <layer_range.h>
#include <netinfo.h>
#include <netclass.h>
#include <pad.h>
#include <zone.h>
#include <lset.h>
#include <padstack.h>
#include <pcb_base_edit_frame.h>
#include <pcb_track.h>
#include <pcbnew_settings.h>
#include <scoped_set_reset.h>
#include <wx/choicdlg.h>
#include <tool/actions.h>
#include <tool/tool_manager.h>
#include <view/view.h>
#include <gal/graphics_abstraction_layer.h>
#include <trigo.h>
#include <view/view_controls.h>

#include <properties/property.h>
#include <properties/property_mgr.h>

#include <dialogs/dialog_microvia_stack.h>
#include <generators/pcb_via_stack.h>
#include <generators_mgr.h>
#include <drc/drc_engine.h>
#include <router/router_tool.h>
#include <router/pns_router.h>
#include <router/pns_routing_settings.h>
#include <widgets/wx_infobar.h>
#include <tools/drawing_tool.h>
#include <tools/generator_tool.h>
#include <tools/pcb_actions.h>
#include <tools/pcb_grid_helper.h>
#include <tools/pcb_selection.h>


const wxString PCB_VIA_STACK::DISPLAY_NAME = _HKI( "Microvia Stack" );
const wxString PCB_VIA_STACK::GENERATOR_TYPE = wxS( "via_stack" );


static wxString styleToString( VIA_STACK_STYLE aStyle )
{
    return aStyle == VIA_STACK_STYLE::STAGGERED ? wxS( "staggered" ) : wxS( "stacked" );
}


static VIA_STACK_STYLE styleFromString( const wxString& aStr )
{
    return aStr == wxS( "staggered" ) ? VIA_STACK_STYLE::STAGGERED : VIA_STACK_STYLE::STACKED;
}


// Resolve a canonical copper layer name back to its id without needing a board.
static PCB_LAYER_ID layerFromName( const wxString& aName, PCB_LAYER_ID aFallback )
{
    wxString name = aName;
    int      layer = LSET::NameToLayer( name );

    if( layer >= 0 && layer < PCB_LAYER_ID_COUNT && !( layer & 1 ) && IsCopperLayer( layer ) )
        return ToLAYER_ID( layer );

    return aFallback;
}


PCB_VIA_STACK::PCB_VIA_STACK( BOARD_ITEM* aParent, PCB_LAYER_ID aLayer ) :
        PCB_GENERATOR( aParent, aLayer ),
        m_startLayer( aLayer ),
        m_endLayer( In1_Cu ),
        m_style( VIA_STACK_STYLE::STACKED ),
        m_pitch( 0 ),
        m_filled( true ),
        m_capped( false ),
        m_viaSize( 0 ),
        m_viaDrill( 0 ),
        m_useNetclass( false ),
        m_netCode( 0 )
{
    m_generatorType = GENERATOR_TYPE;
    m_name = DISPLAY_NAME;
}


int PCB_VIA_STACK::GetNetCode() const
{
    for( BOARD_ITEM* item : GetBoardItems() )
    {
        if( BOARD_CONNECTED_ITEM* bci = dynamic_cast<BOARD_CONNECTED_ITEM*>( item ) )
            return bci->GetNetCode();
    }

    return m_netCode;
}


void PCB_VIA_STACK::SetNetCode( int aNet )
{
    m_netCode = aNet;

    for( BOARD_ITEM* item : GetBoardItems() )
    {
        if( BOARD_CONNECTED_ITEM* bci = dynamic_cast<BOARD_CONNECTED_ITEM*>( item ) )
            bci->SetNetCode( aNet );
    }
}


bool PCB_VIA_STACK::IsSpanValid( BOARD* aBoard, PCB_LAYER_ID aStart, PCB_LAYER_ID aEnd )
{
    LSET enabled = LSET::AllCuMask( aBoard->GetCopperLayerCount() );

    auto present = [&]( PCB_LAYER_ID aLayer )
    {
        return aLayer >= 0 && aLayer < PCB_LAYER_ID_COUNT && enabled.test( aLayer );
    };

    return present( aStart ) && present( aEnd ) && aStart != aEnd;
}


void PCB_VIA_STACK::Move( const VECTOR2I& aMoveVector )
{
    PCB_GENERATOR::Move( aMoveVector );

    if( m_hops )
        m_hops->Move( aMoveVector );
}


void PCB_VIA_STACK::Rotate( const VECTOR2I& aRotCentre, const EDA_ANGLE& aAngle )
{
    PCB_GENERATOR::Rotate( aRotCentre, aAngle );

    if( m_hops )
        m_hops->Rotate( aAngle, aRotCentre );
}


void PCB_VIA_STACK::Flip( const VECTOR2I& aCentre, FLIP_DIRECTION aFlipDirection )
{
    PCB_GENERATOR::Flip( aCentre, aFlipDirection );

    if( m_hops )
        m_hops->Mirror( aCentre, aFlipDirection );

    m_startLayer = GetBoard()->FlipLayer( m_startLayer );
    m_endLayer = GetBoard()->FlipLayer( m_endLayer );
}


void PCB_VIA_STACK::Mirror( const VECTOR2I& aCentre, FLIP_DIRECTION aFlipDirection )
{
    PCB_GENERATOR::Mirror( aCentre, aFlipDirection );

    if( m_hops )
        m_hops->Mirror( aCentre, aFlipDirection );
}


void PCB_VIA_STACK::addMember( BOARD* aBoard, BOARD_COMMIT* aCommit, BOARD_ITEM* aItem )
{
    if( aCommit )
        aCommit->Add( aItem );
    else
        aBoard->Add( aItem, ADD_MODE::APPEND, false );

    AddItem( aItem );
}


std::vector<BOARD_ITEM*> PCB_VIA_STACK::BuildMembers( BOARD* aBoard, int aNetCode ) const
{
    std::vector<BOARD_ITEM*> members;

    if( !IsSpanValid( aBoard, m_startLayer, m_endLayer ) )
        return members;

    // Copper layers spanned, in physical stackup order.
    std::vector<PCB_LAYER_ID> layers;

    for( PCB_LAYER_ID layer : LAYER_RANGE( m_startLayer, m_endLayer, aBoard->GetCopperLayerCount() ) )
        layers.push_back( layer );

    if( layers.size() < 2 )
        return members;

    const int nHops = (int) layers.size() - 1;

    BOARD_DESIGN_SETTINGS& bds = aBoard->GetDesignSettings();
    int                    viaSize;
    int                    viaDrill;

    if( m_useNetclass )
    {
        NETINFO_ITEM* net = aBoard->FindNet( aNetCode );
        NETCLASS*     nc = net ? net->GetNetClass() : nullptr;

        viaSize = ( nc && nc->HasuViaDiameter() ) ? nc->GetuViaDiameter() : bds.GetCurrentViaSize();
        viaDrill = ( nc && nc->HasuViaDrill() ) ? nc->GetuViaDrill() : bds.GetCurrentViaDrill();
    }
    else
    {
        viaSize = m_viaSize > 0 ? m_viaSize : bds.GetCurrentViaSize();
        viaDrill = m_viaDrill > 0 ? m_viaDrill : bds.GetCurrentViaDrill();
    }

    auto hopPos = [&]( int aIndex ) -> VECTOR2I
    {
        if( m_style == VIA_STACK_STYLE::STAGGERED )
        {
            if( m_hops && aIndex < (int) m_hops->PointCount() )
                return m_hops->CPoint( aIndex );

            if( m_hops && m_hops->PointCount() > 1 )
            {
                int      last = m_hops->PointCount() - 1;
                VECTOR2I step = ( m_hops->CPoint( last ) - m_hops->CPoint( last - 1 ) ).Resize( m_pitch );

                if( step != VECTOR2I( 0, 0 ) )
                    return m_hops->CPoint( last ) + step * ( aIndex - last );
            }

            return GetPosition() + VECTOR2I( (long long int) m_pitch * aIndex, 0 );
        }

        return GetPosition(); // stacked hops are coaxial
    };

    // One microvia per adjacent copper pair.
    for( int i = 0; i < nHops; ++i )
    {
        PCB_VIA* via = new PCB_VIA( aBoard );
        via->SetNetCode( aNetCode );
        via->SetViaType( VIATYPE::MICROVIA );
        via->SetLayerPair( layers[i], layers[i + 1] );
        via->SetWidth( PADSTACK::ALL_LAYERS, viaSize );
        via->SetDrill( viaDrill );
        via->SetPosition( hopPos( i ) );

        // Only a hop reaching an outer layer has a face to cap.
        bool capped = m_capped && ( IsExternalCopperLayer( layers[i] ) || IsExternalCopperLayer( layers[i + 1] ) );

        // Stacked hops land on filled copper, and capping needs a filled via under it.
        bool filled = ( m_style == VIA_STACK_STYLE::STACKED ) || m_filled || capped;

        via->Padstack().Drill().is_filled = filled;
        via->Padstack().Drill().is_capped = capped;

        members.push_back( via );
    }

    // Staggered stacks need a short trace on each shared landing layer.
    if( m_style == VIA_STACK_STYLE::STAGGERED )
    {
        // The trace takes the net's width, not whatever the toolbar shows.
        NETINFO_ITEM* net = aBoard->FindNet( aNetCode );
        NETCLASS*     nc = net ? net->GetNetClass() : nullptr;

        int trackWidth = ( nc && nc->HasTrackWidth() ) ? nc->GetTrackWidth() : bds.GetCurrentTrackWidth();

        for( int i = 0; i < nHops - 1; ++i )
        {
            VECTOR2I a = hopPos( i );
            VECTOR2I b = hopPos( i + 1 );

            if( a == b )
                continue;

            PCB_TRACK* trace = new PCB_TRACK( aBoard );
            trace->SetNetCode( aNetCode );
            trace->SetStart( a );
            trace->SetEnd( b );
            trace->SetLayer( layers[i + 1] );
            trace->SetWidth( trackWidth );

            members.push_back( trace );
        }
    }

    return members;
}


// Identifies the hop a member implements. Two members with the same key are interchangeable.
static std::optional<std::tuple<int, int, int>> memberKey( BOARD_ITEM* aItem )
{
    if( aItem->Type() == PCB_VIA_T )
    {
        PCB_VIA* via = static_cast<PCB_VIA*>( aItem );

        return std::make_tuple( (int) PCB_VIA_T, (int) via->TopLayer(), (int) via->BottomLayer() );
    }

    if( aItem->Type() == PCB_TRACE_T )
        return std::make_tuple( (int) PCB_TRACE_T, (int) aItem->GetLayer(), 0 );

    return std::nullopt;
}


bool PCB_VIA_STACK::reuseMembers( BOARD_COMMIT* aCommit, const std::vector<BOARD_ITEM*>& aRebuilt )
{
    if( aRebuilt.empty() || aRebuilt.size() != GetItems().size() )
        return false;

    std::map<std::tuple<int, int, int>, BOARD_ITEM*> live;

    for( BOARD_ITEM* item : GetBoardItems() )
    {
        std::optional<std::tuple<int, int, int>> key = memberKey( item );

        if( !key || !live.emplace( *key, item ).second )
            return false;
    }

    std::vector<std::pair<BOARD_ITEM*, BOARD_ITEM*>> matched;

    for( BOARD_ITEM* fresh : aRebuilt )
    {
        std::optional<std::tuple<int, int, int>> key = memberKey( fresh );

        if( !key )
            return false;

        auto it = live.find( *key );

        if( it == live.end() )
            return false;

        matched.emplace_back( it->second, fresh );
        live.erase( it );
    }

    for( const auto& [item, fresh] : matched )
    {
        if( aCommit )
            aCommit->Modify( item );

        if( item->Type() == PCB_VIA_T )
        {
            PCB_VIA* via = static_cast<PCB_VIA*>( item );
            PCB_VIA* src = static_cast<PCB_VIA*>( fresh );

            via->SetNetCode( src->GetNetCode() );
            via->SetPosition( src->GetPosition() );
            via->SetWidth( PADSTACK::ALL_LAYERS, src->GetWidth( PADSTACK::ALL_LAYERS ) );
            via->SetDrill( src->GetDrillValue() );
            via->Padstack().Drill().is_filled = src->Padstack().Drill().is_filled;
            via->Padstack().Drill().is_capped = src->Padstack().Drill().is_capped;
        }
        else
        {
            PCB_TRACK* trace = static_cast<PCB_TRACK*>( item );
            PCB_TRACK* src = static_cast<PCB_TRACK*>( fresh );

            trace->SetNetCode( src->GetNetCode() );
            trace->SetStart( src->GetStart() );
            trace->SetEnd( src->GetEnd() );
            trace->SetWidth( src->GetWidth() );
        }
    }

    return true;
}


void PCB_VIA_STACK::Regenerate( BOARD* aBoard, BOARD_COMMIT* aCommit )
{
    int                      net = GetNetCode();
    std::vector<BOARD_ITEM*> rebuilt = BuildMembers( aBoard, net );

    if( rebuilt.empty() && !GetItems().empty() )
        return;

    // A drag rebuilds on every motion event. Replacing the members each time would stage an
    // add and a remove per generation in one commit, and undo brings all of them back.
    if( reuseMembers( aCommit, rebuilt ) )
    {
        for( BOARD_ITEM* item : rebuilt )
            delete item;

        return;
    }

    // Remove any existing members. For the commit path the item must be snapshotted for undo
    // (aCommit->Remove) while it is STILL in this group, otherwise the undo image is ungrouped
    // and undoing an edit leaves the vias loose. RemoveItem() unlinks it afterwards.
    for( BOARD_ITEM* item : GetBoardItems() )
    {
        if( aCommit )
        {
            aCommit->Unmodify( item, nullptr );
            aCommit->Remove( item );
            RemoveItem( item );
        }
        else
        {
            RemoveItem( item );
            aBoard->Remove( item );
            delete item;
        }
    }

    for( BOARD_ITEM* member : rebuilt )
        addMember( aBoard, aCommit, member );
}


void PCB_VIA_STACK::EditStart( GENERATOR_TOOL*, BOARD*, BOARD_COMMIT* aCommit )
{
    if( aCommit )
    {
        if( IsNew() )
        {
            aCommit->Add( this );
        }
        else
        {
            // Members must be staged before the move, not after.
            aCommit->Modify( this, nullptr, RECURSE_MODE::RECURSE );
        }
    }

    SetFlags( IN_EDIT );
}


bool PCB_VIA_STACK::Update( GENERATOR_TOOL*, BOARD* aBoard, BOARD_COMMIT* aCommit )
{
    if( !( GetFlags() & IN_EDIT ) )
        return false;

    if( !aBoard || !aCommit )
        return false;

    Regenerate( aBoard, aCommit );
    return true;
}


void PCB_VIA_STACK::EditFinish( GENERATOR_TOOL*, BOARD*, BOARD_COMMIT* )
{
    ClearFlags( IN_EDIT );
}


void PCB_VIA_STACK::EditCancel( GENERATOR_TOOL*, BOARD*, BOARD_COMMIT* )
{
    ClearFlags( IN_EDIT );
}


void PCB_VIA_STACK::Remove( GENERATOR_TOOL*, BOARD*, BOARD_COMMIT* aCommit )
{
    if( !aCommit )
        return;

    for( BOARD_ITEM* item : GetBoardItems() )
    {
        aCommit->Unmodify( item, nullptr );
        aCommit->Remove( item );
    }

    aCommit->Remove( this );
}


int PCB_VIA_STACK::FindNetAtPosition( BOARD* aBoard, const VECTOR2I& aPosition, PCB_LAYER_ID aLayer )
{
    for( FOOTPRINT* footprint : aBoard->Footprints() )
    {
        for( PAD* pad : footprint->Pads() )
        {
            // A mechanical hole carries no net, and must not hide copper under it.
            if( pad->GetAttribute() == PAD_ATTRIB::NPTH )
                continue;

            if( pad->IsOnLayer( aLayer ) && pad->HitTest( aPosition ) )
                return pad->GetNetCode();
        }
    }

    for( PCB_TRACK* track : aBoard->Tracks() )
    {
        if( track->IsOnLayer( aLayer ) && track->HitTest( aPosition ) )
            return track->GetNetCode();
    }

    for( ZONE* zone : aBoard->Zones() )
    {
        if( !zone->GetIsRuleArea() && zone->IsOnLayer( aLayer ) && zone->HitTestFilledArea( aLayer, aPosition ) )
        {
            return zone->GetNetCode();
        }
    }

    return 0;
}


int PCB_VIA_STACK::FindNetAtPosition( BOARD* aBoard, const VECTOR2I& aPosition, const LSET& aLayers )
{
    for( PCB_LAYER_ID layer : aLayers.CuStack() )
    {
        if( int net = FindNetAtPosition( aBoard, aPosition, layer ) )
            return net;
    }

    return 0;
}


void PCB_VIA_STACK::ApplyPreset( const VIA_STACK_PRESET& aPreset )
{
    SetStartLayer( aPreset.m_StartLayer );
    m_endLayer = aPreset.m_EndLayer;
    m_style = aPreset.m_Staggered ? VIA_STACK_STYLE::STAGGERED : VIA_STACK_STYLE::STACKED;
    m_viaSize = aPreset.m_ViaSize;
    m_viaDrill = aPreset.m_ViaDrill;
    m_useNetclass = aPreset.m_UseNetclass;
    m_filled = aPreset.m_Filled;
    m_capped = aPreset.m_Capped;
    m_pitch = aPreset.m_Pitch;
    m_presetName = aPreset.m_Name;
}


VIA_STACK_PRESET PCB_VIA_STACK::ToPreset() const
{
    VIA_STACK_PRESET preset;

    preset.m_Name = m_presetName;
    preset.m_StartLayer = m_startLayer;
    preset.m_EndLayer = m_endLayer;
    preset.m_Staggered = m_style == VIA_STACK_STYLE::STAGGERED;
    preset.m_ViaSize = m_viaSize;
    preset.m_ViaDrill = m_viaDrill;
    preset.m_UseNetclass = m_useNetclass;
    preset.m_Filled = m_filled;
    preset.m_Capped = m_capped;
    preset.m_Pitch = m_pitch;

    return preset;
}


// Copper layers in physical stack order, plus a layer -> position lookup.
static void layerOrder( BOARD* aBoard, std::vector<PCB_LAYER_ID>& aOrder, std::map<int, int>& aOrdinals )
{
    int n = 0;

    for( PCB_LAYER_ID layer : LAYER_RANGE( F_Cu, B_Cu, aBoard->GetCopperLayerCount() ) )
    {
        aOrder.push_back( layer );
        aOrdinals[layer] = n++;
    }
}


PCB_VIA_STACK* PCB_VIA_STACK::CreateFromItems( const std::vector<BOARD_ITEM*>& aItems, BOARD* aBoard,
                                               std::vector<BOARD_ITEM*>* aMembers )
{
    std::vector<PCB_VIA*>   vias;
    std::vector<PCB_TRACK*> traces;

    for( BOARD_ITEM* item : aItems )
    {
        if( item->Type() == PCB_VIA_T )
        {
            PCB_VIA* via = static_cast<PCB_VIA*>( item );

            if( via->GetViaType() != VIATYPE::MICROVIA )
                return nullptr;

            if( dynamic_cast<PCB_GENERATOR*>( via->GetParentGroup() ) )
                return nullptr;

            if( !vias.empty() && via->GetNetCode() != vias.front()->GetNetCode() )
                return nullptr;

            vias.push_back( via );
        }
        else if( item->Type() == PCB_TRACE_T )
        {
            traces.push_back( static_cast<PCB_TRACK*>( item ) );
        }
        else
        {
            return nullptr;
        }
    }

    if( vias.size() < 2 )
        return nullptr;

    std::vector<PCB_LAYER_ID> order;
    std::map<int, int>        ordinals;
    layerOrder( aBoard, order, ordinals );

    // Each via must span one hop and together they must tile a contiguous range.
    std::map<int, PCB_VIA*> hopByUpper;

    for( PCB_VIA* via : vias )
    {
        auto top = ordinals.find( via->TopLayer() );
        auto bot = ordinals.find( via->BottomLayer() );

        if( top == ordinals.end() || bot == ordinals.end() )
            return nullptr;

        if( std::abs( top->second - bot->second ) != 1 )
            return nullptr;

        int upper = std::min( top->second, bot->second );

        if( hopByUpper.count( upper ) )
            return nullptr;

        hopByUpper[upper] = via;
    }

    int first = hopByUpper.begin()->first;
    int last = hopByUpper.rbegin()->first;

    if( (int) hopByUpper.size() != last - first + 1 )
        return nullptr;

    PCB_VIA* topVia = hopByUpper.begin()->second;

    bool coaxial = true;

    for( PCB_VIA* via : vias )
    {
        if( via->GetPosition() != topVia->GetPosition() )
            coaxial = false;
    }

    PCB_VIA_STACK* stack = new PCB_VIA_STACK( aBoard, F_Cu );

    stack->SetStartLayer( order[first] );
    stack->SetEndLayer( order[last + 1] );
    stack->SetViaSize( topVia->GetWidth( PADSTACK::ALL_LAYERS ) );
    stack->SetViaDrill( topVia->GetDrillValue() );
    stack->SetNetCode( topVia->GetNetCode() );
    stack->SetFilled( topVia->Padstack().Drill().is_filled.value_or( aBoard->GetDesignSettings().m_FillVias ) );

    // Only a hop reaching an outer layer carries capping, so only those can say the stack is capped.
    bool capped = false;

    for( PCB_VIA* via : vias )
    {
        if( IsExternalCopperLayer( via->TopLayer() ) || IsExternalCopperLayer( via->BottomLayer() ) )
            capped = capped || via->Padstack().Drill().is_capped.value_or( false );
    }

    stack->SetCapped( capped );
    stack->SetPosition( topVia->GetPosition() );

    if( coaxial )
    {
        stack->SetStyle( VIA_STACK_STYLE::STACKED );
        stack->SetFilled( true );
    }
    else
    {
        stack->SetStyle( VIA_STACK_STYLE::STAGGERED );

        SHAPE_LINE_CHAIN hops;

        for( const auto& [upper, via] : hopByUpper )
            hops.Append( via->GetPosition(), true );

        stack->SetHops( hops );
        stack->SetPitch( ( hops.CPoint( 1 ) - hops.CPoint( 0 ) ).EuclideanNorm() );
    }

    // Only traces that connect hop positions on a shared landing layer belong to the stack,
    // anything else must stay loose or the next regenerate would delete it.
    if( aMembers )
    {
        std::set<int>      landing;
        std::set<VECTOR2I> viaPositions;

        for( int i = first + 1; i <= last; ++i )
            landing.insert( order[i] );

        for( PCB_VIA* via : vias )
            viaPositions.insert( via->GetPosition() );

        for( PCB_VIA* via : vias )
            aMembers->push_back( via );

        for( PCB_TRACK* trace : traces )
        {
            if( landing.count( trace->GetLayer() ) && trace->GetStart() != trace->GetEnd()
                && trace->GetNetCode() == stack->GetNetCode() && viaPositions.count( trace->GetStart() )
                && viaPositions.count( trace->GetEnd() ) )
            {
                aMembers->push_back( trace );
            }
        }
    }

    return stack;
}


// A loose microvia crossing more than one layer step is what an expansion turns into a stack.
static bool isExpandableMicrovia( const std::map<int, int>& aOrdinals, PCB_VIA* aVia )
{
    if( aVia->GetViaType() != VIATYPE::MICROVIA || aVia->GetParentGroup() )
        return false;

    auto top = aOrdinals.find( aVia->TopLayer() );
    auto bot = aOrdinals.find( aVia->BottomLayer() );

    if( top == aOrdinals.end() || bot == aOrdinals.end() )
        return false;

    return std::abs( top->second - bot->second ) > 1;
}


std::set<KIID> PCB_VIA_STACK::CollectExpandableMicrovias( BOARD* aBoard )
{
    std::vector<PCB_LAYER_ID> order;
    std::map<int, int>        ordinals;
    layerOrder( aBoard, order, ordinals );

    std::set<KIID> ids;

    for( PCB_TRACK* track : aBoard->Tracks() )
    {
        if( track->Type() == PCB_VIA_T && isExpandableMicrovia( ordinals, static_cast<PCB_VIA*>( track ) ) )
            ids.insert( track->m_Uuid );
    }

    return ids;
}


int PCB_VIA_STACK::ExpandMultiHopMicrovias( BOARD* aBoard, BOARD_COMMIT* aCommit,
                                            const std::function<const VIA_STACK_PRESET*( PCB_VIA* )>& aMatcher )
{
    std::vector<PCB_LAYER_ID> order;
    std::map<int, int>        ordinals;
    layerOrder( aBoard, order, ordinals );

    std::vector<std::pair<PCB_VIA*, const VIA_STACK_PRESET*>> candidates;

    for( PCB_TRACK* track : aBoard->Tracks() )
    {
        if( track->Type() != PCB_VIA_T )
            continue;

        PCB_VIA* via = static_cast<PCB_VIA*>( track );

        if( !isExpandableMicrovia( ordinals, via ) )
            continue;

        const VIA_STACK_PRESET* preset = nullptr;

        if( aMatcher )
        {
            preset = aMatcher( via );

            if( !preset )
                continue;
        }

        candidates.push_back( { via, preset } );
    }

    for( const auto& [via, preset] : candidates )
    {
        PCB_LAYER_ID top;
        PCB_LAYER_ID bottom;
        via->LayerPair( &top, &bottom );

        PCB_VIA_STACK* stack = new PCB_VIA_STACK( aBoard, F_Cu );

        if( preset )
            stack->ApplyPreset( *preset );

        stack->SetStartLayer( top );
        stack->SetEndLayer( bottom );
        stack->SetStyle( VIA_STACK_STYLE::STACKED );
        stack->SetViaSize( via->GetWidth( PADSTACK::ALL_LAYERS ) );
        stack->SetViaDrill( via->GetDrillValue() );
        stack->SetNetCode( via->GetNetCode() );
        stack->SetFilled( true );
        stack->SetPosition( via->GetPosition() );

        if( aCommit )
        {
            aCommit->Remove( via );
            aCommit->Add( stack );
        }
        else
        {
            aBoard->Remove( via );
            delete via;
            aBoard->Add( stack, ADD_MODE::APPEND, false );
        }

        stack->Regenerate( aBoard, aCommit );
    }

    return (int) candidates.size();
}


void PCB_VIA_STACK::ShowPropertiesDialog( PCB_BASE_EDIT_FRAME* aEditFrame )
{
    // Dialog edits a scratch copy so the undo snapshot below sees the pre-edit settings.
    PCB_VIA_STACK temp( GetBoard(), GetLayer() );
    temp.ApplyPreset( ToPreset() );

    DIALOG_MICROVIA_STACK dlg( aEditFrame, &temp );

    if( dlg.ShowModal() != wxID_OK )
        return;

    BOARD_COMMIT    commit( aEditFrame );
    GENERATOR_TOOL* tool = aEditFrame->GetToolManager()->GetTool<GENERATOR_TOOL>();

    commit.Modify( this );
    ApplyPreset( temp.ToPreset() );

    // A staggered stack stores explicit hop positions, so re-space them to the (possibly new)
    // pitch while keeping each hop's direction. Without this, editing the pitch changes the
    // value but not the distance between vias.
    if( m_style == VIA_STACK_STYLE::STAGGERED && m_hops && m_hops->PointCount() >= 2 )
    {
        SHAPE_LINE_CHAIN rescaled;
        rescaled.Append( m_hops->CPoint( 0 ) );

        for( int i = 1; i < m_hops->PointCount(); ++i )
        {
            VECTOR2I dir = m_hops->CPoint( i ) - m_hops->CPoint( i - 1 );

            if( dir.EuclideanNorm() > 0 )
                dir = dir.Resize( m_pitch );

            rescaled.Append( rescaled.CPoint( i - 1 ) + dir, true );
        }

        SetHops( rescaled );
    }

    EditStart( tool, GetBoard(), &commit );

    // If the stack has no net, inherit it from the copper it now sits on. This is the
    // supported way to net a stack, the connecting traces are generator owned and locked.
    // SetNetCode writes through to the members, so it has to follow EditStart staging them.
    if( GetNetCode() == 0 && IsSpanValid( GetBoard(), m_startLayer, m_endLayer ) )
    {
        LSET span;

        for( PCB_LAYER_ID layer : LAYER_RANGE( m_startLayer, m_endLayer, GetBoard()->GetCopperLayerCount() ) )
            span.set( layer );

        SetNetCode( FindNetAtPosition( GetBoard(), GetPosition(), span ) );
    }

    Update( tool, GetBoard(), &commit );
    EditFinish( tool, GetBoard(), &commit );

    commit.Push( GetCommitMessage() );
}


const STRING_ANY_MAP PCB_VIA_STACK::GetProperties() const
{
    STRING_ANY_MAP props = PCB_GENERATOR::GetProperties();

    props.set( "start_layer", LSET::Name( m_startLayer ) );
    props.set( "end_layer", LSET::Name( m_endLayer ) );
    props.set( "style", styleToString( m_style ) );
    props.set( "filled", m_filled );
    props.set( "capped", m_capped );
    props.set( "use_netclass", m_useNetclass );
    props.set( "preset", m_presetName );

    props.set_iu( "pitch", m_pitch );
    props.set_iu( "via_size", m_viaSize );
    props.set_iu( "via_drill", m_viaDrill );

    if( m_hops )
        props.set( "hops", wxAny( *m_hops ) );

    return props;
}


void PCB_VIA_STACK::SetProperties( const STRING_ANY_MAP& aProps )
{
    PCB_GENERATOR::SetProperties( aProps );

    wxString layerName;

    if( aProps.get_to( "start_layer", layerName ) )
        SetStartLayer( layerFromName( layerName, m_startLayer ) );

    if( aProps.get_to( "end_layer", layerName ) )
        m_endLayer = layerFromName( layerName, m_endLayer );

    wxString style;

    if( aProps.get_to( "style", style ) )
        m_style = styleFromString( style );

    aProps.get_to( "filled", m_filled );
    aProps.get_to( "capped", m_capped );
    aProps.get_to( "use_netclass", m_useNetclass );
    aProps.get_to( "preset", m_presetName );

    aProps.get_to_iu( "pitch", m_pitch );
    aProps.get_to_iu( "via_size", m_viaSize );
    aProps.get_to_iu( "via_drill", m_viaDrill );

    if( auto hops = aProps.get_opt<SHAPE_LINE_CHAIN>( "hops" ) )
        m_hops = *hops;
}


using SCOPED_DRAW_MODE = SCOPED_SET_RESET<DRAWING_TOOL::MODE>;


int DRAWING_TOOL::PlaceMicroviaStack( const TOOL_EVENT& aEvent )
{
    // Consume any router handoff seed (a pre-anchored staggered stack).
    bool         seeded = m_viaStackSeeded;
    VECTOR2I     seedPos = m_viaStackSeedPos;
    PCB_LAYER_ID seedStart = m_viaStackSeedStart;
    PCB_LAYER_ID seedEnd = m_viaStackSeedEnd;
    m_viaStackSeeded = false;

    if( m_isFootprintEditor )
        return 0;

    if( m_inDrawingTool )
        return 0;

    REENTRANCY_GUARD guard( &m_inDrawingTool );

    m_toolMgr->RunAction( ACTIONS::selectionClear );
    m_frame->PushTool( aEvent );
    Activate();

    BOARD*           board = m_frame->GetBoard();
    SCOPED_DRAW_MODE scopedDrawMode( m_mode, MODE::VIA );

    PCB_VIA_STACK settings( board, F_Cu );

    // Place silently with the preset currently selected in the toolbar, matching how via
    // placement uses the current via size. Only fall back to the properties dialog when no
    // presets are defined yet, so there is still a way to place and define one.
    const std::vector<VIA_STACK_PRESET>& presets = board->GetDesignSettings().m_ViaStackPresets;
    int                                  idx = board->GetDesignSettings().GetViaStackIndex();

    if( !presets.empty() )
    {
        settings.ApplyPreset( presets[std::clamp( idx, 0, (int) presets.size() - 1 )] );
    }
    else if( !seeded )
    {
        DIALOG_MICROVIA_STACK dlg( m_frame, &settings );

        if( dlg.ShowModal() != wxID_OK )
        {
            m_frame->PopTool( aEvent );
            return 0;
        }
    }

    // A router handoff anchors the span to the routed layers and is always a staggered walk.
    if( seeded )
    {
        settings.SetStartLayer( seedStart );
        settings.SetEndLayer( seedEnd );
        settings.SetStyle( VIA_STACK_STYLE::STAGGERED );
    }

    bool staggered = settings.GetStyle() == VIA_STACK_STYLE::STAGGERED;

    if( !PCB_VIA_STACK::IsSpanValid( board, settings.GetStartLayer(), settings.GetEndLayer() ) )
    {
        m_frame->ShowInfoBarError( _( "The microvia stack layers are not present on this board." ) );
        m_frame->PopTool( aEvent );
        return 0;
    }

    std::vector<PCB_LAYER_ID> spanLayers;

    for( PCB_LAYER_ID layer :
         LAYER_RANGE( settings.GetStartLayer(), settings.GetEndLayer(), board->GetCopperLayerCount() ) )
        spanLayers.push_back( layer );

    LSET spanSet;

    for( PCB_LAYER_ID layer : spanLayers )
        spanSet.set( layer );

    int nHops = (int) spanLayers.size() - 1;

    // A zero pitch would pin the steering circle onto a single point.
    if( staggered && settings.GetPitch() <= 0 )
    {
        int dia = settings.GetViaSize() > 0 ? settings.GetViaSize() : board->GetDesignSettings().GetCurrentViaSize();
        settings.SetPitch( dia * 2 );
    }

    if( nHops < 1 )
    {
        m_frame->PopTool( aEvent );
        return 0;
    }

    PCB_GRID_HELPER          grid( m_toolMgr, m_frame->GetMagneticItemsSettings() );
    PCB_SELECTION            preview;
    std::vector<BOARD_ITEM*> previewItems;
    std::vector<VECTOR2I>    hops;
    VECTOR2I                 lastDir( 1, 0 );
    bool                     resumeRouting = false; // seeded handoff places one and routes on
    VECTOR2I                 resumePos;
    VECTOR2I                 lastViaPos;

    // Router handoff: the first hop is fixed at the routed head, the user steers the rest.
    if( seeded )
        hops.push_back( seedPos );

    m_view->Add( &preview );

    auto clearPreview = [&]()
    {
        preview.Clear();

        for( BOARD_ITEM* item : previewItems )
            delete item;

        previewItems.clear();
    };

    // DRC context, shared with the plain via tool so a stack is refused over foreign copper the
    // same way (unless "Allow DRC violations" is enabled in the router settings).
    std::shared_ptr<DRC_ENGINE> drcEngine = board->GetDesignSettings().m_DRCEngine;
    int                         drcEpsilon = board->GetDesignSettings().GetDRCEpsilon();
    int                         worstClearance = ComputeWorstViaClearance( m_frame, drcEngine.get() );
    bool                        allowDRCViolations = false;

    if( ROUTER_TOOL* router = m_toolMgr->GetTool<ROUTER_TOOL>() )
        allowDRCViolations = router->Router()->Settings().AllowDRCViolations();

    auto updatePreview = [&]( const VECTOR2I& aCursor )
    {
        clearPreview();

        if( staggered )
        {
            SHAPE_LINE_CHAIN chain;

            for( const VECTOR2I& p : hops )
                chain.Append( p, true );

            chain.Append( aCursor, true );
            settings.SetHops( chain );
            settings.SetPosition( hops.empty() ? aCursor : hops.front() );

            // Only preview the placed hops plus the one being steered, not the
            // rest of the span extrapolated in a straight line.
            int previewHops = std::min( (int) hops.size() + 1, nHops );
            settings.SetEndLayer( spanLayers[previewHops] );
        }
        else
        {
            settings.SetPosition( aCursor );
        }

        previewItems = settings.BuildMembers( board, 0 );

        for( BOARD_ITEM* item : previewItems )
            preview.Add( item );

        m_view->Update( &preview );
    };

    auto placeStack = [&]( const VECTOR2I& aCursor ) -> bool
    {
        BOARD_COMMIT    commit( m_frame );
        PCB_VIA_STACK*  stack = static_cast<PCB_VIA_STACK*>( settings.Clone() );
        GENERATOR_TOOL* genTool = m_toolMgr->GetTool<GENERATOR_TOOL>();

        // Clone keeps the source uuid, each placed stack needs its own.
        stack->SetUuidDirect( KIID() );
        stack->SetParent( board );

        // The preview may have truncated the span, place the full one.
        stack->SetEndLayer( spanLayers.back() );

        if( staggered )
        {
            SHAPE_LINE_CHAIN chain;

            for( const VECTOR2I& p : hops )
                chain.Append( p, true );

            stack->SetHops( chain );
            stack->SetPosition( hops.front() );
        }
        else
        {
            stack->ClearHops();
            stack->SetPosition( aCursor );
        }

        // Inherit the net of whatever the first hop lands on.
        stack->SetNetCode( PCB_VIA_STACK::FindNetAtPosition( board, stack->GetPosition(), spanSet ) );
        stack->SetFlags( IS_NEW );

        // Refuse placement over foreign copper, matching the plain via tool. Build the
        // stack's vias off-board and check each before anything is committed.
        std::vector<BOARD_ITEM*> candidates = stack->BuildMembers( board, stack->GetNetCode() );
        bool                     violates = false;

        for( BOARD_ITEM* item : candidates )
        {
            if( BOARD_CONNECTED_ITEM* copper = dynamic_cast<BOARD_CONNECTED_ITEM*>( item ) )
            {
                if( CheckItemDRCViolation( copper, m_frame, drcEngine.get(), worstClearance, drcEpsilon ) )
                {
                    violates = true;
                    break;
                }
            }
        }

        for( BOARD_ITEM* item : candidates )
            delete item;

        if( violates && !allowDRCViolations )
        {
            m_frame->ShowInfoBarError( _( "Microvia stack location violates DRC." ), true,
                                       WX_INFOBAR::MESSAGE_TYPE::DRC_VIOLATION );
            delete stack;
            return false;
        }

        stack->EditStart( genTool, board, &commit );
        stack->Update( genTool, board, &commit );
        stack->EditFinish( genTool, board, &commit );

        stack->ClearFlags( IS_NEW );

        // Only the last hop reaches the end layer, and a staggered one is not at the cursor.
        for( BOARD_ITEM* item : stack->GetBoardItems() )
        {
            PCB_VIA* via = dynamic_cast<PCB_VIA*>( item );

            if( via && via->IsOnLayer( stack->GetEndLayer() ) )
            {
                lastViaPos = via->GetPosition();
                break;
            }
        }

        commit.Push( _( "Place Microvia Stack" ) );
        return true;
    };

    // Snap the anchor onto nearby copper, using the plain via tool's routines so tracks,
    // arcs and pads all behave the same. A stack may land on any layer it spans, so the
    // probe carries the whole span rather than a single layer.
    int snapViaDia = settings.GetViaSize() > 0 ? settings.GetViaSize() : board->GetDesignSettings().GetCurrentViaSize();

    PCB_VIA probe( board );
    probe.SetViaType( VIATYPE::MICROVIA );
    probe.SetWidth( PADSTACK::ALL_LAYERS, snapViaDia );
    probe.SetLayerPair( settings.GetStartLayer(), settings.GetEndLayer() );

    auto snapToCopper = [&]( const VECTOR2I& aCursor, bool aItemSnap ) -> VECTOR2I
    {
        MAGNETIC_SETTINGS* mag = m_frame->GetMagneticItemsSettings();

        // The loop clears grid snap for the ResolveSnap base, re-enable it here so
        // AlignToSegment actually aligns, matching VIA_PLACER::SnapItem.
        grid.SetSnap( aItemSnap );

        if( aItemSnap && mag )
        {
            if( mag->tracks != MAGNETIC_OPTIONS::NO_EFFECT )
            {
                if( PCB_TRACK* track = FindSnapTrack( m_frame, &probe, aCursor, spanSet, ViaSnapRange( m_frame ) ) )
                    return grid.AlignToSegment( aCursor, SEG( track->GetStart(), track->GetEnd() ) );
            }

            if( mag->pads != MAGNETIC_OPTIONS::NO_EFFECT )
            {
                if( PAD* pad = FindSnapPad( m_frame, &probe, aCursor, spanSet, ViaSnapRange( m_frame ) ) )
                    return pad->GetPosition();
            }
        }

        return grid.ResolveSnap( aCursor, nullptr ).position;
    };

    auto setCursor = [&]()
    {
        m_frame->GetCanvas()->SetCurrentCursor( KICURSOR::BULLSEYE );
    };

    m_controls->ShowCursor( true );
    m_controls->SetAutoPan( true );
    setCursor();

    while( TOOL_EVENT* evt = Wait() )
    {
        setCursor();

        // Match the via tool: item snaps are handled below, the grid helper only does
        // grid and anchor snapping against the GAL grid state.
        grid.SetSnap( false );
        grid.SetUseGrid( getView()->GetGAL()->GetGridSnapping() && !evt->DisableGridSnapping() );

        bool     allowItemSnap = !evt->Modifier( MD_SHIFT );
        VECTOR2I rawCursor = m_controls->GetMousePosition();
        VECTOR2I cursorPos;

        if( staggered && !hops.empty() )
        {
            // Cursor is locked onto a circle of radius = pitch around the previous hop.
            // It steers direction only, following the editor angle mode.
            VECTOR2I dir = rawCursor - hops.back();

            if( dir.EuclideanNorm() < 1 )
                dir = VECTOR2I( 1, 0 );

            LEADER_MODE angleSnap = GetAngleSnapMode();

            if( evt->Modifier( MD_CTRL ) )
                angleSnap = LEADER_MODE::DIRECT;

            if( angleSnap != LEADER_MODE::DIRECT )
            {
                double step = ( angleSnap == LEADER_MODE::DEG90 ) ? M_PI / 2 : M_PI / 4;
                double ang = atan2( (double) dir.y, (double) dir.x );
                ang = round( ang / step ) * step;

                cursorPos = hops.back()
                            + VECTOR2I( KiROUND( cos( ang ) * settings.GetPitch() ),
                                        KiROUND( sin( ang ) * settings.GetPitch() ) );
            }
            else
            {
                cursorPos = hops.back() + dir.Resize( settings.GetPitch() );
            }

            lastDir = cursorPos - hops.back();
        }
        else
        {
            cursorPos = snapToCopper( rawCursor, allowItemSnap );
        }

        // Only the staggered pitch circle locks the cursor. For the anchor and the
        // stacked case let the cross follow the mouse and snap the preview item instead,
        // exactly like plain via placement.
        if( staggered && !hops.empty() )
            m_controls->ForceCursorPosition( true, cursorPos );
        else
            m_controls->ForceCursorPosition( false );

        if( evt->IsCancelInteractive() || evt->IsActivate() )
        {
            break;
        }
        else if( evt->IsClick( BUT_LEFT ) || evt->IsDblClick( BUT_LEFT ) )
        {
            // The tool stays active for repeated placement, Esc leaves. Rapid clicks
            // arrive as double clicks and must place hops too.
            if( staggered )
            {
                hops.push_back( cursorPos );

                if( (int) hops.size() >= nHops )
                {
                    if( !placeStack( cursorPos ) )
                    {
                        // Blocked by DRC, let the user reposition the final hop and retry.
                        hops.pop_back();
                    }
                    else
                    {
                        hops.clear();
                        clearPreview();

                        // A router handoff places exactly one stack, then returns to routing.
                        if( seeded )
                        {
                            resumeRouting = true;
                            resumePos = lastViaPos;
                            break;
                        }
                    }
                }
                else
                {
                    // Show the next hop right away, continuing in the last direction,
                    // instead of waiting for the mouse to move.
                    cursorPos = hops.back() + lastDir.Resize( settings.GetPitch() );
                    m_controls->ForceCursorPosition( true, cursorPos );
                    updatePreview( cursorPos );
                }
            }
            else
            {
                if( placeStack( cursorPos ) )
                {
                    clearPreview();

                    if( seeded )
                    {
                        resumeRouting = true;
                        resumePos = lastViaPos;
                        break;
                    }
                }
            }
        }
        else if( evt->IsAction( &ACTIONS::undo ) || evt->IsAction( &ACTIONS::doDelete ) )
        {
            // Swallowed for the whole loop like the router does, or it would undo the
            // route this placement was handed off from.
            if( staggered && hops.size() > ( seeded ? 1u : 0u ) )
            {
                hops.pop_back();
                lastDir = VECTOR2I( 1, 0 );
                updatePreview( cursorPos );
            }
        }
        else if( evt->IsClick( BUT_RIGHT ) )
        {
            m_toolMgr->VetoContextMenuMouseWarp();
            m_menu->ShowContextMenu( selection() );
        }
        else if( evt->IsMotion() )
        {
            updatePreview( cursorPos );
        }
        else
        {
            evt->SetPassEvent();
        }
    }

    clearPreview();
    m_view->Remove( &preview );
    m_controls->ForceCursorPosition( false );
    m_controls->SetAutoPan( false );
    m_frame->GetCanvas()->SetCurrentCursor( KICURSOR::ARROW );
    m_frame->PopTool( aEvent );

    // Return to routing from the last placed via when handed off from the router. Do not warp
    // the view (last arg false), recentering the canvas mid-workflow is jarring, unlike a via.
    // Priming starts the route at the via itself, not at whatever the next click snaps to.
    if( resumeRouting )
    {
        m_frame->SetActiveLayer( settings.GetEndLayer() );

        if( ROUTER_TOOL* router = m_toolMgr->GetTool<ROUTER_TOOL>() )
            router->SetViaStackResumeLayer( settings.GetEndLayer() );

        m_controls->WarpMouseCursor( resumePos, true, false );
        m_toolMgr->RunAction( PCB_ACTIONS::routeSingleTrack );
        m_toolMgr->PrimeTool( resumePos );
    }

    return 0;
}


static GENERATORS_MGR::REGISTER<PCB_VIA_STACK> registerMe;


IMPLEMENT_ENUM_TO_WXANY( VIA_STACK_STYLE )


static struct PCB_VIA_STACK_DESC
{
    PCB_VIA_STACK_DESC()
    {
        ENUM_MAP<VIA_STACK_STYLE>::Instance()
                .Map( VIA_STACK_STYLE::STACKED, _HKI( "Stacked" ) )
                .Map( VIA_STACK_STYLE::STAGGERED, _HKI( "Staggered" ) );

        PROPERTY_MANAGER& propMgr = PROPERTY_MANAGER::Instance();
        REGISTER_TYPE( PCB_VIA_STACK );
        propMgr.AddTypeCast( new TYPE_CAST<PCB_VIA_STACK, PCB_GENERATOR> );
        propMgr.AddTypeCast( new TYPE_CAST<PCB_VIA_STACK, BOARD_ITEM> );
        propMgr.InheritsAfter( TYPE_HASH( PCB_VIA_STACK ), TYPE_HASH( PCB_GENERATOR ) );
        propMgr.InheritsAfter( TYPE_HASH( PCB_VIA_STACK ), TYPE_HASH( BOARD_ITEM ) );

        ENUM_MAP<PCB_LAYER_ID>& layerEnum = ENUM_MAP<PCB_LAYER_ID>::Instance();

        if( layerEnum.Choices().GetCount() == 0 )
        {
            layerEnum.Undefined( UNDEFINED_LAYER );

            for( PCB_LAYER_ID layer : LSET::AllLayersMask() )
                layerEnum.Map( layer, LSET::Name( layer ) );
        }

        // A stack spans layers, so the single inherited layer is meaningless, as for a via.
        propMgr.Mask( TYPE_HASH( PCB_VIA_STACK ), TYPE_HASH( BOARD_ITEM ), _HKI( "Layer" ) );

        // Read-only: a stack's geometry is rebuilt from these, and the Properties panel has no
        // way to trigger that rebuild. Editing goes through the stack's own dialog.
        const wxString groupStack = _HKI( "Microvia Stack Properties" );

        auto startLayer = new PROPERTY_ENUM<PCB_VIA_STACK, PCB_LAYER_ID>(
                _HKI( "Start Layer" ), NO_SETTER( PCB_VIA_STACK, PCB_LAYER_ID ), &PCB_VIA_STACK::GetStartLayer );
        startLayer->SetChoices( layerEnum.Choices() );
        propMgr.AddProperty( startLayer, groupStack );

        auto endLayer = new PROPERTY_ENUM<PCB_VIA_STACK, PCB_LAYER_ID>(
                _HKI( "End Layer" ), NO_SETTER( PCB_VIA_STACK, PCB_LAYER_ID ), &PCB_VIA_STACK::GetEndLayer );
        endLayer->SetChoices( layerEnum.Choices() );
        propMgr.AddProperty( endLayer, groupStack );

        propMgr.AddProperty(
                new PROPERTY_ENUM<PCB_VIA_STACK, VIA_STACK_STYLE>(
                        _HKI( "Style" ), NO_SETTER( PCB_VIA_STACK, VIA_STACK_STYLE ), &PCB_VIA_STACK::GetStyle ),
                groupStack );

        propMgr.AddProperty( new PROPERTY_ENUM<PCB_VIA_STACK, int>( _HKI( "Net" ), NO_SETTER( PCB_VIA_STACK, int ),
                                                                    &PCB_VIA_STACK::GetNetCode, PT_NET ) );

        propMgr.AddProperty( new PROPERTY<PCB_VIA_STACK, bool>( _HKI( "Use Netclass Values" ),
                                                                NO_SETTER( PCB_VIA_STACK, bool ),
                                                                &PCB_VIA_STACK::GetUseNetclass ),
                             groupStack );

        propMgr.AddProperty( new PROPERTY<PCB_VIA_STACK, int>( _HKI( "Via Diameter" ), NO_SETTER( PCB_VIA_STACK, int ),
                                                               &PCB_VIA_STACK::GetViaSize, PROPERTY_DISPLAY::PT_SIZE ),
                             groupStack );

        propMgr.AddProperty( new PROPERTY<PCB_VIA_STACK, int>( _HKI( "Via Hole" ), NO_SETTER( PCB_VIA_STACK, int ),
                                                               &PCB_VIA_STACK::GetViaDrill, PROPERTY_DISPLAY::PT_SIZE ),
                             groupStack );

        auto isStaggered = []( INSPECTABLE* aItem ) -> bool
        {
            if( PCB_VIA_STACK* stack = dynamic_cast<PCB_VIA_STACK*>( aItem ) )
                return stack->GetStyle() == VIA_STACK_STYLE::STAGGERED;

            return false;
        };

        propMgr.AddProperty( new PROPERTY<PCB_VIA_STACK, int>( _HKI( "Pitch" ), NO_SETTER( PCB_VIA_STACK, int ),
                                                               &PCB_VIA_STACK::GetPitch, PROPERTY_DISPLAY::PT_SIZE ),
                             groupStack )
                .SetAvailableFunc( isStaggered );

        propMgr.AddProperty( new PROPERTY<PCB_VIA_STACK, bool>( _HKI( "Copper-filled" ),
                                                                NO_SETTER( PCB_VIA_STACK, bool ),
                                                                &PCB_VIA_STACK::IsFilled ),
                             groupStack );

        propMgr.AddProperty( new PROPERTY<PCB_VIA_STACK, bool>( _HKI( "Capped" ), NO_SETTER( PCB_VIA_STACK, bool ),
                                                                &PCB_VIA_STACK::IsCapped ),
                             groupStack );
    }
} _PCB_VIA_STACK_DESC;