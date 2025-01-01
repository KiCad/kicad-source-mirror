/*
 * This program source code file is part of KICAD, a free EDA CAD application.
 *
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

#ifndef _KIPYTHON_SETTINGS_H
#define _KIPYTHON_SETTINGS_H

#include <settings/app_settings.h>

class KIPYTHON_SETTINGS : public APP_SETTINGS_BASE
{
public:
    KIPYTHON_SETTINGS();

    virtual ~KIPYTHON_SETTINGS() {}

protected:

    virtual std::string getLegacyFrameName() const override { return "Python_"; }
};


#endif
