/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2017-2018 KiCad Developers, see AUTHORS.txt for contributors.
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


#ifndef KICAD_DIALOG_BOARD_SETUP_H
#define KICAD_DIALOG_BOARD_SETUP_H

#include <widgets/paged_dialog.h>

class PCB_EDIT_FRAME;
class PANEL_SETUP_CONSTRAINTS;
class PANEL_SETUP_LAYERS;
class PANEL_SETUP_TEXT_AND_GRAPHICS;
class PANEL_SETUP_NETCLASSES;
class PANEL_SETUP_RULES;
class PANEL_SETUP_TRACKS_AND_VIAS;
class PANEL_SETUP_MASK_AND_PASTE;
class PANEL_SETUP_BOARD_STACKUP;
class PANEL_SETUP_BOARD_FINISH;
class PANEL_SETUP_SEVERITIES;
class PANEL_TEXT_VARIABLES;


class DIALOG_BOARD_SETUP : public PAGED_DIALOG
{
public:
    DIALOG_BOARD_SETUP( PCB_EDIT_FRAME* aFrame );
    ~DIALOG_BOARD_SETUP();

protected:
    void OnAuxiliaryAction( wxCommandEvent& event ) override;

    PCB_EDIT_FRAME*                  m_frame;

    PANEL_SETUP_CONSTRAINTS*         m_constraints;
    PANEL_SETUP_LAYERS*              m_layers;
    PANEL_SETUP_TEXT_AND_GRAPHICS*   m_textAndGraphics;
    PANEL_SETUP_NETCLASSES*          m_netclasses;
    PANEL_SETUP_RULES*               m_rules;
    PANEL_SETUP_TRACKS_AND_VIAS*     m_tracksAndVias;
    PANEL_SETUP_MASK_AND_PASTE*      m_maskAndPaste;
    PANEL_SETUP_BOARD_STACKUP*       m_physicalStackup;
    PANEL_SETUP_BOARD_FINISH*        m_boardFinish;
    PANEL_SETUP_SEVERITIES*          m_severities;
    PANEL_TEXT_VARIABLES*            m_textVars;

    std::vector<bool>                m_macHack;

    // event handlers
    void OnPageChange( wxBookCtrlEvent& event );

private:
    int m_currentPage;              // the current page index
    int m_physicalStackupPage;      // the page index of the PANEL_SETUP_BOARD_STACKUP page
    int m_layerSetupPage;           // the page index of the PANEL_SETUP_LAYERS page
};


#endif //KICAD_DIALOG_BOARD_SETUP_H
