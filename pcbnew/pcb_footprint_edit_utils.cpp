/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2015 Jean-Pierre Charras, jean-pierre.charras@ujf-grenoble.fr
 * Copyright (C) 2012 SoftPLC Corporation, Dick Hollenbeck <dick@softplc.com>
 * Copyright (C) 2012 Wayne Stambaugh <stambaughw@verizon.net>
 * Copyright (C) 1992-2015 KiCad Developers, see AUTHORS.txt for contributors.
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
#include <gr_basic.h>
#include <class_drawpanel.h>
#include <confirm.h>
#include <pcb_edit_frame.h>
#include <trigo.h>
#include <macros.h>
#include <class_board.h>
#include <class_module.h>
#include <pcbnew.h>
#include <drag.h>
#include <dialog_get_footprint_by_name.h>

#include <connectivity/connectivity_data.h>

static MODULE*           s_ModuleInitialCopy = NULL;   // Copy of module for abort/undo command

static PICKED_ITEMS_LIST s_PickedList;                 // a pick-list to save initial module
                                                       //   and dragged tracks


MODULE* PCB_BASE_FRAME::GetFootprintFromBoardByReference()
{
    wxString        moduleName;
    MODULE*         module = NULL;
    wxArrayString   fplist;

    // Build list of available fp references, to display them in dialog
    for( MODULE* fp = GetBoard()->m_Modules; fp; fp = fp->Next() )
        fplist.Add( fp->GetReference() + wxT("    ( ") + fp->GetValue() + wxT(" )") );

    fplist.Sort();

    DIALOG_GET_FOOTPRINT_BY_NAME dlg( this, fplist );

    if( dlg.ShowModal() != wxID_OK )    //Aborted by user
        return NULL;

    moduleName = dlg.GetValue();
    moduleName.Trim( true );
    moduleName.Trim( false );

    if( !moduleName.IsEmpty() )
    {
        module = GetBoard()->m_Modules;

        while( module )
        {
            if( module->GetReference().CmpNoCase( moduleName ) == 0 )
                break;

            module = module->Next();
        }
    }

    return module;
}


void PCB_BASE_FRAME::PlaceModule( MODULE* aModule, wxDC* aDC, bool aRecreateRatsnest )
{
    wxPoint newpos;

    if( aModule == 0 )
        return;

    OnModify();


    if( aModule->IsNew() )
    {
        SaveCopyInUndoList( aModule, UR_NEW );
    }
    else if( aModule->IsMoving() )
    {
        ITEM_PICKER picker( aModule, UR_CHANGED );
        picker.SetLink( s_ModuleInitialCopy );
        s_PickedList.PushItem( picker );
        s_ModuleInitialCopy = NULL;     // the picker is now owner of s_ModuleInitialCopy.
    }

    if( s_PickedList.GetCount() )
    {
        SaveCopyInUndoList( s_PickedList, UR_UNSPECIFIED );

        // Clear list, but DO NOT delete items, because they are owned by the saved undo
        // list and they therefore in use
        s_PickedList.ClearItemsList();
    }

    auto displ_opts = (PCB_DISPLAY_OPTIONS*)GetDisplayOptions();

    if( displ_opts->m_Show_Module_Ratsnest && aDC )
        TraceModuleRatsNest( aDC );

    newpos = GetCrossHairPosition();
    aModule->SetPosition( newpos );
    aModule->ClearFlags();

    delete s_ModuleInitialCopy;
    s_ModuleInitialCopy = NULL;

    if( aDC )
        aModule->Draw( m_canvas, aDC, GR_OR );

    // Redraw dragged track segments, if any
    bool isDragged = g_DragSegmentList.size() > 0;

    for( unsigned ii = 0; ii < g_DragSegmentList.size(); ii++ )
    {
        TRACK * track = g_DragSegmentList[ii].m_Track;
        track->SetState( IN_EDIT, false );
        track->ClearFlags();

        if( aDC )
            track->Draw( m_canvas, aDC, GR_OR );
    }

    // Delete drag list
    EraseDragList();

    m_canvas->SetMouseCapture( NULL, NULL );

    if( aRecreateRatsnest )
    {
        if( isDragged ) // Some tracks have positions modified: rebuild the connectivity
            m_Pcb->GetConnectivity()->Build(m_Pcb);
        else        // Only pad positions are modified: rebuild the connectivity only for this footprint (faster)
            m_Pcb->GetConnectivity()->Update( aModule );
    }

    if( ( GetBoard()->IsElementVisible( LAYER_RATSNEST ) || displ_opts->m_Show_Module_Ratsnest )
            && aRecreateRatsnest )
        Compile_Ratsnest( aDC, true );

    if( aDC )
        m_canvas->Refresh();

    SetMsgPanel( aModule );
}


/*
 * Rotate the footprint angle degrees in the direction < 0.
 * If incremental == true, the rotation is made from the last orientation,
 * If the module is placed in the absolute orientation angle.
 * If DC == NULL, the component does not redraw.
 * Otherwise, it erases and redraws turns
 */
void PCB_BASE_FRAME::Rotate_Module( wxDC* DC, MODULE* module, double angle, bool incremental )
{
    if( module == NULL )
        return;

    OnModify();

    if( !module->IsMoving() )         // This is a simple rotation, no other edit in progress
    {
        if( DC )                      // Erase footprint to screen
        {
            module->SetFlags( DO_NOT_DRAW );
            m_canvas->RefreshDrawingRect( module->GetBoundingBox() );
            module->ClearFlags( DO_NOT_DRAW );

            if( GetBoard()->IsElementVisible( LAYER_RATSNEST ) )
                DrawGeneralRatsnest( DC );
        }
    }
    else
    {
        if( DC )
        {
            module->DrawOutlinesWhenMoving( m_canvas, DC, g_Offset_Module );
            DrawSegmentWhileMovingFootprint( m_canvas, DC );
        }
    }


    if( incremental )
        module->SetOrientation( module->GetOrientation() + angle );
    else
        module->SetOrientation( angle );

    SetMsgPanel( module );
    m_Pcb->GetConnectivity()->Update( module );

    if( DC )
    {
        if( !module->IsMoving() )
        {
            //  not beiing moved: redraw the module and update ratsnest
            module->Draw( m_canvas, DC, GR_OR );

            if( GetBoard()->IsElementVisible( LAYER_RATSNEST ) )
                Compile_Ratsnest( DC, true );
        }
        else
        {
            // Beiing moved: just redraw it
            module->DrawOutlinesWhenMoving( m_canvas, DC, g_Offset_Module );
            DrawSegmentWhileMovingFootprint( m_canvas, DC );
        }

        if( module->GetEditFlags() == 0 )  // module not in edit: redraw full screen
            m_canvas->Refresh();
    }
}
