/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2018 KiCad Developers, see AUTHORS.txt for contributors.
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

#ifndef TEXT_MOD_GRID_TABLE_H
#define TEXT_MOD_GRID_TABLE_H

#include <base_units.h>
#include <wx/grid.h>
#include <grid_tricks.h>
#include <class_text_mod.h>


class MODULE;
class PCB_BASE_FRAME;

enum TEXT_MOD_COL_ORDER
{
    TMC_TEXT,
    TMC_SHOWN,
    TMC_WIDTH,
    TMC_HEIGHT,
    TMC_THICKNESS,
    TMC_ITALIC,
    TMC_LAYER,
    TMC_ORIENTATION,
    TMC_UPRIGHT,       // keep text upright when viewed from bottom or right of board
    TMC_XOFFSET,
    TMC_YOFFSET,

    TMC_COUNT          // keep as last
};


class TEXT_MOD_GRID_TABLE : public wxGridTableBase, public std::vector<TEXTE_MODULE>
{
public:
    TEXT_MOD_GRID_TABLE( EDA_UNITS_T userUnits, PCB_BASE_FRAME* aFrame );
    ~TEXT_MOD_GRID_TABLE();

    int GetNumberRows() override { return (int) size(); }
    int GetNumberCols() override { return TMC_COUNT; }

    wxString GetColLabelValue( int aCol ) override;
    wxString GetRowLabelValue( int aRow ) override;

    bool IsEmptyCell( int row, int col ) override
    {
        return false;   // don't allow adjacent cell overflow, even if we are actually empty
    }

    bool CanGetValueAs( int aRow, int aCol, const wxString& aTypeName ) override;
    bool CanSetValueAs( int aRow, int aCol, const wxString& aTypeName ) override;
    wxGridCellAttr* GetAttr( int row, int col, wxGridCellAttr::wxAttrKind kind ) override;

    wxString GetValue( int aRow, int aCol ) override;
    bool GetValueAsBool( int aRow, int aCol ) override;
    long GetValueAsLong( int aRow, int aCol ) override;

    void SetValue( int aRow, int aCol, const wxString &aValue ) override;
    void SetValueAsBool( int aRow, int aCol, bool aValue ) override;
    void SetValueAsLong( int aRow, int aCol, long aValue ) override;

private:
    EDA_UNITS_T        m_userUnits;
    PCB_BASE_FRAME*    m_frame;

    wxGridCellAttr*    m_readOnlyAttr;
    wxGridCellAttr*    m_boolColAttr;
    wxGridCellAttr*    m_orientationColAttr;
    wxGridCellAttr*    m_layerColAttr;
};


#endif  // TEXT_MOD_GRID_TABLE_H
