/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2016 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 1992-2016 KiCad Developers, see AUTHORS.txt for contributors.
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

#include <fctsys.h>
#include <class_drawpanel.h>
#include <confirm.h>

#include <gerbview_frame.h>
#include <class_gerber_file_image.h>
#include <class_gerber_file_image_list.h>
#include <class_gerbview_layer_widget.h>

bool GERBVIEW_FRAME::Clear_DrawLayers( bool query )
{
    if( GetGerberLayout() == NULL )
        return false;

    if( query && GetScreen()->IsModify() )
    {
        if( !IsOK( this, _( "Current data will be lost?" ) ) )
            return false;
    }

    GetImagesList()->DeleteAllImages();

    GetGerberLayout()->SetBoundingBox( EDA_RECT() );

    setActiveLayer( 0 );
    ReFillLayerWidget();
    syncLayerBox();
    return true;
}


void GERBVIEW_FRAME::Erase_Current_DrawLayer( bool query )
{
    int layer = getActiveLayer();
    wxString msg;

    msg.Printf( _( "Clear layer %d?" ), layer + 1 );

    if( query && !IsOK( this, msg ) )
        return;

    SetCurItem( NULL );

    GetImagesList()->DeleteImage( layer );

    ReFillLayerWidget();
    syncLayerBox();
    m_canvas->Refresh();
}
