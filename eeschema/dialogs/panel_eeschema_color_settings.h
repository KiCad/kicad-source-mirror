
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

#ifndef PANEL_EESCHEMA_COLOR_SETTINGS_H_
#define PANEL_EESCHEMA_COLOR_SETTINGS_H_

#include <gal/color4d.h>
#include <gal/gal_display_options.h>
#include <layers_id_colors_and_visibility.h>
#include <dialogs/panel_color_settings.h>

class COLOR_SETTINGS;
class SCH_BASE_FRAME;
class PAGE_INFO;
class SCH_ITEM;
class SCH_PREVIEW_PANEL;
class TITLE_BLOCK;
class DS_PROXY_VIEW_ITEM;


class PANEL_EESCHEMA_COLOR_SETTINGS : public PANEL_COLOR_SETTINGS
{
public:
    PANEL_EESCHEMA_COLOR_SETTINGS( SCH_BASE_FRAME* aFrame, wxWindow* aParent );

    ~PANEL_EESCHEMA_COLOR_SETTINGS() override;

    void ResetPanel() override;

protected:
    bool TransferDataFromWindow() override;

    bool TransferDataToWindow() override;

    void OnOverrideItemColorsClicked( wxCommandEvent& aEvent ) override;
    void OnSize( wxSizeEvent& aEvent ) override;
    void onNewThemeSelected() override;
    void onColorChanged() override;

    bool validateSave( bool aQuiet = false ) override;

    bool saveCurrentTheme( bool aValidate ) override;

private:
    SCH_BASE_FRAME*            m_frame;

    SCH_PREVIEW_PANEL*         m_preview;
    PAGE_INFO*                 m_page;
    TITLE_BLOCK*               m_titleBlock;
    DS_PROXY_VIEW_ITEM*        m_drawingSheet;
    std::vector<EDA_ITEM*>     m_previewItems;

    KIGFX::GAL_DISPLAY_OPTIONS m_galDisplayOptions;

private:
    void createPreviewItems();
    void createSwatches();

    void updatePreview();
    void zoomFitPreview();
};


#endif
