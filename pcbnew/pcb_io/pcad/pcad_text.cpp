/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2007, 2008 Lubo Racko <developer@lura.sk>
 * Copyright (C) 2007, 2008, 2012 Alexander Lunev <al.lunev@yahoo.com>
 * Copyright The KiCad Developers, see AUTHORS.TXT for contributors.
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

#include <pcad/pcad_text.h>

#include <common.h>
#include <board.h>
#include <pcb_text.h>
#include <xnode.h>

#include <wx/string.h>

namespace PCAD2KICAD {

PCAD_TEXT::PCAD_TEXT( PCAD_CALLBACKS* aCallbacks, BOARD* aBoard ) :
        PCAD_PCB_COMPONENT( aCallbacks, aBoard )
{
    m_ObjType = wxT( 'T' );
}


PCAD_TEXT::~PCAD_TEXT()
{
}


void PCAD_TEXT::Parse( XNODE* aNode, int aLayer, const wxString& aDefaultUnits,
                      const wxString& aActualConversion )
{
    XNODE*      lNode;
    wxString    str;

    m_PCadLayer     = aLayer;
    m_KiCadLayer    = GetKiCadLayer();
    m_PositionX     = 0;
    m_PositionY     = 0;
    m_Name.mirror   = 0;    // Normal, not mirrored
    lNode = FindNode( aNode, wxT( "pt" ) );

    if( lNode )
    {
        SetPosition( lNode->GetNodeContent(), aDefaultUnits, &m_PositionX, &m_PositionY,
                     aActualConversion );
    }

    lNode = FindNode( aNode, wxT( "rotation" ) );

    if( lNode )
    {
        str = lNode->GetNodeContent();
        str.Trim( false );
        m_Rotation = EDA_ANGLE( StrToInt1Units( str ), TENTHS_OF_A_DEGREE_T );
    }

    aNode->GetAttribute( wxT( "Name" ), &m_Name.text );
    m_Name.text.Replace( wxT( "\r" ), wxT( "" ) );

    str = FindNodeGetContent( aNode, wxT( "justify" ) );
    m_Name.justify = GetJustifyIdentificator( str );

    str = FindNodeGetContent( aNode, wxT( "isFlipped" ) );

    if( str.IsSameAs( wxT( "True" ), false ) )
        m_Name.mirror = 1;

    lNode = FindNode( aNode, wxT( "textStyleRef" ) );

    if( lNode )
        SetFontProperty( lNode, &m_Name, aDefaultUnits, aActualConversion );
}


void PCAD_TEXT::AddToBoard( FOOTPRINT* aFootprint )
{
    m_Name.textPositionX = m_PositionX;
    m_Name.textPositionY = m_PositionY;
    m_Name.textRotation = m_Rotation;

    ::PCB_TEXT* pcbtxt = new ::PCB_TEXT( m_board );
    m_board->Add( pcbtxt, ADD_MODE::APPEND );

    pcbtxt->SetText( m_Name.text );

    if( m_Name.isTrueType )
        SetTextSizeFromTrueTypeFontHeight( pcbtxt, m_Name.textHeight );
    else
        SetTextSizeFromStrokeFontHeight( pcbtxt, m_Name.textHeight );

    pcbtxt->SetItalic( m_Name.isItalic );
    pcbtxt->SetTextThickness( m_Name.textstrokeWidth );

    SetTextJustify( pcbtxt, m_Name.justify );
    pcbtxt->SetTextPos( VECTOR2I( m_Name.textPositionX, m_Name.textPositionY ) );

    pcbtxt->SetMirrored( m_Name.mirror );

    if( pcbtxt->IsMirrored() )
        pcbtxt->SetTextAngle( ANGLE_360 - m_Name.textRotation );
    else
        pcbtxt->SetTextAngle( m_Name.textRotation );

    pcbtxt->SetLayer( m_KiCadLayer );
}


// void PCAD_TEXT::SetPosOffset( int aX_offs, int aY_offs )
// {
// PCAD_PCB_COMPONENT::SetPosOffset( aX_offs, aY_offs );

// m_Name.textPositionX    += aX_offs;
// m_Name.textPositionY    += aY_offs;
// }

} // namespace PCAD2KICAD
