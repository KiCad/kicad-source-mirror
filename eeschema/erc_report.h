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

#ifndef ERC_REPORT_H
#define ERC_REPORT_H

#include <memory>
#include <eda_units.h>
#include <wx/string.h>

class SCHEMATIC;
class RC_ITEMS_PROVIDER;

class ERC_REPORT
{
public:
    ERC_REPORT( SCHEMATIC* aSchematic, EDA_UNITS aReportUnits );

    bool WriteTextReport( const wxString& aFullFileName );
    bool WriteJsonReport( const wxString& aFullFileName );

private:
    SCHEMATIC*                         m_sch;
    EDA_UNITS                          m_reportUnits;
};


#endif