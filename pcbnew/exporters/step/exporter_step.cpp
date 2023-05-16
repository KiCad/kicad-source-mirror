/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2022 Mark Roszko <mark.roszko@gmail.com>
 * Copyright (C) 2016 Cirilo Bernardo <cirilo.bernardo@gmail.com>
 * Copyright (C) 2016-2023 KiCad Developers, see AUTHORS.txt for contributors.
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
#include <board.h>
#include <board_design_settings.h>
#include <footprint.h>
#include <pcb_track.h>
#include <pcb_shape.h>
#include <pad.h>
#include <fp_lib_table.h>
#include "step_pcb_model.h"

#include <pgm_base.h>
#include <base_units.h>
#include <filename_resolver.h>
#include <trace_helpers.h>

#include <Message.hxx>                // OpenCascade messenger
#include <Message_PrinterOStream.hxx> // OpenCascade output messenger
#include <Standard_Failure.hxx>       // In open cascade

#include <Standard_Version.hxx>

#include <wx/crt.h>
#include <wx/log.h>

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
    m_hasDrillOrigin( false ),
    m_hasGridOrigin( false ),
    m_board( aBoard ),
    m_pcbModel( nullptr ),
    m_boardThickness( DEFAULT_BOARD_THICKNESS_MM )
{
    m_solderMaskColor = COLOR4D( 0.08, 0.20, 0.14, 0.83 );
    m_copperColor = COLOR4D( 0.7, 0.61, 0.0, 1.0 );

    // Init m_pcbBaseName to the board short filename (no path, no ext)
    // m_pcbName is used later to identify items in step file
    wxFileName fn( aBoard->GetFileName() );
    m_pcbBaseName = fn.GetName();

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
    bool hasdata = false;

    // Dump the pad holes into the PCB
    for( PAD* pad : aFootprint->Pads() )
    {
        if( m_pcbModel->AddPadHole( pad, aOrigin ) )
            hasdata = true;

        if( ExportTracksAndVias() )
        {
            if( m_pcbModel->AddPadShape( pad, aOrigin ) )
                hasdata = true;
        }
    }

    // Build 3D shapes of the footprint graphic items on external layers:
    if( ExportTracksAndVias() )
    {
        int maxError = m_board->GetDesignSettings().m_MaxError;
        aFootprint->TransformFPShapesToPolySet( m_top_copper_shapes, F_Cu, 0, maxError, ERROR_INSIDE,
                                               false, /* include text */
                                               true, /* include shapes */
                                               false /* include private items */ );
        aFootprint->TransformFPShapesToPolySet( m_bottom_copper_shapes, B_Cu, 0, maxError, ERROR_INSIDE,
                                               false, /* include text */
                                               true, /* include shapes */
                                               false /* include private items */ );
    }

    if( ( !(aFootprint->GetAttributes() & (FP_THROUGH_HOLE|FP_SMD)) ) && !m_params.m_includeUnspecified )
    {
        return hasdata;
    }

    if( ( aFootprint->GetAttributes() & FP_DNP ) && !m_params.m_includeDNP )
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
                    m_board->GetProject()->PcbFootprintLibs()->FindRow( libraryName, false );

            if( fpRow )
                footprintBasePath = fpRow->GetFullURI( true );
        }
        catch( ... )
        {
            // Do nothing if the libraryName is not found in lib table
        }
    }

    // Exit early if we don't want to include footprint models
    if( m_params.m_boardOnly )
    {
        return hasdata;
    }

    VECTOR2D newpos( pcbIUScale.IUTomm( posX ), pcbIUScale.IUTomm( posY ) );

    for( const FP_3DMODEL& fp_model : aFootprint->Models() )
    {
        if( !fp_model.m_Show || fp_model.m_Filename.empty() )

            continue;

        std::vector<wxString> searchedPaths;
        wxString mname = m_resolver->ResolvePath( fp_model.m_Filename, footprintBasePath );


        if( !wxFileName::FileExists( mname ) )
        {
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
                                          fp_model.m_Scale, m_params.m_substModels ) )
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
    if( aTrack->Type() == PCB_VIA_T )
    {
        PAD dummy( nullptr );
        int hole = static_cast<PCB_VIA*>( aTrack )->GetDrillValue();
        dummy.SetDrillSize( VECTOR2I( hole, hole ) );
        dummy.SetPosition( aTrack->GetStart() );
        dummy.SetSize( VECTOR2I( aTrack->GetWidth(), aTrack->GetWidth() ) );

        if( m_pcbModel->AddPadHole( &dummy, aOrigin ) )
        {
            if( m_pcbModel->AddPadShape( &dummy, aOrigin ) )
                return false;
        }

        return true;
    }

    PCB_LAYER_ID pcblayer = aTrack->GetLayer();

    if( pcblayer != F_Cu && pcblayer != B_Cu )
        return false;

    int maxError = m_board->GetDesignSettings().m_MaxError;

    if( pcblayer == F_Cu )
        aTrack->TransformShapeToPolygon( m_top_copper_shapes, pcblayer, 0, maxError, ERROR_INSIDE );
    else
        aTrack->TransformShapeToPolygon( m_bottom_copper_shapes, pcblayer, 0, maxError, ERROR_INSIDE );

    return true;
}


bool EXPORTER_STEP::buildGraphic3DShape( BOARD_ITEM* aItem, VECTOR2D aOrigin )
{
    PCB_SHAPE* graphic = dynamic_cast<PCB_SHAPE*>( aItem );

    if( ! graphic )
        return false;

    PCB_LAYER_ID pcblayer = graphic->GetLayer();

    if( pcblayer != F_Cu && pcblayer != B_Cu )
        return false;

    SHAPE_POLY_SET copper_shapes;
    int maxError = m_board->GetDesignSettings().m_MaxError;


    if( pcblayer == F_Cu )
        graphic->TransformShapeToPolygon( m_top_copper_shapes, pcblayer, 0,
                                          maxError, ERROR_INSIDE );
    else
        graphic->TransformShapeToPolygon( m_bottom_copper_shapes, pcblayer, 0,
                                          maxError, ERROR_INSIDE );

    return true;
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
    if( m_params.m_useDrillOrigin )
        origin = m_board->GetDesignSettings().GetAuxOrigin();
    else if( m_params.m_useGridOrigin )
        origin = m_board->GetDesignSettings().GetGridOrigin();
    else
        origin = m_params.m_origin;

    m_pcbModel = std::make_unique<STEP_PCB_MODEL>( m_pcbBaseName );

    // TODO: Handle when top & bottom soldermask colours are different...
    m_pcbModel->SetBoardColor( m_solderMaskColor.r, m_solderMaskColor.g, m_solderMaskColor.b );
    m_pcbModel->SetCopperColor( m_copperColor.r, m_copperColor.g, m_copperColor.b );

    m_pcbModel->SetPCBThickness( m_boardThickness );

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

    if( ExportTracksAndVias() )
    {
        for( PCB_TRACK* track : m_board->Tracks() )
            buildTrack3DShape( track, origin );

        for( BOARD_ITEM* item : m_board->Drawings() )
            buildGraphic3DShape( item, origin );
    }

    m_top_copper_shapes.Fracture( SHAPE_POLY_SET::PM_FAST );
    m_bottom_copper_shapes.Fracture( SHAPE_POLY_SET::PM_FAST );

    m_pcbModel->AddCopperPolygonShapes( &m_top_copper_shapes, true, origin );
    m_pcbModel->AddCopperPolygonShapes( &m_bottom_copper_shapes, false, origin );

    ReportMessage( wxT( "Create PCB solid model\n" ) );

    wxString msg;
    msg.Printf( wxT( "Board outline: find %d initial points\n" ), pcbOutlines.FullPointCount() );
    ReportMessage( msg );

    if( !m_pcbModel->CreatePCB( pcbOutlines, origin ) )
    {
        ReportMessage( wxT( "could not create PCB solid model\n" ) );
        return false;
    }

    return true;
}


void EXPORTER_STEP::calculatePcbThickness()
{
    m_boardThickness = DEFAULT_BOARD_THICKNESS_MM;

    const BOARD_DESIGN_SETTINGS& bds = m_board->GetDesignSettings();

    if( bds.GetStackupDescriptor().GetCount() )
    {
        int thickness = 0;

        for( BOARD_STACKUP_ITEM* item : bds.GetStackupDescriptor().GetList() )
        {
            switch( item->GetType() )
            {
            case BS_ITEM_TYPE_DIELECTRIC:
                // Dielectric can have sub-layers. Layer 0 is the main layer
                // Not frequent, but possible
                for( int idx = 0; idx < item->GetSublayersCount(); idx++ )
                    thickness += item->GetThickness( idx );

                break;

            case BS_ITEM_TYPE_COPPER:
                if( item->IsEnabled() )
                    thickness += item->GetThickness();

                break;

            default:
                break;
            }
        }

        if( thickness > 0 )
            m_boardThickness = pcbIUScale.IUTomm( thickness );
    }
}


bool EXPORTER_STEP::Export()
{
    // setup opencascade message log
    Message::DefaultMessenger()->RemovePrinters( STANDARD_TYPE( Message_PrinterOStream ) );
    Message::DefaultMessenger()->AddPrinter( new KiCadPrinter( this ) );

    ReportMessage( _( "Determining PCB data\n" ) );
    calculatePcbThickness();
    wxString msg;
    msg.Printf( _( "Board Thickness from stackup: %.3f mm\n" ), m_boardThickness );
    ReportMessage( msg );

    try
    {
        ReportMessage( _( "Build STEP data\n" ) );

        if( !buildBoard3DShapes() )
        {
            ReportMessage( _( "\n** Error building STEP board model. Export aborted. **\n" ) );
            return false;
        }

        ReportMessage( _( "Writing STEP file\n" ) );

        if( !m_pcbModel->WriteSTEP( m_outputFile ) )
        {
            ReportMessage( _( "\n** Error writing STEP file. **\n" ) );
            return false;
        }
        else
        {
            ReportMessage( wxString::Format( _( "\nSTEP file '%s' created.\n" ), m_outputFile ) );
        }
    }
    catch( const Standard_Failure& e )
    {
        ReportMessage( e.GetMessageString() );
        ReportMessage( _( "\n** Error exporting STEP file. Export aborted. **\n" ) );
        return false;
    }
    catch( ... )
    {
        ReportMessage( _( "\n** Error exporting STEP file. Export aborted. **\n" ) );
        return false;
    }

    if( m_fail || m_error )
    {
        if( m_fail )
        {
            msg = _( "Unable to create STEP file.\n"
                     "Check that the board has a valid outline and models." );
        }
        else if( m_error || m_warn )
        {
            msg = _( "STEP file has been created, but there are warnings." );
        }

        ReportMessage( msg );
    }

    return true;
}
