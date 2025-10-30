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
 * along with this program; if not, you may find one here:
 * http://www.gnu.org/licenses/old-licenses/gpl-2.0.html
 * or you may search the http://www.gnu.org website for the version 2 license,
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

#include <kiface_base.h>
#include <confirm.h>
#include <pcb_edit_frame.h>
#include <eda_dde.h>
#include <wx/file.h>
#include <wx/snglinst.h>
#include <wx/app.h>
#include <board.h>
#include <footprint_viewer_frame.h>
#include <footprint.h>
#include <tools/pcb_actions.h>
#include <tools/zone_filler_tool.h>
#include <router/router_tool.h>
#include <dialog_find.h>
#include <dialog_filter_selection.h>
#include <zone_filler.h>
struct PCB_SELECTION_FILTER_OPTIONS;
#include <preview_items/selection_area.h>


DIALOG_FIND::DIALOG_FIND( PCB_EDIT_FRAME* aParent ) :
        DIALOG_FIND_BASE( aParent )
{
    // these members are initialized to avoid warnings about non initialized vars
    m_frame = aParent;
    m_hitList.clear();
    m_it = m_hitList.begin();
    m_upToDate = false;
    m_board = nullptr;
}

DIALOG_FIND::~DIALOG_FIND() {}

void DIALOG_FIND::OnBoardChanged( wxCommandEvent& event ) {}

void DIALOG_FIND::onFindNextClick( wxCommandEvent& aEvent )
{
}

void DIALOG_FIND::onFindPreviousClick( wxCommandEvent& aEvent )
{
}

void DIALOG_FIND::onSearchAgainClick( wxCommandEvent& aEvent )
{
}

void DIALOG_FIND::onTextEnter( wxCommandEvent& event )
{
}

void DIALOG_FIND::onShowSearchPanel( wxHyperlinkEvent& event )
{
}

bool DIALOG_FIND::Show( bool show )
{
    return true;
}


DIALOG_FIND_BASE::DIALOG_FIND_BASE( wxWindow* parent, wxWindowID id, const wxString& title,
                                    const wxPoint& pos, const wxSize& size, long style ) :
        DIALOG_SHIM( parent, id, title, pos, size, style )
{
    // these members are initialized only to avoid warnings about non initialized vars
    searchStringLabel = nullptr;
    m_searchCombo = nullptr;
    m_matchCase = nullptr;
    m_matchWords = nullptr;
    m_wildcards = nullptr;
    m_wrap = nullptr;
    m_includeValues = nullptr;
    m_includeReferences = nullptr;
    m_includeTexts = nullptr;
    m_includeMarkers = nullptr;
    m_includeNets = nullptr;
    m_findNext = nullptr;
    m_findPrevious = nullptr;
    m_searchAgain = nullptr;
    m_closeButton = nullptr;
    m_status = nullptr;
}


DIALOG_FIND_BASE::~DIALOG_FIND_BASE()
{
}


DIALOG_FILTER_SELECTION_BASE::DIALOG_FILTER_SELECTION_BASE( wxWindow* parent, wxWindowID id,
                                                            const wxString& title,
                                                            const wxPoint& pos, const wxSize& size,
                                                            long style ) :
        DIALOG_SHIM( parent, id, title, pos, size, style )
{
    // these members are initialized only to avoid warnings about non initialized vars
    m_Include_Modules = nullptr;
    m_Include_PcbTexts = nullptr;
    m_IncludeLockedModules = nullptr;
    m_Include_Draw_Items = nullptr;
    m_Include_Tracks = nullptr;
    m_Include_Vias = nullptr;
    m_Include_Edges_Items = nullptr;
    m_Include_Zones = nullptr;
    m_sdbSizer1 = nullptr;
    m_sdbSizer1OK = nullptr;
    m_sdbSizer1Cancel = nullptr;
}


DIALOG_FILTER_SELECTION_BASE::~DIALOG_FILTER_SELECTION_BASE()
{
}


DIALOG_FILTER_SELECTION::DIALOG_FILTER_SELECTION( PCB_BASE_FRAME* aParent, OPTIONS& aOptions ) :
        DIALOG_FILTER_SELECTION_BASE( aParent ),
        m_options( aOptions )
{
    // silence another compiler warning about m_options not being used
    if( m_options.includeFootprints )
    {
    }
}


void DIALOG_FILTER_SELECTION::checkBoxClicked( wxCommandEvent& aEvent )
{
}


void DIALOG_FILTER_SELECTION::allItemsClicked( wxCommandEvent& aEvent )
{
}


bool DIALOG_FILTER_SELECTION::TransferDataToWindow()
{
    return true;
}


bool DIALOG_FILTER_SELECTION::TransferDataFromWindow()
{
    return true;
}


void ROUTER_TOOL::NeighboringSegmentFilter( const VECTOR2I&, GENERAL_COLLECTOR&, PCB_SELECTION_TOOL* )
{
}



#include <tools/pcb_selection_tool.h>


/**
 * Private implementation of firewalled private data
 */
class PCB_SELECTION_TOOL::PRIV
{
public:
    DIALOG_FILTER_SELECTION::OPTIONS m_filterOpts;
};


PCB_SELECTION_TOOL::PCB_SELECTION_TOOL() :
        SELECTION_TOOL( "common.InteractiveSelection" ),
        m_frame( NULL ),
        m_enteredGroup( NULL ),
        m_nonModifiedCursor( KICURSOR::ARROW ),
        m_priv( nullptr )
{
}


PCB_SELECTION_TOOL::~PCB_SELECTION_TOOL()
{
}


bool PCB_SELECTION_TOOL::Init()
{
    return true;
}


void PCB_SELECTION_TOOL::Reset( RESET_REASON aReason )
{
}


int PCB_SELECTION_TOOL::Main( const TOOL_EVENT& aEvent )
{
    return 0;
}


void PCB_SELECTION_TOOL::EnterGroup()
{
}


void PCB_SELECTION_TOOL::ExitGroup( bool aSelectGroup )
{
}


PCB_SELECTION& PCB_SELECTION_TOOL::GetSelection()
{
    return m_selection;
}


PCB_SELECTION& PCB_SELECTION_TOOL::RequestSelection( CLIENT_SELECTION_FILTER aClientFilter )
{
    return m_selection;
}


const GENERAL_COLLECTORS_GUIDE PCB_SELECTION_TOOL::getCollectorsGuide() const
{
    return GENERAL_COLLECTORS_GUIDE( LSET(), PCB_LAYER_ID::UNDEFINED_LAYER, nullptr );
}


bool PCB_SELECTION_TOOL::selectPoint( const VECTOR2I& aWhere, bool aOnDrag,
                                      bool* aSelectionCancelledFlag,
                                      CLIENT_SELECTION_FILTER aClientFilter )
{
    return false;
}


bool PCB_SELECTION_TOOL::selectCursor( bool aForceSelect, CLIENT_SELECTION_FILTER aClientFilter )
{
    return false;
}


void PCB_SELECTION_TOOL::SelectMultiple( KIGFX::PREVIEW::SELECTION_AREA& aArea, bool aSubtractive,
                                         bool aExclusiveOr )
{
}


int PCB_SELECTION_TOOL::CursorSelection( const TOOL_EVENT& aEvent )
{
    return 0;
}


int PCB_SELECTION_TOOL::ClearSelection( const TOOL_EVENT& aEvent )
{
    return 0;
}


int PCB_SELECTION_TOOL::SelectAll( const TOOL_EVENT& aEvent )
{
    return 0;
}


int PCB_SELECTION_TOOL::expandConnection( const TOOL_EVENT& aEvent )
{
    return 0;
}


void PCB_SELECTION_TOOL::selectAllConnectedTracks(
        const std::vector<BOARD_CONNECTED_ITEM*>& aStartItems, STOP_CONDITION aStopCondition )
{
}


void PCB_SELECTION_TOOL::SelectAllItemsOnNet( int aNetCode, bool aSelect )
{
}


int PCB_SELECTION_TOOL::selectNet( const TOOL_EVENT& aEvent )
{
    return 0;
}


void PCB_SELECTION_TOOL::selectAllItemsOnSheet( wxString& aSheetPath )
{
}


void PCB_SELECTION_TOOL::zoomFitSelection()
{
}


int PCB_SELECTION_TOOL::selectSheetContents( const TOOL_EVENT& aEvent )
{
    return 0;
}


int PCB_SELECTION_TOOL::selectSameSheet( const TOOL_EVENT& aEvent )
{
    return 0;
}


bool PCB_SELECTION_TOOL::ctrlClickHighlights()
{
    return false;
}


/**
 * Function itemIsIncludedByFilter()
 *
 * Determine if an item is included by the filter specified
 *
 * @return true if aItem should be selected by this filter (i..e not filtered out)
 */
static bool itemIsIncludedByFilter( const BOARD_ITEM& aItem, const BOARD& aBoard,
                                    const DIALOG_FILTER_SELECTION::OPTIONS& aFilterOptions )
{
    return false;
}


int PCB_SELECTION_TOOL::filterSelection( const TOOL_EVENT& aEvent )
{
    return 0;
}


void PCB_SELECTION_TOOL::FilterCollectedItems( GENERAL_COLLECTOR& aCollector, bool aMultiSelect,
                                               PCB_SELECTION_FILTER_OPTIONS* aRejected )
{
}


bool PCB_SELECTION_TOOL::itemPassesFilter( BOARD_ITEM* aItem, bool aMultiSelect,
                                          PCB_SELECTION_FILTER_OPTIONS* aRejected )
{
    return true;
}


void PCB_SELECTION_TOOL::ClearSelection( bool aQuietMode )
{
}


void PCB_SELECTION_TOOL::RebuildSelection()
{
}


bool PCB_SELECTION_TOOL::Selectable( const BOARD_ITEM* aItem, bool checkVisibilityOnly ) const
{
    return false;
}


bool PCB_SELECTION_TOOL::selectionContains( const VECTOR2I& aPoint ) const
{
    return false;
}

void PCB_SELECTION_TOOL::GuessSelectionCandidates( GENERAL_COLLECTOR& aCollector,
                                                   const VECTOR2I& aWhere ) const
{
}


int PCB_SELECTION_TOOL::updateSelection( const TOOL_EVENT& aEvent )
{
    return 0;
}


void PCB_SELECTION_TOOL::setTransitions()
{
}


void PCB_SELECTION_TOOL::select( EDA_ITEM* aItem )
{
}


void PCB_SELECTION_TOOL::unselect( EDA_ITEM* aItem )
{
}


void PCB_SELECTION_TOOL::highlight( EDA_ITEM* aItem, int aHighlightMode,
                                    SELECTION* aGroup )
{
}


void PCB_SELECTION_TOOL::unhighlight( EDA_ITEM* aItem, int aHighlightMode,
                                              SELECTION* aGroup )
{
}

void PCB_TOOL_BASE::doInteractiveItemPlacement( const TOOL_EVENT&        aTool,
                                                INTERACTIVE_PLACER_BASE* aPlacer,
                                                const wxString& aCommitMessage, int aOptions )
{
}


bool PCB_TOOL_BASE::Init()
{
    return true;
}


void PCB_TOOL_BASE::Reset( RESET_REASON aReason )
{
}


void PCB_TOOL_BASE::setTransitions()
{
}


bool PCB_TOOL_BASE::Is45Limited() const
{
    return false;
}


bool PCB_TOOL_BASE::Is90Limited() const
{
    return false;
}


ZONE_FILLER::~ZONE_FILLER()
{
}


ZONE_FILLER_TOOL::ZONE_FILLER_TOOL() :
    PCB_TOOL_BASE( ZONE_FILLER_TOOL_NAME ),
    m_fillInProgress( false )
{
}


ZONE_FILLER_TOOL::~ZONE_FILLER_TOOL()
{
}


void ZONE_FILLER_TOOL::Reset( RESET_REASON aReason )
{
}


void ZONE_FILLER_TOOL::setTransitions()
{
}


PCBNEW_SETTINGS::DISPLAY_OPTIONS& PCB_TOOL_BASE::displayOptions() const
{
    static PCBNEW_SETTINGS::DISPLAY_OPTIONS disp;

    return disp;
}

PCB_DRAW_PANEL_GAL* PCB_TOOL_BASE::canvas() const
{
    return nullptr;
}


const PCB_SELECTION& PCB_TOOL_BASE::selection() const
{
    static PCB_SELECTION sel;

    return sel;
}


PCB_SELECTION& PCB_TOOL_BASE::selection()
{
    static PCB_SELECTION sel;

    return sel;
}

BOX2I PCB_SELECTION::GetBoundingBox() const
{
    return BOX2I();
}


EDA_ITEM* PCB_SELECTION::GetTopLeftItem( bool onlyModules ) const
{
   return nullptr;
}


const std::vector<KIGFX::VIEW_ITEM*> PCB_SELECTION::updateDrawList() const
{
    std::vector<VIEW_ITEM*> items;

   return items;
}


#if 0
#include <3d_canvas/board_adapter.h>

BOARD_ADAPTER::BOARD_ADAPTER() {};
BOARD_ADAPTER::~BOARD_ADAPTER() {};
BBOX_3D::BBOX_3D(){};
BBOX_3D::~BBOX_3D(){};
TRACK_BALL::TRACK_BALL(float x) : CAMERA(x) {};
BVH_CONTAINER_2D::BVH_CONTAINER_2D() : CONTAINER_2D_BASE( OBJECT_2D_TYPE::POLYGON ) {};
BVH_CONTAINER_2D::~BVH_CONTAINER_2D() {};

#endif

#include <board_stackup_manager/stackup_predefined_prms.h>

const FAB_LAYER_COLOR* GetColorStandardList()
{
    return nullptr;
}


int GetColorStandardListCount()
{
    return 0;
}
