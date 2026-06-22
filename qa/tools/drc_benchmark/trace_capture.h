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

#ifndef QA_DRC_BENCHMARK_TRACE_CAPTURE_H
#define QA_DRC_BENCHMARK_TRACE_CAPTURE_H

#include <map>

#include <wx/log.h>
#include <wx/string.h>


/**
 * wxLog chain target that scrapes the engine's "KICAD_DRC_PROFILE" trace channel.
 *
 * The engine emits two line shapes via wxLogTrace:
 *   "DRC provider '<name>' took <ms> ms"   (one per test provider)
 *   "DRC took <ms> ms"                      (whole RunTests pass)
 *
 * Provider rows accumulate so repeated runs on the same target add up; reset with
 * Clear() between runs you want kept separate. Forwarding to the previous target is
 * preserved so normal logging still reaches the console.
 */
class DRC_PROFILE_LOG : public wxLog
{
public:
    DRC_PROFILE_LOG();
    ~DRC_PROFILE_LOG() override = default;

    void Clear();

    const std::map<wxString, double>& ProviderMs() const { return m_providerMs; }
    double                            TotalMs() const { return m_totalMs; }

    /**
     * Parse a single trace message into the maps. Exposed so --selftest can drive it
     * without a live engine. Returns true if the line matched a known shape.
     */
    bool ParseLine( const wxString& aMsg );

protected:
    void DoLogTextAtLevel( wxLogLevel aLevel, const wxString& aMsg ) override;

private:
    std::map<wxString, double> m_providerMs;
    double                     m_totalMs = 0.0;
    wxLog*                     m_prev = nullptr;
};


/**
 * Run the three fixed self-test lines through a fresh parser and confirm the parsed
 * provider map and total match the expected values. Returns 0 on success, non-zero on
 * any mismatch so main() can propagate it as the process exit code.
 */
int RunTraceCaptureSelftest();

#endif // QA_DRC_BENCHMARK_TRACE_CAPTURE_H
