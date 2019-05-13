/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 1992-2019 KiCad Developers, see AUTHORS.txt for contributors.
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

#include <fctsys.h>
#include <kiway.h>
#include <sch_view.h>
#include <sch_draw_panel.h>
#include <sch_edit_frame.h>
#include <sch_component.h>
#include <sch_sheet.h>
#include <sch_bitmap.h>
#include <connection_graph.h>
#include <erc.h>
#include <eeschema_id.h>
#include <netlist_object.h>
#include <tool/tool_manager.h>
#include <tools/ee_actions.h>
#include <tools/ee_picker_tool.h>
#include <tools/sch_editor_control.h>
#include <tools/ee_selection_tool.h>
#include <tools/sch_drawing_tools.h>
#include <project.h>
#include <ee_hotkeys.h>
#include <advanced_config.h>
#include <simulation_cursors.h>
#include <sim/sim_plot_frame.h>
#include <sch_legacy_plugin.h>
#include <class_library.h>
#include <confirm.h>
#include <lib_edit_frame.h>
#include <sch_painter.h>
#include "sch_wire_bus_tool.h"

TOOL_ACTION EE_ACTIONS::refreshPreview( "eeschema.EditorControl.refreshPreview",
         AS_GLOBAL, 0, "", "" );

TOOL_ACTION EE_ACTIONS::simProbe( "eeschema.Simulation.probe",
        AS_GLOBAL, 0,
        _( "Add a simulator probe" ), "" );

TOOL_ACTION EE_ACTIONS::simTune( "eeschema.Simulation.tune",
        AS_GLOBAL, 0,
        _( "Select a value to be tuned" ), "" );

TOOL_ACTION EE_ACTIONS::highlightNet( "eeschema.EditorControl.highlightNet",
        AS_GLOBAL, 0, "", "" );

TOOL_ACTION EE_ACTIONS::clearHighlight( "eeschema.EditorControl.clearHighlight",
        AS_GLOBAL, 0, "", "" );

TOOL_ACTION EE_ACTIONS::updateNetHighlighting( "eeschema.EditorControl.updateNetHighlighting",
        AS_GLOBAL, 0, "", "" );

TOOL_ACTION EE_ACTIONS::highlightNetCursor( "eeschema.EditorControl.highlightNetCursor",
        AS_GLOBAL, 0,
        _( "Highlight Net" ), _( "Highlight wires and pins of a net" ), NULL, AF_ACTIVATE );

TOOL_ACTION EE_ACTIONS::cut( "eeschema.EditorControl.cut",
        AS_GLOBAL, TOOL_ACTION::LegacyHotKey( HK_EDIT_CUT ),
        _( "Cut" ), _( "Cut selected item(s) to clipboard" ), cut_xpm );

TOOL_ACTION EE_ACTIONS::copy( "eeschema.EditorControl.copy",
        AS_GLOBAL, TOOL_ACTION::LegacyHotKey( HK_EDIT_COPY ),
        _( "Copy" ), _( "Copy selected item(s) to clipboard" ), copy_xpm );

TOOL_ACTION EE_ACTIONS::paste( "eeschema.EditorControl.paste",
        AS_GLOBAL, TOOL_ACTION::LegacyHotKey( HK_EDIT_PASTE ),
        _( "Paste" ), _( "Paste clipboard into schematic" ), paste_xpm );

TOOL_ACTION EE_ACTIONS::editWithSymbolEditor( "eeschema.EditorControl.editWithSymbolEditor",
        AS_GLOBAL, TOOL_ACTION::LegacyHotKey( HK_EDIT_COMPONENT_WITH_LIBEDIT ),
        _( "Edit with Symbol Editor" ), _( "Open the symbol editor to edit the symbol" ),
        libedit_xpm );

TOOL_ACTION EE_ACTIONS::showLibraryBrowser( "eeschema.EditorControl.showLibraryBrowser",
        AS_GLOBAL, 0,
        _( "Symbol Library Browser" ), "",
        library_browse_xpm );

TOOL_ACTION EE_ACTIONS::enterSheet( "eeschema.EditorControl.enterSheet",
        AS_GLOBAL, 0,
        _( "Enter Sheet" ), _( "Display the selected sheet's contents in the Eeschema window" ),
        enter_sheet_xpm );

TOOL_ACTION EE_ACTIONS::leaveSheet( "eeschema.EditorControl.leaveSheet",
        AS_GLOBAL, TOOL_ACTION::LegacyHotKey( HK_LEAVE_SHEET ),
        _( "Leave Sheet" ), _( "Display the parent sheet in the Eeschema window" ),
        leave_sheet_xpm );

TOOL_ACTION EE_ACTIONS::navigateHierarchy( "eeschema.EditorControl.navigateHierarchy",
        AS_GLOBAL, 0,
        _( "Show Hierarchy Navigator" ), "",
        hierarchy_nav_xpm );

TOOL_ACTION EE_ACTIONS::explicitCrossProbe( "eeschema.EditorControl.explicitCrossProbe",
        AS_GLOBAL, TOOL_ACTION::LegacyHotKey( HK_SELECT_ITEMS_ON_PCB ),
        _( "Highlight on PCB" ), _( "Highlight corresponding items in PCBNew" ),
        select_same_sheet_xpm );

TOOL_ACTION EE_ACTIONS::toggleHiddenPins( "eeschema.EditorControl.showHiddenPins",
        AS_GLOBAL, 0,
        _( "Show Hidden Pins" ), "",
        hidden_pin_xpm );


SCH_EDITOR_CONTROL::SCH_EDITOR_CONTROL() :
        EE_TOOL_BASE<SCH_EDIT_FRAME>( "eeschema.EditorControl" )
{
}


SCH_EDITOR_CONTROL::~SCH_EDITOR_CONTROL()
{
}


int SCH_EDITOR_CONTROL::CrossProbeToPcb( const TOOL_EVENT& aEvent )
{
    doCrossProbeSchToPcb( aEvent, false );
    return 0;
}


int SCH_EDITOR_CONTROL::ExplicitCrossProbeToPcb( const TOOL_EVENT& aEvent )
{
    doCrossProbeSchToPcb( aEvent, true );
    return 0;
}


void SCH_EDITOR_CONTROL::doCrossProbeSchToPcb( const TOOL_EVENT& aEvent, bool aForce )
{
    // Don't get in an infinite loop SCH -> PCB -> SCH -> PCB -> SCH -> ...
    if( m_probingSchToPcb )
    {
        m_probingSchToPcb = false;
        return;
    }

    EE_SELECTION_TOOL* selTool = m_toolMgr->GetTool<EE_SELECTION_TOOL>();
    SCH_ITEM*          item = nullptr;
    SCH_COMPONENT*     component = nullptr;

    if( aForce )
    {
        SELECTION& selection = selTool->RequestSelection();

        if( selection.GetSize() >= 1 )
            item = (SCH_ITEM*) selection.Front();
    }
    else
    {
        SELECTION& selection = selTool->GetSelection();

        if( selection.GetSize() >= 1 )
            item = (SCH_ITEM*) selection.Front();
    }

    if( !item )
    {
        if( aForce )
            m_frame->SendMessageToPCBNEW( nullptr, nullptr );

        return;
    }


    switch( item->Type() )
    {
    case SCH_FIELD_T:
    case LIB_FIELD_T:
        component = (SCH_COMPONENT*) item->GetParent();
        m_frame->SendMessageToPCBNEW( item, component );
        break;

    case SCH_COMPONENT_T:
        component = (SCH_COMPONENT*) item;
        m_frame->SendMessageToPCBNEW( item, component );
        break;

    case SCH_PIN_T:
        component = (SCH_COMPONENT*) item->GetParent();
        m_frame->SendMessageToPCBNEW( static_cast<SCH_PIN*>( item ), component );
        break;

    case SCH_SHEET_T:
        if( aForce )
            m_frame->SendMessageToPCBNEW( item, nullptr );
        break;

    default:
        break;
    }
}


#ifdef KICAD_SPICE
static bool probeSimulation( SCH_EDIT_FRAME* aFrame, const VECTOR2D& aPosition )
{
    constexpr KICAD_T wiresAndComponents[] = { SCH_LINE_T, SCH_COMPONENT_T, SCH_SHEET_PIN_T, EOT };
    EE_SELECTION_TOOL* selTool = aFrame->GetToolManager()->GetTool<EE_SELECTION_TOOL>();

    EDA_ITEM* item = selTool->SelectPoint( aPosition, wiresAndComponents );

    if( !item )
        return false;

    std::unique_ptr<NETLIST_OBJECT_LIST> netlist( aFrame->BuildNetListBase() );

    for( NETLIST_OBJECT* obj : *netlist )
    {
        if( obj->m_Comp == item )
        {
            auto simFrame = (SIM_PLOT_FRAME*) aFrame->Kiway().Player( FRAME_SIMULATOR, false );

            if( simFrame )
                simFrame->AddVoltagePlot( obj->GetNetName() );

            break;
        }
    }

    return true;
}


int SCH_EDITOR_CONTROL::SimProbe( const TOOL_EVENT& aEvent )
{
    Activate();

    EE_PICKER_TOOL* picker = m_toolMgr->GetTool<EE_PICKER_TOOL>();
    assert( picker );

    m_frame->SetToolID( ID_SIM_PROBE, wxCURSOR_DEFAULT, _( "Add a simulator probe" ) );
    m_frame->GetCanvas()->SetCursor( SIMULATION_CURSORS::GetCursor( SIMULATION_CURSORS::CURSOR::PROBE ) );

    picker->SetClickHandler( std::bind( probeSimulation, m_frame, std::placeholders::_1 ) );
    picker->Activate();
    Wait();

    return 0;
}


static bool tuneSimulation( SCH_EDIT_FRAME* aFrame, const VECTOR2D& aPosition )
{
    constexpr KICAD_T fieldsAndComponents[] = { SCH_COMPONENT_T, SCH_FIELD_T, EOT };
    EE_SELECTION_TOOL* selTool = aFrame->GetToolManager()->GetTool<EE_SELECTION_TOOL>();
    EDA_ITEM*          item = selTool->SelectPoint( aPosition, fieldsAndComponents );

    if( !item )
        return false;

    if( item->Type() != SCH_COMPONENT_T )
    {
        item = item->GetParent();

        if( item->Type() != SCH_COMPONENT_T )
            return false;
    }

    auto simFrame = (SIM_PLOT_FRAME*) aFrame->Kiway().Player( FRAME_SIMULATOR, false );

    if( simFrame )
        simFrame->AddTuner( static_cast<SCH_COMPONENT*>( item ) );

    return true;
}


int SCH_EDITOR_CONTROL::SimTune( const TOOL_EVENT& aEvent )
{
    Activate();

    EE_PICKER_TOOL* picker = m_toolMgr->GetTool<EE_PICKER_TOOL>();
    assert( picker );

    m_frame->SetToolID( ID_SIM_TUNE, wxCURSOR_DEFAULT, _( "Select a value to be tuned" ) );
    m_frame->GetCanvas()->SetCursor( SIMULATION_CURSORS::GetCursor( SIMULATION_CURSORS::CURSOR::TUNE ) );

    picker->SetClickHandler( std::bind( tuneSimulation, m_frame, std::placeholders::_1 ) );
    picker->Activate();
    Wait();

    return 0;
}
#endif /* KICAD_SPICE */


// A singleton reference for clearing the highlight
static VECTOR2D CLEAR;


// TODO(JE) Probably use netcode rather than connection name here eventually
static bool highlightNet( TOOL_MANAGER* aToolMgr, const VECTOR2D& aPosition )
{
    SCH_EDIT_FRAME*     editFrame = static_cast<SCH_EDIT_FRAME*>( aToolMgr->GetEditFrame() );
    EE_SELECTION_TOOL*  selTool = aToolMgr->GetTool<EE_SELECTION_TOOL>();
    SCH_EDITOR_CONTROL* editorControl = aToolMgr->GetTool<SCH_EDITOR_CONTROL>();
    wxString            netName;
    bool                retVal = true;

    if( aPosition != CLEAR )
    {
        if( TestDuplicateSheetNames( false ) > 0 )
        {
            wxMessageBox( _( "Error: duplicate sub-sheet names found in current sheet." ) );
            retVal = false;
        }
        else
        {
            SCH_ITEM* item = (SCH_ITEM*) selTool->GetNode( aPosition );

            if( item && item->Connection( *g_CurrentSheet ) )
                netName = item->Connection( *g_CurrentSheet )->Name();
        }
    }

    if( netName.empty() )
    {
        editFrame->SetStatusText( wxT( "" ) );
        editFrame->SendCrossProbeClearHighlight();
    }
    else
    {
        editFrame->SendCrossProbeNetName( netName );
        editFrame->SetStatusText( wxString::Format( _( "Highlighted net: %s" ),
                                  UnescapeString( netName ) ) );
    }

    editFrame->SetSelectedNetName( netName );
    TOOL_EVENT dummy;
    editorControl->UpdateNetHighlighting( dummy );

    return retVal;
}


int SCH_EDITOR_CONTROL::HighlightNet( const TOOL_EVENT& aEvent )
{
    KIGFX::VIEW_CONTROLS* controls = getViewControls();
    VECTOR2D              cursorPos = controls->GetCursorPosition( !aEvent.Modifier( MD_ALT ) );

    highlightNet( m_toolMgr, cursorPos );

    return 0;
}


int SCH_EDITOR_CONTROL::ClearHighlight( const TOOL_EVENT& aEvent )
{
    highlightNet( m_toolMgr, CLEAR );

    return 0;
}


int SCH_EDITOR_CONTROL::UpdateNetHighlighting( const TOOL_EVENT& aEvent )
{
    SCH_SCREEN*            screen = g_CurrentSheet->LastScreen();
    std::vector<EDA_ITEM*> itemsToRedraw;
    wxString               selectedNetName = m_frame->GetSelectedNetName();

    if( !screen )
        return 0;

    for( SCH_ITEM* item = screen->GetDrawItems(); item; item = item->Next() )
    {
        SCH_CONNECTION* conn = item->Connection( *g_CurrentSheet );
        bool redraw = item->IsBrightened();

        if( conn && conn->Name() == selectedNetName )
            item->SetBrightened();
        else
            item->ClearBrightened();

        redraw |= item->IsBrightened();

        if( item->Type() == SCH_COMPONENT_T )
        {
            SCH_COMPONENT* comp = static_cast<SCH_COMPONENT*>( item );

            redraw |= comp->HasBrightenedPins();

            comp->ClearBrightenedPins();
            std::vector<LIB_PIN*> pins;
            comp->GetPins( pins );

            for( LIB_PIN* pin : pins )
            {
                auto pin_conn = comp->GetConnectionForPin( pin, *g_CurrentSheet );

                if( pin_conn && pin_conn->Name( false ) == selectedNetName )
                {
                    comp->BrightenPin( pin );
                    redraw = true;
                }
            }
        }
        else if( item->Type() == SCH_SHEET_T )
        {
            for( SCH_SHEET_PIN& pin : static_cast<SCH_SHEET*>( item )->GetPins() )
            {
                auto pin_conn = pin.Connection( *g_CurrentSheet );
                bool redrawPin = pin.IsBrightened();

                if( pin_conn && pin_conn->Name() == selectedNetName )
                    pin.SetBrightened();
                else
                    pin.ClearBrightened();

                redrawPin |= pin.IsBrightened();

                if( redrawPin )
                    itemsToRedraw.push_back( &pin );
            }
        }

        if( redraw )
            itemsToRedraw.push_back( item );
    }

    // Be sure hightlight change will be redrawn
    KIGFX::VIEW* view = getView();

    for( auto redrawItem : itemsToRedraw )
        view->Update( (KIGFX::VIEW_ITEM*)redrawItem, KIGFX::VIEW_UPDATE_FLAGS::REPAINT );

    m_frame->GetGalCanvas()->Refresh();

    return 0;
}


int SCH_EDITOR_CONTROL::HighlightNetCursor( const TOOL_EVENT& aEvent )
{
    // TODO(JE) remove once real-time connectivity is a given
    if( !ADVANCED_CFG::GetCfg().m_realTimeConnectivity || !CONNECTION_GRAPH::m_allowRealTime )
        m_frame->RecalculateConnections();

    Activate();

    EE_PICKER_TOOL* picker = m_toolMgr->GetTool<EE_PICKER_TOOL>();
    assert( picker );

    m_frame->SetToolID( ID_HIGHLIGHT_BUTT, wxCURSOR_HAND, _( "Highlight specific net" ) );
    picker->SetClickHandler( std::bind( highlightNet, m_toolMgr, std::placeholders::_1 ) );
    picker->Activate();
    Wait();

    return 0;
}


bool SCH_EDITOR_CONTROL::doCopy()
{
    EE_SELECTION_TOOL* selTool = m_toolMgr->GetTool<EE_SELECTION_TOOL>();
    SELECTION&         selection = selTool->GetSelection();

    if( !selection.GetSize() )
        return false;

    STRING_FORMATTER formatter;
    SCH_LEGACY_PLUGIN plugin;

    plugin.Format( &selection, &formatter );

    return m_toolMgr->SaveClipboard( formatter.GetString() );
}


int SCH_EDITOR_CONTROL::Cut( const TOOL_EVENT& aEvent )
{
    if( doCopy() )
        m_toolMgr->RunAction( EE_ACTIONS::doDelete, true );

    return 0;
}


int SCH_EDITOR_CONTROL::Copy( const TOOL_EVENT& aEvent )
{
    doCopy();

    return 0;
}


int SCH_EDITOR_CONTROL::Paste( const TOOL_EVENT& aEvent )
{
    EE_SELECTION_TOOL* selTool = m_toolMgr->GetTool<EE_SELECTION_TOOL>();

    DLIST<SCH_ITEM>&   dlist = m_frame->GetScreen()->GetDrawList();
    SCH_ITEM*          last = dlist.GetLast();

    std::string        text = m_toolMgr->GetClipboard();
    STRING_LINE_READER reader( text, "Clipboard" );
    SCH_LEGACY_PLUGIN  plugin;

    try
    {
        plugin.LoadContent( reader, m_frame->GetScreen() );
    }
    catch( IO_ERROR& e )
    {
        wxLogError( wxString::Format( "Malformed clipboard: %s" ), GetChars( e.What() ) );
        return 0;
    }

    // SCH_LEGACY_PLUGIN added the items to the DLIST, but not to the view or anything
    // else.  Pull them back out to start with.
    //
    EDA_ITEMS loadedItems;
    SCH_ITEM* next = nullptr;

    // We also make sure any pasted sheets will not cause recursion in the destination.
    // Moreover new sheets create new sheetpaths, and component alternate references must
    // be created and cleared
    //
    bool           sheetsPasted = false;
    SCH_SHEET_LIST hierarchy( g_RootSheet );
    SCH_SHEET_LIST initialHierarchy( g_RootSheet );

    wxFileName     destFn = g_CurrentSheet->Last()->GetFileName();

    if( destFn.IsRelative() )
        destFn.MakeAbsolute( m_frame->Prj().GetProjectPath() );

    for( SCH_ITEM* item = last ? last->Next() : dlist.GetFirst(); item; item = next )
    {
        next = item->Next();
        dlist.Remove( item );

        loadedItems.push_back( item );

        if( item->Type() == SCH_COMPONENT_T )
        {
            SCH_COMPONENT* component = (SCH_COMPONENT*) item;

            component->SetTimeStamp( GetNewTimeStamp() );

            // clear the annotation, but preserve the selected unit
            int unit = component->GetUnit();
            component->ClearAnnotation( nullptr );
            component->SetUnit( unit );
        }
        if( item->Type() == SCH_SHEET_T )
        {
            SCH_SHEET* sheet = (SCH_SHEET*) item;
            wxFileName srcFn = sheet->GetFileName();

            if( srcFn.IsRelative() )
                srcFn.MakeAbsolute( m_frame->Prj().GetProjectPath() );

            SCH_SHEET_LIST sheetHierarchy( sheet );

            if( hierarchy.TestForRecursion( sheetHierarchy, destFn.GetFullPath( wxPATH_UNIX ) ) )
            {
                auto msg = wxString::Format( _( "The pasted sheet \"%s\"\n"
                                                "was dropped because the destination already has "
                                                "the sheet or one of its subsheets as a parent." ),
                                             sheet->GetFileName() );
                DisplayError( m_frame, msg );
                loadedItems.pop_back();
            }
            else
            {
                // Duplicate sheet names and sheet time stamps are not valid.  Use a time stamp
                // based sheet name and update the time stamp for each sheet in the block.
                timestamp_t uid = GetNewTimeStamp();

                sheet->SetName( wxString::Format( wxT( "sheet%8.8lX" ), (unsigned long)uid ) );
                sheet->SetTimeStamp( uid );
                sheetsPasted = true;
            }
        }
    }

    // Now we can resolve the components and add everything to the screen, view, etc.
    //
    SYMBOL_LIB_TABLE* symLibTable = m_frame->Prj().SchSymbolLibTable();
    PART_LIB*         partLib = m_frame->Prj().SchLibs()->GetCacheLibrary();

    for( unsigned i = 0; i < loadedItems.size(); ++i )
    {
        EDA_ITEM* item = loadedItems[i];

        if( item->Type() == SCH_COMPONENT_T )
        {
            SCH_COMPONENT* component = (SCH_COMPONENT*) item;
            component->Resolve( *symLibTable, partLib );
        }
        else if( item->Type() == SCH_SHEET_T )
        {
            SCH_SHEET* sheet = (SCH_SHEET*) item;
            m_frame->InitSheet( sheet, sheet->GetFileName() );
        }

        item->SetFlags( IS_NEW | IS_MOVED );
        m_frame->AddItemToScreenAndUndoList( (SCH_ITEM*) item, i > 0 );

        // Reset flags for subsequent move operation
        item->SetFlags( IS_NEW | IS_MOVED );
    }

    if( sheetsPasted )
    {
        // We clear annotation of new sheet paths.
        SCH_SCREENS screensList( g_RootSheet );
        screensList.ClearAnnotationOfNewSheetPaths( initialHierarchy );
        m_frame->SetSheetNumberAndCount();
    }

    // Now clear the previous selection, select the pasted items, and fire up the "move"
    // tool.
    //
    m_toolMgr->RunAction( EE_ACTIONS::clearSelection, true );
    m_toolMgr->RunAction( EE_ACTIONS::addItemsToSel, true, &loadedItems );

    SELECTION& selection = selTool->GetSelection();

    if( !selection.Empty() )
    {
        SCH_ITEM* item = (SCH_ITEM*) selection.GetTopLeftItem();

        selection.SetReferencePoint( item->GetPosition() );
        m_toolMgr->RunAction( EE_ACTIONS::move, false );
    }

    return 0;
}


int SCH_EDITOR_CONTROL::EditWithSymbolEditor( const TOOL_EVENT& aEvent )
{
    EE_SELECTION_TOOL* selTool = m_toolMgr->GetTool<EE_SELECTION_TOOL>();
    SELECTION&         selection = selTool->RequestSelection( EE_COLLECTOR::ComponentsOnly );
    SCH_COMPONENT*     comp = nullptr;

    if( selection.GetSize() >= 1 )
        comp = (SCH_COMPONENT*) selection.Front();

    if( !comp || comp->GetEditFlags() != 0 )
        return 0;

    wxCommandEvent dummy;
    m_frame->OnOpenLibraryEditor( dummy );

    auto libeditFrame = (LIB_EDIT_FRAME*) m_frame->Kiway().Player( FRAME_SCH_LIB_EDITOR, false );

    if( libeditFrame )
    {
        const LIB_ID& id = comp->GetLibId();
        libeditFrame->LoadComponentAndSelectLib( id, comp->GetUnit(), comp->GetConvert() );
    }

    return 0;
}


int SCH_EDITOR_CONTROL::ShowLibraryBrowser( const TOOL_EVENT& aEvent )
{
    wxCommandEvent dummy;
    m_frame->OnOpenLibraryViewer( dummy );

    return 0;
}


int SCH_EDITOR_CONTROL::EnterSheet( const TOOL_EVENT& aEvent )
{
    EE_SELECTION_TOOL* selTool = m_toolMgr->GetTool<EE_SELECTION_TOOL>();
    const SELECTION&   selection = selTool->RequestSelection( EE_COLLECTOR::SheetsOnly );

    if( selection.GetSize() == 1 )
    {
        g_CurrentSheet->push_back( (SCH_SHEET*) selection.Front() );
        m_frame->DisplayCurrentSheet();
    }

    return 0;
}


int SCH_EDITOR_CONTROL::LeaveSheet( const TOOL_EVENT& aEvent )
{
    if( g_CurrentSheet->Last() != g_RootSheet )
    {
        g_CurrentSheet->pop_back();
        m_frame->DisplayCurrentSheet();
    }

    return 0;
}


int SCH_EDITOR_CONTROL::ToggleHiddenPins( const TOOL_EVENT& aEvent )
{
    m_frame->SetShowAllPins( !m_frame->GetShowAllPins() );

    auto painter = static_cast<KIGFX::SCH_PAINTER*>( getView()->GetPainter() );
    painter->GetSettings()->m_ShowHiddenPins = m_frame->GetShowAllPins();

    getView()->UpdateAllItems( KIGFX::REPAINT );
    m_frame->GetCanvas()->Refresh();

    return 0;
}


void SCH_EDITOR_CONTROL::setTransitions()
{
    /*
    Go( &SCH_EDITOR_CONTROL::ToggleLockSelected,    EE_ACTIONS::toggleLock.MakeEvent() );
    Go( &SCH_EDITOR_CONTROL::LockSelected,          EE_ACTIONS::lock.MakeEvent() );
    Go( &SCH_EDITOR_CONTROL::UnlockSelected,        EE_ACTIONS::unlock.MakeEvent() );
     */

    Go( &SCH_EDITOR_CONTROL::CrossProbeToPcb,       EVENTS::SelectedEvent );
    Go( &SCH_EDITOR_CONTROL::CrossProbeToPcb,       EVENTS::UnselectedEvent );
    Go( &SCH_EDITOR_CONTROL::CrossProbeToPcb,       EVENTS::ClearedEvent );
    Go( &SCH_EDITOR_CONTROL::ExplicitCrossProbeToPcb, EE_ACTIONS::explicitCrossProbe.MakeEvent() );

#ifdef KICAD_SPICE
    Go( &SCH_EDITOR_CONTROL::SimProbe,              EE_ACTIONS::simProbe.MakeEvent() );
    Go( &SCH_EDITOR_CONTROL::SimTune,               EE_ACTIONS::simTune.MakeEvent() );
#endif /* KICAD_SPICE */

    Go( &SCH_EDITOR_CONTROL::HighlightNet,          EE_ACTIONS::highlightNet.MakeEvent() );
    Go( &SCH_EDITOR_CONTROL::ClearHighlight,        EE_ACTIONS::clearHighlight.MakeEvent() );
    Go( &SCH_EDITOR_CONTROL::HighlightNetCursor,    EE_ACTIONS::highlightNetCursor.MakeEvent() );
    Go( &SCH_EDITOR_CONTROL::UpdateNetHighlighting, EVENTS::SelectedItemsModified );
    Go( &SCH_EDITOR_CONTROL::UpdateNetHighlighting, EE_ACTIONS::updateNetHighlighting.MakeEvent() );

    Go( &SCH_EDITOR_CONTROL::Cut,                   EE_ACTIONS::cut.MakeEvent() );
    Go( &SCH_EDITOR_CONTROL::Copy,                  EE_ACTIONS::copy.MakeEvent() );
    Go( &SCH_EDITOR_CONTROL::Paste,                 EE_ACTIONS::paste.MakeEvent() );

    Go( &SCH_EDITOR_CONTROL::EditWithSymbolEditor,  EE_ACTIONS::editWithSymbolEditor.MakeEvent() );
    Go( &SCH_EDITOR_CONTROL::ShowLibraryBrowser,    EE_ACTIONS::showLibraryBrowser.MakeEvent() );

    Go( &SCH_EDITOR_CONTROL::EnterSheet,            EE_ACTIONS::enterSheet.MakeEvent() );
    Go( &SCH_EDITOR_CONTROL::LeaveSheet,            EE_ACTIONS::leaveSheet.MakeEvent() );
    Go( &SCH_EDITOR_CONTROL::NavigateHierarchy,     EE_ACTIONS::navigateHierarchy.MakeEvent() );

    Go( &SCH_EDITOR_CONTROL::ToggleHiddenPins,      EE_ACTIONS::toggleHiddenPins.MakeEvent() );
}
