/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2015-2023 KiCad Developers, see AUTHORS.txt for contributors.
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

#pragma once

#include <dialog_gen_footprint_position_file_base.h>

class PCB_EDIT_FRAME;
class REPORTER;

/**
 * The dialog to create footprint position files and choose options (one or 2 files, units
 * and force all SMD footprints in list)
 */
class DIALOG_GEN_FOOTPRINT_POSITION : public DIALOG_GEN_FOOTPRINT_POSITION_BASE
{
public:
    DIALOG_GEN_FOOTPRINT_POSITION( PCB_EDIT_FRAME* aEditFrame );

private:
    void initDialog();
    void OnOutputDirectoryBrowseClicked( wxCommandEvent& event ) override;
    void OnGenerate( wxCommandEvent& event ) override;

    void onUpdateUIUnits( wxUpdateUIEvent& event ) override;

    void onUpdateUIFileOpt( wxUpdateUIEvent& event ) override;

    void onUpdateUIOnlySMD( wxUpdateUIEvent& event ) override;

    void onUpdateUInegXcoord( wxUpdateUIEvent& event ) override;

    void onUpdateUIExcludeTH( wxUpdateUIEvent& event ) override;

    void onUpdateUIincludeBoardEdge( wxUpdateUIEvent& event ) override;

    /**
     * Creates files in text or csv format
     */
    bool CreateAsciiFiles();

    /**
     * Creates placement files in gerber format
     */
    bool CreateGerberFiles();

    // accessors to options:
    bool UnitsMM();

    bool OneFileOnly();

    bool OnlySMD();

    bool ExcludeAllTH();

    bool ExcludeDNP();

private:
    PCB_EDIT_FRAME* m_editFrame;
    REPORTER*       m_reporter;
};