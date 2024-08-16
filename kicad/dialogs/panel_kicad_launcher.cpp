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
#include <bitmap_store.h>
#include <kicad_manager_frame.h>
#include <kiplatform/policy.h>
#include <policy_keys.h>
#include <tool/tool_manager.h>
#include <tools/kicad_manager_actions.h>
#include <widgets/bitmap_button.h>
#include <wx/stattext.h>

#include "panel_kicad_launcher.h"


PANEL_KICAD_LAUNCHER::PANEL_KICAD_LAUNCHER( wxWindow* aParent ) :
        PANEL_KICAD_LAUNCHER_BASE( aParent )
{
    m_frame = static_cast<KICAD_MANAGER_FRAME*>( aParent );
    m_toolManager = m_frame->GetToolManager();
    CreateLaunchers();

    Bind( wxEVT_SYS_COLOUR_CHANGED,
          wxSysColourChangedEventHandler( PANEL_KICAD_LAUNCHER::onThemeChanged ), this );
}


PANEL_KICAD_LAUNCHER::~PANEL_KICAD_LAUNCHER()
{
    Unbind( wxEVT_SYS_COLOUR_CHANGED,
          wxSysColourChangedEventHandler( PANEL_KICAD_LAUNCHER::onThemeChanged ), this );
}


void PANEL_KICAD_LAUNCHER::CreateLaunchers()
{
    m_frame->SetPcmButton( nullptr );

    if( m_toolsSizer->GetEffectiveRowsCount() > 0 )
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

    auto addLauncher = [&]( const TOOL_ACTION& aAction, BITMAPS aBitmaps,
                            const wxString& aHelpText, bool enabled = true )
        {
            BITMAP_BUTTON* btn = new BITMAP_BUTTON( m_scrolledWindow, wxID_ANY );
            btn->SetBitmap( KiBitmapBundle( aBitmaps ) );
            btn->SetDisabledBitmap( KiDisabledBitmapBundle( aBitmaps ) );
            btn->SetPadding( FromDIP( 4 ) );
            btn->SetToolTip( aAction.GetTooltip() );

            auto handler =
                    [&]( wxEvent& aEvent )
                    {
                        // Defocus the button because leaving the large buttons
                        // focused after a click looks out of place in the launcher
                        m_frame->SetFocus();
                        // Gives a slice of time to update the button state (mandatory on GTK,
                        // useful on MSW to avoid some cosmetic issues).
                        wxSafeYield();

                        OPT_TOOL_EVENT evt = aAction.MakeEvent();
                        evt->SetHasPosition( false );
                        m_toolManager->ProcessEvent( *evt );
                    };

            wxStaticText* label = new wxStaticText( m_scrolledWindow, wxID_ANY, wxEmptyString );
            wxStaticText* help = new wxStaticText( m_scrolledWindow, wxID_ANY, wxEmptyString );

            label->SetToolTip( aAction.GetTooltip() );
            label->SetFont( titleFont );
            label->SetLabel( aAction.GetFriendlyName() );

            help->SetFont( helpFont );
            help->SetLabel( aHelpText );

            btn->Bind( wxEVT_BUTTON, handler );

            // The bug fix below makes this handler active for the entire window width.  Without
            // any visual feedback that's a bit odd.  Disabling for now.
            // label->Bind( wxEVT_LEFT_UP, handler );

            m_toolsSizer->Add( btn, 1, wxALIGN_CENTER_VERTICAL );

            wxBoxSizer* textSizer = new wxBoxSizer( wxVERTICAL );

            textSizer->Add( label );
            textSizer->Add( help );

            m_toolsSizer->Add( textSizer, 1, wxEXPAND | wxALIGN_CENTER_VERTICAL );

            btn->Enable( enabled );
            if( !enabled )
            {
                help->Disable();
                label->Disable();
            }

            return btn;
    };

    addLauncher( KICAD_MANAGER_ACTIONS::editSchematic, BITMAPS::icon_eeschema,
                 _( "Edit the project schematic" ) );

    addLauncher( KICAD_MANAGER_ACTIONS::editSymbols, BITMAPS::icon_libedit,
                 _( "Edit global and/or project schematic symbol libraries" ) );

    addLauncher( KICAD_MANAGER_ACTIONS::editPCB, BITMAPS::icon_pcbnew,
                 _( "Edit the project PCB design" ) );

    addLauncher( KICAD_MANAGER_ACTIONS::editFootprints, BITMAPS::icon_modedit,
                 _( "Edit global and/or project PCB footprint libraries" ) );

    addLauncher( KICAD_MANAGER_ACTIONS::viewGerbers, BITMAPS::icon_gerbview,
                 _( "Preview Gerber files" ) );

    addLauncher( KICAD_MANAGER_ACTIONS::convertImage, BITMAPS::icon_bitmap2component,
                 _( "Convert bitmap images to schematic symbols or PCB footprints" ) );

    addLauncher( KICAD_MANAGER_ACTIONS::showCalculator, BITMAPS::icon_pcbcalculator,
                 _( "Show tools for calculating resistance, current capacity, etc." ) );

    addLauncher( KICAD_MANAGER_ACTIONS::editDrawingSheet, BITMAPS::icon_pagelayout_editor,
                 _( "Edit drawing sheet borders and title blocks for use in schematics and PCB "
                    "designs" ) );

    BITMAP_BUTTON* bb =
            addLauncher( KICAD_MANAGER_ACTIONS::showPluginManager, BITMAPS::icon_pcm,
                         _( "Manage downloadable packages from KiCad and 3rd party repositories" ),
                         ( KIPLATFORM::POLICY::GetPolicyBool( POLICY_KEY_PCM )
                           != KIPLATFORM::POLICY::PBOOL::DISABLED ) );

    m_frame->SetPcmButton( bb );

    Layout();
}


void PANEL_KICAD_LAUNCHER::onThemeChanged( wxSysColourChangedEvent &aEvent )
{
    GetBitmapStore()->ThemeChanged();
    CreateLaunchers();

    aEvent.Skip();
}


