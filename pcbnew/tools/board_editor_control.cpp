/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2014 CERN
 * Copyright (C) 2014-2022 KiCad Developers, see AUTHORS.txt for contributors.
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

#include <functional>
#include <memory>

#include <pgm_base.h>
#include <advanced_config.h>
#include "board_editor_control.h"
#include <bitmaps.h>
#include <pcb_painter.h>
#include <board.h>
#include <board_commit.h>
#include <board_design_settings.h>
#include <pcb_group.h>
#include <footprint.h>
#include <pad.h>
#include <pcb_target.h>
#include <pcb_track.h>
#include <zone.h>
#include <pcb_marker.h>
#include <confirm.h>
#include <dialogs/dialog_page_settings.h>
#include <dialogs/dialog_update_pcb.h>
#include <dialogs/dialog_assign_netclass.h>
#include <kiface_base.h>
#include <kiway.h>
#include <netlist_reader/pcb_netlist.h>
#include <origin_viewitem.h>
#include <pcb_edit_frame.h>
#include <pcbnew_id.h>
#include <project.h>
#include <project/project_file.h> // LAST_PATH_TYPE
#include <tool/tool_manager.h>
#include <tool/tool_event.h>
#include <tools/drawing_tool.h>
#include <tools/pcb_actions.h>
#include <tools/pcb_picker_tool.h>
#include <tools/pcb_selection_conditions.h>
#include <tools/pcb_selection_tool.h>
#include <tools/edit_tool.h>
#include <tools/tool_event_utils.h>
#include <tools/zone_filler_tool.h>
#include <router/router_tool.h>
#include <view/view_controls.h>
#include <view/view_group.h>
#include <wildcards_and_files_ext.h>
#include <drawing_sheet/ds_proxy_undo_item.h>
#include <footprint_edit_frame.h>
#include <wx/filedlg.h>
#include <wx/log.h>

#include <widgets/legacyfiledlg_netlist_options.h>

using namespace std::placeholders;


class ZONE_CONTEXT_MENU : public ACTION_MENU
{
public:
    ZONE_CONTEXT_MENU() :
        ACTION_MENU( true )
    {
        SetIcon( BITMAPS::add_zone );
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


class LOCK_CONTEXT_MENU : public CONDITIONAL_MENU
{
public:
    LOCK_CONTEXT_MENU( TOOL_INTERACTIVE* aTool ) :
        CONDITIONAL_MENU( aTool )
    {
        SetIcon( BITMAPS::locked );
        SetTitle( _( "Locking" ) );

        AddItem( PCB_ACTIONS::lock, PCB_SELECTION_CONDITIONS::HasUnlockedItems );
        AddItem( PCB_ACTIONS::unlock, PCB_SELECTION_CONDITIONS::HasLockedItems );
        AddItem( PCB_ACTIONS::toggleLock, SELECTION_CONDITIONS::ShowAlways );
    }

    ACTION_MENU* create() const override
    {
        return new LOCK_CONTEXT_MENU( this->m_tool );
    }
};


BOARD_EDITOR_CONTROL::BOARD_EDITOR_CONTROL() :
    PCB_TOOL_BASE( "pcbnew.EditorControl" ),
    m_frame( nullptr ),
    m_inPlaceFootprint( false ),
    m_placingFootprint( false ),
    m_inPlaceTarget( false )
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

    if( aReason == MODEL_RELOAD || aReason == GAL_SWITCH || aReason == REDRAW )
    {
        m_placeOrigin->SetPosition( getModel<BOARD>()->GetDesignSettings().GetAuxOrigin() );
        getView()->Remove( m_placeOrigin.get() );
        getView()->Add( m_placeOrigin.get() );
    }
}


bool BOARD_EDITOR_CONTROL::Init()
{
    auto activeToolCondition =
            [this]( const SELECTION& aSel )
            {
                return ( !m_frame->ToolStackIsEmpty() );
            };

    auto inactiveStateCondition =
            [this]( const SELECTION& aSel )
            {
                return ( m_frame->ToolStackIsEmpty() && aSel.Size() == 0 );
            };

    auto placeModuleCondition =
            [this]( const SELECTION& aSel )
            {
                return m_frame->IsCurrentTool( PCB_ACTIONS::placeFootprint ) && aSel.GetSize() == 0;
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

    std::shared_ptr<ZONE_CONTEXT_MENU> zoneMenu = std::make_shared<ZONE_CONTEXT_MENU>();
    zoneMenu->SetTool( this );

    std::shared_ptr<LOCK_CONTEXT_MENU> lockMenu = std::make_shared<LOCK_CONTEXT_MENU>( this );

    // Add the PCB control menus to relevant other tools

    PCB_SELECTION_TOOL* selTool = m_toolMgr->GetTool<PCB_SELECTION_TOOL>();

    if( selTool )
    {
        TOOL_MENU&        toolMenu = selTool->GetToolMenu();
        CONDITIONAL_MENU& menu = toolMenu.GetMenu();

        // Add "Get and Place Footprint" when Selection tool is in an inactive state
        menu.AddItem( PCB_ACTIONS::getAndPlace, inactiveStateCondition );
        menu.AddSeparator();

        toolMenu.RegisterSubMenu( zoneMenu );
        toolMenu.RegisterSubMenu( lockMenu );

        menu.AddMenu( lockMenu.get(), SELECTION_CONDITIONS::NotEmpty, 100 );

        menu.AddMenu( zoneMenu.get(), SELECTION_CONDITIONS::OnlyTypes( { PCB_ZONE_T } ), 100 );
    }

    DRAWING_TOOL* drawingTool = m_toolMgr->GetTool<DRAWING_TOOL>();

    if( drawingTool )
    {
        TOOL_MENU&        toolMenu = drawingTool->GetToolMenu();
        CONDITIONAL_MENU& menu = toolMenu.GetMenu();

        toolMenu.RegisterSubMenu( zoneMenu );

        // Functor to say if the PCB_EDIT_FRAME is in a given mode
        // Capture the tool pointer and tool mode by value
        auto toolActiveFunctor =
                [=]( DRAWING_TOOL::MODE aMode )
                {
                    return [=]( const SELECTION& sel )
                           {
                               return drawingTool->GetDrawingMode() == aMode;
                           };
                };

        menu.AddMenu( zoneMenu.get(), toolActiveFunctor( DRAWING_TOOL::MODE::ZONE ), 300 );
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


int BOARD_EDITOR_CONTROL::SaveCopy( const TOOL_EVENT& aEvent )
{
    m_frame->Files_io_from_id( ID_COPY_BOARD_AS );
    return 0;
}


int BOARD_EDITOR_CONTROL::Revert( const TOOL_EVENT& aEvent )
{
    m_frame->Files_io_from_id( ID_REVERT_BOARD );
    return 0;
}


int BOARD_EDITOR_CONTROL::PageSettings( const TOOL_EVENT& aEvent )
{
    PICKED_ITEMS_LIST   undoCmd;
    DS_PROXY_UNDO_ITEM* undoItem = new DS_PROXY_UNDO_ITEM( m_frame );
    ITEM_PICKER         wrapper( nullptr, undoItem, UNDO_REDO::PAGESETTINGS );

    undoCmd.PushItem( wrapper );
    m_frame->SaveCopyInUndoList( undoCmd, UNDO_REDO::PAGESETTINGS );

    DIALOG_PAGES_SETTINGS dlg( m_frame, pcbIUScale.IU_PER_MILS, VECTOR2I( MAX_PAGE_SIZE_PCBNEW_MILS,
                                                                          MAX_PAGE_SIZE_PCBNEW_MILS ) );
    dlg.SetWksFileName( BASE_SCREEN::m_DrawingSheetFileName );

    if( dlg.ShowModal() == wxID_OK )
    {
        m_frame->GetCanvas()->GetView()->UpdateAllItemsConditionally(
                [&]( KIGFX::VIEW_ITEM* aItem ) -> int
                {
                    EDA_TEXT* text = dynamic_cast<EDA_TEXT*>( aItem );

                    if( text && text->HasTextVars() )
                    {
                        text->ClearRenderCache();
                        text->ClearBoundingBoxCache();
                        return KIGFX::GEOMETRY | KIGFX::REPAINT;
                    }

                    return 0;
                } );

        m_frame->OnModify();
    }
    else
    {
        m_frame->RollbackFromUndo();
    }

    return 0;
}


int BOARD_EDITOR_CONTROL::Plot( const TOOL_EVENT& aEvent )
{
    m_frame->ToPlotter( ID_GEN_PLOT );
    return 0;
}


int BOARD_EDITOR_CONTROL::Search( const TOOL_EVENT& aEvent )
{
    m_frame->ToggleSearch();
    return 0;
}


int BOARD_EDITOR_CONTROL::Find( const TOOL_EVENT& aEvent )
{
    m_frame->ShowFindDialog();
    return 0;
}


int BOARD_EDITOR_CONTROL::FindNext( const TOOL_EVENT& aEvent )
{
    m_frame->FindNext( aEvent.IsAction( &ACTIONS::findPrevious ) );
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
    name += wxT( "." ) + SpecctraSessionFileExtension;

    fullFileName = wxFileSelector( _( "Specctra Session File" ), path, name,
                                   wxT( "." ) + SpecctraSessionFileExtension,
                                   SpecctraSessionFileWildcard(), wxFD_OPEN | wxFD_CHANGE_DIR,
                                   frame() );

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
    {
        fn = fullFileName;
    }

    fullFileName = wxFileSelector( _( "Specctra DSN File" ), fn.GetPath(), fn.GetFullName(),
                                   SpecctraDsnFileExtension, SpecctraDsnFileWildcard(),
                                   wxFD_SAVE | wxFD_OVERWRITE_PROMPT | wxFD_CHANGE_DIR, frame() );

    if( !fullFileName.IsEmpty() )
    {
        m_frame->SetLastPath( LAST_PATH_SPECCTRADSN, fullFileName );
        getEditFrame<PCB_EDIT_FRAME>()->ExportSpecctraFile( fullFileName );
    }

    return 0;
}


int BOARD_EDITOR_CONTROL::ExportNetlist( const TOOL_EVENT& aEvent )
{
    wxCHECK( m_frame, 0 );

    wxFileName fn = m_frame->Prj().GetProjectFullName();

    // Use a different file extension for the board netlist so the schematic netlist file
    // is accidentally overwritten.
    fn.SetExt( wxT( "pcb_net" ) );

    wxFileDialog dlg( m_frame, _( "Export Board Netlist" ), fn.GetPath(), fn.GetFullName(),
                      _( "KiCad board netlist files" ) + AddFileExtListToFilter( { "pcb_net" } ),
                      wxFD_SAVE | wxFD_OVERWRITE_PROMPT );

    dlg.SetExtraControlCreator( &LEGACYFILEDLG_NETLIST_OPTIONS::Create );

    if( dlg.ShowModal() == wxID_CANCEL )
        return 0;

    fn = dlg.GetPath();

    if( !fn.IsDirWritable() )
    {
        wxString msg;

        msg.Printf( _( "Path `%s` is read only." ), fn.GetPath() );
        wxMessageDialog( m_frame, msg, _( "I/O Error" ), wxOK | wxCENTER | wxICON_EXCLAMATION );
        return 0;
    }

    const LEGACYFILEDLG_NETLIST_OPTIONS* noh =
            dynamic_cast<const LEGACYFILEDLG_NETLIST_OPTIONS*>( dlg.GetExtraControl() );
    wxCHECK( noh, 0 );

    NETLIST netlist;

    for( const FOOTPRINT* footprint : board()->Footprints() )
    {
        COMPONENT* component = new COMPONENT( footprint->GetFPID(), footprint->GetReference(),
                                              footprint->GetValue(), footprint->GetPath(),
                                              { footprint->m_Uuid } );

        for( const PAD* pad : footprint->Pads() )
        {
            const wxString& netname = pad->GetShortNetname();

            if( !netname.IsEmpty() )
            {
                component->AddNet( pad->GetNumber(), netname, pad->GetPinFunction(),
                                   pad->GetPinType() );
            }
        }

        netlist.AddComponent( component );
    }

    FILE_OUTPUTFORMATTER formatter( fn.GetFullPath() );

    netlist.Format( "pcb_netlist", &formatter, 0, noh->GetNetlistOptions() );

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
        wxFAIL_MSG( wxT( "GenerateFabFiles(): unexpected request" ) );

    return 0;
}


int BOARD_EDITOR_CONTROL::RepairBoard( const TOOL_EVENT& aEvent )
{
    int      errors = 0;
    wxString details;
    bool     quiet = aEvent.Parameter<bool>();

    // Repair duplicate IDs and missing nets.
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

    for( PCB_TRACK* track : board()->Tracks() )
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

        if( !quiet )
            DisplayInfoMessage( m_frame, msg, details );
    }
    else if( !quiet )
    {
        DisplayInfoMessage( m_frame, _( "No board problems found." ) );
    }

    return 0;
}


int BOARD_EDITOR_CONTROL::UpdatePCBFromSchematic( const TOOL_EVENT& aEvent )
{
    NETLIST netlist;

    if( m_frame->FetchNetlistFromSchematic( netlist, _( "Updating PCB requires a fully annotated "
                                                        "schematic." ) ) )
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
        DisplayErrorMessage( m_frame, _( "Cannot update schematic because Pcbnew is opened in "
                                         "stand-alone mode. In order to create or update PCBs "
                                         "from schematics, you must launch the KiCad project "
                                         "manager and create a project." ) );
        return 0;
    }

    m_frame->RunEeschema();
    KIWAY_PLAYER* frame = m_frame->Kiway().Player( FRAME_SCH, false );

    if( frame )
    {
        std::string payload;

        if( wxWindow* blocking_win = frame->Kiway().GetBlockingDialog() )
            blocking_win->Close( true );

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


int BOARD_EDITOR_CONTROL::ToggleProperties( const TOOL_EVENT& aEvent )
{
    getEditFrame<PCB_EDIT_FRAME>()->ToggleProperties();
    return 0;
}


int BOARD_EDITOR_CONTROL::ToggleSearch( const TOOL_EVENT& aEvent )
{
    getEditFrame<PCB_EDIT_FRAME>()->ToggleSearch();
    return 0;
}


int BOARD_EDITOR_CONTROL::TogglePythonConsole( const TOOL_EVENT& aEvent )
{
    m_frame->ScriptingConsoleEnableDisable();
    return 0;
}


// Track & via size control
int BOARD_EDITOR_CONTROL::TrackWidthInc( const TOOL_EVENT& aEvent )
{
    BOARD_DESIGN_SETTINGS& designSettings = getModel<BOARD>()->GetDesignSettings();
    PCB_SELECTION&         selection = m_toolMgr->GetTool<PCB_SELECTION_TOOL>()->GetSelection();

    if( m_frame->ToolStackIsEmpty()
        && SELECTION_CONDITIONS::OnlyTypes( { PCB_TRACE_T, PCB_ARC_T, PCB_VIA_T } )( selection ) )
    {
        BOARD_COMMIT commit( this );

        for( EDA_ITEM* item : selection )
        {
            if( item->IsType( { PCB_TRACE_T, PCB_ARC_T } ) )
            {
                PCB_TRACK* track = static_cast<PCB_TRACK*>( item );

                // Note: skip first entry which is the current netclass value
                for( int i = 1; i < (int) designSettings.m_TrackWidthList.size(); ++i )
                {
                    int candidate = designSettings.m_TrackWidthList[ i ];

                    if( candidate > track->GetWidth() )
                    {
                        commit.Modify( track );
                        track->SetWidth( candidate );
                        break;
                    }
                }
            }
        }

        commit.Push( wxT( "Increase Track Width" ) );
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
        int widthIndex = designSettings.GetTrackWidthIndex();

        if( routerTool && routerTool->IsToolActive()
            && routerTool->Router()->GetState() == PNS::ROUTER::RouterState::ROUTE_TRACK
            && designSettings.m_UseConnectedTrackWidth && !designSettings.m_TempOverrideTrackWidth )
        {
            designSettings.m_TempOverrideTrackWidth = true;
        }
        else
        {
            widthIndex++;
        }

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
    PCB_SELECTION&         selection = m_toolMgr->GetTool<PCB_SELECTION_TOOL>()->GetSelection();

    if( m_frame->ToolStackIsEmpty()
        && SELECTION_CONDITIONS::OnlyTypes( { PCB_TRACE_T, PCB_ARC_T, PCB_VIA_T } )( selection ) )
    {
        BOARD_COMMIT commit( this );

        for( EDA_ITEM* item : selection )
        {
            if( item->IsType( { PCB_TRACE_T, PCB_ARC_T } ) )
            {
                PCB_TRACK* track = static_cast<PCB_TRACK*>( item );

                // Note: skip first entry which is the current netclass value
                for( int i = designSettings.m_TrackWidthList.size() - 1; i >= 1; --i )
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

        commit.Push( wxT( "Decrease Track Width" ) );
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
        int widthIndex = designSettings.GetTrackWidthIndex();

        if( routerTool && routerTool->IsToolActive()
            && routerTool->Router()->GetState() == PNS::ROUTER::RouterState::ROUTE_TRACK
            && designSettings.m_UseConnectedTrackWidth && !designSettings.m_TempOverrideTrackWidth )
        {
            designSettings.m_TempOverrideTrackWidth = true;
        }
        else
        {
            widthIndex--;
        }

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
    PCB_SELECTION&         selection = m_toolMgr->GetTool<PCB_SELECTION_TOOL>()->GetSelection();

    if( m_frame->ToolStackIsEmpty()
        && SELECTION_CONDITIONS::OnlyTypes( { PCB_TRACE_T, PCB_ARC_T, PCB_VIA_T } )( selection ) )
    {
        BOARD_COMMIT commit( this );

        for( EDA_ITEM* item : selection )
        {
            if( item->Type() == PCB_VIA_T )
            {
                PCB_VIA* via = static_cast<PCB_VIA*>( item );

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

        commit.Push( wxT( "Increase Via Size" ) );
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
    PCB_SELECTION&         selection = m_toolMgr->GetTool<PCB_SELECTION_TOOL>()->GetSelection();

    if( m_frame->ToolStackIsEmpty()
        && SELECTION_CONDITIONS::OnlyTypes( { PCB_TRACE_T, PCB_ARC_T, PCB_VIA_T } )( selection ) )
    {
        BOARD_COMMIT commit( this );

        for( EDA_ITEM* item : selection )
        {
            if( item->Type() == PCB_VIA_T )
            {
                PCB_VIA* via = static_cast<PCB_VIA*>( item );

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


int BOARD_EDITOR_CONTROL::PlaceFootprint( const TOOL_EVENT& aEvent )
{
    if( m_inPlaceFootprint )
        return 0;

    REENTRANCY_GUARD guard( &m_inPlaceFootprint );

    FOOTPRINT*            fp = aEvent.Parameter<FOOTPRINT*>();
    bool                  fromOtherCommand = fp != nullptr;
    KIGFX::VIEW_CONTROLS* controls = getViewControls();
    BOARD_COMMIT          commit( m_frame );
    BOARD*                board = getModel<BOARD>();
    COMMON_SETTINGS*      common_settings = Pgm().GetCommonSettings();

    m_toolMgr->RunAction( PCB_ACTIONS::selectionClear, true );

    m_frame->PushTool( aEvent );

    auto setCursor =
            [&]()
            {
                m_frame->GetCanvas()->SetCurrentCursor( KICURSOR::PENCIL );
            };

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
                        m_frame->PutDataInPreviousState( undo );
                        m_frame->ClearListAndDeleteItems( undo );
                        delete undo;
                    }
                }

                fp = nullptr;
                m_placingFootprint = false;
            };

    Activate();
    // Must be done after Activate() so that it gets set into the correct context
    controls->ShowCursor( true );
    // Set initial cursor
    setCursor();

    VECTOR2I cursorPos = controls->GetCursorPosition();
    bool     ignorePrimePosition = false;
    bool     reselect = false;

    // Prime the pump
    if( fp )
    {
        m_placingFootprint = true;
        fp->SetPosition( cursorPos );
        m_toolMgr->RunAction( PCB_ACTIONS::selectItem, true, fp );
        m_toolMgr->RunAction( ACTIONS::refreshPreview );
    }
    else if( aEvent.HasPosition() )
    {
        m_toolMgr->PrimeTool( aEvent.Position() );
    }
    else if( common_settings->m_Input.immediate_actions && !aEvent.IsReactivate() )
    {
        m_toolMgr->PrimeTool( { 0, 0 } );
        ignorePrimePosition = true;
    }

    // Main loop: keep receiving events
    while( TOOL_EVENT* evt = Wait() )
    {
        setCursor();
        cursorPos = controls->GetCursorPosition( !evt->DisableGridSnapping() );

        if( reselect && fp )
            m_toolMgr->RunAction( PCB_ACTIONS::selectItem, true, fp );

        if( evt->IsCancelInteractive() )
        {
            if( fp )
            {
                cleanup();
            }
            else
            {
                m_frame->PopTool( aEvent );
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
                frame()->PopTool( aEvent );
                break;
            }
        }
        else if( evt->IsClick( BUT_LEFT ) )
        {
            if( !fp )
            {
                // Pick the footprint to be placed
                fp = m_frame->SelectFootprintFromLibTree();

                if( fp == nullptr )
                    continue;

                // If we started with a hotkey which has a position then warp back to that.
                // Otherwise update to the current mouse position pinned inside the autoscroll
                // boundaries.
                if( evt->IsPrime() && !ignorePrimePosition )
                {
                    cursorPos = evt->Position();
                    getViewControls()->WarpMouseCursor( cursorPos, true );
                }
                else
                {
                    getViewControls()->PinCursorInsideNonAutoscrollArea( true );
                    cursorPos = getViewControls()->GetMousePosition();
                }

                m_placingFootprint = true;

                fp->SetLink( niluuid );

                fp->SetFlags( IS_NEW ); // whatever

                // Set parent so that clearance can be loaded
                fp->SetParent( board );
                board->UpdateUserUnits( fp, m_frame->GetCanvas()->GetView() );

                for( PAD* pad : fp->Pads() )
                {
                    pad->SetLocalRatsnestVisible( m_frame->GetPcbNewSettings()->m_Display.m_ShowGlobalRatsnest );

                    // Pads in the library all have orphaned nets.  Replace with Default.
                    pad->SetNetCode( 0 );
                }

                // Put it on FRONT layer,
                // (Can be stored flipped if the lib is an archive built from a board)
                if( fp->IsFlipped() )
                    fp->Flip( fp->GetPosition(), m_frame->GetPcbNewSettings()->m_FlipLeftRight );

                fp->SetOrientation( ANGLE_0 );
                fp->SetPosition( cursorPos );

                commit.Add( fp );
                m_toolMgr->RunAction( PCB_ACTIONS::selectItem, true, fp );

                m_toolMgr->RunAction( ACTIONS::refreshPreview );
            }
            else
            {
                m_toolMgr->RunAction( PCB_ACTIONS::selectionClear, true );
                commit.Push( _( "Place a footprint" ) );
                fp = nullptr;  // to indicate that there is no footprint that we currently modify
                m_placingFootprint = false;
            }
        }
        else if( evt->IsClick( BUT_RIGHT ) )
        {
            m_menu.ShowContextMenu(  selection()  );
        }
        else if( fp && ( evt->IsMotion() || evt->IsAction( &ACTIONS::refreshPreview ) ) )
        {
            fp->SetPosition( cursorPos );
            selection().SetReferencePoint( cursorPos );
            getView()->Update( &selection() );
            getView()->Update( fp );
        }
        else if( fp && evt->IsAction( &PCB_ACTIONS::properties ) )
        {
            // Calling 'Properties' action clears the selection, so we need to restore it
            reselect = true;
        }
        else if( fp && ZONE_FILLER_TOOL::IsZoneFillAction( evt ) )
        {
            wxBell();
        }
        else
        {
            evt->SetPassEvent();
        }

        // Enable autopanning and cursor capture only when there is a footprint to be placed
        controls->SetAutoPan( fp != nullptr );
        controls->CaptureCursor( fp != nullptr );
    }

    controls->SetAutoPan( false );
    controls->CaptureCursor( false );
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

        m_toolMgr->PostEvent( EVENTS::SelectedEvent );
        m_frame->OnModify();
    }

    return 0;
}


static bool mergeZones( EDA_DRAW_FRAME* aFrame, BOARD_COMMIT& aCommit,
                        std::vector<ZONE*>& aOriginZones, std::vector<ZONE*>& aMergedZones )
{
    aCommit.Modify( aOriginZones[0] );

    for( unsigned int i = 1; i < aOriginZones.size(); i++ )
    {
        aOriginZones[0]->Outline()->BooleanAdd( *aOriginZones[i]->Outline(),
                                                SHAPE_POLY_SET::PM_FAST );
    }

    aOriginZones[0]->Outline()->Simplify( SHAPE_POLY_SET::PM_FAST );

    // We should have one polygon, possibly with holes.  If we end up with two polygons (either
    // because the intersection was a single point or because the intersection was within one of
    // the zone's holes) then we can't merge.
    if( aOriginZones[0]->Outline()->IsSelfIntersecting()
            || aOriginZones[0]->Outline()->OutlineCount() > 1 )
    {
        DisplayErrorMessage( aFrame, _( "Zones have insufficient overlap for merging." ) );
        aCommit.Revert();
        return false;
    }

    for( unsigned int i = 1; i < aOriginZones.size(); i++ )
        aCommit.Remove( aOriginZones[i] );

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

    for( EDA_ITEM* item : selection )
    {
        ZONE* curr_area = dynamic_cast<ZONE*>( item );

        if( !curr_area )
            continue;

        if( !firstZone )
            firstZone = curr_area;

        netcode = curr_area->GetNetCode();

        if( firstZone->GetNetCode() != netcode )
        {
            wxLogMessage( _( "Some zone netcodes did not match and were not merged." ) );
            continue;
        }

        if( curr_area->GetAssignedPriority() != firstZone->GetAssignedPriority() )
        {
            wxLogMessage( _( "Some zone priorities did not match and were not merged." ) );
            continue;
        }

        if( curr_area->GetIsRuleArea() != firstZone->GetIsRuleArea() )
        {
            wxLogMessage( _( "Some zones were rule areas and were not merged." ) );
            continue;
        }

        if( curr_area->GetLayerSet() != firstZone->GetLayerSet() )
        {
            wxLogMessage( _( "Some zone layer sets did not match and were not merged." ) );
            continue;
        }

        bool intersects = curr_area == firstZone;

        for( ZONE* candidate : toMerge )
        {
            if( intersects )
                break;

            if( board->TestZoneIntersection( curr_area, candidate ) )
                intersects = true;
        }

        if( !intersects )
        {
            wxLogMessage( _( "Some zones did not intersect and were not merged." ) );
            continue;
        }

        toMerge.push_back( curr_area );
    }

    m_toolMgr->RunAction( PCB_ACTIONS::selectionClear, true );

    if( !toMerge.empty() )
    {
        if( mergeZones( m_frame, commit, toMerge, merged ) )
        {
            commit.Push( wxT( "Merge zones" ) );

            for( EDA_ITEM* item : merged )
                m_toolMgr->RunAction( PCB_ACTIONS::selectItem, true, item );
        }
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

    // If the new zone is on the same layer(s) as the initial zone,
    // offset it a bit so it can more easily be picked.
    if( oldZone->GetIsRuleArea() && ( oldZone->GetLayerSet() == zoneSettings.m_Layers ) )
        newZone->Move( VECTOR2I( pcbIUScale.IU_PER_MM, pcbIUScale.IU_PER_MM ) );
    else if( !oldZone->GetIsRuleArea() && zoneSettings.m_Layers.test( oldZone->GetLayer() ) )
        newZone->Move( VECTOR2I( pcbIUScale.IU_PER_MM, pcbIUScale.IU_PER_MM ) );

    commit.Add( newZone.release() );
    commit.Push( _( "Duplicate zone" ) );

    return 0;
}


int BOARD_EDITOR_CONTROL::CrossProbeToSch( const TOOL_EVENT& aEvent )
{
    doCrossProbePcbToSch( aEvent, false );

    return 0;
}


int BOARD_EDITOR_CONTROL::ExplicitCrossProbeToSch( const TOOL_EVENT& aEvent )
{
    doCrossProbePcbToSch( aEvent, true );

    return 0;
}


void BOARD_EDITOR_CONTROL::doCrossProbePcbToSch( const TOOL_EVENT& aEvent, bool aForce )
{
    // Don't get in an infinite loop PCB -> SCH -> PCB -> SCH -> ...
    if( m_frame->m_probingSchToPcb )
        return;

    PCB_SELECTION_TOOL*  selTool = m_toolMgr->GetTool<PCB_SELECTION_TOOL>();
    const PCB_SELECTION& selection = selTool->GetSelection();
    EDA_ITEM*            focusItem = nullptr;

    if( aEvent.Matches( EVENTS::PointSelectedEvent ) )
        focusItem = selection.GetLastAddedItem();

    m_frame->SendSelectItemsToSch( selection.GetItems(), focusItem, aForce );

    // Update 3D viewer highlighting
    m_frame->Update3DView( false, frame()->GetPcbNewSettings()->m_Display.m_Live3DRefresh );
}


int BOARD_EDITOR_CONTROL::AssignNetclass( const TOOL_EVENT& aEvent )
{
    PCB_SELECTION_TOOL*  selectionTool = m_toolMgr->GetTool<PCB_SELECTION_TOOL>();

    const PCB_SELECTION& selection = selectionTool->RequestSelection(
            []( const VECTOR2I& aPt, GENERAL_COLLECTOR& aCollector, PCB_SELECTION_TOOL* sTool )
            {
                // Iterate from the back so we don't have to worry about removals.
                for( int i = aCollector.GetCount() - 1; i >= 0; --i )
                {
                    if( !dynamic_cast<BOARD_CONNECTED_ITEM*>( aCollector[ i ] ) )
                        aCollector.Remove( aCollector[ i ] );
                }
            },
            true /* prompt user regarding locked items */ );

    intptr_t netCode = -1;
    wxString netName;

    for( EDA_ITEM* item : selection )
    {
        NETINFO_ITEM* net = static_cast<BOARD_CONNECTED_ITEM*>( item )->GetNet();

        if( !net->HasAutoGeneratedNetname() )
        {
            netCode = net->GetNetCode();
            netName = net->GetNetname();
            break;
        }
    }

    if( netName.IsEmpty() )
    {
        m_frame->ShowInfoBarError( _( "Selection contains no items with labeled nets." ) );
        return 0;
    }

    selectionTool->ClearSelection();
    m_toolMgr->RunAction( PCB_ACTIONS::selectNet, true, (void*) netCode );
    canvas()->ForceRefresh();

    DIALOG_ASSIGN_NETCLASS dlg( m_frame, netName, board()->GetNetClassAssignmentCandidates(),
            [this]( const std::vector<wxString>& aNetNames )
            {
                PCB_SELECTION_TOOL*  selTool = m_toolMgr->GetTool<PCB_SELECTION_TOOL>();
                selTool->ClearSelection();

                for( const wxString& curr_netName : aNetNames )
                {
                    int curr_netCode = board()->GetNetInfo().GetNetItem( curr_netName )->GetNetCode();

                    if( curr_netCode > 0 )
                        selTool->SelectAllItemsOnNet( curr_netCode );
                }

                canvas()->ForceRefresh();
                m_frame->UpdateMsgPanel();
            } );

    if( dlg.ShowModal() == wxID_OK )
    {
        board()->SynchronizeNetsAndNetClasses( false );
        // Refresh UI that depends on netclasses, such as the properties panel
        m_toolMgr->ProcessEvent( EVENTS::SelectedItemsModified );
    }

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

    if( aEvent.IsAction( &PCB_ACTIONS::editFpInFpEditor ) )
        editor->LoadFootprintFromBoard( fp );
    else if( aEvent.IsAction( &PCB_ACTIONS::editLibFpInFpEditor ) )
        editor->LoadFootprintFromLibrary( fp->GetFPID() );

    editor->Show( true );
    editor->Raise();        // Iconize( false );

    if( selection.IsHover() )
        m_toolMgr->RunAction( PCB_ACTIONS::selectionClear, true );

    return 0;
}


void BOARD_EDITOR_CONTROL::DoSetDrillOrigin( KIGFX::VIEW* aView, PCB_BASE_FRAME* aFrame,
                                             EDA_ITEM* originViewItem, const VECTOR2D& aPosition )
{
    aFrame->GetDesignSettings().SetAuxOrigin( VECTOR2I( aPosition ) );
    originViewItem->SetPosition( aPosition );
    aView->MarkDirty();
    aFrame->OnModify();
}


int BOARD_EDITOR_CONTROL::DrillOrigin( const TOOL_EVENT& aEvent )
{
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

    m_toolMgr->RunAction( ACTIONS::pickerTool, true, (void*) &aEvent );

    return 0;
}


void BOARD_EDITOR_CONTROL::setTransitions()
{
    Go( &BOARD_EDITOR_CONTROL::New,                    ACTIONS::doNew.MakeEvent() );
    Go( &BOARD_EDITOR_CONTROL::Open,                   ACTIONS::open.MakeEvent() );
    Go( &BOARD_EDITOR_CONTROL::Save,                   ACTIONS::save.MakeEvent() );
    Go( &BOARD_EDITOR_CONTROL::SaveAs,                 ACTIONS::saveAs.MakeEvent() );
    Go( &BOARD_EDITOR_CONTROL::SaveCopy,               ACTIONS::saveCopy.MakeEvent() );
    Go( &BOARD_EDITOR_CONTROL::Revert,                 ACTIONS::revert.MakeEvent() );
    Go( &BOARD_EDITOR_CONTROL::PageSettings,           ACTIONS::pageSettings.MakeEvent() );
    Go( &BOARD_EDITOR_CONTROL::Plot,                   ACTIONS::plot.MakeEvent() );

    Go( &BOARD_EDITOR_CONTROL::Search,                 ACTIONS::showSearch.MakeEvent() );
    Go( &BOARD_EDITOR_CONTROL::Find,                   ACTIONS::find.MakeEvent() );
    Go( &BOARD_EDITOR_CONTROL::FindNext,               ACTIONS::findNext.MakeEvent() );
    Go( &BOARD_EDITOR_CONTROL::FindNext,               ACTIONS::findPrevious.MakeEvent() );

    Go( &BOARD_EDITOR_CONTROL::BoardSetup,             PCB_ACTIONS::boardSetup.MakeEvent() );
    Go( &BOARD_EDITOR_CONTROL::ImportNetlist,          PCB_ACTIONS::importNetlist.MakeEvent() );
    Go( &BOARD_EDITOR_CONTROL::ImportSpecctraSession,
        PCB_ACTIONS::importSpecctraSession.MakeEvent() );
    Go( &BOARD_EDITOR_CONTROL::ExportSpecctraDSN,      PCB_ACTIONS::exportSpecctraDSN.MakeEvent() );

    if( ADVANCED_CFG::GetCfg().m_ShowPcbnewExportNetlist && m_frame &&
        m_frame->GetExportNetlistAction() )
        Go( &BOARD_EDITOR_CONTROL::ExportNetlist, m_frame->GetExportNetlistAction()->MakeEvent() );

    Go( &BOARD_EDITOR_CONTROL::GenerateDrillFiles,
        PCB_ACTIONS::generateDrillFiles.MakeEvent() );
    Go( &BOARD_EDITOR_CONTROL::GenerateFabFiles,       PCB_ACTIONS::generateGerbers.MakeEvent() );
    Go( &BOARD_EDITOR_CONTROL::GeneratePosFile,        PCB_ACTIONS::generatePosFile.MakeEvent() );
    Go( &BOARD_EDITOR_CONTROL::GenerateFabFiles,
        PCB_ACTIONS::generateReportFile.MakeEvent() );
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
    Go( &BOARD_EDITOR_CONTROL::PlaceFootprint,         PCB_ACTIONS::placeFootprint.MakeEvent() );
    Go( &BOARD_EDITOR_CONTROL::DrillOrigin,            PCB_ACTIONS::drillOrigin.MakeEvent() );

    Go( &BOARD_EDITOR_CONTROL::EditFpInFpEditor,       PCB_ACTIONS::editFpInFpEditor.MakeEvent() );
    Go( &BOARD_EDITOR_CONTROL::EditFpInFpEditor,       PCB_ACTIONS::editLibFpInFpEditor.MakeEvent() );

    // Cross-select
    Go( &BOARD_EDITOR_CONTROL::CrossProbeToSch,        EVENTS::PointSelectedEvent );
    Go( &BOARD_EDITOR_CONTROL::CrossProbeToSch,        EVENTS::SelectedEvent );
    Go( &BOARD_EDITOR_CONTROL::CrossProbeToSch,        EVENTS::UnselectedEvent );
    Go( &BOARD_EDITOR_CONTROL::CrossProbeToSch,        EVENTS::ClearedEvent );
    Go( &BOARD_EDITOR_CONTROL::ExplicitCrossProbeToSch, PCB_ACTIONS::selectOnSchematic.MakeEvent() );

    // Other
    Go( &BOARD_EDITOR_CONTROL::ToggleLockSelected,     PCB_ACTIONS::toggleLock.MakeEvent() );
    Go( &BOARD_EDITOR_CONTROL::LockSelected,           PCB_ACTIONS::lock.MakeEvent() );
    Go( &BOARD_EDITOR_CONTROL::UnlockSelected,         PCB_ACTIONS::unlock.MakeEvent() );

    Go( &BOARD_EDITOR_CONTROL::AssignNetclass,         PCB_ACTIONS::assignNetClass.MakeEvent() );

    Go( &BOARD_EDITOR_CONTROL::UpdatePCBFromSchematic,
        ACTIONS::updatePcbFromSchematic.MakeEvent() );
    Go( &BOARD_EDITOR_CONTROL::UpdateSchematicFromPCB,
        ACTIONS::updateSchematicFromPcb.MakeEvent() );
    Go( &BOARD_EDITOR_CONTROL::ShowEeschema,           PCB_ACTIONS::showEeschema.MakeEvent() );
    Go( &BOARD_EDITOR_CONTROL::ToggleLayersManager,    PCB_ACTIONS::showLayersManager.MakeEvent() );
    Go( &BOARD_EDITOR_CONTROL::ToggleProperties,       PCB_ACTIONS::showProperties.MakeEvent() );
    Go( &BOARD_EDITOR_CONTROL::ToggleSearch,           PCB_ACTIONS::showSearch.MakeEvent() );
    Go( &BOARD_EDITOR_CONTROL::TogglePythonConsole,    PCB_ACTIONS::showPythonConsole.MakeEvent() );
    Go( &BOARD_EDITOR_CONTROL::RepairBoard,            PCB_ACTIONS::repairBoard.MakeEvent() );
}


const int BOARD_EDITOR_CONTROL::WIDTH_STEP = 100000;
