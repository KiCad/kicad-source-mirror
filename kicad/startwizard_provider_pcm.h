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

#include <startwizard_provider.h>
#include <memory>

struct PANEL_PCM_STARTWIZARD_MODEL
{
    bool m_autoUpdateCheck;
};

class STARTWIZARD_PROVIDER_PCM : public STARTWIZARD_PROVIDER
{
public:
    STARTWIZARD_PROVIDER_PCM();

    virtual ~STARTWIZARD_PROVIDER_PCM() {}

    virtual bool NeedsUserInput() const override;

    virtual wxPanel* GetWizardPanel( wxWindow* aParent ) override;

    virtual void Finish() override;

private:
    std::shared_ptr<PANEL_PCM_STARTWIZARD_MODEL> m_model;
};

#endif
