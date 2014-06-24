/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2013 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 1992-2013 KiCad Developers, see AUTHORS.txt for contributors.
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

// The DXF reader lib (libdxfrw) comes from LibreCAD project, a 2D CAD program
// libdxfrw can be found on http://sourceforge.net/projects/libdxfrw/
// or (latest sources) on
// https://github.com/LibreCAD/LibreCAD/tree/master/libraries/libdxfrw/src
//
// There is no doc to use it, but have a look to
// https://github.com/LibreCAD/LibreCAD/blob/master/librecad/src/lib/filters/rs_filterdxf.cpp
// and https://github.com/LibreCAD/LibreCAD/blob/master/librecad/src/lib/filters/rs_filterdxf.h
// Each time a dxf entity is read, a "call back" fuction is called
// like void DXF2BRD_CONVERTER::addLine( const DRW_Line& data ) when a line is read.
// this function just add the BOARD entity from dxf parameters (start and end point ...)


#include "libdxfrw.h"
#include "dxf2brd_items.h"
#include <wx/arrstr.h>
#include <wx/regex.h>

#include <trigo.h>
#include <macros.h>
#include <class_board.h>
#include <class_drawsegment.h>
#include <class_pcb_text.h>
#include <convert_from_iu.h>

DXF2BRD_CONVERTER::DXF2BRD_CONVERTER() : DRW_Interface()
{
    m_xOffset   = 0.0;      // X coord offset for conversion (in mm)
    m_yOffset   = 0.0;      // Y coord offset for conversion (in mm)
    m_Dfx2mm    = 1.0;      // The scale factor to convert DXF units to mm
    m_brd       = NULL;
    m_version   = 0;
    m_defaultThickness = 0.1;
    m_brdLayer = Dwgs_User;
}


DXF2BRD_CONVERTER::~DXF2BRD_CONVERTER()
{
}


// coordinate conversions from dxf to internal units
int DXF2BRD_CONVERTER::mapX( double aDxfCoordX )
{
    return Millimeter2iu( m_xOffset + (aDxfCoordX * m_Dfx2mm) );
}


int DXF2BRD_CONVERTER::mapY( double aDxfCoordY )
{
    return Millimeter2iu( m_yOffset - (aDxfCoordY * m_Dfx2mm) );
}


int DXF2BRD_CONVERTER::mapDim( double aDxfValue )
{
    return Millimeter2iu( aDxfValue * m_Dfx2mm );
}


bool DXF2BRD_CONVERTER::ImportDxfFile( const wxString& aFile, BOARD* aBoard )
{
    m_brd = aBoard;

    dxfRW* dxf = new dxfRW( aFile.ToUTF8() );
    bool success = dxf->read( this, true );

    delete dxf;

    return success;
}

// Add aItem the the board
// this item is also added to the list of new items
// (for undo command for instance)
void DXF2BRD_CONVERTER::appendToBoard( BOARD_ITEM * aItem )
{
    m_brd->Add( aItem );
    m_newItemsList.push_back( aItem );
}

/*
 * Implementation of the method which handles layers.
 */
void DXF2BRD_CONVERTER::addLayer( const DRW_Layer& data )
{
    // Not yet useful in Pcbnew.
#if 0
    wxString name = wxString::FromUTF8( data.name.c_str() );
    wxLogMessage( name );
#endif
}


/*
 * Import line entities.
 */
void DXF2BRD_CONVERTER::addLine( const DRW_Line& data )
{
    DRAWSEGMENT*    segm = new DRAWSEGMENT( m_brd );

    segm->SetLayer( (LAYER_ID) m_brdLayer );

    wxPoint         start( mapX( data.basePoint.x ), mapY( data.basePoint.y ) );

    segm->SetStart( start );

    wxPoint         end( mapX( data.secPoint.x ), mapY( data.secPoint.y ) );

    segm->SetEnd( end );

    segm->SetWidth( mapDim( data.thickness == 0 ? m_defaultThickness : data.thickness ) );
    appendToBoard( segm );
}


/*
 * Import Circle entities.
 */
void DXF2BRD_CONVERTER::addCircle( const DRW_Circle& data )
{
    DRAWSEGMENT* segm = new DRAWSEGMENT( m_brd );

    segm->SetLayer( (LAYER_ID) m_brdLayer );
    segm->SetShape( S_CIRCLE );
    wxPoint center( mapX( data.basePoint.x ), mapY( data.basePoint.y ) );
    segm->SetCenter( center );
    wxPoint circle_start( mapX( data.basePoint.x + data.radious ),
                          mapY( data.basePoint.y ) );
    segm->SetArcStart( circle_start );
    segm->SetWidth( mapDim( data.thickness == 0 ? m_defaultThickness
                            : data.thickness ) );
    appendToBoard( segm );
}


/*
 * Import Arc entities.
 */
void DXF2BRD_CONVERTER::addArc( const DRW_Arc& data )
{
    DRAWSEGMENT* segm = new DRAWSEGMENT( m_brd );

    segm->SetLayer( (LAYER_ID) m_brdLayer );
    segm->SetShape( S_ARC );

    // Init arc centre:
    wxPoint center( mapX( data.basePoint.x ), mapY( data.basePoint.y ) );
    segm->SetCenter( center );

    // Init arc start point
    double  arcStartx   = data.radious;
    double  arcStarty   = 0;
    double  startangle = data.staangle;
    double  endangle = data.endangle;

    RotatePoint( &arcStartx, &arcStarty, -RAD2DECIDEG( startangle ) );
    wxPoint arcStart( mapX( arcStartx + data.basePoint.x ),
                      mapY( arcStarty + data.basePoint.y ) );
    segm->SetArcStart( arcStart );

    // calculate arc angle (arcs are CCW, and should be < 0 in Pcbnew)
    double angle = -RAD2DECIDEG( endangle - startangle );

    if( angle > 0.0 )
        angle -= 3600.0;

    segm->SetAngle( angle );

    segm->SetWidth( mapDim( data.thickness == 0 ? m_defaultThickness
                            : data.thickness ) );
    appendToBoard( segm );
}

/**
 * Import texts (TEXT).
 */
void DXF2BRD_CONVERTER::addText(const DRW_Text& data)
{
    TEXTE_PCB*  pcb_text = new TEXTE_PCB( m_brd );
    pcb_text->SetLayer( (LAYER_ID) m_brdLayer );

    wxPoint refPoint( mapX(data.basePoint.x), mapY(data.basePoint.y) );
    wxPoint secPoint( mapX(data.secPoint.x), mapY(data.secPoint.y) );

    if (data.alignV !=0 || data.alignH !=0 ||data.alignH ==DRW_Text::HMiddle)
    {
        if (data.alignH !=DRW_Text::HAligned && data.alignH !=DRW_Text::HFit)
        {
            wxPoint tmp = secPoint;
            secPoint = refPoint;
            refPoint = tmp;
        }
    }

    switch( data.alignV )
    {
        case DRW_Text::VBaseLine:
            pcb_text->SetVertJustify( GR_TEXT_VJUSTIFY_BOTTOM );
            break;

        case DRW_Text::VBottom:
            pcb_text->SetVertJustify( GR_TEXT_VJUSTIFY_BOTTOM );
            break;

        case DRW_Text::VMiddle:
            pcb_text->SetVertJustify( GR_TEXT_VJUSTIFY_CENTER );
            break;

        case DRW_Text::VTop:
            pcb_text->SetVertJustify( GR_TEXT_VJUSTIFY_TOP );
            break;
    }

    switch( data.alignH )
    {
        case DRW_Text::HLeft:
            pcb_text->SetHorizJustify( GR_TEXT_HJUSTIFY_LEFT );
            break;

        case DRW_Text::HCenter:
            pcb_text->SetHorizJustify( GR_TEXT_HJUSTIFY_CENTER );
            break;

        case DRW_Text::HRight:
            pcb_text->SetHorizJustify( GR_TEXT_HJUSTIFY_RIGHT );
            break;

        case DRW_Text::HAligned:
            // no equivalent options in text pcb.
            pcb_text->SetHorizJustify( GR_TEXT_HJUSTIFY_LEFT );
            break;

        case DRW_Text::HMiddle:
            // no equivalent options in text pcb.
            pcb_text->SetHorizJustify( GR_TEXT_HJUSTIFY_CENTER );
            break;

        case DRW_Text::HFit:
            // no equivalent options in text pcb.
            pcb_text->SetHorizJustify( GR_TEXT_HJUSTIFY_LEFT );
            break;
    }

#if 0
    wxString sty = wxString::FromUTF8(data.style.c_str());
    sty=sty.ToLower();

    if (data.textgen==2)
    {
        // Text dir = left to right;
    } else if (data.textgen==4)
    {
        / Text dir = top to bottom;
    } else
    {
    }
#endif

    wxString    text = toNativeString( wxString::FromUTF8( data.text.c_str() ) );

    pcb_text->SetTextPosition( refPoint );
    pcb_text->SetOrientation( data.angle * 10 );
    // The 0.8 factor gives a better height/width ratio with our font
    pcb_text->SetWidth( mapDim( data.height * 0.8 ) );
    pcb_text->SetHeight( mapDim( data.height ) );
    pcb_text->SetThickness( mapDim( data.thickness == 0 ? m_defaultThickness
                                    : data.thickness ) );
    pcb_text->SetText( text );

    appendToBoard( pcb_text );
}


/**
 * Import multi line texts (MTEXT).
 */
void DXF2BRD_CONVERTER::addMText( const DRW_MText& data )
{
    wxString    text = toNativeString( wxString::FromUTF8( data.text.c_str() ) );
    wxString    attrib, tmp;

    /* Some texts start by '\' and have formating chars (font name, font option...)
     *  ending with ';'
     *  Here are some mtext formatting codes:
     *  Format code        Purpose
     * \0...\o            Turns overline on and off
     *  \L...\l            Turns underline on and off
     * \~                 Inserts a nonbreaking space
     \\                 Inserts a backslash
     \\\{...\}            Inserts an opening and closing brace
     \\ \File name;        Changes to the specified font file
     \\ \Hvalue;           Changes to the text height specified in drawing units
     \\ \Hvaluex;          Changes the text height to a multiple of the current text height
     \\ \S...^...;         Stacks the subsequent text at the \, #, or ^ symbol
     \\ \Tvalue;           Adjusts the space between characters, from.75 to 4 times
     \\ \Qangle;           Changes obliquing angle
     \\ \Wvalue;           Changes width factor to produce wide text
     \\ \A                 Sets the alignment value; valid values: 0, 1, 2 (bottom, center, top)    while( text.StartsWith( wxT("\\") ) )
     */
    while( text.StartsWith( wxT( "\\" ) ) )
    {
        attrib << text.BeforeFirst( ';' );
        tmp     = text.AfterFirst( ';' );
        text    = tmp;
    }

    TEXTE_PCB*  pcb_text = new TEXTE_PCB( m_brd );
    pcb_text->SetLayer( (LAYER_ID) m_brdLayer );

    wxPoint     textpos( mapX( data.basePoint.x ), mapY( data.basePoint.y ) );
    pcb_text->SetTextPosition( textpos );
    pcb_text->SetOrientation( data.angle * 10 );
    // The 0.8 factor gives a better height/width ratio with our font
    pcb_text->SetWidth( mapDim( data.height * 0.8 ) );
    pcb_text->SetHeight( mapDim( data.height ) );
    pcb_text->SetThickness( mapDim( data.thickness == 0 ? m_defaultThickness
                                    : data.thickness ) );
    pcb_text->SetText( text );

    // Initialize text justifications:
    if( data.textgen <= 3 )
    {
        pcb_text->SetVertJustify( GR_TEXT_VJUSTIFY_TOP );
    }
    else if( data.textgen <= 6 )
    {
        pcb_text->SetVertJustify( GR_TEXT_VJUSTIFY_CENTER );
    }
    else
    {
        pcb_text->SetVertJustify( GR_TEXT_VJUSTIFY_BOTTOM );
    }

    if( data.textgen % 3 == 1 )
    {
        pcb_text->SetHorizJustify( GR_TEXT_HJUSTIFY_LEFT );
    }
    else if( data.textgen % 3 == 2 )
    {
        pcb_text->SetHorizJustify( GR_TEXT_HJUSTIFY_CENTER );
    }
    else
    {
        pcb_text->SetHorizJustify( GR_TEXT_HJUSTIFY_RIGHT );
    }

#if 0   // These setting have no mening in Pcbnew
    if( data.alignH==1 )
    {
        // Text is left to right;
    }
    else if( data.alignH == 3 )
    {
        // Text is top to bottom;
    }
    else
    {
        // use ByStyle;
    }

    if( data.alignV==1 )
    {
        // use AtLeast;
    }
    else
    {
        // useExact;
    }
#endif

    appendToBoard( pcb_text );
}


/**
 * Sets the header variables from the DXF file.
 */
void DXF2BRD_CONVERTER::addHeader( const DRW_Header* data )
{
    std::map<std::string, DRW_Variant*>::const_iterator it;

    for( it = data->vars.begin(); it != data->vars.end(); it++ )
    {
        std::string key = ( (*it).first ).c_str();

        if( key == "$DWGCODEPAGE" )
        {
            DRW_Variant* var = (*it).second;
            m_codePage = ( *var->content.s );
        }
    }
}


/**
 * Converts a native unicode string into a DXF encoded string.
 *
 * DXF endoding includes the following special sequences:
 * - %%%c for a diameter sign
 * - %%%d for a degree sign
 * - %%%p for a plus/minus sign
 */
wxString DXF2BRD_CONVERTER::toDxfString( const wxString& str )
{
    wxString    res;
    int         j = 0;

    for( unsigned i = 0; i<str.length(); ++i )
    {
        int c = str[i];

        if( c>175 || c<11 )
        {
            res.append( str.Mid( j, i - j ) );
            j = i;

            switch( c )
            {
            case 0x0A:
                res += wxT("\\P");
                break;

                // diameter:
#ifdef __WINDOWS_
            // windows, as always, is special.
            case 0x00D8:
#else
            case 0x2205:
#endif
                res += wxT("%%C");
                break;

            // degree:
            case 0x00B0:
                res += wxT("%%D");
                break;

            // plus/minus
            case 0x00B1:
                res += wxT("%%P");
                break;

            default:
                j--;
                break;
            }

            j++;
        }
    }

    res.append( str.Mid( j ) );
    return res;
}


/**
 * Converts a DXF encoded string into a native Unicode string.
 */
wxString DXF2BRD_CONVERTER::toNativeString( const wxString& data )
{
    wxString    res;

    // Ignore font tags:
    int         j = 0;

    for( unsigned i = 0; i<data.length(); ++i )
    {
        if( data[ i ] == 0x7B )                                     // is '{' ?
        {
            if( data[ i + 1 ] == 0x5c && data[ i + 2 ] == 0x66 )    // is "\f" ?
            {
                // found font tag, append parsed part
                res.append( data.Mid( j, i - j ) );

                // skip to ';'
                for( unsigned k = i + 3; k < data.length(); ++k )
                {
                    if( data[ k ] == 0x3B )
                    {
                        i = j = ++k;
                        break;
                    }
                }

                // add to '}'
                for( unsigned k = i; k<data.length(); ++k )
                {
                    if( data[ k ] == 0x7D )
                    {
                        res.append( data.Mid( i, k - i ) );
                        i = j = ++k;
                        break;
                    }
                }
            }
        }
    }

    res.append( data.Mid( j ) );

#if 1
    wxRegEx regexp;
    // Line feed:
    regexp.Compile( wxT( "\\\\P" ) );
    regexp.Replace( &res, wxT( "\n" ) );

    // Space:
    regexp.Compile( wxT( "\\\\~" ) );
    regexp.Replace( &res, wxT( " " ) );

    // diameter:
    regexp.Compile( wxT( "%%[cC]" ) );
#ifdef __WINDOWS__
    // windows, as always, is special.
    regexp.Replace( &res, wxChar( 0xD8 ) );
#else
    // Empty_set, diameter is 0x2300
    regexp.Replace( &res, wxChar( 0x2205 ) );
#endif

    // degree:
    regexp.Compile( wxT( "%%[dD]" ) );
    regexp.Replace( &res, wxChar( 0x00B0 ) );
    // plus/minus
    regexp.Compile( wxT( "%%[pP]" ) );
    regexp.Replace( &res, wxChar( 0x00B1 ) );
#endif

    return res;
}


void DXF2BRD_CONVERTER::addTextStyle( const DRW_Textstyle& data )
{
    // TODO
}
