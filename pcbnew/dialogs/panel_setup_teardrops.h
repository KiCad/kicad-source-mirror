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

#ifndef PANEL_SETUP_TEARDROPS_H
#define PANEL_SETUP_TEARDROPS_H

#include <widgets/unit_binder.h>
#include <panel_setup_teardrops_base.h>

class PCB_EDIT_FRAME;
class BOARD;
class PAGED_DIALOG;


class PANEL_SETUP_TEARDROPS : public PANEL_SETUP_TEARDROPS_BASE
{
public:
    PANEL_SETUP_TEARDROPS( wxWindow* aParentWindow, PCB_EDIT_FRAME* aFrame );

    bool TransferDataToWindow() override;
    bool TransferDataFromWindow() override;

    void ImportSettingsFrom( BOARD* aBoard );

private:
    BOARD_DESIGN_SETTINGS*  m_BrdSettings;

    UNIT_BINDER             m_teardropMaxLenRound;
    UNIT_BINDER             m_teardropMaxWidthRound;
    UNIT_BINDER             m_teardropMaxLenRect;
    UNIT_BINDER             m_teardropMaxWidthRect;
    UNIT_BINDER             m_teardropMaxLenT2T;
    UNIT_BINDER             m_teardropMaxWidthT2T;
};


#endif //PANEL_SETUP_TEARDROPS_H
