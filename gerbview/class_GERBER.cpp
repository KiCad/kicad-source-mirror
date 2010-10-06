/** @file class_GERBER.cpp
 * a GERBER class handle for a given layer info about used D_CODES and how the layer is drawn
 */

/*
 * This program source code file is part of KICAD, a free EDA CAD application.
 *
 * Copyright (C) 1992-2010 <Jean-Pierre Charras>
 * Copyright (C) 1992-2010 Kicad Developers, see change_log.txt for contributors.
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

#include "fctsys.h"
#include "common.h"
#include "class_drawpanel.h"
#include "confirm.h"
#include "macros.h"

#include "gerbview.h"
#include "class_GERBER.h"

/* Format Gerber: NOTES:
 * Tools and D_CODES
 *   tool number (identification of shapes)
 *   1 to 999
 *
 * D_CODES:
 *   D01 ... D9 = action codes:
 *   D01 = activating light (lower pen) when di ¿½ placement
 *   D02 = light extinction (lift pen) when di ¿½ placement
 *   D03 Flash
 *   D09 = VAPE Flash
 *   D10 ... = Indentification Tool (Opening)
 *
 * For tools:
 * DCode min = D10
 * DCode max = 999
 */


GERBER::GERBER( WinEDA_GerberFrame* aParent, int aLayer )
{
    m_Parent = aParent;
    m_Layer  = aLayer;           // Layer Number

    m_Selected_Tool = FIRST_DCODE;

    ResetDefaultValues();

    for( unsigned ii = 0; ii < DIM( m_Aperture_List ); ii++ )
        m_Aperture_List[ii] = 0;

    m_Pcb = 0;
}


GERBER::~GERBER()
{
    for( unsigned ii = 0; ii < DIM( m_Aperture_List ); ii++ )
    {
        delete m_Aperture_List[ii];

        // m_Aperture_List[ii] = NULL;
    }

    delete m_Pcb;
}


D_CODE* GERBER::GetDCODE( int aDCODE, bool create )
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


APERTURE_MACRO* GERBER::FindApertureMacro( const APERTURE_MACRO& aLookup )
{
    APERTURE_MACRO_SET::iterator iter = m_aperture_macros.find( aLookup );

    if( iter != m_aperture_macros.end() )
    {
        APERTURE_MACRO* pam = (APERTURE_MACRO*) &(*iter);
        return pam;
    }

    return NULL;    // not found
}


void GERBER::ResetDefaultValues()
{
    m_FileName.Empty();
    m_ImageName     = wxT( "no image name" );   // Image name from the IN command
    m_LayerName     = wxT( "no layer name" );   // Layer name from the LN command
    m_LayerNegative = FALSE;                    // TRUE = Negative Layer
    m_ImageNegative = FALSE;                    // TRUE = Negative image
    m_GerbMetric    = FALSE;                    // FALSE = Inches, TRUE = metric
    m_Relative = FALSE;                         // FALSE = absolute Coord, RUE =
                                                // relative Coord
    m_NoTrailingZeros = FALSE;                  // True: trailing zeros deleted
    m_MirorA    = FALSE;                        // True: miror / axe A (X)
    m_MirorB    = FALSE;                        // True: miror / axe B (Y)
    m_Has_DCode = FALSE;                        // TRUE = DCodes in file
                                                // FALSE = no DCode->
                                                // search for separate DCode file

    m_FmtScale.x = m_FmtScale.y = g_Default_GERBER_Format % 10;
    m_FmtLen.x   = m_FmtLen.y = m_FmtScale.x + (g_Default_GERBER_Format / 10);

    m_LayerScale.x = m_LayerScale.y = 1.0;          // scale (X and Y) this
                                                    // layer
    m_Rotation     = 0;
    m_Iterpolation = GERB_INTERPOL_LINEAR_1X;       // Linear, 90 arc, Circ.
    m_360Arc_enbl  = FALSE;                         // 360 deg circular
                                                    // interpolation disable
    m_Current_Tool = 0;                             // Current Tool (Dcode)
                                                    // number selected
    m_CommandState = 0;                             // gives tate of the
                                                    // stacking order analysis
    m_CurrentPos.x = m_CurrentPos.y = 0;            // current specified coord
                                                    // for plot
    m_PreviousPos.x = m_PreviousPos.y = 0;          // old current specified
                                                    // coord for plot
    m_IJPos.x = m_IJPos.y = 0;                      // current centre coord for
                                                    // plot arcs & circles
    m_Current_File    = NULL;                       // File to read
    m_FilesPtr        = 0;
    m_PolygonFillMode = FALSE;
    m_PolygonFillModeState = 0;
}


int GERBER::ReturnUsedDcodeNumber()
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


void GERBER::InitToolTable()
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


/** function ReportMessage
 * Add a message (a string) in message list
 * for instance when reading a Gerber file
 * @param aMessage = the straing to add in list
 */
void GERBER::ReportMessage( const wxString aMessage )
{
    m_Parent->ReportMessage( aMessage );
}


/** function ClearMessageList
 * Clear the message list
 * Call it before reading a Gerber file
 */
void GERBER::ClearMessageList()
{
    m_Parent->ClearMessageList();
}
