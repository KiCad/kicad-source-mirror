/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2020 Ian McInerney <ian.s.mcinerney@ieee.org>
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

#ifndef INFOBAR_H_
#define INFOBAR_H_

#include <functional>
#include <optional>
#include <wx/event.h>
#include <wx/infobar.h>
#include <wx/timer.h>
#include <wx/panel.h>
#include <wx/sizer.h>
#include <reporter.h>


class wxAuiManager;
class wxHyperlinkCtrl;


enum
{
    /// ID for the close button on the frame's infobar
    ID_CLOSE_INFOBAR = 2000,
};


wxDECLARE_EVENT( KIEVT_SHOW_INFOBAR,    wxCommandEvent );
wxDECLARE_EVENT( KIEVT_DISMISS_INFOBAR, wxCommandEvent );

/**
 * A modified version of the wxInfoBar class that allows us to:
 *     * Show the close button along with the other buttons
 *     * Remove all user-provided buttons at once
 *     * Allow automatically hiding the infobar after a time period
 *     * Show/hide using events
 *     * Place it inside an AUI manager
 *
 * This inherits from the generic infobar because the native infobar
 * on GTK doesn't include the icon on the left and it looks worse.
 *
 * There are 2 events associated with the infobar:
 *
 * KIEVT_SHOW_INFOBAR:
 *   An event that tells the infobar to show a message.
 *
 *   The message text is contained inside the string component,
 *   and the message flag is contained inside the int component.
 *
 *   Sample event creation code:
 *       wxCommandEvent* evt = new wxCommandEvent( KIEVT_SHOW_INFOBAR );
 *       evt->SetString( "A message to show" );
 *       evt->SetInt( wxICON_WARNING );
 *
 * KIEVT_DISMISS_INFOBAR:
 *   An event that tells the infobar to hide itself.
 */
class WX_INFOBAR : public wxInfoBarGeneric
{
public:
    /**
     * Construct an infobar that can exist inside an AUI managed frame.
     *
     * @param aParent is the parent
     * @param aMgr is the AUI manager that this infobar is added to
     * @param aWinId is the ID for this infobar object
     */
    WX_INFOBAR( wxWindow* aParent, wxAuiManager* aMgr = nullptr, wxWindowID aWinid = wxID_ANY );

    ~WX_INFOBAR();


    /**
     * Sets the type of message for special handling if needed
     */
    enum class MESSAGE_TYPE
    {
        GENERIC,          /**< GENERIC Are messages that do not have special handling */
        OUTDATED_SAVE,    /**< OUTDATED_SAVE Messages that should be cleared on save */
        DRC_RULES_ERROR,
        DRC_VIOLATION
    };

    MESSAGE_TYPE GetMessageType() const { return m_type; }

    /**
     * Set the time period to show the infobar.
     *
     * This only applies for the next showing of the infobar,
     * so it must be reset every time. A value of 0 disables
     * the automatic hiding (this is the default).
     *
     * @param aTime is the time in milliseconds to show the infobar
     */
    void SetShowTime( int aTime );

    /**
     * Add the default close button to the infobar on the right side.
     *
     * @param aTooltip is the tooltip to give the close button
     */
    void AddCloseButton( const wxString& aTooltip = _( "Hide this message." ) );

    /**
     * Add an already created button to the infobar.
     * New buttons are added in the right-most position.
     *
     * @param aButton is the button to add
     */
    void AddButton( wxButton* aButton );

    /**
     * Add an already created hypertext link to the infobar.
     * New buttons are added in the right-most position.
     *
     * @param aHypertextButton is the button to add
     */
    void AddButton( wxHyperlinkCtrl* aHypertextButton );

    /**
     * Add a button with the provided ID and text.
     * The new button is created on the right-most position.
     *
     * @param aId is the ID to assign to the button
     * @param aLabel is the text for the button
     */
    void AddButton( wxWindowID aId, const wxString& aLabel = wxEmptyString ) override;

    /**
     * Remove all the buttons that have been added by the user.
     */
    void RemoveAllButtons();

    bool HasCloseButton() const;

    wxBitmapButton* GetCloseButton() const;

    /**
     * Provide a callback to be called when the infobar is dismissed (either by user action
     * or timer).
     * @param aCallback
     */
    void SetCallback( std::function<void(void)> aCallback )
    {
        m_callback = aCallback;
    }

    /**
     * Show the infobar with the provided message and icon for a specific period
     * of time.
     *
     * @param aMessage is the message to display
     * @param aTime is the amount of time in milliseconds to show the infobar
     * @param aFlags is the flag containing the icon to display on the left side of the infobar
     */
    void ShowMessageFor( const wxString& aMessage, int aTime, int aFlags = wxICON_INFORMATION,
                         MESSAGE_TYPE aType = WX_INFOBAR::MESSAGE_TYPE::GENERIC );

    /**
     * Show the info bar with the provided message and icon.
     *
     * @param aMessage is the message to display
     * @param aFlags is the flag containing the icon to display on the left side of the infobar
     */
    void ShowMessage( const wxString& aMessage, int aFlags = wxICON_INFORMATION ) override;

    /**
     * Show the info bar with the provided message and icon, setting the type
     *
     * @param aMessage is the message to display
     * @param aFlags is the flag containing the icon to display on the left side of the infobar
     * @param aType is the type of message being displayed
     */
    void ShowMessage( const wxString& aMessage, int aFlags, MESSAGE_TYPE aType );

    /**
     * Dismisses the infobar and updates the containing layout and AUI manager
     * (if one is provided).
     */
    void Dismiss() override;

    /**
     * Send the infobar an event telling it to show a message.
     *
     * @param aMessage is the message to display
     * @param aFlags is the flag containing the icon to display on the left side of the infobar
     */
    void QueueShowMessage( const wxString& aMessage, int aFlags = wxICON_INFORMATION );

    /**
     * Send the infobar an event telling it to hide itself.
     */
    void QueueDismiss();

    /**
     * Returns true if the infobar is being updated.
     */
    bool IsLocked()
    {
        return m_updateLock;
    }

protected:
    /**
     * Event handler for showing the infobar using a wxCommandEvent of the type
     * KIEVT_SHOW_INFOBAR. The message is stored inside the string field, and the
     * icon flag is stored inside the int field.
     */
    void onShowInfoBar( wxCommandEvent& aEvent );

    /**
     * Event handler for dismissing the infobar using a wxCommandEvent of the type
     * KIEVT_DISMISS_INFOBAR.
     */
    void onDismissInfoBar( wxCommandEvent& aEvent );

    /**
     * Event handler for the close button.
     * This is bound to ID_CLOSE_INFOBAR on the infobar.
     */
    void onCloseButton( wxCommandEvent& aEvent );

    /**
     * Event handler for the color theme change event.
     */
    void onThemeChange( wxSysColourChangedEvent& aEvent );

    /**
     * Event handler for the automatic closing timer.
     */
    void onTimer( wxTimerEvent& aEvent );

    void onSize( wxSizeEvent& aEvent );

    /**
     * Update the AUI pane to show or hide this infobar.
     *
     * @param aShow is true to show the pane
     */
    void updateAuiLayout( bool aShow );

protected:
    int           m_showTime;       ///< The time to show the infobar. 0 = don't auto hide
    bool          m_updateLock;     ///< True if this infobar requested the UI update
    wxTimer*      m_showTimer;      ///< The timer counting the autoclose period
    wxAuiManager* m_auiManager;     ///< The AUI manager that contains this infobar
    MESSAGE_TYPE  m_type;           ///< The type of message being displayed
    wxString      m_message;        ///< The original message without wrapping

    std::optional<std::function<void(void)>> m_callback;   ///< Optional callback made when closing infobar

    DECLARE_EVENT_TABLE()
};


/**
 * A wxPanel derived class that hold an infobar and another control.
 * The infobar is located at the top of the panel, and the other control
 * is located below it.
 *
 * This allows the infobar to be controlled nicely by an AUI manager,
 * since adding the infobar on its own to the AUI manager produces
 * artifacts when showing/hiding it due to the AUI pane layout.
 *
 * Note that this implementation currently has issues on Windows with
 * event processing inside the GAL canvas, see:
 * https://gitlab.com/kicad/code/kicad/-/issues/4501
 *
 */
class EDA_INFOBAR_PANEL : public wxPanel
{
public:
    EDA_INFOBAR_PANEL( wxWindow* aParent, wxWindowID aId = wxID_ANY,
                       const wxPoint& aPos = wxDefaultPosition,
                       const wxSize& aSize = wxSize( -1,-1 ),
                       long aStyle = wxTAB_TRAVERSAL,
                       const wxString& aName = wxEmptyString );

    /**
     * Add the given infobar object to the panel
     *
     * @param aInfoBar is the infobar to add
     */
    void AddInfoBar( WX_INFOBAR* aInfoBar );

    /**
     * Add the other item to the panel.
     * This item will expand to fill up the vertical space left.
     *
     * @param aOtherItem is the item to add
     */
    void AddOtherItem( wxWindow* aOtherItem );

protected:
    // The sizer containing the infobar and the other object
    wxFlexGridSizer* m_mainSizer;
};


/**
 * A wrapper for reporting to a #WX_INFOBAR UI element.
 *
 * The infobar is not updated until the @c Finalize() method is called. That method will
 * queue either a show message or a dismiss event for the infobar - so this reporter is
 * safe to use inside a paint event without causing an infinite paint event loop.
 *
 * No action is taken if no message is given to the reporter.
 */
class INFOBAR_REPORTER : public REPORTER
{
public:
    INFOBAR_REPORTER( WX_INFOBAR* aInfoBar ) :
            REPORTER(),
            m_messageSet( false ),
            m_infoBar( aInfoBar ),
            m_severity( RPT_SEVERITY_UNDEFINED )
    {
    }

    virtual ~INFOBAR_REPORTER() {};

    REPORTER& Report( const wxString& aText, SEVERITY aSeverity = RPT_SEVERITY_UNDEFINED ) override;

    bool HasMessage() const override;

    /**
     * Update the infobar with the reported text.
     */
    void Finalize();

private:
    bool                      m_messageSet;
    WX_INFOBAR*               m_infoBar;
    std::unique_ptr<wxString> m_message;
    SEVERITY                  m_severity;
};
#endif // INFOBAR_H_
