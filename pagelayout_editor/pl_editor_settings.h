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

#ifndef _PL_EDITOR_SETTINGS_H
#define _PL_EDITOR_SETTINGS_H

#include <settings/app_settings.h>

class PL_EDITOR_SETTINGS : public APP_SETTINGS_BASE
{
public:
    PL_EDITOR_SETTINGS();

    virtual ~PL_EDITOR_SETTINGS() {}

    virtual bool MigrateFromLegacy( wxConfigBase* aLegacyConfig ) override;

protected:
    virtual std::string getLegacyFrameName() const override { return "PlEditorFrame"; }

public:
    int      m_CornerOrigin;
    int      m_PropertiesFrameWidth;
    wxString m_LastPaperSize;
    int      m_LastCustomWidth;
    int      m_LastCustomHeight;
    bool     m_LastWasPortrait;
    bool     m_BlackBackground;
};


#endif
