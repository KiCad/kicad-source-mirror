/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2021 KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <bitmaps.h>
#include <kicad_manager_frame.h>
#include <tool/tool_manager.h>
#include <tools/kicad_manager_actions.h>
#include <widgets/bitmap_button.h>

#include "panel_kicad_launcher.h"


PANEL_KICAD_LAUNCHER::PANEL_KICAD_LAUNCHER( wxWindow* aParent ) :
        PANEL_KICAD_LAUNCHER_BASE( aParent )
{
    m_toolManager = static_cast<KICAD_MANAGER_FRAME*>( aParent )->GetToolManager();
    CreateLaunchers();
}


void PANEL_KICAD_LAUNCHER::CreateLaunchers()
{
    if( m_toolsSizer->GetRows() > 0 )
    {
        m_toolsSizer->Clear( true );
        m_toolsSizer->SetRows( 0 );
    }

    wxFont titleFont = wxSystemSettings::GetFont( wxSYS_DEFAULT_GUI_FONT );
    titleFont.SetPointSize( titleFont.GetPointSize() + 2 );
    titleFont.SetWeight( wxFONTWEIGHT_BOLD );

    wxFont helpFont = wxSystemSettings::GetFont( wxSYS_DEFAULT_GUI_FONT );
    helpFont.SetStyle( wxFONTSTYLE_ITALIC );

    auto addLauncher =
        [&]( const TOOL_ACTION& aAction, const wxBitmap& aBitmap, const wxString& aHelpText )
        {
            BITMAP_BUTTON* btn = new BITMAP_BUTTON( this, wxID_ANY );
            btn->SetBitmap( aBitmap );
            btn->SetPadding( 3 );
            btn->SetToolTip( aAction.GetDescription() );

            auto handler =
                    [&]( wxEvent& aEvent )
                    {
                        OPT_TOOL_EVENT evt = aAction.MakeEvent();
                        evt->SetHasPosition( false );
                        m_toolManager->ProcessEvent( *evt );
                    };

            wxStaticText* label = new wxStaticText( this, wxID_ANY, aAction.GetLabel() );
            wxStaticText* help;

            label->SetToolTip( aAction.GetDescription() );
            label->SetFont( titleFont );

            help = new wxStaticText( this, wxID_ANY, aHelpText );
            help->SetFont( helpFont );

            btn->Bind( wxEVT_BUTTON, handler );
            label->Bind( wxEVT_LEFT_UP, handler );

            int row = m_toolsSizer->GetRows();

            m_toolsSizer->Add( btn, wxGBPosition( row, 0 ), wxGBSpan( 2, 1 ), wxALL, 0 );
            m_toolsSizer->Add( label, wxGBPosition( row, 1 ), wxGBSpan( 1, 1 ), wxALIGN_BOTTOM, 0 );
            m_toolsSizer->Add( help, wxGBPosition( row + 1, 1 ), wxGBSpan( 1, 1 ), wxALIGN_TOP, 0 );
        };

    addLauncher( KICAD_MANAGER_ACTIONS::editSchematic, KiScaledBitmap( icon_eeschema_xpm, this ),
                 _( "Edit the project schematic" ) );

    addLauncher( KICAD_MANAGER_ACTIONS::editSymbols, KiScaledBitmap( icon_libedit_xpm, this ),
                 _( "Edit global and/or project schematic symbol libraries" ) );

    addLauncher( KICAD_MANAGER_ACTIONS::editPCB, KiScaledBitmap( icon_pcbnew_xpm, this ),
                 _( "Edit the project PCB design" ) );

    addLauncher( KICAD_MANAGER_ACTIONS::editFootprints, KiScaledBitmap( icon_modedit_xpm, this ),
                 _( "Edit glabal and/or project PCB footprint libraries" ) );

    addLauncher( KICAD_MANAGER_ACTIONS::viewGerbers, KiScaledBitmap( icon_gerbview_xpm, this ),
                 _( "Preview Gerber files" ) );

    addLauncher( KICAD_MANAGER_ACTIONS::convertImage,
                 KiScaledBitmap( icon_bitmap2component_xpm, this ),
                 _( "Convert bitmap images to schematic symbols or PCB footprints" ) );

    addLauncher( KICAD_MANAGER_ACTIONS::showCalculator,
                 KiScaledBitmap( icon_pcbcalculator_xpm, this ),
                 _( "Show tools for calculating resistance, current capacity, etc." ) );

    addLauncher( KICAD_MANAGER_ACTIONS::editWorksheet,
                 KiScaledBitmap( icon_pagelayout_editor_xpm, this ),
                 _( "Edit worksheet borders and title blocks for use in schematics and PCB designs" ) );

    if( m_toolsSizer->IsColGrowable( 1 ) )
        m_toolsSizer->RemoveGrowableCol( 1 );

    m_toolsSizer->AddGrowableCol( 1 );
    Layout();
}
