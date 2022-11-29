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

#ifndef _SCH_SEXPR_LIB_PLUGIN_CACHE_
#define _SCH_SEXPR_LIB_PLUGIN_CACHE_

#include "../sch_lib_plugin_cache.h"

class FILE_LINE_READER;
class LIB_FIELD;
class LIB_ITEM;
class LIB_PIN;
class LIB_TEXT;
class LIB_TEXTBOX;
class LINE_READER;
class SCH_SEXPR_PLUGIN;

/**
 * A cache assistant for the KiCad s-expression symbol libraries.
 */
class SCH_SEXPR_PLUGIN_CACHE : public SCH_LIB_PLUGIN_CACHE
{
public:
    SCH_SEXPR_PLUGIN_CACHE( const wxString& aLibraryPath );
    virtual ~SCH_SEXPR_PLUGIN_CACHE();

    // Most all functions in this class throw IO_ERROR exceptions.  There are no
    // error codes nor user interface calls from here, nor in any SCH_PLUGIN objects.
    // Catch these exceptions higher up please.

    /// Save the entire library to file m_libFileName;
    void Save( const std::optional<bool>& aOpt = std::nullopt ) override;

    void Load() override;

    void DeleteSymbol( const wxString& aName ) override;

    static void SaveSymbol( LIB_SYMBOL* aSymbol, OUTPUTFORMATTER& aFormatter,
                            int aNestLevel = 0, const wxString& aLibName = wxEmptyString );

    void SetFileFormatVersionAtLoad( int aVersion ) { m_fileFormatVersionAtLoad = aVersion; }
    int GetFileFormatVersionAtLoad()  const { return m_fileFormatVersionAtLoad; }

private:
    friend SCH_SEXPR_PLUGIN;

    int m_fileFormatVersionAtLoad;

    static void saveSymbolDrawItem( LIB_ITEM* aItem, OUTPUTFORMATTER& aFormatter,
                                    int aNestLevel );
    static void saveField( LIB_FIELD* aField, OUTPUTFORMATTER& aFormatter, int aNestLevel );
    static void savePin( LIB_PIN* aPin, OUTPUTFORMATTER& aFormatter, int aNestLevel = 0 );
    static void saveText( LIB_TEXT* aText, OUTPUTFORMATTER& aFormatter, int aNestLevel = 0 );
    static void saveTextBox( LIB_TEXTBOX* aTextBox, OUTPUTFORMATTER& aFormatter,
                             int aNestLevel = 0 );

    static void saveDcmInfoAsFields( LIB_SYMBOL* aSymbol, OUTPUTFORMATTER& aFormatter,
                                     int& aNextFreeFieldId, int aNestLevel );
};

#endif    // _SCH_SEXPR_LIB_PLUGIN_CACHE_
