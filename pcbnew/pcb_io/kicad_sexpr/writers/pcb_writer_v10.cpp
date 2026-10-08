/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

// Board writer for the KiCad 10.0 file format, extracted from the 10.0.0 release. It can only
// emit what 10.0 knew, so its output always opens there. Do not add new-format features here.

#include <pcb_io/kicad_sexpr/writers/pcb_writer_v10.h>

#include <wx/ffile.h>
#include <wx/mstream.h>
#include <board.h>
#include <board_design_settings.h>
#include <board_stackup_manager/board_stackup.h>
#include <board_stackup_manager/stackup_predefined_prms.h>
#include <callback_gal.h>
#include <component_classes/component_class.h>
#include <convert_basic_shapes_to_polygon.h>
#include <fmt/format.h>
#include <footprint.h>
#include <io/kicad/kicad_io_utils.h>
#include <io/kicad/legacy_format.h>
#include <io/kicad/legacy_pcb_plot_format.h>
#include <io/kicad/legacy_tuning_pattern.h>
#include <layer_range.h>
#include <macros.h>
#include <pad.h>
#include <pcb_dimension.h>
#include <pcb_generator.h>
#include <pcb_group.h>
#include <pcb_io/kicad_sexpr/pcb_io_kicad_sexpr.h>
#include <pcb_point.h>
#include <pcb_reference_image.h>
#include <pcb_barcode.h>
#include <pcb_shape.h>
#include <pcb_table.h>
#include <pcb_tablecell.h>
#include <pcb_target.h>
#include <pcb_text.h>
#include <pcb_textbox.h>
#include <pcb_track.h>
#include <richio.h>
#include <string_utils.h>
#include <zone.h>
#include <build_version.h>


static void formatBoardStackupV10( OUTPUTFORMATTER* aFormatter, const BOARD* aBoard )
{
    const BOARD_STACKUP& stackup = aBoard->GetDesignSettings().GetStackupDescriptor();

    // Board stackup is the ordered list from top to bottom of
    // physical layers and substrate used to build the board.
    if( stackup.GetList().empty() )
        return;

    aFormatter->Print( "(stackup" );

    // Note:
    // Unspecified parameters are not stored in file.
    for( BOARD_STACKUP_ITEM* item : stackup.GetList() )
    {
        wxString layer_name;

        if( item->GetBrdLayerId() == UNDEFINED_LAYER )
            layer_name.Printf( wxT( "dielectric %d" ), item->GetDielectricLayerId() );
        else
            layer_name = LSET::Name( item->GetBrdLayerId() );

        aFormatter->Print( "(layer %s (type %s)", aFormatter->Quotew( layer_name ).c_str(),
                           aFormatter->Quotew( item->GetTypeName() ).c_str() );

        // Output other parameters (in sub layer list there is at least one item)
        for( int idx = 0; idx < item->GetSublayersCount(); idx++ )
        {
            if( idx ) // not for the main (first) layer.
                aFormatter->Print( " addsublayer" );

            if( item->IsColorEditable() && IsPrmSpecified( item->GetColor( idx ) ) )
            {
                aFormatter->Print( "(color %s)", aFormatter->Quotew( item->GetColor( idx ) ).c_str() );
            }

            if( item->IsThicknessEditable() )
            {
                aFormatter->Print(
                        "(thickness %s",
                        EDA_UNIT_UTILS::FormatInternalUnits( pcbIUScale, item->GetThickness( idx ) ).c_str() );

                if( item->GetType() == BS_ITEM_TYPE_DIELECTRIC && item->IsThicknessLocked( idx ) )
                    aFormatter->Print( " locked" );

                aFormatter->Print( ")" );
            }

            if( item->HasMaterialValue( idx ) )
            {
                aFormatter->Print( "(material %s)", aFormatter->Quotew( item->GetMaterial( idx ) ).c_str() );
            }

            if( item->HasEpsilonRValue() && item->HasMaterialValue( idx ) )
                aFormatter->Print( "(epsilon_r %s)", FormatDouble2Str( item->GetEpsilonR( idx ) ).c_str() );

            if( item->HasLossTangentValue() && item->HasMaterialValue( idx ) )
            {
                aFormatter->Print( "(loss_tangent %s)", FormatDouble2Str( item->GetLossTangent( idx ) ).c_str() );
            }
        }

        aFormatter->Print( ")" );
    }

    // Other infos about board, related to layers and other fabrication specifications
    if( IsPrmSpecified( stackup.m_FinishType ) )
        aFormatter->Print( "(copper_finish %s)", aFormatter->Quotew( stackup.m_FinishType ).c_str() );

    KICAD_FORMAT::LEGACY::FormatBool( aFormatter, "dielectric_constraints", stackup.m_HasDielectricConstrains );

    if( stackup.m_EdgeConnectorConstraints > 0 )
    {
        aFormatter->Print( "(edge_connector %s)", stackup.m_EdgeConnectorConstraints > 1 ? "bevelled" : "yes" );
    }

    if( stackup.m_EdgePlating )
        KICAD_FORMAT::LEGACY::FormatBool( aFormatter, "edge_plating", true );

    aFormatter->Print( ")" );
}


void PCB_WRITER_V10::SaveBoard( const wxString& aFileName, BOARD* aBoard )
{
    PRETTIFIED_FILE_OUTPUTFORMATTER formatter( aFileName );
    FormatBoardToFormatter( &formatter, aBoard );
    formatter.Finish();
}


void PCB_WRITER_V10::FormatBoardToFormatter( OUTPUTFORMATTER* aOut, BOARD* aBoard )
{
    m_ctl = CTL_FOR_BOARD;
    m_board = aBoard;

    // If the user wants fonts embedded, make sure that they are added to the board. Otherwise,
    // remove any fonts that were previously embedded.
    if( m_board->GetAreFontsEmbedded() )
        m_board->EmbedFonts();
    else
        m_board->GetEmbeddedFiles()->ClearEmbeddedFonts();

    m_out = aOut;

    m_out->Print( "(kicad_pcb (version %d) (generator \"pcbnew\") (generator_version %s)",
                  PCB_WRITER_V10::FORMAT_VERSION, m_out->Quotew( GetMajorMinorVersion() ).c_str() );

    Format( aBoard );

    m_out->Print( ")" );

    m_out = nullptr;
}


void PCB_WRITER_V10::Format( const BOARD_ITEM* aItem ) const
{
    switch( aItem->Type() )
    {
    case PCB_T: format( static_cast<const BOARD*>( aItem ) ); break;

    case PCB_DIM_ALIGNED_T:
    case PCB_DIM_CENTER_T:
    case PCB_DIM_RADIAL_T:
    case PCB_DIM_ORTHOGONAL_T:
    case PCB_DIM_LEADER_T: format( static_cast<const PCB_DIMENSION_BASE*>( aItem ) ); break;

    case PCB_SHAPE_T: format( static_cast<const PCB_SHAPE*>( aItem ) ); break;

    case PCB_REFERENCE_IMAGE_T: format( static_cast<const PCB_REFERENCE_IMAGE*>( aItem ) ); break;

    case PCB_POINT_T: format( static_cast<const PCB_POINT*>( aItem ) ); break;

    case PCB_TARGET_T: format( static_cast<const PCB_TARGET*>( aItem ) ); break;

    case PCB_FOOTPRINT_T: format( static_cast<const FOOTPRINT*>( aItem ) ); break;

    case PCB_PAD_T: format( static_cast<const PAD*>( aItem ) ); break;

    case PCB_FIELD_T:
        // Handled in the footprint formatter when properties are formatted
        break;

    case PCB_TEXT_T: format( static_cast<const PCB_TEXT*>( aItem ) ); break;

    case PCB_TEXTBOX_T: format( static_cast<const PCB_TEXTBOX*>( aItem ) ); break;

    case PCB_BARCODE_T: format( static_cast<const PCB_BARCODE*>( aItem ) ); break;

    case PCB_TABLE_T: format( static_cast<const PCB_TABLE*>( aItem ) ); break;

    case PCB_GROUP_T: format( static_cast<const PCB_GROUP*>( aItem ) ); break;

    case PCB_GENERATOR_T: format( static_cast<const PCB_GENERATOR*>( aItem ) ); break;

    case PCB_TRACE_T:
    case PCB_ARC_T:
    case PCB_VIA_T: format( static_cast<const PCB_TRACK*>( aItem ) ); break;

    case PCB_ZONE_T: format( static_cast<const ZONE*>( aItem ) ); break;

    default: wxFAIL_MSG( wxT( "Cannot format item " ) + aItem->GetClass() );
    }
}


static std::string formatInternalUnits( const int aValue, const EDA_DATA_TYPE aDataType = EDA_DATA_TYPE::DISTANCE )
{
    return EDA_UNIT_UTILS::FormatInternalUnits( pcbIUScale, aValue, aDataType );
}


static std::string formatInternalUnits( const VECTOR2I& aCoord )
{
    return EDA_UNIT_UTILS::FormatInternalUnits( pcbIUScale, aCoord );
}


static std::string formatInternalUnits( const VECTOR2I& aCoord, const FOOTPRINT* aParentFP )
{
    if( aParentFP )
    {
        VECTOR2I coord = aCoord - aParentFP->GetPosition();
        RotatePoint( coord, -aParentFP->GetOrientation() );
        return formatInternalUnits( coord );
    }

    return formatInternalUnits( aCoord );
}


void PCB_WRITER_V10::formatLayer( PCB_LAYER_ID aLayer, bool aIsKnockout ) const
{
    m_out->Print( "(layer %s %s)", m_out->Quotew( LSET::Name( aLayer ) ).c_str(), aIsKnockout ? "knockout" : "" );
}


void PCB_WRITER_V10::formatPolyPts( const SHAPE_LINE_CHAIN& aOutline, const FOOTPRINT* aParentFP ) const
{
    m_out->Print( "(pts" );

    for( int ii = 0; ii < aOutline.PointCount(); ++ii )
    {
        int ind = aOutline.ArcIndex( ii );

        if( ind < 0 )
        {
            m_out->Print( "(xy %s)", formatInternalUnits( aOutline.CPoint( ii ), aParentFP ).c_str() );
        }
        else
        {
            const SHAPE_ARC& arc = aOutline.Arc( ind );
            m_out->Print( "(arc (start %s) (mid %s) (end %s))", formatInternalUnits( arc.GetP0(), aParentFP ).c_str(),
                          formatInternalUnits( arc.GetArcMid(), aParentFP ).c_str(),
                          formatInternalUnits( arc.GetP1(), aParentFP ).c_str() );

            do
            {
                ++ii;
            } while( ii < aOutline.PointCount() && aOutline.ArcIndex( ii ) == ind );

            --ii;
        }
    }

    m_out->Print( ")" );
}


void PCB_WRITER_V10::formatRenderCache( const EDA_TEXT* aText ) const
{
    wxString                                     resolvedText( aText->GetShownText( FOR_CANVAS ) );
    std::vector<std::unique_ptr<KIFONT::GLYPH>>* cache = aText->GetRenderCache( aText->GetFont(), resolvedText );

    m_out->Print( "(render_cache %s %s", m_out->Quotew( resolvedText ).c_str(),
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


void PCB_WRITER_V10::formatSetup( const BOARD* aBoard ) const
{
    // Setup
    m_out->Print( "(setup" );

    // Save the board physical stackup structure
    if( aBoard->GetDesignSettings().m_HasStackup )
        formatBoardStackupV10( m_out, aBoard );

    BOARD_DESIGN_SETTINGS& dsnSettings = aBoard->GetDesignSettings();

    m_out->Print( "(pad_to_mask_clearance %s)", formatInternalUnits( dsnSettings.m_SolderMaskExpansion ).c_str() );

    if( dsnSettings.m_SolderMaskMinWidth )
    {
        m_out->Print( "(solder_mask_min_width %s)", formatInternalUnits( dsnSettings.m_SolderMaskMinWidth ).c_str() );
    }

    if( dsnSettings.m_SolderPasteMargin != 0 )
    {
        m_out->Print( "(pad_to_paste_clearance %s)", formatInternalUnits( dsnSettings.m_SolderPasteMargin ).c_str() );
    }

    if( dsnSettings.m_SolderPasteMarginRatio != 0 )
    {
        m_out->Print( "(pad_to_paste_clearance_ratio %s)",
                      FormatDouble2Str( dsnSettings.m_SolderPasteMarginRatio ).c_str() );
    }

    KICAD_FORMAT::LEGACY::FormatBool( m_out, "allow_soldermask_bridges_in_footprints",
                                      dsnSettings.m_AllowSoldermaskBridgesInFPs );

    m_out->Print( 0, " (tenting " );
    KICAD_FORMAT::LEGACY::FormatBool( m_out, "front", dsnSettings.m_TentViasFront );
    KICAD_FORMAT::LEGACY::FormatBool( m_out, "back", dsnSettings.m_TentViasBack );
    m_out->Print( 0, ")" );

    m_out->Print( 0, " (covering " );
    KICAD_FORMAT::LEGACY::FormatBool( m_out, "front", dsnSettings.m_CoverViasFront );
    KICAD_FORMAT::LEGACY::FormatBool( m_out, "back", dsnSettings.m_CoverViasBack );
    m_out->Print( 0, ")" );

    m_out->Print( 0, " (plugging " );
    KICAD_FORMAT::LEGACY::FormatBool( m_out, "front", dsnSettings.m_PlugViasFront );
    KICAD_FORMAT::LEGACY::FormatBool( m_out, "back", dsnSettings.m_PlugViasBack );
    m_out->Print( 0, ")" );

    KICAD_FORMAT::LEGACY::FormatBool( m_out, "capping", dsnSettings.m_CapVias );

    KICAD_FORMAT::LEGACY::FormatBool( m_out, "filling", dsnSettings.m_FillVias );

    if( !dsnSettings.m_ZoneLayerProperties.empty() )
    {
        m_out->Print( 0, " (zone_defaults" );

        for( const auto& [layer, properties] : dsnSettings.m_ZoneLayerProperties )
            format( properties, 0, layer );

        m_out->Print( 0, ")\n" );
    }

    VECTOR2I origin = dsnSettings.GetAuxOrigin();

    if( origin != VECTOR2I( 0, 0 ) )
    {
        m_out->Print( "(aux_axis_origin %s %s)", formatInternalUnits( origin.x ).c_str(),
                      formatInternalUnits( origin.y ).c_str() );
    }

    origin = dsnSettings.GetGridOrigin();

    if( origin != VECTOR2I( 0, 0 ) )
    {
        m_out->Print( "(grid_origin %s %s)", formatInternalUnits( origin.x ).c_str(),
                      formatInternalUnits( origin.y ).c_str() );
    }

    KICAD_FORMAT::LEGACY::FormatPlotV10( m_out, aBoard->GetPlotOptions() );

    m_out->Print( ")" );
}


void PCB_WRITER_V10::formatGeneral( const BOARD* aBoard ) const
{
    const BOARD_DESIGN_SETTINGS& dsnSettings = aBoard->GetDesignSettings();

    m_out->Print( "(general" );

    m_out->Print( "(thickness %s)", formatInternalUnits( dsnSettings.GetBoardThickness() ).c_str() );

    KICAD_FORMAT::LEGACY::FormatBool( m_out, "legacy_teardrops", aBoard->LegacyTeardrops() );

    m_out->Print( ")" );

    KICAD_FORMAT::LEGACY::FormatPageV10( m_out, aBoard->GetPageSettings() );
    KICAD_FORMAT::LEGACY::FormatTitle( m_out, aBoard->GetTitleBlock() );
}


void PCB_WRITER_V10::formatBoardLayers( const BOARD* aBoard ) const
{
    m_out->Print( "(layers" );

    // Save only the used copper layers from front to back.

    for( PCB_LAYER_ID layer : aBoard->GetEnabledLayers().CuStack() )
    {
        m_out->Print( "(%d %s %s %s)", layer, m_out->Quotew( LSET::Name( layer ) ).c_str(),
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

            if( aBoard->GetLayerType( layer ) == LT_FRONT || aBoard->GetLayerType( layer ) == LT_BACK )
                print_type = true;
        }

        m_out->Print( "(%d %s %s %s)", layer, m_out->Quotew( LSET::Name( layer ) ).c_str(),
                      print_type ? LAYER::ShowType( aBoard->GetLayerType( layer ) ) : "user",
                      m_board->GetLayerName( layer ) == LSET::Name( layer )
                              ? ""
                              : m_out->Quotew( m_board->GetLayerName( layer ) ).c_str() );
    }

    m_out->Print( ")" );
}


void PCB_WRITER_V10::formatProperties( const BOARD* aBoard ) const
{
    for( const std::pair<const wxString, wxString>& prop : aBoard->GetProperties() )
    {
        m_out->Print( "(property %s %s)", m_out->Quotew( prop.first ).c_str(), m_out->Quotew( prop.second ).c_str() );
    }
}


void PCB_WRITER_V10::formatVariants( const BOARD* aBoard ) const
{
    const std::vector<wxString>& variantNames = aBoard->GetVariantNames();

    if( variantNames.empty() )
        return;

    m_out->Print( "(variants" );

    for( const wxString& variantName : variantNames )
    {
        m_out->Print( "(variant (name %s)", m_out->Quotew( variantName ).c_str() );

        wxString description = aBoard->GetVariantDescription( variantName );

        if( !description.IsEmpty() )
            m_out->Print( "(description %s)", m_out->Quotew( description ).c_str() );

        m_out->Print( ")" );
    }

    m_out->Print( ")" );
}


void PCB_WRITER_V10::formatHeader( const BOARD* aBoard ) const
{
    formatGeneral( aBoard );

    // Layers list.
    formatBoardLayers( aBoard );

    // Setup
    formatSetup( aBoard );

    // Properties
    formatProperties( aBoard );

    // Variants
    formatVariants( aBoard );
}


static bool isDefaultTeardropParameters( const TEARDROP_PARAMETERS& tdParams )
{
    static const TEARDROP_PARAMETERS defaults;

    return tdParams.m_Enabled == defaults.m_Enabled && tdParams.m_BestLengthRatio == defaults.m_BestLengthRatio
           && tdParams.m_TdMaxLen == defaults.m_TdMaxLen && tdParams.m_BestWidthRatio == defaults.m_BestWidthRatio
           && tdParams.m_TdMaxWidth == defaults.m_TdMaxWidth && tdParams.m_CurvedEdges == defaults.m_CurvedEdges
           && tdParams.m_WidthtoSizeFilterRatio == defaults.m_WidthtoSizeFilterRatio
           && tdParams.m_AllowUseTwoTracks == defaults.m_AllowUseTwoTracks
           && tdParams.m_TdOnPadsInZones == defaults.m_TdOnPadsInZones;
}


void PCB_WRITER_V10::formatTeardropParameters( const TEARDROP_PARAMETERS& tdParams ) const
{
    m_out->Print( "(teardrops (best_length_ratio %s) (max_length %s) (best_width_ratio %s) "
                  "(max_width %s)",
                  FormatDouble2Str( tdParams.m_BestLengthRatio ).c_str(),
                  formatInternalUnits( tdParams.m_TdMaxLen ).c_str(),
                  FormatDouble2Str( tdParams.m_BestWidthRatio ).c_str(),
                  formatInternalUnits( tdParams.m_TdMaxWidth ).c_str() );

    KICAD_FORMAT::LEGACY::FormatBool( m_out, "curved_edges", tdParams.m_CurvedEdges );

    m_out->Print( "(filter_ratio %s)", FormatDouble2Str( tdParams.m_WidthtoSizeFilterRatio ).c_str() );

    KICAD_FORMAT::LEGACY::FormatBool( m_out, "enabled", tdParams.m_Enabled );
    KICAD_FORMAT::LEGACY::FormatBool( m_out, "allow_two_segments", tdParams.m_AllowUseTwoTracks );
    KICAD_FORMAT::LEGACY::FormatBool( m_out, "prefer_zone_connections", !tdParams.m_TdOnPadsInZones );
    m_out->Print( ")" );
}


void PCB_WRITER_V10::format( const BOARD* aBoard ) const
{
    std::set<BOARD_ITEM*, BOARD_ITEM::ptr_cmp>  sorted_footprints( aBoard->Footprints().begin(),
                                                                   aBoard->Footprints().end() );
    std::set<BOARD_ITEM*, BOARD::cmp_drawings>  sorted_drawings( aBoard->Drawings().begin(), aBoard->Drawings().end() );
    std::set<PCB_TRACK*, PCB_TRACK::cmp_tracks> sorted_tracks( aBoard->Tracks().begin(), aBoard->Tracks().end() );
    std::set<PCB_POINT*, PCB_POINT::cmp_points> sorted_points( aBoard->Points().begin(), aBoard->Points().end() );
    std::set<BOARD_ITEM*, BOARD_ITEM::ptr_cmp>  sorted_zones( aBoard->Zones().begin(), aBoard->Zones().end() );
    std::set<BOARD_ITEM*, BOARD_ITEM::ptr_cmp>  sorted_groups( aBoard->Groups().begin(), aBoard->Groups().end() );
    std::set<BOARD_ITEM*, BOARD_ITEM::ptr_cmp>  sorted_generators( aBoard->Generators().begin(),
                                                                   aBoard->Generators().end() );
    formatHeader( aBoard );

    // Save the footprints.
    for( BOARD_ITEM* footprint : sorted_footprints )
        Format( footprint );

    // Save the graphical items on the board (not owned by a footprint)
    for( BOARD_ITEM* item : sorted_drawings )
        Format( item );

    // Save the points
    for( PCB_POINT* point : sorted_points )
        Format( point );

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

    m_out->Print( "(embedded_fonts %s)", aBoard->GetEmbeddedFiles()->GetAreFontsEmbedded() ? "yes" : "no" );

    if( !files_to_write.IsEmpty() )
        KICAD_FORMAT::LEGACY::FormatEmbeddedFilesV10( *m_out, files_to_write, ( m_ctl & CTL_FOR_BOARD ) );

    // Remove the files so that they are not freed in the DTOR
    files_to_write.ClearEmbeddedFiles( false );
}


void PCB_WRITER_V10::format( const PCB_DIMENSION_BASE* aDimension ) const
{
    const PCB_DIM_ALIGNED*    aligned = dynamic_cast<const PCB_DIM_ALIGNED*>( aDimension );
    const PCB_DIM_ORTHOGONAL* ortho = dynamic_cast<const PCB_DIM_ORTHOGONAL*>( aDimension );
    const PCB_DIM_CENTER*     center = dynamic_cast<const PCB_DIM_CENTER*>( aDimension );
    const PCB_DIM_RADIAL*     radial = dynamic_cast<const PCB_DIM_RADIAL*>( aDimension );
    const PCB_DIM_LEADER*     leader = dynamic_cast<const PCB_DIM_LEADER*>( aDimension );

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
        KICAD_FORMAT::LEGACY::FormatBool( m_out, "locked", aDimension->IsLocked() );

    formatLayer( aDimension->GetLayer() );

    KICAD_FORMAT::LEGACY::FormatUuid( m_out, aDimension->m_Uuid );

    m_out->Print( "(pts (xy %s %s) (xy %s %s))", formatInternalUnits( aDimension->GetStart().x ).c_str(),
                  formatInternalUnits( aDimension->GetStart().y ).c_str(),
                  formatInternalUnits( aDimension->GetEnd().x ).c_str(),
                  formatInternalUnits( aDimension->GetEnd().y ).c_str() );

    if( aligned )
        m_out->Print( "(height %s)", formatInternalUnits( aligned->GetHeight() ).c_str() );

    if( radial )
    {
        m_out->Print( "(leader_length %s)", formatInternalUnits( radial->GetLeaderLength() ).c_str() );
    }

    if( ortho )
        m_out->Print( "(orientation %d)", static_cast<int>( ortho->GetOrientation() ) );

    if( !center )
    {
        m_out->Print( "(format (prefix %s) (suffix %s) (units %d) (units_format %d) (precision %d)",
                      m_out->Quotew( aDimension->GetPrefix() ).c_str(),
                      m_out->Quotew( aDimension->GetSuffix() ).c_str(), static_cast<int>( aDimension->GetUnitsMode() ),
                      static_cast<int>( aDimension->GetUnitsFormat() ),
                      static_cast<int>( aDimension->GetPrecision() ) );

        if( aDimension->GetOverrideTextEnabled() )
        {
            m_out->Print( "(override_value %s)", m_out->Quotew( aDimension->GetOverrideText() ).c_str() );
        }

        if( aDimension->GetSuppressZeroes() )
            KICAD_FORMAT::LEGACY::FormatBool( m_out, "suppress_zeroes", true );

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
        case DIM_ARROW_DIRECTION::OUTWARD: m_out->Print( "(arrow_direction outward)" ); break;
        case DIM_ARROW_DIRECTION::INWARD:
            m_out->Print( "(arrow_direction inward)" );
            break;
            // No default, handle all cases
        }
    }

    if( aligned )
    {
        m_out->Print( "(extension_height %s)", formatInternalUnits( aligned->GetExtensionHeight() ).c_str() );
    }

    if( leader )
        m_out->Print( "(text_frame %d)", static_cast<int>( leader->GetTextBorder() ) );

    m_out->Print( "(extension_offset %s)", formatInternalUnits( aDimension->GetExtensionOffset() ).c_str() );

    if( aDimension->GetKeepTextAligned() )
        KICAD_FORMAT::LEGACY::FormatBool( m_out, "keep_text_aligned", true );

    m_out->Print( ")" );

    // Write dimension text after all other options to be sure the
    // text options are known when reading the file
    if( !center )
        format( static_cast<const PCB_TEXT*>( aDimension ) );

    m_out->Print( ")" );
}


void PCB_WRITER_V10::format( const PCB_SHAPE* aShape ) const
{
    FOOTPRINT*  parentFP = aShape->GetParentFootprint();
    std::string prefix = parentFP ? "fp" : "gr";

    switch( aShape->GetShape() )
    {
    case SHAPE_T::SEGMENT:
        m_out->Print( "(%s_line (start %s) (end %s)", prefix.c_str(),
                      formatInternalUnits( aShape->GetStart(), parentFP ).c_str(),
                      formatInternalUnits( aShape->GetEnd(), parentFP ).c_str() );
        break;

    case SHAPE_T::RECTANGLE:
        m_out->Print( "(%s_rect (start %s) (end %s)", prefix.c_str(),
                      formatInternalUnits( aShape->GetStart(), parentFP ).c_str(),
                      formatInternalUnits( aShape->GetEnd(), parentFP ).c_str() );

        if( aShape->GetCornerRadius() > 0 )
            m_out->Print( " (radius %s)", formatInternalUnits( aShape->GetCornerRadius() ).c_str() );
        break;

    case SHAPE_T::CIRCLE:
        m_out->Print( "(%s_circle (center %s) (end %s)", prefix.c_str(),
                      formatInternalUnits( aShape->GetStart(), parentFP ).c_str(),
                      formatInternalUnits( aShape->GetEnd(), parentFP ).c_str() );
        break;

    case SHAPE_T::ARC:
        m_out->Print( "(%s_arc (start %s) (mid %s) (end %s)", prefix.c_str(),
                      formatInternalUnits( aShape->GetStart(), parentFP ).c_str(),
                      formatInternalUnits( aShape->GetArcMid(), parentFP ).c_str(),
                      formatInternalUnits( aShape->GetEnd(), parentFP ).c_str() );
        break;

    case SHAPE_T::POLY:
        if( aShape->IsPolyShapeValid() )
        {
            const SHAPE_POLY_SET&   poly = aShape->GetPolyShape();
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
        m_out->Print( "(%s_curve (pts (xy %s) (xy %s) (xy %s) (xy %s))", prefix.c_str(),
                      formatInternalUnits( aShape->GetStart(), parentFP ).c_str(),
                      formatInternalUnits( aShape->GetBezierC1(), parentFP ).c_str(),
                      formatInternalUnits( aShape->GetBezierC2(), parentFP ).c_str(),
                      formatInternalUnits( aShape->GetEnd(), parentFP ).c_str() );
        break;

    default: UNIMPLEMENTED_FOR( aShape->SHAPE_T_asString() ); return;
    };

    KICAD_FORMAT::LEGACY::FormatStroke( m_out, aShape->GetStroke(), pcbIUScale );

    // The filled flag represents if a solid fill is present on circles, rectangles and polygons
    if( ( aShape->GetShape() == SHAPE_T::POLY ) || ( aShape->GetShape() == SHAPE_T::RECTANGLE )
        || ( aShape->GetShape() == SHAPE_T::CIRCLE ) )
    {
        switch( aShape->GetFillMode() )
        {
        case FILL_T::HATCH: m_out->Print( "(fill hatch)" ); break;

        case FILL_T::REVERSE_HATCH: m_out->Print( "(fill reverse_hatch)" ); break;

        case FILL_T::CROSS_HATCH: m_out->Print( "(fill cross_hatch)" ); break;

        case FILL_T::FILLED_SHAPE: KICAD_FORMAT::LEGACY::FormatBool( m_out, "fill", true ); break;

        default: KICAD_FORMAT::LEGACY::FormatBool( m_out, "fill", false ); break;
        }
    }

    if( aShape->IsLocked() )
        KICAD_FORMAT::LEGACY::FormatBool( m_out, "locked", true );

    if( aShape->GetLayerSet().count() > 1 )
        formatLayers( aShape->GetLayerSet(), false /* enumerate layers */ );
    else
        formatLayer( aShape->GetLayer() );

    if( aShape->HasSolderMask() && aShape->GetLocalSolderMaskMargin().has_value()
        && IsExternalCopperLayer( aShape->GetLayer() ) )
    {
        m_out->Print( "(solder_mask_margin %s)",
                      formatInternalUnits( aShape->GetLocalSolderMaskMargin().value() ).c_str() );
    }

    if( aShape->GetNetCode() > 0 )
        m_out->Print( "(net %s)", m_out->Quotew( aShape->GetNetname() ).c_str() );

    KICAD_FORMAT::LEGACY::FormatUuid( m_out, aShape->m_Uuid );
    m_out->Print( ")" );
}


void PCB_WRITER_V10::format( const PCB_REFERENCE_IMAGE* aBitmap ) const
{
    wxCHECK_RET( aBitmap != nullptr && m_out != nullptr, "" );

    const REFERENCE_IMAGE& refImage = aBitmap->GetReferenceImage();

    const wxImage* image = refImage.GetImage().GetImageData();

    wxCHECK_RET( image != nullptr, "wxImage* is NULL" );

    m_out->Print( "(image (at %s %s)", formatInternalUnits( aBitmap->GetPosition().x ).c_str(),
                  formatInternalUnits( aBitmap->GetPosition().y ).c_str() );

    formatLayer( aBitmap->GetLayer() );

    if( refImage.GetImageScale() != 1.0 )
        m_out->Print( "%s", fmt::format( "(scale {:g})", refImage.GetImageScale() ).c_str() );

    if( aBitmap->IsLocked() )
        KICAD_FORMAT::LEGACY::FormatBool( m_out, "locked", true );

    wxMemoryOutputStream ostream;
    refImage.GetImage().SaveImageData( ostream );

    KICAD_FORMAT::LEGACY::FormatStreamData( *m_out, *ostream.GetOutputStreamBuffer() );

    KICAD_FORMAT::LEGACY::FormatUuid( m_out, aBitmap->m_Uuid );
    m_out->Print( ")" ); // Closes image token.
}


void PCB_WRITER_V10::format( const PCB_POINT* aPoint ) const
{
    m_out->Print( "(point (at %s) (size %s)", formatInternalUnits( aPoint->GetPosition() ).c_str(),
                  formatInternalUnits( aPoint->GetSize() ).c_str() );

    formatLayer( aPoint->GetLayer() );

    KICAD_FORMAT::LEGACY::FormatUuid( m_out, aPoint->m_Uuid );
    m_out->Print( ")" );
}


void PCB_WRITER_V10::format( const PCB_TARGET* aTarget ) const
{
    m_out->Print( "(target %s (at %s) (size %s)", ( aTarget->GetShape() ) ? "x" : "plus",
                  formatInternalUnits( aTarget->GetPosition() ).c_str(),
                  formatInternalUnits( aTarget->GetSize() ).c_str() );

    if( aTarget->GetWidth() != 0 )
        m_out->Print( "(width %s)", formatInternalUnits( aTarget->GetWidth() ).c_str() );

    formatLayer( aTarget->GetLayer() );
    KICAD_FORMAT::LEGACY::FormatUuid( m_out, aTarget->m_Uuid );
    m_out->Print( ")" );
}


void PCB_WRITER_V10::format( const FOOTPRINT* aFootprint ) const
{
    if( !( m_ctl & CTL_OMIT_INITIAL_COMMENTS ) )
    {
        const wxArrayString* initial_comments = aFootprint->GetInitialComments();

        if( initial_comments )
        {
            for( unsigned i = 0; i < initial_comments->GetCount(); ++i )
                m_out->Print( "%s\n", TO_UTF8( ( *initial_comments )[i] ) );
        }
    }

    if( m_ctl & CTL_OMIT_LIBNAME )
    {
        m_out->Print( "(footprint %s", m_out->Quotes( aFootprint->GetFPID().GetLibItemName() ).c_str() );
    }
    else
    {
        m_out->Print( "(footprint %s",
                      m_out->Quotes( KICAD_FORMAT::LEGACY::FormatLibId( aFootprint->GetFPID() ) ).c_str() );
    }

    if( !( m_ctl & CTL_OMIT_FOOTPRINT_VERSION ) )
    {
        m_out->Print( "(version %d) (generator \"pcbnew\") (generator_version %s)", PCB_WRITER_V10::FORMAT_VERSION,
                      m_out->Quotew( GetMajorMinorVersion() ).c_str() );
    }

    if( aFootprint->IsLocked() )
        KICAD_FORMAT::LEGACY::FormatBool( m_out, "locked", true );

    if( aFootprint->IsPlaced() )
        KICAD_FORMAT::LEGACY::FormatBool( m_out, "placed", true );

    formatLayer( aFootprint->GetLayer() );

    if( !( m_ctl & CTL_OMIT_UUIDS ) )
        KICAD_FORMAT::LEGACY::FormatUuid( m_out, aFootprint->m_Uuid );

    if( !( m_ctl & CTL_OMIT_AT ) )
    {
        m_out->Print( "(at %s %s)", formatInternalUnits( aFootprint->GetPosition() ).c_str(),
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
        if( !field )
            continue;

        m_out->Print( "(property %s %s", m_out->Quotew( field->GetUntranslatedName() ).c_str(),
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
        m_out->Print( "(property ki_fp_filters %s)", m_out->Quotew( aFootprint->GetFilters() ).c_str() );
    }

    if( !( m_ctl & CTL_OMIT_PATH ) && !aFootprint->GetPath().empty() )
        m_out->Print( "(path %s)", m_out->Quotew( aFootprint->GetPath().AsString() ).c_str() );

    if( !aFootprint->GetSheetname().empty() )
        m_out->Print( "(sheetname %s)", m_out->Quotew( aFootprint->GetSheetname() ).c_str() );

    if( !aFootprint->GetSheetfile().empty() )
        m_out->Print( "(sheetfile %s)", m_out->Quotew( aFootprint->GetSheetfile() ).c_str() );

    // Emit unit info for gate swapping metadata (flat pin list form)
    if( !aFootprint->GetUnitInfo().empty() )
    {
        m_out->Print( "(units" );

        for( const FOOTPRINT::FP_UNIT_INFO& u : aFootprint->GetUnitInfo() )
        {
            m_out->Print( "(unit (name %s)", m_out->Quotew( u.m_unitName ).c_str() );
            m_out->Print( "(pins" );

            for( const wxString& n : u.m_pins )
                m_out->Print( " %s", m_out->Quotew( n ).c_str() );

            m_out->Print( ")" ); // </pins>
            m_out->Print( ")" ); // </unit>
        }

        m_out->Print( ")" ); // </units>
    }

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
        m_out->Print( "(clearance %s)", formatInternalUnits( aFootprint->GetLocalClearance().value() ).c_str() );
    }

    if( aFootprint->GetLocalZoneConnection() != ZONE_CONNECTION::INHERITED )
    {
        m_out->Print( "(zone_connect %d)", static_cast<int>( aFootprint->GetLocalZoneConnection() ) );
    }

    // Attributes
    if( aFootprint->GetAttributes() || aFootprint->AllowMissingCourtyard() || aFootprint->AllowSolderMaskBridges() )
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

    // Expand inner layers is the default stackup mode
    if( aFootprint->GetStackupMode() != FOOTPRINT_STACKUP::EXPAND_INNER_LAYERS )
    {
        m_out->Print( "(stackup" );

        const LSET& fpLset = aFootprint->GetStackupLayers();
        for( PCB_LAYER_ID layer : fpLset.Seq() )
        {
            wxString canonicalName( LSET::Name( layer ) );
            m_out->Print( "(layer %s)", m_out->Quotew( canonicalName ).c_str() );
        }

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

    KICAD_FORMAT::LEGACY::FormatBool( m_out, "duplicate_pad_numbers_are_jumpers",
                                      aFootprint->GetDuplicatePadNumbersAreJumpers() );

    const JUMPER_GROUP_SET& jumperGroups = aFootprint->JumperPadGroups();

    if( !jumperGroups.IsEmpty() )
    {
        m_out->Print( "(jumper_pad_groups" );

        for( const JUMPER_GROUP& group : jumperGroups.GetAll() )
        {
            m_out->Print( "(" );

            for( const wxString& padName : group.GetNames() )
                m_out->Print( "%s ", m_out->Quotew( padName ).c_str() );

            m_out->Print( ")" );
        }

        m_out->Print( ")" );
    }

    Format( &aFootprint->Reference() );
    Format( &aFootprint->Value() );

    std::set<PAD*, FOOTPRINT::cmp_pads>            sorted_pads( aFootprint->Pads().begin(), aFootprint->Pads().end() );
    std::set<BOARD_ITEM*, FOOTPRINT::cmp_drawings> sorted_drawings( aFootprint->GraphicalItems().begin(),
                                                                    aFootprint->GraphicalItems().end() );
    std::set<PCB_POINT*, PCB_POINT::cmp_points>    sorted_points( aFootprint->Points().begin(),
                                                                  aFootprint->Points().end() );
    std::set<ZONE*, FOOTPRINT::cmp_zones>     sorted_zones( aFootprint->Zones().begin(), aFootprint->Zones().end() );
    std::set<BOARD_ITEM*, PCB_GROUP::ptr_cmp> sorted_groups( aFootprint->Groups().begin(), aFootprint->Groups().end() );

    // Save drawing elements.

    for( BOARD_ITEM* gr : sorted_drawings )
        Format( gr );

    for( PCB_POINT* point : sorted_points )
        Format( point );

    // Save pads.
    for( PAD* pad : sorted_pads )
        Format( pad );

    // Save zones.
    for( BOARD_ITEM* zone : sorted_zones )
        Format( zone );

    // Save groups.
    for( BOARD_ITEM* group : sorted_groups )
        Format( group );

    // Save variants.
    const bool baseDnp = aFootprint->IsDNP();
    const bool baseExcludedFromBOM = aFootprint->IsExcludedFromBOM();
    const bool baseExcludedFromPosFiles = aFootprint->IsExcludedFromPosFiles();

    for( const auto& [variantName, variant] : aFootprint->GetVariants() )
    {
        m_out->Print( "(variant (name %s)", m_out->Quotew( variantName ).c_str() );

        if( variant.GetDNP() != baseDnp )
            KICAD_FORMAT::LEGACY::FormatBool( m_out, "dnp", variant.GetDNP() );

        if( variant.GetExcludedFromBOM() != baseExcludedFromBOM )
            KICAD_FORMAT::LEGACY::FormatBool( m_out, "exclude_from_bom", variant.GetExcludedFromBOM() );

        if( variant.GetExcludedFromPosFiles() != baseExcludedFromPosFiles )
        {
            KICAD_FORMAT::LEGACY::FormatBool( m_out, "exclude_from_pos_files", variant.GetExcludedFromPosFiles() );
        }

        for( const auto& [fieldName, fieldValue] : variant.GetFields() )
        {
            const PCB_FIELD* baseField = aFootprint->GetField( fieldName );
            const wxString   baseValue = baseField ? baseField->GetText() : wxString();

            if( fieldValue == baseValue )
                continue;

            m_out->Print( "(field (name %s) (value %s))", m_out->Quotew( fieldName ).c_str(),
                          m_out->Quotew( fieldValue ).c_str() );
        }

        m_out->Print( ")" );
    }

    KICAD_FORMAT::LEGACY::FormatBool( m_out, "embedded_fonts", aFootprint->GetEmbeddedFiles()->GetAreFontsEmbedded() );

    if( !aFootprint->GetEmbeddedFiles()->IsEmpty() )
        KICAD_FORMAT::LEGACY::FormatEmbeddedFilesV10( *m_out, *aFootprint, !( m_ctl & CTL_FOR_BOARD ) );

    // Save 3D info.
    auto bs3D = aFootprint->Models().begin();
    auto es3D = aFootprint->Models().end();

    while( bs3D != es3D )
    {
        if( !bs3D->m_Filename.IsEmpty() )
        {
            m_out->Print( "(model %s", m_out->Quotew( bs3D->m_Filename ).c_str() );

            if( !bs3D->m_Show )
                KICAD_FORMAT::LEGACY::FormatBool( m_out, "hide", !bs3D->m_Show );

            if( bs3D->m_Opacity != 1.0 )
                m_out->Print( "%s", fmt::format( "(opacity {:.4f})", bs3D->m_Opacity ).c_str() );

            m_out->Print( "(offset (xyz %s %s %s))", FormatDouble2Str( bs3D->m_Offset.x ).c_str(),
                          FormatDouble2Str( bs3D->m_Offset.y ).c_str(), FormatDouble2Str( bs3D->m_Offset.z ).c_str() );

            m_out->Print( "(scale (xyz %s %s %s))", FormatDouble2Str( bs3D->m_Scale.x ).c_str(),
                          FormatDouble2Str( bs3D->m_Scale.y ).c_str(), FormatDouble2Str( bs3D->m_Scale.z ).c_str() );

            m_out->Print( "(rotate (xyz %s %s %s))", FormatDouble2Str( bs3D->m_Rotation.x ).c_str(),
                          FormatDouble2Str( bs3D->m_Rotation.y ).c_str(),
                          FormatDouble2Str( bs3D->m_Rotation.z ).c_str() );

            m_out->Print( ")" );
        }

        ++bs3D;
    }

    m_out->Print( ")" );
}


void PCB_WRITER_V10::formatLayers( LSET aLayerMask, bool aEnumerateLayers, bool aIsZone ) const
{
    static const LSET cu_all( LSET::AllCuMask() );
    static const LSET fr_bk( { B_Cu, F_Cu } );
    static const LSET adhes( { B_Adhes, F_Adhes } );
    static const LSET paste( { B_Paste, F_Paste } );
    static const LSET silks( { B_SilkS, F_SilkS } );
    static const LSET mask( { B_Mask, F_Mask } );
    static const LSET crt_yd( { B_CrtYd, F_CrtYd } );
    static const LSET fab( { B_Fab, F_Fab } );

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
            if( aIsZone )
                output += ' ' + m_out->Quotew( "F&B.Cu" );
            else
                output += ' ' + m_out->Quotew( "*.Cu" );

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


void PCB_WRITER_V10::format( const PAD* aPad ) const
{
    const BOARD* board = aPad->GetBoard();

    auto shapeName = [&]( PCB_LAYER_ID aLayer )
    {
        switch( aPad->GetShape( aLayer ) )
        {
        case PAD_SHAPE::CIRCLE: return "circle";
        case PAD_SHAPE::RECTANGLE: return "rect";
        case PAD_SHAPE::OVAL: return "oval";
        case PAD_SHAPE::TRAPEZOID: return "trapezoid";
        case PAD_SHAPE::CHAMFERED_RECT:
        case PAD_SHAPE::ROUNDRECT: return "roundrect";
        case PAD_SHAPE::CUSTOM: return "custom";

        default: THROW_IO_ERROR( wxString::Format( _( "unknown pad type: %d" ), aPad->GetShape( aLayer ) ) );
        }
    };

    const char* type;

    switch( aPad->GetAttribute() )
    {
    case PAD_ATTRIB::PTH: type = "thru_hole"; break;
    case PAD_ATTRIB::SMD: type = "smd"; break;
    case PAD_ATTRIB::CONN: type = "connect"; break;
    case PAD_ATTRIB::NPTH: type = "np_thru_hole"; break;

    default: THROW_IO_ERROR( wxString::Format( wxT( "unknown pad attribute: %d" ), aPad->GetAttribute() ) );
    }

    const char* property = nullptr;

    switch( aPad->GetProperty() )
    {
    case PAD_PROP::NONE: break; // could be "none"
    case PAD_PROP::BGA: property = "pad_prop_bga"; break;
    case PAD_PROP::FIDUCIAL_GLBL: property = "pad_prop_fiducial_glob"; break;
    case PAD_PROP::FIDUCIAL_LOCAL: property = "pad_prop_fiducial_loc"; break;
    case PAD_PROP::TESTPOINT: property = "pad_prop_testpoint"; break;
    case PAD_PROP::HEATSINK: property = "pad_prop_heatsink"; break;
    case PAD_PROP::CASTELLATED: property = "pad_prop_castellated"; break;
    case PAD_PROP::MECHANICAL: property = "pad_prop_mechanical"; break;
    case PAD_PROP::PRESSFIT: property = "pad_prop_pressfit"; break;

    default: THROW_IO_ERROR( wxString::Format( wxT( "unknown pad property: %d" ), aPad->GetProperty() ) );
    }

    m_out->Print( "(pad %s %s %s", m_out->Quotew( aPad->GetNumber() ).c_str(), type, shapeName( F_Cu ) );

    m_out->Print( "(at %s %s)", formatInternalUnits( aPad->GetFPRelativePosition() ).c_str(),
                  aPad->GetOrientation().IsZero() ? ""
                                                  : EDA_UNIT_UTILS::FormatAngle( aPad->GetOrientation() ).c_str() );

    m_out->Print( "(size %s)", formatInternalUnits( aPad->GetSize( F_Cu ) ).c_str() );

    if( aPad->GetDelta( F_Cu ).x != 0 || aPad->GetDelta( F_Cu ).y != 0 )
    {
        m_out->Print( "(rect_delta %s)", formatInternalUnits( aPad->GetDelta( F_Cu ) ).c_str() );
    }

    const VECTOR2I& drill = aPad->GetDrillSize();
    VECTOR2I        shapeoffset = aPad->GetOffset( F_Cu );
    bool            forceShapeOffsetOutput = false;

    aPad->Padstack().ForEachUniqueLayer(
            [&]( PCB_LAYER_ID layer )
            {
                if( aPad->GetOffset( layer ) != shapeoffset )
                    forceShapeOffsetOutput = true;
            } );

    if( drill.x > 0 || drill.y > 0 || shapeoffset.x != 0 || shapeoffset.y != 0 || forceShapeOffsetOutput )
    {
        m_out->Print( "(drill" );

        if( aPad->GetDrillShape() == PAD_DRILL_SHAPE::OBLONG )
            m_out->Print( " oval" );

        if( drill.x > 0 )
            m_out->Print( " %s", formatInternalUnits( drill.x ).c_str() );

        if( drill.y > 0 && drill.x != drill.y )
            m_out->Print( " %s", formatInternalUnits( drill.y ).c_str() );

        // NOTE: Shape offest is a property of the copper shape, not of the drill, but this was put
        // in the file format under the drill section. So, it is left here to minimize file format
        // changes, but note that the other padstack layers (if present) will have an offset stored
        // separately.
        if( shapeoffset.x != 0 || shapeoffset.y != 0 || forceShapeOffsetOutput )
            m_out->Print( "(offset %s)", formatInternalUnits( aPad->GetOffset( F_Cu ) ).c_str() );

        m_out->Print( ")" );
    }

    if( aPad->Padstack().SecondaryDrill().size.x > 0 )
    {
        m_out->Print( "(backdrill (size %s) (layers %s %s))",
                      formatInternalUnits( aPad->Padstack().SecondaryDrill().size.x ).c_str(),
                      m_out->Quotew( LSET::Name( aPad->Padstack().SecondaryDrill().start ) ).c_str(),
                      m_out->Quotew( LSET::Name( aPad->Padstack().SecondaryDrill().end ) ).c_str() );
    }

    if( aPad->Padstack().TertiaryDrill().size.x > 0 )
    {
        m_out->Print( "(tertiary_drill (size %s) (layers %s %s))",
                      formatInternalUnits( aPad->Padstack().TertiaryDrill().size.x ).c_str(),
                      m_out->Quotew( LSET::Name( aPad->Padstack().TertiaryDrill().start ) ).c_str(),
                      m_out->Quotew( LSET::Name( aPad->Padstack().TertiaryDrill().end ) ).c_str() );
    }

    auto formatPostMachining = [&]( const char* aName, const PADSTACK::POST_MACHINING_PROPS& aProps )
    {
        if( !aProps.mode.has_value() || aProps.mode == PAD_DRILL_POST_MACHINING_MODE::NOT_POST_MACHINED )
            return;

        m_out->Print( "(%s %s", aName,
                      aProps.mode == PAD_DRILL_POST_MACHINING_MODE::COUNTERBORE ? "counterbore" : "countersink" );

        if( aProps.size > 0 )
            m_out->Print( " (size %s)", formatInternalUnits( aProps.size ).c_str() );

        if( aProps.depth > 0 )
            m_out->Print( " (depth %s)", formatInternalUnits( aProps.depth ).c_str() );

        if( aProps.angle > 0 )
            m_out->Print( " (angle %s)", FormatDouble2Str( aProps.angle / 10.0 ).c_str() );

        m_out->Print( ")" );
    };

    formatPostMachining( "front_post_machining", aPad->Padstack().FrontPostMachining() );
    formatPostMachining( "back_post_machining", aPad->Padstack().BackPostMachining() );

    // Add pad property, if exists.
    if( property )
        m_out->Print( "(property %s)", property );

    formatLayers( aPad->GetLayerSet(), false /* enumerate layers */ );

    if( aPad->GetAttribute() == PAD_ATTRIB::PTH )
    {
        KICAD_FORMAT::LEGACY::FormatBool( m_out, "remove_unused_layers", aPad->GetRemoveUnconnected() );

        if( aPad->GetRemoveUnconnected() )
        {
            KICAD_FORMAT::LEGACY::FormatBool( m_out, "keep_end_layers", aPad->GetKeepTopBottom() );

            if( board ) // Will be nullptr in footprint library
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

    auto formatCornerProperties = [&]( PCB_LAYER_ID aLayer )
    {
        // Output the radius ratio for rounded and chamfered rect pads
        if( aPad->GetShape( aLayer ) == PAD_SHAPE::ROUNDRECT || aPad->GetShape( aLayer ) == PAD_SHAPE::CHAMFERED_RECT )
        {
            m_out->Print( "(roundrect_rratio %s)",
                          FormatDouble2Str( aPad->GetRoundRectRadiusRatio( aLayer ) ).c_str() );
        }

        // Output the chamfer corners for chamfered rect pads
        if( aPad->GetShape( aLayer ) == PAD_SHAPE::CHAMFERED_RECT )
        {
            m_out->Print( "(chamfer_ratio %s)", FormatDouble2Str( aPad->GetChamferRectRatio( aLayer ) ).c_str() );

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

    // For normal padstacks, this is the one and only set of properties. For complex ones, this
    // will represent the front layer properties, and other layers will be formatted below
    formatCornerProperties( F_Cu );

    // Unconnected pad is default net so don't save it.
    if( !( m_ctl & CTL_OMIT_PAD_NETS ) && aPad->GetNetCode() > 0 )
        m_out->Print( "(net %s)", m_out->Quotew( aPad->GetNetname() ).c_str() );

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
        m_out->Print( "(die_length %s)", formatInternalUnits( aPad->GetPadToDieLength() ).c_str() );
    }

    if( aPad->GetPadToDieDelay() != 0 )
    {
        m_out->Print( "(die_delay %s)", formatInternalUnits( aPad->GetPadToDieDelay(), EDA_DATA_TYPE::TIME ).c_str() );
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
        m_out->Print( "(clearance %s)", formatInternalUnits( aPad->GetLocalClearance().value() ).c_str() );
    }

    if( aPad->GetLocalZoneConnection() != ZONE_CONNECTION::INHERITED )
    {
        m_out->Print( "(zone_connect %d)", static_cast<int>( aPad->GetLocalZoneConnection() ) );
    }

    if( aPad->GetLocalThermalSpokeWidthOverride().has_value() )
    {
        m_out->Print( "(thermal_bridge_width %s)",
                      formatInternalUnits( aPad->GetLocalThermalSpokeWidthOverride().value() ).c_str() );
    }

    EDA_ANGLE defaultThermalSpokeAngle = ANGLE_90;

    if( aPad->GetShape( F_Cu ) == PAD_SHAPE::CIRCLE
        || ( aPad->GetShape( F_Cu ) == PAD_SHAPE::CUSTOM && aPad->GetAnchorPadShape( F_Cu ) == PAD_SHAPE::CIRCLE ) )
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
        m_out->Print( "(thermal_gap %s)", formatInternalUnits( aPad->GetLocalThermalGapOverride().value() ).c_str() );
    }

    auto anchorShape = [&]( PCB_LAYER_ID aLayer )
    {
        switch( aPad->GetAnchorPadShape( aLayer ) )
        {
        case PAD_SHAPE::RECTANGLE: return "rect";
        default:
        case PAD_SHAPE::CIRCLE: return "circle";
        }
    };

    auto formatPrimitives = [&]( PCB_LAYER_ID aLayer )
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
                    m_out->Print( "(gr_line (start %s) (end %s)", formatInternalUnits( primitive->GetStart() ).c_str(),
                                  formatInternalUnits( primitive->GetEnd() ).c_str() );
                }
                break;

            case SHAPE_T::RECTANGLE:
                if( primitive->IsProxyItem() )
                {
                    m_out->Print( "(gr_bbox (start %s) (end %s)", formatInternalUnits( primitive->GetStart() ).c_str(),
                                  formatInternalUnits( primitive->GetEnd() ).c_str() );
                }
                else
                {
                    m_out->Print( "(gr_rect (start %s) (end %s)", formatInternalUnits( primitive->GetStart() ).c_str(),
                                  formatInternalUnits( primitive->GetEnd() ).c_str() );

                    if( primitive->GetCornerRadius() > 0 )
                    {
                        m_out->Print( " (radius %s)", formatInternalUnits( primitive->GetCornerRadius() ).c_str() );
                    }
                }
                break;

            case SHAPE_T::ARC:
                m_out->Print( "(gr_arc (start %s) (mid %s) (end %s)",
                              formatInternalUnits( primitive->GetStart() ).c_str(),
                              formatInternalUnits( primitive->GetArcMid() ).c_str(),
                              formatInternalUnits( primitive->GetEnd() ).c_str() );
                break;

            case SHAPE_T::CIRCLE:
                m_out->Print( "(gr_circle (center %s) (end %s)", formatInternalUnits( primitive->GetStart() ).c_str(),
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
                    const SHAPE_POLY_SET&   poly = primitive->GetPolyShape();
                    const SHAPE_LINE_CHAIN& outline = poly.Outline( 0 );

                    m_out->Print( "(gr_poly" );
                    formatPolyPts( outline );
                }
                break;

            default: break;
            }

            if( !primitive->IsProxyItem() )
                m_out->Print( "(width %s)", formatInternalUnits( primitive->GetWidth() ).c_str() );

            // The filled flag represents if a solid fill is present on circles,
            // rectangles and polygons
            if( ( primitive->GetShape() == SHAPE_T::POLY ) || ( primitive->GetShape() == SHAPE_T::RECTANGLE )
                || ( primitive->GetShape() == SHAPE_T::CIRCLE ) )
            {
                KICAD_FORMAT::LEGACY::FormatBool( m_out, "fill", primitive->IsSolidFill() );
            }

            m_out->Print( ")" );
        }

        m_out->Print( ")" ); // end of (primitives
    };

    bool hasCustomLayer = false;

    aPad->Padstack().ForEachUniqueLayer(
            [&]( PCB_LAYER_ID aLayer )
            {
                hasCustomLayer |= aPad->GetShape( aLayer ) == PAD_SHAPE::CUSTOM;
            } );

    if( aPad->GetShape( F_Cu ) == PAD_SHAPE::CUSTOM
        || ( hasCustomLayer && aPad->GetCustomShapeInZoneOpt() == CUSTOM_SHAPE_ZONE_MODE::CONVEXHULL ) )
    {
        m_out->Print( "(options" );

        if( aPad->GetCustomShapeInZoneOpt() == CUSTOM_SHAPE_ZONE_MODE::CONVEXHULL )
            m_out->Print( "(clearance convexhull)" );
        else
            m_out->Print( "(clearance outline)" );

        // Output the anchor pad shape (circle/rect)
        m_out->Print( "(anchor %s)", anchorShape( F_Cu ) );

        m_out->Print( ")" ); // end of (options ...

        // Output graphic primitive of the pad shape
        if( aPad->GetShape( F_Cu ) == PAD_SHAPE::CUSTOM )
            formatPrimitives( F_Cu );
    }

    if( !isDefaultTeardropParameters( aPad->GetTeardropParams() ) )
        formatTeardropParameters( aPad->GetTeardropParams() );

    if( aPad->Padstack().FrontOuterLayers().has_solder_mask.has_value()
        || aPad->Padstack().BackOuterLayers().has_solder_mask.has_value() )
    {
        m_out->Print( 0, " (tenting " );
        KICAD_FORMAT::LEGACY::FormatOptBool( m_out, "front", aPad->Padstack().FrontOuterLayers().has_solder_mask );
        KICAD_FORMAT::LEGACY::FormatOptBool( m_out, "back", aPad->Padstack().BackOuterLayers().has_solder_mask );
        m_out->Print( 0, ")" );
    }

    KICAD_FORMAT::LEGACY::FormatUuid( m_out, aPad->m_Uuid );

    auto formatPadLayer = [&]( PCB_LAYER_ID aLayer )
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

            if( aPad->GetCustomShapeInZoneOpt() == CUSTOM_SHAPE_ZONE_MODE::CONVEXHULL )
                m_out->Print( "(clearance convexhull)" );
            else
                m_out->Print( "(clearance outline)" );

            // Output the anchor pad shape (circle/rect)
            m_out->Print( "(anchor %s)", anchorShape( aLayer ) );

            m_out->Print( ")" ); // end of (options ...

            // Output graphic primitive of the pad shape
            formatPrimitives( aLayer );
        }

        EDA_ANGLE defaultLayerAngle = ANGLE_90;

        if( aPad->GetShape( aLayer ) == PAD_SHAPE::CIRCLE
            || ( aPad->GetShape( aLayer ) == PAD_SHAPE::CUSTOM
                 && aPad->GetAnchorPadShape( aLayer ) == PAD_SHAPE::CIRCLE ) )
        {
            defaultLayerAngle = ANGLE_45;
        }

        EDA_ANGLE layerSpokeAngle = padstack.ThermalSpokeAngle( aLayer );

        if( layerSpokeAngle != defaultLayerAngle )
        {
            m_out->Print( "(thermal_bridge_angle %s)", EDA_UNIT_UTILS::FormatAngle( layerSpokeAngle ).c_str() );
        }

        if( padstack.ThermalGap( aLayer ).has_value() )
        {
            m_out->Print( "(thermal_gap %s)", formatInternalUnits( *padstack.ThermalGap( aLayer ) ).c_str() );
        }

        if( padstack.ThermalSpokeWidth( aLayer ).has_value() )
        {
            m_out->Print( "(thermal_bridge_width %s)",
                          formatInternalUnits( *padstack.ThermalSpokeWidth( aLayer ) ).c_str() );
        }

        if( padstack.Clearance( aLayer ).has_value() )
        {
            m_out->Print( "(clearance %s)", formatInternalUnits( *padstack.Clearance( aLayer ) ).c_str() );
        }

        if( padstack.ZoneConnection( aLayer ).has_value() )
        {
            m_out->Print( "(zone_connect %d)", static_cast<int>( *padstack.ZoneConnection( aLayer ) ) );
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


void PCB_WRITER_V10::format( const PCB_BARCODE* aBarcode ) const
{
    wxCHECK_RET( aBarcode != nullptr && m_out != nullptr, "" );

    m_out->Print( "(barcode" );

    if( aBarcode->IsLocked() )
        KICAD_FORMAT::LEGACY::FormatBool( m_out, "locked", true );

    m_out->Print( "(at %s %s)", formatInternalUnits( aBarcode->GetPosition() ).c_str(),
                  EDA_UNIT_UTILS::FormatAngle( aBarcode->GetAngle() ).c_str() );

    formatLayer( aBarcode->GetLayer() );

    m_out->Print( "(size %s %s)", formatInternalUnits( aBarcode->GetWidth() ).c_str(),
                  formatInternalUnits( aBarcode->GetHeight() ).c_str() );

    m_out->Print( "(text %s)", m_out->Quotew( aBarcode->GetText() ).c_str() );

    m_out->Print( "(text_height %s)", formatInternalUnits( aBarcode->GetTextSize() ).c_str() );

    const char* typeStr = "code39";

    switch( aBarcode->GetKind() )
    {
    case BARCODE_T::CODE_39: typeStr = "code39"; break;
    case BARCODE_T::CODE_128: typeStr = "code128"; break;
    case BARCODE_T::DATA_MATRIX: typeStr = "datamatrix"; break;
    case BARCODE_T::QR_CODE: typeStr = "qr"; break;
    case BARCODE_T::MICRO_QR_CODE: typeStr = "microqr"; break;
    }

    m_out->Print( "(type %s)", typeStr );

    if( aBarcode->GetKind() == BARCODE_T::QR_CODE || aBarcode->GetKind() == BARCODE_T::MICRO_QR_CODE )
    {
        const char* eccStr = "L";
        switch( aBarcode->GetErrorCorrection() )
        {
        case BARCODE_ECC_T::L: eccStr = "L"; break;
        case BARCODE_ECC_T::M: eccStr = "M"; break;
        case BARCODE_ECC_T::Q: eccStr = "Q"; break;
        case BARCODE_ECC_T::H: eccStr = "H"; break;
        }

        m_out->Print( "(ecc_level %s)", eccStr );
    }

    KICAD_FORMAT::LEGACY::FormatBool( m_out, "hide", !aBarcode->GetShowText() );
    KICAD_FORMAT::LEGACY::FormatBool( m_out, "knockout", aBarcode->IsKnockout() );

    if( aBarcode->GetMargin().x != 0 || aBarcode->GetMargin().y != 0 )
    {
        m_out->Print( "(margins %s %s)", formatInternalUnits( aBarcode->GetMargin().x ).c_str(),
                      formatInternalUnits( aBarcode->GetMargin().y ).c_str() );
    }

    KICAD_FORMAT::LEGACY::FormatUuid( m_out, aBarcode->m_Uuid );

    m_out->Print( ")" );
}


void PCB_WRITER_V10::format( const PCB_TEXT* aText ) const
{
    FOOTPRINT*       parentFP = aText->GetParentFootprint();
    std::string      prefix;
    std::string      type;
    VECTOR2I         pos = aText->GetTextPos();
    const PCB_FIELD* field = aText->Type() == PCB_FIELD_T ? static_cast<const PCB_FIELD*>( aText ) : nullptr;

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
        m_out->Print( "(%s_text %s %s", prefix.c_str(), type.c_str(), m_out->Quotew( aText->GetText() ).c_str() );

        if( aText->IsLocked() )
            KICAD_FORMAT::LEGACY::FormatBool( m_out, "locked", true );
    }

    m_out->Print( "(at %s %s)", formatInternalUnits( pos ).c_str(),
                  EDA_UNIT_UTILS::FormatAngle( aText->GetTextAngle() ).c_str() );

    if( parentFP && !aText->IsKeepUpright() )
        KICAD_FORMAT::LEGACY::FormatBool( m_out, "unlocked", true );

    formatLayer( aText->GetLayer(), aText->IsKnockout() );

    if( field && !field->IsVisible() )
        KICAD_FORMAT::LEGACY::FormatBool( m_out, "hide", true );

    KICAD_FORMAT::LEGACY::FormatUuid( m_out, aText->m_Uuid );

    // Currently, texts have no specific color and no hyperlink.
    // so ensure they are never written in kicad_pcb file
    int ctl_flags = CTL_OMIT_COLOR | CTL_OMIT_HYPERLINK;

    KICAD_FORMAT::LEGACY::FormatTextV10( m_out, *aText, pcbIUScale, ctl_flags );

    if( aText->GetFont() && aText->GetFont()->IsOutline() )
        formatRenderCache( aText );

    if( !field )
        m_out->Print( ")" );
}


void PCB_WRITER_V10::format( const PCB_TEXTBOX* aTextBox ) const
{
    FOOTPRINT* parentFP = aTextBox->GetParentFootprint();

    m_out->Print( "(%s %s",
                  aTextBox->Type() == PCB_TABLECELL_T ? "table_cell"
                  : parentFP                          ? "fp_text_box"
                                                      : "gr_text_box",
                  m_out->Quotew( aTextBox->GetText() ).c_str() );

    if( aTextBox->IsLocked() )
        KICAD_FORMAT::LEGACY::FormatBool( m_out, "locked", true );

    if( aTextBox->GetShape() == SHAPE_T::RECTANGLE )
    {
        m_out->Print( "(start %s) (end %s)", formatInternalUnits( aTextBox->GetStart(), parentFP ).c_str(),
                      formatInternalUnits( aTextBox->GetEnd(), parentFP ).c_str() );
    }
    else if( aTextBox->GetShape() == SHAPE_T::POLY )
    {
        const SHAPE_POLY_SET&   poly = aTextBox->GetPolyShape();
        const SHAPE_LINE_CHAIN& outline = poly.Outline( 0 );

        formatPolyPts( outline, parentFP );
    }
    else
    {
        UNIMPLEMENTED_FOR( aTextBox->SHAPE_T_asString() );
    }

    m_out->Print( "(margins %s %s %s %s)", formatInternalUnits( aTextBox->GetMarginLeft() ).c_str(),
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

    KICAD_FORMAT::LEGACY::FormatUuid( m_out, aTextBox->m_Uuid );

    KICAD_FORMAT::LEGACY::FormatTextV10( m_out, *aTextBox, pcbIUScale, 0 );

    if( aTextBox->Type() != PCB_TABLECELL_T )
    {
        KICAD_FORMAT::LEGACY::FormatBool( m_out, "border", aTextBox->IsBorderEnabled() );
        KICAD_FORMAT::LEGACY::FormatStroke( m_out, aTextBox->GetStroke(), pcbIUScale );

        KICAD_FORMAT::LEGACY::FormatBool( m_out, "knockout", aTextBox->IsKnockout() );
    }

    if( aTextBox->GetFont() && aTextBox->GetFont()->IsOutline() )
        formatRenderCache( aTextBox );

    m_out->Print( ")" );
}


void PCB_WRITER_V10::format( const PCB_TABLE* aTable ) const
{
    wxCHECK_RET( aTable != nullptr && m_out != nullptr, "" );

    m_out->Print( "(table (column_count %d)", aTable->GetColCount() );

    KICAD_FORMAT::LEGACY::FormatUuid( m_out, aTable->m_Uuid );

    if( aTable->IsLocked() )
        KICAD_FORMAT::LEGACY::FormatBool( m_out, "locked", true );

    formatLayer( aTable->GetLayer() );

    m_out->Print( "(border" );
    KICAD_FORMAT::LEGACY::FormatBool( m_out, "external", aTable->StrokeExternal() );
    KICAD_FORMAT::LEGACY::FormatBool( m_out, "header", aTable->StrokeHeaderSeparator() );

    if( aTable->StrokeExternal() || aTable->StrokeHeaderSeparator() )
        KICAD_FORMAT::LEGACY::FormatStroke( m_out, aTable->GetBorderStroke(), pcbIUScale );

    m_out->Print( ")" ); // Close `border` token.

    m_out->Print( "(separators" );
    KICAD_FORMAT::LEGACY::FormatBool( m_out, "rows", aTable->StrokeRows() );
    KICAD_FORMAT::LEGACY::FormatBool( m_out, "cols", aTable->StrokeColumns() );

    if( aTable->StrokeRows() || aTable->StrokeColumns() )
        KICAD_FORMAT::LEGACY::FormatStroke( m_out, aTable->GetSeparatorsStroke(), pcbIUScale );

    m_out->Print( ")" ); // Close `separators` token.

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

    m_out->Print( ")" ); // Close `cells` token.
    m_out->Print( ")" ); // Close `table` token.
}


void PCB_WRITER_V10::format( const PCB_GROUP* aGroup ) const
{
    wxArrayString memberIds;

    if( m_board )
    {
        const auto& cache = m_board->GetItemByIdCache();

        std::unordered_set<const EDA_ITEM*> validPtrs;

        for( const auto& [uuid, item] : cache )
            validPtrs.insert( item );

        for( EDA_ITEM* member : aGroup->GetItems() )
        {
            if( validPtrs.count( member ) )
                memberIds.Add( member->m_Uuid.AsString() );
        }
    }
    else
    {
        for( EDA_ITEM* member : aGroup->GetItems() )
            memberIds.Add( member->m_Uuid.AsString() );
    }

    if( memberIds.empty() )
        return;

    m_out->Print( "(group %s", m_out->Quotew( aGroup->GetName() ).c_str() );

    KICAD_FORMAT::LEGACY::FormatUuid( m_out, aGroup->m_Uuid );

    if( aGroup->IsLocked() )
        KICAD_FORMAT::LEGACY::FormatBool( m_out, "locked", true );

    if( aGroup->HasDesignBlockLink() )
        m_out->Print( "(lib_id \"%s\")", KICAD_FORMAT::LEGACY::FormatLibId( aGroup->GetDesignBlockLibId() ).c_str() );

    memberIds.Sort();

    m_out->Print( "(members" );

    for( const wxString& memberId : memberIds )
        m_out->Print( " %s", m_out->Quotew( memberId ).c_str() );

    m_out->Print( ")" ); // Close `members` token.
    m_out->Print( ")" ); // Close `group` token.
}


void PCB_WRITER_V10::format( const PCB_GENERATOR* aGenerator ) const
{
    // Some conditions appear to still be creating ghost tuning patterns. Don't save them.
    if( aGenerator->GetGeneratorType() == wxT( "tuning_pattern" ) && aGenerator->GetItems().empty() )
    {
        return;
    }

    m_out->Print( "(generated" );

    KICAD_FORMAT::LEGACY::FormatUuid( m_out, aGenerator->m_Uuid );

    m_out->Print( "(type %s) (name %s) (layer %s)", TO_UTF8( aGenerator->GetGeneratorType() ),
                  m_out->Quotew( aGenerator->GetName() ).c_str(),
                  m_out->Quotew( LSET::Name( aGenerator->GetLayer() ) ).c_str() );

    if( aGenerator->IsLocked() )
        KICAD_FORMAT::LEGACY::FormatBool( m_out, "locked", true );

    for( const auto& [key, value] : KICAD_FORMAT::LEGACY::TuningPropertiesForEra( *aGenerator, false ) )
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

            KICAD_FORMAT::LEGACY::FormatBool( m_out, key, val );
        }
        else if( value.CheckType<VECTOR2I>() )
        {
            VECTOR2I val;
            value.GetAs( &val );

            m_out->Print( "(%s (xy %s))", key.c_str(), formatInternalUnits( val ).c_str() );
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

    m_out->Print( ")" ); // Close `members` token.
    m_out->Print( ")" ); // Close `generated` token.
}


void PCB_WRITER_V10::format( const PCB_TRACK* aTrack ) const
{
    if( aTrack->Type() == PCB_VIA_T )
    {
        PCB_LAYER_ID layer1, layer2;

        const PCB_VIA* via = static_cast<const PCB_VIA*>( aTrack );
        const BOARD*   board = via->GetBoard();

        wxCHECK_RET( board != nullptr, wxT( "Via has no parent." ) );

        m_out->Print( "(via" );

        via->LayerPair( &layer1, &layer2 );

        switch( via->GetViaType() )
        {
        case VIATYPE::THROUGH: //  Default shape not saved.
            break;

        case VIATYPE::BLIND: m_out->Print( " blind " ); break;

        case VIATYPE::BURIED: m_out->Print( " buried " ); break;

        case VIATYPE::MICROVIA: m_out->Print( " micro " ); break;

        default: THROW_IO_ERROR( wxString::Format( _( "unknown via type %d" ), via->GetViaType() ) );
        }

        m_out->Print( "(at %s) (size %s)", formatInternalUnits( aTrack->GetStart() ).c_str(),
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

        if( via->Padstack().SecondaryDrill().size.x > 0 )
        {
            m_out->Print( "(backdrill (size %s) (layers %s %s))",
                          formatInternalUnits( via->Padstack().SecondaryDrill().size.x ).c_str(),
                          m_out->Quotew( LSET::Name( via->Padstack().SecondaryDrill().start ) ).c_str(),
                          m_out->Quotew( LSET::Name( via->Padstack().SecondaryDrill().end ) ).c_str() );
        }

        if( via->Padstack().TertiaryDrill().size.x > 0 )
        {
            m_out->Print( "(tertiary_drill (size %s) (layers %s %s))",
                          formatInternalUnits( via->Padstack().TertiaryDrill().size.x ).c_str(),
                          m_out->Quotew( LSET::Name( via->Padstack().TertiaryDrill().start ) ).c_str(),
                          m_out->Quotew( LSET::Name( via->Padstack().TertiaryDrill().end ) ).c_str() );
        }

        auto formatPostMachining = [&]( const char* aName, const PADSTACK::POST_MACHINING_PROPS& aProps )
        {
            if( !aProps.mode.has_value() || aProps.mode == PAD_DRILL_POST_MACHINING_MODE::NOT_POST_MACHINED )
                return;

            m_out->Print( "(%s %s", aName,
                          aProps.mode == PAD_DRILL_POST_MACHINING_MODE::COUNTERBORE ? "counterbore" : "countersink" );

            if( aProps.size > 0 )
                m_out->Print( " (size %s)", formatInternalUnits( aProps.size ).c_str() );

            if( aProps.depth > 0 )
                m_out->Print( " (depth %s)", formatInternalUnits( aProps.depth ).c_str() );

            if( aProps.angle > 0 )
                m_out->Print( " (angle %s)", FormatDouble2Str( aProps.angle / 10.0 ).c_str() );

            m_out->Print( ")" );
        };

        formatPostMachining( "front_post_machining", via->Padstack().FrontPostMachining() );
        formatPostMachining( "back_post_machining", via->Padstack().BackPostMachining() );

        m_out->Print( "(layers %s %s)", m_out->Quotew( LSET::Name( layer1 ) ).c_str(),
                      m_out->Quotew( LSET::Name( layer2 ) ).c_str() );

        switch( via->Padstack().UnconnectedLayerMode() )
        {
        case UNCONNECTED_LAYER_MODE::REMOVE_ALL:
            KICAD_FORMAT::LEGACY::FormatBool( m_out, "remove_unused_layers", true );
            KICAD_FORMAT::LEGACY::FormatBool( m_out, "keep_end_layers", false );
            break;

        case UNCONNECTED_LAYER_MODE::REMOVE_EXCEPT_START_AND_END:
            KICAD_FORMAT::LEGACY::FormatBool( m_out, "remove_unused_layers", true );
            KICAD_FORMAT::LEGACY::FormatBool( m_out, "keep_end_layers", true );
            break;

        case UNCONNECTED_LAYER_MODE::START_END_ONLY:
            KICAD_FORMAT::LEGACY::FormatBool( m_out, "start_end_only", true );
            break;

        case UNCONNECTED_LAYER_MODE::KEEP_ALL: break;
        }

        if( via->IsLocked() )
            KICAD_FORMAT::LEGACY::FormatBool( m_out, "locked", true );

        if( via->GetIsFree() )
            KICAD_FORMAT::LEGACY::FormatBool( m_out, "free", true );

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

        if( padstack.FrontOuterLayers().has_solder_mask.has_value()
            || padstack.BackOuterLayers().has_solder_mask.has_value() )
        {
            m_out->Print( 0, " (tenting " );
            KICAD_FORMAT::LEGACY::FormatOptBool( m_out, "front", padstack.FrontOuterLayers().has_solder_mask );
            KICAD_FORMAT::LEGACY::FormatOptBool( m_out, "back", padstack.BackOuterLayers().has_solder_mask );
            m_out->Print( 0, ")" );
        }

        if( padstack.Drill().is_capped.has_value() )
            KICAD_FORMAT::LEGACY::FormatOptBool( m_out, "capping", padstack.Drill().is_capped );

        if( padstack.FrontOuterLayers().has_covering.has_value()
            || padstack.BackOuterLayers().has_covering.has_value() )
        {
            m_out->Print( 0, " (covering " );
            KICAD_FORMAT::LEGACY::FormatOptBool( m_out, "front", padstack.FrontOuterLayers().has_covering );
            KICAD_FORMAT::LEGACY::FormatOptBool( m_out, "back", padstack.BackOuterLayers().has_covering );
            m_out->Print( 0, ")" );
        }

        if( padstack.FrontOuterLayers().has_plugging.has_value()
            || padstack.BackOuterLayers().has_plugging.has_value() )
        {
            m_out->Print( 0, " (plugging " );
            KICAD_FORMAT::LEGACY::FormatOptBool( m_out, "front", padstack.FrontOuterLayers().has_plugging );
            KICAD_FORMAT::LEGACY::FormatOptBool( m_out, "back", padstack.BackOuterLayers().has_plugging );
            m_out->Print( 0, ")" );
        }

        if( padstack.Drill().is_filled.has_value() )
            KICAD_FORMAT::LEGACY::FormatOptBool( m_out, "filling", padstack.Drill().is_filled );

        if( padstack.Mode() != PADSTACK::MODE::NORMAL )
        {
            m_out->Print( "(padstack" );

            if( padstack.Mode() == PADSTACK::MODE::FRONT_INNER_BACK )
            {
                m_out->Print( "(mode front_inner_back)" );

                m_out->Print( "(layer \"Inner\"" );
                m_out->Print( "(size %s)", formatInternalUnits( padstack.Size( PADSTACK::INNER_LAYERS ).x ).c_str() );
                m_out->Print( ")" );
                m_out->Print( "(layer \"B.Cu\"" );
                m_out->Print( "(size %s)", formatInternalUnits( padstack.Size( B_Cu ).x ).c_str() );
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
                    m_out->Print( "(size %s)", formatInternalUnits( padstack.Size( layer ).x ).c_str() );
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
                          formatInternalUnits( arc->GetStart() ).c_str(), formatInternalUnits( arc->GetMid() ).c_str(),
                          formatInternalUnits( arc->GetEnd() ).c_str(),
                          formatInternalUnits( arc->GetWidth() ).c_str() );
        }
        else
        {
            m_out->Print( "(segment (start %s) (end %s) (width %s)", formatInternalUnits( aTrack->GetStart() ).c_str(),
                          formatInternalUnits( aTrack->GetEnd() ).c_str(),
                          formatInternalUnits( aTrack->GetWidth() ).c_str() );
        }

        if( aTrack->IsLocked() )
            KICAD_FORMAT::LEGACY::FormatBool( m_out, "locked", true );

        if( aTrack->GetLayerSet().count() > 1 )
            formatLayers( aTrack->GetLayerSet(), false /* enumerate layers */ );
        else
            formatLayer( aTrack->GetLayer() );

        if( aTrack->HasSolderMask() && aTrack->GetLocalSolderMaskMargin().has_value()
            && IsExternalCopperLayer( aTrack->GetLayer() ) )
        {
            m_out->Print( "(solder_mask_margin %s)",
                          formatInternalUnits( aTrack->GetLocalSolderMaskMargin().value() ).c_str() );
        }
    }

    m_out->Print( "(net %s)", m_out->Quotew( aTrack->GetNetname() ).c_str() );

    KICAD_FORMAT::LEGACY::FormatUuid( m_out, aTrack->m_Uuid );
    m_out->Print( ")" );
}


void PCB_WRITER_V10::format( const ZONE* aZone ) const
{
    m_out->Print( "(zone" );

    if( aZone->IsOnCopperLayer() && !aZone->GetIsRuleArea() && aZone->GetNetCode() > 0 )
        m_out->Print( "(net %s)", m_out->Quotew( aZone->GetNetname() ).c_str() );

    if( aZone->IsLocked() )
        KICAD_FORMAT::LEGACY::FormatBool( m_out, "locked", true );

    // If a zone exists on multiple layers, format accordingly
    LSET layers = aZone->GetLayerSet();

    if( aZone->GetBoard() )
        layers &= aZone->GetBoard()->GetEnabledLayers();

    // Always enumerate every layer for a zone on a copper layer
    if( layers.count() > 1 )
        formatLayers( layers, aZone->IsOnCopperLayer(), true );
    else
        formatLayer( aZone->GetFirstLayer() );

    if( !aZone->IsTeardropArea() )
        KICAD_FORMAT::LEGACY::FormatUuid( m_out, aZone->m_Uuid );

    if( !aZone->GetZoneName().empty() && !aZone->IsTeardropArea() )
        m_out->Print( "(name %s)", m_out->Quotew( aZone->GetZoneName() ).c_str() );

    // Save the outline aux info
    std::string hatch;

    switch( aZone->GetHatchStyle() )
    {
    default:
    case ZONE_BORDER_DISPLAY_STYLE::NO_HATCH: hatch = "none"; break;
    case ZONE_BORDER_DISPLAY_STYLE::DIAGONAL_EDGE: hatch = "edge"; break;
    case ZONE_BORDER_DISPLAY_STYLE::DIAGONAL_FULL: hatch = "full"; break;
    }

    m_out->Print( "(hatch %s %s)", hatch.c_str(), formatInternalUnits( aZone->GetBorderHatchPitch() ).c_str() );


    if( aZone->GetAssignedPriority() > 0 )
        m_out->Print( "(priority %d)", aZone->GetAssignedPriority() );

    // Add teardrop keywords in file: (attr (teardrop (type xxx))) where xxx is the teardrop type
    if( aZone->IsTeardropArea() )
    {
        m_out->Print( "(attr (teardrop (type %s)))",
                      aZone->GetTeardropAreaType() == TEARDROP_TYPE::TD_VIAPAD ? "padvia" : "track_end" );
    }

    m_out->Print( "(connect_pads" );

    switch( aZone->GetPadConnection() )
    {
    default:
    case ZONE_CONNECTION::THERMAL: // Default option not saved or loaded.
        break;

    case ZONE_CONNECTION::THT_THERMAL: m_out->Print( " thru_hole_only" ); break;

    case ZONE_CONNECTION::FULL: m_out->Print( " yes" ); break;

    case ZONE_CONNECTION::NONE: m_out->Print( " no" ); break;
    }

    m_out->Print( "(clearance %s)", formatInternalUnits( aZone->GetLocalClearance().value() ).c_str() );

    m_out->Print( ")" );

    m_out->Print( "(min_thickness %s)", formatInternalUnits( aZone->GetMinThickness() ).c_str() );

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
        KICAD_FORMAT::LEGACY::FormatBool( m_out, "enabled", aZone->GetPlacementAreaEnabled() );

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
        // These are transitory and should not be saved
        case PLACEMENT_SOURCE_T::DESIGN_BLOCK: break;
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

    if( aZone->GetCornerSmoothingType() != ZONE_SETTINGS::CORNER_SMOOTHING::NO_SMOOTHING )
    {
        switch( aZone->GetCornerSmoothingType() )
        {
        case ZONE_SETTINGS::CORNER_SMOOTHING::CHAMFER: m_out->Print( "(smoothing chamfer)" ); break;

        case ZONE_SETTINGS::CORNER_SMOOTHING::FILLET: m_out->Print( "(smoothing fillet)" ); break;

        default:
            THROW_IO_ERROR( wxString::Format( _( "unknown zone corner smoothing type %d" ),
                                              static_cast<int>( aZone->GetCornerSmoothingType() ) ) );
        }

        if( aZone->GetCornerRadius() != 0 )
            m_out->Print( "(radius %s)", formatInternalUnits( aZone->GetCornerRadius() ).c_str() );
    }

    m_out->Print( "(island_removal_mode %d)", static_cast<int>( aZone->GetIslandRemovalMode() ) );

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
            m_out->Print( "(hatch_smoothing_level %d) (hatch_smoothing_value %s)", aZone->GetHatchSmoothingLevel(),
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
        // Footprint outlines are now stored in library coordinates. The legacy format
        // stores these in board coordinates, unlike the other footprint graphics.
        SHAPE_POLY_SET::POLYGON poly = aZone->GetBoardOutline().Polygon( 0 );

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
                KICAD_FORMAT::LEGACY::FormatBool( m_out, "island", true );

            const SHAPE_LINE_CHAIN& chain = fv->COutline( ii );

            formatPolyPts( chain );
            m_out->Print( ")" );
        }
    }

    m_out->Print( ")" );
}


void PCB_WRITER_V10::format( const ZONE_LAYER_PROPERTIES& aZoneLayerProperties, int aNestLevel,
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


void PCB_WRITER_V10::SaveFootprintFile( const wxString& aFileName, FOOTPRINT* aFootprint )
{
    if( aFootprint->GetAreFontsEmbedded() )
        aFootprint->EmbedFonts();
    else
        aFootprint->GetEmbeddedFiles()->ClearEmbeddedFonts();

    PRETTIFIED_FILE_OUTPUTFORMATTER formatter( aFileName );

    m_out = &formatter;
    m_ctl = CTL_FOR_LIBRARY;
    Format( aFootprint );
    formatter.Finish();
    m_out = nullptr;
}
