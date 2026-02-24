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

#ifndef DRC_RE_VIA_STYLE_OVERLAY_PANEL_H
#define DRC_RE_VIA_STYLE_OVERLAY_PANEL_H

#include <memory>
#include <wx/choice.h>
#include <units_provider.h>

#include "drc_re_bitmap_overlay_panel.h"

enum class EDA_UNITS;

class DRC_RE_VIA_STYLE_CONSTRAINT_DATA;
class UNIT_BINDER;
class wxStaticText;


/**
 * Overlay panel for via style constraints showing via diameter and hole size fields
 * positioned over a diagram.
 */
class DRC_RE_VIA_STYLE_OVERLAY_PANEL : public DRC_RE_BITMAP_OVERLAY_PANEL
{
public:
    DRC_RE_VIA_STYLE_OVERLAY_PANEL( wxWindow* aParent,
                                    DRC_RE_VIA_STYLE_CONSTRAINT_DATA* aData,
                                    EDA_UNITS aUnits );

    ~DRC_RE_VIA_STYLE_OVERLAY_PANEL() override = default;

    bool TransferDataToWindow() override;
    bool TransferDataFromWindow() override;

    bool ValidateInputs( int* aErrorCount, wxString* aValidationMessage ) override;
    wxString GenerateRule( const RULE_GENERATION_CONTEXT& aContext ) override;

private:
    DRC_RE_VIA_STYLE_CONSTRAINT_DATA* m_data;

    UNITS_PROVIDER m_unitsProvider;

    wxChoice*     m_viaTypeChoice;
    wxStaticText* m_viaTypeLabel;

    // Unit binders for via diameter fields
    std::unique_ptr<UNIT_BINDER> m_minViaDiameterBinder;
    std::unique_ptr<UNIT_BINDER> m_maxViaDiameterBinder;

    // Unit binders for via hole size fields
    std::unique_ptr<UNIT_BINDER> m_minViaHoleSizeBinder;
    std::unique_ptr<UNIT_BINDER> m_maxViaHoleSizeBinder;
};

#endif // DRC_RE_VIA_STYLE_OVERLAY_PANEL_H
