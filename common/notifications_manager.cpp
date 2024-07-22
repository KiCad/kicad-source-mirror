/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2023 Mark Roszko <mark.roszko@gmail.com>
 * Copyright (C) 2023 KiCad Developers, see AUTHORS.txt for contributors.
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

#include <wx/filename.h>
#include <wx/frame.h>
#include <wx/hyperlink.h>
#include <wx/panel.h>
#include <wx/scrolwin.h>
#include <wx/sizer.h>
#include <wx/settings.h>
#include <wx/stattext.h>
#include <wx/string.h>

#include <paths.h>

#include <notifications_manager.h>
#include <widgets/kistatusbar.h>
#include <json_common.h>

#include "core/wx_stl_compat.h"

#include <algorithm>
#include <fstream>
#include <map>
#include <core/json_serializers.h>
#include <optional>
#include <string>
#include <tuple>
#include <vector>
#include <wx/string.h>


NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE( NOTIFICATION, title, description, href, key, date )

class NOTIFICATION_PANEL : public wxPanel
{
public:
    NOTIFICATION_PANEL( wxWindow* aParent, NOTIFICATIONS_MANAGER* aManager, NOTIFICATION* aNoti ) :
            wxPanel( aParent, wxID_ANY, wxDefaultPosition, wxSize( -1, 75 ),
                     wxBORDER_SIMPLE ),
            m_hlDetails( nullptr ),
            m_notification( aNoti ),
            m_manager( aManager )
    {
        SetSizeHints( wxDefaultSize, wxDefaultSize );

        wxBoxSizer* mainSizer;
        mainSizer = new wxBoxSizer( wxVERTICAL );

        SetBackgroundColour( wxSystemSettings::GetColour( wxSYS_COLOUR_3DLIGHT ) );

        m_stTitle = new wxStaticText( this, wxID_ANY, aNoti->title );
        m_stTitle->Wrap( -1 );
        m_stTitle->SetFont( wxFont( wxNORMAL_FONT->GetPointSize(), wxFONTFAMILY_DEFAULT,
                                        wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD, false, wxEmptyString ) );
        mainSizer->Add( m_stTitle, 0, wxALL | wxEXPAND, 1 );

        m_stDescription = new wxStaticText( this, wxID_ANY, aNoti->description, wxDefaultPosition,
                                          wxDefaultSize, 0 );
        m_stDescription->Wrap( -1 );
        mainSizer->Add( m_stDescription, 0, wxALL | wxEXPAND, 1 );

        wxBoxSizer* tailSizer;
        tailSizer = new wxBoxSizer( wxHORIZONTAL );

        if( !aNoti->href.IsEmpty() )
        {
            m_hlDetails =
                    new wxHyperlinkCtrl( this, wxID_ANY, _( "View Details" ), aNoti->href,
                                         wxDefaultPosition, wxDefaultSize, wxHL_DEFAULT_STYLE );
            tailSizer->Add( m_hlDetails, 0, wxALL, 2 );
        }

        m_hlDismiss = new wxHyperlinkCtrl( this, wxID_ANY, _( "Dismiss" ), aNoti->href,
                                           wxDefaultPosition, wxDefaultSize, wxHL_DEFAULT_STYLE );
        tailSizer->Add( m_hlDismiss, 0, wxALL, 2 );

        mainSizer->Add( tailSizer, 1, wxEXPAND, 5 );

        if( m_hlDetails != nullptr )
        {
            m_hlDetails->Bind( wxEVT_HYPERLINK, &NOTIFICATION_PANEL::onDetails, this );
        }

        m_hlDismiss->Bind( wxEVT_HYPERLINK, &NOTIFICATION_PANEL::onDismiss, this );

        SetSizer( mainSizer );
        Layout();
    }

private:
    void onDetails( wxHyperlinkEvent& aEvent )
    {
        wxString url = aEvent.GetURL();

        if( url.StartsWith( wxS( "kicad://" ) ) )
        {
            url.Replace( wxS( "kicad://" ), wxS( "" ) );

            if( url == wxS( "pcm" ) )
            {
                // TODO
            }
        }
        else
        {
            wxLaunchDefaultBrowser( aEvent.GetURL(), wxBROWSER_NEW_WINDOW );
        }
    }

    void onDismiss( wxHyperlinkEvent& aEvent )
    {
        CallAfter(
                [this]()
                {
                    // This will cause this panel to get deleted
                    m_manager->Remove( m_notification->key );
                } );
    }

    wxStaticText* m_stTitle;
    wxStaticText* m_stDescription;
    wxHyperlinkCtrl* m_hlDetails;
    wxHyperlinkCtrl* m_hlDismiss;
    NOTIFICATION* m_notification;
    NOTIFICATIONS_MANAGER* m_manager;
};


class NOTIFICATIONS_LIST : public wxFrame
{
public:
    NOTIFICATIONS_LIST( NOTIFICATIONS_MANAGER* aManager, wxWindow* parent, const wxPoint& pos ) :
            wxFrame( parent, wxID_ANY, _( "Notifications" ), pos, wxSize( 300, 150 ),
                     wxFRAME_NO_TASKBAR | wxBORDER_SIMPLE ),
            m_manager( aManager )
    {
        SetSizeHints( wxDefaultSize, wxDefaultSize );

        wxBoxSizer* bSizer1;
        bSizer1 = new wxBoxSizer( wxVERTICAL );

        m_scrolledWindow = new wxScrolledWindow( this, wxID_ANY, wxDefaultPosition,
                                                 wxSize( -1, -1 ), wxVSCROLL );
        m_scrolledWindow->SetScrollRate( 5, 5 );
        m_contentSizer = new wxBoxSizer( wxVERTICAL );

        m_scrolledWindow->SetSizer( m_contentSizer );
        m_scrolledWindow->Layout();
        m_contentSizer->Fit( m_scrolledWindow );
        bSizer1->Add( m_scrolledWindow, 1, wxEXPAND | wxALL, 0 );

        m_noNotificationsText = new wxStaticText(
                m_scrolledWindow, wxID_ANY, _( "There are no notifications available" ),
                wxDefaultPosition, wxDefaultSize, wxALIGN_CENTER_HORIZONTAL );
        m_noNotificationsText->Wrap( -1 );
        m_contentSizer->Add( m_noNotificationsText, 1, wxALL | wxEXPAND, 5 );

        Bind( wxEVT_KILL_FOCUS, &NOTIFICATIONS_LIST::onFocusLoss, this );
        m_scrolledWindow->Bind( wxEVT_KILL_FOCUS, &NOTIFICATIONS_LIST::onFocusLoss, this );

        SetSizer( bSizer1 );
        Layout();

        SetFocus();
    }


    void onFocusLoss( wxFocusEvent& aEvent )
    {
        // check if a child like say, the hyperlink texts got focus
        if( !IsDescendant( aEvent.GetWindow() ) )
            Close( true );

        aEvent.Skip();
    }


    void Add( NOTIFICATION* aNoti )
    {
        m_noNotificationsText->Hide();

        NOTIFICATION_PANEL* panel = new NOTIFICATION_PANEL( m_scrolledWindow, m_manager, aNoti );
        m_contentSizer->Add( panel, 0, wxEXPAND | wxALL, 2 );
        m_scrolledWindow->Layout();
        m_contentSizer->Fit( m_scrolledWindow );

        // call this at this window otherwise the child panels dont resize width properly
        Layout();

        m_panelMap[aNoti] = panel;
    }


    void Remove( NOTIFICATION* aNoti )
    {
        auto it = m_panelMap.find( aNoti );
        if( it != m_panelMap.end() )
        {
            NOTIFICATION_PANEL* panel = m_panelMap[aNoti];
            m_contentSizer->Detach( panel );
            panel->Destroy();

            m_panelMap.erase( it );

            // ensure the window contents get shifted as needed
            m_scrolledWindow->Layout();
            Layout();
        }

        if( m_panelMap.size() == 0 )
        {
            m_noNotificationsText->Show();
        }
    }

private:
    wxScrolledWindow*                                      m_scrolledWindow;
    ///< Inner content of the scrolled window, add panels here
    wxBoxSizer*                                            m_contentSizer;
    std::unordered_map<NOTIFICATION*, NOTIFICATION_PANEL*> m_panelMap;
    NOTIFICATIONS_MANAGER*                                 m_manager;
    ///< Text to be displayed when no notifications are present, this gets a Show/Hide call as needed
    wxStaticText*                                          m_noNotificationsText;
};


NOTIFICATIONS_MANAGER::NOTIFICATIONS_MANAGER()
{
    m_destFileName = wxFileName( PATHS::GetUserCachePath(), wxT( "notifications.json" ) );
}


void NOTIFICATIONS_MANAGER::Load()
{
    nlohmann::json saved_json;

    std::ifstream saved_json_stream( m_destFileName.GetFullPath().fn_str() );

    try
    {
        saved_json_stream >> saved_json;

        m_notifications = saved_json.get<std::vector<NOTIFICATION>>();
    }
    catch( std::exception& )
    {
        // failed to load the json, which is fine, default to no notificaitons
    }

    if( wxGetEnv( wxT( "KICAD_TEST_NOTI" ), nullptr ) )
    {
        CreateOrUpdate( wxS( "test" ), wxS( "Test Notification" ), wxS( "Test please ignore" ),
                wxS( "https://kicad.org" ) );
    }
}


void NOTIFICATIONS_MANAGER::Save()
{
    std::ofstream jsonFileStream( m_destFileName.GetFullPath().fn_str() );

    nlohmann::json saveJson = nlohmann::json( m_notifications );
    jsonFileStream << std::setw( 4 ) << saveJson << std::endl;
    jsonFileStream.flush();
    jsonFileStream.close();
}


void NOTIFICATIONS_MANAGER::CreateOrUpdate( const wxString& aKey,
                                    const wxString& aTitle,
                                    const wxString& aDescription,
                                    const wxString& aHref )
{
    wxCHECK_RET( !aKey.IsEmpty(), wxS( "Notification key must not be empty" ) );

    auto it = std::find_if( m_notifications.begin(), m_notifications.end(),
                            [&]( const NOTIFICATION& noti )
                            {
                                return noti.key == aKey;
                            } );

    if( it != m_notifications.end() )
    {
        NOTIFICATION& noti = *it;

        noti.title = aTitle;
        noti.description = aDescription;
        noti.href = aHref;
    }
    else
    {
        m_notifications.emplace_back( NOTIFICATION{ aTitle, aDescription, aHref,
                                                    aKey, wxEmptyString } );
    }

    if( m_shownDialogs.size() > 0 )
    {
        // update dialogs
        for( NOTIFICATIONS_LIST* list : m_shownDialogs )
        {
            list->Add( &m_notifications.back() );
        }
    }

    for( KISTATUSBAR* statusBar : m_statusBars )
    {
        statusBar->SetNotificationCount( m_notifications.size() );
    }

    Save();
}


void NOTIFICATIONS_MANAGER::Remove( const wxString& aKey )
{
    auto it = std::find_if( m_notifications.begin(), m_notifications.end(),
                            [&]( const NOTIFICATION& noti )
                            {
                                return noti.key == aKey;
                            } );

    if( it == m_notifications.end() )
    {
        return;
    }

    if( m_shownDialogs.size() > 0 )
    {
        // update dialogs

        for( NOTIFICATIONS_LIST* list : m_shownDialogs )
        {
            list->Remove( &(*it) );
        }
    }

    m_notifications.erase( it );

    Save();

    for( KISTATUSBAR* statusBar : m_statusBars )
    {
        statusBar->SetNotificationCount( m_notifications.size() );
    }
}


void NOTIFICATIONS_MANAGER::onListWindowClosed( wxCloseEvent& aEvent )
{
    NOTIFICATIONS_LIST* evtWindow = dynamic_cast<NOTIFICATIONS_LIST*>( aEvent.GetEventObject() );

    m_shownDialogs.erase( std::remove_if( m_shownDialogs.begin(), m_shownDialogs.end(),
                                          [&]( NOTIFICATIONS_LIST* dialog )
                                          {
                                              return dialog == evtWindow;
                                          } ) );

    aEvent.Skip();
}


void NOTIFICATIONS_MANAGER::ShowList( wxWindow* aParent, wxPoint aPos )
{
    NOTIFICATIONS_LIST* list = new NOTIFICATIONS_LIST( this, aParent, aPos );

    for( NOTIFICATION& job : m_notifications )
    {
        list->Add( &job );
    }

    m_shownDialogs.push_back( list );

    list->Bind( wxEVT_CLOSE_WINDOW, &NOTIFICATIONS_MANAGER::onListWindowClosed, this );

    // correct the position
    wxSize windowSize = list->GetSize();
    list->SetPosition( aPos - windowSize );

    list->Show();
}


void NOTIFICATIONS_MANAGER::RegisterStatusBar( KISTATUSBAR* aStatusBar )
{
    m_statusBars.push_back( aStatusBar );

    // notifications should already be loaded so set the initial notification count
    aStatusBar->SetNotificationCount( m_notifications.size() );
}


void NOTIFICATIONS_MANAGER::UnregisterStatusBar( KISTATUSBAR* aStatusBar )
{
    m_statusBars.erase( std::remove_if( m_statusBars.begin(), m_statusBars.end(),
                                        [&]( KISTATUSBAR* statusBar )
                                          {
                                            return statusBar == aStatusBar;
                                          } ) );
}