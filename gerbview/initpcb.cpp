/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2007 Jean-Pierre Charras, jaen-pierre.charras@gipsa-lab.inpg.com
 * Copyright (C) 1992-2011 KiCad Developers, see AUTHORS.txt for contributors.
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
 * @file gerbview/initpcb.cpp
 */

#include "fctsys.h"
#include "common.h"
#include "class_drawpanel.h"
#include "confirm.h"

#include "gerbview.h"
#include "class_gerber_draw_item.h"
#include "class_GERBER.h"
#include "class_gerbview_layer_widget.h"


bool GERBVIEW_FRAME::Clear_Pcb( bool query )
{
    int layer;

    if( GetBoard() == NULL )
        return FALSE;

    if( query && GetScreen()->IsModify() )
    {
        if( !IsOK( this, _( "Current data will be lost?" ) ) )
            return FALSE;
    }

    SetCurItem( NULL );
    GetBoard()->m_Drawings.DeleteAll();

    for( layer = 0; layer < LAYER_COUNT; layer++ )
    {
        if( g_GERBER_List[layer] )
        {
            g_GERBER_List[layer]->InitToolTable();
            g_GERBER_List[layer]->ResetDefaultValues();
        }
    }

    GetBoard()->SetBoundingBox( EDA_RECT() );

    GetBoard()->m_Status_Pcb  = 0;
    GetBoard()->m_NbNodes     = 0;
    GetBoard()->m_NbNoconnect = 0;

    SetScreen( new PCB_SCREEN() );
    GetScreen()->Init();

    setActiveLayer(FIRST_COPPER_LAYER);
    m_LayersManager->UpdateLayerIcons();
    syncLayerBox();
    return true;
}


void GERBVIEW_FRAME::Erase_Current_Layer( bool query )
{
    int      layer = getActiveLayer();
    wxString msg;

    msg.Printf( _( "Clear layer %d?" ), layer + 1 );

    if( query && !IsOK( this, msg ) )
        return;

    SetCurItem( NULL );

    BOARD_ITEM* item = GetBoard()->m_Drawings;
    BOARD_ITEM * next;

    for( ; item; item = next )
    {
        next = item->Next();
        GERBER_DRAW_ITEM* gerb_item = (GERBER_DRAW_ITEM*) item;

        if( gerb_item->GetLayer() != layer )
            continue;

        gerb_item->DeleteStructure();
    }

    if( g_GERBER_List[layer] )
    {
        g_GERBER_List[layer]->InitToolTable();
        g_GERBER_List[layer]->ResetDefaultValues();
    }

    GetScreen()->SetModify();
    m_canvas->Refresh();
    m_LayersManager->UpdateLayerIcons();
    syncLayerBox();
}
