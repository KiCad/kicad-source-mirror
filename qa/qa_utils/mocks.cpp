/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 1992-2020 KiCad Developers, see AUTHORS.txt for contributors.
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

#include <pgm_base.h>
#include <kiface_base.h>
#include <confirm.h>
#include <pcb_edit_frame.h>
#include <eda_dde.h>
#include <wx/file.h>
#include <wx/snglinst.h>
#include <wx/app.h>
#include <board.h>
#include <fp_lib_table.h>
#include <footprint_viewer_frame.h>
#include <footprint_wizard_frame.h>
#include <footprint.h>
#include <tools/pcb_actions.h>
#include <router/router_tool.h>
#include <dialog_find.h>
#include <dialog_filter_selection.h>

FP_LIB_TABLE GFootprintTable;


#if 0

static struct IFACE : public KIFACE_BASE
{
    // Of course all are overloads, implementations of the KIFACE.

    IFACE( const char* aName, KIWAY::FACE_T aType ) :
        KIFACE_BASE( aName, aType )
    {}

    bool OnKifaceStart( PGM_BASE* aProgram, int aCtlBits ) override
    {
        return true;
    }

    void OnKifaceEnd() override {}

    wxWindow* CreateWindow( wxWindow* aParent, int aClassId, KIWAY* aKiway, int aCtlBits = 0 ) override
    {
        assert( false );
        return nullptr;
    }

    /**
     * Function IfaceOrAddress
     * return a pointer to the requested object.  The safest way to use this
     * is to retrieve a pointer to a static instance of an interface, similar to
     * how the KIFACE interface is exported.  But if you know what you are doing
     * use it to retrieve anything you want.
     *
     * @param aDataId identifies which object you want the address of.
     *
     * @return void* - and must be cast into the know type.
     */
    void* IfaceOrAddress( int aDataId ) override
    {
        return NULL;
    }
}
kiface( "pcb_test_frame", KIWAY::FACE_PCB );

static struct PGM_TEST_FRAME : public PGM_BASE
{
    bool OnPgmInit();

    void OnPgmExit()
    {
        Kiway.OnKiwayEnd();

        // Destroy everything in PGM_BASE, especially wxSingleInstanceCheckerImpl
        // earlier than wxApp and earlier than static destruction would.
        PGM_BASE::Destroy();
    }

    void MacOpenFile( const wxString& aFileName )   override
    {
        wxFileName filename( aFileName );

        if( filename.FileExists() )
        {
    #if 0
            // this pulls in EDA_DRAW_FRAME type info, which we don't want in
            // the single_top link image.
            KIWAY_PLAYER* frame = dynamic_cast<KIWAY_PLAYER*>( App().GetTopWindow() );
    #else
            KIWAY_PLAYER* frame = (KIWAY_PLAYER*) App().GetTopWindow();
    #endif

            if( frame )
                frame->OpenProjectFiles( std::vector<wxString>( 1, aFileName ) );
        }
    }
}
program;

PGM_BASE& Pgm()
{
    return program;
}


// Similar to PGM_BASE& Pgm(), but return nullptr when a *.ki_face
// is run from a python script, mot from a Kicad application
PGM_BASE* PgmOrNull()
{
    return nullptr; //&program;
}


KIFACE_BASE& Kiface()
{
    return kiface;
}
#endif

// FP_LIB_TABLE GFootprintTable;

DIALOG_FIND::DIALOG_FIND( PCB_BASE_FRAME* aParent ) : DIALOG_FIND_BASE( aParent )
{
    // these members are initialized to avoid warnings about non initialized vars
    m_frame = aParent;
    m_hitList.clear();
    m_it = m_hitList.begin();
    m_upToDate = false;
}

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

void DIALOG_FIND::OnCloseButtonClick( wxCommandEvent& aEvent )
{
}

void DIALOG_FIND::OnClose( wxCloseEvent& aEvent )
{
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
    m_includeVias = nullptr;
    m_findNext = nullptr;
    m_findPrevious = nullptr;
    m_searchAgain = nullptr;
    m_closeButton = nullptr;
    m_status = nullptr;
}


DIALOG_FIND_BASE::~DIALOG_FIND_BASE()
{
}


FOOTPRINT* PCB_BASE_FRAME::GetFootprintFromBoardByReference()
{
    return nullptr;
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
    m_staticLine = nullptr;
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
    if( m_options.includeModules )
    {
    }
}


void DIALOG_FILTER_SELECTION::checkBoxClicked( wxCommandEvent& aEvent )
{
}


void DIALOG_FILTER_SELECTION::allItemsClicked( wxCommandEvent& aEvent )
{
}


bool DIALOG_FILTER_SELECTION::TransferDataFromWindow()
{
    return true;
}


void ROUTER_TOOL::NeighboringSegmentFilter( const VECTOR2I&, GENERAL_COLLECTOR& )
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
        PCB_TOOL_BASE( "pcbnew.InteractiveSelection" ),
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


PCB_SELECTION& PCB_SELECTION_TOOL::RequestSelection( CLIENT_SELECTION_FILTER aClientFilter,
                                                 bool aConfirmLockedItems )
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


bool PCB_SELECTION_TOOL::selectMultiple()
{
    return false;
}


int PCB_SELECTION_TOOL::CursorSelection( const TOOL_EVENT& aEvent )
{
    return 0;
}


int PCB_SELECTION_TOOL::ClearSelection( const TOOL_EVENT& aEvent )
{
    return 0;
}


int PCB_SELECTION_TOOL::SelectItems( const TOOL_EVENT& aEvent )
{
    return 0;
}


int PCB_SELECTION_TOOL::SelectItem( const TOOL_EVENT& aEvent )
{
    return 0;
}


int PCB_SELECTION_TOOL::SelectAll( const TOOL_EVENT& aEvent )
{
    return 0;
}


void PCB_SELECTION_TOOL::AddItemToSel( BOARD_ITEM* aItem, bool aQuietMode )
{
}


int PCB_SELECTION_TOOL::UnselectItems( const TOOL_EVENT& aEvent )
{
    return 0;
}


int PCB_SELECTION_TOOL::UnselectItem( const TOOL_EVENT& aEvent )
{
    return 0;
}


void PCB_SELECTION_TOOL::RemoveItemFromSel( BOARD_ITEM* aItem, bool aQuietMode )
{
}


void PCB_SELECTION_TOOL::BrightenItem( BOARD_ITEM* aItem )
{
}


void PCB_SELECTION_TOOL::UnbrightenItem( BOARD_ITEM* aItem )
{
}


int PCB_SELECTION_TOOL::expandConnection( const TOOL_EVENT& aEvent )
{
    return 0;
}


void PCB_SELECTION_TOOL::selectConnectedTracks( BOARD_CONNECTED_ITEM& aStartItem,
                                                STOP_CONDITION aStopCondition )
{
}


void PCB_SELECTION_TOOL::selectAllItemsOnNet( int aNetCode, bool aSelect )
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


void PCB_SELECTION_TOOL::FilterCollectedItems( GENERAL_COLLECTOR& aCollector, bool aMultiSelect )
{
}


bool PCB_SELECTION_TOOL::itemPassesFilter( BOARD_ITEM* aItem, bool aMultiSelect )
{
    return true;
}


void PCB_SELECTION_TOOL::ClearSelection( bool aQuietMode )
{
}


void PCB_SELECTION_TOOL::RebuildSelection()
{
}


int PCB_SELECTION_TOOL::SelectionMenu( const TOOL_EVENT& aEvent )
{
    return 0;
}


bool PCB_SELECTION_TOOL::doSelectionMenu( GENERAL_COLLECTOR* aCollector )
{
    return false;
}


bool PCB_SELECTION_TOOL::Selectable( const BOARD_ITEM* aItem, bool checkVisibilityOnly ) const
{
    return false;
}


void PCB_SELECTION_TOOL::select( BOARD_ITEM* aItem )
{
}


void PCB_SELECTION_TOOL::unselect( BOARD_ITEM* aItem )
{
}

void PCB_SELECTION_TOOL::highlight( BOARD_ITEM* aItem, int aMode, PCB_SELECTION* aGroup )
{
}

void PCB_SELECTION_TOOL::highlightInternal( BOARD_ITEM* aItem, int aMode, bool aUsingOverlay )
{
}


void PCB_SELECTION_TOOL::unhighlight( BOARD_ITEM* aItem, int aMode, PCB_SELECTION* aGroup )
{
}


void PCB_SELECTION_TOOL::unhighlightInternal( BOARD_ITEM* aItem, int aMode, bool aUsingOverlay )
{
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


int PCB_SELECTION_TOOL::UpdateMenu( const TOOL_EVENT& aEvent )
{
    return 0;
}


void PCB_SELECTION_TOOL::setTransitions()
{
}

void PCB_TOOL_BASE::doInteractiveItemPlacement( const std::string& aTool,
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


const PCB_DISPLAY_OPTIONS& PCB_TOOL_BASE::displayOptions() const
{
    static PCB_DISPLAY_OPTIONS disp;

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


EDA_ITEM* PCB_SELECTION::GetTopLeftItem( bool onlyModules ) const
{
   return nullptr;
}


const std::vector<KIGFX::VIEW_ITEM*> PCB_SELECTION::updateDrawList() const
{
    std::vector<VIEW_ITEM*> items;

   return items;
}


const LSET PCB_SELECTION::GetSelectionLayers()
{
  return LSET();
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
