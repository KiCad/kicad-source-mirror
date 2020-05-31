/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2020 CERN
 * @author Jon Evans <jon@craftyjon.com>
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

#ifndef KICAD_NET_SETTINGS_H
#define KICAD_NET_SETTINGS_H

#include <netclass.h>
#include <settings/nested_settings.h>

/**
 * NET_SETTINGS stores various net-related settings in a project context.  These settings are
 * accessible and editable from both the schematic and PCB editors.
 */
class NET_SETTINGS : public NESTED_SETTINGS
{
public:
    NET_SETTINGS( JSON_SETTINGS* aParent, const std::string& aPath );

    virtual ~NET_SETTINGS();

    NETCLASSES m_NetClasses;

private:
    NETCLASSPTR m_defaultClass;

    // TODO: Add diff pairs, bus information, etc here.
};

#endif // KICAD_NET_SETTINGS_H
