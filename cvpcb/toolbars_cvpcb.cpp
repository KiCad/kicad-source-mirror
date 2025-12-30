/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2016 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 2011 Wayne Stambaugh <stambaughw@verizon.net>
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
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

#include <toolbars_cvpcb.h>

#include <tool/action_toolbar.h>
#include <tool/actions.h>

#include <cvpcb_id.h>
#include <cvpcb_mainframe.h>
#include <tools/cvpcb_actions.h>
#include <wx/stattext.h>

#include <settings/settings_manager.h>


std::optional<TOOLBAR_CONFIGURATION> CVPCB_TOOLBAR_SETTINGS::DefaultToolbarConfig( TOOLBAR_LOC aToolbar )
{
    TOOLBAR_CONFIGURATION config;

    // clang-format off
    switch( aToolbar )
    {
    case TOOLBAR_LOC::LEFT:
    case TOOLBAR_LOC::RIGHT:
    case TOOLBAR_LOC::TOP_AUX:
        return std::nullopt;

    case TOOLBAR_LOC::TOP_MAIN:
        config.AppendAction( CVPCB_ACTIONS::saveAssociationsToSchematic );

        config.AppendSeparator()
              .AppendAction( ACTIONS::showFootprintLibTable );

        config.AppendSeparator()
              .AppendAction( CVPCB_ACTIONS::showFootprintViewer );

        config.AppendSeparator()
              .AppendAction( CVPCB_ACTIONS::gotoPreviousNA )
              .AppendAction( CVPCB_ACTIONS::gotoNextNA );

        config.AppendSeparator()
              .AppendAction( ACTIONS::undo )
              .AppendAction( ACTIONS::redo )
              .AppendAction( CVPCB_ACTIONS::autoAssociate )
              .AppendAction( CVPCB_ACTIONS::deleteAll );

        // Add tools for footprint names filtering:
        config.AppendSeparator()
              .AppendSpacer( 15 )
              .AppendControl( CVPCB_ACTION_TOOLBAR_CONTROLS::footprintFilter );
        break;
    }

    // clang-format on
    return config;
}


void CVPCB_MAINFRAME::configureToolbars()
{
    EDA_BASE_FRAME::configureToolbars();

    auto footprintFilterFactory =
        [this]( ACTION_TOOLBAR* aToolbar )
        {

            // wxGTK with GTK3 has a serious issue with bold texts: strings are incorrectly sized
            // and truncated after the first space.
            // so use SetLabelMarkup is a trick to fix this issue.
            wxString msg_bold = _( "Footprint Filters:" );
            wxStaticText* text = new wxStaticText( m_tbTopMain, wxID_ANY, msg_bold );
            text->SetFont( m_tbTopMain->GetFont().Bold() );

            #ifdef __WXGTK3__
            text->SetLabelMarkup( "<b>" + msg_bold + "</b>" );
            #endif

            aToolbar->AddControl( text );

            aToolbar->Add( CVPCB_ACTIONS::FilterFPbyFPFilters );
            aToolbar->Add( CVPCB_ACTIONS::filterFPbyPin );
            aToolbar->Add( CVPCB_ACTIONS::FilterFPbyLibrary );

            aToolbar->AddScaledSeparator( this );

            if( !m_tcFilterString )
            {
                m_tcFilterString = new wxTextCtrl( aToolbar, wxID_ANY, wxEmptyString, wxDefaultPosition,
                                                   wxDefaultSize, wxTE_PROCESS_ENTER );
            }

            // Min size on Mac is (a not very useful) single character
            m_tcFilterString->SetMinSize( wxSize( 150, -1 ) );
            m_tcFilterString->Bind( wxEVT_TEXT_ENTER, &CVPCB_MAINFRAME::onTextFilterChanged, this );

            aToolbar->AddControl( m_tcFilterString );
        };

    RegisterCustomToolbarControlFactory( CVPCB_ACTION_TOOLBAR_CONTROLS::footprintFilter, footprintFilterFactory );
}


ACTION_TOOLBAR_CONTROL CVPCB_ACTION_TOOLBAR_CONTROLS::footprintFilter( "control.FootprintFilters",
                                                                       _( "Footprint filters" ),
                                                                       _( "Footprint filtering controls" ),
                                                                       { FRAME_CVPCB } );
