/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2018 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 2008 Wayne Stambaugh <stambaughw@gmail.com>
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
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
#include <regex>
#include <wildcards_and_files_ext.h>
#include <wx/filedlg.h>
#include <wx/regex.h>
#include <wx/translation.h>
#include <regex>

bool compareFileExtensions( const std::string& aExtension,
                            const std::vector<std::string>& aReference, bool aCaseSensitive )
{
    // Form the regular expression string by placing all possible extensions into it as alternatives
    std::string regexString = "(";
    bool        first = true;

    for( const std::string& ext : aReference )
    {
        // The | separate goes between the extensions
        if( !first )
            regexString += "|";
        else
            first = false;

        regexString += ext;
    }

    regexString += ")";

    // Create the regex and see if it matches
    std::regex extRegex( regexString, aCaseSensitive ? std::regex::ECMAScript : std::regex::icase );
    return std::regex_match( aExtension, extRegex );
}


wxString formatWildcardExt( const wxString& aWildcard )
{
    wxString wc;
#if defined( __WXGTK__ )

    for( const auto& ch : aWildcard )
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
        filter << wxS( " (" ) << wxFileSelectorDefaultWildcardStr << wxS( ")|" )
               << wxFileSelectorDefaultWildcardStr;
        return filter;
    }

    wxString files_filter = wxS( " (" );

    // Add extensions to the info message:
    for( const std::string& ext : aExts )
    {
        if( files_filter.length() > 2 )
            files_filter << wxS( "; " );

        files_filter << "*." << ext;
    }

    files_filter << wxS( ")|*." );

    // Add extensions to the filter list, using a formatted string (GTK specific):
    bool first = true;

    for( const std::string& ext : aExts )
    {
        if( !first )
            files_filter << wxS( ";*." );

        first = false;

        files_filter << formatWildcardExt( ext );
    }

    return files_filter;
}

const std::string FILEEXT::BackupFileSuffix( "-bak" );
const std::string FILEEXT::LockFilePrefix( "~" );
const std::string FILEEXT::LockFileExtension( "lck" );

const std::string FILEEXT::KiCadSymbolLibFileExtension( "kicad_sym" );
const std::string FILEEXT::SchematicSymbolFileExtension( "sym" );
const std::string FILEEXT::LegacySymbolLibFileExtension( "lib" );
const std::string FILEEXT::LegacySymbolDocumentFileExtension( "dcm" );

const std::string FILEEXT::VrmlFileExtension( "wrl" );

const std::string FILEEXT::ProjectFileExtension( "kicad_pro" );
const std::string FILEEXT::LegacyProjectFileExtension( "pro" );
const std::string FILEEXT::ProjectLocalSettingsFileExtension( "kicad_prl" );
const std::string FILEEXT::LegacySchematicFileExtension( "sch" );
const std::string FILEEXT::CadstarSchematicFileExtension( "csa" );
const std::string FILEEXT::CadstarPartsLibraryFileExtension( "lib" );
const std::string FILEEXT::KiCadSchematicFileExtension( "kicad_sch" );
const std::string FILEEXT::SpiceFileExtension( "cir" );
const std::string FILEEXT::SpiceModelFileExtension( "model" );
const std::string FILEEXT::SpiceSubcircuitFileExtension( "sub" );
const std::string FILEEXT::IbisFileExtension( "ibs" );
const std::string FILEEXT::CadstarNetlistFileExtension( "frp" );
const std::string FILEEXT::OrCadPcb2NetlistFileExtension( "net" );
const std::string FILEEXT::NetlistFileExtension( "net" );
const std::string FILEEXT::AllegroNetlistFileExtension( "txt" );
const std::string FILEEXT::PADSNetlistFileExtension( "asc" );
const std::string FILEEXT::FootprintAssignmentFileExtension( "cmp" );
const std::string FILEEXT::GerberFileExtension( "gbr" );
const std::string FILEEXT::GerberJobFileExtension( "gbrjob" );
const std::string FILEEXT::HtmlFileExtension( "html" );
const std::string FILEEXT::EquFileExtension( "equ" );
const std::string FILEEXT::HotkeyFileExtension( "hotkeys" );
const std::string FILEEXT::DatabaseLibraryFileExtension( "kicad_dbl" );
const std::string FILEEXT::HTTPLibraryFileExtension( "kicad_httplib" );
const std::string FILEEXT::KiCadJobSetFileExtension( "kicad_jobset" );

const std::string FILEEXT::ArchiveFileExtension( "zip" );

const std::string FILEEXT::LegacyPcbFileExtension( "brd" );
const std::string FILEEXT::EaglePcbFileExtension( "brd" );
const std::string FILEEXT::CadstarPcbFileExtension( "cpa" );
const std::string FILEEXT::KiCadPcbFileExtension( "kicad_pcb" );
const std::string FILEEXT::DrawingSheetFileExtension( "kicad_wks" );
const std::string FILEEXT::DesignRulesFileExtension( "kicad_dru" );

const std::string FILEEXT::PdfFileExtension( "pdf" );
const std::string FILEEXT::MacrosFileExtension( "mcr" );
const std::string FILEEXT::DrillFileExtension( "drl" );
const std::string FILEEXT::SVGFileExtension( "svg" );
const std::string FILEEXT::ReportFileExtension( "rpt" );
const std::string FILEEXT::FootprintPlaceFileExtension( "pos" );

const std::string FILEEXT::KiCadFootprintLibPathExtension( "pretty" ); // this is a directory
const std::string FILEEXT::LegacyFootprintLibPathExtension( "mod" );   // this is a file
const std::string FILEEXT::AltiumFootprintLibPathExtension( "PcbLib" ); // this is a file
const std::string FILEEXT::CadstarFootprintLibPathExtension( "cpa" );   // this is a file
const std::string FILEEXT::EagleFootprintLibPathExtension( "lbr" );     // this is a file
const std::string FILEEXT::GedaPcbFootprintLibFileExtension( "fp" );    // this is a file

const std::string FILEEXT::KiCadFootprintFileExtension( "kicad_mod" );
const std::string FILEEXT::SpecctraDsnFileExtension( "dsn" );
const std::string FILEEXT::SpecctraSessionFileExtension( "ses" );
const std::string FILEEXT::IpcD356FileExtension( "d356" );
const std::string FILEEXT::Ipc2581FileExtension( "xml" );
const std::string FILEEXT::WorkbookFileExtension( "wbk" );

const std::string FILEEXT::KiCadDesignBlockLibPathExtension( "kicad_blocks" ); // this is a directory
const std::string FILEEXT::KiCadDesignBlockPathExtension( "kicad_block" );     // this is a directory

const std::string FILEEXT::PngFileExtension( "png" );
const std::string FILEEXT::JpegFileExtension( "jpg" );
const std::string FILEEXT::TextFileExtension( "txt" );
const std::string FILEEXT::MarkdownFileExtension( "md" );
const std::string FILEEXT::CsvFileExtension( "csv" );
const std::string FILEEXT::TsvFileExtension( "tsv" );
const std::string FILEEXT::XmlFileExtension( "xml" );
const std::string FILEEXT::JsonFileExtension( "json" );
const std::string FILEEXT::PythonFileExtension( "py" );

const std::string FILEEXT::StepFileExtension( "step" );
const std::string FILEEXT::StepZFileAbrvExtension( "stpz" );
const std::string FILEEXT::StepFileAbrvExtension( "stp" );
const std::string FILEEXT::GltfBinaryFileExtension( "glb" );
const std::string FILEEXT::BrepFileExtension( "brep" );
const std::string FILEEXT::XaoFileExtension( "xao" );
const std::string FILEEXT::PlyFileExtension( "ply" );
const std::string FILEEXT::StlFileExtension( "stl" );
const std::string FILEEXT::U3DFileExtension( "u3d" );

const std::string FILEEXT::GencadFileExtension( "cad" );

const wxString FILEEXT::GerberFileExtensionsRegex( "(gbr|gko|pho|(g[tb][alops])|(gm?\\d\\d*)|(gp[tb]))" );

const std::string FILEEXT::FootprintLibraryTableFileName( "fp-lib-table" );
const std::string FILEEXT::SymbolLibraryTableFileName( "sym-lib-table" );
const std::string FILEEXT::DesignBlockLibraryTableFileName( "design-block-lib-table" );

const std::string FILEEXT::KiCadUriPrefix( "kicad-embed" );


bool FILEEXT::IsGerberFileExtension( const wxString& ext )
{
    static wxRegEx gerberRE( GerberFileExtensionsRegex, wxRE_ICASE );

    return gerberRE.Matches( ext );
}


wxString FILEEXT::AllFilesWildcard()
{
    return _( "All files" ) + AddFileExtListToFilter( {} );
}


wxString FILEEXT::KiCadSymbolLibFileWildcard()
{
    return _( "KiCad symbol library files" )
            + AddFileExtListToFilter( { KiCadSymbolLibFileExtension } );
}


wxString FILEEXT::ProjectFileWildcard()
{
    return _( "KiCad project files" ) + AddFileExtListToFilter( { ProjectFileExtension } );
}


wxString FILEEXT::LegacyProjectFileWildcard()
{
    return _( "KiCad legacy project files" )
            + AddFileExtListToFilter( { LegacyProjectFileExtension } );
}


wxString FILEEXT::AllProjectFilesWildcard()
{
    return _( "All KiCad project files" )
            + AddFileExtListToFilter( { ProjectFileExtension, LegacyProjectFileExtension } );
}


wxString FILEEXT::AllSchematicFilesWildcard()
{
    return _( "All KiCad schematic files" )
            + AddFileExtListToFilter( { KiCadSchematicFileExtension, LegacySchematicFileExtension } );
}


wxString FILEEXT::LegacySchematicFileWildcard()
{
    return _( "KiCad legacy schematic files" )
            + AddFileExtListToFilter( { LegacySchematicFileExtension } );
}


wxString FILEEXT::KiCadSchematicFileWildcard()
{
    return _( "KiCad s-expression schematic files" )
            + AddFileExtListToFilter( { KiCadSchematicFileExtension } );
}


wxString FILEEXT::AltiumProjectFilesWildcard()
{
    return _( "Altium Project files" ) + AddFileExtListToFilter( { "PrjPcb" } );
}


wxString FILEEXT::CadstarArchiveFilesWildcard()
{
    return _( "CADSTAR Archive files" ) + AddFileExtListToFilter( { "csa", "cpa" } );
}


wxString FILEEXT::EagleFilesWildcard()
{
    return _( "Eagle XML files" ) + AddFileExtListToFilter( { "sch", "brd" } );
}


wxString FILEEXT::OrCadPcb2NetlistFileWildcard()
{
    return _( "OrcadPCB2 netlist files" )
            + AddFileExtListToFilter( { OrCadPcb2NetlistFileExtension } );
}


wxString FILEEXT::NetlistFileWildcard()
{
    return _( "KiCad netlist files" ) + AddFileExtListToFilter( { "net" } );
}


wxString FILEEXT::AllegroNetlistFileWildcard()
{
    return _( "Allegro netlist files" )
            + AddFileExtListToFilter( { AllegroNetlistFileExtension } );
}


wxString FILEEXT::PADSNetlistFileWildcard()
{
    return _( "PADS netlist files" ) + AddFileExtListToFilter( { PADSNetlistFileExtension } );
}


wxString FILEEXT::EasyEdaArchiveWildcard()
{
    return _( "EasyEDA (JLCEDA) Std backup archive" ) + AddFileExtListToFilter( { "zip" } );
}


wxString FILEEXT::EasyEdaProFileWildcard()
{
    return _( "EasyEDA (JLCEDA) Pro files" ) + AddFileExtListToFilter( { "epro", "zip" } );
}


wxString FILEEXT::PcbFileWildcard()
{
    return _( "KiCad printed circuit board files" )
           + AddFileExtListToFilter( { KiCadPcbFileExtension } );
}


wxString FILEEXT::KiCadFootprintLibFileWildcard()
{
    return _( "KiCad footprint files" ) + AddFileExtListToFilter( { KiCadFootprintFileExtension } );
}


wxString FILEEXT::KiCadFootprintLibPathWildcard()
{
    return _( "KiCad footprint library paths" )
            + AddFileExtListToFilter( { KiCadFootprintLibPathExtension } );
}


wxString FILEEXT::KiCadDesignBlockPathWildcard()
{
    return _( "KiCad design block path" )
           + AddFileExtListToFilter( { KiCadDesignBlockPathExtension } );
}


wxString FILEEXT::KiCadDesignBlockLibPathWildcard()
{
    return _( "KiCad design block library paths" )
           + AddFileExtListToFilter( { KiCadDesignBlockLibPathExtension } );
}


wxString FILEEXT::DrawingSheetFileWildcard()
{
    return _( "Drawing sheet files" )
            + AddFileExtListToFilter( { DrawingSheetFileExtension } );
}


// Wildcard for cvpcb symbol to footprint link file
wxString FILEEXT::FootprintAssignmentFileWildcard()
{
    return _( "KiCad symbol footprint link files" )
            + AddFileExtListToFilter( { FootprintAssignmentFileExtension } );
}


// Wildcard for reports and fabrication documents
wxString FILEEXT::DrillFileWildcard()
{
    return _( "Drill files" )
            + AddFileExtListToFilter( { DrillFileExtension, "nc", "xnc", "txt" } );
}


wxString FILEEXT::SVGFileWildcard()
{
    return _( "SVG files" ) + AddFileExtListToFilter( { SVGFileExtension } );
}


wxString FILEEXT::HtmlFileWildcard()
{
    return _( "HTML files" ) + AddFileExtListToFilter( { "htm", "html" } );
}


wxString FILEEXT::CsvFileWildcard()
{
    return _( "CSV Files" ) + AddFileExtListToFilter( { CsvFileExtension } );
}


wxString FILEEXT::CsvTsvFileWildcard()
{
    return _( "CSV/TSV Files" ) + AddFileExtListToFilter( { CsvFileExtension, TsvFileExtension } );
}


wxString FILEEXT::PdfFileWildcard()
{
    return _( "Portable document format files" ) + AddFileExtListToFilter( { "pdf" } );
}


wxString FILEEXT::PSFileWildcard()
{
    return _( "PostScript files" ) + AddFileExtListToFilter( { "ps" } );
}


wxString FILEEXT::JsonFileWildcard()
{
    return _( "Json files" ) + AddFileExtListToFilter( { JsonFileExtension } );
}


wxString FILEEXT::ReportFileWildcard()
{
    return _( "Report files" ) + AddFileExtListToFilter( { ReportFileExtension } );
}


wxString FILEEXT::FootprintPlaceFileWildcard()
{
    return _( "Component placement files" ) + AddFileExtListToFilter( { "pos" } );
}


wxString FILEEXT::Shapes3DFileWildcard()
{
    return _( "VRML and X3D files" ) + AddFileExtListToFilter( { "wrl", "x3d" } );
}


wxString FILEEXT::IDF3DFileWildcard()
{
    return _( "IDFv3 footprint files" ) + AddFileExtListToFilter( { "idf" } );
}


wxString FILEEXT::TextFileWildcard()
{
    return _( "Text files" ) + AddFileExtListToFilter( { "txt" } );
}


wxString FILEEXT::ModLegacyExportFileWildcard()
{
    return _( "Legacy footprint export files" ) + AddFileExtListToFilter( { "emp" } );
}


wxString FILEEXT::ErcFileWildcard()
{
    return _( "Electrical rule check file" ) + AddFileExtListToFilter( { "erc" } );
}


wxString FILEEXT::SpiceLibraryFileWildcard()
{
    return _( "SPICE library file" ) + AddFileExtListToFilter( { "lib", "mod" } );
}


wxString FILEEXT::SpiceNetlistFileWildcard()
{
    return _( "SPICE netlist file" ) + AddFileExtListToFilter( { "cir" } );
}


wxString FILEEXT::CadstarNetlistFileWildcard()
{
    return _( "CadStar netlist file" ) + AddFileExtListToFilter( { "frp" } );
}


wxString FILEEXT::EquFileWildcard()
{
    return _( "Symbol footprint association files" ) + AddFileExtListToFilter( { "equ" } );
}


wxString FILEEXT::ZipFileWildcard()
{
    return _( "Zip file" ) + AddFileExtListToFilter( { "zip" } );
}


wxString FILEEXT::GencadFileWildcard()
{
    return _( "GenCAD 1.4 board files" ) + AddFileExtListToFilter( { GencadFileExtension } );
}


wxString FILEEXT::DxfFileWildcard()
{
    return _( "DXF Files" ) + AddFileExtListToFilter( { "dxf" } );
}


wxString FILEEXT::GerberJobFileWildcard()
{
    return _( "Gerber job file" ) + AddFileExtListToFilter( { GerberJobFileExtension } );
}


wxString FILEEXT::SpecctraDsnFileWildcard()
{
    return _( "Specctra DSN file" )
            + AddFileExtListToFilter( { SpecctraDsnFileExtension } );
}


wxString FILEEXT::SpecctraSessionFileWildcard()
{
    return _( "Specctra Session file" )
            + AddFileExtListToFilter( { SpecctraSessionFileExtension } );
}


wxString FILEEXT::IpcD356FileWildcard()
{
    return _( "IPC-D-356 Test Files" )
            + AddFileExtListToFilter( { IpcD356FileExtension } );
}


wxString FILEEXT::WorkbookFileWildcard()
{
    return _( "Workbook file" )
            + AddFileExtListToFilter( { WorkbookFileExtension } );
}


wxString FILEEXT::PngFileWildcard()
{
    return _( "PNG file" ) + AddFileExtListToFilter( { "png" } );
}


wxString FILEEXT::JpegFileWildcard()
{
    return _( "Jpeg file" ) + AddFileExtListToFilter( { "jpg", "jpeg" } );
}


wxString FILEEXT::HotkeyFileWildcard()
{
    return _( "Hotkey file" ) + AddFileExtListToFilter( { HotkeyFileExtension } );
}


wxString FILEEXT::JobsetFileWildcard()
{
    return _( "KiCad jobset files" ) + AddFileExtListToFilter( { KiCadJobSetFileExtension } );
}
