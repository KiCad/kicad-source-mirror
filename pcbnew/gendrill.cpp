/*************************************************************/
/* Functions to create EXCELLON drill files and report files */
/*************************************************************/

/**
 * @info for EXCELLON format, see:
 * http://www.excellon.com/applicationengineering/manuals/program.htm
 * and the CNC-7 manual.
 */

#include "fctsys.h"

#include <vector>

#include "common.h"
#include "plot_common.h"
#include "trigo.h"
#include "confirm.h"
#include "kicad_string.h"
#include "gestfich.h"
#include "pcbnew.h"
#include "wxPcbStruct.h"
#include "pcbplot.h"
#include "macros.h"
#include "appl_wxstruct.h"
#include "class_board_design_settings.h"

#include "gendrill.h"

#include "dialog_gendrill.h"   //  Dialog box for drill file generation

#include "build_version.h"

const wxString DrillFileExtension( wxT( "drl" ) );
const wxString DrillFileWildcard( _( "Drill files (*.drl)|*.drl" ) );

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

// Helper functions:
static void Gen_Line_EXCELLON( char * aLine, double aCoordX, double aCoordY, DRILL_PRECISION& aPrecision );
static void WriteEndOfFile_EXCELLON( FILE* aFile );

static double                  s_ConversionUnits;    /* Conversion unite for
                                                      * drill / pcb */
static std::vector<DRILL_TOOL> s_ToolListBuffer;
static std::vector<HOLE_INFO>  s_HoleListBuffer;



/* This function displays and deletes the dialog frame for drill tools
 */
void WinEDA_PcbFrame::InstallDrillFrame( wxCommandEvent& event )
{
    DIALOG_GENDRILL* frame = new DIALOG_GENDRILL( this );
    frame->ShowModal();
    frame->Destroy();
}


/**
 * Function GenDrillOrReportFiles
 * Calls the functions to create EXCELLON drill files and/or drill map files
 * >When all holes are through, only one excellon file is created.
 * >When there are some partial holes (some blind or buried vias),
 *  one excellon file is created, for all through holes,
 *  and one file per layer pair, which have one or more holes, excluding
 *  through holes, already in the first file.
 */
void DIALOG_GENDRILL::GenDrillOrReportFiles( )
{
    wxFileName fn;
    wxString   layer_extend;              /* added to the  Board FileName to
                                           * create FullFileName (= Board
                                           * FileName + layer pair names) */
    wxString   msg;
    bool       ExistsBuriedVias = false;  /* If true, drill files are created
                                           * layer pair by layer pair for
                                           * buried vias */
    int        layer1 = LAYER_N_BACK;
    int        layer2 = LAYER_N_FRONT;
    bool       gen_through_holes = true;

    UpdateConfig(); /* set params and Save drill options */

    m_Parent->MsgPanel->EraseMsgBox();

    /* Set conversion scale depending on drill file units */
    s_ConversionUnits = 0.0001;         // EXCELLON units = INCHES
    if( !m_UnitDrillIsInch )
        s_ConversionUnits = 0.00254;    // EXCELLON units = mm

    if( m_MicroViasCount || m_BlindOrBuriedViasCount )
        ExistsBuriedVias = true;

    for( ; ; )
    {
        Build_Holes_List( m_Parent->GetBoard(), s_HoleListBuffer,
                          s_ToolListBuffer, layer1, layer2,
                          gen_through_holes ? false : true );

        if( s_ToolListBuffer.size() > 0 ) //holes?
        {
            fn = m_Parent->GetScreen()->m_FileName;
            layer_extend.Empty();

            if( !gen_through_holes )
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

            wxFileDialog dlg( this, _( "Save Drill File" ), fn.GetPath(),
                              fn.GetFullName(), DrillFileWildcard,
                              wxFD_SAVE );

            if( dlg.ShowModal() == wxID_CANCEL )
                break;

            FILE* aFile = wxFopen( dlg.GetPath(), wxT( "w" ) );

            if( aFile == 0 )
            {
                msg = _( "Unable to create file " ) + dlg.GetPath();
                DisplayError( this, msg );
                EndModal( 0 );
                return;
            }

            Create_Drill_File_EXCELLON( aFile, m_FileDrillOffset,
                                        s_HoleListBuffer, s_ToolListBuffer );

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

            if( !ExistsBuriedVias )
                break;
        }
        if(  gen_through_holes )
            layer2 = layer1 + 1;
        else
        {
            if( layer2 >= LAYER_N_FRONT )    // no more layer pair to consider
                break;
            layer1++;
            layer2++;                      // use next layer pair

            if( layer2 == m_Parent->GetBoard()->GetCopperLayerCount() - 1 )
                layer2 = LAYER_N_FRONT;         // the last layer is always the
                                                // component layer
        }

        gen_through_holes = false;
    }

    if( m_Choice_Drill_Report->GetSelection() > 0 )
    {
        GenDrillReport( m_Parent->GetScreen()->m_FileName );
    }

    EndModal( 0 );
}


/**
 * Create the drill file in EXCELLON format
 * @return hole count
 * @param aHoleListBuffer = hole descriptor list
 * @param aToolListBuffer = Drill tools list
 */
int DIALOG_GENDRILL::Create_Drill_File_EXCELLON( FILE*   aFile,
                                                 wxPoint aOffset,
                                                 std::vector<HOLE_INFO>&  aHoleListBuffer,
                                                 std::vector<DRILL_TOOL>& aToolListBuffer )
{
    int   diam, holes_count;
    int   x0, y0, xf, yf, xc, yc;
    double xt, yt;
    char  line[1024];

    SetLocaleTo_C_standard(); // Use the standard notation for double numbers

    Write_Excellon_Header( aFile, m_MinimalHeader, (zeros_fmt) m_ZerosFormat );

    holes_count = 0;
 
    /* Write the tool list */
    for( unsigned ii = 0; ii < aToolListBuffer.size(); ii++ )
    {
        fprintf( aFile, "T%dC%.3f\n", ii + 1,
                 double (aToolListBuffer[ii].m_Diameter) * s_ConversionUnits  );
    }

    fputs( "%\n", aFile );                      // End of header info

    fputs( "G90\n", aFile );                    // Absolute mode
    fputs( "G05\n", aFile );                    // Drill mode
    /* Units : */
    if( !m_MinimalHeader )
    {
        if( m_UnitDrillIsInch  )
            fputs( "M72\n", aFile );    /* M72 = inch mode */
        else 
            fputs( "M71\n", aFile );    /* M71 = metric mode */
    }

    /* Read the hole file and generate lines for normal holes (oblong
     * holes will be created later) */
    int tool_reference = -2;
    for( unsigned ii = 0; ii < aHoleListBuffer.size(); ii++ )
    {
        if( aHoleListBuffer[ii].m_Hole_Shape )
            continue;  // oblong holes will be created later
        if( tool_reference != aHoleListBuffer[ii].m_Tool_Reference )
        {
            tool_reference = aHoleListBuffer[ii].m_Tool_Reference;
            fprintf( aFile, "T%d\n", tool_reference );
        }

        x0 = aHoleListBuffer[ii].m_Hole_Pos_X - aOffset.x;
        y0 = aHoleListBuffer[ii].m_Hole_Pos_Y - aOffset.y;

        if( !m_Mirror )
            y0 *= -1;

        xt = x0 * s_ConversionUnits;
        yt = y0 * s_ConversionUnits;
        Gen_Line_EXCELLON( line, xt, yt, m_Precision );

        fputs( line, aFile );
        holes_count++;
    }

    /* Read the hole file and generate lines for normal holes (oblong holes
     * will be created later) */
    tool_reference = -2;    // set to a value not used for
                            // aHoleListBuffer[ii].m_Tool_Reference
    for( unsigned ii = 0; ii < aHoleListBuffer.size(); ii++ )
    {
        if( aHoleListBuffer[ii].m_Hole_Shape == 0 )
            continue;  // wait for oblong holes
        if( tool_reference != aHoleListBuffer[ii].m_Tool_Reference )
        {
            tool_reference = aHoleListBuffer[ii].m_Tool_Reference;
            fprintf( aFile, "T%d\n", tool_reference );
        }

        diam = MIN( aHoleListBuffer[ii].m_Hole_SizeX,
                    aHoleListBuffer[ii].m_Hole_SizeY );
        if( diam == 0 )
            continue;

        /* Compute the hole coordinates: */
        xc = x0 = xf = aHoleListBuffer[ii].m_Hole_Pos_X - aOffset.x;
        yc = y0 = yf = aHoleListBuffer[ii].m_Hole_Pos_Y - aOffset.y;

        /* Compute the start and end coordinates for the shape */
        if( aHoleListBuffer[ii].m_Hole_SizeX < aHoleListBuffer[ii].m_Hole_SizeY )
        {
            int delta = ( aHoleListBuffer[ii].m_Hole_SizeY
                          - aHoleListBuffer[ii].m_Hole_SizeX ) / 2;
            y0 -= delta; yf += delta;
        }
        else
        {
            int delta = ( aHoleListBuffer[ii].m_Hole_SizeX
                          - aHoleListBuffer[ii].m_Hole_SizeY ) / 2;
            x0 -= delta; xf += delta;
        }
        RotatePoint( &x0, &y0, xc, yc, aHoleListBuffer[ii].m_Hole_Orient );
        RotatePoint( &xf, &yf, xc, yc, aHoleListBuffer[ii].m_Hole_Orient );


        if( !m_Mirror )
        {
            y0 *= -1;  yf *= -1;
        }

        xt = x0 * s_ConversionUnits;
        yt = y0 * s_ConversionUnits;
        Gen_Line_EXCELLON( line, xt, yt, m_Precision );

        /* remove the '\n' from end of line, because we must add the "G85"
         * command to the line: */
        for( int kk = 0; line[kk] != 0; kk++ )
            if( line[kk] == '\n' || line[kk] =='\r' )
                line[kk] = 0;

        fputs( line, aFile );

        fputs( "G85", aFile );    // add the "G85" command

        xt = xf * s_ConversionUnits;
        yt = yf * s_ConversionUnits;
        Gen_Line_EXCELLON( line, xt, yt, m_Precision );
 
        fputs( line, aFile );
        fputs( "G05\n", aFile );
        holes_count++;
    }

    WriteEndOfFile_EXCELLON( aFile );

    SetLocaleTo_Default();  // Revert to locale double notation

    return holes_count;
}


/* Created a line like:
 * X48000Y19500
 * According to the selected format
 */
void Gen_Line_EXCELLON( char * aLine, double aCoordX, double aCoordY, DRILL_PRECISION& aPrecision )
{
    wxString xs, ys;
    int      xpad = aPrecision.m_lhs + aPrecision.m_rhs;
    int      ypad = xpad;

    /* I need to come up with an algorithm that handles any lhs:rhs format.*/
    /* one idea is to take more inputs for xpad/ypad when metric is used.  */

    switch( DIALOG_GENDRILL::m_ZerosFormat )
    {
    default:
    case DECIMAL_FORMAT:
        sprintf( aLine, "X%.3fY%.3f\n", aCoordX, aCoordY );
        break;

    case SUPPRESS_LEADING:             /* that should work now */
        for( int i = 0; i< aPrecision.m_rhs; i++ )
        {
            aCoordX *= 10; aCoordY *= 10;
        }

        sprintf( aLine, "X%dY%d\n", wxRound( aCoordX ), wxRound( aCoordY ) );
        break;

    case SUPPRESS_TRAILING:
    {
        for( int i = 0; i < aPrecision.m_rhs; i++ )
        {
            aCoordX *= 10;
            aCoordY *= 10;
        }

        if( aCoordX < 0 )
            xpad++;
        if( aCoordY < 0 )
            ypad++;

        xs.Printf( wxT( "%0*d" ), xpad, wxRound( aCoordX ) );
        ys.Printf( wxT( "%0*d" ), ypad, wxRound( aCoordY ) );

        size_t j = xs.Len() - 1;
        while( xs[j] == '0' && j )
            xs.Truncate( j-- );

        j = ys.Len() - 1;
        while( ys[j] == '0' && j )
            ys.Truncate( j-- );

        sprintf( aLine, "X%sY%s\n", CONV_TO_UTF8( xs ), CONV_TO_UTF8( ys ) );
        break;
    }

    case KEEP_ZEROS:
        for( int i = 0; i< aPrecision.m_rhs; i++ )
        {
            aCoordX *= 10; aCoordY *= 10;
        }

        if( aCoordX < 0 )
            xpad++;
        if( aCoordY < 0 )
            ypad++;
        xs.Printf( wxT( "%0*d" ), xpad, wxRound( aCoordX ) );
        ys.Printf( wxT( "%0*d" ), ypad, wxRound( aCoordY ) );
        sprintf( aLine, "X%sY%s\n", CONV_TO_UTF8( xs ), CONV_TO_UTF8( ys ) );
        break;
    }
}


/* Print the DRILL file header. The full header is:
 * M48
 * ;DRILL file {PCBNEW (2007-11-29-b)} date 17/1/2008-21:02:35
 * ;FORMAT={ <precision> / absolute / <units> / <numbers format>}
 * R,T
 * VER,1
 * FMAT,2
 * INCH,TZ
 * TCST,OFF
 * ICI,OFF
 * ATC,ON
 */
void DIALOG_GENDRILL::Write_Excellon_Header( FILE* aFile, bool aMinimalHeader, zeros_fmt aFormat )
{
    char Line[256];

    fputs( "M48\n", aFile );    // The beginning of a header

    if( !aMinimalHeader )
    {
        DateAndTime( Line );

        // The next 2 lines in EXCELLON files are comments:
        wxString msg = wxGetApp().GetTitle() + wxT( " " ) + GetBuildVersion();
        fprintf( aFile, ";DRILL file {%s} date %s\n", CONV_TO_UTF8( msg ),
                 Line );
        msg = wxT( ";FORMAT={" );

        // Print precision:
        if( aFormat != DECIMAL_FORMAT )
            msg << m_Choice_Precision->GetStringSelection();
        else
            msg << wxT( "-.-" );  // in decimal format the precision is irrelevant
        msg << wxT( "/ absolute / " );
        msg << ( m_UnitDrillIsInch ? wxT( "inch" ) : wxT( "metric" ) );

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

        msg << zero_fmt[aFormat];
        msg << wxT( "}\n" );
        fputs( CONV_TO_UTF8( msg ), aFile );
        fputs( "FMAT,2\n", aFile );     // Use Format 2 commands (version used since 1979)
    }

    fputs( m_UnitDrillIsInch ? "INCH" : "METRIC", aFile );

    switch( aFormat )
    {
    case SUPPRESS_LEADING:
    case DECIMAL_FORMAT:
        fputs( ",TZ\n", aFile );
        break;

    case SUPPRESS_TRAILING:
        fputs( ",LZ\n", aFile );
        break;

    case KEEP_ZEROS:
        fputs( ",TZ\n", aFile ); // TZ is acceptable when all zeros are kept
        break;
    }
}


void WriteEndOfFile_EXCELLON( FILE* aFile )
{
    //add if minimal here
    fputs( "T0\nM30\n", aFile );
    fclose( aFile );
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
                      wxFD_SAVE | wxFD_OVERWRITE_PROMPT );

    if( dlg.ShowModal() == wxID_CANCEL )
        return;

    FILE* plotfile = wxFopen( dlg.GetPath(), wxT( "wt" ) );

    if( plotfile == 0 )
    {
        msg = _( "Unable to create file" );
        msg << wxT( " <" ) << dlg.GetPath() << wxT( ">" );
        DisplayError( this, msg );
        return;
    }

    GenDrillMapFile( m_Parent->GetBoard(),
                     plotfile,
                     dlg.GetPath(),
                     m_Parent->GetScreen()->m_CurrentSheetDesc,
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
    wxString   wildcard = _( "Drill report files (.rpt)|*.rpt" );

    fn = aFileName;
    fn.SetName( fn.GetName() + wxT( "-drl" ) );
    fn.SetExt( wxT( "rpt" ) );

    wxFileDialog dlg( this, _( "Save Drill Report File" ), fn.GetPath(),
                      fn.GetFullName(), wildcard,
                      wxFD_SAVE | wxFD_OVERWRITE_PROMPT );

    if( dlg.ShowModal() == wxID_CANCEL )
        return;

    FILE* report_dest = wxFopen( dlg.GetPath(), wxT( "w" ) );

    if( report_dest == 0 )
    {
        msg = _( "Unable to create file " ) + dlg.GetPath();
        DisplayError( this, msg );
        return;
    }

    GenDrillReportFile( report_dest, m_Parent->GetBoard(),
                        m_Parent->GetScreen()->m_FileName,
                        m_UnitDrillIsInch,
                        s_HoleListBuffer,
                        s_ToolListBuffer );
}
