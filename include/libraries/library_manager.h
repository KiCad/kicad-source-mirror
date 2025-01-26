/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 * @author Jon Evans <jon@craftyjon.com>
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

#ifndef LIBRARY_MANAGER_H
#define LIBRARY_MANAGER_H

#include <tl/expected.hpp>

#include <kicommon.h>
#include <libraries/library_table.h>


struct KICOMMON_API LIBRARY_ERROR
{
    wxString message;
};


template<typename ResultType>
using LIBRARY_RESULT = tl::expected<ResultType, LIBRARY_ERROR>;

class LIBRARY_MANAGER_ADAPTER;


class KICOMMON_API LIBRARY_MANAGER
{
public:
    LIBRARY_MANAGER();

    ~LIBRARY_MANAGER();

    void LoadGlobalTables();

    /// Notify all adapters that the project has changed
    void ProjectChanged();

    void RegisterAdapter( LIBRARY_TABLE_TYPE aType,
                          std::unique_ptr<LIBRARY_MANAGER_ADAPTER>&& aAdapter );

    std::optional<LIBRARY_MANAGER_ADAPTER*> Adapter( LIBRARY_TABLE_TYPE aType ) const;

    /**
     * Returns a flattened list of libraries of the given type
     * @param aType determines which type of libraries to return (symbol, footprint, ...)
     * @param aIncludeInvalid will include the nicknames of libraries even if they could not be
     *                        loaded for some reason (file not found, etc)
     * @return a list of library nicknames (the first part of a LIB_ID)
     */
    std::vector<const LIBRARY_TABLE_ROW*> Rows( LIBRARY_TABLE_TYPE aType,
                                                LIBRARY_TABLE_SCOPE aScope = LIBRARY_TABLE_SCOPE::BOTH,
                                                bool aIncludeInvalid = false ) const;

    /**
     *
     * @param aType determines which type of libraries to return (symbol, footprint, ...)
     * @param aScope determines whether to search project, global, or both library tables
     * @param aNickname is the library nickname to retrieve
     * @return the row, or a LIBRARY_ERROR with an error message
     */
    LIBRARY_RESULT<const LIBRARY_TABLE_ROW*> GetRow( LIBRARY_TABLE_TYPE aType,
                                                     const wxString& aNickname,
                                                     LIBRARY_TABLE_SCOPE aScope = LIBRARY_TABLE_SCOPE::BOTH ) const;

    void LoadProjectTables( const wxString& aProjectPath );

    /**
     * Return the full location specifying URI for the LIB, either in original UI form or
     * in environment variable expanded form.
     *
     * @param aType determines which tables will be searched for the library
     * @param aNickname is the library to look up
     * @param aSubstituted Tells if caller wanted the substituted form, else not.
     * @return the URI for the given library, or nullopt if the nickname is not a valid library
     */
    std::optional<wxString> GetFullURI( LIBRARY_TABLE_TYPE aType, const wxString& aNickname,
                                        bool aSubstituted = false ) const;

    static bool UrisAreEquivalent( const wxString& aURI1, const wxString& aURI2 );

private:
    void loadTables( const wxString& aTablePath, LIBRARY_TABLE_SCOPE aScope );

    std::vector<std::unique_ptr<LIBRARY_TABLE>> m_tables;
    std::vector<std::unique_ptr<LIBRARY_TABLE>> m_project_tables;

    std::map<LIBRARY_TABLE_TYPE, std::unique_ptr<LIBRARY_MANAGER_ADAPTER>> m_adapters;
};


/**
 * The interface used by the classes that actually can load IO plugins for the
 * different parts of KiCad and return concrete types (symbols, footprints, etc)
 */
class KICOMMON_API LIBRARY_MANAGER_ADAPTER
{
public:
    /**
     * Constructs a type-specific adapter into the library manager.  The code for these
     * generally resides in app-specific libraries (eeschema/pcbnew for example) rather than
     * being in kicommon.
     *
     * @param aManager should usually be Pgm().GetLibraryManager() except in QA tests
     */
    LIBRARY_MANAGER_ADAPTER( LIBRARY_MANAGER& aManager ) :
            m_manager( aManager )
    {}

    virtual ~LIBRARY_MANAGER_ADAPTER();

    /// The type of library table this adapter works with
    virtual LIBRARY_TABLE_TYPE Type() const = 0;

    virtual int GetModifyHash() const { return 0; };

    /// Notify the adapter that the active project has changed
    virtual void ProjectChanged() {};

    /// Return true if the given nickname exists and is not a read-only library
    virtual bool IsWritable( const wxString& aNickname ) const { return false; }

protected:

    virtual void doPreload() = 0;

    LIBRARY_MANAGER& m_manager;
};


#endif //LIBRARY_MANAGER_H
