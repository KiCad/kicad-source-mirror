/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2018 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 2007-2012 SoftPLC Corporation, Dick Hollenbeck <dick@softplc.com>
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
 * @file wildcards_and_files_ext.h
 * Definition of file extensions used in Kicad.
 */

#ifndef INCLUDE_WILDCARDS_AND_FILES_EXT_H_
#define INCLUDE_WILDCARDS_AND_FILES_EXT_H_

#include <wx/wx.h>

#include <string>
#include <vector>

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

wxString AddFileExtListToFilter( const std::vector<std::string>& aExts );

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
wxString formatWildcardExt( const wxString& aWildcard );

// Do NOT use wxString for these.  wxStrings are not thread-safe, even when const.  (For the
// curious the UTF8 cacheing strategy puts iterators in a linked list.  Insertion and removal
// from the linked list is not thread-safe.)

extern const std::string SchematicSymbolFileExtension;
extern const std::string SchematicLibraryFileExtension;
extern const std::string SchematicBackupFileExtension;

extern const std::string VrmlFileExtension;
extern const std::string ProjectFileExtension;
extern const std::string SchematicFileExtension;
extern const std::string NetlistFileExtension;
extern const std::string GerberFileExtension;
extern const std::string GerberJobFileExtension;
extern const std::string HtmlFileExtension;
extern const std::string EquFileExtension;

extern const std::string LegacyPcbFileExtension;
extern const std::string KiCadPcbFileExtension;
#define PcbFileExtension    KiCadPcbFileExtension       // symlink choice
extern const std::string PageLayoutDescrFileExtension;

extern const std::string LegacyFootprintLibPathExtension;
extern const std::string PdfFileExtension;
extern const std::string MacrosFileExtension;
extern const std::string ComponentFileExtension;
extern const std::string DrillFileExtension;
extern const std::string SVGFileExtension;
extern const std::string ReportFileExtension;
extern const std::string FootprintPlaceFileExtension;
extern const std::string KiCadFootprintFileExtension;
extern const std::string KiCadFootprintLibPathExtension;
extern const std::string GedaPcbFootprintLibFileExtension;
extern const std::string EagleFootprintLibPathExtension;
extern const std::string ComponentFileExtension;
extern const std::string PageLayoutDescrFileExtension;
extern const std::string KiCadLib3DShapesPathExtension;
extern const std::string SpecctraDsnFileExtension;
extern const std::string IpcD356FileExtension;

extern const std::string PngFileExtension;
extern const std::string JpegFileExtension;

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

extern wxString AllFilesWildcard();

extern wxString ComponentFileWildcard();
extern wxString PageLayoutDescrFileWildcard();
extern wxString SchematicSymbolFileWildcard();
extern wxString SchematicLibraryFileWildcard();
extern wxString ProjectFileWildcard();
extern wxString SchematicFileWildcard();
extern wxString BoardFileWildcard();
extern wxString NetlistFileWildcard();
extern wxString GerberFileWildcard();
extern wxString HtmlFileWildcard();
extern wxString CsvFileWildcard();
extern wxString LegacyPcbFileWildcard();
extern wxString PcbFileWildcard();
extern wxString EaglePcbFileWildcard();
extern wxString EagleSchematicFileWildcard();
extern wxString EagleFilesWildcard();
extern wxString PCadPcbFileWildcard();
extern wxString PdfFileWildcard();
extern wxString PSFileWildcard();
extern wxString MacrosFileWildcard();
extern wxString ComponentFileWildcard();
extern wxString DrillFileWildcard();
extern wxString SVGFileWildcard();
extern wxString ReportFileWildcard();
extern wxString FootprintPlaceFileWildcard();
extern wxString Shapes3DFileWildcard();
extern wxString IDF3DFileWildcard();
extern wxString DocModulesFileName();
extern wxString LegacyFootprintLibPathWildcard();
extern wxString KiCadFootprintLibFileWildcard();
extern wxString KiCadFootprintLibPathWildcard();
extern wxString GedaPcbFootprintLibFileWildcard();
extern wxString EagleFootprintLibPathWildcard();
extern wxString TextFileWildcard();
extern wxString ModLegacyExportFileWildcard();
extern wxString ErcFileWildcard();
extern wxString SpiceLibraryFileWildcard();
extern wxString SpiceNetlistFileWildcard();
extern wxString CadstarNetlistFileWildcard();
extern wxString EquFileWildcard();
extern wxString ZipFileWildcard();
extern wxString GencadFileWildcard();
extern wxString DxfFileWildcard();
extern wxString GerberJobFileWildcard();
extern wxString SpecctraDsnFileWildcard();
extern wxString IpcD356FileWildcard();
extern wxString WorkbookFileWildcard();
extern wxString PngFileWildcard();
extern wxString JpegFileWildcard();

/**
 * @}
 */

#endif  // INCLUDE_WILDCARDS_AND_FILES_EXT_H_
