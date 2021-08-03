/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2019-2021 KiCad Developers, see AUTHORS.txt for contributors.
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


#ifndef QA_PCBNEW_BOARD_TEST_UTILS__H
#define QA_PCBNEW_BOARD_TEST_UTILS__H

#include <string>
#include <wx/string.h>

class BOARD;
class BOARD_ITEM;
class SETTINGS_MANAGER;


namespace KI_TEST
{
/**
 * A helper that contains logic to assist in dumping boards to
 * disk depending on some environment variables.
 *
 * This is useful when setting up or verifying unit tests that work on BOARD
 * objects.
 *
 * To dump files set the KICAD_TEST_DUMP_BOARD_FILES environment variable.
 * Files will be written to the system temp directory (/tmp on Linux, or as set
 * by $TMP and friends).
 */
class BOARD_DUMPER
{
public:
    BOARD_DUMPER();

    void DumpBoardToFile( BOARD& aBoard, const std::string& aName ) const;

    const bool m_dump_boards;
};


void LoadBoard( SETTINGS_MANAGER& aSettingsManager, const wxString& aRelPath,
                std::unique_ptr<BOARD>& aBoard );

void FillZones( BOARD* m_board, int aFillVersion );


} // namespace KI_TEST

#endif // QA_PCBNEW_BOARD_TEST_UTILS__H