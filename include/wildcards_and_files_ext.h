/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2018 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 2007-2012 SoftPLC Corporation, Dick Hollenbeck <dick@softplc.com>
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
 * @file wildcards_and_files_ext.h
 * Definition of file extensions used in Kicad.
 */

#ifndef INCLUDE_WILDCARDS_AND_FILES_EXT_H_
#define INCLUDE_WILDCARDS_AND_FILES_EXT_H_

#include <kicommon.h>
#include <string>
#include <vector>
#include <wx/string.h>

/**
 * Compare the given extension against the reference extensions to see if it matches any
 * of the reference extensions.
 *
 * This function uses the C++ regular expression functionality to perform the comparison,
 * so the reference extensions can be regular expressions of their own right. This means
 * that partial searches can be made, for example ^g.* can be used to see if the first
 * character of the extension is g. The reference extensions are concatenated together
 * as alternatives when doing the evaluation (e.g. (dxf|svg|^g.*) ).
 *
 * @param aExtension is the extension to test
 * @param aReference is a vector containing the extensions to test against
 * @param aCaseSensitive says if the comparison should be case sensitive or not
 *
 * @return if the extension matches any reference extensions
 */
KICOMMON_API bool compareFileExtensions( const std::string&              aExtension,
                                         const std::vector<std::string>& aReference, bool aCaseSensitive = false );

/**
 * Build the wildcard extension file dialog wildcard filter to add to the base message dialog.
 *
 * For instance, to open .txt files in a file dialog:
 * the base message is for instance "Text files"
 * the ext list is " (*.txt)|*.txt"
 * and the returned string to add to the base message is " (*.txt)|*.txt"
 * the message to display in the dialog is  "Text files (*.txt)|*.txt"
 *
 * This function produces a case-insensitive filter (so .txt, .TXT and .tXT
 * are all match if you pass "txt" into the function).
 *
 * @param aExts is the list of exts to add to the filter. Do not include the
 * leading dot. Empty means "allow all files".
 *
 * @return the appropriate file dialog wildcard filter list.
 */

KICOMMON_API wxString AddFileExtListToFilter( const std::vector<std::string>& aExts );

/**
 * Format wildcard extension to support case sensitive file dialogs.
 *
 * The file extension wildcards of the GTK+ file dialog are case sensitive so using all lower
 * case characters means that only file extensions that are all lower case will show up in the
 * file dialog.  The GTK+ file dialog does support regular expressions so the file extension
 * is converted to a regular expression ( sch -> [sS][cC][hH] ) when wxWidgets is built against
 * GTK+.  Please make sure you call this function when adding new file wildcards.
 *
 * @note When calling wxFileDialog with a default file defined, make sure you include the
 *       file extension along with the file name.  Otherwise, on GTK+ builds, the file
 *       dialog will append the wildcard regular expression as the file extension which is
 *       surely not what you want.
 *
 * @param aWildcard is the extension part of the wild card.
 *
 * @return the build appropriate file dialog wildcard filter.
 */
KICOMMON_API wxString formatWildcardExt( const wxString& aWildcard );


class KICOMMON_API FILEEXT
{
public:
    FILEEXT() = delete;

    /**
     * \defgroup file_extensions File Extension Definitions
     *
     * @note Please do not changes these.  If a different file extension is needed, create a new
     *       definition in here.  If you create a extension definition in another file, make sure
     *       to add it to the Doxygen group "file_extensions" using the "addtogroup" tag. Also
     *       note, just because they are defined as const doesn't guarantee that they cannot be
     *       changed.
     *
     * @{
     */
    static const std::string BackupFileSuffix;
    static const std::string LockFilePrefix;
    static const std::string LockFileExtension;

    static const std::string SchematicSymbolFileExtension;
    static const std::string LegacySymbolLibFileExtension;
    static const std::string LegacySymbolDocumentFileExtension;
    static const std::string SchematicBackupFileExtension;

    static const std::string VrmlFileExtension;
    static const std::string ProjectFileExtension;
    static const std::string LegacyProjectFileExtension;
    static const std::string ProjectLocalSettingsFileExtension;
    static const std::string LegacySchematicFileExtension;
    static const std::string CadstarSchematicFileExtension;
    static const std::string CadstarPartsLibraryFileExtension;
    static const std::string KiCadSchematicFileExtension;
    static const std::string IbisFileExtension;
    static const std::string SpiceFileExtension;
    static const std::string SpiceModelFileExtension;
    static const std::string SpiceSubcircuitFileExtension;
    static const std::string CadstarNetlistFileExtension;
    static const std::string OrCadPcb2NetlistFileExtension;
    static const std::string NetlistFileExtension;
    static const std::string AllegroNetlistFileExtension;
    static const std::string PADSNetlistFileExtension;
    static const std::string GerberFileExtension;
    static const std::string GerberJobFileExtension;
    static const std::string HtmlFileExtension;
    static const std::string EquFileExtension;
    static const std::string HotkeyFileExtension;
    static const std::string DatabaseLibraryFileExtension;
    static const std::string HTTPLibraryFileExtension;

    static const std::string ArchiveFileExtension;

    static const std::string LegacyPcbFileExtension;
    static const std::string EaglePcbFileExtension;
    static const std::string CadstarPcbFileExtension;
    static const std::string KiCadPcbFileExtension;
    #define PcbFileExtension    KiCadPcbFileExtension       // symlink choice
    static const std::string KiCadSymbolLibFileExtension;
    static const std::string DrawingSheetFileExtension;
    static const std::string DesignRulesFileExtension;

    static const std::string LegacyFootprintLibPathExtension;
    static const std::string PdfFileExtension;
    static const std::string MacrosFileExtension;
    static const std::string FootprintAssignmentFileExtension;
    static const std::string DrillFileExtension;
    static const std::string SVGFileExtension;
    static const std::string ReportFileExtension;
    static const std::string FootprintPlaceFileExtension;
    static const std::string KiCadFootprintFileExtension;
    static const std::string KiCadFootprintLibPathExtension;
    static const std::string AltiumFootprintLibPathExtension;
    static const std::string CadstarFootprintLibPathExtension;
    static const std::string GedaPcbFootprintLibFileExtension;
    static const std::string EagleFootprintLibPathExtension;
    static const std::string SpecctraDsnFileExtension;
    static const std::string SpecctraSessionFileExtension;
    static const std::string IpcD356FileExtension;
    static const std::string Ipc2581FileExtension;
    static const std::string WorkbookFileExtension;

    static const std::string KiCadDesignBlockLibPathExtension;
    static const std::string KiCadDesignBlockPathExtension;

    static const std::string PngFileExtension;
    static const std::string JpegFileExtension;
    static const std::string TextFileExtension;
    static const std::string MarkdownFileExtension;
    static const std::string CsvFileExtension;
    static const std::string TsvFileExtension;
    static const std::string XmlFileExtension;
    static const std::string JsonFileExtension;
    static const std::string PythonFileExtension;

    static const std::string StepFileExtension;
    static const std::string StepZFileAbrvExtension;
    static const std::string StepFileAbrvExtension;
    static const std::string GltfBinaryFileExtension;
    static const std::string BrepFileExtension;
    static const std::string XaoFileExtension;
    static const std::string PlyFileExtension;
    static const std::string StlFileExtension;
    static const std::string U3DFileExtension;

    static const std::string GencadFileExtension;

    static const std::string KiCadJobSetFileExtension;

    static const wxString GerberFileExtensionsRegex;

    static const std::string FootprintLibraryTableFileName;
    static const std::string SymbolLibraryTableFileName;
    static const std::string DesignBlockLibraryTableFileName;

    static const std::string KiCadUriPrefix;

    /**
     * @}
     */

    /**
     * \defgroup file_wildcards File Wildcard Definitions
     *
     * @note Please do not changes these.  If a different file wildcard is needed, create a new
     *       definition in here.  If you create a wildcard definition in another file, make sure
     *       to add it to the Doxygen group "file_extensions" using the "addtogroup" tag and
     *       correct handle the GTK+ file dialog case sensitivity issue.
     * @{
     */


    static bool IsGerberFileExtension( const wxString& ext );
    static wxString AllFilesWildcard();

    static wxString FootprintAssignmentFileWildcard();
    static wxString DrawingSheetFileWildcard();
    static wxString KiCadSymbolLibFileWildcard();
    static wxString ProjectFileWildcard();
    static wxString LegacyProjectFileWildcard();
    static wxString AllProjectFilesWildcard();
    static wxString AllSchematicFilesWildcard();
    static wxString KiCadSchematicFileWildcard();
    static wxString LegacySchematicFileWildcard();
    static wxString BoardFileWildcard();
    static wxString OrCadPcb2NetlistFileWildcard();
    static wxString NetlistFileWildcard();
    static wxString AllegroNetlistFileWildcard();
    static wxString PADSNetlistFileWildcard();
    static wxString HtmlFileWildcard();
    static wxString CsvFileWildcard();
    static wxString CsvTsvFileWildcard();
    static wxString PcbFileWildcard();
    static wxString CadstarArchiveFilesWildcard();
    static wxString AltiumProjectFilesWildcard();
    static wxString EagleFilesWildcard();
    static wxString EasyEdaArchiveWildcard();
    static wxString EasyEdaProFileWildcard();
    static wxString PdfFileWildcard();
    static wxString PSFileWildcard();
    static wxString MacrosFileWildcard();
    static wxString DrillFileWildcard();
    static wxString SVGFileWildcard();
    static wxString JsonFileWildcard();
    static wxString ReportFileWildcard();
    static wxString FootprintPlaceFileWildcard();
    static wxString Shapes3DFileWildcard();
    static wxString IDF3DFileWildcard();
    static wxString DocModulesFileName();
    static wxString KiCadFootprintLibFileWildcard();
    static wxString KiCadFootprintLibPathWildcard();
    static wxString KiCadDesignBlockLibPathWildcard();
    static wxString KiCadDesignBlockPathWildcard();
    static wxString TextFileWildcard();
    static wxString ModLegacyExportFileWildcard();
    static wxString ErcFileWildcard();
    static wxString SpiceLibraryFileWildcard();
    static wxString SpiceNetlistFileWildcard();
    static wxString CadstarNetlistFileWildcard();
    static wxString EquFileWildcard();
    static wxString ZipFileWildcard();
    static wxString GencadFileWildcard();
    static wxString DxfFileWildcard();
    static wxString GerberJobFileWildcard();
    static wxString SpecctraDsnFileWildcard();
    static wxString SpecctraSessionFileWildcard();
    static wxString IpcD356FileWildcard();
    static wxString WorkbookFileWildcard();
    static wxString PngFileWildcard();
    static wxString JpegFileWildcard();
    static wxString HotkeyFileWildcard();
    static wxString JobsetFileWildcard();

    /**
     * @}
     */
};


#endif  // INCLUDE_WILDCARDS_AND_FILES_EXT_H_
