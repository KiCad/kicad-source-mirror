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

#include <cstdlib>
#include <map>
#include <set>
#include <vector>

#include <board.h>
#include <board_design_settings.h>
#include <layer_range.h>
#include <padstack.h>
#include <pcb_generator.h>
#include <pcb_track.h>
#include <generators/pcb_via_stack.h>
#include <drc/drc_engine.h>
#include <drc/drc_item.h>
#include <drc/drc_test_provider.h>
#include <board_stackup_manager/board_stackup.h>
#include <board_stackup_manager/stackup_predefined_prms.h>

/*
    Microvia stack topology tests.

    Errors generated:
    - DRCE_MALFORMED_MICROVIA_STACK_SPAN
    - DRCE_MICROVIA_STACK_NOT_FILLED
    - DRCE_MICROVIA_STACK_DEPTH
    - DRCE_MICROVIA_ASPECT_RATIO
    - DRCE_MICROVIA_CROSSES_CORE

    The span test asks whether a generator's hops still tile the span it declares. Nothing
    rebuilds a stack on load, so a file can carry one that does not. The other three ask whether
    the copper can be built, and apply to every microvia however it was placed.

    Where these rules come from:

    IPC-2226 defines a microvia as a hole of aspect ratio 1:1 or better, no deeper than 0.25 mm.
    That is why a hop spans one dielectric and why deeper structures are built by stacking.

    IPC-4761 names the via protection types. A filled via is Type V and a filled and capped via is
    Type VII, which is the via in pad treatment used for BGA fanout.

    IPC-6012 carries the reliability requirements for microvias, including the thermal cycling a
    stacked structure has to survive. Unfilled stacked microvias separate at the target pad.

    Maximum stack depth is deliberately not standardised. It is a fabricator capability, so it is a
    board setting rather than a fixed number here.
*/

class DRC_TEST_PROVIDER_VIA_STACK : public DRC_TEST_PROVIDER
{
public:
    DRC_TEST_PROVIDER_VIA_STACK() {}

    virtual ~DRC_TEST_PROVIDER_VIA_STACK() = default;

    virtual bool Run() override;

    virtual const wxString GetName() const override { return wxT( "microvia_stack" ); };

private:
    void checkSpan( PCB_VIA_STACK* aStack, int aCopperLayers );
    void checkColumn( const std::vector<PCB_VIA*>& aColumn );
    void checkLooseMicrovias( BOARD* aBoard );
    void checkAspectRatio( BOARD* aBoard );
};


void DRC_TEST_PROVIDER_VIA_STACK::checkSpan( PCB_VIA_STACK* aStack, int aCopperLayers )
{
    std::vector<PCB_VIA*> vias;

    for( BOARD_ITEM* item : aStack->GetBoardItems() )
    {
        if( item->Type() == PCB_VIA_T )
        {
            PCB_VIA* via = static_cast<PCB_VIA*>( item );

            if( via->GetViaType() == VIATYPE::MICROVIA )
                vias.push_back( via );
        }
    }

    if( vias.empty() )
        return;

    // IPC-2226 confines a microvia to one dielectric, so the hops must tile the declared span
    // with single layer steps and no gap. A span naming layers absent from the board is broken
    // by definition, and iterating it would never terminate.
    if( !m_drcEngine->IsErrorLimitExceeded( DRCE_MALFORMED_MICROVIA_STACK_SPAN ) )
    {
        if( !PCB_VIA_STACK::IsSpanValid( m_drcEngine->GetBoard(), aStack->GetStartLayer(), aStack->GetEndLayer() ) )
        {
            std::shared_ptr<DRC_ITEM> drcItem = DRC_ITEM::Create( DRCE_MALFORMED_MICROVIA_STACK_SPAN );
            drcItem->SetItems( aStack );
            reportViolation( drcItem, aStack->GetPosition(), aStack->GetLayer() );
            return;
        }

        std::map<int, int> layerToIndex;
        int                spanLen = 0;

        for( PCB_LAYER_ID layer : LAYER_RANGE( aStack->GetStartLayer(), aStack->GetEndLayer(), aCopperLayers ) )
            layerToIndex[layer] = spanLen++;

        std::set<int> covered;
        bool          broken = false;

        for( PCB_VIA* via : vias )
        {
            auto top = layerToIndex.find( via->TopLayer() );
            auto bot = layerToIndex.find( via->BottomLayer() );

            if( top == layerToIndex.end() || bot == layerToIndex.end() || std::abs( top->second - bot->second ) != 1 )
            {
                broken = true;
                break;
            }

            covered.insert( std::min( top->second, bot->second ) );
        }

        if( broken || (int) covered.size() != spanLen - 1 )
        {
            std::shared_ptr<DRC_ITEM> drcItem = DRC_ITEM::Create( DRCE_MALFORMED_MICROVIA_STACK_SPAN );
            drcItem->SetItems( aStack );
            reportViolation( drcItem, aStack->GetPosition(), aStack->GetLayer() );
        }
    }
}


// Depth and fill are properties of the copper, not of the object that placed it.
void DRC_TEST_PROVIDER_VIA_STACK::checkColumn( const std::vector<PCB_VIA*>& aColumn )
{
    // Not standardised. A fabricator quotes its own limit, and a designer may accept a deeper
    // stack in one area than another, so the limit is resolved per via rather than read once.
    PCB_VIA*       top = aColumn.front();
    DRC_CONSTRAINT depthConstraint =
            m_drcEngine->EvalRules( MICROVIA_STACK_DEPTH_CONSTRAINT, top, nullptr, top->GetLayer() );

    if( depthConstraint.GetValue().HasMax() && (int) aColumn.size() > depthConstraint.GetValue().Max()
        && !m_drcEngine->IsErrorLimitExceeded( DRCE_MICROVIA_STACK_DEPTH ) )
    {
        std::shared_ptr<DRC_ITEM> drcItem = DRC_ITEM::Create( DRCE_MICROVIA_STACK_DEPTH );
        drcItem->SetErrorMessage( wxString::Format( _( "Stack is %d hops deep, limit is %d" ), (int) aColumn.size(),
                                                    depthConstraint.GetValue().Max() ) );
        drcItem->SetItems( top );
        drcItem->SetViolatingRule( depthConstraint.GetParentRule() );
        reportViolation( drcItem, top->GetPosition(), top->GetLayer() );
    }

    if( m_drcEngine->IsErrorLimitExceeded( DRCE_MICROVIA_STACK_NOT_FILLED ) )
        return;

    // IPC-6012 requires the hops of a stacked microvia to be filled. An unfilled hop is a dimple,
    // and the joint separates under thermal cycling.
    // An unset fill takes the board default.
    bool boardFillsVias = m_drcEngine->GetBoard()->GetDesignSettings().m_FillVias;

    for( PCB_VIA* via : aColumn )
    {
        if( !via->Padstack().Drill().is_filled.value_or( boardFillsVias ) )
        {
            std::shared_ptr<DRC_ITEM> drcItem = DRC_ITEM::Create( DRCE_MICROVIA_STACK_NOT_FILLED );
            drcItem->SetItems( via );
            reportViolation( drcItem, via->GetPosition(), via->GetLayer() );
        }
    }
}


void DRC_TEST_PROVIDER_VIA_STACK::checkLooseMicrovias( BOARD* aBoard )
{
    std::vector<std::vector<PCB_VIA*>> columns = PCB_VIA::CollectMicroviaColumns( aBoard );

    const int progressDelta = 250;
    int       ii = 0;

    for( const std::vector<PCB_VIA*>& column : columns )
    {
        if( !reportProgress( ii++, columns.size(), progressDelta ) )
            return;

        checkColumn( column );
    }
}


// The dielectric a microvia crosses is the stackup item between its two copper layers, so both
// its thickness and its declared material come from the same walk.
void DRC_TEST_PROVIDER_VIA_STACK::checkAspectRatio( BOARD* aBoard )
{
    BOARD_DESIGN_SETTINGS& bds = aBoard->GetDesignSettings();

    bool checkRatio = !m_drcEngine->IsErrorLimitExceeded( DRCE_MICROVIA_ASPECT_RATIO );
    bool checkCore = !m_drcEngine->IsErrorLimitExceeded( DRCE_MICROVIA_CROSSES_CORE );

    if( !checkRatio && !checkCore )
        return;

    const std::vector<BOARD_STACKUP_ITEM*>& stack = bds.GetStackupDescriptor().GetList();

    const int progressDelta = 500;
    int       ii = 0;

    for( PCB_TRACK* track : aBoard->Tracks() )
    {
        if( !reportProgress( ii++, aBoard->Tracks().size(), progressDelta ) )
            return;

        if( track->Type() != PCB_VIA_T )
            continue;

        PCB_VIA* via = static_cast<PCB_VIA*>( track );

        if( via->GetViaType() != VIATYPE::MICROVIA || via->GetDrillValue() <= 0 )
            continue;

        // Find the copper items the via runs between, and the dielectric they enclose.
        int first = -1;
        int last = -1;

        for( int i = 0; i < (int) stack.size(); ++i )
        {
            if( stack[i]->GetType() != BS_ITEM_TYPE_COPPER )
                continue;

            PCB_LAYER_ID layer = stack[i]->GetBrdLayerId();

            if( layer == via->TopLayer() || layer == via->BottomLayer() )
            {
                if( first < 0 )
                    first = i;
                else
                    last = i;
            }
        }

        if( first < 0 || last < 0 )
            continue;

        int      thickness = 0;
        wxString dielectric;
        bool     crossesCore = false;

        for( int i = first + 1; i < last; ++i )
        {
            if( stack[i]->GetType() != BS_ITEM_TYPE_DIELECTRIC || !stack[i]->IsEnabled() )
                continue;

            // A dielectric can be built from several sublayers.
            for( int sub = 0; sub < stack[i]->GetSublayersCount(); ++sub )
                thickness += stack[i]->GetThickness( sub );

            if( stack[i]->GetTypeName() == KEY_CORE )
                crossesCore = true;

            if( dielectric.IsEmpty() )
                dielectric = stack[i]->GetTypeName();
        }

        // IPC-2226 builds microvias in the layers laminated onto the core, not through it.
        // Laser drilling relies on the resin ablating, and core is glass reinforced.
        if( crossesCore && checkCore )
        {
            std::shared_ptr<DRC_ITEM> drcItem = DRC_ITEM::Create( DRCE_MICROVIA_CROSSES_CORE );

            drcItem->SetErrorMessage(
                    wxString::Format( _( "The stackup gives the dielectric between %s and %s as core" ),
                                      LSET::Name( via->TopLayer() ), LSET::Name( via->BottomLayer() ) ) );
            drcItem->SetItems( via );
            reportViolation( drcItem, via->GetPosition(), via->GetLayer() );
        }

        if( !checkRatio || thickness <= 0 )
            continue;

        DRC_CONSTRAINT ratioConstraint =
                m_drcEngine->EvalRules( MICROVIA_ASPECT_RATIO_CONSTRAINT, via, nullptr, via->GetLayer() );

        if( !ratioConstraint.GetValue().HasMax() )
            continue;

        // Ratios are parsed to three decimals, so the stored maximum is scaled by 1000.
        double limit = ratioConstraint.GetValue().Max() / 1000.0;

        if( limit <= 0.0 )
            continue;

        // IPC-2226 measures the hole against the dielectric plus the copper foil over it.
        // The laser enters from the outer side.
        bool fromBack = IsExternalCopperLayer( stack[last]->GetBrdLayerId() )
                        && !IsExternalCopperLayer( stack[first]->GetBrdLayerId() );

        thickness += stack[fromBack ? last : first]->GetThickness();

        double ratio = (double) thickness / (double) via->GetDrillValue();

        if( ratio <= limit )
            continue;

        if( m_drcEngine->IsErrorLimitExceeded( DRCE_MICROVIA_ASPECT_RATIO ) )
        {
            if( !checkCore )
                return;

            checkRatio = false;
            continue;
        }

        std::shared_ptr<DRC_ITEM> drcItem = DRC_ITEM::Create( DRCE_MICROVIA_ASPECT_RATIO );

        drcItem->SetErrorMessage(
                wxString::Format( _( "Crosses %s dielectric: aspect ratio %.2f exceeds the %.2f limit" ),
                                  dielectric.IsEmpty() ? _( "an unnamed" ) : dielectric, ratio, limit ) );

        drcItem->SetItems( via );
        drcItem->SetViolatingRule( ratioConstraint.GetParentRule() );
        reportViolation( drcItem, via->GetPosition(), via->GetLayer() );
    }
}


bool DRC_TEST_PROVIDER_VIA_STACK::Run()
{
    if( m_drcEngine->IsErrorLimitExceeded( DRCE_MALFORMED_MICROVIA_STACK_SPAN )
        && m_drcEngine->IsErrorLimitExceeded( DRCE_MICROVIA_STACK_NOT_FILLED )
        && m_drcEngine->IsErrorLimitExceeded( DRCE_MICROVIA_STACK_DEPTH )
        && m_drcEngine->IsErrorLimitExceeded( DRCE_MICROVIA_ASPECT_RATIO )
        && m_drcEngine->IsErrorLimitExceeded( DRCE_MICROVIA_CROSSES_CORE ) )
    {
        return true;
    }

    if( !reportPhase( _( "Checking microvia stacks..." ) ) )
        return false;

    BOARD* board = m_drcEngine->GetBoard();
    int    copperLayers = board->GetCopperLayerCount();

    if( !m_drcEngine->IsErrorLimitExceeded( DRCE_MALFORMED_MICROVIA_STACK_SPAN ) )
    {
        for( PCB_GENERATOR* gen : board->Generators() )
        {
            if( m_drcEngine->IsCancelled() )
                break;

            if( PCB_VIA_STACK* stack = dynamic_cast<PCB_VIA_STACK*>( gen ) )
                checkSpan( stack, copperLayers );
        }
    }

    if( !m_drcEngine->IsCancelled()
        && !( m_drcEngine->IsErrorLimitExceeded( DRCE_MICROVIA_STACK_NOT_FILLED )
              && m_drcEngine->IsErrorLimitExceeded( DRCE_MICROVIA_STACK_DEPTH ) ) )
    {
        checkLooseMicrovias( board );
    }

    if( !m_drcEngine->IsCancelled()
        && !( m_drcEngine->IsErrorLimitExceeded( DRCE_MICROVIA_ASPECT_RATIO )
              && m_drcEngine->IsErrorLimitExceeded( DRCE_MICROVIA_CROSSES_CORE ) ) )
    {
        checkAspectRatio( board );
    }

    return !m_drcEngine->IsCancelled();
}


namespace detail
{
static DRC_REGISTER_TEST_PROVIDER<DRC_TEST_PROVIDER_VIA_STACK> dummy;
}
