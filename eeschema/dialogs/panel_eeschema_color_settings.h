
/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2020 Jon Evans <jon@craftyjon.com>
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
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

#include <dialogs/panel_color_settings.h>
#include <class_draw_panel_gal.h>
#include <gal_display_options_common.h>

class PAGE_INFO;
class EDA_ITEM;
class SCH_PREVIEW_PANEL;
class TITLE_BLOCK;
class DS_PROXY_VIEW_ITEM;


class PANEL_EESCHEMA_COLOR_SETTINGS : public PANEL_COLOR_SETTINGS
{
public:
    PANEL_EESCHEMA_COLOR_SETTINGS( wxWindow* aParent );

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

    void createSwatches() override;

private:
    void createPreviewItems();

    void updatePreview();
    void updateAllowedSwatches();
    void zoomFitPreview();

private:
    SCH_PREVIEW_PANEL*           m_preview;
    PAGE_INFO*                   m_page;
    TITLE_BLOCK*                 m_titleBlock;
    DS_PROXY_VIEW_ITEM*          m_drawingSheet;
    std::vector<EDA_ITEM*>       m_previewItems;

    GAL_DISPLAY_OPTIONS_IMPL     m_galDisplayOptions;
    EDA_DRAW_PANEL_GAL::GAL_TYPE m_galType;
};


#endif
