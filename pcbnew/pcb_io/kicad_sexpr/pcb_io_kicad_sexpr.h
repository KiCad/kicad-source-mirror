/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2012 CERN.
 * Copyright (C) 1992-2024 KiCad Developers, see AUTHORS.txt for contributors.
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

#ifndef PCB_IO_KICAD_SEXPR_H_
#define PCB_IO_KICAD_SEXPR_H_

#include <pcb_io/pcb_io.h>
#include <pcb_io/pcb_io_mgr.h>
#include <ctl_flags.h>

#include <richio.h>
#include <string>
#include <layer_ids.h>
#include <lset.h>
#include <boost/ptr_container/ptr_map.hpp>
#include <wx_filename.h>
#include "widgets/report_severity.h"

class BOARD;
class BOARD_ITEM;
class FP_CACHE;
class LSET;
class PCB_IO_KICAD_SEXPR_PARSER;
class NETINFO_MAPPING;
class BOARD_DESIGN_SETTINGS;
class PCB_DIMENSION_BASE;
class PCB_SHAPE;
class PCB_REFERENCE_IMAGE;
class PCB_TARGET;
class PAD;
class PADSTACK;
class PCB_GROUP;
class PCB_GENERATOR;
class PCB_TRACK;
class ZONE;
class PCB_TEXT;
class PCB_TEXTBOX;
class PCB_TABLE;
class EDA_TEXT;
class SHAPE_LINE_CHAIN;
class TEARDROP_PARAMETERS;
class PCB_IO_KICAD_SEXPR;   // forward decl

/// Current s-expression file format version.  2 was the last legacy format version.

//#define SEXPR_BOARD_FILE_VERSION    3         // first s-expression format, used legacy cu stack
//#define SEXPR_BOARD_FILE_VERSION    4         // reversed cu stack, changed Inner* to In* in reverse order
//                                              // went to 32 Cu layers from 16.
//----------------- Start of 5.0 development -----------------
//#define SEXPR_BOARD_FILE_VERSION    20160815  // differential pair settings per net class
//#define SEXPR_BOARD_FILE_VERSION    20170123  // EDA_TEXT refactor, moved 'hide'
//#define SEXPR_BOARD_FILE_VERSION    20170920  // long pad names and custom pad shape
//#define SEXPR_BOARD_FILE_VERSION    20170922  // Keepout zones can exist on multiple layers
//#define SEXPR_BOARD_FILE_VERSION    20171114  // Save 3D model offset in mm, instead of inches
//#define SEXPR_BOARD_FILE_VERSION    20171125  // Locked/unlocked FP_TEXT
//#define SEXPR_BOARD_FILE_VERSION    20171130  // 3D model offset written using "offset" parameter
//----------------- Start of 6.0 development -----------------
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
//#define SEXPR_BOARD_FILE_VERSION    20210228  // Move global margins back to board file
//#define SEXPR_BOARD_FILE_VERSION    20210424  // Correct locked flag syntax (remove parens).
//#define SEXPR_BOARD_FILE_VERSION    20210606  // Change overbar syntax from `~...~` to `~{...}`.
//#define SEXPR_BOARD_FILE_VERSION    20210623  // Add support for reading/writing arcs in polygons
//#define SEXPR_BOARD_FILE_VERSION    20210722  // Reading/writing group locked flags
//#define SEXPR_BOARD_FILE_VERSION    20210824  // Opacity in 3D colors
//#define SEXPR_BOARD_FILE_VERSION    20210925  // Locked flag for fp_text
//#define SEXPR_BOARD_FILE_VERSION    20211014  // Arc formatting
//----------------- Start of 7.0 development -----------------
//#define SEXPR_BOARD_FILE_VERSION    20211226  // Add radial dimension
//#define SEXPR_BOARD_FILE_VERSION    20211227  // Add thermal relief spoke angle overrides
//#define SEXPR_BOARD_FILE_VERSION    20211228  // Add allow_soldermask_bridges footprint attribute
//#define SEXPR_BOARD_FILE_VERSION    20211229  // Stroke formatting
//#define SEXPR_BOARD_FILE_VERSION    20211230  // Dimensions in footprints
//#define SEXPR_BOARD_FILE_VERSION    20211231  // Private footprint layers
//#define SEXPR_BOARD_FILE_VERSION    20211232  // Fonts
//#define SEXPR_BOARD_FILE_VERSION    20220131  // Textboxes
//#define SEXPR_BOARD_FILE_VERSION    20220211  // End support for V5 zone fill strategy
//#define SEXPR_BOARD_FILE_VERSION    20220225  // Remove TEDIT
//#define SEXPR_BOARD_FILE_VERSION    20220308  // Knockout text and Locked graphic text property saved
//#define SEXPR_BOARD_FILE_VERSION    20220331  // Plot on all layers selection setting
//#define SEXPR_BOARD_FILE_VERSION    20220417  // Automatic dimension precisions
//#define SEXPR_BOARD_FILE_VERSION    20220427  // Exclude Edge.Cuts & Margin from fp private layers
//#define SEXPR_BOARD_FILE_VERSION    20220609  // Add teardrop keywords to identify teardrop zones
//#define SEXPR_BOARD_FILE_VERSION    20220621  // Add Image support
//#define SEXPR_BOARD_FILE_VERSION    20220815  // Add allow-soldermask-bridges-in-FPs flag
//#define SEXPR_BOARD_FILE_VERSION    20220818  // First-class storage for net-ties
//#define SEXPR_BOARD_FILE_VERSION    20220914  // Number boxes for custom-shape pads
//#define SEXPR_BOARD_FILE_VERSION    20221018  // Via & pad zone-layer-connections
//----------------- Start of 8.0 development -----------------
//#define SEXPR_BOARD_FILE_VERSION    20230410  // DNP attribute propagated from schematic to attr
//#define SEXPR_BOARD_FILE_VERSION    20230517  // Teardrop parameters for pads and vias
//#define SEXPR_BOARD_FILE_VERSION    20230620  // PCB Fields
//#define SEXPR_BOARD_FILE_VERSION    20230730  // Connectivity for graphic shapes
//#define SEXPR_BOARD_FILE_VERSION    20230825  // Textbox explicit border flag
//#define SEXPR_BOARD_FILE_VERSION    20230906  // Multiple image type support in files
//#define SEXPR_BOARD_FILE_VERSION    20230913  // Custom-shaped-pad spoke templates
//#define SEXPR_BOARD_FILE_VERSION    20231007  // Generative objects
//#define SEXPR_BOARD_FILE_VERSION    20231014  // V8 file format normalization
//#define SEXPR_BOARD_FILE_VERSION    20231212  // Reference image locking/UUIDs, footprint boolean format
//#define SEXPR_BOARD_FILE_VERSION    20231231  // Use 'uuid' rather than 'id' for generators and groups
//#define SEXPR_BOARD_FILE_VERSION    20240108  // Convert teardrop parameters to explicit bools
//----------------- Start of 9.0 development -----------------
//#define SEXPR_BOARD_FILE_VERSION    20240201  // Use nullable properties for overrides
//#define SEXPR_BOARD_FILE_VERSION    20240202  // Tables
//#define SEXPR_BOARD_FILE_VERSION    20240225  // Rationalization of solder_paste_margin
//#define SEXPR_BOARD_FILE_VERSION    20240609  // Add 'tenting' keyword
//#define SEXPR_BOARD_FILE_VERSION    20240617  // Table angles
//#define SEXPR_BOARD_FILE_VERSION    20240703  // User layer types
//#define SEXPR_BOARD_FILE_VERSION    20240706  // Embedded Files
//#define SEXPR_BOARD_FILE_VERSION    20240819  // Embedded Files - Update hash algorithm to Murmur3
//#define SEXPR_BOARD_FILE_VERSION    20240928  // Component classes
//#define SEXPR_BOARD_FILE_VERSION    20240929  // Complex padstacks
//#define SEXPR_BOARD_FILE_VERSION    20241006  // Via stacks
//#define SEXPR_BOARD_FILE_VERSION    20241007  // Tracks can have soldermask layer and margin
//#define SEXPR_BOARD_FILE_VERSION    20241009  // Evolve placement rule areas file format
#define SEXPR_BOARD_FILE_VERSION      20241010  // Graphic shapes can have soldermask layer and margin

#define BOARD_FILE_HOST_VERSION       20200825  ///< Earlier files than this include the host tag
#define LEGACY_ARC_FORMATTING         20210925  ///< These were the last to use old arc formatting
#define LEGACY_NET_TIES               20220815  ///< These were the last to use the keywords field
                                                ///<   to indicate a net-tie.
#define FIRST_NORMALIZED_VERISON      20230924  ///< Earlier files did not have normalized bools


// common combinations of the above:

/// Format output for the clipboard instead of footprint library or BOARD
#define CTL_FOR_CLIPBOARD           (CTL_OMIT_INITIAL_COMMENTS) // (CTL_OMIT_NETS)

/// Format output for a footprint library instead of clipboard or BOARD
#define CTL_FOR_LIBRARY \
    ( CTL_OMIT_PAD_NETS | CTL_OMIT_UUIDS | CTL_OMIT_PATH | CTL_OMIT_AT | CTL_OMIT_LIBNAME )

/// The zero arg constructor when PCB_PLUGIN is used for PLUGIN::Load() and PLUGIN::Save()ing
/// a BOARD file underneath IO_MGR.
#define CTL_FOR_BOARD               (CTL_OMIT_INITIAL_COMMENTS|CTL_OMIT_FOOTPRINT_VERSION)

/**
 * Helper class for creating a footprint library cache.
 *
 * The new footprint library design is a file path of individual footprint files that contain
 * a single footprint per file.  This class is a helper only for the footprint portion of the
 * PLUGIN API, and only for the #PCB_PLUGIN plugin.  It is private to this implementation file so
 * it is not placed into a header.
 */
class FP_CACHE_ITEM
{
    WX_FILENAME                m_filename;
    std::unique_ptr<FOOTPRINT> m_footprint;

public:
    FP_CACHE_ITEM( FOOTPRINT* aFootprint, const WX_FILENAME& aFileName );

    const WX_FILENAME& GetFileName() const { return m_filename; }
    void               SetFilePath( const wxString& aFilePath ) { m_filename.SetPath( aFilePath ); }
    const FOOTPRINT*   GetFootprint() const { return m_footprint.get(); }
};

typedef boost::ptr_map<wxString, FP_CACHE_ITEM> FP_CACHE_FOOTPRINT_MAP;

class FP_CACHE
{
    PCB_IO_KICAD_SEXPR*   m_owner;        // Plugin object that owns the cache.
    wxFileName    m_lib_path;     // The path of the library.
    wxString      m_lib_raw_path; // For quick comparisons.
    FP_CACHE_FOOTPRINT_MAP m_footprints;   // Map of footprint filename to FOOTPRINT*.

    bool m_cache_dirty;          // Stored separately because it's expensive to check
                                 // m_cache_timestamp against all the files.
    long long m_cache_timestamp; // A hash of the timestamps for all the footprint
                                 // files.

public:
    FP_CACHE( PCB_IO_KICAD_SEXPR* aOwner, const wxString& aLibraryPath );

    wxString GetPath() const { return m_lib_raw_path; }

    bool IsWritable() const { return m_lib_path.IsOk() && m_lib_path.IsDirWritable(); }

    bool Exists() const { return m_lib_path.IsOk() && m_lib_path.DirExists(); }

    FP_CACHE_FOOTPRINT_MAP& GetFootprints() { return m_footprints; }

    // Most all functions in this class throw IO_ERROR exceptions.  There are no
    // error codes nor user interface calls from here, nor in any PLUGIN.
    // Catch these exceptions higher up please.

    /**
     * Save the footprint cache or a single footprint from it to disk
     *
     * @param aFootprint if set, save only this footprint, otherwise, save the full library
     */
    void Save( FOOTPRINT* aFootprint = nullptr );

    void Load();

    void Remove( const wxString& aFootprintName );

    /**
     * Generate a timestamp representing all source files in the cache (including the
     * parent directory).
     * Timestamps should not be considered ordered.  They either match or they don't.
     */
    static long long GetTimestamp( const wxString& aLibPath );

    /**
     * Return true if the cache is not up-to-date.
     */
    bool IsModified();

    /**
     * Check if \a aPath is the same as the current cache path.
     *
     * This tests paths by converting \a aPath using the native separators.  Internally
     * #FP_CACHE stores the current path using native separators.  This prevents path
     * miscompares on Windows due to the fact that paths can be stored with / instead of \\
     * in the footprint library table.
     *
     * @param aPath is the library path to test against.
     * @return true if \a aPath is the same as the cache path.
     */
    bool IsPath( const wxString& aPath ) const;

    void SetPath( const wxString& aPath );
};


/**
 * A #PLUGIN derivation for saving and loading Pcbnew s-expression formatted files.
 *
 * @note This class is not thread safe, but it is re-entrant multiple times in sequence.
 */
class PCB_IO_KICAD_SEXPR : public PCB_IO
{
public:
    const IO_BASE::IO_FILE_DESC GetBoardFileDesc() const override
    {
        // Would have used wildcards_and_files_ext.cpp's KiCadPcbFileExtension,
        // but to be pure, a plugin should not assume that it will always be linked
        // with the core of the Pcbnew code. (Might someday be a DLL/DSO.)  Besides,
        // file extension policy should be controlled by the plugin.
        return IO_BASE::IO_FILE_DESC( _HKI( "KiCad printed circuit board files" ), { "kicad_pcb" } );
    }

    const IO_BASE::IO_FILE_DESC GetLibraryFileDesc() const override
    {
        return IO_BASE::IO_FILE_DESC( _HKI( "KiCad footprint file" ), { "kicad_mod" } );
    }

    const IO_BASE::IO_FILE_DESC GetLibraryDesc() const override
    {
        return IO_BASE::IO_FILE_DESC( _HKI( "KiCad footprint files" ), {}, { "kicad_mod" }, false );
    }

    void SetQueryUserCallback( std::function<bool( wxString aTitle, int aIcon, wxString aMessage,
                                                   wxString aOKButtonTitle )> aCallback ) override
    {
        m_queryUserCallback = std::move( aCallback );
    }

    bool CanReadBoard( const wxString& aFileName ) const override;

    void SaveBoard( const wxString& aFileName, BOARD* aBoard,
                    const std::map<std::string, UTF8>* aProperties = nullptr ) override;

    BOARD* LoadBoard( const wxString& aFileName, BOARD* aAppendToMe,
                      const std::map<std::string, UTF8>* aProperties = nullptr, PROJECT* aProject = nullptr ) override;

    BOARD* DoLoad( LINE_READER& aReader, BOARD* aAppendToMe, const std::map<std::string, UTF8>* aProperties,
                     PROGRESS_REPORTER* aProgressReporter, unsigned aLineCount );

    void FootprintEnumerate( wxArrayString& aFootprintNames, const wxString& aLibraryPath,
                             bool aBestEfforts, const std::map<std::string, UTF8>* aProperties = nullptr ) override;

    const FOOTPRINT* GetEnumeratedFootprint( const wxString& aLibraryPath,
                                             const wxString& aFootprintName,
                                             const std::map<std::string, UTF8>* aProperties = nullptr ) override;

    bool FootprintExists( const wxString& aLibraryPath, const wxString& aFootprintName,
                          const std::map<std::string, UTF8>* aProperties = nullptr ) override;

    FOOTPRINT* ImportFootprint( const wxString& aFootprintPath, wxString& aFootprintNameOut,
                                const std::map<std::string, UTF8>* aProperties = nullptr ) override;

    FOOTPRINT* FootprintLoad( const wxString& aLibraryPath, const wxString& aFootprintName,
                              bool  aKeepUUID = false,
                              const std::map<std::string, UTF8>* aProperties = nullptr ) override;

    void FootprintSave( const wxString& aLibraryPath, const FOOTPRINT* aFootprint,
                        const std::map<std::string, UTF8>* aProperties = nullptr ) override;

    void FootprintDelete( const wxString& aLibraryPath, const wxString& aFootprintName,
                          const std::map<std::string, UTF8>* aProperties = nullptr ) override;

    long long GetLibraryTimestamp( const wxString& aLibraryPath ) const override;

    void CreateLibrary( const wxString& aLibraryPath,
                        const std::map<std::string, UTF8>* aProperties = nullptr) override;

    bool DeleteLibrary( const wxString& aLibraryPath,
                        const std::map<std::string, UTF8>* aProperties = nullptr ) override;

    bool IsLibraryWritable( const wxString& aLibraryPath ) override;

    PCB_IO_KICAD_SEXPR( int aControlFlags = CTL_FOR_BOARD );

    virtual ~PCB_IO_KICAD_SEXPR();

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
                                   const std::map<std::string, UTF8>* aProperties, bool checkModified );

    void init( const std::map<std::string, UTF8>* aProperties );

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

    void formatTeardropParameters( const TEARDROP_PARAMETERS& tdParams, int aNestLevel = 0 ) const;

private:
    void format( const BOARD* aBoard, int aNestLevel = 0 ) const;

    void format( const PCB_DIMENSION_BASE* aDimension, int aNestLevel = 0 ) const;

    void format( const PCB_REFERENCE_IMAGE* aBitmap, int aNestLevel = 0 ) const;

    void format( const PCB_GROUP* aGroup, int aNestLevel = 0 ) const;

    void format( const PCB_SHAPE* aSegment, int aNestLevel = 0 ) const;

    void format( const PCB_TARGET* aTarget, int aNestLevel = 0 ) const;

    void format( const FOOTPRINT* aFootprint, int aNestLevel = 0 ) const;

    void format( const PAD* aPad, int aNestLevel = 0 ) const;

    void format( const PCB_TEXT* aText, int aNestLevel = 0 ) const;
    void format( const PCB_TEXTBOX* aTextBox, int aNestLevel = 0 ) const;

    void format( const PCB_TABLE* aTable, int aNestLevel = 0 ) const;

    void format( const PCB_GENERATOR* aGenerator, int aNestLevel = 0 ) const;

    void format( const PCB_TRACK* aTrack, int aNestLevel = 0 ) const;

    void format( const ZONE* aZone, int aNestLevel = 0 ) const;

    void formatPolyPts( const SHAPE_LINE_CHAIN& outline, int aNestLevel, bool aCompact,
                        const FOOTPRINT* aParentFP = nullptr ) const;

    void formatRenderCache( const EDA_TEXT* aText, int aNestLevel ) const;

    void formatLayer( PCB_LAYER_ID aLayer, bool aIsKnockout = false ) const;

    void formatLayers( LSET aLayerMask, int aNestLevel = 0 ) const;

    void formatTenting( const PADSTACK& aPadstack ) const;

    friend class FP_CACHE;

protected:
    wxString               m_error;      ///< for throwing exceptions

    FP_CACHE*              m_cache;      ///< Footprint library cache

    LINE_READER*           m_reader;     ///< no ownership
    wxString               m_filename;   ///< for saves only, name is in m_reader for loads

    STRING_FORMATTER       m_sf;
    OUTPUTFORMATTER*       m_out;        ///< output any Format()s to this, no ownership
    int                    m_ctl;
    NETINFO_MAPPING*       m_mapping;    ///< mapping for net codes, so only not empty net codes
                                         ///< are stored with consecutive integers as net codes

    std::function<bool( wxString aTitle, int aIcon, wxString aMsg, wxString aAction )> m_queryUserCallback;
};

#endif  // PCB_IO_KICAD_SEXPR_H_
