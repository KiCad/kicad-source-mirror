/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2017-2023 KiCad Developers, see AUTHORS.txt for contributors.
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

#include <mutex>
#include <widgets/paged_dialog.h>
#include "panel_setup_formatting.h"

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
    // event handlers
    void onPageChanged( wxBookCtrlEvent& aEvent ) override;
    void onAuxiliaryAction( wxCommandEvent& aEvent ) override;

    PCB_EDIT_FRAME*            m_frame;
    PANEL_SETUP_LAYERS*        m_layers;
    PANEL_SETUP_BOARD_STACKUP* m_physicalStackup;

public:
    static std::mutex g_Mutex;      // Mutex to prevent multiple windows opening

private:
    size_t m_currentPage;              // the current page index
    size_t m_layersPage;
    size_t m_physicalStackupPage;
    size_t m_boardFinishPage;
    size_t m_textAndGraphicsPage;
    size_t m_formattingPage;
    size_t m_maskAndPagePage;
    size_t m_constraintsPage;
    size_t m_tracksAndViasPage;
    size_t m_netclassesPage;
    size_t m_severitiesPage;
};


#endif //KICAD_DIALOG_BOARD_SETUP_H
