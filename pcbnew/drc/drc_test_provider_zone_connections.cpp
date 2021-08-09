/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2021 KiCad Developers.
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

#include <board.h>
#include <board_design_settings.h>
#include <connectivity/connectivity_data.h>
#include <zone.h>
#include <footprint.h>
#include <pad.h>
#include <pcb_track.h>
#include <geometry/shape_line_chain.h>
#include <geometry/shape_poly_set.h>
#include <drc/drc_rule.h>
#include <drc/drc_test_provider.h>


/*
    This loads some rule resolvers for the ZONE_FILLER, and checks that pad thermal relief
    connections have at least the required number of spokes.

    Errors generated:
    - DRCE_STARVED_THERMAL
*/

class DRC_TEST_PROVIDER_ZONE_CONNECTIONS : public DRC_TEST_PROVIDER
{
public:
    DRC_TEST_PROVIDER_ZONE_CONNECTIONS()
    {
    }

    virtual ~DRC_TEST_PROVIDER_ZONE_CONNECTIONS()
    {
    }

    virtual bool Run() override;

    virtual const wxString GetName() const override
    {
        return "zone connections";
    };

    virtual const wxString GetDescription() const override
    {
        return "Checks thermal reliefs for a sufficient number of connecting spokes";
    }

    virtual std::set<DRC_CONSTRAINT_T> GetConstraintTypes() const override
    {
        return { ZONE_CONNECTION_CONSTRAINT, THERMAL_RELIEF_GAP_CONSTRAINT,
                 THERMAL_SPOKE_WIDTH_CONSTRAINT, MIN_RESOLVED_SPOKES_CONSTRAINT };
    }
};

bool DRC_TEST_PROVIDER_ZONE_CONNECTIONS::Run()
{
    const int delta = 5;  // This is the number of tests between 2 calls to the progress bar
    int       ii = 0;

    BOARD*                             board = m_drcEngine->GetBoard();
    BOARD_DESIGN_SETTINGS&             bds = board->GetDesignSettings();
    std::shared_ptr<CONNECTIVITY_DATA> connectivity = board->GetConnectivity();
    DRC_CONSTRAINT                     constraint;
    std::vector<ZONE*>                 zones;

    if( !reportPhase( _( "Checking thermal reliefs..." ) ) )
        return false;   // DRC cancelled

    for( ZONE* zone : board->Zones() )
        zones.push_back( zone );

    for( FOOTPRINT* footprint : board->Footprints() )
    {
        for( ZONE* zone : footprint->Zones() )
            zones.push_back( zone );
    }

    for( ZONE* zone : zones )
    {
        if( !reportProgress( ii++, zones.size(), delta ) )
            return false;

        for( PCB_LAYER_ID layer : zone->GetLayerSet().Seq() )
        {
            const SHAPE_POLY_SET& zoneFill = zone->GetFilledPolysList( layer );

            for( FOOTPRINT* footprint : board->Footprints() )
            {
                for( PAD* pad : footprint->Pads() )
                {
                    if( m_drcEngine->IsErrorLimitExceeded( DRCE_STARVED_THERMAL ) )
                        return true;

                    if( !pad->FlashLayer( layer ) )
                        continue;

                    if( pad->GetNetCode() != zone->GetNetCode() || pad->GetNetCode() <= 0 )
                        continue;

                    EDA_RECT item_boundingbox = pad->GetBoundingBox();

                    if( !item_boundingbox.Intersects( zone->GetCachedBoundingBox() ) )
                        continue;

                    constraint = bds.m_DRCEngine->EvalZoneConnection( pad, zone, layer );
                    ZONE_CONNECTION conn = constraint.m_ZoneConnection;

                    if( conn != ZONE_CONNECTION::THERMAL )
                        continue;

                    constraint = bds.m_DRCEngine->EvalRules( MIN_RESOLVED_SPOKES_CONSTRAINT,
                                                             pad, zone, layer );
                    int minCount = constraint.m_Value.Min();

                    if( minCount <= 0 )
                        continue;

                    SHAPE_POLY_SET padPoly;
                    pad->TransformShapeWithClearanceToPolygon( padPoly, layer, 0, ARC_LOW_DEF,
                                                               ERROR_OUTSIDE );

                    SHAPE_LINE_CHAIN& padOutline = padPoly.Outline( 0 );
                    std::vector<SHAPE_LINE_CHAIN::INTERSECTION> intersections;
                    int spokes = 0;

                    for( int jj = 0; jj < zoneFill.OutlineCount(); ++jj )
                        padOutline.Intersect( zoneFill.Outline( jj ), intersections, true );

                    spokes += intersections.size() / 2;

                    for( PCB_TRACK* track : connectivity->GetConnectedTracks( pad ) )
                    {
                        if( padOutline.PointInside( track->GetStart() ) )
                        {
                            if( zone->GetFilledPolysList( layer ).Collide( track->GetEnd() ) )
                                spokes++;
                        }
                        else if( padOutline.PointInside( track->GetEnd() ) )
                        {
                            if( zone->GetFilledPolysList( layer ).Collide( track->GetStart() ) )
                                spokes++;
                        }
                    }

                    if( spokes < minCount )
                    {
                        std::shared_ptr<DRC_ITEM> drce = DRC_ITEM::Create( DRCE_STARVED_THERMAL );

                        m_msg.Printf( _( "(%s min spoke count %d; actual %d)" ),
                                      constraint.GetName(),
                                      minCount,
                                      spokes );

                        drce->SetErrorMessage( drce->GetErrorText() + wxS( " " ) + m_msg );
                        drce->SetItems( zone, pad );
                        drce->SetViolatingRule( constraint.GetParentRule() );

                        reportViolation( drce, pad->GetPosition() );
                    }
                }
            }
        }
    }

    return true;
}


namespace detail
{
static DRC_REGISTER_TEST_PROVIDER<DRC_TEST_PROVIDER_ZONE_CONNECTIONS> dummy;
}
