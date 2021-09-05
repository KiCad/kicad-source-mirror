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
#include <zone.h>
#include <footprint.h>
#include <pcb_shape.h>
#include <geometry/shape_poly_set.h>
#include <drc/drc_rule.h>
#include <drc/drc_test_provider.h>
#include <advanced_config.h>

/*
    Checks for slivers in copper layers

    Errors generated:
    - DRCE_COPPER_SLIVER
*/

class DRC_TEST_PROVIDER_SLIVER_CHECKER : public DRC_TEST_PROVIDER
{
public:
    DRC_TEST_PROVIDER_SLIVER_CHECKER()
    {
    }

    virtual ~DRC_TEST_PROVIDER_SLIVER_CHECKER()
    {
    }

    virtual bool Run() override;

    virtual const wxString GetName() const override
    {
        return "sliver checker";
    };

    virtual const wxString GetDescription() const override
    {
        return "Checks copper layers for slivers";
    }

private:
    wxString layerDesc( PCB_LAYER_ID aLayer );
};


wxString DRC_TEST_PROVIDER_SLIVER_CHECKER::layerDesc( PCB_LAYER_ID aLayer )
{
    return wxString::Format( wxT( "(%s)" ), m_drcEngine->GetBoard()->GetLayerName( aLayer ) );
}


bool DRC_TEST_PROVIDER_SLIVER_CHECKER::Run()
{
    const int delta = 250;  // This is the number of tests between 2 calls to the progress bar

    if( m_drcEngine->IsErrorLimitExceeded( DRCE_COPPER_SLIVER ) )
        return true;    // Continue with other tests

    if( !reportPhase( _( "Running sliver detection on copper layers..." ) ) )
        return false;   // DRC cancelled

    int    widthTolerance = Millimeter2iu( ADVANCED_CFG::GetCfg().m_SliverWidthTolerance );
    double angleTolerance = ADVANCED_CFG::GetCfg().m_SliverAngleTolerance;
    int    testLength = widthTolerance / ( 2 * sin( DEG2RAD( angleTolerance / 2 ) ) );
    LSET   copperLayers = m_drcEngine->GetBoard()->GetEnabledLayers() & LSET::AllCuMask();

    for( PCB_LAYER_ID layer : copperLayers.Seq() )
    {
        if( m_drcEngine->IsErrorLimitExceeded( DRCE_COPPER_SLIVER ) )
            continue;

        int            itemCount = 0;
        int            itemIdx = 0;
        SHAPE_POLY_SET poly;


        forEachGeometryItem( s_allBasicItems, LSET().set( layer ),
                [&]( BOARD_ITEM* item ) -> bool
                {
                    ++itemCount;
                    return true;
                } );

        forEachGeometryItem( s_allBasicItems, LSET().set( layer ),
                [&]( BOARD_ITEM* item ) -> bool
                {
                    if( !reportProgress( itemIdx++, itemCount, delta ) )
                        return false;

                    if( item->Type() == PCB_ZONE_T || item->Type() == PCB_FP_ZONE_T )
                    {
                        ZONE* zone = static_cast<ZONE*>( item );

                        poly.BooleanAdd( zone->GetFilledPolysList( layer ),
                                         SHAPE_POLY_SET::PM_FAST );
                    }
                    else
                    {
                        item->TransformShapeWithClearanceToPolygon( poly, layer, 0, ARC_LOW_DEF,
                                                                    ERROR_OUTSIDE );
                    }

                    return true;
                } );

        poly.Simplify( SHAPE_POLY_SET::PM_FAST );

        // Sharpen corners
        poly.Deflate( widthTolerance / 2, ARC_LOW_DEF, SHAPE_POLY_SET::CHAMFER_ACUTE_CORNERS );

        for( int jj = 0; jj < poly.OutlineCount(); ++jj )
        {
            const std::vector<VECTOR2I>& pts = poly.Outline( jj ).CPoints();
            int                          ptCount = pts.size();

            for( int kk = 0; kk < ptCount; ++kk )
            {
                VECTOR2I pt = pts[ kk ];
                VECTOR2I ptBefore = pts[ ( ptCount + kk - 1 ) % ptCount ];
                VECTOR2I ptAfter  = pts[ ( kk + 1 ) % ptCount ];

                VECTOR2I vBefore = ( ptBefore - pt );
                VECTOR2I vAfter = ( ptAfter - pt );
                VECTOR2I vIncluded = vBefore.Resize( testLength ) - vAfter.Resize( testLength );

                if( vIncluded.SquaredEuclideanNorm() < SEG::Square( widthTolerance ) )
                {
                    std::shared_ptr<DRC_ITEM> drce = DRC_ITEM::Create( DRCE_COPPER_SLIVER );
                    drce->SetErrorMessage( drce->GetErrorText() + wxS( " " ) + layerDesc( layer ) );
                    reportViolation( drce, (wxPoint) pt );
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
