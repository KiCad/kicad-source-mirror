/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2011-2014 Jean-Pierre Charras, jp.charras at wanadoo.fr
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

#ifndef LAYER_BOX_SELECTOR_H
#define LAYER_BOX_SELECTOR_H

#include <wx/bmpcbox.h>


/**
 * Base class to build a layer list.
 */
class LAYER_SELECTOR
{
public:
    LAYER_SELECTOR();

    virtual ~LAYER_SELECTOR() { }

    bool SetLayersHotkeys( bool value );

protected:
    /// Return true if the layer id is enabled (i.e. is it should be displayed).
    virtual bool isLayerEnabled( int aLayer ) const = 0;

    bool m_layerhotkeys;
};


/**
 * Display a layer list in a wxBitmapComboBox.
 */
class LAYER_BOX_SELECTOR : public wxBitmapComboBox, public LAYER_SELECTOR
{
public:
    LAYER_BOX_SELECTOR( wxWindow* parent, wxWindowID id, const wxPoint& pos = wxDefaultPosition,
                        const wxSize& size = wxDefaultSize, int n = 0,
                        const wxString choices[] = nullptr );

    ~LAYER_BOX_SELECTOR() override;

    int GetLayerSelection() const;

    int SetLayerSelection( int layer );

    // Reload the Layers
    // Virtual pure function because GerbView uses its own functions in a derived class
    virtual void Resync() = 0;

private:
#ifdef __WXMAC__
    void onKeyDown( wxKeyEvent& aEvent );
    void OnDrawBackground( wxDC& dc, const wxRect& rect, int item, int flags) const override;
#endif
};

#endif // LAYER_BOX_SELECTOR_H
