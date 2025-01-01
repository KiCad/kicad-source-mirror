/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2022 Mikolaj Wielgus
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
 * along with this program; if not, you may find one here:
 * https://www.gnu.org/licenses/gpl-3.0.html
 * or you may search the http://www.gnu.org website for the version 3 license,
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

#ifndef SIM_LIBRARY_H
#define SIM_LIBRARY_H

#include <sim/sim_model.h>
#include <reporter.h>


class SIM_LIBRARY
{
public:
    static constexpr auto LIBRARY_FIELD = "Sim.Library";
    static constexpr auto NAME_FIELD = "Sim.Name";

    struct MODEL
    {
        std::string name;
        SIM_MODEL&  model;
    };

    virtual ~SIM_LIBRARY() = default;
    SIM_LIBRARY() = default;

    /**
     * Read library from a source file (e.g. in Spice format), and return a newly constructed
     * object of an appropriate subclass.
     *
     * @param aFilePath Path to the file.
     * @param aReporter The reporter the library reports to
     * @param aForceFullParse Caller requires fully parsed models.  If false fallback models can
     *                        be generarted for performance.
     * @return The library loaded in a newly constructed object.
     */
    static std::unique_ptr<SIM_LIBRARY>
    Create( const wxString& aFilePath, bool aForceFullParse, REPORTER& aReporter,
            const std::function<wxString( const wxString&, const wxString& )>& aResolver );

    /**
     * Read library from a source file. Must be in the format appropriate to the subclass, e.g.
     * Spice for SIM_LIBRARY_SPICE).
     *
     * @param aFilePath Path to the file.
     * @throw IO_ERROR on read or parsing error.
     */
    virtual void ReadFile( const wxString& aFilePath, REPORTER& aReporter ) = 0;

    SIM_MODEL* FindModel( const std::string& aModelName ) const;

    std::vector<MODEL> GetModels() const;

    std::string GetFilePath() const { return m_filePath; }

protected:
    std::vector<std::string>                m_modelNames;
    std::vector<std::unique_ptr<SIM_MODEL>> m_models;

    std::function<wxString( const wxString&, const wxString& )> m_pathResolver;

    std::string m_filePath;
};



#endif // SIM_LIBRARY_H
