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

    auto addLauncher =
        [&]( const TOOL_ACTION& aAction, const wxBitmap& aBitmap,
             const wxString& aHelpText = wxEmptyString )
        {
            BITMAP_BUTTON* btn = new BITMAP_BUTTON( this, wxID_ANY );
            btn->SetBitmap( aBitmap );
            btn->SetPadding( 5 );
            btn->SetToolTip( aAction.GetDescription() );

            auto handler =
                    [&]( wxEvent& aEvent )
                    {
                        OPT_TOOL_EVENT evt = aAction.MakeEvent();
                        evt->SetHasPosition( false );
                        m_toolManager->ProcessEvent( *evt );
                    };

            bool createHelp = !aHelpText.IsEmpty();

            wxStaticText* label = new wxStaticText( this, wxID_ANY, aAction.GetLabel() );
            wxStaticText* help;

            label->SetToolTip( aAction.GetDescription() );

            if( createHelp )
            {
                help = new wxStaticText( this, wxID_ANY, aHelpText );
                help->SetForegroundColour( wxSystemSettings::GetColour( wxSYS_COLOUR_GRAYTEXT ) );
            }

            btn->Bind( wxEVT_BUTTON, handler );
            label->Bind( wxEVT_LEFT_UP, handler );

            int row     = m_toolsSizer->GetRows();
            int rowSpan = createHelp ? 2 : 1;
            int flags   = createHelp ? wxALIGN_BOTTOM : wxALIGN_CENTER_VERTICAL;

            m_toolsSizer->Add( btn,   wxGBPosition( row, 0 ), wxGBSpan( rowSpan, 1 ), 0, 0 );
            m_toolsSizer->Add( label, wxGBPosition( row, 1 ), wxGBSpan( 1, 1 ), flags, 0 );

            if( createHelp )
            {
                m_toolsSizer->Add( help, wxGBPosition( row + 1, 1 ), wxGBSpan( 1, 1 ),
                                   wxALIGN_TOP, 0 );
            }
        };

    addLauncher( KICAD_MANAGER_ACTIONS::editSchematic, KiScaledBitmap( icon_eeschema_xpm, this ) );

    addLauncher( KICAD_MANAGER_ACTIONS::editSymbols, KiScaledBitmap( icon_libedit_xpm, this ) );

    addLauncher( KICAD_MANAGER_ACTIONS::editPCB, KiScaledBitmap( icon_pcbnew_xpm, this ) );

    addLauncher( KICAD_MANAGER_ACTIONS::editFootprints, KiScaledBitmap( icon_modedit_xpm, this ) );

    addLauncher( KICAD_MANAGER_ACTIONS::viewGerbers, KiScaledBitmap( icon_gerbview_xpm, this ) );

    addLauncher( KICAD_MANAGER_ACTIONS::convertImage,
                 KiScaledBitmap( icon_bitmap2component_xpm, this ) );

    addLauncher( KICAD_MANAGER_ACTIONS::showCalculator,
                 KiScaledBitmap( icon_pcbcalculator_xpm, this ) );

    addLauncher( KICAD_MANAGER_ACTIONS::editWorksheet,
                 KiScaledBitmap( icon_pagelayout_editor_xpm, this ) );

    if( m_toolsSizer->IsColGrowable( 1 ) )
        m_toolsSizer->RemoveGrowableCol( 1 );

    m_toolsSizer->AddGrowableCol( 1 );
    Layout();
}
