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

#include <base_units.h>
#include <base_screen.h>
#include <gerbview_frame.h>
#include <gerber_file_image.h>
#include <gerber_file_image_list.h>
#include "gerbview_printout.h"
#include <view/view.h>
#include <gerbview_painter.h>
#include <math/util.h>      // for KiROUND


GERBVIEW_PRINTOUT::GERBVIEW_PRINTOUT( GBR_LAYOUT* aLayout, const BOARD_PRINTOUT_SETTINGS& aParams,
                                      const KIGFX::VIEW* aView, const wxString& aTitle ) :
    BOARD_PRINTOUT( aParams, aView, aTitle )
{
    m_layout = aLayout;
}


bool GERBVIEW_PRINTOUT::OnPrintPage( int aPage )
{
    // Store the layerset, as it is going to be modified below and the original settings are needed
    LSET lset = m_settings.m_LayerSet;

    LSEQ seq = lset.UIOrder();
    wxCHECK( unsigned( aPage - 1 ) < seq.size(), false );
    auto layerId = seq[aPage - 1];

    // In gerbview, draw layers are always printed on separate pages because handling negative
    // objects when using only one page is tricky

    // Enable only one layer to create a printout
    m_settings.m_LayerSet = LSET( layerId );

    GERBER_FILE_IMAGE_LIST& gbrImgList = GERBER_FILE_IMAGE_LIST::GetImagesList();
    GERBER_FILE_IMAGE*      gbrImage = gbrImgList.GetGbrImage( layerId );
    wxString                gbr_filename;

    if( gbrImage )
        gbr_filename = gbrImage->m_FileName;

    DrawPage( gbr_filename, aPage, m_settings.m_pageCount );

    // Restore the original layer set, so the next page can be printed
    m_settings.m_LayerSet = lset;

    return true;
}


int GERBVIEW_PRINTOUT::milsToIU( double aMils ) const
{
    return KiROUND( IU_PER_MILS * aMils );
}


void GERBVIEW_PRINTOUT::setupViewLayers( KIGFX::VIEW& aView, const LSET& aLayerSet )
{
    BOARD_PRINTOUT::setupViewLayers( aView, aLayerSet );

    for( LSEQ layerSeq = m_settings.m_LayerSet.Seq(); layerSeq; ++layerSeq )
        aView.SetLayerVisible( GERBVIEW_LAYER_ID_START + *layerSeq, true );
}


void GERBVIEW_PRINTOUT::setupGal( KIGFX::GAL* aGal )
{
    BOARD_PRINTOUT::setupGal( aGal );
    aGal->SetWorldUnitLength( 1.0/IU_PER_MM /* 10 nm */ / 25.4 /* 1 inch in mm */ );
}


EDA_RECT GERBVIEW_PRINTOUT::getBoundingBox()
{
    return m_layout->ComputeBoundingBox();
}


std::unique_ptr<KIGFX::PAINTER> GERBVIEW_PRINTOUT::getPainter( KIGFX::GAL* aGal )
{
    return std::make_unique<KIGFX::GERBVIEW_PAINTER>( aGal );
}
