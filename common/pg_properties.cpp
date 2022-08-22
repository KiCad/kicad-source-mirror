/*
 * This program source code file is part of KICAD, a free EDA CAD application.
 *
 * Copyright (C) 2020 CERN
 * @author Maciej Suminski <maciej.suminski@cern.ch>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 3
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <pg_properties.h>
#include <property_mgr.h>
#include <wx/dc.h>
#include <wx/propgrid/propgrid.h>
#include <wx/regex.h>

#include <common.h>
#include <validators.h>
#include <convert_to_biu.h>
#include <property.h>
#include <widgets/color_swatch.h>

// reg-ex describing a signed valid value with a unit
static const wxChar REGEX_SIGNED_DISTANCE[] = wxT( "([-+]?[0-9]+[\\.?[0-9]*) *(mm|in|mils)*" );
static const wxChar REGEX_UNSIGNED_DISTANCE[] = wxT( "([0-9]+[\\.?[0-9]*) *(mm|in|mils)*" );

wxPGProperty* PGPropertyFactory( const PROPERTY_BASE* aProperty )
{
    wxPGProperty* ret = nullptr;
    PROPERTY_DISPLAY display = aProperty->Display();

    switch( display )
    {
    case PROPERTY_DISPLAY::PT_SIZE:
        ret = new PGPROPERTY_SIZE();
        break;

    case PROPERTY_DISPLAY::PT_COORD:
        ret = new PGPROPERTY_COORD();
        break;

    case PROPERTY_DISPLAY::PT_DECIDEGREE:
    case PROPERTY_DISPLAY::PT_DEGREE:
    {
        auto prop = new PGPROPERTY_ANGLE();

        if( display == PROPERTY_DISPLAY::PT_DECIDEGREE )
            prop->SetScale( 10.0 );

        ret = prop;
        break;
    }

    default:
        wxFAIL;
        /* fall through */
    case PROPERTY_DISPLAY::PT_DEFAULT:
    {
        // Create a corresponding wxPGProperty
        size_t typeId = aProperty->TypeHash();

        // Enum property
        if( aProperty->HasChoices() )
        {
            // I do not know why enum property takes a non-const reference to wxPGChoices..
            ret = new wxEnumProperty( wxPG_LABEL, wxPG_LABEL,
                    const_cast<wxPGChoices&>( aProperty->Choices() ) );
        }
        else if( typeId == TYPE_HASH( int ) || typeId == TYPE_HASH( long ) )
        {
            ret = new wxIntProperty();
        }
        else if( typeId == TYPE_HASH( unsigned int ) || typeId == TYPE_HASH( unsigned long ) )
        {
            ret = new wxUIntProperty();
        }
        else if( typeId == TYPE_HASH( float ) || typeId == TYPE_HASH( double ) )
        {
            ret = new wxFloatProperty();
        }
        else if( typeId == TYPE_HASH( bool ) )
        {
            ret = new wxBoolProperty();
            ret->SetAttribute( wxT( "UseCheckbox" ), true );
        }
        else if( typeId == TYPE_HASH( wxString ) )
        {
            ret = new wxStringProperty();
        }
        else
        {
            wxFAIL_MSG( wxString::Format( "Property '" + aProperty->Name() +
                        "' is not supported by PGPropertyFactory" ) );
            ret = new wxPropertyCategory();
            ret->Enable( false );
        }
        break;
    }
    }

    if( ret )
    {
        ret->SetLabel( aProperty->Name() );
        ret->SetName( aProperty->Name() );
        ret->Enable( !aProperty->IsReadOnly() );
        ret->SetClientData( const_cast<PROPERTY_BASE*>( aProperty ) );
    }

    return ret;
}


PGPROPERTY_DISTANCE::PGPROPERTY_DISTANCE( const wxString& aRegEx )
{
    m_regExValidator.reset( new REGEX_VALIDATOR( aRegEx ) );
}


PGPROPERTY_DISTANCE::~PGPROPERTY_DISTANCE()
{
}


bool PGPROPERTY_DISTANCE::StringToDistance( wxVariant& aVariant, const wxString& aText, int aArgFlags ) const
{
    wxRegEx regDimension( m_regExValidator->GetRegEx(), wxRE_ICASE );
    wxASSERT( regDimension.IsValid() );

    if( !regDimension.Matches( aText ) )
    {
        aVariant.MakeNull();
        return false;
    }


    // Get the value
    wxString valueText = regDimension.GetMatch( aText, 1 );
    double value = 0.0;

    if( !valueText.ToDouble( &value ) )
    {
        aVariant.MakeNull();
        return false;
    }


    // Determine units: use the app setting if unit is not explicitly specified
    EDA_UNITS unit;
    wxString unitText = regDimension.GetMatch( aText, 2 ).Lower();

    if( unitText == "mm" )
        unit = EDA_UNITS::MILLIMETRES;
    else if( unitText == "in" )
        unit = EDA_UNITS::INCHES;
    else if( unitText == "mils" )
        unit = EDA_UNITS::MILS;
    else
        unit = PROPERTY_MANAGER::Instance().GetUnits();


    // Conversion to internal units
    long newValueIU;

    switch( unit )
    {
        case EDA_UNITS::INCHES:
            newValueIU = Mils2iu( value * 1000.0 );
            break;

        case EDA_UNITS::MILS:
            newValueIU = Mils2iu( value );
            break;

        case EDA_UNITS::MILLIMETRES:
            newValueIU = Millimeter2iu( value );
            break;

        case EDA_UNITS::UNSCALED:
            newValueIU = value;
            break;

        default:
            // DEGREEs are handled by PGPROPERTY_ANGLE
            wxFAIL;
            break;
    }

    if( aVariant.IsNull() || newValueIU != aVariant.GetLong() )
    {
        aVariant = newValueIU;
        return true;
    }

    return false;
}


wxString PGPROPERTY_DISTANCE::DistanceToString( wxVariant& aVariant, int aArgFlags ) const
{
    wxCHECK( aVariant.GetType() == wxPG_VARIANT_TYPE_LONG, wxEmptyString );

    switch( PROPERTY_MANAGER::Instance().GetUnits() )
    {
        case EDA_UNITS::INCHES:
            return wxString::Format( wxT( "%g in" ), Iu2Mils( aVariant.GetLong() ) / 1000.0 );

        case EDA_UNITS::MILS:
            return wxString::Format( wxT( "%g mils" ), Iu2Mils( aVariant.GetLong() ) );

        case EDA_UNITS::MILLIMETRES:
            return wxString::Format( wxT( "%g mm" ), Iu2Millimeter( aVariant.GetLong() ) );

        case EDA_UNITS::UNSCALED:
            return wxString::Format( wxT( "%li" ), aVariant.GetLong() );

        default:
            // DEGREEs are handled by PGPROPERTY_ANGLE
            break;
    }

    wxFAIL;
    return wxEmptyString;
}


PGPROPERTY_SIZE::PGPROPERTY_SIZE( const wxString& aLabel, const wxString& aName,
        long aValue )
    : wxUIntProperty( aLabel, aName, aValue ), PGPROPERTY_DISTANCE( REGEX_UNSIGNED_DISTANCE )
{
}


wxValidator* PGPROPERTY_SIZE::DoGetValidator() const
{
    return m_regExValidator.get();
}


PGPROPERTY_COORD::PGPROPERTY_COORD( const wxString& aLabel, const wxString& aName,
        long aValue )
    : wxIntProperty( aLabel, aName, aValue ), PGPROPERTY_DISTANCE( REGEX_SIGNED_DISTANCE )
{
}


wxValidator* PGPROPERTY_COORD::DoGetValidator() const
{
    return m_regExValidator.get();
}


bool PGPROPERTY_ANGLE::StringToValue( wxVariant& aVariant, const wxString& aText, int aArgFlags ) const
{
    double value = 0.0;

    if( !aText.ToDouble( &value ) )
    {
        aVariant.MakeNull();
        return true;
    }

    value *= m_scale;

    if( aVariant.IsNull() || aVariant.GetDouble() != value )
    {
        aVariant = value;
        return true;
    }

    return false;
}


wxString PGPROPERTY_ANGLE::ValueToString( wxVariant& aVariant, int aArgFlags ) const
{
    wxCHECK( aVariant.GetType() == wxPG_VARIANT_TYPE_DOUBLE, wxEmptyString );
    return wxString::Format( wxT("%g\u00B0"), aVariant.GetDouble() / m_scale );
}


wxSize PGPROPERTY_COLORENUM::OnMeasureImage( int aItem ) const
{
    // TODO(JE) calculate size from window metrics?
    return wxSize( 16, 12 );
}


void PGPROPERTY_COLORENUM::OnCustomPaint( wxDC& aDC, const wxRect& aRect,
                                          wxPGPaintData& aPaintData )
{
    int index = aPaintData.m_choiceItem;

    if( index < 0 )
        index = GetIndex();

    // GetIndex can return -1 when the control hasn't been set up yet
    if( index < 0 || index >= static_cast<int>( GetChoices().GetCount() ) )
        return;

    wxString layer = GetChoices().GetLabel( index );
    wxColour color = GetColor( layer );

    if( color == wxNullColour )
        return;

    aDC.SetPen( *wxTRANSPARENT_PEN );
    aDC.SetBrush( wxBrush( color ) );
    aDC.DrawRectangle( aRect );

    aPaintData.m_drawnWidth = aRect.width;
}
