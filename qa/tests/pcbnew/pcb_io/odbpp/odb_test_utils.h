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

#ifndef QA_PCBNEW_ODBPP_TEST_UTILS_H
#define QA_PCBNEW_ODBPP_TEST_UTILS_H

#include <filesystem>
#include <string>

class BOARD;
class REPORTER;

/**
 * Save and restore the static ODB++ exporter formatting state.
 *
 * SaveBoard() writes these, so without a guard the units/sigfig one test asks for would follow
 * the shared qa_pcbnew binary into every later ODB++ test.
 */
struct ODB_EXPORT_STATE_GUARD
{
    ODB_EXPORT_STATE_GUARD();
    ~ODB_EXPORT_STATE_GUARD();

    double      m_scale;
    double      m_symbolScale;
    int         m_sigfig;
    std::string m_unitsStr;
};

/**
 * Export a board to an ODB++ product model tree rooted at aDir and return aDir.
 *
 * aDir must be a directory the caller does not share with any other export, since the exporter
 * does not clear stale files from a previous run out of a reused directory.
 */
std::filesystem::path ExportOdb( const BOARD& aBoard, const std::filesystem::path& aDir,
                                 const std::string& aUnits = "mm", const std::string& aSigfig = "6",
                                 REPORTER* aReporter = nullptr );

#endif // QA_PCBNEW_ODBPP_TEST_UTILS_H
