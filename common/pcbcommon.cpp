/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2008 Wayne Stambaugh <stambaughw@verizon.net>
 * Copyright (C) 1992-2008 KiCad Developers, see change_log.txt for contributors.
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

/*
 * This file contains some functions used in the PCB
 * applications Pcbnew and CvPcb.
 */

#include <fctsys.h>
#include <pgm_base.h>
#include <kiface_i.h>

#include <pcbcommon.h>
#include <class_board.h>
#include <3d_viewer.h>


wxString LayerMaskDescribe( const BOARD *aBoard, LSET aMask )
{
    // Try the single or no- layer case (easy)
    LAYER_ID layer = aMask.ExtractLayer();

    switch( (int) layer )
    {
    case UNSELECTED_LAYER:
        return _( "No layers" );

    case UNDEFINED_LAYER:
        break;

    default:
        return aBoard->GetLayerName( layer );
    }

    // Try to be smart and useful, starting with outer copper
    // (which are more important than internal ones)
    wxString layerInfo;

    if( aMask[F_Cu] )
        AccumulateDescription( layerInfo, aBoard->GetLayerName( F_Cu ) );

    if( aMask[B_Cu] )
        AccumulateDescription( layerInfo, aBoard->GetLayerName( B_Cu ) );

    if( ( aMask & LSET::InternalCuMask() ).any() )
        AccumulateDescription( layerInfo, _("Internal" ) );

    if( ( aMask & LSET::AllNonCuMask() ).any() )
        AccumulateDescription( layerInfo, _("Non-copper" ) );

    return layerInfo;
}

