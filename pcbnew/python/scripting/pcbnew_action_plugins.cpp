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

#include "pcbnew_action_plugins.h"
#include <pgm_base.h>
#include <settings/settings_manager.h>
#include <pcbnew_settings.h>
#include <bitmaps.h>
#include <board.h>
#include <board_commit.h>
#include <board_design_settings.h>
#include <footprint.h>
#include <gal/graphics_abstraction_layer.h>
#include <pcb_track.h>
#include <zone.h>
#include <pcbnew_settings.h>
#include <tool/action_menu.h>
#include <tool/action_toolbar.h>
#include <tool/tool_manager.h>
#include <tools/pcb_actions.h>
#include <tools/pcb_selection_tool.h>
#include <pcb_painter.h>
#include <wx/msgdlg.h>
#include <wx/app.h>
#include <kiplatform/app.h>
#include "../../scripting/python_scripting.h"

PYTHON_ACTION_PLUGIN::PYTHON_ACTION_PLUGIN( PyObject* aAction )
{
    PyLOCK lock;

    m_PyAction = aAction;
    Py_XINCREF( aAction );
}


PYTHON_ACTION_PLUGIN::~PYTHON_ACTION_PLUGIN()
{
    PyLOCK lock;

    Py_XDECREF( m_PyAction );
}


PyObject* PYTHON_ACTION_PLUGIN::CallMethod( const char* aMethod, PyObject* aArglist )
{
    PyLOCK lock;

    PyErr_Clear();

    // pFunc is a new reference to the desired method
    PyObject* pFunc = PyObject_GetAttrString( m_PyAction, aMethod );

    if( pFunc && PyCallable_Check( pFunc ) )
    {
        PyObject* result = PyObject_CallObject( pFunc, aArglist );

        if( !wxTheApp )
            KIPLATFORM::APP::AttachConsole( true );

        if( PyErr_Occurred() )
        {
            wxString message = _HKI( "Exception on python action plugin code" );
            wxString traceback = PyErrStringWithTraceback();

            std::cerr << message << std::endl << std::endl;
            std::cerr << traceback << std::endl;

            if( wxTheApp )
                wxSafeShowMessage( wxGetTranslation( message ), traceback );
        }

        if( !wxTheApp )
        {
            wxArrayString messages;
            messages.Add( "Fatal error");
            messages.Add( wxString::Format(
                    "The application handle was destroyed after running Python plugin '%s'.",
                    m_cachedName ) );

            // Poor man's ASCII message box
            {
                int maxLen = 0;

                for( const wxString& msg : messages )
                    if( (int)msg.length() > maxLen )
                        maxLen = msg.length();

                wxChar   ch = '*';
                wxString border( ch, 3 );

                std::cerr << wxString( ch, maxLen + 2 + border.length() * 2 ) << std::endl;
                std::cerr << border << ' ' << wxString( ' ', maxLen ) << ' ' << border << std::endl;

                for( wxString msg : messages )
                    std::cerr << border << ' ' << msg.Pad( maxLen - msg.length() ) << ' ' << border
                              << std::endl;

                std::cerr << border << ' ' << wxString( ' ', maxLen ) << ' ' << border << std::endl;
                std::cerr << wxString( ch, maxLen + 2 + border.length() * 2 ) << std::endl;
            }

#ifdef _WIN32
            std::cerr << std::endl << "Press any key to abort..." << std::endl;
            (void) std::getchar();
#endif

            abort();
        }

        if( result )
        {
            Py_XDECREF( pFunc );
            return result;
        }
    }
    else
    {
        wxString msg = wxString::Format( _( "Method '%s' not found, or not callable" ), aMethod );
        wxMessageBox( msg, _( "Unknown Method" ), wxICON_ERROR | wxOK );
    }

    if( pFunc )
    {
        Py_XDECREF( pFunc );
    }

    return nullptr;
}


wxString PYTHON_ACTION_PLUGIN::CallRetStrMethod( const char* aMethod, PyObject* aArglist )
{
    wxString    ret;
    PyLOCK      lock;

    PyObject* result = CallMethod( aMethod, aArglist );

    ret = PyStringToWx( result );
    Py_XDECREF( result );

    return ret;
}


wxString PYTHON_ACTION_PLUGIN::GetCategoryName()
{
    PyLOCK lock;

    return CallRetStrMethod( "GetCategoryName" );
}


wxString PYTHON_ACTION_PLUGIN::GetClassName()
{
    PyLOCK lock;

    return CallRetStrMethod( "GetClassName" );
}


wxString PYTHON_ACTION_PLUGIN::GetName()
{
    PyLOCK lock;

    wxString name = CallRetStrMethod( "GetName" );
    m_cachedName = name;

    return name;
}


wxString PYTHON_ACTION_PLUGIN::GetDescription()
{
    PyLOCK lock;

    return CallRetStrMethod( "GetDescription" );
}


bool PYTHON_ACTION_PLUGIN::GetShowToolbarButton()
{
    PyLOCK lock;

    PyObject* result = CallMethod( "GetShowToolbarButton");

    return PyObject_IsTrue(result);
}


wxString PYTHON_ACTION_PLUGIN::GetIconFileName( bool aDark )
{
    PyLOCK lock;

    PyObject* arglist = Py_BuildValue( "(i)", static_cast<int>( aDark ) );

    wxString result = CallRetStrMethod( "GetIconFileName", arglist );

    Py_DECREF( arglist );

    return result;
}


wxString PYTHON_ACTION_PLUGIN::GetPluginPath()
{
    PyLOCK lock;

    return CallRetStrMethod( "GetPluginPath" );
}


void PYTHON_ACTION_PLUGIN::Run()
{
    PyLOCK lock;

    CallMethod( "Run" );
}


void* PYTHON_ACTION_PLUGIN::GetObject()
{
    return (void*) m_PyAction;
}


void PYTHON_ACTION_PLUGINS::register_action( PyObject* aPyAction )
{
    PYTHON_ACTION_PLUGIN* fw = new PYTHON_ACTION_PLUGIN( aPyAction );

    fw->register_action();
}


void PYTHON_ACTION_PLUGINS::deregister_action( PyObject* aPyAction )
{
    // deregister also destroys the previously created "PYTHON_ACTION_PLUGIN object"
    ACTION_PLUGINS::deregister_object( (void*) aPyAction );
}


void PCB_EDIT_FRAME::OnActionPluginMenu( wxCommandEvent& aEvent )
{
    ACTION_PLUGIN* actionPlugin = ACTION_PLUGINS::GetActionByMenu( aEvent.GetId() );

	if( actionPlugin )
        RunActionPlugin( actionPlugin );
}


void PCB_EDIT_FRAME::OnActionPluginButton( wxCommandEvent& aEvent )
{
	ACTION_PLUGIN* actionPlugin = ACTION_PLUGINS::GetActionByButton( aEvent.GetId() );

	if( actionPlugin )
        RunActionPlugin( actionPlugin );
}

void PCB_EDIT_FRAME::RunActionPlugin( ACTION_PLUGIN* aActionPlugin )
{

    PICKED_ITEMS_LIST itemsList;
    BOARD*  currentPcb  = GetBoard();
    bool    fromEmpty   = false;

    // Append tracks:
    for( PCB_TRACK* item : currentPcb->Tracks() )
    {
        ITEM_PICKER picker( nullptr, item, UNDO_REDO::CHANGED );
        itemsList.PushItem( picker );
    }

    // Append footprints:
    for( FOOTPRINT* item : currentPcb->Footprints() )
    {
        ITEM_PICKER picker( nullptr, item, UNDO_REDO::CHANGED );
        itemsList.PushItem( picker );
    }

    // Append drawings
    for( BOARD_ITEM* item : currentPcb->Drawings() )
    {
        ITEM_PICKER picker( nullptr, item, UNDO_REDO::CHANGED );
        itemsList.PushItem( picker );
    }

    // Append zones outlines
    for( ZONE* zone : currentPcb->Zones() )
    {
        ITEM_PICKER picker( nullptr, zone, UNDO_REDO::CHANGED );
        itemsList.PushItem( picker );
    }

    if( itemsList.GetCount() > 0 )
        SaveCopyInUndoList( itemsList, UNDO_REDO::CHANGED );
    else
        fromEmpty = true;

    itemsList.ClearItemsList();

    BOARD_COMMIT commit( this );

    // Execute plugin itself...
    ACTION_PLUGINS::SetActionRunning( true );
    aActionPlugin->Run();
    ACTION_PLUGINS::SetActionRunning( false );

    // Get back the undo buffer to fix some modifications
    PICKED_ITEMS_LIST* oldBuffer = nullptr;

    if( fromEmpty )
    {
        oldBuffer = new PICKED_ITEMS_LIST();
    }
    else
    {
        oldBuffer = PopCommandFromUndoList();
        wxASSERT( oldBuffer );
    }

    // Try do discover what was modified
    PICKED_ITEMS_LIST deletedItemsList;

    // The list of existing items after running the action script
    const auto currItemList = currentPcb->GetItemSet();

    // Found deleted items
    for( unsigned int i = 0; i < oldBuffer->GetCount(); i++ )
    {
        BOARD_ITEM* item = (BOARD_ITEM*) oldBuffer->GetPickedItem( i );
        ITEM_PICKER picker( nullptr, item, UNDO_REDO::DELETED );

        wxASSERT( item );

        if( currItemList.find( item ) == currItemList.end() )
        {
            deletedItemsList.PushItem( picker );
            commit.Removed( item );
        }
    }

    // Mark deleted elements in undolist

    for( unsigned int i = 0; i < deletedItemsList.GetCount(); i++ )
    {
        oldBuffer->PushItem( deletedItemsList.GetItemWrapper( i ) );
    }

    // Find new footprints
    for( FOOTPRINT* item : currentPcb->Footprints() )
    {
        if( !oldBuffer->ContainsItem( item ) )
        {
            ITEM_PICKER picker( nullptr, item, UNDO_REDO::NEWITEM );
            oldBuffer->PushItem( picker );
            commit.Added( item );
        }
    }

    for( PCB_TRACK* item : currentPcb->Tracks() )
    {
        if( !oldBuffer->ContainsItem( item ) )
        {
            ITEM_PICKER picker( nullptr, item, UNDO_REDO::NEWITEM );
            oldBuffer->PushItem( picker );
            commit.Added( item );
        }
    }

    for( BOARD_ITEM* item : currentPcb->Drawings() )
    {
        if( !oldBuffer->ContainsItem( item ) )
        {
            ITEM_PICKER picker( nullptr, item, UNDO_REDO::NEWITEM );
            oldBuffer->PushItem( picker );
            commit.Added( item );
        }
    }

    for( ZONE* zone : currentPcb->Zones() )
    {
        if( !oldBuffer->ContainsItem( zone ) )
        {
            ITEM_PICKER picker( nullptr, zone, UNDO_REDO::NEWITEM );
            oldBuffer->PushItem( picker );
            commit.Added( zone );
        }
    }

    if( oldBuffer->GetCount() )
    {
        OnModify();
        PushCommandToUndoList( oldBuffer );
    }
    else
    {
        delete oldBuffer;
    }

    // Apply changes, UndoList already handled
    commit.Push( _( "Apply Action Script" ), SKIP_UNDO | SKIP_SET_DIRTY );

    RebuildAndRefresh();
}


void PCB_EDIT_FRAME::RebuildAndRefresh()
{
    // The list of existing items after running the action script
    const BOARD_ITEM_SET items = GetBoard()->GetItemSet();

    // Sync selection with items selection state
    SELECTION&          selection = GetCurrentSelection();
    PCB_SELECTION_TOOL* selTool = m_toolManager->GetTool<PCB_SELECTION_TOOL>();
    EDA_ITEMS           to_add;
    EDA_ITEMS           to_remove;

    for( BOARD_ITEM* item : items )
    {
        if( item->IsSelected() && !selection.Contains( item ) )
        {
            item->ClearSelected(); // temporarily
            to_add.push_back( item );
        }
    }

    for( EDA_ITEM* item : selection.GetItems() )
    {
        if( !item->IsSelected() && item->IsBOARD_ITEM() )
            to_remove.push_back( static_cast<BOARD_ITEM*>( item ) );
    }

    if( !to_add.empty() )
        selTool->AddItemsToSel( &to_add );

    if( !to_remove.empty() )
        selTool->RemoveItemsFromSel( &to_remove );

    m_pcb->BuildConnectivity();

    PCB_DRAW_PANEL_GAL* canvas = GetCanvas();

    canvas->GetView()->Clear();
    canvas->GetView()->InitPreview();
    canvas->GetGAL()->SetGridOrigin( VECTOR2D( m_pcb->GetDesignSettings().GetGridOrigin() ) );
    canvas->DisplayBoard( m_pcb );

    // allow tools to re-add their view items (selection previews, grids, etc.)
    if( m_toolManager )
        m_toolManager->ResetTools( TOOL_BASE::REDRAW );

    // reload the drawing-sheet
    SetPageSettings( m_pcb->GetPageSettings() );

    canvas->SyncLayersVisibility( m_pcb );

    canvas->Refresh();
}


void PCB_EDIT_FRAME::buildActionPluginMenus( ACTION_MENU* actionMenu )
{
    if( !actionMenu ) // Should not occur.
        return;

    for( int ii = 0; ii < ACTION_PLUGINS::GetActionsCount(); ii++ )
    {
        wxMenuItem* item;
        ACTION_PLUGIN* ap = ACTION_PLUGINS::GetAction( ii );
        const wxBitmap& bitmap = ap->iconBitmap.IsOk() ? ap->iconBitmap :
                                                         KiBitmap( BITMAPS::puzzle_piece );

        item = KIUI::AddMenuItem( actionMenu, wxID_ANY, ap->GetName(), ap->GetDescription(),
                                  bitmap );

        Connect( item->GetId(), wxEVT_COMMAND_MENU_SELECTED,
                 wxCommandEventHandler( PCB_EDIT_FRAME::OnActionPluginMenu ) );

        ACTION_PLUGINS::SetActionMenu( ii, item->GetId() );
    }
}


void PCB_EDIT_FRAME::addActionPluginTools( ACTION_TOOLBAR* aToolbar )
{
    bool need_separator = true;
    const std::vector<LEGACY_OR_API_PLUGIN>& orderedPlugins = GetOrderedActionPlugins();

    for( const auto& entry : orderedPlugins )
    {
        // API plugins are handled by EDA_BASE_FRAME
        if( !std::holds_alternative<ACTION_PLUGIN*>( entry ) )
            continue;

        ACTION_PLUGIN* ap = std::get<ACTION_PLUGIN*>( entry );

        if( GetActionPluginButtonVisible( ap->GetPluginPath(), ap->GetShowToolbarButton() ) )
        {
            if( need_separator )
            {
                aToolbar->AddScaledSeparator( this );
                need_separator = false;
            }

            // Add button
            wxBitmap bitmap;

            if ( ap->iconBitmap.IsOk() )
                bitmap = KiScaledBitmap( ap->iconBitmap, this );
            else
                bitmap = KiScaledBitmap( BITMAPS::puzzle_piece, this );

            wxAuiToolBarItem* button = aToolbar->AddTool( wxID_ANY, wxEmptyString,
                                                               bitmap, ap->GetName() );

            Connect( button->GetId(), wxEVT_COMMAND_MENU_SELECTED,
                     wxCommandEventHandler( PCB_EDIT_FRAME::OnActionPluginButton ) );

            // Link action plugin to button
            ACTION_PLUGINS::SetActionButton( ap, button->GetId() );
        }
    }
}


std::vector<LEGACY_OR_API_PLUGIN> PCB_EDIT_FRAME::GetOrderedActionPlugins()
{
    PCBNEW_SETTINGS*                  cfg = GetAppSettings<PCBNEW_SETTINGS>( "pcbnew" );
    std::vector<ACTION_PLUGIN*>       plugins;
    std::vector<LEGACY_OR_API_PLUGIN> orderedPlugins;

    for( int i = 0; i < ACTION_PLUGINS::GetActionsCount(); i++ )
        plugins.push_back( ACTION_PLUGINS::GetAction( i ) );

    // First add plugins that have entries in settings
    if( cfg )
    {
        for( const auto& [path, show] : cfg->m_VisibleActionPlugins )
        {
            auto loc = std::find_if( plugins.begin(), plugins.end(),
                    [&path] ( ACTION_PLUGIN* plugin )
                    {
                        return plugin->GetPluginPath() == path;
                    } );

            if( loc != plugins.end() )
            {
                orderedPlugins.push_back( *loc );
                plugins.erase( loc );
            }
        }
    }

    // Now append new plugins that have not been configured yet
    for( ACTION_PLUGIN* remaining_plugin : plugins )
        orderedPlugins.push_back( remaining_plugin );

    // Finally append API plugins
    for( const PLUGIN_ACTION* action : GetOrderedPluginActions( PLUGIN_ACTION_SCOPE::PCB, cfg ) )
        orderedPlugins.push_back( action );

    return orderedPlugins;
}


bool PCB_EDIT_FRAME::GetActionPluginButtonVisible( const wxString& aPluginPath, bool aPluginDefault )
{
    if( PCBNEW_SETTINGS* cfg = GetAppSettings<PCBNEW_SETTINGS>( "pcbnew" ) )
    {
        for( const auto& [path, visible] : cfg->m_VisibleActionPlugins )
        {
            if( path == aPluginPath )
                return visible;
        }

        for( const auto& [identifier, visible] : cfg->m_Plugins.actions )
        {
            if( identifier == aPluginPath )
                return visible;
        }
    }

    // Plugin is not in settings, return default.
    return  aPluginDefault;
}
