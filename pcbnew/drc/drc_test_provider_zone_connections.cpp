/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2021 KiCad Developers.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, you may find one here:
 * http://www.gnu.org/licenses/old-licenses/gpl-2.0.html
 * or you may search the http://www.gnu.org website for the version 2 license,
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

#include <drc/drc_rule.h>
#include <drc/drc_test_provider.h>


/*
    This doesn't actually run any tests; it just loads the various zone connectionrules for
    the ZONE_FILLER.
*/

class DRC_TEST_PROVIDER_ZONE_CONNECTIONS : public DRC_TEST_PROVIDER
{
public:
    DRC_TEST_PROVIDER_ZONE_CONNECTIONS()
    {
    }

    virtual ~DRC_TEST_PROVIDER_ZONE_CONNECTIONS()
    {
    }

    virtual bool Run() override
    {
        return true;
    }

    virtual const wxString GetName() const override
    {
        return "zone connections";
    };

    virtual const wxString GetDescription() const override
    {
        return "Compiles zone connection rules for the zone filler";
    }

    virtual std::set<DRC_CONSTRAINT_T> GetConstraintTypes() const override
    {
        return { ZONE_CONNECTION_CONSTRAINT, THERMAL_RELIEF_GAP_CONSTRAINT,
                 THERMAL_SPOKE_WIDTH_CONSTRAINT };
    }
};


namespace detail
{
static DRC_REGISTER_TEST_PROVIDER<DRC_TEST_PROVIDER_ZONE_CONNECTIONS> dummy;
}
