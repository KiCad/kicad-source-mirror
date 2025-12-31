/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2010-2015 Jean-Pierre Charras, jean-pierre.charras at wanadoo.fr
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
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


#ifndef DIALOG_FOOTPRINT_PROPERTIES_H
#define DIALOG_FOOTPRINT_PROPERTIES_H


#include <dialog_footprint_properties_base.h>
#include <wx/valnum.h>
#include <pcb_fields_grid_table.h>
#include <footprint.h>
#include <widgets/unit_binder.h>
#include <widgets/margin_offset_binder.h>


class PCB_EDIT_FRAME;
class PANEL_FP_PROPERTIES_3D_MODEL;
class PANEL_EMBEDDED_FILES;

class DIALOG_FOOTPRINT_PROPERTIES: public DIALOG_FOOTPRINT_PROPERTIES_BASE
{
public:
    // The dialog can be closed for several reasons.
    enum FP_PROPS_RETVALUE
    {
        FP_PROPS_CANCEL,
        FP_PROPS_UPDATE_FP,
        FP_PROPS_CHANGE_FP,
        FP_PROPS_OK,
        FP_PROPS_EDIT_BOARD_FP,
        FP_PROPS_EDIT_LIBRARY_FP
    };

    DIALOG_FOOTPRINT_PROPERTIES( PCB_EDIT_FRAME* aParent, FOOTPRINT* aFootprint );
    ~DIALOG_FOOTPRINT_PROPERTIES() override;

    bool Validate() override;

    bool TransferDataToWindow() override;
    bool TransferDataFromWindow() override;

    ///< @return the value depending on the way the dialog was closed.
    enum FP_PROPS_RETVALUE GetReturnValue() { return m_returnValue; }

private:
    // virtual event functions
    void EditFootprint( wxCommandEvent&  ) override;
    void EditLibraryFootprint( wxCommandEvent&  ) override;
    void UpdateFootprint( wxCommandEvent&  ) override;
    void ChangeFootprint( wxCommandEvent&  ) override;
    void OnAddField( wxCommandEvent&  ) override;
    void OnDeleteField( wxCommandEvent&  ) override;
    void OnUpdateUI( wxUpdateUIEvent&  ) override;
    void OnPageChanging( wxNotebookEvent& event ) override;
    void OnCombobox( wxCommandEvent& event ) override;
    void OnText( wxCommandEvent& event ) override;
    void OnChoice( wxCommandEvent& event ) override;
    void OnCheckBox( wxCommandEvent& event ) override;

private:
    PCB_EDIT_FRAME*                  m_frame;
    FOOTPRINT*                       m_footprint;

    static int                       m_page;       // remember the last open page during session

    PCB_FIELDS_GRID_TABLE*           m_fields;
    UNIT_BINDER                      m_posX;
    UNIT_BINDER                      m_posY;
    UNIT_BINDER                      m_orientation;

    UNIT_BINDER                      m_netClearance;
    UNIT_BINDER                      m_solderMask;
    MARGIN_OFFSET_BINDER             m_solderPaste;

    wxString                         m_delayedErrorMessage;
    wxGrid*                          m_delayedFocusGrid;
    int                              m_delayedFocusRow;
    int                              m_delayedFocusColumn;
    bool                             m_initialFocus;

    enum FP_PROPS_RETVALUE           m_returnValue; // the option that closed the dialog

    PANEL_FP_PROPERTIES_3D_MODEL*    m_3dPanel;

    bool                             m_initialized;

    PANEL_EMBEDDED_FILES*            m_embeddedFiles;
};


#endif      // DIALOG_FOOTPRINT_PROPERTIES_H
