/*
 * Symbol chooser timing instrumentation.
 *
 * Enable with environment variable:
 *   KICAD_TRACE=KI_TRACE_SYM_CHOOSER
 *
 * Logs lines of the form:
 *   KI_TRACE_SYM_CHOOSER | step=<name> step_ms=XX.XXX total_ms=YY.YYY mode=cold|warm [extra]
 *
 * "cold" = first symbol chooser session (initial library load)
 * "warm" = subsequent sessions (should mostly hit caches)
 */

#pragma once

#include <chrono>
#include <wx/string.h>
#include <trace_helpers.h>

namespace SYM_CHOOSER_TIMING
{
    using clock = std::chrono::steady_clock;
    using ms    = std::chrono::duration<double, std::milli>;

    inline clock::time_point g_start_time;   // Set at beginning of PickSymbolFromLibrary
    inline bool              g_started   = false;
    inline bool              g_firstRun  = true;   // Cleared after first full AddLibraries call

    inline void Start()
    {
        g_start_time = clock::now();
        g_started = true;
    }

    inline double TotalMs()
    {
        if( !g_started )
            return 0.0;
        return ms( clock::now() - g_start_time ).count();
    }

    // Helper object to measure and log incremental steps relative to g_start_time
    class STEP_LOGGER
    {
    public:
        STEP_LOGGER()
        {
            m_last = g_started ? g_start_time : clock::now();
        }

        void Log( const char* aStep, const wxString& aExtra = wxEmptyString )
        {
            auto now = clock::now();
            double step_ms  = ms( now - m_last ).count();
            double total_ms = g_started ? ms( now - g_start_time ).count() : step_ms;
            m_last = now;

            KI_TRACE( wxT("KI_TRACE_SYM_CHOOSER"), wxT("step=%s step_ms=%.3f total_ms=%.3f mode=%s%s%s\n"),
                      wxString::FromUTF8( aStep ).c_str(), step_ms, total_ms,
                      g_firstRun ? wxT("cold") : wxT("warm"),
                      aExtra.IsEmpty() ? wxT("") : wxT(" "),
                      aExtra.c_str() );
        }

    private:
        clock::time_point m_last;
    };

    inline void LogRaw( const char* aStep, double aStepMs, double aTotalMs, const wxString& aExtra = wxEmptyString )
    {
        KI_TRACE( wxT("KI_TRACE_SYM_CHOOSER"), wxT("step=%s step_ms=%.3f total_ms=%.3f mode=%s%s%s\n"),
                  wxString::FromUTF8( aStep ).c_str(), aStepMs, aTotalMs,
                  g_firstRun ? wxT("cold") : wxT("warm"),
                  aExtra.IsEmpty() ? wxT("") : wxT(" "),
                  aExtra.c_str() );
    }
}
