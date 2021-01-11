/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2014 CERN
 * Copyright (C) 2014-2020 KiCad Developers, see AUTHORS.txt for contributors.
 * @author Maciej Suminski <maciej.suminski@cern.ch>
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
#include "board_editor_control.h"
#include "drawing_tool.h"
#include "pcb_actions.h"
#include "pcb_picker_tool.h"
#include "pcb_selection_tool.h"
#include "edit_tool.h"
#include "tool/tool_event.h"
#include <bitmaps.h>
#include <board_commit.h>
#include <board.h>
#include <pcb_group.h>
#include <footprint.h>
#include <pcb_target.h>
#include <track.h>
#include <zone.h>
#include <pcb_marker.h>
#include <collectors.h>
#include <confirm.h>
#include <cstdint>
#include <dialogs/dialog_page_settings.h>
#include <dialogs/dialog_update_pcb.h>
#include <functional>
#include <gestfich.h>
#include <kiface_i.h>
#include <kiway.h>
#include <memory>
#include <netlist_reader/pcb_netlist.h>
#include <origin_viewitem.h>
#include <painter.h>
#include <pcb_edit_frame.h>
#include <pcbnew_id.h>
#include <pcbnew_settings.h>
#include <project.h>
#include <project/project_file.h> // LAST_PATH_TYPE
#include <tool/tool_manager.h>
#include <tools/tool_event_utils.h>
#include <router/router_tool.h>
#include <view/view_controls.h>
#include <view/view_group.h>
#include <wildcards_and_files_ext.h>
#include <page_layout/ws_proxy_undo_item.h>
#include <footprint_edit_frame.h>

using namespace std::placeholders;


class ZONE_CONTEXT_MENU : public ACTION_MENU
{
public:
    ZONE_CONTEXT_MENU() :
        ACTION_MENU( true )
    {
        SetIcon( add_zone_xpm );
        SetTitle( _( "Zones" ) );

        Add( PCB_ACTIONS::zoneFill );
        Add( PCB_ACTIONS::zoneFillAll );
        Add( PCB_ACTIONS::zoneUnfill );
        Add( PCB_ACTIONS::zoneUnfillAll );

        AppendSeparator();

        Add( PCB_ACTIONS::zoneMerge );
        Add( PCB_ACTIONS::zoneDuplicate );
        Add( PCB_ACTIONS::drawZoneCutout );
        Add( PCB_ACTIONS::drawSimilarZone );
    }


protected:
    ACTION_MENU* create() const override
    {
        return new ZONE_CONTEXT_MENU();
    }
};


class LOCK_CONTEXT_MENU : public ACTION_MENU
{
public:
    LOCK_CONTEXT_MENU() :
        ACTION_MENU( true )
    {
        SetIcon( locked_xpm );
        SetTitle( _( "Locking" ) );

        Add( PCB_ACTIONS::lock );
        Add( PCB_ACTIONS::unlock );
        Add( PCB_ACTIONS::toggleLock );
    }

    ACTION_MENU* create() const override
    {
        return new LOCK_CONTEXT_MENU();
    }
};


BOARD_EDITOR_CONTROL::BOARD_EDITOR_CONTROL() :
    PCB_TOOL_BASE( "pcbnew.EditorControl" ),
    m_frame( nullptr )
{
    m_placeOrigin = std::make_unique<KIGFX::ORIGIN_VIEWITEM>( KIGFX::COLOR4D( 0.8, 0.0, 0.0, 1.0 ),
                                                              KIGFX::ORIGIN_VIEWITEM::CIRCLE_CROSS );
}


BOARD_EDITOR_CONTROL::~BOARD_EDITOR_CONTROL()
{
}


void BOARD_EDITOR_CONTROL::Reset( RESET_REASON aReason )
{
    m_frame = getEditFrame<PCB_EDIT_FRAME>();

    if( aReason == MODEL_RELOAD || aReason == GAL_SWITCH )
    {
        m_placeOrigin->SetPosition( getModel<BOARD>()->GetDesignSettings().m_AuxOrigin );
        getView()->Remove( m_placeOrigin.get() );
        getView()->Add( m_placeOrigin.get() );
    }
}


bool BOARD_EDITOR_CONTROL::Init()
{
    auto activeToolCondition =
            [ this ] ( const SELECTION& aSel )
            {
                return ( !m_frame->ToolStackIsEmpty() );
            };

    auto inactiveStateCondition =
            [ this ] ( const SELECTION& aSel )
            {
                return ( m_frame->ToolStackIsEmpty() && aSel.Size() == 0 );
            };

    auto placeModuleCondition =
            [ this ] ( const SELECTION& aSel )
            {
                return m_frame->IsCurrentTool( PCB_ACTIONS::placeModule ) && aSel.GetSize() == 0;
            };

    auto& ctxMenu = m_menu.GetMenu();

    // "Cancel" goes at the top of the context menu when a tool is active
    ctxMenu.AddItem( ACTIONS::cancelInteractive, activeToolCondition, 1 );
    ctxMenu.AddSeparator( 1 );

    // "Get and Place Footprint" should be available for Place Footprint tool
    ctxMenu.AddItem( PCB_ACTIONS::getAndPlace, placeModuleCondition, 1000 );
    ctxMenu.AddSeparator( 1000 );

    // Finally, add the standard zoom & grid items
    getEditFrame<PCB_BASE_FRAME>()->AddStandardSubMenus( m_menu );

    auto zoneMenu = std::make_shared<ZONE_CONTEXT_MENU>();
    zoneMenu->SetTool( this );

    auto lockMenu = std::make_shared<LOCK_CONTEXT_MENU>();
    lockMenu->SetTool( this );

    // Add the PCB control menus to relevant other tools

    PCB_SELECTION_TOOL* selTool = m_toolMgr->GetTool<PCB_SELECTION_TOOL>();

    if( selTool )
    {
        auto& toolMenu = selTool->GetToolMenu();
        auto& menu = toolMenu.GetMenu();

        // Add "Get and Place Footprint" when Selection tool is in an inactive state
        menu.AddItem( PCB_ACTIONS::getAndPlace, inactiveStateCondition );
        menu.AddSeparator();

        toolMenu.AddSubMenu( zoneMenu );
        toolMenu.AddSubMenu( lockMenu );

        menu.AddMenu( lockMenu.get(), SELECTION_CONDITIONS::NotEmpty, 100 );

        menu.AddMenu( zoneMenu.get(), SELECTION_CONDITIONS::OnlyType( PCB_ZONE_T ), 200 );
    }

    DRAWING_TOOL* drawingTool = m_toolMgr->GetTool<DRAWING_TOOL>();

    if( drawingTool )
    {
        auto& toolMenu = drawingTool->GetToolMenu();
        auto& menu = toolMenu.GetMenu();

        toolMenu.AddSubMenu( zoneMenu );

        // Functor to say if the PCB_EDIT_FRAME is in a given mode
        // Capture the tool pointer and tool mode by value
        auto toolActiveFunctor = [=]( DRAWING_TOOL::MODE aMode )
        {
            return [=]( const SELECTION& sel )
            {
                return drawingTool->GetDrawingMode() == aMode;
            };
        };

        menu.AddMenu( zoneMenu.get(), toolActiveFunctor( DRAWING_TOOL::MODE::ZONE ), 200 );
    }

    return true;
}


int BOARD_EDITOR_CONTROL::New( const TOOL_EVENT& aEvent )
{
    m_frame->Files_io_from_id( ID_NEW_BOARD );
    return 0;
}


int BOARD_EDITOR_CONTROL::Open( const TOOL_EVENT& aEvent )
{
    m_frame->Files_io_from_id( ID_LOAD_FILE );
    return 0;
}


int BOARD_EDITOR_CONTROL::Save( const TOOL_EVENT& aEvent )
{
    m_frame->Files_io_from_id( ID_SAVE_BOARD );
    return 0;
}


int BOARD_EDITOR_CONTROL::SaveAs( const TOOL_EVENT& aEvent )
{
    m_frame->Files_io_from_id( ID_SAVE_BOARD_AS );
    return 0;
}


int BOARD_EDITOR_CONTROL::SaveCopyAs( const TOOL_EVENT& aEvent )
{
    m_frame->Files_io_from_id( ID_COPY_BOARD_AS );
    return 0;
}


int BOARD_EDITOR_CONTROL::PageSettings( const TOOL_EVENT& aEvent )
{
    PICKED_ITEMS_LIST   undoCmd;
    WS_PROXY_UNDO_ITEM* undoItem = new WS_PROXY_UNDO_ITEM( m_frame );
    ITEM_PICKER         wrapper( nullptr, undoItem, UNDO_REDO::PAGESETTINGS );

    undoCmd.PushItem( wrapper );
    m_frame->SaveCopyInUndoList( undoCmd, UNDO_REDO::PAGESETTINGS );

    DIALOG_PAGES_SETTINGS dlg( m_frame, IU_PER_MILS,
                               wxSize( MAX_PAGE_SIZE_PCBNEW_MILS, MAX_PAGE_SIZE_PCBNEW_MILS ) );
    dlg.SetWksFileName( BASE_SCREEN::m_PageLayoutDescrFileName );

    if( dlg.ShowModal() != wxID_OK )
        m_frame->RollbackFromUndo();

    return 0;
}


int BOARD_EDITOR_CONTROL::Plot( const TOOL_EVENT& aEvent )
{
    m_frame->ToPlotter( ID_GEN_PLOT );
    return 0;
}


int BOARD_EDITOR_CONTROL::BoardSetup( const TOOL_EVENT& aEvent )
{
    getEditFrame<PCB_EDIT_FRAME>()->ShowBoardSetupDialog();
    return 0;
}


int BOARD_EDITOR_CONTROL::ImportNetlist( const TOOL_EVENT& aEvent )
{
    getEditFrame<PCB_EDIT_FRAME>()->InstallNetlistFrame();
    return 0;
}


int BOARD_EDITOR_CONTROL::ImportSpecctraSession( const TOOL_EVENT& aEvent )
{
    wxString fullFileName = frame()->GetBoard()->GetFileName();
    wxString path;
    wxString name;
    wxString ext;

    wxFileName::SplitPath( fullFileName, &path, &name, &ext );
    name += wxT( ".ses" );

    fullFileName = EDA_FILE_SELECTOR( _( "Merge Specctra Session file:" ), path, name,
                                      wxT( ".ses" ), wxT( "*.ses" ), frame(), wxFD_OPEN, false );

    if( !fullFileName.IsEmpty() )
        getEditFrame<PCB_EDIT_FRAME>()->ImportSpecctraSession( fullFileName );

    return 0;
}


int BOARD_EDITOR_CONTROL::ExportSpecctraDSN( const TOOL_EVENT& aEvent )
{
    wxString    fullFileName = m_frame->GetLastPath( LAST_PATH_SPECCTRADSN );
    wxFileName  fn;

    if( fullFileName.IsEmpty() )
    {
        fn = m_frame->GetBoard()->GetFileName();
        fn.SetExt( SpecctraDsnFileExtension );
    }
    else
        fn = fullFileName;

    fullFileName = EDA_FILE_SELECTOR( _( "Specctra DSN File" ), fn.GetPath(), fn.GetFullName(),
                                      SpecctraDsnFileExtension, SpecctraDsnFileWildcard(),
                                      frame(), wxFD_SAVE | wxFD_OVERWRITE_PROMPT, false );

    if( !fullFileName.IsEmpty() )
    {
        m_frame->SetLastPath( LAST_PATH_SPECCTRADSN, fullFileName );
        getEditFrame<PCB_EDIT_FRAME>()->ExportSpecctraFile( fullFileName );
    }

    return 0;
}


int BOARD_EDITOR_CONTROL::GenerateFabFiles( const TOOL_EVENT& aEvent )
{
    wxCommandEvent dummy;

    if( aEvent.IsAction( &PCB_ACTIONS::generateGerbers ) )
        m_frame->ToPlotter( ID_GEN_PLOT_GERBER );
    else if( aEvent.IsAction( &PCB_ACTIONS::generateReportFile ) )
        m_frame->GenFootprintsReport( dummy );
    else if( aEvent.IsAction( &PCB_ACTIONS::generateD356File ) )
        m_frame->GenD356File( dummy );
    else if( aEvent.IsAction( &PCB_ACTIONS::generateBOM ) )
        m_frame->RecreateBOMFileFromBoard( dummy );
    else
        wxFAIL_MSG( "GenerateFabFiles(): unexpected request" );

    return 0;
}


int BOARD_EDITOR_CONTROL::RepairBoard( const TOOL_EVENT& aEvent )
{
    int      errors = 0;
    wxString details;

    /*******************************
     * Repair duplicate IDs and missing nets
     */

    std::set<KIID> ids;
    int            duplicates = 0;

    auto processItem =
            [&]( EDA_ITEM* aItem )
            {
                if( ids.count( aItem->m_Uuid ) )
                {
                    duplicates++;
                    const_cast<KIID&>( aItem->m_Uuid ) = KIID();
                }

                ids.insert( aItem->m_Uuid );

                BOARD_CONNECTED_ITEM* cItem = dynamic_cast<BOARD_CONNECTED_ITEM*>( aItem );

                if( cItem && cItem->GetNetCode() )
                {
                    NETINFO_ITEM* netinfo = cItem->GetNet();

                    if( netinfo && !board()->FindNet( netinfo->GetNetname() ) )
                    {
                        board()->Add( netinfo );

                        details += wxString::Format( _( "Orphaned net %s re-parented.\n" ),
                                                     netinfo->GetNetname() );
                        errors++;
                    }
                }
            };

    // Footprint IDs are the most important, so give them the first crack at "claiming" a
    // particular KIID.

    for( FOOTPRINT* footprint : board()->Footprints() )
        processItem( footprint );

    // After that the principal use is for DRC marker pointers, which are most likely to pads
    // or tracks.

    for( FOOTPRINT* footprint : board()->Footprints() )
    {
        for( PAD* pad : footprint->Pads() )
            processItem( pad );
    }

    for( TRACK* track : board()->Tracks() )
        processItem( track );

    // From here out I don't think order matters much.

    for( FOOTPRINT* footprint : board()->Footprints() )
    {
        processItem( &footprint->Reference() );
        processItem( &footprint->Value() );

        for( BOARD_ITEM* item : footprint->GraphicalItems() )
            processItem( item );

        for( ZONE* zone : footprint->Zones() )
            processItem( zone );

        for( PCB_GROUP* group : footprint->Groups() )
            processItem( group );
    }

    for( BOARD_ITEM* drawing : board()->Drawings() )
        processItem( drawing );

    for( ZONE* zone : board()->Zones() )
        processItem( zone );

    for( PCB_MARKER* marker : board()->Markers() )
        processItem( marker );

    for( PCB_GROUP* group : board()->Groups() )
        processItem( group );

    if( duplicates )
    {
        errors += duplicates;
        details += wxString::Format( _( "%d duplicate IDs replaced.\n" ), duplicates );
    }

    /*******************************
     * Your test here
     */

    /*******************************
     * Inform the user
     */

    if( errors )
    {
        m_frame->OnModify();

        wxString msg = wxString::Format( _( "%d potential problems repaired." ), errors );
        DisplayInfoMessage( m_frame, msg, details );
    }
    else
    {
        DisplayInfoMessage( m_frame, _( "No board problems found." ) );
    }

    return 0;
}


int BOARD_EDITOR_CONTROL::UpdatePCBFromSchematic( const TOOL_EVENT& aEvent )
{
    NETLIST netlist;

    if( m_frame->FetchNetlistFromSchematic( netlist, PCB_EDIT_FRAME::ANNOTATION_DIALOG ) )
    {
        DIALOG_UPDATE_PCB updateDialog( m_frame, &netlist );
        updateDialog.ShowModal();
    }

    return 0;
}

int BOARD_EDITOR_CONTROL::UpdateSchematicFromPCB( const TOOL_EVENT& aEvent )
{
    if( Kiface().IsSingle() )
    {
        DisplayErrorMessage(
                m_frame, _( "Cannot update schematic because Pcbnew is opened in stand-alone "
                            "mode. In order to create or update PCBs from schematics, you "
                            "must launch the KiCad project manager and create a project." ) );
        return 0;
    }

    m_frame->RunEeschema();
    KIWAY_PLAYER* frame = m_frame->Kiway().Player( FRAME_SCH, false );

    if( frame )
    {
        std::string payload;
        m_frame->Kiway().ExpressMail( FRAME_SCH, MAIL_SCH_UPDATE, payload, m_frame );
    }
    return 0;
}


int BOARD_EDITOR_CONTROL::ShowEeschema( const TOOL_EVENT& aEvent )
{
    m_frame->RunEeschema();
    return 0;
}


int BOARD_EDITOR_CONTROL::ToggleLayersManager( const TOOL_EVENT& aEvent )
{
    getEditFrame<PCB_EDIT_FRAME>()->ToggleLayersManager();
    return 0;
}


int BOARD_EDITOR_CONTROL::TogglePythonConsole( const TOOL_EVENT& aEvent )
{
#if defined( KICAD_SCRIPTING_WXPYTHON )
    m_frame->ScriptingConsoleEnableDisable();
#endif
    return 0;
}


// Track & via size control
int BOARD_EDITOR_CONTROL::TrackWidthInc( const TOOL_EVENT& aEvent )
{
    BOARD_DESIGN_SETTINGS& designSettings = getModel<BOARD>()->GetDesignSettings();
    constexpr KICAD_T      types[] = { PCB_TRACE_T, PCB_VIA_T, EOT };
    PCB_SELECTION&         selection = m_toolMgr->GetTool<PCB_SELECTION_TOOL>()->GetSelection();

    if( m_frame->ToolStackIsEmpty() && SELECTION_CONDITIONS::OnlyTypes( types )( selection ) )
    {
        BOARD_COMMIT commit( this );

        for( EDA_ITEM* item : selection )
        {
            if( item->Type() == PCB_TRACE_T )
            {
                TRACK* track = (TRACK*) item;

                for( int candidate : designSettings.m_TrackWidthList )
                {
                    if( candidate > track->GetWidth() )
                    {
                        commit.Modify( track );
                        track->SetWidth( candidate );
                        break;
                    }
                }
            }
        }

        commit.Push( "Increase Track Width" );
        return 0;
    }

    ROUTER_TOOL* routerTool = m_toolMgr->GetTool<ROUTER_TOOL>();

    if( routerTool && routerTool->IsToolActive()
            && routerTool->Router()->Mode() == PNS::PNS_MODE_ROUTE_DIFF_PAIR )
    {
        int widthIndex = designSettings.GetDiffPairIndex() + 1;

        // If we go past the last track width entry in the list, start over at the beginning
        if( widthIndex >= (int) designSettings.m_DiffPairDimensionsList.size() )
            widthIndex = 0;

        designSettings.SetDiffPairIndex( widthIndex );
        designSettings.UseCustomDiffPairDimensions( false );

        m_toolMgr->RunAction( PCB_ACTIONS::trackViaSizeChanged, true );
    }
    else
    {
        int widthIndex = designSettings.GetTrackWidthIndex() + 1;

        // If we go past the last track width entry in the list, start over at the beginning
        if( widthIndex >= (int) designSettings.m_TrackWidthList.size() )
            widthIndex = 0;

        designSettings.SetTrackWidthIndex( widthIndex );
        designSettings.UseCustomTrackViaSize( false );

        m_toolMgr->RunAction( PCB_ACTIONS::trackViaSizeChanged, true );
    }

    return 0;
}


int BOARD_EDITOR_CONTROL::TrackWidthDec( const TOOL_EVENT& aEvent )
{
    BOARD_DESIGN_SETTINGS& designSettings = getModel<BOARD>()->GetDesignSettings();
    constexpr KICAD_T      types[] = { PCB_TRACE_T, PCB_VIA_T, EOT };
    PCB_SELECTION&         selection = m_toolMgr->GetTool<PCB_SELECTION_TOOL>()->GetSelection();

    if( m_frame->ToolStackIsEmpty() && SELECTION_CONDITIONS::OnlyTypes( types )( selection ) )
    {
        BOARD_COMMIT commit( this );

        for( EDA_ITEM* item : selection )
        {
            if( item->Type() == PCB_TRACE_T )
            {
                TRACK* track = (TRACK*) item;

                for( int i = designSettings.m_TrackWidthList.size() - 1; i >= 0; --i )
                {
                    int candidate = designSettings.m_TrackWidthList[ i ];

                    if( candidate < track->GetWidth() )
                    {
                        commit.Modify( track );
                        track->SetWidth( candidate );
                        break;
                    }
                }
            }
        }

        commit.Push( "Decrease Track Width" );
        return 0;
    }

    ROUTER_TOOL* routerTool = m_toolMgr->GetTool<ROUTER_TOOL>();

    if( routerTool && routerTool->IsToolActive()
            && routerTool->Router()->Mode() == PNS::PNS_MODE_ROUTE_DIFF_PAIR )
    {
        int widthIndex = designSettings.GetDiffPairIndex() - 1;

        // If we get to the lowest entry start over at the highest
        if( widthIndex < 0 )
            widthIndex = designSettings.m_DiffPairDimensionsList.size() - 1;

        designSettings.SetDiffPairIndex( widthIndex );
        designSettings.UseCustomDiffPairDimensions( false );

        m_toolMgr->RunAction( PCB_ACTIONS::trackViaSizeChanged, true );
    }
    else
    {
        int widthIndex = designSettings.GetTrackWidthIndex() - 1;

        // If we get to the lowest entry start over at the highest
        if( widthIndex < 0 )
            widthIndex = designSettings.m_TrackWidthList.size() - 1;

        designSettings.SetTrackWidthIndex( widthIndex );
        designSettings.UseCustomTrackViaSize( false );

        m_toolMgr->RunAction( PCB_ACTIONS::trackViaSizeChanged, true );
    }

    return 0;
}


int BOARD_EDITOR_CONTROL::ViaSizeInc( const TOOL_EVENT& aEvent )
{
    BOARD_DESIGN_SETTINGS& designSettings = getModel<BOARD>()->GetDesignSettings();
    constexpr KICAD_T      types[] = { PCB_TRACE_T, PCB_VIA_T, EOT };
    PCB_SELECTION&         selection = m_toolMgr->GetTool<PCB_SELECTION_TOOL>()->GetSelection();

    if( m_frame->ToolStackIsEmpty() && SELECTION_CONDITIONS::OnlyTypes( types )( selection ) )
    {
        BOARD_COMMIT commit( this );

        for( EDA_ITEM* item : selection )
        {
            if( item->Type() == PCB_VIA_T )
            {
                VIA* via = (VIA*) item;

                for( VIA_DIMENSION candidate : designSettings.m_ViasDimensionsList )
                {
                    if( candidate.m_Diameter > via->GetWidth() )
                    {
                        commit.Modify( via );
                        via->SetWidth( candidate.m_Diameter );
                        via->SetDrill( candidate.m_Drill );
                        break;
                    }
                }
            }
        }

        commit.Push( "Increase Via Size" );
    }
    else
    {
        int sizeIndex = designSettings.GetViaSizeIndex() + 1;

        // If we go past the last via entry in the list, start over at the beginning
        if( sizeIndex >= (int) designSettings.m_ViasDimensionsList.size() )
            sizeIndex = 0;

        designSettings.SetViaSizeIndex( sizeIndex );
        designSettings.UseCustomTrackViaSize( false );

        m_toolMgr->RunAction( PCB_ACTIONS::trackViaSizeChanged, true );
    }

    return 0;
}


int BOARD_EDITOR_CONTROL::ViaSizeDec( const TOOL_EVENT& aEvent )
{
    BOARD_DESIGN_SETTINGS& designSettings = getModel<BOARD>()->GetDesignSettings();
    constexpr KICAD_T      types[] = { PCB_TRACE_T, PCB_VIA_T, EOT };
    PCB_SELECTION&         selection = m_toolMgr->GetTool<PCB_SELECTION_TOOL>()->GetSelection();

    if( m_frame->ToolStackIsEmpty() && SELECTION_CONDITIONS::OnlyTypes( types )( selection ) )
    {
        BOARD_COMMIT commit( this );

        for( EDA_ITEM* item : selection )
        {
            if( item->Type() == PCB_VIA_T )
            {
                VIA* via = (VIA*) item;

                for( int i = designSettings.m_ViasDimensionsList.size() - 1; i >= 0; --i )
                {
                    VIA_DIMENSION candidate = designSettings.m_ViasDimensionsList[ i ];

                    if( candidate.m_Diameter < via->GetWidth() )
                    {
                        commit.Modify( via );
                        via->SetWidth( candidate.m_Diameter );
                        via->SetDrill( candidate.m_Drill );
                        break;
                    }
                }
            }
        }

        commit.Push( "Decrease Via Size" );
    }
    else
    {
        int sizeIndex = 0; // Assume we only have a single via size entry

        // If there are more, cycle through them backwards
        if( designSettings.m_ViasDimensionsList.size() > 0 )
        {
            sizeIndex = designSettings.GetViaSizeIndex() - 1;

            // If we get to the lowest entry start over at the highest
            if( sizeIndex < 0 )
                sizeIndex = designSettings.m_ViasDimensionsList.size() - 1;
        }

        designSettings.SetViaSizeIndex( sizeIndex );
        designSettings.UseCustomTrackViaSize( false );

        m_toolMgr->RunAction( PCB_ACTIONS::trackViaSizeChanged, true );
    }

    return 0;
}


int BOARD_EDITOR_CONTROL::PlaceModule( const TOOL_EVENT& aEvent )
{
    FOOTPRINT*            fp = aEvent.Parameter<FOOTPRINT*>();
    KIGFX::VIEW_CONTROLS* controls = getViewControls();
    BOARD_COMMIT          commit( m_frame );
    BOARD*                board = getModel<BOARD>();

    m_toolMgr->RunAction( PCB_ACTIONS::selectionClear, true );
    controls->ShowCursor( true );

    std::string tool = aEvent.GetCommandStr().get();
    m_frame->PushTool( tool );
    Activate();

    VECTOR2I cursorPos = controls->GetCursorPosition();
    bool     reselect = false;
    bool     fromOtherCommand = fp != nullptr;

    // Prime the pump
    if( fp )
    {
        fp->SetPosition( wxPoint( cursorPos.x, cursorPos.y ) );
        m_toolMgr->RunAction( PCB_ACTIONS::selectItem, true, fp );
        m_toolMgr->RunAction( ACTIONS::refreshPreview );
    }
    else if( aEvent.HasPosition() )
        m_toolMgr->RunAction( PCB_ACTIONS::cursorClick );

    auto setCursor =
            [&]()
            {
                m_frame->GetCanvas()->SetCurrentCursor( KICURSOR::PENCIL );
            };

    // Set initial cursor
    setCursor();

    // Main loop: keep receiving events
    while( TOOL_EVENT* evt = Wait() )
    {
        setCursor();
        cursorPos = controls->GetCursorPosition( !evt->Modifier( MD_ALT ) );

        if( reselect && fp )
            m_toolMgr->RunAction( PCB_ACTIONS::selectItem, true, fp );

        auto cleanup =
                [&] ()
                {
                    m_toolMgr->RunAction( PCB_ACTIONS::selectionClear, true );
                    commit.Revert();

                    if( fromOtherCommand )
                    {
                        PICKED_ITEMS_LIST* undo = m_frame->PopCommandFromUndoList();

                        if( undo )
                        {
                            m_frame->PutDataInPreviousState( undo, false );
                            undo->ClearListAndDeleteItems();
                            delete undo;
                        }
                    }

                    fp = NULL;
                };

        if( evt->IsCancelInteractive() )
        {
            if( fp )
                cleanup();
            else
            {
                m_frame->PopTool( tool );
                break;
            }
        }
        else if( evt->IsActivate() )
        {
            if( fp )
                cleanup();

            if( evt->IsMoveTool() )
            {
                // leave ourselves on the stack so we come back after the move
                break;
            }
            else
            {
                frame()->PopTool( tool );
                break;
            }
        }
        else if( evt->IsClick( BUT_LEFT ) )
        {
            if( !fp )
            {
                // Pick the footprint to be placed
                fp = m_frame->SelectFootprintFromLibTree();

                if( fp == NULL )
                    continue;

                fp->SetLink( niluuid );

                fp->SetFlags(IS_NEW ); // whatever

                // Set parent so that clearance can be loaded
                fp->SetParent( board );

                // Pads in the library all have orphaned nets.  Replace with Default.
                for( PAD* pad : fp->Pads() )
                {
                    pad->SetLocked( !m_frame->Settings().m_AddUnlockedPads );
                    pad->SetNetCode( 0 );
                }

                // Put it on FRONT layer,
                // (Can be stored flipped if the lib is an archive built from a board)
                if( fp->IsFlipped() )
                    fp->Flip( fp->GetPosition(), m_frame->Settings().m_FlipLeftRight );

                fp->SetOrientation( 0 );
                fp->SetPosition( wxPoint( cursorPos.x, cursorPos.y ) );

                commit.Add( fp );
                m_toolMgr->RunAction( PCB_ACTIONS::selectItem, true, fp );
                controls->SetCursorPosition( cursorPos, false );
            }
            else
            {
                m_toolMgr->RunAction( PCB_ACTIONS::selectionClear, true );
                commit.Push( _( "Place a footprint" ) );
                fp = NULL;  // to indicate that there is no footprint that we currently modify
            }
        }
        else if( evt->IsClick( BUT_RIGHT ) )
        {
            m_menu.ShowContextMenu(  selection()  );
        }
        else if( fp && ( evt->IsMotion() || evt->IsAction( &ACTIONS::refreshPreview ) ) )
        {
            fp->SetPosition( wxPoint( cursorPos.x, cursorPos.y ) );
            selection().SetReferencePoint( cursorPos );
            getView()->Update( &selection() );
            getView()->Update( fp );
        }
        else if( fp && evt->IsAction( &PCB_ACTIONS::properties ) )
        {
            // Calling 'Properties' action clears the selection, so we need to restore it
            reselect = true;
        }
        else
        {
            evt->SetPassEvent();
        }

        // Enable autopanning and cursor capture only when there is a footprint to be placed
        controls->SetAutoPan( !!fp );
        controls->CaptureCursor( !!fp );
    }

    m_frame->GetCanvas()->SetCurrentCursor( KICURSOR::ARROW );
    return 0;
}


int BOARD_EDITOR_CONTROL::ToggleLockSelected( const TOOL_EVENT& aEvent )
{
    return modifyLockSelected( TOGGLE );
}


int BOARD_EDITOR_CONTROL::LockSelected( const TOOL_EVENT& aEvent )
{
    return modifyLockSelected( ON );
}


int BOARD_EDITOR_CONTROL::UnlockSelected( const TOOL_EVENT& aEvent )
{
    return modifyLockSelected( OFF );
}


int BOARD_EDITOR_CONTROL::modifyLockSelected( MODIFY_MODE aMode )
{
    PCB_SELECTION_TOOL*  selTool = m_toolMgr->GetTool<PCB_SELECTION_TOOL>();
    const PCB_SELECTION& selection = selTool->GetSelection();
    BOARD_COMMIT         commit( m_frame );

    if( selection.Empty() )
        m_toolMgr->RunAction( PCB_ACTIONS::selectionCursor, true );

    // Resolve TOGGLE mode
    if( aMode == TOGGLE )
    {
        aMode = ON;

        for( EDA_ITEM* item : selection )
        {
            BOARD_ITEM* board_item = static_cast<BOARD_ITEM*>( item );

            if( board_item->IsLocked() )
            {
                aMode = OFF;
                break;
            }
        }
    }

    bool modified = false;

    for( EDA_ITEM* item : selection )
    {
        BOARD_ITEM* board_item = static_cast<BOARD_ITEM*>( item );

        commit.Modify( board_item );

        if( aMode == ON )
        {
            modified |= !board_item->IsLocked();
            board_item->SetLocked( true );
        }
        else
        {
            modified |= board_item->IsLocked();
            board_item->SetLocked( false );
        }
    }

    if( modified )
    {
        commit.Push( aMode == ON ? _( "Lock" ) : _( "Unlock" ) );
        m_toolMgr->PostEvent( EVENTS::SelectedItemsModified );
        m_frame->OnModify();
    }

    return 0;
}


int BOARD_EDITOR_CONTROL::PlaceTarget( const TOOL_EVENT& aEvent )
{
    KIGFX::VIEW* view = getView();
    KIGFX::VIEW_CONTROLS* controls = getViewControls();
    BOARD* board = getModel<BOARD>();
    PCB_TARGET* target = new PCB_TARGET( board );

    // Init the new item attributes
    target->SetLayer( Edge_Cuts );
    target->SetWidth( board->GetDesignSettings().GetLineThickness( Edge_Cuts ) );
    target->SetSize( Millimeter2iu( 5 ) );
    VECTOR2I cursorPos = controls->GetCursorPosition();
    target->SetPosition( wxPoint( cursorPos.x, cursorPos.y ) );

    // Add a VIEW_GROUP that serves as a preview for the new item
    KIGFX::VIEW_GROUP preview( view );
    preview.Add( target );
    view->Add( &preview );

    m_toolMgr->RunAction( PCB_ACTIONS::selectionClear, true );

    std::string tool = aEvent.GetCommandStr().get();
    m_frame->PushTool( tool );
    Activate();

    auto setCursor =
            [&]()
            {
                m_frame->GetCanvas()->SetCurrentCursor( KICURSOR::ARROW );
            };

    // Set initial cursor
    setCursor();

    // Main loop: keep receiving events
    while( TOOL_EVENT* evt = Wait() )
    {
        setCursor();
        cursorPos = controls->GetCursorPosition( !evt->Modifier( MD_ALT ) );

        if( evt->IsCancelInteractive() )
        {
            frame()->PopTool( tool );
            break;
        }
        else if( evt->IsActivate() )
        {
            if( evt->IsMoveTool() )
            {
                // leave ourselves on the stack so we come back after the move
                break;
            }
            else
            {
                frame()->PopTool( tool );
                break;
            }
        }
        else if( evt->IsAction( &PCB_ACTIONS::incWidth ) )
        {
            target->SetWidth( target->GetWidth() + WIDTH_STEP );
            view->Update( &preview );
        }
        else if( evt->IsAction( &PCB_ACTIONS::decWidth ) )
        {
            int width = target->GetWidth();

            if( width > WIDTH_STEP )
            {
                target->SetWidth( width - WIDTH_STEP );
                view->Update( &preview );
            }
        }
        else if( evt->IsClick( BUT_LEFT ) )
        {
            assert( target->GetSize() > 0 );
            assert( target->GetWidth() > 0 );

            BOARD_COMMIT commit( m_frame );
            commit.Add( target );
            commit.Push( "Place a layer alignment target" );

            preview.Remove( target );

            // Create next PCB_TARGET
            target = new PCB_TARGET( *target );
            preview.Add( target );
        }
        else if( evt->IsClick( BUT_RIGHT ) )
        {
            m_menu.ShowContextMenu( selection() );
        }
        else if( evt->IsMotion() )
        {
            target->SetPosition( wxPoint( cursorPos.x, cursorPos.y ) );
            view->Update( &preview );
        }
        else
        {
            evt->SetPassEvent();
        }
    }

    preview.Clear();
    delete target;
    view->Remove( &preview );

    m_frame->GetCanvas()->SetCurrentCursor( KICURSOR::ARROW );
    return 0;
}


static bool mergeZones( BOARD_COMMIT& aCommit, std::vector<ZONE*>& aOriginZones,
                        std::vector<ZONE*>& aMergedZones )
{
    for( unsigned int i = 1; i < aOriginZones.size(); i++ )
    {
        aOriginZones[0]->Outline()->BooleanAdd( *aOriginZones[i]->Outline(),
                                                SHAPE_POLY_SET::PM_FAST );
    }

    aOriginZones[0]->Outline()->Simplify( SHAPE_POLY_SET::PM_FAST );

    // We should have one polygon with hole
    // We can have 2 polygons with hole, if the 2 initial polygons have only one common corner
    // and therefore cannot be merged (they are dectected as intersecting)
    // but we should never have more than 2 polys
    if( aOriginZones[0]->Outline()->OutlineCount() > 1 )
    {
        wxLogMessage( "BOARD::mergeZones error: more than 2 polys after merging" );
        return false;
    }

    for( unsigned int i = 1; i < aOriginZones.size(); i++ )
    {
        aCommit.Remove( aOriginZones[i] );
    }

    aCommit.Modify( aOriginZones[0] );
    aMergedZones.push_back( aOriginZones[0] );

    aOriginZones[0]->SetLocalFlags( 1 );
    aOriginZones[0]->HatchBorder();
    aOriginZones[0]->CacheTriangulation();

    return true;
}


int BOARD_EDITOR_CONTROL::ZoneMerge( const TOOL_EVENT& aEvent )
{
    const PCB_SELECTION& selection = m_toolMgr->GetTool<PCB_SELECTION_TOOL>()->GetSelection();
    BOARD*               board = getModel<BOARD>();
    BOARD_COMMIT         commit( m_frame );

    if( selection.Size() < 2 )
        return 0;

    int netcode = -1;

    ZONE* firstZone = nullptr;
    std::vector<ZONE*> toMerge, merged;

    for( auto item : selection )
    {
        ZONE* curr_area = dynamic_cast<ZONE*>( item );

        if( !curr_area )
            continue;

        if( !firstZone )
            firstZone = curr_area;

        netcode = curr_area->GetNetCode();

        if( firstZone->GetNetCode() != netcode )
            continue;

        if( curr_area->GetPriority() != firstZone->GetPriority() )
            continue;

        if( curr_area->GetIsRuleArea() != firstZone->GetIsRuleArea() )
            continue;

        if( curr_area->GetLayer() != firstZone->GetLayer() )
            continue;

        if( !board->TestZoneIntersection( curr_area, firstZone ) )
            continue;

        toMerge.push_back( curr_area );
    }

    m_toolMgr->RunAction( PCB_ACTIONS::selectionClear, true );

    if( mergeZones( commit, toMerge, merged ) )
    {
        commit.Push( "Merge zones" );

        for( auto item : merged )
            m_toolMgr->RunAction( PCB_ACTIONS::selectItem, true, item );
    }

    return 0;
}


int BOARD_EDITOR_CONTROL::ZoneDuplicate( const TOOL_EVENT& aEvent )
{
    PCB_SELECTION_TOOL*  selTool = m_toolMgr->GetTool<PCB_SELECTION_TOOL>();
    const PCB_SELECTION& selection = selTool->GetSelection();

    // because this pops up the zone editor, it would be confusing to handle multiple zones,
    // so just handle single selections containing exactly one zone
    if( selection.Size() != 1 )
        return 0;

    ZONE* oldZone = dyn_cast<ZONE*>( selection[0] );

    if( !oldZone )
        return 0;

    ZONE_SETTINGS zoneSettings;
    zoneSettings << *oldZone;
    int dialogResult;

    if( oldZone->GetIsRuleArea() )
        dialogResult = InvokeRuleAreaEditor( m_frame, &zoneSettings );
    else if( oldZone->IsOnCopperLayer() )
        dialogResult = InvokeCopperZonesEditor( m_frame, &zoneSettings );
    else
        dialogResult = InvokeNonCopperZonesEditor( m_frame, &zoneSettings );

    if( dialogResult != wxID_OK )
        return 0;

    // duplicate the zone
    BOARD_COMMIT commit( m_frame );

    std::unique_ptr<ZONE> newZone = std::make_unique<ZONE>( *oldZone );
    newZone->ClearSelected();
    newZone->UnFill();
    zoneSettings.ExportSetting( *newZone );

    // If the new zone is on the same layer(s) as the the initial zone,
    // offset it a bit so it can more easily be picked.
    if( oldZone->GetIsRuleArea() && ( oldZone->GetLayerSet() == zoneSettings.m_Layers ) )
        newZone->Move( wxPoint( IU_PER_MM, IU_PER_MM ) );
    else if( !oldZone->GetIsRuleArea() && zoneSettings.m_Layers.test( oldZone->GetLayer() ) )
        newZone->Move( wxPoint( IU_PER_MM, IU_PER_MM ) );

    commit.Add( newZone.release() );
    commit.Push( _( "Duplicate zone" ) );

    return 0;
}


int BOARD_EDITOR_CONTROL::EditFpInFpEditor( const TOOL_EVENT& aEvent )
{
    PCB_SELECTION_TOOL*  selTool = m_toolMgr->GetTool<PCB_SELECTION_TOOL>();
    const PCB_SELECTION& selection = selTool->RequestSelection( EDIT_TOOL::FootprintFilter );

    if( selection.Empty() )
        return 0;

    FOOTPRINT* fp = selection.FirstOfKind<FOOTPRINT>();

    if( !fp )
        return 0;

    PCB_BASE_EDIT_FRAME* editFrame = getEditFrame<PCB_BASE_EDIT_FRAME>();

    auto editor = (FOOTPRINT_EDIT_FRAME*) editFrame->Kiway().Player( FRAME_FOOTPRINT_EDITOR, true );

    editor->LoadFootprintFromBoard( fp );

    editor->Show( true );
    editor->Raise();        // Iconize( false );

    if( selection.IsHover() )
        m_toolMgr->RunAction( PCB_ACTIONS::selectionClear, true );

    return 0;
}


void BOARD_EDITOR_CONTROL::DoSetDrillOrigin( KIGFX::VIEW* aView, PCB_BASE_FRAME* aFrame,
                                             EDA_ITEM* originViewItem, const VECTOR2D& aPosition )
{
    aFrame->GetDesignSettings().m_AuxOrigin = (wxPoint) aPosition;
    originViewItem->SetPosition( (wxPoint) aPosition );
    aView->MarkDirty();
    aFrame->OnModify();
}


int BOARD_EDITOR_CONTROL::DrillOrigin( const TOOL_EVENT& aEvent )
{
    std::string      tool = aEvent.GetCommandStr().get();
    PCB_PICKER_TOOL* picker = m_toolMgr->GetTool<PCB_PICKER_TOOL>();

    // Deactivate other tools; particularly important if another PICKER is currently running
    Activate();

    picker->SetClickHandler(
        [this] ( const VECTOR2D& pt ) -> bool
        {
            m_frame->SaveCopyInUndoList( m_placeOrigin.get(), UNDO_REDO::DRILLORIGIN );
            DoSetDrillOrigin( getView(), m_frame, m_placeOrigin.get(), pt );
            return false;   // drill origin is a one-shot; don't continue with tool
        } );

    m_toolMgr->RunAction( ACTIONS::pickerTool, true, &tool );

    return 0;
}


int BOARD_EDITOR_CONTROL::FlipPcbView( const TOOL_EVENT& aEvent )
{
    view()->SetMirror( !view()->IsMirroredX(), false );
    view()->RecacheAllItems();
    frame()->GetCanvas()->ForceRefresh();
    frame()->OnDisplayOptionsChanged();
    return 0;
}


void BOARD_EDITOR_CONTROL::setTransitions()
{
    Go( &BOARD_EDITOR_CONTROL::New,                    ACTIONS::doNew.MakeEvent() );
    Go( &BOARD_EDITOR_CONTROL::Open,                   ACTIONS::open.MakeEvent() );
    Go( &BOARD_EDITOR_CONTROL::Save,                   ACTIONS::save.MakeEvent() );
    Go( &BOARD_EDITOR_CONTROL::SaveAs,                 ACTIONS::saveAs.MakeEvent() );
    Go( &BOARD_EDITOR_CONTROL::SaveCopyAs,             ACTIONS::saveCopyAs.MakeEvent() );
    Go( &BOARD_EDITOR_CONTROL::PageSettings,           ACTIONS::pageSettings.MakeEvent() );
    Go( &BOARD_EDITOR_CONTROL::Plot,                   ACTIONS::plot.MakeEvent() );

    Go( &BOARD_EDITOR_CONTROL::BoardSetup,             PCB_ACTIONS::boardSetup.MakeEvent() );
    Go( &BOARD_EDITOR_CONTROL::ImportNetlist,          PCB_ACTIONS::importNetlist.MakeEvent() );
    Go( &BOARD_EDITOR_CONTROL::ImportSpecctraSession,  PCB_ACTIONS::importSpecctraSession.MakeEvent() );
    Go( &BOARD_EDITOR_CONTROL::ExportSpecctraDSN,      PCB_ACTIONS::exportSpecctraDSN.MakeEvent() );
    Go( &BOARD_EDITOR_CONTROL::GenerateDrillFiles,     PCB_ACTIONS::generateDrillFiles.MakeEvent() );
    Go( &BOARD_EDITOR_CONTROL::GenerateFabFiles,       PCB_ACTIONS::generateGerbers.MakeEvent() );
    Go( &BOARD_EDITOR_CONTROL::GeneratePosFile,        PCB_ACTIONS::generatePosFile.MakeEvent() );
    Go( &BOARD_EDITOR_CONTROL::GenerateFabFiles,       PCB_ACTIONS::generateReportFile.MakeEvent() );
    Go( &BOARD_EDITOR_CONTROL::GenerateFabFiles,       PCB_ACTIONS::generateD356File.MakeEvent() );
    Go( &BOARD_EDITOR_CONTROL::GenerateFabFiles,       PCB_ACTIONS::generateBOM.MakeEvent() );

    // Track & via size control
    Go( &BOARD_EDITOR_CONTROL::TrackWidthInc,          PCB_ACTIONS::trackWidthInc.MakeEvent() );
    Go( &BOARD_EDITOR_CONTROL::TrackWidthDec,          PCB_ACTIONS::trackWidthDec.MakeEvent() );
    Go( &BOARD_EDITOR_CONTROL::ViaSizeInc,             PCB_ACTIONS::viaSizeInc.MakeEvent() );
    Go( &BOARD_EDITOR_CONTROL::ViaSizeDec,             PCB_ACTIONS::viaSizeDec.MakeEvent() );

    // Zone actions
    Go( &BOARD_EDITOR_CONTROL::ZoneMerge,              PCB_ACTIONS::zoneMerge.MakeEvent() );
    Go( &BOARD_EDITOR_CONTROL::ZoneDuplicate,          PCB_ACTIONS::zoneDuplicate.MakeEvent() );

    // Placing tools
    Go( &BOARD_EDITOR_CONTROL::PlaceTarget,            PCB_ACTIONS::placeTarget.MakeEvent() );
    Go( &BOARD_EDITOR_CONTROL::PlaceModule,            PCB_ACTIONS::placeModule.MakeEvent() );
    Go( &BOARD_EDITOR_CONTROL::DrillOrigin,            PCB_ACTIONS::drillOrigin.MakeEvent() );

    Go( &BOARD_EDITOR_CONTROL::EditFpInFpEditor,       PCB_ACTIONS::editFpInFpEditor.MakeEvent() );

    // Other
    Go( &BOARD_EDITOR_CONTROL::ToggleLockSelected,     PCB_ACTIONS::toggleLock.MakeEvent() );
    Go( &BOARD_EDITOR_CONTROL::LockSelected,           PCB_ACTIONS::lock.MakeEvent() );
    Go( &BOARD_EDITOR_CONTROL::UnlockSelected,         PCB_ACTIONS::unlock.MakeEvent() );

    Go( &BOARD_EDITOR_CONTROL::UpdatePCBFromSchematic, ACTIONS::updatePcbFromSchematic.MakeEvent() );
    Go( &BOARD_EDITOR_CONTROL::UpdateSchematicFromPCB, ACTIONS::updateSchematicFromPcb.MakeEvent() );
    Go( &BOARD_EDITOR_CONTROL::ShowEeschema,           PCB_ACTIONS::showEeschema.MakeEvent() );
    Go( &BOARD_EDITOR_CONTROL::ToggleLayersManager,    PCB_ACTIONS::showLayersManager.MakeEvent() );
    Go( &BOARD_EDITOR_CONTROL::TogglePythonConsole,    PCB_ACTIONS::showPythonConsole.MakeEvent() );
    Go( &BOARD_EDITOR_CONTROL::FlipPcbView,            PCB_ACTIONS::flipBoard.MakeEvent() );
    Go( &BOARD_EDITOR_CONTROL::RepairBoard,            PCB_ACTIONS::repairBoard.MakeEvent() );
}


const int BOARD_EDITOR_CONTROL::WIDTH_STEP = 100000;
