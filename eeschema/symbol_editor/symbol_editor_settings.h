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
* along with this program; if not, you may find one here:
* http://www.gnu.org/licenses/old-licenses/gpl-2.0.html
* or you may search the http://www.gnu.org website for the version 2 license,
* or you may write to the Free Software Foundation, Inc.,
* 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
*/

#pragma once

#include <settings/app_settings.h>
#include <project/sch_project_settings.h>


class SYMBOL_EDITOR_SETTINGS : public APP_SETTINGS_BASE
{
public:

    struct AUI_PANELS
    {
        int  properties_panel_width;
        float properties_splitter;
        bool show_properties;
    };

    struct DEFAULTS
    {
        int line_width;
        int text_size;
        int pin_length;
        int pin_name_size;
        int pin_num_size;
    };

    struct REPEAT
    {
        int label_delta;
        int pin_step;
    };

    struct DIALOG_IMPORT_GRAPHICS
    {
        bool     interactive_placement;
        wxString last_file;
        double   dxf_line_width;
        int      dxf_line_width_units;
        int      origin_units;
        double   origin_x;
        double   origin_y;
        int      dxf_units;
    };

    struct LIB_FIELD_EDITOR
    {
        std::map<std::string, int> field_widths;
        int                        width;
        int                        height;
    };

    SYMBOL_EDITOR_SETTINGS();

    virtual ~SYMBOL_EDITOR_SETTINGS() {}

    virtual bool MigrateFromLegacy( wxConfigBase* aLegacyConfig ) override;

    AUI_PANELS m_AuiPanels;

    DEFAULTS m_Defaults;

    REPEAT m_Repeat;

    DIALOG_IMPORT_GRAPHICS m_ImportGraphics;

    bool m_ShowPinElectricalType;
    bool m_ShowHiddenPins;
    bool m_ShowHiddenFields;
    bool m_ShowPinAltIcons;

    ///< When true, dragging an outline edge will drag pins rooted on it
    bool m_dragPinsAlongWithEdges;

    int m_LibWidth;

    int m_LibrarySortMode;

    wxString m_EditSymbolVisibleColumns;

    wxString m_PinTableVisibleColumns;

    bool m_UseEeschemaColorSettings;

    SCH_SELECTION_FILTER_OPTIONS m_SelectionFilter;

    LIB_FIELD_EDITOR m_LibFieldEditor;

protected:

    virtual std::string getLegacyFrameName() const override { return "LibeditFrame"; }
};
