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

#include <board_design_settings.h>
#include <drc/drc_engine.h>
#include <drc/drc_item.h>
#include <drc/drc_rule.h>
#include <drc/drc_rule_condition.h>
#include <drc/drc_test_provider.h>
#include <collectors.h>
#include <pcb_shape.h>
#include <pad.h>
#include <pcb_track.h>

#include <drawing_sheet/ds_draw_item.h>
#include <drawing_sheet/ds_proxy_view_item.h>

#include <limits>

/*
    Miscellaneous tests:

    - DRCE_DISABLED_LAYER_ITEM,               ///< item on a disabled layer
    - DRCE_INVALID_OUTLINE,                   ///< invalid board outline
    - DRCE_UNRESOLVED_VARIABLE,
    - DRCE_ASSERTION_FAILURE,                 ///< user-defined assertions
    - DRCE_GENERIC_WARNING                    ///< user-defined warnings
    - DRCE_GENERIC_ERROR                      ///< user-defined errors
*/

static void findClosestOutlineGap( BOARD* aBoard, PCB_SHAPE*& aItemA, PCB_SHAPE*& aItemB,
                                   VECTOR2I& aMidpoint, int& aDistance )
{
    PCB_TYPE_COLLECTOR items;
    items.Collect( aBoard, { PCB_SHAPE_T } );

    std::vector<PCB_SHAPE*> shapes;

    for( int ii = 0; ii < items.GetCount(); ++ii )
    {
        PCB_SHAPE* shape = static_cast<PCB_SHAPE*>( items[ii] );

        if( shape->GetLayer() == Edge_Cuts )
            shapes.push_back( shape );
    }

    int       best = std::numeric_limits<int>::max();
    VECTOR2I  bestA;
    VECTOR2I  bestB;

    for( size_t ii = 0; ii < shapes.size(); ++ii )
    {
        std::shared_ptr<SHAPE> shapeA = shapes[ii]->GetEffectiveShape();

        for( size_t jj = ii + 1; jj < shapes.size(); ++jj )
        {
            std::shared_ptr<SHAPE> shapeB = shapes[jj]->GetEffectiveShape();
            VECTOR2I            ptA;
            VECTOR2I            ptB;

            if( shapeA && shapeB && shapeA->NearestPoints( shapeB.get(), ptA, ptB ) )
            {
                int dist = ( ptA - ptB ).EuclideanNorm();

                if( dist < best )
                {
                    best = dist;
                    bestA = ptA;
                    bestB = ptB;
                    aItemA = shapes[ii];
                    aItemB = shapes[jj];
                }
            }
        }
    }

    if( aItemA && aItemB )
    {
        aDistance = best;
        aMidpoint = ( bestA + bestB ) / 2;
    }
    else
    {
        aDistance = 0;
        aMidpoint = VECTOR2I();
    }
}

class DRC_TEST_PROVIDER_MISC : public DRC_TEST_PROVIDER
{
public:
    DRC_TEST_PROVIDER_MISC() :
        m_board( nullptr )
    {
        m_isRuleDriven = false;
    }

    virtual ~DRC_TEST_PROVIDER_MISC() = default;

    virtual bool Run() override;

    virtual const wxString GetName() const override { return wxT( "miscellaneous" ); };

private:
    void testOutline();
    void testDisabledLayers();
    void testTextVars();
    void testAssertions();

    BOARD* m_board;
};


void DRC_TEST_PROVIDER_MISC::testOutline()
{
    SHAPE_POLY_SET dummyOutline;
    bool           errorHandled = false;

    OUTLINE_ERROR_HANDLER errorHandler =
            [&]( const wxString& msg, BOARD_ITEM* itemA, BOARD_ITEM* itemB, const VECTOR2I& pt )
            {
                errorHandled = true;

                if( m_drcEngine->IsErrorLimitExceeded( DRCE_INVALID_OUTLINE ) )
                    return;

                if( !itemA )        // If we only have a single item, make sure it's A
                    std::swap( itemA, itemB );

                VECTOR2I markerPos = pt;
                int      gap = 0;

                if( itemA && itemB )
                {
                    std::shared_ptr<SHAPE> shapeA = itemA->GetEffectiveShape();
                    std::shared_ptr<SHAPE> shapeB = itemB->GetEffectiveShape();
                    VECTOR2I            ptA;
                    VECTOR2I            ptB;

                    if( shapeA && shapeB && shapeA->NearestPoints( shapeB.get(), ptA, ptB ) )
                    {
                        gap = ( ptA - ptB ).EuclideanNorm();
                        markerPos = ( ptA + ptB ) / 2;
                    }
                }
                else
                {
                    PCB_SHAPE* shapeA = nullptr;
                    PCB_SHAPE* shapeB = nullptr;

                    findClosestOutlineGap( m_board, shapeA, shapeB, markerPos, gap );

                    itemA = shapeA;
                    itemB = shapeB;
                }

                std::shared_ptr<DRC_ITEM> drcItem = DRC_ITEM::Create( DRCE_INVALID_OUTLINE );

                wxString msg2;

                if( itemA && itemB )
                    msg2.Printf( _( "%s (gap %s)" ), msg, MessageTextFromValue( gap ) );
                else
                    msg2 = msg;

                drcItem->SetErrorMessage( drcItem->GetErrorText() + wxS( " " ) + msg2 );
                drcItem->SetItems( itemA, itemB );

                reportViolation( drcItem, markerPos, Edge_Cuts );
            };

    // Test for very small graphic items (a few nm size) that can create issues
    // when trying to build the board outlines, and they are not easy to locate onn screen.
    const int minSizeForValideGraphics =  pcbIUScale.mmToIU( 0.001 );

    if( !TestBoardOutlinesGraphicItems(m_board, minSizeForValideGraphics, &errorHandler ) )
    {
        if( errorHandled )
        {
            // if there are invalid items on Edge.Cuts, they are already reported
        }
        else
        {
            std::shared_ptr<DRC_ITEM> drcItem = DRC_ITEM::Create( DRCE_INVALID_OUTLINE );
            wxString msg;

            msg.Printf( _( "(Suspicious items found on Edge.Cuts layer)" ) );

            drcItem->SetErrorMessage( drcItem->GetErrorText() + wxS( " " ) + msg );
            drcItem->SetItems( m_board );

            reportViolation( drcItem, m_board->GetBoundingBox().Centre(), Edge_Cuts );
        }
    }


    // Use the standard chaining epsilon here so that we report errors that might affect
    // other tools (such as 3D viewer).
    int chainingEpsilon = m_board->GetOutlinesChainingEpsilon();

    // Arc to segment approximation error (not critical here: we do not use the outline shape):
    int maxError = pcbIUScale.mmToIU( 0.05 );

    if( !BuildBoardPolygonOutlines( m_board, dummyOutline, maxError, chainingEpsilon, &errorHandler ) )
    {
        if( errorHandled )
        {
            // if there is an invalid outline, then there must be an outline
        }
        else
        {
            std::shared_ptr<DRC_ITEM> drcItem = DRC_ITEM::Create( DRCE_INVALID_OUTLINE );
            wxString msg;

            msg.Printf( _( "(no edges found on Edge.Cuts layer)" ) );

            drcItem->SetErrorMessage( drcItem->GetErrorText() + wxS( " " ) + msg );
            drcItem->SetItems( m_board );

            reportViolation( drcItem, m_board->GetBoundingBox().Centre(), Edge_Cuts );
        }
    }
}


void DRC_TEST_PROVIDER_MISC::testDisabledLayers()
{
    const int progressDelta = 2000;
    int       ii = 0;
    int       items = 0;

    auto countItems =
            [&]( BOARD_ITEM* item ) -> bool
            {
                ++items;
                return true;
            };

    LSET disabledLayers = LSET( m_board->GetEnabledLayers() ).flip();

    // Perform the test only for copper layers
    disabledLayers &= LSET::AllCuMask();

    auto checkDisabledLayers =
            [&]( BOARD_ITEM* item ) -> bool
            {
                if( m_drcEngine->IsErrorLimitExceeded( DRCE_DISABLED_LAYER_ITEM ) )
                    return false;

                if( !reportProgress( ii++, items, progressDelta ) )
                    return false;

                PCB_LAYER_ID badLayer = UNDEFINED_LAYER;

                if( item->Type() == PCB_PAD_T )
                {
                    PAD* pad = static_cast<PAD*>( item );

                    if( pad->GetAttribute() == PAD_ATTRIB::SMD
                            || pad->GetAttribute() == PAD_ATTRIB::CONN )
                    {
                        if( disabledLayers.test( pad->GetPrincipalLayer() ) )
                            badLayer = item->GetLayer();
                    }
                    else
                    {
                        // Through hole pad pierces all physical layers.
                    }
                }
                else if( item->Type() == PCB_VIA_T )
                {
                    PCB_VIA* via = static_cast<PCB_VIA*>( item );
                    PCB_LAYER_ID top;
                    PCB_LAYER_ID bottom;

                    via->LayerPair( &top, &bottom );

                    if( disabledLayers.test( top ) )
                        badLayer = top;
                    else if( disabledLayers.test( bottom ) )
                        badLayer = bottom;
                }
                else if( item->Type() == PCB_ZONE_T )
                {
                    // Footprint zones just get a top/bottom/inner setting, so they're on
                    // whatever inner layers there are.
                }
                else
                {
                    LSET badLayers = disabledLayers & item->GetLayerSet();

                    if( badLayers.any() )
                        badLayer = badLayers.Seq().front();
                }

                if( badLayer != UNDEFINED_LAYER )
                {
                    auto drcItem = DRC_ITEM::Create( DRCE_DISABLED_LAYER_ITEM );
                    wxString msg;

                    msg.Printf( _( "(layer %s)" ), LayerName( badLayer ) );

                    drcItem->SetErrorMessage( drcItem->GetErrorText() + wxS( " " ) + msg );
                    drcItem->SetItems( item );

                    reportViolation( drcItem, item->GetPosition(), UNDEFINED_LAYER );
                }

                return true;
            };

    forEachGeometryItem( s_allBasicItems, LSET::AllLayersMask(), countItems );
    forEachGeometryItem( s_allBasicItems, LSET::AllLayersMask(), checkDisabledLayers );
}


void DRC_TEST_PROVIDER_MISC::testAssertions()
{
    const int progressDelta = 2000;
    int       ii = 0;
    int       items = 0;

    auto countItems =
            [&]( BOARD_ITEM* item ) -> bool
            {
                ++items;
                return true;
            };

    auto checkAssertions =
            [&]( BOARD_ITEM* item ) -> bool
            {
                if( !reportProgress( ii++, items, progressDelta ) )
                    return false;

                if( !m_drcEngine->IsErrorLimitExceeded( DRCE_ASSERTION_FAILURE ) )
                {
                    m_drcEngine->ProcessAssertions( item,
                            [&]( const DRC_CONSTRAINT* c )
                            {
                                auto drcItem = DRC_ITEM::Create( DRCE_ASSERTION_FAILURE );
                                drcItem->SetErrorMessage( drcItem->GetErrorText() + wxS( " (" )
                                                            + c->GetName() + wxS( ")" ) );
                                drcItem->SetItems( item );
                                drcItem->SetViolatingRule( c->GetParentRule() );

                                reportViolation( drcItem, item->GetPosition(), item->GetLayer() );
                            } );
                }

                return true;
            };

    forEachGeometryItem( {}, LSET::AllLayersMask(), countItems );
    forEachGeometryItem( {}, LSET::AllLayersMask(), checkAssertions );
}


void DRC_TEST_PROVIDER_MISC::testTextVars()
{
    const int progressDelta = 2000;
    int       ii = 0;
    int       items = 0;

    static const std::vector<KICAD_T> itemTypes = {
        PCB_FIELD_T,
        PCB_TEXT_T, PCB_TEXTBOX_T, PCB_TABLECELL_T,
        PCB_DIMENSION_T
    };

    auto testAssertion =
            [&]( BOARD_ITEM* item, const wxString& text, const VECTOR2I& pos, int layer )
            {
                static wxRegEx warningExpr( wxS( "^\\$\\{DRC_WARNING\\s*([^}]*)\\}(.*)$" ) );
                static wxRegEx errorExpr( wxS( "^\\$\\{DRC_ERROR\\s*([^}]*)\\}(.*)$" ) );

                if( warningExpr.Matches( text ) )
                {
                    if( !m_drcEngine->IsErrorLimitExceeded( DRCE_GENERIC_WARNING ) )
                    {
                        std::shared_ptr<DRC_ITEM> drcItem = DRC_ITEM::Create( DRCE_GENERIC_WARNING );
                        wxString                  drcText = warningExpr.GetMatch( text, 1 );

                        if( item )
                            drcItem->SetItems( item );
                        else
                            drcText += _( " (in drawing sheet)" );

                        drcItem->SetErrorMessage( drcText );

                        reportViolation( drcItem, pos, layer );
                    }

                    return true;
                }

                if( errorExpr.Matches( text ) )
                {
                    if( !m_drcEngine->IsErrorLimitExceeded( DRCE_GENERIC_ERROR ) )
                    {
                        std::shared_ptr<DRC_ITEM> drcItem = DRC_ITEM::Create( DRCE_GENERIC_ERROR );
                        wxString                  drcText = errorExpr.GetMatch( text, 1 );

                        if( item )
                            drcItem->SetItems( item );
                        else
                            drcText += _( " (in drawing sheet)" );

                        drcItem->SetErrorMessage( drcText );

                        reportViolation( drcItem, pos, layer );
                    }

                    return true;
                }

                return false;
            };

    forEachGeometryItem( itemTypes, LSET::AllLayersMask(),
            [&]( BOARD_ITEM* item ) -> bool
            {
                ++items;
                return true;
            } );

    forEachGeometryItem( itemTypes, LSET::AllLayersMask(),
            [&]( BOARD_ITEM* item ) -> bool
            {
                if( m_drcEngine->IsErrorLimitExceeded( DRCE_UNRESOLVED_VARIABLE ) )
                    return false;

                if( !reportProgress( ii++, items, progressDelta ) )
                    return false;

                if( EDA_TEXT* textItem = dynamic_cast<EDA_TEXT*>( item ) )
                {
                    wxString result = ExpandEnvVarSubstitutions( textItem->GetShownText( true ),
                                                                 nullptr /*project already done*/ );

                    if( result.Matches( wxT( "*${*}*" ) ) )
                    {
                        auto drcItem = DRC_ITEM::Create( DRCE_UNRESOLVED_VARIABLE );
                        drcItem->SetItems( item );

                        reportViolation( drcItem, item->GetPosition(), item->GetLayer() );
                    }

                    testAssertion( item, textItem->GetText(), item->GetPosition(), item->GetLayer() );
                }

                return true;
            } );

    DS_PROXY_VIEW_ITEM* drawingSheet = m_drcEngine->GetDrawingSheet();
    DS_DRAW_ITEM_LIST   drawItems( pcbIUScale, FOR_ERC_DRC );

    if( !drawingSheet || m_drcEngine->IsErrorLimitExceeded( DRCE_UNRESOLVED_VARIABLE ) )
        return;

    drawItems.SetPageNumber( wxT( "1" ) );
    drawItems.SetSheetCount( 1 );
    drawItems.SetFileName( wxT( "dummyFilename" ) );
    drawItems.SetSheetName( wxT( "dummySheet" ) );
    drawItems.SetSheetLayer( wxT( "dummyLayer" ) );
    drawItems.SetProject( m_board->GetProject() );
    drawItems.BuildDrawItemsList( drawingSheet->GetPageInfo(), drawingSheet->GetTitleBlock() );

    for( DS_DRAW_ITEM_BASE* item = drawItems.GetFirst(); item; item = drawItems.GetNext() )
    {
        if( m_drcEngine->IsErrorLimitExceeded( DRCE_UNRESOLVED_VARIABLE ) )
            break;

        if( m_drcEngine->IsCancelled() )
            return;

        if( DS_DRAW_ITEM_TEXT* text = dynamic_cast<DS_DRAW_ITEM_TEXT*>( item ) )
        {
            if( testAssertion( nullptr, text->GetText(), text->GetPosition(), LAYER_DRAWINGSHEET ) )
            {
                // Don't run unresolved test
            }
            else if( text->GetShownText( true ).Matches( wxT( "*${*}*" ) ) )
            {
                std::shared_ptr<DRC_ITEM> drcItem = DRC_ITEM::Create( DRCE_UNRESOLVED_VARIABLE );
                drcItem->SetItems( drawingSheet );

                reportViolation( drcItem, text->GetPosition(), LAYER_DRAWINGSHEET );
            }
        }
    }
}


bool DRC_TEST_PROVIDER_MISC::Run()
{
    m_board = m_drcEngine->GetBoard();

    if( !m_drcEngine->IsErrorLimitExceeded( DRCE_INVALID_OUTLINE ) )
    {
        if( !reportPhase( _( "Checking board outline..." ) ) )
            return false;   // DRC cancelled

        testOutline();
    }

    if( !m_drcEngine->IsErrorLimitExceeded( DRCE_DISABLED_LAYER_ITEM ) )
    {
        if( !reportPhase( _( "Checking disabled layers..." ) ) )
            return false;   // DRC cancelled

        testDisabledLayers();
    }

    if( !m_drcEngine->IsErrorLimitExceeded( DRCE_UNRESOLVED_VARIABLE ) )
    {
        if( !reportPhase( _( "Checking text variables..." ) ) )
            return false;   // DRC cancelled

        testTextVars();
    }

    if( !m_drcEngine->IsErrorLimitExceeded( DRCE_ASSERTION_FAILURE )
            || !m_drcEngine->IsErrorLimitExceeded( DRCE_GENERIC_WARNING )
            || !m_drcEngine->IsErrorLimitExceeded( DRCE_GENERIC_ERROR ) )
    {
        if( !reportPhase( _( "Checking assertions..." ) ) )
            return false;   // DRC cancelled

        testAssertions();
    }

    return !m_drcEngine->IsCancelled();
}


namespace detail
{
static DRC_REGISTER_TEST_PROVIDER<DRC_TEST_PROVIDER_MISC> dummy;
}
