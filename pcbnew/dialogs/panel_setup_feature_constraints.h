/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2019 KiCad Developers, see AUTHORS.txt for contributors.
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


#ifndef PANEL_SETUP_FEATURE_CONSTRAINTS_H
#define PANEL_SETUP_FEATURE_CONSTRAINTS_H

#include <class_board.h>
#include <widgets/unit_binder.h>
#include <widgets/paged_dialog.h>
#include <panel_setup_feature_constraints_base.h>

class PCB_EDIT_FRAME;
class BOARD_DESIGN_SETTINGS;


class PANEL_SETUP_FEATURE_CONSTRAINTS : public PANEL_SETUP_FEATURE_CONSTRAINTS_BASE
{
private:
    PCB_EDIT_FRAME*         m_Frame;
    BOARD_DESIGN_SETTINGS*  m_BrdSettings;

public:
    UNIT_BINDER             m_trackMinWidth;
    UNIT_BINDER             m_viaMinSize;
    UNIT_BINDER             m_viaMinDrill;
    UNIT_BINDER             m_uviaMinSize;
    UNIT_BINDER             m_uviaMinDrill;
    UNIT_BINDER             m_holeToHoleMin;
    UNIT_BINDER             m_edgeClearance;
    UNIT_BINDER             m_maxError;

public:
    PANEL_SETUP_FEATURE_CONSTRAINTS( PAGED_DIALOG* aParent, PCB_EDIT_FRAME* aFrame );
    ~PANEL_SETUP_FEATURE_CONSTRAINTS( ) override { };

    void ImportSettingsFrom( BOARD* aBoard );

private:
    bool TransferDataToWindow() override;
    bool TransferDataFromWindow() override;

    void onChangeOutlineOpt( wxCommandEvent& event ) override;
};

#endif //PANEL_SETUP_FEATURE_CONSTRAINTS_H
