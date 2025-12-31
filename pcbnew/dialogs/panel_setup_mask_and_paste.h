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


#ifndef PANEL_SETUP_DEFAULT_CLEARANCES_H
#define PANEL_SETUP_DEFAULT_CLEARANCES_H

#include <board.h>
#include <widgets/unit_binder.h>
#include <widgets/margin_offset_binder.h>
#include <widgets/paged_dialog.h>
#include <dialogs/panel_setup_mask_and_paste_base.h>

class PCB_EDIT_FRAME;
class BOARD;
class BOARD_DESIGN_SETTINGS;


class PANEL_SETUP_MASK_AND_PASTE : public PANEL_SETUP_MASK_AND_PASTE_BASE
{
private:
    PCB_EDIT_FRAME*         m_Frame;
    BOARD_DESIGN_SETTINGS*  m_BrdSettings;

    UNIT_BINDER             m_maskExpansion;
    UNIT_BINDER             m_maskMinWidth;
    UNIT_BINDER             m_maskToCopperClearance;
    MARGIN_OFFSET_BINDER    m_pasteMargin;

public:
    PANEL_SETUP_MASK_AND_PASTE( wxWindow* aParentWindow, PCB_EDIT_FRAME* aFrame );
    ~PANEL_SETUP_MASK_AND_PASTE( ) { };

    bool TransferDataToWindow() override;
    bool TransferDataFromWindow() override;

    void ImportSettingsFrom( BOARD* aBoard );
};

#endif //PANEL_SETUP_DEFAULT_CLEARANCES_H


