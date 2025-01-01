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


#ifndef PANEL_SETUP_CONSTRAINTS_H
#define PANEL_SETUP_CONSTRAINTS_H

#include <widgets/unit_binder.h>
#include <panel_setup_constraints_base.h>

class BOARD;
class BOARD_DESIGN_SETTINGS;
class PAGED_DIALOG;
class PCB_EDIT_FRAME;
class wxCommandEvent;


class PANEL_SETUP_CONSTRAINTS : public PANEL_SETUP_CONSTRAINTS_BASE
{
public:
    PANEL_SETUP_CONSTRAINTS( wxWindow* aParentWindow, PCB_EDIT_FRAME* aFrame );
    ~PANEL_SETUP_CONSTRAINTS( ) override { };

    void ImportSettingsFrom( BOARD* aBoard );

private:
    bool TransferDataToWindow() override;
    bool TransferDataFromWindow() override;

public:
    UNIT_BINDER             m_minClearance;
    UNIT_BINDER             m_minConn;
    UNIT_BINDER             m_trackMinWidth;
    UNIT_BINDER             m_viaMinAnnulus;
    UNIT_BINDER             m_viaMinSize;
    UNIT_BINDER             m_throughHoleMin;
    UNIT_BINDER             m_uviaMinSize;
    UNIT_BINDER             m_uviaMinDrill;
    UNIT_BINDER             m_holeToHoleMin;
    UNIT_BINDER             m_holeClearance;
    UNIT_BINDER             m_edgeClearance;
    UNIT_BINDER             m_silkClearance;
    UNIT_BINDER             m_minGrooveWidth;
    UNIT_BINDER             m_minTextHeight;
    UNIT_BINDER             m_minTextThickness;
    UNIT_BINDER             m_maxError;

private:
    PCB_EDIT_FRAME*         m_Frame;
    BOARD_DESIGN_SETTINGS*  m_BrdSettings;
};

#endif //PANEL_SETUP_CONSTRAINTS_H
