
#ifndef DIALOG_COLOR_CONFIG_H_
#define DIALOG_COLOR_CONFIG_H_

#include <wx/statline.h>


class wxBoxSizer;
class wxStaticLine;
class wxStdDialogButtonSizer;


extern void SeedLayers();


/***********************************************/
/* Derived class for the frame color settings. */
/***********************************************/

class DIALOG_COLOR_CONFIG : public wxDialog
{
private:
    DECLARE_DYNAMIC_CLASS( DIALOG_COLOR_CONFIG )

    EDA_DRAW_FRAME*         m_parent;
    wxBoxSizer*             m_outerBoxSizer;
    wxBoxSizer*             m_mainBoxSizer;
    wxBoxSizer*             m_columnBoxSizer;
    wxBoxSizer*             m_rowBoxSizer;
    wxBitmapButton*         m_bitmapButton;
    wxRadioBox*             m_SelBgColor;
    wxStaticLine*           m_line;
    wxStdDialogButtonSizer* m_stdDialogButtonSizer;

    // Creation
    bool Create( wxWindow* aParent,
                 wxWindowID aId = wxID_ANY,
                 const wxString& aCaption =  _( "EESchema Colors" ),
                 const wxPoint& aPosition = wxDefaultPosition,
                 const wxSize& aSize = wxDefaultSize,
                 long aStyle = wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER );

    // Initializes member variables
    void Init();

    // Creates the controls and sizers
    void CreateControls();

    wxBitmap GetBitmapResource( const wxString& aName );
    wxIcon   GetIconResource( const wxString& aName );
    static bool ShowToolTips();

    bool    UpdateColorsSettings();
    void    SetColor( wxCommandEvent& aEvent );
    void    OnOkClick( wxCommandEvent& aEvent );
    void    OnCancelClick( wxCommandEvent& aEvent );
    void    OnApplyClick( wxCommandEvent& aEvent );

public:
    // Constructors and destructor
    DIALOG_COLOR_CONFIG();
    DIALOG_COLOR_CONFIG( EDA_DRAW_FRAME* aParent );
    ~DIALOG_COLOR_CONFIG();
};

#endif    // DIALOG_COLOR_CONFIG_H_
