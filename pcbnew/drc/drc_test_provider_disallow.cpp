/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2004-2022 KiCad Developers.
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

#include <common.h>
#include <drc/drc_engine.h>
#include <drc/drc_item.h>
#include <drc/drc_rule.h>
#include <drc/drc_test_provider.h>
#include <pad.h>
#include <zone.h>

/*
    "Disallow" test. Goes through all items, matching types/conditions drop errors.
    Errors generated:
    - DRCE_ALLOWED_ITEMS
    - DRCE_TEXT_ON_EDGECUTS
*/

class DRC_TEST_PROVIDER_DISALLOW : public DRC_TEST_PROVIDER
{
public:
    DRC_TEST_PROVIDER_DISALLOW()
    {
    }

    virtual ~DRC_TEST_PROVIDER_DISALLOW()
    {
    }

    virtual bool Run() override;

    virtual const wxString GetName() const override
    {
        return "disallow";
    };

    virtual const wxString GetDescription() const override
    {
        return "Tests for disallowed items (e.g. keepouts)";
    }
};


bool DRC_TEST_PROVIDER_DISALLOW::Run()
{
    if( !reportPhase( _( "Checking keepouts & disallow constraints..." ) ) )
        return false;   // DRC cancelled

    int    count = 0;
    int    delta = 10;
    int    ii = 0;

    auto checkTextOnEdgeCuts =
            [&]( BOARD_ITEM* item )
            {
                if( item->Type() == PCB_TEXT_T || item->Type() == PCB_TEXTBOX_T
                        || BaseType( item->Type() ) == PCB_DIMENSION_T )
                {
                    if( item->GetLayer() == Edge_Cuts )
                    {
                        std::shared_ptr<DRC_ITEM> drc = DRC_ITEM::Create( DRCE_TEXT_ON_EDGECUTS );
                        drc->SetItems( item );
                        reportViolation( drc, item->GetPosition(), Edge_Cuts );
                    }
                }
            };

    auto checkDisallow =
            [&]( BOARD_ITEM* item )
            {
                auto constraint = m_drcEngine->EvalRules( DISALLOW_CONSTRAINT, item, nullptr,
                                                          UNDEFINED_LAYER );

                if( constraint.m_DisallowFlags && constraint.GetSeverity() != RPT_SEVERITY_IGNORE )
                {
                    std::shared_ptr<DRC_ITEM> drcItem = DRC_ITEM::Create( DRCE_ALLOWED_ITEMS );

                    m_msg.Printf( drcItem->GetErrorText() + wxS( " (%s)" ),
                                  constraint.GetName() );

                    drcItem->SetErrorMessage( m_msg );
                    drcItem->SetItems( item );
                    drcItem->SetViolatingRule( constraint.GetParentRule() );

                    reportViolation( drcItem, item->GetPosition(), item->GetLayerSet().Seq()[0] );
                }
            };

    forEachGeometryItem( {}, LSET::AllLayersMask(),
            [&]( BOARD_ITEM* item ) -> bool
            {
                ZONE* zone = dynamic_cast<ZONE*>( item );

                if( zone && zone->GetIsRuleArea() )
                    return true;

                // Report progress on zone copper layers only.  Everything else is in the noise.
                if( zone )
                {
                    for( PCB_LAYER_ID layer : zone->GetLayerSet().Seq() )
                    {
                        if( IsCopperLayer( layer ) )
                            count++;
                    }
                }

                return true;
            } );

    forEachGeometryItem( {}, LSET::AllLayersMask(),
            [&]( BOARD_ITEM* item ) -> bool
            {
                if( !m_drcEngine->IsErrorLimitExceeded( DRCE_TEXT_ON_EDGECUTS ) )
                    checkTextOnEdgeCuts( item );

                if( !m_drcEngine->IsErrorLimitExceeded( DRCE_ALLOWED_ITEMS ) )
                {
                    ZONE* zone = dynamic_cast<ZONE*>( item );
                    PAD*  pad = dynamic_cast<PAD*>( item );

                    if( zone && zone->GetIsRuleArea() )
                        return true;

                    // Report progress on zone copper layers only.  Everything else is in the
                    // noise.
                    if( zone )
                    {
                        for( PCB_LAYER_ID layer : zone->GetLayerSet().Seq() )
                        {
                            if( IsCopperLayer( layer ) )
                            {
                                if( !reportProgress( ii++, count, delta ) )
                                    return false;   // DRC cancelled
                            }
                        }
                    }

                    item->ClearFlags( HOLE_PROXY );     // Just in case

                    checkDisallow( item );

                    bool hasHole;

                    switch( item->Type() )
                    {
                    case PCB_VIA_T: hasHole = true;                     break;
                    case PCB_PAD_T: hasHole = pad->GetDrillSizeX() > 0; break;
                    default:        hasHole = false;                    break;
                    }

                    if( hasHole )
                    {
                        item->SetFlags( HOLE_PROXY );
                        {
                            checkDisallow( item );
                        }
                        item->ClearFlags( HOLE_PROXY );
                    }
                }

                return true;
            } );

    reportRuleStatistics();

    return true;
}


namespace detail
{
static DRC_REGISTER_TEST_PROVIDER<DRC_TEST_PROVIDER_DISALLOW> dummy;
}
