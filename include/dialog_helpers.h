/**
 * @file dialog_helpers.h
 * @brief Helper dialog and control classes.
 * @note Due to use of wxFormBuilder to create dialogs many of them should be removed.
 */

#ifndef  DIALOG_HELPERS_H_
#define  DIALOG_HELPERS_H_


#include <common.h>             // EDA_UNITS_T


class EDA_DRAW_FRAME;



/**
 * class EDA_LIST_DIALOG
 *
 * Used to display a list of elements for selection, and an help of info line
 * about the selected item.
 */
class EDA_LIST_DIALOG : public wxDialog
{
private:
    wxListBox*           m_listBox;
    wxTextCtrl*          m_messages;
    void (*m_callBackFct)( wxString& Text );

public:
    /**
     * Constructor:
     * @param aParent Pointer to the parent window.
     * @param aTitle The title shown on top.
     * @param aItemList A wxArrayString of the list of elements.
     * @param aRefText An item name if an item must be preselected.
     * @param aCallBackFunction callback function to display comments
     * @param aPos The position of the dialog.
     */
    EDA_LIST_DIALOG( EDA_DRAW_FRAME* aParent, const wxString& aTitle,
                     const wxArrayString& aItemList, const wxString& aRefText,
                     void(* aCallBackFunction)(wxString& Text) = NULL,
                     wxPoint aPos = wxDefaultPosition );
    ~EDA_LIST_DIALOG();

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


/**
 * Class EDA_GRAPHIC_TEXT_CTRL
 * is a custom text edit control to edit/enter Kicad dimensions ( INCHES or MM )
 */
class EDA_GRAPHIC_TEXT_CTRL
{
public:
    EDA_UNITS_T   m_UserUnit;
    int           m_Internal_Unit;

    wxTextCtrl*   m_FrameText;
    wxTextCtrl*   m_FrameSize;
private:
    wxStaticText* m_Title;

public:
    EDA_GRAPHIC_TEXT_CTRL( wxWindow* parent, const wxString& Title,
                           const wxString& TextToEdit, int textsize,
                           EDA_UNITS_T user_unit, wxBoxSizer* BoxSizer, int framelen = 200,
                           int internal_unit = EESCHEMA_INTERNAL_UNIT );

    ~EDA_GRAPHIC_TEXT_CTRL();

    const wxString  GetText() const;
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
    static wxString FormatSize( int internalUnit, EDA_UNITS_T user_unit, int textSize );

    static int      ParseSize( const wxString& sizeText, int internalUnit,
                               EDA_UNITS_T user_unit );
};


/**************************************************************************/
/* Class to edit/enter a coordinate (pair of values) ( INCHES or MM ) in  */
/* dialog boxes,                                                          */
/**************************************************************************/
class EDA_POSITION_CTRL
{
public:
    EDA_UNITS_T   m_UserUnit;
    int           m_Internal_Unit;
    wxPoint       m_Pos_To_Edit;

    wxTextCtrl*   m_FramePosX;
    wxTextCtrl*   m_FramePosY;
private:
    wxStaticText* m_TextX, * m_TextY;

public:
    EDA_POSITION_CTRL( wxWindow* parent, const wxString& title,
                         const wxPoint& pos_to_edit,
                         EDA_UNITS_T user_unit, wxBoxSizer* BoxSizer,
                         int internal_unit = EESCHEMA_INTERNAL_UNIT );

    ~EDA_POSITION_CTRL();

    void    Enable( bool x_win_on, bool y_win_on );
    void    SetValue( int x_value, int y_value );
    wxPoint GetValue();
};


/*************************************************************
 *  Class to edit/enter a size (pair of values for X and Y size)
 *  ( INCHES or MM ) in dialog boxes
 ***************************************************************/
class EDA_SIZE_CTRL : public EDA_POSITION_CTRL
{
public:
    EDA_SIZE_CTRL( wxWindow* parent, const wxString& title,
                   const wxSize& size_to_edit,
                   EDA_UNITS_T user_unit, wxBoxSizer* BoxSizer,
                   int internal_unit = EESCHEMA_INTERNAL_UNIT );

    ~EDA_SIZE_CTRL() { }
    wxSize GetValue();
};


/****************************************************************/
/* Class to edit/enter a value ( INCHES or MM ) in dialog boxes */
/****************************************************************/
class EDA_VALUE_CTRL
{
public:
    EDA_UNITS_T   m_UserUnit;
    int           m_Value;
    wxTextCtrl*   m_ValueCtrl;
private:
    int           m_Internal_Unit;
    wxStaticText* m_Text;

public:
    EDA_VALUE_CTRL( wxWindow* parent, const wxString& title, int value,
                    EDA_UNITS_T user_unit, wxBoxSizer* BoxSizer,
                    int internal_unit = EESCHEMA_INTERNAL_UNIT );

    ~EDA_VALUE_CTRL();

    int  GetValue();
    void SetValue( int new_value );
    void Enable( bool enbl );

    void SetToolTip( const wxString& text )
    {
        m_ValueCtrl->SetToolTip( text );
    }
};

#endif    // DIALOG_HELPERS_H_
