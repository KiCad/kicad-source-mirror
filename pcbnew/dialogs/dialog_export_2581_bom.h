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

#ifndef IPC2581_EXPORT_BOM_DIALOG_H
#define IPC2581_EXPORT_BOM_DIALOG_H

#include <vector>

#include "dialog_export_2581_bom_base.h"

class BOARD;

/// Fields that supply the IPC-2581 BOM section
struct IPC2581_BOM_FIELDS
{
    wxString m_revision;
    wxString m_internalId;
    wxString m_mfgPn;
    wxString m_mfg;
    wxString m_distPn;
    wxString m_dist;
};

class DIALOG_EXPORT_2581_BOM : public DIALOG_EXPORT_2581_BOM_BASE
{
public:
    DIALOG_EXPORT_2581_BOM( wxWindow* aParent, BOARD* aBoard, const IPC2581_BOM_FIELDS& aFields );

    const IPC2581_BOM_FIELDS& GetFields() const { return m_fields; }

private:
    void onMfgPNChange( wxCommandEvent& event ) override;
    void onDistPNChange( wxCommandEvent& event ) override;

    bool TransferDataToWindow() override;
    bool TransferDataFromWindow() override;

    wxString choiceValue( const wxChoice* aChoice ) const;

    IPC2581_BOM_FIELDS m_fields;
};

#endif // IPC2581_EXPORT_BOM_DIALOG_H
