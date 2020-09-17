/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2004-2020 KiCad Developers.
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

#include <class_pcb_text.h>
#include <drc/drc_engine.h>
#include <drc/drc_item.h>
#include <drc/drc_rule.h>
#include <drc/drc_test_provider.h>

#include <ws_draw_item.h>
#include <ws_proxy_view_item.h>

/*
    Miscellaneous tests:

    - DRCE_DISABLED_LAYER_ITEM,               ///< item on a disabled layer
    - DRCE_INVALID_OUTLINE,                   ///< invalid board outline
    - DRCE_UNRESOLVED_VARIABLE,

    TODO:
    - if grows too big, split into separate providers
*/

class DRC_TEST_PROVIDER_MISC : public DRC_TEST_PROVIDER
{
public:
    DRC_TEST_PROVIDER_MISC()
    {
        m_isRuleDriven = false;
    }

    virtual ~DRC_TEST_PROVIDER_MISC()
    {
    }

    virtual bool Run() override;

    virtual const wxString GetName() const override
    {
        return "miscellanous";
    };

    virtual const wxString GetDescription() const override
    {
        return "Misc checks (board outline, missing textvars)";
    }

    virtual std::set<DRC_CONSTRAINT_TYPE_T> GetConstraintTypes() const override;

    int GetNumPhases() const override;

private:
    void testOutline();
    void testDisabledLayers();
    void testTextVars();

    BOARD* m_board;
};


void DRC_TEST_PROVIDER_MISC::testOutline()
{
    wxPoint error_loc( m_board->GetBoardEdgesBoundingBox().GetPosition() );

    SHAPE_POLY_SET boardOutlines;

    if( m_board->GetBoardPolygonOutlines( boardOutlines, nullptr, &error_loc ) )
        return;

    std::shared_ptr<DRC_ITEM> drcItem = DRC_ITEM::Create( DRCE_INVALID_OUTLINE );

    m_msg.Printf( drcItem->GetErrorText() + _( " (not a closed shape)" ) );

    drcItem->SetErrorMessage( m_msg );
    drcItem->SetItems( m_board );

    reportViolation( drcItem, error_loc );
}


void DRC_TEST_PROVIDER_MISC::testDisabledLayers()
{
    LSET disabledLayers = m_board->GetEnabledLayers().flip();

    // Perform the test only for copper layers
    disabledLayers &= LSET::AllCuMask();

    auto checkDisabledLayers =
            [&]( BOARD_ITEM* item ) -> bool
            {
                LSET refLayers ( item->GetLayer() );

                if( ( disabledLayers & refLayers ).any() )
                {
                    std::shared_ptr<DRC_ITEM>drcItem = DRC_ITEM::Create( DRCE_DISABLED_LAYER_ITEM );

                    m_msg.Printf( drcItem->GetErrorText() + _( " (layer %s)" ),
                                  item->GetLayerName() );

                    drcItem->SetErrorMessage( m_msg );
                    drcItem->SetItems( item );

                    reportViolation( drcItem, item->GetPosition());
                }
                return true;
            };

    // fixme: what about graphical items?
    forEachGeometryItem( { PCB_TRACE_T, PCB_ARC_T, PCB_VIA_T, PCB_ZONE_AREA_T, PCB_PAD_T },
                           LSET::AllLayersMask(), checkDisabledLayers );
}

void DRC_TEST_PROVIDER_MISC::testTextVars()
{
    auto checkUnresolvedTextVar =
            [&]( EDA_ITEM* item ) -> bool
            {
                if( m_drcEngine->IsErrorLimitExceeded( DRCE_UNRESOLVED_VARIABLE ) )
                    return false;

                EDA_TEXT* text = dynamic_cast<EDA_TEXT*>( item );

                if( text && text->GetShownText().Matches( wxT( "*${*}*" ) ) )
                {
                    std::shared_ptr<DRC_ITEM>drcItem = DRC_ITEM::Create( DRCE_UNRESOLVED_VARIABLE );
                    drcItem->SetItems( item );

                    reportViolation( drcItem, item->GetPosition());
                }
                return true;
            };

    forEachGeometryItem( { PCB_MODULE_TEXT_T, PCB_TEXT_T }, LSET::AllLayersMask(),
                         checkUnresolvedTextVar );

    KIGFX::WS_PROXY_VIEW_ITEM* worksheet = m_drcEngine->GetWorksheet();
    WS_DRAW_ITEM_LIST          wsItems;

    if( !worksheet || m_drcEngine->IsErrorLimitExceeded( DRCE_UNRESOLVED_VARIABLE ) )
        return;

    wsItems.SetMilsToIUfactor( IU_PER_MILS );
    wsItems.SetSheetNumber( 1 );
    wsItems.SetSheetCount( 1 );
    wsItems.SetFileName( "dummyFilename" );
    wsItems.SetSheetName( "dummySheet" );
    wsItems.SetSheetLayer( "dummyLayer" );
    wsItems.SetProject( m_board->GetProject() );
    wsItems.BuildWorkSheetGraphicList( worksheet->GetPageInfo(), worksheet->GetTitleBlock() );

    for( WS_DRAW_ITEM_BASE* item = wsItems.GetFirst(); item; item = wsItems.GetNext() )
    {
        if( m_drcEngine->IsErrorLimitExceeded( DRCE_UNRESOLVED_VARIABLE ) )
            break;

        WS_DRAW_ITEM_TEXT* text = dynamic_cast<WS_DRAW_ITEM_TEXT*>( item );

        if( text && text->GetShownText().Matches( wxT( "*${*}*" ) ) )
        {
            std::shared_ptr<DRC_ITEM> drcItem = DRC_ITEM::Create( DRCE_UNRESOLVED_VARIABLE );
            drcItem->SetItems( text );

            reportViolation( drcItem, text->GetPosition());
        }
    }
}


bool DRC_TEST_PROVIDER_MISC::Run()
{
    m_board = m_drcEngine->GetBoard();

    reportPhase( _( "Checking board outline..." ));
    testOutline();

    reportPhase( _( "Checking disabled layers..." ));
    testDisabledLayers();

    reportPhase( _( "Checking text variables..." ));
    testTextVars();

    return true;
}


int DRC_TEST_PROVIDER_MISC::GetNumPhases() const
{
    return 3;
}


std::set<DRC_CONSTRAINT_TYPE_T> DRC_TEST_PROVIDER_MISC::GetConstraintTypes() const
{
    return {};
}


namespace detail
{
static DRC_REGISTER_TEST_PROVIDER<DRC_TEST_PROVIDER_MISC> dummy;
}
