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

#include <functional>

#include <dialog_shim.h>
#include <wx/arrstr.h>

class wxCheckBox;
class wxChoice;
class wxSlider;


/**
 * Ask for the settings used to trace a placed image into outlines.
 *
 * The caller draws its own preview through the callback.
 */
class DIALOG_CONVERT_IMAGE_TO_OUTLINES : public DIALOG_SHIM
{
public:
    /**
     * @param aParent is the window the dialog belongs to.
     * @param aLayerNames are the layers to offer, empty when the caller has no layers.
     * @param aSelectedLayer is the layer to start on, an index into aLayerNames.
     * @param aOnChange receives the threshold and invert setting whenever they change.
     */
    DIALOG_CONVERT_IMAGE_TO_OUTLINES( wxWindow* aParent, const wxArrayString& aLayerNames, int aSelectedLayer,
                                      std::function<void( double, bool )> aOnChange );

    /// Brightness cut deciding which pixels are traced, from 0 to 1.
    double GetThreshold() const;

    bool GetInvert() const;

    /// Index into the layer names given to the constructor, or -1 when there were none.
    int GetLayerSelection() const;

private:
    void onSettingChanged( wxCommandEvent& aEvent );
    void onThresholdScrolled( wxScrollEvent& aEvent );

    wxSlider*             m_threshold;
    wxCheckBox*           m_invert;
    wxChoice*             m_layer;
    std::function<void( double, bool )> m_onChange;
};
