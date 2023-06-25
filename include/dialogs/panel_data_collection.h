/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, you may find one here:
 * http://www.gnu.org/licenses/old-licenses/gpl-2.0.html
 * or you may search the http://www.gnu.org website for the version 2 license,
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

#ifndef PANEL_DATA_COLLECTION_H
#define PANEL_DATA_COLLECTION_H

#include <dialogs/panel_data_collection_base.h>


class COMMON_SETTINGS;
class PAGED_DIALOG;

struct PANEL_DATA_COLLECTION_MODEL
{
    bool m_enableSentry;
};


class PANEL_DATA_COLLECTION : public PANEL_DATA_COLLECTION_BASE
{
public:
    PANEL_DATA_COLLECTION( std::shared_ptr<PANEL_DATA_COLLECTION_MODEL> model, wxWindow* aParent, bool aWizard = true );
    PANEL_DATA_COLLECTION( wxWindow* aParent );

    void ResetPanel() override;

protected:
    bool TransferDataFromWindow() override;
    bool TransferDataToWindow() override;

    virtual void OnResetIdClick( wxCommandEvent& aEvent ) override;

private:
    std::shared_ptr<PANEL_DATA_COLLECTION_MODEL> m_model;

    bool m_wizard;

    void applySettingsToPanel();
};

#endif //PANEL_DATA_COLLECTION_H
