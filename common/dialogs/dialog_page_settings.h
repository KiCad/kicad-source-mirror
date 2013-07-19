/*
 * This program source code file is part of KICAD, a free EDA CAD application.
 *
 * Copyright (C) 1992-2013 Kicad Developers, see AUTHORS.txt for contributors.
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
    EDA_DRAW_FRAME* m_parent;
    BASE_SCREEN*    m_screen;
    wxArrayString   m_pageFmt;          /// list of page sizes (not translated)
    bool            m_initialized;
    bool            m_localPrjConfigChanged;    /// the page layuout filename was changed
    wxBitmap*       m_page_bitmap;      /// Temporary bitmap for the page layout example.
    wxSize          m_layout_size;      /// Logical page layout size.
    PAGE_INFO       m_pageInfo;         /// Temporary page info.
    bool            m_customFmt;        /// true if the page selection is custom
    TITLE_BLOCK     m_tb;               /// Temporary title block (basic inscriptions).
    wxString        m_plDescrFileName;  /// Temporary BASE_SCREEN::m_PageLayoutDescrFileName copy



public:
    DIALOG_PAGES_SETTINGS( EDA_DRAW_FRAME* parent );
    ~DIALOG_PAGES_SETTINGS();

    const wxString GetWksFileName()
    {
        return m_filePicker->GetPath();
    }

    void SetWksFileName(const wxString& aFilename )
    {
         m_filePicker->SetPath( aFilename );
    }

    void EnableWksFileNamePicker( bool aEnable )
    {
         m_filePicker->Enable( aEnable );
    }

private:
    void initDialog();  // Initialisation of member variables

//    void OnCloseWindow( wxCloseEvent& event );

    // event handler for wxID_OK
    void OnOkClick( wxCommandEvent& event );

    // event handler for wxID_CANCEL
    void OnCancelClick( wxCommandEvent& event );

    // event handlers for page size choice
    void OnPaperSizeChoice( wxCommandEvent& event );
    void OnUserPageSizeXTextUpdated( wxCommandEvent& event );
    void OnUserPageSizeYTextUpdated( wxCommandEvent& event );
    void OnPageOrientationChoice( wxCommandEvent& event );

    // event handler for texts in title block
    void OnRevisionTextUpdated( wxCommandEvent& event );
    void OnDateTextUpdated( wxCommandEvent& event );
    void OnTitleTextUpdated( wxCommandEvent& event );
    void OnCompanyTextUpdated( wxCommandEvent& event );
    void OnComment1TextUpdated( wxCommandEvent& event );
    void OnComment2TextUpdated( wxCommandEvent& event );
    void OnComment3TextUpdated( wxCommandEvent& event );
    void OnComment4TextUpdated( wxCommandEvent& event );

    // Handle button click for setting the date from the picker
    void OnDateApplyClick( wxCommandEvent& event );

    // .kicad_wks file description selection
	void OnWksFileSelection( wxFileDirPickerEvent& event );

    // Save in the current title block the new page settings
    // return true if changes are made, or false if not
    bool SavePageSettings();

    void SetCurrentPageSizeSelection( const wxString& aPaperSize );

    // Update page layout example
    void UpdatePageLayoutExample();

    // Get page layout info from selected dialog items
    void GetPageLayoutInfoFromDialog();

    // Get custom page size in mils from dialog
    void GetCustomSizeMilsFromDialog();

    /// @return true if the local prj config is chande
    /// i.e. if the page layout descr file has chnaged
    bool LocalPrjConfigChanged() { return m_localPrjConfigChanged; }
};

#endif  // _DIALOG_PAGES_SETTINGS_H_
