/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2024 KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef DRC_RE_LAYER_SELECTION_COMBO_H
#define DRC_RE_LAYER_SELECTION_COMBO_H

#include <wx/wx.h>
#include <wx/combo.h>
#include <wx/checklst.h>

#include "drc_re_layers_selection_choice_popup.h"


class DRC_RE_LAYER_SELECTION_COMBO : public wxComboCtrl
{
public:
    DRC_RE_LAYER_SELECTION_COMBO( wxWindow* aParent, const std::vector<PCB_LAYER_ID>& aLayerIDs,
                                  const std::function<wxString( PCB_LAYER_ID )>& aNameGetter );

    ~DRC_RE_LAYER_SELECTION_COMBO();

    wxString GetSelectedItemsString();

    std::vector<PCB_LAYER_ID> GetSelectedLayers();

    void SetItemsSelected( const std::vector<PCB_LAYER_ID>& aSelectedLayers );

private:
    void onPopupClose( wxCommandEvent& aEvent );

    void onKeyDown( wxKeyEvent& aEvent );

    void onMouseClick( wxMouseEvent& aEvent );

private:
    DRC_RE_LAYER_SELECTION_CHOICE_POPUP*    m_popup;
    std::vector<PCB_LAYER_ID>               m_layerIDs;   // Stores the layer IDs
    std::function<wxString( PCB_LAYER_ID )> m_nameGetter; // Function to get layer names
};

#endif // DRC_RE_LAYER_SELECTION_COMBO_H