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

#ifndef SIM_LIBRARY_SPICE_H
#define SIM_LIBRARY_SPICE_H

#include <sim/sim_library.h>
#include <sim/spice_library_parser.h>


class SIM_LIBRARY_SPICE : public SIM_LIBRARY
{
public:
    friend class SPICE_LIBRARY_PARSER;

    SIM_LIBRARY_SPICE( bool aForceFullParse );

    // @copydoc SIM_LIBRARY::ReadFile()
    void ReadFile( const wxString& aFilePath, REPORTER& aReporter ) override;

private:
    std::unique_ptr<SPICE_LIBRARY_PARSER> m_spiceLibraryParser;
};

#endif // SIM_LIBRARY_SPICE_H
