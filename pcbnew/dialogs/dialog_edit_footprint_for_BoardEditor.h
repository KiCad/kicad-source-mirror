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


#ifndef DIALOG_EDIT_FOOTPRINT_FOR_BOARDEDITOR_H
#define DIALOG_EDIT_FOOTPRINT_FOR_BOARDEDITOR_H


#include <dialog_edit_footprint_for_BoardEditor_base.h>
#include <wx/valnum.h>
#include <text_mod_grid_table.h>
#include <class_module.h>
#include <widgets/unit_binder.h>


class PCB_EDIT_FRAME;
class PANEL_PREV_3D;


class DIALOG_FOOTPRINT_BOARD_EDITOR: public DIALOG_FOOTPRINT_BOARD_EDITOR_BASE
{
private:
    wxConfigBase*                    m_config;
    PCB_EDIT_FRAME*                  m_frame;
    MODULE*                          m_footprint;

    static int                       m_page;       // remember the last open page during session

    TEXT_MOD_GRID_TABLE*             m_texts;
    UNIT_BINDER                      m_posX;
    UNIT_BINDER                      m_posY;
    wxFloatingPointValidator<double> m_OrientValidator;
    double                           m_OrientValue;

    UNIT_BINDER                      m_netClearance;
    UNIT_BINDER                      m_solderMask;
    UNIT_BINDER                      m_solderPaste;

    std::vector<MODULE_3D_SETTINGS>  m_shapes3D_list;
    PANEL_PREV_3D*                   m_PreviewPane;

    wxString                         m_delayedErrorMessage;
    wxGrid*                          m_delayedFocusGrid;
    int                              m_delayedFocusRow;
    int                              m_delayedFocusColumn;
    bool                             m_initialFocus;

    bool                             m_inSelect;

public:
    // The dialog can be closed for several reasons.
    enum FP_PRM_EDITOR_RETVALUE
    {
        PRM_EDITOR_WANT_UPDATE_FP,
        PRM_EDITOR_WANT_EXCHANGE_FP,
        PRM_EDITOR_EDIT_OK,
        PRM_EDITOR_EDIT_BOARD_FOOTPRINT,
        PRM_EDITOR_EDIT_LIBRARY_FOOTPRINT
    };

public:
    // Constructor and destructor
    DIALOG_FOOTPRINT_BOARD_EDITOR( PCB_EDIT_FRAME* aParent, MODULE* aModule );
    ~DIALOG_FOOTPRINT_BOARD_EDITOR() override;

    bool Validate() override;

    bool TransferDataToWindow() override;
    bool TransferDataFromWindow() override;

private:
    // virtual event functions
    void On3DModelSelected( wxGridEvent&  ) override;
    void On3DModelCellChanged( wxGridEvent& aEvent ) override;
    void OnRemove3DModel( wxCommandEvent&  ) override;
    void OnAdd3DModel( wxCommandEvent&  ) override;
    void OnAdd3DRow( wxCommandEvent&  ) override;
    void EditFootprint( wxCommandEvent&  ) override;
    void EditLibraryFootprint( wxCommandEvent&  ) override;
    void UpdateModule( wxCommandEvent&  ) override;
    void ExchangeModule( wxCommandEvent&  ) override;
    void ModuleOrientEvent( wxCommandEvent&  ) override;
    void OnOtherOrientation( wxCommandEvent& aEvent ) override;
    void Cfg3DPath( wxCommandEvent&  ) override;
    void OnGridSize( wxSizeEvent& aEvent ) override;
    void OnAddField( wxCommandEvent&  ) override;
    void OnDeleteField( wxCommandEvent&  ) override;
    void OnUpdateUI( wxUpdateUIEvent&  ) override;

    void select3DModel( int aModelIdx );

    void adjustGridColumns( int aWidth );

    /**
     * Update the orientation validated control, without triggering a change
     * event on the control (which would update the radio buttons)
     */
    void updateOrientationControl();
};


#endif      // DIALOG_EDIT_FOOTPRINT_FOR_BOARDEDITOR_H
