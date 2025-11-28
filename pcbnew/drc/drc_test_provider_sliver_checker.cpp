/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers.
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

#include <atomic>
#include <board.h>
#include <board_design_settings.h>
#include <zone.h>
#include <footprint.h>
#include <pcb_shape.h>
#include <geometry/shape_poly_set.h>
#include <drc/drc_rule.h>
#include <drc/drc_item.h>
#include <drc/drc_test_provider.h>
#include <advanced_config.h>
#include <progress_reporter.h>
#include <thread_pool.h>

/*
    Checks for slivers in copper layers

    Errors generated:
    - DRCE_COPPER_SLIVER
*/

class DRC_TEST_PROVIDER_SLIVER_CHECKER : public DRC_TEST_PROVIDER
{
public:
    DRC_TEST_PROVIDER_SLIVER_CHECKER()
    {}

    virtual ~DRC_TEST_PROVIDER_SLIVER_CHECKER() = default;

    virtual bool Run() override;

    virtual const wxString GetName() const override { return wxT( "sliver checker" ); };

private:
    wxString layerDesc( PCB_LAYER_ID aLayer );
};


wxString DRC_TEST_PROVIDER_SLIVER_CHECKER::layerDesc( PCB_LAYER_ID aLayer )
{
    return wxString::Format( wxT( "(%s)" ), m_drcEngine->GetBoard()->GetLayerName( aLayer ) );
}


bool DRC_TEST_PROVIDER_SLIVER_CHECKER::Run()
{
    if( m_drcEngine->IsErrorLimitExceeded( DRCE_COPPER_SLIVER ) )
        return true;    // Continue with other tests

    if( !reportPhase( _( "Running sliver detection on copper layers..." ) ) )
        return false;   // DRC cancelled

    int64_t widthTolerance = pcbIUScale.mmToIU( ADVANCED_CFG::GetCfg().m_SliverWidthTolerance );
    int64_t squared_width = widthTolerance * widthTolerance;

    double angleTolerance = ADVANCED_CFG::GetCfg().m_SliverAngleTolerance;
    double cosangleTol = 2.0 * cos( DEG2RAD( angleTolerance ) );
    LSEQ   copperLayers = LSET::AllCuMask( m_drcEngine->GetBoard()->GetCopperLayerCount() ).Seq();
    int    layerCount = copperLayers.size();

    // Report progress on board zones only.  Everything else is in the noise.
    int    zoneLayerCount = 0;
    std::atomic<size_t> done( 1 );

    for( PCB_LAYER_ID layer : copperLayers )
    {
        for( ZONE* zone : m_drcEngine->GetBoard()->Zones() )
        {
            if( !zone->GetIsRuleArea() && zone->IsOnLayer( layer ) )
                zoneLayerCount++;
        }
    }

    PROGRESS_REPORTER* reporter = m_drcEngine->GetProgressReporter();

    if( reporter && reporter->IsCancelled() )
        return false;   // DRC cancelled

    std::vector<SHAPE_POLY_SET> layerPolys( layerCount );

    auto build_layer_polys =
            [&]( int layerIdx ) -> size_t
            {
                PCB_LAYER_ID    layer = copperLayers[layerIdx];
                SHAPE_POLY_SET& poly = layerPolys[layerIdx];

                if( m_drcEngine->IsCancelled() )
                    return 0;

                SHAPE_POLY_SET  fill;

                forEachGeometryItem( s_allBasicItems, LSET().set( layer ),
                        [&]( BOARD_ITEM* item ) -> bool
                        {
                            if( ZONE* zone = dynamic_cast<ZONE*>( item) )
                            {
                                if( !zone->GetIsRuleArea() )
                                {
                                    fill = zone->GetFill( layer )->CloneDropTriangulation();
                                    poly.Append( fill );

                                    // Report progress on board zones only.  Everything else is
                                    // in the noise.
                                    done.fetch_add( 1 );
                                }
                            }
                            else
                            {
                                item->TransformShapeToPolygon( poly, layer, 0, ARC_LOW_DEF,
                                                               ERROR_INSIDE );
                            }

                            if( m_drcEngine->IsCancelled() )
                                return false;

                            return true;
                        } );


                if( m_drcEngine->IsCancelled() )
                    return 0;

                poly.Simplify();

                return 1;
            };

    thread_pool& tp = GetKiCadThreadPool();

    auto returns = tp.submit_loop( 0, copperLayers.size(), build_layer_polys );

    for( auto& ret : returns )
    {
        std::future_status status = ret.wait_for( std::chrono::milliseconds( 250 ) );

        while( status != std::future_status::ready )
        {
            reportProgress( zoneLayerCount, done );
            status = ret.wait_for( std::chrono::milliseconds( 250 ) );
        }
    }

    for( int ii = 0; ii < layerCount; ++ii )
    {
        PCB_LAYER_ID    layer = copperLayers[ii];
        SHAPE_POLY_SET& poly = layerPolys[ii];

        if( m_drcEngine->IsErrorLimitExceeded( DRCE_COPPER_SLIVER ) )
            continue;

        // Frequently, in filled areas, some points of the polygons are very near (dist is only
        // a few internal units, like 2 or 3 units.
        // We skip very small vertices: one cannot really compute a valid orientation of
        // such a vertex
        // So skip points near than min_len (in internal units).
        const int min_len = pcbIUScale.mmToIU( ADVANCED_CFG::GetCfg().m_SliverMinimumLength );

        for( int jj = 0; jj < poly.OutlineCount(); ++jj )
        {
            const std::vector<VECTOR2I>& pts = poly.Outline( jj ).CPoints();
            int                          ptCount = pts.size();
            int                          offset = 0;

            auto area = [&]( const VECTOR2I& p, const VECTOR2I& q, const VECTOR2I& r ) -> VECTOR2I::extended_type
                {
                    return static_cast<VECTOR2I::extended_type>( q.y - p.y ) * ( r.x - q.x ) -
                           static_cast<VECTOR2I::extended_type>( q.x - p.x ) * ( r.y - q.y );
                };

            auto isLocallyInside = [&]( int aA, int aB ) -> bool
                {
                    int prev = ( ptCount + aA - 1 ) % ptCount;
                    int next = ( aA + 1 ) % ptCount;

                    if( area( pts[prev], pts[aA], pts[next] ) < 0 )
                        return area( pts[aA], pts[aB], pts[next] ) >= 0 && area( pts[aA], pts[prev], pts[aB] ) >= 0;
                    else
                        return area( pts[aA], pts[aB], pts[prev] ) < 0 || area( pts[aA], pts[next], pts[aB] ) < 0;
                };

            if( ptCount <= 5 )
                continue;

            for( int kk = 0; kk < ptCount; kk += offset )
            {
                int      prior_index = ( ptCount + kk - 1 ) % ptCount;
                int      next_index  = ( kk + 1 ) % ptCount;
                VECTOR2I pt = pts[ kk ];
                VECTOR2I ptPrior = pts[ prior_index ];
                VECTOR2I vPrior = ( ptPrior - pt );
                int forward_offset = 1;

                offset = 1;

                while( std::abs( vPrior.x ) < min_len && std::abs( vPrior.y ) < min_len
                        && offset < ptCount )
                {
                    pt = pts[ ( kk + offset++ ) % ptCount ];
                    vPrior = ( ptPrior - pt );
                }

                if( offset >= ptCount )
                    break;

                VECTOR2I ptAfter  = pts[ next_index ];
                VECTOR2I vAfter = ( ptAfter - pt );

                while( std::abs( vAfter.x ) < min_len && std::abs( vAfter.y ) < min_len
                        && forward_offset < ptCount )
                {
                    next_index = ( kk + forward_offset++ ) % ptCount;
                    ptAfter  = pts[ next_index ];
                    vAfter = ( ptAfter - pt );
                }

                if( offset >= ptCount )
                    break;

                // Negative dot product means that the angle is > 90°
                if( vPrior.Dot( vAfter ) <= 0 )
                    continue;

                if( !isLocallyInside( prior_index, next_index ) )
                    continue;

                VECTOR2I vIncluded = ptAfter - ptPrior;
                double arm1 = vPrior.SquaredEuclideanNorm();
                double arm2 = vAfter.SquaredEuclideanNorm();
                double opp  = vIncluded.SquaredEuclideanNorm();

                double cos_ang = std::abs( ( opp - arm1 - arm2 ) / ( std::sqrt( arm1 ) * std::sqrt( arm2 ) ) );

                if( cos_ang > cosangleTol && 2.0 - cos_ang > std::numeric_limits<float>::epsilon() && opp > squared_width )
                {
                    std::shared_ptr<DRC_ITEM> drce = DRC_ITEM::Create( DRCE_COPPER_SLIVER );
                    drce->SetErrorDetail( layerDesc( layer ) );
                    reportViolation( drce, pt, layer );
                }
            }
        }
    }

    return true;
}


namespace detail
{
static DRC_REGISTER_TEST_PROVIDER<DRC_TEST_PROVIDER_SLIVER_CHECKER> dummy;
}
