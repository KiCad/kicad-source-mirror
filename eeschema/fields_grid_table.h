/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2018-2022 KiCad Developers, see AUTHORS.txt for contributors.
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

#ifndef FIELDS_GRID_TABLE_H
#define FIELDS_GRID_TABLE_H

#include <sch_validators.h>
#include <wx/grid.h>
#include <sch_symbol.h>
#include <grid_tricks.h>
#include <validators.h>

class SCH_BASE_FRAME;
class DIALOG_SHIM;
class SCH_LABEL_BASE;


class FIELDS_GRID_TRICKS : public GRID_TRICKS
{
public:
    FIELDS_GRID_TRICKS( WX_GRID* aGrid, DIALOG_SHIM* aDialog,
                        std::function<void( wxCommandEvent& )> aAddHandler ) :
        GRID_TRICKS( aGrid, std::move( aAddHandler ) ),
        m_dlg( aDialog )
    {}

protected:
    virtual void showPopupMenu( wxMenu& menu, wxGridEvent& aEvent ) override;
    virtual void doPopupSelection( wxCommandEvent& event ) override;
    DIALOG_SHIM* m_dlg;
};


enum FIELDS_DATA_COL_ORDER
{
    FDC_NAME,
    FDC_VALUE,
    FDC_SHOWN,
    FDC_SHOW_NAME,
    FDC_H_ALIGN,
    FDC_V_ALIGN,
    FDC_ITALIC,
    FDC_BOLD,
    FDC_TEXT_SIZE,
    FDC_ORIENTATION,
    FDC_POSX,
    FDC_POSY,
    FDC_FONT,
    FDC_COLOR,
    FDC_ALLOW_AUTOPLACE,

    FDC_COUNT       // keep as last
};


template <class T>
class FIELDS_GRID_TABLE : public wxGridTableBase, public std::vector<T>
{
public:
    FIELDS_GRID_TABLE( DIALOG_SHIM* aDialog, SCH_BASE_FRAME* aFrame, WX_GRID* aGrid,
                       LIB_SYMBOL* aSymbol );
    FIELDS_GRID_TABLE( DIALOG_SHIM* aDialog, SCH_EDIT_FRAME* aFrame, WX_GRID* aGrid,
                       SCH_SYMBOL* aSymbol );
    FIELDS_GRID_TABLE( DIALOG_SHIM* aDialog, SCH_EDIT_FRAME* aFrame, WX_GRID* aGrid,
                       SCH_SHEET* aSheet );
    FIELDS_GRID_TABLE( DIALOG_SHIM* aDialog, SCH_EDIT_FRAME* aFrame, WX_GRID* aGrid,
                       SCH_LABEL_BASE* aLabel );
    ~FIELDS_GRID_TABLE();

    int GetNumberRows() override { return (int) this->size(); }
    int GetNumberCols() override { return FDC_COUNT; }

    wxString GetColLabelValue( int aCol ) override;

    bool IsEmptyCell( int row, int col ) override
    {
        return false;   // don't allow adjacent cell overflow, even if we are actually empty
    }

    bool CanGetValueAs( int aRow, int aCol, const wxString& aTypeName ) override;
    bool CanSetValueAs( int aRow, int aCol, const wxString& aTypeName ) override;
    wxGridCellAttr* GetAttr( int row, int col, wxGridCellAttr::wxAttrKind kind ) override;

    wxString GetValue( int aRow, int aCol ) override;
    bool GetValueAsBool( int aRow, int aCol ) override;

    void SetValue( int aRow, int aCol, const wxString& aValue ) override;
    void SetValueAsBool( int aRow, int aCol, bool aValue ) override;

    wxString StringFromBool( bool aValue ) const;
    bool BoolFromString( wxString aValue ) const;

protected:
    void initGrid( WX_GRID* aGrid );

    void onUnitsChanged( wxCommandEvent& aEvent );

private:
    SCH_BASE_FRAME* m_frame;
    DIALOG_SHIM*    m_dialog;
    WX_GRID*        m_grid;
    KICAD_T         m_parentType;
    int             m_mandatoryFieldCount;
    LIB_SYMBOL*     m_part;
    wxString        m_symbolNetlist;
    wxString        m_curdir;

    FIELD_VALIDATOR   m_fieldNameValidator;
    FIELD_VALIDATOR   m_referenceValidator;
    FIELD_VALIDATOR   m_valueValidator;
    FIELD_VALIDATOR   m_urlValidator;
    FIELD_VALIDATOR   m_nonUrlValidator;
    FIELD_VALIDATOR   m_filepathValidator;

    wxGridCellAttr*       m_readOnlyAttr;
    wxGridCellAttr*       m_fieldNameAttr;
    wxGridCellAttr*       m_referenceAttr;
    wxGridCellAttr*       m_valueAttr;
    wxGridCellAttr*       m_footprintAttr;
    wxGridCellAttr*       m_urlAttr;
    wxGridCellAttr*       m_nonUrlAttr;
    wxGridCellAttr*       m_filepathAttr;
    wxGridCellAttr*       m_boolAttr;
    wxGridCellAttr*       m_vAlignAttr;
    wxGridCellAttr*       m_hAlignAttr;
    wxGridCellAttr*       m_orientationAttr;
    wxGridCellAttr*       m_netclassAttr;
    wxGridCellAttr*       m_fontAttr;
    wxGridCellAttr*       m_colorAttr;

    std::unique_ptr<NUMERIC_EVALUATOR>        m_eval;
    std::map< std::pair<int, int>, wxString > m_evalOriginal;
};


#endif  // FIELDS_GRID_TABLE_H
