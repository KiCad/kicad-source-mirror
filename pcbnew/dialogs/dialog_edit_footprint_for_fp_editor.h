/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2010-2015 Jean-Pierre Charras, jean-pierre.charras at wanadoo.fr
 * Copyright (C) 1992-2015 KiCad Developers, see AUTHORS.txt for contributors.
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

#ifndef DIALOG_EDIT_FOOTPRINT_FOR_MODEDIT_H
#define DIALOG_EDIT_FOOTPRINT_FOR_MODEDIT_H

#include <vector>
#include <text_mod_grid_table.h>
#include <widgets/unit_binder.h>
#include <class_module.h>
#include <dialog_edit_footprint_for_fp_editor_base.h>


class PANEL_PREV_3D;
class FOOTPRINT_EDIT_FRAME;


class DIALOG_FOOTPRINT_FP_EDITOR : public DIALOG_FOOTPRINT_FP_EDITOR_BASE
{
private:
    wxConfigBase*                    m_config;
    FOOTPRINT_EDIT_FRAME*            m_frame;
    MODULE*                          m_footprint;

    static int                       m_page;       // remember the last open page during session

    TEXT_MOD_GRID_TABLE*             m_texts;

    UNIT_BINDER                      m_netClearance;
    UNIT_BINDER                      m_solderMask;
    UNIT_BINDER                      m_solderPaste;

    std::vector<MODULE_3D_SETTINGS>  m_shapes3D_list;
    PANEL_PREV_3D*                   m_PreviewPane;

    wxControl*                       m_delayedFocusCtrl;
    int                              m_delayedFocusPage;

    WX_GRID*                         m_delayedFocusGrid;
    int                              m_delayedFocusRow;
    int                              m_delayedFocusColumn;
    wxString                         m_delayedErrorMessage;

    bool                             m_inSelect;

public:
    // Constructor and destructor
    DIALOG_FOOTPRINT_FP_EDITOR( FOOTPRINT_EDIT_FRAME* aParent, MODULE* aModule );
    ~DIALOG_FOOTPRINT_FP_EDITOR() override;

    bool Validate() override;

    bool TransferDataToWindow() override;
    bool TransferDataFromWindow() override;

private:
    // virtual event functions
    void On3DModelSelected( wxGridEvent&  ) override;
    void On3DModelCellChanged( wxGridEvent& aEvent ) override;
    void OnRemove3DModel( wxCommandEvent& event ) override;
    void OnAdd3DModel( wxCommandEvent& event ) override;
    void OnAdd3DRow( wxCommandEvent& event ) override;
    void Cfg3DPath( wxCommandEvent& event ) override;
    void OnGridSize( wxSizeEvent& event ) override;
    void OnFootprintNameText( wxCommandEvent& event ) override;
    void OnGridCellChanging( wxGridEvent& event );
    void OnAddField( wxCommandEvent& event ) override;
    void OnDeleteField( wxCommandEvent& event ) override;
    void OnUpdateUI( wxUpdateUIEvent& event ) override;

    bool checkFootprintName( const wxString& aFootprintName );

    void select3DModel( int aModelIdx );

    void adjustGridColumns( int aWidth );
};


#endif      //  DIALOG_EDIT_FOOTPRINT_FOR_MODEDIT_H
