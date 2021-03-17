/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2021 Jon Evans <jon@craftyjon.com>
 * Copyright (C) 2021 KiCad Developers, see AUTHORS.txt for contributors.
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

#ifndef KICAD_SYMBOL_ASYNC_LOADER_H
#define KICAD_SYMBOL_ASYNC_LOADER_H

#include <atomic>
#include <future>
#include <mutex>
#include <vector>

#include <symbol_tree_model_adapter.h>

class LIB_PART;
class PROGRESS_REPORTER;


class SYMBOL_ASYNC_LOADER
{
public:
    /**
     * Constructs a loader for symbol libraries
     * @param aNicknames is a list of library nicknames to load
     * @param aAdapter is a pointer to the parent symbol tree model adapter
     * @param aOutput will be filled with the loaded parts
     * @param aReporter will be used to repord progress, of not null
     */
    SYMBOL_ASYNC_LOADER( const std::vector<wxString>& aNicknames,
                         SYMBOL_TREE_MODEL_ADAPTER* aAdapter,
                         std::unordered_map<wxString, std::vector<LIB_PART*>>& aOutput,
                         PROGRESS_REPORTER* aReporter = nullptr );

    ~SYMBOL_ASYNC_LOADER();

    /**
     * Spins up threads to load all the libraries in m_nicknames
     */
    void Start();

    /**
     * Finalizes the threads and combines the output into the target output map
     */
    bool Join();

    /**
     * Cancels a load in-progress
     */
    void Abort();

    ///< Return true if loading is done
    bool Done();

    ///< Returns a string containing any errors generated during the load
    const wxString& GetErrors() const { return m_errors; }

    ///< Represents a pair of <nickname, loaded parts list>
    typedef std::pair<wxString, std::vector<LIB_PART*>> LOADED_PAIR;

private:
    ///< Worker job that loads libraries and returns a list of pairs of <nickname, loaded parts>
    std::vector<LOADED_PAIR> worker();

    ///<  list of libraries to load
    std::vector<wxString> m_nicknames;

    ///< Handle to the adapter that owns this loader
    SYMBOL_TREE_MODEL_ADAPTER* m_adapter;

    ///< True if we are loading only power symbols
    bool m_onlyPowerSymbols;

    ///< Handle to map that will be filled with the loaded parts per library
    std::unordered_map<wxString, std::vector<LIB_PART*>>& m_output;

    ///< Progress reporter (may be null)
    PROGRESS_REPORTER* m_reporter;

    size_t              m_threadCount;
    std::atomic<size_t> m_nextLibrary;
    std::atomic_bool    m_canceled;
    wxString            m_errors;
    std::mutex          m_errorMutex;

    std::vector<std::future<std::vector<LOADED_PAIR>>> m_returns;
};

#endif
