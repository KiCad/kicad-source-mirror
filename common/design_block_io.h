/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2024 Mike Williams <mike@mikebwilliams.com>
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
#ifndef DESIGN_BLOCK_IO_H
#define DESIGN_BLOCK_IO_H

#include <kicommon.h>
#include <io/io_base.h>
#include <io/io_mgr.h>

class DESIGN_BLOCK;
class DESIGN_BLOCK_IO;
class wxArrayString;

class KICOMMON_API DESIGN_BLOCK_IO_MGR : public IO_MGR
{
public:
    enum DESIGN_BLOCK_FILE_T
    {
        DESIGN_BLOCK_FILE_UNKNOWN = 0, ///< 0 is not a legal menu id on Mac
        KICAD_SEXP,                    ///< S-expression KiCad file format.

        FILE_TYPE_NONE,
        NESTED_TABLE
    };

    static const wxString      ShowType( DESIGN_BLOCK_FILE_T aFileType );
    static DESIGN_BLOCK_IO*    FindPlugin( DESIGN_BLOCK_FILE_T aFileType );
    static DESIGN_BLOCK_FILE_T EnumFromStr( const wxString& aFileType );
    static DESIGN_BLOCK_FILE_T GuessPluginTypeFromLibPath( const wxString& aLibPath, int aCtl = 0 );

    /**
     * Convert a design block library to the latest KiCad format.
     */
    static bool ConvertLibrary( std::map<std::string, UTF8>* aOldFileProps,
                                const wxString& aOldFilePath, const wxString& aNewFilePath );
};


class KICOMMON_API DESIGN_BLOCK_IO : public IO_BASE
{
public:
    DESIGN_BLOCK_IO() : IO_BASE( wxS( "KiCad" ) ) {}

    const IO_BASE::IO_FILE_DESC GetLibraryDesc() const override;
    long long                   GetLibraryTimestamp( const wxString& aLibraryPath ) const;

    void DesignBlockEnumerate( wxArrayString& aDesignBlockNames, const wxString& aLibraryPath,
                               bool                               aBestEfforts,
                               const std::map<std::string, UTF8>* aProperties = nullptr );

    const DESIGN_BLOCK*
    GetEnumeratedDesignBlock( const wxString& aLibraryPath, const wxString& aDesignBlockName,
                              const std::map<std::string, UTF8>* aProperties = nullptr )
    {
        return DesignBlockLoad( aLibraryPath, aDesignBlockName, false, aProperties );
    }

    bool DesignBlockExists( const wxString& aLibraryPath, const wxString& aDesignBlockName,
                            const std::map<std::string, UTF8>* aProperties = nullptr );

    DESIGN_BLOCK* ImportDesignBlock( const wxString&                    aDesignBlockPath,
                                     wxString&                          aDesignBlockNameOut,
                                     const std::map<std::string, UTF8>* aProperties = nullptr )
    {
        return nullptr;
    }

    void CreateLibrary( const wxString&                    aLibraryPath,
                        const std::map<std::string, UTF8>* aProperties = nullptr ) override;

    virtual bool DeleteLibrary( const wxString&                    aLibraryPath,
                                const std::map<std::string, UTF8>* aProperties = nullptr ) override;


    bool IsLibraryWritable( const wxString& aLibraryPath ) override;

    DESIGN_BLOCK* DesignBlockLoad( const wxString& aLibraryPath, const wxString& aDesignBlockName,
                                   bool                               aKeepUUID = false,
                                   const std::map<std::string, UTF8>* aProperties = nullptr );

    void DesignBlockSave( const wxString& aLibraryPath, const DESIGN_BLOCK* aDesignBlock,
                          const std::map<std::string, UTF8>* aProperties = nullptr );

    void DesignBlockDelete( const wxString& aLibraryPath, const wxString& aDesignBlockName,
                            const std::map<std::string, UTF8>* aProperties = nullptr );
};

#endif
