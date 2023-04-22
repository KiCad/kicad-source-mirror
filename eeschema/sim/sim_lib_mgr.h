/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2022 Mikolaj Wielgus
 * Copyright (C) 2022-2023 KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 3
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, you may find one here:
 * https://www.gnu.org/licenses/gpl-3.0.html
 * or you may search the http://www.gnu.org website for the version 3 license,
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

#ifndef SIM_LIB_MGR_H
#define SIM_LIB_MGR_H

#include <memory>
#include <map>
#include <vector>
#include <string>

#include <sim/sim_library.h>
#include <sim/sim_model.h>

class PROJECT;
class SCH_SYMBOL;


class SIM_LIB_MGR
{
public:
    SIM_LIB_MGR( const PROJECT* aPrj, REPORTER* aReporter = nullptr );
    virtual ~SIM_LIB_MGR() = default;

    void SetReporter( REPORTER* aReporter ) { m_reporter = aReporter; }
    void SetForceFullParse() { m_forceFullParse = true; }

    void Clear();

    void SetLibrary( const wxString& aLibraryPath );

    SIM_MODEL& CreateModel( SIM_MODEL::TYPE aType, const std::vector<LIB_PIN*>& aPins );

    SIM_MODEL& CreateModel( const SIM_MODEL* aBaseModel, const std::vector<LIB_PIN*>& aPins );

    template <typename T>
    SIM_MODEL& CreateModel( const SIM_MODEL* aBaseModel, const std::vector<LIB_PIN*>& aPins,
                            const std::vector<T>& aFields );

    // TODO: The argument can be made const.
    SIM_LIBRARY::MODEL CreateModel( const SCH_SHEET_PATH* aSheetPath, SCH_SYMBOL& aSymbol );

    template <typename T>
    SIM_LIBRARY::MODEL CreateModel( const std::vector<T>& aFields,
                                    const std::vector<LIB_PIN*>& aPins, bool aResolved );

    template <typename T>
    SIM_LIBRARY::MODEL CreateModel( const wxString& aLibraryPath,
                                    const std::string& aBaseModelName,
                                    const std::vector<T>& aFields,
                                    const std::vector<LIB_PIN*>& aPins );

    void SetModel( int aIndex, std::unique_ptr<SIM_MODEL> aModel );

    std::map<wxString, std::reference_wrapper<const SIM_LIBRARY>> GetLibraries() const;
    std::vector<std::reference_wrapper<SIM_MODEL>> GetModels() const;

    static wxString ResolveLibraryPath( const wxString& aLibraryPath, const PROJECT* aProject );
    wxString ResolveEmbeddedLibraryPath( const wxString& aLibPath, const wxString& aRelativeLib );

private:
    const PROJECT*                                   m_project;
    REPORTER*                                        m_reporter;
    bool                                             m_forceFullParse;
    std::map<wxString, std::unique_ptr<SIM_LIBRARY>> m_libraries;
    std::vector<std::unique_ptr<SIM_MODEL>>          m_models;
};


#endif // SIM_LIB_MGR_H
