/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2016 CERN
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 *
 * @author Wayne Stambaugh <stambaughw@gmail.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 3
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef _SCH_IO_MGR_H_
#define _SCH_IO_MGR_H_

#include <import_export.h>
#include <map>
#include <enum_vector.h>
#include <reporter.h>
#include <i18n_utility.h>
#include <io/io_base.h>
#include <io/io_mgr.h>
#include <wx/arrstr.h>


class SCH_SHEET;
class SCH_SCREEN;
class SCH_IO;
class SCHEMATIC;
class SYMBOL_LIB_TABLE;
class KIWAY;
class LIB_SYMBOL;
class LEGACY_SYMBOL_LIB;
class PROGRESS_REPORTER;


/**
 * A factory which returns an instance of a #SCH_IO.
 */
class SCH_IO_MGR : public IO_MGR
{
public:

    /**
     * A set of file types that the #SCH_IO_MGR knows about, and for which there
     * has been a plugin written, in alphabetical order.
     */
    // clang-format off
    DEFINE_ENUM_VECTOR( SCH_FILE_T,
    {
        SCH_KICAD,            ///< The s-expression version of the schematic.
        SCH_LEGACY,           ///< Legacy Eeschema file formats prior to s-expression.
        SCH_ALTIUM,           ///< Altium file format
        SCH_CADSTAR_ARCHIVE,  ///< CADSTAR Schematic Archive
        SCH_DATABASE,         ///< KiCad database library
        SCH_EAGLE,            ///< Autodesk Eagle file format
        SCH_EASYEDA,          ///< EasyEDA Std schematic file
        SCH_EASYEDAPRO,       ///< EasyEDA Pro archive
        SCH_LTSPICE,          ///< LtSpice Schematic format
        SCH_HTTP,             ///< KiCad HTTP library

        // Add your schematic type here.
        SCH_FILE_UNKNOWN
    } )
    // clang-format on

    /**
     * Return a #SCH_IO which the caller can use to import, export, save, or load
     * design documents.
     *
     * @param aFileType is from #SCH_FILE_T and tells which plugin to find.
     *
     * @return the plugin corresponding to aFileType or NULL if not found.
     *  Caller owns the returned object.
     */
    APIEXPORT
    static SCH_IO* FindPlugin( SCH_FILE_T aFileType );

    /**
     * Return a brief name for a plugin, given aFileType enum.
     */
    static const wxString ShowType( SCH_FILE_T aFileType );

    /**
     * Return the #SCH_FILE_T from the corresponding plugin type name: "kicad", "legacy", etc.
     */
    static SCH_FILE_T EnumFromStr( const wxString& aFileType );

    /**
     * Return a plugin type given a symbol library using the file extension of \a aLibPath.
     */
    static SCH_FILE_T GuessPluginTypeFromLibPath( const wxString& aLibPath, int aCtl = 0 );

    /**
     * Return a plugin type given a schematic using the file extension of \a aSchematicPath.
     */
    static SCH_FILE_T GuessPluginTypeFromSchPath( const wxString& aSchematicPath, int aCtl = 0 );

    /**
     * Convert a schematic symbol library to the latest KiCad format
     */
    static bool ConvertLibrary( std::map<std::string, UTF8>* aOldFileProps, const wxString& aOldFilePath,
                                const wxString& aNewFilepath );
};

#endif // _SCH_IO_MGR_H_
