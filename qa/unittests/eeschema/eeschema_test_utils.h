/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2019-2022 KiCad Developers, see AUTHORS.TXT for contributors.
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


#include <schematic.h>
#include <settings/settings_manager.h>
#include <sch_io_mgr.h>
#include <wx/filename.h>

namespace KI_TEST
{
/**
 * Get the configured location of Eeschema test data.
 *
 * By default, this is the test data in the source tree, but can be overridden
 * by the KICAD_TEST_EESCHEMA_DATA_DIR environment variable.
 *
 * @return a filename referring to the test data dir to use.
 */
wxFileName GetEeschemaTestDataDir();

/**
 * A generic fixture for loading schematics and associated settings for qa tests
 */
class SCHEMATIC_TEST_FIXTURE
{
public:
    SCHEMATIC_TEST_FIXTURE() : m_schematic( nullptr ), m_manager( true )
    {
        m_pi = SCH_IO_MGR::FindPlugin( SCH_IO_MGR::SCH_KICAD );
    }

    virtual ~SCHEMATIC_TEST_FIXTURE()
    {
        m_schematic.Reset();
        delete m_pi;
    }

protected:
    virtual void loadSchematic( const wxString& aRelativePath );

    virtual wxFileName getSchematicFile( const wxString& aBaseName );

    ///> Schematic to load
    SCHEMATIC m_schematic;

    SCH_PLUGIN* m_pi;

    SETTINGS_MANAGER m_manager;
};


} // namespace KI_TEST

#endif // QA_EESCHEMA_EESCHEMA_TEST_UTILS__H