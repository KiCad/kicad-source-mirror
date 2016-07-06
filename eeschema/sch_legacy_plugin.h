#ifndef _SCH_LEGACY_PLUGIN_H_
#define _SCH_LEGACY_PLUGIN_H_

/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2016 CERN
 * Copyright (C) 2016 KiCad Developers, see change_log.txt for contributors.
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

#include <sch_io_mgr.h>


class KIWAY;
class LINE_READER;
class SCH_SCREEN;
class SCH_SHEET;
class SCH_BITMAP;
class SCH_JUNCTION;
class SCH_NO_CONNECT;
class SCH_LINE;
class SCH_BUS_ENTRY_BASE;
class SCH_TEXT;
class SCH_COMPONENT;
class PROPERTIES;


/**
 * Class SCH_LEGACY_PLUGIN
 *
 * is a #SCH_PLUGIN derivation for loading schematic files created before the new
 * s-expression file format.
 *
 * The legacy parser and formatter attempt to be compatible with the legacy file format.
 * The original parser was very forgiving in that it would parse only part of a keyword.
 * So "$C", "$Co", and "$Com" could be used for "$Comp" and the old parser would allow
 * this.  This parser is not that forgiving and sticks to the legacy file format document.
 *
 * As with all SCH_PLUGINs there is no UI dependencies i.e. windowing calls allowed.
 */
class SCH_LEGACY_PLUGIN : public SCH_PLUGIN
{
public:

    const wxString GetName() const
    {
        return wxT( "Eeschema-Legacy" );
    }

    const wxString GetFileExtension() const
    {
        return wxT( "sch" );
    }

    SCH_SHEET* Load( const wxString& aFileName, KIWAY* aKiway,
                     SCH_SHEET* aAppendToMe = NULL, const PROPERTIES* aProperties = NULL );

    void Save( const wxString& aFileName, SCH_SHEET* aSheet,
               const PROPERTIES* aProperties = NULL ) {}

    //-----</PLUGIN IMPLEMENTATION>---------------------------------------------

    SCH_LEGACY_PLUGIN();
    virtual ~SCH_LEGACY_PLUGIN() {}

private:
    void loadHierarchy( SCH_SHEET* aSheet );
    void loadHeader( FILE_LINE_READER& aReader, SCH_SCREEN* aScreen );
    void loadPageSettings( FILE_LINE_READER& aReader, SCH_SCREEN* aScreen );
    void loadFile( const wxString& aFileName, SCH_SCREEN* aScreen );
    SCH_SHEET* loadSheet( FILE_LINE_READER& aReader );
    SCH_BITMAP* loadBitmap( FILE_LINE_READER& aReader );
    SCH_JUNCTION* loadJunction( FILE_LINE_READER& aReader );
    SCH_NO_CONNECT* loadNoConnect( FILE_LINE_READER& aReader );
    SCH_LINE* loadWire( FILE_LINE_READER& aReader );
    SCH_BUS_ENTRY_BASE* loadBusEntry( FILE_LINE_READER& aReader );
    SCH_TEXT* loadText( FILE_LINE_READER& aReader );
    SCH_COMPONENT* loadComponent( FILE_LINE_READER& aReader );

protected:

    int               m_version;    ///< Version of file being loaded.
    wxString          m_error;      ///< For throwing exceptions
    wxString          m_path;       ///< Root project path for loading child sheets.
    const PROPERTIES* m_props;      ///< Passed via Save() or Load(), no ownership, may be NULL.
    KIWAY*            m_kiway;      ///< Required for path to legacy component libraries.
    SCH_SHEET*        m_rootSheet;  ///< The root sheet of the schematic being loaded..

    /// initialize PLUGIN like a constructor would.
    void init( KIWAY* aKiway, const PROPERTIES* aProperties = NULL );
};

#endif  // _SCH_LEGACY_PLUGIN_H_
