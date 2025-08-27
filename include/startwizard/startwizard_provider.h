/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2023 Mark Roszko <mark.roszko@gmail.com>
 * Copyright (C) 1992-2023 KiCad Developers, see AUTHORS.txt for contributors.
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

#ifndef STARTWIZARD_PROVIDER_H
#define STARTWIZARD_PROVIDER_H

#include <wx/string.h>

class wxWizardPageSimple;
class wxPanel;
class wxWindow;
class STARTWIZARD;

class KICOMMON_API STARTWIZARD_PROVIDER
{
public:
    STARTWIZARD_PROVIDER( const wxString& aPageName ) : m_pageName( aPageName ) {}

    virtual ~STARTWIZARD_PROVIDER() = default;

    virtual wxString Name() const = 0;

    virtual bool NeedsUserInput() const { return false; }

    virtual wxPanel* GetWizardPanel( wxWindow* aParent, STARTWIZARD* aWizard ) { return nullptr; }

    const wxString& GetPageName() const { return m_pageName; }

    virtual void Finish() {}

    /// Apply whatever actions and settings should happen if the user cancels the startup wizard
    virtual void ApplyDefaults() {}

private:
    wxString m_pageName;
};

#endif
