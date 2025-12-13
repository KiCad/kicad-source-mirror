/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2023 Jean-Pierre Charras  jp.charras at wanadoo.fr
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

#include <gerbview_frame.h>
#include <gerber_file_image.h>
#include <gerber_file_image_list.h>
#include <dialog_draw_layers_settings.h>
#include <widgets/wx_grid.h>


DIALOG_DRAW_LAYERS_SETTINGS::DIALOG_DRAW_LAYERS_SETTINGS( GERBVIEW_FRAME* aParent ) :
    DIALOG_DRAW_LAYERS_SETTINGS_BASE( aParent ),
    m_parent( aParent ),
    m_offsetX( aParent, m_stOffsetX, m_tcOffsetX, m_stUnitX, true ),
    m_offsetY( aParent, m_stOffsetY, m_tcOffsetY, m_stUnitY, true ),
    m_rotation( aParent, m_stLayerRot, m_tcRotation, m_stUnitRot, true )
{
    m_rotation.SetUnits( EDA_UNITS::DEGREES );
    m_rotation.SetPrecision( 3 );

    SetupStandardButtons();
    finishDialogSettings();
}


bool DIALOG_DRAW_LAYERS_SETTINGS::TransferDataToWindow()
{
    GERBER_FILE_IMAGE* gbrImage = m_parent->GetGbrImage( m_parent->GetActiveLayer() );

    if( !gbrImage )
        return true;

    wxFileName filename( gbrImage->m_FileName );
    m_stLayerName->SetLabel( filename.GetFullName() );

    m_offsetX.SetValue( gbrImage->m_DisplayOffset.x );
    m_offsetY.SetValue( gbrImage->m_DisplayOffset.y );
    m_rotation.SetValue( gbrImage->m_DisplayRotation.AsDegrees() );

    return true;
}

bool DIALOG_DRAW_LAYERS_SETTINGS::TransferDataFromWindow()
{
    std::vector <GERBER_FILE_IMAGE*> gbrCandidates;
    GERBER_FILE_IMAGE* gbrImage;
    GERBER_FILE_IMAGE_LIST* images = m_parent->GetGerberLayout()->GetImagesList();

    switch( m_rbScope->GetSelection() )
    {
    case 0:     // candidate = active layer
        gbrImage = m_parent->GetGbrImage( m_parent->GetActiveLayer() );

        if( gbrImage )
            gbrCandidates.push_back( gbrImage );

        break;

    case 1:     // All layers
        for( unsigned layer = 0; layer < images->ImagesMaxCount(); ++layer )
        {
            gbrImage = images->GetGbrImage( layer );

            if( gbrImage )
                gbrCandidates.push_back( gbrImage );
        }

        break;

    case 2:     // All active layers
        for( unsigned layer = 0; layer < images->ImagesMaxCount(); ++layer )
        {
            gbrImage = images->GetGbrImage( layer );

            if( gbrImage && m_parent->IsLayerVisible( layer ) )
                gbrCandidates.push_back( gbrImage );
        }

        break;
    }

    // Now update all candidates
    for( unsigned ii = 0; ii < gbrCandidates.size(); ++ii )
    {
        gbrImage = gbrCandidates[ii];
        double offsetX = m_offsetX.GetValue();
        double offsetY = m_offsetY.GetValue();
        EDA_ANGLE rot = m_rotation.GetAngleValue();

        gbrImage->SetDrawOffetAndRotation( VECTOR2D( offsetX/gerbIUScale.IU_PER_MM,
                                                     offsetY/gerbIUScale.IU_PER_MM ), rot );
    }

    return true;
}