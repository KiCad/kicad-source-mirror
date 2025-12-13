/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 1992-2016 Jean-Pierre Charras <jp.charras at wanadoo.fr>
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


#include <math/util.h>      // for KiROUND
#include <trigo.h>          // for RotatePoint

#include <gerbview.h>
#include <gerbview_frame.h>
#include <gerber_file_image.h>
#include <gerber_file_image_list.h>
#include <excellon_image.h>
#include <excellon_defaults.h>
#include <macros.h>
#include <richio.h>
#include <string_utils.h>
#include <X2_gerber_attributes.h>
#include <view/view.h>
#include <gerbview_settings.h>

#include <cmath>
#include <charconv>

#include <dialogs/html_message_box.h>

// A helper function to calculate the arc center of an arc
// known by 2 end points, the radius, and the angle direction (CW or CCW)
// Arc angles are <= 180 degrees in circular interpol.
static VECTOR2I computeCenter( VECTOR2I aStart, VECTOR2I aEnd, int& aRadius, bool aRotCCW )
{
    VECTOR2I center;
    VECTOR2D end;
    end.x = double(aEnd.x - aStart.x);
    end.y = double(aEnd.y - aStart.y);

    // Be sure aRadius/2 > dist between aStart and aEnd
    double min_radius = end.EuclideanNorm() * 2;

    if( min_radius <= aRadius )
    {
        // Adjust the radius and the arc center for a 180 deg arc between end points
        aRadius = KiROUND( min_radius );
        center.x = ( aStart.x + aEnd.x + 1 ) / 2;
        center.y = ( aStart.y + aEnd.y + 1 ) / 2;
        return center;
    }

    /* to compute the centers position easily:
     * rotate the segment (0,0 to end.x,end.y) to make it horizontal (end.y = 0).
     * the X center position is end.x/2
     * the Y center positions are on the vertical line starting at end.x/2, 0
     * and solve aRadius^2 = X^2 + Y^2  (2 values)
     */
    EDA_ANGLE seg_angle( end );
    VECTOR2D  h_segm = end;
    RotatePoint( h_segm, seg_angle );
    double    cX = h_segm.x/2;
    double    cY1 = sqrt( (double)aRadius*aRadius - cX*cX );
    double    cY2 = -cY1;
    VECTOR2D  center1( cX, cY1 );
    RotatePoint( center1, -seg_angle );
    EDA_ANGLE arc_angle1 = EDA_ANGLE( end - center1 ) - EDA_ANGLE( VECTOR2D( 0.0, 0.0 ) - center1 );
    VECTOR2D  center2( cX, cY2 );
    RotatePoint( center2, -seg_angle );
    EDA_ANGLE arc_angle2 = EDA_ANGLE( end - center2 ) - EDA_ANGLE( VECTOR2D( 0.0, 0.0 ) - center2 );

    if( !aRotCCW )
    {
        if( arc_angle1 < ANGLE_0 )
            arc_angle1 += ANGLE_360;

        if( arc_angle2 < ANGLE_0 )
            arc_angle2 += ANGLE_360;
    }
    else
    {
        if( arc_angle1 > ANGLE_0 )
            arc_angle1 -= ANGLE_360;

        if( arc_angle2 > ANGLE_0 )
            arc_angle2 -= ANGLE_360;
    }

    // Arc angle must be <= 180.0 degrees.
    // So choose the center that create a arc angle <= 180.0
    if( std::abs( arc_angle1 ) <= ANGLE_180 )
    {
        center.x = KiROUND( center1.x );
        center.y = KiROUND( center1.y );
    }
    else
    {
        center.x = KiROUND( center2.x );
        center.y = KiROUND( center2.y );
    }

    return center+aStart;
}

extern int    ReadInt( char*& text, bool aSkipSeparator = true );
extern double ReadDouble( char*& text, bool aSkipSeparator = true );

// See rs274d.cpp:
extern void fillFlashedGBRITEM(  GERBER_DRAW_ITEM* aGbrItem,
                                 APERTURE_T        aAperture,
                                 int               Dcode_index,
                                 const VECTOR2I&   aPos,
                                 VECTOR2I          aSize,
                                 bool              aLayerNegative );

extern void fillLineGBRITEM( GERBER_DRAW_ITEM* aGbrItem,
                             int Dcode_index,
                             const VECTOR2I& aStart,
                             const VECTOR2I& aEnd,
                             VECTOR2I aPenSize,
                             bool aLayerNegative );

extern void fillArcGBRITEM( GERBER_DRAW_ITEM* aGbrItem,
                            int Dcode_index,
                            const VECTOR2I& aStart,
                            const VECTOR2I& aEnd,
                            const VECTOR2I& aRelCenter,
                            VECTOR2I aPenSize,
                            bool aClockwise,
                            bool aMultiquadrant,
                            bool aLayerNegative );

// Gerber X2 files have a file attribute which specify the type of image
// (copper, solder paste ... and sides tpo, bottom or inner copper layers)
// Excellon drill files do not have attributes, so, just to identify the image
// In gerbview, we add this attribute, similar to a Gerber drill file
static const char file_attribute[] = ".FileFunction,Other,Drill*";

static EXCELLON_CMD excellonHeaderCmdList[] =
{
    { "M0",     DRILL_M_END,                 -1 },  // End of Program - No Rewind
    { "M00",    DRILL_M_END,                 -1 },  // End of Program - No Rewind
    { "M15",    DRILL_M_TOOL_DOWN,            0 },  // tool down (starting a routed hole)
    { "M16",    DRILL_M_TOOL_UP,              0 },  // tool up (ending a routed hole)
    { "M17",    DRILL_M_TOOL_UP,              0 },  // tool up similar to M16 for a viewer
    { "M30",    DRILL_M_ENDFILE,             -1 },  // End of File (last line of NC drill)
    { "M47",    DRILL_M_MESSAGE,             -1 },  // Operator Message
    { "M45",    DRILL_M_LONGMESSAGE,         -1 },  // Long Operator message (use more than one line)
    { "M48",    DRILL_M_HEADER,               0 },  // beginning of a header
    { "M95",    DRILL_M_ENDHEADER,            0 },  // End of the header
    { "METRIC", DRILL_METRIC_HEADER,          1 },
    { "INCH",   DRILL_IMPERIAL_HEADER,        1 },
    { "M71",    DRILL_M_METRIC,               1 },
    { "M72",    DRILL_M_IMPERIAL,             1 },
    { "M25",    DRILL_M_BEGINPATTERN,         0 },  // Beginning of Pattern
    { "M01",    DRILL_M_ENDPATTERN,           0 },  // End of Pattern
    { "M97",    DRILL_M_CANNEDTEXT,          -1 },
    { "M98",    DRILL_M_CANNEDTEXT,          -1 },
    { "DETECT", DRILL_DETECT_BROKEN,         -1 },
    { "ICI",    DRILL_INCREMENTALHEADER,      1 },
    { "FMAT",   DRILL_FMT,                    1 },  // Use Format command
    { ";FILE_FORMAT",
                DRILL_FORMAT_ALTIUM,          1 },  // Use Format command
    { ";",      DRILL_HEADER_SKIP,            0 },  // Other ; hints that we don't implement
    { "ATC",    DRILL_AUTOMATIC_TOOL_CHANGE,  0 },
    { "TCST",   DRILL_TOOL_CHANGE_STOP,       0 },  // Tool Change Stop
    { "AFS",    DRILL_AUTOMATIC_SPEED,        0 },  // Automatic Feeds and Speeds
    { "VER",    DRILL_AXIS_VERSION,           1 },  // Selection of X and Y Axis Version
    { "R",      DRILL_RESET_CMD,             -1 },  // Reset commands
    { "%",      DRILL_REWIND_STOP,           -1 },  // Rewind stop. End of the header
    { "/",      DRILL_SKIP,                  -1 },  // Clear Tool Linking. End of the header
    // Keep this item after all commands starting by 'T':
    { "T",      DRILL_TOOL_INFORMATION,       0 },  // Tool Information
    { "",       DRILL_M_UNKNOWN,              0 }   // last item in list
};

static EXCELLON_CMD excellon_G_CmdList[] =
{
    { "G90", DRILL_G_ABSOLUTE,    0 },  // Absolute Mode
    { "G91", DRILL_G_INCREMENTAL, 0 },  // Incremental Input Mode
    { "G90", DRILL_G_ZEROSET,     0 },  // Absolute Mode
    { "G00", DRILL_G_ROUT,        1 },  // Route Mode
    { "G05", DRILL_G_DRILL,       0 },  // Drill Mode
    { "G85", DRILL_G_SLOT,        0 },  // Canned Mode slot (oval holes)
    { "G01", DRILL_G_LINEARMOVE,  1 },  // Linear (Straight Line) routing Mode
    { "G02", DRILL_G_CWMOVE,      1 },  // Circular CW Mode
    { "G03", DRILL_G_CCWMOVE,     1 },  // Circular CCW Mode
    { "G93", DRILL_G_ZERO_SET,    1 },  // Zero Set (XnnYmm and coordinates origin)
    { "",    DRILL_G_UNKNOWN,     0 },  // last item in list
};


bool GERBVIEW_FRAME::Read_EXCELLON_File( const wxString& aFullFileName )
{
    wxString msg;
    int layerId = GetActiveLayer();      // current layer used in GerbView
    GERBER_FILE_IMAGE_LIST* images = GetGerberLayout()->GetImagesList();
    GERBER_FILE_IMAGE* gerber_layer = images->GetGbrImage( layerId );

    // If the active layer contains old gerber or nc drill data, remove it
    if( gerber_layer )
        Erase_Current_DrawLayer( false );

    std::unique_ptr<EXCELLON_IMAGE> drill_layer_uptr = std::make_unique<EXCELLON_IMAGE>( layerId );

    EXCELLON_DEFAULTS nc_defaults;
    GERBVIEW_SETTINGS* cfg = static_cast<GERBVIEW_SETTINGS*>( config() );
    cfg->GetExcellonDefaults( nc_defaults );

    // Read the Excellon drill file:
    bool success = drill_layer_uptr->LoadFile( aFullFileName, &nc_defaults );

    if( !success )
    {
        drill_layer_uptr.reset();
        msg.Printf( _( "File %s not found." ), aFullFileName );
        ShowInfoBarError( msg );
        return false;
    }

    EXCELLON_IMAGE* drill_layer = drill_layer_uptr.release();

    layerId = images->AddGbrImage( drill_layer, layerId );

    if( layerId < 0 )
    {
        delete drill_layer;
        ShowInfoBarError( _( "No empty layers to load file into." ) );
        return false;
    }

    // Display errors list
    if( drill_layer->GetMessages().size() > 0 )
    {
        HTML_MESSAGE_BOX dlg( this, _( "Error reading EXCELLON drill file" ) );
        dlg.ListSet( drill_layer->GetMessages() );
        dlg.ShowModal();
    }

    if( GetCanvas() )
    {
        for( GERBER_DRAW_ITEM* item : drill_layer->GetItems() )
            GetCanvas()->GetView()->Add( (KIGFX::VIEW_ITEM*) item );
    }

    return success;
}


void EXCELLON_IMAGE::ResetDefaultValues()
{
    GERBER_FILE_IMAGE::ResetDefaultValues();
    SelectUnits( false, nullptr );      // Default unit = inch
    m_hasFormat = false;                // will be true if a Altium file containing
                                        // the nn:mm file format is read

    // Files using non decimal can use No Trailing zeros or No leading Zeros
    // Unfortunately, the identifier (INCH,TZ or INCH,LZ for instance) is not
    // always set in drill files.
    // The option leading zeros looks like more frequent, so use this default
    m_NoTrailingZeros = true;
}


/*
 * Original function derived from drill_file_p() of gerbv 2.7.0.
 * Copyright of the source file drill.cpp included below:
 */
/*
 * gEDA - GNU Electronic Design Automation
 * drill.c
 * Copyright (C) 2000-2006 Andreas Andersson
 *
 * $Id$
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 */
bool EXCELLON_IMAGE::TestFileIsExcellon( const wxString& aFullFileName )
{
    char* letter;
    bool  foundM48 = false;
    bool  foundM30 = false;
    bool  foundPercent = false;
    bool  foundT = false;
    bool  foundX = false;
    bool  foundY = false;

    FILE* file = wxFopen( aFullFileName, "rb" );

    if( file == nullptr )
        return false;

    FILE_LINE_READER excellonReader( file, aFullFileName );

    try
    {
        while( true )
        {
            if( excellonReader.ReadLine() == nullptr )
                break;

            // Remove all whitespace from the beginning and end
            char* line = StrPurge( excellonReader.Line() );

            // Skip empty lines
            if( *line == 0 )
                continue;

            // Check that file is not binary (non-printing chars)
            size_t len = strlen( line );

            for( size_t i = 0; i < len; i++ )
            {
                if( !isascii( line[i] ) )
                    return false;
            }

            // We don't want to look for any commands after a comment so
            // just end the line early if we find a comment
            char* buf = strstr( line, ";" );
            if( buf != nullptr )
                *buf = 0;

            // Check for M48 = start of drill header
            if( strstr( line, "M48" ) )
                foundM48 = true;

            // Check for M30 = end of drill program
            if( strstr( line, "M30" ) )
                if( foundPercent )
                    foundM30 = true; // Found M30 after % = good

            // Check for % on its own line at end of header
            if( ( letter = strstr( line, "%" ) ) != nullptr )
                if( ( letter[1] == '\r' ) || ( letter[1] == '\n' ) )
                    foundPercent = true;

            // Check for T<number>
            if( ( letter = strstr( line, "T" ) ) != nullptr )
            {
                if( !foundT && ( foundX || foundY ) )
                    foundT = false; /* Found first T after X or Y */
                else
                {
                    double x_val;

                    if( wxString( letter + 1 ).ToCDouble( &x_val ) )
                        foundT = true;
                }
            }

            // look for X<number> or Y<number>
            if( ( letter = strstr( line, "X" ) ) != nullptr )
            {
                double x_val;

                if( wxString( letter + 1 ).ToCDouble( &x_val ) )
                    foundX = true;
            }

            if( ( letter = strstr( line, "Y" ) ) != nullptr )
            {
                double x_val;

                if( wxString( letter + 1 ).ToCDouble( &x_val ) )
                    foundY = true;
            }
        }
    }
    catch( IO_ERROR& )
    {
        return false;
    }


    /* Now form logical expression determining if this is a drill file */
    if( ( ( foundX || foundY ) && foundT ) && ( foundM48 || ( foundPercent && foundM30 ) ) )
        return true;
    else if( foundM48 && foundT && foundPercent && foundM30 )
        /* Pathological case of drill file with valid header
           and EOF but no drill XY locations. */
        return true;

    return false;
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
bool EXCELLON_IMAGE::LoadFile( const wxString & aFullFileName, EXCELLON_DEFAULTS* aDefaults )
{
    // Set the default parameter values:
    ResetDefaultValues();
    ClearMessageList();

    m_Current_File = wxFopen( aFullFileName, wxT( "rt" ) );

    if( m_Current_File == nullptr )
        return false;

    // Initial format setting, usualy defined in file, but not always...
    m_NoTrailingZeros = aDefaults->m_LeadingZero;
    m_GerbMetric = aDefaults->m_UnitsMM;

    wxString msg;
    m_FileName = aFullFileName;

    // FILE_LINE_READER will close the file.
    FILE_LINE_READER excellonReader( m_Current_File, m_FileName );

    while( true )
    {
        if( excellonReader.ReadLine() == nullptr )
            break;

        char* line = excellonReader.Line();
        char* text = StrPurge( line );

        if( *text == 0 ) // Skip empty lines
            continue;

        if( m_State == EXCELLON_IMAGE::READ_HEADER_STATE )
        {
            Execute_HEADER_And_M_Command( text );

            // Now units (inch/mm) are known, set the coordinate format
            SelectUnits( m_GerbMetric, aDefaults );
        }
        else
        {
            switch( *text )
            {
            case ';':
            case 'M':
                Execute_HEADER_And_M_Command( text );
                break;

            case 'G':       // Line type Gxx : command
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

            case 'T':               // Select Tool command (can also create
                                    // the tool with an embedded definition)
                Select_Tool( text );
                break;

            case '%':
                break;

            default:
                msg.Printf( wxT( "Unexpected symbol 0x%2.2X &lt;%c&gt;" ), *text, *text );
                AddMessageToList( msg );
                break;
            }   // End switch
        }
    }

    // Add our file attribute, to identify the drill file
    X2_ATTRIBUTE dummy;
    char* text = (char*)file_attribute;
    int dummyline = 0;
    dummy.ParseAttribCmd( nullptr, nullptr, 0, text, dummyline );
    delete m_FileFunction;
    m_FileFunction = new X2_ATTRIBUTE_FILEFUNCTION( dummy );

    m_InUse = true;

    return true;
}


bool EXCELLON_IMAGE::Execute_HEADER_And_M_Command( char*& text )
{
    EXCELLON_CMD* cmd = nullptr;
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
        msg.Printf( _( "Unknown Excellon command &lt;%s&gt;" ), text );
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
    case DRILL_M_ENDFILE:
        // if a route command is in progress, finish it
        if( m_RouteModeOn )
            FinishRouteCommand();

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

    case DRILL_FORMAT_ALTIUM:
        readFileFormat( text );
        break;

    case DRILL_HEADER_SKIP:
        break;

    case DRILL_M_METRIC:
        SelectUnits( true, nullptr );
        break;

    case DRILL_IMPERIAL_HEADER:  // command like INCH,TZ or INCH,LZ
    case DRILL_METRIC_HEADER:    // command like METRIC,TZ or METRIC,LZ
        SelectUnits( cmd->m_Code == DRILL_METRIC_HEADER ? true : false, nullptr );

        if( *text != ',' )
        {
            // No TZ or LZ specified. Should be a decimal format
            // but this is not always the case. Use our default setting
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
            AddMessageToList( wxT( "ICI command has no parameter" ) );
            break;
        }
        text++;     // skip separator
        // Parameter should be ON or OFF
        if( strncasecmp( text, "OFF", 3 ) == 0 )
            m_Relative = false;
        else if( strncasecmp( text, "ON", 2 ) == 0 )
            m_Relative = true;
        else
            AddMessageToList( wxT( "ICI command has incorrect parameter" ) );
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

    case DRILL_M_TOOL_DOWN:      // tool down (starting a routed hole or polyline)
        // Only the last position is useful:
        if( m_RoutePositions.size() > 1 )
            m_RoutePositions.erase( m_RoutePositions.begin(), m_RoutePositions.begin() + m_RoutePositions.size() - 1 );

        break;

    case DRILL_M_TOOL_UP:        // tool up (ending a routed polyline)
        FinishRouteCommand();
        break;
    }

    while( *text )
        text++;

    return true;
}


void EXCELLON_IMAGE::readFileFormat( char*& aText )
{
    int mantissaDigits = 0;
    int characteristicDigits = 0;

    // Example String: ;FILE_FORMAT=4:4
    // The ;FILE_FORMAT potion will already be stripped off.
    // Parse the rest strictly as single_digit:single_digit like 4:4 or 2:4
    // Don't allow anything clever like spaces or multiple digits
    if( *aText != '=' )
        return;

    aText++;

    if( !isdigit( *aText ) )
        return;

    characteristicDigits = *aText - '0';
    aText++;

    if( *aText != ':' )
        return;

    aText++;

    if( !isdigit( *aText ) )
        return;

    mantissaDigits = *aText - '0';

    m_hasFormat = true;
    m_FmtLen.x = m_FmtLen.y = characteristicDigits + mantissaDigits;
    m_FmtScale.x = m_FmtScale.y = mantissaDigits;
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
    D_CODE* dcode = GetDCODEOrCreate( iprm + FIRST_DCODE );

    if( dcode == nullptr )
        return false;

    // conv_scale = scaling factor from inch to Internal Unit
    double conv_scale = gerbIUScale.IU_PER_MILS * 1000;

    if( m_GerbMetric )
        conv_scale /= 25.4;

    dcode->m_Size.x = dcode->m_Size.y = KiROUND( dprm * conv_scale );
    dcode->m_ApertType = APT_CIRCLE;
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
        case 'Y':
            ReadXYCoord( text, true );

            if( *text == 'I' || *text == 'J' )
                ReadIJCoord( text );

            break;

        case 'G':  // G85 is found here for oval holes
            m_PreviousPos = m_CurrentPos;
            Execute_EXCELLON_G_Command( text );
            break;

        case 0:     // E.O.L: execute command
            if( m_RouteModeOn )
            {
                // We are in routing mode, and this is an intermediate point.
                // So just store it
                int rmode = 0;  // linear routing.

                if( m_Iterpolation == GERB_INTERPOL_ARC_NEG )
                    rmode = ROUTE_CW;
                else if( m_Iterpolation == GERB_INTERPOL_ARC_POS )
                    rmode = ROUTE_CCW;

                if( m_LastArcDataType == ARC_INFO_TYPE_CENTER )
                {
                    EXCELLON_ROUTE_COORD point( m_CurrentPos, m_IJPos, rmode );
                    m_RoutePositions.push_back( point );
                }
                else
                {
                    EXCELLON_ROUTE_COORD point( m_CurrentPos, m_ArcRadius, rmode );
                    m_RoutePositions.push_back( point );
                }
                return true;
            }

            tool = GetDCODE( m_Current_Tool );
            if( !tool )
            {
                wxString msg;
                msg.Printf( _( "Tool %d not defined" ), m_Current_Tool );
                AddMessageToList( msg );
                return false;
            }

            gbritem = new GERBER_DRAW_ITEM( this );
            AddItemToList( gbritem );

            if( m_SlotOn )  // Oblong hole
            {
                fillLineGBRITEM( gbritem, tool->m_Num_Dcode, m_PreviousPos, m_CurrentPos, tool->m_Size, false );
                // the hole is made: reset the slot on command (G85)
                // (it is needed for each oblong hole)
                m_SlotOn = false;
            }
            else
            {
                fillFlashedGBRITEM( gbritem, tool->m_ApertType, tool->m_Num_Dcode, m_CurrentPos, tool->m_Size,
                                    false );
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
    int tool_id = CodeNumber( text );

    // T0 is legal, but is not a selection tool. it is a special command
    if( tool_id >= 0 )
    {
        int dcode_id = tool_id + FIRST_DCODE;     // Remember: dcodes are >= FIRST_DCODE

        if( !D_CODE::IsValidDcodeValue(dcode_id ) )
            dcode_id = LAST_DCODE;

        m_Current_Tool = dcode_id;
        D_CODE* currDcode = GetDCODE( dcode_id );

        // if the tool does not exist, and the definition is embedded, create this tool
        if( currDcode == nullptr && tool_id > 0 )
        {
            text = startline;   // text starts at the beginning of the command
            readToolInformation( text );
            currDcode = GetDCODE( dcode_id );
        }

        // If the Tool is still not existing, create a dummy tool
        if( !currDcode )
            currDcode = GetDCODEOrCreate( dcode_id, true );

        if( currDcode )
            currDcode->m_InUse = true;
    }

    while( *text )
        text++;

    return tool_id >= 0;
}


bool EXCELLON_IMAGE::Execute_EXCELLON_G_Command( char*& text )
{
    EXCELLON_CMD* cmd     = nullptr;
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
        ReadXYCoord( text, true );
        m_Offset = m_CurrentPos;
        break;

    case DRILL_G_ROUT:
        m_SlotOn = false;

        if( m_RouteModeOn )
            FinishRouteCommand();

        m_RouteModeOn = true;
        m_RoutePositions.clear();
        m_LastArcDataType = ARC_INFO_TYPE_NONE;
        ReadXYCoord( text, true );
        // This is the first point (starting point) of routing
        m_RoutePositions.emplace_back( m_CurrentPos );
        break;

    case DRILL_G_DRILL:
        m_SlotOn = false;

        if( m_RouteModeOn )
            FinishRouteCommand();

        m_RouteModeOn = false;
        m_RoutePositions.clear();
        m_LastArcDataType = ARC_INFO_TYPE_NONE;
        break;

    case DRILL_G_SLOT:
        m_SlotOn = true;
        break;

    case DRILL_G_LINEARMOVE:
        m_LastArcDataType = ARC_INFO_TYPE_NONE;
        m_Iterpolation = GERB_INTERPOL_LINEAR_1X;
        ReadXYCoord( text, true );
        m_RoutePositions.emplace_back( m_CurrentPos );
        break;

    case DRILL_G_CWMOVE:
        m_Iterpolation = GERB_INTERPOL_ARC_NEG;
        ReadXYCoord( text, true );

        if( *text == 'I' || *text == 'J' )
            ReadIJCoord( text );

        if( m_LastArcDataType == ARC_INFO_TYPE_CENTER )
            m_RoutePositions.emplace_back( m_CurrentPos, m_IJPos, ROUTE_CW );
        else
            m_RoutePositions.emplace_back( m_CurrentPos, m_ArcRadius, ROUTE_CW );

        break;

    case DRILL_G_CCWMOVE:
        m_Iterpolation = GERB_INTERPOL_ARC_POS;
        ReadXYCoord( text, true );

        if( *text == 'I' || *text == 'J' )
            ReadIJCoord( text );

        if( m_LastArcDataType == ARC_INFO_TYPE_CENTER )
            m_RoutePositions.emplace_back( m_CurrentPos, m_IJPos, ROUTE_CCW );
        else
            m_RoutePositions.emplace_back( m_CurrentPos, m_ArcRadius, ROUTE_CCW );

        break;

    case DRILL_G_ABSOLUTE:
        m_Relative = false;         // false = absolute coord
        break;

    case DRILL_G_INCREMENTAL:
        m_Relative = true;          // true = relative coord
        break;

    case DRILL_G_UNKNOWN:
    default:
        AddMessageToList( wxString::Format( _( "Unknown Excellon G Code: &lt;%s&gt;" ), From_UTF8(gcmd) ) );

        while( *text )
            text++;

        return false;
    }

    return success;
}

void EXCELLON_IMAGE::SelectUnits( bool aMetric, EXCELLON_DEFAULTS* aDefaults )
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
     *
     * However some drill files do not use standard values.
     */
    if( aMetric )
    {
        m_GerbMetric = true;

        if( !m_hasFormat )
        {
            if( aDefaults )
            {
                // number of digits in mantissa
                m_FmtScale.x = m_FmtScale.y = aDefaults->m_MmMantissaLen;
                // number of digits (mantissa+integer)
                m_FmtLen.x = m_FmtLen.y = aDefaults->m_MmIntegerLen
                                          + aDefaults->m_MmMantissaLen;
            }
            else
            {
                m_FmtScale.x = m_FmtScale.y = FMT_MANTISSA_MM;
                m_FmtLen.x = m_FmtLen.y = FMT_INTEGER_MM + FMT_MANTISSA_MM;
            }
        }
    }
    else
    {
        m_GerbMetric = false;

        if( !m_hasFormat )
        {
            if( aDefaults )
            {
                m_FmtScale.x = m_FmtScale.y = aDefaults->m_InchMantissaLen;
                m_FmtLen.x = m_FmtLen.y = aDefaults->m_InchIntegerLen
                                          + aDefaults->m_InchMantissaLen;
            }
            else
            {
                m_FmtScale.x = m_FmtScale.y = FMT_MANTISSA_INCH;
                m_FmtLen.x = m_FmtLen.y = FMT_INTEGER_INCH + FMT_MANTISSA_INCH;
            }
        }
    }
}


void EXCELLON_IMAGE::FinishRouteCommand()
{
    // Ends a route command started by M15 ot G01, G02 or G03 command
    // if a route command is not in progress, do nothing

    if( !m_RouteModeOn )
        return;

    D_CODE* tool = GetDCODE( m_Current_Tool );

    if( !tool )
    {
        AddMessageToList( wxString::Format( wxT( "Unknown tool code %d" ), m_Current_Tool ) );
        return;
    }

    for( size_t ii = 1; ii < m_RoutePositions.size(); ii++ )
    {
        GERBER_DRAW_ITEM* gbritem = new GERBER_DRAW_ITEM( this );

        if( m_RoutePositions[ii].m_rmode == 0 )     // linear routing
        {
        fillLineGBRITEM( gbritem, tool->m_Num_Dcode,
                        m_RoutePositions[ii-1].GetPos(), m_RoutePositions[ii].GetPos(),
                        tool->m_Size, false );
        }
        else    // circular (cw or ccw) routing
        {
        bool rot_ccw = m_RoutePositions[ii].m_rmode == ROUTE_CW;
        int radius = m_RoutePositions[ii].m_radius; // Can be adjusted by computeCenter.
        VECTOR2I center;

        if( m_RoutePositions[ii].m_arc_type_info == ARC_INFO_TYPE_CENTER )
            center = VECTOR2I( m_RoutePositions[ii].m_cx, m_RoutePositions[ii].m_cy );
        else
            center = computeCenter( m_RoutePositions[ii-1].GetPos(),
                                    m_RoutePositions[ii].GetPos(), radius, rot_ccw );

        fillArcGBRITEM( gbritem, tool->m_Num_Dcode,
                         m_RoutePositions[ii-1].GetPos(), m_RoutePositions[ii].GetPos(),
                         center - m_RoutePositions[ii-1].GetPos(),
                         tool->m_Size, not rot_ccw , true,
                         false );
        }

        AddItemToList( gbritem );

        StepAndRepeatItem( *gbritem );
    }

    m_RoutePositions.clear();
    m_RouteModeOn = false;
}
