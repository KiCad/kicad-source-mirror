/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2024 KiCad Developers, see AUTHORS.txt for contributors.
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
#ifndef DESIGN_BLOCK_PANE_H
#define DESIGN_BLOCK_PANE_H

#include <design_block_tree_model_adapter.h>
#include <widgets/html_window.h>
#include <widgets/wx_panel.h>
#include <wx/checkbox.h>
#include <wx/filedlgcustomize.h>
#include <eeschema_settings.h>


class SCH_EDIT_FRAME;
class PANEL_DESIGN_BLOCK_CHOOSER;


class DESIGN_BLOCK_PANE : public WX_PANEL
{
public:
    /**
     * Create dialog to choose design_block.
     *
     * @param aParent   a SCH_BASE_FRAME parent window.
     * @param aAllowFieldEdits  if false, all functions that allow the user to edit fields
     *                          (currently just footprint selection) will not be available.
     * @param aShowFootprints   if false, all footprint preview and selection features are
     *                          disabled. This forces aAllowFieldEdits false too.
     */
    DESIGN_BLOCK_PANE( SCH_EDIT_FRAME* aParent, const LIB_ID* aPreselect,
                       std::vector<LIB_ID>& aHistoryList );

    /**
     * To be called after this dialog returns from ShowModal().
     *
     * For multi-unit design_blocks, if the user selects the design_block itself rather than picking
     * an individual unit, 0 will be returned in aUnit.
     * Beware that this is an invalid unit number - this should be replaced with whatever
     * default is desired (usually 1).
     *
     * @param aUnit if not NULL, the selected unit is filled in here.
     * @return the #LIB_ID of the design_block that has been selected.
     */
    LIB_ID GetSelectedLibId( int* aUnit = nullptr ) const;
    void   SelectLibId( const LIB_ID& aLibId );

    void RefreshLibs();

    /* Handler for checkbox events */
    void OnCheckBox( wxCommandEvent& aEvent );
    void UpdateCheckboxes();

    void OnSaveSheetAsDesignBlock( wxCommandEvent& aEvent );
    void OnSaveSelectionAsDesignBlock( wxCommandEvent& aEvent );

    void OnDeleteLibrary( wxCommandEvent& aEvent );
    void OnDeleteDesignBlock( wxCommandEvent& aEvent );

    PANEL_DESIGN_BLOCK_CHOOSER* GetDesignBlockPanel() const { return m_chooserPanel; }

protected:
    PANEL_DESIGN_BLOCK_CHOOSER* m_chooserPanel;

    wxCheckBox*                 m_repeatedPlacement;
    wxCheckBox*                 m_placeAsSheet;
    wxCheckBox*                 m_keepAnnotations;

    SCH_EDIT_FRAME*             m_frame;
};


// This is a helper class for the file dialog to allow the user to choose similar options
// as the design block chooser when importing a sheet.
class FILEDLG_IMPORT_SHEET_CONTENTS : public wxFileDialogCustomizeHook
{
public:
    FILEDLG_IMPORT_SHEET_CONTENTS( EESCHEMA_SETTINGS* aSettings );

    virtual void AddCustomControls( wxFileDialogCustomize& customizer ) override;

    virtual void TransferDataFromCustomControls() override;

private:
    EESCHEMA_SETTINGS* m_settings;

    wxFileDialogCheckBox* m_cbRepeatedPlacement;
    wxFileDialogCheckBox* m_cbPlaceAsSheet;
    wxFileDialogCheckBox* m_cbKeepAnnotations;

    wxDECLARE_NO_COPY_CLASS( FILEDLG_IMPORT_SHEET_CONTENTS );
};

#endif
