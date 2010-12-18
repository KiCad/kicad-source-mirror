// file dialog_helpers.h

#ifndef  _DIALOG_HELPERS_H_
#define  _DIALOG_HELPERS_H_

/* some small helper classes used in dialogs
 * Due to use of wxFormBuilder to create dialogs
 * Many of them should be removed
 */

/**
 * class WinEDAListBox
 *
 * Used to display a list of elements for selection, and an help of info line
 * about the selected item.
 */
class WinEDAListBox : public wxDialog
{
private:
    wxListBox*           m_listBox;
    wxTextCtrl*          m_messages;
    void (*m_callBackFct)( wxString& Text );

public:
    /**
     * Constructor:
     * @param aParent = apointeur to the parent window
     * @param aTitle = the title shown on top.
     * @param aItemList = a wxArrayStrin: the list of elements.
     * @param aRefText = an item name if an item must be preselected.
     * @param aCallBackFunction callback function to display comments
     * @param aPos = position of the dialog.
     */
    WinEDAListBox( WinEDA_DrawFrame* aParent, const wxString& aTitle,
                   const wxArrayString& aItemList, const wxString& aRefText,
                   void(* aCallBackFunction)(wxString& Text) = NULL,
                   wxPoint aPos = wxDefaultPosition );
    ~WinEDAListBox();

    void     SortList();
    void     Append( const wxString& aItemStr );
    void     InsertItems( const wxArrayString& aItemList, int aPosition = 0 );
    void     MoveMouseToOrigin();
    wxString GetTextSelection();

private:
    void     OnClose( wxCloseEvent& event );
    void     OnCancelClick( wxCommandEvent& event );
    void     OnOkClick( wxCommandEvent& event );
    void     ClickOnList( wxCommandEvent& event );
    void     D_ClickOnList( wxCommandEvent& event );
    void     OnKeyEvent( wxKeyEvent& event );

    DECLARE_EVENT_TABLE()
};


/************************************************/
/* Class to enter a line, is some dialog frames */
/************************************************/
class WinEDA_EnterText
{
public:
    bool          m_Modify;

private:
    wxString      m_NewText;
    wxTextCtrl*   m_FrameText;
    wxStaticText* m_Title;

public:
    WinEDA_EnterText( wxWindow* parent, const wxString& Title,
                      const wxString& TextToEdit, wxBoxSizer* BoxSizer,
                      const wxSize& Size, bool Multiline = false );

    ~WinEDA_EnterText()
    {
    }


    wxString GetValue();
    void     GetValue( char* buffer, int lenmax );
    void     SetValue( const wxString& new_text );
    void     Enable( bool enbl );

    void SetFocus() { m_FrameText->SetFocus(); }
    void SetInsertionPoint( int n ) { m_FrameText->SetInsertionPoint( n ); }
    void SetSelection( int n, int m )
    {
        m_FrameText->SetSelection( n, m );
    }
};


/************************************************************************/
/* Class to edit/enter a graphic text and its dimension ( INCHES or MM )*/
/************************************************************************/
class WinEDA_GraphicTextCtrl
{
public:
    UserUnitType  m_UserUnit;
    int           m_Internal_Unit;

    wxTextCtrl*   m_FrameText;
    wxTextCtrl*   m_FrameSize;
private:
    wxStaticText* m_Title;

public:
    WinEDA_GraphicTextCtrl( wxWindow* parent, const wxString& Title,
                            const wxString& TextToEdit, int textsize,
                            UserUnitType user_unit, wxBoxSizer* BoxSizer, int framelen = 200,
                            int internal_unit = EESCHEMA_INTERNAL_UNIT );

    ~WinEDA_GraphicTextCtrl();

    wxString        GetText();
    int             GetTextSize();
    void            Enable( bool state );
    void            SetTitle( const wxString& title );

    void SetFocus() { m_FrameText->SetFocus(); }
    void            SetValue( const wxString& value );
    void            SetValue( int value );

    /**
     * Function FormatSize
     * formats a string containing the size in the desired units.
     */
    static wxString FormatSize( int internalUnit, UserUnitType user_unit, int textSize );

    static int      ParseSize( const wxString& sizeText, int internalUnit,
                               UserUnitType user_unit );
};


/**************************************************************************/
/* Class to edit/enter a coordinate (pair of values) ( INCHES or MM ) in  */
/* dialog boxes,                                                          */
/**************************************************************************/
class WinEDA_PositionCtrl
{
public:
    UserUnitType  m_UserUnit;
    int           m_Internal_Unit;
    wxPoint       m_Pos_To_Edit;

    wxTextCtrl*   m_FramePosX;
    wxTextCtrl*   m_FramePosY;
private:
    wxStaticText* m_TextX, * m_TextY;

public:
    WinEDA_PositionCtrl( wxWindow* parent, const wxString& title,
                         const wxPoint& pos_to_edit,
                         UserUnitType user_unit, wxBoxSizer* BoxSizer,
                         int internal_unit = EESCHEMA_INTERNAL_UNIT );

    ~WinEDA_PositionCtrl();

    void    Enable( bool x_win_on, bool y_win_on );
    void    SetValue( int x_value, int y_value );
    wxPoint GetValue();
};


/*************************************************************
 *  Class to edit/enter a size (pair of values for X and Y size)
 *  ( INCHES or MM ) in dialog boxes
 ***************************************************************/
class WinEDA_SizeCtrl : public WinEDA_PositionCtrl
{
public:
    WinEDA_SizeCtrl( wxWindow* parent, const wxString& title,
                     const wxSize& size_to_edit,
                     UserUnitType user_unit, wxBoxSizer* BoxSizer,
                     int internal_unit = EESCHEMA_INTERNAL_UNIT );

    ~WinEDA_SizeCtrl() { }
    wxSize GetValue();
};


/****************************************************************/
/* Class to edit/enter a value ( INCHES or MM ) in dialog boxes */
/****************************************************************/
class WinEDA_ValueCtrl
{
public:
    UserUnitType  m_UserUnit;
    int           m_Value;
    wxTextCtrl*   m_ValueCtrl;
private:
    int           m_Internal_Unit;
    wxStaticText* m_Text;

public:
    WinEDA_ValueCtrl( wxWindow* parent, const wxString& title, int value,
                      UserUnitType user_unit, wxBoxSizer* BoxSizer,
                      int internal_unit = EESCHEMA_INTERNAL_UNIT );

    ~WinEDA_ValueCtrl();

    int  GetValue();
    void SetValue( int new_value );
    void Enable( bool enbl );

    void SetToolTip( const wxString& text )
    {
        m_ValueCtrl->SetToolTip( text );
    }
};

/*************************/
/* class WinEDAChoiceBox */
/*************************/

/* class to display a choice list.
 *  This is a wrapper to wxComboBox (or wxChoice)
 *  but because they have some problems, WinEDAChoiceBox uses workarounds:
 *  - in wxGTK 2.6.2 wxGetSelection() does not work properly,
 *  - and wxChoice crashes if compiled in non unicode mode and uses utf8 codes
 */

#define EVT_KICAD_CHOICEBOX EVT_COMBOBOX
class WinEDAChoiceBox : public wxComboBox
{
public:
    WinEDAChoiceBox( wxWindow* parent, wxWindowID id,
                     const wxPoint& pos = wxDefaultPosition,
                     const wxSize& size = wxDefaultSize,
                     int n = 0, const wxString choices[] = NULL ) :
        wxComboBox( parent, id, wxEmptyString, pos, size,
                    n, choices, wxCB_READONLY )
    {
    }


    WinEDAChoiceBox( wxWindow* parent, wxWindowID id,
                     const wxPoint& pos, const wxSize& size,
                     const wxArrayString& choices ) :
        wxComboBox( parent, id, wxEmptyString, pos, size,
                    choices, wxCB_READONLY )
    {
    }


    int GetChoice()
    {
        return GetCurrentSelection();
    }
};

#endif
