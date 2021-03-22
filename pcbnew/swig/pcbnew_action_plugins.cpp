/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2017-2021 KiCad Developers, see AUTHORS.txt for contributors.
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
#include <bitmaps.h>
#include <board.h>
#include <footprint.h>
#include <track.h>
#include <zone.h>
#include <menus_helpers.h>
#include <pcbnew_settings.h>
#include <python_scripting.h>
#include <tool/action_menu.h>
#include <tool/action_toolbar.h>

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

        if( PyErr_Occurred() )
        {
            wxMessageBox( PyErrStringWithTraceback(),
                    _( "Exception on python action plugin code" ),
                    wxICON_ERROR | wxOK );
        }

        if( result )
        {
            Py_XDECREF( pFunc );
            return result;
        }
    }
    else
    {
        wxString msg = wxString::Format( _( "Method \"%s\" not found, or not callable" ), aMethod );
        wxMessageBox( msg, _( "Unknown Method" ), wxICON_ERROR | wxOK );
    }

    if( pFunc )
    {
        Py_XDECREF( pFunc );
    }

    return NULL;
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


wxString PYTHON_ACTION_PLUGIN::GetName()
{
    PyLOCK lock;

    return CallRetStrMethod( "GetName" );
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


wxString PYTHON_ACTION_PLUGIN::GetIconFileName( bool dark )
{
    PyLOCK lock;

    PyObject* arglist = Py_BuildValue( "(i)", (int) dark );

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


#if defined(KICAD_SCRIPTING) && defined(KICAD_SCRIPTING_ACTION_MENU)

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
    for( TRACK* item : currentPcb->Tracks() )
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
    PICKED_ITEMS_LIST* oldBuffer = NULL;

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
    std::set<BOARD_ITEM*> currItemList;

    // Append tracks:
    for( TRACK* item : currentPcb->Tracks() )
        currItemList.insert( item );

    // Append footprints:
    for( FOOTPRINT* item : currentPcb->Footprints() )
        currItemList.insert( item );

    // Append drawings
    for( BOARD_ITEM* item : currentPcb->Drawings() )
        currItemList.insert( item );

    // Append zones outlines
    for( ZONE* zone : currentPcb->Zones() )
        currItemList.insert( zone );

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

    for( TRACK* item : currentPcb->Tracks() )
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

    commit.Push( _( "Apply action script" ) );
    ActivateGalCanvas();
}


void PCB_EDIT_FRAME::buildActionPluginMenus( ACTION_MENU* actionMenu )
{
    if( !actionMenu ) // Should not occur.
        return;

    for( int ii = 0; ii < ACTION_PLUGINS::GetActionsCount(); ii++ )
    {
        wxMenuItem* item;
        ACTION_PLUGIN* ap = ACTION_PLUGINS::GetAction( ii );
        const wxBitmap& bitmap = ap->iconBitmap.IsOk() ? ap->iconBitmap : KiBitmap( BITMAPS::puzzle_piece );

        item = AddMenuItem( actionMenu, wxID_ANY,  ap->GetName(), ap->GetDescription(), bitmap );

        Connect( item->GetId(), wxEVT_COMMAND_MENU_SELECTED,
                wxCommandEventHandler( PCB_EDIT_FRAME::OnActionPluginMenu ) );

        ACTION_PLUGINS::SetActionMenu( ii, item->GetId() );
    }
}


void PCB_EDIT_FRAME::AddActionPluginTools()
{
    bool need_separator = true;
    const std::vector<ACTION_PLUGIN*>& orderedPlugins = GetOrderedActionPlugins();

    for( ACTION_PLUGIN* ap : orderedPlugins )
    {
        if( GetActionPluginButtonVisible( ap->GetPluginPath(), ap->GetShowToolbarButton() ) )
        {
            if( need_separator )
            {
                m_mainToolBar->AddScaledSeparator( this );
                need_separator = false;
            }

            // Add button
            wxBitmap bitmap;

            if ( ap->iconBitmap.IsOk() )
                bitmap = KiScaledBitmap( ap->iconBitmap, this );
            else
                bitmap = KiScaledBitmap( BITMAPS::puzzle_piece, this );

            wxAuiToolBarItem* button = m_mainToolBar->AddTool(
                    wxID_ANY, wxEmptyString, bitmap, ap->GetName() );

            Connect( button->GetId(), wxEVT_COMMAND_MENU_SELECTED,
                    wxCommandEventHandler( PCB_EDIT_FRAME::OnActionPluginButton ) );

            // Link action plugin to button
            ACTION_PLUGINS::SetActionButton( ap, button->GetId() );
        }
    }
}


std::vector<ACTION_PLUGIN*> PCB_EDIT_FRAME::GetOrderedActionPlugins()
{
    std::vector<ACTION_PLUGIN*> plugins;
    std::vector<ACTION_PLUGIN*> orderedPlugins;

    for( int i = 0; i < ACTION_PLUGINS::GetActionsCount(); i++ )
        plugins.push_back( ACTION_PLUGINS::GetAction( i ) );

    // First add plugins that have entries in settings
    for( const auto& pair : m_settings->m_VisibleActionPlugins )
    {
        auto loc = std::find_if( plugins.begin(), plugins.end(),
                [pair] ( ACTION_PLUGIN* plugin )
                {
                    return plugin->GetPluginPath() == pair.first;
                } );

        if( loc != plugins.end() )
        {
            orderedPlugins.push_back( *loc );
            plugins.erase( loc );
        }
    }

    // Now append new plugins that have not been configured yet
    for( auto remaining_plugin : plugins )
        orderedPlugins.push_back( remaining_plugin );

    return orderedPlugins;
}


bool PCB_EDIT_FRAME::GetActionPluginButtonVisible( const wxString& aPluginPath, bool aPluginDefault )
{
    auto& settings = m_settings->m_VisibleActionPlugins;

    for( const auto& entry : settings )
    {
        if( entry.first == aPluginPath )
            return entry.second;
    }

    // Plugin is not in settings, return default.
    return  aPluginDefault;
}


#endif
