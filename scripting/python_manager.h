/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2023 Jon Evans <jon@craftyjon.com>
 * Copyright (C) 2023 KiCad Developers, see AUTHORS.txt for contributors.
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

#ifndef KICAD_PYTHON_MANAGER_H
#define KICAD_PYTHON_MANAGER_H

#include <functional>
#include <optional>

#include <wx/wx.h>


class PYTHON_MANAGER
{
public:
    PYTHON_MANAGER( const wxString& aInterpreterPath ) :
            m_interpreterPath( aInterpreterPath )
    {}

    void Execute( const wxString& aArgs,
                  const std::function<void(int, const wxString&)>& aCallback );

    wxString GetInterpreterPath() const { return m_interpreterPath; }
    void SetInterpreterPath( const wxString& aPath ) { m_interpreterPath = aPath; }

private:
    wxString m_interpreterPath;
};


#endif //KICAD_PYTHON_MANAGER_H
