/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2013 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 2007-2012 SoftPLC Corporation, Dick Hollenbeck <dick@softplc.com>
 * Copyright (C) 2008 Wayne Stambaugh <stambaughw@gmail.com>
 * Copyright (C) 1992-2017 KiCad Developers, see AUTHORS.txt for contributors.
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
 * The common library
 * @file wildcards_and_files_ext.h
 */

#ifndef INCLUDE_WILDCARDS_AND_FILES_EXT_H_
#define INCLUDE_WILDCARDS_AND_FILES_EXT_H_

#include <wx/wx.h>

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

extern const wxString AllFilesWildcard;

extern const wxString SchematicSymbolFileExtension;
extern const wxString SchematicLibraryFileExtension;
extern const wxString SchematicBackupFileExtension;

extern const wxString VrmlFileExtension;
extern const wxString ProjectFileExtension;
extern const wxString SchematicFileExtension;
extern const wxString NetlistFileExtension;
extern const wxString GerberFileExtension;
extern const wxString GerberJobFileExtension;
extern const wxString HtmlFileExtension;

extern const wxString LegacyPcbFileExtension;
extern const wxString KiCadPcbFileExtension;
#define PcbFileExtension    KiCadPcbFileExtension       // symlink choice
extern const wxString PageLayoutDescrFileExtension;

extern const wxString LegacyFootprintLibPathExtension;
extern const wxString PdfFileExtension;
extern const wxString MacrosFileExtension;
extern const wxString ComponentFileExtension;
extern const wxString DrillFileExtension;
extern const wxString SVGFileExtension;
extern const wxString ReportFileExtension;
extern const wxString FootprintPlaceFileExtension;
extern const wxString KiCadFootprintFileExtension;
extern const wxString KiCadFootprintLibPathExtension;
extern const wxString GedaPcbFootprintLibFileExtension;
extern const wxString EagleFootprintLibPathExtension;
extern const wxString ComponentFileExtension;
extern const wxString PageLayoutDescrFileExtension;
extern const wxString KiCadLib3DShapesPathExtension;
extern const wxString SpecctraDsnFileExtension;
extern const wxString IpcD356FileExtension;

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

/**
 * @}
 */

#endif  // INCLUDE_WILDCARDS_AND_FILES_EXT_H_
