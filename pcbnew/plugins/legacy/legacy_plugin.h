#ifndef LEGACY_PLUGIN_H_
#define LEGACY_PLUGIN_H_

/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2012 SoftPLC Corporation, Dick Hollenbeck <dick@softplc.com>
 * Copyright (C) 2016-2021 KiCad Developers, see AUTHORS.txt for contributors.
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

#include <io_mgr.h>
#include <string>
#include <layer_ids.h>
#include <memory>


// FOOTPRINT_LIBRARY_HEADER_CNT gives the number of characters to compare to detect
// a footprint library. A few variants may have been used, and so we can only be
// sure that the header contains "PCBNEW-LibModule-V", not "PCBNEW-LibModule-V1".

#define FOOTPRINT_LIBRARY_HEADER       "PCBNEW-LibModule-V1"
#define FOOTPRINT_LIBRARY_HEADER_CNT    18

class PCB_TARGET;
class FOOTPRINT;
class NETCLASS;
class NETCLASSES;
class ZONE;
class PCB_DIMENSION_BASE;
class NETINFO_ITEM;
class FP_TEXT;
class PAD;
struct LP_CACHE;


/**
 * A #PLUGIN derivation which could possibly be put into a DLL/DSO.
 *
 * As with any PLUGIN, there is no UI, i.e. windowing calls allowed.
 */
class LEGACY_PLUGIN : public PLUGIN
{
    friend struct LP_CACHE;

public:
    LEGACY_PLUGIN();
    ~LEGACY_PLUGIN();

    const wxString PluginName() const override
    {
        return wxT( "KiCad-Legacy" );
    }

    const wxString GetFileExtension() const override
    {
        return wxT( "brd" );
    }

    BOARD* Load( const wxString& aFileName, BOARD* aAppendToMe,
                 const PROPERTIES* aProperties = nullptr, PROJECT* aProject = nullptr,
                 PROGRESS_REPORTER* aProgressReporter = nullptr ) override;

    void FootprintEnumerate( wxArrayString& aFootprintNames, const wxString& aLibraryPath,
                             bool aBestEfforts,
                             const PROPERTIES* aProperties = nullptr ) override;

    FOOTPRINT* FootprintLoad( const wxString& aLibraryPath, const wxString& aFootprintName,
                              bool  aKeepUUID = false,
                              const PROPERTIES* aProperties = nullptr ) override;

    bool FootprintLibDelete( const wxString& aLibraryPath,
                             const PROPERTIES* aProperties = nullptr ) override;

    long long GetLibraryTimestamp( const wxString& aLibraryPath ) const override;

    bool IsFootprintLibWritable( const wxString& aLibraryPath ) override;

    typedef int BIU;

    void SetReader( LINE_READER* aReader )      { m_reader = aReader; }

    // return the new .kicad_pcb layer id from the old (legacy) layer id
    static PCB_LAYER_ID leg_layer2new( int cu_count, LAYER_NUM aLayerNum );

    static LSET leg_mask2new( int cu_count, unsigned aMask );

protected:
    /// initialize PLUGIN like a constructor would, and futz with fresh BOARD if needed.
    void init( const PROPERTIES* aProperties );

    void checkpoint();

    ///< Converts net code using the mapping table if available,
    ///< otherwise returns unchanged net code
    inline int getNetCode( int aNetCode )
    {
        if( (unsigned int) aNetCode < m_netCodes.size() )
            return m_netCodes[aNetCode];

        return aNetCode;
    }

    /**
     * Parse an ASCII decimal floating point value and scales it into a BIU according to the
     * current value of diskToBui.
     *
     * This function is the complement of #fmtBIU().  One has to know what the other is doing.
     *
     * @param aValue is the ASCII value in C locale form with possible leading whitespace
     * @param nptrptr may be NULL, but if not, then it tells where to put a pointer to the
     *                next unconsumed input text. See "man strtod" for more information.
     * @return the converted Board Internal Unit.
     */
    BIU biuParse( const char* aValue, const char** nptrptr = nullptr );

    /**
     * Parse an ASCII decimal floating point value which is certainly an angle.
     *
     * This is a dedicated function for encapsulating support for the migration from
     * tenths of degrees to degrees in floating point.  This function is the complement of
     * fmtDEG().  One has to know what the other is doing.
     *
     * @param aValue is the ASCII value in C locale form with possible leading whitespace.
     * @param nptrptr may be NULL, but if not, then it tells where to put a pointer to the
     *                next unconsumed input text. See "man strtod" for more information.
     * @return the string converted to a primitive double type
     */
    double degParse( const char* aValue, const char** nptrptr = nullptr );

    void checkVersion();

    void loadAllSections( bool doAppend );


    void loadGENERAL();
    void loadSETUP();
    void loadSHEET();

    void load3D( FOOTPRINT* aFootprint );
    void loadPAD( FOOTPRINT* aFootprint );
    void loadMODULE_TEXT( FP_TEXT* aText );
    void loadFP_SHAPE( FOOTPRINT* aFootprint );

    void loadPCB_LINE();
    void loadNETINFO_ITEM();
    void loadPCB_TEXT();
    void loadNETCLASS();
    void loadFOOTPRINT( FOOTPRINT* aFootprint );

    /**
     * Read a list of segments (Tracks and Vias, or Segzones)
     *
     * @param aStructType is either PCB_TRACE_T to indicate tracks and vias, or NOT_USED
     *                    to indicate oldschool zone segments (which are discarded).
     */
    void loadTrackList( int aStructType );

    void loadZONE_CONTAINER();      // "$CZONE_OUTLINE"
    void loadDIMENSION();           // "$COTATION"
    void loadPCB_TARGET();          // "$PCB_TARGET"

    /// we only cache one footprint library for now, this determines which one.
    void cacheLib( const wxString& aLibraryPath );

protected:
    int                m_cu_count;

    wxString           m_error;                  ///< for throwing exceptions
    BOARD*             m_board;                  ///< which BOARD, no ownership here
    const PROPERTIES*  m_props;                  ///< passed via Save() or Load(), no ownership,
                                                 ///<  may be NULL.
    PROGRESS_REPORTER* m_progressReporter;       ///< may be NULL, no ownership
    unsigned           m_lastProgressLine;
    unsigned           m_lineCount;              ///< for progress reporting

    LINE_READER*       m_reader;                 ///< no ownership here.
    FILE*              m_fp;                     ///< no ownership here.

    wxString           m_field;                  ///< reused to stuff FOOTPRINT fields.
    int                m_loading_format_version; ///< which BOARD_FORMAT_VERSION am I Load()ing?
    LP_CACHE*          m_cache;
    bool               m_showLegacyZoneWarning;

    std::vector<int>   m_netCodes;               ///< net codes mapping for boards being loaded

    double  biuToDisk;              ///< convert from BIUs to disk engineering units
                                    ///< with this scale factor

    double  diskToBiu;              ///< convert from disk engineering units to BIUs
                                    ///< with this scale factor
};

#endif  // LEGACY_PLUGIN_H_
