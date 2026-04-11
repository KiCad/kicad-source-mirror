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

#ifndef DRC_RE_PERMITTED_LAYERS_OVERLAY_PANEL_H
#define DRC_RE_PERMITTED_LAYERS_OVERLAY_PANEL_H

#include "drc_re_bitmap_overlay_panel.h"

class DRC_RE_PERMITTED_LAYERS_CONSTRAINT_DATA;
class wxCheckBox;


/**
 * Overlay panel for permitted layers constraints showing checkboxes
 * for top and bottom layer selection positioned over a diagram.
 */
class DRC_RE_PERMITTED_LAYERS_OVERLAY_PANEL : public DRC_RE_BITMAP_OVERLAY_PANEL
{
public:
    DRC_RE_PERMITTED_LAYERS_OVERLAY_PANEL( wxWindow* aParent,
                                           DRC_RE_PERMITTED_LAYERS_CONSTRAINT_DATA* aData );

    ~DRC_RE_PERMITTED_LAYERS_OVERLAY_PANEL() override = default;

    bool TransferDataToWindow() override;
    bool TransferDataFromWindow() override;

    bool ValidateInputs( int* aErrorCount, wxString* aValidationMessage ) override;
    wxString GenerateRule( const RULE_GENERATION_CONTEXT& aContext ) override;

private:
    DRC_RE_PERMITTED_LAYERS_CONSTRAINT_DATA* m_data;

    wxCheckBox* m_topLayerCheckbox;
    wxCheckBox* m_bottomLayerCheckbox;
};

#endif // DRC_RE_PERMITTED_LAYERS_OVERLAY_PANEL_H
