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
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#pragma once

#include <dialogs/dialog_export_older_version_base.h>

class DIALOG_EXPORT_OLDER_VERSION : public DIALOG_EXPORT_OLDER_VERSION_BASE
{
public:
    DIALOG_EXPORT_OLDER_VERSION( wxWindow* aParent, const wxArrayString& aTargets, const wxArrayString& aVariants,
                                 const wxString& aDefaultPath );

    wxString GetTarget() const;

    /// The variant to flatten, or empty for the base design.
    wxString GetVariant() const;

    wxString GetPath() const;

    bool GetDropInsteadOfApproximate() const;

private:
    void updateVariantChoice();

    bool TransferDataFromWindow() override;
};
