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

#pragma once

#include <memory>
#include <widgets/paged_dialog.h>

class SCH_EDIT_FRAME;
class PANEL_SETUP_SEVERITIES;
class PANEL_TEMPLATE_FIELDNAMES;
class PANEL_SETUP_FORMATTING;
class PANEL_SETUP_PINMAP;
class PANEL_TEXT_VARIABLES;
class PANEL_SETUP_NETCLASSES;
class PANEL_SETUP_BUSES;
class ERC_ITEM;


class DIALOG_SCHEMATIC_SETUP : public PAGED_DIALOG
{
public:
    DIALOG_SCHEMATIC_SETUP( SCH_EDIT_FRAME* aFrame );
    ~DIALOG_SCHEMATIC_SETUP() = default;

protected:
    // event handlers
    void onPageChanged( wxBookCtrlEvent& aEvent ) override;
    void onAuxiliaryAction( wxCommandEvent& aEvent ) override;

protected:
    SCH_EDIT_FRAME*           m_frame;

    std::shared_ptr<ERC_ITEM> m_pinToPinError;

    size_t                    m_formattingPage;
    size_t                    m_annotationPage;
    size_t                    m_fieldNameTemplatesPage;
    size_t                    m_bomPresetsPage;
    size_t                    m_pinMapPage;
    size_t                    m_busesPage;
    size_t                    m_textVarsPage;
    size_t                    m_severitiesPage;
    size_t                    m_netclassesPage;
    size_t                    m_embeddedFilesPage;
};
