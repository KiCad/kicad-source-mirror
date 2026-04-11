/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2024 KiCad Developers, see AUTHORS.txt for contributors.
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

#ifndef DRC_RE_MATCHED_LENGTH_DIFF_PAIR_OVERLAY_PANEL_H
#define DRC_RE_MATCHED_LENGTH_DIFF_PAIR_OVERLAY_PANEL_H

#include <memory>

#include <units_provider.h>

#include "drc_re_bitmap_overlay_panel.h"

enum class EDA_UNITS;

class DRC_RE_MATCHED_LENGTH_DIFF_PAIR_CONSTRAINT_DATA;
class UNIT_BINDER;
class wxCheckBox;


class DRC_RE_MATCHED_LENGTH_DIFF_PAIR_OVERLAY_PANEL : public DRC_RE_BITMAP_OVERLAY_PANEL
{
public:
    DRC_RE_MATCHED_LENGTH_DIFF_PAIR_OVERLAY_PANEL( wxWindow*                                        aParent,
                                                   DRC_RE_MATCHED_LENGTH_DIFF_PAIR_CONSTRAINT_DATA* aData,
                                                   EDA_UNITS                                        aUnits );

    ~DRC_RE_MATCHED_LENGTH_DIFF_PAIR_OVERLAY_PANEL() override = default;

    bool TransferDataToWindow() override;
    bool TransferDataFromWindow() override;

    bool     ValidateInputs( int* aErrorCount, wxString* aValidationMessage ) override;
    wxString GenerateRule( const RULE_GENERATION_CONTEXT& aContext ) override;

private:
    DRC_RE_MATCHED_LENGTH_DIFF_PAIR_CONSTRAINT_DATA* m_data;

    UNITS_PROVIDER m_unitsProvider;

    std::unique_ptr<UNIT_BINDER> m_optLengthBinder;
    std::unique_ptr<UNIT_BINDER> m_toleranceBinder;
    std::unique_ptr<UNIT_BINDER> m_maxSkewBinder;

    wxCheckBox* m_withinDiffPairsCheckbox;
};

#endif // DRC_RE_MATCHED_LENGTH_DIFF_PAIR_OVERLAY_PANEL_H
