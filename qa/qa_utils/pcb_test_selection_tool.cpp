#include "pcb_test_frame.h"
#include "pcb_test_selection_tool.h"

#include <pcbnew/tools/pcb_actions.h>
#include <tool/actions.h>
#include <tool/tool_manager.h>

PCB_TEST_SELECTION_TOOL::PCB_TEST_SELECTION_TOOL() : SELECTION_TOOL( "common.InteractiveSelection" )
{
}

PCB_TEST_SELECTION_TOOL::~PCB_TEST_SELECTION_TOOL()
{
    view()->Remove( &m_selection );
}

bool PCB_TEST_SELECTION_TOOL::Init()
{
    view()->Remove( &m_selection );
    view()->Add( &m_selection );

    return true;
}

void PCB_TEST_SELECTION_TOOL::Reset( RESET_REASON aReason )
{
}


void PCB_TEST_SELECTION_TOOL::ClearSelection()
{
    if( m_selection.Empty() )
        return;

    while( m_selection.GetSize() )
        unhighlight( m_selection.Front(), SELECTED, &m_selection );

    view()->Update( &m_selection );

    m_selection.SetIsHover( false );
    m_selection.ClearReferencePoint();
}

#include <collectors.h>


const GENERAL_COLLECTORS_GUIDE PCB_TEST_SELECTION_TOOL::getCollectorsGuide() const
{
    GENERAL_COLLECTORS_GUIDE guide( board()->GetVisibleLayers(),
                                    (PCB_LAYER_ID) view()->GetTopLayer(), view() );

    bool padsDisabled = !board()->IsElementVisible( LAYER_PADS );

    // account for the globals
    guide.SetIgnoreFPTextOnBack( !board()->IsElementVisible( LAYER_FP_TEXT ) );
    guide.SetIgnoreFPTextOnFront( !board()->IsElementVisible( LAYER_FP_TEXT ) );
    guide.SetIgnoreFootprintsOnBack( !board()->IsElementVisible( LAYER_FOOTPRINTS_BK ) );
    guide.SetIgnoreFootprintsOnFront( !board()->IsElementVisible( LAYER_FOOTPRINTS_FR ) );
    guide.SetIgnorePadsOnBack( padsDisabled );
    guide.SetIgnorePadsOnFront( padsDisabled );
    guide.SetIgnoreThroughHolePads( padsDisabled );
    guide.SetIgnoreFPValues( !board()->IsElementVisible( LAYER_FP_VALUES ) );
    guide.SetIgnoreFPReferences( !board()->IsElementVisible( LAYER_FP_REFERENCES ) );
    guide.SetIgnoreThroughVias( !board()->IsElementVisible( LAYER_VIAS ) );
    guide.SetIgnoreBlindBuriedVias( !board()->IsElementVisible( LAYER_VIAS ) );
    guide.SetIgnoreMicroVias( !board()->IsElementVisible( LAYER_VIAS ) );
    guide.SetIgnoreTracks( !board()->IsElementVisible( LAYER_TRACKS ) );

    return guide;
}

bool PCB_TEST_SELECTION_TOOL::selectPoint( const VECTOR2I& aWhere )
{
    GENERAL_COLLECTORS_GUIDE guide = getCollectorsGuide();
    GENERAL_COLLECTOR        collector;

    for( auto item : m_selection.Items() )
    {
        unhighlight( item, SELECTED, &m_selection );
    }

    m_selection.Clear();
    m_selection.ClearReferencePoint();

    if( m_selectableTypes.empty() )
        collector.Collect( board(), GENERAL_COLLECTOR::AllBoardItems, aWhere, guide );
    else
        collector.Collect( board(), m_selectableTypes, aWhere, guide );

    if( collector.GetCount() > 0 )
    {
        for( int i = 0; i < collector.GetCount(); ++i )
        {
            {
                select( collector[i] );
            }
        }
    }

    m_toolMgr->ProcessEvent( EVENTS::SelectedEvent );

    if( m_selectionHook )
        m_selectionHook( frame(), &m_selection );

    return false;
}


void PCB_TEST_SELECTION_TOOL::setTransitions()
{
    Go( &PCB_TEST_SELECTION_TOOL::Main, ACTIONS::selectionActivate.MakeEvent() );
}


void PCB_TEST_SELECTION_TOOL::highlightInternal( EDA_ITEM* aItem, int aMode, bool aUsingOverlay )
{
    if( aMode == SELECTED )
        aItem->SetSelected();
    else if( aMode == BRIGHTENED )
        aItem->SetBrightened();

    if( aUsingOverlay && aMode != BRIGHTENED )
        view()->Hide( aItem, true ); // Hide the original item, so it is shown only on overlay

    if( aItem->IsBOARD_ITEM() )
    {
        BOARD_ITEM* boardItem = static_cast<BOARD_ITEM*>( aItem );
        boardItem->RunOnChildren( std::bind( &PCB_TEST_SELECTION_TOOL::highlightInternal, this, std::placeholders::_1,
                                             aMode, aUsingOverlay ),
                                  RECURSE_MODE::RECURSE );
    }
}


void PCB_TEST_SELECTION_TOOL::unhighlight( EDA_ITEM* aItem, int aMode, SELECTION* aGroup )
{
    if( aGroup )
        aGroup->Remove( aItem );

    unhighlightInternal( aItem, aMode, aGroup != nullptr );
    view()->Update( aItem, KIGFX::REPAINT );

    // Many selections are very temporal and updating the display each time just creates noise.
    if( aMode == BRIGHTENED )
        getView()->MarkTargetDirty( KIGFX::TARGET_OVERLAY );
}


void PCB_TEST_SELECTION_TOOL::unhighlightInternal( EDA_ITEM* aItem, int aMode, bool aUsingOverlay )
{
    if( aMode == SELECTED )
        aItem->ClearSelected();
    else if( aMode == BRIGHTENED )
        aItem->ClearBrightened();

    if( aUsingOverlay && aMode != BRIGHTENED )
    {
        view()->Hide( aItem, false ); // Restore original item visibility...
        view()->Update( aItem );      // ... and make sure it's redrawn un-selected
    }

    if( aItem->IsBOARD_ITEM() )
    {
        BOARD_ITEM* boardItem = static_cast<BOARD_ITEM*>( aItem );
        boardItem->RunOnChildren( std::bind( &PCB_TEST_SELECTION_TOOL::unhighlightInternal, this, std::placeholders::_1,
                                             aMode, aUsingOverlay ),
                                  RECURSE_MODE::RECURSE );
    }
}


void PCB_TEST_SELECTION_TOOL::highlight( EDA_ITEM* aItem, int aMode, SELECTION* aGroup )
{
    if( aGroup )
        aGroup->Add( aItem );

    highlightInternal( aItem, aMode, aGroup != nullptr );
    view()->Update( aItem, KIGFX::REPAINT );
}


void PCB_TEST_SELECTION_TOOL::select( EDA_ITEM* aItem )
{
    if( aItem->IsSelected() )
        return;

    highlight( aItem, SELECTED, &m_selection );
}


void PCB_TEST_SELECTION_TOOL::unselect( EDA_ITEM* aItem )
{
    unhighlight( aItem, SELECTED, &m_selection );
}

int PCB_TEST_SELECTION_TOOL::Main( const TOOL_EVENT& aEvent )
{
    // Main loop: keep receiving events
    while( TOOL_EVENT* evt = Wait() )
    {
        if( evt->IsClick( BUT_LEFT ) )
        {
            selectPoint( evt->Position() );
        }
        else if( evt->IsCancel() )
        {
            if( !GetSelection().Empty() )
            {
                ClearSelection();
            }
        }
        else
        {
            evt->SetPassEvent();
        }
    }

    // Shutting down; clear the selection
    m_selection.Clear();
    m_disambiguateTimer.Stop();

    return 0;
}

void PCB_TEST_SELECTION_TOOL::SetSelectableItemTypes( const std::vector<KICAD_T> aTypes )
{
    m_selectableTypes = aTypes;
}
