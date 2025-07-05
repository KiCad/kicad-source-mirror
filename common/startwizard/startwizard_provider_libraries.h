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
 

#ifndef STARTWIZARD_PROVIDER_LIBRARIES_H
#define STARTWIZARD_PROVIDER_LIBRARIES_H

#include <libraries/library_table.h>
#include <startwizard/startwizard_provider.h>

enum class STARTWIZARD_LIBRARIES_MODE
{
    USE_DEFAULTS,
    IMPORT,
    CREATE_BLANK
};

struct STARTWIZARD_PROVIDER_LIBRARIES_MODEL
{
    STARTWIZARD_LIBRARIES_MODE mode = STARTWIZARD_LIBRARIES_MODE::USE_DEFAULTS;

    std::vector<LIBRARY_TABLE_TYPE> missing_tables;
};

class STARTWIZARD_PROVIDER_LIBRARIES : public STARTWIZARD_PROVIDER
{
public:
    STARTWIZARD_PROVIDER_LIBRARIES();

    virtual ~STARTWIZARD_PROVIDER_LIBRARIES() {}

    wxString Name() const override { return wxT( "libraries" ); }

    bool NeedsUserInput() const override;

    wxPanel* GetWizardPanel( wxWindow* aParent, STARTWIZARD* aWizard ) override;

    void Finish() override;

    void ApplyDefaults() override;

private:
    std::shared_ptr<STARTWIZARD_PROVIDER_LIBRARIES_MODEL> m_model;
};


#endif //STARTWIZARD_PROVIDER_LIBRARIES_H
