/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
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

#ifndef DRC_REPORT_H
#define DRC_REPORT_H

#include <memory>
#include <eda_units.h>
#include <wx/string.h>

class BOARD;
class RC_ITEMS_PROVIDER;

class DRC_REPORT
{
public:
    DRC_REPORT( BOARD* aBoard,
                EDA_UNITS aReportUnits,
                std::shared_ptr<RC_ITEMS_PROVIDER> aMarkersProvider,
                std::shared_ptr<RC_ITEMS_PROVIDER> aRatsnestProvider,
                std::shared_ptr<RC_ITEMS_PROVIDER> aFpWarningsProvider );

    bool WriteTextReport( const wxString& aFullFileName );
    bool WriteJsonReport( const wxString& aFullFileName );

private:
    BOARD*                             m_board;
    EDA_UNITS                          m_reportUnits;
    std::shared_ptr<RC_ITEMS_PROVIDER> m_markersProvider;
    std::shared_ptr<RC_ITEMS_PROVIDER> m_ratsnestProvider;
    std::shared_ptr<RC_ITEMS_PROVIDER> m_fpWarningsProvider;
    int                                m_reportedSeverities;
};


#endif