/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 1992-2015 KiCad Developers, see AUTHORS.txt for contributors.
 * @author Maciej Suminski <maciej.suminski@cern.ch>
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


#include <wx/wx.h>
#include <wx/richtooltip.h>
#include "dialog_assistant.h"
#include <bitmaps.h>
#include <wx/statline.h>
#include <wx/textctrl.h>
#include <wxPcbStruct.h>
#include <class_draw_panel_gal.h>

class HINT : public wxFrame
{
public:
    HINT( wxWindow* parent, const wxString& msg = wxEmptyString, wxWindowID id = wxID_ANY,
            const wxString& title = wxEmptyString, const wxPoint& pos = wxDefaultPosition,
            const wxSize& size = wxDefaultSize,
            long style = ASSISTANT_STYLE
            ) :
    wxFrame( parent, id, title, pos, size, style ), m_parent( parent )
    {
        m_sizer = new wxBoxSizer( wxVERTICAL );

        m_txt = new wxStaticText( this, wxID_ANY, msg );
        m_txt->SetForegroundColour( *wxBLACK );
        m_sizer->Add( m_txt, 1, wxALL|wxALIGN_CENTER, 5 );

        SetBackgroundColour( wxColour( 0xff, 0xff, 0xcc ) );
        layout();

        m_timer.SetOwner( this );
        Connect( wxEVT_TIMER, wxTimerEventHandler( HINT::onTimer ), NULL, this );
        Connect( wxEVT_LEFT_DOWN, wxMouseEventHandler( HINT::onButton ), NULL, this );
        m_txt->Connect( wxEVT_LEFT_DOWN, wxMouseEventHandler( HINT::onButton ), NULL, this );
    }

    ~HINT()
    {
    }

    void DisplayHint( const wxString& aMsg, int aTimeout = 0 )
    {
        m_txt->SetLabelMarkup( aMsg );
        layout();
        Show();

        if( aTimeout )
            m_timer.StartOnce( aTimeout );
    }

    void onTimer( wxTimerEvent& aEvent )
    {
        Hide();
    }

    void onButton( wxMouseEvent& aEvent )
    {
        Hide();
    }

protected:

    void layout()
    {
        Layout();
        //m_sizer->Layout();
        m_sizer->Fit( this );
        SetPosition( m_parent->GetPosition() - wxSize( this->GetSize().x, 0 ) );
    }

    wxBoxSizer* m_sizer;
    wxStaticText* m_txt;
    wxWindow* m_parent;
    wxTimer m_timer;
};

KICAD_ASSISTANT::KICAD_ASSISTANT( wxWindow* parent, wxWindowID id, const wxString& title,
        const wxPoint& pos, const wxSize& size, long style ) :
    wxFrame( parent, id, title, pos, size, style ), m_parent( parent )
{
    SetSizeHints( wxDefaultSize, wxDefaultSize );

    wxBoxSizer* bSizer1;
    bSizer1 = new wxBoxSizer( wxVERTICAL );

    m_imgPaperclip = new wxStaticBitmap( this, wxID_ANY, KiBitmap( paperclip_xpm ),
            wxDefaultPosition, wxDefaultSize, 0 );
    bSizer1->Add( m_imgPaperclip, 1, wxALL, 5 );

    SetSizer( bSizer1 );
    Layout();
    bSizer1->Fit( this );

    Centre( wxBOTH );

    m_hint = new HINT( this );
    m_hint->Enable( false );
    m_hint->SetCanFocus( false );
    SetCanFocus( false );

    Connect( wxEVT_CLOSE_WINDOW, wxCloseEventHandler( KICAD_ASSISTANT::OnClose ), NULL, this );
    Connect( wxEVT_MOVE, wxMoveEventHandler( KICAD_ASSISTANT::OnMove ), NULL, this );
    Connect( wxEVT_LEFT_DOWN, wxMouseEventHandler( KICAD_ASSISTANT::onButton ), NULL, this );

    // generate random hints
    m_hints[WELCOME].push_back( "Welcome to KiCad!" );
    m_hints[WELCOME].push_back( "Hi! Did you miss me?" );
    m_hints[WELCOME].push_back( "Laying out tracks, eh?" );
    m_hints[WELCOME].push_back( "It looks like you're trying to write a lett..\nWait a moment.." );
    m_hints[WELCOME].push_back( "I do not recognize this window.." );
    m_hints[WELCOME].push_back( "Hmph, word processors looked much better 10 years ago.." );
    m_hints[WELCOME].push_back( "Hey! I like this window more than my previous home." );
    m_hints[WELCOME].push_back( "Surprised, huh?" );
    m_hints[WELCOME].push_back( "What are you looking at?" );
    m_hints[WELCOME].push_back( "I was afraid I will never get a job again.." );
    m_hints[WELCOME].push_back( "How did I get here?" );

    m_hints[TIP_OF_THE_DAY].push_back("<b>Tip of the day:</b>\n\nDid you know that R=U/I?");
    m_hints[TIP_OF_THE_DAY].push_back("<b>Tip of the day:</b>\n\nDid you know that tracks are made from copper?");
    m_hints[TIP_OF_THE_DAY].push_back("<b>Tip of the day:</b>\n\nNever measure voltage with ammeter.");
    m_hints[TIP_OF_THE_DAY].push_back("<b>Tip of the day:</b>\n\nAlways wear safety goggles.");
    m_hints[TIP_OF_THE_DAY].push_back("<b>Tip of the day:</b>\n\nHardware is fine until it releases smoke,\nhence it is the smoke driving circuits.");
    m_hints[TIP_OF_THE_DAY].push_back("<b>Tip of the day:</b>\n\nA hot soldering iron looks exactly the same as the cold one.");
    m_hints[TIP_OF_THE_DAY].push_back("<b>Tip of the day:</b>\n\n'Absolute maximum ratings' are for people who need limits in their lifes.");
    m_hints[TIP_OF_THE_DAY].push_back("<b>Tip of the day:</b>\n\nNoone reads tips of the day.");
    m_hints[TIP_OF_THE_DAY].push_back("<b>Tip of the day:</b>\n\nElectrical devices operate much better when they are powered.");
    m_hints[TIP_OF_THE_DAY].push_back("<b>Tip of the day:</b>\n\nKeep hardware away from liquids.");
    m_hints[TIP_OF_THE_DAY].push_back("<b>Tip of the day:</b>\n\nYou always have more electrical charge than you think.");
    m_hints[TIP_OF_THE_DAY].push_back("<b>Tip of the day:</b>\n\nDid you know that negative terminal of a 9V battery tastes different than the positive one?");
    m_hints[TIP_OF_THE_DAY].push_back("<b>Tip of the day:</b>\n\nKicking faulty hardware helps your mental health.");

    m_hints[ROUTING].push_back("I used to draw better tracks when I was your age.");
    m_hints[ROUTING].push_back("Wrong direction!");
    m_hints[ROUTING].push_back("I have seen better layouts, you know.");
    m_hints[ROUTING].push_back("Why do not you switch to knitting?");
    m_hints[ROUTING].push_back("And I had to draw tracks with a pen..");
    m_hints[ROUTING].push_back("Feeling like an artist?");
    m_hints[ROUTING].push_back("I doubt you can fit this track there.");
    m_hints[ROUTING].push_back("Keep going! It is already 2\% done!");
    m_hints[ROUTING].push_back("Stay calm and keep routing.");
    m_hints[ROUTING].push_back("Uh, that track is a little bit.. strange.");
    m_hints[ROUTING].push_back("You could have done better.");
    m_hints[ROUTING].push_back("Go! Go! Go!");
    m_hints[ROUTING].push_back("Should not you go to sleep?");
    m_hints[ROUTING].push_back("Do not worry! The next one will be better!");
    m_hints[ROUTING].push_back("I would help you if I could click.");

    m_hints[UNDO].push_back("It was good! Why?!");
    m_hints[UNDO].push_back("Ooops..");
    m_hints[UNDO].push_back("I told you it was wrong.");
    m_hints[UNDO].push_back("CTRL+Z, one of the greatest invention of humankind.");
    m_hints[UNDO].push_back("I doubt you can do it better.");
    m_hints[UNDO].push_back("Do you need some help?");
    m_hints[UNDO].push_back("Do not panic.");
    m_hints[UNDO].push_back("Oh my.. Again?");
    m_hints[UNDO].push_back("Come on!");
    m_hints[UNDO].push_back("You can not finish the layout by undoing it!");
    m_hints[UNDO].push_back("Ok, just one more chance.");
    m_hints[UNDO].push_back("Well, happens.");

    m_hints[MOVING].push_back("It looked better in its previous place.");
    m_hints[MOVING].push_back("It will not help you anyway.");
    m_hints[MOVING].push_back("Are you sure that is a good idea?");
    m_hints[MOVING].push_back("I liked its previous position.");
    m_hints[MOVING].push_back("I would not touch it if I were you.");
    m_hints[MOVING].push_back("And what now?");
    m_hints[MOVING].push_back("You will not find any better place for this item.");
    m_hints[MOVING].push_back("Be cautious, this part looks fragile.");
    m_hints[MOVING].push_back("Do not drop it, we do not have any spares.");
    m_hints[MOVING].push_back("I advise you not to move it.");
    m_hints[MOVING].push_back("Try on the opposite side.");
}

KICAD_ASSISTANT::~KICAD_ASSISTANT()
{
    //std::cout << "Destroyed" << std::endl;
}

void KICAD_ASSISTANT::onButton( wxMouseEvent& aEvent )
{
    RandomHint( WELCOME, 6000 );
}

void KICAD_ASSISTANT::DisplayHint( const wxString& aMsg, int aTimeout, int aChance )
{
    if(!IsVisible())
        return;

    int r = 1 + rand() % 100;
    if( r > aChance )
        return;

    m_hint->DisplayHint( aMsg, aTimeout );

    PCB_EDIT_FRAME* frame = dynamic_cast<PCB_EDIT_FRAME*>( m_parent );

    if( frame && frame->IsGalCanvasActive() )
        frame->GetGalCanvas()->SetFocus();
}

void KICAD_ASSISTANT::RandomHint( HINT_CATEGORY aCategory, int aTimeout, int aChance )
{
    std::vector<wxString>& hints = m_hints[aCategory];
    if( hints.empty() )
        return;

    int idx = rand() % hints.size();
    DisplayHint( hints[idx], aTimeout, aChance );
}

void KICAD_ASSISTANT::OnClose(wxCloseEvent& event)
{
    if( event.CanVeto() )
    {
        // Hide the window to avoid disasters when sb tries to call its methods while it is destroyed
        event.Veto();
        m_hint->Hide();
        Hide();
        return;
    }

    Destroy();
}

void KICAD_ASSISTANT::OnMove(wxMoveEvent& event)
{
    m_hint->SetPosition( GetPosition() - wxSize( m_hint->GetSize().x, 0 ) );
}
