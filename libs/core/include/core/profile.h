/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2013 CERN
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 * @author Tomasz Wlostowski <tomasz.wlostowski@cern.ch>
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

/**
 * @file profile.h:
 * @brief Simple profiling functions for measuring code execution time.
 */

#ifndef TPROFILE_H
#define TPROFILE_H

#include <atomic>
#include <chrono>
#include <sstream>
#include <string>
#include <iostream>
#include <iomanip>
#include <cstdint>

/**
 * A small class to help profiling.
 *
 * It allows the calculation of the elapsed time (in milliseconds) between
 * its creation (or the last call to Start() ) and the last call to Stop()
 */
class PROF_TIMER
{
public:
    /**
     * Create a PROF_COUNTER for measuring an elapsed time in milliseconds.
     *
     * @param aName a string that will be printed in message.
     * @param aAutostart true (default) to immediately start the timer
     */
    PROF_TIMER( const std::string& aName, bool aAutostart = true ) :
        m_name( aName ), m_running( false )
    {
        if( aAutostart )
            Start();
    }

    /**
     * Create a PROF_COUNTER for measuring an elapsed time in milliseconds
     *
     * The counter is started and the string to print in message is left empty.
     */
    PROF_TIMER()
    {
        Start();
    }

    /**
     * Start or restart the counter.
     */
    void Start()
    {
        m_running = true;
        m_starttime = CLOCK::now();
        m_lasttime = m_starttime;
    }


    /**
     * Save the time when this function was called, and set the counter stane to stop.
     */
    void Stop()
    {
        if( !m_running )
            return;

        m_stoptime = CLOCK::now();
        m_running = false;
    }

    /**
     * Print the elapsed time (in a suitable unit) to a stream.
     *
     * The unit is automatically chosen from ns, us, ms and s, depending on the
     * size of the current count.
     *
     * @param the stream to print to.
     */
    void Show( std::ostream& aStream = std::cerr )
    {
        using DURATION = std::chrono::duration<double, std::nano>;

        const auto   duration = SinceStart<DURATION>();
        const double cnt = duration.count();

        if( m_name.size() )
        {
            aStream << m_name << " took ";
        }

        if( cnt < 1e3 )
            aStream << cnt << "ns";
        else if( cnt < 1e6 )
            aStream << ( cnt / 1e3 ) << "µs";
        else if( cnt < 1e9 )
            aStream << ( cnt / 1e6 ) << "ms";
        else
            aStream << ( cnt / 1e9 ) << "s";

        aStream << std::endl;
    }

    /**
     * @return the time since the timer was started. If the timer is stopped, the duration
     *         is from the start time to the time it was stopped, else it is to the current
     *         time.
     */
    template <typename DURATION>
    DURATION SinceStart( bool aSinceLast = false )
    {
        const TIME_POINT stoptime = m_running ? CLOCK::now() : m_stoptime;
        const TIME_POINT starttime = aSinceLast ? m_lasttime : m_starttime;

        m_lasttime = stoptime;

        return std::chrono::duration_cast<DURATION>( stoptime - starttime );
    }

    /**
     * @param aSinceLast only get the time since the last time the time was read.
     * @return the elapsed time in ms since the timer was started.
     */
    double msecs( bool aSinceLast = false )
    {
        using DUR_MS = std::chrono::duration<double, std::milli>;
        return SinceStart<DUR_MS>( aSinceLast ).count();
    }

    std::string to_string()
    {
        using DURATION = std::chrono::duration<double, std::nano>;

        const auto   duration = SinceStart<DURATION>();
        const double cnt = duration.count();
        std::string retv;

        if( !m_name.empty() )
            retv = m_name + ": ";

        std::stringstream time;

        if( cnt < 1e3 )
            time << cnt << "ns";
        else if( cnt < 1e6 )
            time << ( cnt / 1e3 ) << "µs";
        else if( cnt < 1e9 )
            time << ( cnt / 1e6 ) << "ms";
        else
            time << ( cnt / 1e9 ) << "s";

        retv += time.str();

        return retv;
    }

private:
    std::string m_name;     // a string printed in message
    bool m_running;

    using CLOCK = std::chrono::high_resolution_clock;
    using TIME_POINT = std::chrono::time_point<CLOCK>;

    TIME_POINT m_starttime, m_lasttime, m_stoptime;
};


/**
 * A simple RAII class to measure the time of an operation.
 *
 * On construction, a timer is started, and on destruction, the timer is
 * ended, and the time difference is written into the given duration.
 *
 * For example:
 *
 * DURATION duration; // select a duration type as needed
 * {
 *     SCOPED_PROF_TIMER<DURATION> timer( duration );
 *     timed_activity();
 * }
 * // duration is now the time timed activity took
 *
 * From C++17, with class template argument deduction, you should be able to
 * omit the <DURATION>.
 */
template <typename DURATION>
class SCOPED_PROF_TIMER : public PROF_TIMER
{
public:
    SCOPED_PROF_TIMER( DURATION& aDuration ) : PROF_TIMER(), m_duration( aDuration )
    {
    }

    ~SCOPED_PROF_TIMER()
    {
        // update the output
        m_duration = m_counter.SinceStart<DURATION>();
    }

private:
    ///< The counter to use to do the profiling
    PROF_TIMER m_counter;

    ///< The duration to update at the end of the scope
    DURATION& m_duration;
};


/**
 * An alternate way to calculate an elapsed time (in microsecondes) to class PROF_COUNTER
 *
 * @return an ever increasing indication of elapsed microseconds.  Use this by computing
 *         differences between two calls.
 * @author Dick Hollenbeck
 */
int64_t GetRunningMicroSecs();


/**
 * A thread-safe event counter
 */
class PROF_COUNTER
{
public:
    PROF_COUNTER() :
            m_name( "Anonymous" ),
            m_count( 0 )
    {
    }

    PROF_COUNTER( const std::string& aName ) :
            m_name( aName ),
            m_count( 0 )
    {
    }

    unsigned long long Count() const
    {
        return m_count.load();
    }

    void Reset()
    {
        m_count.store( 0 );
    }

    unsigned long long operator++( int )
    {
        return m_count++;
    }

    void Show( std::ostream& aStream = std::cerr )
    {
        if( m_name.size() )
            aStream << m_name << ": ";

        aStream << m_count.load();
        aStream << std::endl;
    }

private:
    std::string        m_name;
    std::atomic_ullong m_count;
};

#endif  // TPROFILE_H
