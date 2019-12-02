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
#include <../common/dialogs/dialog_list_selector_base.h>

void ConvertMarkdown2Html( const wxString& aMarkdownInput, wxString& aHtmlOutput );

class EDA_DRAW_FRAME;

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
     * @param aItemHeaders an optional array containing the column header names for the dialog.
     * @param aItemList = A wxArrayString of the list of elements.
     * @param aRefText = An item name if an item must be preselected.
     * @param aCallBackFunction = callback function to display comments
     * @param aCallBackFunctionData = a pointer to pass to @a aCallBackFunction
     */
    EDA_LIST_DIALOG( EDA_DRAW_FRAME* aParent, const wxString& aTitle,
                     const wxArrayString& aItemHeaders,
                     const std::vector<wxArrayString>& aItemList,
                     const wxString& aRefText,
                     void (* aCallBackFunction)( wxString& text, void* data ) = NULL,
                     void* aCallBackFunctionData = NULL );

    // ~EDA_LIST_DIALOG() {}

    void     SetListLabel( const wxString& aLabel );
    void     SetOKLabel( const wxString& aLabel );

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
    void     onListItemSelected( wxListEvent& event ) override;
    void     onListItemActivated( wxListEvent& event ) override;
    void     textChangeInFilterBox(wxCommandEvent& event) override;

    void    initDialog( const wxArrayString& aItemHeaders,
                        const wxString& aSelection);
    void    sortList();
    void    (* m_cb_func)( wxString& text, void* data );
    void*   m_cb_data;
    const   std::vector<wxArrayString>* m_itemsListCp;
};


/**************************************************************************/
/* Class to edit/enter a coordinate (pair of values) ( INCHES or MM ) in  */
/* dialog boxes,                                                          */
/**************************************************************************/
class EDA_POSITION_CTRL
{
public:
    EDA_UNITS_T   m_UserUnit;

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



#endif    // DIALOG_HELPERS_H_
