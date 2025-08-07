/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2012 CERN
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
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

#include <wx/dir.h>
#include <wx/ffile.h>
#include <wx/log.h>
#include <wx/msgdlg.h>
#include <wx/mstream.h>

#include <board.h>
#include <board_design_settings.h>
#include <callback_gal.h>
#include <confirm.h>
#include <convert_basic_shapes_to_polygon.h> // for enum RECT_CHAMFER_POSITIONS definition
#include <fmt/core.h>
#include <font/fontconfig.h>
#include <footprint.h>
#include <io/kicad/kicad_io_utils.h>
#include <kiface_base.h>
#include <layer_range.h>
#include <macros.h>
#include <pad.h>
#include <pcb_dimension.h>
#include <pcb_generator.h>
#include <pcb_group.h>
#include <pcb_io/kicad_sexpr/pcb_io_kicad_sexpr.h>
#include <pcb_io/kicad_sexpr/pcb_io_kicad_sexpr_parser.h>
#include <pcb_reference_image.h>
#include <pcb_shape.h>
#include <pcb_table.h>
#include <pcb_tablecell.h>
#include <pcb_target.h>
#include <pcb_text.h>
#include <pcb_textbox.h>
#include <pcb_track.h>
#include <pcbnew_settings.h>
#include <pgm_base.h>
#include <progress_reporter.h>
#include <reporter.h>
#include <string_utils.h>
#include <trace_helpers.h>
#include <wildcards_and_files_ext.h>
#include <zone.h>

#include <build_version.h>
#include <filter_reader.h>
#include <ctl_flags.h>


using namespace PCB_KEYS_T;


FP_CACHE_ENTRY::FP_CACHE_ENTRY( FOOTPRINT* aFootprint, const WX_FILENAME& aFileName ) :
        m_filename( aFileName ),
        m_footprint( aFootprint )
{ }


FP_CACHE::FP_CACHE( PCB_IO_KICAD_SEXPR* aOwner, const wxString& aLibraryPath )
{
    m_owner = aOwner;
    m_lib_raw_path = aLibraryPath;
    m_lib_path.SetPath( aLibraryPath );
    m_cache_timestamp = 0;
    m_cache_dirty = true;
}


void FP_CACHE::Save( FOOTPRINT* aFootprintFilter )
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

    for( auto it = m_footprints.begin(); it != m_footprints.end(); ++it )
    {
        FP_CACHE_ENTRY*             fpCacheEntry = it->second;
        std::unique_ptr<FOOTPRINT>& footprint = fpCacheEntry->GetFootprint();

        if( aFootprintFilter && footprint.get() != aFootprintFilter )
            continue;

        // If we've requested to embed the fonts in the footprint, do so.  Otherwise, clear the
        // embedded fonts from the footprint.  Embedded fonts will be used if available.
        if( footprint->GetAreFontsEmbedded() )
            footprint->EmbedFonts();
        else
            footprint->GetEmbeddedFiles()->ClearEmbeddedFonts();

        WX_FILENAME fn = fpCacheEntry->GetFileName();
        wxString    fileName = fn.GetFullPath();

        // Allow file output stream to go out of scope to close the file stream before
        // renaming the file.
        {
            wxLogTrace( traceKicadPcbPlugin, wxT( "Writing library file '%s'." ),
                        fileName );

            PRETTIFIED_FILE_OUTPUTFORMATTER formatter( fileName );

            m_owner->SetOutputFormatter( &formatter );
            m_owner->Format( footprint.get() );
        }

        m_cache_timestamp += fn.GetTimestamp();
    }

    if( m_lib_path.IsFileReadable() && m_lib_path.GetModificationTime().IsValid() )
        m_cache_timestamp += m_lib_path.GetModificationTime().GetValue().GetValue();

    // If we've saved the full cache, we clear the dirty flag.
    if( !aFootprintFilter )
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
    wxString fileSpec = wxT( "*." ) + wxString( FILEEXT::KiCadFootprintFileExtension );

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
                PCB_IO_KICAD_SEXPR_PARSER       parser( &reader, nullptr, nullptr );

                FOOTPRINT* footprint = dynamic_cast<FOOTPRINT*>( parser.Parse() );
                wxString fpName = fn.GetName();

                if( !footprint )
                    THROW_IO_ERROR( wxEmptyString );   // caught locally, just below...

                footprint->SetFPID( LIB_ID( wxEmptyString, fpName ) );
                m_footprints.insert( fpName, new FP_CACHE_ENTRY( footprint, fn ) );
            }
            catch( const IO_ERROR& ioe )
            {
                if( !cacheError.IsEmpty() )
                    cacheError += wxT( "\n\n" );

                cacheError += wxString::Format( _( "Unable to read file '%s'" ) + '\n',
                                                fn.GetFullPath() );
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
    auto it = m_footprints.find( aFootprintName );

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
        footprint.second->SetFilePath( aPath );
}


bool FP_CACHE::IsModified()
{
    m_cache_dirty = m_cache_dirty || GetTimestamp( m_lib_path.GetFullPath() ) != m_cache_timestamp;

    return m_cache_dirty;
}


long long FP_CACHE::GetTimestamp( const wxString& aLibPath )
{
    wxString fileSpec = wxT( "*." ) + wxString( FILEEXT::KiCadFootprintFileExtension );

    return TimestampDir( aLibPath, fileSpec );
}


bool PCB_IO_KICAD_SEXPR::CanReadBoard( const wxString& aFileName ) const
{
    if( !PCB_IO::CanReadBoard( aFileName ) )
        return false;

    try
    {
        FILE_LINE_READER reader( aFileName );
        PCB_IO_KICAD_SEXPR_PARSER       parser( &reader, nullptr, m_queryUserCallback );

        return parser.IsValidBoardHeader();
    }
    catch( const IO_ERROR& )
    {
    }

    return false;
}


void PCB_IO_KICAD_SEXPR::SaveBoard( const wxString& aFileName, BOARD* aBoard,
                                    const std::map<std::string, UTF8>* aProperties )
{
    wxString sanityResult = aBoard->GroupsSanityCheck();

    if( sanityResult != wxEmptyString && m_queryUserCallback )
    {
        if( !m_queryUserCallback(
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

    // If the user wants fonts embedded, make sure that they are added to the board.  Otherwise,
    // remove any fonts that were previously embedded.
    if( m_board->GetAreFontsEmbedded() )
        m_board->EmbedFonts();
    else
        m_board->GetEmbeddedFiles()->ClearEmbeddedFonts();

    // Prepare net mapping that assures that net codes saved in a file are consecutive integers
    m_mapping->SetBoard( aBoard );

    PRETTIFIED_FILE_OUTPUTFORMATTER formatter( aFileName );

    m_out = &formatter;     // no ownership

    m_out->Print( "(kicad_pcb (version %d) (generator \"pcbnew\") (generator_version %s)",
                  SEXPR_BOARD_FILE_VERSION,
                  m_out->Quotew( GetMajorMinorVersion() ).c_str() );

    Format( aBoard );

    m_out->Print( ")" );
    m_out->Finish();

    m_out = nullptr;
}


BOARD_ITEM* PCB_IO_KICAD_SEXPR::Parse( const wxString& aClipboardSourceInput )
{
    std::string input = TO_UTF8( aClipboardSourceInput );

    STRING_LINE_READER reader( input, wxT( "clipboard" ) );
    PCB_IO_KICAD_SEXPR_PARSER         parser( &reader, nullptr, m_queryUserCallback );

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


void PCB_IO_KICAD_SEXPR::Format( const BOARD_ITEM* aItem ) const
{
    switch( aItem->Type() )
    {
    case PCB_T:
        format( static_cast<const BOARD*>( aItem ) );
        break;

    case PCB_DIM_ALIGNED_T:
    case PCB_DIM_CENTER_T:
    case PCB_DIM_RADIAL_T:
    case PCB_DIM_ORTHOGONAL_T:
    case PCB_DIM_LEADER_T:
        format( static_cast<const PCB_DIMENSION_BASE*>( aItem ) );
        break;

    case PCB_SHAPE_T:
        format( static_cast<const PCB_SHAPE*>( aItem ) );
        break;

    case PCB_REFERENCE_IMAGE_T:
        format( static_cast<const PCB_REFERENCE_IMAGE*>( aItem ) );
        break;

    case PCB_TARGET_T:
        format( static_cast<const PCB_TARGET*>( aItem ) );
        break;

    case PCB_FOOTPRINT_T:
        format( static_cast<const FOOTPRINT*>( aItem ) );
        break;

    case PCB_PAD_T:
        format( static_cast<const PAD*>( aItem ) );
        break;

    case PCB_FIELD_T:
        // Handled in the footprint formatter when properties are formatted
        break;

    case PCB_TEXT_T:
        format( static_cast<const PCB_TEXT*>( aItem ) );
        break;

    case PCB_TEXTBOX_T:
        format( static_cast<const PCB_TEXTBOX*>( aItem ) );
        break;

    case PCB_TABLE_T:
        format( static_cast<const PCB_TABLE*>( aItem ) );
        break;

    case PCB_GROUP_T:
        format( static_cast<const PCB_GROUP*>( aItem ) );
        break;

    case PCB_GENERATOR_T:
        format( static_cast<const PCB_GENERATOR*>( aItem ) );
        break;

    case PCB_TRACE_T:
    case PCB_ARC_T:
    case PCB_VIA_T:
        format( static_cast<const PCB_TRACK*>( aItem ) );
        break;

    case PCB_ZONE_T:
        format( static_cast<const ZONE*>( aItem ) );
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


void PCB_IO_KICAD_SEXPR::formatLayer( PCB_LAYER_ID aLayer, bool aIsKnockout ) const
{
    m_out->Print( "(layer %s %s)",
                  m_out->Quotew( LSET::Name( aLayer ) ).c_str(),
                  aIsKnockout ? "knockout" : "" );
}


void PCB_IO_KICAD_SEXPR::formatPolyPts( const SHAPE_LINE_CHAIN& outline,
                                        const FOOTPRINT* aParentFP ) const
{
    m_out->Print( "(pts" );

    for( int ii = 0; ii < outline.PointCount();  ++ii )
    {
        int ind = outline.ArcIndex( ii );

        if( ind < 0 )
        {
            m_out->Print( "(xy %s)",
                          formatInternalUnits( outline.CPoint( ii ), aParentFP ).c_str() );
        }
        else
        {
            const SHAPE_ARC& arc = outline.Arc( ind );
            m_out->Print( "(arc (start %s) (mid %s) (end %s))",
                          formatInternalUnits( arc.GetP0(), aParentFP ).c_str(),
                          formatInternalUnits( arc.GetArcMid(), aParentFP ).c_str(),
                          formatInternalUnits( arc.GetP1(), aParentFP ).c_str() );

            do
            {
                ++ii;
            } while( ii < outline.PointCount() && outline.ArcIndex( ii ) == ind );

            --ii;
        }
    }

    m_out->Print( ")" );
}


void PCB_IO_KICAD_SEXPR::formatRenderCache( const EDA_TEXT* aText ) const
{
    wxString resolvedText( aText->GetShownText( true ) );
    std::vector<std::unique_ptr<KIFONT::GLYPH>>* cache = aText->GetRenderCache( aText->GetFont(),
                                                                                resolvedText );

    m_out->Print( "(render_cache %s %s",
                  m_out->Quotew( resolvedText ).c_str(),
                  EDA_UNIT_UTILS::FormatAngle( aText->GetDrawRotation() ).c_str() );

    KIGFX::GAL_DISPLAY_OPTIONS empty_opts;

    CALLBACK_GAL callback_gal( empty_opts,
            // Polygon callback
            [&]( const SHAPE_LINE_CHAIN& aPoly )
            {
                m_out->Print( "(polygon" );
                formatPolyPts( aPoly );
                m_out->Print( ")" );
            } );

    callback_gal.SetLineWidth( aText->GetTextThickness() );
    callback_gal.DrawGlyphs( *cache );

    m_out->Print( ")" );
}


void PCB_IO_KICAD_SEXPR::formatSetup( const BOARD* aBoard ) const
{
    // Setup
    m_out->Print( "(setup" );

    // Save the board physical stackup structure
    const BOARD_STACKUP& stackup = aBoard->GetDesignSettings().GetStackupDescriptor();

    if( aBoard->GetDesignSettings().m_HasStackup )
        stackup.FormatBoardStackup( m_out, aBoard );

    BOARD_DESIGN_SETTINGS& dsnSettings = aBoard->GetDesignSettings();

    m_out->Print( "(pad_to_mask_clearance %s)",
                  formatInternalUnits( dsnSettings.m_SolderMaskExpansion ).c_str() );

    if( dsnSettings.m_SolderMaskMinWidth )
    {
        m_out->Print( "(solder_mask_min_width %s)",
                      formatInternalUnits( dsnSettings.m_SolderMaskMinWidth ).c_str() );
    }

    if( dsnSettings.m_SolderPasteMargin != 0 )
    {
        m_out->Print( "(pad_to_paste_clearance %s)",
                      formatInternalUnits( dsnSettings.m_SolderPasteMargin ).c_str() );
    }

    if( dsnSettings.m_SolderPasteMarginRatio != 0 )
    {
        m_out->Print( "(pad_to_paste_clearance_ratio %s)",
                      FormatDouble2Str( dsnSettings.m_SolderPasteMarginRatio ).c_str() );
    }

    KICAD_FORMAT::FormatBool( m_out, "allow_soldermask_bridges_in_footprints",
                              dsnSettings.m_AllowSoldermaskBridgesInFPs );

    m_out->Print( 0, " (tenting " );
    KICAD_FORMAT::FormatBool( m_out, "front", dsnSettings.m_TentViasFront );
    KICAD_FORMAT::FormatBool( m_out, "back", dsnSettings.m_TentViasBack );
    m_out->Print( 0, ")" );

    m_out->Print( 0, " (covering " );
    KICAD_FORMAT::FormatBool( m_out, "front", dsnSettings.m_CoverViasFront );
    KICAD_FORMAT::FormatBool( m_out, "back", dsnSettings.m_CoverViasBack );
    m_out->Print( 0, ")" );

    m_out->Print( 0, " (plugging " );
    KICAD_FORMAT::FormatBool( m_out, "front", dsnSettings.m_PlugViasFront );
    KICAD_FORMAT::FormatBool( m_out, "back", dsnSettings.m_PlugViasBack );
    m_out->Print( 0, ")" );

    KICAD_FORMAT::FormatBool( m_out, "capping", dsnSettings.m_CapVias );

    KICAD_FORMAT::FormatBool( m_out, "filling", dsnSettings.m_FillVias );

    if( !dsnSettings.GetDefaultZoneSettings().m_LayerProperties.empty() )
    {
        m_out->Print( 0, " (zone_defaults" );

        for( const auto& [layer, properties] : dsnSettings.GetDefaultZoneSettings().m_LayerProperties )
            format( properties, 0, layer );

        m_out->Print( 0, ")\n" );
    }

    VECTOR2I origin = dsnSettings.GetAuxOrigin();

    if( origin != VECTOR2I( 0, 0 ) )
    {
        m_out->Print( "(aux_axis_origin %s %s)",
                      formatInternalUnits( origin.x ).c_str(),
                      formatInternalUnits( origin.y ).c_str() );
    }

    origin = dsnSettings.GetGridOrigin();

    if( origin != VECTOR2I( 0, 0 ) )
    {
        m_out->Print( "(grid_origin %s %s)",
                      formatInternalUnits( origin.x ).c_str(),
                      formatInternalUnits( origin.y ).c_str() );
    }

    aBoard->GetPlotOptions().Format( m_out );

    m_out->Print( ")" );
}


void PCB_IO_KICAD_SEXPR::formatGeneral( const BOARD* aBoard ) const
{
    const BOARD_DESIGN_SETTINGS& dsnSettings = aBoard->GetDesignSettings();

    m_out->Print( "(general" );

    m_out->Print( "(thickness %s)",
                  formatInternalUnits( dsnSettings.GetBoardThickness() ).c_str() );

    KICAD_FORMAT::FormatBool( m_out, "legacy_teardrops", aBoard->LegacyTeardrops() );

    m_out->Print( ")" );

    aBoard->GetPageSettings().Format( m_out );
    aBoard->GetTitleBlock().Format( m_out );
}


void PCB_IO_KICAD_SEXPR::formatBoardLayers( const BOARD* aBoard ) const
{
    m_out->Print( "(layers" );

    // Save only the used copper layers from front to back.

    for( PCB_LAYER_ID layer : aBoard->GetEnabledLayers().CuStack() )
    {
        m_out->Print( "(%d %s %s %s)",
                      layer,
                      m_out->Quotew( LSET::Name( layer ) ).c_str(),
                      LAYER::ShowType( aBoard->GetLayerType( layer ) ),
                      LSET::Name( layer ) == m_board->GetLayerName( layer )
                            ? ""
                            : m_out->Quotew( m_board->GetLayerName( layer ) ).c_str() );

    }

    // Save used non-copper layers in the order they are defined.
    LSEQ seq = aBoard->GetEnabledLayers().TechAndUserUIOrder();

    for( PCB_LAYER_ID layer : seq )
    {
        bool print_type = false;

        // User layers (layer id >= User_1) have a qualifier
        // default is "user", but other qualifiers exist
        if( layer >= User_1 )
        {
            if( IsCopperLayer( layer ) )
                print_type = true;

            if( aBoard->GetLayerType( layer ) == LT_FRONT
                || aBoard->GetLayerType( layer ) == LT_BACK )
                print_type = true;
        }

        m_out->Print( "(%d %s %s %s)",
                      layer,
                      m_out->Quotew( LSET::Name( layer ) ).c_str(),
                      print_type
                            ? LAYER::ShowType( aBoard->GetLayerType( layer ) )
                            : "user",
                      m_board->GetLayerName( layer ) == LSET::Name( layer )
                            ? ""
                            : m_out->Quotew( m_board->GetLayerName( layer ) ).c_str() );
    }

    m_out->Print( ")" );
}


void PCB_IO_KICAD_SEXPR::formatNetInformation( const BOARD* aBoard ) const
{
    for( NETINFO_ITEM* net : *m_mapping )
    {
        if( net == nullptr )    // Skip not actually existing nets (orphan nets)
            continue;

        m_out->Print( "(net %d %s)",
                      m_mapping->Translate( net->GetNetCode() ),
                      m_out->Quotew( net->GetNetname() ).c_str() );
    }
}


void PCB_IO_KICAD_SEXPR::formatProperties( const BOARD* aBoard ) const
{
    for( const std::pair<const wxString, wxString>& prop : aBoard->GetProperties() )
    {
        m_out->Print( "(property %s %s)",
                      m_out->Quotew( prop.first ).c_str(),
                      m_out->Quotew( prop.second ).c_str() );
    }
}


void PCB_IO_KICAD_SEXPR::formatHeader( const BOARD* aBoard ) const
{
    formatGeneral( aBoard );

    // Layers list.
    formatBoardLayers( aBoard );

    // Setup
    formatSetup( aBoard );

    // Properties
    formatProperties( aBoard );

    // Save net codes and names
    formatNetInformation( aBoard );
}


bool isDefaultTeardropParameters( const TEARDROP_PARAMETERS& tdParams )
{
    static const TEARDROP_PARAMETERS defaults;

    return tdParams.m_Enabled == defaults.m_Enabled
            && tdParams.m_BestLengthRatio == defaults.m_BestLengthRatio
            && tdParams.m_TdMaxLen == defaults.m_TdMaxLen
            && tdParams.m_BestWidthRatio == defaults.m_BestWidthRatio
            && tdParams.m_TdMaxWidth == defaults.m_TdMaxWidth
            && tdParams.m_CurvedEdges == defaults.m_CurvedEdges
            && tdParams.m_WidthtoSizeFilterRatio == defaults.m_WidthtoSizeFilterRatio
            && tdParams.m_AllowUseTwoTracks == defaults.m_AllowUseTwoTracks
            && tdParams.m_TdOnPadsInZones == defaults.m_TdOnPadsInZones;
}


void PCB_IO_KICAD_SEXPR::formatTeardropParameters( const TEARDROP_PARAMETERS& tdParams ) const
{
    m_out->Print( "(teardrops (best_length_ratio %s) (max_length %s) (best_width_ratio %s) "
                  "(max_width %s)",
                  FormatDouble2Str( tdParams.m_BestLengthRatio ).c_str(),
                  formatInternalUnits( tdParams.m_TdMaxLen ).c_str(),
                  FormatDouble2Str( tdParams.m_BestWidthRatio ).c_str(),
                  formatInternalUnits( tdParams.m_TdMaxWidth ).c_str() );

    KICAD_FORMAT::FormatBool( m_out, "curved_edges", tdParams.m_CurvedEdges );

    m_out->Print( "(filter_ratio %s)",
                  FormatDouble2Str( tdParams.m_WidthtoSizeFilterRatio ).c_str() );

    KICAD_FORMAT::FormatBool( m_out, "enabled", tdParams.m_Enabled );
    KICAD_FORMAT::FormatBool( m_out, "allow_two_segments", tdParams.m_AllowUseTwoTracks );
    KICAD_FORMAT::FormatBool( m_out, "prefer_zone_connections", !tdParams.m_TdOnPadsInZones );
    m_out->Print( ")" );
}


void PCB_IO_KICAD_SEXPR::format( const BOARD* aBoard ) const
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
    std::set<BOARD_ITEM*, BOARD_ITEM::ptr_cmp>  sorted_generators( aBoard->Generators().begin(),
                                                                   aBoard->Generators().end() );
    formatHeader( aBoard );

    // Save the footprints.
    for( BOARD_ITEM* footprint : sorted_footprints )
        Format( footprint );

    // Save the graphical items on the board (not owned by a footprint)
    for( BOARD_ITEM* item : sorted_drawings )
        Format( item );

    // Do not save PCB_MARKERs, they can be regenerated easily.

    // Save the tracks and vias.
    for( PCB_TRACK* track : sorted_tracks )
        Format( track );

    // Save the polygon (which are the newer technology) zones.
    for( auto zone : sorted_zones )
        Format( zone );

    // Save the groups
    for( BOARD_ITEM* group : sorted_groups )
        Format( group );

    // Save the generators
    for( BOARD_ITEM* gen : sorted_generators )
        Format( gen );

    // Save any embedded files
    // Consolidate the embedded models in footprints into a single map
    // to avoid duplicating the same model in the board file.
    EMBEDDED_FILES files_to_write;

    for( auto& file : aBoard->GetEmbeddedFiles()->EmbeddedFileMap() )
        files_to_write.AddFile( file.second );

    for( BOARD_ITEM* item : sorted_footprints )
    {
        FOOTPRINT* fp = static_cast<FOOTPRINT*>( item );

        for( auto& file : fp->GetEmbeddedFiles()->EmbeddedFileMap() )
            files_to_write.AddFile( file.second );
    }

    m_out->Print( "(embedded_fonts %s)",
                  aBoard->GetEmbeddedFiles()->GetAreFontsEmbedded() ? "yes" : "no" );

    if( !files_to_write.IsEmpty() )
        files_to_write.WriteEmbeddedFiles( *m_out, ( m_ctl & CTL_FOR_BOARD ) );

    // Remove the files so that they are not freed in the DTOR
    files_to_write.ClearEmbeddedFiles( false );
}


void PCB_IO_KICAD_SEXPR::format( const PCB_DIMENSION_BASE* aDimension ) const
{
    const PCB_DIM_ALIGNED*    aligned = dynamic_cast<const PCB_DIM_ALIGNED*>( aDimension );
    const PCB_DIM_ORTHOGONAL* ortho   = dynamic_cast<const PCB_DIM_ORTHOGONAL*>( aDimension );
    const PCB_DIM_CENTER*     center  = dynamic_cast<const PCB_DIM_CENTER*>( aDimension );
    const PCB_DIM_RADIAL*     radial  = dynamic_cast<const PCB_DIM_RADIAL*>( aDimension );
    const PCB_DIM_LEADER*     leader  = dynamic_cast<const PCB_DIM_LEADER*>( aDimension );

    m_out->Print( "(dimension" );

    if( ortho ) // must be tested before aligned, because ortho is derived from aligned
                // and aligned is not null
        m_out->Print( "(type orthogonal)" );
    else if( aligned )
        m_out->Print( "(type aligned)" );
    else if( leader )
        m_out->Print( "(type leader)" );
    else if( center )
        m_out->Print( "(type center)" );
    else if( radial )
        m_out->Print( "(type radial)" );
    else
        wxFAIL_MSG( wxT( "Cannot format unknown dimension type!" ) );

    if( aDimension->IsLocked() )
        KICAD_FORMAT::FormatBool( m_out, "locked", aDimension->IsLocked() );

    formatLayer( aDimension->GetLayer() );

    KICAD_FORMAT::FormatUuid( m_out, aDimension->m_Uuid );

    m_out->Print( "(pts (xy %s %s) (xy %s %s))",
                  formatInternalUnits( aDimension->GetStart().x ).c_str(),
                  formatInternalUnits( aDimension->GetStart().y ).c_str(),
                  formatInternalUnits( aDimension->GetEnd().x ).c_str(),
                  formatInternalUnits( aDimension->GetEnd().y ).c_str() );

    if( aligned )
        m_out->Print( "(height %s)", formatInternalUnits( aligned->GetHeight() ).c_str() );

    if( radial )
    {
        m_out->Print( "(leader_length %s)",
                      formatInternalUnits( radial->GetLeaderLength() ).c_str() );
    }

    if( ortho )
        m_out->Print( "(orientation %d)", static_cast<int>( ortho->GetOrientation() ) );

    if( !center )
    {
        m_out->Print( "(format (prefix %s) (suffix %s) (units %d) (units_format %d) (precision %d)",
                      m_out->Quotew( aDimension->GetPrefix() ).c_str(),
                      m_out->Quotew( aDimension->GetSuffix() ).c_str(),
                      static_cast<int>( aDimension->GetUnitsMode() ),
                      static_cast<int>( aDimension->GetUnitsFormat() ),
                      static_cast<int>(  aDimension->GetPrecision() ) );

        if( aDimension->GetOverrideTextEnabled() )
        {
            m_out->Print( "(override_value %s)",
                          m_out->Quotew( aDimension->GetOverrideText() ).c_str() );
        }

        if( aDimension->GetSuppressZeroes() )
            KICAD_FORMAT::FormatBool( m_out, "suppress_zeroes", true );

        m_out->Print( ")" );
    }

    m_out->Print( "(style (thickness %s) (arrow_length %s) (text_position_mode %d)",
                  formatInternalUnits( aDimension->GetLineThickness() ).c_str(),
                  formatInternalUnits( aDimension->GetArrowLength() ).c_str(),
                  static_cast<int>( aDimension->GetTextPositionMode() ) );

    if( ortho || aligned )
    {
        switch( aDimension->GetArrowDirection() )
        {
        case DIM_ARROW_DIRECTION::OUTWARD:
            m_out->Print( "(arrow_direction outward)" );
            break;
        case DIM_ARROW_DIRECTION::INWARD:
            m_out->Print( "(arrow_direction inward)" );
            break;
        // No default, handle all cases
        }
    }

    if( aligned )
    {
        m_out->Print( "(extension_height %s)",
                      formatInternalUnits( aligned->GetExtensionHeight() ).c_str() );
    }

    if( leader )
        m_out->Print( "(text_frame %d)", static_cast<int>( leader->GetTextBorder() ) );

    m_out->Print( "(extension_offset %s)",
                  formatInternalUnits( aDimension->GetExtensionOffset() ).c_str() );

    if( aDimension->GetKeepTextAligned() )
        KICAD_FORMAT::FormatBool( m_out, "keep_text_aligned", true );

    m_out->Print( ")" );

    // Write dimension text after all other options to be sure the
    // text options are known when reading the file
    if( !center )
        format( static_cast<const PCB_TEXT*>( aDimension ) );

    m_out->Print( ")" );
}


void PCB_IO_KICAD_SEXPR::format( const PCB_SHAPE* aShape ) const
{
    FOOTPRINT*  parentFP = aShape->GetParentFootprint();
    std::string prefix = parentFP ? "fp" : "gr";

    switch( aShape->GetShape() )
    {
    case SHAPE_T::SEGMENT:
        m_out->Print( "(%s_line (start %s) (end %s)",
                      prefix.c_str(),
                      formatInternalUnits( aShape->GetStart(), parentFP ).c_str(),
                      formatInternalUnits( aShape->GetEnd(), parentFP ).c_str() );
        break;

    case SHAPE_T::RECTANGLE:
        m_out->Print( "(%s_rect (start %s) (end %s)",
                      prefix.c_str(),
                      formatInternalUnits( aShape->GetStart(), parentFP ).c_str(),
                      formatInternalUnits( aShape->GetEnd(), parentFP ).c_str() );
        break;

    case SHAPE_T::CIRCLE:
        m_out->Print( "(%s_circle (center %s) (end %s)",
                      prefix.c_str(),
                      formatInternalUnits( aShape->GetStart(), parentFP ).c_str(),
                      formatInternalUnits( aShape->GetEnd(), parentFP ).c_str() );
        break;

    case SHAPE_T::ARC:
        m_out->Print( "(%s_arc (start %s) (mid %s) (end %s)",
                      prefix.c_str(),
                      formatInternalUnits( aShape->GetStart(), parentFP ).c_str(),
                      formatInternalUnits( aShape->GetArcMid(), parentFP ).c_str(),
                      formatInternalUnits( aShape->GetEnd(), parentFP ).c_str() );
        break;

    case SHAPE_T::POLY:
        if( aShape->IsPolyShapeValid() )
        {
            const SHAPE_POLY_SET& poly = aShape->GetPolyShape();
            const SHAPE_LINE_CHAIN& outline = poly.Outline( 0 );

            m_out->Print( "(%s_poly", prefix.c_str() );
            formatPolyPts( outline, parentFP );
        }
        else
        {
            return;
        }

        break;

    case SHAPE_T::BEZIER:
        m_out->Print( "(%s_curve (pts (xy %s) (xy %s) (xy %s) (xy %s))",
                      prefix.c_str(),
                      formatInternalUnits( aShape->GetStart(), parentFP ).c_str(),
                      formatInternalUnits( aShape->GetBezierC1(), parentFP ).c_str(),
                      formatInternalUnits( aShape->GetBezierC2(), parentFP ).c_str(),
                      formatInternalUnits( aShape->GetEnd(), parentFP ).c_str() );
        break;

    default:
        UNIMPLEMENTED_FOR( aShape->SHAPE_T_asString() );
        return;
    };

    aShape->GetStroke().Format( m_out, pcbIUScale );

    // The filled flag represents if a solid fill is present on circles, rectangles and polygons
    if( ( aShape->GetShape() == SHAPE_T::POLY )
        || ( aShape->GetShape() == SHAPE_T::RECTANGLE )
        || ( aShape->GetShape() == SHAPE_T::CIRCLE ) )
    {
        switch( aShape->GetFillMode() )
        {
        case FILL_T::HATCH:
            m_out->Print( "(fill hatch)" );
            break;

        case FILL_T::REVERSE_HATCH:
            m_out->Print( "(fill reverse_hatch)" );
            break;

        case FILL_T::CROSS_HATCH:
            m_out->Print( "(fill cross_hatch)" );
            break;

        case FILL_T::FILLED_SHAPE:
            KICAD_FORMAT::FormatBool( m_out, "fill", true );
            break;

        default:
            KICAD_FORMAT::FormatBool( m_out, "fill", false );
            break;
        }
    }

    if( aShape->IsLocked() )
        KICAD_FORMAT::FormatBool( m_out, "locked", true );

    if( aShape->GetLayerSet().count() > 1 )
        formatLayers( aShape->GetLayerSet(), false /* enumerate layers */ );
    else
        formatLayer( aShape->GetLayer() );

    if( aShape->HasSolderMask()
        && aShape->GetLocalSolderMaskMargin().has_value()
        && IsExternalCopperLayer( aShape->GetLayer() ) )
    {
        m_out->Print( "(solder_mask_margin %s)",
                      formatInternalUnits( aShape->GetLocalSolderMaskMargin().value() ).c_str() );
    }

    if( aShape->GetNetCode() > 0 )
        m_out->Print( "(net %d)", m_mapping->Translate( aShape->GetNetCode() ) );

    KICAD_FORMAT::FormatUuid( m_out, aShape->m_Uuid );
    m_out->Print( ")" );
}


void PCB_IO_KICAD_SEXPR::format( const PCB_REFERENCE_IMAGE* aBitmap ) const
{
    wxCHECK_RET( aBitmap != nullptr && m_out != nullptr, "" );

    const REFERENCE_IMAGE& refImage = aBitmap->GetReferenceImage();

    const wxImage* image = refImage.GetImage().GetImageData();

    wxCHECK_RET( image != nullptr, "wxImage* is NULL" );

    m_out->Print( "(image (at %s %s)",
                  formatInternalUnits( aBitmap->GetPosition().x ).c_str(),
                  formatInternalUnits( aBitmap->GetPosition().y ).c_str() );

    formatLayer( aBitmap->GetLayer() );

    if( refImage.GetImageScale() != 1.0 )
        m_out->Print( "%s", fmt::format("(scale {:g})", refImage.GetImageScale()).c_str() );

    if( aBitmap->IsLocked() )
        KICAD_FORMAT::FormatBool( m_out, "locked", true );

    wxMemoryOutputStream ostream;
    refImage.GetImage().SaveImageData( ostream );

    KICAD_FORMAT::FormatStreamData( *m_out, *ostream.GetOutputStreamBuffer() );

    KICAD_FORMAT::FormatUuid( m_out, aBitmap->m_Uuid );
    m_out->Print( ")" );      // Closes image token.
}


void PCB_IO_KICAD_SEXPR::format( const PCB_TARGET* aTarget ) const
{
    m_out->Print( "(target %s (at %s) (size %s)",
                  ( aTarget->GetShape() ) ? "x" : "plus",
                  formatInternalUnits( aTarget->GetPosition() ).c_str(),
                  formatInternalUnits( aTarget->GetSize() ).c_str() );

    if( aTarget->GetWidth() != 0 )
        m_out->Print( "(width %s)", formatInternalUnits( aTarget->GetWidth() ).c_str() );

    formatLayer( aTarget->GetLayer() );
    KICAD_FORMAT::FormatUuid( m_out, aTarget->m_Uuid );
    m_out->Print( ")" );
}


void PCB_IO_KICAD_SEXPR::format( const FOOTPRINT* aFootprint ) const
{
    if( !( m_ctl & CTL_OMIT_INITIAL_COMMENTS ) )
    {
        const wxArrayString* initial_comments = aFootprint->GetInitialComments();

        if( initial_comments )
        {
            for( unsigned i = 0; i < initial_comments->GetCount(); ++i )
                m_out->Print( "%s\n", TO_UTF8( (*initial_comments)[i] ) );
        }
    }

    if( m_ctl & CTL_OMIT_LIBNAME )
    {
        m_out->Print( "(footprint %s",
                      m_out->Quotes( aFootprint->GetFPID().GetLibItemName() ).c_str() );
    }
    else
    {
        m_out->Print( "(footprint %s",
                      m_out->Quotes( aFootprint->GetFPID().Format() ).c_str() );
    }

    if( !( m_ctl & CTL_OMIT_FOOTPRINT_VERSION ) )
    {
        m_out->Print( "(version %d) (generator \"pcbnew\") (generator_version %s)",
                      SEXPR_BOARD_FILE_VERSION,
                      m_out->Quotew( GetMajorMinorVersion() ).c_str() );
    }

    if( aFootprint->IsLocked() )
        KICAD_FORMAT::FormatBool( m_out, "locked", true );

    if( aFootprint->IsPlaced() )
        KICAD_FORMAT::FormatBool( m_out, "placed", true );

    formatLayer( aFootprint->GetLayer() );

    if( !( m_ctl & CTL_OMIT_UUIDS ) )
        KICAD_FORMAT::FormatUuid( m_out, aFootprint->m_Uuid );

    if( !( m_ctl & CTL_OMIT_AT ) )
    {
        m_out->Print( "(at %s %s)",
                      formatInternalUnits( aFootprint->GetPosition() ).c_str(),
                      aFootprint->GetOrientation().IsZero()
                            ? ""
                            : EDA_UNIT_UTILS::FormatAngle( aFootprint->GetOrientation() ).c_str() );
    }

    if( !aFootprint->GetLibDescription().IsEmpty() )
        m_out->Print( "(descr %s)", m_out->Quotew( aFootprint->GetLibDescription() ).c_str() );

    if( !aFootprint->GetKeywords().IsEmpty() )
        m_out->Print( "(tags %s)", m_out->Quotew( aFootprint->GetKeywords() ).c_str() );

    for( const PCB_FIELD* field : aFootprint->GetFields() )
    {
        m_out->Print( "(property %s %s",
                      m_out->Quotew( field->GetCanonicalName() ).c_str(),
                      m_out->Quotew( field->GetText() ).c_str() );

        format( field );

        m_out->Print( ")" );
    }

    if( const COMPONENT_CLASS* compClass = aFootprint->GetStaticComponentClass() )
    {
        if( !compClass->IsEmpty() )
        {
            m_out->Print( "(component_classes" );

            for( const COMPONENT_CLASS* constituent : compClass->GetConstituentClasses() )
                m_out->Print( "(class %s)", m_out->Quotew( constituent->GetName() ).c_str() );

            m_out->Print( ")" );
        }
    }

    if( !aFootprint->GetFilters().empty() )
    {
        m_out->Print( "(property ki_fp_filters %s)",
                      m_out->Quotew( aFootprint->GetFilters() ).c_str() );
    }

    if( !( m_ctl & CTL_OMIT_PATH ) && !aFootprint->GetPath().empty() )
        m_out->Print( "(path %s)", m_out->Quotew( aFootprint->GetPath().AsString() ).c_str() );

    if( !aFootprint->GetSheetname().empty() )
        m_out->Print( "(sheetname %s)", m_out->Quotew( aFootprint->GetSheetname() ).c_str() );

    if( !aFootprint->GetSheetfile().empty() )
        m_out->Print( "(sheetfile %s)", m_out->Quotew( aFootprint->GetSheetfile() ).c_str() );

    if( aFootprint->GetLocalSolderMaskMargin().has_value() )
    {
        m_out->Print( "(solder_mask_margin %s)",
                      formatInternalUnits( aFootprint->GetLocalSolderMaskMargin().value() ).c_str() );
    }

    if( aFootprint->GetLocalSolderPasteMargin().has_value() )
    {
        m_out->Print( "(solder_paste_margin %s)",
                      formatInternalUnits( aFootprint->GetLocalSolderPasteMargin().value() ).c_str() );
    }

    if( aFootprint->GetLocalSolderPasteMarginRatio().has_value() )
    {
        m_out->Print( "(solder_paste_margin_ratio %s)",
                      FormatDouble2Str( aFootprint->GetLocalSolderPasteMarginRatio().value() ).c_str() );
    }

    if( aFootprint->GetLocalClearance().has_value() )
    {
        m_out->Print( "(clearance %s)",
                      formatInternalUnits( aFootprint->GetLocalClearance().value() ).c_str() );
    }

    if( aFootprint->GetLocalZoneConnection() != ZONE_CONNECTION::INHERITED )
    {
        m_out->Print( "(zone_connect %d)",
                      static_cast<int>( aFootprint->GetLocalZoneConnection() ) );
    }

    // Attributes
    if( aFootprint->GetAttributes()
            || aFootprint->AllowMissingCourtyard()
            || aFootprint->AllowSolderMaskBridges() )
    {
        m_out->Print( "(attr" );

        if( aFootprint->GetAttributes() & FP_SMD )
            m_out->Print( " smd" );

        if( aFootprint->GetAttributes() & FP_THROUGH_HOLE )
            m_out->Print( " through_hole" );

        if( aFootprint->GetAttributes() & FP_BOARD_ONLY )
            m_out->Print( " board_only" );

        if( aFootprint->GetAttributes() & FP_EXCLUDE_FROM_POS_FILES )
            m_out->Print( " exclude_from_pos_files" );

        if( aFootprint->GetAttributes() & FP_EXCLUDE_FROM_BOM )
            m_out->Print( " exclude_from_bom" );

        if( aFootprint->AllowMissingCourtyard() )
            m_out->Print( " allow_missing_courtyard" );

        if( aFootprint->GetAttributes() & FP_DNP )
            m_out->Print( " dnp" );

        if( aFootprint->AllowSolderMaskBridges() )
            m_out->Print( " allow_soldermask_bridges" );

        m_out->Print( ")" );
    }

    if( aFootprint->GetPrivateLayers().any() )
    {
        m_out->Print( "(private_layers" );

        for( PCB_LAYER_ID layer : aFootprint->GetPrivateLayers().Seq() )
        {
            wxString canonicalName( LSET::Name( layer ) );
            m_out->Print( " %s", m_out->Quotew( canonicalName ).c_str() );
        }

        m_out->Print( ")" );
    }

    if( aFootprint->IsNetTie() )
    {
        m_out->Print( "(net_tie_pad_groups" );

        for( const wxString& group : aFootprint->GetNetTiePadGroups() )
            m_out->Print( " %s", m_out->Quotew( group ).c_str() );

        m_out->Print( ")" );
    }

    KICAD_FORMAT::FormatBool( m_out, "duplicate_pad_numbers_are_jumpers",
                              aFootprint->GetDuplicatePadNumbersAreJumpers() );

    const std::vector<std::set<wxString>>& jumperGroups = aFootprint->JumperPadGroups();

    if( !jumperGroups.empty() )
    {
        m_out->Print( "(jumper_pad_groups" );

        for( const std::set<wxString>& group : jumperGroups )
        {
            m_out->Print( "(" );

            for( const wxString& padName : group )
                m_out->Print( "%s ", m_out->Quotew( padName ).c_str() );

            m_out->Print( ")" );
        }

        m_out->Print( ")" );
    }

    Format( (BOARD_ITEM*) &aFootprint->Reference() );
    Format( (BOARD_ITEM*) &aFootprint->Value() );

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
        Format( gr );

    // Save pads.
    for( PAD* pad : sorted_pads )
        Format( pad );

    // Save zones.
    for( BOARD_ITEM* zone : sorted_zones )
        Format( zone );

    // Save groups.
    for( BOARD_ITEM* group : sorted_groups )
        Format( group );

    KICAD_FORMAT::FormatBool( m_out, "embedded_fonts",
                              aFootprint->GetEmbeddedFiles()->GetAreFontsEmbedded() );

    if( !aFootprint->GetEmbeddedFiles()->IsEmpty() )
        aFootprint->WriteEmbeddedFiles( *m_out, !( m_ctl & CTL_FOR_BOARD ) );

    // Save 3D info.
    auto bs3D = aFootprint->Models().begin();
    auto es3D = aFootprint->Models().end();

    while( bs3D != es3D )
    {
        if( !bs3D->m_Filename.IsEmpty() )
        {
            m_out->Print( "(model %s", m_out->Quotew( bs3D->m_Filename ).c_str() );

            if( !bs3D->m_Show )
                KICAD_FORMAT::FormatBool( m_out, "hide", !bs3D->m_Show );

            if( bs3D->m_Opacity != 1.0 )
                m_out->Print( "%s", fmt::format("(opacity {:.4f})", bs3D->m_Opacity).c_str() );

            m_out->Print( "(offset (xyz %s %s %s))",
                          FormatDouble2Str( bs3D->m_Offset.x ).c_str(),
                          FormatDouble2Str( bs3D->m_Offset.y ).c_str(),
                          FormatDouble2Str( bs3D->m_Offset.z ).c_str() );

            m_out->Print( "(scale (xyz %s %s %s))",
                          FormatDouble2Str( bs3D->m_Scale.x ).c_str(),
                          FormatDouble2Str( bs3D->m_Scale.y ).c_str(),
                          FormatDouble2Str( bs3D->m_Scale.z ).c_str() );

            m_out->Print( "(rotate (xyz %s %s %s))",
                          FormatDouble2Str( bs3D->m_Rotation.x ).c_str(),
                          FormatDouble2Str( bs3D->m_Rotation.y ).c_str(),
                          FormatDouble2Str( bs3D->m_Rotation.z ).c_str() );

            m_out->Print( ")" );
        }

        ++bs3D;
    }

    m_out->Print( ")" );
}


void PCB_IO_KICAD_SEXPR::formatLayers( LSET aLayerMask, bool aEnumerateLayers ) const
{
    static const LSET cu_all( LSET::AllCuMask() );
    static const LSET fr_bk(  { B_Cu, F_Cu } );
    static const LSET adhes(  { B_Adhes, F_Adhes } );
    static const LSET paste(  { B_Paste, F_Paste } );
    static const LSET silks(  { B_SilkS, F_SilkS } );
    static const LSET mask(   { B_Mask, F_Mask } );
    static const LSET crt_yd( { B_CrtYd, F_CrtYd } );
    static const LSET fab(    { B_Fab, F_Fab } );

    LSET cu_board_mask = LSET::AllCuMask( m_board ? m_board->GetCopperLayerCount() : MAX_CU_LAYERS );

    std::string output;

    if( !aEnumerateLayers )
    {
        // If all copper layers present on the board are enabled, then output the wildcard
        if( ( aLayerMask & cu_board_mask ) == cu_board_mask )
        {
            output += ' ' + m_out->Quotew( "*.Cu" );

            // Clear all copper bits because pads might have internal layers that aren't part of the
            // board enabled, and we don't want to output those in the layers listing if we already
            // output the wildcard.
            aLayerMask &= ~cu_all;
        }
        else if( ( aLayerMask & cu_board_mask ) == fr_bk )
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
    }

    // output any individual layers not handled in wildcard combos above
    for( int layer = 0; layer < PCB_LAYER_ID_COUNT; ++layer )
    {
        if( aLayerMask[layer] )
            output += ' ' + m_out->Quotew( LSET::Name( PCB_LAYER_ID( layer ) ) );
    }

    m_out->Print( "(layers %s)", output.c_str() );
}


void PCB_IO_KICAD_SEXPR::format( const PAD* aPad ) const
{
    const BOARD* board = aPad->GetBoard();

    auto shapeName =
        [&]( PCB_LAYER_ID aLayer )
        {
            switch( aPad->GetShape( aLayer ) )
            {
            case PAD_SHAPE::CIRCLE:          return "circle";
            case PAD_SHAPE::RECTANGLE:       return "rect";
            case PAD_SHAPE::OVAL:            return "oval";
            case PAD_SHAPE::TRAPEZOID:       return "trapezoid";
            case PAD_SHAPE::CHAMFERED_RECT:
            case PAD_SHAPE::ROUNDRECT:       return "roundrect";
            case PAD_SHAPE::CUSTOM:          return "custom";

            default:
                THROW_IO_ERROR( wxString::Format( _( "unknown pad type: %d"),
                                aPad->GetShape( aLayer ) ) );
            }
        };

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
    case PAD_PROP::MECHANICAL:       property = "pad_prop_mechanical";    break;

    default:
        THROW_IO_ERROR( wxString::Format( wxT( "unknown pad property: %d" ),
                                          aPad->GetProperty() ) );
    }

    m_out->Print( "(pad %s %s %s",
                  m_out->Quotew( aPad->GetNumber() ).c_str(),
                  type,
                  shapeName( PADSTACK::ALL_LAYERS ) );

    m_out->Print( "(at %s %s)",
                  formatInternalUnits( aPad->GetFPRelativePosition() ).c_str(),
                  aPad->GetOrientation().IsZero()
                        ? ""
                        : EDA_UNIT_UTILS::FormatAngle( aPad->GetOrientation() ).c_str() );

    m_out->Print( "(size %s)", formatInternalUnits( aPad->GetSize( PADSTACK::ALL_LAYERS ) ).c_str() );

    if( aPad->GetDelta( PADSTACK::ALL_LAYERS ).x != 0
        || aPad->GetDelta( PADSTACK::ALL_LAYERS ).y != 0 )
    {
        m_out->Print( "(rect_delta %s)",
                      formatInternalUnits( aPad->GetDelta( PADSTACK::ALL_LAYERS ) ).c_str() );
    }

    VECTOR2I sz = aPad->GetDrillSize();
    VECTOR2I shapeoffset = aPad->GetOffset( PADSTACK::ALL_LAYERS );

    if( (sz.x > 0) || (sz.y > 0) ||
        (shapeoffset.x != 0) || (shapeoffset.y != 0) )
    {
        m_out->Print( "(drill" );

        if( aPad->GetDrillShape() == PAD_DRILL_SHAPE::OBLONG )
            m_out->Print( " oval" );

        if( sz.x > 0 )
            m_out->Print( " %s", formatInternalUnits( sz.x ).c_str() );

        if( sz.y > 0  && sz.x != sz.y )
            m_out->Print( " %s", formatInternalUnits( sz.y ).c_str() );

        // NOTE: Shape offest is a property of the copper shape, not of the drill, but this was put
        // in the file format under the drill section.  So, it is left here to minimize file format
        // changes, but note that the other padstack layers (if present) will have an offset stored
        // separately.
        if( shapeoffset.x != 0 || shapeoffset.y != 0 )
        {
            m_out->Print( "(offset %s)",
                          formatInternalUnits( aPad->GetOffset( PADSTACK::ALL_LAYERS ) ).c_str() );
        }

        m_out->Print( ")" );
    }

    // Add pad property, if exists.
    if( property )
        m_out->Print( "(property %s)", property );

    formatLayers( aPad->GetLayerSet(), false /* enumerate layers */ );

    if( aPad->GetAttribute() == PAD_ATTRIB::PTH )
    {
        KICAD_FORMAT::FormatBool( m_out, "remove_unused_layers", aPad->GetRemoveUnconnected() );

        if( aPad->GetRemoveUnconnected() )
        {
            KICAD_FORMAT::FormatBool( m_out, "keep_end_layers", aPad->GetKeepTopBottom() );

            if( board )     // Will be nullptr in footprint library
            {
                m_out->Print( "(zone_layer_connections" );

                for( PCB_LAYER_ID layer : board->GetEnabledLayers().CuStack() )
                {
                    if( aPad->GetZoneLayerOverride( layer ) == ZLO_FORCE_FLASHED )
                        m_out->Print( " %s", m_out->Quotew( LSET::Name( layer ) ).c_str() );
                }

                m_out->Print( ")" );
            }
        }
    }

    auto formatCornerProperties =
        [&]( PCB_LAYER_ID aLayer )
        {
            // Output the radius ratio for rounded and chamfered rect pads
            if( aPad->GetShape( aLayer ) == PAD_SHAPE::ROUNDRECT
                || aPad->GetShape( aLayer ) == PAD_SHAPE::CHAMFERED_RECT)
            {
                m_out->Print( "(roundrect_rratio %s)",
                              FormatDouble2Str( aPad->GetRoundRectRadiusRatio( aLayer ) ).c_str() );
            }

            // Output the chamfer corners for chamfered rect pads
            if( aPad->GetShape( aLayer ) == PAD_SHAPE::CHAMFERED_RECT)
            {
                m_out->Print( "(chamfer_ratio %s)",
                              FormatDouble2Str( aPad->GetChamferRectRatio( aLayer ) ).c_str() );

                m_out->Print( "(chamfer" );

                if( ( aPad->GetChamferPositions( aLayer ) & RECT_CHAMFER_TOP_LEFT ) )
                    m_out->Print( " top_left" );

                if( ( aPad->GetChamferPositions( aLayer ) & RECT_CHAMFER_TOP_RIGHT ) )
                    m_out->Print( " top_right" );

                if( ( aPad->GetChamferPositions( aLayer ) & RECT_CHAMFER_BOTTOM_LEFT ) )
                    m_out->Print( " bottom_left" );

                if( ( aPad->GetChamferPositions( aLayer ) & RECT_CHAMFER_BOTTOM_RIGHT ) )
                    m_out->Print( " bottom_right" );

                m_out->Print( ")" );
            }

        };

    // For normal padstacks, this is the one and only set of properties.  For complex ones, this
    // will represent the front layer properties, and other layers will be formatted below
    formatCornerProperties( PADSTACK::ALL_LAYERS );

    // Unconnected pad is default net so don't save it.
    if( !( m_ctl & CTL_OMIT_PAD_NETS ) && aPad->GetNetCode() != NETINFO_LIST::UNCONNECTED )
    {
        m_out->Print( "(net %d %s)", m_mapping->Translate( aPad->GetNetCode() ),
                      m_out->Quotew( aPad->GetNetname() ).c_str() );
    }

    // Pin functions and types are closely related to nets, so if CTL_OMIT_NETS is set, omit
    // them as well (for instance when saved from library editor).
    if( !( m_ctl & CTL_OMIT_PAD_NETS ) )
    {
        if( !aPad->GetPinFunction().IsEmpty() )
            m_out->Print( "(pinfunction %s)", m_out->Quotew( aPad->GetPinFunction() ).c_str() );

        if( !aPad->GetPinType().IsEmpty() )
            m_out->Print( "(pintype %s)", m_out->Quotew( aPad->GetPinType() ).c_str() );
    }

    if( aPad->GetPadToDieLength() != 0 )
    {
        m_out->Print( "(die_length %s)",
                      formatInternalUnits( aPad->GetPadToDieLength() ).c_str() );
    }

    if( aPad->GetPadToDieDelay() != 0 )
    {
        m_out->Print( "(die_delay %s)", formatInternalUnits( aPad->GetPadToDieDelay() ).c_str() );
    }

    if( aPad->GetLocalSolderMaskMargin().has_value() )
    {
        m_out->Print( "(solder_mask_margin %s)",
                      formatInternalUnits( aPad->GetLocalSolderMaskMargin().value() ).c_str() );
    }

    if( aPad->GetLocalSolderPasteMargin().has_value() )
    {
        m_out->Print( "(solder_paste_margin %s)",
                      formatInternalUnits( aPad->GetLocalSolderPasteMargin().value() ).c_str() );
    }

    if( aPad->GetLocalSolderPasteMarginRatio().has_value() )
    {
        m_out->Print( "(solder_paste_margin_ratio %s)",
                      FormatDouble2Str( aPad->GetLocalSolderPasteMarginRatio().value() ).c_str() );
    }

    if( aPad->GetLocalClearance().has_value() )
    {
        m_out->Print( "(clearance %s)",
                      formatInternalUnits( aPad->GetLocalClearance().value() ).c_str() );
    }

    if( aPad->GetLocalZoneConnection() != ZONE_CONNECTION::INHERITED )
    {
        m_out->Print( "(zone_connect %d)",
                      static_cast<int>( aPad->GetLocalZoneConnection() ) );
    }

    if( aPad->GetLocalThermalSpokeWidthOverride().has_value() )
    {
        m_out->Print( "(thermal_bridge_width %s)",
                      formatInternalUnits( aPad->GetLocalThermalSpokeWidthOverride().value() ).c_str() );
    }

    EDA_ANGLE defaultThermalSpokeAngle = ANGLE_90;

    if( aPad->GetShape( PADSTACK::ALL_LAYERS ) == PAD_SHAPE::CIRCLE ||
      ( aPad->GetShape( PADSTACK::ALL_LAYERS ) == PAD_SHAPE::CUSTOM
          && aPad->GetAnchorPadShape( PADSTACK::ALL_LAYERS ) == PAD_SHAPE::CIRCLE ) )
    {
        defaultThermalSpokeAngle = ANGLE_45;
    }

    if( aPad->GetThermalSpokeAngle() != defaultThermalSpokeAngle )
    {
        m_out->Print( "(thermal_bridge_angle %s)",
                      EDA_UNIT_UTILS::FormatAngle( aPad->GetThermalSpokeAngle() ).c_str() );
    }

    if( aPad->GetLocalThermalGapOverride().has_value() )
    {
        m_out->Print( "(thermal_gap %s)",
                      formatInternalUnits( aPad->GetLocalThermalGapOverride().value() ).c_str() );
    }

    auto anchorShape =
        [&]( PCB_LAYER_ID aLayer )
        {
            switch( aPad->GetAnchorPadShape( aLayer ) )
            {
            case PAD_SHAPE::RECTANGLE:  return "rect";
            default:
            case PAD_SHAPE::CIRCLE:     return "circle";
            }
        };

    auto formatPrimitives =
        [&]( PCB_LAYER_ID aLayer )
        {
            m_out->Print( "(primitives" );

            // Output all basic shapes
            for( const std::shared_ptr<PCB_SHAPE>& primitive : aPad->GetPrimitives( aLayer ) )
            {
                switch( primitive->GetShape() )
                {
                case SHAPE_T::SEGMENT:
                    if( primitive->IsProxyItem() )
                    {
                        m_out->Print( "(gr_vector (start %s) (end %s)",
                                      formatInternalUnits( primitive->GetStart() ).c_str(),
                                      formatInternalUnits( primitive->GetEnd() ).c_str() );
                    }
                    else
                    {
                        m_out->Print( "(gr_line (start %s) (end %s)",
                                      formatInternalUnits( primitive->GetStart() ).c_str(),
                                      formatInternalUnits( primitive->GetEnd() ).c_str() );
                    }
                    break;

                case SHAPE_T::RECTANGLE:
                    if( primitive->IsProxyItem() )
                    {
                        m_out->Print( "(gr_bbox (start %s) (end %s)",
                                      formatInternalUnits( primitive->GetStart() ).c_str(),
                                      formatInternalUnits( primitive->GetEnd() ).c_str() );
                    }
                    else
                    {
                        m_out->Print( "(gr_rect (start %s) (end %s)",
                                      formatInternalUnits( primitive->GetStart() ).c_str(),
                                      formatInternalUnits( primitive->GetEnd() ).c_str() );
                    }
                    break;

                case SHAPE_T::ARC:
                    m_out->Print( "(gr_arc (start %s) (mid %s) (end %s)",
                                  formatInternalUnits( primitive->GetStart() ).c_str(),
                                  formatInternalUnits( primitive->GetArcMid() ).c_str(),
                                  formatInternalUnits( primitive->GetEnd() ).c_str() );
                    break;

                case SHAPE_T::CIRCLE:
                    m_out->Print( "(gr_circle (center %s) (end %s)",
                                  formatInternalUnits( primitive->GetStart() ).c_str(),
                                  formatInternalUnits( primitive->GetEnd() ).c_str() );
                    break;

                case SHAPE_T::BEZIER:
                    m_out->Print( "(gr_curve (pts (xy %s) (xy %s) (xy %s) (xy %s))",
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

                        m_out->Print( "(gr_poly" );
                        formatPolyPts( outline );
                    }
                    break;

                default:
                    break;
                }

                if( !primitive->IsProxyItem() )
                    m_out->Print( "(width %s)", formatInternalUnits( primitive->GetWidth() ).c_str() );

                // The filled flag represents if a solid fill is present on circles,
                // rectangles and polygons
                if( ( primitive->GetShape() == SHAPE_T::POLY )
                    || ( primitive->GetShape() == SHAPE_T::RECTANGLE )
                    || ( primitive->GetShape() == SHAPE_T::CIRCLE ) )
                {
                    KICAD_FORMAT::FormatBool( m_out, "fill", primitive->IsSolidFill() );
                }

                m_out->Print( ")" );
            }

            m_out->Print( ")" );   // end of (primitives
        };

    if( aPad->GetShape( PADSTACK::ALL_LAYERS ) == PAD_SHAPE::CUSTOM )
    {
        m_out->Print( "(options" );

        if( aPad->GetCustomShapeInZoneOpt() == PADSTACK::CUSTOM_SHAPE_ZONE_MODE::CONVEXHULL )
            m_out->Print( "(clearance convexhull)" );
        else
            m_out->Print( "(clearance outline)" );

        // Output the anchor pad shape (circle/rect)
        m_out->Print( "(anchor %s)", anchorShape( PADSTACK::ALL_LAYERS ) );

        m_out->Print( ")");  // end of (options ...

        // Output graphic primitive of the pad shape
        formatPrimitives( PADSTACK::ALL_LAYERS );
    }

    if( !isDefaultTeardropParameters( aPad->GetTeardropParams() ) )
        formatTeardropParameters( aPad->GetTeardropParams() );

    m_out->Print( 0, " (tenting " );
    KICAD_FORMAT::FormatOptBool( m_out, "front",
                                 aPad->Padstack().FrontOuterLayers().has_solder_mask );
    KICAD_FORMAT::FormatOptBool( m_out, "back",
                                 aPad->Padstack().BackOuterLayers().has_solder_mask );
    m_out->Print( 0, ")" );

    KICAD_FORMAT::FormatUuid( m_out, aPad->m_Uuid );

    // TODO: Refactor so that we call formatPadLayer( ALL_LAYERS ) above instead of redundant code
    auto formatPadLayer =
        [&]( PCB_LAYER_ID aLayer )
        {
            const PADSTACK& padstack = aPad->Padstack();

            m_out->Print( "(shape %s)", shapeName( aLayer ) );
            m_out->Print( "(size %s)", formatInternalUnits( aPad->GetSize( aLayer ) ).c_str() );

            const VECTOR2I& delta = aPad->GetDelta( aLayer );

            if( delta.x != 0 || delta.y != 0 )
                m_out->Print( "(rect_delta %s)", formatInternalUnits( delta ).c_str() );

            shapeoffset = aPad->GetOffset( aLayer );

            if( shapeoffset.x != 0 || shapeoffset.y != 0 )
                m_out->Print( "(offset %s)", formatInternalUnits( shapeoffset ).c_str() );

            formatCornerProperties( aLayer );

            if( aPad->GetShape( aLayer ) == PAD_SHAPE::CUSTOM )
            {
                m_out->Print( "(options" );

                // Output the anchor pad shape (circle/rect)
                m_out->Print( "(anchor %s)", anchorShape( aLayer ) );

                m_out->Print( ")" ); // end of (options ...

                // Output graphic primitive of the pad shape
                formatPrimitives( aLayer );
            }

            EDA_ANGLE defaultLayerAngle = ANGLE_90;

            if( aPad->GetShape( aLayer ) == PAD_SHAPE::CIRCLE ||
                ( aPad->GetShape( aLayer ) == PAD_SHAPE::CUSTOM
                  && aPad->GetAnchorPadShape( aLayer ) == PAD_SHAPE::CIRCLE ) )
            {
                defaultLayerAngle = ANGLE_45;
            }

            EDA_ANGLE layerSpokeAngle = padstack.ThermalSpokeAngle( aLayer );

            if( layerSpokeAngle != defaultLayerAngle )
            {
                m_out->Print( "(thermal_bridge_angle %s)",
                              EDA_UNIT_UTILS::FormatAngle( layerSpokeAngle ).c_str() );
            }

            if( padstack.ThermalGap( aLayer ).has_value() )
            {
                m_out->Print( "(thermal_gap %s)",
                              formatInternalUnits( *padstack.ThermalGap( aLayer ) ).c_str() );
            }

            if( padstack.ThermalSpokeWidth( aLayer ).has_value() )
            {
                m_out->Print( "(thermal_bridge_width %s)",
                              formatInternalUnits( *padstack.ThermalSpokeWidth( aLayer ) ).c_str() );
            }

            if( padstack.Clearance( aLayer ).has_value() )
            {
                m_out->Print( "(clearance %s)",
                              formatInternalUnits( *padstack.Clearance( aLayer ) ).c_str() );
            }

            if( padstack.ZoneConnection( aLayer ).has_value() )
            {
                m_out->Print( "(zone_connect %d)",
                              static_cast<int>( *padstack.ZoneConnection( aLayer ) ) );
            }
        };


    if( aPad->Padstack().Mode() != PADSTACK::MODE::NORMAL )
    {
        if( aPad->Padstack().Mode() == PADSTACK::MODE::FRONT_INNER_BACK )
        {
            m_out->Print( "(padstack (mode front_inner_back)" );

            m_out->Print( "(layer \"Inner\"" );
            formatPadLayer( PADSTACK::INNER_LAYERS );
            m_out->Print( ")" );
            m_out->Print( "(layer \"B.Cu\"" );
            formatPadLayer( B_Cu );
            m_out->Print( ")" );
        }
        else
        {
            m_out->Print( "(padstack (mode custom)" );

            int layerCount = board ? board->GetCopperLayerCount() : MAX_CU_LAYERS;

            for( PCB_LAYER_ID layer : LAYER_RANGE( F_Cu, B_Cu, layerCount ) )
            {
                if( layer == F_Cu )
                    continue;

                m_out->Print( "(layer %s", m_out->Quotew( LSET::Name( layer ) ).c_str() );
                formatPadLayer( layer );
                m_out->Print( ")" );
            }
        }

        m_out->Print( ")" );
    }

    m_out->Print( ")" );
}


void PCB_IO_KICAD_SEXPR::format( const PCB_TEXT* aText ) const
{
    FOOTPRINT*       parentFP = aText->GetParentFootprint();
    std::string      prefix;
    std::string      type;
    VECTOR2I         pos = aText->GetTextPos();
    const PCB_FIELD* field = dynamic_cast<const PCB_FIELD*>( aText );

    // Always format dimension text as gr_text
    if( dynamic_cast<const PCB_DIMENSION_BASE*>( aText ) )
        parentFP = nullptr;

    if( parentFP )
    {
        prefix = "fp";
        type = "user";

        pos -= parentFP->GetPosition();
        RotatePoint( pos, -parentFP->GetOrientation() );
    }
    else
    {
        prefix = "gr";
    }

    if( !field )
    {
        m_out->Print( "(%s_text %s %s",
                      prefix.c_str(),
                      type.c_str(),
                      m_out->Quotew( aText->GetText() ).c_str() );

        if( aText->IsLocked() )
            KICAD_FORMAT::FormatBool( m_out, "locked", true );
    }

    m_out->Print( "(at %s %s)",
                  formatInternalUnits( pos ).c_str(),
                  EDA_UNIT_UTILS::FormatAngle( aText->GetTextAngle() ).c_str() );

    if( parentFP && !aText->IsKeepUpright() )
        KICAD_FORMAT::FormatBool( m_out, "unlocked", true );

    formatLayer( aText->GetLayer(), aText->IsKnockout() );

    if( field && !field->IsVisible() )
        KICAD_FORMAT::FormatBool( m_out, "hide", true );

    KICAD_FORMAT::FormatUuid( m_out, aText->m_Uuid );

    // Currently, texts have no specific color and no hyperlink.
    // so ensure they are never written in kicad_pcb file
    int ctl_flags = CTL_OMIT_COLOR | CTL_OMIT_HYPERLINK;

    aText->EDA_TEXT::Format( m_out, ctl_flags );

    if( aText->GetFont() && aText->GetFont()->IsOutline() )
        formatRenderCache( aText );

    if( !field )
        m_out->Print( ")" );
}


void PCB_IO_KICAD_SEXPR::format( const PCB_TEXTBOX* aTextBox ) const
{
    FOOTPRINT*  parentFP = aTextBox->GetParentFootprint();

    m_out->Print( "(%s %s",
                  aTextBox->Type() == PCB_TABLECELL_T ? "table_cell"
                                                      : parentFP ? "fp_text_box"
                                                                 : "gr_text_box",
                  m_out->Quotew( aTextBox->GetText() ).c_str() );

    if( aTextBox->IsLocked() )
        KICAD_FORMAT::FormatBool( m_out, "locked", true );

    if( aTextBox->GetShape() == SHAPE_T::RECTANGLE )
    {
        m_out->Print( "(start %s) (end %s)",
                      formatInternalUnits( aTextBox->GetStart(), parentFP ).c_str(),
                      formatInternalUnits( aTextBox->GetEnd(), parentFP ).c_str() );
    }
    else if( aTextBox->GetShape() == SHAPE_T::POLY )
    {
        const SHAPE_POLY_SET& poly = aTextBox->GetPolyShape();
        const SHAPE_LINE_CHAIN& outline = poly.Outline( 0 );

        formatPolyPts( outline, parentFP );
    }
    else
    {
        UNIMPLEMENTED_FOR( aTextBox->SHAPE_T_asString() );
    }

    m_out->Print( "(margins %s %s %s %s)",
                  formatInternalUnits( aTextBox->GetMarginLeft() ).c_str(),
                  formatInternalUnits( aTextBox->GetMarginTop() ).c_str(),
                  formatInternalUnits( aTextBox->GetMarginRight() ).c_str(),
                  formatInternalUnits( aTextBox->GetMarginBottom() ).c_str() );

    if( const PCB_TABLECELL* cell = dynamic_cast<const PCB_TABLECELL*>( aTextBox ) )
        m_out->Print( "(span %d %d)", cell->GetColSpan(), cell->GetRowSpan() );

    EDA_ANGLE angle = aTextBox->GetTextAngle();

    if( parentFP )
    {
        angle -= parentFP->GetOrientation();
        angle.Normalize720();
    }

    if( !angle.IsZero() )
        m_out->Print( "(angle %s)", EDA_UNIT_UTILS::FormatAngle( angle ).c_str() );

    formatLayer( aTextBox->GetLayer() );

    KICAD_FORMAT::FormatUuid( m_out, aTextBox->m_Uuid );

    aTextBox->EDA_TEXT::Format( m_out, 0 );

    if( aTextBox->Type() != PCB_TABLECELL_T )
    {
        KICAD_FORMAT::FormatBool( m_out, "border", aTextBox->IsBorderEnabled() );
        aTextBox->GetStroke().Format( m_out, pcbIUScale );

        KICAD_FORMAT::FormatBool( m_out, "knockout", aTextBox->IsKnockout() );
    }

    if( aTextBox->GetFont() && aTextBox->GetFont()->IsOutline() )
        formatRenderCache( aTextBox );

    m_out->Print( ")" );
}


void PCB_IO_KICAD_SEXPR::format( const PCB_TABLE* aTable ) const
{
    wxCHECK_RET( aTable != nullptr && m_out != nullptr, "" );

    m_out->Print( "(table (column_count %d)", aTable->GetColCount() );

    if( aTable->IsLocked() )
        KICAD_FORMAT::FormatBool( m_out, "locked", true );

    formatLayer( aTable->GetLayer() );

    m_out->Print( "(border" );
    KICAD_FORMAT::FormatBool( m_out, "external", aTable->StrokeExternal() );
    KICAD_FORMAT::FormatBool( m_out, "header", aTable->StrokeHeaderSeparator() );

    if( aTable->StrokeExternal() || aTable->StrokeHeaderSeparator() )
        aTable->GetBorderStroke().Format( m_out, pcbIUScale );

    m_out->Print( ")" );                // Close `border` token.

    m_out->Print( "(separators" );
    KICAD_FORMAT::FormatBool( m_out, "rows", aTable->StrokeRows() );
    KICAD_FORMAT::FormatBool( m_out, "cols", aTable->StrokeColumns() );

    if( aTable->StrokeRows() || aTable->StrokeColumns() )
        aTable->GetSeparatorsStroke().Format( m_out, pcbIUScale );

    m_out->Print( ")" );               // Close `separators` token.

    m_out->Print( "(column_widths" );

    for( int col = 0; col < aTable->GetColCount(); ++col )
        m_out->Print( " %s", formatInternalUnits( aTable->GetColWidth( col ) ).c_str() );

    m_out->Print( ")" );

    m_out->Print( "(row_heights" );

    for( int row = 0; row < aTable->GetRowCount(); ++row )
        m_out->Print( " %s", formatInternalUnits( aTable->GetRowHeight( row ) ).c_str() );

    m_out->Print( ")" );

    m_out->Print( "(cells" );

    for( PCB_TABLECELL* cell : aTable->GetCells() )
        format( static_cast<PCB_TEXTBOX*>( cell ) );

    m_out->Print( ")" );        // Close `cells` token.
    m_out->Print( ")" );        // Close `table` token.
}


void PCB_IO_KICAD_SEXPR::format( const PCB_GROUP* aGroup ) const
{
    // Don't write empty groups
    if( aGroup->GetItems().empty() )
        return;

    m_out->Print( "(group %s", m_out->Quotew( aGroup->GetName() ).c_str() );

    KICAD_FORMAT::FormatUuid( m_out, aGroup->m_Uuid );

    if( aGroup->IsLocked() )
        KICAD_FORMAT::FormatBool( m_out, "locked", true );

    if( aGroup->HasDesignBlockLink() )
        m_out->Print( "(lib_id \"%s\")", aGroup->GetDesignBlockLibId().Format().c_str() );

    wxArrayString memberIds;

    for( EDA_ITEM* member : aGroup->GetItems() )
        memberIds.Add( member->m_Uuid.AsString() );

    memberIds.Sort();

    m_out->Print( "(members" );

    for( const wxString& memberId : memberIds )
        m_out->Print( " %s", m_out->Quotew( memberId ).c_str() );

    m_out->Print( ")" );        // Close `members` token.
    m_out->Print( ")" );        // Close `group` token.
}


void PCB_IO_KICAD_SEXPR::format( const PCB_GENERATOR* aGenerator ) const
{
    // Some conditions appear to still be creating ghost tuning patterns.  Don't save them.
    if( aGenerator->GetGeneratorType() == wxT( "tuning_pattern" )
            && aGenerator->GetItems().empty() )
    {
        return;
    }

    m_out->Print( "(generated" );

    KICAD_FORMAT::FormatUuid( m_out, aGenerator->m_Uuid );

    m_out->Print( "(type %s) (name %s) (layer %s)",
                  TO_UTF8( aGenerator->GetGeneratorType() ),
                  m_out->Quotew( aGenerator->GetName() ).c_str(),
                  m_out->Quotew( LSET::Name( aGenerator->GetLayer() ) ).c_str() );

    if( aGenerator->IsLocked() )
        KICAD_FORMAT::FormatBool( m_out, "locked", true );

    for( const auto& [key, value] : aGenerator->GetProperties() )
    {
        if( value.CheckType<double>() || value.CheckType<int>() || value.CheckType<long>()
            || value.CheckType<long long>() )
        {
            double val;

            if( !value.GetAs( &val ) )
                continue;

            std::string buf = fmt::format( "{:.10g}", val );

            // Don't quote numbers
            m_out->Print( "(%s %s)", key.c_str(), buf.c_str() );
        }
        else if( value.CheckType<bool>() )
        {
            bool val;
            value.GetAs( &val );

            KICAD_FORMAT::FormatBool( m_out, key, val );
        }
        else if( value.CheckType<VECTOR2I>() )
        {
            VECTOR2I val;
            value.GetAs( &val );

            m_out->Print( "(%s (xy %s))",
                          key.c_str(),
                          formatInternalUnits( val ).c_str() );
        }
        else if( value.CheckType<SHAPE_LINE_CHAIN>() )
        {
            SHAPE_LINE_CHAIN val;
            value.GetAs( &val );

            m_out->Print( "(%s ", key.c_str() );
            formatPolyPts( val );
            m_out->Print( ")" );
        }
        else
        {
            wxString val;

            if( value.CheckType<wxString>() )
            {
                value.GetAs( &val );
            }
            else if( value.CheckType<std::string>() )
            {
                std::string str;
                value.GetAs( &str );

                val = wxString::FromUTF8( str );
            }

            m_out->Print( "(%s %s)", key.c_str(), m_out->Quotew( val ).c_str() );
        }
    }

    wxArrayString memberIds;

    for( EDA_ITEM* member : aGenerator->GetItems() )
        memberIds.Add( member->m_Uuid.AsString() );

    memberIds.Sort();

    m_out->Print( "(members" );

    for( const wxString& memberId : memberIds )
        m_out->Print( " %s", m_out->Quotew( memberId ).c_str() );

    m_out->Print( ")" );        // Close `members` token.
    m_out->Print( ")" );        // Close `generated` token.
}


void PCB_IO_KICAD_SEXPR::format( const PCB_TRACK* aTrack ) const
{
    if( aTrack->Type() == PCB_VIA_T )
    {
        PCB_LAYER_ID  layer1, layer2;

        const PCB_VIA* via = static_cast<const PCB_VIA*>( aTrack );
        const BOARD*   board = via->GetBoard();

        wxCHECK_RET( board != nullptr, wxT( "Via has no parent." ) );

        m_out->Print( "(via" );

        via->LayerPair( &layer1, &layer2 );

        switch( via->GetViaType() )
        {
        case VIATYPE::THROUGH: //  Default shape not saved.
            break;

        case VIATYPE::BLIND_BURIED:
            m_out->Print( " blind " );
            break;

        case VIATYPE::MICROVIA:
            m_out->Print( " micro " );
            break;

        default:
            THROW_IO_ERROR( wxString::Format( _( "unknown via type %d"  ), via->GetViaType() ) );
        }

        m_out->Print( "(at %s) (size %s)",
                      formatInternalUnits( aTrack->GetStart() ).c_str(),
                      formatInternalUnits( via->GetWidth( F_Cu ) ).c_str() );

        // Old boards were using UNDEFINED_DRILL_DIAMETER value in file for via drill when
        // via drill was the netclass value.
        // recent boards always set the via drill to the actual value, but now we need to
        // always store the drill value, because netclass value is not stored in the board file.
        // Otherwise the drill value of some (old) vias can be unknown
        if( via->GetDrill() != UNDEFINED_DRILL_DIAMETER )
            m_out->Print( "(drill %s)", formatInternalUnits( via->GetDrill() ).c_str() );
        else
            m_out->Print( "(drill %s)", formatInternalUnits( via->GetDrillValue() ).c_str() );

        m_out->Print( "(layers %s %s)",
                      m_out->Quotew( LSET::Name( layer1 ) ).c_str(),
                      m_out->Quotew( LSET::Name( layer2 ) ).c_str() );

        switch( via->Padstack().UnconnectedLayerMode() )
        {
        case PADSTACK::UNCONNECTED_LAYER_MODE::REMOVE_ALL:
            KICAD_FORMAT::FormatBool( m_out, "remove_unused_layers", true );
            KICAD_FORMAT::FormatBool( m_out, "keep_end_layers", false );
            break;

        case PADSTACK::UNCONNECTED_LAYER_MODE::REMOVE_EXCEPT_START_AND_END:
            KICAD_FORMAT::FormatBool( m_out, "remove_unused_layers", true );
            KICAD_FORMAT::FormatBool( m_out, "keep_end_layers", true );
            break;

        case PADSTACK::UNCONNECTED_LAYER_MODE::START_END_ONLY:
            KICAD_FORMAT::FormatBool( m_out, "start_end_only", true );
            break;

        case PADSTACK::UNCONNECTED_LAYER_MODE::KEEP_ALL:
            break;
        }

        if( via->IsLocked() )
            KICAD_FORMAT::FormatBool( m_out, "locked", true );

        if( via->GetIsFree() )
            KICAD_FORMAT::FormatBool( m_out, "free", true );

        if( via->GetRemoveUnconnected() )
        {
            m_out->Print( "(zone_layer_connections" );

            for( PCB_LAYER_ID layer : board->GetEnabledLayers().CuStack() )
            {
                if( via->GetZoneLayerOverride( layer ) == ZLO_FORCE_FLASHED )
                    m_out->Print( " %s", m_out->Quotew( LSET::Name( layer ) ).c_str() );
            }

            m_out->Print( ")" );
        }

        const PADSTACK& padstack = via->Padstack();

        m_out->Print( 0, " (tenting " );
        KICAD_FORMAT::FormatOptBool( m_out, "front", padstack.FrontOuterLayers().has_solder_mask );
        KICAD_FORMAT::FormatOptBool( m_out, "back", padstack.BackOuterLayers().has_solder_mask );
        m_out->Print( 0, ")" );

        KICAD_FORMAT::FormatOptBool( m_out, "capping", padstack.Drill().is_capped );

        m_out->Print( 0, " (covering " );
        KICAD_FORMAT::FormatOptBool( m_out, "front", padstack.FrontOuterLayers().has_covering );
        KICAD_FORMAT::FormatOptBool( m_out, "back", padstack.BackOuterLayers().has_covering );
        m_out->Print( 0, ")" );

        m_out->Print( 0, " (plugging " );
        KICAD_FORMAT::FormatOptBool( m_out, "front", padstack.FrontOuterLayers().has_plugging );
        KICAD_FORMAT::FormatOptBool( m_out, "back", padstack.BackOuterLayers().has_plugging );
        m_out->Print( 0, ")" );

        KICAD_FORMAT::FormatOptBool( m_out, "filling", padstack.Drill().is_filled );

        if( padstack.Mode() != PADSTACK::MODE::NORMAL )
        {
            m_out->Print( "(padstack" );

            if( padstack.Mode() == PADSTACK::MODE::FRONT_INNER_BACK )
            {
                m_out->Print( "(mode front_inner_back)" );

                m_out->Print( "(layer \"Inner\"" );
                m_out->Print( "(size %s)",
                              formatInternalUnits( padstack.Size( PADSTACK::INNER_LAYERS ).x ).c_str() );
                m_out->Print( ")" );
                m_out->Print( "(layer \"B.Cu\"" );
                m_out->Print( "(size %s)",
                              formatInternalUnits( padstack.Size( B_Cu ).x ).c_str() );
                m_out->Print( ")" );
            }
            else
            {
                m_out->Print( "(mode custom)" );

                for( PCB_LAYER_ID layer : LAYER_RANGE( F_Cu, B_Cu, board->GetCopperLayerCount() ) )
                {
                    if( layer == F_Cu )
                        continue;

                    m_out->Print( "(layer %s", m_out->Quotew( LSET::Name( layer ) ).c_str() );
                    m_out->Print( "(size %s)",
                                  formatInternalUnits( padstack.Size( layer ).x ).c_str() );
                    m_out->Print( ")" );
                }
            }

            m_out->Print( ")" );
        }

        if( !isDefaultTeardropParameters( via->GetTeardropParams() ) )
            formatTeardropParameters( via->GetTeardropParams() );
    }
    else
    {
        if( aTrack->Type() == PCB_ARC_T )
        {
            const PCB_ARC* arc = static_cast<const PCB_ARC*>( aTrack );

            m_out->Print( "(arc (start %s) (mid %s) (end %s) (width %s)",
                          formatInternalUnits( arc->GetStart() ).c_str(),
                          formatInternalUnits( arc->GetMid() ).c_str(),
                          formatInternalUnits( arc->GetEnd() ).c_str(),
                          formatInternalUnits( arc->GetWidth() ).c_str() );
        }
        else
        {
            m_out->Print( "(segment (start %s) (end %s) (width %s)",
                          formatInternalUnits( aTrack->GetStart() ).c_str(),
                          formatInternalUnits( aTrack->GetEnd() ).c_str(),
                          formatInternalUnits( aTrack->GetWidth() ).c_str() );
        }

        if( aTrack->IsLocked() )
            KICAD_FORMAT::FormatBool( m_out, "locked", true );

        if( aTrack->GetLayerSet().count() > 1 )
            formatLayers( aTrack->GetLayerSet(), false /* enumerate layers */ );
        else
            formatLayer( aTrack->GetLayer() );

        if( aTrack->HasSolderMask()
                && aTrack->GetLocalSolderMaskMargin().has_value()
                && IsExternalCopperLayer( aTrack->GetLayer() ) )
        {
            m_out->Print( "(solder_mask_margin %s)",
                          formatInternalUnits( aTrack->GetLocalSolderMaskMargin().value() ).c_str() );
        }
    }

    m_out->Print( "(net %d)", m_mapping->Translate( aTrack->GetNetCode() ) );

    KICAD_FORMAT::FormatUuid( m_out, aTrack->m_Uuid );
    m_out->Print( ")" );
}


void PCB_IO_KICAD_SEXPR::format( const ZONE* aZone ) const
{
    // Save the NET info.
    // For keepout and non copper zones, net code and net name are irrelevant
    // so be sure a dummy value is stored, just for ZONE compatibility
    // (perhaps netcode and netname should be not stored)

    bool has_no_net = aZone->GetIsRuleArea() || !aZone->IsOnCopperLayer();

    m_out->Print( "(zone (net %d) (net_name %s)",
                  has_no_net ? 0 : m_mapping->Translate( aZone->GetNetCode() ),
                  m_out->Quotew( has_no_net ? wxString( wxT("") ) : aZone->GetNetname() ).c_str() );

    if( aZone->IsLocked() )
        KICAD_FORMAT::FormatBool( m_out, "locked", true );

    // If a zone exists on multiple layers, format accordingly
    LSET layers = aZone->GetLayerSet();

    if( aZone->GetBoard() )
        layers &= aZone->GetBoard()->GetEnabledLayers();

    // Always enumerate every layer for a zone on a copper layer
    if( layers.count() > 1 )
        formatLayers( layers, aZone->IsOnCopperLayer() );
    else
        formatLayer( aZone->GetFirstLayer() );

    if( !aZone->IsTeardropArea() )
        KICAD_FORMAT::FormatUuid( m_out, aZone->m_Uuid );

    if( !aZone->GetZoneName().empty() && !aZone->IsTeardropArea() )
        m_out->Print( "(name %s)", m_out->Quotew( aZone->GetZoneName() ).c_str() );

    // Save the outline aux info
    std::string hatch;

    switch( aZone->GetHatchStyle() )
    {
    default:
    case ZONE_BORDER_DISPLAY_STYLE::NO_HATCH:      hatch = "none"; break;
    case ZONE_BORDER_DISPLAY_STYLE::DIAGONAL_EDGE: hatch = "edge"; break;
    case ZONE_BORDER_DISPLAY_STYLE::DIAGONAL_FULL: hatch = "full"; break;
    }

    m_out->Print( "(hatch %s %s)", hatch.c_str(),
                  formatInternalUnits( aZone->GetBorderHatchPitch() ).c_str() );



    if( aZone->GetAssignedPriority() > 0 )
        m_out->Print( "(priority %d)", aZone->GetAssignedPriority() );

    // Add teardrop keywords in file: (attr (teardrop (type xxx))) where xxx is the teardrop type
    if( aZone->IsTeardropArea() )
    {
        m_out->Print( "(attr (teardrop (type %s)))",
                      aZone->GetTeardropAreaType() == TEARDROP_TYPE::TD_VIAPAD ? "padvia"
                                                                               : "track_end" );
    }

    m_out->Print( "(connect_pads" );

    switch( aZone->GetPadConnection() )
    {
    default:
    case ZONE_CONNECTION::THERMAL: // Default option not saved or loaded.
        break;

    case ZONE_CONNECTION::THT_THERMAL:
        m_out->Print( " thru_hole_only" );
        break;

    case ZONE_CONNECTION::FULL:
        m_out->Print( " yes" );
        break;

    case ZONE_CONNECTION::NONE:
        m_out->Print( " no" );
        break;
    }

    m_out->Print( "(clearance %s)",
                  formatInternalUnits( aZone->GetLocalClearance().value() ).c_str() );

    m_out->Print( ")" );

    m_out->Print( "(min_thickness %s)",
                  formatInternalUnits( aZone->GetMinThickness() ).c_str() );

    if( aZone->GetIsRuleArea() )
    {
        // Keepout settings
        m_out->Print( "(keepout (tracks %s) (vias %s) (pads %s) (copperpour %s) (footprints %s))",
                      aZone->GetDoNotAllowTracks() ? "not_allowed" : "allowed",
                      aZone->GetDoNotAllowVias() ? "not_allowed" : "allowed",
                      aZone->GetDoNotAllowPads() ? "not_allowed" : "allowed",
                      aZone->GetDoNotAllowZoneFills() ? "not_allowed" : "allowed",
                      aZone->GetDoNotAllowFootprints() ? "not_allowed" : "allowed" );

        // Multichannel settings
        m_out->Print( "(placement" );
        KICAD_FORMAT::FormatBool( m_out, "enabled", aZone->GetPlacementAreaEnabled() );

        switch( aZone->GetPlacementAreaSourceType() )
        {
        case PLACEMENT_SOURCE_T::SHEETNAME:
            m_out->Print( "(sheetname %s)", m_out->Quotew( aZone->GetPlacementAreaSource() ).c_str() );
            break;
        case PLACEMENT_SOURCE_T::COMPONENT_CLASS:
            m_out->Print( "(component_class %s)", m_out->Quotew( aZone->GetPlacementAreaSource() ).c_str() );
            break;
        case PLACEMENT_SOURCE_T::GROUP_PLACEMENT:
            m_out->Print( "(group %s)", m_out->Quotew( aZone->GetPlacementAreaSource() ).c_str() );
            break;
        }

        m_out->Print( ")" );
    }

    m_out->Print( "(fill" );

    // Default is not filled.
    if( aZone->IsFilled() )
        m_out->Print( " yes" );

    // Default is polygon filled.
    if( aZone->GetFillMode() == ZONE_FILL_MODE::HATCH_PATTERN )
        m_out->Print( "(mode hatch)" );

    if( !aZone->IsTeardropArea() )
    {
        m_out->Print( "(thermal_gap %s) (thermal_bridge_width %s)",
                      formatInternalUnits( aZone->GetThermalReliefGap() ).c_str(),
                      formatInternalUnits( aZone->GetThermalReliefSpokeWidth() ).c_str() );
    }

    if( aZone->GetCornerSmoothingType() != ZONE_SETTINGS::SMOOTHING_NONE )
    {
        switch( aZone->GetCornerSmoothingType() )
        {
        case ZONE_SETTINGS::SMOOTHING_CHAMFER:
            m_out->Print( "(smoothing chamfer)" );
            break;

        case ZONE_SETTINGS::SMOOTHING_FILLET:
            m_out->Print( "(smoothing fillet)" );
            break;

        default:
            THROW_IO_ERROR( wxString::Format( _( "unknown zone corner smoothing type %d"  ),
                                              aZone->GetCornerSmoothingType() ) );
        }

        if( aZone->GetCornerRadius() != 0 )
            m_out->Print( "(radius %s)", formatInternalUnits( aZone->GetCornerRadius() ).c_str() );
    }

    m_out->Print( "(island_removal_mode %d)",
                  static_cast<int>( aZone->GetIslandRemovalMode() ) );

    if( aZone->GetIslandRemovalMode() == ISLAND_REMOVAL_MODE::AREA )
    {
        m_out->Print( "(island_area_min %s)",
                      formatInternalUnits( aZone->GetMinIslandArea() / pcbIUScale.IU_PER_MM ).c_str() );
    }

    if( aZone->GetFillMode() == ZONE_FILL_MODE::HATCH_PATTERN )
    {
        m_out->Print( "(hatch_thickness %s) (hatch_gap %s) (hatch_orientation %s)",
                      formatInternalUnits( aZone->GetHatchThickness() ).c_str(),
                      formatInternalUnits( aZone->GetHatchGap() ).c_str(),
                      FormatDouble2Str( aZone->GetHatchOrientation().AsDegrees() ).c_str() );

        if( aZone->GetHatchSmoothingLevel() > 0 )
        {
            m_out->Print( "(hatch_smoothing_level %d) (hatch_smoothing_value %s)",
                          aZone->GetHatchSmoothingLevel(),
                          FormatDouble2Str( aZone->GetHatchSmoothingValue() ).c_str() );
        }

        m_out->Print( "(hatch_border_algorithm %s) (hatch_min_hole_area %s)",
                      aZone->GetHatchBorderAlgorithm() ? "hatch_thickness" : "min_thickness",
                      FormatDouble2Str( aZone->GetHatchHoleMinArea() ).c_str() );
    }

    m_out->Print( ")" );

    for( const auto& [layer, properties] : aZone->LayerProperties() )
    {
        format( properties, 0, layer );
    }

    if( aZone->GetNumCorners() )
    {
        SHAPE_POLY_SET::POLYGON poly = aZone->Outline()->Polygon(0);

        for( const SHAPE_LINE_CHAIN& chain : poly )
        {
            m_out->Print( "(polygon" );
            formatPolyPts( chain );
            m_out->Print( ")" );
        }
    }

    // Save the PolysList (filled areas)
    for( PCB_LAYER_ID layer : aZone->GetLayerSet().Seq() )
    {
        const std::shared_ptr<SHAPE_POLY_SET>& fv = aZone->GetFilledPolysList( layer );

        for( int ii = 0; ii < fv->OutlineCount(); ++ii )
        {
            m_out->Print( "(filled_polygon" );
            m_out->Print( "(layer %s)", m_out->Quotew( LSET::Name( layer ) ).c_str() );

            if( aZone->IsIsland( layer, ii ) )
                KICAD_FORMAT::FormatBool( m_out, "island", true );

            const SHAPE_LINE_CHAIN& chain = fv->COutline( ii );

            formatPolyPts( chain );
            m_out->Print( ")" );
        }
    }

    m_out->Print( ")" );
}


void PCB_IO_KICAD_SEXPR::format( const ZONE_LAYER_PROPERTIES& aZoneLayerProperties, int aNestLevel,
                                 PCB_LAYER_ID aLayer ) const
{
    // Do not store the layer properties if no value is actually set.
    if( !aZoneLayerProperties.hatching_offset.has_value() )
        return;

    m_out->Print( aNestLevel, "(property\n" );
    m_out->Print( aNestLevel, "(layer %s)\n", m_out->Quotew( LSET::Name( aLayer ) ).c_str() );

    if( aZoneLayerProperties.hatching_offset.has_value() )
    {
        m_out->Print( aNestLevel, "(hatch_position (xy %s))",
                      formatInternalUnits( aZoneLayerProperties.hatching_offset.value() ).c_str() );
    }

    m_out->Print( aNestLevel, ")\n" );
}


PCB_IO_KICAD_SEXPR::PCB_IO_KICAD_SEXPR( int aControlFlags ) : PCB_IO( wxS( "KiCad" ) ),
    m_cache( nullptr ),
    m_ctl( aControlFlags ),
    m_mapping( new NETINFO_MAPPING() )
{
    init( nullptr );
    m_out = &m_sf;
}


PCB_IO_KICAD_SEXPR::~PCB_IO_KICAD_SEXPR()
{
    delete m_cache;
    delete m_mapping;
}


BOARD* PCB_IO_KICAD_SEXPR::LoadBoard( const wxString& aFileName, BOARD* aAppendToMe,
                                      const std::map<std::string, UTF8>* aProperties,
                                      PROJECT* aProject )
{
    FILE_LINE_READER reader( aFileName );

    unsigned lineCount = 0;

    fontconfig::FONTCONFIG::SetReporter( &WXLOG_REPORTER::GetInstance() );

    if( m_progressReporter )
    {
        m_progressReporter->Report( wxString::Format( _( "Loading %s..." ), aFileName ) );

        if( !m_progressReporter->KeepRefreshing() )
            THROW_IO_ERROR( _( "Open canceled by user." ) );

        while( reader.ReadLine() )
            lineCount++;

        reader.Rewind();
    }

    BOARD* board = DoLoad( reader, aAppendToMe, aProperties, m_progressReporter, lineCount );

    // Give the filename to the board if it's new
    if( !aAppendToMe )
        board->SetFileName( aFileName );

    return board;
}


BOARD* PCB_IO_KICAD_SEXPR::DoLoad( LINE_READER& aReader, BOARD* aAppendToMe,
                                   const std::map<std::string, UTF8>* aProperties,
                                   PROGRESS_REPORTER* aProgressReporter, unsigned aLineCount)
{
    init( aProperties );

    PCB_IO_KICAD_SEXPR_PARSER parser( &aReader, aAppendToMe, m_queryUserCallback,
                                      aProgressReporter, aLineCount );
    BOARD* board;

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


void PCB_IO_KICAD_SEXPR::init( const std::map<std::string, UTF8>* aProperties )
{
    m_board = nullptr;
    m_reader = nullptr;
    m_props = aProperties;
}


void PCB_IO_KICAD_SEXPR::validateCache( const wxString& aLibraryPath, bool checkModified )
{
    fontconfig::FONTCONFIG::SetReporter( nullptr );

    if( !m_cache || !m_cache->IsPath( aLibraryPath ) || ( checkModified && m_cache->IsModified() ) )
    {
        // a spectacular episode in memory management:
        delete m_cache;
        m_cache = new FP_CACHE( this, aLibraryPath );
        m_cache->Load();
    }
}


void PCB_IO_KICAD_SEXPR::FootprintEnumerate( wxArrayString& aFootprintNames,
                                             const wxString& aLibPath, bool aBestEfforts,
                                             const std::map<std::string, UTF8>* aProperties )
{
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


const FOOTPRINT* PCB_IO_KICAD_SEXPR::getFootprint( const wxString& aLibraryPath,
                                                   const wxString& aFootprintName,
                                                   const std::map<std::string, UTF8>* aProperties,
                                                   bool checkModified )
{
    init( aProperties );

    try
    {
        validateCache( aLibraryPath, checkModified );
    }
    catch( const IO_ERROR& )
    {
        // do nothing with the error
    }

    auto it = m_cache->GetFootprints().find( aFootprintName );

    if( it == m_cache->GetFootprints().end() )
        return nullptr;

    return it->second->GetFootprint().get();
}


const FOOTPRINT* PCB_IO_KICAD_SEXPR::GetEnumeratedFootprint( const wxString& aLibraryPath,
                                                             const wxString& aFootprintName,
                                                             const std::map<std::string, UTF8>* aProperties )
{
    return getFootprint( aLibraryPath, aFootprintName, aProperties, false );
}


bool PCB_IO_KICAD_SEXPR::FootprintExists( const wxString& aLibraryPath,
                                          const wxString& aFootprintName,
                                          const std::map<std::string, UTF8>* aProperties )
{
    // Note: checking the cache sounds like a good idea, but won't catch files which differ
    // only in case.
    //
    // Since this goes out to the native filesystem, we get platform differences (ie: MSW's
    // case-insensitive filesystem) handled "for free".
    // Warning: footprint names frequently contain a point. So be careful when initializing
    // wxFileName, and use a CTOR with extension specified
    wxFileName footprintFile( aLibraryPath, aFootprintName, FILEEXT::KiCadFootprintFileExtension );

    return footprintFile.Exists();
}


FOOTPRINT* PCB_IO_KICAD_SEXPR::ImportFootprint( const wxString& aFootprintPath,
                                                wxString& aFootprintNameOut,
                                                const std::map<std::string, UTF8>* aProperties )
{
    wxString fcontents;
    wxFFile  f( aFootprintPath );

    fontconfig::FONTCONFIG::SetReporter( nullptr );

    if( !f.IsOpened() )
        return nullptr;

    f.ReadAll( &fcontents );

    aFootprintNameOut = wxFileName( aFootprintPath ).GetName();

    return dynamic_cast<FOOTPRINT*>( Parse( fcontents ) );
}


FOOTPRINT* PCB_IO_KICAD_SEXPR::FootprintLoad( const wxString& aLibraryPath,
                                              const wxString& aFootprintName,
                                              bool  aKeepUUID,
                                              const std::map<std::string, UTF8>* aProperties )
{
    fontconfig::FONTCONFIG::SetReporter( nullptr );

    const FOOTPRINT* footprint = getFootprint( aLibraryPath, aFootprintName, aProperties, true );

    if( footprint )
    {
        FOOTPRINT* copy;

        if( aKeepUUID )
            copy = static_cast<FOOTPRINT*>( footprint->Clone() );
        else
            copy = static_cast<FOOTPRINT*>( footprint->Duplicate( IGNORE_PARENT_GROUP ) );

        copy->SetParent( nullptr );
        return copy;
    }

    return nullptr;
}


void PCB_IO_KICAD_SEXPR::FootprintSave( const wxString& aLibraryPath, const FOOTPRINT* aFootprint,
                                        const std::map<std::string, UTF8>* aProperties )
{
    init( aProperties );

    // In this public PLUGIN API function, we can safely assume it was
    // called for saving into a library path.
    m_ctl = CTL_FOR_LIBRARY;

    validateCache( aLibraryPath, !aProperties || !aProperties->contains( "skip_cache_validation" ) );

    if( !m_cache->IsWritable() )
    {
        if( !m_cache->Exists() )
        {
            const wxString msg = wxString::Format( _( "Library '%s' does not exist.\n"
                                                      "Would you like to create it?"),
                                                      aLibraryPath );

            if( !Pgm().IsGUI() || wxMessageBox( msg, _( "Library Not Found" ), wxYES_NO | wxICON_QUESTION ) != wxYES )
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

    wxString fpName = aFootprint->GetFPID().GetLibItemName().wx_str();
    ReplaceIllegalFileNameChars( fpName, '_' );

    // Quietly overwrite footprint and delete footprint file from path for any by same name.
    wxFileName fn( aLibraryPath, fpName, FILEEXT::KiCadFootprintFileExtension );

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
    auto     it = m_cache->GetFootprints().find( footprintName );

    if( it != m_cache->GetFootprints().end() )
    {
        wxLogTrace( traceKicadPcbPlugin, wxT( "Removing footprint file '%s'." ), fullPath );
        m_cache->GetFootprints().erase( footprintName );
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
            footprint->Flip( footprint->GetPosition(), cfg->m_FlipDirection );
        else
            footprint->Flip( footprint->GetPosition(), FLIP_DIRECTION::TOP_BOTTOM );
    }

    // Detach it from the board and its group
    footprint->SetParent( nullptr );
    footprint->SetParentGroup( nullptr );

    wxLogTrace( traceKicadPcbPlugin, wxT( "Creating s-expr footprint file '%s'." ), fullPath );
    m_cache->GetFootprints().insert( footprintName,
                                     new FP_CACHE_ENTRY( footprint,
                                                         WX_FILENAME( fn.GetPath(), fullName ) ) );
    m_cache->Save( footprint );
}


void PCB_IO_KICAD_SEXPR::FootprintDelete( const wxString& aLibraryPath,
                                          const wxString& aFootprintName,
                                          const std::map<std::string, UTF8>* aProperties )
{
    init( aProperties );

    validateCache( aLibraryPath );

    if( !m_cache->IsWritable() )
    {
        THROW_IO_ERROR( wxString::Format( _( "Library '%s' is read only." ),
                                          aLibraryPath.GetData() ) );
    }

    m_cache->Remove( aFootprintName );
}



long long PCB_IO_KICAD_SEXPR::GetLibraryTimestamp( const wxString& aLibraryPath ) const
{
    return FP_CACHE::GetTimestamp( aLibraryPath );
}


void PCB_IO_KICAD_SEXPR::CreateLibrary( const wxString& aLibraryPath,
                                        const std::map<std::string, UTF8>* aProperties )
{
    if( wxDir::Exists( aLibraryPath ) )
    {
        THROW_IO_ERROR( wxString::Format( _( "Cannot overwrite library path '%s'." ),
                                          aLibraryPath.GetData() ) );
    }

    init( aProperties );

    delete m_cache;
    m_cache = new FP_CACHE( this, aLibraryPath );
    m_cache->Save();
}


bool PCB_IO_KICAD_SEXPR::DeleteLibrary( const wxString& aLibraryPath,
                                        const std::map<std::string, UTF8>* aProperties )
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

            if( tmp.GetExt() != FILEEXT::KiCadFootprintFileExtension )
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


bool PCB_IO_KICAD_SEXPR::IsLibraryWritable( const wxString& aLibraryPath )
{
    init( nullptr );

    validateCache( aLibraryPath );

    return m_cache->IsWritable();
}
