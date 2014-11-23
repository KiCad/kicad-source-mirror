/**
 * @file class_GERBER.cpp
 * a GERBER class handle for a given layer info about used D_CODES and how the layer is drawn
 */

/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 1992-2013 Jean-Pierre Charras  jp.charras at wanadoo.fr
 * Copyright (C) 1992-2013 KiCad Developers, see change_log.txt for contributors.
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

#include <fctsys.h>
#include <common.h>
#include <class_drawpanel.h>
#include <confirm.h>
#include <macros.h>

#include <gerbview.h>
#include <gerbview_frame.h>
#include <class_GERBER.h>
#include <class_X2_gerber_attributes.h>

#include <algorithm>


/**
 * Function scaletoIU
 * converts a distance given in floating point to our internal units
 */
extern int scaletoIU( double aCoord, bool isMetric );       // defined it rs274d_read_XY_and_IJ_coordiantes.cpp

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
 *   D10 ... = Indentification Tool (Opening)
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


GERBER_IMAGE::GERBER_IMAGE( GERBVIEW_FRAME* aParent, int aLayer )
{
    m_Parent = aParent;
    m_GraphicLayer = aLayer;        // Graphic layer Number

    m_Selected_Tool = FIRST_DCODE;
    m_FileFunction = NULL;          // file function parameters

    ResetDefaultValues();

    for( unsigned ii = 0; ii < DIM( m_Aperture_List ); ii++ )
        m_Aperture_List[ii] = 0;
}


GERBER_IMAGE::~GERBER_IMAGE()
{
    for( unsigned ii = 0; ii < DIM( m_Aperture_List ); ii++ )
    {
        delete m_Aperture_List[ii];
    }

    delete m_FileFunction;
}

/*
 * Function GetItemsList
 * returns the first GERBER_DRAW_ITEM * item of the items list
 */
GERBER_DRAW_ITEM * GERBER_IMAGE::GetItemsList()
{
    return m_Parent->GetItemsList();
}

D_CODE* GERBER_IMAGE::GetDCODE( int aDCODE, bool create )
{
    unsigned ndx = aDCODE - FIRST_DCODE;

    if( ndx < (unsigned) DIM( m_Aperture_List ) )
    {
        // lazily create the D_CODE if it does not exist.
        if( create )
        {
            if( m_Aperture_List[ndx] == NULL )
                m_Aperture_List[ndx] = new D_CODE( ndx + FIRST_DCODE );
        }

        return m_Aperture_List[ndx];
    }
    return NULL;
}


APERTURE_MACRO* GERBER_IMAGE::FindApertureMacro( const APERTURE_MACRO& aLookup )
{
    APERTURE_MACRO_SET::iterator iter = m_aperture_macros.find( aLookup );

    if( iter != m_aperture_macros.end() )
    {
        APERTURE_MACRO* pam = (APERTURE_MACRO*) &(*iter);
        return pam;
    }

    return NULL;    // not found
}


void GERBER_IMAGE::ResetDefaultValues()
{
    m_InUse         = false;
    m_GBRLayerParams.ResetDefaultValues();
    m_FileName.Empty();
    m_ImageName     = wxT( "no name" );             // Image name from the IN command
    m_ImageNegative = false;                        // true = Negative image
    m_IsX2_file     = false;                        // true only if a %TF, %TA or %TD command
    delete m_FileFunction;                          // file function parameters
    m_FileFunction = NULL;
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
    m_DecimalFormat = false;                        // true: use floating point notations for coordinates
    m_ImageOffset.x   = m_ImageOffset.y = 0;        // Coord Offset, from IO command
    m_ImageRotation = 0;                            // Allowed 0, 90, 180, 270 (in degree)
    m_LocalRotation = 0.0;                          // Layer totation from RO command (in 0.1 degree)
    m_Offset.x = 0;
    m_Offset.y = 0;                                 // Coord Offset, from OF command
    m_Scale.x  = m_Scale.y = 1.0;                   // scale (A and B) this layer
    m_MirrorA  = false;                             // true: miror / axe A (default = X)
    m_MirrorB  = false;                             // true: miror / axe B (default = Y)
    m_SwapAxis = false;                             // false if A = X, B = Y; true if A =Y, B = Y
    m_Has_DCode = false;                            // true = DCodes in file
                                                    // false = no DCode->
                                                    // search for separate DCode file
    m_FmtScale.x = m_FmtScale.y = 4;                // Initialize default format to 3.4 => 4
    m_FmtLen.x   = m_FmtLen.y = 3 + 4;              // Initialize default format len = 3+4

    m_Iterpolation = GERB_INTERPOL_LINEAR_1X;       // Linear, 90 arc, Circ.
    m_360Arc_enbl  = false;                         // 360 deg circular
                                                    // interpolation disable
    m_Current_Tool = 0;                             // Current Dcode selected
    m_CommandState = 0;                             // State of the current command
    m_CurrentPos.x = m_CurrentPos.y = 0;            // current specified coord
    m_PreviousPos.x = m_PreviousPos.y = 0;          // last specified coord
    m_IJPos.x = m_IJPos.y = 0;                      // current centre coord for
                                                    // plot arcs & circles
    m_Current_File    = NULL;                       // Gerger file to read
    m_FilesPtr        = 0;
    m_PolygonFillMode = false;
    m_PolygonFillModeState = 0;
    m_Selected_Tool = FIRST_DCODE;
}

/* Function HasNegativeItems
 * return true if at least one item must be drawn in background color
 * used to optimize screen refresh
 */
bool GERBER_IMAGE::HasNegativeItems()
{
    if( m_hasNegativeItems < 0 )    // negative items are not yet searched: find them if any
    {
        if( m_ImageNegative )       // A negative layer is expected having always negative objects.
            m_hasNegativeItems = 1;
        else
        {
            m_hasNegativeItems = 0;
            for( GERBER_DRAW_ITEM* item = GetItemsList(); item; item = item->Next() )
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

int GERBER_IMAGE::UsedDcodeNumber()
{
    int count = 0;

    for( unsigned ii = 0; ii < DIM( m_Aperture_List ); ii++ )
    {
        if( m_Aperture_List[ii] )
            if( m_Aperture_List[ii]->m_InUse || m_Aperture_List[ii]->m_Defined )
                ++count;
    }

    return count;
}


void GERBER_IMAGE::InitToolTable()
{
    for( int count = 0; count < TOOLS_MAX_COUNT; count++ )
    {
        if( m_Aperture_List[count] == NULL )
            continue;

        m_Aperture_List[count]->m_Num_Dcode = count + FIRST_DCODE;
        m_Aperture_List[count]->Clear_D_CODE_Data();
    }

    m_aperture_macros.clear();
}


/**
 * Function ReportMessage
 * Add a message (a string) in message list
 * for instance when reading a Gerber file
 * @param aMessage = the straing to add in list
 */
void GERBER_IMAGE::ReportMessage( const wxString aMessage )
{
    m_Parent->ReportMessage( aMessage );
}


/**
 * Function ClearMessageList
 * Clear the message list
 * Call it before reading a Gerber file
 */
void GERBER_IMAGE::ClearMessageList()
{
    m_Parent->ClearMessageList();
}


/**
 * Function StepAndRepeatItem
 * Gerber format has a command Step an Repeat
 * This function must be called when reading a gerber file and
 * after creating a new gerber item that must be repeated
 * (i.e when m_XRepeatCount or m_YRepeatCount are > 1)
 * @param aItem = the item to repeat
 */
void GERBER_IMAGE::StepAndRepeatItem( const GERBER_DRAW_ITEM& aItem )
{
    if( GetLayerParams().m_XRepeatCount < 2 &&
        GetLayerParams().m_YRepeatCount < 2 )
        return; // Nothing to repeat
    // Duplicate item:
    wxString msg;
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
            m_Parent->GetGerberLayout()->m_Drawings.Append( dupItem );
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
void GERBER_IMAGE::DisplayImageInfo( void )
{
    wxString msg;

    m_Parent->ClearMsgPanel();

    // Display Image name (Image specific)
    m_Parent->AppendMsgPanel( _( "Image name" ), m_ImageName, CYAN );

    // Display graphic layer number used to draw this Image
    // (not a Gerber parameter but is also image specific)
    msg.Printf( wxT( "%d" ), m_GraphicLayer + 1 );
    m_Parent->AppendMsgPanel( _( "Graphic layer" ), msg, BROWN );

    // Display Image rotation (Image specific)
    msg.Printf( wxT( "%d" ), m_ImageRotation );
    m_Parent->AppendMsgPanel( _( "Img Rot." ), msg, CYAN );

    // Display Image polarity (Image specific)
    msg = m_ImageNegative ? _("Negative") : _("Normal");
    m_Parent->AppendMsgPanel( _( "Polarity" ), msg, BROWN );

    // Display Image justification and offset for justification (Image specific)
    msg = m_ImageJustifyXCenter ? _("Center") : _("Normal");
    m_Parent->AppendMsgPanel( _( "X Justify" ), msg, DARKRED );

    msg = m_ImageJustifyYCenter ? _("Center") : _("Normal");
    m_Parent->AppendMsgPanel( _( "Y Justify" ), msg, DARKRED );

    if( g_UserUnit == INCHES )
        msg.Printf( wxT( "X=%f Y=%f" ), (double) m_ImageJustifyOffset.x/10000,
                                    (double) m_ImageJustifyOffset.y/10000 );
    else
        msg.Printf( wxT( "X=%f Y=%f" ), (double) m_ImageJustifyOffset.x*2.54/1000,
                                    (double) m_ImageJustifyOffset.y*2.54/1000 );
    m_Parent->AppendMsgPanel( _( "Image Justify Offset" ), msg, DARKRED );
}

// GERBER_IMAGE_LIST is a helper class to handle a list of GERBER_IMAGE files
GERBER_IMAGE_LIST::GERBER_IMAGE_LIST()
{
    m_GERBER_List.reserve( GERBER_DRAWLAYERS_COUNT );

    for( unsigned layer = 0; layer < GERBER_DRAWLAYERS_COUNT; ++layer )
        m_GERBER_List.push_back( NULL );
}

GERBER_IMAGE_LIST::~GERBER_IMAGE_LIST()
{
    ClearList();

    for( unsigned layer = 0; layer < m_GERBER_List.size(); ++layer )
    {
        delete m_GERBER_List[layer];
        m_GERBER_List[layer] = NULL;
    }
}

GERBER_IMAGE* GERBER_IMAGE_LIST::GetGbrImage( int aIdx )
{
    if( (unsigned)aIdx < m_GERBER_List.size() )
        return m_GERBER_List[aIdx];

    return NULL;
}

/**
 * creates a new, empty GERBER_IMAGE* at index aIdx
 * or at the first free location if aIdx < 0
 * @param aIdx = the location to use ( 0 ... GERBER_DRAWLAYERS_COUNT-1 )
 * @return true if the index used, or -1 if no room to add image
 */
int GERBER_IMAGE_LIST::AddGbrImage( GERBER_IMAGE* aGbrImage, int aIdx )
{
    int idx = aIdx;

    if( idx < 0 )
    {
        for( idx = 0; idx < (int)m_GERBER_List.size(); idx++ )
        {
            if( !IsUsed( idx ) )
                break;
        }
    }

    if( idx >= (int)m_GERBER_List.size() )
        return -1;  // No room

    m_GERBER_List[idx] = aGbrImage;

    return idx;
}


// remove all loaded data in list, but do not delete empty images
// (can be reused)
void GERBER_IMAGE_LIST::ClearList()
{
    for( unsigned layer = 0; layer < m_GERBER_List.size(); ++layer )
        ClearImage( layer );
}

// remove the loaded data of image aIdx, but do not delete it
void GERBER_IMAGE_LIST::ClearImage( int aIdx )
{
    if( aIdx >= 0 && aIdx < (int)m_GERBER_List.size() && m_GERBER_List[aIdx] )
    {
        m_GERBER_List[aIdx]->InitToolTable();
        m_GERBER_List[aIdx]->ResetDefaultValues();
        m_GERBER_List[aIdx]->m_InUse = false;
    }
}

// Build a name for image aIdx which can be used in layers manager
const wxString GERBER_IMAGE_LIST::GetDisplayName( int aIdx )
{
    wxString name;

    GERBER_IMAGE* gerber = NULL;

    if( aIdx >= 0 && aIdx < (int)m_GERBER_List.size() )
        gerber = m_GERBER_List[aIdx];

    if( IsUsed(aIdx ) )
    {
        if( gerber->m_FileFunction )
            name.Printf( _( "Layer %d (%s, %s)" ), aIdx + 1,
                         GetChars( gerber->m_FileFunction->GetFileType() ),
                         GetChars( gerber->m_FileFunction->GetBrdLayerId() ) );
        else
            name.Printf( _( "Layer %d *" ), aIdx + 1 );
    }
    else
        name.Printf( _( "Layer %d" ), aIdx + 1 );

    return name;
}

// return true if image is used (loaded and not cleared)
bool GERBER_IMAGE_LIST::IsUsed( int aIdx )
{
    if( aIdx >= 0 && aIdx < (int)m_GERBER_List.size() )
        return m_GERBER_List[aIdx] != NULL && m_GERBER_List[aIdx]->m_InUse;

    return false;
}

// Helper function, for std::sort.
// Sort loaded images by Z order priority, if they have the X2 FileFormat info
// returns true if the first argument (ref) is ordered before the second (test).
static bool sortZorder( const GERBER_IMAGE* const& ref, const GERBER_IMAGE* const& test )
{
    if( !ref && !test )
        return false;        // do not change order: no criteria to sort items

    if( !ref || !ref->m_InUse )
        return false;       // Not used: ref ordered after

    if( !test || !test->m_InUse )
        return true;        // Not used: ref ordered before

    if( !ref->m_FileFunction && !test->m_FileFunction )
        return false;        // do not change order: no criteria to sort items

    if( !ref->m_FileFunction )
        return false;

    if( !test->m_FileFunction )
        return true;

    if( ref->m_FileFunction->GetZOrder() != test->m_FileFunction->GetZOrder() )
        return ref->m_FileFunction->GetZOrder() > test->m_FileFunction->GetZOrder();

    return ref->m_FileFunction->GetZSubOrder() > test->m_FileFunction->GetZSubOrder();
}

void GERBER_IMAGE_LIST::SortImagesByZOrder( GERBER_DRAW_ITEM* aDrawList )
{
    std::sort( m_GERBER_List.begin(), m_GERBER_List.end(), sortZorder );

    // The image order has changed.
    // Graphic layer numbering must be updated to match the widgets layer order

    // Store the old/new graphic layer info:
    std::map <int, int> tab_lyr;

    for( unsigned layer = 0; layer < m_GERBER_List.size(); ++layer )
    {
        if( m_GERBER_List[layer] )
        {
            tab_lyr[m_GERBER_List[layer]->m_GraphicLayer] = layer;
            m_GERBER_List[layer]->m_GraphicLayer = layer ;
        }
    }

    // update the graphic layer in items to draw
    for( GERBER_DRAW_ITEM* item = aDrawList; item; item = item->Next() )
    {
        int layer = item->GetLayer();
        item->SetLayer( tab_lyr[layer] );
    }
}


// The global image list:
GERBER_IMAGE_LIST g_GERBER_List;
