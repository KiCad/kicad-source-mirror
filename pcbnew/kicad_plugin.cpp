/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2012 CERN
 * Copyright (C) 1992-2011 KiCad Developers, see change_log.txt for contributors.
 *
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

#include <fctsys.h>
#include <kicad_string.h>
#include <common.h>
#include <build_version.h>      // LEGACY_BOARD_FILE_VERSION
#include <macros.h>
#include <3d_struct.h>
#include <wildcards_and_files_ext.h>

#include <class_board.h>
#include <class_module.h>
#include <class_pcb_text.h>
#include <class_dimension.h>
#include <class_track.h>
#include <class_zone.h>
#include <class_drawsegment.h>
#include <class_mire.h>
#include <class_edge_mod.h>
#include <pcb_plot_params.h>
#include <zones.h>
#include <kicad_plugin.h>
#include <pcb_parser.h>

#include <wx/dir.h>
#include <wx/filename.h>
#include <wx/wfstream.h>
#include <boost/ptr_container/ptr_map.hpp>
#include <memory.h>

using namespace std;


#define FMTIU        BOARD_ITEM::FormatInternalUnits

/**
 * Definition for enabling and disabling footprint library trace output.  See the
 * wxWidgets documentation on useing the WXTRACE environment variable.
 */
static const wxString traceFootprintLibrary( wxT( "KicadFootprintLib" ) );


/**
 * Class FP_CACHE_ITEM
 * is helper class for creating a footprint library cache.
 *
 * The new footprint library design is a file path of individual module files
 * that contain a single module per file.  This class is a helper only for the
 * footprint portion of the PLUGIN API, and only for the #PCB_IO plugin.  It is
 * private to this implementation file so it is not placed into a header.
 */
class FP_CACHE_ITEM
{
    wxFileName              m_file_name; ///< The the full file name and path of the footprint to cache.
    bool                    m_writable;  ///< Writability status of the footprint file.
    wxDateTime              m_mod_time;  ///< The last file modified time stamp.
    auto_ptr< MODULE >      m_module;

public:
    FP_CACHE_ITEM( MODULE* aModule, const wxFileName& aFileName );

    wxString    GetName() const { return m_file_name.GetDirs().Last(); }
    wxFileName  GetFileName() const { return m_file_name; }
    bool        IsModified() const;
    MODULE*     GetModule() const { return m_module.get(); }
    void        UpdateModificationTime() { m_mod_time = m_file_name.GetModificationTime(); }
};


FP_CACHE_ITEM::FP_CACHE_ITEM( MODULE* aModule, const wxFileName& aFileName ) :
    m_module( aModule )
{
    m_file_name = aFileName;
    m_mod_time.Now();
}


bool FP_CACHE_ITEM::IsModified() const
{
    return m_file_name.GetModificationTime() != m_mod_time;
}


typedef boost::ptr_map< std::string, FP_CACHE_ITEM >  MODULE_MAP;
typedef MODULE_MAP::iterator                          MODULE_ITER;
typedef MODULE_MAP::const_iterator                    MODULE_CITER;


class FP_CACHE
{
    PCB_IO*         m_owner;        /// Plugin object that owns the cache.
    wxFileName      m_lib_path;     /// The path of the library.
    wxDateTime      m_mod_time;     /// Footprint library path modified time stamp.
    MODULE_MAP      m_modules;      /// Map of footprint file name per MODULE*.

public:
    FP_CACHE( PCB_IO* aOwner, const wxString& aLibraryPath );

    wxString GetPath() const { return m_lib_path.GetPath(); }
    wxDateTime GetLastModificationTime() const { return m_mod_time; }
    bool IsWritable() const { return m_lib_path.IsOk() && m_lib_path.IsDirWritable(); }
    MODULE_MAP& GetModules() { return m_modules; }

    // Most all functions in this class throw IO_ERROR exceptions.  There are no
    // error codes nor user interface calls from here, nor in any PLUGIN.
    // Catch these exceptions higher up please.

    /// save the entire legacy library to m_lib_name;
    void Save();

    void Load();

    void Remove( const wxString& aFootprintName );

    wxDateTime GetLibModificationTime();

    bool IsModified();
};


FP_CACHE::FP_CACHE( PCB_IO* aOwner, const wxString& aLibraryPath )
{
    m_owner = aOwner;
    m_lib_path.SetPath( aLibraryPath );
}


wxDateTime FP_CACHE::GetLibModificationTime()
{
    return m_lib_path.GetModificationTime();
}


void FP_CACHE::Save()
{
    if( !m_lib_path.DirExists() && !m_lib_path.Mkdir() )
    {
        THROW_IO_ERROR( wxString::Format( _( "Cannot create footprint library path <%s>" ),
                                          m_lib_path.GetPath().GetData() ) );
    }

    if( !m_lib_path.IsDirWritable() )
    {
        THROW_IO_ERROR( wxString::Format( _( "Footprint library path <%s> is read only" ),
                                          GetChars( m_lib_path.GetPath() ) ) );
    }

    for( MODULE_ITER it = m_modules.begin();  it != m_modules.end();  ++it )
    {
        wxFileName fn = it->second->GetFileName();

        if( fn.FileExists() && !it->second->IsModified() )
            continue;

        wxString tempFileName = fn.CreateTempFileName( fn.GetPath() );

        // Allow file output stream to go out of scope to close the file stream before
        // renaming the file.
        {
            wxLogTrace( traceFootprintLibrary, wxT( "Creating temporary library file %s" ),
                        GetChars( tempFileName ) );

            FILE_OUTPUTFORMATTER formatter( tempFileName );

            m_owner->SetOutputFormatter( &formatter );
            m_owner->Format( (BOARD_ITEM*) it->second->GetModule() );
        }

        wxRemove( fn.GetFullPath() );     // it is not an error if this does not exist

        if( wxRename( tempFileName, fn.GetFullPath() ) )
        {
            THROW_IO_ERROR( wxString::Format( _( "Cannot rename temporary file <%s> to footprint library file <%s>" ),
                                              tempFileName.GetData(),
                                              fn.GetFullPath().GetData() ) );
        }

        it->second->UpdateModificationTime();
        m_mod_time = GetLibModificationTime();
    }
}


void FP_CACHE::Load()
{
    wxDir dir( m_lib_path.GetPath() );

    if( !dir.IsOpened() )
    {
        THROW_IO_ERROR( wxString::Format( _( "Footprint library path <%s> does not exist" ),
                                          m_lib_path.GetPath().GetData() ) );
    }

    wxString fpFileName;
    wxString wildcard = wxT( "*." ) + KiCadFootprintFileExtension;

    if( !dir.GetFirst( &fpFileName, wildcard, wxDIR_FILES ) )
        return;

    do
    {
        // reader now owns fp, will close on exception or return
        FILE_LINE_READER    reader( fpFileName );

        m_owner->m_parser->SetLineReader( &reader );

        std::string name = TO_UTF8( fpFileName );

        m_modules.insert( name, new FP_CACHE_ITEM( (MODULE*) m_owner->m_parser->Parse(), fpFileName ) );

    } while( dir.GetNext( &fpFileName ) );

    // Remember the file modification time of library file when the
    // cache snapshot was made, so that in a networked environment we will
    // reload the cache as needed.
    m_mod_time = GetLibModificationTime();
}


void FP_CACHE::Remove( const wxString& aFootprintName )
{

    std::string footprintName = TO_UTF8( aFootprintName );

    MODULE_CITER it = m_modules.find( footprintName );

    if( it == m_modules.end() )
    {
        THROW_IO_ERROR( wxString::Format( _( "library <%s> has no footprint %s to delete" ),
                                          m_lib_path.GetPath().GetData(),
                                          aFootprintName.GetData() ) );
    }

    // Remove the module from the cache and delete the module file from the library.
    wxString fullPath = it->second->GetFileName().GetFullPath();
    m_modules.erase( footprintName );
    wxRemoveFile( fullPath );
}


bool FP_CACHE::IsModified()
{
    if( !m_lib_path.DirExists() )
        return true;

    for( MODULE_ITER it = m_modules.begin();  it != m_modules.end();  ++it )
    {
        wxFileName fn = it->second->GetFileName();

        if( !fn.FileExists() )
        {
            wxLogTrace( traceFootprintLibrary, wxT( "Footprint cache file '%s' does not exist." ),
                        fn.GetFullPath().GetData() );
            return true;
        }

        if( it->second->IsModified() )
        {
            wxLogTrace( traceFootprintLibrary,
                        wxT( "Footprint cache file '%s' has been modified." ),
                        fn.GetFullPath().GetData() );
            return true;
        }
    }

    return false;
}


void PCB_IO::Save( const wxString& aFileName, BOARD* aBoard, PROPERTIES* aProperties )
{
    LOCALE_IO   toggle;     // toggles on, then off, the C locale.

    m_board = aBoard;

    FILE_OUTPUTFORMATTER    formatter( aFileName );

    m_out = &formatter;     // no ownership

    m_out->Print( 0, "(kicad_pcb (version %d) (host pcbnew %s)\n", SEXPR_BOARD_FILE_VERSION,
                  formatter.Quotew( GetBuildVersion() ).c_str() );

    Format( aBoard, 1 );

    m_out->Print( 0, ")\n" );
}


BOARD_ITEM* PCB_IO::Parse( const wxString& aClipboardSourceInput ) throw( PARSE_ERROR, IO_ERROR )
{
    std::string input = TO_UTF8( aClipboardSourceInput );

    STRING_LINE_READER  reader( input, wxT( "clipboard" ) );

    m_parser->SetLineReader( &reader );

    return m_parser->Parse();
}


void PCB_IO::Format( BOARD_ITEM* aItem, int aNestLevel ) const
    throw( IO_ERROR )
{
    LOCALE_IO   toggle;     // public API function, perform anything convenient for caller

    switch( aItem->Type() )
    {
    case PCB_T:
        format( (BOARD*) aItem, aNestLevel );
        break;

    case PCB_DIMENSION_T:
        format( ( DIMENSION*) aItem, aNestLevel );
        break;

    case PCB_LINE_T:
        format( (DRAWSEGMENT*) aItem, aNestLevel );
        break;

    case PCB_MODULE_EDGE_T:
        format( (EDGE_MODULE*) aItem, aNestLevel );
        break;

    case PCB_TARGET_T:
        format( (PCB_TARGET*) aItem, aNestLevel );
        break;

    case PCB_MODULE_T:
        format( (MODULE*) aItem, aNestLevel );
        break;

    case PCB_PAD_T:
        format( (D_PAD*) aItem, aNestLevel );
        break;

    case PCB_TEXT_T:
        format( (TEXTE_PCB*) aItem, aNestLevel );
        break;

    case PCB_MODULE_TEXT_T:
        format( (TEXTE_MODULE*) aItem, aNestLevel );
        break;

    case PCB_TRACE_T:
    case PCB_VIA_T:
        format( (TRACK*) aItem, aNestLevel );
        break;

    case PCB_ZONE_AREA_T:
        format( (ZONE_CONTAINER*) aItem, aNestLevel );
        break;

    default:
        wxFAIL_MSG( wxT( "Cannot format item " ) + aItem->GetClass() );
    }
}


void PCB_IO::formatLayer( const BOARD_ITEM* aItem ) const
{
    if( m_ctl & CTL_STD_LAYER_NAMES )
    {
        LAYER_NUM layer = aItem->GetLayer();

        // English layer names should never need quoting.
        m_out->Print( 0, " (layer %s)", TO_UTF8( BOARD::GetStandardLayerName( layer ) ) );
    }
    else
        m_out->Print( 0, " (layer %s)", m_out->Quotew( aItem->GetLayerName() ).c_str() );
}


void PCB_IO::format( BOARD* aBoard, int aNestLevel ) const
    throw( IO_ERROR )
{
    m_out->Print( 0, "\n" );

    m_out->Print( aNestLevel, "(general\n" );
    m_out->Print( aNestLevel+1, "(links %d)\n", aBoard->GetRatsnestsCount() );
    m_out->Print( aNestLevel+1, "(no_connects %d)\n", aBoard->GetUnconnectedNetCount() );

    // Write Bounding box info
    m_out->Print( aNestLevel+1,  "(area %s %s %s %s)\n",
                  FMTIU( aBoard->GetBoundingBox().GetX() ).c_str(),
                  FMTIU( aBoard->GetBoundingBox().GetY() ).c_str(),
                  FMTIU( aBoard->GetBoundingBox().GetRight() ).c_str(),
                  FMTIU( aBoard->GetBoundingBox().GetBottom() ).c_str() );
    m_out->Print( aNestLevel+1, "(thickness %s)\n",
                  FMTIU( aBoard->GetDesignSettings().GetBoardThickness() ).c_str() );

    m_out->Print( aNestLevel+1, "(drawings %d)\n", aBoard->m_Drawings.GetCount() );
    m_out->Print( aNestLevel+1, "(tracks %d)\n", aBoard->GetNumSegmTrack() );
    m_out->Print( aNestLevel+1, "(zones %d)\n", aBoard->GetNumSegmZone() );
    m_out->Print( aNestLevel+1, "(modules %d)\n", aBoard->m_Modules.GetCount() );
    m_out->Print( aNestLevel+1, "(nets %d)\n", aBoard->GetNetCount() );
    m_out->Print( aNestLevel, ")\n\n" );

    aBoard->GetPageSettings().Format( m_out, aNestLevel, m_ctl );
    aBoard->GetTitleBlock().Format( m_out, aNestLevel, m_ctl );

    // Layers.
    m_out->Print( aNestLevel, "(layers\n" );

    // Save only the used copper layers from front to back.
    for( LAYER_NUM layer = LAST_COPPER_LAYER; layer >= FIRST_COPPER_LAYER; --layer) 
    {
        LAYER_MSK mask = GetLayerMask( layer );
        if( mask & aBoard->GetEnabledLayers() )
        {
            m_out->Print( aNestLevel+1, "(%d %s %s", layer,
                          m_out->Quotew( aBoard->GetLayerName( layer ) ).c_str(),
                          LAYER::ShowType( aBoard->GetLayerType( layer ) ) );

            if( !( aBoard->GetVisibleLayers() & mask ) )
                m_out->Print( 0, " hide" );

            m_out->Print( 0, ")\n" );
        }
    }

    // Save used non-copper layers in the order they are defined.
    for( LAYER_NUM layer = FIRST_NON_COPPER_LAYER; layer <= LAST_NON_COPPER_LAYER; ++layer)
    {
        LAYER_MSK mask = GetLayerMask( layer );
        if( mask & aBoard->GetEnabledLayers() )
        {
            m_out->Print( aNestLevel+1, "(%d %s user", layer,
                          m_out->Quotew( aBoard->GetLayerName( layer ) ).c_str() );

            if( !( aBoard->GetVisibleLayers() & mask ) )
                m_out->Print( 0, " hide" );

            m_out->Print( 0, ")\n" );
        }
    }

    m_out->Print( aNestLevel, ")\n\n" );

    // Setup
    m_out->Print( aNestLevel, "(setup\n" );

    // Save current default track width, for compatibility with older Pcbnew version;
    m_out->Print( aNestLevel+1, "(last_trace_width %s)\n",
                  FMTIU( aBoard->GetCurrentTrackWidth() ).c_str() );

    // Save custom tracks width list (the first is not saved here: this is the netclass value
    for( unsigned ii = 1; ii < aBoard->m_TrackWidthList.size(); ii++ )
        m_out->Print( aNestLevel+1, "(user_trace_width %s)\n",
                      FMTIU( aBoard->m_TrackWidthList[ii] ).c_str() );

    m_out->Print( aNestLevel+1, "(trace_clearance %s)\n",
                  FMTIU( aBoard->m_NetClasses.GetDefault()->GetClearance() ).c_str() );

    // ZONE_SETTINGS
    m_out->Print( aNestLevel+1, "(zone_clearance %s)\n",
                  FMTIU( aBoard->GetZoneSettings().m_ZoneClearance ).c_str() );
    m_out->Print( aNestLevel+1, "(zone_45_only %s)\n",
                  aBoard->GetZoneSettings().m_Zone_45_Only ? "yes" : "no" );

    m_out->Print( aNestLevel+1, "(trace_min %s)\n",
                  FMTIU( aBoard->GetDesignSettings().m_TrackMinWidth ).c_str() );

    m_out->Print( aNestLevel+1, "(segment_width %s)\n",
                  FMTIU( aBoard->GetDesignSettings().m_DrawSegmentWidth ).c_str() );
    m_out->Print( aNestLevel+1, "(edge_width %s)\n",
                  FMTIU( aBoard->GetDesignSettings().m_EdgeSegmentWidth ).c_str() );

    // Save current default via size, for compatibility with older Pcbnew version;
    m_out->Print( aNestLevel+1, "(via_size %s)\n",
                  FMTIU( aBoard->m_NetClasses.GetDefault()->GetViaDiameter() ).c_str() );
    m_out->Print( aNestLevel+1, "(via_drill %s)\n",
                  FMTIU( aBoard->m_NetClasses.GetDefault()->GetViaDrill() ).c_str() );
    m_out->Print( aNestLevel+1, "(via_min_size %s)\n",
                  FMTIU( aBoard->GetDesignSettings().m_ViasMinSize ).c_str() );
    m_out->Print( aNestLevel+1, "(via_min_drill %s)\n",
                  FMTIU( aBoard->GetDesignSettings().m_ViasMinDrill ).c_str() );

    // Save custom vias diameters list (the first is not saved here: this is
    // the netclass value
    for( unsigned ii = 1; ii < aBoard->m_ViasDimensionsList.size(); ii++ )
        m_out->Print( aNestLevel+1, "(user_via %s %s)\n",
                      FMTIU( aBoard->m_ViasDimensionsList[ii].m_Diameter ).c_str(),
                      FMTIU( aBoard->m_ViasDimensionsList[ii].m_Drill ).c_str() );

    // for old versions compatibility:
    m_out->Print( aNestLevel+1, "(uvia_size %s)\n",
                  FMTIU( aBoard->m_NetClasses.GetDefault()->GetuViaDiameter() ).c_str() );
    m_out->Print( aNestLevel+1, "(uvia_drill %s)\n",
                  FMTIU( aBoard->m_NetClasses.GetDefault()->GetuViaDrill() ).c_str() );
    m_out->Print( aNestLevel+1, "(uvias_allowed %s)\n",
                  ( aBoard->GetDesignSettings().m_MicroViasAllowed ) ? "yes" : "no" );
    m_out->Print( aNestLevel+1, "(uvia_min_size %s)\n",
                  FMTIU( aBoard->GetDesignSettings().m_MicroViasMinSize ).c_str() );
    m_out->Print( aNestLevel+1, "(uvia_min_drill %s)\n",
                  FMTIU( aBoard->GetDesignSettings().m_MicroViasMinDrill ).c_str() );

    m_out->Print( aNestLevel+1, "(pcb_text_width %s)\n",
                  FMTIU( aBoard->GetDesignSettings().m_PcbTextWidth ).c_str() );
    m_out->Print( aNestLevel+1, "(pcb_text_size %s %s)\n",
                  FMTIU( aBoard->GetDesignSettings().m_PcbTextSize.x ).c_str(),
                  FMTIU( aBoard->GetDesignSettings().m_PcbTextSize.y ).c_str() );

    m_out->Print( aNestLevel+1, "(mod_edge_width %s)\n",
                  FMTIU( aBoard->GetDesignSettings().m_ModuleSegmentWidth ).c_str() );
    m_out->Print( aNestLevel+1, "(mod_text_size %s %s)\n",
                  FMTIU( aBoard->GetDesignSettings().m_ModuleTextSize.x ).c_str(),
                  FMTIU( aBoard->GetDesignSettings().m_ModuleTextSize.y ).c_str() );
    m_out->Print( aNestLevel+1, "(mod_text_width %s)\n",
                  FMTIU( aBoard->GetDesignSettings().m_ModuleTextWidth ).c_str() );

    m_out->Print( aNestLevel+1, "(pad_size %s %s)\n",
                  FMTIU( aBoard->GetDesignSettings().m_Pad_Master.GetSize().x ).c_str(),
                  FMTIU( aBoard->GetDesignSettings().m_Pad_Master.GetSize().y ).c_str() );
    m_out->Print( aNestLevel+1, "(pad_drill %s)\n",
                  FMTIU( aBoard->GetDesignSettings().m_Pad_Master.GetDrillSize().x ).c_str() );

    m_out->Print( aNestLevel+1, "(pad_to_mask_clearance %s)\n",
                  FMTIU( aBoard->GetDesignSettings().m_SolderMaskMargin ).c_str() );

    if( aBoard->GetDesignSettings().m_SolderMaskMinWidth )
        m_out->Print( aNestLevel+1, "(solder_mask_min_width %s)\n",
                      FMTIU( aBoard->GetDesignSettings().m_SolderMaskMinWidth ).c_str() );

    if( aBoard->GetDesignSettings().m_SolderPasteMargin != 0 )
        m_out->Print( aNestLevel+1, "(pad_to_paste_clearance %s)\n",
                      FMTIU( aBoard->GetDesignSettings().m_SolderPasteMargin ).c_str() );

    if( aBoard->GetDesignSettings().m_SolderPasteMarginRatio != 0 )
        m_out->Print( aNestLevel+1, "(pad_to_paste_clearance_ratio %g)\n",
                      aBoard->GetDesignSettings().m_SolderPasteMarginRatio );

    m_out->Print( aNestLevel+1, "(aux_axis_origin %s %s)\n",
                  FMTIU( aBoard->GetOriginAxisPosition().x ).c_str(),
                  FMTIU( aBoard->GetOriginAxisPosition().y ).c_str() );

    m_out->Print( aNestLevel+1, "(visible_elements %X)\n",
                  aBoard->GetDesignSettings().GetVisibleElements() );

    aBoard->GetPlotOptions().Format( m_out, aNestLevel+1 );

    m_out->Print( aNestLevel, ")\n\n" );

    int netcount = aBoard->GetNetCount();

    for( int i = 0;  i < netcount;  ++i )
    {
        NETINFO_ITEM*   net = aBoard->FindNet( i );
        m_out->Print( aNestLevel, "(net %d %s)\n",
                      net->GetNet(),
                      m_out->Quotew( net->GetNetname() ).c_str() );
    }

    m_out->Print( 0, "\n" );

    // Save the default net class first.
    aBoard->m_NetClasses.GetDefault()->Format( m_out, aNestLevel, m_ctl );

    // Save the rest of the net classes alphabetically.
    for( NETCLASSES::const_iterator it = aBoard->m_NetClasses.begin();
         it != aBoard->m_NetClasses.end();
         ++it )
    {
        NETCLASS* netclass = it->second;
        netclass->Format( m_out, aNestLevel, m_ctl );
    }

    // Save the modules.
    for( MODULE* module = aBoard->m_Modules;  module;  module = (MODULE*) module->Next() )
    {
        Format( module, aNestLevel );
        m_out->Print( 0, "\n" );
    }

    // Save the graphical items on the board (not owned by a module)
    for( BOARD_ITEM* item = aBoard->m_Drawings;  item;  item = item->Next() )
        Format( item, aNestLevel );

    if( aBoard->m_Drawings.GetCount() )
        m_out->Print( 0, "\n" );

    // Do not save MARKER_PCBs, they can be regenerated easily.

    // Save the tracks and vias.
    for( TRACK* track = aBoard->m_Track;  track; track = track->Next() )
        Format( track, aNestLevel );

    if( aBoard->m_Track.GetCount() )
        m_out->Print( 0, "\n" );

    /// @todo Add warning here that the old segment filed zones are no longer supported and
    ///       will not be saved.

    // Save the polygon (which are the newer technology) zones.
    for( int i=0;  i < aBoard->GetAreaCount();  ++i )
        Format( aBoard->GetArea( i ), aNestLevel );
}


void PCB_IO::format( DIMENSION* aDimension, int aNestLevel ) const
    throw( IO_ERROR )
{
    m_out->Print( aNestLevel, "(dimension %s (width %s)",
                  FMT_IU( aDimension->GetValue() ).c_str(),
                  FMT_IU( aDimension->GetWidth() ).c_str() );

    formatLayer( aDimension );

    if( aDimension->GetTimeStamp() )
        m_out->Print( 0, " (tstamp %lX)", aDimension->GetTimeStamp() );

    m_out->Print( 0, "\n" );

    Format( (TEXTE_PCB*) &aDimension->Text(), aNestLevel+1 );

    m_out->Print( aNestLevel+1, "(feature1 (pts (xy %s %s) (xy %s %s)))\n",
                  FMT_IU( aDimension->m_featureLineDO.x ).c_str(),
                  FMT_IU( aDimension->m_featureLineDO.y ).c_str(),
                  FMT_IU( aDimension->m_featureLineDF.x ).c_str(),
                  FMT_IU( aDimension->m_featureLineDF.y ).c_str() );

    m_out->Print( aNestLevel+1, "(feature2 (pts (xy %s %s) (xy %s %s)))\n",
                  FMT_IU( aDimension->m_featureLineGO.x ).c_str(),
                  FMT_IU( aDimension->m_featureLineGO.y ).c_str(),
                  FMT_IU( aDimension->m_featureLineGF.x ).c_str(),
                  FMT_IU( aDimension->m_featureLineGF.y ).c_str() );

    m_out->Print( aNestLevel+1, "(crossbar (pts (xy %s %s) (xy %s %s)))\n",
                  FMT_IU( aDimension->m_crossBarO.x ).c_str(),
                  FMT_IU( aDimension->m_crossBarO.y ).c_str(),
                  FMT_IU( aDimension->m_crossBarF.x ).c_str(),
                  FMT_IU( aDimension->m_crossBarF.y ).c_str() );

    m_out->Print( aNestLevel+1, "(arrow1a (pts (xy %s %s) (xy %s %s)))\n",
                  FMT_IU( aDimension->m_arrowD1O.x ).c_str(),
                  FMT_IU( aDimension->m_arrowD1O.y ).c_str(),
                  FMT_IU( aDimension->m_arrowD1F.x ).c_str(),
                  FMT_IU( aDimension->m_arrowD1F.y ).c_str() );

    m_out->Print( aNestLevel+1, "(arrow1b (pts (xy %s %s) (xy %s %s)))\n",
                  FMT_IU( aDimension->m_arrowD2O.x ).c_str(),
                  FMT_IU( aDimension->m_arrowD2O.y ).c_str(),
                  FMT_IU( aDimension->m_arrowD2F.x ).c_str(),
                  FMT_IU( aDimension->m_arrowD2F.y ).c_str() );

    m_out->Print( aNestLevel+1, "(arrow2a (pts (xy %s %s) (xy %s %s)))\n",
                  FMT_IU( aDimension->m_arrowG1O.x ).c_str(),
                  FMT_IU( aDimension->m_arrowG1O.y ).c_str(),
                  FMT_IU( aDimension->m_arrowG1F.x ).c_str(),
                  FMT_IU( aDimension->m_arrowG1F.y ).c_str() );

    m_out->Print( aNestLevel+1, "(arrow2b (pts (xy %s %s) (xy %s %s)))\n",
                  FMT_IU( aDimension->m_arrowG2O.x ).c_str(),
                  FMT_IU( aDimension->m_arrowG2O.y ).c_str(),
                  FMT_IU( aDimension->m_arrowG2F.x ).c_str(),
                  FMT_IU( aDimension->m_arrowG2F.y ).c_str() );

    m_out->Print( aNestLevel, ")\n" );
}


void PCB_IO::format( DRAWSEGMENT* aSegment, int aNestLevel ) const
    throw( IO_ERROR )
{
    unsigned i;

    switch( aSegment->GetShape() )
    {
    case S_SEGMENT:  // Line
        m_out->Print( aNestLevel, "(gr_line (start %s) (end %s)",
                      FMT_IU( aSegment->GetStart() ).c_str(),
                      FMT_IU( aSegment->GetEnd() ).c_str() );

        if( aSegment->GetAngle() != 0.0 )
            m_out->Print( 0, " (angle %s)", FMT_ANGLE( aSegment->GetAngle() ).c_str() );

        break;

    case S_CIRCLE:  // Circle
        m_out->Print( aNestLevel, "(gr_circle (center %s) (end %s)",
                      FMT_IU( aSegment->GetStart() ).c_str(),
                      FMT_IU( aSegment->GetEnd() ).c_str() );
        break;

    case S_ARC:     // Arc
        m_out->Print( aNestLevel, "(gr_arc (start %s) (end %s) (angle %s)",
                      FMT_IU( aSegment->GetStart() ).c_str(),
                      FMT_IU( aSegment->GetEnd() ).c_str(),
                      FMT_ANGLE( aSegment->GetAngle() ).c_str() );
        break;

    case S_POLYGON: // Polygon
        m_out->Print( aNestLevel, "(gr_poly (pts" );

        for( i = 0;  i < aSegment->GetPolyPoints().size();  ++i )
            m_out->Print( 0, " (xy %s)", FMT_IU( aSegment->GetPolyPoints()[i] ).c_str() );

        m_out->Print( 0, ")" );
        break;

    case S_CURVE:   // Bezier curve
        m_out->Print( aNestLevel, "(gr_curve (pts (xy %s) (xy %s) (xy %s) (xy %s))",
                      FMT_IU( aSegment->GetStart() ).c_str(),
                      FMT_IU( aSegment->GetBezControl1() ).c_str(),
                      FMT_IU( aSegment->GetBezControl2() ).c_str(),
                      FMT_IU( aSegment->GetEnd() ).c_str() );
        break;

    default:
        wxFAIL_MSG( wxT( "Cannot format invalid DRAWSEGMENT type." ) );
    };

    formatLayer( aSegment );

    if( aSegment->GetWidth() != 0 )
        m_out->Print( 0, " (width %s)", FMT_IU( aSegment->GetWidth() ).c_str() );

    if( aSegment->GetTimeStamp() )
        m_out->Print( 0, " (tstamp %lX)", aSegment->GetTimeStamp() );

    if( aSegment->GetStatus() )
        m_out->Print( 0, " (status %X)", aSegment->GetStatus() );

    m_out->Print( 0, ")\n" );
}


void PCB_IO::format( EDGE_MODULE* aModuleDrawing, int aNestLevel ) const
    throw( IO_ERROR )
{
    switch( aModuleDrawing->GetShape() )
    {
    case S_SEGMENT:  // Line
        m_out->Print( aNestLevel, "(fp_line (start %s) (end %s)",
                      FMT_IU( aModuleDrawing->GetStart0() ).c_str(),
                      FMT_IU( aModuleDrawing->GetEnd0() ).c_str() );
        break;

    case S_CIRCLE:  // Circle
        m_out->Print( aNestLevel, "(fp_circle (center %s) (end %s)",
                      FMT_IU( aModuleDrawing->GetStart0() ).c_str(),
                      FMT_IU( aModuleDrawing->GetEnd0() ).c_str() );
        break;

    case S_ARC:     // Arc
        m_out->Print( aNestLevel, "(fp_arc (start %s) (end %s) (angle %s)",
                      FMT_IU( aModuleDrawing->GetStart0() ).c_str(),
                      FMT_IU( aModuleDrawing->GetEnd0() ).c_str(),
                      FMT_ANGLE( aModuleDrawing->GetAngle() ).c_str() );
        break;

    case S_POLYGON: // Polygon
        m_out->Print( aNestLevel, "(fp_poly (pts" );

        for( unsigned i = 0;  i < aModuleDrawing->GetPolyPoints().size();  ++i )
        {
            int nestLevel = 0;

            if( i && !(i%4) )   // newline every 4(pts)
            {
                nestLevel = aNestLevel + 1;
                m_out->Print( 0, "\n" );
            }

            m_out->Print( nestLevel, "%s(xy %s)",
                          nestLevel ? "" : " ",
                          FMT_IU( aModuleDrawing->GetPolyPoints()[i] ).c_str() );
        }
        m_out->Print( 0, ")" );
        break;

    case S_CURVE:   // Bezier curve
        m_out->Print( aNestLevel, "(fp_curve (pts (xy %s) (xy %s) (xy %s) (xy %s))",
                      FMT_IU( aModuleDrawing->GetStart0() ).c_str(),
                      FMT_IU( aModuleDrawing->GetBezControl1() ).c_str(),
                      FMT_IU( aModuleDrawing->GetBezControl2() ).c_str(),
                      FMT_IU( aModuleDrawing->GetEnd0() ).c_str() );
        break;

    default:
        wxFAIL_MSG( wxT( "Cannot format invalid DRAWSEGMENT type." ) );
    };

    formatLayer( aModuleDrawing );

    if( aModuleDrawing->GetWidth() != 0 )
        m_out->Print( 0, " (width %s)", FMT_IU( aModuleDrawing->GetWidth() ).c_str() );

    /*  11-Nov-2021 remove if no one whines after a couple of months.  Simple graphic items
        perhaps do not need these.
    if( aModuleDrawing->GetTimeStamp() )
        m_out->Print( 0, " (tstamp %lX)", aModuleDrawing->GetTimeStamp() );

    if( aModuleDrawing->GetStatus() )
        m_out->Print( 0, " (status %X)", aModuleDrawing->GetStatus() );
    */

    m_out->Print( 0, ")\n" );
}


void PCB_IO::format( PCB_TARGET* aTarget, int aNestLevel ) const
    throw( IO_ERROR )
{
    m_out->Print( aNestLevel, "(target %s (at %s) (size %s)",
                  ( aTarget->GetShape() ) ? "x" : "plus",
                  FMT_IU( aTarget->GetPosition() ).c_str(),
                  FMT_IU( aTarget->GetSize() ).c_str() );

    if( aTarget->GetWidth() != 0 )
        m_out->Print( 0, " (width %s)", FMT_IU( aTarget->GetWidth() ).c_str() );

    formatLayer( aTarget );

    if( aTarget->GetTimeStamp() )
        m_out->Print( 0, " (tstamp %lX)", aTarget->GetTimeStamp() );

    m_out->Print( 0, ")\n" );
}


void PCB_IO::format( MODULE* aModule, int aNestLevel ) const
    throw( IO_ERROR )
{
    m_out->Print( aNestLevel, "(module %s", m_out->Quotew( aModule->GetLibRef() ).c_str() );

    if( aModule->IsLocked() )
        m_out->Print( aNestLevel, " locked" );

    if( aModule->IsPlaced() )
        m_out->Print( aNestLevel, " placed" );

    formatLayer( aModule );

    if( !( m_ctl & CTL_OMIT_TSTAMPS ) )
    {
        m_out->Print( 0, " (tedit %lX) (tstamp %lX)\n",
                       aModule->GetLastEditTime(), aModule->GetTimeStamp() );
    }
    else
        m_out->Print( 0, "\n" );

    m_out->Print( aNestLevel+1, "(at %s", FMT_IU( aModule->GetPosition() ).c_str() );

    if( aModule->GetOrientation() != 0.0 )
        m_out->Print( 0, " %s", FMT_ANGLE( aModule->GetOrientation() ).c_str() );

    m_out->Print( 0, ")\n" );

    if( !aModule->GetDescription().IsEmpty() )
        m_out->Print( aNestLevel+1, "(descr %s)\n",
                      m_out->Quotew( aModule->GetDescription() ).c_str() );

    if( !aModule->GetKeywords().IsEmpty() )
        m_out->Print( aNestLevel+1, "(tags %s)\n",
                      m_out->Quotew( aModule->GetKeywords() ).c_str() );

    if( !aModule->GetPath().IsEmpty() )
        m_out->Print( aNestLevel+1, "(path %s)\n",
                      m_out->Quotew( aModule->GetPath() ).c_str() );

    if( aModule->GetPlacementCost90() != 0 )
        m_out->Print( aNestLevel+1, "(autoplace_cost90 %d)\n", aModule->GetPlacementCost90() );

    if( aModule->GetPlacementCost180() != 0 )
        m_out->Print( aNestLevel+1, "(autoplace_cost180 %d)\n", aModule->GetPlacementCost180() );

    if( aModule->GetLocalSolderMaskMargin() != 0 )
        m_out->Print( aNestLevel+1, "(solder_mask_margin %s)\n",
                      FMT_IU( aModule->GetLocalSolderMaskMargin() ).c_str() );

    if( aModule->GetLocalSolderPasteMargin() != 0 )
        m_out->Print( aNestLevel+1, "(solder_paste_margin %s)\n",
                      FMT_IU( aModule->GetLocalSolderPasteMargin() ).c_str() );

    if( aModule->GetLocalSolderPasteMarginRatio() != 0 )
        m_out->Print( aNestLevel+1, "(solder_paste_ratio %g)\n",
                      aModule->GetLocalSolderPasteMarginRatio() );

    if( aModule->GetLocalClearance() != 0 )
        m_out->Print( aNestLevel+1, "(clearance %s)\n",
                      FMT_IU( aModule->GetLocalClearance() ).c_str() );

    if( aModule->GetZoneConnection() != UNDEFINED_CONNECTION )
        m_out->Print( aNestLevel+1, "(zone_connect %d)\n", aModule->GetZoneConnection() );

    if( aModule->GetThermalWidth() != 0 )
        m_out->Print( aNestLevel+1, "(thermal_width %s)\n",
                      FMT_IU( aModule->GetThermalWidth() ).c_str() );

    if( aModule->GetThermalGap() != 0 )
        m_out->Print( aNestLevel+1, "(thermal_gap %s)\n",
                      FMT_IU( aModule->GetThermalGap() ).c_str() );

    // Attributes
    if( aModule->GetAttributes() != MOD_DEFAULT )
    {
        m_out->Print( aNestLevel+1, "(attr" );

        if( aModule->GetAttributes() & MOD_CMS )
            m_out->Print( 0, " smd" );

        if( aModule->GetAttributes() & MOD_VIRTUAL )
            m_out->Print( 0, " virtual" );

        m_out->Print( 0, ")\n" );
    }

    Format( (BOARD_ITEM*) &aModule->Reference(), aNestLevel+1 );
    Format( (BOARD_ITEM*) &aModule->Value(), aNestLevel+1 );

    // Save drawing elements.
    for( BOARD_ITEM* gr = aModule->GraphicalItems();  gr;  gr = gr->Next() )
        Format( gr, aNestLevel+1 );

    // Save pads.
    for( D_PAD* pad = aModule->Pads();  pad;  pad = pad->Next() )
        Format( pad, aNestLevel+1 );

    // Save 3D info.
    for( S3D_MASTER* t3D = aModule->Models();  t3D;  t3D = t3D->Next() )
    {
        if( !t3D->m_Shape3DName.IsEmpty() )
        {
            m_out->Print( aNestLevel+1, "(model %s\n",
                          m_out->Quotew( t3D->m_Shape3DName ).c_str() );

            m_out->Print( aNestLevel+2, "(at (xyz %.16g %.16g %.16g))\n",
                          t3D->m_MatPosition.x,
                          t3D->m_MatPosition.y,
                          t3D->m_MatPosition.z );

            m_out->Print( aNestLevel+2, "(scale (xyz %.16g %.16g %.16g))\n",
                          t3D->m_MatScale.x,
                          t3D->m_MatScale.y,
                          t3D->m_MatScale.z );

            m_out->Print( aNestLevel+2, "(rotate (xyz %.16g %.16g %.16g))\n",
                          t3D->m_MatRotation.x,
                          t3D->m_MatRotation.y,
                          t3D->m_MatRotation.z );

            m_out->Print( aNestLevel+1, ")\n" );
        }
    }

    m_out->Print( aNestLevel, ")\n" );
}


void PCB_IO::formatLayers( LAYER_MSK aLayerMask, int aNestLevel ) const
    throw( IO_ERROR )
{
    m_out->Print( aNestLevel, "(layers" );

    LAYER_MSK cuMask = ALL_CU_LAYERS;

    if( m_board )
        cuMask &= m_board->GetEnabledLayers();

    // output copper layers first, then non copper

    if( ( aLayerMask & cuMask ) == cuMask )
    {
        m_out->Print( 0, " *.Cu" );
        aLayerMask &= ~ALL_CU_LAYERS;       // clear bits, so they are not output again below
    }
    else if( ( aLayerMask & cuMask ) == (LAYER_BACK | LAYER_FRONT) )
    {
        m_out->Print( 0, " F&B.Cu" );
        aLayerMask &= ~(LAYER_BACK | LAYER_FRONT);
    }

    if( ( aLayerMask & (ADHESIVE_LAYER_BACK | ADHESIVE_LAYER_FRONT)) == (ADHESIVE_LAYER_BACK | ADHESIVE_LAYER_FRONT) )
    {
        m_out->Print( 0, " *.Adhes" );
        aLayerMask &= ~(ADHESIVE_LAYER_BACK | ADHESIVE_LAYER_FRONT);
    }

    if( ( aLayerMask & (SOLDERPASTE_LAYER_BACK | SOLDERPASTE_LAYER_FRONT)) == (SOLDERPASTE_LAYER_BACK | SOLDERPASTE_LAYER_FRONT) )
    {
        m_out->Print( 0, " *.Paste" );
        aLayerMask &= ~(SOLDERPASTE_LAYER_BACK | SOLDERPASTE_LAYER_FRONT);
    }

    if( ( aLayerMask & (SILKSCREEN_LAYER_BACK | SILKSCREEN_LAYER_FRONT)) == (SILKSCREEN_LAYER_BACK | SILKSCREEN_LAYER_FRONT) )
    {
        m_out->Print( 0, " *.SilkS" );
        aLayerMask &= ~(SILKSCREEN_LAYER_BACK | SILKSCREEN_LAYER_FRONT);
    }

    if( ( aLayerMask & (SOLDERMASK_LAYER_BACK | SOLDERMASK_LAYER_FRONT)) == (SOLDERMASK_LAYER_BACK | SOLDERMASK_LAYER_FRONT) )
    {
        m_out->Print( 0, " *.Mask" );
        aLayerMask &= ~(SOLDERMASK_LAYER_BACK | SOLDERMASK_LAYER_FRONT);
    }

    // output any individual layers not handled in wildcard combos above

    if( m_board )
        aLayerMask &= m_board->GetEnabledLayers();

    wxString layerName;

    for( LAYER_NUM layer = FIRST_LAYER; layer < NB_PCB_LAYERS; ++layer )
    {
        if( aLayerMask & GetLayerMask( layer ) )
        {
            if( m_board && !(m_ctl & CTL_STD_LAYER_NAMES) )
                layerName = m_board->GetLayerName( layer );

            else    // I am being called from FootprintSave()
                layerName = BOARD::GetStandardLayerName( layer );

            m_out->Print( 0, " %s", m_out->Quotew( layerName ).c_str() );
        }
    }

    m_out->Print( 0, ")\n" );
}


void PCB_IO::format( D_PAD* aPad, int aNestLevel ) const
    throw( IO_ERROR )
{
    const char* shape;

    switch( aPad->GetShape() )
    {
    case PAD_CIRCLE:    shape = "circle";       break;
    case PAD_RECT:      shape = "rect";         break;
    case PAD_OVAL:      shape = "oval";         break;
    case PAD_TRAPEZOID: shape = "trapezoid";    break;

    default:
        THROW_IO_ERROR( wxString::Format( _( "unknown pad type: %d"), aPad->GetShape() ) );
    }

    const char* type;

    switch( aPad->GetAttribute() )
    {
    case PAD_STANDARD:          type = "thru_hole";      break;
    case PAD_SMD:               type = "smd";            break;
    case PAD_CONN:              type = "connect";        break;
    case PAD_HOLE_NOT_PLATED:   type = "np_thru_hole";   break;

    default:
        THROW_IO_ERROR( wxString::Format( _( "unknown pad attribute: %d" ),
                                          aPad->GetAttribute() ) );
    }

    m_out->Print( aNestLevel, "(pad %s %s %s",
                  m_out->Quotew( aPad->GetPadName() ).c_str(),
                  type, shape );
    m_out->Print( 0, " (at %s", FMT_IU( aPad->GetPos0() ).c_str() );

    if( aPad->GetOrientation() != 0.0 )
        m_out->Print( 0, " %s", FMT_ANGLE( aPad->GetOrientation() ).c_str() );

    m_out->Print( 0, ")" );
    m_out->Print( 0, " (size %s)", FMT_IU( aPad->GetSize() ).c_str() );

    if( (aPad->GetDelta().GetWidth()) != 0 || (aPad->GetDelta().GetHeight() != 0 ) )
        m_out->Print( 0, " (rect_delta %s )", FMT_IU( aPad->GetDelta() ).c_str() );

    wxSize sz = aPad->GetDrillSize();

    if( (sz.GetWidth() > 0) || (sz.GetHeight() > 0) )
    {
        m_out->Print( 0, " (drill" );

        if( aPad->GetDrillShape() == PAD_OVAL )
            m_out->Print( 0, " oval" );

        if( sz.GetWidth() > 0 )
            m_out->Print( 0,  " %s", FMT_IU( sz.GetWidth() ).c_str() );

        if( sz.GetHeight() > 0  && sz.GetWidth() != sz.GetHeight() )
            m_out->Print( 0,  " %s", FMT_IU( sz.GetHeight() ).c_str() );

        if( (aPad->GetOffset().x != 0) || (aPad->GetOffset().y != 0) )
            m_out->Print( 0, " (offset %s)", FMT_IU( aPad->GetOffset() ).c_str() );

        m_out->Print( 0, ")" );
    }

    m_out->Print( 0, "\n" );

    formatLayers( aPad->GetLayerMask(), aNestLevel+1 );

    // Unconnected pad is default net so don't save it.
    if( !(m_ctl & CTL_OMIT_NETS) && aPad->GetNet() != 0 )
    {
        m_out->Print( aNestLevel+1, "(net %d %s)\n",
                      aPad->GetNet(), m_out->Quotew( aPad->GetNetname() ).c_str() );
    }

    if( aPad->GetPadToDieLength() != 0 )
        m_out->Print( aNestLevel+1, "(die_length %s)\n",
                      FMT_IU( aPad->GetPadToDieLength() ).c_str() );

    if( aPad->GetLocalSolderMaskMargin() != 0 )
        m_out->Print( aNestLevel+1, "(solder_mask_margin %s)\n",
                      FMT_IU( aPad->GetLocalSolderMaskMargin() ).c_str() );

    if( aPad->GetLocalSolderPasteMargin() != 0 )
        m_out->Print( aNestLevel+1, "(solder_paste_margin %s)\n",
                      FMT_IU( aPad->GetLocalSolderPasteMargin() ).c_str() );

    if( aPad->GetLocalSolderPasteMarginRatio() != 0 )
        m_out->Print( aNestLevel+1, "(solder_paste_margin_ratio %g)\n",
                      aPad->GetLocalSolderPasteMarginRatio() );

    if( aPad->GetLocalClearance() != 0 )
        m_out->Print( aNestLevel+1, "(clearance %s)\n",
                      FMT_IU( aPad->GetLocalClearance() ).c_str() );

    if( aPad->GetZoneConnection() != UNDEFINED_CONNECTION )
        m_out->Print( aNestLevel+1, "(zone_connect %d)\n", aPad->GetZoneConnection() );

    if( aPad->GetThermalWidth() != 0 )
        m_out->Print( aNestLevel+1, "(thermal_width %s)\n",
                      FMT_IU( aPad->GetThermalWidth() ).c_str() );

    if( aPad->GetThermalGap() != 0 )
        m_out->Print( aNestLevel+1, "(thermal_gap %s)\n",
                      FMT_IU( aPad->GetThermalGap() ).c_str() );

    m_out->Print( aNestLevel, ")\n" );
}


void PCB_IO::format( TEXTE_PCB* aText, int aNestLevel ) const
    throw( IO_ERROR )
{
    m_out->Print( aNestLevel, "(gr_text %s (at %s",
                  m_out->Quotew( aText->GetText() ).c_str(),
                  FMT_IU( aText->GetTextPosition() ).c_str() );

    if( aText->GetOrientation() != 0.0 )
        m_out->Print( 0, " %s", FMT_ANGLE( aText->GetOrientation() ).c_str() );

    m_out->Print( 0, ")" );

    formatLayer( aText );

    if( aText->GetTimeStamp() )
        m_out->Print( 0, " (tstamp %lX)", aText->GetTimeStamp() );

    m_out->Print( 0, "\n" );

    aText->EDA_TEXT::Format( m_out, aNestLevel, m_ctl );

    m_out->Print( aNestLevel, ")\n" );
}


void PCB_IO::format( TEXTE_MODULE* aText, int aNestLevel ) const
    throw( IO_ERROR )
{
    MODULE*  parent = (MODULE*) aText->GetParent();
    double   orient = aText->GetOrientation();
    wxString type;

    switch( aText->GetType() )
    {
    case TEXTE_MODULE::TEXT_is_REFERENCE: type = wxT( "reference" );     break;
    case TEXTE_MODULE::TEXT_is_VALUE:     type = wxT( "value" );         break;
    default:                              type = wxT( "user" );
    }

    // Due to the Pcbnew history, m_Orient is saved in screen value
    // but it is handled as relative to its parent footprint
    if( parent )
        orient += parent->GetOrientation();

    m_out->Print( aNestLevel, "(fp_text %s %s (at %s",
                  m_out->Quotew( type ).c_str(),
                  m_out->Quotew( aText->GetText() ).c_str(),
                  FMT_IU( aText->GetPos0() ).c_str() );

    if( orient != 0.0 )
        m_out->Print( 0, " %s", FMT_ANGLE( orient ).c_str() );

    m_out->Print( 0, ")" );
    formatLayer( aText );

    if( !aText->IsVisible() )
        m_out->Print( 0, " hide" );

    m_out->Print( 0, "\n" );

    aText->EDA_TEXT::Format( m_out, aNestLevel, m_ctl );

    m_out->Print( aNestLevel, ")\n" );
}


void PCB_IO::format( TRACK* aTrack, int aNestLevel ) const
    throw( IO_ERROR )
{
    if( aTrack->Type() == PCB_VIA_T )
    {
        LAYER_NUM layer1, layer2;

        SEGVIA* via = (SEGVIA*) aTrack;
        BOARD* board = (BOARD*) via->GetParent();

        wxCHECK_RET( board != 0, wxT( "Via " ) + via->GetSelectMenuText() +
                     wxT( " has no parent." ) );

        m_out->Print( aNestLevel, "(via" );

        via->ReturnLayerPair( &layer1, &layer2 );

        switch( aTrack->GetShape() )
        {
        case VIA_THROUGH:           //  Default shape not saved.
            break;

        case VIA_BLIND_BURIED:
            m_out->Print( 0, " blind" );
            break;

        case VIA_MICROVIA:
            m_out->Print( 0, " micro" );
            break;

        default:
            THROW_IO_ERROR( wxString::Format( _( "unknown via type %d"  ), aTrack->GetShape() ) );
        }

        m_out->Print( 0, " (at %s) (size %s)",
                      FMT_IU( aTrack->GetStart() ).c_str(),
                      FMT_IU( aTrack->GetWidth() ).c_str() );

        if( aTrack->GetDrill() != UNDEFINED_DRILL_DIAMETER )
            m_out->Print( 0, " (drill %s)", FMT_IU( aTrack->GetDrill() ).c_str() );

        m_out->Print( 0, " (layers %s %s)",
                      m_out->Quotew( m_board->GetLayerName( layer1 ) ).c_str(),
                      m_out->Quotew( m_board->GetLayerName( layer2 ) ).c_str() );
    }
    else
    {
        m_out->Print( aNestLevel, "(segment (start %s) (end %s) (width %s)",
                      FMT_IU( aTrack->GetStart() ).c_str(), FMT_IU( aTrack->GetEnd() ).c_str(),
                      FMT_IU( aTrack->GetWidth() ).c_str() );

        m_out->Print( 0, " (layer %s)", m_out->Quotew( aTrack->GetLayerName() ).c_str() );
    }

    m_out->Print( 0, " (net %d)", aTrack->GetNet() );

    if( aTrack->GetTimeStamp() != 0 )
        m_out->Print( 0, " (tstamp %lX)", aTrack->GetTimeStamp() );

    if( aTrack->GetStatus() != 0 )
        m_out->Print( 0, " (status %X)", aTrack->GetStatus() );

    m_out->Print( 0, ")\n" );
}


void PCB_IO::format( ZONE_CONTAINER* aZone, int aNestLevel ) const
    throw( IO_ERROR )
{
    // Save the NET info; For keepout zones, net code and net name are irrelevant
    // so be sure a dummy value is stored, just for ZONE_CONTAINER compatibility
    // (perhaps netcode and netname should be not stored)
    m_out->Print( aNestLevel, "(zone (net %d) (net_name %s)",
                  aZone->GetIsKeepout() ? 0 : aZone->GetNet(),
                  m_out->Quotew( aZone->GetIsKeepout() ? wxT("") : aZone->GetNetName() ).c_str() );

    formatLayer( aZone );

    m_out->Print( 0, " (tstamp %lX)", aZone->GetTimeStamp() );

    // Save the outline aux info
    std::string hatch;

    switch( aZone->GetHatchStyle() )
    {
    default:
    case CPolyLine::NO_HATCH:       hatch = "none";    break;
    case CPolyLine::DIAGONAL_EDGE:  hatch = "edge";    break;
    case CPolyLine::DIAGONAL_FULL:  hatch = "full";    break;
    }

    m_out->Print( 0, " (hatch %s %s)\n", hatch.c_str(),
                  FMT_IU( aZone->Outline()->GetHatchPitch() ).c_str() );

    if( aZone->GetPriority() > 0 )
        m_out->Print( aNestLevel+1, "(priority %d)\n", aZone->GetPriority() );

    m_out->Print( aNestLevel+1, "(connect_pads" );

    switch( aZone->GetPadConnection() )
    {
    default:
    case THERMAL_PAD:       // Default option not saved or loaded.
        break;

    case THT_THERMAL:
        m_out->Print( 0, " thru_hole_only" );
        break;

    case PAD_IN_ZONE:
        m_out->Print( 0, " yes" );
        break;

    case PAD_NOT_IN_ZONE:
        m_out->Print( 0, " no" );
        break;
    }

    m_out->Print( 0, " (clearance %s))\n",
                  FMT_IU( aZone->GetZoneClearance() ).c_str() );

    m_out->Print( aNestLevel+1, "(min_thickness %s)\n",
                  FMT_IU( aZone->GetMinThickness() ).c_str() );

    if( aZone->GetIsKeepout() )
    {
        m_out->Print( aNestLevel+1, "(keepout (tracks %s) (vias %s) (copperpour %s))\n",
                      aZone->GetDoNotAllowTracks() ? "not_allowed" : "allowed",
                      aZone->GetDoNotAllowVias() ? "not_allowed" : "allowed",
                      aZone->GetDoNotAllowCopperPour() ? "not_allowed" : "allowed" );
    }

    m_out->Print( aNestLevel+1, "(fill" );

    // Default is not filled.
    if( aZone->IsFilled() )
        m_out->Print( 0, " yes" );

    // Default is polygon filled.
    if( aZone->GetFillMode() )
        m_out->Print( 0, " (mode segment)" );

    m_out->Print( 0, " (arc_segments %d) (thermal_gap %s) (thermal_bridge_width %s)",
                  aZone->GetArcSegmentCount(),
                  FMT_IU( aZone->GetThermalReliefGap() ).c_str(),
                  FMT_IU( aZone->GetThermalReliefCopperBridge() ).c_str() );

    if( aZone->GetCornerSmoothingType() != ZONE_SETTINGS::SMOOTHING_NONE )
    {
        m_out->Print( 0, " (smoothing" );

        switch( aZone->GetCornerSmoothingType() )
        {
        case ZONE_SETTINGS::SMOOTHING_CHAMFER:
            m_out->Print( 0, " chamfer" );
            break;

        case ZONE_SETTINGS::SMOOTHING_FILLET:
            m_out->Print( 0,  " fillet" );
            break;

        default:
            THROW_IO_ERROR( wxString::Format( _( "unknown zone corner smoothing type %d"  ),
                                              aZone->GetCornerSmoothingType() ) );
        }
        m_out->Print( 0, ")" );

        if( aZone->GetCornerRadius() != 0 )
            m_out->Print( 0, " (radius %s)",
                          FMT_IU( aZone->GetCornerRadius() ).c_str() );
    }

    m_out->Print( 0, ")\n" );

    const std::vector< CPolyPt >& cv = aZone->Outline()->m_CornersList;
    int newLine = 0;

    if( cv.size() )
    {
        m_out->Print( aNestLevel+1, "(polygon\n");
        m_out->Print( aNestLevel+2, "(pts\n" );

        for( std::vector< CPolyPt >::const_iterator it = cv.begin();  it != cv.end();  ++it )
        {
            if( newLine == 0 )
                m_out->Print( aNestLevel+3, "(xy %s %s)",
                              FMT_IU( it->x ).c_str(), FMT_IU( it->y ).c_str() );
            else
                m_out->Print( 0, " (xy %s %s)",
                              FMT_IU( it->x ).c_str(), FMT_IU( it->y ).c_str() );

            if( newLine < 4 )
            {
                newLine += 1;
            }
            else
            {
                newLine = 0;
                m_out->Print( 0, "\n" );
            }

            if( it->end_contour )
            {
                if( newLine != 0 )
                    m_out->Print( 0, "\n" );

                m_out->Print( aNestLevel+2, ")\n" );

                if( it+1 != cv.end() )
                {
                    newLine = 0;
                    m_out->Print( aNestLevel+1, ")\n" );
                    m_out->Print( aNestLevel+1, "(polygon\n" );
                    m_out->Print( aNestLevel+2, "(pts" );
                }
            }
        }

        m_out->Print( aNestLevel+1, ")\n" );
    }

    // Save the PolysList
    const std::vector< CPolyPt >& fv = aZone->GetFilledPolysList();
    newLine = 0;

    if( fv.size() )
    {
        m_out->Print( aNestLevel+1, "(filled_polygon\n" );
        m_out->Print( aNestLevel+2, "(pts\n" );

        for( std::vector< CPolyPt >::const_iterator it = fv.begin();  it != fv.end();  ++it )
        {
            if( newLine == 0 )
                m_out->Print( aNestLevel+3, "(xy %s %s)",
                              FMT_IU( it->x ).c_str(), FMT_IU( it->y ).c_str() );
            else
                m_out->Print( 0, " (xy %s %s)",
                              FMT_IU( it->x ).c_str(), FMT_IU( it->y ).c_str() );

            if( newLine < 4 )
            {
                newLine += 1;
            }
            else
            {
                newLine = 0;
                m_out->Print( 0, "\n" );
            }

            if( it->end_contour )
            {
                if( newLine != 0 )
                    m_out->Print( 0, "\n" );

                m_out->Print( aNestLevel+2, ")\n" );

                if( it+1 != fv.end() )
                {
                    newLine = 0;
                    m_out->Print( aNestLevel+1, ")\n" );
                    m_out->Print( aNestLevel+1, "(filled_polygon\n" );
                    m_out->Print( aNestLevel+2, "(pts\n" );
                }
            }
        }

        m_out->Print( aNestLevel+1, ")\n" );
    }

    // Save the filling segments list
    const std::vector< SEGMENT >& segs = aZone->FillSegments();

    if( segs.size() )
    {
        m_out->Print( aNestLevel+1, "(fill_segments\n" );

        for( std::vector< SEGMENT >::const_iterator it = segs.begin();  it != segs.end();  ++it )
        {
            m_out->Print( aNestLevel+2, "(pts (xy %s) (xy %s))\n",
                          FMT_IU( it->m_Start ).c_str(),
                          FMT_IU( it->m_End ).c_str() );
        }

        m_out->Print( aNestLevel+1, ")\n" );
    }

    m_out->Print( aNestLevel, ")\n" );
}


PCB_IO::PCB_IO() :
    m_cache( 0 ),
    m_ctl( 0 ),
    m_parser( new PCB_PARSER() )
{
    init( 0 );
    m_out = &m_sf;
}


PCB_IO::PCB_IO( int aControlFlags ) :
    m_cache( 0 ),
    m_ctl( aControlFlags ),
    m_parser( new PCB_PARSER() )
{
    init( 0 );
    m_out = &m_sf;
}


PCB_IO::~PCB_IO()
{
    delete m_cache;
    delete m_parser;
}


BOARD* PCB_IO::Load( const wxString& aFileName, BOARD* aAppendToMe, PROPERTIES* aProperties )
{
    FILE_LINE_READER    reader( aFileName );

    m_parser->SetLineReader( &reader );
    m_parser->SetBoard( aAppendToMe );

    BOARD* board = dynamic_cast<BOARD*>( m_parser->Parse() );
    wxASSERT( board );

    // Give the filename to the board if it's new
    if( !aAppendToMe )
        board->SetFileName( aFileName );

    return board;
}


void PCB_IO::init( PROPERTIES* aProperties )
{
    m_board = NULL;
    m_props = aProperties;
}


void PCB_IO::cacheLib( const wxString& aLibraryPath )
{
    if( !m_cache || m_cache->GetPath() != aLibraryPath || m_cache->IsModified() )
    {
        // a spectacular episode in memory management:
        delete m_cache;
        m_cache = new FP_CACHE( this, aLibraryPath );
        m_cache->Load();
    }
}


wxArrayString PCB_IO::FootprintEnumerate( const wxString& aLibraryPath, PROPERTIES* aProperties )
{
    LOCALE_IO   toggle;     // toggles on, then off, the C locale.

    init( aProperties );

    cacheLib( aLibraryPath );

    const MODULE_MAP& mods = m_cache->GetModules();

    wxArrayString ret;

    for( MODULE_CITER it = mods.begin();  it != mods.end();  ++it )
    {
        ret.Add( FROM_UTF8( it->first.c_str() ) );
    }

    return ret;
}


MODULE* PCB_IO::FootprintLoad( const wxString& aLibraryPath, const wxString& aFootprintName,
                               PROPERTIES* aProperties )
{
    LOCALE_IO   toggle;     // toggles on, then off, the C locale.

    init( aProperties );

    cacheLib( aLibraryPath );

    const MODULE_MAP& mods = m_cache->GetModules();

    MODULE_CITER it = mods.find( TO_UTF8( aFootprintName ) );

    if( it == mods.end() )
    {
        return NULL;
    }

    // copy constructor to clone the already loaded MODULE
    return new MODULE( *it->second->GetModule() );
}


void PCB_IO::FootprintSave( const wxString& aLibraryPath, const MODULE* aFootprint,
                            PROPERTIES* aProperties )
{
    LOCALE_IO   toggle;     // toggles on, then off, the C locale.

    init( aProperties );

    // In this public PLUGIN API function, we can safely assume it was
    // called for saving into a library path.
    m_ctl = CTL_FOR_LIBRARY;

    cacheLib( aLibraryPath );

    if( !m_cache->IsWritable() )
    {
        THROW_IO_ERROR( wxString::Format( _( "Library <%s> is read only" ),
                                          aLibraryPath.GetData() ) );
    }

    std::string footprintName = TO_UTF8( aFootprint->GetLibRef() );

    MODULE_MAP& mods = m_cache->GetModules();

    // Quietly overwrite module and delete module file from path for any by same name.
    wxFileName fn( aLibraryPath, aFootprint->GetLibRef(), KiCadFootprintFileExtension );

    if( !fn.IsOk() )
    {
        THROW_IO_ERROR( wxString::Format( _( "Footprint file name <%s> is not valid." ),
                                          GetChars( fn.GetFullPath() ) ) );
    }

    if( fn.FileExists() && !fn.IsFileWritable() )
    {
        THROW_IO_ERROR( wxString::Format( _( "user does not have write permission to delete file <%s> " ),
                                          GetChars( fn.GetFullPath() ) ) );
    }

    MODULE_CITER it = mods.find( footprintName );

    if( it != mods.end() )
    {
        wxLogTrace( traceFootprintLibrary, wxT( "Removing footprint library file '%s'." ),
                    fn.GetFullPath().GetData() );
        mods.erase( footprintName );
        wxRemoveFile( fn.GetFullPath() );
    }

    // I need my own copy for the cache
    MODULE* module = new MODULE( *aFootprint );

    // and it's time stamp must be 0, it should have no parent, orientation should
    // be zero, and it should be on the front layer.
    module->SetTimeStamp( 0 );
    module->SetParent( 0 );
    module->SetOrientation( 0 );

    if( module->GetLayer() != LAYER_N_FRONT )
        module->Flip( module->GetPosition() );

    wxLogTrace( traceFootprintLibrary, wxT( "Creating s-expression footprint file: %s." ),
                fn.GetFullPath().GetData() );
    mods.insert( footprintName, new FP_CACHE_ITEM( module, fn ) );
    m_cache->Save();
}


void PCB_IO::FootprintDelete( const wxString& aLibraryPath, const wxString& aFootprintName )
{
    LOCALE_IO   toggle;     // toggles on, then off, the C locale.

    init( NULL );

    cacheLib( aLibraryPath );

    if( !m_cache->IsWritable() )
    {
        THROW_IO_ERROR( wxString::Format( _( "Library <%s> is read only" ),
                                          aLibraryPath.GetData() ) );
    }

    m_cache->Remove( aFootprintName );
}


void PCB_IO::FootprintLibCreate( const wxString& aLibraryPath, PROPERTIES* aProperties )
{
    if( wxDir::Exists( aLibraryPath ) )
    {
        THROW_IO_ERROR( wxString::Format( _( "cannot overwrite library path <%s>" ),
                                          aLibraryPath.GetData() ) );
    }

    LOCALE_IO   toggle;

    init( aProperties );

    delete m_cache;
    m_cache = new FP_CACHE( this, aLibraryPath );
    m_cache->Save();
}


bool PCB_IO::FootprintLibDelete( const wxString& aLibraryPath, PROPERTIES* aProperties )
{
    wxFileName fn;
    fn.SetPath( aLibraryPath );

    // Return if there is no library path to delete.
    if( !fn.DirExists() )
        return false;

    if( !fn.IsDirWritable() )
    {
        THROW_IO_ERROR( wxString::Format( _( "user does not have permission to delete directory <%s>" ),
                                          aLibraryPath.GetData() ) );
    }

    wxDir dir( aLibraryPath );

    if( dir.HasSubDirs() )
    {
        THROW_IO_ERROR( wxString::Format( _( "library directory <%s> has unexpected sub-directories" ),
                                          aLibraryPath.GetData() ) );
    }

    // All the footprint files must be deleted before the directory can be deleted.
    if( dir.HasFiles() )
    {
        unsigned      i;
        wxFileName    tmp;
        wxArrayString files;

        wxDir::GetAllFiles( aLibraryPath, &files );

        for( i = 0;  i < files.GetCount();  i++ )
        {
            tmp = files[i];

            if( tmp.GetExt() != KiCadFootprintFileExtension )
            {
                THROW_IO_ERROR( wxString::Format( _( "unexpected file <%s> was found in library path '%s'" ),
                                                  files[i].GetData(), aLibraryPath.GetData() ) );
            }
        }

        for( i = 0;  i < files.GetCount();  i++ )
        {
            wxRemoveFile( files[i] );
        }
    }

    wxLogTrace( traceFootprintLibrary, wxT( "Removing footprint library '%s'" ),
                aLibraryPath.GetData() );

    // Some of the more elaborate wxRemoveFile() crap puts up its own wxLog dialog
    // we don't want that.  we want bare metal portability with no UI here.
    if( !wxRmdir( aLibraryPath ) )
    {
        THROW_IO_ERROR( wxString::Format( _( "footprint library <%s> cannot be deleted" ),
                                          aLibraryPath.GetData() ) );
    }

    // For some reason removing a directory in Windows is not immediately updated.  This delay
    // prevents an error when attempting to immediately recreate the same directory when over
    // writing an existing library.
#ifdef __WINDOWS__
    wxMilliSleep( 250L );
#endif

    if( m_cache && m_cache->GetPath() == aLibraryPath )
    {
        delete m_cache;
        m_cache = NULL;
    }

    return true;
}


bool PCB_IO::IsFootprintLibWritable( const wxString& aLibraryPath )
{
    LOCALE_IO   toggle;

    init( NULL );

    cacheLib( aLibraryPath );

    return m_cache->IsWritable();
}
