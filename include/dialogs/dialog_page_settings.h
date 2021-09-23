/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2020-2021 KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef _DIALOG_PAGES_SETTINGS_H_
#define _DIALOG_PAGES_SETTINGS_H_

#include <page_info.h>
#include <title_block.h>
#include <widgets/unit_binder.h>
#include <dialog_page_settings_base.h>

class DS_DATA_MODEL;

/*!
 * DIALOG_PAGES_SETTINGS class declaration
 */

class DIALOG_PAGES_SETTINGS: public DIALOG_PAGES_SETTINGS_BASE
{
public:
    DIALOG_PAGES_SETTINGS( EDA_DRAW_FRAME* aParent, double aIuPerMils,
                           const wxSize& aMaxUserSizeMils );
    virtual ~DIALOG_PAGES_SETTINGS();

    const wxString GetWksFileName()
    {
        return m_textCtrlFilePicker->GetValue();
    }

    void SetWksFileName(const wxString& aFilename )
    {
         m_textCtrlFilePicker->SetValue( aFilename );
    }

    void EnableWksFileNamePicker( bool aEnable )
    {
         m_textCtrlFilePicker->Enable( aEnable );
         m_browseButton->Enable( aEnable );
    }

private:
    virtual void onTransferDataToWindow() {}

    virtual bool onSavePageSettings()
    {
        // default just return true savepagesettings to succeed
        return true;
    }

    virtual bool TransferDataToWindow() override;
    virtual bool TransferDataFromWindow() override;

    // event handlers for page size choice
    void OnPaperSizeChoice( wxCommandEvent& event ) override;
    void OnUserPageSizeXTextUpdated( wxCommandEvent& event ) override;
    void OnUserPageSizeYTextUpdated( wxCommandEvent& event ) override;
    void OnPageOrientationChoice( wxCommandEvent& event ) override;

    // event handler for texts in title block
    void OnRevisionTextUpdated( wxCommandEvent& event ) override;
    void OnDateTextUpdated( wxCommandEvent& event ) override;
    void OnTitleTextUpdated( wxCommandEvent& event ) override;
    void OnCompanyTextUpdated( wxCommandEvent& event ) override;
    void OnComment1TextUpdated( wxCommandEvent& event ) override;
    void OnComment2TextUpdated( wxCommandEvent& event ) override;
    void OnComment3TextUpdated( wxCommandEvent& event ) override;
    void OnComment4TextUpdated( wxCommandEvent& event ) override;
    void OnComment5TextUpdated( wxCommandEvent& event ) override;
    void OnComment6TextUpdated( wxCommandEvent& event ) override;
    void OnComment7TextUpdated( wxCommandEvent& event ) override;
    void OnComment8TextUpdated( wxCommandEvent& event ) override;
    void OnComment9TextUpdated( wxCommandEvent& event ) override;

    // Handle button click for setting the date from the picker
    void OnDateApplyClick( wxCommandEvent& event ) override;

    // .kicad_wks file description selection
    void OnWksFileSelection( wxCommandEvent& event ) override;

    // Save in the current title block the new page settings
    // return true if changes are made, or false if not
    bool SavePageSettings();

    void SetCurrentPageSizeSelection( const wxString& aPaperSize );

    // Update drawing sheet example
    void UpdateDrawingSheetExample();

    // Get page layout info from selected dialog items
    void GetPageLayoutInfoFromDialog();

    // Get custom page size in mils from dialog
    void GetCustomSizeMilsFromDialog();

    /// @return true if the local prj config is chande
    /// i.e. if the drawing sheet file has chnaged
    bool LocalPrjConfigChanged() { return m_localPrjConfigChanged; }

protected:
    EDA_DRAW_FRAME* m_parent;
    BASE_SCREEN*    m_screen;
    wxString        m_projectPath; // the curr project path
    wxArrayString   m_pageFmt;     /// list of page sizes (not translated)
    bool            m_initialized;
    bool            m_localPrjConfigChanged; /// the page layuout filename was changed
    wxBitmap*       m_pageBitmap;            /// Temporary bitmap for the drawing sheet example.
    wxSize          m_layout_size;           /// Logical drawing sheet size.
    wxSize          m_maxPageSizeMils;       /// The max page size allowed by the caller frame
    PAGE_INFO       m_pageInfo;              /// Temporary page info.
    bool            m_customFmt;             /// true if the page selection is custom
    TITLE_BLOCK     m_tb;                    /// Temporary title block (basic inscriptions).
    DS_DATA_MODEL*  m_drawingSheet; // the alternate and temporary drawing sheet shown by the
                                    // dialog when the initial one is replaced by a new one
    double          m_iuPerMils;

private:
    UNIT_BINDER m_customSizeX;
    UNIT_BINDER m_customSizeY;
};

#endif  // _DIALOG_PAGES_SETTINGS_H_
