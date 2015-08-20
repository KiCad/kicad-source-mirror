/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 1992-2012 KiCad Developers, see AUTHORS.txt for contributors.
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

/**
 * @file edit.cpp
 * @brief  Module editor dialog box for editing module properties and characteristics.
 */

#include <fctsys.h>
#include <kiface_i.h>
#include <kiway.h>
#include <confirm.h>
#include <class_drawpanel.h>
#include <pcbnew.h>
#include <wxPcbStruct.h>
#include <module_editor_frame.h>
#include <3d_viewer.h>

#include <class_module.h>
#include <class_pad.h>
#include <class_edge_mod.h>

#include <dialog_edit_module_for_BoardEditor.h>


/*
 * Show module property dialog.
 */
void PCB_EDIT_FRAME::InstallModuleOptionsFrame( MODULE* Module, wxDC* DC )
{
    if( Module == NULL )
        return;

#ifdef __WXMAC__
    // avoid Avoid "writes" in the dialog, creates errors with WxOverlay and NSView & Modal
    // Raising an Exception - Fixes #764678
    DC = NULL;
#endif

    DIALOG_MODULE_BOARD_EDITOR* dlg = new DIALOG_MODULE_BOARD_EDITOR( this, Module, DC );

    int retvalue = dlg->ShowModal();
    /* retvalue =
     *  FP_PRM_EDITOR_RETVALUE::PRM_EDITOR_ABORT if abort,
     *  FP_PRM_EDITOR_RETVALUE::PRM_EDITOR_WANT_EXCHANGE_FP if exchange module,
     *  FP_PRM_EDITOR_RETVALUE::PRM_EDITOR_EDIT_OK for normal edition
     *  FP_PRM_EDITOR_RETVALUE::PRM_EDITOR_WANT_MODEDIT for a goto editor command
     */
    dlg->Destroy();

#ifdef __WXMAC__
    // If something edited, push a refresh request
    if( retvalue == DIALOG_MODULE_BOARD_EDITOR::PRM_EDITOR_EDIT_OK )
        m_canvas->Refresh();
#endif

    if( retvalue == DIALOG_MODULE_BOARD_EDITOR::PRM_EDITOR_WANT_MODEDIT )
    {
        FOOTPRINT_EDIT_FRAME* editor = (FOOTPRINT_EDIT_FRAME*) Kiway().Player( FRAME_PCB_MODULE_EDITOR, true );

        editor->Load_Module_From_BOARD( Module );
        SetCurItem( NULL );

        editor->Show( true );
        editor->Raise();        // Iconize( false );
    }

    if( retvalue == DIALOG_MODULE_BOARD_EDITOR::PRM_EDITOR_WANT_EXCHANGE_FP )
    {
        InstallExchangeModuleFrame( Module );
        // Warning: the current item can be deleted by exchange module
        SetCurItem( NULL );
    }
}


void FOOTPRINT_EDIT_FRAME::RemoveStruct( EDA_ITEM* Item )
{
    if( Item == NULL )
        return;

    switch( Item->Type() )
    {
    case PCB_PAD_T:
        DeletePad( (D_PAD*) Item, false );
        break;

    case PCB_MODULE_TEXT_T:
    {
        TEXTE_MODULE* text = static_cast<TEXTE_MODULE*>( Item );

        switch( text->GetType() )
        {
        case TEXTE_MODULE::TEXT_is_REFERENCE:
            DisplayError( this, _( "Cannot delete REFERENCE!" ) );
            break;

        case TEXTE_MODULE::TEXT_is_VALUE:
            DisplayError( this, _( "Cannot delete VALUE!" ) );
            break;

        case TEXTE_MODULE::TEXT_is_DIVERS:
            DeleteTextModule( text );
        }
    }
    break;

    case PCB_MODULE_EDGE_T:
        Delete_Edge_Module( (EDGE_MODULE*) Item );
        m_canvas->Refresh();
        break;

    case PCB_MODULE_T:
        break;

    default:
    {
        wxString Line;
        Line.Printf( wxT( " RemoveStruct: item type %d unknown." ), Item->Type() );
        wxMessageBox( Line );
    }
    break;
    }
}
