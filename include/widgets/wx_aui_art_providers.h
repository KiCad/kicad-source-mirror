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

/**
 * wxWidgets 3.1 has support for dark mode detection, but 3.0 doesn't.
 * The main purpose of this replacement art provider is to backport that functionality
 * so that it is available in Linux systems that will likely be stuck on 3.0 for a while.
 */
class WX_AUI_TOOLBAR_ART : public wxAuiDefaultToolBarArt
{
public:
    WX_AUI_TOOLBAR_ART() : wxAuiDefaultToolBarArt() {}

    virtual ~WX_AUI_TOOLBAR_ART() = default;

    void DrawButton( wxDC& aDc, wxWindow* aWindow, const wxAuiToolBarItem& aItem,
                     const wxRect& aRect ) override;
};


class WX_AUI_DOCK_ART : public wxAuiDefaultDockArt
{
public:
    WX_AUI_DOCK_ART();
};


#endif // KICAD_WX_AUI_ART_PROVIDERS_H
