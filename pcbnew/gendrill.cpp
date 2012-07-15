/**
 * @file gendrill.cpp
 * @brief Functions to create EXCELLON drill files and report files.
 */

/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 1992-2010 Jean_Pierre Charras <jp.charras@ujf-grenoble.fr>
 * Copyright (C) 1992-2010 KiCad Developers, see change_log.txt for contributors.
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
#include <confirm.h>
#include <kicad_string.h>
#include <gestfich.h>
#include <wxPcbStruct.h>
#include <macros.h>
#include <appl_wxstruct.h>
#include <build_version.h>

#include <class_board.h>

#include <pcbplot.h>
#include <pcbnew.h>
#include <gendrill.h>
#include <wildcards_and_files_ext.h>

#include <dialog_gendrill.h>   //  Dialog box for drill file generation


/*
 *  Creates the drill files in EXCELLON format
 *  Number format:
 *      - Floating point format
 *      - integer format
 *      - integer format: "Trailing Zero" ( TZ ) or "Leading Zero"
 *  Units
 *      - Decimal
 *      - Metric
 *
 *  The drill maps can be created in HPGL or PS format
 *
 * dialog_gendrill.cpp  is the file which handles
 * the Dialog box for drill file generation
 */

static std::vector<DRILL_TOOL> s_ToolListBuffer;
static std::vector<HOLE_INFO>  s_HoleListBuffer;


/* This function displays the dialog frame for drill tools
 */
void PCB_EDIT_FRAME::InstallDrillFrame( wxCommandEvent& event )
{
    DIALOG_GENDRILL dlg( this );

    dlg.ShowModal();
}


/**
 * Function GenDrillAndReportFiles
 * Calls the functions to create EXCELLON drill files and/or drill map files
 * >When all holes are through, only one excellon file is created.
 * >When there are some partial holes (some blind or buried vias),
 *  one excellon file is created, for all plated through holes,
 *  and one file per layer pair, which have one or more holes, excluding
 *  through holes, already in the first file.
 *  one file for all Not Plated through holes
 */
void DIALOG_GENDRILL::GenDrillAndReportFiles()
{
    wxFileName fn;
    wxString   layer_extend;              /* added to the  Board FileName to
                                           * create FullFileName (= Board
                                           * FileName + layer pair names) */
    wxString   msg;
    bool       hasBuriedVias = false;  /* If true, drill files are created
                                        * layer pair by layer pair for
                                        * buried vias */
    int        layer1 = LAYER_N_BACK;
    int        layer2 = LAYER_N_FRONT;
    bool       gen_through_holes = true;
    bool       gen_NPTH_holes    = false;

    wxString   currentWD = ::wxGetCwd();

    UpdateConfig(); // set params and Save drill options

    m_parent->ClearMsgPanel();

    if( m_microViasCount || m_blindOrBuriedViasCount )
        hasBuriedVias = true;

    for( ; ; )
    {
        Build_Holes_List( m_parent->GetBoard(), s_HoleListBuffer,
                          s_ToolListBuffer, layer1, layer2,
                          gen_through_holes ? false : true, gen_NPTH_holes );

        if( s_ToolListBuffer.size() > 0 ) //  holes?
        {
            fn = m_parent->GetScreen()->GetFileName();
            layer_extend.Empty();

            if( gen_NPTH_holes )
            {
                layer_extend << wxT( "-NPTH" );
            }
            else if( !gen_through_holes )
            {
                if( layer1 == LAYER_N_BACK )
                    layer_extend << wxT( "-copper" );
                else
                    layer_extend << wxT( "-inner" ) << layer1;
                if( layer2 == LAYER_N_FRONT )
                    layer_extend << wxT( "-cmp" );
                else
                    layer_extend << wxT( "-inner" ) << layer2;
            }

            fn.SetName( fn.GetName() + layer_extend );
            fn.SetExt( DrillFileExtension );

            wxFileDialog dlg( this, _( "Save Drill File" ), ::wxGetCwd(),
                              fn.GetFullName(), wxGetTranslation( DrillFileWildcard ),
                              wxFD_SAVE | wxFD_CHANGE_DIR );

            if( dlg.ShowModal() == wxID_CANCEL )
                break;

            FILE* aFile = wxFopen( dlg.GetPath(), wxT( "w" ) );

            if( aFile == 0 )
            {
                msg.Printf( _( "Unable to create drill file %s" ), GetChars( dlg.GetPath() ) );
                wxMessageBox( msg );
                ::wxSetWorkingDirectory( currentWD );
                EndModal( 0 );
                return;
            }

            EXCELLON_WRITER excellonWriter( m_parent->GetBoard(),
                                            aFile, m_FileDrillOffset,
                                            &s_HoleListBuffer, &s_ToolListBuffer );
            excellonWriter.SetFormat( !m_UnitDrillIsInch,
                                      (EXCELLON_WRITER::zeros_fmt) m_ZerosFormat,
                                      m_Precision.m_lhs, m_Precision.m_rhs );
            excellonWriter.SetOptions( m_Mirror, m_MinimalHeader, m_FileDrillOffset );
            excellonWriter.CreateDrillFile();

            switch( m_Choice_Drill_Map->GetSelection() )
            {
            case 0:
                break;

            case 1:
                GenDrillMap( dlg.GetPath(), s_HoleListBuffer, s_ToolListBuffer,
                             PLOT_FORMAT_HPGL );
                break;

            case 2:
                GenDrillMap( dlg.GetPath(), s_HoleListBuffer, s_ToolListBuffer,
                             PLOT_FORMAT_POST );
                break;

            case 3:
                GenDrillMap( dlg.GetPath(), s_HoleListBuffer, s_ToolListBuffer,
                             PLOT_FORMAT_GERBER );
                break;

            case 4:
                GenDrillMap( dlg.GetPath(), s_HoleListBuffer, s_ToolListBuffer,
                             PLOT_FORMAT_DXF );
                break;
            }
        }

        if( gen_NPTH_holes )    // The last drill file was created
            break;

        if( !hasBuriedVias )
            gen_NPTH_holes = true;
        else
        {
            if(  gen_through_holes )
                layer2 = layer1 + 1;    // prepare generation of first layer pair
            else
            {
                if( layer2 >= LAYER_N_FRONT )    // no more layer pair to consider
                {
                    layer1 = LAYER_N_BACK;
                    layer2 = LAYER_N_FRONT;
                    gen_NPTH_holes = true;
                    continue;
                }
                layer1++;
                layer2++;                      // use next layer pair

                if( layer2 == m_parent->GetBoard()->GetCopperLayerCount() - 1 )
                    layer2 = LAYER_N_FRONT;         // the last layer is always the
                                                    // component layer
            }

            gen_through_holes = false;
        }
    }

    if( m_Choice_Drill_Report->GetSelection() > 0 )
    {
        fn = m_parent->GetScreen()->GetFileName();
        GenDrillReport( fn.GetFullName() );
    }

    ::wxSetWorkingDirectory( currentWD );
}


/**
 * Create the drill file in EXCELLON format
 * @return hole count
 */
int EXCELLON_WRITER::CreateDrillFile()
{
    int    diam, holes_count;
    int    x0, y0, xf, yf, xc, yc;
    double xt, yt;
    char   line[1024];

    SetLocaleTo_C_standard(); // Use the standard notation for double numbers

    WriteHeader();

    holes_count = 0;

    /* Write the tool list */
    for( unsigned ii = 0; ii < m_toolListBuffer->size(); ii++ )
    {
        DRILL_TOOL& tool_descr = (*m_toolListBuffer)[ii];
        fprintf( m_file, "T%dC%.3f\n", ii + 1,
                 tool_descr.m_Diameter * m_conversionUnits  );
    }

    fputs( "%\n", m_file );                         // End of header info

    fputs( "G90\n", m_file );                       // Absolute mode
    fputs( "G05\n", m_file );                       // Drill mode
    /* Units : */
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
    for( unsigned ii = 0; ii < m_holeListBuffer->size(); ii++ )
    {
        HOLE_INFO& hole_descr = (*m_holeListBuffer)[ii];

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
                            // aHoleListBuffer[ii].m_Tool_Reference
    for( unsigned ii = 0; ii < m_holeListBuffer->size(); ii++ )
    {
        HOLE_INFO& hole_descr = (*m_holeListBuffer)[ii];
        if( hole_descr.m_Hole_Shape == 0 )
            continue;  // wait for oblong holes
        if( tool_reference != hole_descr.m_Tool_Reference )
        {
            tool_reference = hole_descr.m_Tool_Reference;
            fprintf( m_file, "T%d\n", tool_reference );
        }

        diam = MIN( hole_descr.m_Hole_Size.x,
                    hole_descr.m_Hole_Size.y );
        if( diam == 0 )
            continue;

        /* Compute the hole coordinates: */
        xc = x0 = xf = hole_descr.m_Hole_Pos.x - m_offset.x;
        yc = y0 = yf = hole_descr.m_Hole_Pos.y - m_offset.y;

        /* Compute the start and end coordinates for the shape */
        if( hole_descr.m_Hole_Size.x < hole_descr.m_Hole_Size.y )
        {
            int delta = ( hole_descr.m_Hole_Size.y - hole_descr.m_Hole_Size.x ) / 2;
            y0 -= delta; yf += delta;
        }
        else
        {
            int delta = ( hole_descr.m_Hole_Size.x - hole_descr.m_Hole_Size.y ) / 2;
            x0 -= delta; xf += delta;
        }
        RotatePoint( &x0, &y0, xc, yc, hole_descr.m_Hole_Orient );
        RotatePoint( &xf, &yf, xc, yc, hole_descr.m_Hole_Orient );


        if( !m_mirror )
        {
            y0 *= -1;  yf *= -1;
        }

        xt = x0 * m_conversionUnits;
        yt = y0 * m_conversionUnits;
        WriteCoordinates( line, xt, yt );

        /* remove the '\n' from end of line, because we must add the "G85"
         * command to the line: */
        for( int kk = 0; line[kk] != 0; kk++ )
            if( line[kk] == '\n' || line[kk] =='\r' )
                line[kk] = 0;

        fputs( line, m_file );

        fputs( "G85", m_file );    // add the "G85" command

        xt = xf * m_conversionUnits;
        yt = yf * m_conversionUnits;
        WriteCoordinates( line, xt, yt );

        fputs( line, m_file );
        fputs( "G05\n", m_file );
        holes_count++;
    }

    WriteEndOfFile();

    SetLocaleTo_Default();  // Revert to locale double notation

    return holes_count;
}


/**
 * SetFormat
 * Initialize internal parameters to match the given format
 * @param aMetric = true for metric coordinates, false for imperial units
 * @param aZerosFmt =  DECIMAL_FORMAT, SUPPRESS_LEADING, SUPPRESS_TRAILING, KEEP_ZEROS
 * @param aLeftDigits = number of digits for integer part of coordinates
 * @param aRightDigits = number of digits for mantissa part of coordinates
 */
void EXCELLON_WRITER::SetFormat( bool      aMetric,
                                 zeros_fmt aZerosFmt,
                                 int       aLeftDigits,
                                 int       aRightDigits )
{
    m_unitsDecimal = aMetric;
    m_zeroFormat   = aZerosFmt;

    /* Set conversion scale depending on drill file units */
    if( m_unitsDecimal )
        m_conversionUnits = 1.0 / IU_PER_MM; // EXCELLON units = mm
    else
        m_conversionUnits = 0.001 / IU_PER_MILS; // EXCELLON units = INCHES

    m_precision.m_lhs = aLeftDigits;
    m_precision.m_rhs = aRightDigits;
}


/* Created a line like:
 * X48000Y19500
 * According to the selected format
 */
void EXCELLON_WRITER::WriteCoordinates( char* aLine, double aCoordX, double aCoordY )
{
    wxString xs, ys;
    int      xpad = m_precision.m_lhs + m_precision.m_rhs;
    int      ypad = xpad;

    switch( DIALOG_GENDRILL::m_ZerosFormat )
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


/* Print the DRILL file header. The full header is:
 * M48
 * ;DRILL file {PCBNEW (2007-11-29-b)} date 17/1/2008-21:02:35
 * ;FORMAT={ <precision> / absolute / <units> / <numbers format>}
 * FMAT,2
 * INCH,TZ
 */
void EXCELLON_WRITER::WriteHeader()
{
    fputs( "M48\n", m_file );    // The beginning of a header

    if( !m_minimalHeader )
    {
        // The next 2 lines in EXCELLON files are comments:
        wxString msg = wxGetApp().GetTitle() + wxT( " " ) + GetBuildVersion();
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
         * because some EXCELLON parsers do not like non ascii values
         * so we use ONLY english (ascii) strings.
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


void EXCELLON_WRITER::WriteEndOfFile()
{
    //add if minimal here
    fputs( "T0\nM30\n", m_file );
    fclose( m_file );
}


/* Generate the drill plan (Drill map) format HPGL or POSTSCRIPT
 */
void DIALOG_GENDRILL::GenDrillMap( const wxString           aFileName,
                                   std::vector<HOLE_INFO>&  aHoleListBuffer,
                                   std::vector<DRILL_TOOL>& buffer,
                                   int                      format )
{
    wxFileName fn;
    wxString   ext, wildcard;
    wxString   msg;

    /* Init extension */
    switch( format )
    {
    case PLOT_FORMAT_HPGL:
        ext = wxT( "plt" );
        wildcard = _( "HPGL plot files (.plt)|*.plt" );
        break;

    case PLOT_FORMAT_POST:
        ext = wxT( "ps" );
        wildcard = _( "PostScript files (.ps)|*.ps" );
        break;

    case PLOT_FORMAT_GERBER:
        ext = wxT( "pho" );
        wildcard = _( "Gerber files (.pho)|*.pho" );
        break;

    case PLOT_FORMAT_DXF:
        ext = wxT( "dxf" );
        wildcard = _( "DXF files (.dxf)|*.dxf" );
        break;

    default:
        DisplayError( this, wxT( "DIALOG_GENDRILL::GenDrillMap() error" ) );
        return;
    }

    /* Init file name */
    fn = aFileName;
    fn.SetName( fn.GetName() + wxT( "-drl" ) );
    fn.SetExt( ext );

    wxFileDialog dlg( this, _( "Save Drill Plot File" ), fn.GetPath(),
                      fn.GetFullName(), wildcard,
                      wxFD_SAVE );

    if( dlg.ShowModal() == wxID_CANCEL )
        return;

    FILE* plotfile = wxFopen( dlg.GetPath(), wxT( "wt" ) );

    if( plotfile == 0 )
    {
        msg = _( "Unable to create file" );
        msg << wxT( " <" ) << dlg.GetPath() << wxT( ">" );
        wxMessageBox( msg );
        return;
    }

    GenDrillMapFile( m_parent->GetBoard(),
                     plotfile,
                     dlg.GetPath(),
                     m_parent->GetPageSettings(),
                     s_HoleListBuffer,
                     s_ToolListBuffer,
                     m_UnitDrillIsInch,
                     format, m_FileDrillOffset );
}


/*
 *  Create a list of drill values and drill count
 */
void DIALOG_GENDRILL::GenDrillReport( const wxString aFileName )
{
    wxFileName fn;
    wxString   msg;

    fn = aFileName;
    fn.SetName( fn.GetName() + wxT( "-drl" ) );
    fn.SetExt( ReportFileExtension );

    wxFileDialog dlg( this, _( "Save Drill Report File" ), fn.GetPath(),
                      fn.GetFullName(), wxGetTranslation( ReportFileWildcard ),
                      wxFD_SAVE );

    if( dlg.ShowModal() == wxID_CANCEL )
        return;

    FILE* report_dest = wxFopen( dlg.GetPath(), wxT( "w" ) );

    if( report_dest == 0 )
    {
        msg = _( "Unable to create file " ) + dlg.GetPath();
        wxMessageBox( msg );
        return;
    }

    GenDrillReportFile( report_dest, m_parent->GetBoard(),
                        m_parent->GetScreen()->GetFileName(),
                        m_UnitDrillIsInch,
                        s_HoleListBuffer,
                        s_ToolListBuffer );
}
