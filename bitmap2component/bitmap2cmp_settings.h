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

#ifndef _BITMAP2CMP_SETTINGS_H
#define _BITMAP2CMP_SETTINGS_H

#include <settings/app_settings.h>

class BITMAP2CMP_SETTINGS : public APP_SETTINGS_BASE
{
public:
    BITMAP2CMP_SETTINGS();

    virtual ~BITMAP2CMP_SETTINGS() {}

    virtual bool MigrateFromLegacy( wxConfigBase* aLegacyConfig ) override;

public:
    wxString m_BitmapFileName;
    wxString m_ConvertedFileName;
    int      m_Units;
    int      m_Threshold;
    bool     m_Negative;
    int      m_LastFormat;
    int      m_LastLayer;

protected:
    virtual std::string getLegacyFrameName() const override { return "Bmconverter_"; }
};


#endif
