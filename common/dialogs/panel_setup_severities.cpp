/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
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

#include <widgets/paged_dialog.h>
#include <widgets/ui_common.h>
#include <rc_item.h>
#include <dialogs/panel_setup_severities.h>
#include <wx/radiobut.h>
#include <wx/scrolwin.h>
#include <wx/sizer.h>
#include <wx/stattext.h>
#include "confirm.h"


PANEL_SETUP_SEVERITIES::PANEL_SETUP_SEVERITIES( wxWindow* aParentWindow,
                                                std::vector<std::reference_wrapper<RC_ITEM>> aItems,
                                                std::map<int, SEVERITY>& aSeverities,
                                                RC_ITEM* aPinMapSpecialCase ) :
        wxPanel( aParentWindow ),
        m_severities( aSeverities ),
        m_items( std::move( aItems ) ),
        m_pinMapSpecialCase( aPinMapSpecialCase )
{
    wxString          severities[]  = { _( "Error" ), _( "Warning" ), _( "Ignore" ) };
    int               severityCount = sizeof( severities ) / sizeof( wxString );
    int               baseID        = 1000;
    wxBoxSizer*       panelSizer    = new wxBoxSizer( wxVERTICAL );
    wxScrolledWindow* scrollWin     = new wxScrolledWindow( this, wxID_ANY,
                                                            wxDefaultPosition, wxDefaultSize,
                                                            wxTAB_TRAVERSAL | wxVSCROLL );
    bool              firstLine     = true;

    scrollWin->SetScrollRate( 0, 5 );

    wxBoxSizer* scrollWinSizer = new wxBoxSizer( wxVERTICAL );
    scrollWin->SetSizer( scrollWinSizer );

    wxFlexGridSizer* gridSizer = new wxFlexGridSizer( 0, 2, 0, 5 );
    gridSizer->SetFlexibleDirection( wxBOTH );
    gridSizer->SetVGap( 5 );

    for( const RC_ITEM& item : m_items )
    {
        int      errorCode = item.GetErrorCode();
        wxString msg       = item.GetErrorText( true );

        if( m_pinMapSpecialCase && errorCode == m_pinMapSpecialCase->GetErrorCode() )
            continue;

        if( errorCode == 0 )
        {
            wxStaticText* heading = new wxStaticText( scrollWin, wxID_ANY, msg );
            wxFont        headingFont = heading->GetFont();

            heading->SetFont( headingFont.Bold() );

            if( !firstLine )
            {
                gridSizer->AddSpacer( 5 );  // col 1
                gridSizer->AddSpacer( 5 );  // col 2
            }

            gridSizer->Add( heading, 0, wxALIGN_BOTTOM | wxALL | wxEXPAND, 4  );
            gridSizer->AddSpacer( 0 );  // col 2
        }
        else if( !msg.IsEmpty() )   // items with no message are for internal use only
        {
            wxStaticText* errorLabel = new wxStaticText( scrollWin, wxID_ANY, msg + wxT( ":" ) );
            gridSizer->Add( errorLabel, 0, wxALIGN_CENTER_VERTICAL | wxLEFT, 15  );

            // OSX can't handle more than 100 radio buttons in a single window (yes, seriously),
            // so we have to create a window for each set
            wxPanel*    radioPanel = new wxPanel( scrollWin );
            wxBoxSizer* radioSizer = new wxBoxSizer( wxHORIZONTAL );

            for( int i = 0; i < severityCount; ++i )
            {
                m_buttonMap[ errorCode ][i] = new wxRadioButton( radioPanel,
                                                                 baseID + errorCode * 10 + i,
                                                                 severities[i],
                                                                 wxDefaultPosition, wxDefaultSize,
                                                                 i == 0 ? wxRB_GROUP : 0 );
                radioSizer->Add( m_buttonMap[ errorCode ][i], 0,
                                 wxRIGHT | wxALIGN_CENTER_VERTICAL, 30 );
            }

            radioPanel->SetSizer( radioSizer );
            radioPanel->Layout();
            gridSizer->Add( radioPanel, 0, wxALIGN_CENTER_VERTICAL  );
        }

        firstLine = false;
    }


    if( m_pinMapSpecialCase )
    {
        wxString pinMapSeverities[] = { _( "From Pin Conflicts Map" ), wxT( "" ), _( "Ignore" ) };
        int      errorCode          = m_pinMapSpecialCase->GetErrorCode();
        wxString msg                = m_pinMapSpecialCase->GetErrorText( true );

        wxStaticText* errorLabel = new wxStaticText( scrollWin, wxID_ANY, msg + wxT( ":" ) );
        gridSizer->Add( errorLabel, 0, wxALIGN_CENTER_VERTICAL | wxALL | wxEXPAND, 15  );

        wxPanel*    radioPanel = new wxPanel( scrollWin );
        wxBoxSizer* radioSizer = new wxBoxSizer( wxHORIZONTAL );

        for( size_t i = 0; i < 3; ++i )
        {
            if( pinMapSeverities[i] == wxT( "" ) )
            {
                wxStaticText* spacer = new wxStaticText( radioPanel, wxID_ANY, wxT( "" ) );
                radioSizer->Add( spacer, 0, wxRIGHT | wxEXPAND, 17 );
            }
            else
            {
                m_buttonMap[ errorCode ][i] = new wxRadioButton( radioPanel,
                                                                 baseID + errorCode * 10 + i,
                                                                 pinMapSeverities[i],
                                                                 wxDefaultPosition, wxDefaultSize,
                                                                 i == 0 ? wxRB_GROUP : 0 );
                radioSizer->Add( m_buttonMap[ errorCode ][i], 0, wxEXPAND );
            }
        }

        radioPanel->SetSizer( radioSizer );
        radioPanel->Layout();
        gridSizer->Add( radioPanel, 0, wxALIGN_CENTER_VERTICAL  );
    }

    scrollWinSizer->Add( gridSizer, 1, wxEXPAND | wxALL, 5 );
    panelSizer->Add( scrollWin, 1, wxEXPAND, 0 );

    Bind( wxEVT_IDLE,
          [this]( wxIdleEvent& aEvent )
          {
              if( m_lastLoaded != m_severities )
              {
                  wxWindow* dialog = wxGetTopLevelParent( this );
                  wxWindow* topLevelFocus = wxGetTopLevelParent( wxWindow::FindFocus() );

                  if( topLevelFocus == dialog )
                      checkReload();
              }
          } );

    SetSizer( panelSizer );
    Layout();
    panelSizer->Fit( this );
}


void PANEL_SETUP_SEVERITIES::checkReload()
{
    // MUST update lastLoaded before calling IsOK (or we'll end up re-entering through the idle
    // event until we crash the stack).
    m_lastLoaded = m_severities;

    if( IsOK( m_parent, _( "The violation severities have been changed outside the Setup dialog.\n"
                           "Do you wish to reload them?" ) ) )
    {
        TransferDataToWindow();
    }
}


void PANEL_SETUP_SEVERITIES::ImportSettingsFrom( std::map<int, SEVERITY>& aSettings )
{
    for( const RC_ITEM& item : m_items )
    {
        int errorCode = item.GetErrorCode();

        wxRadioButton* button = nullptr;

        switch( aSettings[ errorCode ] )
        {
        case RPT_SEVERITY_ERROR:   button = m_buttonMap[ errorCode ][0]; break;
        case RPT_SEVERITY_WARNING: button = m_buttonMap[ errorCode ][1]; break;
        case RPT_SEVERITY_IGNORE:  button = m_buttonMap[ errorCode ][2]; break;
        default: break;
        }

        if( button )    // this entry must actually exist
            button->SetValue( true );
    }

    if( m_pinMapSpecialCase )
    {
        int pinMapCode  = m_pinMapSpecialCase->GetErrorCode();
        int newSeverity = aSettings[ pinMapCode ];

        m_buttonMap[ pinMapCode ][0]->SetValue( newSeverity != RPT_SEVERITY_IGNORE );
        m_buttonMap[ pinMapCode ][2]->SetValue( newSeverity == RPT_SEVERITY_IGNORE );
    }
}


bool PANEL_SETUP_SEVERITIES::TransferDataToWindow()
{
    m_lastLoaded = m_severities;

    for( const RC_ITEM& item : m_items )
    {
        int errorCode = item.GetErrorCode();

        if( !m_buttonMap[ errorCode ][0] )  // this entry does not actually exist
            continue;

        if( m_pinMapSpecialCase && errorCode == m_pinMapSpecialCase->GetErrorCode() )
            continue;

        switch( m_severities[ errorCode ] )
        {
        case RPT_SEVERITY_ERROR:   m_buttonMap[ errorCode ][0]->SetValue( true ); break;
        case RPT_SEVERITY_WARNING: m_buttonMap[ errorCode ][1]->SetValue( true ); break;
        case RPT_SEVERITY_IGNORE:  m_buttonMap[ errorCode ][2]->SetValue( true ); break;
        default:                                                                  break;
        }
    }

    if( m_pinMapSpecialCase )
    {
        int pinMapCode = m_pinMapSpecialCase->GetErrorCode();
        int severity   = m_severities[pinMapCode];

        m_buttonMap[ pinMapCode ][0]->SetValue( severity != RPT_SEVERITY_IGNORE );
        m_buttonMap[ pinMapCode ][2]->SetValue( severity == RPT_SEVERITY_IGNORE );
    }

    return true;
}


bool PANEL_SETUP_SEVERITIES::TransferDataFromWindow()
{
    for( const RC_ITEM& item : m_items )
    {
        int errorCode = item.GetErrorCode();

        if( m_pinMapSpecialCase && m_pinMapSpecialCase->GetErrorCode() == errorCode )
            continue;

        if( !m_buttonMap[ errorCode ][0] )  // this entry does not actually exist
            continue;

        SEVERITY severity = RPT_SEVERITY_UNDEFINED;

        if( m_buttonMap[ errorCode ][0]->GetValue() )
            severity = RPT_SEVERITY_ERROR;
        else if( m_buttonMap[ errorCode ][1]->GetValue() )
            severity = RPT_SEVERITY_WARNING;
        else if( m_buttonMap[ errorCode ][2]->GetValue() )
            severity = RPT_SEVERITY_IGNORE;

        m_severities[ errorCode ] = severity;
    }

    if( m_pinMapSpecialCase )
    {
        int      pinMapCode = m_pinMapSpecialCase->GetErrorCode();
        SEVERITY severity   = RPT_SEVERITY_UNDEFINED;

        if( m_buttonMap[ pinMapCode ][0]->GetValue() )
            severity = RPT_SEVERITY_ERROR;
        else if( m_buttonMap[ pinMapCode ][2]->GetValue() )
            severity = RPT_SEVERITY_IGNORE;

        m_severities[ pinMapCode ] = severity;
    }

    return true;
}
