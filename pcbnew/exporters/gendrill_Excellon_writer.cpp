/**
 * @file gendrill_Excellon_writer.cpp
 * @brief Functions to create EXCELLON drill files and report files.
 */

/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 1992-2012 Jean_Pierre Charras <jp.charras at wanadoo.fr>
 * Copyright (C) 1992-2012 KiCad Developers, see change_log.txt for contributors.
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
 * @see for EXCELLON format, see:
 * http://www.excellon.com/manuals/program.htm
 * and the CNC-7 manual.
 */

#include <fctsys.h>

#include <vector>

#include <plot_common.h>
#include <trigo.h>
#include <macros.h>
#include <kicad_string.h>
#include <wxPcbStruct.h>
#include <pgm_base.h>
#include <build_version.h>

#include <class_board.h>
#include <class_module.h>
#include <class_track.h>

#include <pcbplot.h>
#include <pcbnew.h>
#include <gendrill_Excellon_writer.h>
#include <wildcards_and_files_ext.h>
#include <reporter.h>

//#include <dialog_gendrill.h>   //  Dialog box for drill file generation

EXCELLON_WRITER::EXCELLON_WRITER( BOARD* aPcb )
{
    m_file = NULL;
    m_pcb  = aPcb;
    m_zeroFormat      = DECIMAL_FORMAT;
    m_conversionUnits = 0.0001;
    m_unitsDecimal    = true;
    m_mirror = false;
    m_merge_PTH_NPTH = false;
    m_minimalHeader = false;
    m_ShortHeader = false;
    m_mapFileFmt = PLOT_FORMAT_PDF;
    m_pageInfo = NULL;
}


void EXCELLON_WRITER::CreateDrillandMapFilesSet(  const wxString& aPlotDirectory,
                                            bool aGenDrill, bool aGenMap,
                                            REPORTER * aReporter )
{
    wxFileName fn;
    wxString msg;

    wxString    layername_extend;   // added to the board filefame to create a full filename
                                    //(board fileName + layer pair names)

    // If some buried/blind vias are found, drill files are created
    // layer pair by layer pair for buried vias
    bool hasBuriedVias = false;

    for( TRACK* track = m_pcb->m_Track; track != NULL; track = track->Next() )
    {
        if( track->Type() == PCB_VIA_T )
        {
            const VIA *via = static_cast<const VIA*>( track );

            if( via->GetViaType() == VIA_MICROVIA ||
                via->GetViaType() == VIA_BLIND_BURIED )
            {
                hasBuriedVias = true;
                break;
            }
        }
    }


    int        layer1 = F_Cu;
    int        layer2 = B_Cu;
    bool       gen_through_holes = true;
    bool       gen_NPTH_holes    = false;

    for( ; ; )
    {
        BuildHolesList( layer1, layer2, gen_through_holes ? false : true,
                                       gen_NPTH_holes, m_merge_PTH_NPTH );

        if( GetHolesCount() > 0 ) // has holes?
        {
            fn = m_pcb->GetFileName();
            layername_extend.Empty();

            if( gen_NPTH_holes )
            {
                layername_extend << wxT( "-NPTH" );
            }
            else if( !gen_through_holes )
            {
                if( layer1 == F_Cu )
                    layername_extend << wxT( "-front" );
                else
                    layername_extend << wxT( "-inner" ) << layer1;

                if( layer2 == B_Cu )
                    layername_extend << wxT( "-back" );
                else
                    layername_extend << wxT( "-inner" ) << layer2;
            }

            fn.SetName( fn.GetName() + layername_extend );

            fn.SetPath( aPlotDirectory );

            if( aGenDrill )
            {
                fn.SetExt( DrillFileExtension );
                wxString fullFilename = fn.GetFullPath();

                FILE* file = wxFopen( fullFilename, wxT( "w" ) );

                if( file == NULL )
                {
                    if( aReporter )
                    {
                        msg.Printf(  _( "** Unable to create %s **\n" ),
                                          GetChars( fullFilename ) );
                        aReporter->Report( msg );
                    }
                    break;
                }
                else
                {
                    if( aReporter )
                    {
                        msg.Printf( _( "Create file %s\n" ), GetChars( fullFilename ) );
                        aReporter->Report( msg );
                    }
                }

                CreateDrillFile( file );
            }

            if( aGenMap )
            {
                fn.SetExt( wxEmptyString ); // Will be added by GenDrillMap
                wxString fullfilename = fn.GetFullPath() + wxT( "-drl_map" );
                fullfilename << wxT(".") << GetDefaultPlotExtension( m_mapFileFmt );

                bool success = GenDrillMapFile( fullfilename, m_mapFileFmt );

                if( ! success )
                {
                    if( aReporter )
                    {
                        msg.Printf( _( "** Unable to create %s **\n" ), GetChars( fullfilename ) );
                        aReporter->Report( msg );
                    }

                    return;
                }
                else
                {
                    if( aReporter )
                    {
                        msg.Printf( _( "Create file %s\n" ), GetChars( fullfilename ) );
                        aReporter->Report( msg );
                    }
                }
            }
        }

        if( gen_NPTH_holes )    // The last drill file was created
            break;

        if( !hasBuriedVias )
            gen_NPTH_holes = true;
        else
        {
            if( gen_through_holes )
                layer2 = layer1 + 1;    // done with through-board holes, prepare generation of first layer pair
            else
            {
                if( layer2 >= B_Cu )    // no more layer pair to consider
                {
                    layer1 = F_Cu;
                    layer2 = B_Cu;
                    gen_NPTH_holes = true;
                    continue;
                }

                layer1++;
                layer2++;                      // use next layer pair

                if( layer2 == m_pcb->GetCopperLayerCount() - 1 )
                    layer2 = B_Cu;      // the last layer is always the back layer
            }

            gen_through_holes = false;
        }
    }
}


/*
 *  Creates the drill files in EXCELLON format
 *  Number format:
 *      - Floating point format
 *      - integer format
 *      - integer format: "Trailing Zero" ( TZ ) or "Leading Zero"
 *  Units
 *      - Decimal
 *      - Metric
 */
int EXCELLON_WRITER::CreateDrillFile( FILE* aFile )
{
    m_file = aFile;

    int    diam, holes_count;
    int    x0, y0, xf, yf, xc, yc;
    double xt, yt;
    char   line[1024];

    LOCALE_IO dummy;    // Use the standard notation for double numbers

    WriteEXCELLONHeader();

    holes_count = 0;

    /* Write the tool list */
    for( unsigned ii = 0; ii < m_toolListBuffer.size(); ii++ )
    {
        DRILL_TOOL& tool_descr = m_toolListBuffer[ii];
        fprintf( m_file, "T%dC%.3f\n", ii + 1,
                 tool_descr.m_Diameter * m_conversionUnits  );
    }

    fputs( "%\n", m_file );                         // End of header info

    fputs( "G90\n", m_file );                       // Absolute mode
    fputs( "G05\n", m_file );                       // Drill mode

    // Units :
    if( !m_minimalHeader )
    {
        if( m_unitsDecimal  )
            fputs( "M71\n", m_file );       /* M71 = metric mode */
        else
            fputs( "M72\n", m_file );       /* M72 = inch mode */
    }

    /* Read the hole file and generate lines for normal holes (oblong
     * holes will be created later) */
    int tool_reference = -2;

    for( unsigned ii = 0; ii < m_holeListBuffer.size(); ii++ )
    {
        HOLE_INFO& hole_descr = m_holeListBuffer[ii];

        if( hole_descr.m_Hole_Shape )
            continue;  // oblong holes will be created later

        if( tool_reference != hole_descr.m_Tool_Reference )
        {
            tool_reference = hole_descr.m_Tool_Reference;
            fprintf( m_file, "T%d\n", tool_reference );
        }

        x0 = hole_descr.m_Hole_Pos.x - m_offset.x;
        y0 = hole_descr.m_Hole_Pos.y - m_offset.y;

        if( !m_mirror )
            y0 *= -1;

        xt = x0 * m_conversionUnits;
        yt = y0 * m_conversionUnits;
        WriteCoordinates( line, xt, yt );

        fputs( line, m_file );
        holes_count++;
    }

    /* Read the hole file and generate lines for normal holes (oblong holes
     * will be created later) */
    tool_reference = -2;    // set to a value not used for
                            // m_holeListBuffer[ii].m_Tool_Reference
    for( unsigned ii = 0; ii < m_holeListBuffer.size(); ii++ )
    {
        HOLE_INFO& hole_descr = m_holeListBuffer[ii];

        if( hole_descr.m_Hole_Shape == 0 )
            continue;  // wait for oblong holes

        if( tool_reference != hole_descr.m_Tool_Reference )
        {
            tool_reference = hole_descr.m_Tool_Reference;
            fprintf( m_file, "T%d\n", tool_reference );
        }

        diam = std::min( hole_descr.m_Hole_Size.x, hole_descr.m_Hole_Size.y );

        if( diam == 0 )
            continue;

        /* Compute the hole coordinates: */
        xc = x0 = xf = hole_descr.m_Hole_Pos.x - m_offset.x;
        yc = y0 = yf = hole_descr.m_Hole_Pos.y - m_offset.y;

        /* Compute the start and end coordinates for the shape */
        if( hole_descr.m_Hole_Size.x < hole_descr.m_Hole_Size.y )
        {
            int delta = ( hole_descr.m_Hole_Size.y - hole_descr.m_Hole_Size.x ) / 2;
            y0 -= delta;
            yf += delta;
        }
        else
        {
            int delta = ( hole_descr.m_Hole_Size.x - hole_descr.m_Hole_Size.y ) / 2;
            x0 -= delta;
            xf += delta;
        }

        RotatePoint( &x0, &y0, xc, yc, hole_descr.m_Hole_Orient );
        RotatePoint( &xf, &yf, xc, yc, hole_descr.m_Hole_Orient );

        if( !m_mirror )
        {
            y0 *= -1;
            yf *= -1;
        }

        xt = x0 * m_conversionUnits;
        yt = y0 * m_conversionUnits;
        WriteCoordinates( line, xt, yt );

        /* remove the '\n' from end of line, because we must add the "G85"
         * command to the line: */
        for( int kk = 0; line[kk] != 0; kk++ )
        {
            if( line[kk] == '\n' || line[kk] =='\r' )
                line[kk] = 0;
        }

        fputs( line, m_file );
        fputs( "G85", m_file );    // add the "G85" command

        xt = xf * m_conversionUnits;
        yt = yf * m_conversionUnits;
        WriteCoordinates( line, xt, yt );

        fputs( line, m_file );
        fputs( "G05\n", m_file );
        holes_count++;
    }

    WriteEXCELLONEndOfFile();

    return holes_count;
}


void EXCELLON_WRITER::SetFormat( bool      aMetric,
                                 ZEROS_FMT aZerosFmt,
                                 int       aLeftDigits,
                                 int       aRightDigits )
{
    m_unitsDecimal = aMetric;
    m_zeroFormat   = aZerosFmt;

    /* Set conversion scale depending on drill file units */
    if( m_unitsDecimal )
        m_conversionUnits = 1.0 / IU_PER_MM;        // EXCELLON units = mm
    else
        m_conversionUnits = 0.001 / IU_PER_MILS;    // EXCELLON units = INCHES

    // Set the zero counts. if aZerosFmt == DECIMAL_FORMAT, these values
    // will be set, but not used.
    if( aLeftDigits <= 0 )
        aLeftDigits = m_unitsDecimal ? 3 : 2;

    if( aRightDigits <= 0 )
        aRightDigits = m_unitsDecimal ? 3 : 4;

    m_precision.m_lhs = aLeftDigits;
    m_precision.m_rhs = aRightDigits;
}


void EXCELLON_WRITER::WriteCoordinates( char* aLine, double aCoordX, double aCoordY )
{
    wxString xs, ys;
    int      xpad = m_precision.m_lhs + m_precision.m_rhs;
    int      ypad = xpad;

    switch( m_zeroFormat )
    {
    default:
    case DECIMAL_FORMAT:
        /* In Excellon files, resolution is 1/1000 mm or 1/10000 inch (0.1 mil)
         * Although in decimal format, Excellon specifications do not specify
         * clearly the resolution. However it seems to be 1/1000mm or 0.1 mil
         * like in non decimal formats, so we trunk coordinates to 3 or 4 digits in mantissa
         * Decimal format just prohibit useless leading 0:
         * 0.45 or .45 is right, but 00.54 is incorrect.
         */
        if( m_unitsDecimal )
        {
            // resolution is 1/1000 mm
            xs.Printf( wxT( "%.3f" ), aCoordX );
            ys.Printf( wxT( "%.3f" ), aCoordY );
        }
        else
        {
            // resolution is 1/10000 inch
            xs.Printf( wxT( "%.4f" ), aCoordX );
            ys.Printf( wxT( "%.4f" ), aCoordY );
        }

        //Remove useless trailing 0
        while( xs.Last() == '0' )
            xs.RemoveLast();

        while( ys.Last() == '0' )
            ys.RemoveLast();

        sprintf( aLine, "X%sY%s\n", TO_UTF8( xs ), TO_UTF8( ys ) );
        break;

    case SUPPRESS_LEADING:
        for( int i = 0; i< m_precision.m_rhs; i++ )
        {
            aCoordX *= 10; aCoordY *= 10;
        }

        sprintf( aLine, "X%dY%d\n", KiROUND( aCoordX ), KiROUND( aCoordY ) );
        break;

    case SUPPRESS_TRAILING:
    {
        for( int i = 0; i < m_precision.m_rhs; i++ )
        {
            aCoordX *= 10;
            aCoordY *= 10;
        }

        if( aCoordX < 0 )
            xpad++;

        if( aCoordY < 0 )
            ypad++;

        xs.Printf( wxT( "%0*d" ), xpad, KiROUND( aCoordX ) );
        ys.Printf( wxT( "%0*d" ), ypad, KiROUND( aCoordY ) );

        size_t j = xs.Len() - 1;

        while( xs[j] == '0' && j )
            xs.Truncate( j-- );

        j = ys.Len() - 1;

        while( ys[j] == '0' && j )
            ys.Truncate( j-- );

        sprintf( aLine, "X%sY%s\n", TO_UTF8( xs ), TO_UTF8( ys ) );
        break;
    }

    case KEEP_ZEROS:
        for( int i = 0; i< m_precision.m_rhs; i++ )
        {
            aCoordX *= 10; aCoordY *= 10;
        }

        if( aCoordX < 0 )
            xpad++;

        if( aCoordY < 0 )
            ypad++;

        xs.Printf( wxT( "%0*d" ), xpad, KiROUND( aCoordX ) );
        ys.Printf( wxT( "%0*d" ), ypad, KiROUND( aCoordY ) );
        sprintf( aLine, "X%sY%s\n", TO_UTF8( xs ), TO_UTF8( ys ) );
        break;
    }
}


void EXCELLON_WRITER::WriteEXCELLONHeader()
{
    fputs( "M48\n", m_file );    // The beginning of a header

    if( !m_minimalHeader )
    {
        // The next 2 lines in EXCELLON files are comments:
        wxString msg;
        msg << wxT("KiCad") << wxT( " " ) << GetBuildVersion();

        fprintf( m_file, ";DRILL file {%s} date %s\n", TO_UTF8( msg ),
                 TO_UTF8( DateAndTime() ) );
        msg = wxT( ";FORMAT={" );

        // Print precision:
        if( m_zeroFormat != DECIMAL_FORMAT )
            msg << m_precision.GetPrecisionString();
        else
            msg << wxT( "-:-" );  // in decimal format the precision is irrelevant

        msg << wxT( "/ absolute / " );
        msg << ( m_unitsDecimal ? wxT( "metric" ) :  wxT( "inch" ) );

        /* Adding numbers notation format.
         * this is same as m_Choice_Zeros_Format strings, but NOT translated
         * because some EXCELLON parsers do not like non ASCII values
         * so we use ONLY English (ASCII) strings.
         * if new options are added in m_Choice_Zeros_Format, they must also
         * be added here
         */
        msg << wxT( " / " );

        const wxString zero_fmt[4] =
        {
            wxT( "decimal" ),
            wxT( "suppress leading zeros" ),
            wxT( "suppress trailing zeros" ),
            wxT( "keep zeros" )
        };

        msg << zero_fmt[m_zeroFormat];
        msg << wxT( "}\n" );
        fputs( TO_UTF8( msg ), m_file );
        fputs( "FMAT,2\n", m_file );     // Use Format 2 commands (version used since 1979)
    }

    fputs( m_unitsDecimal ? "METRIC" : "INCH", m_file );

    switch( m_zeroFormat )
    {
    case SUPPRESS_LEADING:
    case DECIMAL_FORMAT:
        fputs( ",TZ\n", m_file );
        break;

    case SUPPRESS_TRAILING:
        fputs( ",LZ\n", m_file );
        break;

    case KEEP_ZEROS:
        fputs( ",TZ\n", m_file ); // TZ is acceptable when all zeros are kept
        break;
    }
}


void EXCELLON_WRITER::WriteEXCELLONEndOfFile()
{
    //add if minimal here
    fputs( "T0\nM30\n", m_file );
    fclose( m_file );
}



/* Helper function for sorting hole list.
 * Compare function used for sorting holes  by increasing diameter value
 * and X value
 */
static bool CmpHoleDiameterValue( const HOLE_INFO& a, const HOLE_INFO& b )
{
    if( a.m_Hole_Diameter != b.m_Hole_Diameter )
        return a.m_Hole_Diameter < b.m_Hole_Diameter;

    if( a.m_Hole_Pos.x != b.m_Hole_Pos.x )
        return a.m_Hole_Pos.x < b.m_Hole_Pos.x;

    return a.m_Hole_Pos.y < b.m_Hole_Pos.y;
}


void EXCELLON_WRITER::BuildHolesList( int aFirstLayer,
                                      int aLastLayer,
                                      bool aExcludeThroughHoles,
                                      bool aGenerateNPTH_list,
                                      bool aMerge_PTH_NPTH )
{
    HOLE_INFO new_hole;
    int       hole_value;

    m_holeListBuffer.clear();
    m_toolListBuffer.clear();

    if( (aFirstLayer >= 0) && (aLastLayer >= 0) )
    {
        if( aFirstLayer > aLastLayer )
            std::swap( aFirstLayer, aLastLayer );
    }

    if ( aGenerateNPTH_list && aMerge_PTH_NPTH )
    {
        return;
    }

    // build hole list for vias
    if( ! aGenerateNPTH_list )  // vias are always plated !
    {
        for( VIA* via = GetFirstVia( m_pcb->m_Track ); via; via = GetFirstVia( via->Next() ) )
        {
            hole_value = via->GetDrillValue();

            if( hole_value == 0 )   // Should not occur.
                continue;

            new_hole.m_Tool_Reference = -1;         // Flag value for Not initialized
            new_hole.m_Hole_Orient    = 0;
            new_hole.m_Hole_Diameter  = hole_value;
            new_hole.m_Hole_Size.x = new_hole.m_Hole_Size.y = new_hole.m_Hole_Diameter;

            new_hole.m_Hole_Shape = 0;              // hole shape: round
            new_hole.m_Hole_Pos = via->GetStart();

            via->LayerPair( &new_hole.m_Hole_Top_Layer, &new_hole.m_Hole_Bottom_Layer );

            // LayerPair return params with m_Hole_Bottom_Layer > m_Hole_Top_Layer
            // Remember: top layer = 0 and bottom layer = 31 for through hole vias
            // the via should be at least from aFirstLayer to aLastLayer
            if( (new_hole.m_Hole_Top_Layer > aFirstLayer) && (aFirstLayer >= 0) )
                continue;   // via above the first layer

            if( (new_hole.m_Hole_Bottom_Layer < aLastLayer) && (aLastLayer >= 0) )
                continue;   // via below the last layer

            if( aExcludeThroughHoles && (new_hole.m_Hole_Bottom_Layer == B_Cu)
               && (new_hole.m_Hole_Top_Layer == F_Cu) )
                continue;

            m_holeListBuffer.push_back( new_hole );
        }
    }

    // build hole list for pads (assumed always through holes)
    if( !aExcludeThroughHoles || aGenerateNPTH_list )
    {
        for( MODULE* module = m_pcb->m_Modules;  module;  module = module->Next() )
        {
            // Read and analyse pads
            for( D_PAD* pad = module->Pads();  pad;  pad = pad->Next() )
            {
                if( ! aGenerateNPTH_list &&
                    pad->GetAttribute() == PAD_HOLE_NOT_PLATED &&
                    ! aMerge_PTH_NPTH )
                    continue;

                if( aGenerateNPTH_list && pad->GetAttribute() != PAD_HOLE_NOT_PLATED )
                    continue;

                if( pad->GetDrillSize().x == 0 )
                    continue;

                new_hole.m_Hole_NotPlated = (pad->GetAttribute() == PAD_HOLE_NOT_PLATED);
                new_hole.m_Tool_Reference = -1;         // Flag is: Not initialized
                new_hole.m_Hole_Orient    = pad->GetOrientation();
                new_hole.m_Hole_Shape     = 0;           // hole shape: round
                new_hole.m_Hole_Diameter  = std::min( pad->GetDrillSize().x, pad->GetDrillSize().y );
                new_hole.m_Hole_Size.x    = new_hole.m_Hole_Size.y = new_hole.m_Hole_Diameter;

                if( pad->GetDrillShape() != PAD_DRILL_CIRCLE )
                    new_hole.m_Hole_Shape = 1; // oval flag set

                new_hole.m_Hole_Size         = pad->GetDrillSize();
                new_hole.m_Hole_Pos          = pad->GetPosition();               // hole position
                new_hole.m_Hole_Bottom_Layer = B_Cu;
                new_hole.m_Hole_Top_Layer    = F_Cu;// pad holes are through holes
                m_holeListBuffer.push_back( new_hole );
            }
        }
    }

    // Sort holes per increasing diameter value
    sort( m_holeListBuffer.begin(), m_holeListBuffer.end(), CmpHoleDiameterValue );

    // build the tool list
    int        LastHole = -1; /* Set to not initialized (this is a value not used
                               * for m_holeListBuffer[ii].m_Hole_Diameter) */
    DRILL_TOOL new_tool( 0 );
    unsigned   jj;

    for( unsigned ii = 0; ii < m_holeListBuffer.size(); ii++ )
    {
        if( m_holeListBuffer[ii].m_Hole_Diameter != LastHole )
        {
            new_tool.m_Diameter = ( m_holeListBuffer[ii].m_Hole_Diameter );
            m_toolListBuffer.push_back( new_tool );
            LastHole = new_tool.m_Diameter;
        }

        jj = m_toolListBuffer.size();

        if( jj == 0 )
            continue;                                        // Should not occurs

        m_holeListBuffer[ii].m_Tool_Reference = jj;          // Tool value Initialized (value >= 1)

        m_toolListBuffer.back().m_TotalCount++;

        if( m_holeListBuffer[ii].m_Hole_Shape )
            m_toolListBuffer.back().m_OvalCount++;
    }
}
