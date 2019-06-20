/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2017 Jean_Pierre Charras <jp.charras at wanadoo.fr>
 * Copyright (C) 1992-2017 KiCad Developers, see AUTHORS.txt for contributors.
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

#include <fctsys.h>

#include <vector>

#include <plotter.h>
#include <kicad_string.h>
#include <pcb_edit_frame.h>
#include <pgm_base.h>
#include <build_version.h>

#include <class_board.h>

#include <pcbplot.h>
#include <pcbnew.h>
#include <gendrill_gerber_writer.h>
#include <wildcards_and_files_ext.h>
#include <reporter.h>
#include <gbr_metadata.h>
#include <class_module.h>


GERBER_WRITER::GERBER_WRITER( BOARD* aPcb )
    : GENDRILL_WRITER_BASE( aPcb )
{
    m_zeroFormat      = SUPPRESS_LEADING;
    m_conversionUnits = 1.0;
    m_unitsMetric    = true;
    m_drillFileExtension = "gbr";
    m_merge_PTH_NPTH = false;
}


void GERBER_WRITER::CreateDrillandMapFilesSet( const wxString& aPlotDirectory,
                                                 bool aGenDrill, bool aGenMap,
                                                 REPORTER * aReporter )
{
    // Note: In Gerber drill files, NPTH and PTH are always separate files
    m_merge_PTH_NPTH = false;

    wxFileName  fn;
    wxString    msg;

    std::vector<DRILL_LAYER_PAIR> hole_sets = getUniqueLayerPairs();

    // append a pair representing the NPTH set of holes, for separate drill files.
    // (Gerber drill files are separate files for PTH and NPTH)
    hole_sets.push_back( DRILL_LAYER_PAIR( F_Cu, B_Cu ) );

    for( std::vector<DRILL_LAYER_PAIR>::const_iterator it = hole_sets.begin();
         it != hole_sets.end();  ++it )
    {
        DRILL_LAYER_PAIR  pair = *it;
        // For separate drill files, the last layer pair is the NPTH drill file.
        bool doing_npth = ( it == hole_sets.end() - 1 );

        buildHolesList( pair, doing_npth );

        // The file is created if it has holes, or if it is the non plated drill file
        // to be sure the NPTH file is up to date in separate files mode.
        if( getHolesCount() > 0 || doing_npth )
        {
            fn = getDrillFileName( pair, doing_npth, false );
            fn.SetPath( aPlotDirectory );

            if( aGenDrill )
            {
                wxString fullFilename = fn.GetFullPath();

                int result = createDrillFile( fullFilename, doing_npth, pair );

                if( result < 0 )
                {
                    if( aReporter )
                    {
                        msg.Printf( _( "** Unable to create %s **\n" ), fullFilename );
                        aReporter->Report( msg );
                    }
                    break;
                }
                else
                {
                    if( aReporter )
                    {
                        msg.Printf( _( "Create file %s\n" ), fullFilename );
                        aReporter->Report( msg );
                    }
                }

            }
        }
    }

    if( aGenMap )
        CreateMapFilesSet( aPlotDirectory, aReporter );
}

// A helper class to transform an oblong hole to a segment
static void convertOblong2Segment( wxSize aSize, double aOrient, wxPoint& aStart, wxPoint& aEnd );

int GERBER_WRITER::createDrillFile( wxString& aFullFilename, bool aIsNpth,
                                    DRILL_LAYER_PAIR aLayerPair )
{
    int    holes_count;

    LOCALE_IO dummy;    // Use the standard notation for double numbers

    GERBER_PLOTTER plotter;

    // Gerber drill file imply X2 format:
    plotter.UseX2format( true );
    plotter.UseX2NetAttributes( true );

    // Add the standard X2 header, without FileFunction
    AddGerberX2Header( &plotter, m_pcb );
    plotter.SetViewport( m_offset, IU_PER_MILS/10, /* scale */ 1.0, /* mirror */false );
    // has meaning only for gerber plotter. Must be called only after SetViewport
    plotter.SetGerberCoordinatesFormat( 6 );
    plotter.SetCreator( wxT( "PCBNEW" ) );

    // Add the standard X2 FileFunction for drill files
    // %TF.FileFunction,Plated[NonPlated],layer1num,layer2num,PTH[NPTH][Blind][Buried],Drill[Route][Mixed]*%
    wxString text = BuildFileFunctionAttributeString( aLayerPair, aIsNpth );
    plotter.AddLineToHeader( text );

    // Add file polarity (positive)
    text = "%TF.FilePolarity,Positive*%";
    plotter.AddLineToHeader( text );

    if( !plotter.OpenFile( aFullFilename ) )
        return -1;

    plotter.StartPlot();

    holes_count = 0;

    wxPoint hole_pos;
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

        if( dyn_cast<const VIA*>(hole_descr.m_ItemParent ) )
        {
            gbr_metadata.SetApertureAttrib( GBR_APERTURE_METADATA::GBR_APERTURE_ATTRIB_VIADRILL );

            if( !last_item_is_via )
            {
                // be sure the current object attribute is cleared for vias
                plotter.EndBlock( NULL );
            }

            last_item_is_via = true;
        }
        else if( dyn_cast<const D_PAD*>( hole_descr.m_ItemParent ) )
        {
            last_item_is_via = false;

            // Good practice of oblong pad holes (slots) is to use a specific aperture for routing, not used
            // in drill commands
            if( hole_descr.m_Hole_Shape )
                gbr_metadata.SetApertureAttrib( GBR_APERTURE_METADATA::GBR_APERTURE_ATTRIB_COMPONENTOBLONGDRILL );
            else
                gbr_metadata.SetApertureAttrib( GBR_APERTURE_METADATA::GBR_APERTURE_ATTRIB_COMPONENTDRILL );

            // Add object attribute: component reference to pads (mainly usefull for users)
            const D_PAD* pad = dyn_cast<const D_PAD*>( hole_descr.m_ItemParent );
            wxString ref = pad->GetParent()->GetReference();

#if 0   // Set to 1 to force a dummy reference for the parent pad.
            if( ref.IsEmpty() )
                ref = "<undefinedref>";
#endif
            gbr_metadata.SetCmpReference( ref );
            gbr_metadata.SetNetAttribType( GBR_NETLIST_METADATA::GBR_NETINFO_CMP );
        }

        if( hole_descr.m_Hole_Shape )
        {
            #if 0   // set to 1 to use flashed oblong holes.
                    // Currently not possible for hole orient != 0 or 90 deg
            // Use flashed oblong hole
            plotter.FlashPadOval( hole_pos, hole_descr.m_Hole_Size,
                                  hole_descr.m_Hole_Orient, FILLED, &gbr_metadata );
            #else
            // Use routing for oblong hole (Slots)
            wxPoint start, end;
            convertOblong2Segment( hole_descr.m_Hole_Size,
                                   hole_descr.m_Hole_Orient, start, end );
            int width = std::min( hole_descr.m_Hole_Size.x, hole_descr.m_Hole_Size.y );

            if ( width == 0 )
                continue;

            plotter.ThickSegment( start+hole_pos, end+hole_pos,
                                  width, FILLED, &gbr_metadata );
            #endif
        }
        else
        {
            int diam = std::min( hole_descr.m_Hole_Size.x, hole_descr.m_Hole_Size.y );
            plotter.FlashPadCircle( hole_pos, diam, FILLED, &gbr_metadata );
        }

        holes_count++;
    }

    plotter.EndPlot();

    return holes_count;
}


void convertOblong2Segment( wxSize aSize, double aOrient, wxPoint& aStart, wxPoint& aEnd )
{
    wxSize  size( aSize );
    double orient = aOrient;

    /* The pad will be drawn as an oblong shape with size.y > size.x
     * (Oval vertical orientation 0)
     */
    if( size.x > size.y )
    {
        std::swap( size.x, size.y );
        orient = AddAngles( orient, 900 );
    }

    int deltaxy = size.y - size.x;     // distance between centers of the oval

    int cx = 0;
    int cy = deltaxy / 2;
    RotatePoint( &cx, &cy, orient );
    aStart = wxPoint( cx, cy );
    cx = 0; cy = -deltaxy / 2;
    RotatePoint( &cx, &cy, orient );
    aEnd = wxPoint( cx, cy );
}


void GERBER_WRITER::SetFormat( int aRightDigits )
{
    /* Set conversion scale depending on drill file units */
    m_conversionUnits = 1.0 / IU_PER_MM;        // Gerber units = mm

    // Set precison (unit is mm).
    m_precision.m_lhs = 4;
    m_precision.m_rhs = aRightDigits == 6 ? 6 : 5;
}


const wxString GERBER_WRITER::getDrillFileName( DRILL_LAYER_PAIR aPair, bool aNPTH,
                                         bool aMerge_PTH_NPTH ) const
{
    // Gerber files extension is always .gbr.
    // Therefore, to mark drill files, add "-drl" to the filename.
    wxFileName fname( GENDRILL_WRITER_BASE::getDrillFileName( aPair, aNPTH, aMerge_PTH_NPTH ) );
    fname.SetName( fname.GetName() + "-drl" );

    return fname.GetFullPath();
}
