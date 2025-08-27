/*
 * This program source code file is part of KiCad, a free EDA CAD application.
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

#ifndef STARTWIZARD_H
#define STARTWIZARD_H

#include <kicommon.h>
#include <startwizard/startwizard_provider.h>

class wxWizard;
class wxWindow;


class KICOMMON_API STARTWIZARD
{
public:
    STARTWIZARD();

    ~STARTWIZARD();

    STARTWIZARD( const STARTWIZARD& ) = delete;
    STARTWIZARD& operator=( const STARTWIZARD& ) = delete;

    void CheckAndRun( wxWindow* parent );

    STARTWIZARD_PROVIDER* GetProvider( const wxString& aName );

private:
    wxWizard* m_wizard;
    std::vector<std::unique_ptr<STARTWIZARD_PROVIDER>> m_providers;
};

#endif
