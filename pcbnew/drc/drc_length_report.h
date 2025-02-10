/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers.
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

#ifndef __DRC_LENGTH_REPORT_H
#define __DRC_LENGTH_REPORT_H

#include <set>

class DRC_RULE;

class DRC_LENGTH_REPORT
{
public:
    struct ENTRY
    {
        int                             netcode;
        wxString                        netname;
        NETINFO_ITEM*                   netinfo;
        BOARD_CONNECTED_ITEM*           fromItem;
        BOARD_CONNECTED_ITEM*           toItem;
        DRC_RULE*                       matchingRule;
        wxString                        from;
        wxString                        to;
        std::set<BOARD_CONNECTED_ITEM*> items;
        int                             viaCount;
        double                          totalRoute;
        double                          totalRouteDelay;
        int                             totalVia;
        int64_t                         totalViaDelay;
        int                             totalPadToDie;
        int64_t                         totalPadToDieDelay;
        double                          total;
        double                          totalDelay;
    };

    DRC_LENGTH_REPORT()
    {
    }

    ~DRC_LENGTH_REPORT()
    {
    }

    void Clear()
    {
        m_report.clear();
    }

    void Add( const ENTRY& ent )
    {
        m_report.push_back( ent );
    }

    const std::vector<ENTRY>& GetEntries() const
    {
        return m_report;
    }

private:
    std::vector<ENTRY> m_report;
};

#endif