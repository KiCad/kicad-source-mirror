/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2012 CERN.
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

#ifndef KICAD_PLUGIN_H_
#define KICAD_PLUGIN_H_

#include <io_mgr.h>
#include <string>

class BOARD;
class BOARD_ITEM;
class FP_CACHE;
class PCB_PARSER;


/// Current s-expression file format version.  2 was the last legacy format version.
#define SEXPR_BOARD_FILE_VERSION    3

/// Use English default layer names
#define CTL_UNTRANSLATED_LAYERS     (1 << 0)

#define CTL_OMIT_NETS               (1 << 1)

#define CTL_OMIT_TSTAMPS            (1 << 2)

// common combinations of the above:

/// Format output for the clipboard instead of footprint library or BOARD
#define CTL_FOR_CLIPBOARD           (CTL_UNTRANSLATED_LAYERS|CTL_OMIT_NETS)

/// Format output for a footprint library instead of clipboard or BOARD
#define CTL_FOR_LIBRARY             (CTL_UNTRANSLATED_LAYERS|CTL_OMIT_NETS|CTL_OMIT_TSTAMPS)

/**
 * Class PCB_IO
 * is a PLUGIN derivation for saving and loading Pcbnew s-expression formatted files.
 *
 * @note This class is not thread safe, but it is re-entrant multiple times in sequence.
 */
class PCB_IO : public PLUGIN
{
    friend class FP_CACHE;

public:

    //-----<PLUGIN API>---------------------------------------------------------

    const wxString& PluginName() const
    {
        static const wxString name = wxT( "KiCad" );
        return name;
    }

    const wxString& GetFileExtension() const
    {
        // Would have used wildcards_and_files_ext.cpp's KiCadPcbFileExtension,
        // but to be pure, a plugin should not assume that it will always be linked
        // with the core of the pcbnew code. (Might someday be a DLL/DSO.)  Besides,
        // file extension policy should be controlled by the plugin.
        static const wxString extension = wxT( "kicad_pcb" );
        return extension;
    }

    void Save( const wxString& aFileName, BOARD* aBoard,
               PROPERTIES* aProperties = NULL );          // overload

    BOARD* Load( const wxString& aFileName, BOARD* aAppendToMe, PROPERTIES* aProperties = NULL );

    wxArrayString FootprintEnumerate( const wxString& aLibraryPath, PROPERTIES* aProperties = NULL);

    MODULE* FootprintLoad( const wxString& aLibraryPath, const wxString& aFootprintName,
                           PROPERTIES* aProperties = NULL );

    void FootprintSave( const wxString& aLibraryPath, const MODULE* aFootprint,
                        PROPERTIES* aProperties = NULL );

    void FootprintDelete( const wxString& aLibraryPath, const wxString& aFootprintName );

    void FootprintLibCreate( const wxString& aLibraryPath, PROPERTIES* aProperties = NULL);

    bool FootprintLibDelete( const wxString& aLibraryPath, PROPERTIES* aProperties = NULL );

    bool IsFootprintLibWritable( const wxString& aLibraryPath );

    //-----</PLUGIN API>--------------------------------------------------------

    PCB_IO();

    PCB_IO( int aControlFlags );

    ~PCB_IO();

    /**
     * Function Format
     * outputs \a aItem to \a aFormatter in s-expression format.
     *
     * @param aItem A pointer the an #BOARD_ITEM object to format.
     * @param aNestLevel The indentation nest level.
     * @throw IO_ERROR on write error.
     */
    void Format( BOARD_ITEM* aItem, int aNestLevel = 0 ) const
        throw( IO_ERROR );

    std::string GetStringOutput( bool doClear )
    {
        std::string ret = m_sf.GetString();
        if( doClear )
            m_sf.Clear();

        return ret;
    }

    void SetOutputFormatter( OUTPUTFORMATTER* aFormatter ) { m_out = aFormatter; }

    BOARD_ITEM* Parse( const wxString& aClipboardSourceInput )
        throw( PARSE_ERROR, IO_ERROR );

protected:

    wxString        m_error;        ///< for throwing exceptions
    BOARD*          m_board;        ///< which BOARD, no ownership here
    PROPERTIES*     m_props;        ///< passed via Save() or Load(), no ownership, may be NULL.
    FP_CACHE*       m_cache;        ///< Footprint library cache.

    LINE_READER*    m_reader;       ///< no ownership here.
    wxString        m_filename;     ///< for saves only, name is in m_reader for loads

    int             m_loading_format_version; ///< which #SEXPR_BOARD_FILE_VERSION should be Load()ed?

    STRING_FORMATTER    m_sf;
    OUTPUTFORMATTER*    m_out;      ///< output any Format()s to this, no ownership
    int                 m_ctl;
    PCB_PARSER*         m_parser;


private:
    void format( BOARD* aBoard, int aNestLevel = 0 ) const
        throw( IO_ERROR );

    void format( DIMENSION* aDimension, int aNestLevel = 0 ) const
        throw( IO_ERROR );

    void format( EDGE_MODULE* aModuleDrawing, int aNestLevel = 0 ) const
        throw( IO_ERROR );

    void format( DRAWSEGMENT* aSegment, int aNestLevel = 0 ) const
        throw( IO_ERROR );

    void format( PCB_TARGET* aTarget, int aNestLevel = 0 ) const
        throw( IO_ERROR );

    void format( MODULE* aModule, int aNestLevel = 0 ) const
        throw( IO_ERROR );

    void format( D_PAD* aPad, int aNestLevel = 0 ) const
        throw( IO_ERROR );

    void format( TEXTE_PCB* aText, int aNestLevel = 0 ) const
        throw( IO_ERROR );

    void format( TEXTE_MODULE* aText, int aNestLevel = 0 ) const
        throw( IO_ERROR );

    void format( TRACK* aTrack, int aNestLevel = 0 ) const
        throw( IO_ERROR );

    void format( ZONE_CONTAINER* aZone, int aNestLevel = 0 ) const
        throw( IO_ERROR );

    void formatLayer( const BOARD_ITEM* aItem ) const;

    void formatLayers( int aLayerMask, int aNestLevel = 0 ) const
        throw( IO_ERROR );

    /// we only cache one footprint library for now, this determines which one.
    void cacheLib( const wxString& aLibraryPath );

    void init( PROPERTIES* aProperties );
};

#endif  // KICAD_PLUGIN_H_
