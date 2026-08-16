/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#pragma once

#include "dialog_fp_edit_pad_table_base.h"
#include "string_utils.h"

#include <memory>
#include <vector>
#include <pad.h>

class PCB_BASE_FRAME;
class FOOTPRINT;
class UNITS_PROVIDER;

class DIALOG_FP_EDIT_PAD_TABLE : public DIALOG_FP_EDIT_PAD_TABLE_BASE
{
public:
    // Column indices (after adding Type column)
    enum COLS
    {
        COL_NUMBER = 0,
        COL_TYPE,
        COL_SHAPE,
        COL_POS_X,
        COL_POS_Y,
        COL_SIZE_X,
        COL_SIZE_Y,
        COL_DRILL_X,
        COL_DRILL_Y,
        COL_P2D_LENGTH,
        COL_P2D_DELAY,
        COL_COUNT,
    };

    DIALOG_FP_EDIT_PAD_TABLE( PCB_BASE_FRAME* aParent, FOOTPRINT* aFootprint );
    ~DIALOG_FP_EDIT_PAD_TABLE() override;

    bool TransferDataToWindow() override;
    bool TransferDataFromWindow() override;
    void CaptureOriginalPadState();
    void RestoreOriginalPadState();

private:
    void OnSize( wxSizeEvent& aEvent ) override;
    void OnCharHook( wxKeyEvent& aEvent ) override;
    void OnCellChanged( wxGridEvent& aEvent ) override;
    void OnSelectCell( wxGridEvent& aEvent ) override;
    void OnUpdateUI( wxUpdateUIEvent& aEvent ) override;
    void OnCancel( wxCommandEvent& aEvent ) override;
    void OnExportButtonClick( wxCommandEvent& aEvent ) override;
    void OnImportButtonClick( wxCommandEvent& aEvent ) override;

    void InitColumnProportions();

    void updateSummary();
    void setRowNullableEditors( int aRowId ) const;

    void setPadFromGridCell( PAD& aPad, int aRowId, COLS aCol );
    void restoreOriginalPadData();

    PAD* getPadForRow( int aRowId ) const;

private:
    PCB_BASE_FRAME*     m_frame;

    // Proportional resize support
    std::vector<double> m_colProportions;    // relative widths captured after init
    std::vector<int>    m_minColWidths;      // initial (minimum) widths

    struct PAD_SNAPSHOT
    {
        explicit PAD_SNAPSHOT( PAD* aPad ) :
                padstack( aPad )
        {
        }

        wxString   number;
        PAD_SHAPE  shape{ PAD_SHAPE::CHAMFERED_RECT };
        PADSTACK   padstack;
        VECTOR2I   position;
        VECTOR2I   size;
        PAD_ATTRIB attribute{ PAD_ATTRIB::PTH };
        int        padToDieLength{ 0 };
        int        padToDieDelay{ 0 };
    };

    void restorePadFromSnapshot( PAD& aPad, const PAD_SNAPSHOT& aSnap ) const;

    // Comparison function to order the pads in the map
    struct PAD_SNAPSHOT_COMPARE
    {
        bool operator()( const PAD* a, const PAD* b ) const
        {
            const int cmpVal = StrNumCmp( a->GetNumber(), b->GetNumber() );

            // First sort by alphanumeric ordering
            if( cmpVal < 0 )
                return true;

            if( cmpVal > 0 )
                return false;

            // Sort by x and then y
            if( a->GetCenter().x < b->GetCenter().x )
                return true;

            if( a->GetCenter().x > b->GetCenter().x )
                return false;

            if( a->GetCenter().y < b->GetCenter().y )
                return true;

            if( a->GetCenter().y > b->GetCenter().y )
                return false;

            // For degenerate pads, sort by raw pointer value
            return a < b;
        }
    };

    // Original pad data for cancel rollback, keyed by raw pointer: pad numbers
    // can be edited in the grid, so no value-dependent ordering may be used for
    // the key.
    std::map<PAD*, PAD_SNAPSHOT> m_originalPads;

    // Pads shown in the grid, ordered by pad number.
    std::vector<PAD*> m_rowPads;

    // Pads removed during the dialog session (by import, and later by row
    // deletion), kept alive until the dialog is accepted or cancelled.
    std::vector<PAD*> m_removedPads;

    bool                              m_cancelled = false; // set if user hit cancel

    FOOTPRINT*                        m_footprint;
    std::unique_ptr<UNITS_PROVIDER>   m_unitsProvider;
    bool                              m_summaryDirty;
};
