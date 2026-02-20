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
 * along with this program; if not, you may find one here:
 * http://www.gnu.org/licenses/old-licenses/gpl-2.0.html
 * or you may search the http://www.gnu.org website for the version 2 license,
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

#ifndef DRC_RE_RTG_DIFF_PAIR_OVERLAY_PANEL_H
#define DRC_RE_RTG_DIFF_PAIR_OVERLAY_PANEL_H

#include <memory>

#include <units_provider.h>

#include "drc_re_bitmap_overlay_panel.h"

enum class EDA_UNITS;

class DRC_RE_ROUTING_DIFF_PAIR_CONSTRAINT_DATA;
class UNIT_BINDER;

/**
 * Overlay panel for differential pair routing constraints showing gap, width,
 * and uncoupled length fields positioned over a diagram.
 */
class DRC_RE_ROUTING_DIFF_PAIR_OVERLAY_PANEL : public DRC_RE_BITMAP_OVERLAY_PANEL
{
public:
    DRC_RE_ROUTING_DIFF_PAIR_OVERLAY_PANEL( wxWindow* aParent,
                                             DRC_RE_ROUTING_DIFF_PAIR_CONSTRAINT_DATA* aData,
                                             EDA_UNITS aUnits );

    ~DRC_RE_ROUTING_DIFF_PAIR_OVERLAY_PANEL() override = default;

    bool TransferDataToWindow() override;
    bool TransferDataFromWindow() override;

    bool ValidateInputs( int* aErrorCount, wxString* aValidationMessage ) override;
    wxString GenerateRule( const RULE_GENERATION_CONTEXT& aContext ) override;

private:
    DRC_RE_ROUTING_DIFF_PAIR_CONSTRAINT_DATA* m_data;

    UNITS_PROVIDER m_unitsProvider;

    std::unique_ptr<UNIT_BINDER> m_minGapBinder;
    std::unique_ptr<UNIT_BINDER> m_preferredGapBinder;
    std::unique_ptr<UNIT_BINDER> m_maxGapBinder;
    std::unique_ptr<UNIT_BINDER> m_minWidthBinder;
    std::unique_ptr<UNIT_BINDER> m_preferredWidthBinder;
    std::unique_ptr<UNIT_BINDER> m_maxWidthBinder;
    std::unique_ptr<UNIT_BINDER> m_maxUncoupledLengthBinder;
    std::unique_ptr<UNIT_BINDER> m_maxSkewBinder;
};

#endif // DRC_RE_RTG_DIFF_PAIR_OVERLAY_PANEL_H
