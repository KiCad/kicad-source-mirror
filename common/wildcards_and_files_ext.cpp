/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2018 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 2008 Wayne Stambaugh <stambaughw@gmail.com>
 * Copyright (C) 1992-2018 KiCad Developers, see AUTHORS.txt for contributors.
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

/**
 * @file wildcards_and_files_ext.cpp
 * Definition of file extensions used in Kicad.
 */
#include <wildcards_and_files_ext.h>


wxString formatWildcardExt( const wxString& aWildcard )
{
    wxString wc;
#if defined( __WXGTK__ )

    for( auto ch : aWildcard )
    {
        if( wxIsalpha( ch ) )
            wc += wxString::Format( "[%c%c]", wxTolower( ch ), wxToupper( ch ) );
        else
            wc += ch;
    }

    return wc;
#else
    wc = aWildcard;

    return wc;
#endif
}


wxString AddFileExtListToFilter( const std::vector<std::string>& aExts )
{
    if( aExts.size() == 0 )
    {
        // The "all files" wildcard is different on different systems
        wxString filter;
        filter << " (" << wxFileSelectorDefaultWildcardStr << ")|"
               << wxFileSelectorDefaultWildcardStr;
        return filter;
    }

    wxString files_filter = " (";

    // Add extensions to the info message:
    for( const auto& ext : aExts )
    {
        files_filter << " *." << ext;
    }

    files_filter << ")|*.";

    // Add extensions to the filter list, using a formated string (GTK specific):
    bool first = true;
    for( const auto& ext : aExts )
    {
        if( !first )
            files_filter << ";*.";

        first = false;

        files_filter << formatWildcardExt( ext );
    }

    return files_filter;
}


const std::string SchematicSymbolFileExtension( "sym" );
const std::string SchematicLibraryFileExtension( "lib" );

const std::string VrmlFileExtension( "wrl" );

const std::string ProjectFileExtension( "pro" );
const std::string SchematicFileExtension( "sch" );
const std::string NetlistFileExtension( "net" );
const std::string ComponentFileExtension( "cmp" );
const std::string GerberFileExtension( "gbr" );
const std::string GerberJobFileExtension( "gbrjob" );
const std::string HtmlFileExtension( "html" );
const std::string EquFileExtension( "equ" );

const std::string LegacyPcbFileExtension( "brd" );
const std::string KiCadPcbFileExtension( "kicad_pcb" );
const std::string PageLayoutDescrFileExtension( "kicad_wks" );

const std::string PdfFileExtension( "pdf" );
const std::string MacrosFileExtension( "mcr" );
const std::string DrillFileExtension( "drl" );
const std::string SVGFileExtension( "svg" );
const std::string ReportFileExtension( "rpt" );
const std::string FootprintPlaceFileExtension( "pos" );
const std::string KiCadLib3DShapesPathExtension( "3dshapes" );  ///< 3D shapes default libpath

const std::string KiCadFootprintLibPathExtension( "pretty" );   ///< KICAD PLUGIN libpath
const std::string LegacyFootprintLibPathExtension( "mod" );
const std::string EagleFootprintLibPathExtension( "lbr" );

const std::string KiCadFootprintFileExtension( "kicad_mod" );
const std::string GedaPcbFootprintLibFileExtension( "fp" );
const std::string SpecctraDsnFileExtension( "dsn" );
const std::string IpcD356FileExtension( "d356" );

const std::string PngFileExtension( "png" );
const std::string JpegFileExtension( "jpg" );


wxString AllFilesWildcard()
{
    return _( "All files" ) + AddFileExtListToFilter( {} );
}


wxString SchematicSymbolFileWildcard()
{
    return _( "KiCad drawing symbol files" ) + AddFileExtListToFilter( { "sym" } );
}


wxString SchematicLibraryFileWildcard()
{
    return _( "KiCad symbol library files" ) + AddFileExtListToFilter( { "lib" } );
}


wxString ProjectFileWildcard()
{
    return _( "KiCad project files" ) + AddFileExtListToFilter( { "pro" } );
}


wxString SchematicFileWildcard()
{
    return _( "KiCad schematic files" ) + AddFileExtListToFilter( { "sch" } );
}


wxString EagleSchematicFileWildcard()
{
    return _( "Eagle XML schematic files" ) + AddFileExtListToFilter( { "sch" } );
}


wxString EagleFilesWildcard()
{
    return _( "Eagle XML files" ) + AddFileExtListToFilter( { "sch", "brd" } );
}


wxString NetlistFileWildcard()
{
    return _( "KiCad netlist files" ) + AddFileExtListToFilter( { "net" } );
}


wxString GerberFileWildcard()
{
    return _( "Gerber files" ) + AddFileExtListToFilter( { "pho" } );
}


wxString LegacyPcbFileWildcard()
{
    return _( "KiCad printed circuit board files" ) + AddFileExtListToFilter( { "brd" } );
}


wxString EaglePcbFileWildcard()
{
    return _( "Eagle ver. 6.x XML PCB files" ) + AddFileExtListToFilter( { "brd" } );
}


wxString PCadPcbFileWildcard()
{
    return _( "P-Cad 200x ASCII PCB files" ) + AddFileExtListToFilter( { "pcb" } );
}


wxString PcbFileWildcard()
{
    return _( "KiCad printed circuit board files" ) + AddFileExtListToFilter( { "kicad_pcb" } );
}


wxString KiCadFootprintLibFileWildcard()
{
    return _( "KiCad footprint files" ) + AddFileExtListToFilter( { "kicad_mod" } );
}


wxString KiCadFootprintLibPathWildcard()
{
    return _( "KiCad footprint library paths" ) + AddFileExtListToFilter( { "pretty" } );
}


wxString LegacyFootprintLibPathWildcard()
{
    return _( "Legacy footprint library files" ) + AddFileExtListToFilter( { "mod" } );
}


wxString EagleFootprintLibPathWildcard()
{
    return _( "Eagle ver. 6.x XML library files" ) + AddFileExtListToFilter( { "lbr" } );
}


wxString GedaPcbFootprintLibFileWildcard()
{
    return _( "Geda PCB footprint library files" ) + AddFileExtListToFilter( { "fp" } );
}


wxString PageLayoutDescrFileWildcard()
{
    return _( "Page layout design files" ) + AddFileExtListToFilter( { "kicad_wks" } );
}


// Wildcard for cvpcb component to footprint link file
wxString ComponentFileWildcard()
{
    return _( "KiCad symbol footprint link files" ) + AddFileExtListToFilter( { "cmp" } );
}


// Wildcard for reports and fabrication documents
wxString DrillFileWildcard()
{
    return _( "Drill files" ) + AddFileExtListToFilter( { "drl", "nc", "xnc" } );
}


wxString SVGFileWildcard()
{
    return _( "SVG files" ) + AddFileExtListToFilter( { "svg" } );
}


wxString HtmlFileWildcard()
{
    return _( "HTML files" ) + AddFileExtListToFilter( { "htm", "html" } );
}


wxString CsvFileWildcard()
{
    return _( "CSV Files" ) + AddFileExtListToFilter( { "csv" } );
}


wxString PdfFileWildcard()
{
    return _( "Portable document format files" ) + AddFileExtListToFilter( { "pdf" } );
}


wxString PSFileWildcard()
{
    return _( "PostScript files" ) + AddFileExtListToFilter( { "ps" } );
}


wxString ReportFileWildcard()
{
    return _( "Report files" ) + AddFileExtListToFilter( { "rpt" } );
}


wxString FootprintPlaceFileWildcard()
{
    return _( "Footprint place files" ) + AddFileExtListToFilter( { "pos" } );
}


wxString Shapes3DFileWildcard()
{
    return _( "VRML and X3D files" ) + AddFileExtListToFilter( { "wrl", "x3d" } );
}


wxString IDF3DFileWildcard()
{
    return _( "IDFv3 footprint files" ) + AddFileExtListToFilter( { "idf" } );
}


wxString TextFileWildcard()
{
    return _( "Text files" ) + AddFileExtListToFilter( { "txt" } );
}


wxString ModLegacyExportFileWildcard()
{
    return _( "Legacy footprint export files" ) + AddFileExtListToFilter( { "emp" } );
}


wxString ErcFileWildcard()
{
    return _( "Electronic rule check file" ) + AddFileExtListToFilter( { "erc" } );
}


wxString SpiceLibraryFileWildcard()
{
    return _( "Spice library file" ) + AddFileExtListToFilter( { "lib" } );
}


wxString SpiceNetlistFileWildcard()
{
    return _( "SPICE netlist file" ) + AddFileExtListToFilter( { "cir" } );
}


wxString CadstarNetlistFileWildcard()
{
    return _( "CadStar netlist file" ) + AddFileExtListToFilter( { "frp" } );
}


wxString EquFileWildcard()
{
    return _( "Symbol footprint association files" ) + AddFileExtListToFilter( { "equ" } );
}


wxString ZipFileWildcard()
{
    return _( "Zip file" ) + AddFileExtListToFilter( { "zip" } );
}


wxString GencadFileWildcard()
{
    return _( "GenCAD 1.4 board files" ) + AddFileExtListToFilter( { "cad" } );
}


wxString DxfFileWildcard()
{
    return _( "DXF Files" ) + AddFileExtListToFilter( { "dxf" } );
}


wxString GerberJobFileWildcard()
{
    return _( "Gerber job file" ) + AddFileExtListToFilter( { "gbrjob" } );
}


wxString SpecctraDsnFileWildcard()
{
    return _( "Specctra DSN file" ) + AddFileExtListToFilter( { "dsn" } );
}


wxString IpcD356FileWildcard()
{
    return _( "IPC-D-356 Test Files" ) + AddFileExtListToFilter( { "d356" } );
}


wxString WorkbookFileWildcard()
{
    return _( "Workbook file" ) + AddFileExtListToFilter( { "wbk" } );
}


wxString PngFileWildcard()
{
    return _( "PNG file" ) + AddFileExtListToFilter( { "png" } );
}


wxString JpegFileWildcard()
{
    return _( "Jpeg file" ) + AddFileExtListToFilter( { "jpg", "jpeg" } );
}
