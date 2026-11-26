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
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include <api/api_sch_annotation.h>

#include <wx/intl.h>

#include <erc/erc.h>
#include <kiway.h>
#include <netlist_exporters/netlist_exporter_kicad.h>
#include <reporter.h>
#include <richio.h>
#include <sch_reference_list.h>
#include <schematic.h>


bool CheckSchematicReadyForNetlist( SCHEMATIC& aSchematic, REPORTER& aReporter )
{
    const int annotationErrors = aSchematic.CheckAnnotateAll(
            [&aReporter]( ERCE_T, const wxString& aMsg, SCH_REFERENCE*, SCH_REFERENCE* )
            {
                aReporter.Report( aMsg, RPT_SEVERITY_ERROR );
            } );

    if( annotationErrors > 0 )
        return false;

    ERC_TESTER ercTester( &aSchematic );

    if( ercTester.TestDuplicateSheetNames( false ) > 0 )
    {
        aReporter.Report( _( "Duplicate sheet names found." ), RPT_SEVERITY_ERROR );
        return false;
    }

    return true;
}


void FormatSchematicNetlist( SCHEMATIC& aSchematic, KIWAY* aKiway, std::string& aNetlist )
{
    NETLIST_EXPORTER_KICAD exporter( &aSchematic, aKiway );
    STRING_FORMATTER       formatter;

    exporter.Format( &formatter, GNL_ALL | GNL_OPT_KICAD );

    aNetlist = formatter.GetString();
}


bool GenerateSchematicNetlist( SCHEMATIC& aSchematic, KIWAY* aKiway, std::string& aNetlist, REPORTER& aReporter )
{
    if( !CheckSchematicReadyForNetlist( aSchematic, aReporter ) )
        return false;

    aSchematic.RebuildConnectivity();
    FormatSchematicNetlist( aSchematic, aKiway, aNetlist );

    return true;
}
