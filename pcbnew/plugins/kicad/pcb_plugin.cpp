/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2012 CERN
 * Copyright (C) 1992-2023 KiCad Developers, see AUTHORS.txt for contributors.
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

#include <advanced_config.h>
#include <base_units.h>
#include <board.h>
#include <board_design_settings.h>
#include <confirm.h>
#include <convert_basic_shapes_to_polygon.h> // for enum RECT_CHAMFER_POSITIONS definition
#include <core/arraydim.h>
#include <pcb_dimension.h>
#include <footprint.h>
#include <string_utils.h>
#include <kiface_base.h>
#include <locale_io.h>
#include <macros.h>
#include <pad.h>
#include <pcb_group.h>
#include <pcb_shape.h>
#include <pcb_bitmap.h>
#include <pcb_target.h>
#include <pcb_text.h>
#include <pcb_textbox.h>
#include <pcbnew_settings.h>
#include <pgm_base.h>
#include <plugins/kicad/pcb_plugin.h>
#include <plugins/kicad/pcb_parser.h>
#include <trace_helpers.h>
#include <pcb_track.h>
#include <progress_reporter.h>
#include <wildcards_and_files_ext.h>
#include <wx/dir.h>
#include <wx/log.h>
#include <zone.h>
#include <zones.h>

// For some reason wxWidgets is built with wxUSE_BASE64 unset so expose the wxWidgets
// base64 code. Needed for PCB_BITMAP
#define wxUSE_BASE64 1
#include <wx/base64.h>
#include <wx/mstream.h>


using namespace PCB_KEYS_T;


FP_CACHE_ITEM::FP_CACHE_ITEM( FOOTPRINT* aFootprint, const WX_FILENAME& aFileName ) :
        m_filename( aFileName ),
        m_footprint( aFootprint )
{ }


FP_CACHE::FP_CACHE( PCB_PLUGIN* aOwner, const wxString& aLibraryPath )
{
    m_owner = aOwner;
    m_lib_raw_path = aLibraryPath;
    m_lib_path.SetPath( aLibraryPath );
    m_cache_timestamp = 0;
    m_cache_dirty = true;
}


void FP_CACHE::Save( FOOTPRINT* aFootprint )
{
    m_cache_timestamp = 0;

    if( !m_lib_path.DirExists() && !m_lib_path.Mkdir() )
    {
        THROW_IO_ERROR( wxString::Format( _( "Cannot create footprint library '%s'." ),
                                          m_lib_raw_path ) );
    }

    if( !m_lib_path.IsDirWritable() )
    {
        THROW_IO_ERROR( wxString::Format( _( "Footprint library '%s' is read only." ),
                                          m_lib_raw_path ) );
    }

    for( FP_CACHE_FOOTPRINT_MAP::iterator it = m_footprints.begin(); it != m_footprints.end(); ++it )
    {
        if( aFootprint && aFootprint != it->second->GetFootprint() )
            continue;

        WX_FILENAME fn = it->second->GetFileName();

        wxString tempFileName =
#ifdef USE_TMP_FILE
        wxFileName::CreateTempFileName( fn.GetPath() );
#else
        fn.GetFullPath();
#endif
        // Allow file output stream to go out of scope to close the file stream before
        // renaming the file.
        {
            wxLogTrace( traceKicadPcbPlugin, wxT( "Creating temporary library file '%s'." ),
                    tempFileName );

            FILE_OUTPUTFORMATTER formatter( tempFileName );

            m_owner->SetOutputFormatter( &formatter );
            m_owner->Format( (BOARD_ITEM*) it->second->GetFootprint() );
        }

#ifdef USE_TMP_FILE
        wxRemove( fn.GetFullPath() );     // it is not an error if this does not exist

        // Even on Linux you can see an _intermittent_ error when calling wxRename(),
        // and it is fully inexplicable.  See if this dodges the error.
        wxMilliSleep( 250L );

        if( !wxRenameFile( tempFileName, fn.GetFullPath() ) )
        {
            wxString msg = wxString::Format( _( "Cannot rename temporary file '%s' to '%s'" ),
                                             tempFileName,
                                             fn.GetFullPath() );
            THROW_IO_ERROR( msg );
        }
#endif
        m_cache_timestamp += fn.GetTimestamp();
    }

    m_cache_timestamp += m_lib_path.GetModificationTime().GetValue().GetValue();

    // If we've saved the full cache, we clear the dirty flag.
    if( !aFootprint )
        m_cache_dirty = false;
}


void FP_CACHE::Load()
{
    m_cache_dirty = false;
    m_cache_timestamp = 0;

    wxDir dir( m_lib_raw_path );

    if( !dir.IsOpened() )
    {
        wxString msg = wxString::Format( _( "Footprint library '%s' not found." ),
                                         m_lib_raw_path );
        THROW_IO_ERROR( msg );
    }

    wxString fullName;
    wxString fileSpec = wxT( "*." ) + KiCadFootprintFileExtension;

    // wxFileName construction is egregiously slow.  Construct it once and just swap out
    // the filename thereafter.
    WX_FILENAME fn( m_lib_raw_path, wxT( "dummyName" ) );

    if( dir.GetFirst( &fullName, fileSpec ) )
    {
        wxString cacheError;

        do
        {
            fn.SetFullName( fullName );

            // Queue I/O errors so only files that fail to parse don't get loaded.
            try
            {
                FILE_LINE_READER reader( fn.GetFullPath() );
                PCB_PARSER       parser( &reader, nullptr, nullptr );

                FOOTPRINT* footprint = dynamic_cast<FOOTPRINT*>( parser.Parse() );
                wxString fpName = fn.GetName();

                if( !footprint )
                {
                    THROW_IO_ERROR( wxString::Format( _( "Unable to read file '%s'" ),
                                                      fn.GetFullPath() ) );
                }

                footprint->SetFPID( LIB_ID( wxEmptyString, fpName ) );
                m_footprints.insert( fpName, new FP_CACHE_ITEM( footprint, fn ) );
            }
            catch( const IO_ERROR& ioe )
            {
                if( !cacheError.IsEmpty() )
                    cacheError += wxT( "\n\n" );

                cacheError += ioe.What();
            }
        } while( dir.GetNext( &fullName ) );

        m_cache_timestamp = GetTimestamp( m_lib_raw_path );

        if( !cacheError.IsEmpty() )
            THROW_IO_ERROR( cacheError );
    }
}


void FP_CACHE::Remove( const wxString& aFootprintName )
{
    FP_CACHE_FOOTPRINT_MAP::const_iterator it = m_footprints.find( aFootprintName );

    if( it == m_footprints.end() )
    {
        wxString msg = wxString::Format( _( "Library '%s' has no footprint '%s'." ),
                                         m_lib_raw_path,
                                         aFootprintName );
        THROW_IO_ERROR( msg );
    }

    // Remove the footprint from the cache and delete the footprint file from the library.
    wxString fullPath = it->second->GetFileName().GetFullPath();
    m_footprints.erase( aFootprintName );
    wxRemoveFile( fullPath );
}


bool FP_CACHE::IsPath( const wxString& aPath ) const
{
    return aPath == m_lib_raw_path;
}


void FP_CACHE::SetPath( const wxString& aPath )
{
    m_lib_raw_path = aPath;
    m_lib_path.SetPath( aPath );


    for( const auto& footprint : GetFootprints() )
    {
        footprint.second->SetFilePath( aPath );
    }
}


bool FP_CACHE::IsModified()
{
    m_cache_dirty = m_cache_dirty || GetTimestamp( m_lib_path.GetFullPath() ) != m_cache_timestamp;

    return m_cache_dirty;
}


long long FP_CACHE::GetTimestamp( const wxString& aLibPath )
{
    wxString fileSpec = wxT( "*." ) + KiCadFootprintFileExtension;

    return TimestampDir( aLibPath, fileSpec );
}


void PCB_PLUGIN::Save( const wxString& aFileName, BOARD* aBoard, const STRING_UTF8_MAP* aProperties )
{
    LOCALE_IO   toggle;     // toggles on, then off, the C locale.

    wxString sanityResult = aBoard->GroupsSanityCheck();

    if( sanityResult != wxEmptyString && m_queryUserCallback )
    {
        if( !(*m_queryUserCallback)(
                    _( "Internal Group Data Error" ), wxICON_ERROR,
                    wxString::Format( _( "Please report this bug.  Error validating group "
                                         "structure: %s\n\nSave anyway?" ), sanityResult ),
                    _( "Save Anyway" ) ) )
        {
            return;
        }
    }

    init( aProperties );

    m_board = aBoard;       // after init()

    // Prepare net mapping that assures that net codes saved in a file are consecutive integers
    m_mapping->SetBoard( aBoard );

    FILE_OUTPUTFORMATTER    formatter( aFileName );

    m_out = &formatter;     // no ownership

    m_out->Print( 0, "(kicad_pcb (version %d) (generator pcbnew)\n", SEXPR_BOARD_FILE_VERSION );

    Format( aBoard, 1 );

    m_out->Print( 0, ")\n" );

    m_out = nullptr;
}


BOARD_ITEM* PCB_PLUGIN::Parse( const wxString& aClipboardSourceInput )
{
    std::string input = TO_UTF8( aClipboardSourceInput );

    STRING_LINE_READER reader( input, wxT( "clipboard" ) );
    PCB_PARSER         parser( &reader, nullptr, m_queryUserCallback );

    try
    {
        return parser.Parse();
    }
    catch( const PARSE_ERROR& parse_error )
    {
        if( parser.IsTooRecent() )
            throw FUTURE_FORMAT_ERROR( parse_error, parser.GetRequiredVersion() );
        else
            throw;
    }
}


void PCB_PLUGIN::Format( const BOARD_ITEM* aItem, int aNestLevel ) const
{
    LOCALE_IO   toggle;     // public API function, perform anything convenient for caller

    switch( aItem->Type() )
    {
    case PCB_T:
        format( static_cast<const BOARD*>( aItem ), aNestLevel );
        break;

    case PCB_DIM_ALIGNED_T:
    case PCB_DIM_CENTER_T:
    case PCB_DIM_RADIAL_T:
    case PCB_DIM_ORTHOGONAL_T:
    case PCB_DIM_LEADER_T:
        format( static_cast<const PCB_DIMENSION_BASE*>( aItem ), aNestLevel );
        break;

    case PCB_SHAPE_T:
        format( static_cast<const PCB_SHAPE*>( aItem ), aNestLevel );
        break;

    case PCB_BITMAP_T:
        format( static_cast<const PCB_BITMAP*>( aItem ), aNestLevel );
        break;

    case PCB_TARGET_T:
        format( static_cast<const PCB_TARGET*>( aItem ), aNestLevel );
        break;

    case PCB_FOOTPRINT_T:
        format( static_cast<const FOOTPRINT*>( aItem ), aNestLevel );
        break;

    case PCB_PAD_T:
        format( static_cast<const PAD*>( aItem ), aNestLevel );
        break;

    case PCB_TEXT_T:
        format( static_cast<const PCB_TEXT*>( aItem ), aNestLevel );
        break;

    case PCB_TEXTBOX_T:
        format( static_cast<const PCB_TEXTBOX*>( aItem ), aNestLevel );
        break;

    case PCB_GROUP_T:
        format( static_cast<const PCB_GROUP*>( aItem ), aNestLevel );
        break;

    case PCB_TRACE_T:
    case PCB_ARC_T:
    case PCB_VIA_T:
        format( static_cast<const PCB_TRACK*>( aItem ), aNestLevel );
        break;

    case PCB_ZONE_T:
        format( static_cast<const ZONE*>( aItem ), aNestLevel );
        break;

    default:
        wxFAIL_MSG( wxT( "Cannot format item " ) + aItem->GetClass() );
    }
}


std::string formatInternalUnits( int aValue )
{
    return EDA_UNIT_UTILS::FormatInternalUnits( pcbIUScale, aValue );
}


std::string formatInternalUnits( const VECTOR2I& aCoord )
{
    return EDA_UNIT_UTILS::FormatInternalUnits( pcbIUScale, aCoord );
}


std::string formatInternalUnits( const VECTOR2I& aCoord, const FOOTPRINT* aParentFP )
{
    if( aParentFP )
    {
        VECTOR2I coord = aCoord - aParentFP->GetPosition();
        RotatePoint( coord, -aParentFP->GetOrientation() );
        return formatInternalUnits( coord );
    }

    return formatInternalUnits( aCoord );
}


void PCB_PLUGIN::formatLayer( PCB_LAYER_ID aLayer, bool aIsKnockout ) const
{
    m_out->Print( 0, " (layer %s%s)",
                  m_out->Quotew( LSET::Name( aLayer ) ).c_str(),
                  aIsKnockout ? " knockout" : "" );
}


void PCB_PLUGIN::formatPolyPts( const SHAPE_LINE_CHAIN& outline, int aNestLevel,
                                bool aCompact, const FOOTPRINT* aParentFP ) const
{
    m_out->Print( aNestLevel + 1, "(pts\n" );

    bool needNewline = false;
    int  nestLevel = aNestLevel + 2;
    int  shapesAdded = 0;

    for( int ii = 0; ii < outline.PointCount();  ++ii )
    {
        int ind = outline.ArcIndex( ii );

        if( ind < 0 )
        {
            m_out->Print( nestLevel, "(xy %s)",
                          formatInternalUnits( outline.CPoint( ii ), aParentFP ).c_str() );
            needNewline = true;
        }
        else
        {
            const SHAPE_ARC& arc = outline.Arc( ind );
            m_out->Print( nestLevel, "(arc (start %s) (mid %s) (end %s))",
                          formatInternalUnits( arc.GetP0(), aParentFP ).c_str(),
                          formatInternalUnits( arc.GetArcMid(), aParentFP ).c_str(),
                          formatInternalUnits( arc.GetP1(), aParentFP ).c_str() );
            needNewline = true;

            do
            {
                ++ii;
            } while( ii < outline.PointCount() && outline.ArcIndex( ii ) == ind );

            --ii;
        }

        ++shapesAdded;

        if( !( shapesAdded % 4 ) || !aCompact )
        {
            // newline every 4 shapes if compact save
            m_out->Print( 0, "\n" );
            needNewline = false;
        }
    }

    if( needNewline )
        m_out->Print( 0, "\n" );

    m_out->Print( aNestLevel + 1, ")\n" );
}


void PCB_PLUGIN::formatRenderCache( const EDA_TEXT* aText, int aNestLevel ) const
{
    const wxString& shownText = aText->GetShownText( true );
    std::vector<std::unique_ptr<KIFONT::GLYPH>>* cache = aText->GetRenderCache( aText->GetFont(),
                                                                                shownText );

    m_out->Print( aNestLevel, "(render_cache %s %s\n",
                  m_out->Quotew( shownText ).c_str(),
                  EDA_UNIT_UTILS::FormatAngle( aText->GetDrawRotation() ).c_str() );

    for( const std::unique_ptr<KIFONT::GLYPH>& baseGlyph : *cache )
    {
        KIFONT::OUTLINE_GLYPH* glyph = static_cast<KIFONT::OUTLINE_GLYPH*>( baseGlyph.get() );

        if( glyph->OutlineCount() > 0 )
        {
            for( int ii = 0; ii < glyph->OutlineCount(); ++ii )
            {
                m_out->Print( aNestLevel + 1, "(polygon\n" );

                formatPolyPts( glyph->Outline( ii ), aNestLevel + 1, true );

                for( int jj = 0; jj < glyph->HoleCount( ii ); ++jj )
                    formatPolyPts( glyph->Hole( ii, jj ), aNestLevel + 2, true );

                m_out->Print( aNestLevel + 1, ")\n" );
            }
        }
    }

    m_out->Print( aNestLevel, ")\n" );
}


void PCB_PLUGIN::formatSetup( const BOARD* aBoard, int aNestLevel ) const
{
    // Setup
    m_out->Print( aNestLevel, "(setup\n" );

    // Save the board physical stackup structure
    const BOARD_STACKUP& stackup = aBoard->GetDesignSettings().GetStackupDescriptor();

    if( aBoard->GetDesignSettings().m_HasStackup )
        stackup.FormatBoardStackup( m_out, aBoard, aNestLevel+1 );

    BOARD_DESIGN_SETTINGS& dsnSettings = aBoard->GetDesignSettings();

    m_out->Print( aNestLevel+1, "(pad_to_mask_clearance %s)\n",
                  formatInternalUnits( dsnSettings.m_SolderMaskExpansion ).c_str() );

    if( dsnSettings.m_SolderMaskMinWidth )
    {
        m_out->Print( aNestLevel+1, "(solder_mask_min_width %s)\n",
                      formatInternalUnits( dsnSettings.m_SolderMaskMinWidth ).c_str() );
    }

    if( dsnSettings.m_SolderPasteMargin != 0 )
    {
        m_out->Print( aNestLevel+1, "(pad_to_paste_clearance %s)\n",
                      formatInternalUnits( dsnSettings.m_SolderPasteMargin ).c_str() );
    }

    if( dsnSettings.m_SolderPasteMarginRatio != 0 )
    {
        m_out->Print( aNestLevel+1, "(pad_to_paste_clearance_ratio %s)\n",
                      FormatDouble2Str( dsnSettings.m_SolderPasteMarginRatio ).c_str() );
    }

    if( dsnSettings.m_AllowSoldermaskBridgesInFPs )
    {
        m_out->Print( aNestLevel+1, "(allow_soldermask_bridges_in_footprints yes)\n" );
    }

    VECTOR2I origin = dsnSettings.GetAuxOrigin();

    if( origin != VECTOR2I( 0, 0 ) )
    {
        m_out->Print( aNestLevel+1, "(aux_axis_origin %s %s)\n",
                      formatInternalUnits( origin.x ).c_str(),
                      formatInternalUnits( origin.y ).c_str() );
    }

    origin = dsnSettings.GetGridOrigin();

    if( origin != VECTOR2I( 0, 0 ) )
    {
        m_out->Print( aNestLevel+1, "(grid_origin %s %s)\n",
                      formatInternalUnits( origin.x ).c_str(),
                      formatInternalUnits( origin.y ).c_str() );
    }

    aBoard->GetPlotOptions().Format( m_out, aNestLevel+1 );

    m_out->Print( aNestLevel, ")\n\n" );
}


void PCB_PLUGIN::formatGeneral( const BOARD* aBoard, int aNestLevel ) const
{
    const BOARD_DESIGN_SETTINGS& dsnSettings = aBoard->GetDesignSettings();

    m_out->Print( 0, "\n" );
    m_out->Print( aNestLevel, "(general\n" );
    m_out->Print( aNestLevel+1, "(thickness %s)\n",
                  formatInternalUnits( dsnSettings.GetBoardThickness() ).c_str() );

    m_out->Print( aNestLevel, ")\n\n" );

    aBoard->GetPageSettings().Format( m_out, aNestLevel, m_ctl );
    aBoard->GetTitleBlock().Format( m_out, aNestLevel, m_ctl );
}


void PCB_PLUGIN::formatBoardLayers( const BOARD* aBoard, int aNestLevel ) const
{
    m_out->Print( aNestLevel, "(layers\n" );

    // Save only the used copper layers from front to back.

    for( LSEQ cu = aBoard->GetEnabledLayers().CuStack();  cu;  ++cu )
    {
        PCB_LAYER_ID layer = *cu;

        m_out->Print( aNestLevel+1, "(%d %s %s", layer,
                      m_out->Quotew( LSET::Name( layer ) ).c_str(),
                      LAYER::ShowType( aBoard->GetLayerType( layer ) ) );

        if( LSET::Name( layer ) != m_board->GetLayerName( layer ) )
            m_out->Print( 0, " %s", m_out->Quotew( m_board->GetLayerName( layer ) ).c_str() );

        m_out->Print( 0, ")\n" );
    }

    // Save used non-copper layers in the order they are defined.
    // desired sequence for non Cu BOARD layers.
    static const PCB_LAYER_ID non_cu[] =
    {
        B_Adhes,        // 32
        F_Adhes,
        B_Paste,
        F_Paste,
        B_SilkS,
        F_SilkS,
        B_Mask,
        F_Mask,
        Dwgs_User,
        Cmts_User,
        Eco1_User,
        Eco2_User,
        Edge_Cuts,
        Margin,
        B_CrtYd,
        F_CrtYd,
        B_Fab,
        F_Fab,
        User_1,
        User_2,
        User_3,
        User_4,
        User_5,
        User_6,
        User_7,
        User_8,
        User_9
    };

    for( LSEQ seq = aBoard->GetEnabledLayers().Seq( non_cu, arrayDim( non_cu ) ); seq; ++seq )
    {
        PCB_LAYER_ID layer = *seq;

        m_out->Print( aNestLevel+1, "(%d %s user", layer,
                      m_out->Quotew( LSET::Name( layer ) ).c_str() );

        if( m_board->GetLayerName( layer ) != LSET::Name( layer ) )
            m_out->Print( 0, " %s", m_out->Quotew( m_board->GetLayerName( layer ) ).c_str() );

        m_out->Print( 0, ")\n" );
    }

    m_out->Print( aNestLevel, ")\n\n" );
}


void PCB_PLUGIN::formatNetInformation( const BOARD* aBoard, int aNestLevel ) const
{
    for( NETINFO_ITEM* net : *m_mapping )
    {
        if( net == nullptr )    // Skip not actually existing nets (orphan nets)
            continue;

        m_out->Print( aNestLevel, "(net %d %s)\n",
                                  m_mapping->Translate( net->GetNetCode() ),
                                  m_out->Quotew( net->GetNetname() ).c_str() );
    }

    m_out->Print( 0, "\n" );
}


void PCB_PLUGIN::formatProperties( const BOARD* aBoard, int aNestLevel ) const
{
    for( const std::pair<const wxString, wxString>& prop : aBoard->GetProperties() )
    {
        m_out->Print( aNestLevel, "(property %s %s)\n",
                      m_out->Quotew( prop.first ).c_str(),
                      m_out->Quotew( prop.second ).c_str() );
    }

    if( !aBoard->GetProperties().empty() )
        m_out->Print( 0, "\n" );
}


void PCB_PLUGIN::formatHeader( const BOARD* aBoard, int aNestLevel ) const
{
    formatGeneral( aBoard, aNestLevel );

    // Layers list.
    formatBoardLayers( aBoard, aNestLevel );

    // Setup
    formatSetup( aBoard, aNestLevel );

    // Properties
    formatProperties( aBoard, aNestLevel );

    // Save net codes and names
    formatNetInformation( aBoard, aNestLevel );
}


void PCB_PLUGIN::format( const BOARD* aBoard, int aNestLevel ) const
{
    std::set<BOARD_ITEM*, BOARD_ITEM::ptr_cmp> sorted_footprints( aBoard->Footprints().begin(),
                                                                  aBoard->Footprints().end() );
    std::set<BOARD_ITEM*, BOARD_ITEM::ptr_cmp> sorted_drawings( aBoard->Drawings().begin(),
                                                                aBoard->Drawings().end() );
    std::set<PCB_TRACK*, PCB_TRACK::cmp_tracks> sorted_tracks( aBoard->Tracks().begin(),
                                                               aBoard->Tracks().end() );
    std::set<BOARD_ITEM*, BOARD_ITEM::ptr_cmp> sorted_zones( aBoard->Zones().begin(),
                                                             aBoard->Zones().end() );
    std::set<BOARD_ITEM*, BOARD_ITEM::ptr_cmp> sorted_groups( aBoard->Groups().begin(),
                                                              aBoard->Groups().end() );
    formatHeader( aBoard, aNestLevel );

    // Save the footprints.
    for( BOARD_ITEM* footprint : sorted_footprints )
    {
        Format( footprint, aNestLevel );
        m_out->Print( 0, "\n" );
    }

    // Save the graphical items on the board (not owned by a footprint)
    for( BOARD_ITEM* item : sorted_drawings )
        Format( item, aNestLevel );

    if( sorted_drawings.size() )
        m_out->Print( 0, "\n" );

    // Do not save PCB_MARKERs, they can be regenerated easily.

    // Save the tracks and vias.
    for( PCB_TRACK* track : sorted_tracks )
        Format( track, aNestLevel );

    if( sorted_tracks.size() )
        m_out->Print( 0, "\n" );

    // Save the polygon (which are the newer technology) zones.
    for( auto zone : sorted_zones )
        Format( zone, aNestLevel );

    // Save the groups
    for( BOARD_ITEM* group : sorted_groups )
        Format( group, aNestLevel );
}


void PCB_PLUGIN::format( const PCB_DIMENSION_BASE* aDimension, int aNestLevel ) const
{
    const PCB_DIM_ALIGNED*    aligned = dynamic_cast<const PCB_DIM_ALIGNED*>( aDimension );
    const PCB_DIM_ORTHOGONAL* ortho   = dynamic_cast<const PCB_DIM_ORTHOGONAL*>( aDimension );
    const PCB_DIM_CENTER*     center  = dynamic_cast<const PCB_DIM_CENTER*>( aDimension );
    const PCB_DIM_RADIAL*     radial  = dynamic_cast<const PCB_DIM_RADIAL*>( aDimension );
    const PCB_DIM_LEADER*     leader  = dynamic_cast<const PCB_DIM_LEADER*>( aDimension );

    m_out->Print( aNestLevel, "(dimension" );

    if( aDimension->IsLocked() )
        m_out->Print( 0, " locked" );

    if( ortho ) // must be tested before aligned, because ortho is derived from aligned
                // and aligned is not null
        m_out->Print( 0, " (type orthogonal)" );
    else if( aligned )
        m_out->Print( 0, " (type aligned)" );
    else if( leader )
        m_out->Print( 0, " (type leader)" );
    else if( center )
        m_out->Print( 0, " (type center)" );
    else if( radial )
        m_out->Print( 0, " (type radial)" );
    else
        wxFAIL_MSG( wxT( "Cannot format unknown dimension type!" ) );

    formatLayer( aDimension->GetLayer() );

    m_out->Print( 0, " (tstamp %s)", TO_UTF8( aDimension->m_Uuid.AsString() ) );

    m_out->Print( 0, "\n" );

    m_out->Print( aNestLevel+1, "(pts (xy %s %s) (xy %s %s))\n",
                  formatInternalUnits( aDimension->GetStart().x ).c_str(),
                  formatInternalUnits( aDimension->GetStart().y ).c_str(),
                  formatInternalUnits( aDimension->GetEnd().x ).c_str(),
                  formatInternalUnits( aDimension->GetEnd().y ).c_str() );

    if( aligned )
    {
        m_out->Print( aNestLevel+1, "(height %s)\n",
                      formatInternalUnits( aligned->GetHeight() ).c_str() );
    }

    if( radial )
    {
        m_out->Print( aNestLevel+1, "(leader_length %s)\n",
                      formatInternalUnits( radial->GetLeaderLength() ).c_str() );
    }

    if( ortho )
    {
        m_out->Print( aNestLevel+1, "(orientation %d)\n",
                      static_cast<int>( ortho->GetOrientation() ) );
    }

    if( !center )
    {
        format( static_cast<const PCB_TEXT*>( aDimension ), aNestLevel + 1 );

        m_out->Print( aNestLevel + 1, "(format (prefix %s) (suffix %s) (units %d) (units_format %d) (precision %d)",
                      m_out->Quotew( aDimension->GetPrefix() ).c_str(),
                      m_out->Quotew( aDimension->GetSuffix() ).c_str(),
                      static_cast<int>( aDimension->GetUnitsMode() ),
                      static_cast<int>( aDimension->GetUnitsFormat() ),
                      static_cast<int>(  aDimension->GetPrecision() ) );

        if( aDimension->GetOverrideTextEnabled() )
        {
            m_out->Print( 0, " (override_value %s)",
                          m_out->Quotew( aDimension->GetOverrideText() ).c_str() );
        }

        if( aDimension->GetSuppressZeroes() )
            m_out->Print( 0, " suppress_zeroes" );

        m_out->Print( 0, ")\n" );
    }

    m_out->Print( aNestLevel+1, "(style (thickness %s) (arrow_length %s) (text_position_mode %d)",
                  formatInternalUnits( aDimension->GetLineThickness() ).c_str(),
                  formatInternalUnits( aDimension->GetArrowLength() ).c_str(),
                  static_cast<int>( aDimension->GetTextPositionMode() ) );

    if( aligned )
    {
        m_out->Print( 0, " (extension_height %s)",
                      formatInternalUnits( aligned->GetExtensionHeight() ).c_str() );
    }

    if( leader )
        m_out->Print( 0, " (text_frame %d)", static_cast<int>( leader->GetTextBorder() ) );

    m_out->Print( 0, " (extension_offset %s)",
                  formatInternalUnits( aDimension->GetExtensionOffset() ).c_str() );

    if( aDimension->GetKeepTextAligned() )
        m_out->Print( 0, " keep_text_aligned" );

    m_out->Print( 0, ")\n" );

    m_out->Print( aNestLevel, ")\n" );
}


void PCB_PLUGIN::format( const PCB_SHAPE* aShape, int aNestLevel ) const
{
    FOOTPRINT*  parentFP = aShape->GetParentFootprint();
    std::string prefix = parentFP ? "fp" : "gr";
    std::string locked = aShape->IsLocked() ? " locked" : "";

    switch( aShape->GetShape() )
    {
    case SHAPE_T::SEGMENT:
        m_out->Print( aNestLevel, "(%s_line%s (start %s) (end %s)\n",
                      prefix.c_str(),
                      locked.c_str(),
                      formatInternalUnits( aShape->GetStart(), parentFP ).c_str(),
                      formatInternalUnits( aShape->GetEnd(), parentFP ).c_str() );
        break;

    case SHAPE_T::RECT:
        m_out->Print( aNestLevel, "(%s_rect%s (start %s) (end %s)\n",
                      prefix.c_str(),
                      locked.c_str(),
                      formatInternalUnits( aShape->GetStart(), parentFP ).c_str(),
                      formatInternalUnits( aShape->GetEnd(), parentFP ).c_str() );
        break;

    case SHAPE_T::CIRCLE:
        m_out->Print( aNestLevel, "(%s_circle%s (center %s) (end %s)\n",
                      prefix.c_str(),
                      locked.c_str(),
                      formatInternalUnits( aShape->GetStart(), parentFP ).c_str(),
                      formatInternalUnits( aShape->GetEnd(), parentFP ).c_str() );
        break;

    case SHAPE_T::ARC:
        m_out->Print( aNestLevel, "(%s_arc%s (start %s) (mid %s) (end %s)\n",
                      prefix.c_str(),
                      locked.c_str(),
                      formatInternalUnits( aShape->GetStart(), parentFP ).c_str(),
                      formatInternalUnits( aShape->GetArcMid(), parentFP ).c_str(),
                      formatInternalUnits( aShape->GetEnd(), parentFP ).c_str() );
        break;

    case SHAPE_T::POLY:
        if( aShape->IsPolyShapeValid() )
        {
            const SHAPE_POLY_SET& poly = aShape->GetPolyShape();
            const SHAPE_LINE_CHAIN& outline = poly.Outline( 0 );

            m_out->Print( aNestLevel, "(%s_poly%s\n",
                          prefix.c_str(),
                          locked.c_str() );
            formatPolyPts( outline, aNestLevel, ADVANCED_CFG::GetCfg().m_CompactSave, parentFP );
        }
        else
        {
            wxFAIL_MSG( wxT( "Cannot format invalid polygon." ) );
            return;
        }

        break;

    case SHAPE_T::BEZIER:
        m_out->Print( aNestLevel, "(%s_curve%s (pts (xy %s) (xy %s) (xy %s) (xy %s))\n",
                      prefix.c_str(),
                      locked.c_str(),
                      formatInternalUnits( aShape->GetStart(), parentFP ).c_str(),
                      formatInternalUnits( aShape->GetBezierC1(), parentFP ).c_str(),
                      formatInternalUnits( aShape->GetBezierC2(), parentFP ).c_str(),
                      formatInternalUnits( aShape->GetEnd(), parentFP ).c_str() );
        break;

    default:
        UNIMPLEMENTED_FOR( aShape->SHAPE_T_asString() );
        return;
    };

    aShape->GetStroke().Format( m_out, pcbIUScale, aNestLevel + 1 );

    // The filled flag represents if a solid fill is present on circles, rectangles and polygons
    if( ( aShape->GetShape() == SHAPE_T::POLY )
        || ( aShape->GetShape() == SHAPE_T::RECT )
        || ( aShape->GetShape() == SHAPE_T::CIRCLE ) )
    {
        m_out->Print( 0, aShape->IsFilled() ? " (fill solid)" : " (fill none)" );
    }

    formatLayer( aShape->GetLayer() );

    m_out->Print( 0, " (tstamp %s)", TO_UTF8( aShape->m_Uuid.AsString() ) );

    m_out->Print( 0, ")\n" );
}


void PCB_PLUGIN::format( const PCB_BITMAP* aBitmap, int aNestLevel ) const
{
    wxCHECK_RET( aBitmap != nullptr && m_out != nullptr, "" );

    const wxImage* image = aBitmap->GetImage()->GetImageData();

    wxCHECK_RET( image != nullptr, "wxImage* is NULL" );

    m_out->Print( aNestLevel, "(image (at %s %s)",
                  formatInternalUnits( aBitmap->GetPosition().x ).c_str(),
                  formatInternalUnits( aBitmap->GetPosition().y ).c_str() );

    formatLayer( aBitmap->GetLayer() );

    if( aBitmap->GetImage()->GetScale() != 1.0 )
        m_out->Print( 0, " (scale %g)", aBitmap->GetImage()->GetScale() );

    m_out->Print( 0, "\n" );

    m_out->Print( aNestLevel + 1, "(data" );

    wxMemoryOutputStream stream;

    image->SaveFile( stream, wxBITMAP_TYPE_PNG );

    // Write binary data in hexadecimal form (ASCII)
    wxStreamBuffer* buffer = stream.GetOutputStreamBuffer();
    wxString out = wxBase64Encode( buffer->GetBufferStart(), buffer->GetBufferSize() );

    // Apparently the MIME standard character width for base64 encoding is 76 (unconfirmed)
    // so use it in a vein attempt to be standard like.
#define MIME_BASE64_LENGTH 76

    size_t first = 0;

    while( first < out.Length() )
    {
        m_out->Print( 0, "\n" );
        m_out->Print( aNestLevel + 2, "%s", TO_UTF8( out( first, MIME_BASE64_LENGTH ) ) );
        first += MIME_BASE64_LENGTH;
    }

    m_out->Print( 0, "\n" );
    m_out->Print( aNestLevel + 1, ")\n" );  // Closes data token.
    m_out->Print( aNestLevel, ")\n" );      // Closes image token.
}


void PCB_PLUGIN::format( const PCB_TARGET* aTarget, int aNestLevel ) const
{
    m_out->Print( aNestLevel, "(target %s (at %s) (size %s)",
                  ( aTarget->GetShape() ) ? "x" : "plus",
                  formatInternalUnits( aTarget->GetPosition() ).c_str(),
                  formatInternalUnits( aTarget->GetSize() ).c_str() );

    if( aTarget->GetWidth() != 0 )
        m_out->Print( 0, " (width %s)", formatInternalUnits( aTarget->GetWidth() ).c_str() );

    formatLayer( aTarget->GetLayer() );

    m_out->Print( 0, " (tstamp %s)", TO_UTF8( aTarget->m_Uuid.AsString() ) );

    m_out->Print( 0, ")\n" );
}


void PCB_PLUGIN::format( const FOOTPRINT* aFootprint, int aNestLevel ) const
{
    if( !( m_ctl & CTL_OMIT_INITIAL_COMMENTS ) )
    {
        const wxArrayString* initial_comments = aFootprint->GetInitialComments();

        if( initial_comments )
        {
            for( unsigned i = 0; i < initial_comments->GetCount(); ++i )
                m_out->Print( aNestLevel, "%s\n", TO_UTF8( (*initial_comments)[i] ) );

            m_out->Print( 0, "\n" );    // improve readability?
        }
    }

    if( m_ctl & CTL_OMIT_LIBNAME )
    {
        m_out->Print( aNestLevel, "(footprint %s",
                      m_out->Quotes( aFootprint->GetFPID().GetLibItemName() ).c_str() );
    }
    else
    {
        m_out->Print( aNestLevel, "(footprint %s",
                      m_out->Quotes( aFootprint->GetFPID().Format() ).c_str() );
    }

    if( !( m_ctl & CTL_OMIT_FOOTPRINT_VERSION ) )
        m_out->Print( 0, " (version %d) (generator pcbnew)\n ", SEXPR_BOARD_FILE_VERSION );

    if( aFootprint->IsLocked() )
        m_out->Print( 0, " locked" );

    if( aFootprint->IsPlaced() )
        m_out->Print( 0, " placed" );

    formatLayer( aFootprint->GetLayer() );

    m_out->Print( 0, "\n" );

    if( !( m_ctl & CTL_OMIT_TSTAMPS ) )
        m_out->Print( aNestLevel+1, "(tstamp %s)\n", TO_UTF8( aFootprint->m_Uuid.AsString() ) );

    if( !( m_ctl & CTL_OMIT_AT ) )
    {
        m_out->Print( aNestLevel+1, "(at %s", formatInternalUnits( aFootprint->GetPosition() ).c_str() );

        if( !aFootprint->GetOrientation().IsZero() )
            m_out->Print( 0, " %s", EDA_UNIT_UTILS::FormatAngle( aFootprint->GetOrientation() ).c_str() );

        m_out->Print( 0, ")\n" );
    }

    if( !aFootprint->GetDescription().IsEmpty() )
    {
        m_out->Print( aNestLevel+1, "(descr %s)\n",
                      m_out->Quotew( aFootprint->GetDescription() ).c_str() );
    }

    if( !aFootprint->GetKeywords().IsEmpty() )
    {
        m_out->Print( aNestLevel+1, "(tags %s)\n",
                      m_out->Quotew( aFootprint->GetKeywords() ).c_str() );
    }

    const std::map<wxString, wxString>& props = aFootprint->GetProperties();

    for( const std::pair<const wxString, wxString>& prop : props )
    {
        m_out->Print( aNestLevel+1, "(property %s %s)\n",
                      m_out->Quotew( prop.first ).c_str(),
                      m_out->Quotew( prop.second ).c_str() );
    }

    if( !( m_ctl & CTL_OMIT_PATH ) && !aFootprint->GetPath().empty() )
    {
        m_out->Print( aNestLevel+1, "(path %s)\n",
                      m_out->Quotew( aFootprint->GetPath().AsString() ).c_str() );
    }

    if( aFootprint->GetLocalSolderMaskMargin() != 0 )
    {
        m_out->Print( aNestLevel+1, "(solder_mask_margin %s)\n",
                      formatInternalUnits( aFootprint->GetLocalSolderMaskMargin() ).c_str() );
    }

    if( aFootprint->GetLocalSolderPasteMargin() != 0 )
    {
        m_out->Print( aNestLevel+1, "(solder_paste_margin %s)\n",
                      formatInternalUnits( aFootprint->GetLocalSolderPasteMargin() ).c_str() );
    }

    if( aFootprint->GetLocalSolderPasteMarginRatio() != 0 )
    {
        m_out->Print( aNestLevel+1, "(solder_paste_ratio %s)\n",
                      FormatDouble2Str( aFootprint->GetLocalSolderPasteMarginRatio() ).c_str() );
    }

    if( aFootprint->GetLocalClearance() != 0 )
    {
        m_out->Print( aNestLevel+1, "(clearance %s)\n",
                      formatInternalUnits( aFootprint->GetLocalClearance() ).c_str() );
    }

    if( aFootprint->GetZoneConnection() != ZONE_CONNECTION::INHERITED )
    {
        m_out->Print( aNestLevel+1, "(zone_connect %d)\n",
                                    static_cast<int>( aFootprint->GetZoneConnection() ) );
    }

    // Attributes
    if( aFootprint->GetAttributes() )
    {
        m_out->Print( aNestLevel+1, "(attr" );

        if( aFootprint->GetAttributes() & FP_SMD )
            m_out->Print( 0, " smd" );

        if( aFootprint->GetAttributes() & FP_THROUGH_HOLE )
            m_out->Print( 0, " through_hole" );

        if( aFootprint->GetAttributes() & FP_BOARD_ONLY )
            m_out->Print( 0, " board_only" );

        if( aFootprint->GetAttributes() & FP_EXCLUDE_FROM_POS_FILES )
            m_out->Print( 0, " exclude_from_pos_files" );

        if( aFootprint->GetAttributes() & FP_EXCLUDE_FROM_BOM )
            m_out->Print( 0, " exclude_from_bom" );

        if( aFootprint->GetAttributes() & FP_ALLOW_MISSING_COURTYARD )
            m_out->Print( 0, " allow_missing_courtyard" );

        if( aFootprint->GetAttributes() & FP_DNP )
            m_out->Print( 0, " dnp" );

        if( aFootprint->GetAttributes() & FP_ALLOW_SOLDERMASK_BRIDGES )
            m_out->Print( 0, " allow_soldermask_bridges" );

        m_out->Print( 0, ")\n" );
    }

    if( aFootprint->GetPrivateLayers().any() )
    {
        m_out->Print( aNestLevel+1, "(private_layers" );

        for( PCB_LAYER_ID layer : aFootprint->GetPrivateLayers().Seq() )
        {
            wxString canonicalName( LSET::Name( layer ) );
            m_out->Print( 0, " \"%s\"", canonicalName.ToStdString().c_str() );
        }

        m_out->Print( 0, ")\n" );
    }

    if( aFootprint->IsNetTie() )
    {
        m_out->Print( aNestLevel+1, "(net_tie_pad_groups" );

        for( const wxString& group : aFootprint->GetNetTiePadGroups() )
            m_out->Print( 0, " \"%s\"", EscapeString( group, CTX_QUOTED_STR ).ToStdString().c_str() );

        m_out->Print( 0, ")\n" );
    }

    Format( (BOARD_ITEM*) &aFootprint->Reference(), aNestLevel + 1 );
    Format( (BOARD_ITEM*) &aFootprint->Value(), aNestLevel + 1 );

    std::set<PAD*, FOOTPRINT::cmp_pads> sorted_pads( aFootprint->Pads().begin(),
                                                     aFootprint->Pads().end() );
    std::set<BOARD_ITEM*, FOOTPRINT::cmp_drawings> sorted_drawings(
            aFootprint->GraphicalItems().begin(),
            aFootprint->GraphicalItems().end() );
    std::set<ZONE*, FOOTPRINT::cmp_zones> sorted_zones( aFootprint->Zones().begin(),
                                                        aFootprint->Zones().end() );
    std::set<BOARD_ITEM*, PCB_GROUP::ptr_cmp> sorted_groups( aFootprint->Groups().begin(),
                                                             aFootprint->Groups().end() );

    // Save drawing elements.

    for( BOARD_ITEM* gr : sorted_drawings )
        Format( gr, aNestLevel+1 );

    // Save pads.
    for( PAD* pad : sorted_pads )
        Format( pad, aNestLevel+1 );

    // Save zones.
    for( BOARD_ITEM* zone : sorted_zones )
        Format( zone, aNestLevel + 1 );

    // Save groups.
    for( BOARD_ITEM* group : sorted_groups )
        Format( group, aNestLevel + 1 );

    // Save 3D info.
    auto bs3D = aFootprint->Models().begin();
    auto es3D = aFootprint->Models().end();

    while( bs3D != es3D )
    {
        if( !bs3D->m_Filename.IsEmpty() )
        {
            m_out->Print( aNestLevel+1, "(model %s%s\n",
                          m_out->Quotew( bs3D->m_Filename ).c_str(),
                          bs3D->m_Show ? "" : " hide" );

            if( bs3D->m_Opacity != 1.0 )
                m_out->Print( aNestLevel+2, "(opacity %0.4f)", bs3D->m_Opacity );

            m_out->Print( aNestLevel+2, "(offset (xyz %s %s %s))\n",
                          FormatDouble2Str( bs3D->m_Offset.x ).c_str(),
                          FormatDouble2Str( bs3D->m_Offset.y ).c_str(),
                          FormatDouble2Str( bs3D->m_Offset.z ).c_str() );

            m_out->Print( aNestLevel+2, "(scale (xyz %s %s %s))\n",
                          FormatDouble2Str( bs3D->m_Scale.x ).c_str(),
                          FormatDouble2Str( bs3D->m_Scale.y ).c_str(),
                          FormatDouble2Str( bs3D->m_Scale.z ).c_str() );

            m_out->Print( aNestLevel+2, "(rotate (xyz %s %s %s))\n",
                          FormatDouble2Str( bs3D->m_Rotation.x ).c_str(),
                          FormatDouble2Str( bs3D->m_Rotation.y ).c_str(),
                          FormatDouble2Str( bs3D->m_Rotation.z ).c_str() );

            m_out->Print( aNestLevel+1, ")\n" );
        }

        ++bs3D;
    }

    m_out->Print( aNestLevel, ")\n" );
}


void PCB_PLUGIN::formatLayers( LSET aLayerMask, int aNestLevel ) const
{
    std::string output;

    if( aNestLevel == 0 )
        output += ' ';

    output += "(layers";

    static const LSET cu_all( LSET::AllCuMask() );
    static const LSET fr_bk(  2, B_Cu,       F_Cu );
    static const LSET adhes(  2, B_Adhes,    F_Adhes );
    static const LSET paste(  2, B_Paste,    F_Paste );
    static const LSET silks(  2, B_SilkS,    F_SilkS );
    static const LSET mask(   2, B_Mask,     F_Mask );
    static const LSET crt_yd( 2, B_CrtYd,    F_CrtYd );
    static const LSET fab(    2, B_Fab,      F_Fab );

    LSET cu_mask = cu_all;

    // output copper layers first, then non copper

    if( ( aLayerMask & cu_mask ) == cu_mask )
    {
        output += ' ' + m_out->Quotew( "*.Cu" );
        aLayerMask &= ~cu_all;          // clear bits, so they are not output again below
    }
    else if( ( aLayerMask & cu_mask ) == fr_bk )
    {
        output += ' ' + m_out->Quotew( "F&B.Cu" );
        aLayerMask &= ~fr_bk;
    }

    if( ( aLayerMask & adhes ) == adhes )
    {
        output += ' ' + m_out->Quotew( "*.Adhes" );
        aLayerMask &= ~adhes;
    }

    if( ( aLayerMask & paste ) == paste )
    {
        output += ' ' + m_out->Quotew( "*.Paste" );
        aLayerMask &= ~paste;
    }

    if( ( aLayerMask & silks ) == silks )
    {
        output += ' ' + m_out->Quotew( "*.SilkS" );
        aLayerMask &= ~silks;
    }

    if( ( aLayerMask & mask ) == mask )
    {
        output += ' ' + m_out->Quotew( "*.Mask" );
        aLayerMask &= ~mask;
    }

    if( ( aLayerMask & crt_yd ) == crt_yd )
    {
        output += ' ' + m_out->Quotew( "*.CrtYd" );
        aLayerMask &= ~crt_yd;
    }

    if( ( aLayerMask & fab ) == fab )
    {
        output += ' ' + m_out->Quotew( "*.Fab" );
        aLayerMask &= ~fab;
    }

    // output any individual layers not handled in wildcard combos above
    wxString layerName;

    for( int layer = 0; layer < PCB_LAYER_ID_COUNT; ++layer )
    {
        if( aLayerMask[layer] )
        {
            layerName = LSET::Name( PCB_LAYER_ID( layer ) );
            output += ' ';
            output += m_out->Quotew( layerName );
        }
    }

    m_out->Print( aNestLevel, "%s)", output.c_str() );
}


void PCB_PLUGIN::format( const PAD* aPad, int aNestLevel ) const
{
    const BOARD* board = aPad->GetBoard();
    const char*  shape;

    switch( aPad->GetShape() )
    {
    case PAD_SHAPE::CIRCLE:          shape = "circle";       break;
    case PAD_SHAPE::RECT:            shape = "rect";         break;
    case PAD_SHAPE::OVAL:            shape = "oval";         break;
    case PAD_SHAPE::TRAPEZOID:       shape = "trapezoid";    break;
    case PAD_SHAPE::CHAMFERED_RECT:
    case PAD_SHAPE::ROUNDRECT:       shape = "roundrect";    break;
    case PAD_SHAPE::CUSTOM:          shape = "custom";       break;

    default:
        THROW_IO_ERROR( wxString::Format( _( "unknown pad type: %d"), aPad->GetShape() ) );
    }

    const char* type;

    switch( aPad->GetAttribute() )
    {
    case PAD_ATTRIB::PTH:    type = "thru_hole";      break;
    case PAD_ATTRIB::SMD:    type = "smd";            break;
    case PAD_ATTRIB::CONN:   type = "connect";        break;
    case PAD_ATTRIB::NPTH:   type = "np_thru_hole";   break;

    default:
        THROW_IO_ERROR( wxString::Format( wxT( "unknown pad attribute: %d" ),
                                          aPad->GetAttribute() ) );
    }

    const char* property = nullptr;

    switch( aPad->GetProperty() )
    {
    case PAD_PROP::NONE:                                                  break;  // could be "none"
    case PAD_PROP::BGA:              property = "pad_prop_bga";           break;
    case PAD_PROP::FIDUCIAL_GLBL:    property = "pad_prop_fiducial_glob"; break;
    case PAD_PROP::FIDUCIAL_LOCAL:   property = "pad_prop_fiducial_loc";  break;
    case PAD_PROP::TESTPOINT:        property = "pad_prop_testpoint";     break;
    case PAD_PROP::HEATSINK:         property = "pad_prop_heatsink";      break;
    case PAD_PROP::CASTELLATED:      property = "pad_prop_castellated";   break;

    default:
        THROW_IO_ERROR( wxString::Format( wxT( "unknown pad property: %d" ),
                                          aPad->GetProperty() ) );
    }

    m_out->Print( aNestLevel, "(pad %s %s %s",
                  m_out->Quotew( aPad->GetNumber() ).c_str(),
                  type,
                  shape );

    if( aPad->IsLocked() )
        m_out->Print( 0, " locked" );

    m_out->Print( 0, " (at %s", formatInternalUnits( aPad->GetFPRelativePosition() ).c_str() );

    if( !aPad->GetOrientation().IsZero() )
        m_out->Print( 0, " %s", EDA_UNIT_UTILS::FormatAngle( aPad->GetOrientation() ).c_str() );

    m_out->Print( 0, ")" );

    m_out->Print( 0, " (size %s)", formatInternalUnits( aPad->GetSize() ).c_str() );

    if( (aPad->GetDelta().x) != 0 || (aPad->GetDelta().y != 0 ) )
        m_out->Print( 0, " (rect_delta %s)", formatInternalUnits( aPad->GetDelta() ).c_str() );

    VECTOR2I sz = aPad->GetDrillSize();
    VECTOR2I shapeoffset = aPad->GetOffset();

    if( (sz.x > 0) || (sz.y > 0) ||
        (shapeoffset.x != 0) || (shapeoffset.y != 0) )
    {
        m_out->Print( 0, " (drill" );

        if( aPad->GetDrillShape() == PAD_DRILL_SHAPE_OBLONG )
            m_out->Print( 0, " oval" );

        if( sz.x > 0 )
            m_out->Print( 0,  " %s", formatInternalUnits( sz.x ).c_str() );

        if( sz.y > 0  && sz.x != sz.y )
            m_out->Print( 0,  " %s", formatInternalUnits( sz.y ).c_str() );

        if( (shapeoffset.x != 0) || (shapeoffset.y != 0) )
            m_out->Print( 0, " (offset %s)", formatInternalUnits( aPad->GetOffset() ).c_str() );

        m_out->Print( 0, ")" );
    }

    // Add pad property, if exists.
    if( property )
        m_out->Print( 0, " (property %s)", property );

    formatLayers( aPad->GetLayerSet() );

    if( aPad->GetAttribute() == PAD_ATTRIB::PTH )
    {
        if( aPad->GetRemoveUnconnected() )
        {
            m_out->Print( 0, " (remove_unused_layers)" );

            if( aPad->GetKeepTopBottom() )
                m_out->Print( 0, " (keep_end_layers)" );

            if( board )     // Will be nullptr in footprint library
            {
                m_out->Print( 0, " (zone_layer_connections" );

                for( LSEQ cu = board->GetEnabledLayers().CuStack();  cu;  ++cu )
                {
                    if( aPad->GetZoneLayerOverride( *cu ) == ZLO_FORCE_FLASHED )
                        m_out->Print( 0, " %s", m_out->Quotew( LSET::Name( *cu ) ).c_str() );
                }

                m_out->Print( 0, ")" );
            }
        }
    }

    // Output the radius ratio for rounded and chamfered rect pads
    if( aPad->GetShape() == PAD_SHAPE::ROUNDRECT || aPad->GetShape() == PAD_SHAPE::CHAMFERED_RECT)
    {
        m_out->Print( 0,  " (roundrect_rratio %s)",
                      FormatDouble2Str( aPad->GetRoundRectRadiusRatio() ).c_str() );
    }

    // Output the chamfer corners for chamfered rect pads
    if( aPad->GetShape() == PAD_SHAPE::CHAMFERED_RECT)
    {
        m_out->Print( 0, "\n" );

        m_out->Print( aNestLevel+1,  "(chamfer_ratio %s)",
                      FormatDouble2Str( aPad->GetChamferRectRatio() ).c_str() );

        m_out->Print( 0, " (chamfer" );

        if( ( aPad->GetChamferPositions() & RECT_CHAMFER_TOP_LEFT ) )
            m_out->Print( 0,  " top_left" );

        if( ( aPad->GetChamferPositions() & RECT_CHAMFER_TOP_RIGHT ) )
            m_out->Print( 0,  " top_right" );

        if( ( aPad->GetChamferPositions() & RECT_CHAMFER_BOTTOM_LEFT ) )
            m_out->Print( 0,  " bottom_left" );

        if( ( aPad->GetChamferPositions() & RECT_CHAMFER_BOTTOM_RIGHT ) )
            m_out->Print( 0,  " bottom_right" );

        m_out->Print( 0,  ")" );
    }

    std::string output;

    // Unconnected pad is default net so don't save it.
    if( !( m_ctl & CTL_OMIT_PAD_NETS ) && aPad->GetNetCode() != NETINFO_LIST::UNCONNECTED )
    {
        StrPrintf( &output, " (net %d %s)", m_mapping->Translate( aPad->GetNetCode() ),
                   m_out->Quotew( aPad->GetNetname() ).c_str() );
        }

    // Pin functions and types are closely related to nets, so if CTL_OMIT_NETS is set, omit
    // them as well (for instance when saved from library editor).
    if( !( m_ctl & CTL_OMIT_PAD_NETS ) )
    {
        if( !aPad->GetPinFunction().IsEmpty() )
        {
            StrPrintf( &output, " (pinfunction %s)",
                       m_out->Quotew( aPad->GetPinFunction() ).c_str() );
        }

        if( !aPad->GetPinType().IsEmpty() )
        {
            StrPrintf( &output, " (pintype %s)",
                       m_out->Quotew( aPad->GetPinType() ).c_str() );
        }
    }

    if( aPad->GetPadToDieLength() != 0 )
    {
        StrPrintf( &output, " (die_length %s)",
                   formatInternalUnits( aPad->GetPadToDieLength() ).c_str() );
    }

    if( aPad->GetLocalSolderMaskMargin() != 0 )
    {
        StrPrintf( &output, " (solder_mask_margin %s)",
                   formatInternalUnits( aPad->GetLocalSolderMaskMargin() ).c_str() );
    }

    if( aPad->GetLocalSolderPasteMargin() != 0 )
    {
        StrPrintf( &output, " (solder_paste_margin %s)",
                   formatInternalUnits( aPad->GetLocalSolderPasteMargin() ).c_str() );
    }

    if( aPad->GetLocalSolderPasteMarginRatio() != 0 )
    {
        StrPrintf( &output, " (solder_paste_margin_ratio %s)",
                   FormatDouble2Str( aPad->GetLocalSolderPasteMarginRatio() ).c_str() );
    }

    if( aPad->GetLocalClearance() != 0 )
    {
        StrPrintf( &output, " (clearance %s)",
                   formatInternalUnits( aPad->GetLocalClearance() ).c_str() );
    }

    if( aPad->GetZoneConnection() != ZONE_CONNECTION::INHERITED )
    {
        StrPrintf( &output, " (zone_connect %d)",
                   static_cast<int>( aPad->GetZoneConnection() ) );
    }

    if( aPad->GetThermalSpokeWidth() != 0 )
    {
        StrPrintf( &output, " (thermal_bridge_width %s)",
                   formatInternalUnits( aPad->GetThermalSpokeWidth() ).c_str() );
    }

    if( ( aPad->GetShape() == PAD_SHAPE::CIRCLE && aPad->GetThermalSpokeAngle() != ANGLE_45 )
            || ( aPad->GetShape() != PAD_SHAPE::CIRCLE && aPad->GetThermalSpokeAngle() != ANGLE_90 ) )
    {
        StrPrintf( &output, " (thermal_bridge_angle %s)",
                   EDA_UNIT_UTILS::FormatAngle( aPad->GetThermalSpokeAngle() ).c_str() );
    }

    if( aPad->GetThermalGap() != 0 )
    {
        StrPrintf( &output, " (thermal_gap %s)",
                   formatInternalUnits( aPad->GetThermalGap() ).c_str() );
    }

    if( output.size() )
    {
        m_out->Print( 0, "\n" );
        m_out->Print( aNestLevel+1, "%s", output.c_str()+1 );   // +1 skips 1st space on 1st element
    }

    if( aPad->GetShape() == PAD_SHAPE::CUSTOM )
    {
        m_out->Print( 0, "\n");
        m_out->Print( aNestLevel+1, "(options" );

        if( aPad->GetCustomShapeInZoneOpt() == CUST_PAD_SHAPE_IN_ZONE_CONVEXHULL )
            m_out->Print( 0, " (clearance convexhull)" );
        #if 1   // Set to 1 to output the default option
        else
            m_out->Print( 0, " (clearance outline)" );
        #endif

        // Output the anchor pad shape (circle/rect)
        if( aPad->GetAnchorPadShape() == PAD_SHAPE::RECT )
            shape = "rect";
        else
            shape = "circle";

        m_out->Print( 0, " (anchor %s)", shape );

        m_out->Print( 0, ")");  // end of (options ...

        // Output graphic primitive of the pad shape
        m_out->Print( 0, "\n");
        m_out->Print( aNestLevel+1, "(primitives" );

        int nested_level = aNestLevel+2;

        // Output all basic shapes
        for( const std::shared_ptr<PCB_SHAPE>& primitive : aPad->GetPrimitives() )
        {
            m_out->Print( 0, "\n");

            switch( primitive->GetShape() )
            {
            case SHAPE_T::SEGMENT:
                m_out->Print( nested_level, "(gr_line (start %s) (end %s)",
                              formatInternalUnits( primitive->GetStart() ).c_str(),
                              formatInternalUnits( primitive->GetEnd() ).c_str() );
                break;

            case SHAPE_T::RECT:
                if( primitive->IsAnnotationProxy() )
                {
                    m_out->Print( nested_level, "(gr_bbox (start %s) (end %s)",
                                  formatInternalUnits( primitive->GetStart() ).c_str(),
                                  formatInternalUnits( primitive->GetEnd() ).c_str() );
                }
                else
                {
                    m_out->Print( nested_level, "(gr_rect (start %s) (end %s)",
                                  formatInternalUnits( primitive->GetStart() ).c_str(),
                                  formatInternalUnits( primitive->GetEnd() ).c_str() );
                }
                break;

            case SHAPE_T::ARC:
                m_out->Print( nested_level, "(gr_arc (start %s) (mid %s) (end %s)",
                              formatInternalUnits( primitive->GetStart() ).c_str(),
                              formatInternalUnits( primitive->GetArcMid() ).c_str(),
                              formatInternalUnits( primitive->GetEnd() ).c_str() );
                break;

            case SHAPE_T::CIRCLE:
                m_out->Print( nested_level, "(gr_circle (center %s) (end %s)",
                              formatInternalUnits( primitive->GetStart() ).c_str(),
                              formatInternalUnits( primitive->GetEnd() ).c_str() );
                break;

            case SHAPE_T::BEZIER:
                m_out->Print( nested_level, "(gr_curve (pts (xy %s) (xy %s) (xy %s) (xy %s))",
                              formatInternalUnits( primitive->GetStart() ).c_str(),
                              formatInternalUnits( primitive->GetBezierC1() ).c_str(),
                              formatInternalUnits( primitive->GetBezierC2() ).c_str(),
                              formatInternalUnits( primitive->GetEnd() ).c_str() );
                break;

            case SHAPE_T::POLY:
                if( primitive->IsPolyShapeValid() )
                {
                    const SHAPE_POLY_SET& poly = primitive->GetPolyShape();
                    const SHAPE_LINE_CHAIN& outline = poly.Outline( 0 );

                    m_out->Print( nested_level, "(gr_poly\n" );
                    formatPolyPts( outline, nested_level, ADVANCED_CFG::GetCfg().m_CompactSave );
                    m_out->Print( nested_level, " " );  // just to align the next info at the right place
                }
                break;

            default:
                break;
            }

            m_out->Print( 0, " (width %s)", formatInternalUnits( primitive->GetWidth() ).c_str() );

            // The filled flag represents if a solid fill is present on circles,
            // rectangles and polygons
            if( ( primitive->GetShape() == SHAPE_T::POLY )
                || ( primitive->GetShape() == SHAPE_T::RECT )
                || ( primitive->GetShape() == SHAPE_T::CIRCLE ) )
            {
                m_out->Print( 0, primitive->IsFilled() ? " (fill yes)" : " (fill none)" );
            }

            m_out->Print( 0, ")" );
        }

        m_out->Print( 0, "\n");
        m_out->Print( aNestLevel+1, ")" );   // end of (basic_shapes
    }

    m_out->Print( 0, " (tstamp %s)", TO_UTF8( aPad->m_Uuid.AsString() ) );

    m_out->Print( 0, ")\n" );
}


void PCB_PLUGIN::format( const PCB_TEXT* aText, int aNestLevel ) const
{
    FOOTPRINT*  parentFP = aText->GetParentFootprint();
    std::string prefix;
    std::string type;
    VECTOR2I    pos = aText->GetTextPos();

    // Always format dimension text as gr_text
    if( dynamic_cast<const PCB_DIMENSION_BASE*>( aText ) )
        parentFP = nullptr;

    if( parentFP )
    {
        prefix = "fp";

        switch( aText->GetType() )
        {
        case PCB_TEXT::TEXT_is_REFERENCE: type = " reference"; break;
        case PCB_TEXT::TEXT_is_VALUE:     type = " value";     break;
        case PCB_TEXT::TEXT_is_DIVERS:    type = " user";      break;
        }

        pos -= parentFP->GetPosition();
        RotatePoint( pos, -parentFP->GetOrientation() );
    }
    else
    {
        prefix = "gr";
    }

    m_out->Print( aNestLevel, "(%s_text%s%s %s (at %s",
                  prefix.c_str(),
                  type.c_str(),
                  aText->IsLocked() ? " locked" : "",
                  m_out->Quotew( aText->GetText() ).c_str(),
                  formatInternalUnits( pos ).c_str() );

    // Due to Pcbnew history, fp_text angle is saved as an absolute on screen angle.
    if( !aText->GetTextAngle().IsZero() )
        m_out->Print( 0, " %s", EDA_UNIT_UTILS::FormatAngle( aText->GetTextAngle() ).c_str() );

    if( parentFP && !aText->IsKeepUpright() )
        m_out->Print( 0, " unlocked" );

    m_out->Print( 0, ")" );

    formatLayer( aText->GetLayer(), aText->IsKnockout() );

    if( parentFP && !aText->IsVisible() )
        m_out->Print( 0, " hide" );

    m_out->Print( 0, " (tstamp %s)", TO_UTF8( aText->m_Uuid.AsString() ) );

    m_out->Print( 0, "\n" );

    aText->EDA_TEXT::Format( m_out, aNestLevel, m_ctl | CTL_OMIT_HIDE );

    if( aText->GetFont() && aText->GetFont()->IsOutline() )
        formatRenderCache( aText, aNestLevel + 1 );

    m_out->Print( aNestLevel, ")\n" );
}


void PCB_PLUGIN::format( const PCB_TEXTBOX* aTextBox, int aNestLevel ) const
{
    FOOTPRINT*  parentFP = aTextBox->GetParentFootprint();

    m_out->Print( aNestLevel, "(%s_text_box%s %s\n",
                  parentFP ? "fp" : "gr",
                  aTextBox->IsLocked() ? " locked" : "",
                  m_out->Quotew( aTextBox->GetText() ).c_str() );

    if( aTextBox->GetShape() == SHAPE_T::RECT )
    {
        m_out->Print( aNestLevel + 1, "(start %s) (end %s)\n",
                      formatInternalUnits( aTextBox->GetStart(), parentFP ).c_str(),
                      formatInternalUnits( aTextBox->GetEnd(), parentFP ).c_str() );
    }
    else if( aTextBox->GetShape() == SHAPE_T::POLY )
    {
        const SHAPE_POLY_SET& poly = aTextBox->GetPolyShape();
        const SHAPE_LINE_CHAIN& outline = poly.Outline( 0 );

        formatPolyPts( outline, aNestLevel, true, parentFP );
    }
    else
    {
        UNIMPLEMENTED_FOR( aTextBox->SHAPE_T_asString() );
    }

    EDA_ANGLE angle = aTextBox->GetTextAngle();

    if( parentFP )
    {
        angle -= parentFP->GetOrientation();
        angle.Normalize720();
    }

    if( !angle.IsZero() )
        m_out->Print( aNestLevel + 1, "(angle %s)", EDA_UNIT_UTILS::FormatAngle( angle ).c_str() );

    formatLayer( aTextBox->GetLayer() );

    m_out->Print( 0, " (tstamp %s)", TO_UTF8( aTextBox->m_Uuid.AsString() ) );

    m_out->Print( 0, "\n" );

    // PCB_TEXTBOXes are never hidden, so always omit "hide" attribute
    aTextBox->EDA_TEXT::Format( m_out, aNestLevel, m_ctl | CTL_OMIT_HIDE );

    if( aTextBox->GetStroke().GetWidth() > 0 )
        aTextBox->GetStroke().Format( m_out, pcbIUScale, aNestLevel + 1 );

    if( aTextBox->GetFont() && aTextBox->GetFont()->IsOutline() )
        formatRenderCache( aTextBox, aNestLevel + 1 );

    m_out->Print( aNestLevel, ")\n" );
}


void PCB_PLUGIN::format( const PCB_GROUP* aGroup, int aNestLevel ) const
{
    // Don't write empty groups
    if( aGroup->GetItems().empty() )
        return;

    m_out->Print( aNestLevel, "(group %s%s (id %s)\n",
                              m_out->Quotew( aGroup->GetName() ).c_str(),
                              aGroup->IsLocked() ? " locked" : "",
                              TO_UTF8( aGroup->m_Uuid.AsString() ) );

    m_out->Print( aNestLevel + 1, "(members\n" );

    wxArrayString memberIds;

    for( BOARD_ITEM* member : aGroup->GetItems() )
        memberIds.Add( member->m_Uuid.AsString() );

    memberIds.Sort();

    for( const wxString& memberId : memberIds )
        m_out->Print( aNestLevel + 2, "%s\n", TO_UTF8( memberId ) );

    m_out->Print( aNestLevel + 1, ")\n" );  // Close `members` token.
    m_out->Print( aNestLevel, ")\n" );      // Close `group` token.
}


void PCB_PLUGIN::format( const PCB_TRACK* aTrack, int aNestLevel ) const
{
    if( aTrack->Type() == PCB_VIA_T )
    {
        PCB_LAYER_ID  layer1, layer2;

        const PCB_VIA* via = static_cast<const PCB_VIA*>( aTrack );
        const BOARD*   board = via->GetBoard();

        wxCHECK_RET( board != nullptr, wxT( "Via has no parent." ) );

        m_out->Print( aNestLevel, "(via" );

        via->LayerPair( &layer1, &layer2 );

        switch( via->GetViaType() )
        {
        case VIATYPE::THROUGH: //  Default shape not saved.
            break;

        case VIATYPE::BLIND_BURIED:
            m_out->Print( 0, " blind" );
            break;

        case VIATYPE::MICROVIA:
            m_out->Print( 0, " micro" );
            break;

        default:
            THROW_IO_ERROR( wxString::Format( _( "unknown via type %d"  ), via->GetViaType() ) );
        }

        if( via->IsLocked() )
            m_out->Print( 0, " locked" );

        m_out->Print( 0, " (at %s) (size %s)",
                      formatInternalUnits( aTrack->GetStart() ).c_str(),
                      formatInternalUnits( aTrack->GetWidth() ).c_str() );

        // Old boards were using UNDEFINED_DRILL_DIAMETER value in file for via drill when
        // via drill was the netclass value.
        // recent boards always set the via drill to the actual value, but now we need to
        // always store the drill value, because netclass value is not stored in the board file.
        // Otherwise the drill value of some (old) vias can be unknown
        if( via->GetDrill() != UNDEFINED_DRILL_DIAMETER )
            m_out->Print( 0, " (drill %s)", formatInternalUnits( via->GetDrill() ).c_str() );
        else
            m_out->Print( 0, " (drill %s)", formatInternalUnits( via->GetDrillValue() ).c_str() );

        m_out->Print( 0, " (layers %s %s)",
                      m_out->Quotew( LSET::Name( layer1 ) ).c_str(),
                      m_out->Quotew( LSET::Name( layer2 ) ).c_str() );

        if( via->GetRemoveUnconnected() )
        {
            m_out->Print( 0, " (remove_unused_layers)" );

            if( via->GetKeepStartEnd() )
                m_out->Print( 0, " (keep_end_layers)" );
        }

        if( via->GetIsFree() )
            m_out->Print( 0, " (free)" );

        if( via->GetRemoveUnconnected() )
        {
            m_out->Print( 0, " (zone_layer_connections" );

            for( LSEQ cu = board->GetEnabledLayers().CuStack();  cu;  ++cu )
            {
                if( via->GetZoneLayerOverride( *cu ) == ZLO_FORCE_FLASHED )
                    m_out->Print( 0, " %s", m_out->Quotew( LSET::Name( *cu ) ).c_str() );
            }

            m_out->Print( 0, ")" );
        }
    }
    else if( aTrack->Type() == PCB_ARC_T )
    {
        const PCB_ARC* arc = static_cast<const PCB_ARC*>( aTrack );

        m_out->Print( aNestLevel, "(arc%s (start %s) (mid %s) (end %s) (width %s)",
                      arc->IsLocked() ? " locked" : "",
                      formatInternalUnits( arc->GetStart() ).c_str(),
                      formatInternalUnits( arc->GetMid() ).c_str(),
                      formatInternalUnits( arc->GetEnd() ).c_str(),
                      formatInternalUnits( arc->GetWidth() ).c_str() );

        m_out->Print( 0, " (layer %s)", m_out->Quotew( LSET::Name( arc->GetLayer() ) ).c_str() );
    }
    else
    {
        m_out->Print( aNestLevel, "(segment%s (start %s) (end %s) (width %s)",
                      aTrack->IsLocked() ? " locked" : "",
                      formatInternalUnits( aTrack->GetStart() ).c_str(),
                      formatInternalUnits( aTrack->GetEnd() ).c_str(),
                      formatInternalUnits( aTrack->GetWidth() ).c_str() );

        m_out->Print( 0, " (layer %s)", m_out->Quotew( LSET::Name( aTrack->GetLayer() ) ).c_str() );
    }

    m_out->Print( 0, " (net %d)", m_mapping->Translate( aTrack->GetNetCode() ) );

    m_out->Print( 0, " (tstamp %s)", TO_UTF8( aTrack->m_Uuid.AsString() ) );

    m_out->Print( 0, ")\n" );
}


void PCB_PLUGIN::format( const ZONE* aZone, int aNestLevel ) const
{
    // Save the NET info.
    // For keepout and non copper zones, net code and net name are irrelevant
    // so be sure a dummy value is stored, just for ZONE compatibility
    // (perhaps netcode and netname should be not stored)
    bool has_no_net = aZone->GetIsRuleArea() || !aZone->IsOnCopperLayer();

    m_out->Print( aNestLevel, "(zone%s (net %d) (net_name %s)",
                  aZone->IsLocked() ? " locked" : "",
                  has_no_net ? 0 : m_mapping->Translate( aZone->GetNetCode() ),
                  m_out->Quotew( has_no_net ? wxString( wxT("") ) : aZone->GetNetname() ).c_str() );

    // If a zone exists on multiple layers, format accordingly
    if( aZone->GetLayerSet().count() > 1 )
    {
        formatLayers( aZone->GetLayerSet() );
    }
    else
    {
        formatLayer( aZone->GetFirstLayer() );
    }

    m_out->Print( 0, " (tstamp %s)", TO_UTF8( aZone->m_Uuid.AsString() ) );

    if( !aZone->GetZoneName().empty() )
        m_out->Print( 0, " (name %s)", m_out->Quotew( aZone->GetZoneName() ).c_str() );

    // Save the outline aux info
    std::string hatch;

    switch( aZone->GetHatchStyle() )
    {
    default:
    case ZONE_BORDER_DISPLAY_STYLE::NO_HATCH:      hatch = "none"; break;
    case ZONE_BORDER_DISPLAY_STYLE::DIAGONAL_EDGE: hatch = "edge"; break;
    case ZONE_BORDER_DISPLAY_STYLE::DIAGONAL_FULL: hatch = "full"; break;
    }

    m_out->Print( 0, " (hatch %s %s)\n", hatch.c_str(),
                  formatInternalUnits( aZone->GetBorderHatchPitch() ).c_str() );

    if( aZone->GetAssignedPriority() > 0 )
        m_out->Print( aNestLevel+1, "(priority %d)\n", aZone->GetAssignedPriority() );

    // Add teardrop keywords in file: (attr (teardrop (type xxx)))where xxx is the teardrop type
    if( aZone->IsTeardropArea() )
    {
        const char* td_type;

        switch( aZone->GetTeardropAreaType() )
        {
        case TEARDROP_TYPE::TD_VIAPAD:          // a teardrop on a via or pad
            td_type = "padvia";
            break;

        default:
        case TEARDROP_TYPE::TD_TRACKEND:        // a teardrop on a track end
            td_type = "track_end";
            break;
        }

        m_out->Print( aNestLevel+1, "(attr (teardrop (type %s)))\n", td_type );
    }

    m_out->Print( aNestLevel+1, "(connect_pads" );

    switch( aZone->GetPadConnection() )
    {
    default:
    case ZONE_CONNECTION::THERMAL: // Default option not saved or loaded.
        break;

    case ZONE_CONNECTION::THT_THERMAL:
        m_out->Print( 0, " thru_hole_only" );
        break;

    case ZONE_CONNECTION::FULL:
        m_out->Print( 0, " yes" );
        break;

    case ZONE_CONNECTION::NONE:
        m_out->Print( 0, " no" );
        break;
    }

    m_out->Print( 0, " (clearance %s))\n", formatInternalUnits( aZone->GetLocalClearance() ).c_str() );

    m_out->Print( aNestLevel+1, "(min_thickness %s)", formatInternalUnits( aZone->GetMinThickness() ).c_str() );

    // We continue to write this for 3rd-party parsers, but we no longer read it (as of V7).
    m_out->Print( 0, " (filled_areas_thickness no)" );

    m_out->Print( 0, "\n" );

    if( aZone->GetIsRuleArea() )
    {
        m_out->Print( aNestLevel + 1,
                      "(keepout (tracks %s) (vias %s) (pads %s) (copperpour %s) "
                      "(footprints %s))\n",
                      aZone->GetDoNotAllowTracks() ? "not_allowed" : "allowed",
                      aZone->GetDoNotAllowVias() ? "not_allowed" : "allowed",
                      aZone->GetDoNotAllowPads() ? "not_allowed" : "allowed",
                      aZone->GetDoNotAllowCopperPour() ? "not_allowed" : "allowed",
                      aZone->GetDoNotAllowFootprints() ? "not_allowed" : "allowed" );
    }

    m_out->Print( aNestLevel + 1, "(fill" );

    // Default is not filled.
    if( aZone->IsFilled() )
        m_out->Print( 0, " yes" );

    // Default is polygon filled.
    if( aZone->GetFillMode() == ZONE_FILL_MODE::HATCH_PATTERN )
        m_out->Print( 0, " (mode hatch)" );

    m_out->Print( 0, " (thermal_gap %s) (thermal_bridge_width %s)",
                  formatInternalUnits( aZone->GetThermalReliefGap() ).c_str(),
                  formatInternalUnits( aZone->GetThermalReliefSpokeWidth() ).c_str() );

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
            m_out->Print( 0, " (radius %s)", formatInternalUnits( aZone->GetCornerRadius() ).c_str() );
    }

    if( aZone->GetIslandRemovalMode() != ISLAND_REMOVAL_MODE::ALWAYS )
    {
        m_out->Print( 0, " (island_removal_mode %d) (island_area_min %s)",
                      static_cast<int>( aZone->GetIslandRemovalMode() ),
                      formatInternalUnits( aZone->GetMinIslandArea() / pcbIUScale.IU_PER_MM ).c_str() );
    }

    if( aZone->GetFillMode() == ZONE_FILL_MODE::HATCH_PATTERN )
    {
        m_out->Print( 0, "\n" );
        m_out->Print( aNestLevel+2, "(hatch_thickness %s) (hatch_gap %s) (hatch_orientation %s)",
                      formatInternalUnits( aZone->GetHatchThickness() ).c_str(),
                      formatInternalUnits( aZone->GetHatchGap() ).c_str(),
                      FormatDouble2Str( aZone->GetHatchOrientation().AsDegrees() ).c_str() );

        if( aZone->GetHatchSmoothingLevel() > 0 )
        {
            m_out->Print( 0, "\n" );
            m_out->Print( aNestLevel+2, "(hatch_smoothing_level %d) (hatch_smoothing_value %s)",
                          aZone->GetHatchSmoothingLevel(),
                          FormatDouble2Str( aZone->GetHatchSmoothingValue() ).c_str() );
        }

        m_out->Print( 0, "\n" );
        m_out->Print( aNestLevel+2, "(hatch_border_algorithm %s) (hatch_min_hole_area %s)",
                      aZone->GetHatchBorderAlgorithm() ? "hatch_thickness" : "min_thickness",
                      FormatDouble2Str( aZone->GetHatchHoleMinArea() ).c_str() );
    }

    m_out->Print( 0, ")\n" );

    if( aZone->GetNumCorners() )
    {
        SHAPE_POLY_SET::POLYGON poly = aZone->Outline()->Polygon(0);

        for( auto& chain : poly )
        {
            m_out->Print( aNestLevel + 1, "(polygon\n" );
            formatPolyPts( chain, aNestLevel + 1, ADVANCED_CFG::GetCfg().m_CompactSave );
            m_out->Print( aNestLevel + 1, ")\n" );
        }
    }

    // Save the PolysList (filled areas)
    for( PCB_LAYER_ID layer : aZone->GetLayerSet().Seq() )
    {
        const std::shared_ptr<SHAPE_POLY_SET>& fv = aZone->GetFilledPolysList( layer );

        for( int ii = 0; ii < fv->OutlineCount(); ++ii )
        {
            m_out->Print( aNestLevel + 1, "(filled_polygon\n" );
            m_out->Print( aNestLevel + 2, "(layer %s)\n",
                          m_out->Quotew( LSET::Name( layer ) ).c_str() );

            if( aZone->IsIsland( layer, ii ) )
                m_out->Print( aNestLevel + 2, "(island)\n" );

            const SHAPE_LINE_CHAIN& chain = fv->COutline( ii );

            formatPolyPts( chain, aNestLevel + 1, ADVANCED_CFG::GetCfg().m_CompactSave );
            m_out->Print( aNestLevel + 1, ")\n" );
        }
    }

    m_out->Print( aNestLevel, ")\n" );
}


PCB_PLUGIN::PCB_PLUGIN( int aControlFlags ) :
    m_cache( nullptr ),
    m_ctl( aControlFlags ),
    m_mapping( new NETINFO_MAPPING() ),
    m_queryUserCallback( nullptr )
{
    init( nullptr );
    m_out = &m_sf;
}


PCB_PLUGIN::~PCB_PLUGIN()
{
    delete m_cache;
    delete m_mapping;
}


BOARD* PCB_PLUGIN::Load( const wxString& aFileName, BOARD* aAppendToMe,
                         const STRING_UTF8_MAP* aProperties, PROJECT* aProject,
                         PROGRESS_REPORTER* aProgressReporter )
{
    FILE_LINE_READER reader( aFileName );

    unsigned lineCount = 0;

    if( aProgressReporter )
    {
        aProgressReporter->Report( wxString::Format( _( "Loading %s..." ), aFileName ) );

        if( !aProgressReporter->KeepRefreshing() )
            THROW_IO_ERROR( _( "Open cancelled by user." ) );

        while( reader.ReadLine() )
            lineCount++;

        reader.Rewind();
    }

    BOARD* board = DoLoad( reader, aAppendToMe, aProperties, aProgressReporter, lineCount );

    // Give the filename to the board if it's new
    if( !aAppendToMe )
        board->SetFileName( aFileName );

    return board;
}


BOARD* PCB_PLUGIN::DoLoad( LINE_READER& aReader, BOARD* aAppendToMe, const STRING_UTF8_MAP* aProperties,
                           PROGRESS_REPORTER* aProgressReporter, unsigned aLineCount)
{
    init( aProperties );

    PCB_PARSER parser( &aReader, aAppendToMe, m_queryUserCallback, aProgressReporter, aLineCount );
    BOARD*     board;

    try
    {
        board = dynamic_cast<BOARD*>( parser.Parse() );
    }
    catch( const FUTURE_FORMAT_ERROR& )
    {
        // Don't wrap a FUTURE_FORMAT_ERROR in another
        throw;
    }
    catch( const PARSE_ERROR& parse_error )
    {
        if( parser.IsTooRecent() )
            throw FUTURE_FORMAT_ERROR( parse_error, parser.GetRequiredVersion() );
        else
            throw;
    }

    if( !board )
    {
        // The parser loaded something that was valid, but wasn't a board.
        THROW_PARSE_ERROR( _( "This file does not contain a PCB." ), parser.CurSource(),
                           parser.CurLine(), parser.CurLineNumber(), parser.CurOffset() );
    }

    return board;
}


void PCB_PLUGIN::init( const STRING_UTF8_MAP* aProperties )
{
    m_board = nullptr;
    m_reader = nullptr;
    m_props = aProperties;
}


void PCB_PLUGIN::validateCache( const wxString& aLibraryPath, bool checkModified )
{
    if( !m_cache || !m_cache->IsPath( aLibraryPath ) || ( checkModified && m_cache->IsModified() ) )
    {
        // a spectacular episode in memory management:
        delete m_cache;
        m_cache = new FP_CACHE( this, aLibraryPath );
        m_cache->Load();
    }
}


void PCB_PLUGIN::FootprintEnumerate( wxArrayString& aFootprintNames, const wxString& aLibPath,
                                     bool aBestEfforts, const STRING_UTF8_MAP* aProperties )
{
    LOCALE_IO toggle;     // toggles on, then off, the C locale.
    wxDir     dir( aLibPath );
    wxString  errorMsg;

    init( aProperties );

    try
    {
        validateCache( aLibPath );
    }
    catch( const IO_ERROR& ioe )
    {
        errorMsg = ioe.What();
    }

    // Some of the files may have been parsed correctly so we want to add the valid files to
    // the library.

    for( const auto& footprint : m_cache->GetFootprints() )
        aFootprintNames.Add( footprint.first );

    if( !errorMsg.IsEmpty() && !aBestEfforts )
        THROW_IO_ERROR( errorMsg );
}


const FOOTPRINT* PCB_PLUGIN::getFootprint( const wxString& aLibraryPath,
                                           const wxString& aFootprintName,
                                           const STRING_UTF8_MAP* aProperties,
                                           bool checkModified )
{
    LOCALE_IO   toggle;     // toggles on, then off, the C locale.

    init( aProperties );

    try
    {
        validateCache( aLibraryPath, checkModified );
    }
    catch( const IO_ERROR& )
    {
        // do nothing with the error
    }

    FP_CACHE_FOOTPRINT_MAP&       footprints = m_cache->GetFootprints();
    FP_CACHE_FOOTPRINT_MAP::const_iterator it = footprints.find( aFootprintName );

    if( it == footprints.end() )
        return nullptr;

    return it->second->GetFootprint();
}


const FOOTPRINT* PCB_PLUGIN::GetEnumeratedFootprint( const wxString& aLibraryPath,
                                                     const wxString& aFootprintName,
                                                     const STRING_UTF8_MAP* aProperties )
{
    return getFootprint( aLibraryPath, aFootprintName, aProperties, false );
}


bool PCB_PLUGIN::FootprintExists( const wxString& aLibraryPath, const wxString& aFootprintName,
                                  const STRING_UTF8_MAP* aProperties )
{
    // Note: checking the cache sounds like a good idea, but won't catch files which differ
    // only in case.
    //
    // Since this goes out to the native filesystem, we get platform differences (ie: MSW's
    // case-insensitive filesystem) handled "for free".
    // Warning: footprint names frequently contain a point. So be careful when initializing
    // wxFileName, and use a CTOR with extension specified
    wxFileName footprintFile( aLibraryPath, aFootprintName, KiCadFootprintFileExtension );

    return footprintFile.Exists();
}


FOOTPRINT* PCB_PLUGIN::FootprintLoad( const wxString& aLibraryPath,
                                      const wxString& aFootprintName,
                                      bool  aKeepUUID,
                                      const STRING_UTF8_MAP* aProperties )
{
    const FOOTPRINT* footprint = getFootprint( aLibraryPath, aFootprintName, aProperties, true );

    if( footprint )
    {
        FOOTPRINT* copy;

        if( aKeepUUID )
            copy = static_cast<FOOTPRINT*>( footprint->Clone() );
        else
            copy = static_cast<FOOTPRINT*>( footprint->Duplicate() );

        copy->SetParent( nullptr );
        return copy;
    }

    return nullptr;
}


void PCB_PLUGIN::FootprintSave( const wxString& aLibraryPath, const FOOTPRINT* aFootprint,
                                const STRING_UTF8_MAP* aProperties )
{
    LOCALE_IO   toggle;     // toggles on, then off, the C locale.

    init( aProperties );

    // In this public PLUGIN API function, we can safely assume it was
    // called for saving into a library path.
    m_ctl = CTL_FOR_LIBRARY;

    validateCache( aLibraryPath );

    if( !m_cache->IsWritable() )
    {
        if( !m_cache->Exists() )
        {
            const wxString msg = wxString::Format( _( "Library '%s' does not exist.\n"
                                                      "Would you like to create it?"),
                                                      aLibraryPath );

            if( !Pgm().IsGUI()
                || wxMessageBox( msg, _( "Library Not Found" ), wxYES_NO | wxICON_QUESTION )
                           != wxYES )
                return;

            // Save throws its own IO_ERROR on failure, so no need to recreate here
            m_cache->Save( nullptr );
        }
        else
        {
            wxString msg = wxString::Format( _( "Library '%s' is read only." ), aLibraryPath );
            THROW_IO_ERROR( msg );
        }
    }

    wxString footprintName = aFootprint->GetFPID().GetLibItemName();

    FP_CACHE_FOOTPRINT_MAP& footprints = m_cache->GetFootprints();

    // Quietly overwrite footprint and delete footprint file from path for any by same name.
    wxFileName fn( aLibraryPath, aFootprint->GetFPID().GetLibItemName(),
                   KiCadFootprintFileExtension );

    // Write through symlinks, don't replace them
    WX_FILENAME::ResolvePossibleSymlinks( fn );

    if( !fn.IsOk() )
    {
        THROW_IO_ERROR( wxString::Format( _( "Footprint file name '%s' is not valid." ),
                                          fn.GetFullPath() ) );
    }

    if( fn.FileExists() && !fn.IsFileWritable() )
    {
        THROW_IO_ERROR( wxString::Format( _( "Insufficient permissions to delete '%s'." ),
                                          fn.GetFullPath() ) );
    }

    wxString fullPath = fn.GetFullPath();
    wxString fullName = fn.GetFullName();
    FP_CACHE_FOOTPRINT_MAP::const_iterator it = footprints.find( footprintName );

    if( it != footprints.end() )
    {
        wxLogTrace( traceKicadPcbPlugin, wxT( "Removing footprint file '%s'." ), fullPath );
        footprints.erase( footprintName );
        wxRemoveFile( fullPath );
    }

    // I need my own copy for the cache
    FOOTPRINT* footprint = static_cast<FOOTPRINT*>( aFootprint->Clone() );

    // It's orientation should be zero and it should be on the front layer.
    footprint->SetOrientation( ANGLE_0 );

    if( footprint->GetLayer() != F_Cu )
    {
        PCBNEW_SETTINGS* cfg = dynamic_cast<PCBNEW_SETTINGS*>( Kiface().KifaceSettings() );

        if( cfg )
            footprint->Flip( footprint->GetPosition(), cfg->m_FlipLeftRight );
        else
            footprint->Flip( footprint->GetPosition(), false );
    }

    // Detach it from the board
    footprint->SetParent( nullptr );

    wxLogTrace( traceKicadPcbPlugin, wxT( "Creating s-expr footprint file '%s'." ), fullPath );
    footprints.insert( footprintName,
                       new FP_CACHE_ITEM( footprint, WX_FILENAME( fn.GetPath(), fullName ) ) );
    m_cache->Save( footprint );
}


void PCB_PLUGIN::FootprintDelete( const wxString& aLibraryPath, const wxString& aFootprintName,
                                  const STRING_UTF8_MAP* aProperties )
{
    LOCALE_IO   toggle;     // toggles on, then off, the C locale.

    init( aProperties );

    validateCache( aLibraryPath );

    if( !m_cache->IsWritable() )
    {
        THROW_IO_ERROR( wxString::Format( _( "Library '%s' is read only." ),
                                          aLibraryPath.GetData() ) );
    }

    m_cache->Remove( aFootprintName );
}



long long PCB_PLUGIN::GetLibraryTimestamp( const wxString& aLibraryPath ) const
{
    return FP_CACHE::GetTimestamp( aLibraryPath );
}


void PCB_PLUGIN::FootprintLibCreate( const wxString& aLibraryPath, const STRING_UTF8_MAP* aProperties )
{
    if( wxDir::Exists( aLibraryPath ) )
    {
        THROW_IO_ERROR( wxString::Format( _( "Cannot overwrite library path '%s'." ),
                                          aLibraryPath.GetData() ) );
    }

    LOCALE_IO   toggle;

    init( aProperties );

    delete m_cache;
    m_cache = new FP_CACHE( this, aLibraryPath );
    m_cache->Save();
}


bool PCB_PLUGIN::FootprintLibDelete( const wxString& aLibraryPath, const STRING_UTF8_MAP* aProperties )
{
    wxFileName fn;
    fn.SetPath( aLibraryPath );

    // Return if there is no library path to delete.
    if( !fn.DirExists() )
        return false;

    if( !fn.IsDirWritable() )
    {
        THROW_IO_ERROR( wxString::Format( _( "Insufficient permissions to delete folder '%s'." ),
                                          aLibraryPath.GetData() ) );
    }

    wxDir dir( aLibraryPath );

    if( dir.HasSubDirs() )
    {
        THROW_IO_ERROR( wxString::Format( _( "Library folder '%s' has unexpected sub-folders." ),
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
                THROW_IO_ERROR( wxString::Format( _( "Unexpected file '%s' found in library "
                                                     "path '%s'." ),
                                                  files[i].GetData(),
                                                  aLibraryPath.GetData() ) );
            }
        }

        for( i = 0;  i < files.GetCount();  i++ )
            wxRemoveFile( files[i] );
    }

    wxLogTrace( traceKicadPcbPlugin, wxT( "Removing footprint library '%s'." ),
                aLibraryPath.GetData() );

    // Some of the more elaborate wxRemoveFile() crap puts up its own wxLog dialog
    // we don't want that.  we want bare metal portability with no UI here.
    if( !wxRmdir( aLibraryPath ) )
    {
        THROW_IO_ERROR( wxString::Format( _( "Footprint library '%s' cannot be deleted." ),
                                          aLibraryPath.GetData() ) );
    }

    // For some reason removing a directory in Windows is not immediately updated.  This delay
    // prevents an error when attempting to immediately recreate the same directory when over
    // writing an existing library.
#ifdef __WINDOWS__
    wxMilliSleep( 250L );
#endif

    if( m_cache && !m_cache->IsPath( aLibraryPath ) )
    {
        delete m_cache;
        m_cache = nullptr;
    }

    return true;
}


bool PCB_PLUGIN::IsFootprintLibWritable( const wxString& aLibraryPath )
{
    LOCALE_IO   toggle;

    init( nullptr );

    validateCache( aLibraryPath );

    return m_cache->IsWritable();
}
