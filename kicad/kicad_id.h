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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef KICAD_ID_H
#define KICAD_ID_H

#include <id.h>
#include <eda_base_frame.h>

/**
 * Legacy IDs for things that have not yet been moved to ACTIONs.
 */

enum id_kicad_frm {
    ID_LEFT_FRAME = ID_KICAD_MANAGER_START,
    ID_PROJECT_TREE,
    ID_EDIT_LOCAL_FILE_IN_TEXT_EDITOR,
    ID_EDIT_ADVANCED_CFG,
    ID_IMPORT_CADSTAR_ARCHIVE_PROJECT,
    ID_IMPORT_EAGLE_PROJECT,
    ID_IMPORT_EASYEDA_PROJECT,
    ID_IMPORT_EASYEDAPRO_PROJECT,
    ID_IMPORT_ALTIUM_PROJECT,
    ID_IMPORT_PADS_PROJECT,
    ID_IMPORT_PCAD_PROJECT,
    ID_IMPORT_GEDA_PROJECT,
    ID_IMPORT_DIPTRACE_PROJECT,
    ID_COMPARE_PROJECT_BRANCHES
};

#endif
