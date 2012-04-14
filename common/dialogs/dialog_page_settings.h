/*
 * This program source code file is part of KICAD, a free EDA CAD application.
 *
 * Copyright (C) 1992-2010 <Creator>
 * Copyright (C) 1992-2010 Kicad Developers, see AUTHORS.txt for contributors.
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

#ifndef _DIALOG_PAGES_SETTINGS_H_
#define _DIALOG_PAGES_SETTINGS_H_

#include <dialog_page_settings_base.h>

#define MAX_PAGE_EXAMPLE_SIZE 200

/*!
 * DIALOG_PAGES_SETTINGS class declaration
 */

class DIALOG_PAGES_SETTINGS: public DIALOG_PAGES_SETTINGS_BASE
{
private:
    EDA_DRAW_FRAME* m_Parent;
    BASE_SCREEN*    m_Screen;
    wxArrayString   m_pageFmt;          /// list of page sizes (not translated)
    bool            m_initialized;
    bool            m_modified;
    bool            m_save_flag;
    wxBitmap*       m_page_bitmap;      /// Temporary bitmap for the page layout example.
    wxSize          m_layout_size;      /// Logical page layout size.
    PAGE_INFO       m_pageInfo;         /// Temporary page info.
    TITLE_BLOCK     m_tb;               /// Temporary title block (basic inscriptions).

    static wxSize   s_LastSize;         /// Last position and size.
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

    /// exEVT_COMMAND_CHOICE_SELECTED event handler for ID_CHICE_PAGE_SIZE
    void OnPaperSizeChoice( wxCommandEvent& event );

    /// exEVT_COMMAND_TEXT_UPDATED event handler for ID_TEXTCTRL_USER_PAGE_SIZE_X
    void OnUserPageSizeXTextUpdated( wxCommandEvent& event );

    /// exEVT_COMMAND_TEXT_UPDATED event handler for ID_TEXTCTRL_USER_PAGE_SIZE_Y
    void OnUserPageSizeYTextUpdated( wxCommandEvent& event );

    /// exEVT_COMMAND_CHOICE_SELECTED event handler for ID_CHOICE_PAGE_ORIENTATION
    void OnPageOrientationChoice( wxCommandEvent& event );

    /// exEVT_COMMAND_TEXT_UPDATED event handler for ID_TEXTCTRL_REVISION
    void OnRevisionTextUpdated( wxCommandEvent& event );

    /// exEVT_COMMAND_TEXT_UPDATED event handler for ID_TEXTCTRL_TITLE
    void OnTitleTextUpdated( wxCommandEvent& event );

    /// exEVT_COMMAND_TEXT_UPDATED event handler for ID_TEXTCTRL_COMPANY
    void OnCompanyTextUpdated( wxCommandEvent& event );

    /// exEVT_COMMAND_TEXT_UPDATED event handler for ID_TEXTCTRL_COMMENT1
    void OnComment1TextUpdated( wxCommandEvent& event );

    /// exEVT_COMMAND_TEXT_UPDATED event handler for ID_TEXTCTRL_COMMENT2
    void OnComment2TextUpdated( wxCommandEvent& event );

    /// exEVT_COMMAND_TEXT_UPDATED event handler for ID_TEXTCTRL_COMMENT3
    void OnComment3TextUpdated( wxCommandEvent& event );

    /// exEVT_COMMAND_TEXT_UPDATED event handler for ID_TEXTCTRL_COMMENT4
    void OnComment4TextUpdated( wxCommandEvent& event );

    void SetCurrentPageSizeSelection( const wxString& aPaperSize );

    void SavePageSettings( wxCommandEvent& event );

    /// Update page layout example
    void UpdatePageLayoutExample();

    /// Get page layout info from selected dialog items
    void GetPageLayoutInfoFromDialog();

    /// Get custom page size in mils from dialog
    void GetCustomSizeMilsFromDialog();
};

#endif  // _DIALOG_PAGES_SETTINGS_H_
