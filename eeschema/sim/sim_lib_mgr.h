/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2022 Mikolaj Wielgus
 * Copyright (C) 2022 KiCad Developers, see AUTHORS.txt for contributors.
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
    SIM_LIB_MGR( const PROJECT& aPrj );
    virtual ~SIM_LIB_MGR() = default;

    void Clear();

    SIM_LIBRARY& CreateLibrary( const wxString& aLibraryPath, REPORTER* aReporter );

    SIM_MODEL& CreateModel( SIM_MODEL::TYPE aType, int aSymbolPinCount );

    SIM_MODEL& CreateModel( const SIM_MODEL& aBaseModel, int aSymbolPinCount );

    template <typename T>
    SIM_MODEL& CreateModel( const SIM_MODEL& aBaseModel, int aSymbolPinCount,
                            const std::vector<T>& aFields );

    // TODO: The argument can be made const.
    SIM_LIBRARY::MODEL CreateModel( const SCH_SHEET_PATH* aSheetPath, SCH_SYMBOL& aSymbol );

    template <typename T>
    SIM_LIBRARY::MODEL CreateModel( const std::vector<T>& aFields, int aSymbolPinCount );

    template <typename T>
    SIM_LIBRARY::MODEL CreateModel( const wxString& aLibraryPath, const std::string& aBaseModelName,
                                    const std::vector<T>& aFields, int aSymbolPinCount );

    void SetModel( int aIndex, std::unique_ptr<SIM_MODEL> aModel );

    std::map<wxString, std::reference_wrapper<const SIM_LIBRARY>> GetLibraries() const;
    std::vector<std::reference_wrapper<SIM_MODEL>> GetModels() const;

private:
    const PROJECT&                                   m_project;
    std::map<wxString, std::unique_ptr<SIM_LIBRARY>> m_libraries;
    std::vector<std::unique_ptr<SIM_MODEL>>          m_models;
};


#endif // SIM_LIB_MGR_H
