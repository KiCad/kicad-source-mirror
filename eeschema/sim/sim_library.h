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

#ifndef SIM_LIBRARY_H
#define SIM_LIBRARY_H

#include <sim/sim_model.h>


class SIM_LIBRARY
{
public:
    static constexpr auto LIBRARY_FIELD = "Sim_Library";
    static constexpr auto NAME_FIELD = "Sim_Name";

    virtual ~SIM_LIBRARY() = default;
    SIM_LIBRARY() = default;

    /**
     * Read library from a source file (e.g. in Spice format), and return a newly constructed
     * object of an appropriate subclass.
     *
     * @param aFilePath Path to the file.
     * @return The library loaded in a newly constructed object.
     */
    static std::unique_ptr<SIM_LIBRARY> Create( wxString aFilePath );

    /**
     * Read library from a source file. Must be in the format appropriate to the subclass, e.g.
     * Spice for SIM_LIBRARY_SPICE).
     *
     * @param aFilePath Path to the file.
     * @throw IO_ERROR on read or parsing error.
     */
    virtual void ReadFile( const wxString& aFilePath ) = 0;

    /**
     * Write library to a source file (e.g. in Spice format).
     *
     * @param aFilePath Path to the file.
     * @throw IO_ERROR on write error.
     */
    virtual void WriteFile( const wxString& aFilePath ) = 0;

    SIM_MODEL* FindModel( const wxString& aModelName ) const;

    std::vector<std::reference_wrapper<SIM_MODEL>> GetModels() const;
    const std::vector<wxString>& GetModelNames() const { return m_modelNames; }

    wxString GetFilePath() const { return m_filePath; }
    wxString GetError() const { return m_error; }
    
protected:
    std::vector<std::unique_ptr<SIM_MODEL>> m_models;
    std::vector<wxString> m_modelNames;

    wxString m_filePath;
    wxString m_error;
};



#endif // SIM_LIBRARY_H
