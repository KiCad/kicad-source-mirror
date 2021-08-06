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

#ifndef DIALOG_FOOTPRINT_PROPERTIES_FP_EDITOR_H
#define DIALOG_FOOTPRINT_PROPERTIES_FP_EDITOR_H

#include <vector>
#include <fp_text_grid_table.h>
#include <widgets/unit_binder.h>
#include <footprint.h>
#include <dialog_footprint_properties_fp_editor_base.h>


class FOOTPRINT_EDIT_FRAME;
class PANEL_FP_PROPERTIES_3D_MODEL;


enum class NOTEBOOK_PAGES
{
    PAGE_UNKNOWN = -1,
    PAGE_GENERAL = 0,
    PAGE_CLEARANCES = 1,
    PAGE_3D_MODELS = 2
};

class DIALOG_FOOTPRINT_PROPERTIES_FP_EDITOR : public DIALOG_FOOTPRINT_PROPERTIES_FP_EDITOR_BASE
{
public:
    DIALOG_FOOTPRINT_PROPERTIES_FP_EDITOR( FOOTPRINT_EDIT_FRAME* aParent, FOOTPRINT* aFootprint );
    ~DIALOG_FOOTPRINT_PROPERTIES_FP_EDITOR() override;

    bool Validate() override;

    bool TransferDataToWindow() override;
    bool TransferDataFromWindow() override;

private:
    // virtual event functions
    void OnGridSize( wxSizeEvent& event ) override;
    void OnFootprintNameText( wxCommandEvent& event ) override;
    void OnGridCellChanging( wxGridEvent& event );
    void OnAddField( wxCommandEvent& event ) override;
    void OnDeleteField( wxCommandEvent& event ) override;
    void OnUpdateUI( wxUpdateUIEvent& event ) override;

    bool checkFootprintName( const wxString& aFootprintName );

    void adjustGridColumns( int aWidth );

private:
    FOOTPRINT_EDIT_FRAME*    m_frame;
    FOOTPRINT*               m_footprint;

    static NOTEBOOK_PAGES    m_page;       // remember the last open page during session

    FP_TEXT_GRID_TABLE*      m_texts;

    UNIT_BINDER              m_netClearance;
    UNIT_BINDER              m_solderMask;
    UNIT_BINDER              m_solderPaste;
    UNIT_BINDER              m_solderPasteRatio;

    wxControl*               m_delayedFocusCtrl;
    NOTEBOOK_PAGES           m_delayedFocusPage;

    WX_GRID*                 m_delayedFocusGrid;
    int                      m_delayedFocusRow;
    int                      m_delayedFocusColumn;
    wxString                 m_delayedErrorMessage;

    PANEL_FP_PROPERTIES_3D_MODEL* m_3dPanel;
};


#endif      //  DIALOG_FOOTPRINT_PROPERTIES_FP_EDITOR_H
