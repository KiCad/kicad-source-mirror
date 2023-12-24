/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2023 KiCad Developers, see AUTHORS.txt for contributors.
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


#ifndef IO_BASE_H_
#define IO_BASE_H_

#include <wx/string.h>

class REPORTER;
class PROGRESS_REPORTER;

class IO_BASE
{
public:

    virtual ~IO_BASE() = default;

    /**
     * Return a brief hard coded name for this IO interface.
     */
    const wxString& GetName() const { return m_name; }

    /**
     * Set an optional reporter for warnings/errors.
     */
    virtual void SetReporter( REPORTER* aReporter ) { m_reporter = aReporter; }

    /**
     * Set an optional progress reporter.
     */
    virtual void SetProgressReporter( PROGRESS_REPORTER* aReporter ) { m_progressReporter = aReporter; }

protected:
    // Delete the zero-argument base constructor to force proper construction
    IO_BASE() = delete;

    /**
     *
     * @param aName is the user-visible name for the IO loader
     */
    IO_BASE( const wxString& aName ) :
        m_name( aName ),
        m_reporter( nullptr ),
        m_progressReporter( nullptr )
    {
    }


    /// Name of the IO loader
    wxString m_name;

    /// Reporter to log errors/warnings to, may be nullptr
    REPORTER* m_reporter;

    /// Progress reporter to track the progress of the operation, may be nullptr
    PROGRESS_REPORTER* m_progressReporter;
};

#endif // IO_BASE_H_