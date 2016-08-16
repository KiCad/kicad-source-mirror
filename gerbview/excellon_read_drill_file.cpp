/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 1992-2016 Jean-Pierre Charras <jp.charras at wanadoo.fr>
 * Copyright (C) 1992-2016 KiCad Developers, see AUTHORS.txt for contributors.
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

/**
 * @file excellon_read_drill_file.cpp
 *  Functions to read drill files (EXCELLON format) created by Pcbnew
 *  These files use only a subset of EXCELLON commands.
 */


#include <fctsys.h>
#include <common.h>
#include <confirm.h>

#include <gerbview.h>
#include <gerbview_frame.h>
#include <class_gerber_file_image.h>
#include <class_gerber_file_image_list.h>
#include <class_excellon.h>
#include <kicad_string.h>
#include <class_X2_gerber_attributes.h>

#include <cmath>

#include <html_messagebox.h>

// Default format for dimensions: they are the default values, not the actual values
// number of digits in mantissa:
static const int fmtMantissaMM = 3;
static const int fmtMantissaInch = 4;
// number of digits, integer part:
static const int fmtIntegerMM = 3;
static const int fmtIntegerInch = 2;

extern int    ReadInt( char*& text, bool aSkipSeparator = true );
extern double ReadDouble( char*& text, bool aSkipSeparator = true );

// See ds274d.cpp:
extern void fillFlashedGBRITEM(  GERBER_DRAW_ITEM* aGbrItem,
                                 APERTURE_T        aAperture,
                                 int               Dcode_index,
                                 const wxPoint&    aPos,
                                 wxSize            aSize,
                                 bool              aLayerNegative );
void fillLineGBRITEM(  GERBER_DRAW_ITEM* aGbrItem,
                              int               Dcode_index,
                              const wxPoint&    aStart,
                              const wxPoint&    aEnd,
                              wxSize            aPenSize,
                              bool              aLayerNegative  );

// Getber X2 files have a file attribute which specify the type of image
// (copper, solder paste ... and sides tpo, bottom or inner copper layers)
// Excellon drill files do not have attributes, so, just to identify the image
// In gerbview, we add this attribute, like a Gerber drill file
static const char file_attribute[] = ".FileFunction,Other,Drill*";

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


bool GERBVIEW_FRAME::Read_EXCELLON_File( const wxString& aFullFileName )
{
    wxString msg;
    int layerId = getActiveLayer();      // current layer used in GerbView
    GERBER_FILE_IMAGE_LIST* images = GetGerberLayout()->GetImagesList();
    EXCELLON_IMAGE* drill_Layer = (EXCELLON_IMAGE*) images->GetGbrImage( layerId );

    if( drill_Layer == NULL )
    {
        drill_Layer = new EXCELLON_IMAGE( layerId );
        layerId = images->AddGbrImage( drill_Layer, layerId );
    }

    if( layerId < 0 )
    {
        DisplayError( this, _( "No room to load file" ) );
        return false;
    }

    // Read the Excellon drill file:
    bool success = drill_Layer->LoadFile( aFullFileName );

    if( !success )
    {
        msg.Printf( _( "File %s not found" ), GetChars( aFullFileName ) );
        DisplayError( this, msg );
        return false;
    }

    // Display errors list
    if( drill_Layer->GetMessages().size() > 0 )
    {
        HTML_MESSAGE_BOX dlg( this, _( "Error reading EXCELLON drill file" ) );
        dlg.ListSet( drill_Layer->GetMessages() );
        dlg.ShowModal();
    }
    return success;
}

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

bool EXCELLON_IMAGE::LoadFile( const wxString & aFullFileName )
{
    // Set the default parmeter values:
    ResetDefaultValues();
    ClearMessageList();

    m_Current_File = wxFopen( aFullFileName, wxT( "rt" ) );

    if( m_Current_File == NULL )
        return false;

    m_FileName = aFullFileName;

    LOCALE_IO toggleIo;

    // FILE_LINE_READER will close the file.
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
                AddMessageToList( msg );
            }
                break;
            }   // End switch
        }
    }

    // Add our file attribute, to identify the drill file
    X2_ATTRIBUTE dummy;
    char* text = (char*)file_attribute;
    dummy.ParseAttribCmd( m_Current_File, NULL, 0, text );
    delete m_FileFunction;
    m_FileFunction = new X2_ATTRIBUTE_FILEFUNCTION( dummy );

    m_InUse = true;

    return true;
}


bool EXCELLON_IMAGE::Execute_HEADER_Command( char*& text )
{
    EXCELLON_CMD* cmd = NULL;
    wxString      msg;

    // Search command in list
    for( unsigned ii = 0; ; ii++ )
    {
        EXCELLON_CMD* candidate = &excellonHeaderCmdList[ii];
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
        AddMessageToList( msg );
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
            AddMessageToList( _( "METRIC command has no parameter" ) );
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
            AddMessageToList( _( "INCH command has no parameter" ) );
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
            AddMessageToList( _( "ICI command has no parameter" ) );
            break;
        }
        text++;     // skip separator
        // Parameter should be ON or OFF
        if( strncasecmp( text, "OFF", 3 ) == 0 )
            m_Relative = false;
        else if( strncasecmp( text, "ON", 2 ) == 0 )
            m_Relative = true;
        else
            AddMessageToList( _( "ICI command has incorrect parameter" ) );
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
        readToolInformation( text );
        break;
    }

    while( *text )
        text++;

    return true;
}


bool EXCELLON_IMAGE::readToolInformation( char*& aText )
{
    // Read a tool definition like T1C0.02 or T1F00S00C0.02 or T1C0.02F00S00
    // and enter the TCODE param in list (using the D_CODE param management, which
    // is similar to TCODE params.
    if( *aText == 'T' )     // This is the beginning of the definition
        aText++;

    // Read tool number:
    int iprm = ReadInt( aText, false );

    // Skip Feed rate and Spindle speed, if any here
    while( *aText && ( *aText == 'F' || *aText == 'S' ) )
    {
        aText++;
        ReadInt( aText, false );
    }

    // Read tool shape
    if( ! *aText )
        AddMessageToList( wxString:: Format(
                       _( "Tool definition shape not found" ) ) );
    else if( *aText != 'C' )
        AddMessageToList( wxString:: Format(
                       _( "Tool definition '%c' not supported" ), *aText ) );
    if( *aText )
        aText++;

    //read tool diameter:
    double dprm = ReadDouble( aText, false );
    m_Has_DCode = true;

    // Initialize Dcode to handle this Tool
    // Remember: dcodes are >= FIRST_DCODE
    D_CODE* dcode = GetDCODE( iprm + FIRST_DCODE );

    if( dcode == NULL )
        return false;

    // conv_scale = scaling factor from inch to Internal Unit
    double conv_scale = IU_PER_MILS * 1000;

    if( m_GerbMetric )
        conv_scale /= 25.4;

    dcode->m_Size.x = dcode->m_Size.y = KiROUND( dprm * conv_scale );
    dcode->m_Shape  = APT_CIRCLE;
    dcode->m_Defined  = true;

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
                    msg.Printf( _( "Tool %d not defined" ), m_Current_Tool );
                    AddMessageToList( msg );
                    return false;
                }

                gbritem = new GERBER_DRAW_ITEM( this );
                m_Drawings.Append( gbritem );

                if( m_SlotOn )  // Oblong hole
                {
                    fillLineGBRITEM( gbritem, tool->m_Num_Dcode,
                                    m_PreviousPos, m_CurrentPos,
                                    tool->m_Size, false );
                    // the hole is made: reset the slot on command (G85)
                    // (it is needed for each oblong hole)
                    m_SlotOn = false;
                }
                else
                {
                    fillFlashedGBRITEM( gbritem, tool->m_Shape, tool->m_Num_Dcode,
                                    m_CurrentPos, tool->m_Size, false );
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
    // Select the tool from the command line Tn, with n = 1 ... TOOLS_MAX_COUNT - 1
    // Because some drill file have an embedded TCODE definition (like T1C.008F0S0)
    // in tool selection command, if the tool is not defined in list,
    // and the definition is embedded, it will be entered in list
    char * startline = text;    // the tool id starts here.
    int tool_id = TCodeNumber( text );

    // T0 is legal, but is not a selection tool. it is a special command
    if( tool_id >= 0 )
    {
        int dcode_id = tool_id + FIRST_DCODE;     // Remember: dcodes are >= FIRST_DCODE

        if( dcode_id > (TOOLS_MAX_COUNT - 1) )
            dcode_id = TOOLS_MAX_COUNT - 1;

        m_Current_Tool = dcode_id;
        D_CODE* currDcode = GetDCODE( dcode_id , false );

        if( currDcode == NULL && tool_id > 0 )   // if the definition is embedded, enter it
        {
            text = startline;   // text starts at the beginning of the command
            readToolInformation( text );
            currDcode = GetDCODE( dcode_id , false );
        }

        if( currDcode )
            currDcode->m_InUse = true;
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
        AddMessageToList( msg );
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
     *
     * Inches: Default fmt = 2.4 for X and Y axis: 6 digits with  0.0001 resolution
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
