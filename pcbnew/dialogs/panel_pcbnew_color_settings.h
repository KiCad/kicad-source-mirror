/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2020 Jon Evans <jon@craftyjon.com>
 * Copyright (C) 2020-2021 KiCad Developers, see AUTHORS.txt for contributors.
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

#ifndef PANEL_PCBNEW_COLOR_SETTINGS_H_
#define PANEL_PCBNEW_COLOR_SETTINGS_H_

#include <gal/color4d.h>
#include <layer_ids.h>
#include <dialogs/panel_color_settings.h>

class COLOR_SETTINGS;
class PAGE_INFO;
class PCB_EDIT_FRAME;
class FOOTPRINT_PREVIEW_PANEL;
class TITLE_BLOCK;
class DS_PROXY_VIEW_ITEM;


class PANEL_PCBNEW_COLOR_SETTINGS : public PANEL_COLOR_SETTINGS
{
public:
    PANEL_PCBNEW_COLOR_SETTINGS( PCB_EDIT_FRAME* aFrame, wxWindow* aParent );

    ~PANEL_PCBNEW_COLOR_SETTINGS() override;

    void ResetPanel() override;

protected:
    bool TransferDataFromWindow() override;

    bool TransferDataToWindow() override;

    void OnSize( wxSizeEvent& aEvent ) override;
    void onNewThemeSelected() override;
    void onColorChanged() override;

    enum COLOR_CONTEXT_ID
    {
        ID_COPY = wxID_HIGHEST + 1,
        ID_PASTE,
        ID_REVERT
    };

private:
    void createPreviewItems();
    void createSwatches();

    void updatePreview();
    void zoomFitPreview();

    PCB_EDIT_FRAME*          m_frame;

    FOOTPRINT_PREVIEW_PANEL* m_preview;
    PAGE_INFO*               m_page;
    TITLE_BLOCK*             m_titleBlock;
};


#endif
