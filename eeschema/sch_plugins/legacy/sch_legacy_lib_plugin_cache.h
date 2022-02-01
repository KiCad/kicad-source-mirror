/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2022 KiCad Developers, see AUTHORS.txt for contributors.
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

#include "../sch_lib_plugin_cache.h"

class FILE_LINE_READER;
class LIB_FIELD;
class LIB_PIN;
class LIB_SHAPE;
class LIB_TEXT;
class LINE_READER;
class SCH_LEGACY_PLUGIN;


/**
 * A cache assistant for KiCad legacy symbol libraries.
 */
class SCH_LEGACY_PLUGIN_CACHE : public SCH_LIB_PLUGIN_CACHE
{
public:
    SCH_LEGACY_PLUGIN_CACHE( const wxString& aLibraryPath );
    virtual ~SCH_LEGACY_PLUGIN_CACHE() {}

    // Most all functions in this class throw IO_ERROR exceptions.  There are no
    // error codes nor user interface calls from here, nor in any SCH_PLUGIN objects.
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
    friend SCH_LEGACY_PLUGIN;

    void              loadHeader( FILE_LINE_READER& aReader );
    static void       loadAliases( std::unique_ptr<LIB_SYMBOL>& aSymbol, LINE_READER& aReader,
                                   LIB_SYMBOL_MAP* aMap = nullptr );
    static void       loadField( std::unique_ptr<LIB_SYMBOL>& aSymbol, LINE_READER& aReader );
    static void       loadDrawEntries( std::unique_ptr<LIB_SYMBOL>& aSymbol, LINE_READER& aReader,
                                       int aMajorVersion, int aMinorVersion );
    static void       loadFootprintFilters( std::unique_ptr<LIB_SYMBOL>& aSymbol,
                                            LINE_READER& aReader );
    void              loadDocs();
    static LIB_SHAPE* loadArc( std::unique_ptr<LIB_SYMBOL>& aSymbol, LINE_READER& aReader );
    static LIB_SHAPE* loadCircle( std::unique_ptr<LIB_SYMBOL>& aSymbol, LINE_READER& aReader );
    static LIB_TEXT*  loadText( std::unique_ptr<LIB_SYMBOL>& aSymbol, LINE_READER& aReader,
                                int aMajorVersion, int aMinorVersion );
    static LIB_SHAPE* loadRect( std::unique_ptr<LIB_SYMBOL>& aSymbol, LINE_READER& aReader );
    static LIB_PIN*   loadPin( std::unique_ptr<LIB_SYMBOL>& aSymbol, LINE_READER& aReader );
    static LIB_SHAPE* loadPolyLine( std::unique_ptr<LIB_SYMBOL>& aSymbol, LINE_READER& aReader );
    static LIB_SHAPE* loadBezier( std::unique_ptr<LIB_SYMBOL>& aSymbol, LINE_READER& aReader );

    static FILL_T   parseFillMode( LINE_READER& aReader, const char* aLine, const char** aOutput );

    void            saveDocFile();
    static void     saveArc( LIB_SHAPE* aArc, OUTPUTFORMATTER& aFormatter );
    static void     saveBezier( LIB_SHAPE* aBezier, OUTPUTFORMATTER& aFormatter );
    static void     saveCircle( LIB_SHAPE* aCircle, OUTPUTFORMATTER& aFormatter );
    static void     saveField( const LIB_FIELD* aField, OUTPUTFORMATTER& aFormatter );
    static void     savePin( const LIB_PIN* aPin, OUTPUTFORMATTER& aFormatter );
    static void     savePolyLine( LIB_SHAPE* aPolyLine, OUTPUTFORMATTER& aFormatter );
    static void     saveRectangle( LIB_SHAPE* aRectangle, OUTPUTFORMATTER& aFormatter );
    static void     saveText( const LIB_TEXT* aText, OUTPUTFORMATTER& aFormatter );

    int             m_versionMajor;
    int             m_versionMinor;
};

#endif    // _SCH_LEGACY_LIB_PLUGIN_CACHE_
