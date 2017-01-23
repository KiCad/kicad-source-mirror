/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2007, 2008 Lubo Racko <developer@lura.sk>
 * Copyright (C) 2007, 2008, 2012 Alexander Lunev <al.lunev@yahoo.com>
 * Copyright (C) 2012 KiCad Developers, see CHANGELOG.TXT for contributors.
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
 * @file pcb_text.cpp
 */

#include <wx/wx.h>
#include <wx/config.h>

#include <common.h>

#include <pcb_text.h>

namespace PCAD2KICAD {

PCB_TEXT::PCB_TEXT( PCB_CALLBACKS* aCallbacks, BOARD* aBoard ) : PCB_COMPONENT( aCallbacks,
                                                                                aBoard )
{
    m_objType = wxT( 'T' );
}


PCB_TEXT::~PCB_TEXT()
{
}


void PCB_TEXT::Parse( XNODE*        aNode,
                      int           aLayer,
                      wxString      aDefaultMeasurementUnit,
                      wxString      aActualConversion )
{
    XNODE*      lNode;
    wxString    str;

    m_PCadLayer     = aLayer;
    m_KiCadLayer    = GetKiCadLayer();
    m_positionX     = 0;
    m_positionY     = 0;
    m_name.mirror   = 0;    // Normal, not mirrored
    lNode = FindNode( aNode, wxT( "pt" ) );

    if( lNode )
        SetPosition( lNode->GetNodeContent(), aDefaultMeasurementUnit,
                     &m_positionX, &m_positionY, aActualConversion );

    lNode = FindNode( aNode, wxT( "rotation" ) );

    if( lNode )
    {
        str = lNode->GetNodeContent();
        str.Trim( false );
        m_rotation = StrToInt1Units( str );
    }

    aNode->GetAttribute( wxT( "Name" ), &m_name.text );
    m_name.text.Replace( "\r", "" );

    str = FindNodeGetContent( aNode, wxT( "justify" ) );
    m_name.justify = GetJustifyIdentificator( str );

    str = FindNodeGetContent( aNode, wxT( "isFlipped" ) );

    if( str == wxT( "True" ) )
        m_name.mirror = 1;

    lNode = FindNode( aNode, wxT( "textStyleRef" ) );

    if( lNode )
        SetFontProperty( lNode, &m_name, aDefaultMeasurementUnit, aActualConversion );
}


void PCB_TEXT::AddToModule( MODULE* aModule )
{
}


void PCB_TEXT::AddToBoard()
{
    m_name.textPositionX = m_positionX;
    m_name.textPositionY = m_positionY;
    m_name.textRotation = m_rotation;

    TEXTE_PCB* pcbtxt = new TEXTE_PCB( m_board );
    m_board->Add( pcbtxt, ADD_APPEND );

    pcbtxt->SetText( m_name.text );

    SetTextSizeFromStrokeFontHeight( pcbtxt, m_name.textHeight );

    pcbtxt->SetThickness( m_name.textstrokeWidth );
    pcbtxt->SetTextAngle( m_name.textRotation );

    SetTextJustify( pcbtxt, m_name.justify );
    pcbtxt->SetTextPos( wxPoint( m_name.textPositionX,
                                 m_name.textPositionY ) );

    pcbtxt->SetMirrored( m_name.mirror );
    pcbtxt->SetTimeStamp( 0 );

    pcbtxt->SetLayer( m_KiCadLayer );
}


// void PCB_TEXT::SetPosOffset( int aX_offs, int aY_offs )
// {
// PCB_COMPONENT::SetPosOffset( aX_offs, aY_offs );

// m_name.textPositionX    += aX_offs;
// m_name.textPositionY    += aY_offs;
// }

} // namespace PCAD2KICAD
