/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2010-2014 Jean-Pierre Charras  jp.charras at wanadoo.fr
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

/**
 * @file X2_gerber_attributes.cpp
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
 * .FileFunction .FileFunction Identifies the file's function in the PCB.
 *  Other Standard Attributes, not yet used in Gerbview:
 * .Part Identifies the part the file represents, e.g. a single PCB
 * .MD5 Sets the MD5 file signature or checksum.
 */

#include <wx/log.h>
#include <X2_gerber_attributes.h>
#include <string_utils.h>


X2_ATTRIBUTE::X2_ATTRIBUTE()
{
}


X2_ATTRIBUTE::~X2_ATTRIBUTE()
{
}


const wxString& X2_ATTRIBUTE::GetAttribute()
{
    return m_Prms.Item( 0 );
}


const wxString& X2_ATTRIBUTE::GetPrm( int aIdx )
{
    static const wxString dummy;

    if( GetPrmCount() > aIdx && aIdx >= 0 )
        return m_Prms.Item( aIdx );

    return dummy;
}


void X2_ATTRIBUTE::DbgListPrms()
{
    wxLogMessage( wxT( "prms count %d" ), GetPrmCount() );

    for( int ii = 0; ii < GetPrmCount(); ii++ )
        wxLogMessage( m_Prms.Item( ii ) );
}


bool X2_ATTRIBUTE::ParseAttribCmd( FILE* aFile, char *aBuffer, int aBuffSize, char* &aText,
                                   int& aLineNum )
{
    // parse a TF, TA, TO ... command and fill m_Prms by the parameters found.
    // the "%TF" (start of command) is already read by the caller

    bool ok = true;
    std::string data;

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
                m_Prms.Add( From_UTF8( data.c_str() ) );
                data.clear();
                aText++;
                break;

            case ',':       // End of parameter (separator)
                aText++;
                m_Prms.Add( From_UTF8( data.c_str() ) );
                data.clear();
                break;

            default:
                data += *aText;
                aText++;
                break;
            }
        }

        // end of current line, read another one.
        if( aBuffer && aFile )
        {
            if( fgets( aBuffer, aBuffSize, aFile ) == nullptr )
            {
                // end of file
                ok = false;
                break;
            }

            aLineNum++;
            aText = aBuffer;
        }
        else
        {
            return ok;
        }
    }

    return ok;
}


X2_ATTRIBUTE_FILEFUNCTION::X2_ATTRIBUTE_FILEFUNCTION( X2_ATTRIBUTE& aAttributeBase )
    : X2_ATTRIBUTE()
{
    m_Prms = aAttributeBase.GetPrms();
    m_z_order = 0;

    // ensure at least 7 parameters exist.
    while( GetPrmCount() < 7 )
        m_Prms.Add( wxEmptyString );

    set_Z_Order();
}


const wxString& X2_ATTRIBUTE_FILEFUNCTION::GetFileType()
{
    // the type of layer (Copper, Soldermask ... )
    return m_Prms.Item( 1 );
}


const wxString& X2_ATTRIBUTE_FILEFUNCTION::GetBrdLayerId()
{
    // the brd layer identifier: Ln (for Copper type) or Top, Bot
    return m_Prms.Item( 2 );
}


const wxString X2_ATTRIBUTE_FILEFUNCTION::GetDrillLayerPair()
{
    // the layer pair identifiers, for drill files, i.e.
    // with m_Prms.Item( 1 ) = "Plated" or "NonPlated"
    wxString lpair = m_Prms.Item( 2 ) + ',' + m_Prms.Item( 3 );
    return lpair;
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


const wxString& X2_ATTRIBUTE_FILEFUNCTION::GetLPType()
{
    // Only for drill files:  the Layer Pair type (PTH, NPTH, Blind or Buried)
    return m_Prms.Item( 4 );
}


const wxString& X2_ATTRIBUTE_FILEFUNCTION::GetRouteType()
{
    // Only for drill files:  the drill/routing type(Drill, Route, Mixed)
    return m_Prms.Item( 5 );
}


bool X2_ATTRIBUTE_FILEFUNCTION::IsCopper()
{
    // the filefunction label, if any
    return GetFileType().IsSameAs( wxT( "Copper" ), false );
}


bool X2_ATTRIBUTE_FILEFUNCTION::IsDrillFile()
{
    // the filefunction label, if any
    return GetFileType().IsSameAs( wxT( "Plated" ), false )
           || GetFileType().IsSameAs( wxT( "NonPlated" ), false );
}


void X2_ATTRIBUTE_FILEFUNCTION::set_Z_Order()
{
    m_z_order = 100;     // high level
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

    if( GetFileType().IsSameAs( wxT( "Soldermask" ), false ) )
    {
        // solder mask layer: the priority is top then bottom
        m_z_order = 1;       // for top

        if( GetBrdLayerId().IsSameAs( wxT( "Bot" ), false ) )
            m_z_order = -m_z_order;
    }

    if( GetFileType().IsSameAs( wxT( "Legend" ), false ) )
    {
        // Silk screen layer: the priority is top then bottom
        m_z_order = 2;       // for top

        if( GetBrdLayerId().IsSameAs( wxT( "Bot" ), false ) )
            m_z_order = -m_z_order;
    }

    if( GetFileType().IsSameAs( wxT( "Paste" ), false ) )
    {
        // solder paste layer: the priority is top then bottom
        m_z_order = 3;       // for top

        if( GetBrdLayerId().IsSameAs( wxT( "Bot" ), false ) )
            m_z_order = -m_z_order;
    }

    if( GetFileType().IsSameAs( wxT( "Glue" ), false ) )
    {
        // Glue spots used to fix components to the board prior to soldering:
        // the priority is top then bottom
        m_z_order = 4;       // for top

        if( GetBrdLayerId().IsSameAs( wxT( "Bot" ), false ) )
            m_z_order = -m_z_order;
    }
}

