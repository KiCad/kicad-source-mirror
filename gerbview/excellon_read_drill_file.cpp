/**
 * @file excellon_read_drill_file.cpp
 *  Functions to read drill files (EXCELLON format) created by Pcbnew
 *  These files use only a subset of EXCELLON commands.
 */


/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 1992-2011 Jean-Pierre Charras <jean-pierre.charras@gipsa-lab.inpg.fr>
 * Copyright (C) 1992-2011 KiCad Developers, see change_log.txt for contributors.
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

/*
 *  Here is a sample of drill files created by Pcbnew, in decimal format:
 * (Note: coordinates formats are same as Gerber, and T commands are near Gerber D commands).
 *  M48
 *  ;DRILL file {PCBnew (2011-03-14 BZR 2894)-testing} date 15/03/2011 14:23:22
 *  ;FORMAT={-:-/ absolute / inch / decimal}
 *  FMAT,2
 *  INCH,TZ
 *  T1C0.02
 *  T2C0.032
 *  %
 *  G90
 *  G05
 *  M72
 *  T1
 *  X1.580Y-1.360
 *  X1.580Y-4.860
 *  X8.680Y-1.360
 *  X8.680Y-4.860
 *  T2
 *  X2.930Y-3.560
 *  X5.280Y-2.535
 *  X5.405Y-2.610
 *  X5.620Y-2.900
 *  T0
 *  M30
 */
 /*
  * Note there are some variant of tool definition:
  * T1F00S00C0.2 or T1C0.02F00S00 ... Feed Rate and Spindle Speed of Tool 1
  * Feed Rate and Spindle Speed are just skipped because they are not used in a viewer
  */

#include <fctsys.h>
#include <common.h>
#include <confirm.h>

#include <gerbview.h>
#include <gerbview_frame.h>
#include <trigo.h>
#include <macros.h>
#include <base_units.h>
#include <class_gerber_draw_item.h>
#include <class_GERBER.h>
#include <class_excellon.h>
#include <kicad_string.h>

#include <cmath>

#include <html_messagebox.h>

// Default format for dimensions
// number of digits in mantissa:
static int fmtMantissaMM = 3;
static int fmtMantissaInch = 4;
// number of digits, integer part:
static int fmtIntegerMM = 3;
static int fmtIntegerInch = 2;

extern int    ReadInt( char*& text, bool aSkipSeparator = true );
extern double ReadDouble( char*& text, bool aSkipSeparator = true );
extern void fillFlashedGBRITEM(  GERBER_DRAW_ITEM* aGbrItem,
                                 APERTURE_T        aAperture,
                                 int               Dcode_index,
                                 int         aLayer,
                                 const wxPoint&    aPos,
                                 wxSize            aSize,
                                 bool              aLayerNegative );
void fillLineGBRITEM(  GERBER_DRAW_ITEM* aGbrItem,
                              int               Dcode_index,
                              int         aLayer,
                              const wxPoint&    aStart,
                              const wxPoint&    aEnd,
                              wxSize            aPenSize,
                              bool              aLayerNegative  );

static EXCELLON_CMD excellonHeaderCmdList[] =
{
    { "M0",     DRILL_M_END,                 -1 },  // End of Program - No Rewind
    { "M00",    DRILL_M_END,                 -1 },  // End of Program - No Rewind
    { "M30",    DRILL_M_ENDREWIND,           -1 },  // End of Program Rewind
    { "M47",    DRILL_M_MESSAGE,             -1 },  // Operator Message
    { "M45",    DRILL_M_LONGMESSAGE,         -1 },  // Long Operator message (use more than one line)
    { "M48",    DRILL_M_HEADER,              0  },  // beginning of a header
    { "M95",    DRILL_M_ENDHEADER,           0  },  // End of the header
    { "METRIC", DRILL_METRICHEADER,          1  },
    { "INCH",   DRILL_IMPERIALHEADER,        1  },
    { "M71",    DRILL_M_METRIC,              1  },
    { "M72",    DRILL_M_IMPERIAL,            1  },
    { "M25",    DRILL_M_BEGINPATTERN,        0  },  // Beginning of Pattern
    { "M01",    DRILL_M_ENDPATTERN,          0  },  // End of Pattern
    { "M97",    DRILL_M_CANNEDTEXT,          -1 },
    { "M98",    DRILL_M_CANNEDTEXT,          -1 },
    { "DETECT", DRILL_DETECT_BROKEN,         -1 },
    { "ICI",    DRILL_INCREMENTALHEADER,     1  },
    { "FMAT",   DRILL_FMT,                   1  },  // Use Format command
    { "ATC",    DRILL_AUTOMATIC_TOOL_CHANGE, 0  },
    { "TCST",   DRILL_TOOL_CHANGE_STOP,      0  },  // Tool Change Stop
    { "AFS",    DRILL_AUTOMATIC_SPEED },            // Automatic Feeds and Speeds
    { "VER",    DRILL_AXIS_VERSION,          1  },  // Selection of X and Y Axis Version
    { "R",      DRILL_RESET_CMD,             -1 },  // Reset commands
    { "%",      DRILL_REWIND_STOP,           -1 },  // Rewind stop. End of the header
    { "/",      DRILL_SKIP,                  -1 },  // Clear Tool Linking. End of the header
    // Keep this item after all commands starting by 'T':
    { "T",      DRILL_TOOL_INFORMATION,      0  },  // Tool Information
    { "",       DRILL_M_UNKNOWN,             0  }   // last item in list
};

static EXCELLON_CMD excellon_G_CmdList[] =
{
    { "G90", DRILL_G_ABSOLUTE,    0 },  // Absolute Mode
    { "G91", DRILL_G_INCREMENTAL, 0 },  // Incremental Input Mode
    { "G90", DRILL_G_ZEROSET,     0 },  // Absolute Mode
    { "G00", DRILL_G_ROUT,        1 },  // Route Mode
    { "G05", DRILL_G_DRILL,       0 },  // Drill Mode
    { "G85", DRILL_G_SLOT,        0 },  // Drill Mode slot (oval holes)
    { "G01", DRILL_G_LINEARMOVE,  0 },  // Linear (Straight Line) Mode
    { "G02", DRILL_G_CWMOVE,      0 },  // Circular CW Mode
    { "G03", DRILL_G_CCWMOVE,     0 },  // Circular CCW Mode
    { "G93", DRILL_G_ZERO_SET,    1 },  // Zero Set (XnnYmm and coordintes origin)
    { "",    DRILL_G_UNKNOWN,     0 },  // last item in list
};


/*
 * Read a EXCELLON file.
 * Gerber classes are used because there is likeness between Gerber files
 * and Excellon files
 * DCode can easily store T code (tool size) as round (or oval) shape
 * Drill commands are similar to flashed gerber items
 * Routing commands are similar to Gerber polygons
 * coordinates have the same format as Gerber, can be given in:
 *   decimal format (i.i. floating notation format)
 *   integer 2.4 format in imperial units,
 *   integer 3.2 or 3.3 format (metric units).
 */
bool GERBVIEW_FRAME::Read_EXCELLON_File( const wxString& aFullFileName )
{
    wxString msg;
    int layer = getActiveLayer();      // current layer used in GerbView

    if( g_GERBER_List[layer] == NULL )
    {
        g_GERBER_List[layer] = new EXCELLON_IMAGE( this, layer );
    }

    EXCELLON_IMAGE* drill_Layer = (EXCELLON_IMAGE*) g_GERBER_List[layer];
    ClearMessageList();

    /* Read the gerber file */
    FILE * file = wxFopen( aFullFileName, wxT( "rt" ) );
    if( file == NULL )
    {
        msg.Printf( _( "File %s not found" ), GetChars( aFullFileName ) );
        DisplayError( this, msg, 10 );
        return false;
    }

    wxString path = wxPathOnly( aFullFileName );

    if( path != wxEmptyString )
        wxSetWorkingDirectory( path );

    bool success = drill_Layer->Read_EXCELLON_File( file, aFullFileName );

    // Display errors list
    if( m_Messages.size() > 0 )
    {
        HTML_MESSAGE_BOX dlg( this, _( "Files not found" ) );
        dlg.ListSet( m_Messages );
        dlg.ShowModal();
    }
    return success;
}

bool EXCELLON_IMAGE::Read_EXCELLON_File( FILE * aFile,
                                        const wxString & aFullFileName )
{
    /* Set the gerber scale: */
    ResetDefaultValues();

    m_FileName = aFullFileName;
    m_Current_File = aFile;

    SetLocaleTo_C_standard();

    // FILE_LINE_READER will close the file.
    if( m_Current_File == NULL )
    {
        wxMessageBox( wxT("NULL!"), m_FileName );
        return false;
    }

    FILE_LINE_READER excellonReader( m_Current_File, m_FileName );
    while( true )
    {
        if( excellonReader.ReadLine() == 0 )
            break;

        char* line = excellonReader.Line();
        char* text = StrPurge( line );

        if( *text == ';' )       // comment: skip line
            continue;

        if( m_State == EXCELLON_IMAGE::READ_HEADER_STATE )
        {
            Execute_HEADER_Command( text );
        }
        else
        {
            switch( *text )
            {
            case 'M':
                Execute_HEADER_Command( text );
                break;

            case 'G': /* Line type Gxx : command */
                Execute_EXCELLON_G_Command( text );
                break;

            case 'X':
            case 'Y':               // command like X12550Y19250
                Execute_Drill_Command(text);
                break;

            case 'I':
            case 'J':               /* Auxiliary Move command */
                m_IJPos = ReadIJCoord( text );
                if( *text == '*' )  // command like X35142Y15945J504
                {
                    Execute_Drill_Command( text);
                }
                break;

            case 'T': // Tool command
                Select_Tool( text );
                break;

            case '%':
                break;

            default:
            {
                wxString msg;
                msg.Printf( wxT( "Unexpected symbol &lt;%c&gt;" ), *text );
                if( GetParent() )
                    GetParent()->ReportMessage( msg );
            }
                break;
            }   // End switch
        }
    }
    SetLocaleTo_Default();
    return true;
}


bool EXCELLON_IMAGE::Execute_HEADER_Command( char*& text )
{
    EXCELLON_CMD* cmd = NULL;
    int           iprm;
    double        dprm;
    D_CODE*       dcode;
    wxString      msg;

    // Search command in list
    EXCELLON_CMD* candidate;

    for( unsigned ii = 0; ; ii++ )
    {
        candidate = &excellonHeaderCmdList[ii];
        int len = candidate->m_Name.size();
        if( len == 0 )                                                  // End of list reached
            break;
        if( candidate->m_Name.compare( 0, len, text, len ) == 0 )       // found.
        {
            cmd   = candidate;
            text += len;
            break;
        }
    }

    if( !cmd )
    {
        msg.Printf( wxT( "Unknown Excellon command &lt;%s&gt;" ), text );
        ReportMessage( msg );
        while( *text )
            text++;

        return false;
    }

    // Execute command
    // some do nothing
    switch( cmd->m_Code )
    {
    case DRILL_SKIP:
    case DRILL_M_UNKNOWN:
        break;

    case DRILL_M_END:
        break;

    case DRILL_M_ENDREWIND:
        break;

    case DRILL_M_MESSAGE:
        break;

    case DRILL_M_LONGMESSAGE:
        break;

    case DRILL_M_HEADER:
        m_State = READ_HEADER_STATE;
        break;

    case DRILL_M_ENDHEADER:
        m_State = READ_PROGRAM_STATE;
        break;

    case DRILL_REWIND_STOP:         // End of header. No action in a viewer
        m_State = READ_PROGRAM_STATE;
        break;

    case DRILL_M_METRIC:
        SelectUnits( true );
        break;

    case DRILL_METRICHEADER:    // command like METRIC,TZ or METRIC,LZ
        SelectUnits( true );
        if( *text != ',' )
        {
            ReportMessage( _( "METRIC command has no parameter" ) );
            break;
        }
        text++;     // skip separator
        if( *text == 'T' )
            m_NoTrailingZeros = false;
        else
            m_NoTrailingZeros = true;
        break;

    case DRILL_M_IMPERIAL:
        SelectUnits( false );
        break;

    case DRILL_IMPERIALHEADER:  // command like INCH,TZ or INCH,LZ
        SelectUnits( false );
        if( *text != ',' )
        {
            ReportMessage( _( "INCH command has no parameter" ) );
            break;
        }
        text++;     // skip separator
        if( *text == 'T' )
            m_NoTrailingZeros = false;
        else
            m_NoTrailingZeros = true;
        break;

    case DRILL_M_BEGINPATTERN:
        break;

    case DRILL_M_ENDPATTERN:
        break;

    case DRILL_M_CANNEDTEXT:
        break;

    case DRILL_M_TIPCHECK:
        break;

    case DRILL_DETECT_BROKEN:
        break;

    case DRILL_INCREMENTALHEADER:
        if( *text != ',' )
        {
            ReportMessage( _( "ICI command has no parameter" ) );
            break;
        }
        text++;     // skip separator
        // Parameter should be ON or OFF
        if( strnicmp( text, "OFF", 3 ) == 0 )
            m_Relative = false;
        else if( strnicmp( text, "ON", 2 ) == 0 )
            m_Relative = true;
        else
            ReportMessage( _( "ICI command has incorrect parameter" ) );
        break;

    case DRILL_TOOL_CHANGE_STOP:
        break;

    case DRILL_AUTOMATIC_SPEED:
        break;

    case DRILL_AXIS_VERSION:
        break;

    case DRILL_RESET_CMD:
        break;

    case DRILL_AUTOMATIC_TOOL_CHANGE:
        break;

    case DRILL_FMT:
        break;

    case DRILL_TOOL_INFORMATION:

        // Read a tool definition like T1C0.02:
        // or T1F00S00C0.02 or T1C0.02F00S00
        // Read tool number:
        iprm = ReadInt( text, false );

        // Skip Feed rate and Spindle speed, if any here
        while( *text && ( *text == 'F' || *text == 'S' ) )
        {
            text++;
            ReadInt( text, false );
        }

        // Read tool shape
        if( *text != 'C' )
            ReportMessage( wxString:: Format(
                           _( "Tool definition <%c> not supported" ), *text ) );
        if( *text )
            text++;

        //read tool diameter:
        dprm = ReadDouble( text, false );
        m_Has_DCode = true;

        // Initialize Dcode to handle this Tool
        dcode = GetDCODE( iprm + FIRST_DCODE );     // Remember: dcodes are >= FIRST_DCODE
        if( dcode == NULL )
            break;
        // conv_scale = scaling factor from inch to Internal Unit
        double conv_scale = IU_PER_MILS * 1000;
        if( m_GerbMetric )
            conv_scale /= 25.4;

        dcode->m_Size.x = dcode->m_Size.y = KiROUND( dprm * conv_scale );
        dcode->m_Shape  = APT_CIRCLE;
        break;
    }

    while( *text )
        text++;

    return true;
}


bool EXCELLON_IMAGE::Execute_Drill_Command( char*& text )
{
    D_CODE*  tool;
    GERBER_DRAW_ITEM * gbritem;
    while( true )
    {
        switch( *text )
        {
            case 'X':
                ReadXYCoord( text );
                break;
            case 'Y':
                ReadXYCoord( text );
                break;
            case 'G':  // G85 is found here for oval holes
                m_PreviousPos = m_CurrentPos;
                Execute_EXCELLON_G_Command( text );
                break;
            case 0:     // E.O.L: execute command
                tool = GetDCODE( m_Current_Tool, false );
                if( !tool )
                {
                    wxString msg;
                    msg.Printf( _( "Tool <%d> not defined" ), m_Current_Tool );
                    ReportMessage( msg );
                    return false;
                }
                gbritem = new GERBER_DRAW_ITEM( GetParent()->GetGerberLayout(), this );
                GetParent()->GetGerberLayout()->m_Drawings.Append( gbritem );
                if( m_SlotOn )  // Oval hole
                {
                    fillLineGBRITEM( gbritem,
                                    tool->m_Num_Dcode, GetParent()->getActiveLayer(),
                                    m_PreviousPos, m_CurrentPos,
                                    tool->m_Size, false );
                }
                else
                {
                    fillFlashedGBRITEM( gbritem, tool->m_Shape,
                                    tool->m_Num_Dcode, GetParent()->getActiveLayer(),
                                    m_CurrentPos,
                                    tool->m_Size, false );
                }
                StepAndRepeatItem( *gbritem );
                m_PreviousPos = m_CurrentPos;
                return true;
                break;

            default:
                text++;
                break;
        }
    }

    return true;
}


bool EXCELLON_IMAGE::Select_Tool( char*& text )
{
    int tool_id = TCodeNumber( text );

    if( tool_id >= 0 )
    {
        tool_id += FIRST_DCODE;     // Remember: dcodes are >= FIRST_DCODE
        if( tool_id > (TOOLS_MAX_COUNT - 1) )
            tool_id = TOOLS_MAX_COUNT - 1;
        m_Current_Tool = tool_id;
        D_CODE* pt_Dcode = GetDCODE( tool_id , false );
        if( pt_Dcode )
            pt_Dcode->m_InUse = true;
    }
    while( *text )
        text++;

    return tool_id >= 0;
}


bool EXCELLON_IMAGE::Execute_EXCELLON_G_Command( char*& text )
{
    EXCELLON_CMD* cmd     = NULL;
    bool          success = false;
    int           id = DRILL_G_UNKNOWN;

    // Search command in list
    EXCELLON_CMD* candidate;
    char * gcmd = text;         // gcmd points the G command, for error messages.

    for( unsigned ii = 0; ; ii++ )
    {
        candidate = &excellon_G_CmdList[ii];
        int len = candidate->m_Name.size();
        if( len == 0 )                                                  // End of list reached
            break;
        if( candidate->m_Name.compare( 0, len, text, len ) == 0 )       // found.
        {
            cmd     = candidate;
            text   += len;
            success = true;
            id = cmd->m_Code;
            break;
        }
    }

    switch( id )
    {
    case DRILL_G_ZERO_SET:
        ReadXYCoord( text );
        m_Offset = m_CurrentPos;
        break;

    case DRILL_G_ROUT:
        m_SlotOn = false;
        m_PolygonFillMode = true;
        break;

    case DRILL_G_DRILL:
        m_SlotOn = false;
        m_PolygonFillMode = false;
        break;

    case DRILL_G_SLOT:
        m_SlotOn = true;
        break;

    case DRILL_G_LINEARMOVE:
        m_Iterpolation = GERB_INTERPOL_LINEAR_1X;
        break;

    case DRILL_G_CWMOVE:
        m_Iterpolation = GERB_INTERPOL_ARC_NEG;
        break;

    case DRILL_G_CCWMOVE:
        m_Iterpolation = GERB_INTERPOL_ARC_POS;
        break;

    case DRILL_G_ABSOLUTE:
        m_Relative = false;         // false = absolute coord
        break;

    case DRILL_G_INCREMENTAL:
        m_Relative = true;          // true = relative coord
        break;

    case DRILL_G_UNKNOWN:
    default:
    {
        wxString msg;
        msg.Printf( _( "Unknown Excellon G Code: &lt;%s&gt;" ), GetChars(FROM_UTF8(gcmd)) );
        ReportMessage( msg );
        while( *text )
            text++;
        return false;
    }
    }
    return success;
}

void EXCELLON_IMAGE::SelectUnits( bool aMetric )
{
    /* Coordinates are measured either in inch or metric (millimeters).
     * Inch coordinates are in six digits (00.0000) with increments
     * as small as 0.0001 (1/10,000).
     * Metric coordinates can be measured in microns (thousandths of a millimeter)
     * in one of the following three ways:
     *  Five digit 10 micron resolution (000.00)
     *  Six digit 10 micron resolution (0000.00)
     *  Six digit micron resolution (000.000)
     */
    /* Inches: Default fmt = 2.4 for X and Y axis: 6 digits with  0.0001 resolution
     * metric: Default fmt = 3.3 for X and Y axis: 6 digits, 1 micron resolution
     */
    if( aMetric )
    {
        m_GerbMetric = true;
        // number of digits in mantissa
        m_FmtScale.x = m_FmtScale.y = fmtMantissaMM;
        // number of digits (mantissa+interger)
        m_FmtLen.x = m_FmtLen.y = fmtIntegerMM+fmtMantissaMM;
    }
    else
    {
        m_GerbMetric = false;
        m_FmtScale.x = m_FmtScale.y = fmtMantissaInch;
        m_FmtLen.x = m_FmtLen.y = fmtIntegerInch+fmtMantissaInch;
    }
}
