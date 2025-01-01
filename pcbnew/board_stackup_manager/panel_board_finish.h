/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2019 Jean-Pierre Charras, jp.charras at wanadoo.fr
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

#ifndef PANEL_SETUP_BOARD_FINISH_H
#define PANEL_SETUP_BOARD_FINISH_H


#include <board.h>
#include "panel_board_finish_base.h"

class PAGED_DIALOG;
class PCB_EDIT_FRAME;


class PANEL_SETUP_BOARD_FINISH : public PANEL_SETUP_BOARD_FINISH_BASE
{
public:
    PANEL_SETUP_BOARD_FINISH( wxWindow* aParentWindow, PCB_EDIT_FRAME* aFrame );
    ~PANEL_SETUP_BOARD_FINISH();

    void ImportSettingsFrom( BOARD* aBoard );

    // Called by wxWidgets: transfer current settings stored in m_stackup to the board
    bool TransferDataFromWindow() override;

    bool TransferDataFromWindow( BOARD_STACKUP& aStackup );

private:
    void synchronizeWithBoard();

private:
    PCB_EDIT_FRAME*         m_frame;
    BOARD*                  m_board;
    BOARD_DESIGN_SETTINGS*  m_brdSettings;
};

#endif      // #ifndef PANEL_SETUP_BOARD_FINISH_H
