/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2010-2015 Jean-Pierre Charras, jean-pierre.charras at wanadoo.fr
 * Copyright (C) 1992-2016 KiCad Developers, see AUTHORS.txt for contributors.
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
#include <fp_text_grid_table.h>
#include <footprint.h>
#include <widgets/unit_binder.h>


class PCB_EDIT_FRAME;
class PANEL_PREV_3D;


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

private:
    PCB_EDIT_FRAME*                  m_frame;
    FOOTPRINT*                       m_footprint;

    static int                       m_page;       // remember the last open page during session

    FP_TEXT_GRID_TABLE*             m_texts;
    UNIT_BINDER                      m_posX;
    UNIT_BINDER                      m_posY;
    wxFloatingPointValidator<double> m_OrientValidator;
    double                           m_OrientValue;

    UNIT_BINDER                      m_netClearance;
    UNIT_BINDER                      m_solderMask;
    UNIT_BINDER                      m_solderPaste;

    std::vector<FP_3DMODEL>          m_shapes3D_list;
    PANEL_PREV_3D*                   m_PreviewPane;

    wxString                         m_delayedErrorMessage;
    wxGrid*                          m_delayedFocusGrid;
    int                              m_delayedFocusRow;
    int                              m_delayedFocusColumn;
    bool                             m_initialFocus;

    bool                             m_inSelect;
    std::vector<bool>                m_macHack;
    enum FP_PROPS_RETVALUE           m_returnValue; // the option that closed the dialog

public:
    // Constructor and destructor
    DIALOG_FOOTPRINT_PROPERTIES( PCB_EDIT_FRAME* aParent, FOOTPRINT* aFootprint );
    ~DIALOG_FOOTPRINT_PROPERTIES() override;

    bool Validate() override;

    bool TransferDataToWindow() override;
    bool TransferDataFromWindow() override;

    /// @return the value depending on the way the dialog was closed:
    enum FP_PROPS_RETVALUE GetReturnValue() { return m_returnValue; }

private:
    // virtual event functions
    void On3DModelSelected( wxGridEvent&  ) override;
    void On3DModelCellChanged( wxGridEvent& aEvent ) override;
    void OnRemove3DModel( wxCommandEvent&  ) override;
    void OnAdd3DModel( wxCommandEvent&  ) override;
    void OnAdd3DRow( wxCommandEvent&  ) override;
    void EditFootprint( wxCommandEvent&  ) override;
    void EditLibraryFootprint( wxCommandEvent&  ) override;
    void UpdateFootprint( wxCommandEvent&  ) override;
    void ChangeFootprint( wxCommandEvent&  ) override;
    void FootprintOrientEvent( wxCommandEvent&  ) override;
    void OnOtherOrientation( wxCommandEvent& aEvent ) override;
    void Cfg3DPath( wxCommandEvent&  ) override;
    void OnGridSize( wxSizeEvent& aEvent ) override;
    void OnAddField( wxCommandEvent&  ) override;
    void OnDeleteField( wxCommandEvent&  ) override;
    void OnUpdateUI( wxUpdateUIEvent&  ) override;
    void OnPageChange( wxNotebookEvent& event ) override;

    void select3DModel( int aModelIdx );

    void adjustGridColumns( int aWidth );

    /**
     * Update the orientation validated control, without triggering a change
     * event on the control (which would update the radio buttons)
     */
    void updateOrientationControl();
};


#endif      // DIALOG_FOOTPRINT_PROPERTIES_H
