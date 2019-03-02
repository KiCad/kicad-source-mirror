/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 1992-2019 KiCad Developers, see AUTHORS.txt for contributors.
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


#ifndef _DIALOG_EDIT_COMPONENT_IN_LIB_H_
#define _DIALOG_EDIT_COMPONENT_IN_LIB_H_

#include <fields_grid_table.h>
#include <class_library.h>
#include <widgets/unit_binder.h>
#include <dialog_edit_component_in_lib_base.h>


class LIB_EDIT_FRAME;
class LIB_PART;
class WX_GRID;


class DIALOG_EDIT_COMPONENT_IN_LIBRARY: public DIALOG_EDIT_COMPONENT_IN_LIBRARY_BASE
{
    static int m_lastOpenedPage;    // To remember the last notebook selection

public:
    wxConfigBase*   m_config;

    LIB_EDIT_FRAME* m_Parent;
    LIB_PART*       m_libEntry;

    FIELDS_GRID_TABLE<LIB_FIELD>* m_fields;

    int             m_currentAlias;
    LIB_ALIASES     m_aliasesBuffer;
    UNIT_BINDER     m_pinNameOffset;

    wxControl*      m_delayedFocusCtrl;
    WX_GRID*        m_delayedFocusGrid;
    int             m_delayedFocusRow;
    int             m_delayedFocusColumn;
    int             m_delayedFocusPage;
    wxString        m_delayedErrorMessage;

    wxString        m_shownColumns;
    int             m_width;

    bool TransferDataToWindow() override;
    bool TransferDataFromWindow() override;

    bool Validate() override;

public:
    /// Constructors
    DIALOG_EDIT_COMPONENT_IN_LIBRARY( LIB_EDIT_FRAME* parent, LIB_PART* aLibEntry );
    ~DIALOG_EDIT_COMPONENT_IN_LIBRARY();

private:
    void transferAliasDataToBuffer();

    void OnAddField( wxCommandEvent& event ) override;
    void OnDeleteField( wxCommandEvent& event ) override;
    void OnMoveUp( wxCommandEvent& event ) override;
    void OnMoveDown( wxCommandEvent& event ) override;
    void OnSymbolNameKillFocus( wxFocusEvent& event ) override;
    void OnSymbolNameText( wxCommandEvent& event ) override;
    void OnSelectAlias( wxCommandEvent& event ) override;
    void OnAddAlias( wxCommandEvent& event ) override;
    void OnDeleteAlias( wxCommandEvent& event ) override;
    void OnAddFootprintFilter( wxCommandEvent& event ) override;
    void OnDeleteFootprintFilter( wxCommandEvent& event ) override;
    void OnEditFootprintFilter( wxCommandEvent& event ) override;
    void OnSizeGrid( wxSizeEvent& event ) override;
    void OnSizeAliasGrid( wxSizeEvent& event ) override;
    void OnGridCellChanging( wxGridEvent& event );
    void OnAliasGridCellChanging( wxGridEvent& event );
    void OnAliasNameKillFocus( wxFocusEvent& event ) override;
    void OnAliasNameText( wxCommandEvent& event ) override;
    void OnEditSpiceModel( wxCommandEvent& event ) override;
    void OnUpdateUI( wxUpdateUIEvent& event ) override;
    void OnFilterDClick( wxMouseEvent& event ) override;
    void OnCancelButtonClick( wxCommandEvent& event ) override;

    void updateAliasName( bool aFromGrid, const wxString& aName );
    bool checkAliasName( const wxString& aName );
    void adjustGridColumns( int aWidth );
    void adjustAliasGridColumns( int aWidth );
};

#endif // _DIALOG_EDIT_COMPONENT_IN_LIB_H_
