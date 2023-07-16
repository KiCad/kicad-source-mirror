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
#include <i18n_utility.h>

PCB_FIELD::PCB_FIELD( FOOTPRINT* aParent, int aFieldId, const wxString& aName ) :
        PCB_TEXT( aParent, PCB_FIELD_T ),
        m_id( aFieldId ),
        m_name( aName )
{
}


PCB_FIELD::PCB_FIELD( const PCB_TEXT& aText, int aFieldId, const wxString& aName ) :
        PCB_TEXT( aText ),
        m_id( aFieldId ),
        m_name( aName )
{
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
        if( m_id < MANDATORY_FIELDS )
            return TEMPLATE_FIELDNAME::GetDefaultFieldName( m_id, false );
        else
            return m_name;
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


wxString PCB_FIELD::GetTextTypeDescription() const
{
    if( m_id < MANDATORY_FIELDS )
        return TEMPLATE_FIELDNAME::GetDefaultFieldName( m_id, false );
    else
        return _( "User Field" );
}


wxString PCB_FIELD::GetItemDescription( UNITS_PROVIDER* aUnitsProvider ) const
{
    switch( m_id )
    {
    case REFERENCE_FIELD:
        return wxString::Format( _( "Reference '%s'" ),
                                 GetParentFootprint()->GetReference() );

    case VALUE_FIELD:
        return wxString::Format( _( "Value '%s' of %s" ),
                                 KIUI::EllipsizeMenuText( GetText() ),
                                 GetParentFootprint()->GetReference() );

    case FOOTPRINT_FIELD:
        return wxString::Format( _( "Footprint '%s' of %s" ),
                                 KIUI::EllipsizeMenuText( GetText() ),
                                 GetParentFootprint()->GetReference() );
    case DATASHEET_FIELD:
        return wxString::Format( _( "Datasheet '%s' of %s" ),
                                 KIUI::EllipsizeMenuText( GetText() ),
                                 GetParentFootprint()->GetReference() );

    default: 
        break; // avoid unreachable code / missing return statement warnings
    }

    return wxString::Format( _( "Field '%s' of %s" ), KIUI::EllipsizeMenuText( GetText() ),
                             GetParentFootprint()->GetReference() );
}


double PCB_FIELD::ViewGetLOD( int aLayer, KIGFX::VIEW* aView ) const
{
    constexpr double HIDE = std::numeric_limits<double>::max();

    if( !aView )
        return 0.0;

    // Handle Render tab switches
    if( IsValue() && !aView->IsLayerVisible( LAYER_MOD_VALUES ) )
        return HIDE;

    if( IsReference() && !aView->IsLayerVisible( LAYER_MOD_REFERENCES ) )
        return HIDE;

    return PCB_TEXT::ViewGetLOD( aLayer, aView );
}


EDA_ITEM* PCB_FIELD::Clone() const
{
    return new PCB_FIELD( *this );
}


static struct PCB_FIELD_DESC
{
    PCB_FIELD_DESC()
    {
        PROPERTY_MANAGER& propMgr = PROPERTY_MANAGER::Instance();
        REGISTER_TYPE( PCB_FIELD );
        propMgr.AddTypeCast( new TYPE_CAST<PCB_FIELD, PCB_TEXT> );
        propMgr.AddTypeCast( new TYPE_CAST<PCB_FIELD, BOARD_ITEM> );
        propMgr.AddTypeCast( new TYPE_CAST<PCB_FIELD, EDA_TEXT> );
        propMgr.InheritsAfter( TYPE_HASH( PCB_FIELD ), TYPE_HASH( BOARD_ITEM ) );
        propMgr.InheritsAfter( TYPE_HASH( PCB_FIELD ), TYPE_HASH( PCB_TEXT ) );
        propMgr.InheritsAfter( TYPE_HASH( PCB_FIELD ), TYPE_HASH( EDA_TEXT ) );

        auto isNotFootprintFootprint =
                []( INSPECTABLE* aItem ) -> bool
                {
                    if( PCB_FIELD* field = dynamic_cast<PCB_FIELD*>( aItem ) )
                        return !field->IsFootprint();

                    return true;
                };

        propMgr.OverrideAvailability( TYPE_HASH( PCB_FIELD ), TYPE_HASH( EDA_TEXT ), _HKI( "Text" ),
                                      isNotFootprintFootprint );
    }
} _PCB_FIELD_DESC;
