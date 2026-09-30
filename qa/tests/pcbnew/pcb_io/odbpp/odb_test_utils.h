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

#include <clocale>
#include <filesystem>
#include <map>
#include <set>
#include <string>
#include <vector>

#include <wx/string.h>

#include <layer_ids.h>
#include <pad.h>
#include <qa_utils/file_utils.h>
#include <pcbnew/pcb_io/odbpp/odb_export_job.h>

class BOARD;
class REPORTER;

/// Set C LC_CTYPE on non-Windows hosts and restore the previous locale
class SCOPED_C_CTYPE
{
public:
    SCOPED_C_CTYPE() : m_previous( std::setlocale( LC_CTYPE, nullptr ) )
    {
#ifndef _WIN32
        m_ok = std::setlocale( LC_CTYPE, "C" ) != nullptr;
#endif
    }

    ~SCOPED_C_CTYPE() { std::setlocale( LC_CTYPE, m_previous.c_str() ); }

    bool Ok() const { return m_ok; }

private:
    std::string m_previous;
    bool        m_ok = true;
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

/**
 * Read a text file into one string per line, with no trailing newline characters.
 */
std::vector<std::string> ReadLines( const std::filesystem::path& aFile );

const KI_TEST::SCOPED_TEMP_DIR& TempDir();

size_t CountPads( const BOARD& aBoard, PAD_PROP aProperty, PCB_LAYER_ID aLayer );

class OdbFeatureFile
{
public:
    explicit OdbFeatureFile( const std::filesystem::path& aFile );

    size_t CountRecordsWithOption( const std::string& aName, const std::string& aOption ) const;
    size_t CountRecordsWithFlag( const std::string& aName ) const;
    size_t CountRecordsWithInteger( const std::string& aName, int aValue ) const;

private:
    size_t CountRecordsWithValue( const std::string& aName, const std::string& aValue ) const;

    std::map<std::string, std::string> m_names;
    std::vector<std::string>           m_records;
};

/**
 * The matrix and layer directories of an exported ODB++ product model tree.
 */
class ODB_PRODUCT
{
public:
    explicit ODB_PRODUCT( const std::filesystem::path& aRoot );

    const std::vector<ODB_MATRIX_ROW>& Matrix() const { return m_matrix; }

    /// Path of a file in a layer directory, matched to the directory name without regard to case
    wxString LayerFile( const wxString& aLayer, const wxString& aFile ) const;

private:
    std::filesystem::path       m_stepDir;
    std::vector<ODB_MATRIX_ROW> m_matrix;
};

#endif // QA_PCBNEW_ODBPP_TEST_UTILS_H
