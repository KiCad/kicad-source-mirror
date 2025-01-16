/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.TXT for contributors.
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

#ifndef QA_EESCHEMA_EESCHEMA_TEST_UTILS__H
#define QA_EESCHEMA_EESCHEMA_TEST_UTILS__H

#include <memory>
#include <schematic.h>
#include <settings/settings_manager.h>
#include <sch_io/sch_io_mgr.h>
#include <wx/filename.h>

#include <connection_graph.h>
#include <netlist_exporter_kicad.h>
#include <netlist_exporter_spice.h>
#include <netlist_reader/netlist_reader.h>
#include <netlist_reader/pcb_netlist.h>
#include <project.h>
#include <sch_sheet.h>
#include <schematic.h>
#include <settings/settings_manager.h>
#include <wildcards_and_files_ext.h>

namespace KI_TEST
{
/**
 * A generic fixture for loading schematics and associated settings for qa tests
 */
class SCHEMATIC_TEST_FIXTURE
{
public:
    SCHEMATIC_TEST_FIXTURE();

    ~SCHEMATIC_TEST_FIXTURE();

protected:
    virtual void LoadSchematic( const wxFileName& aFn );
    virtual wxFileName SchematicQAPath( const wxString& aBaseName );
    SETTINGS_MANAGER& SettingsManager() { return m_manager; }

    std::unique_ptr<SCHEMATIC>    m_schematic;
    IO_RELEASER<SCH_IO> m_pi;
    SETTINGS_MANAGER    m_manager;
};


} // namespace KI_TEST


template <typename Exporter>
class TEST_NETLIST_EXPORTER_FIXTURE : public KI_TEST::SCHEMATIC_TEST_FIXTURE
{
public:
    virtual wxString GetNetlistPath( bool aTest = false );
    virtual unsigned GetNetlistOptions() { return 0; }

    void WriteNetlist();

    virtual void CompareNetlists() = 0;

    void Cleanup();

    void TestNetlist( const wxString& aBaseName );
};

#endif // QA_EESCHEMA_EESCHEMA_TEST_UTILS__H
