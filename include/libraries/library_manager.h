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
#include <project.h>


struct KICOMMON_API LIBRARY_ERROR
{
    wxString message;
};


template<typename ResultType>
using LIBRARY_RESULT = tl::expected<ResultType, LIBRARY_ERROR>;


class KICOMMON_API LIBRARY_MANAGER
{
public:
    LIBRARY_MANAGER();

    ~LIBRARY_MANAGER();

    void LoadGlobalTables();

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

private:
    void loadTables( const wxString& aTablePath, LIBRARY_TABLE_SCOPE aScope );

    std::vector<std::unique_ptr<LIBRARY_TABLE>> m_tables;
    std::vector<std::unique_ptr<LIBRARY_TABLE>> m_project_tables;
};


/**
 * The interface used by the classes that actually can load IO plugins for the
 * different parts of KiCad and return concrete types (symbols, footprints, etc)
 */
class KICOMMON_API LIBRARY_MANAGER_ADAPTER : public PROJECT::_ELEM
{
public:
    /**
     * Constructs a project-specific and schematic-specific adapter into the library manager.
     * @param aManager should usually be Pgm().GetLibraryManager() except in QA tests
     * @param aProject is the project to construct the adapter for
     */
    LIBRARY_MANAGER_ADAPTER( LIBRARY_MANAGER& aManager, PROJECT& aProject ) :
            m_manager( aManager ),
            m_project( aProject )
    {}

    ~LIBRARY_MANAGER_ADAPTER() override = default;

    virtual int GetModifyHash() const { return 0; };

protected:

    virtual void doPreload() = 0;

    LIBRARY_MANAGER& m_manager;

    PROJECT& m_project;
};


#endif //LIBRARY_MANAGER_H
