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

#ifndef PANEL_SCH_DATA_SOURCES_H
#define PANEL_SCH_DATA_SOURCES_H

#include <memory>

#include <pcm.h>
#include <widgets/resettable_panel.h>

class wxButton;
class wxListBox;
class wxStaticText;
class EDA_BASE_FRAME;


class PANEL_SCH_DATA_SOURCES : public RESETTABLE_PANEL
{
public:
    PANEL_SCH_DATA_SOURCES( wxWindow* aParent, EDA_BASE_FRAME* aFrame );

    bool TransferDataToWindow() override;
    bool TransferDataFromWindow() override;

    void ResetPanel() override;

private:
    void populateInstalledSources();
    void OnManageDataSources( wxCommandEvent& aEvent );

private:
    EDA_BASE_FRAME*                              m_frame;
    std::shared_ptr<PLUGIN_CONTENT_MANAGER>      m_pcm;
    wxStaticText*                                m_description;
    wxStaticText*                                m_status;
    wxListBox*                                   m_sourcesList;
    wxButton*                                    m_manageButton;
};


#endif
