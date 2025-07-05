/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
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
 

#ifndef STARTWIZARD_PROVIDER_SETTINGS_H
#define STARTWIZARD_PROVIDER_SETTINGS_H

#include <memory>

#include <startwizard/startwizard_provider.h>


enum class STARTWIZARD_SETTINGS_MODE
{
    USE_DEFAULTS,
    IMPORT
};

struct STARTWIZARD_PROVIDER_SETTINGS_MODEL
{
    STARTWIZARD_SETTINGS_MODE mode = STARTWIZARD_SETTINGS_MODE::USE_DEFAULTS;
    wxString import_path;
};

class STARTWIZARD_PROVIDER_SETTINGS : public STARTWIZARD_PROVIDER
{
public:
    STARTWIZARD_PROVIDER_SETTINGS();

    virtual ~STARTWIZARD_PROVIDER_SETTINGS() {}

    wxString Name() const override { return wxT( "settings" ); }

    bool NeedsUserInput() const override;

    wxPanel* GetWizardPanel( wxWindow* aParent, STARTWIZARD* aWizard ) override;

    void Finish() override;

    void ApplyDefaults() override;

    const STARTWIZARD_PROVIDER_SETTINGS_MODEL& GetModel() const { return *m_model; }

private:
    std::shared_ptr<STARTWIZARD_PROVIDER_SETTINGS_MODEL> m_model;
};



#endif //STARTWIZARD_PROVIDER_SETTINGS_H
