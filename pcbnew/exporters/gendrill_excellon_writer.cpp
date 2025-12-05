/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2018 Jean_Pierre Charras <jp.charras at wanadoo.fr>
 * Copyright (C) 2015 SoftPLC Corporation, Dick Hollenbeck <dick@softplc.com>
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
 * @file gendrill_Excellon_writer.cpp
 * @brief Functions to create EXCELLON drill files and report files.
 */


/**
 * @see for EXCELLON format, see:
 * http://www.excellon.com/manuals/program.htm
 * and the CNC-7 manual.
 */

#include <plotters/plotter.h>
#include <string_utils.h>
#include <macros.h>
#include <pcb_edit_frame.h>
#include <build_version.h>
#include <math/util.h>      // for KiROUND
#include <trigo.h>

#include <pcbplot.h>
#include <board.h>
#include <gendrill_excellon_writer.h>
#include <wildcards_and_files_ext.h>
#include <reporter.h>
#include <gbr_metadata.h>

#include <fmt/format.h>


// Oblong holes can be drilled by a "canned slot" command (G85) or a routing command
// a linear routing command (G01) is perhaps more usual for drill files
//
// set m_useRouteModeForOval to false to use a canned slot hole (old way)
// set m_useRouteModeForOval to true (preferred mode) to use a linear routed hole (new way)


EXCELLON_WRITER::EXCELLON_WRITER( BOARD* aPcb )
    : GENDRILL_WRITER_BASE( aPcb )
{
    m_file = nullptr;
    m_zeroFormat      = DECIMAL_FORMAT;
    m_conversionUnits = 0.0001;
    m_mirror = false;
    m_merge_PTH_NPTH = false;
    m_minimalHeader = false;
    m_drillFileExtension = FILEEXT::DrillFileExtension;
    m_useRouteModeForOval = true;
    m_mantissaLenght = 3;   // suitable to print coordinates in mm
}


bool EXCELLON_WRITER::CreateDrillandMapFilesSet( const wxString& aPlotDirectory, bool aGenDrill,
                                                 bool aGenMap, REPORTER * aReporter )
{
    bool        success = true;
    wxFileName  fn;
    wxString    msg;

    std::vector<DRILL_SPAN> hole_sets = getUniqueLayerPairs();

    if( !m_merge_PTH_NPTH )
        hole_sets.emplace_back( F_Cu, B_Cu, false, true );

    for( std::vector<DRILL_SPAN>::const_iterator it = hole_sets.begin();
         it != hole_sets.end();  ++it )
    {
        const DRILL_SPAN& span = *it;
        bool doing_npth = m_merge_PTH_NPTH ? false : span.m_IsNonPlatedFile;

        buildHolesList( span, doing_npth );

        // The file is created if it has holes, or if it is the non plated drill file to be
        // sure the NPTH file is up to date in separate files mode.
        // Also a PTH drill/map file is always created, to be sure at least one plated hole
        // drill file is created (do not create any PTH drill file can be seen as not working
        // drill generator).
        if( getHolesCount() > 0 || doing_npth || span.Pair() == DRILL_LAYER_PAIR( F_Cu, B_Cu ) )
        {
            fn = getDrillFileName( span, doing_npth, m_merge_PTH_NPTH );
            fn.SetPath( aPlotDirectory );

            if( aGenDrill )
            {
                wxString fullFilename = fn.GetFullPath();

                FILE* file = wxFopen( fullFilename, wxT( "w" ) );

                if( file == nullptr )
                {
                    if( aReporter )
                    {
                        msg.Printf( _( "Failed to create file '%s'." ), fullFilename );
                        aReporter->Report( msg, RPT_SEVERITY_ERROR );
                        success = false;
                    }

                    break;
                }
                else
                {
                    if( aReporter )
                    {
                        msg.Printf( _( "Created file '%s'" ), fullFilename );
                        aReporter->Report( msg, RPT_SEVERITY_ACTION );
                    }
                }

                TYPE_FILE file_type = TYPE_FILE::PTH_FILE;

                if( span.Pair() == DRILL_LAYER_PAIR( F_Cu, B_Cu ) && !span.m_IsBackdrill )
                {
                    if( m_merge_PTH_NPTH )
                        file_type = TYPE_FILE::MIXED_FILE;
                    else if( doing_npth )
                        file_type = TYPE_FILE::NPTH_FILE;
                }
                else if( span.m_IsBackdrill )
                {
                    file_type = TYPE_FILE::NPTH_FILE;
                }

                bool wroteDrillFile = false;

                try
                {
                    createDrillFile( file, span, file_type );
                    wroteDrillFile = true;
                }
                catch( ... )     // Capture fmt::print exception on write issues
                {
                    fclose( file );
                    msg.Printf( _( "Failed to write file '%s'." ), fullFilename );
                    aReporter->Report( msg, RPT_SEVERITY_ERROR );
                    success = false;
                }

                if( wroteDrillFile && span.m_IsBackdrill && getHolesCount() > 0 )
                {
                    if( !writeBackdrillLayerPairFile( aPlotDirectory, aReporter, span ) )
                    {
                        success = false;
                        break;
                    }
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


void EXCELLON_WRITER::writeHoleAttribute( HOLE_ATTRIBUTE aAttribute )
{
    // Hole attributes are comments (lines starting by ';') in the drill files
    // For tools (file header), they are similar to X2 apertures attributes.
    // for attributes added in coordinate list, they are just comments.
    if( !m_minimalHeader )
    {
        switch( aAttribute )
        {
        case HOLE_ATTRIBUTE::HOLE_VIA_THROUGH:
            fmt::print( m_file, "{}", "; #@! TA.AperFunction,Plated,PTH,ViaDrill\n" );
            break;

        case HOLE_ATTRIBUTE::HOLE_VIA_BURIED:
            fmt::print( m_file, "{}", "; #@! TA.AperFunction,Plated,Buried,ViaDrill\n" );
            break;

        case HOLE_ATTRIBUTE::HOLE_VIA_BACKDRILL:
            fmt::print( m_file, "{}", "; #@! TA.AperFunction,NonPlated,BackDrill\n" );
            break;

        case HOLE_ATTRIBUTE::HOLE_PAD:
        //case HOLE_ATTRIBUTE::HOLE_PAD_CASTELLATED:
            fmt::print( m_file, "{}", "; #@! TA.AperFunction,Plated,PTH,ComponentDrill\n" );
            break;

        case HOLE_ATTRIBUTE::HOLE_PAD_CASTELLATED:
            fmt::print( m_file, "{}", "; #@! TA.AperFunction,Plated,PTH,CastelletedDrill\n" );
            break;

        case HOLE_ATTRIBUTE::HOLE_PAD_PRESSFIT:
            fmt::print( m_file, "{}", "; #@! TA.AperFunction,Plated,PTH,ComponentDrill,PressFit\n" );
            break;

        case HOLE_ATTRIBUTE::HOLE_MECHANICAL:
            fmt::print( m_file, "{}", "; #@! TA.AperFunction,NonPlated,NPTH,ComponentDrill\n" );
            break;

        case HOLE_ATTRIBUTE::HOLE_UNKNOWN:
            fmt::print( m_file, "{}", "; #@! TD\n" );
            break;
        }
    }
}


int EXCELLON_WRITER::createDrillFile( FILE* aFile, const DRILL_SPAN& aSpan,
                                      TYPE_FILE aHolesType, bool aTagBackdrillHit )
{
    // if units are mm, the resolution is 0.001 mm (3 digits in mantissa)
    // if units are inches, the resolution is 0.1 mil (4 digits in mantissa)
    m_mantissaLenght = m_unitsMetric ? 3 : 4;

    m_file = aFile;

    int    diam, holes_count;
    int    x0, y0, xf, yf, xc, yc;
    double xt, yt;
    char   line[1024];

    writeEXCELLONHeader( aSpan, aHolesType );

    holes_count = 0;

    /* Write the tool list */
    for( unsigned ii = 0; ii < m_toolListBuffer.size(); ii++ )
    {
        DRILL_TOOL& tool_descr = m_toolListBuffer[ii];

#if USE_ATTRIB_FOR_HOLES
        writeHoleAttribute( tool_descr.m_HoleAttribute );
#endif
        fmt::print( m_file, "T{}C{:.{}f}\n", ii + 1, tool_descr.m_Diameter * m_conversionUnits, m_mantissaLenght );

        if( !m_minimalHeader )
        {
            if( tool_descr.m_IsBackdrill )
            {
                auto formatStub = [&]( int aStubLength )
                {
                    double stubMM = pcbIUScale.IUTomm( aStubLength );
                    double stubInches = stubMM / 25.4;
                    wxString tmp = fmt::format( "{:.3f}mm ({:.4f}\")", stubMM, stubInches );
                    return tmp;
               };

                wxString comment = wxT( "; Backdrill" );

                if( tool_descr.m_MinStubLength.has_value() )
                {
                    comment += wxT( " stub " );
                    comment += formatStub( *tool_descr.m_MinStubLength );;

                    if( tool_descr.m_MaxStubLength.has_value()
                            && tool_descr.m_MaxStubLength != tool_descr.m_MinStubLength )
                    {
                        comment += wxT( " to " );
                        comment += formatStub( *tool_descr.m_MaxStubLength );;
                    }
                }

                if( tool_descr.m_HasPostMachining )
                    comment += wxT( ", post-machining" );

                comment += wxT( "\n" );
                fmt::print( m_file, "{}", TO_UTF8( comment ) );
            }
            else if( tool_descr.m_HasPostMachining )
            {
                fmt::print( m_file, "{}", "; Post-machining\n" );
            }
        }
    }

    fmt::print( m_file, "{}", "%\n" );              // End of header info
    fmt::print( m_file, "{}", "G90\n" );            // Absolute mode
    fmt::print( m_file, "{}", "G05\n" );            // Drill mode

    /* Read the hole list and generate data for normal holes (oblong
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
            fmt::print( m_file, "T{}\n", tool_reference );
        }

        x0 = hole_descr.m_Hole_Pos.x - m_offset.x;
        y0 = hole_descr.m_Hole_Pos.y - m_offset.y;

        if( !m_mirror )
            y0 *= -1;

        xt = x0 * m_conversionUnits;
        yt = y0 * m_conversionUnits;
        writeHoleComments( hole_descr, aTagBackdrillHit );
        writeCoordinates( line, sizeof( line ), xt, yt );

        fmt::print( m_file, "{}", line );
        holes_count++;
    }

    /* Read the hole list and generate data for oblong holes
     */
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
            fmt::print( m_file, "T{}\n", tool_reference );
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
        writeHoleComments( hole_descr, aTagBackdrillHit );

        if( m_useRouteModeForOval )
            fmt::print( m_file, "{}", "G00" );    // Select the routing mode

        writeCoordinates( line, sizeof( line ), xt, yt );

        if( !m_useRouteModeForOval )
        {
            /* remove the '\n' from end of line, because we must add the "G85"
             * command to the line: */
            for( int kk = 0; line[kk] != 0; kk++ )
            {
                if( line[kk] < ' ' )
                    line[kk] = 0;
            }

            fmt::print( m_file, "{}", line );
            fmt::print( m_file, "{}", "G85" );         // add the "G85" command
        }
        else
        {
            fmt::print( m_file, "{}", line );
            fmt::print( m_file, "{}", "M15\nG01" );    // tool down and linear routing from last coordinates
        }

        xt = xf * m_conversionUnits;
        yt = yf * m_conversionUnits;
        writeCoordinates( line, sizeof( line ), xt, yt );

        fmt::print( m_file, "{}",line );

        if( m_useRouteModeForOval )
            fmt::print( m_file, "{}", "M16\n" );       // Tool up (end routing)

        fmt::print( m_file, "{}", "G05\n" );           // Select drill mode
        holes_count++;
    }

    writeEXCELLONEndOfFile();

    return holes_count;
}


void EXCELLON_WRITER::SetFormat( bool aMetric, ZEROS_FMT aZerosFmt, int aLeftDigits,
                                 int aRightDigits )
{
    m_unitsMetric = aMetric;
    m_zeroFormat   = aZerosFmt;

    /* Set conversion scale depending on drill file units */
    if( m_unitsMetric )
        m_conversionUnits = 1.0 / pcbIUScale.IU_PER_MM;       // EXCELLON units = mm
    else
        m_conversionUnits = 0.001 / pcbIUScale.IU_PER_MILS;   // EXCELLON units = in

    // Set the zero counts. if aZerosFmt == DECIMAL_FORMAT, these values
    // will be set, but not used.
    if( aLeftDigits <= 0 )
        aLeftDigits = m_unitsMetric ? 3 : 2;

    if( aRightDigits <= 0 )
        aRightDigits = m_unitsMetric ? 3 : 4;

    m_precision.m_Lhs = aLeftDigits;
    m_precision.m_Rhs = aRightDigits;
}


void EXCELLON_WRITER::writeCoordinates( char* aLine, size_t aLineSize, double aCoordX,
                                        double aCoordY )
{
    wxString xs, ys;
    int      xpad = m_precision.m_Lhs + m_precision.m_Rhs;
    int      ypad = xpad;

    // if units are mm, the resolution is 0.001 mm (3 digits in mantissa)
    // if units are inches, the resolution is 0.1 mil (4 digits in mantissa)
    // in DECIMAL_FORMAT we could use more digits.

    switch( m_zeroFormat )
    {
    default:
    case DECIMAL_FORMAT:
        /* In Excellon files, resolution is 1/1000 mm or 1/10000 inch (0.1 mil)
         * Although in decimal format, Excellon specifications do not specify
         * clearly the resolution. However it seems to be usually 1/1000mm or 0.1 mil
         * like in non decimal formats, so we trunk coordinates to m_mantissaLenght in mantissa
         * Decimal format just prohibit useless leading 0:
         * 0.45 or .45 is right, but 00.54 is incorrect.
         */
        xs = fmt::format( "{:.{}f}", aCoordX, m_mantissaLenght );
        ys = fmt::format( "{:.{}f}", aCoordY, m_mantissaLenght );

        //Remove useless trailing 0
        while( xs.Last() == '0' )
            xs.RemoveLast();

        if( xs.Last() == '.' )      // however keep a trailing 0 after the floating point separator
            xs << '0';

        while( ys.Last() == '0' )
            ys.RemoveLast();

        if( ys.Last() == '.' )
            ys << '0';

        std::snprintf( aLine, aLineSize, "X%sY%s\n", TO_UTF8( xs ), TO_UTF8( ys ) );
        break;

    case SUPPRESS_LEADING:
        for( int i = 0; i< m_precision.m_Rhs; i++ )
        {
            aCoordX *= 10; aCoordY *= 10;
        }

        std::snprintf( aLine, aLineSize, "X%dY%d\n", KiROUND( aCoordX ), KiROUND( aCoordY ) );
        break;

    case SUPPRESS_TRAILING:
    {
        for( int i = 0; i < m_precision.m_Rhs; i++ )
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

        std::snprintf( aLine, aLineSize, "X%sY%s\n", TO_UTF8( xs ), TO_UTF8( ys ) );
        break;
    }

    case KEEP_ZEROS:
        for( int i = 0; i< m_precision.m_Rhs; i++ )
        {
            aCoordX *= 10; aCoordY *= 10;
        }

        if( aCoordX < 0 )
            xpad++;

        if( aCoordY < 0 )
            ypad++;

        xs.Printf( wxT( "%0*d" ), xpad, KiROUND( aCoordX ) );
        ys.Printf( wxT( "%0*d" ), ypad, KiROUND( aCoordY ) );
        std::snprintf( aLine, aLineSize, "X%sY%s\n", TO_UTF8( xs ), TO_UTF8( ys ) );
        break;
    }
}


void EXCELLON_WRITER::writeEXCELLONHeader( const DRILL_SPAN& aSpan, TYPE_FILE aHolesType )
{
    fmt::print( m_file, "{}", "M48\n" );    // The beginning of a header

    if( !m_minimalHeader )
    {
        // The next lines in EXCELLON files are comments:
        wxString msg;
        msg << wxT( "KiCad " ) << GetBuildVersion();

        fmt::print( m_file, "; DRILL file {} date {}\n",
                            TO_UTF8( msg ), TO_UTF8( GetISO8601CurrentDateTime() ) );
        msg = wxT( "; FORMAT={" );

        // Print precision:
        // Note in decimal format the precision is not used.
        // the floating point notation has higher priority than the precision.
        if( m_zeroFormat != DECIMAL_FORMAT )
            msg << m_precision.GetPrecisionString();
        else
            msg << wxT( "-:-" );  // in decimal format the precision is irrelevant

        msg << wxT( "/ absolute / " );
        msg << ( m_unitsMetric ? wxT( "metric" ) : wxT( "inch" ) );

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

        msg << zero_fmt[m_zeroFormat] << wxT( "}\n" );
        fmt::print( m_file, "{}", TO_UTF8( msg ) );

        // add the structured comment TF.CreationDate:
        // The attribute value must conform to the full version of the ISO 8601
        msg = GbrMakeCreationDateAttributeString( GBR_NC_STRING_FORMAT_NCDRILL ) + wxT( "\n" );
        fmt::print( m_file, "{}", TO_UTF8( msg ) );

        // Add the application name that created the drill file
        msg = wxT( "; #@! TF.GenerationSoftware,Kicad,Pcbnew," );
        msg << GetBuildVersion() << wxT( "\n" );
        fmt::print( m_file, "{}", TO_UTF8( msg ) );

        // Add the standard X2 FileFunction for drill files
        // TF.FileFunction,Plated[NonPlated],layer1num,layer2num,PTH[NPTH]
        msg = BuildFileFunctionAttributeString( aSpan, aHolesType , true ) + wxT( "\n" );
        fmt::print( m_file, "{}", TO_UTF8( msg ) );

        fmt::print( m_file, "{}",  "FMAT,2\n" );     // Use Format 2 commands (version used since 1979)
    }

    fmt::print( m_file, "{}", m_unitsMetric ? "METRIC" : "INCH" );

    switch( m_zeroFormat )
    {
    case DECIMAL_FORMAT:
        fmt::print( m_file, "{}", "\n" );
        break;

    case SUPPRESS_LEADING:
        fmt::print( m_file, "{}", ",TZ\n" );
        break;

    case SUPPRESS_TRAILING:
        fmt::print( m_file, "{}", ",LZ\n" );
        break;

    case KEEP_ZEROS:
        // write nothing, but TZ is acceptable when all zeros are kept
        fmt::print( m_file, "{}", "\n" );
        break;
    }
}


void EXCELLON_WRITER::writeEXCELLONEndOfFile()
{
    // add if minimal here
    fmt::print( m_file, "{}", "M30\n" );
    fclose( m_file );
}


wxFileName EXCELLON_WRITER::getBackdrillLayerPairFileName( const DRILL_SPAN& aSpan ) const
{
    wxFileName fn = m_pcb->GetFileName();
    wxString   extend;

    extend << wxT( "-" )
           << wxString::FromUTF8( layerPairName( aSpan.Pair() ).c_str() )
           << wxT( "-backdrill" );

    fn.SetName( fn.GetName() + extend );
    fn.SetExt( m_drillFileExtension );

    return fn;
}


bool EXCELLON_WRITER::writeBackdrillLayerPairFile( const wxString& aPlotDirectory,
                                                   REPORTER* aReporter, const DRILL_SPAN& aSpan )
{
    wxFileName fn = getBackdrillLayerPairFileName( aSpan );
    fn.SetPath( aPlotDirectory );

    wxString fullFilename = fn.GetFullPath();
    FILE*    file = wxFopen( fullFilename, wxT( "w" ) );

    if( file == nullptr )
    {
        if( aReporter )
        {
            wxString msg;
            msg.Printf( _( "Failed to create file '%s'." ), fullFilename );
            aReporter->Report( msg, RPT_SEVERITY_ERROR );
        }

        return false;
    }
    else if( aReporter )
    {
        wxString msg;
        msg.Printf( _( "Created file '%s'" ), fullFilename );
        aReporter->Report( msg, RPT_SEVERITY_ACTION );
    }

    try
    {
        createDrillFile( file, aSpan, TYPE_FILE::NPTH_FILE, true );
    }
    catch( ... )
    {
        fclose( file );

        if( aReporter )
        {
            wxString msg;
            msg.Printf( _( "Failed to write file '%s'." ), fullFilename );
            aReporter->Report( msg, RPT_SEVERITY_ERROR );
        }

        return false;
    }

    return true;
}


void EXCELLON_WRITER::writeHoleComments( const HOLE_INFO& aHole, bool aTagBackdrillHit )
{
    if( aTagBackdrillHit && aHole.m_IsBackdrill )
        fmt::print( m_file, "{}", "; backdrill\n" );

    writePostMachiningComment( aHole.m_FrontPostMachining, aHole.m_FrontPostMachiningSize,
                               aHole.m_FrontPostMachiningDepth, aHole.m_FrontPostMachiningAngle,
                               wxT( "front" ) );

    writePostMachiningComment( aHole.m_BackPostMachining, aHole.m_BackPostMachiningSize,
                               aHole.m_BackPostMachiningDepth, aHole.m_BackPostMachiningAngle,
                               wxT( "back" ) );
}


void EXCELLON_WRITER::writePostMachiningComment( PAD_DRILL_POST_MACHINING_MODE aMode,
                                                 int aSizeIU, int aDepthIU, int aAngleDeciDegree,
                                                 const wxString& aSideLabel )
{
    if( aMode != PAD_DRILL_POST_MACHINING_MODE::COUNTERBORE
            && aMode != PAD_DRILL_POST_MACHINING_MODE::COUNTERSINK )
    {
        return;
    }

    wxString comment;
    comment << wxT( "; Post-machining " ) << aSideLabel << wxT( " " )
            << ( aMode == PAD_DRILL_POST_MACHINING_MODE::COUNTERSINK ? wxT( "countersink" )
                                                                    : wxT( "counterbore" ) );

    wxString sizeStr = formatLinearValue( aSizeIU );

    if( !sizeStr.IsEmpty() )
        comment << wxT( " dia " ) << sizeStr;

    wxString depthStr = formatLinearValue( aDepthIU );

    if( !depthStr.IsEmpty() )
        comment << wxT( " depth " ) << depthStr;

    if( aMode == PAD_DRILL_POST_MACHINING_MODE::COUNTERSINK && aAngleDeciDegree > 0 )
    {
        double    angle = aAngleDeciDegree / 10.0;
        wxString  angleStr;

        if( ( aAngleDeciDegree % 10 ) == 0 )
            angleStr = fmt::format( "{:.0f}deg", angle );
        else
            angleStr = fmt::format( "{:.1f}deg", angle );

        comment << wxT( " angle " ) << angleStr;
    }

    comment << wxT( "\n" );
    fmt::print( m_file, "{}", TO_UTF8( comment ) );
}


wxString EXCELLON_WRITER::formatLinearValue( int aValueIU ) const
{
    if( aValueIU <= 0 )
        return wxString();

    double converted = aValueIU * m_conversionUnits;
    wxString value;
    value = fmt::format( "{:.{}f}", converted, m_mantissaLenght );
    value << ( m_unitsMetric ? wxT( "mm" ) : wxT( "in" ) );

    return value;
}
