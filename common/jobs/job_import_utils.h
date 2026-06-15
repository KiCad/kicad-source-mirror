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

#ifndef JOB_IMPORT_UTILS_H
#define JOB_IMPORT_UTILS_H

#include <kicommon.h>
#include <json_common.h>
#include <wx/string.h>
#include <map>
#include <utility>
#include <vector>

class REPORTER;

/**
 * Output format for the report shared by the board and schematic import jobs.
 */
enum class IMPORT_REPORT_FORMAT
{
    NONE,
    JSON,
    TEXT
};


NLOHMANN_JSON_SERIALIZE_ENUM( IMPORT_REPORT_FORMAT,
                              {
                                  { IMPORT_REPORT_FORMAT::NONE, "none" },
                                  { IMPORT_REPORT_FORMAT::JSON, "json" },
                                  { IMPORT_REPORT_FORMAT::TEXT, "text" }
                              } )


/**
 * Data describing a completed import, rendered uniformly by #WriteImportReport for both the
 * board and schematic import jobs.
 */
struct KICOMMON_API IMPORT_REPORT_DATA
{
    wxString m_sourceFile;
    wxString m_sourceFormat;
    wxString m_outputFile;

    /// Statistics keyed by their JSON name (lower case).  The text report capitalizes the first
    /// letter of each key for display.  Order is preserved in both renderings.
    std::vector<std::pair<wxString, size_t>> m_statistics;

    /// Extra members merged verbatim into the JSON report (e.g. layer mapping).  The text report
    /// ignores them.
    nlohmann::json m_extraJson = nlohmann::json::object();

    std::vector<wxString> m_warnings;
    std::vector<wxString> m_errors;
};


/**
 * Parse the user-facing report format name ("none", "json" or "text") into its enum.
 *
 * @return false if the name is not recognized, leaving @p aFormat untouched.
 */
KICOMMON_API bool ParseImportReportFormat( const wxString& aText, IMPORT_REPORT_FORMAT& aFormat );


/**
 * Build the default output path for an import by swapping the input file's extension for the
 * given KiCad extension, keeping it in the input file's directory.
 */
KICOMMON_API wxString DefaultImportOutputPath( const wxString& aInputFile,
                                               const wxString& aKiCadExt );


/**
 * Load an explicit layer-mapping file into @p aMap.  The file is a flat JSON object whose keys
 * are source (foreign) layer names and whose values are KiCad layer names, e.g.
 * @code { "Top": "F.Cu", "Bottom": "B.Cu" } @endcode
 *
 * @return false if the file cannot be read or does not parse as an object of string-to-string
 *         pairs, leaving @p aMap untouched and a human-readable reason in @p aError.
 */
KICOMMON_API bool LoadLayerMapFile( const wxString& aFile, std::map<wxString, wxString>& aMap,
                                    wxString& aError );


/**
 * Emit an import report in the requested format to @p aReportFile, or to @p aReporter (at INFO
 * severity) when no file is given.  A NONE format is a no-op.
 */
KICOMMON_API void WriteImportReport( REPORTER* aReporter, IMPORT_REPORT_FORMAT aFormat,
                                     const wxString& aReportFile, const IMPORT_REPORT_DATA& aData );

#endif
