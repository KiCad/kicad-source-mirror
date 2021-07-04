/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2021 KiCad Developers, see AUTHORS.txt for contributors.
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

#include "panel_3D_colors.h"
#include <widgets/color_swatch.h>
#include <3d_canvas/board_adapter.h>
#include <3d_viewer/eda_3d_viewer.h>
#include <eda_3d_viewer_settings.h>
#include <settings/settings_manager.h>
#include <board_stackup_manager/stackup_predefined_prms.h>
#include <board_stackup_manager/board_stackup.h>
#include <board_design_settings.h>
#include <pgm_base.h>


PANEL_3D_COLORS::PANEL_3D_COLORS( EDA_3D_VIEWER_FRAME* aFrame, wxWindow* aParent ) :
        PANEL_3D_COLORS_BASE( aParent ),
        m_frame( aFrame ),
        m_boardAdapter( aFrame->GetAdapter() )
{
#define ADD_COLOR( list, r, g, b, a, name ) \
    list.push_back( CUSTOM_COLOR_ITEM( r/255.0, g/255.0, b/255.0, a, name ) )

    m_backgroundTop->SetDefaultColor( COLOR4D( 0.8, 0.8, 0.9, 1.0 ) );
    m_backgroundBottom->SetDefaultColor( COLOR4D(  0.4, 0.4, 0.5, 1.0  ) );

    m_silkscreenTop->SetDefaultColor( COLOR4D( 0.9, 0.9, 0.9, 1.0 ) );
    m_silkscreenBottom->SetDefaultColor( COLOR4D( 0.9, 0.9, 0.9, 1.0 ) );

    ADD_COLOR( m_silkscreenColors, 241, 241, 241, 1.0, "White" );
    ADD_COLOR( m_silkscreenColors, 4, 18, 21, 1.0, "Dark" );
    m_silkscreenTop->SetUserColors( &m_silkscreenColors );
    m_silkscreenBottom->SetUserColors( &m_silkscreenColors );

    m_solderMaskTop->SetDefaultColor( COLOR4D( 0.1, 0.2, 0.1, 0.83 ) );
    m_solderMaskBottom->SetDefaultColor( COLOR4D( 0.1, 0.2, 0.1, 0.83 ) );

    ADD_COLOR( m_maskColors, 20, 51, 36, 0.83, "Green" );
    ADD_COLOR( m_maskColors, 91, 168, 12, 0.83, "Light Green" );
    ADD_COLOR( m_maskColors, 13, 104, 11, 0.83, "Saturated Green" );
    ADD_COLOR( m_maskColors, 181, 19, 21, 0.83, "Red" );
    ADD_COLOR( m_maskColors, 239, 53, 41, 0.83, "Red Light Orange" );
    ADD_COLOR( m_maskColors, 210, 40, 14, 0.83, "Red 2" );
    ADD_COLOR( m_maskColors, 2, 59, 162, 0.83, "Blue" );
    ADD_COLOR( m_maskColors, 54, 79, 116, 0.83, "Light blue 1" );
    ADD_COLOR( m_maskColors, 61, 85, 130, 0.83, "Light blue 2" );
    ADD_COLOR( m_maskColors, 21, 70, 80, 0.83, "Green blue (dark)" );
    ADD_COLOR( m_maskColors, 11, 11, 11, 0.83, "Black" );
    ADD_COLOR( m_maskColors, 245, 245, 245, 0.83, "White" );
    ADD_COLOR( m_maskColors, 119, 31, 91, 0.83, "Purple" );
    ADD_COLOR( m_maskColors, 32, 2, 53, 0.83, "Purple Dark" );
    m_solderMaskTop->SetUserColors( &m_maskColors );
    m_solderMaskBottom->SetUserColors( &m_maskColors );

    m_solderPaste->SetDefaultColor( COLOR4D( 0.4, 0.4, 0.4, 1.0 ) );

    ADD_COLOR( m_pasteColors, 128, 128, 128, 1.0, "grey" );
    ADD_COLOR( m_pasteColors, 213, 213, 213, 1.0, "Silver" );
    ADD_COLOR( m_pasteColors, 90, 90, 90, 1.0, "grey 2" );
    m_solderPaste->SetUserColors( &m_pasteColors );

    m_surfaceFinish->SetDefaultColor( COLOR4D( 0.75, 0.61, 0.23, 1.0 ) );

    ADD_COLOR( m_finishColors, 184, 115, 50, 1.0, "Copper" );
    ADD_COLOR( m_finishColors, 178, 156, 0, 1.0, "Gold" );
    ADD_COLOR( m_finishColors, 213, 213, 213, 1.0, "Silver" );
    ADD_COLOR( m_finishColors, 160, 160, 160, 1.0, "Tin" );
    m_surfaceFinish->SetUserColors( &m_finishColors );

    m_boardBody->SetDefaultColor( COLOR4D( 0.4, 0.4, 0.5, 0.9 ) );

    ADD_COLOR( m_boardColors, 51, 43, 22, 0.9, "FR4 natural, dark" );
    ADD_COLOR( m_boardColors, 109, 116, 75, 0.9, "FR4 natural" );
    ADD_COLOR( m_boardColors, 78, 14, 5, 0.9, "brown/red" );
    ADD_COLOR( m_boardColors, 146, 99, 47, 0.9, "brown 1" );
    ADD_COLOR( m_boardColors, 160, 123, 54, 0.9, "brown 2" );
    ADD_COLOR( m_boardColors, 146, 99, 47, 0.9, "brown 3" );
    ADD_COLOR( m_boardColors, 63, 126, 71, 0.9, "green 1" );
    ADD_COLOR( m_boardColors, 117, 122, 90, 0.9, "green 2" );
    m_boardBody->SetUserColors( &m_boardColors );

    // Only allow the stackup to be used in the PCB editor, since it isn't editable in the other
    // frames
    m_loadStackup->Show( aFrame->Parent()->IsType( FRAME_PCB_EDITOR ) );

#undef ADD_COLOR
}


bool PANEL_3D_COLORS::TransferDataToWindow()
{
    auto to_COLOR4D = []( const SFVEC4F& src )
            {
                return COLOR4D( src.r, src.g, src.b, src.a );
            };

    m_backgroundTop->SetSwatchColor( to_COLOR4D( m_boardAdapter.m_BgColorTop ), false );
    m_backgroundBottom->SetSwatchColor( to_COLOR4D( m_boardAdapter.m_BgColorBot ), false );
    m_silkscreenTop->SetSwatchColor( to_COLOR4D( m_boardAdapter.m_SilkScreenColorTop ), false );
    m_silkscreenBottom->SetSwatchColor( to_COLOR4D( m_boardAdapter.m_SilkScreenColorBot ), false );
    m_solderMaskTop->SetSwatchColor( to_COLOR4D( m_boardAdapter.m_SolderMaskColorTop ), false );
    m_solderMaskBottom->SetSwatchColor( to_COLOR4D( m_boardAdapter.m_SolderMaskColorBot ), false );
    m_solderPaste->SetSwatchColor( to_COLOR4D( m_boardAdapter.m_SolderPasteColor ), false );
    m_surfaceFinish->SetSwatchColor( to_COLOR4D( m_boardAdapter.m_CopperColor ), false );
    m_boardBody->SetSwatchColor( to_COLOR4D( m_boardAdapter.m_BoardBodyColor ), false );

    return true;
}


bool PANEL_3D_COLORS::TransferDataFromWindow()
{
    auto to_SFVEC4F = []( const COLOR4D& src )
            {
                return SFVEC4F( src.r, src.g, src.b, src.a );
            };

    m_boardAdapter.m_BgColorTop = to_SFVEC4F( m_backgroundTop->GetSwatchColor() );
    m_boardAdapter.m_BgColorBot = to_SFVEC4F( m_backgroundBottom->GetSwatchColor() );
    m_boardAdapter.m_SilkScreenColorTop = to_SFVEC4F( m_silkscreenTop->GetSwatchColor() );
    m_boardAdapter.m_SilkScreenColorBot = to_SFVEC4F( m_silkscreenBottom->GetSwatchColor() );
    m_boardAdapter.m_SolderMaskColorTop = to_SFVEC4F( m_solderMaskTop->GetSwatchColor() );
    m_boardAdapter.m_SolderMaskColorBot = to_SFVEC4F( m_solderMaskBottom->GetSwatchColor() );
    m_boardAdapter.m_SolderPasteColor = to_SFVEC4F( m_solderPaste->GetSwatchColor() );
    m_boardAdapter.m_CopperColor = to_SFVEC4F( m_surfaceFinish->GetSwatchColor() );
    m_boardAdapter.m_BoardBodyColor = to_SFVEC4F( m_boardBody->GetSwatchColor() );

    auto cfg = Pgm().GetSettingsManager().GetAppSettings<EDA_3D_VIEWER_SETTINGS>();

    if( cfg )
        m_frame->SaveSettings( cfg );

    return true;
}


void PANEL_3D_COLORS::OnLoadColorsFromBoardStackup( wxCommandEvent& event )
{
    const BOARD*           brd       = m_boardAdapter.GetBoard();
    const FAB_LAYER_COLOR* stdColors = GetColorStandardList();
    wxColour               color;

    if( brd )
    {
        const BOARD_STACKUP& stckp = brd->GetDesignSettings().GetStackupDescriptor();

        for( const BOARD_STACKUP_ITEM* stckpItem : stckp.GetList() )
        {
            wxString colorName = stckpItem->GetColor();

            if( colorName.StartsWith( "#" ) ) // This is a user defined color.
            {
                color.Set( colorName );
            }
            else
            {
                for( int i = 0; i < GetColorStandardListCount(); i++ )
                {
                    if( stdColors[i].m_ColorName == colorName )
                    {
                        color = stdColors[i].m_Color;
                        break;
                    }
                }
            }

            if( color.IsOk() )
            {
                switch( stckpItem->GetBrdLayerId() )
                {
                case F_SilkS:
                    m_boardAdapter.m_SilkScreenColorTop.r = color.Red() / 255.0;
                    m_boardAdapter.m_SilkScreenColorTop.g = color.Green() / 255.0;
                    m_boardAdapter.m_SilkScreenColorTop.b = color.Blue() / 255.0;
                    break;
                case B_SilkS:
                    m_boardAdapter.m_SilkScreenColorBot.r = color.Red() / 255.0;
                    m_boardAdapter.m_SilkScreenColorBot.g = color.Green() / 255.0;
                    m_boardAdapter.m_SilkScreenColorBot.b = color.Blue() / 255.0;
                    break;
                case F_Mask:
                    m_boardAdapter.m_SolderMaskColorTop.r = color.Red() / 255.0;
                    m_boardAdapter.m_SolderMaskColorTop.g = color.Green() / 255.0;
                    m_boardAdapter.m_SolderMaskColorTop.b = color.Blue() / 255.0;
                    // Keep the previous alpha value
                    //m_boardAdapter.m_SolderMaskColorTop.a = color.Alpha() / 255.0;
                    break;
                case B_Mask:
                    m_boardAdapter.m_SolderMaskColorBot.r = color.Red() / 255.0;
                    m_boardAdapter.m_SolderMaskColorBot.g = color.Green() / 255.0;
                    m_boardAdapter.m_SolderMaskColorBot.b = color.Blue() / 255.0;
                    // Keep the previous alpha value
                    //m_boardAdapter.m_SolderMaskColorBot.a = color.Alpha() / 255.0;
                    break;
                default:
                    break;
                }
            }
        }

        TransferDataToWindow();
    }
}
