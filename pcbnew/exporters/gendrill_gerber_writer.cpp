/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2017 Jean_Pierre Charras <jp.charras at wanadoo.fr>
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

/**
 * @file gendrill_gerber_writer.cpp
 * @brief Functions to create drill files in gerber X2 format.
 */

#include <plotters/plotter_gerber.h>
#include <string_utils.h>
#include <locale_io.h>
#include <board.h>
#include <footprint.h>
#include <pcb_track.h>
#include <pad.h>
#include <pcbplot.h>
#include <gendrill_gerber_writer.h>
#include <reporter.h>
#include <gbr_metadata.h>

// set to 1 to use flashed oblong holes, 0 to draw them by a line (route holes).
// WARNING: currently ( gerber-layer-format-specification-revision-2023-08 ),
// oblong holes **must be routed* in a drill file and not flashed,
// so set FLASH_OVAL_HOLE to 0
#define FLASH_OVAL_HOLE 0


GERBER_WRITER::GERBER_WRITER( BOARD* aPcb )
    : GENDRILL_WRITER_BASE( aPcb )
{
    m_zeroFormat      = SUPPRESS_LEADING;
    m_conversionUnits = 1.0;
    m_unitsMetric    = true;
    m_drillFileExtension = wxT( "gbr" );
    m_merge_PTH_NPTH = false;
}


bool GERBER_WRITER::CreateDrillandMapFilesSet( const wxString& aPlotDirectory, bool aGenDrill,
                                               bool aGenMap, bool aGenTenting, REPORTER* aReporter )
{
    bool success = true;
    // Note: In Gerber drill files, NPTH and PTH are always separate files
    m_merge_PTH_NPTH = false;

    wxFileName  fn;
    wxString    msg;

    std::vector<DRILL_SPAN> hole_sets = getUniqueLayerPairs();

    hole_sets.emplace_back( F_Cu, B_Cu, false, true );

    for( std::vector<DRILL_SPAN>::const_iterator it = hole_sets.begin();
         it != hole_sets.end();  ++it )
    {
        const DRILL_SPAN& span = *it;
        bool doing_npth = span.m_IsNonPlatedFile;

        buildHolesList( span, doing_npth );

        if( getHolesCount() == 0 )
            continue;

        fn = getDrillFileName( span, doing_npth, m_merge_PTH_NPTH );
        fn.SetPath( aPlotDirectory );

        if( aGenDrill )
        {
            wxString fullFilename = fn.GetFullPath();
            bool     isNonPlated = doing_npth || span.m_IsBackdrill;
            bool     wroteDrillFile = false;

            int result = createDrillFile( fullFilename, isNonPlated, span );

            if( result < 0 )
            {
                if( aReporter )
                {
                    msg.Printf( _( "Failed to create file '%s'." ), fullFilename );
                    aReporter->Report( msg, RPT_SEVERITY_ERROR );
                }

                success = false;
                break;
            }

            wroteDrillFile = true;

            if( aReporter )
            {
                msg.Printf( _( "Created file '%s'." ), fullFilename );
                aReporter->Report( msg, RPT_SEVERITY_ACTION );
            }

            if( wroteDrillFile && span.m_IsBackdrill )
            {
                if( !writeBackdrillLayerPairFile( aPlotDirectory, aReporter, span ) )
                {
                    success = false;
                    break;
                }
            }
        }

        if( doing_npth )
            continue;

        for( IPC4761_FEATURES feature :
             { IPC4761_FEATURES::FILLED, IPC4761_FEATURES::CAPPED,
               IPC4761_FEATURES::COVERED_BACK, IPC4761_FEATURES::COVERED_FRONT,
               IPC4761_FEATURES::PLUGGED_BACK, IPC4761_FEATURES::PLUGGED_FRONT,
               IPC4761_FEATURES::TENTED_BACK, IPC4761_FEATURES::TENTED_FRONT } )
        {
            if( !aGenTenting )
            {
                if( feature == IPC4761_FEATURES::TENTED_BACK
                        || feature == IPC4761_FEATURES::TENTED_FRONT )
                {
                    continue;
                }
            }

            if( !hasViaType( feature ) )
                continue;

            fn = getProtectionFileName( span, feature );
            fn.SetPath( aPlotDirectory );

            wxString fullFilename = fn.GetFullPath();

            if( createProtectionFile( fullFilename, feature, span.Pair() ) < 0 )
            {
                if( aReporter )
                {
                    msg.Printf( _( "Failed to create file '%s'." ), fullFilename );
                    aReporter->Report( msg, RPT_SEVERITY_ERROR );
                    success = false;
                }
            }
            else
            {
                if( aReporter )
                {
                    msg.Printf( _( "Created file '%s'." ), fullFilename );
                    aReporter->Report( msg, RPT_SEVERITY_ACTION );
                }
            }
        }
    }

    if( aGenMap )
        success &= CreateMapFilesSet( aPlotDirectory, aReporter );

    if( aReporter )
        aReporter->ReportTail( _( "Done." ), RPT_SEVERITY_INFO );

    return success;
}


#if !FLASH_OVAL_HOLE
// A helper class to transform an oblong hole to a segment
static void convertOblong2Segment( const VECTOR2I& aSize, const EDA_ANGLE& aOrient, VECTOR2I& aStart, VECTOR2I& aEnd );
#endif

int GERBER_WRITER::createProtectionFile( const wxString& aFullFilename, IPC4761_FEATURES aFeature,
                                         DRILL_LAYER_PAIR aLayerPair )
{
    GERBER_PLOTTER plotter;
    // Gerber drill file imply X2 format:
    plotter.UseX2format( true );
    plotter.UseX2NetAttributes( true );
    plotter.DisableApertMacros( false );

    // Add the standard X2 header, without FileFunction
    AddGerberX2Header( &plotter, m_pcb );
    plotter.SetViewport( m_offset, pcbIUScale.IU_PER_MILS / 10, /* scale */ 1.0,
                         /* mirror */ false );

    // has meaning only for gerber plotter. Must be called only after SetViewport
    plotter.SetGerberCoordinatesFormat( 6 );
    plotter.SetCreator( wxT( "PCBNEW" ) );

    // Add the standard X2 FileFunction for drill files
    // %TF.FileFunction,Plated[NonPlated],layer1num,layer2num,PTH[NPTH][Blind][Buried],Drill[Rout][Mixed]*%
    wxString text = "%TF,FileFunction,Other,";

    std::string attrib;
    switch( aFeature )
    {
    case IPC4761_FEATURES::CAPPED:
        text << wxT( "Capping" );
        attrib = "Capping";
        break;
    case IPC4761_FEATURES::FILLED:
        text << wxT( "Filling" );
        attrib = "Filling";
        break;
    case IPC4761_FEATURES::COVERED_BACK:
        text << wxT( "Covering-Back" );
        attrib = "Covering";
        break;
    case IPC4761_FEATURES::COVERED_FRONT:
        text << wxT( "Covering-Front" );
        attrib = "Covering";
        break;
    case IPC4761_FEATURES::PLUGGED_BACK:
        text << wxT( "Plugging-Back" );
        attrib = "Plugging";
        break;
    case IPC4761_FEATURES::PLUGGED_FRONT:
        text << wxT( "Plugging-Front" );
        attrib = "Plugging";
        break;
    case IPC4761_FEATURES::TENTED_BACK:
        text << wxT( "Tenting-Back" );
        attrib = "Tenting";
        break;
    case IPC4761_FEATURES::TENTED_FRONT:
        text << wxT( "Tenting-Front" );
        attrib = "Tenting";
        break;
    default: return -1;
    }
    text << wxT( "*%" );
    plotter.AddLineToHeader( text );

    // Add file polarity (positive)
    text = wxT( "%TF.FilePolarity,Positive*%" );
    plotter.AddLineToHeader( text );


    if( !plotter.OpenFile( aFullFilename ) )
        return -1;

    plotter.StartPlot( wxT( "1" ) );

    int holes_count = 0;

    for( auto& hole_descr : m_holeListBuffer )
    {
        if( !dyn_cast<const PCB_VIA*>( hole_descr.m_ItemParent ) )
        {
            continue;
        }

        const PCB_VIA* via = dyn_cast<const PCB_VIA*>( hole_descr.m_ItemParent );

        bool cont = false;
        int  diameter = hole_descr.m_Hole_Diameter;
        // clang-format off: suggestion is inconsitent
        switch( aFeature )
        {
        case IPC4761_FEATURES::FILLED:
            cont = ! hole_descr.m_Hole_Filled;
            break;
        case IPC4761_FEATURES::CAPPED:
            cont = ! hole_descr.m_Hole_Capped;
            break;
        case IPC4761_FEATURES::COVERED_BACK:
            cont = !hole_descr.m_Hole_Bot_Covered;
            diameter = via->GetWidth( via->BottomLayer() );
            break;
        case IPC4761_FEATURES::COVERED_FRONT:
            cont = ! hole_descr.m_Hole_Top_Covered;
            diameter = via->GetWidth( via->TopLayer() );
            break;
        case IPC4761_FEATURES::PLUGGED_BACK:
            cont = !hole_descr.m_Hole_Bot_Plugged;
            break;
        case IPC4761_FEATURES::PLUGGED_FRONT:
            cont = ! hole_descr.m_Hole_Top_Plugged;
            break;
        case IPC4761_FEATURES::TENTED_BACK:
            cont = ! hole_descr.m_Hole_Bot_Tented;
            diameter = via->GetWidth( via->BottomLayer() );
            break;
        case IPC4761_FEATURES::TENTED_FRONT:
            cont = ! hole_descr.m_Hole_Top_Tented;
            diameter = via->GetWidth( via->TopLayer() );
            break;
        }
        // clang-format on: suggestion is inconsitent

        if( cont )
            continue;

        GBR_METADATA gbr_metadata;

        gbr_metadata.SetApertureAttrib( attrib );

        plotter.FlashPadCircle( hole_descr.m_Hole_Pos, diameter, &gbr_metadata );

        holes_count++;
    }

    plotter.EndPlot();

    return holes_count;
}

int GERBER_WRITER::createDrillFile( wxString& aFullFilename, bool aIsNpth,
                                    const DRILL_SPAN& aSpan )
{
    int    holes_count;

    LOCALE_IO dummy;    // Use the standard notation for double numbers

    GERBER_PLOTTER plotter;

    // Gerber drill file imply X2 format:
    plotter.UseX2format( true );
    plotter.UseX2NetAttributes( true );
    plotter.DisableApertMacros( false );

    // Add the standard X2 header, without FileFunction
    AddGerberX2Header( &plotter, m_pcb );
    plotter.SetViewport( m_offset, pcbIUScale.IU_PER_MILS/10, /* scale */ 1.0, /* mirror */false );

    // has meaning only for gerber plotter. Must be called only after SetViewport
    plotter.SetGerberCoordinatesFormat( 6 );
    plotter.SetCreator( wxT( "PCBNEW" ) );

    // Add the standard X2 FileFunction for drill files
    // %TF.FileFunction,Plated[NonPlated],layer1num,layer2num,PTH[NPTH][Blind][Buried],Drill[Rout][Mixed]*%
    wxString text = BuildFileFunctionAttributeString( aSpan,
                                                      aIsNpth ? TYPE_FILE::NPTH_FILE
                                                              : TYPE_FILE::PTH_FILE );
    plotter.AddLineToHeader( text );

    // Add file polarity (positive)
    text = wxT( "%TF.FilePolarity,Positive*%" );
    plotter.AddLineToHeader( text );

    if( !plotter.OpenFile( aFullFilename ) )
        return -1;

    plotter.StartPlot( wxT( "1" ) );

    holes_count = 0;

    VECTOR2I hole_pos;
    bool last_item_is_via = true;   // a flag to clear object attributes when a via hole is created.

    for( unsigned ii = 0; ii < m_holeListBuffer.size(); ii++ )
    {
        HOLE_INFO& hole_descr = m_holeListBuffer[ii];
        hole_pos = hole_descr.m_Hole_Pos;

        // Manage the aperture attributes: in drill files 3 attributes can be used:
        // "ViaDrill", only for vias, not pads
        // "ComponentDrill", only for Through Holes pads
        // "Slot" for oblong holes;
        GBR_METADATA gbr_metadata;

        if( dyn_cast<const PCB_VIA*>( hole_descr.m_ItemParent ) )
        {
            if( hole_descr.m_IsBackdrill )
                gbr_metadata.SetApertureAttrib( GBR_APERTURE_METADATA::GBR_APERTURE_ATTRIB_BACKDRILL );
            else
                gbr_metadata.SetApertureAttrib( GBR_APERTURE_METADATA::GBR_APERTURE_ATTRIB_VIADRILL );

            if( !last_item_is_via )
            {
                // be sure the current object attribute is cleared for vias
                plotter.EndBlock( nullptr );
            }

            last_item_is_via = true;
        }
        else if( dyn_cast<const PAD*>( hole_descr.m_ItemParent ) )
        {
            last_item_is_via = false;
            const PAD* pad = dyn_cast<const PAD*>( hole_descr.m_ItemParent );

            if( pad->GetProperty() == PAD_PROP::CASTELLATED )
            {
                gbr_metadata.SetApertureAttrib(
                        GBR_APERTURE_METADATA::GBR_APERTURE_ATTRIB_CASTELLATEDDRILL );
            }
            else if( pad->GetProperty() == PAD_PROP::PRESSFIT )
            {
                gbr_metadata.SetApertureAttrib(
                        GBR_APERTURE_METADATA::GBR_APERTURE_ATTRIB_PRESSFITDRILL );
            }
            else
            {
                // Good practice of oblong pad holes (slots) is to use a specific aperture for
                // routing, not used in drill commands.
                if( hole_descr.m_Hole_Shape )
                {
                    gbr_metadata.SetApertureAttrib(
                            GBR_APERTURE_METADATA::GBR_APERTURE_ATTRIB_CMP_OBLONG_DRILL );
                }
                else
                {
                    gbr_metadata.SetApertureAttrib(
                            GBR_APERTURE_METADATA::GBR_APERTURE_ATTRIB_CMP_DRILL );
                }
            }

            // Add object attribute: component reference to pads (mainly useful for users)
            wxString ref = pad->GetParentFootprint()->GetReference();

            gbr_metadata.SetCmpReference( ref );
            gbr_metadata.SetNetAttribType( GBR_NETLIST_METADATA::GBR_NETINFO_CMP );
        }

        if( hole_descr.m_Hole_Shape )
        {
#if FLASH_OVAL_HOLE     // set to 1 to use flashed oblong holes,
                        // 0 to draw them as a line.
            plotter.FlashPadOval( hole_pos, hole_descr.m_Hole_Size, hole_descr.m_Hole_Orient,
                                  &gbr_metadata );
#else
            // Use routing for oblong hole (Slots)
            VECTOR2I start, end;
            convertOblong2Segment( hole_descr.m_Hole_Size, hole_descr.m_Hole_Orient, start, end );
            int width = std::min( hole_descr.m_Hole_Size.x, hole_descr.m_Hole_Size.y );

            if ( width == 0 )
                continue;

            plotter.ThickSegment( start+hole_pos, end+hole_pos, width, &gbr_metadata );
#endif
        }
        else
        {
            int diam = std::min( hole_descr.m_Hole_Size.x, hole_descr.m_Hole_Size.y );
            plotter.FlashPadCircle( hole_pos, diam, &gbr_metadata );
        }

        holes_count++;
    }

    plotter.EndPlot();

    return holes_count;
}


wxFileName GERBER_WRITER::getBackdrillLayerPairFileName( const DRILL_SPAN& aSpan ) const
{
    wxFileName fn = m_pcb->GetFileName();
    wxString   pairName = wxString::FromUTF8( layerPairName( aSpan.Pair() ).c_str() );

    fn.SetName( fn.GetName() + wxT( "-" ) + pairName + wxT( "-backdrill-drl" ) );
    fn.SetExt( m_drillFileExtension );

    return fn;
}


bool GERBER_WRITER::writeBackdrillLayerPairFile( const wxString& aPlotDirectory,
                                                 REPORTER* aReporter, const DRILL_SPAN& aSpan )
{
    wxFileName fn = getBackdrillLayerPairFileName( aSpan );
    fn.SetPath( aPlotDirectory );

    wxString fullFilename = fn.GetFullPath();

    if( createDrillFile( fullFilename, true, aSpan ) < 0 )
    {
        if( aReporter )
        {
            wxString msg;
            msg.Printf( _( "Failed to create file '%s'." ), fullFilename );
            aReporter->Report( msg, RPT_SEVERITY_ERROR );
        }

        return false;
    }

    if( aReporter )
    {
        wxString msg;
        msg.Printf( _( "Created file '%s'." ), fullFilename );
        aReporter->Report( msg, RPT_SEVERITY_ACTION );
    }

    return true;
}


#if !FLASH_OVAL_HOLE
void convertOblong2Segment( const VECTOR2I& aSize, const EDA_ANGLE& aOrient, VECTOR2I& aStart,
                            VECTOR2I& aEnd )
{
    VECTOR2I    size( aSize );
    EDA_ANGLE orient( aOrient );

    /* The pad will be drawn as an oblong shape with size.y > size.x
     * (Oval vertical orientation 0)
     */
    if( size.x > size.y )
    {
        std::swap( size.x, size.y );
        orient += ANGLE_90;
    }

    int deltaxy = size.y - size.x;     // distance between centers of the oval
    aStart = VECTOR2I( 0, deltaxy / 2 );
    RotatePoint( aStart, orient );

    aEnd = VECTOR2I( 0, -deltaxy / 2 );
    RotatePoint( aEnd, orient );
}
#endif


void GERBER_WRITER::SetFormat( int aRightDigits )
{
    /* Set conversion scale depending on drill file units */
    m_conversionUnits = 1.0 / pcbIUScale.IU_PER_MM; // Gerber units = mm

    // Set precision (unit is mm).
    m_precision.m_Lhs = 4;
    m_precision.m_Rhs = aRightDigits == 6 ? 6 : 5;
}


const wxString GERBER_WRITER::getDrillFileName( const DRILL_SPAN& aSpan, bool aNPTH,
                                                bool aMerge_PTH_NPTH ) const
{
    // Gerber files extension is always .gbr.
    // Therefore, to mark drill files, add "-drl" to the filename.
    wxFileName fname( GENDRILL_WRITER_BASE::getDrillFileName( aSpan, aNPTH, aMerge_PTH_NPTH ) );
    fname.SetName( fname.GetName() + wxT( "-drl" ) );

    return fname.GetFullPath();
}


bool GERBER_WRITER::hasViaType( IPC4761_FEATURES aFeature )
{
    for( auto& hole_descr : m_holeListBuffer )
    {
        if( !dyn_cast<const PCB_VIA*>( hole_descr.m_ItemParent ) )
        {
            continue;
        }

        switch( aFeature )
        {
        case IPC4761_FEATURES::FILLED:
            if( hole_descr.m_Hole_Filled )
                return true;
            break;

        case IPC4761_FEATURES::CAPPED:
            if( hole_descr.m_Hole_Capped )
                return true;
            break;

        case IPC4761_FEATURES::COVERED_BACK:
            if( hole_descr.m_Hole_Bot_Covered )
                return true;
            break;

        case IPC4761_FEATURES::COVERED_FRONT:
            if( hole_descr.m_Hole_Top_Covered )
                return true;
            break;

        case IPC4761_FEATURES::PLUGGED_BACK:
            if( hole_descr.m_Hole_Bot_Plugged )
                return true;
            break;

        case IPC4761_FEATURES::PLUGGED_FRONT:
            if( hole_descr.m_Hole_Top_Plugged )
                return true;
            break;

        case IPC4761_FEATURES::TENTED_BACK:
            if( hole_descr.m_Hole_Bot_Tented )
                return true;
            break;

        case IPC4761_FEATURES::TENTED_FRONT:
            if( hole_descr.m_Hole_Top_Tented )
                return true;
            break;
        }
    }

    return false;
}