/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef SIM_LIBRARY_IBIS_H
#define SIM_LIBRARY_IBIS_H

#include <sim/kibis/kibis.h>
#include <sim/sim_library.h>
#include <sim/sim_model_ibis.h>

class SIM_LIBRARY_IBIS : public SIM_LIBRARY
{
    friend class SIM_MODEL_IBIS;

public:
    static constexpr auto PIN_FIELD = "Sim.Ibis.Pin";
    static constexpr auto MODEL_FIELD = "Sim.Ibis.Model";
    static constexpr auto DIFF_FIELD = "Sim.Ibis.Diff";

    // @copydoc SIM_LIBRARY::ReadFile()
    void ReadFile( const wxString& aFilePath, REPORTER& aReporter ) override;

    bool InitModel( SIM_MODEL_IBIS& aModel, wxString aCompName );
    bool isPinDiff( const std::string& aComp, const std::string& aPinNumber ) const;

protected:
    mutable KIBIS m_kibis;
    std::vector<std::pair<std::string, std::string>> m_diffPins;
};

#endif // SIM_LIBRARY_IBIS_H
