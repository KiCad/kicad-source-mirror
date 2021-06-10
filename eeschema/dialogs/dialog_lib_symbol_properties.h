/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 1992-2021 KiCad Developers, see AUTHORS.txt for contributors.
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


#ifndef DIALOG_LIB_SYMBOL_PROPERTIES_H
#define DIALOG_LIB_SYMBOL_PROPERTIES_H

#include <fields_grid_table.h>
#include <widgets/unit_binder.h>
#include <dialog_lib_symbol_properties_base.h>


class SYMBOL_EDIT_FRAME;
class LIB_SYMBOL;
class WX_GRID;


class DIALOG_LIB_SYMBOL_PROPERTIES: public DIALOG_LIB_SYMBOL_PROPERTIES_BASE
{
public:
    DIALOG_LIB_SYMBOL_PROPERTIES( SYMBOL_EDIT_FRAME* parent, LIB_SYMBOL* aLibEntry );
    ~DIALOG_LIB_SYMBOL_PROPERTIES();

protected:
    bool TransferDataToWindow() override;
    bool TransferDataFromWindow() override;

    bool Validate() override;

    virtual void onPowerCheckBox( wxCommandEvent& aEvent ) override;

private:
    void OnAddField( wxCommandEvent& event ) override;
    void OnDeleteField( wxCommandEvent& event ) override;
    void OnMoveUp( wxCommandEvent& event ) override;
    void OnMoveDown( wxCommandEvent& event ) override;
    void OnSymbolNameKillFocus( wxFocusEvent& event ) override;
    void OnSymbolNameText( wxCommandEvent& event ) override;
    void OnAddFootprintFilter( wxCommandEvent& event ) override;
    void OnDeleteFootprintFilter( wxCommandEvent& event ) override;
    void OnEditFootprintFilter( wxCommandEvent& event ) override;
    void OnSizeGrid( wxSizeEvent& event ) override;
    void OnGridCellChanging( wxGridEvent& event );
    void OnEditSpiceModel( wxCommandEvent& event ) override;
    void OnUpdateUI( wxUpdateUIEvent& event ) override;
    void OnFilterDClick( wxMouseEvent& event ) override;
    void OnCancelButtonClick( wxCommandEvent& event ) override;

    void adjustGridColumns( int aWidth );
    void syncControlStates( bool aIsAlias );

public:
    SYMBOL_EDIT_FRAME* m_Parent;
    LIB_SYMBOL*        m_libEntry;

    FIELDS_GRID_TABLE<LIB_FIELD>* m_fields;

    UNIT_BINDER        m_pinNameOffset;

    wxControl*         m_delayedFocusCtrl;
    WX_GRID*           m_delayedFocusGrid;
    int                m_delayedFocusRow;
    int                m_delayedFocusColumn;
    int                m_delayedFocusPage;
    wxString           m_delayedErrorMessage;

    wxString           m_shownColumns;
    int                m_width;

private:
    static int m_lastOpenedPage;    // To remember the last notebook selection

    enum LAST_LAYOUT {
        NONE,
        ALIAS,
        PARENT
    };

    static LAST_LAYOUT m_lastLayout;
};

#endif // DIALOG_LIB_SYMBOL_PROPERTIES_H
