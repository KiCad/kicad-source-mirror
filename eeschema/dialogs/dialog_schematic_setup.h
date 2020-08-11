/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2020 KiCad Developers, see AUTHORS.txt for contributors.
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


#ifndef KICAD_DIALOG_SCHEMATIC_SETUP_H
#define KICAD_DIALOG_SCHEMATIC_SETUP_H

#include <widgets/paged_dialog.h>

class SCH_EDIT_FRAME;
class PANEL_SETUP_SEVERITIES;
class PANEL_EESCHEMA_TEMPLATE_FIELDNAMES;
class PANEL_SETUP_FORMATTING;
class PANEL_SETUP_PINMAP;
class PANEL_TEXT_VARIABLES;
class PANEL_SETUP_NETCLASSES;
class ERC_ITEM;


class DIALOG_SCHEMATIC_SETUP : public PAGED_DIALOG
{
public:
    DIALOG_SCHEMATIC_SETUP( SCH_EDIT_FRAME* aFrame );
    ~DIALOG_SCHEMATIC_SETUP();

protected:
    void OnAuxiliaryAction( wxCommandEvent& event ) override;

    SCH_EDIT_FRAME*                     m_frame;

    PANEL_SETUP_FORMATTING*             m_formatting;
    PANEL_EESCHEMA_TEMPLATE_FIELDNAMES* m_fieldNameTemplates;
    PANEL_SETUP_PINMAP*                 m_pinMap;
    PANEL_SETUP_SEVERITIES*             m_severities;
    PANEL_SETUP_NETCLASSES*             m_netclasses;
    PANEL_TEXT_VARIABLES*               m_textVars;
    std::shared_ptr<ERC_ITEM>           m_pinToPinError;

    std::vector<bool>                   m_macHack;

    // event handlers
    void OnPageChange( wxBookCtrlEvent& event );
};


#endif //KICAD_DIALOG_SCHEMATIC_SETUP_H
