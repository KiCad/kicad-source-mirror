/////////////////////////////////////////////////////////////////////////////
// Name:        setpage.h
/////////////////////////////////////////////////////////////////////////////

#ifndef _DIALOG_PAGES_SETTINGS_H_
#define _DIALOG_PAGES_SETTINGS_H_

#include <dialog_page_settings_base.h>

/*!
 * DIALOG_PAGES_SETTINGS class declaration
 */

class DIALOG_PAGES_SETTINGS: public DIALOG_PAGES_SETTINGS_BASE
{
private:
    EDA_DRAW_FRAME* m_Parent;
    BASE_SCREEN*    m_Screen;
    bool            m_modified;
    PAGE_INFO       m_user_size;        ///< instantiated just to get the size

    static wxSize   s_LastSize;		        ///< last position and size
    static wxPoint	s_LastPos;

public:
    DIALOG_PAGES_SETTINGS( EDA_DRAW_FRAME* parent );
    ~DIALOG_PAGES_SETTINGS();

    /**
     * Function Show
     * overloads the wxDialog::Show() function so it can position the
     * dialog at its remembered size and position.
     */
    bool Show( bool show );


private:
    /// Initialises member variables
    void initDialog();

    /// wxEVT_CLOSE_WINDOW event handler for ID_DIALOG
    void OnCloseWindow( wxCloseEvent& event );

    /// wxEVT_COMMAND_BUTTON_CLICKED event handler for wxID_OK
    void OnOkClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_BUTTON_CLICKED event handler for wxID_CANCEL
    void OnCancelClick( wxCommandEvent& event );

    void setCurrentPageSizeSelection( const wxString& aPaperSize );
    void SavePageSettings(wxCommandEvent& event);
    void ReturnSizeSelected(wxCommandEvent& event);

    void onPaperSizeChoice( wxCommandEvent& event );
};

#endif  // _DIALOG_PAGES_SETTINGS_H_
