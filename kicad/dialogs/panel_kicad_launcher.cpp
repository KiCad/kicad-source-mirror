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
#include <wx/stattext.h>

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
#ifndef __WXGTK__
    titleFont.SetPointSize( titleFont.GetPointSize() + 2 );
#endif
    titleFont.SetWeight( wxFONTWEIGHT_BOLD );

    wxFont helpFont = wxSystemSettings::GetFont( wxSYS_DEFAULT_GUI_FONT );
    helpFont.SetStyle( wxFONTSTYLE_ITALIC );

    auto addLauncher =
        [&]( const TOOL_ACTION& aAction, const wxBitmap& aBitmap, const wxString& aHelpText )
        {
            BITMAP_BUTTON* btn = new BITMAP_BUTTON( this, wxID_ANY );
            btn->SetBitmap( aBitmap );
            btn->SetPadding( 5 );
            btn->SetToolTip( aAction.GetDescription() );

            auto handler =
                    [&]( wxEvent& aEvent )
                    {
                        // Defocus the button because leaving the large buttons 
                        // focused after a click looks out of place in the launcher
                        GetParent()->SetFocus();

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

            // The bug fix below makes this handler active for the entire window width.  Without
            // any visual feedback that's a bit odd.  Disabling for now.
            // label->Bind( wxEVT_LEFT_UP, handler );

            int row = m_toolsSizer->GetRows();

            m_toolsSizer->Add( btn, wxGBPosition( row, 0 ), wxGBSpan( 2, 1 ), wxBOTTOM, 12 );

            // Due to https://trac.wxwidgets.org/ticket/16088?cversion=0&cnum_hist=7 GTK fails to
            // correctly set the BestSize of non-default-size or styled text so we need to make
            // sure that the BestSize isn't needed by setting wxEXPAND.  Unfortunately this makes
            // wxALIGN_BOTTOM non-functional, so we have to jump through a bunch more hoops to
            // try and align the title and help text in the middle of the icon.
            m_toolsSizer->Add( label, wxGBPosition( row, 1 ), wxGBSpan( 1, 1 ),
                               wxTOP | wxEXPAND, 10 );

            m_toolsSizer->Add( help, wxGBPosition( row + 1, 1 ), wxGBSpan( 1, 1 ),
                               wxALIGN_TOP | wxTOP, 1 );
        };

    addLauncher( KICAD_MANAGER_ACTIONS::editSchematic,
                 KiScaledBitmap( BITMAPS::icon_eeschema, this, 48, true ),
                 _( "Edit the project schematic" ) );

    addLauncher( KICAD_MANAGER_ACTIONS::editSymbols,
                 KiScaledBitmap( BITMAPS::icon_libedit, this, 48, true ),
                 _( "Edit global and/or project schematic symbol libraries" ) );

    addLauncher( KICAD_MANAGER_ACTIONS::editPCB,
                 KiScaledBitmap( BITMAPS::icon_pcbnew, this, 48, true ),
                 _( "Edit the project PCB design" ) );

    addLauncher( KICAD_MANAGER_ACTIONS::editFootprints,
                 KiScaledBitmap( BITMAPS::icon_modedit, this, 48, true ),
                 _( "Edit global and/or project PCB footprint libraries" ) );

    addLauncher( KICAD_MANAGER_ACTIONS::viewGerbers,
                 KiScaledBitmap( BITMAPS::icon_gerbview, this, 48, true ),
                 _( "Preview Gerber files" ) );

    addLauncher( KICAD_MANAGER_ACTIONS::convertImage,
                 KiScaledBitmap( BITMAPS::icon_bitmap2component, this, 48, true ),
                 _( "Convert bitmap images to schematic symbols or PCB footprints" ) );

    addLauncher( KICAD_MANAGER_ACTIONS::showCalculator,
                 KiScaledBitmap( BITMAPS::icon_pcbcalculator, this, 48, true ),
                 _( "Show tools for calculating resistance, current capacity, etc." ) );

    addLauncher( KICAD_MANAGER_ACTIONS::editDrawingSheet,
                 KiScaledBitmap( BITMAPS::icon_pagelayout_editor, this, 48, true ),
                 _( "Edit drawing sheet borders and title blocks for use in schematics and PCB "
                    "designs" ) );

#ifdef PCM
    addLauncher( KICAD_MANAGER_ACTIONS::showPluginManager,
                 KiScaledBitmap( BITMAPS::icon_pcm, this, 48, true ),
                 _( "Manage downloadable packages from KiCad and 3rd party repositories" ) );
#endif

    Layout();
}
