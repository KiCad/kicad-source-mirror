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
 */

#include <board.h>
#include <drc/drc_engine.h>
#include <drc/drc_item.h>
#include <drc/drc_test_provider.h>
#include <generators/pcb_via_stitch.h>
#include <geometry/shape_poly_set.h>

/*
    Same-net via-stitch zone overlap test.

    Generated errors:
    - DRCE_VIA_STITCH_OVERLAP
*/

class DRC_TEST_PROVIDER_VIA_STITCH : public DRC_TEST_PROVIDER
{
public:
    DRC_TEST_PROVIDER_VIA_STITCH() = default;

    virtual ~DRC_TEST_PROVIDER_VIA_STITCH() = default;

    virtual bool Run() override;

    virtual const wxString GetName() const override { return wxT( "via_stitch_overlap" ); }
};


bool DRC_TEST_PROVIDER_VIA_STITCH::Run()
{
    if( m_drcEngine->IsErrorLimitExceeded( DRCE_VIA_STITCH_OVERLAP ) )
    {
        REPORT_AUX( wxT( "Via-stitch overlap tests not run." ) );
        return true;        // continue with other tests
    }

    if( !reportPhase( _( "Checking via-stitch zone overlaps..." ) ) )
        return false;       // DRC cancelled

    // Collect the stitch generators present on the board.
    std::vector<PCB_VIA_STITCH*> stitches;

    for( PCB_GENERATOR* gen : m_drcEngine->GetBoard()->Generators() )
    {
        if( PCB_VIA_STITCH* stitch = dynamic_cast<PCB_VIA_STITCH*>( gen ) )
            stitches.push_back( stitch );
    }

    for( size_t i = 0; i < stitches.size(); ++i )
    {
        if( m_drcEngine->IsErrorLimitExceeded( DRCE_VIA_STITCH_OVERLAP ) )
            break;

        PCB_VIA_STITCH* a = stitches[i];

        if( a->GetNetCode() == 0 )
            continue;

        for( size_t j = i + 1; j < stitches.size(); ++j )
        {
            PCB_VIA_STITCH* b = stitches[j];

            if( b->GetNetCode() != a->GetNetCode() )
                continue;

            SHAPE_POLY_SET intersection = a->Outline();
            intersection.BooleanIntersection( b->Outline() );

            if( intersection.OutlineCount() == 0 )
                continue;

            std::shared_ptr<DRC_ITEM> drcItem = DRC_ITEM::Create( DRCE_VIA_STITCH_OVERLAP );
            drcItem->SetItems( a, b );

            // Report at the center of intersection
            VECTOR2I pos = intersection.BBox().Centre();
            reportViolation( drcItem, pos, a->GetLayer() );
        }
    }

    return !m_drcEngine->IsCancelled();
}


namespace detail
{
static DRC_REGISTER_TEST_PROVIDER<DRC_TEST_PROVIDER_VIA_STITCH> dummy;
}
