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

#include <drc/drc_engine.h>
#include <drc/drc_item.h>
#include <drc/drc_test_provider.h>
#include <footprint.h>
#include <footprint_utils.h>
#include <pad.h>
#include <pad_utils.h>

/*
    Footprint tests:

    - DRCE_FOOTPRINT_TYPE_MISMATCH,
    - DRCE_OVERLAPPING_PADS,
    - DRCE_PAD_TH_WITH_NO_HOLE,
    - DRCE_PADSTACK,
    - DRCE_PADSTACK_INVALID,
    - DRCE_FOOTPRINT (unknown or duplicate pads in net-tie pad groups),
    - DRCE_SHORTING_ITEMS,
    - DRCE_FOOTPRINT_SCALED_WITH_PADS
*/

class DRC_TEST_PROVIDER_FOOTPRINT_CHECKS : public DRC_TEST_PROVIDER
{
public:
    DRC_TEST_PROVIDER_FOOTPRINT_CHECKS()
    {
        m_isRuleDriven = false;
    }

    virtual ~DRC_TEST_PROVIDER_FOOTPRINT_CHECKS() = default;

    virtual bool Run() override;

    virtual const wxString GetName() const override { return wxT( "footprint checks" ); };
};


bool DRC_TEST_PROVIDER_FOOTPRINT_CHECKS::Run()
{
    if( !reportPhase( _( "Checking footprints..." ) ) )
        return false;   // DRC cancelled

    const auto errorHandler =
            [&]( const BOARD_ITEM* aItemA, const BOARD_ITEM* aItemB, const BOARD_ITEM* aItemC,
                 int aErrorCode, const wxString& aMsg, const VECTOR2I& aPt, PCB_LAYER_ID aLayer )
            {
                std::shared_ptr<DRC_ITEM> drcItem = DRC_ITEM::Create( aErrorCode );

                if( !aMsg.IsEmpty() )
                    drcItem->SetErrorDetail( aMsg );

                drcItem->SetItems( aItemA, aItemB, aItemC );
                reportViolation( drcItem, aPt, aLayer );
            };

    for( FOOTPRINT* footprint : m_drcEngine->GetBoard()->Footprints() )
    {
        if( !m_drcEngine->IsErrorLimitExceeded( DRCE_FOOTPRINT_TYPE_MISMATCH ) )
        {
            FOOTPRINT_UTILS::CheckFootprintAttributes( *footprint,
                    [&]( const wxString& aMsg )
                    {
                        errorHandler( footprint, nullptr, nullptr, DRCE_FOOTPRINT_TYPE_MISMATCH,
                                      aMsg, footprint->GetPosition(), footprint->GetLayer() );
                    } );
        }

        if( !m_drcEngine->IsErrorLimitExceeded( DRCE_FOOTPRINT_SCALED_WITH_PADS ) )
        {
            const TRANSFORM_TRS& xform = footprint->GetTransform();

            if( !xform.HasUnitScale() )
            {
                if( !footprint->Pads().empty() )
                {
                    errorHandler( footprint, nullptr, nullptr, DRCE_FOOTPRINT_SCALED_WITH_PADS, wxEmptyString,
                                  footprint->GetPosition(), footprint->GetLayer() );
                }
            }
        }

        if( !m_drcEngine->IsErrorLimitExceeded( DRCE_PAD_TH_WITH_NO_HOLE )
                || !m_drcEngine->IsErrorLimitExceeded( DRCE_PADSTACK ) )
        {
            FOOTPRINT_UTILS::CheckPads( *footprint, *m_drcEngine,
                    [&]( const PAD& aPad, const PAD_UTILS::PAD_CHECK_RESULT& aResult )
                    {
                        const int drcPadErrorCode = ConvertPadCheckToDrcCode( aResult.code );

                        if( !m_drcEngine->IsErrorLimitExceeded( drcPadErrorCode ) )
                        {
                            wxString detail;
                            if( !aResult.message.IsEmpty() )
                                detail = wxString::Format( wxT( "(%s)" ), aResult.message );

                            errorHandler( &aPad, nullptr, nullptr, drcPadErrorCode, detail,
                                          aPad.GetPosition(), aPad.GetPrincipalLayer() );
                        }
                    } );
        }

        // Don't call FOOTPRINT_UTILS::CheckShortingPads().  At the board level we know about nets,
        // and the pads may have the same net even though they're distinct pads.

        if( footprint->IsNetTie() )
        {
            if( !m_drcEngine->IsErrorLimitExceeded( DRCE_SHORTING_ITEMS ) )
            {
                FOOTPRINT_UTILS::CheckNetTies( *footprint,
                        [&]( const BOARD_ITEM* aItemA, const BOARD_ITEM* aItemB,
                             const BOARD_ITEM* aItemC, const VECTOR2I& aPosition )
                        {
                            errorHandler( aItemA, aItemB, aItemC, DRCE_SHORTING_ITEMS,
                                          wxEmptyString, aPosition, footprint->GetLayer() );
                        } );
            }

            FOOTPRINT_UTILS::CheckNetTiePadGroups( *footprint,
                    [&]( const wxString& aMsg )
                    {
                        errorHandler( footprint, nullptr, nullptr, DRCE_FOOTPRINT, aMsg,
                                      footprint->GetPosition(), footprint->GetLayer() );
                    } );
        }
    }

    return !m_drcEngine->IsCancelled();
}


namespace detail
{
static DRC_REGISTER_TEST_PROVIDER<DRC_TEST_PROVIDER_FOOTPRINT_CHECKS> dummy;
}
