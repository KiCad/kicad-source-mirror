/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2021 KiCad Developers, see AUTHORS.txt for contributors.
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

#ifndef KICAD_WX_AUI_ART_PROVIDERS_H
#define KICAD_WX_AUI_ART_PROVIDERS_H

#include <wx/aui/auibar.h>
#include <wx/aui/dockart.h>


class WX_AUI_TOOLBAR_ART : public wxAuiDefaultToolBarArt
{
public:
    WX_AUI_TOOLBAR_ART() : wxAuiDefaultToolBarArt() {}

    virtual ~WX_AUI_TOOLBAR_ART() = default;

    wxSize GetToolSize( wxDC& aDc, wxWindow* aWindow, const wxAuiToolBarItem& aItem ) override;

    /**
     * Unfortunately we need to re-implement this to actually be able to control the size
     */
    void DrawButton( wxDC& aDc, wxWindow* aWindow, const wxAuiToolBarItem& aItem,
                     const wxRect& aRect ) override;
};


class WX_AUI_DOCK_ART : public wxAuiDefaultDockArt
{
public:
    WX_AUI_DOCK_ART();
};


#endif // KICAD_WX_AUI_ART_PROVIDERS_H
