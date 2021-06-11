/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2012 CERN.
 * Copyright (C) 1992-2020 KiCad Developers, see AUTHORS.txt for contributors.
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
class PCB_DIMENSION_BASE;
class FP_SHAPE;
class PCB_SHAPE;
class PCB_TARGET;
class PAD;
class FP_TEXT;
class PCB_GROUP;
class TRACK;
class ZONE;
class PCB_TEXT;


/// Current s-expression file format version.  2 was the last legacy format version.

//#define SEXPR_BOARD_FILE_VERSION    3         // first s-expression format, used legacy cu stack
//#define SEXPR_BOARD_FILE_VERSION    4         // reversed cu stack, changed Inner* to In* in reverse order
//                                              // went to 32 Cu layers from 16.
//#define SEXPR_BOARD_FILE_VERSION    20160815  // differential pair settings per net class
//#define SEXPR_BOARD_FILE_VERSION    20170123  // EDA_TEXT refactor, moved 'hide'
//#define SEXPR_BOARD_FILE_VERSION    20170920  // long pad names and custom pad shape
//#define SEXPR_BOARD_FILE_VERSION    20170922  // Keepout zones can exist on multiple layers
//#define SEXPR_BOARD_FILE_VERSION    20171114  // Save 3D model offset in mm, instead of inches
//#define SEXPR_BOARD_FILE_VERSION    20171125  // Locked/unlocked FP_TEXT
//#define SEXPR_BOARD_FILE_VERSION    20171130  // 3D model offset written using "offset" parameter
//#define SEXPR_BOARD_FILE_VERSION    20190331  // hatched zones and chamfered round rect pads
//#define SEXPR_BOARD_FILE_VERSION    20190421  // curves in custom pads
//#define SEXPR_BOARD_FILE_VERSION    20190516  // Remove segment count from zones
//#define SEXPR_BOARD_FILE_VERSION    20190605  // Add layer defaults
//#define SEXPR_BOARD_FILE_VERSION    20190905  // Add board physical stackup info in setup section
//#define SEXPR_BOARD_FILE_VERSION    20190907  // Keepout areas in footprints
//#define SEXPR_BOARD_FILE_VERSION    20191123  // pin function in pads
//#define SEXPR_BOARD_FILE_VERSION    20200104  // pad property for fabrication
//#define SEXPR_BOARD_FILE_VERSION    20200119  // arcs in tracks
//#define SEXPR_BOARD_FILE_VERSION    20200512  // page -> paper
//#define SEXPR_BOARD_FILE_VERSION    20200518  // save hole_to_hole_min
//#define SEXPR_BOARD_FILE_VERSION    20200614  // Add support for fp_rects and gr_rects
//#define SEXPR_BOARD_FILE_VERSION    20200625  // Multilayer zones, zone names, island controls
//#define SEXPR_BOARD_FILE_VERSION    20200628  // remove visibility settings
//#define SEXPR_BOARD_FILE_VERSION    20200724  // Add KIID to footprints
//#define SEXPR_BOARD_FILE_VERSION    20200807  // Add zone hatch advanced settings
//#define SEXPR_BOARD_FILE_VERSION    20200808  // Add properties to footprints
//#define SEXPR_BOARD_FILE_VERSION    20200809  // Add REMOVE_UNUSED_LAYERS option to vias and THT pads
//#define SEXPR_BOARD_FILE_VERSION    20200811  // Add groups
//#define SEXPR_BOARD_FILE_VERSION    20200818  // Remove Status flag bitmap and setup counts
//#define SEXPR_BOARD_FILE_VERSION    20200819  // Add board-level properties
//#define SEXPR_BOARD_FILE_VERSION    20200825  // Remove host information
//#define SEXPR_BOARD_FILE_VERSION    20200828  // Add new fabrication attributes
//#define SEXPR_BOARD_FILE_VERSION    20200829  // Remove library name from exported footprints
//#define SEXPR_BOARD_FILE_VERSION    20200909  // Change DIMENSION format
//#define SEXPR_BOARD_FILE_VERSION    20200913  // Add leader dimension
//#define SEXPR_BOARD_FILE_VERSION    20200916  // Add center dimension
//#define SEXPR_BOARD_FILE_VERSION    20200921  // Add orthogonal dimension
//#define SEXPR_BOARD_FILE_VERSION    20200922  // Add user name to layer definition.
//#define SEXPR_BOARD_FILE_VERSION    20201002  // Add groups in footprints (for footprint editor).
//#define SEXPR_BOARD_FILE_VERSION    20201114  // Add first-class support for filled shapes.
//#define SEXPR_BOARD_FILE_VERSION    20201115  // module -> footprint and change fill syntax.
//#define SEXPR_BOARD_FILE_VERSION    20201116  // Write version and generator string in footprint files.
//#define SEXPR_BOARD_FILE_VERSION    20201220  // Add free via token
//#define SEXPR_BOARD_FILE_VERSION    20210108  // Pad locking moved from footprint to pads
//#define SEXPR_BOARD_FILE_VERSION    20210126  // Store pintype alongside pinfunction (in pads).
//#define SEXPR_BOARD_FILE_VERSION      20210228  // Move global margins back to board file
#define SEXPR_BOARD_FILE_VERSION      20210424  // Correct locked flag syntax (remove parens).

#define BOARD_FILE_HOST_VERSION       20200825  ///< Earlier files than this include the host tag

#define CTL_OMIT_PAD_NETS           (1 << 1)    ///< Omit pads net names (useless in library)
#define CTL_OMIT_TSTAMPS            (1 << 2)    ///< Omit component time stamp (useless in library)
#define CTL_OMIT_INITIAL_COMMENTS   (1 << 3)    ///< omit FOOTPRINT initial comments
#define CTL_OMIT_PATH               (1 << 4)    ///< Omit component sheet time stamp (useless in library)
#define CTL_OMIT_AT                 (1 << 5)    ///< Omit position and rotation. (always saved
                                                ///< with potion 0,0 and rotation = 0 in library).
//#define CTL_OMIT_HIDE             (1 << 6)    // found and defined in eda_text.h
#define CTL_OMIT_LIBNAME            (1 << 7)    ///< Omit lib alias when saving (used for
                                                ///< board/not library).
#define CTL_OMIT_FOOTPRINT_VERSION  (1 << 8)    ///< Omit the version string from the (footprint)
                                                ///<sexpr group

// common combinations of the above:

/// Format output for the clipboard instead of footprint library or BOARD
#define CTL_FOR_CLIPBOARD           (CTL_OMIT_INITIAL_COMMENTS) // (CTL_OMIT_NETS)

/// Format output for a footprint library instead of clipboard or BOARD
#define CTL_FOR_LIBRARY                                                                    \
    ( CTL_OMIT_PAD_NETS | CTL_OMIT_TSTAMPS | CTL_OMIT_PATH | CTL_OMIT_AT | CTL_OMIT_LIBNAME )

/// The zero arg constructor when PCB_IO is used for PLUGIN::Load() and PLUGIN::Save()ing
/// a BOARD file underneath IO_MGR.
#define CTL_FOR_BOARD               (CTL_OMIT_INITIAL_COMMENTS|CTL_OMIT_FOOTPRINT_VERSION)


/**
 * A #PLUGIN derivation for saving and loading Pcbnew s-expression formatted files.
 *
 * @note This class is not thread safe, but it is re-entrant multiple times in sequence.
 */
class PCB_IO : public PLUGIN
{
public:
    const wxString PluginName() const override
    {
        return wxT( "KiCad" );
    }

    const wxString GetFileExtension() const override
    {
        // Would have used wildcards_and_files_ext.cpp's KiCadPcbFileExtension,
        // but to be pure, a plugin should not assume that it will always be linked
        // with the core of the Pcbnew code. (Might someday be a DLL/DSO.)  Besides,
        // file extension policy should be controlled by the plugin.
        return wxT( "kicad_pcb" );
    }

    virtual void Save( const wxString& aFileName, BOARD* aBoard,
                       const PROPERTIES* aProperties = nullptr ) override;

    BOARD* Load( const wxString& aFileName, BOARD* aAppendToMe,
                 const PROPERTIES* aProperties = nullptr, PROJECT* aProject = nullptr ) override;

    BOARD* DoLoad( LINE_READER& aReader, BOARD* aAppendToMe, const PROPERTIES* aProperties );

    void FootprintEnumerate( wxArrayString& aFootprintNames, const wxString& aLibraryPath,
                             bool aBestEfforts, const PROPERTIES* aProperties = nullptr ) override;

    const FOOTPRINT* GetEnumeratedFootprint( const wxString& aLibraryPath,
                                             const wxString& aFootprintName,
                                             const PROPERTIES* aProperties = nullptr ) override;

    bool FootprintExists( const wxString& aLibraryPath, const wxString& aFootprintName,
                          const PROPERTIES* aProperties = nullptr ) override;

    FOOTPRINT* FootprintLoad( const wxString& aLibraryPath, const wxString& aFootprintName,
                              bool  aKeepUUID = false,
                              const PROPERTIES* aProperties = nullptr ) override;

    void FootprintSave( const wxString& aLibraryPath, const FOOTPRINT* aFootprint,
                        const PROPERTIES* aProperties = nullptr ) override;

    void FootprintDelete( const wxString& aLibraryPath, const wxString& aFootprintName,
                          const PROPERTIES* aProperties = nullptr ) override;

    long long GetLibraryTimestamp( const wxString& aLibraryPath ) const override;

    void FootprintLibCreate( const wxString& aLibraryPath,
                             const PROPERTIES* aProperties = nullptr) override;

    bool FootprintLibDelete( const wxString& aLibraryPath,
                             const PROPERTIES* aProperties = nullptr ) override;

    bool IsFootprintLibWritable( const wxString& aLibraryPath ) override;

    PCB_IO( int aControlFlags = CTL_FOR_BOARD );

    virtual ~PCB_IO();

    /**
     * Output \a aItem to \a aFormatter in s-expression format.
     *
     * @param aItem A pointer the an #BOARD_ITEM object to format.
     * @param aNestLevel The indentation nest level.
     * @throw IO_ERROR on write error.
     */
    void Format( const BOARD_ITEM* aItem, int aNestLevel = 0 ) const;

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
    void validateCache( const wxString& aLibraryPath, bool checkModified = true );

    const FOOTPRINT* getFootprint( const wxString& aLibraryPath, const wxString& aFootprintName,
                                   const PROPERTIES* aProperties, bool checkModified );

    void init( const PROPERTIES* aProperties );

    /// formats the board setup information
    void formatSetup( const BOARD* aBoard, int aNestLevel = 0 ) const;

    /// formats the General section of the file
    void formatGeneral( const BOARD* aBoard, int aNestLevel = 0 ) const;

    /// formats the board layer information
    void formatBoardLayers( const BOARD* aBoard, int aNestLevel = 0 ) const;

    /// formats the Nets and Netclasses
    void formatNetInformation( const BOARD* aBoard, int aNestLevel = 0 ) const;

    /// formats the Nets and Netclasses
    void formatProperties( const BOARD* aBoard, int aNestLevel = 0 ) const;

    /// writes everything that comes before the board_items, like settings and layers etc
    void formatHeader( const BOARD* aBoard, int aNestLevel = 0 ) const;

private:
    void format( const BOARD* aBoard, int aNestLevel = 0 ) const;

    void format( const PCB_DIMENSION_BASE* aDimension, int aNestLevel = 0 ) const;

    void format( const FP_SHAPE* aFPShape, int aNestLevel = 0 ) const;

    void format( const PCB_GROUP* aGroup, int aNestLevel = 0 ) const;

    void format( const PCB_SHAPE* aSegment, int aNestLevel = 0 ) const;

    void format( const PCB_TARGET* aTarget, int aNestLevel = 0 ) const;

    void format( const FOOTPRINT* aFootprint, int aNestLevel = 0 ) const;

    void format( const PAD* aPad, int aNestLevel = 0 ) const;

    void format( const PCB_TEXT* aText, int aNestLevel = 0 ) const;

    void format( const FP_TEXT* aText, int aNestLevel = 0 ) const;

    void format( const TRACK* aTrack, int aNestLevel = 0 ) const;

    void format( const ZONE* aZone, int aNestLevel = 0 ) const;

    void formatLayer( const BOARD_ITEM* aItem ) const;

    void formatLayers( LSET aLayerMask, int aNestLevel = 0 ) const;

    friend class FP_CACHE;

protected:
    wxString        m_error;        ///< for throwing exceptions
    BOARD*          m_board;        ///< which BOARD, no ownership here

    const
    PROPERTIES*     m_props;        ///< passed via Save() or Load(), no ownership, may be NULL.
    FP_CACHE*       m_cache;        ///< Footprint library cache.

    LINE_READER*    m_reader;       ///< no ownership here.
    wxString        m_filename;     ///< for saves only, name is in m_reader for loads

    STRING_FORMATTER    m_sf;
    OUTPUTFORMATTER*    m_out;      ///< output any Format()s to this, no ownership
    int                 m_ctl;
    PCB_PARSER*         m_parser;
    NETINFO_MAPPING*    m_mapping;  ///< mapping for net codes, so only not empty net codes
                                    ///< are stored with consecutive integers as net codes
};

#endif  // KICAD_PLUGIN_H_
