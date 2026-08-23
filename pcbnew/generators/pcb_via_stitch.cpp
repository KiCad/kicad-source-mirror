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

#include <pcb_via_stitch.h>

#include <dialogs/dialog_via_stitch_properties.h>
#include <pcb_base_edit_frame.h>
#include <tool/tool_manager.h>
#include <tools/generator_tool.h>
#include <board_commit.h>

#include <cmath>
#include <map>
#include <vector>

#include <boost/random/mersenne_twister.hpp>
#include <boost/random/uniform_real_distribution.hpp>

#include <geometry/poisson_disk.h>
#include <geometry/seg.h>
#include <geometry/shape_line_chain.h>
#include <geometry/shape_rect.h>
#include <gal/graphics_abstraction_layer.h>
#include <gal/painter.h>
#include <tool/action_menu.h>
#include <tools/pcb_actions.h>
#include <view/view.h>
#include <zone.h>
#include <pad.h>

#include <pcb_track.h>
#include <board.h>
#include <board_design_settings.h>
#include <convert_basic_shapes_to_polygon.h>
#include <footprint.h>
#include <geometry/shape_segment.h>
#include <drc/drc_engine.h>
#include <drc/drc_rtree.h>
#include <drc/drc_rule.h>
#include <properties/property.h>
#include <properties/property_mgr.h>
#include <drc/drc_cache_generator.h>

const wxString PCB_VIA_STITCH::DISPLAY_NAME = _HKI( "Via Stitching" );
const wxString PCB_VIA_STITCH::GENERATOR_TYPE = wxS( "via_stitch" );


const std::vector<VECTOR2D>& PCB_VIA_STITCH::bakedPoissonTile()
{
    static const std::vector<VECTOR2D> samples = POISSON_DISK::ToroidalUnitTile(
            1.0 / POISSON_TILE_PITCHES, POISSON_TILE_SEED );
    return samples;
}


PCB_VIA_STITCH::PCB_VIA_STITCH( BOARD_ITEM* aParent ) :
        PCB_GENERATOR_POLY( aParent, F_Cu ),
        m_viaTemplate( std::make_unique<PCB_VIA>( this ) )
{
    m_generatorType = GENERATOR_TYPE;
    m_name = DISPLAY_NAME;


    m_viaTemplate->SetIsFree( true );
}


PCB_VIA_STITCH::PCB_VIA_STITCH( const PCB_VIA_STITCH& aOther ) :
        PCB_GENERATOR_POLY( aOther ),
        m_pitch( aOther.m_pitch ),
        m_layout( aOther.m_layout ),
        m_mode( aOther.m_mode ),
        m_seed( aOther.m_seed ),
        m_viaTemplate( std::make_unique<PCB_VIA>( *aOther.m_viaTemplate ) ),
        m_lastNetName( aOther.m_lastNetName ),
        m_netCode( aOther.m_netCode ),
        m_lastGuardedNetName( aOther.m_lastGuardedNetName ),
        m_guardedNetCode( aOther.m_guardedNetCode ),
        m_excludedCells( aOther.m_excludedCells ),
        m_excludedPositions( aOther.m_excludedPositions ),
        m_childGridConfig( aOther.m_childGridConfig ),
        m_originOffset( aOther.m_originOffset )
{
    // m_childContextMenu is intentionally not copied
}


PCB_VIA_STITCH::PCB_VIA_STITCH( PCB_VIA_STITCH&& aOther ) noexcept = default;

PCB_VIA_STITCH& PCB_VIA_STITCH::operator=( const PCB_VIA_STITCH& aOther )
{
    if( this != &aOther )
    {
        PCB_GENERATOR_POLY::operator=( aOther );
        m_pitch = aOther.m_pitch;
        m_layout = aOther.m_layout;
        m_mode = aOther.m_mode;
        m_seed = aOther.m_seed;
        m_viaTemplate = std::make_unique<PCB_VIA>( *aOther.m_viaTemplate );
        m_lastNetName = aOther.m_lastNetName;
        m_netCode = aOther.m_netCode;
        m_lastGuardedNetName = aOther.m_lastGuardedNetName;
        m_guardedNetCode = aOther.m_guardedNetCode;
        m_excludedCells = aOther.m_excludedCells;
        m_excludedPositions = aOther.m_excludedPositions;
        m_childGridConfig = aOther.m_childGridConfig;
        m_originOffset = aOther.m_originOffset;
        // m_childContextMenu is intentionally not copied.
        m_childContextMenu.reset();
    }
    return *this;
}

PCB_VIA_STITCH& PCB_VIA_STITCH::operator=( PCB_VIA_STITCH&& aOther ) noexcept = default;

PCB_VIA_STITCH::~PCB_VIA_STITCH() = default;


int PCB_VIA_STITCH::GetViaSize() const
{
    return m_viaTemplate->GetWidth( PADSTACK::ALL_LAYERS );
}


void PCB_VIA_STITCH::SetViaSize( int aVal )
{
    m_viaTemplate->SetWidth( aVal );
    MarkDirty();
}


int PCB_VIA_STITCH::GetViaDrill() const
{
    int d = m_viaTemplate->GetDrillValue();
    return d > 0 ? d : 0;
}


void PCB_VIA_STITCH::SetViaDrill( int aVal )
{
    m_viaTemplate->SetDrill( aVal );
    MarkDirty();
}


std::vector<std::pair<wxString, const BOARD_ITEM*>> PCB_VIA_STITCH::GetTemplateItems() const
{
    return { { wxS( "via" ), m_viaTemplate.get() } };
}


void PCB_VIA_STITCH::SetTemplateItem( const wxString& aName,
                                      std::unique_ptr<BOARD_ITEM> aItem )
{
    if( aName != wxS( "via" ) || !aItem || aItem->Type() != PCB_VIA_T )
        return;

    m_viaTemplate.reset( static_cast<PCB_VIA*>( aItem.release() ) );
    m_viaTemplate->SetParent( this );
    m_viaTemplate->SetIsFree( true );
}

void PCB_VIA_STITCH::ViewDraw( int aLayer, KIGFX::VIEW* aView ) const
{
    if( m_outline.OutlineCount() == 0 )
        return;

    // Only paint on the outline layer.  ViewGetLayers() also lists LAYER_VIAS /
    // LAYER_VIA_HOLES / LAYER_ANCHOR for the produced vias' rendering, but those are
    // handled by the via children themselves; we don't want to redraw the outline
    // multiple times per frame.
    if( aLayer != LAYER_VIA_STITCHING )
        return;

    KIGFX::GAL* gal = aView->GetGAL();

    // We are drawing perpendicular ticks along the outline to indicate the stitch zone
    // It kind of looks like a sewing pattern with the ticks, so it fits.
    KIGFX::RENDER_SETTINGS* settings = aView->GetPainter()->GetSettings();
    const KIGFX::COLOR4D    borderColor = settings->GetColor( this, LAYER_VIA_STITCHING );

    const SHAPE_LINE_CHAIN& outline = m_outline.COutline( 0 );
    const int               n       = outline.PointCount();

    gal->SetIsFill( false );
    gal->SetIsStroke( true );
    gal->SetStrokeColor( borderColor );
    gal->SetLineWidth( pcbIUScale.mmToIU( 0.1 ) );
    gal->DrawPolyline( outline );

    const double tickSpacing = pcbIUScale.mmToIU( 2.0 );
    const double tickLength  = pcbIUScale.mmToIU( 0.5 );

    // Approximate centroid — used to resolve which perpendicular direction is inward
    // regardless of polygon winding order.
    VECTOR2D centroid( 0.0, 0.0 );
    for( int i = 0; i < n; ++i )
    {
        centroid.x += outline.CPoint( i ).x;
        centroid.y += outline.CPoint( i ).y;
    }
    centroid.x /= n;
    centroid.y /= n;

    gal->SetLineWidth( 1 ); // hairline ticks

    for( int i = 0; i < n; ++i )
    {
        const VECTOR2D p1( outline.CPoint( i ) );
        const VECTOR2D p2( outline.CPoint( ( i + 1 ) % n ) );

        const VECTOR2D edgeVec = p2 - p1;
        const double   edgeLen = edgeVec.EuclideanNorm();

        if( edgeLen < 1.0 )
            continue;

        const VECTOR2D edgeDir = edgeVec / edgeLen;

        // Pick the perpendicular that points toward the centroid
        const VECTOR2D edgeMid  = ( p1 + p2 ) / 2.0;
        const VECTOR2D perp     = VECTOR2D( -edgeDir.y, edgeDir.x );
        const VECTOR2D toCentr  = centroid - edgeMid;
        const bool     perpToward = ( perp.x * toCentr.x + perp.y * toCentr.y >= 0.0 );
        const VECTOR2D inward     = perpToward ? perp : VECTOR2D( -perp.x, -perp.y );

        for( double t = tickSpacing; t < edgeLen; t += tickSpacing )
        {
            const VECTOR2D base = p1 + edgeDir * t;
            gal->DrawLine( base, base + inward * tickLength );
        }
    }
}


VECTOR2I PCB_VIA_STITCH::GetPosition() const
{
    SHAPE_POLY_SET::VERTEX_INDEX index;

    // An empty outline has no vertex 0 (e.g. a freshly constructed generator before the
    // outline is set); fall back to the anchor origin rather than throwing.
    if( !m_outline.GetRelativeIndices( 0, &index ) )
        return m_origin;

    return m_outline.CVertex( index );
}


void PCB_VIA_STITCH::InitializeDefaults( BOARD* aBoard )
{
    if( m_viaTemplate->GetWidth( PADSTACK::ALL_LAYERS ) <= 0 )
        m_viaTemplate->SetWidth( defaultViaSize( aBoard ) );

    if( m_viaTemplate->GetDrillValue() <= 0 )
        m_viaTemplate->SetDrill( defaultViaDrill( aBoard ) );

    if( m_pitch <= 0 )
    {
        m_pitch = defaultPitch( aBoard );

        m_childGridConfig.pitch = m_pitch;
    }
}


void PCB_VIA_STITCH::EditStart( GENERATOR_TOOL* aTool, BOARD* aBoard, BOARD_COMMIT* aCommit )
{
    if( aCommit )
    {
        if( IsNew() )
            aCommit->Add( this );
        else
            aCommit->Modify( this );
    }

    // Initialize property defaults on first edit
    InitializeDefaults( aBoard );

    // Resolve net code from saved name (SetProperties has no board reference)
    if( m_netCode == 0 && !m_lastNetName.empty() && aBoard )
    {
        if( NETINFO_ITEM* net = aBoard->FindNet( m_lastNetName ) )
            m_netCode = net->GetNetCode();
    }

    if( m_guardedNetCode == 0 && !m_lastGuardedNetName.empty() && aBoard )
    {
        if( NETINFO_ITEM* net = aBoard->FindNet( m_lastGuardedNetName ) )
            m_guardedNetCode = net->GetNetCode();
    }

    SetFlags( IN_EDIT );
}


int PCB_VIA_STITCH::defaultPitch( BOARD* aBoard ) const
{
    int mm2 = pcbIUScale.mmToIU( 2.0 );
    int viaSize = m_viaTemplate->GetWidth( PADSTACK::ALL_LAYERS ) > 0 ? m_viaTemplate->GetWidth( PADSTACK::ALL_LAYERS )
                                                : defaultViaSize( aBoard );
    int pitch = std::max( mm2, viaSize * 2 );
    return pitch;
}


int PCB_VIA_STITCH::defaultViaSize( BOARD* aBoard ) const
{
    if( !aBoard )
        return pcbIUScale.mmToIU( 0.6 );

    BOARD_DESIGN_SETTINGS& ds = aBoard->GetDesignSettings();
    int val = ds.GetCurrentViaSize();
    if( val <= 0 )
        val = pcbIUScale.mmToIU( 0.6 );
    return val;
}

int PCB_VIA_STITCH::defaultViaDrill( BOARD* aBoard ) const
{
    if( !aBoard )
        return pcbIUScale.mmToIU( 0.3 );

    BOARD_DESIGN_SETTINGS& ds = aBoard->GetDesignSettings();
    int val = ds.GetCurrentViaDrill();
    if( val <= 0 )
        val = pcbIUScale.mmToIU( 0.3 );
    return val;
}


bool PCB_VIA_STITCH::Update( GENERATOR_TOOL* aTool, BOARD* aBoard, BOARD_COMMIT* aCommit )
{
    if( !( GetFlags() & IN_EDIT ) )
        return false;

    if( !aBoard || !aCommit )
        return false;

    // Resolve net code from the saved name in case EditStart() was not called
    // (e.g. programmatic regeneration after load)
    if( m_netCode == 0 && !m_lastNetName.empty() )
    {
        if( NETINFO_ITEM* net = aBoard->FindNet( m_lastNetName ) )
            m_netCode = net->GetNetCode();
    }

    if( m_guardedNetCode == 0 && !m_lastGuardedNetName.empty() )
    {
        if( NETINFO_ITEM* net = aBoard->FindNet( m_lastGuardedNetName ) )
            m_guardedNetCode = net->GetNetCode();
    }

    int detectPitch = m_pitch > 0 ? m_pitch : defaultPitch( aBoard );

    // Avoid accidentally calculating a grid offset change because we just changed layout modes
    const GRID_CONFIG currentConfig{ m_layout, m_mode, detectPitch };
    const bool        childrenAreOnThisGrid = m_childGridConfig == currentConfig;

    m_childGridConfig = currentConfig;

    // Try to detect origin offset change due to moved via
    if( usesGridCells() && childrenAreOnThisGrid )
    {
        for( BOARD_ITEM* it : GetBoardItems() )
        {
            if( it->Type() != PCB_VIA_T )
                continue;

            VECTOR2I p = static_cast<PCB_VIA*>( it )->GetPosition();
            VECTOR2I expectedPos = positionForCell( cellForPosition( p ), detectPitch );

            if( expectedPos != p )
            {
                int offsetY = ( ( p.y % detectPitch ) + detectPitch ) % detectPitch;

                // The dragged via defines the new grid origin.  On a staggered layout an odd
                // row carries an extra half-pitch x shift, which must be removed before
                // computing the origin offset or the whole grid re-anchors half a pitch off.
                // (p.y - offsetY) is an exact multiple of the pitch, so this row index matches
                // what cellForPosition()/positionForCell() will derive after the re-anchor.
                int row    = ( p.y - offsetY ) / detectPitch;
                int xShift = ( m_layout == PCB_VIA_STITCH_LAYOUT::STAGGERED
                               && ( ( row % 2 + 2 ) % 2 != 0 ) ) ? detectPitch / 2 : 0;

                m_originOffset =
                        VECTOR2I( ( ( ( p.x - xShift ) % detectPitch ) + detectPitch ) % detectPitch,
                                  offsetY );
                break;
            }
        }
    }

    // Take snapshot of existing vias, we then selectively remove vias only if they should be removed
    // we want to reduce file churn of KIIDs
    std::vector<PCB_VIA*> existingVias;

    for( BOARD_ITEM* item : GetBoardItems() )
    {
        if( item->Type() == PCB_VIA_T )
            existingVias.push_back( static_cast<PCB_VIA*>( item ) );
    }

    bool changed = false;

    auto removeVia =
            [&]( PCB_VIA* aVia )
            {
                RemoveItem( aVia );
                m_pendingRemovals.insert( aVia );
                changed = true;
            };

    if( m_outline.IsEmpty() )
    {
        for( PCB_VIA* via : existingVias )
            removeVia( via );

        m_lastUpdateChangedVias = changed;
        ClearDirty();
        return false;
    }

    int pitch = m_pitch > 0 ? m_pitch : defaultPitch( aBoard );

    if( m_viaTemplate->GetWidth( PADSTACK::ALL_LAYERS ) <= 0 )
        m_viaTemplate->SetWidth( defaultViaSize( aBoard ) );
    if( m_viaTemplate->GetDrillValue() <= 0 )
        m_viaTemplate->SetDrill( defaultViaDrill( aBoard ) );

    // Compute the set of grid cells where a via fits cleanly (DRC + same-net zone overlap).
    // TODO, we will use a cache grid
    std::set<VECTOR2I> placementCells = buildPlacementCells( aBoard );

    // Guard mode currently has to exclude by a proximity heuristic since we aren't using a grid.
    const int64_t guardExcludeRadiusSq = ( (int64_t) pitch / 4 ) * ( pitch / 4 );

    auto isExcluded =
            [&]( const VECTOR2I& cell ) -> bool
            {
                // Guard vias are walked along the guarded net's envelope, so a stored position
                // will rarely land exactly on a regenerated via.  Match by proximity instead.
                if( m_mode == PCB_VIA_STITCH_MODE::GUARD )
                {
                    for( const VECTOR2I& excluded : m_excludedPositions )
                    {
                        VECTOR2I d = excluded - cell;

                        if( (int64_t) d.x * d.x + (int64_t) d.y * d.y < guardExcludeRadiusSq )
                            return true;
                    }

                    return false;
                }

                return usesGridCells() ? m_excludedCells.count( cell ) > 0
                                       : m_excludedPositions.count( cell ) > 0;
            };

    // Get the new list of desired via cells
    std::map<VECTOR2I, VECTOR2I> desired;

    for( const VECTOR2I& cell : placementCells )
    {
        if( !isExcluded( cell ) )
            desired.emplace( cell, positionForCell( cell, pitch ) );
    }

    // Determine existing vias we can keep
    std::map<VECTOR2I, PCB_VIA*> kept;

    for( PCB_VIA* via : existingVias )
    {
        VECTOR2I cell = cellForPosition( via->GetPosition() );

        if( desired.count( cell ) && kept.emplace( cell, via ).second )
            continue;

        removeVia( via );
    }

    // Vias retired earlier may need to be unretired, this usually occurs when someone
    // is shrinking the outline and then changes their mind in the same drag
    std::map<VECTOR2I, PCB_VIA*> resurrectable;

    for( PCB_VIA* via : m_pendingRemovals )
        resurrectable.try_emplace( cellForPosition( via->GetPosition() ), via );

    // Now create a reference via to compare against, we need to see if existing vias need to be nuked
    // anyway on a property change. In which case we will just regen rather than play games
    std::unique_ptr<PCB_VIA> ref( static_cast<PCB_VIA*>( m_viaTemplate->Clone() ) );
    ref->SetParent( aBoard );
    ref->SetIsFree( true );

    if( m_netCode )
        ref->SetNetCode( m_netCode );

    auto matchesRef =
            [&]( const PCB_VIA* aVia ) -> bool
            {
                // The padstack carries size, drill, layer span, mask/paste attributes,
                // backdrill and post-machining.  That covers everything the via dialog
                // can edit except the net and teardrop settings, checked separately.
                return aVia->GetPosition() == ref->GetPosition()
                        && aVia->GetLayer() == ref->GetLayer()
                        && aVia->GetViaType() == ref->GetViaType()
                        && aVia->Padstack() == ref->Padstack()
                        && aVia->GetNetCode() == ref->GetNetCode()
                        && aVia->GetTeardropParams() == ref->GetTeardropParams();
            };

    for( const auto& [cell, pt] : desired )
    {
        auto it = kept.find( cell );

        if( it != kept.end() )
        {
            PCB_VIA* via = it->second;

            ref->SetPosition( pt );

            if( matchesRef( via ) )
                continue;   // unchanged — no commit entry, UUID preserved

            // Just nudge the via
            ref->SetPosition( via->GetPosition() );

            if( matchesRef( via ) )
            {
                aCommit->Modify( via );
                via->SetPosition( pt );
                changed = true;
                continue;
            }

            removeVia( via );
        }

        // Resurrect a via retired earlier
        auto rit = resurrectable.find( cell );

        if( rit != resurrectable.end() )
        {
            PCB_VIA* via = rit->second;

            ref->SetPosition( pt );

            if( matchesRef( via ) )
            {
                m_pendingRemovals.erase( via );
                AddItem( via );
                changed = true;
                continue;
            }
        }

        PCB_VIA* via = static_cast<PCB_VIA*>( m_viaTemplate->Clone() );
        via->ResetUuidDirect();

        via->SetParent( aBoard );
        via->SetPosition( pt );

        if( m_netCode )
            via->SetNetCode( m_netCode );

        // Vias must be marked free to avoid connectivity stealing them away
        via->SetIsFree( true );

        // If the generator is selected (point editing), make sure to mark new vias
        // as selected to match
        if( IsSelected() )
            via->SetSelected();

        AddItem( via );
        aCommit->Add( via );
        changed = true;
    }

    m_lastUpdateChangedVias = changed;
    ClearDirty();
    return true;
}


const STRING_ANY_MAP PCB_VIA_STITCH::GetProperties() const
{
    STRING_ANY_MAP props = PCB_GENERATOR::GetProperties();

    props.set_iu( "pitch", m_pitch );
    props.set( "layout", static_cast<int>( m_layout ) );
    props.set( "mode", static_cast<int>( m_mode ) );
    props.set( "seed", static_cast<int>( m_seed ) );
    props.set_iu( "origin_offset_x", m_originOffset.x );
    props.set_iu( "origin_offset_y", m_originOffset.y );

    // Save the net by name so it survives across sessions (net codes are session-local)
    if( !m_lastNetName.empty() )
        props.set( "net_name", m_lastNetName );

    if( !m_lastGuardedNetName.empty() )
        props.set( "guarded_net_name", m_lastGuardedNetName );

    if( !m_excludedCells.empty() )
    {
        std::vector<VECTOR2I> cells( m_excludedCells.begin(), m_excludedCells.end() );
        props.set( "excluded_grid_cells", wxAny( cells ) );
    }

    if( !m_excludedPositions.empty() )
    {
        SHAPE_LINE_CHAIN chain;
        for( const VECTOR2I& pos : m_excludedPositions )
            chain.Append( pos );
        props.set( "excluded_positions", wxAny( chain ) );
    }

    // Serialize the single zone outline as a SHAPE_LINE_CHAIN.
    // The file format natively supports SHAPE_LINE_CHAIN but not SHAPE_POLY_SET directly.
    if( m_outline.OutlineCount() > 0 )
        props.set( "outline", wxAny( m_outline.COutline( 0 ) ) );

    return props;
}


void PCB_VIA_STITCH::SetProperties( const STRING_ANY_MAP& aProps )
{
    PCB_GENERATOR::SetProperties( aProps );

    aProps.get_to_iu( "pitch", m_pitch );

    if( auto layout = aProps.get_opt<int>( "layout" ) )
        m_layout = static_cast<PCB_VIA_STITCH_LAYOUT>( *layout );

    if( auto mode = aProps.get_opt<int>( "mode" ) )
        m_mode = static_cast<PCB_VIA_STITCH_MODE>( *mode );

    if( auto seed = aProps.get_opt<int>( "seed" ) )
        m_seed = static_cast<uint32_t>( *seed );

    aProps.get_to_iu( "origin_offset_x", m_originOffset.x );
    aProps.get_to_iu( "origin_offset_y", m_originOffset.y );

    m_childGridConfig = GRID_CONFIG{ m_layout, m_mode, m_pitch };

    // Restore net name; net code is resolved from the board in EditStart()/Update()
    aProps.get_to( "net_name", m_lastNetName );
    aProps.get_to( "guarded_net_name", m_lastGuardedNetName );

    m_excludedCells.clear();
    m_excludedPositions.clear();


    if( auto cells = aProps.get_opt<std::vector<VECTOR2I>>( "excluded_grid_cells" ) )
        m_excludedCells.insert( cells->begin(), cells->end() );

    if( auto chain = aProps.get_opt<SHAPE_LINE_CHAIN>( "excluded_positions" ) )
    {
        for( int i = 0; i < chain->PointCount(); ++i )
            m_excludedPositions.insert( chain->CPoint( i ) );
    }

    // Restore the single zone outline
    m_outline.RemoveAllContours();

    if( auto outline = aProps.get_opt<SHAPE_LINE_CHAIN>( "outline" ) )
    {
        // A chain parsed from the file is a bare point list; the closed flag is not
        // serialized and AddOutline() asserts on open chains
        outline->SetClosed( true );
        m_outline.AddOutline( *outline );
    }
}


void PCB_VIA_STITCH::EditFinish( GENERATOR_TOOL* aTool, BOARD* aBoard, BOARD_COMMIT* aCommit )
{
    if( !( GetFlags() & IN_EDIT ) )
        return;

    // The edit session is over, any vias still pending removal can now be committed
    // for removal.
    if( aCommit )
    {
        for( PCB_VIA* via : m_pendingRemovals )
            aCommit->Remove( via );
    }

    m_pendingRemovals.clear();

    ClearFlags( IN_EDIT );
}


void PCB_VIA_STITCH::EditCancel( GENERATOR_TOOL* aTool, BOARD* aBoard, BOARD_COMMIT* aCommit )
{
    if( !( GetFlags() & IN_EDIT ) )
        return;

    // The commit is being reverted; re-attach retired vias so group membership matches
    // the pre-edit state they revert to.
    for( PCB_VIA* via : m_pendingRemovals )
        AddItem( via );

    m_pendingRemovals.clear();

    ClearFlags( IN_EDIT );
}


ACTION_MENU* PCB_VIA_STITCH::GetChildContextMenu( TOOL_INTERACTIVE* aTool ) const
{
    if( !m_childContextMenu )
    {
        m_childContextMenu = std::make_unique<ACTION_MENU>( true, aTool );
        m_childContextMenu->Add( PCB_ACTIONS::excludeStitchVia );
    }

    return m_childContextMenu.get();
}


void PCB_VIA_STITCH::Remove( GENERATOR_TOOL* aTool, BOARD* aBoard, BOARD_COMMIT* aCommit )
{
    if( !aCommit )
        return;

    for( BOARD_ITEM* item : GetBoardItems() )
        aCommit->Remove( item );

    // Retired-but-unpushed vias are detached from the group and would otherwise be
    // orphaned on the board.
    for( PCB_VIA* via : m_pendingRemovals )
        aCommit->Remove( via );

    m_pendingRemovals.clear();

    aCommit->Remove( this );
}


VECTOR2I PCB_VIA_STITCH::cellForPosition( const VECTOR2I& aPos ) const
{
    int pitch = m_pitch > 0 ? m_pitch : 1;

    auto roundDiv = []( int a, int b ) -> int
    {
        return ( a >= 0 ) ? ( a + b / 2 ) / b : -( ( -a + b / 2 ) / b );
    };

    // Poisson layout and GUARD mode both place at arbitrary positions, not a grid —
    // cell index == absolute position.
    if( !usesGridCells() )
        return aPos;

    int row = roundDiv( aPos.y - m_originOffset.y, pitch );
    int xShift = ( m_layout == PCB_VIA_STITCH_LAYOUT::STAGGERED && ( ( row % 2 + 2 ) % 2 != 0 ) )
                         ? pitch / 2
                         : 0;
    int col = roundDiv( aPos.x - m_originOffset.x - xShift, pitch );

    return VECTOR2I( col, row );
}


VECTOR2I PCB_VIA_STITCH::positionForCell( const VECTOR2I& aCell, int aPitch ) const
{
    if( !usesGridCells() )
        return aCell;

    int xShift = ( m_layout == PCB_VIA_STITCH_LAYOUT::STAGGERED && ( ( aCell.y % 2 + 2 ) % 2 != 0 ) )
                         ? aPitch / 2
                         : 0;
    return VECTOR2I( aCell.x * aPitch + m_originOffset.x + xShift,
                     aCell.y * aPitch + m_originOffset.y );
}


std::set<VECTOR2I> PCB_VIA_STITCH::buildPlacementCells( BOARD* aBoard ) const
{
    std::set<VECTOR2I> cells;

    if( !aBoard || m_outline.IsEmpty() )
        return cells;

    int pitch = m_pitch > 0 ? m_pitch : defaultPitch( aBoard );
    int viaSize = m_viaTemplate->GetWidth( PADSTACK::ALL_LAYERS ) > 0 ? m_viaTemplate->GetWidth( PADSTACK::ALL_LAYERS ) : defaultViaSize( aBoard );

    if( GetNetCode() == 0 )
        return cells;

    DRC_ENGINE* drcEngine = aBoard->GetDesignSettings().m_DRCEngine.get();

    DRC_CACHE_GENERATOR cacheGenerator;
    cacheGenerator.SetDRCEngine( drcEngine );
    cacheGenerator.Run();

    // Pre-clip every zone fill to the stitch outline bbox before adding it to perLayerFills.
    // Without this, a board-wide GND zone would drag in an entire complex polygon when we may
    // only be stitching part of it.
    // This also helps us skip same net zones entirely if they aren't in here
    BOX2I stitchBBox = m_outline.BBox();

    SHAPE_POLY_SET stitchClip;
    stitchClip.NewOutline();
    stitchClip.Append( stitchBBox.GetLeft(), stitchBBox.GetTop() );
    stitchClip.Append( stitchBBox.GetRight(), stitchBBox.GetTop() );
    stitchClip.Append( stitchBBox.GetRight(), stitchBBox.GetBottom() );
    stitchClip.Append( stitchBBox.GetLeft(), stitchBBox.GetBottom() );

    std::map<PCB_LAYER_ID, SHAPE_POLY_SET> perLayerFills;

    for( ZONE* zone : collectAllZones( aBoard ) )
    {
        if( zone->GetIsRuleArea() )
            continue;

        if( zone->GetNetCode() != GetNetCode() )
            continue;

        if( !zone->GetBoundingBox().Intersects( stitchBBox ) )
            continue;

        for( PCB_LAYER_ID layer : zone->GetLayerSet() )
        {
            SHAPE_POLY_SET clipped;

            if( m_mode == PCB_VIA_STITCH_MODE::GUARD )
            {
                if( const SHAPE_POLY_SET* outline = zone->Outline() )
                    clipped = *outline;
            }
            else
            {
                SHAPE_POLY_SET* fill = zone->GetFill( layer );

                if( !fill || fill->IsEmpty() )
                    continue;

                clipped = *fill;
            }

            if( clipped.IsEmpty() )
                continue;

            clipped.BooleanIntersection( stitchClip );

            if( clipped.OutlineCount() == 0 )
                continue;

            perLayerFills[layer].BooleanAdd( clipped );
        }
    }

    // Extract the via copper layers, the layer stack may contain the mask layers
    LSET boardCopperLayers = LSET::AllCuMask( aBoard->GetCopperLayerCount() );
    LSET viaCopperLayers   = m_viaTemplate->GetLayerSet() & boardCopperLayers;

    // Filter down to just those via layers that actually have same-net copper on them.
    std::vector<const SHAPE_POLY_SET*> viaLayerFills;

    for( PCB_LAYER_ID layer : viaCopperLayers )
    {
        auto it = perLayerFills.find( layer );

        if( it != perLayerFills.end() && !it->second.IsEmpty() )
            viaLayerFills.push_back( &it->second );
    }

    // Is there even enough layers left to stitch?
    if( viaLayerFills.size() < 2 )
        return cells;

    // Now build the region where a via would stitch at least two layers together. 
    SHAPE_POLY_SET allowedRegion;
    SHAPE_POLY_SET coveredSoFar = *viaLayerFills[0];

    for( size_t i = 1; i < viaLayerFills.size(); ++i )
    {
        SHAPE_POLY_SET overlap = *viaLayerFills[i];
        overlap.BooleanIntersection( coveredSoFar );

        if( !overlap.IsEmpty() )
            allowedRegion.BooleanAdd( overlap );

        if( i + 1 < viaLayerFills.size() )
            coveredSoFar.BooleanAdd( *viaLayerFills[i] );
    }

    if( allowedRegion.OutlineCount() == 0 )
        return cells;

    // Now intersect it with the via stitch outline
    allowedRegion.BooleanIntersection( m_outline );

    if( allowedRegion.OutlineCount() == 0 )
        return cells;

    // Generate a obstacle mask for via placement
    // Rather than DRC-probing every possible placement cell against the r-tree
    // to test for collisions. We instead create a keep-out polygon with obstacles
    // inflated by the via-radius and clearance we want. The result is now
    // we only have to do a point-in-polygon test for placement

    const int polyApproxError = pcbIUScale.mmToIU( 0.005 );

    // Require the whole via, not just its center, to land on same-net copper inside the
    // outline
    allowedRegion.Deflate( viaSize / 2, CORNER_STRATEGY::ROUND_ALL_CORNERS, polyApproxError );

    if( allowedRegion.OutlineCount() == 0 )
        return cells;

    // Probe used for clearance evaluation; it must carry the stitch net because clearance
    // rules can key off the net class.
    std::unique_ptr<PCB_VIA> probe( static_cast<PCB_VIA*>( m_viaTemplate->Clone() ) );
    probe->SetParent( const_cast<BOARD*>( aBoard ) );

    if( m_netCode )
        probe->SetNetCode( m_netCode );

    const int worstClearance = aBoard->m_DRCMaxClearance;

    BOX2I queryBBox = stitchBBox;
    queryBBox.Inflate( worstClearance + viaSize );

    SHAPE_POLY_SET queryClip;
    queryClip.NewOutline();
    queryClip.Append( queryBBox.GetLeft(), queryBBox.GetTop() );
    queryClip.Append( queryBBox.GetRight(), queryBBox.GetTop() );
    queryClip.Append( queryBBox.GetRight(), queryBBox.GetBottom() );
    queryClip.Append( queryBBox.GetLeft(), queryBBox.GetBottom() );

    SHAPE_POLY_SET obstacles;

    int viaDrill = m_viaTemplate->GetDrillValue() > 0 ? m_viaTemplate->GetDrillValue()
                                                      : defaultViaDrill( aBoard );

    auto evalConstraint =
            [&]( DRC_CONSTRAINT_T aType, BOARD_ITEM* aOther, PCB_LAYER_ID aLayer ) -> int
            {
                if( !drcEngine )
                    return 0;

                DRC_CONSTRAINT constraint =
                        drcEngine->EvalRules( aType, probe.get(), aOther, aLayer );
                return constraint.GetValue().Min();
            };

    auto isSameNet =
            [&]( BOARD_ITEM* aOther ) -> bool
            {
                BOARD_CONNECTED_ITEM* cItem = dynamic_cast<BOARD_CONNECTED_ITEM*>( aOther );
                return cItem && cItem->GetNetCode() == GetNetCode();
            };

    // Drilled holes are obstacles as we need to respect the hole to hole constraint
    std::set<BOARD_ITEM*> holeSeen;

    auto addHoleObstacle =
            [&]( BOARD_ITEM* aOther, PCB_LAYER_ID aLayer )
            {
                if( !aOther->HasHole() || !holeSeen.insert( aOther ).second )
                    return;

                std::shared_ptr<SHAPE_SEGMENT> hole = aOther->GetEffectiveHoleShape();

                if( !hole )
                    return;

                int margin = viaDrill / 2
                             + evalConstraint( HOLE_TO_HOLE_CONSTRAINT, aOther, UNDEFINED_LAYER );

                if( !isSameNet( aOther ) )
                {
                    margin = std::max( margin,
                                       viaSize / 2 + evalConstraint( HOLE_CLEARANCE_CONSTRAINT,
                                                                     aOther, aLayer ) );
                }

                TransformOvalToPolygon( obstacles, hole->GetSeg().A, hole->GetSeg().B,
                                        hole->GetWidth() + 2 * margin, polyApproxError,
                                        ERROR_OUTSIDE );
            };

    SHAPE_RECT queryRect( queryBBox.GetPosition(), queryBBox.GetWidth(), queryBBox.GetHeight() );

    for( PCB_LAYER_ID layer : viaCopperLayers )
    {
        std::set<BOARD_ITEM*> seen;

        auto enumerate =
                [&]( BOARD_ITEM* aOther ) -> bool
                {
                    if( !seen.insert( aOther ).second )
                        return false;

                    // Our own children are regenerated along with us and must not block
                    // their own cells.
                    if( aOther->GetParentGroup() == static_cast<const EDA_GROUP*>( this ) )
                        return false;

                    // Same for children retired during the same edit: they are
                    // detached from the group but stay on the board until the commit is
                    // pushed, and their cells must stay placeable so they can resurrect.
                    if( aOther->Type() == PCB_VIA_T
                            && m_pendingRemovals.count( static_cast<PCB_VIA*>( aOther ) ) )
                    {
                        return false;
                    }

                    addHoleObstacle( aOther, layer );

                    bool sameNet = isSameNet( aOther );

                    if( aOther->Type() == PCB_PAD_T )
                    {
                        // A copper-zone can be placed with thermal reliefs turned off
                        // We don't want to accidentally place vias on top of that pad
                        // So process the PAD as a obstacle we need to clear
                        PAD* pad = static_cast<PAD*>( aOther );

                        if( !pad->FlashLayer( layer ) )
                            return false;
                    }
                    else if( aOther->Type() != PCB_VIA_T && sameNet )
                    {
                        return false;
                    }

                    // Our copper against theirs.
                    int margin = viaSize / 2 + evalConstraint( CLEARANCE_CONSTRAINT, aOther, layer );

                    // Our drill against their copper (hole clearance), cross-net only.
                    if( !sameNet )
                    {
                        margin = std::max( margin,
                                           viaDrill / 2 + evalConstraint( HOLE_CLEARANCE_CONSTRAINT,
                                                                          aOther, layer ) );
                    }

                    aOther->TransformShapeToPolygon( obstacles, layer, margin, polyApproxError,
                                                     ERROR_OUTSIDE );
                    return false;
                };

        if( aBoard->m_CopperItemRTreeCache )
        {
            aBoard->m_CopperItemRTreeCache->CheckColliding( &queryRect, layer, worstClearance,
                                                            enumerate );
        }
    }

    // Add NPTH mounting holes which have no copper
    for( FOOTPRINT* footprint : aBoard->Footprints() )
    {
        if( !footprint->GetBoundingBox().Intersects( queryBBox ) )
            continue;

        for( PAD* pad : footprint->Pads() )
        {
            if( pad->GetBoundingBox().Intersects( queryBBox ) )
                addHoleObstacle( pad, F_Cu );
        }
    }

    obstacles.Simplify();

    for( ZONE* zone : collectAllZones( aBoard ) )
    {
        if( !zone->GetBoundingBox().Intersects( queryBBox ) )
            continue;

        if( zone->GetIsRuleArea() )
        {
            // Honor via keepouts (any net).
            if( !zone->GetDoNotAllowVias() || !( zone->GetLayerSet() & viaCopperLayers ).any() )
                continue;

            if( const SHAPE_POLY_SET* keepout = zone->Outline() )
            {
                SHAPE_POLY_SET area = *keepout;
                area.BooleanIntersection( queryClip );

                if( !area.IsEmpty() )
                {
                    area.Inflate( viaSize / 2, CORNER_STRATEGY::ROUND_ALL_CORNERS,
                                  polyApproxError );
                    obstacles.BooleanAdd( area );
                }
            }

            continue;
        }

        if( zone->GetNetCode() == GetNetCode() )
            continue;

        // A different-net zone only blocks near its fill outside its own outline.
        // A via dropped inside the zone is fine because the refill punches an anti-pad around it.
        for( PCB_LAYER_ID layer : LSET( zone->GetLayerSet() & viaCopperLayers ) )
        {
            SHAPE_POLY_SET* fill = zone->GetFill( layer );

            if( !fill || fill->IsEmpty() )
                continue;

            SHAPE_POLY_SET nearFill = *fill;
            nearFill.BooleanIntersection( queryClip );

            if( nearFill.IsEmpty() )
                continue;

            int zoneMargin = std::max(
                    viaSize / 2 + evalConstraint( CLEARANCE_CONSTRAINT, zone, layer ),
                    viaDrill / 2 + evalConstraint( HOLE_CLEARANCE_CONSTRAINT, zone, layer ) );

            nearFill.Inflate( zoneMargin, CORNER_STRATEGY::ROUND_ALL_CORNERS, polyApproxError );

            if( const SHAPE_POLY_SET* outline = zone->Outline() )
            {
                SHAPE_POLY_SET interior = *outline;
                interior.BooleanIntersection( queryClip );
                nearFill.BooleanSubtract( interior );
            }

            if( !nearFill.IsEmpty() )
                obstacles.BooleanAdd( nearFill );
        }
    }

    allowedRegion.BooleanSubtract( obstacles );

    if( allowedRegion.OutlineCount() == 0 )
        return cells;

    allowedRegion.BuildBBoxCaches();

    BOX2I bbox = allowedRegion.BBox();

    auto isValid =
            [&]( const VECTOR2I& pt ) -> bool
            {
                return allowedRegion.Contains( pt, -1, 0, true );
            };

    if( m_mode == PCB_VIA_STITCH_MODE::GUARD )
    {
        // Walk the perimeter of each guarded-net item's clearance envelope, dropping
        // candidate vias every `pitch` of arc length.  The envelope is the item's shape
        // inflated by (viaRadius + DRC clearance), so a via dropped on it sits at the
        // closest legal distance from the trace
        if( m_guardedNetCode == 0 )
            return cells;

        // Merge every guarded-net item's clearance envelope into one polygon, and their bare
        // shapes into another so the sampler can tell facing vias from same-side ones.
        SHAPE_POLY_SET mergedEnvelope;
        SHAPE_POLY_SET mergedGuarded;

        for( PCB_TRACK* track : aBoard->Tracks() )
        {
            if( track->GetNetCode() != m_guardedNetCode )
                continue;

            if( !track->GetBoundingBox().Intersects( stitchBBox ) )
                continue;

            PCB_LAYER_ID layer = track->GetLayer();

            if( track->Type() == PCB_VIA_T )
                layer = F_Cu;  // arbitrary copper layer for clearance evaluation

            int requiredClearance = evalConstraint( CLEARANCE_CONSTRAINT, track, layer );

            // Safety margin so envelope samples land outside the obstacle mask.  The
            // guarded items are themselves obstacles, polygonized outward with up to
            // polyApproxError of overshoot, so the envelope must sit at least that much
            // further out; the extra 2µm absorbs the sample coordinates' integer rounding.
            const int safetyMargin = polyApproxError + pcbIUScale.mmToIU( 0.002 );
            int       margin       = viaSize / 2 + requiredClearance + safetyMargin;

            SHAPE_POLY_SET envelope;
            track->TransformShapeToPolygon( envelope, layer, margin, polyApproxError,
                                            ERROR_OUTSIDE );

            mergedEnvelope.BooleanAdd( envelope );

            track->TransformShapeToPolygon( mergedGuarded, layer, 0, polyApproxError,
                                            ERROR_OUTSIDE );
        }

        mergedEnvelope.Simplify();
        mergedGuarded.Simplify();

        for( const VECTOR2I& pt : SampleGuardEnvelope( mergedEnvelope, mergedGuarded, pitch,
                                                       isValid ) )
        {
            cells.insert( pt );
        }

        return cells;
    }

    if( m_layout == PCB_VIA_STITCH_LAYOUT::POISSON )
    {
        // We tile a one time generated toroidal Poisson pattern
        // across the bbox at (pitch * POISSON_TILE_PITCHES) per tile.  The tiling is
        // anchored to the global (0, 0) origin plus a per-seed sub-tile shift.
        // This gives us a deterministic but non-grid distribution
        const std::vector<VECTOR2D>& tile     = bakedPoissonTile();
        const int                    tileSize = std::max( 1, pitch * POISSON_TILE_PITCHES );

        // Per-seed sub-tile origin shift in [0, tileSize).
        boost::random::mt19937                           seedRng( m_seed );
        boost::random::uniform_real_distribution<double> uniform( 0.0, 1.0 );
        const double offsetX = uniform( seedRng ) * tileSize;
        const double offsetY = uniform( seedRng ) * tileSize;

        // Range of tiles that intersect bbox.  Tile (tx, ty) covers
        // [tx*tileSize + offset, (tx+1)*tileSize + offset).
        const int firstTileX = (int) std::floor( ( bbox.GetX()      - offsetX ) / (double) tileSize );
        const int firstTileY = (int) std::floor( ( bbox.GetY()      - offsetY ) / (double) tileSize );
        const int lastTileX  = (int) std::floor( ( bbox.GetRight()  - offsetX ) / (double) tileSize );
        const int lastTileY  = (int) std::floor( ( bbox.GetBottom() - offsetY ) / (double) tileSize );

        for( int ty = firstTileY; ty <= lastTileY; ++ty )
        {
            for( int tx = firstTileX; tx <= lastTileX; ++tx )
            {
                for( const VECTOR2D& s : tile )
                {
                    VECTOR2I pt(
                            (int) std::round( offsetX + ( tx + s.x ) * tileSize ),
                            (int) std::round( offsetY + ( ty + s.y ) * tileSize ) );

                    if( !bbox.Contains( pt ) )
                        continue;

                    if( isValid( pt ) )
                        cells.insert( pt );  // Poisson "cell" == absolute position
                }
            }
        }

        return cells;
    }
    else
    {
        // Anchor the grid to global (0, 0) + m_originOffset.
        int      startRow = (int) std::floor( double( bbox.GetY() - m_originOffset.y ) / pitch );
        int      startCol = (int) std::floor( double( bbox.GetX() - m_originOffset.x ) / pitch );
        VECTOR2I origin( startCol * pitch + m_originOffset.x, startRow * pitch + m_originOffset.y );

        for( int row = startRow, y = origin.y; y <= bbox.GetBottom(); y += pitch, ++row )
        {
            // Odd rows are shifted right by half the pitch when stagger is enabled.
            int xOffset =
                    ( m_layout == PCB_VIA_STITCH_LAYOUT::STAGGERED && ( ( row % 2 + 2 ) % 2 != 0 ) ) ? pitch / 2 : 0;

            for( int col = startCol, x = origin.x + xOffset; x <= bbox.GetRight(); x += pitch, ++col )
            {
                VECTOR2I pt( x, y );

                if( isValid( pt ) )
                    cells.insert( VECTOR2I( col, row ) );
            }
        }

        return cells;
    }
}


std::vector<ZONE*> PCB_VIA_STITCH::GetZonesNeedingRefillAfterUpdate() const
{
    std::vector<ZONE*> result;

    // A regeneration that added, moved, or removed nothing can't have changed any
    // foreign-net anti-pads, so no refill is needed.
    if( !m_lastUpdateChangedVias )
        return result;

    const BOARD* brd = GetBoard();

    if( !brd )
        return result;

    BOX2I myBBox = GetBoundingBox();

    // Lets return all intersecting zones with different nets
    for( ZONE* zone : collectAllZones( brd ) )
    {
        if( zone->GetIsRuleArea() )
            continue;

        if( zone->GetNetCode() == m_netCode )
            continue;

        if( !zone->GetBoundingBox().Intersects( myBBox ) )
            continue;

        result.push_back( zone );
    }

    return result;
}


void PCB_VIA_STITCH::OnZoneFillChanged( const std::vector<ZONE*>& aZones )
{
    if( m_netCode == 0 )
        return;

    BOX2I myBBox = GetBoundingBox();

    for( ZONE* zone : aZones )
    {
        if( zone->GetNetCode() != m_netCode )
            continue;

        if( !zone->GetBoundingBox().Intersects( myBBox ) )
            continue;

        MarkDirty();
        return;
    }
}


void PCB_VIA_STITCH::ShowPropertiesDialog( PCB_BASE_EDIT_FRAME* aEditFrame )
{
    BOARD_COMMIT    commit( aEditFrame );
    GENERATOR_TOOL* genTool = aEditFrame->GetToolManager()->GetTool<GENERATOR_TOOL>();

    commit.Modify( this );

    DIALOG_VIA_STITCH_PROPERTIES dlg( aEditFrame, this );

    if( dlg.ShowModal() != wxID_OK )
        return;

    EditStart( genTool, GetBoard(), &commit );
    Update( genTool, GetBoard(), &commit );
    EditFinish( genTool, GetBoard(), &commit );

    commit.Push( _( "Edit Via Stitching" ) );
}


void PCB_VIA_STITCH::ExcludePosition( const VECTOR2I& aPos )
{
    if( usesGridCells() )
        m_excludedCells.insert( cellForPosition( aPos ) );
    else
        m_excludedPositions.insert( aPos );

    MarkDirty();
}


void PCB_VIA_STITCH::ClearExclusion( const VECTOR2I& aPos )
{
    if( usesGridCells() )
        m_excludedCells.erase( cellForPosition( aPos ) );
    else
        m_excludedPositions.erase( aPos );

    MarkDirty();
}


void PCB_VIA_STITCH::ClearAllExclusions()
{
    m_excludedCells.clear();
    m_excludedPositions.clear();
    MarkDirty();
}


int PCB_VIA_STITCH::GetNetCode() const
{
    if( m_netCode == 0 && !m_lastNetName.empty() )
    {
        if( const BOARD* board = GetBoard() )
        {
            if( NETINFO_ITEM* net = board->FindNet( m_lastNetName ) )
                m_netCode = net->GetNetCode();
        }
    }

    return m_netCode;
}


int PCB_VIA_STITCH::GetGuardedNetCode() const
{
    if( m_guardedNetCode == 0 && !m_lastGuardedNetName.empty() )
    {
        if( const BOARD* board = GetBoard() )
        {
            if( NETINFO_ITEM* net = board->FindNet( m_lastGuardedNetName ) )
                m_guardedNetCode = net->GetNetCode();
        }
    }

    return m_guardedNetCode;
}


void PCB_VIA_STITCH::SetNetCode( int aNetCode )
{
    m_netCode = aNetCode;
    if( BOARD* board = GetBoard() )
    {
        if( NETINFO_ITEM* net = board->FindNet( aNetCode ) )
            m_lastNetName = net->GetNetname();
        else
            m_lastNetName.clear();
    }

    for( BOARD_ITEM* item : GetBoardItems() )
        if( BOARD_CONNECTED_ITEM* bci = dynamic_cast<BOARD_CONNECTED_ITEM*>( item ) )
            bci->SetNetCode( aNetCode );

    MarkDirty();
}


void PCB_VIA_STITCH::SetGuardedNetCode( int aNetCode )
{
    m_guardedNetCode = aNetCode;

    if( BOARD* board = GetBoard() )
    {
        if( NETINFO_ITEM* net = board->FindNet( aNetCode ) )
            m_lastGuardedNetName = net->GetNetname();
        else
            m_lastGuardedNetName.clear();
    }

    MarkDirty();
}


std::vector<ZONE*> PCB_VIA_STITCH::collectAllZones( const BOARD* aBoard )
{
    std::vector<ZONE*> zones;

    if( !aBoard )
        return zones;

    zones.reserve( aBoard->Zones().size() );

    for( ZONE* zone : aBoard->Zones() )
        zones.push_back( zone );

    for( FOOTPRINT* footprint : aBoard->Footprints() )
    {
        for( ZONE* zone : footprint->Zones() )
            zones.push_back( zone );
    }

    return zones;
}


std::vector<VECTOR2I>
PCB_VIA_STITCH::SampleGuardEnvelope( const SHAPE_POLY_SET& aEnvelope,
                                     const SHAPE_POLY_SET& aGuarded, int aPitch,
                                     const std::function<bool( const VECTOR2I& )>& aIsValid )
{
    std::vector<VECTOR2I> accepted;

    if( aPitch <= 0 )
        return accepted;

    // Keeps adjacent same-side vias from doubling up where the walk rounds a corner or an
    // end cap. Arc length will report back a larger than expected value in this case.
    const int64_t minDistSq = (int64_t) ( aPitch * 0.7 ) * (int64_t) ( aPitch * 0.7 );

    auto farEnough =
            [&]( const VECTOR2I& pt ) -> bool
            {
                for( const VECTOR2I& other : accepted )
                {
                    VECTOR2I d = other - pt;

                    if( (int64_t) d.x * d.x + (int64_t) d.y * d.y >= minDistSq )
                        continue;

                    if( aGuarded.Collide( SEG( pt, other ) ) )
                        continue;

                    return false;
                }

                return true;
            };

    auto walkChain =
            [&]( const SHAPE_LINE_CHAIN& chain )
            {
                double cursor     = 0.0;
                double nextSample = aPitch / 2.0;

                for( int i = 0; i < chain.SegmentCount(); ++i )
                {
                    SEG    seg = chain.CSegment( i );
                    double segLen = ( VECTOR2D( seg.B ) - VECTOR2D( seg.A ) ).EuclideanNorm();

                    if( segLen < 1.0 )
                        continue;

                    while( nextSample <= cursor + segLen )
                    {
                        double t = ( nextSample - cursor ) / segLen;
                        VECTOR2I pt( (int) std::round( seg.A.x + t * ( seg.B.x - seg.A.x ) ),
                                     (int) std::round( seg.A.y + t * ( seg.B.y - seg.A.y ) ) );

                        nextSample += aPitch;

                        if( aIsValid( pt ) && farEnough( pt ) )
                            accepted.push_back( pt );
                    }

                    cursor += segLen;
                }
            };

    for( int o = 0; o < aEnvelope.OutlineCount(); ++o )
        walkChain( aEnvelope.COutline( o ) );

    return accepted;
}


static struct PCB_VIA_STITCH_DESC
{
    PCB_VIA_STITCH_DESC()
    {
        PROPERTY_MANAGER& propMgr = PROPERTY_MANAGER::Instance();
        REGISTER_TYPE( PCB_VIA_STITCH );
        propMgr.AddTypeCast( new TYPE_CAST<PCB_VIA_STITCH, PCB_GENERATOR> );
        propMgr.AddTypeCast( new TYPE_CAST<PCB_VIA_STITCH, BOARD_ITEM> );
        propMgr.InheritsAfter( TYPE_HASH( PCB_VIA_STITCH ), TYPE_HASH( PCB_GENERATOR ) );
        propMgr.InheritsAfter( TYPE_HASH( PCB_VIA_STITCH ), TYPE_HASH( BOARD_ITEM ) );


        propMgr.AddProperty( new PROPERTY<PCB_VIA_STITCH, int>( _HKI( "Size" ),
                                     &PCB_VIA_STITCH::SetViaSize, &PCB_VIA_STITCH::GetViaSize,
                                     PROPERTY_DISPLAY::PT_SIZE ) );

        propMgr.AddProperty( new PROPERTY<PCB_VIA_STITCH, int>( _HKI( "Drill" ),
                                     &PCB_VIA_STITCH::SetViaDrill, &PCB_VIA_STITCH::GetViaDrill,
                                     PROPERTY_DISPLAY::PT_SIZE ) );

        propMgr.AddProperty( new PROPERTY<PCB_VIA_STITCH, int>( _HKI( "Pitch" ),
                                     &PCB_VIA_STITCH::SetPitch, &PCB_VIA_STITCH::GetPitch,
                                     PROPERTY_DISPLAY::PT_SIZE ) );

        ENUM_MAP<PCB_VIA_STITCH_LAYOUT>::Instance()
                .Undefined( PCB_VIA_STITCH_LAYOUT::PLAIN )
                .Map( PCB_VIA_STITCH_LAYOUT::PLAIN,     _HKI( "Plain grid" ) )
                .Map( PCB_VIA_STITCH_LAYOUT::STAGGERED, _HKI( "Staggered grid" ) )
                .Map( PCB_VIA_STITCH_LAYOUT::POISSON,   _HKI( "Poisson disk" ) );

        auto inStitchMode =
                []( INSPECTABLE* aItem ) -> bool
                {
                    if( PCB_VIA_STITCH* stitch = dynamic_cast<PCB_VIA_STITCH*>( aItem ) )
                        return stitch->GetMode() == PCB_VIA_STITCH_MODE::STITCH;
                    return true;
                };

        auto inGuardMode =
                []( INSPECTABLE* aItem ) -> bool
                {
                    if( PCB_VIA_STITCH* stitch = dynamic_cast<PCB_VIA_STITCH*>( aItem ) )
                        return stitch->GetMode() == PCB_VIA_STITCH_MODE::GUARD;
                    return true;
                };

        auto layoutProp = new PROPERTY_ENUM<PCB_VIA_STITCH, PCB_VIA_STITCH_LAYOUT>( _HKI( "Layout" ),
                                     &PCB_VIA_STITCH::SetLayout, &PCB_VIA_STITCH::GetLayout );
        layoutProp->SetAvailableFunc( inStitchMode );
        propMgr.AddProperty( layoutProp );

        ENUM_MAP<PCB_VIA_STITCH_MODE>::Instance()
                .Undefined( PCB_VIA_STITCH_MODE::STITCH )
                .Map( PCB_VIA_STITCH_MODE::STITCH, _HKI( "Stitch" ) )
                .Map( PCB_VIA_STITCH_MODE::GUARD,  _HKI( "Guard" ) );

        propMgr.AddProperty( new PROPERTY_ENUM<PCB_VIA_STITCH, PCB_VIA_STITCH_MODE>( _HKI( "Mode" ),
                                     &PCB_VIA_STITCH::SetMode, &PCB_VIA_STITCH::GetMode ) );

        auto seedProp = new PROPERTY<PCB_VIA_STITCH, uint32_t>( _HKI( "Seed" ),
                                     &PCB_VIA_STITCH::SetSeed, &PCB_VIA_STITCH::GetSeed );
        seedProp->SetAvailableFunc(
                []( INSPECTABLE* aItem ) -> bool
                {
                    if( PCB_VIA_STITCH* stitch = dynamic_cast<PCB_VIA_STITCH*>( aItem ) )
                    {
                        return stitch->GetMode() == PCB_VIA_STITCH_MODE::STITCH
                                && stitch->GetLayout() == PCB_VIA_STITCH_LAYOUT::POISSON;
                    }
                    return true;
                } );
        propMgr.AddProperty( seedProp );

        propMgr.AddProperty( new PROPERTY_ENUM<PCB_VIA_STITCH, int>( _HKI( "Net" ),
                                     &PCB_VIA_STITCH::SetNetCode, &PCB_VIA_STITCH::GetNetCode, PT_NET ) );

        auto guardedNetProp = new PROPERTY_ENUM<PCB_VIA_STITCH, int>( _HKI( "Guarded Net" ),
                                     &PCB_VIA_STITCH::SetGuardedNetCode,
                                     &PCB_VIA_STITCH::GetGuardedNetCode, PT_NET );
        guardedNetProp->SetAvailableFunc( inGuardMode );
        propMgr.AddProperty( guardedNetProp );

        // The stitch lives on its own GAL layer and the vias are multi-layer configured separately
        propMgr.Mask( TYPE_HASH( PCB_VIA_STITCH ), TYPE_HASH( BOARD_ITEM ), _HKI( "Layer" ) );
    }
} _PCB_VIA_STITCH_DESC;


ENUM_TO_WXANY( PCB_VIA_STITCH_LAYOUT );
ENUM_TO_WXANY( PCB_VIA_STITCH_MODE );


static GENERATORS_MGR::REGISTER<PCB_VIA_STITCH> registerMe;