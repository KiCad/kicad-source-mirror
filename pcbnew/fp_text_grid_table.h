/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2018-2021 KiCad Developers, see AUTHORS.txt for contributors.
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

#ifndef FP_TEXT_GRID_TABLE_H
#define FP_TEXT_GRID_TABLE_H

#include <base_units.h>
#include <wx/grid.h>
#include <grid_tricks.h>
#include <fp_text.h>


class FOOTPRINT;
class PCB_BASE_FRAME;

enum FP_TEXT_COL_ORDER
{
    FPT_TEXT,
    FPT_SHOWN,
    FPT_WIDTH,
    FPT_HEIGHT,
    FPT_THICKNESS,
    FPT_ITALIC,
    FPT_LAYER,
    FPT_ORIENTATION,
    FPT_UPRIGHT,       // keep text upright when viewed from bottom or right of board
    FPT_XOFFSET,
    FPT_YOFFSET,

    FPT_COUNT          // keep as last
};


class FP_TEXT_GRID_TABLE : public wxGridTableBase, public std::vector<FP_TEXT>
{
public:
    FP_TEXT_GRID_TABLE( EDA_UNITS userUnits, PCB_BASE_FRAME* aFrame );
    ~FP_TEXT_GRID_TABLE();

    int GetNumberRows() override { return (int) size(); }
    int GetNumberCols() override { return FPT_COUNT; }

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

    void SetValue( int aRow, int aCol, const wxString& aValue ) override;
    void SetValueAsBool( int aRow, int aCol, bool aValue ) override;
    void SetValueAsLong( int aRow, int aCol, long aValue ) override;

private:
    EDA_UNITS       m_userUnits;
    PCB_BASE_FRAME* m_frame;

    wxGridCellAttr* m_readOnlyAttr;
    wxGridCellAttr* m_boolColAttr;
    wxGridCellAttr* m_orientationColAttr;
    wxGridCellAttr* m_layerColAttr;
};


#endif  // FP_TEXT_GRID_TABLE_H
