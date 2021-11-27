/*
* This program source code file is part of KiCad, a free EDA CAD application.
*
* Copyright (C) 2020-2021 KiCad Developers, see AUTHORS.txt for contributors.
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

#ifndef SYMBOL_EDITOR_SETTINGS_H
#define SYMBOL_EDITOR_SETTINGS_H

#include <settings/app_settings.h>


class SYMBOL_EDITOR_SETTINGS : public APP_SETTINGS_BASE
{
public:

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

    SYMBOL_EDITOR_SETTINGS();

    virtual ~SYMBOL_EDITOR_SETTINGS() {}

    virtual bool MigrateFromLegacy( wxConfigBase* aLegacyConfig ) override;

    DEFAULTS m_Defaults;

    REPEAT m_Repeat;

    bool m_ShowPinElectricalType;

    int m_LibWidth;

    wxString m_EditSymbolVisibleColumns;

    wxString m_PinTableVisibleColumns;

    bool m_UseEeschemaColorSettings;

protected:

    virtual std::string getLegacyFrameName() const override { return "LibeditFrame"; }
};

#endif
