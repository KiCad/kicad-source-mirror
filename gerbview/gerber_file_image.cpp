/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 1992-2019 Jean-Pierre Charras  jp.charras at wanadoo.fr
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

#include <gerbview.h>
#include <gerbview_frame.h>
#include <gerber_file_image.h>
#include <macros.h>
#include <X2_gerber_attributes.h>
#include <algorithm>
#include <map>
//#include <numeric>
#include <core/arraydim.h>
#include <core/profile.h>


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
    m_PositiveDrawColor  = WHITE;   // The color used to draw positive items for this image

    m_Selected_Tool = 0;
    m_FileFunction = nullptr;          // file function parameters

    ResetDefaultValues();

    m_ApertureList.clear();
}


GERBER_FILE_IMAGE::~GERBER_FILE_IMAGE()
{
    // Reverse iteration to avoid O(N^2) memcpy in a vector erase downstream.
    // It results in a O(N^2) std::find, which is somewhat faster.
    for( auto it = GetItems().rbegin(); it < GetItems().rend(); it++ )
        delete *it;

    m_drawings.clear();

    for( const auto& ap_item : m_ApertureList )
        delete ap_item.second;

    delete m_FileFunction;
}


void GERBER_FILE_IMAGE::AddMessageToList( const wxString& aMessage )
{
    /* Add a message to the message list, but only if there are less than max_messages
     * to avoid very long list (can happens if trying to read a non gerber file)
     */
    const int max_messages = 50;    // Arbitrary but reasonable value.

    if( m_messagesList.size() < max_messages )
        m_messagesList.Add( aMessage );
    else if( m_messagesList.size() == max_messages )
        m_messagesList.Add( _( "Too many messages, some are skipped" ) );
}


void GERBER_FILE_IMAGE::SetDrawOffetAndRotation( VECTOR2D aOffsetMM, EDA_ANGLE aRotation )
{
    m_DisplayOffset.x = KiROUND( aOffsetMM.x * gerbIUScale.IU_PER_MM );
    m_DisplayOffset.y = KiROUND( aOffsetMM.y * gerbIUScale.IU_PER_MM );
    m_DisplayRotation = aRotation;

    // Clear m_AbsolutePolygon member of Gerber items, because draw coordinates
    // are now outdated
    for( GERBER_DRAW_ITEM* item : GetItems() )
        item->m_AbsolutePolygon.RemoveAllContours();
}


D_CODE* GERBER_FILE_IMAGE::GetDCODEOrCreate( int aDCODE, bool aCreateIfNoExist )
{
    auto it = m_ApertureList.find(aDCODE);

    if( it != m_ApertureList.end() )
        return it->second;

    // lazily create the D_CODE if it does not exist.
    if( aCreateIfNoExist )
    {
        D_CODE* dcode = new D_CODE(aDCODE);
        m_ApertureList.insert({ aDCODE, dcode });
        return dcode;
    }

    return nullptr;
}


D_CODE* GERBER_FILE_IMAGE::GetDCODE( int aDCODE ) const
{
    auto it = m_ApertureList.find(aDCODE);

    if ( it != m_ApertureList.end() )
        return it->second;

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
    m_ImageName     = wxEmptyString;                // Image name from the IN command (deprecated)
    m_ImageNegative = false;                        // true = Negative image
    m_IsX2_file     = false;                        // true only if a %TF, %TA or %TD command
    delete m_FileFunction;                          // file function parameters
    m_FileFunction = nullptr;
    m_MD5_value.Empty();                            // MD5 value found in a %TF.MD5 command
    m_PartString.Empty();                           // string found in a %TF.Part command
    m_hasNegativeItems    = -1;                     // set to uninitialized
    m_ImageJustifyOffset = VECTOR2I( 0, 0 );        // Image justify Offset
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

    m_DisplayOffset.x = m_DisplayOffset.y = 0;
    m_DisplayRotation = ANGLE_0;
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
        {
            m_hasNegativeItems = 1;
        }
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

    for( const auto& n : m_ApertureList )
    {
        D_CODE* dcode = n.second;

        wxCHECK2( dcode, continue );

        if( dcode->m_InUse || dcode->m_Defined )
            count++;
    }

    return count;
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
    if( GetLayerParams().m_XRepeatCount < 2 && GetLayerParams().m_YRepeatCount < 2 )
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
            VECTOR2I          move_vector;
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
 * Some are deprecated
 */
void GERBER_FILE_IMAGE::DisplayImageInfo(  GERBVIEW_FRAME* aMainFrame  )
{
    wxString msg;

    aMainFrame->ClearMsgPanel();

    // Display the Gerber variant (X1 / X2
    aMainFrame->AppendMsgPanel( _( "Format" ), m_IsX2_file ? wxT( "X2" ) : wxT( "X1" ) );

    // Display Image name (Image specific). IM command (Image Name) is deprecated
    // So non empty image name is very rare, probably never found
    if( !m_ImageName.IsEmpty() )
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

    msg.Printf( wxT( "X=%s Y=%s" ),
                aMainFrame->MessageTextFromValue( m_ImageJustifyOffset.x ),
                aMainFrame->MessageTextFromValue( m_ImageJustifyOffset.y ) );

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

    if( cmd.IsEmpty() || cmd == wxT( ".AperFunction" ) )
        m_AperFunction.Clear();
}


INSPECT_RESULT GERBER_FILE_IMAGE::Visit( INSPECTOR inspector, void* testData,
                                         const std::vector<KICAD_T>& aScanTypes )
{
    for( KICAD_T scanType : aScanTypes )
    {
        if( scanType == GERBER_DRAW_ITEM_T )
        {
            if( IterateForward( GetItems(), inspector, testData, { scanType } ) == INSPECT_RESULT::QUIT )
                return INSPECT_RESULT::QUIT;
        }
    }

    return INSPECT_RESULT::CONTINUE;
}
