/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2023 Mike Williams, mike@mikebwilliams.com
 * Copyright (C) 1992-2023 KiCad Developers, see AUTHORS.txt for contributors.
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

#include <pcb_field.h>
#include <footprint.h>
#include <board_design_settings.h>

PCB_FIELD::PCB_FIELD( FOOTPRINT* aParent, int aFieldId, const wxString& aName ) :
        PCB_TEXT( aParent, TEXT_TYPE( aFieldId ) )
{
    m_name = aName;
    SetId( aFieldId );
}


PCB_FIELD::PCB_FIELD( const PCB_TEXT& aText, int aFieldId, const wxString& aName ) :
        PCB_TEXT( aText )
{
    m_name = aName;
    SetId( aFieldId );
}


void PCB_FIELD::StyleFromSettings( const BOARD_DESIGN_SETTINGS& settings )
{
    SetTextSize( settings.GetTextSize( GetLayer() ) );
    SetTextThickness( settings.GetTextThickness( GetLayer() ) );
    SetItalic( settings.GetTextItalic( GetLayer() ) );
    SetKeepUpright( settings.GetTextUpright( GetLayer() ) );
    SetMirrored( IsBackLayer( GetLayer() ) );
}


wxString PCB_FIELD::GetName( bool aUseDefaultName ) const
{
    if( m_parent && m_parent->Type() == PCB_FOOTPRINT_T )
    {
        if( m_id >= 0 && m_id < MANDATORY_FIELDS )
            return TEMPLATE_FIELDNAME::GetDefaultFieldName( m_id );
        else if( m_name.IsEmpty() && aUseDefaultName )
            return TEMPLATE_FIELDNAME::GetDefaultFieldName( m_id );
        else
            return m_name;
    }
    else
    {
        wxFAIL_MSG( "Unhandled field owner type." );
        return m_name;
    }
}


wxString PCB_FIELD::GetCanonicalName() const
{
    if( m_parent && m_parent->Type() == PCB_FOOTPRINT_T )
    {
        switch( m_id )
        {
        case  REFERENCE_FIELD: return wxT( "Reference" );
        case  VALUE_FIELD:     return wxT( "Value" );
        case  FOOTPRINT_FIELD: return wxT( "Footprint" );
        case  DATASHEET_FIELD: return wxT( "Datasheet" );
        default:               return m_name;
        }
    }
    else
    {
        if( m_parent )
        {
            wxFAIL_MSG( wxString::Format( "Unhandled field owner type (id %d, parent type %d).",
                                          m_id, m_parent->Type() ) );
        }

        return m_name;
    }
}


void PCB_FIELD::SetId( int aId )
{
    m_id = aId;

    switch(m_id)
    {
        case REFERENCE_FIELD:
            SetType(TEXT_is_REFERENCE);
            break;
        case VALUE_FIELD:
            SetType(TEXT_is_VALUE);
            break;
        default:
            SetType(TEXT_is_DIVERS);
            break;
    }
}


EDA_ITEM* PCB_FIELD::Clone() const
{
    return new PCB_FIELD( *this );
}
