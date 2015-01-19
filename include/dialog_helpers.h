/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2010 Jean-Pierre Charras, jaen-pierre.charras@gipsa-lab.inpg.com
 * Copyright (C) 1992-2012 KiCad Developers, see AUTHORS.txt for contributors.
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

/**
 * @file dialog_helpers.h
 * @brief Helper dialog and control classes.
 * @note Due to use of wxFormBuilder to create dialogs many of them should be removed.
 */

#ifndef  DIALOG_HELPERS_H_
#define  DIALOG_HELPERS_H_


#include <common.h>             // EDA_UNITS_T
#include <dialog_shim.h>
#include <../common/dialogs/dialog_list_selector_base.h>


class EDA_DRAW_FRAME;

#define SORT_LIST true

/**
 * class EDA_LIST_DIALOG
 *
 * A dialog which shows:
 *   a list of elements for selection,
 *   a text control to display help or info about the selected item.
 *   2 buttons (OK and Cancel)
 *
 */
class EDA_LIST_DIALOG : public EDA_LIST_DIALOG_BASE
{
public:

    /**
     * Constructor:
     * @param aParent Pointer to the parent window.
     * @param aTitle = The title shown on top.
     * @param aItemHeaders is an array containing the column header names for the dialog.
     * @param aItemList = A wxArrayString of the list of elements.
     * @param aRefText = An item name if an item must be preselected.
     * @param aCallBackFunction = callback function to display comments
     * @param aCallBackFunctionData = a pointer to pass to @a aCallBackFunction
     * @param aSortList = true to sort list items by alphabetic order.
     */
    EDA_LIST_DIALOG( EDA_DRAW_FRAME* aParent, const wxString& aTitle,
                     const wxArrayString& aItemHeaders,
                     const std::vector<wxArrayString>& aItemList,
                     const wxString& aRefText,
                     void (* aCallBackFunction)( wxString& text, void* data ) = NULL,
                     void* aCallBackFunctionData = NULL,
                     bool aSortList = false );

    // ~EDA_LIST_DIALOG() {}

    void     Append( const wxArrayString& aItemStr );
    void     InsertItems( const std::vector<wxArrayString>& aItemList, int aPosition = 0 );

    /**
     * Function GetTextSelection
     * return the selected text from \a aColumn in the wxListCtrl in the dialog.
     *
     * @param aColumn is the column to return the text from.
     * @return a wxString object containing the selected text from \a aColumn.
     */
    wxString GetTextSelection( int aColumn = 0 );

private:
    void     onClose( wxCloseEvent& event );
    void     onCancelClick( wxCommandEvent& event );
    void     onOkClick( wxCommandEvent& event );
    void     onListItemSelected( wxListEvent& event );
    void     onListItemActivated( wxListEvent& event );
    void     textChangeInFilterBox(wxCommandEvent& event);

    void    initDialog( const wxArrayString& aItemHeaders,
                        const wxString& aSelection);
    void    sortList();
    bool    m_sortList;
    void    (* m_cb_func)( wxString& text, void* data );
    void*   m_cb_data;
    const   std::vector<wxArrayString>* m_itemsListCp;
};


/**
 * Class EDA_GRAPHIC_TEXT_CTRL
 * is a custom text edit control to edit/enter Kicad dimensions ( INCHES or MM )
 */
class EDA_GRAPHIC_TEXT_CTRL
{
public:
    EDA_UNITS_T   m_UserUnit;

    wxTextCtrl*   m_FrameText;
    wxTextCtrl*   m_FrameSize;
private:
    wxStaticText* m_Title;

public:
    EDA_GRAPHIC_TEXT_CTRL( wxWindow* parent, const wxString& Title,
                           const wxString& TextToEdit, int textsize,
                           EDA_UNITS_T user_unit, wxBoxSizer* BoxSizer, int framelen = 200 );

    ~EDA_GRAPHIC_TEXT_CTRL();

    const wxString  GetText() const;
    int             GetTextSize();
    void            Enable( bool state );
    void            SetTitle( const wxString& title );

    void            SetFocus() { m_FrameText->SetFocus(); }
    void            SetValue( const wxString& value );
    void            SetValue( int value );

    /**
     * Function FormatSize
     * formats a string containing the size in the desired units.
     */
    static wxString FormatSize( EDA_UNITS_T user_unit, int textSize );

    static int      ParseSize( const wxString& sizeText, EDA_UNITS_T user_unit );
};


/**************************************************************************/
/* Class to edit/enter a coordinate (pair of values) ( INCHES or MM ) in  */
/* dialog boxes,                                                          */
/**************************************************************************/
class EDA_POSITION_CTRL
{
public:
    EDA_UNITS_T   m_UserUnit;
    wxPoint       m_Pos_To_Edit;

    wxTextCtrl*   m_FramePosX;
    wxTextCtrl*   m_FramePosY;
private:
    wxStaticText* m_TextX, * m_TextY;

public:
    EDA_POSITION_CTRL( wxWindow* parent, const wxString& title,
                       const wxPoint& pos_to_edit, EDA_UNITS_T user_unit, wxBoxSizer* BoxSizer );

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
                   const wxSize& size_to_edit, EDA_UNITS_T user_unit, wxBoxSizer* BoxSizer );

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
    wxStaticText* m_Text;

public:
    EDA_VALUE_CTRL( wxWindow* parent, const wxString& title, int value,
                    EDA_UNITS_T user_unit, wxBoxSizer* BoxSizer );

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
