
#ifndef _DIALOG_COLOR_CONFIG_H_
#define _DIALOG_COLOR_CONFIG_H_

#include "wx/statline.h"


class wxBoxSizer;
class wxStaticLine;
class wxStdDialogButtonSizer;


// Specify the width and height of every (color-displaying / bitmap) button
const int BUTT_SIZE_X = 16;
const int BUTT_SIZE_Y = 16;


/********************/
/* Layer menu list. */
/********************/

struct ColorButton
{
    wxString        m_Name;
    int             m_Layer;
};

struct ButtonIndex
{
    wxString      m_Name;
    ColorButton*  m_Buttons;
};


/***********************************************/
/* Derived class for the frame color settings. */
/***********************************************/

class DIALOG_COLOR_CONFIG: public wxDialog
{
private:
    DECLARE_DYNAMIC_CLASS( DIALOG_COLOR_CONFIG )

    EDA_DRAW_FRAME*         m_Parent;
    wxBoxSizer*             OuterBoxSizer;
    wxBoxSizer*             MainBoxSizer;
    wxBoxSizer*             ColumnBoxSizer;
    wxBoxSizer*             RowBoxSizer;
    wxBitmapButton*         BitmapButton;
    wxRadioBox*             m_SelBgColor;
    wxStaticLine*           Line;
    wxStdDialogButtonSizer* StdDialogButtonSizer;
    wxButton*               Button;

    // Creation
    bool Create( wxWindow* aParent,
                 wxWindowID aId = wxID_ANY,
                 const wxString& aCaption =  _( "EESchema Colors" ),
                 const wxPoint& aPosition = wxDefaultPosition,
                 const wxSize& aSize = wxDefaultSize,
                 long aStyle = wxDEFAULT_DIALOG_STYLE | MAYBE_RESIZE_BORDER );

    // Initializes member variables
    void Init();

    // Creates the controls and sizers
    void CreateControls();

    wxBitmap GetBitmapResource( const wxString& aName );
    wxIcon GetIconResource( const wxString& aName );
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

#endif    // _DIALOG_COLOR_CONFIG_H_
