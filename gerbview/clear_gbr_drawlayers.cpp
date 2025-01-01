/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2016 Jean-Pierre Charras, jp.charras at wanadoo.fr
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

/**
 * @file clear_gbr_drawlayers.cpp
 * @brief erase a given or all draw layers, an free memory relative to the cleared layer(s)
 */

#include <confirm.h>
#include <gerbview_frame.h>
#include <gerber_file_image.h>
#include <gerber_file_image_list.h>
#include <view/view.h>
#include <base_screen.h>
#include "widgets/gerbview_layer_widget.h"

#include <tool/tool_manager.h>

bool GERBVIEW_FRAME::Clear_DrawLayers( bool query )
{
    if( GetGerberLayout() == nullptr )
        return false;

    if( query && GetScreen()->IsContentModified() )
    {
        if( !IsOK( this, _( "Current data will be lost?" ) ) )
            return false;
    }

    if( GetCanvas() )
    {
        if( m_toolManager )
            m_toolManager->ResetTools( TOOL_BASE::MODEL_RELOAD );

        GetCanvas()->GetView()->Clear();

        // Reinit the drawing-sheet view, cleared by GetView()->Clear():
        SetPageSettings( GetPageSettings() );
    }

    GetImagesList()->DeleteAllImages();

    GetGerberLayout()->SetBoundingBox( BOX2I() );

    SetActiveLayer( 0 );
    ReFillLayerWidget();
    syncLayerBox();
    return true;
}


void GERBVIEW_FRAME::Erase_Current_DrawLayer( bool query )
{
    int layer = GetActiveLayer();
    wxString msg;

    msg.Printf( _( "Clear layer %d?" ), layer + 1 );

    if( query && !IsOK( this, msg ) )
        return;

    if( m_toolManager )
        m_toolManager->ResetTools( TOOL_BASE::MODEL_RELOAD );

    RemapLayers( GetImagesList()->RemoveImage( layer ) );

    ReFillLayerWidget();
    syncLayerBox();
    GetCanvas()->Refresh();
}
