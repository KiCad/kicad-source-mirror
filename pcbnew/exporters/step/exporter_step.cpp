/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2022 Mark Roszko <mark.roszko@gmail.com>
 * Copyright (C) 2016 Cirilo Bernardo <cirilo.bernardo@gmail.com>
 * Copyright (C) 2016-2025 KiCad Developers, see AUTHORS.txt for contributors.
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

#include "exporter_step.h"
#include <advanced_config.h>
#include <board.h>
#include <board_design_settings.h>
#include <footprint.h>
#include <pcb_textbox.h>
#include <pcb_track.h>
#include <pcb_shape.h>
#include <pad.h>
#include <zone.h>
#include <fp_lib_table.h>
#include "step_pcb_model.h"

#include <pgm_base.h>
#include <base_units.h>
#include <filename_resolver.h>
#include <trace_helpers.h>
#include <project_pcb.h>
#include <wildcards_and_files_ext.h>

#include <Message.hxx>                // OpenCascade messenger
#include <Message_PrinterOStream.hxx> // OpenCascade output messenger
#include <Standard_Failure.hxx>       // In open cascade

#include <Standard_Version.hxx>

#include <wx/crt.h>
#include <wx/log.h>
#include <core/profile.h>        // To use GetRunningMicroSecs or another profiling utility

#define OCC_VERSION_MIN 0x070500

#if OCC_VERSION_HEX < OCC_VERSION_MIN
#include <Message_Messenger.hxx>
#endif


void ReportMessage( const wxString& aMessage )
{
    wxPrintf( aMessage );
    fflush( stdout ); // Force immediate printing (needed on mingw)
}

class KiCadPrinter : public Message_Printer
{
public:
    KiCadPrinter( EXPORTER_STEP* aConverter ) : m_converter( aConverter ) {}

protected:
#if OCC_VERSION_HEX < OCC_VERSION_MIN
    virtual void Send( const TCollection_ExtendedString& theString,
                       const Message_Gravity theGravity,
                       const Standard_Boolean theToPutEol ) const override
    {
        Send( TCollection_AsciiString( theString ), theGravity, theToPutEol );
    }

    virtual void Send( const TCollection_AsciiString& theString,
                       const Message_Gravity theGravity,
                       const Standard_Boolean theToPutEol ) const override
#else
    virtual void send( const TCollection_AsciiString& theString,
                        const Message_Gravity theGravity ) const override
#endif
    {
        if( theGravity >= Message_Warning
            || ( wxLog::IsAllowedTraceMask( traceKiCad2Step ) && theGravity == Message_Info ) )
      {
          ReportMessage( theString.ToCString() );

#if OCC_VERSION_HEX < OCC_VERSION_MIN
          if( theToPutEol )
              ReportMessage( wxT( "\n" ) );
#else
          ReportMessage( wxT( "\n" ) );
#endif
      }

      if( theGravity == Message_Warning )
          m_converter->SetWarn();

      if( theGravity >= Message_Alarm )
          m_converter->SetError();

      if( theGravity == Message_Fail )
          m_converter->SetFail();
    }

private:
    EXPORTER_STEP* m_converter;
};


EXPORTER_STEP::EXPORTER_STEP( BOARD* aBoard, const EXPORTER_STEP_PARAMS& aParams ) :
    m_params( aParams ),
    m_error( false ),
    m_fail( false ),
    m_warn( false ),
    m_board( aBoard ),
    m_pcbModel( nullptr )
{
    m_copperColor = COLOR4D( 0.7, 0.61, 0.0, 1.0 );

    if( m_params.m_ExportComponents )
        m_padColor = COLOR4D( 0.50, 0.50, 0.50, 1.0 );
    else
        m_padColor = m_copperColor;

    // TODO: make configurable
    m_platingThickness = pcbIUScale.mmToIU( 0.025 );

    // Init m_pcbBaseName to the board short filename (no path, no ext)
    // m_pcbName is used later to identify items in step file
    wxFileName fn( aBoard->GetFileName() );
    m_pcbBaseName = fn.GetName();

    // Remove the autosave prefix
    m_pcbBaseName.StartsWith( FILEEXT::AutoSaveFilePrefix, &m_pcbBaseName );

    m_resolver = std::make_unique<FILENAME_RESOLVER>();
    m_resolver->Set3DConfigDir( wxT( "" ) );
    // needed to add the project to the search stack
    m_resolver->SetProject( aBoard->GetProject() );
    m_resolver->SetProgramBase( &Pgm() );
}


EXPORTER_STEP::~EXPORTER_STEP()
{
}


bool EXPORTER_STEP::buildFootprint3DShapes( FOOTPRINT* aFootprint, VECTOR2D aOrigin )
{
    bool              hasdata = false;
    std::vector<PAD*> padsMatchingNetFilter;
    int               maxError = m_board->GetDesignSettings().m_MaxError;

    // Dump the pad holes into the PCB
    for( PAD* pad : aFootprint->Pads() )
    {
        std::shared_ptr<SHAPE_SEGMENT> holeShape = pad->GetEffectiveHoleShape();

        SHAPE_POLY_SET holePoly;
        holeShape->TransformToPolygon( holePoly, maxError, ERROR_INSIDE );

        for( PCB_LAYER_ID pcblayer : pad->GetLayerSet().Seq() )
        {
            if( pad->IsOnLayer( pcblayer ) )
                m_poly_holes[pcblayer].Append( holePoly );
        }

        if( pad->HasHole() )
        {
            int platingThickness = pad->GetAttribute() == PAD_ATTRIB::PTH ? m_platingThickness : 0;

            if( m_pcbModel->AddHole( *holeShape, platingThickness, F_Cu, B_Cu, false, aOrigin, true,
                                     true ) )
            {
                hasdata = true;
            }

            //// Cut holes in silkscreen (buggy: insufficient polyset self-intersection checking)
            //if( m_layersToExport.Contains( F_SilkS ) || m_layersToExport.Contains( B_SilkS ) )
            //{
            //    m_poly_holes[F_SilkS].Append( holePoly );
            //    m_poly_holes[B_SilkS].Append( holePoly );
            //}
        }

        if( !m_params.m_NetFilter.IsEmpty() && !pad->GetNetname().Matches( m_params.m_NetFilter ) )
            continue;

        if( m_params.m_ExportPads )
        {
            if( m_pcbModel->AddPadShape( pad, aOrigin, false ) )
                hasdata = true;

            if( m_params.m_ExportSoldermask )
            {
                for( PCB_LAYER_ID pcblayer : pad->GetLayerSet().Seq() )
                {
                    if( pcblayer != F_Mask && pcblayer != B_Mask )
                        continue;

                    SHAPE_POLY_SET poly;
                    PCB_LAYER_ID cuLayer = ( pcblayer == F_Mask ) ? F_Cu : B_Cu;
                    pad->TransformShapeToPolygon( poly, cuLayer,
                                                  pad->GetSolderMaskExpansion( cuLayer ), maxError,
                                                  ERROR_INSIDE );

                    m_poly_shapes[pcblayer].Append( poly );
                }
            }
        }

        padsMatchingNetFilter.push_back( pad );
    }

    // Build 3D shapes of the footprint graphic items:
    for( PCB_LAYER_ID pcblayer : m_layersToExport.Seq() )
    {
        if( IsCopperLayer( pcblayer ) && !m_params.m_ExportTracksVias )
            continue;

        SHAPE_POLY_SET buffer;

        aFootprint->TransformFPShapesToPolySet( buffer, pcblayer, 0, maxError, ERROR_INSIDE,
                                                true, /* include text */
                                                true, /* include shapes */
                                                false /* include private items */ );

        if( m_params.m_NetFilter.IsEmpty() || !IsCopperLayer( pcblayer ) )
        {
            m_poly_shapes[pcblayer].Append( buffer );
        }
        else
        {
            // Only add shapes colliding with any matching pads
            for( const SHAPE_POLY_SET::POLYGON& poly : buffer.CPolygons() )
            {
                for( PAD* pad : padsMatchingNetFilter )
                {
                    if( !pad->IsOnLayer( pcblayer ) )
                        continue;

                    std::shared_ptr<SHAPE_POLY_SET> padPoly = pad->GetEffectivePolygon( pcblayer );
                    SHAPE_POLY_SET                  gfxPoly( poly );

                    if( padPoly->Collide( &gfxPoly ) )
                    {
                        m_poly_shapes[pcblayer].Append( gfxPoly );
                        break;
                    }
                }
            }
        }
    }

    if( ( !(aFootprint->GetAttributes() & (FP_THROUGH_HOLE|FP_SMD)) ) && !m_params.m_IncludeUnspecified )
    {
        return hasdata;
    }

    if( ( aFootprint->GetAttributes() & FP_DNP ) && !m_params.m_IncludeDNP )
    {
        return hasdata;
    }

    // Prefetch the library for this footprint
    // In case we need to resolve relative footprint paths
    wxString libraryName = aFootprint->GetFPID().GetLibNickname();
    wxString footprintBasePath = wxEmptyString;

    double posX = aFootprint->GetPosition().x - aOrigin.x;
    double posY = (aFootprint->GetPosition().y) - aOrigin.y;

    if( m_board->GetProject() )
    {
        try
        {
            // FindRow() can throw an exception
            const FP_LIB_TABLE_ROW* fpRow =
                    PROJECT_PCB::PcbFootprintLibs( m_board->GetProject() )->FindRow( libraryName, false );

            if( fpRow )
                footprintBasePath = fpRow->GetFullURI( true );
        }
        catch( ... )
        {
            // Do nothing if the libraryName is not found in lib table
        }
    }

    // Exit early if we don't want to include footprint models
    if( m_params.m_BoardOnly || !m_params.m_ExportComponents )
    {
        return hasdata;
    }

    bool componentFilter = !m_params.m_ComponentFilter.IsEmpty();
    std::vector<wxString> componentFilterPatterns;

    if( componentFilter )
    {
        wxStringTokenizer tokenizer( m_params.m_ComponentFilter, wxS( "," ), wxTOKEN_STRTOK );

        while( tokenizer.HasMoreTokens() )
            componentFilterPatterns.push_back( tokenizer.GetNextToken().Trim( false ) );

        bool found = false;

        for( const wxString& pattern : componentFilterPatterns )
        {
            if( aFootprint->GetReference().Matches( pattern ) )
            {
                found = true;
                break;
            }
        }

        if( !found )
            return hasdata;
    }

    VECTOR2D newpos( pcbIUScale.IUTomm( posX ), pcbIUScale.IUTomm( posY ) );

    for( const FP_3DMODEL& fp_model : aFootprint->Models() )
    {
        if( !fp_model.m_Show || fp_model.m_Filename.empty() )
            continue;

        std::vector<wxString> searchedPaths;
        wxString mname = m_resolver->ResolvePath( fp_model.m_Filename, footprintBasePath, aFootprint );


        if( mname.empty() || !wxFileName::FileExists( mname ) )
        {
            // the error path will return an empty name sometimes, at least report back the original filename
            if( mname.empty() )
                mname = fp_model.m_Filename;

            ReportMessage( wxString::Format( wxT( "Could not add 3D model to %s.\n"
                                                  "File not found: %s\n" ),
                                             aFootprint->GetReference(), mname ) );
            continue;
        }

        std::string fname( mname.ToUTF8() );
        std::string refName( aFootprint->GetReference().ToUTF8() );
        try
        {
            bool bottomSide = aFootprint->GetLayer() == B_Cu;

            // the rotation is stored in degrees but opencascade wants radians
            VECTOR3D modelRot = fp_model.m_Rotation;
            modelRot *= M_PI;
            modelRot /= 180.0;

            if( m_pcbModel->AddComponent( fname, refName, bottomSide,
                                          newpos,
                                          aFootprint->GetOrientation().AsRadians(),
                                          fp_model.m_Offset, modelRot,
                                          fp_model.m_Scale, m_params.m_SubstModels ) )
            {
                hasdata = true;
            }
        }
        catch( const Standard_Failure& e )
        {
            ReportMessage( wxString::Format( wxT( "Could not add 3D model to %s.\n"
                                                  "OpenCASCADE error: %s\n" ),
                                             aFootprint->GetReference(), e.GetMessageString() ) );
        }

    }

    return hasdata;
}


bool EXPORTER_STEP::buildTrack3DShape( PCB_TRACK* aTrack, VECTOR2D aOrigin )
{
    bool skipCopper = !m_params.m_ExportTracksVias
                      || ( !m_params.m_NetFilter.IsEmpty()
                           && !aTrack->GetNetname().Matches( m_params.m_NetFilter ) );

    int maxError = m_board->GetDesignSettings().m_MaxError;

    if( m_params.m_ExportSoldermask )
    {
        if( aTrack->IsOnLayer( F_Mask ) )
        {
            SHAPE_POLY_SET poly;
            aTrack->TransformShapeToPolygon( poly, F_Mask, 0, maxError, ERROR_INSIDE );

            m_poly_shapes[F_Mask].Append( poly );
        }

        if( aTrack->IsOnLayer( B_Mask ) )
        {
            SHAPE_POLY_SET poly;
            aTrack->TransformShapeToPolygon( poly, B_Mask, 0, maxError, ERROR_INSIDE );

            m_poly_shapes[B_Mask].Append( poly );
        }
    }

    if( aTrack->Type() == PCB_VIA_T )
    {
        PCB_VIA* via = static_cast<PCB_VIA*>( aTrack );

        std::shared_ptr<SHAPE_SEGMENT> holeShape = via->GetEffectiveHoleShape();
        SHAPE_POLY_SET                 holePoly;
        holeShape->TransformToPolygon( holePoly, maxError, ERROR_INSIDE );

        LSET layers( via->GetLayerSet() & m_layersToExport );

        PCB_LAYER_ID top_layer, bot_layer;
        via->LayerPair( &top_layer, &bot_layer );

        if( !skipCopper )
        {
            for( PCB_LAYER_ID pcblayer : layers.Seq() )
            {
                const std::shared_ptr<SHAPE>& shape = via->GetEffectiveShape( pcblayer );

                SHAPE_POLY_SET poly;
                shape->TransformToPolygon( poly, maxError, ERROR_INSIDE );
                m_poly_shapes[pcblayer].Append( poly );
                m_poly_holes[pcblayer].Append( holePoly );
            }

            m_pcbModel->AddBarrel( *holeShape, top_layer, bot_layer, true, aOrigin );
        }

        //// Cut holes in silkscreen (buggy: insufficient polyset self-intersection checking)
        //if( m_layersToExport.Contains( F_SilkS ) || m_layersToExport.Contains( B_SilkS ) )
        //{
        //    m_poly_holes[F_SilkS].Append( holePoly );
        //    m_poly_holes[B_SilkS].Append( holePoly );
        //}

        m_pcbModel->AddHole( *holeShape, m_platingThickness, top_layer, bot_layer, true, aOrigin,
                             !m_params.m_FillAllVias, m_params.m_CutViasInBody );

        return true;
    }

    if( skipCopper )
        return true;

    PCB_LAYER_ID pcblayer = aTrack->GetLayer();

    if( !m_layersToExport.Contains( pcblayer ) )
        return false;

    aTrack->TransformShapeToPolygon( m_poly_shapes[pcblayer], pcblayer, 0, maxError, ERROR_INSIDE );

    return true;
}


void EXPORTER_STEP::buildZones3DShape( VECTOR2D aOrigin )
{
    for( ZONE* zone : m_board->Zones() )
    {
        LSET layers = zone->GetLayerSet();

        if( ( layers & LSET::AllCuMask() ).count() && !m_params.m_NetFilter.IsEmpty()
            && !zone->GetNetname().Matches( m_params.m_NetFilter ) )
        {
            continue;
        }

        for( PCB_LAYER_ID layer : layers.Seq() )
        {
            SHAPE_POLY_SET fill_shape;
            zone->TransformSolidAreasShapesToPolygon( layer, fill_shape );
            fill_shape.Unfracture();

            fill_shape.SimplifyOutlines( ADVANCED_CFG::GetCfg().m_TriangulateSimplificationLevel );

            m_poly_shapes[layer].Append( fill_shape );
        }
    }
}


bool EXPORTER_STEP::buildGraphic3DShape( BOARD_ITEM* aItem, VECTOR2D aOrigin )
{
    PCB_LAYER_ID pcblayer = aItem->GetLayer();

    if( !m_layersToExport.Contains( pcblayer ) )
        return false;

    if( IsCopperLayer( pcblayer ) && !m_params.m_ExportTracksVias )
        return false;

    if( IsInnerCopperLayer( pcblayer ) && !m_params.m_ExportInnerCopper )
        return false;

    int maxError = m_board->GetDesignSettings().m_MaxError;

    switch( aItem->Type() )
    {
    case PCB_SHAPE_T:
    {
        PCB_SHAPE* graphic = static_cast<PCB_SHAPE*>( aItem );

        if( IsCopperLayer( pcblayer ) && !m_params.m_NetFilter.IsEmpty()
            && !graphic->GetNetname().Matches( m_params.m_NetFilter ) )
        {
            return true;
        }

        graphic->TransformShapeToPolygon( m_poly_shapes[pcblayer], pcblayer, 0, maxError,
                                          ERROR_INSIDE );

        break;
    }

    case PCB_TEXT_T:
    {
        PCB_TEXT* text = static_cast<PCB_TEXT*>( aItem );

        text->TransformTextToPolySet( m_poly_shapes[pcblayer], 0, maxError, ERROR_INSIDE );
        break;
    }

    case PCB_TEXTBOX_T:
    {
        PCB_TEXTBOX* textbox = static_cast<PCB_TEXTBOX*>( aItem );

        textbox->TransformTextToPolySet( m_poly_shapes[pcblayer], 0, maxError, ERROR_INSIDE );
        break;
    }

    case PCB_TABLE_T:
        // JEY TODO: tables
        break;

    default: wxFAIL_MSG( "buildGraphic3DShape: unhandled item type" );
    }

    return true;
}


void EXPORTER_STEP::initOutputVariant()
{
    // Specialize the STEP_PCB_MODEL generator for specific output format
    // it can have some minor actions for the generator
    switch( m_params.m_Format )
    {
        case EXPORTER_STEP_PARAMS::FORMAT::STEP:
            m_pcbModel->SpecializeVariant( OUTPUT_FORMAT::FMT_OUT_STEP );
            break;

        case EXPORTER_STEP_PARAMS::FORMAT::BREP:
            m_pcbModel->SpecializeVariant( OUTPUT_FORMAT::FMT_OUT_BREP );
            break;

        case EXPORTER_STEP_PARAMS::FORMAT::XAO:
            m_pcbModel->SpecializeVariant( OUTPUT_FORMAT::FMT_OUT_XAO );
            break;

        case EXPORTER_STEP_PARAMS::FORMAT::GLB:
            m_pcbModel->SpecializeVariant( OUTPUT_FORMAT::FMT_OUT_GLTF );
            break;

        case EXPORTER_STEP_PARAMS::FORMAT::PLY:
            m_pcbModel->SpecializeVariant( OUTPUT_FORMAT::FMT_OUT_PLY );
            break;

        case EXPORTER_STEP_PARAMS::FORMAT::STL:
            m_pcbModel->SpecializeVariant( OUTPUT_FORMAT::FMT_OUT_STL );
            break;

        default:
            m_pcbModel->SpecializeVariant( OUTPUT_FORMAT::FMT_OUT_UNKNOWN );
            break;
    }
}


bool EXPORTER_STEP::buildBoard3DShapes()
{
    if( m_pcbModel )
        return true;

    SHAPE_POLY_SET pcbOutlines; // stores the board main outlines

    if( !m_board->GetBoardPolygonOutlines( pcbOutlines,
                                           /* error handler */ nullptr,
                                           /* allows use arcs in outlines */ true ) )
    {
        wxLogWarning( _( "Board outline is malformed. Run DRC for a full analysis." ) );
    }

    VECTOR2D origin;

    // Determine the coordinate system reference:
    // Precedence of reference point is Drill Origin > Grid Origin > User Offset
    if( m_params.m_UseDrillOrigin )
        origin = m_board->GetDesignSettings().GetAuxOrigin();
    else if( m_params.m_UseGridOrigin )
        origin = m_board->GetDesignSettings().GetGridOrigin();
    else
        origin = m_params.m_Origin;

    m_pcbModel = std::make_unique<STEP_PCB_MODEL>( m_pcbBaseName );

    initOutputVariant();

    m_pcbModel->SetCopperColor( m_copperColor.r, m_copperColor.g, m_copperColor.b );
    m_pcbModel->SetPadColor( m_padColor.r, m_padColor.g, m_padColor.b );

    m_pcbModel->SetStackup( m_board->GetStackupOrDefault() );
    m_pcbModel->SetEnabledLayers( m_layersToExport );
    m_pcbModel->SetFuseShapes( m_params.m_FuseShapes );
    m_pcbModel->SetNetFilter( m_params.m_NetFilter );

    // Note: m_params.m_BoardOutlinesChainingEpsilon is used only to build the board outlines,
    // not to set OCC chaining epsilon (much smaller)
    //
    // Set the min distance between 2 points for OCC to see these 2 points as merged
    // OCC_MAX_DISTANCE_TO_MERGE_POINTS is acceptable for OCC, otherwise there are issues
    // to handle the shapes chaining on copper layers, because the Z dist is 0.035 mm and the
    // min dist must be much smaller (we use 0.001 mm giving good results)
    m_pcbModel->OCCSetMergeMaxDistance( OCC_MAX_DISTANCE_TO_MERGE_POINTS );

    m_pcbModel->SetMaxError( m_board->GetDesignSettings().m_MaxError );

    // For copper layers, only pads and tracks are added, because adding everything on copper
    // generate unreasonable file sizes and take a unreasonable calculation time.
    for( FOOTPRINT* fp : m_board->Footprints() )
        buildFootprint3DShapes( fp, origin );

    for( PCB_TRACK* track : m_board->Tracks() )
        buildTrack3DShape( track, origin );

    for( BOARD_ITEM* item : m_board->Drawings() )
        buildGraphic3DShape( item, origin );

    if( m_params.m_ExportZones )
    {
        buildZones3DShape( origin );
    }

    SHAPE_POLY_SET pcbOutlinesNoArcs = pcbOutlines;
    pcbOutlinesNoArcs.ClearArcs();

    for( PCB_LAYER_ID pcblayer : m_layersToExport.Seq() )
    {
        SHAPE_POLY_SET poly = m_poly_shapes[pcblayer];
        poly.Simplify();

        poly.SimplifyOutlines( pcbIUScale.mmToIU( 0.003 ) );
        poly.Simplify();

        SHAPE_POLY_SET holes = m_poly_holes[pcblayer];
        holes.Simplify();

        // Mask layer is negative
        if( pcblayer == F_Mask || pcblayer == B_Mask )
        {
            SHAPE_POLY_SET mask = pcbOutlinesNoArcs;

            mask.BooleanSubtract( poly );
            mask.BooleanSubtract( holes );

            poly = mask;
        }
        else
        {
            // Subtract holes
            poly.BooleanSubtract( holes );

            // Clip to board outline
            poly.BooleanIntersection( pcbOutlinesNoArcs );
        }

        m_pcbModel->AddPolygonShapes( &poly, pcblayer, origin );
    }

    ReportMessage( wxT( "Create PCB solid model\n" ) );

    wxString msg;
    msg.Printf( wxT( "Board outline: find %d initial points\n" ), pcbOutlines.FullPointCount() );
    ReportMessage( msg );

    if( !m_pcbModel->CreatePCB( pcbOutlines, origin, m_params.m_ExportBoardBody ) )
    {
        ReportMessage( wxT( "could not create PCB solid model\n" ) );
        return false;
    }

    return true;
}


bool EXPORTER_STEP::Export()
{
    // Display the export time, for statistics
    int64_t stats_startExportTime = GetRunningMicroSecs();

    // setup opencascade message log
    Message::DefaultMessenger()->RemovePrinters( STANDARD_TYPE( Message_PrinterOStream ) );
    Message::DefaultMessenger()->AddPrinter( new KiCadPrinter( this ) );

    ReportMessage( _( "Determining PCB data\n" ) );

    if( m_params.m_OutputFile.IsEmpty() )
    {
        wxFileName fn = m_board->GetFileName();
        fn.SetName( fn.GetName() );
        fn.SetExt( m_params.GetDefaultExportExtension() );

        m_params.m_OutputFile = fn.GetFullName();
    }

    m_layersToExport = LSET::ExternalCuMask();

    if( m_params.m_ExportInnerCopper )
        m_layersToExport |= LSET::InternalCuMask();

    if( m_params.m_ExportSilkscreen )
    {
        m_layersToExport.set( F_SilkS );
        m_layersToExport.set( B_SilkS );
    }

    if( m_params.m_ExportSoldermask )
    {
        m_layersToExport.set( F_Mask );
        m_layersToExport.set( B_Mask );
    }

    m_layersToExport &= m_board->GetEnabledLayers();

    try
    {
        ReportMessage( wxString::Format( _( "Build %s data\n" ), m_params.GetFormatName() ) );

        if( !buildBoard3DShapes() )
        {
            ReportMessage( _( "\n** Error building STEP board model. Export aborted. **\n" ) );
            return false;
        }

        ReportMessage( wxString::Format( _( "Writing %s file\n" ), m_params.GetFormatName() ) );

        bool success = true;
        if( m_params.m_Format == EXPORTER_STEP_PARAMS::FORMAT::STEP )
            success = m_pcbModel->WriteSTEP( m_outputFile, m_params.m_OptimizeStep );
        else if( m_params.m_Format == EXPORTER_STEP_PARAMS::FORMAT::BREP )
            success = m_pcbModel->WriteBREP( m_outputFile );
        else if( m_params.m_Format == EXPORTER_STEP_PARAMS::FORMAT::XAO )
            success = m_pcbModel->WriteXAO( m_outputFile );
        else if( m_params.m_Format == EXPORTER_STEP_PARAMS::FORMAT::GLB )
            success = m_pcbModel->WriteGLTF( m_outputFile );
        else if( m_params.m_Format == EXPORTER_STEP_PARAMS::FORMAT::PLY )
            success = m_pcbModel->WritePLY( m_outputFile );
        else if( m_params.m_Format == EXPORTER_STEP_PARAMS::FORMAT::STL )
            success = m_pcbModel->WriteSTL( m_outputFile );

        if( !success )
        {
            ReportMessage( wxString::Format( _( "\n** Error writing %s file. **\n" ),
                                             m_params.GetFormatName() ) );
            return false;
        }
        else
        {
            ReportMessage( wxString::Format( _( "%s file '%s' created.\n" ),
                                             m_params.GetFormatName(), m_outputFile ) );
        }
    }
    catch( const Standard_Failure& e )
    {
        ReportMessage( e.GetMessageString() );
        ReportMessage( wxString::Format( _( "\n** Error exporting %s file. Export aborted. **\n" ),
                                         m_params.GetFormatName() ) );
        return false;
    }
    catch( ... )
    {
        ReportMessage( wxString::Format( _( "\n** Error exporting %s file. Export aborted. **\n" ),
                                         m_params.GetFormatName() ) );
        return false;
    }

    if( m_fail || m_error )
    {
        wxString msg;

        if( m_fail )
        {
            msg = wxString::Format( _( "Unable to create %s file.\n"
                                       "Check that the board has a valid outline and models." ),
                                    m_params.GetFormatName() );
        }
        else if( m_error || m_warn )
        {
            msg = wxString::Format( _( "%s file has been created, but there are warnings." ),
                                    m_params.GetFormatName() );
        }

        ReportMessage( msg );
    }

    // Display calculation time in seconds
    double calculation_time = (double)( GetRunningMicroSecs() - stats_startExportTime) / 1e6;
    ReportMessage( wxString::Format( _( "\nExport time %.3f s\n" ), calculation_time ) );

    return true;
}
