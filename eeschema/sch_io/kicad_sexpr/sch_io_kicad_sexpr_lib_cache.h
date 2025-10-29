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

#ifndef SCH_IO_KICAD_SEXPR_LIB_CACHE_H_
#define SCH_IO_KICAD_SEXPR_LIB_CACHE_H_

#include "sch_io/sch_io_lib_cache.h"

class FILE_LINE_READER;
class SCH_PIN;
class SCH_TEXT;
class SCH_TEXTBOX;
class LINE_READER;
class SCH_IO_KICAD_SEXPR;

/**
 * A cache assistant for the KiCad s-expression symbol libraries.
 */
class SCH_IO_KICAD_SEXPR_LIB_CACHE : public SCH_IO_LIB_CACHE
{
public:
    SCH_IO_KICAD_SEXPR_LIB_CACHE( const wxString& aLibraryPath );
    virtual ~SCH_IO_KICAD_SEXPR_LIB_CACHE();

    // Most all functions in this class throw IO_ERROR exceptions.  There are no
    // error codes nor user interface calls from here, nor in any SCH_IO objects.
    // Catch these exceptions higher up please.

    /// Save the entire library to file m_libFileName;
    void Save( const std::optional<bool>& aOpt = std::nullopt ) override;

    void Load() override;

    void DeleteSymbol( const wxString& aName ) override;

    static void SaveSymbol( LIB_SYMBOL* aSymbol, OUTPUTFORMATTER& aFormatter,
                            const wxString& aLibName = wxEmptyString, bool aIncludeData = true );

    void SetFileFormatVersionAtLoad( int aVersion ) { m_fileFormatVersionAtLoad = aVersion; }
    int GetFileFormatVersionAtLoad()  const { return m_fileFormatVersionAtLoad; }

private:
    friend SCH_IO_KICAD_SEXPR;

    /**
     * Update the parent symbol links for derived symbols.
     *
     * This is now performed post library load because the symbol load order cannot be controlled
     * when the symbol library is saved as a single symbol per file in a folder.
     *
     * @throw #IO_ERROR if a parent symbol name cannot be found for any symbols in the library.
     */
    void updateParentSymbolLinks();

    void formatLibraryHeader( OUTPUTFORMATTER& aFormatter );

    bool isLibraryPathValid() const;

    int m_fileFormatVersionAtLoad;

    static void saveSymbolDrawItem( SCH_ITEM* aItem, OUTPUTFORMATTER& aFormatter );
    static void saveField( SCH_FIELD* aField, OUTPUTFORMATTER& aFormatter );
    static void savePin( SCH_PIN* aPin, OUTPUTFORMATTER& aFormatter );
    static void saveText( SCH_TEXT* aText, OUTPUTFORMATTER& aFormatter );
    static void saveTextBox( SCH_TEXTBOX* aTextBox, OUTPUTFORMATTER& aFormatter );

    static void saveDcmInfoAsFields( LIB_SYMBOL* aSymbol, OUTPUTFORMATTER& aFormatter );
};

#endif    // SCH_IO_KICAD_SEXPR_LIB_CACHE_H_
