/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2017 Jean_Pierre Charras <jp.charras at wanadoo.fr>
 * Copyright (C) 2015 SoftPLC Corporation, Dick Hollenbeck <dick@softplc.com>
 * Copyright (C) 1992-2017 KiCad Developers, see change_log.txt for contributors.
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
 * @file gendrill_Excellon_writer.cpp
 * @brief Functions to create EXCELLON drill files and report files.
 */


/**
 * @see for EXCELLON format, see:
 * http://www.excellon.com/manuals/program.htm
 * and the CNC-7 manual.
 */

#include <fctsys.h>

#include <plot_common.h>
#include <kicad_string.h>
#include <wxPcbStruct.h>
#include <pgm_base.h>
#include <build_version.h>

#include <pcbplot.h>
#include <pcbnew.h>
#include <gendrill_Excellon_writer.h>
#include <wildcards_and_files_ext.h>
#include <reporter.h>

// Comment/uncomment this to write or not a comment
// in drill file when PTH and NPTH are merged to flag
// tools used for PTH and tools used for NPTH
// #define WRITE_PTH_NPTH_COMMENT


EXCELLON_WRITER::EXCELLON_WRITER( BOARD* aPcb )
    : GENDRILL_WRITER_BASE( aPcb )
{
    m_file = NULL;
    m_zeroFormat      = DECIMAL_FORMAT;
    m_conversionUnits = 0.0001;
    m_mirror = false;
    m_merge_PTH_NPTH = false;
    m_minimalHeader = false;
    m_drillFileExtension = DrillFileExtension;
}


void EXCELLON_WRITER::CreateDrillandMapFilesSet( const wxString& aPlotDirectory,
                                                 bool aGenDrill, bool aGenMap,
                                                 REPORTER * aReporter )
{
    wxFileName  fn;
    wxString    msg;

    std::vector<DRILL_LAYER_PAIR> hole_sets = getUniqueLayerPairs();

    // append a pair representing the NPTH set of holes, for separate drill files.
    if( !m_merge_PTH_NPTH )
        hole_sets.push_back( DRILL_LAYER_PAIR( F_Cu, B_Cu ) );

    for( std::vector<DRILL_LAYER_PAIR>::const_iterator it = hole_sets.begin();
         it != hole_sets.end();  ++it )
    {
        DRILL_LAYER_PAIR  pair = *it;
        // For separate drill files, the last layer pair is the NPTH drill file.
        bool doing_npth = m_merge_PTH_NPTH ? false : ( it == hole_sets.end() - 1 );

        buildHolesList( pair, doing_npth );

        // The file is created if it has holes, or if it is the non plated drill file
        // to be sure the NPTH file is up to date in separate files mode.
        if( getHolesCount() > 0 || doing_npth )
        {
            fn = getDrillFileName( pair, doing_npth, m_merge_PTH_NPTH );
            fn.SetPath( aPlotDirectory );

            if( aGenDrill )
            {
                wxString fullFilename = fn.GetFullPath();

                FILE* file = wxFopen( fullFilename, wxT( "w" ) );

                if( file == NULL )
                {
                    if( aReporter )
                    {
                        msg.Printf( _( "** Unable to create %s **\n" ), GetChars( fullFilename ) );
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

                createDrillFile( file );
            }
        }
    }

    if( aGenMap )
        CreateMapFilesSet( aPlotDirectory, aReporter );
}


int EXCELLON_WRITER::createDrillFile( FILE* aFile )
{
    m_file = aFile;

    int    diam, holes_count;
    int    x0, y0, xf, yf, xc, yc;
    double xt, yt;
    char   line[1024];

    LOCALE_IO dummy;    // Use the standard notation for double numbers

    writeEXCELLONHeader();

    holes_count = 0;

#ifdef WRITE_PTH_NPTH_COMMENT
    // if PTH_ and NPTH are merged write a comment in drill file at the
    // beginning of NPTH section
    bool writePTHcomment  = m_merge_PTH_NPTH;
    bool writeNPTHcomment = m_merge_PTH_NPTH;
#endif

    /* Write the tool list */
    for( unsigned ii = 0; ii < m_toolListBuffer.size(); ii++ )
    {
        DRILL_TOOL& tool_descr = m_toolListBuffer[ii];

#ifdef WRITE_PTH_NPTH_COMMENT
        if( writePTHcomment && !tool_descr.m_Hole_NotPlated )
        {
            writePTHcomment = false;
            fprintf( m_file, ";TYPE=PLATED\n" );
        }

        if( writeNPTHcomment && tool_descr.m_Hole_NotPlated )
        {
            writeNPTHcomment = false;
            fprintf( m_file, ";TYPE=NON_PLATED\n" );
        }
#endif

        fprintf( m_file, "T%dC%.3f\n", ii + 1, tool_descr.m_Diameter * m_conversionUnits );
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
        writeCoordinates( line, xt, yt );

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
        writeCoordinates( line, xt, yt );

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
        writeCoordinates( line, xt, yt );

        fputs( line, m_file );
        fputs( "G05\n", m_file );
        holes_count++;
    }

    writeEXCELLONEndOfFile();

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


void EXCELLON_WRITER::writeCoordinates( char* aLine, double aCoordX, double aCoordY )
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


void EXCELLON_WRITER::writeEXCELLONHeader()
{
    fputs( "M48\n", m_file );    // The beginning of a header

    if( !m_minimalHeader )
    {
        // The next 2 lines in EXCELLON files are comments:
        wxString msg;
        msg << wxT("KiCad") << wxT( " " ) << GetBuildVersion();

        fprintf( m_file, ";DRILL file {%s} date %s\n", TO_UTF8( msg ), TO_UTF8( DateAndTime() ) );
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


void EXCELLON_WRITER::writeEXCELLONEndOfFile()
{
    //add if minimal here
    fputs( "T0\nM30\n", m_file );
    fclose( m_file );
}
