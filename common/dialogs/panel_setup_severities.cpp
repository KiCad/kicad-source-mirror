/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2020 KiCad Developers, see AUTHORS.txt for contributors.
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
#include "panel_setup_severities.h"


PANEL_SETUP_SEVERITIES::PANEL_SETUP_SEVERITIES( PAGED_DIALOG* aParent, RC_ITEM& aDummyItem,
                                                std::map<int, int>& aSeverities,
                                                int aFirstErrorCode, int aLastErrorCode,
                                                int aPinMapSpecialCase ) :
        wxPanel( aParent->GetTreebook() ),
        m_severities( aSeverities ),
        m_firstErrorCode( aFirstErrorCode ),
        m_lastErrorCode( aLastErrorCode ),
        m_pinMapSpecialCase( aPinMapSpecialCase )
{
    wxString          severities[] = { _( "Error" ), _( "Warning" ), _( "Ignore" ) };
    int               severityCount = sizeof( severities ) / sizeof( wxString );
    int               baseID = 1000;
    wxBoxSizer*       panelSizer = new wxBoxSizer( wxVERTICAL );
    wxScrolledWindow* scrollWin = new wxScrolledWindow( this, wxID_ANY,
                                                        wxDefaultPosition, wxDefaultSize,
                                                        wxTAB_TRAVERSAL | wxVSCROLL );

    scrollWin->SetScrollRate( 0, 5 );

    wxFlexGridSizer* gridSizer = new wxFlexGridSizer( 0, 2, 0, 5 );
    gridSizer->SetFlexibleDirection( wxBOTH );

   	for( int errorCode = m_firstErrorCode; errorCode <= m_lastErrorCode; ++errorCode )
    {
   	    wxString msg = aDummyItem.GetErrorText( errorCode );

        // When msg is empty, for some reason, the current errorCode is not supported
        // by the RC_ITEM aDummyItem.
        // Skip this errorCode.
   	    if( !msg.IsEmpty() )
        {
            wxStaticText* errorLabel = new wxStaticText( scrollWin, wxID_ANY, msg + wxT( ":" ) );
            gridSizer->Add( errorLabel, 0, wxALIGN_CENTER_VERTICAL | wxALL | wxEXPAND, 4  );

            // OSX can't handle more than 100 radio buttons in a single window (yes, seriously),
            // so we have to create a window for each set
   	        wxPanel*    radioPanel = new wxPanel( scrollWin );
   	        wxBoxSizer* radioSizer = new wxBoxSizer( wxHORIZONTAL );

            for( size_t i = 0; i < severityCount; ++i )
            {
                m_buttonMap[ errorCode ][i] = new wxRadioButton( radioPanel,
                                                                 baseID + errorCode * 10 + i,
                                                                 severities[i],
                                                                 wxDefaultPosition,
                                                                 wxDefaultSize,
                                                                 i == 0 ? wxRB_GROUP : 0 );
                radioSizer->Add( m_buttonMap[ errorCode ][i], 0, wxRIGHT | wxEXPAND, 30 );
            }

            radioPanel->SetSizer( radioSizer );
            radioPanel->Layout();
            gridSizer->Add( radioPanel, 0, wxALIGN_CENTER_VERTICAL | wxALL | wxEXPAND, 4  );
        }
    }

   	if( m_pinMapSpecialCase >= 0 )
    {
        wxString pinMapSeverities[] = { _( "From Pin Conflicts Map" ), _( "" ), _( "Ignore" ) };
   	    int      errorCode = m_pinMapSpecialCase;
        wxString msg = aDummyItem.GetErrorText( errorCode );

        wxStaticText* errorLabel = new wxStaticText( scrollWin, wxID_ANY, msg + wxT( ":" ) );
        gridSizer->Add( errorLabel, 0, wxALIGN_CENTER_VERTICAL | wxALL | wxEXPAND, 4  );

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
                                                                 wxDefaultPosition,
                                                                 wxDefaultSize,
                                                                 i == 0 ? wxRB_GROUP : 0 );
                radioSizer->Add( m_buttonMap[ errorCode ][i], 0, wxEXPAND );
            }
        }

        radioPanel->SetSizer( radioSizer );
        radioPanel->Layout();
        gridSizer->Add( radioPanel, 0, wxALIGN_CENTER_VERTICAL | wxALL | wxEXPAND, 4  );
    }

    scrollWin->SetSizer( gridSizer );
    scrollWin->Layout();
   	gridSizer->Fit( scrollWin );
    panelSizer->Add( scrollWin, 1, wxEXPAND | wxALL, 5 );

    this->SetSizer( panelSizer );
   	this->Layout();
   	panelSizer->Fit( this );
}


void PANEL_SETUP_SEVERITIES::ImportSettingsFrom( std::map<int, int>& aSettings )
{
    for( int errorCode = m_firstErrorCode; errorCode <= m_lastErrorCode; ++errorCode )
    {
        if(! m_buttonMap[ errorCode ][0] )  // this entry does not actually exist
            continue;

        switch( aSettings[ errorCode ] )
        {
        case RPT_SEVERITY_ERROR:   m_buttonMap[ errorCode ][0]->SetValue( true ); break;
        case RPT_SEVERITY_WARNING: m_buttonMap[ errorCode ][1]->SetValue( true ); break;
        case RPT_SEVERITY_IGNORE:  m_buttonMap[ errorCode ][2]->SetValue( true ); break;
        default:                                                                  break;
        }
    }

    if( m_pinMapSpecialCase >= 0 )
    {
        int newSeverity = aSettings[ m_pinMapSpecialCase ];

        m_buttonMap[ m_pinMapSpecialCase ][0]->SetValue( newSeverity != RPT_SEVERITY_IGNORE );
        m_buttonMap[ m_pinMapSpecialCase ][1]->SetValue( newSeverity == RPT_SEVERITY_IGNORE );
    }
}


bool PANEL_SETUP_SEVERITIES::TransferDataToWindow()
{
    for( int errorCode = m_firstErrorCode; errorCode <= m_lastErrorCode; ++errorCode )
    {
        if( !m_buttonMap[ errorCode ][0] )  // this entry does not actually exist
            continue;

        switch( m_severities[ errorCode ] )
        {
        case RPT_SEVERITY_ERROR:   m_buttonMap[ errorCode ][0]->SetValue( true ); break;
        case RPT_SEVERITY_WARNING: m_buttonMap[ errorCode ][1]->SetValue( true ); break;
        case RPT_SEVERITY_IGNORE:  m_buttonMap[ errorCode ][2]->SetValue( true ); break;
        default:                                                                  break;
        }
    }

    if( m_pinMapSpecialCase >= 0 )
    {
        int severity = m_severities[ m_pinMapSpecialCase ];

        m_buttonMap[ m_pinMapSpecialCase ][0]->SetValue( severity != RPT_SEVERITY_IGNORE );
        m_buttonMap[ m_pinMapSpecialCase ][2]->SetValue( severity == RPT_SEVERITY_IGNORE );
    }

    return true;
}


bool PANEL_SETUP_SEVERITIES::TransferDataFromWindow()
{
    for( int errorCode = m_firstErrorCode; errorCode <= m_lastErrorCode; ++errorCode )
    {
        if( !m_buttonMap[ errorCode ][0] )  // this entry does not actually exist
            continue;

        int severity = RPT_SEVERITY_UNDEFINED;

        if( m_buttonMap[ errorCode ][0]->GetValue() )
            severity = RPT_SEVERITY_ERROR;
        else if( m_buttonMap[ errorCode ][1]->GetValue() )
            severity = RPT_SEVERITY_WARNING;
        else if( m_buttonMap[ errorCode ][2]->GetValue() )
            severity = RPT_SEVERITY_IGNORE;

        m_severities[ errorCode ] = severity;
    }

    if( m_pinMapSpecialCase >= 0 )
    {
        int severity = RPT_SEVERITY_UNDEFINED;

        if( m_buttonMap[ m_pinMapSpecialCase ][0]->GetValue() )
            severity = RPT_SEVERITY_ERROR;
        else if( m_buttonMap[ m_pinMapSpecialCase ][2]->GetValue() )
            severity = RPT_SEVERITY_IGNORE;

        m_severities[ m_pinMapSpecialCase ] = severity;
    }

    return true;
}
