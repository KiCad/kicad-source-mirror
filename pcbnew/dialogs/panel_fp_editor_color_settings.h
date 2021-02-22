/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2020 Jon Evans <jon@craftyjon.com>
 * Copyright (C) 2020 KiCad Developers, see AUTHORS.txt for contributors.
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

#ifndef PANEL_FP_EDITOR_COLOR_SETTINGS_H
#define PANEL_FP_EDITOR_COLOR_SETTINGS_H

#include <gal/color4d.h>
#include <layers_id_colors_and_visibility.h>
#include <dialogs/panel_color_settings.h>

class COLOR_SETTINGS;
class PAGE_INFO;
class FOOTPRINT_EDIT_FRAME;
class TITLE_BLOCK;
class DS_PROXY_VIEW_ITEM;


class PANEL_FP_EDITOR_COLOR_SETTINGS : public PANEL_COLOR_SETTINGS
{
public:
    PANEL_FP_EDITOR_COLOR_SETTINGS( FOOTPRINT_EDIT_FRAME* aFrame, wxWindow* aParent );

    ~PANEL_FP_EDITOR_COLOR_SETTINGS() override;

protected:
    bool TransferDataFromWindow() override;

    bool TransferDataToWindow() override;

    enum COLOR_CONTEXT_ID
    {
        ID_COPY = wxID_HIGHEST + 1,
        ID_PASTE,
        ID_REVERT
    };

private:
    FOOTPRINT_EDIT_FRAME*  m_frame;
    PAGE_INFO*             m_page;
    TITLE_BLOCK*           m_titleBlock;

    void createSwatches();
};


#endif // PANEL_FP_EDITOR_COLOR_SETTINGS_H
