/**
 * @file gerber_file_image.cpp
 * a GERBER class handle for a given layer info about used D_CODES and how the layer is drawn
 */

/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 1992-2019 Jean-Pierre Charras  jp.charras at wanadoo.fr
 * Copyright (C) 1992-2019 KiCad Developers, see AUTHORS.txt for contributors.
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

#include <convert_to_biu.h>
#include <gerbview.h>
#include <gerbview_frame.h>
#include <gerber_file_image.h>
#include <macros.h>
#include <X2_gerber_attributes.h>
#include <algorithm>
#include <map>
#include <core/arraydim.h>


/**
 * Function scaletoIU
 * converts a distance given in floating point to our internal units
 */
extern int scaletoIU( double aCoord, bool isMetric );       // defined it rs274d_read_XY_and_IJ_coordinates.cpp

/* Format Gerber: NOTES:
 * Tools and D_CODES
 *   tool number (identification of shapes)
 *   1 to 999
 *
 * D_CODES:
 *   D01 ... D9 = action codes:
 *   D01 = activating light (lower pen) when di placement
 *   D02 = light extinction (lift pen) when di placement
 *   D03 Flash
 *   D09 = VAPE Flash
 *   D10 ... = Identification Tool (Opening)
 *
 * For tools:
 * DCode min = D10
 * DCode max = 999
 */


GERBER_LAYER::GERBER_LAYER()
{
    ResetDefaultValues();
}


GERBER_LAYER::~GERBER_LAYER()
{
}


void GERBER_LAYER::ResetDefaultValues()
{
    m_LayerName     = wxT( "no name" );             // Layer name from the LN command
    m_LayerNegative = false;                        // true = Negative Layer
    m_StepForRepeat.x     = m_StepForRepeat.y = 0;  // X and Y offsets for Step and Repeat command
    m_XRepeatCount        = 1;                      // The repeat count on X axis
    m_YRepeatCount        = 1;                      // The repeat count on Y axis
    m_StepForRepeatMetric = false;                  // false = Inches, true = metric
}


GERBER_FILE_IMAGE::GERBER_FILE_IMAGE( int aLayer ) :
    EDA_ITEM( nullptr, GERBER_IMAGE_T )
{
    m_GraphicLayer = aLayer;        // Graphic layer Number
    m_IsVisible    = true;          // must be drawn
    m_PositiveDrawColor  = WHITE;   // The color used to draw positive items for this image

    m_Selected_Tool = 0;
    m_FileFunction = nullptr;          // file function parameters

    ResetDefaultValues();

    for( unsigned ii = 0; ii < arrayDim( m_Aperture_List ); ii++ )
        m_Aperture_List[ii] = nullptr;
}


GERBER_FILE_IMAGE::~GERBER_FILE_IMAGE()
{

    for( auto item : GetItems() )
        delete item;

    m_drawings.clear();

    for( unsigned ii = 0; ii < arrayDim( m_Aperture_List ); ii++ )
    {
        delete m_Aperture_List[ii];
    }

    delete m_FileFunction;
}


D_CODE* GERBER_FILE_IMAGE::GetDCODEOrCreate( int aDCODE, bool aCreateIfNoExist )
{
    unsigned ndx = aDCODE - FIRST_DCODE;

    if( ndx < (unsigned) arrayDim( m_Aperture_List ) )
    {
        // lazily create the D_CODE if it does not exist.
        if( aCreateIfNoExist )
        {
            if( m_Aperture_List[ndx] == nullptr )
                m_Aperture_List[ndx] = new D_CODE( ndx + FIRST_DCODE );
        }

        return m_Aperture_List[ndx];
    }

    return nullptr;
}


D_CODE* GERBER_FILE_IMAGE::GetDCODE( int aDCODE ) const
{
    unsigned ndx = aDCODE - FIRST_DCODE;

    if( ndx < (unsigned) arrayDim( m_Aperture_List ) )
    {
        return m_Aperture_List[ndx];
    }

    return nullptr;
}


APERTURE_MACRO* GERBER_FILE_IMAGE::FindApertureMacro( const APERTURE_MACRO& aLookup )
{
    APERTURE_MACRO_SET::iterator iter = m_aperture_macros.find( aLookup );

    if( iter != m_aperture_macros.end() )
    {
        APERTURE_MACRO* pam = (APERTURE_MACRO*) &(*iter);
        return pam;
    }

    return nullptr;    // not found
}


void GERBER_FILE_IMAGE::ResetDefaultValues()
{
    m_InUse         = false;
    m_GBRLayerParams.ResetDefaultValues();
    m_FileName.Empty();
    m_ImageName     = wxT( "no name" );             // Image name from the IN command
    m_ImageNegative = false;                        // true = Negative image
    m_IsX2_file     = false;                        // true only if a %TF, %TA or %TD command
    delete m_FileFunction;                          // file function parameters
    m_FileFunction = nullptr;
    m_MD5_value.Empty();                            // MD5 value found in a %TF.MD5 command
    m_PartString.Empty();                           // string found in a %TF.Part command
    m_hasNegativeItems    = -1;                     // set to uninitialized
    m_ImageJustifyOffset  = wxPoint(0,0);           // Image justify Offset
    m_ImageJustifyXCenter = false;                  // Image Justify Center on X axis (default = false)
    m_ImageJustifyYCenter = false;                  // Image Justify Center on Y axis (default = false)
    m_GerbMetric    = false;                        // false = Inches (default), true = metric
    m_Relative = false;                             // false = absolute Coord,
                                                    // true = relative Coord
    m_NoTrailingZeros = false;                      // true: trailing zeros deleted
    m_ImageOffset.x   = m_ImageOffset.y = 0;        // Coord Offset, from IO command
    m_ImageRotation = 0;                            // Allowed 0, 90, 180, 270 (in degree)
    m_LocalRotation = 0.0;                          // Layer rotation from RO command (in 0.1 degree)
    m_Offset.x = 0;
    m_Offset.y = 0;                                 // Coord Offset, from OF command
    m_Scale.x  = m_Scale.y = 1.0;                   // scale (A and B) this layer
    m_MirrorA  = false;                             // true: mirror / axe A (default = X)
    m_MirrorB  = false;                             // true: mirror / axe B (default = Y)
    m_SwapAxis = false;                             // false if A = X, B = Y; true if A =Y, B = Y
    m_Has_DCode = false;                            // true = DCodes in file
                                                    // false = no DCode-> perhaps deprecated RS274D file
    m_Has_MissingDCode = false;                     // true = some D_Codes are used, but not defined
                                                    // perhaps deprecated RS274D file
    m_FmtScale.x = m_FmtScale.y = 4;                // Initialize default format to 3.4 => 4
    m_FmtLen.x   = m_FmtLen.y = 3 + 4;              // Initialize default format len = 3+4

    m_Iterpolation = GERB_INTERPOL_LINEAR_1X;       // Linear, 90 arc, Circ.
    m_360Arc_enbl  = true;                          // 360 deg circular mode (G75) selected as default
                                                    // interpolation disable
    m_AsArcG74G75Cmd = false;                       // false until a G74 or G75 command is found
    m_Current_Tool = 0;                             // Current Dcode selected
    m_CommandState = 0;                             // State of the current command
    m_CurrentPos.x = m_CurrentPos.y = 0;            // current specified coord
    m_PreviousPos.x = m_PreviousPos.y = 0;          // last specified coord
    m_IJPos.x = m_IJPos.y = 0;                      // current centre coord for
                                                    // plot arcs & circles
    m_LastCoordIsIJPos = false;                     // True only after a IJ coordinate is read
    m_ArcRadius = 0;                                // radius of arcs in circular interpol (given by A## command).
                                                    // in command like X##Y##A##
    m_LastArcDataType = ARC_INFO_TYPE_NONE;         // Extra coordinate info type for arcs
                                                    // (radius or IJ center coord)
    m_LineNum = 0;                                  // line number in file being read
    m_Current_File    = nullptr;                    // Gerber file to read
    m_PolygonFillMode = false;
    m_PolygonFillModeState = 0;
    m_Selected_Tool = 0;
    m_Last_Pen_Command = 0;
    m_Exposure = false;
}


/* Function HasNegativeItems
 * return true if at least one item must be drawn in background color
 * used to optimize screen refresh
 */
bool GERBER_FILE_IMAGE::HasNegativeItems()
{
    if( m_hasNegativeItems < 0 )    // negative items are not yet searched: find them if any
    {
        if( m_ImageNegative )       // A negative layer is expected having always negative objects.
            m_hasNegativeItems = 1;
        else
        {
            m_hasNegativeItems = 0;
            for( GERBER_DRAW_ITEM* item : GetItems() )
            {
                if( item->GetLayer() != m_GraphicLayer )
                    continue;
                if( item->HasNegativeItems() )
                {
                    m_hasNegativeItems = 1;
                    break;
                }
            }
        }
    }
    return m_hasNegativeItems == 1;
}

int GERBER_FILE_IMAGE::GetDcodesCount()
{
    int count = 0;

    for( unsigned ii = 0; ii < arrayDim( m_Aperture_List ); ii++ )
    {
        if( m_Aperture_List[ii] )
            if( m_Aperture_List[ii]->m_InUse || m_Aperture_List[ii]->m_Defined )
                ++count;
    }

    return count;
}


void GERBER_FILE_IMAGE::InitToolTable()
{
    for( int count = 0; count < TOOLS_MAX_COUNT; count++ )
    {
        if( m_Aperture_List[count] == nullptr )
            continue;

        m_Aperture_List[count]->m_Num_Dcode = count + FIRST_DCODE;
        m_Aperture_List[count]->Clear_D_CODE_Data();
    }

    m_aperture_macros.clear();
}


/**
 * Function StepAndRepeatItem
 * Gerber format has a command Step an Repeat
 * This function must be called when reading a gerber file and
 * after creating a new gerber item that must be repeated
 * (i.e when m_XRepeatCount or m_YRepeatCount are > 1)
 * @param aItem = the item to repeat
 */
void GERBER_FILE_IMAGE::StepAndRepeatItem( const GERBER_DRAW_ITEM& aItem )
{
    if( GetLayerParams().m_XRepeatCount < 2 &&
        GetLayerParams().m_YRepeatCount < 2 )
        return; // Nothing to repeat

    // Duplicate item:
    for( int ii = 0; ii < GetLayerParams().m_XRepeatCount; ii++ )
    {
        for( int jj = 0; jj < GetLayerParams().m_YRepeatCount; jj++ )
        {
            // the first gerber item already exists (this is the template)
            // create duplicate only if ii or jj > 0
            if( jj == 0 && ii == 0 )
                continue;

            GERBER_DRAW_ITEM* dupItem = new GERBER_DRAW_ITEM( aItem );
            wxPoint           move_vector;
            move_vector.x = scaletoIU( ii * GetLayerParams().m_StepForRepeat.x,
                                   GetLayerParams().m_StepForRepeatMetric );
            move_vector.y = scaletoIU( jj * GetLayerParams().m_StepForRepeat.y,
                                   GetLayerParams().m_StepForRepeatMetric );
            dupItem->MoveXY( move_vector );
            AddItemToList( dupItem );
        }
    }
}


/**
 * Function DisplayImageInfo
 * has knowledge about the frame and how and where to put status information
 * about this object into the frame's message panel.
 * Display info about Image Parameters.
 * These parameters are valid for the entire file, and must set only once
 * (If more than once, only the last value is used)
 */
void GERBER_FILE_IMAGE::DisplayImageInfo(  GERBVIEW_FRAME* aMainFrame  )
{
    wxString msg;

    aMainFrame->ClearMsgPanel();

    // Display Image name (Image specific)
    aMainFrame->AppendMsgPanel( _( "Image name" ), m_ImageName );

    // Display graphic layer number used to draw this Image
    // (not a Gerber parameter but is also image specific)
    msg.Printf( wxT( "%d" ), m_GraphicLayer + 1 );
    aMainFrame->AppendMsgPanel( _( "Graphic layer" ), msg );

    // Display Image rotation (Image specific)
    msg.Printf( wxT( "%d" ), m_ImageRotation );
    aMainFrame->AppendMsgPanel( _( "Img Rot." ), msg );

    // Display Image polarity (Image specific)
    msg = m_ImageNegative ? _("Negative") : _("Normal");
    aMainFrame->AppendMsgPanel( _( "Polarity" ), msg );

    // Display Image justification and offset for justification (Image specific)
    msg = m_ImageJustifyXCenter ? _("Center") : _("Normal");
    aMainFrame->AppendMsgPanel( _( "X Justify" ), msg );

    msg = m_ImageJustifyYCenter ? _("Center") : _("Normal");
    aMainFrame->AppendMsgPanel( _( "Y Justify" ), msg );

    switch( aMainFrame->GetUserUnits() )
    {
    case EDA_UNITS::MILS:
        msg.Printf( wxT( "X=%f Y=%f" ), Iu2Mils( m_ImageJustifyOffset.x ),
                                        Iu2Mils( m_ImageJustifyOffset.y ) );
        break;

    case EDA_UNITS::INCHES:
        msg.Printf( wxT( "X=%f Y=%f" ), Iu2Mils( m_ImageJustifyOffset.x ) / 1000.0,
                                        Iu2Mils( m_ImageJustifyOffset.y ) / 1000.0 );
        break;

    case EDA_UNITS::MILLIMETRES:
        msg.Printf( wxT( "X=%f Y=%f" ), Iu2Millimeter( m_ImageJustifyOffset.x ),
                                        Iu2Millimeter( m_ImageJustifyOffset.y ) );
        break;

    default:
        wxASSERT_MSG( false, "Invalid unit" );
    }


    aMainFrame->AppendMsgPanel( _( "Image Justify Offset" ), msg );
}


void GERBER_FILE_IMAGE::RemoveAttribute( X2_ATTRIBUTE& aAttribute )
{
    /* Called when a %TD command is found
     * Remove the attribute specified by the %TD command.
     * is no attribute, all current attributes specified by the %TO and the %TA
     * commands are cleared.
     * if a attribute name is specified (for instance %TD.CN*%) is specified,
     * only this attribute is cleared
     */
    wxString cmd = aAttribute.GetPrm( 0 );
    m_NetAttributeDict.ClearAttribute( &cmd );

    if( cmd.IsEmpty() || cmd == ".AperFunction" )
        m_AperFunction.Clear();
}


SEARCH_RESULT GERBER_FILE_IMAGE::Visit( INSPECTOR inspector, void* testData, const KICAD_T scanTypes[] )
{
    KICAD_T        stype;
    SEARCH_RESULT  result = SEARCH_RESULT::CONTINUE;
    const KICAD_T* p    = scanTypes;
    bool           done = false;

    while( !done )
    {
        stype = *p;

        switch( stype )
        {
        case GERBER_IMAGE_T:
        case GERBER_LAYOUT_T:
            ++p;
            break;

        case GERBER_DRAW_ITEM_T:
            result = IterateForward( GetItems(), inspector, testData, p );
            ++p;
            break;

        case EOT:
        default:        // catch EOT or ANY OTHER type here and return.
            done = true;
            break;
        }

        if( result == SEARCH_RESULT::QUIT )
            break;
    }

    return result;
}
