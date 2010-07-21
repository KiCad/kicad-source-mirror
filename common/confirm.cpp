/************************/
/* Menu "CONFIRMATION"  */
/* Function get_Message */
/* Test requires ESC    */
/************************/

#include "fctsys.h"
#include "common.h"

enum id_dialog {
    ID_TIMOUT = 1500
};


/* Class for displaying messages, similar to wxMessageDialog,
 * but can be erased after a time out expires.
 *
 * @note - Do not use the time feature.  It is broken by design because
 *         the dialog is shown as modal and wxWidgets will assert when
 *         compiled in the debug mode.  This is because the dialog steals
 *         the event queue when dialog is modal so the timer event never
 *         gets to the dialog event handle.  Using dialogs to display
 *         transient is brain dead anyway.  Use the message panel or some
 *         other method.
 */
class WinEDA_MessageDialog : public wxMessageDialog
{
private:
    int     m_LifeTime;
    wxTimer m_Timer;

public:
    WinEDA_MessageDialog( wxWindow * parent, const wxString &msg,
                          const wxString &title, int style, int lifetime );
    ~WinEDA_MessageDialog() { };

    void OnTimeOut( wxTimerEvent& event );

    DECLARE_EVENT_TABLE()
};

BEGIN_EVENT_TABLE( WinEDA_MessageDialog, wxMessageDialog )
    EVT_TIMER( ID_TIMOUT, WinEDA_MessageDialog::OnTimeOut )
END_EVENT_TABLE()


WinEDA_MessageDialog::WinEDA_MessageDialog( wxWindow*       parent,
                                            const wxString& msg,
                                            const wxString& title,
                                            int             style,
                                            int             lifetime ) :
    wxMessageDialog( parent, msg, title, style )
{
    m_LifeTime = lifetime;
    m_Timer.SetOwner( this, ID_TIMOUT );
    if( m_LifeTime > 0 )
        m_Timer.Start( 100 * m_LifeTime, wxTIMER_ONE_SHOT );
}


void WinEDA_MessageDialog::OnTimeOut( wxTimerEvent& event )
{
    m_Timer.Stop();
    EndModal( wxID_YES );   /* Does not work, I do not know why (this
                             * function is correctly called after time out).
                             * See not above as to why this doesn't work. */
}


/* Display an error or warning message.
 * If display time > 0 the dialog disappears after displayTime 0.1 seconds
 *
 */
void DisplayError( wxWindow* parent, const wxString& text, int displaytime )
{
    wxMessageDialog* dialog;

    if( displaytime > 0 )
        dialog = new WinEDA_MessageDialog( parent, text, _( "Warning" ),
                                           wxOK | wxICON_INFORMATION,
                                           displaytime );
    else
        dialog = new WinEDA_MessageDialog( parent, text, _( "Error" ),
                                           wxOK | wxICON_ERROR, 0 );

    dialog->ShowModal();
    dialog->Destroy();
}


/* Display an informational message.
 */
void DisplayInfoMessage( wxWindow* parent, const wxString& text,
                         int displaytime )
{
    wxMessageDialog* dialog;

    dialog = new WinEDA_MessageDialog( parent, text, _( "Info:" ),
                                       wxOK | wxICON_INFORMATION, displaytime );

    dialog->ShowModal();
    dialog->Destroy();
}


bool IsOK( wxWindow* parent, const wxString& text )
{
    int ii;

    ii = wxMessageBox( text, _( "Confirmation" ),
                       wxYES_NO | wxCENTRE | wxICON_HAND, parent );
    if( ii == wxYES )
        return TRUE;
    return FALSE;
}

