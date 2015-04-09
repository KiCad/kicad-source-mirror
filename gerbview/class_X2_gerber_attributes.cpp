/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2010-2014 Jean-Pierre Charras  jp.charras at wanadoo.fr
 * Copyright (C) 1992-2014 KiCad Developers, see change_log.txt for contributors.
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
 * @file class_X2_gerber_attributes.cpp
 */

/*
 * Manage the gerber extensions (attributes) in the new X2 version
 * only few extensions are handled
 * See http://www.ucamco.com/files/downloads/file/81/the_gerber_file_format_specification.pdf
 *
 * gerber attributes in the new X2 version look like:
 * %TF.FileFunction,Copper,L1,Top*%
 *
 * Currently:
 *  .FileFunction .FileFunction Identifies the file’s function in the PCB.
 * Other Standard Attributes, not yet used in Gerbview:
 *  .Part Identifies the part the file represents, e.g. a single PCB
 *  .MD5 Sets the MD5 file signature or checksum.
 */

#include <wx/log.h>
#include <class_X2_gerber_attributes.h>

/*
 * class X2_ATTRIBUTE
 * The attribute value consists of a number of substrings separated by a “,”
*/

X2_ATTRIBUTE::X2_ATTRIBUTE()
{
}

X2_ATTRIBUTE::~X2_ATTRIBUTE()
{
}

/* return the attribute name (for instance .FileFunction)
 * which is given by TF command.
 */
const wxString& X2_ATTRIBUTE::GetAttribute()
{
    return m_Prms.Item( 0 );
}

/* return a parameter
 * aIdx = the index of the parameter
 * aIdx = 0 is the parameter read after the TF function
 * (the same as GetAttribute())
 */
const wxString& X2_ATTRIBUTE::GetPrm( int aIdx)
{
    static const wxString dummy;

    if( GetPrmCount() < aIdx && aIdx >= 0 )
        return m_Prms.Item( aIdx );

    return dummy;
}

// Debug function: pring using wxLogMessage le list of parameters
void X2_ATTRIBUTE::DbgListPrms()
{
    wxLogMessage( wxT("prms count %d"), GetPrmCount() );

    for( int ii = 0; ii < GetPrmCount(); ii++ )
        wxLogMessage( m_Prms.Item( ii ) );
}

/*
 * parse a TF command and fill m_Prms by the parameters found.
 * aFile = a FILE* ptr to the current Gerber file.
 * buff = the buffer containing current Gerber data (GERBER_BUFZ size)
 * text = a pointer to the first char to read in Gerber data
 */
bool X2_ATTRIBUTE::ParseAttribCmd( FILE* aFile, char *aBuffer, int aBuffSize, char* &aText )
{
    bool ok = true;
    wxString data;

    for( ; ; )
    {
        while( *aText )
        {
            switch( *aText )
            {
            case '%':       // end of command
                return ok;  // success completion

            case ' ':
            case '\r':
            case '\n':
                aText++;
                break;

            case '*':       // End of block
                m_Prms.Add( data );
                data.Empty();
                aText++;
                break;

            case ',':       // End of parameter
                aText++;
                m_Prms.Add( data );
                data.Empty();
                break;

            default:
                data.Append( *aText );
                aText++;
                break;
            }
        }

        // end of current line, read another one.
        if( aBuffer )
        {
            if( fgets( aBuffer, aBuffSize, aFile ) == NULL )
            {
                // end of file
                ok = false;
                break;
            }

            aText = aBuffer;
        }
        else
            return ok;
    }

    return ok;
}

/*
 * class X2_ATTRIBUTE_FILEFUNCTION ( from %TF.FileFunction in Gerber file)
 *  Example file function:
 *  %TF.FileFunction,Copper,L1,Top*%
 * - Type. Such as copper, solder mask etc.
 * - Position. Specifies where the file appears in the PCB layer structure.
 *      Corresponding position substring:
 *      Copper layer:   L1, L2, L3...to indicate the layer position followed by Top, Inr or
 *                      Bot. L1 is always the top copper layer. E.g. L2,Inr.
 *      Extra layer, e.g. solder mask: Top or Bot – defines the attachment of the layer.
 *      Drill/rout layer: E.g. 1,4 – where 1 is the start and 4 is the end copper layer. The
 *                        pair 1,4 defines the span of the drill/rout file
 * Optional index. This can be used in instances where for example there are two solder
 *                 masks on the same side. The index counts from the PCB surface outwards.
 */
X2_ATTRIBUTE_FILEFUNCTION::X2_ATTRIBUTE_FILEFUNCTION( X2_ATTRIBUTE& aAttributeBase )
    : X2_ATTRIBUTE()
{
    m_Prms = aAttributeBase.GetPrms();
    m_z_order = 0;

    //ensure at least 5 parameters
    while( GetPrmCount() < 5 )
        m_Prms.Add( wxEmptyString );

    set_Z_Order();
}

const wxString& X2_ATTRIBUTE_FILEFUNCTION::GetFileType()
{
    // the type of layer (Copper ,  Soldermask ... )
    return m_Prms.Item( 1 );
}

const wxString& X2_ATTRIBUTE_FILEFUNCTION::GetBrdLayerId()
{
    // the brd layer identifier: Ln (for Copper type) or Top, Bot
    return m_Prms.Item( 2 );
}

const wxString& X2_ATTRIBUTE_FILEFUNCTION::GetBrdLayerSide()
{
    if( IsCopper() )
        // the brd layer identifier: Top, Bot, Inr
        return m_Prms.Item( 3 );
    else
        // the brd layer identifier: Top, Bot ( same as GetBrdLayerId() )
        return m_Prms.Item( 2 );
}

const wxString& X2_ATTRIBUTE_FILEFUNCTION::GetLabel()
{
    if( IsCopper() )
       return m_Prms.Item( 4 );
    else
        return m_Prms.Item( 3 );
}


bool X2_ATTRIBUTE_FILEFUNCTION::IsCopper()
{
    // the filefunction label, if any
    return GetFileType().IsSameAs( wxT( "Copper" ), false );
}

// Initialize the z order priority of the current file, from its attributes
// this priority is the order of layers from top to bottom to draw/display gerber images
// Stack up is(  from external copper layer to external)
// copper, then solder paste, then solder mask, then silk screen.
// and global stackup is Front (top) layers then internal copper layers then Back (bottom) layers
void X2_ATTRIBUTE_FILEFUNCTION::set_Z_Order()
{
    m_z_order = -100;     // low level
    m_z_sub_order = 0;

    if( IsCopper() )
    {
        // Copper layer: the priority is the layer Id
        m_z_order = 0;
        wxString num = GetBrdLayerId().Mid( 1 );
        long lnum;
        if( num.ToLong( &lnum ) )
            m_z_sub_order = -lnum;
    }

    if( GetFileType().IsSameAs( wxT( "Paste" ), false ) )
    {
        // solder paste layer: the priority is top then bottom
        m_z_order = 1;       // for top

        if( GetBrdLayerId().IsSameAs( wxT( "Bot" ), false ) )
            m_z_order = -m_z_order;
    }

    if( GetFileType().IsSameAs( wxT( "Soldermask" ), false ) )
    {
        // solder mask layer: the priority is top then bottom
        m_z_order = 2;       // for top

        if( GetBrdLayerId().IsSameAs( wxT( "Bot" ), false ) )
            m_z_order = -m_z_order;
    }

    if( GetFileType().IsSameAs( wxT( "Legend" ), false ) )
    {
        // Silk screen layer: the priority is top then bottom
        m_z_order = 3;       // for top

        if( GetFileType().IsSameAs( wxT( "Legend" ), false ) )

        if( GetBrdLayerId().IsSameAs( wxT( "Bot" ), false ) )
            m_z_order = -m_z_order;
    }
}

