/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 *
 * @author Wayne Stambaugh <stambaughw@gmail.com>
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
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef _SCH_LEGACY_LIB_PLUGIN_CACHE_
#define _SCH_LEGACY_LIB_PLUGIN_CACHE_

#include <memory>

#include <eda_shape.h>  // FILL_T

#include "sch_io/sch_io_lib_cache.h"

class FILE_LINE_READER;
class SCH_PIN;
class SCH_FIELD;
class SCH_SHAPE;
class SCH_TEXT;
class LINE_READER;
class SCH_IO_KICAD_LEGACY;


/**
 * A cache assistant for KiCad legacy symbol libraries.
 */
class SCH_IO_KICAD_LEGACY_LIB_CACHE : public SCH_IO_LIB_CACHE
{
public:
    SCH_IO_KICAD_LEGACY_LIB_CACHE( const wxString& aLibraryPath );
    virtual ~SCH_IO_KICAD_LEGACY_LIB_CACHE() {}

    // Most all functions in this class throw IO_ERROR exceptions.  There are no
    // error codes nor user interface calls from here, nor in any SCH_IO objects.
    // Catch these exceptions higher up please.

    /// Save the entire library to file m_libFileName;
    void Save( const std::optional<bool>& aOpt ) override;

    void Load() override;

    void DeleteSymbol( const wxString& aName ) override;

    static LIB_SYMBOL* LoadPart( LINE_READER& aReader, int aMajorVersion, int aMinorVersion,
                                 LIB_SYMBOL_MAP* aMap = nullptr );
    static void      SaveSymbol( LIB_SYMBOL* aSymbol, OUTPUTFORMATTER& aFormatter,
                                 LIB_SYMBOL_MAP* aMap = nullptr );

private:
    friend SCH_IO_KICAD_LEGACY;

    void              loadHeader( FILE_LINE_READER& aReader );
    static void       loadAliases( std::unique_ptr<LIB_SYMBOL>& aSymbol, LINE_READER& aReader,
                                   LIB_SYMBOL_MAP* aMap = nullptr );
    static void       loadField( std::unique_ptr<LIB_SYMBOL>& aSymbol, LINE_READER& aReader );
    static void       loadDrawEntries( std::unique_ptr<LIB_SYMBOL>& aSymbol, LINE_READER& aReader,
                                       int aMajorVersion, int aMinorVersion );
    static void       loadFootprintFilters( std::unique_ptr<LIB_SYMBOL>& aSymbol,
                                            LINE_READER& aReader );
    void              loadDocs();
    static SCH_SHAPE* loadArc( LINE_READER& aReader );
    static SCH_SHAPE* loadCircle( LINE_READER& aReader );
    static SCH_ITEM*  loadText( LINE_READER& aReader, int aMajorVersion, int aMinorVersion );
    static SCH_SHAPE* loadRect( LINE_READER& aReader );
    static SCH_PIN*   loadPin( std::unique_ptr<LIB_SYMBOL>& aSymbol, LINE_READER& aReader );
    static SCH_SHAPE* loadPolyLine( LINE_READER& aReader );
    static SCH_SHAPE* loadBezier( LINE_READER& aReader );

    static FILL_T   parseFillMode( LINE_READER& aReader, const char* aLine, const char** aOutput );

    void            saveDocFile();
    static void     saveArc( SCH_SHAPE* aArc, OUTPUTFORMATTER& aFormatter );
    static void     saveBezier( SCH_SHAPE* aBezier, OUTPUTFORMATTER& aFormatter );
    static void     saveCircle( SCH_SHAPE* aCircle, OUTPUTFORMATTER& aFormatter );
    static void     saveField( const SCH_FIELD* aField, int aLegacyFieldIdx,
                               OUTPUTFORMATTER& aFormatter );
    static void     savePin( const SCH_PIN* aPin, OUTPUTFORMATTER& aFormatter );
    static void     savePolyLine( SCH_SHAPE* aPolyLine, OUTPUTFORMATTER& aFormatter );
    static void     saveRectangle( SCH_SHAPE* aRectangle, OUTPUTFORMATTER& aFormatter );
    static void     saveText( const SCH_TEXT* aText, OUTPUTFORMATTER& aFormatter );

    int             m_versionMajor;
    int             m_versionMinor;
};

#endif    // _SCH_LEGACY_LIB_PLUGIN_CACHE_
