/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2012 CERN.
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

#ifndef KICAD_PLUGIN_H_
#define KICAD_PLUGIN_H_

#include <io_mgr.h>
#include <string>
#include <layers_id_colors_and_visibility.h>

class BOARD;
class BOARD_ITEM;
class FP_CACHE;
class PCB_PARSER;
class NETINFO_MAPPING;
class BOARD_DESIGN_SETTINGS;
class DIMENSION;
class EDGE_MODULE;
class DRAWSEGMENT;
class PCB_TARGET;
class D_PAD;
class TEXTE_MODULE;
class TRACK;
class ZONE_CONTAINER;
class TEXTE_PCB;


/// Current s-expression file format version.  2 was the last legacy format version.

//#define SEXPR_BOARD_FILE_VERSION    3         // first s-expression format, used legacy cu stack
//#define SEXPR_BOARD_FILE_VERSION    4         // reversed cu stack, changed Inner* to In* in reverse order
//                                              // went to 32 Cu layers from 16.
//#define SEXPR_BOARD_FILE_VERSION    20160815  // differential pair settings per net class
//#define SEXPR_BOARD_FILE_VERSION    20170123  // EDA_TEXT refactor, moved 'hide'
//#define SEXPR_BOARD_FILE_VERSION    20170920  // long pad names and custom pad shape
//#define SEXPR_BOARD_FILE_VERSION    20170922  // Keepout zones can exist on multiple layers
//#define SEXPR_BOARD_FILE_VERSION    20171114  // Save 3D model offset in mm, instead of inches
//#define SEXPR_BOARD_FILE_VERSION    20171125  // Locked/unlocked TEXTE_MODULE
//#define SEXPR_BOARD_FILE_VERSION    20171130  // 3D model offset written using "offset" parameter
//#define SEXPR_BOARD_FILE_VERSION    20190331  // hatched zones and chamfered round rect pads
//#define SEXPR_BOARD_FILE_VERSION    20190421  // curves in custom pads
//#define SEXPR_BOARD_FILE_VERSION    20190516  // Remove segment count from zones
#define SEXPR_BOARD_FILE_VERSION      20190605  // Add layer defaults

#define CTL_STD_LAYER_NAMES         (1 << 0)    ///< Use English Standard layer names
#define CTL_OMIT_NETS               (1 << 1)    ///< Omit pads net names (useless in library)
#define CTL_OMIT_TSTAMPS            (1 << 2)    ///< Omit component time stamp (useless in library)
#define CTL_OMIT_INITIAL_COMMENTS   (1 << 3)    ///< omit MODULE initial comments
#define CTL_OMIT_PATH               (1 << 4)    ///< Omit component sheet time stamp (useless in library)
#define CTL_OMIT_AT                 (1 << 5)    ///< Omit position and rotation
                                                // (always saved with potion 0,0 and rotation = 0 in library)
//#define CTL_OMIT_HIDE             (1 << 6)    // found and defined in eda_text.h


// common combinations of the above:

/// Format output for the clipboard instead of footprint library or BOARD
#define CTL_FOR_CLIPBOARD           (CTL_STD_LAYER_NAMES|CTL_OMIT_NETS)

/// Format output for a footprint library instead of clipboard or BOARD
#define CTL_FOR_LIBRARY             (CTL_STD_LAYER_NAMES|CTL_OMIT_NETS|CTL_OMIT_TSTAMPS|CTL_OMIT_PATH|CTL_OMIT_AT)

/// The zero arg constructor when PCB_IO is used for PLUGIN::Load() and PLUGIN::Save()ing
/// a BOARD file underneath IO_MGR.
#define CTL_FOR_BOARD               (CTL_OMIT_INITIAL_COMMENTS)


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

    const wxString PluginName() const override
    {
        return wxT( "KiCad" );
    }

    const wxString GetFileExtension() const override
    {
        // Would have used wildcards_and_files_ext.cpp's KiCadPcbFileExtension,
        // but to be pure, a plugin should not assume that it will always be linked
        // with the core of the pcbnew code. (Might someday be a DLL/DSO.)  Besides,
        // file extension policy should be controlled by the plugin.
        return wxT( "kicad_pcb" );
    }

    virtual void Save( const wxString& aFileName, BOARD* aBoard,
               const PROPERTIES* aProperties = NULL ) override;

    BOARD* Load( const wxString& aFileName, BOARD* aAppendToMe,
                 const PROPERTIES* aProperties = NULL ) override;

    void FootprintEnumerate( wxArrayString& aFootprintNames, const wxString& aLibraryPath,
                             const PROPERTIES* aProperties = NULL ) override;

    const MODULE* GetEnumeratedFootprint( const wxString& aLibraryPath,
                                          const wxString& aFootprintName,
                                          const PROPERTIES* aProperties = NULL ) override;

    bool FootprintExists( const wxString& aLibraryPath, const wxString& aFootprintName,
                          const PROPERTIES* aProperties = NULL ) override;

    MODULE* FootprintLoad( const wxString& aLibraryPath, const wxString& aFootprintName,
                           const PROPERTIES* aProperties = NULL ) override;

    void FootprintSave( const wxString& aLibraryPath, const MODULE* aFootprint,
                        const PROPERTIES* aProperties = NULL ) override;

    void FootprintDelete( const wxString& aLibraryPath, const wxString& aFootprintName,
                          const PROPERTIES* aProperties = NULL ) override;

    long long GetLibraryTimestamp( const wxString& aLibraryPath ) const override;

    void FootprintLibCreate( const wxString& aLibraryPath, const PROPERTIES* aProperties = NULL) override;

    bool FootprintLibDelete( const wxString& aLibraryPath, const PROPERTIES* aProperties = NULL ) override;

    bool IsFootprintLibWritable( const wxString& aLibraryPath ) override;

    //-----</PLUGIN API>--------------------------------------------------------

    PCB_IO( int aControlFlags = CTL_FOR_BOARD );

    ~PCB_IO();

    /**
     * Function Format
     * outputs \a aItem to \a aFormatter in s-expression format.
     *
     * @param aItem A pointer the an #BOARD_ITEM object to format.
     * @param aNestLevel The indentation nest level.
     * @throw IO_ERROR on write error.
     */
    void Format( BOARD_ITEM* aItem, int aNestLevel = 0 ) const;

    std::string GetStringOutput( bool doClear )
    {
        std::string ret = m_sf.GetString();
        if( doClear )
            m_sf.Clear();

        return ret;
    }

    void SetOutputFormatter( OUTPUTFORMATTER* aFormatter ) { m_out = aFormatter; }

    BOARD_ITEM* Parse( const wxString& aClipboardSourceInput );

protected:

    wxString        m_error;        ///< for throwing exceptions
    BOARD*          m_board;        ///< which BOARD, no ownership here

    const
    PROPERTIES*     m_props;        ///< passed via Save() or Load(), no ownership, may be NULL.
    FP_CACHE*       m_cache;        ///< Footprint library cache.

    LINE_READER*    m_reader;       ///< no ownership here.
    wxString        m_filename;     ///< for saves only, name is in m_reader for loads

    int             m_loading_format_version; ///< which #SEXPR_BOARD_FILE_VERSION should be Load()ed?

    STRING_FORMATTER    m_sf;
    OUTPUTFORMATTER*    m_out;      ///< output any Format()s to this, no ownership
    int                 m_ctl;
    PCB_PARSER*         m_parser;
    NETINFO_MAPPING*    m_mapping;  ///< mapping for net codes, so only not empty net codes
                                    ///< are stored with consecutive integers as net codes

    void validateCache( const wxString& aLibraryPath, bool checkModified = true );

    const MODULE* getFootprint( const wxString& aLibraryPath, const wxString& aFootprintName,
                  const PROPERTIES* aProperties, bool checkModified );

    void init( const PROPERTIES* aProperties );

    /// formats the board setup information
    void formatSetup( BOARD* aBoard, int aNestLevel = 0 ) const;

    /// formats the defaults subsection of the board setup
    void formatDefaults( const BOARD_DESIGN_SETTINGS& aSettings, int aNestLevel ) const;

    /// formats the General section of the file
    void formatGeneral( BOARD* aBoard, int aNestLevel = 0 ) const;

    /// formats the board layer information
    void formatBoardLayers( BOARD* aBoard, int aNestLevel = 0 ) const;

    /// formats the Nets and Netclasses
    void formatNetInformation( BOARD* aBoard, int aNestLevel = 0 ) const;

    /// writes everything that comes before the board_items, like settings and layers etc
    void formatHeader( BOARD* aBoard, int aNestLevel = 0 ) const;

private:
    void format( BOARD* aBoard, int aNestLevel = 0 ) const;

    void format( DIMENSION* aDimension, int aNestLevel = 0 ) const;

    void format( EDGE_MODULE* aModuleDrawing, int aNestLevel = 0 ) const;

    void format( DRAWSEGMENT* aSegment, int aNestLevel = 0 ) const;

    void format( PCB_TARGET* aTarget, int aNestLevel = 0 ) const;

    void format( MODULE* aModule, int aNestLevel = 0 ) const;

    void format( D_PAD* aPad, int aNestLevel = 0 ) const;

    void format( TEXTE_PCB* aText, int aNestLevel = 0 ) const;

    void format( TEXTE_MODULE* aText, int aNestLevel = 0 ) const;

    void format( TRACK* aTrack, int aNestLevel = 0 ) const;

    void format( ZONE_CONTAINER* aZone, int aNestLevel = 0 ) const;

    void formatLayer( const BOARD_ITEM* aItem ) const;

    void formatLayers( LSET aLayerMask, int aNestLevel = 0 ) const;
};

#endif  // KICAD_PLUGIN_H_
