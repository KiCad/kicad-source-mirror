/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2017 CERN
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 * @author Maciej Suminski <maciej.suminski@cern.ch>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 3
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


#ifndef LIB_LOGGER_H
#define LIB_LOGGER_H

#include <wx/log.h>

class LIB_LOGGER : public wxLogGui
{
public:
    LIB_LOGGER();

    ~LIB_LOGGER() override;

    void Activate();

    void Deactivate();

    void Flush() override;

private:
    wxLog* m_previousLogger;
    bool   m_activated;
};

#endif /* LIB_LOGGER_H */