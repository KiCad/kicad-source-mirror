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

#ifndef STARTWIZARD_PROVIDER_PCM_H
#define STARTWIZARD_PROVIDER_PCM_H

#include <startwizard/startwizard_provider.h>
#include <memory>

struct STARTWIZARD_PROVIDER_PRIVACY_MODEL
{
    bool m_autoUpdateKiCad = true;
    bool m_autoUpdatePCM   = true;
    bool m_enableSentry    = false;
};

class STARTWIZARD_PROVIDER_PRIVACY : public STARTWIZARD_PROVIDER
{
public:
    STARTWIZARD_PROVIDER_PRIVACY();

    virtual ~STARTWIZARD_PROVIDER_PRIVACY() {}

    wxString Name() const override { return wxT( "privacy" ); }

    bool NeedsUserInput() const override;

    wxPanel* GetWizardPanel( wxWindow* aParent, STARTWIZARD* aWizard ) override;

    void Finish() override;

    void ApplyDefaults() override;

private:
    std::shared_ptr<STARTWIZARD_PROVIDER_PRIVACY_MODEL> m_model;
};

#endif
