/*
 * This program source code file is part of KICAD, a free EDA CAD application.
 *
 * Copyright (C) 2020 CERN
 * Copyright (C) 2021-2023 KiCad Developers, see AUTHORS.TXT for contributors.
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

#include <wx/dc.h>
#include <wx/propgrid/propgrid.h>

#include <macros.h>
#include <validators.h>
#include <eda_draw_frame.h>
#include <eda_units.h>
#include <properties/color4d_variant.h>
#include <properties/eda_angle_variant.h>
#include <properties/pg_properties.h>
#include <properties/pg_editors.h>
#include <properties/property_mgr.h>
#include <properties/property.h>
#include <string_utils.h>
#include <widgets/color_swatch.h>

// reg-ex describing a signed valid value with a unit
static const wxChar REGEX_SIGNED_DISTANCE[] = wxT( "([-+]?[0-9]+[\\.?[0-9]*) *(mm|in|mils)*" );
static const wxChar REGEX_UNSIGNED_DISTANCE[] = wxT( "([0-9]+[\\.?[0-9]*) *(mm|in|mils)*" );


class wxAnyToEDA_ANGLE_VARIANTRegistrationImpl : public wxAnyToVariantRegistration
{
public:
    wxAnyToEDA_ANGLE_VARIANTRegistrationImpl( wxVariantDataFactory factory )
        : wxAnyToVariantRegistration( factory )
    {
    }

public:
    static bool IsSameClass(const wxAnyValueType* otherType)
    {
        return AreSameClasses( *s_instance.get(), *otherType );
    }

    static wxAnyValueType* GetInstance()
    {
        return s_instance.get();
    }

    virtual wxAnyValueType* GetAssociatedType() override
    {
        return wxAnyToEDA_ANGLE_VARIANTRegistrationImpl::GetInstance();
    }
private:
    static bool AreSameClasses(const wxAnyValueType& a, const wxAnyValueType& b)
    {
        return wxTypeId(a) == wxTypeId(b);
    }

    static wxAnyValueTypeScopedPtr s_instance;
};


wxAnyValueTypeScopedPtr wxAnyToEDA_ANGLE_VARIANTRegistrationImpl::s_instance( new wxAnyValueTypeImpl<EDA_ANGLE>() );

static wxAnyToEDA_ANGLE_VARIANTRegistrationImpl s_wxAnyToEDA_ANGLE_VARIANTRegistration( &EDA_ANGLE_VARIANT_DATA::VariantDataFactory );


class wxAnyToCOLOR4D_VARIANTRegistrationImpl : public wxAnyToVariantRegistration
{
public:
    wxAnyToCOLOR4D_VARIANTRegistrationImpl( wxVariantDataFactory factory )
            : wxAnyToVariantRegistration( factory )
    {
    }

public:
    static bool IsSameClass(const wxAnyValueType* otherType)
    {
        return AreSameClasses( *s_instance.get(), *otherType );
    }

    static wxAnyValueType* GetInstance()
    {
        return s_instance.get();
    }

    virtual wxAnyValueType* GetAssociatedType() override
    {
        return wxAnyToCOLOR4D_VARIANTRegistrationImpl::GetInstance();
    }
private:
    static bool AreSameClasses(const wxAnyValueType& a, const wxAnyValueType& b)
    {
        return wxTypeId(a) == wxTypeId(b);
    }

    static wxAnyValueTypeScopedPtr s_instance;
};

wxAnyValueTypeScopedPtr wxAnyToCOLOR4D_VARIANTRegistrationImpl::s_instance( new wxAnyValueTypeImpl<KIGFX::COLOR4D>() );

static wxAnyToCOLOR4D_VARIANTRegistrationImpl s_wxAnyToCOLOR4D_VARIANTRegistration( &COLOR4D_VARIANT_DATA::VariantDataFactory );


wxPGProperty* PGPropertyFactory( const PROPERTY_BASE* aProperty, EDA_DRAW_FRAME* aFrame )
{
    wxPGProperty* ret = nullptr;
    PROPERTY_DISPLAY display = aProperty->Display();

    switch( display )
    {
    case PROPERTY_DISPLAY::PT_SIZE:
        ret = new PGPROPERTY_SIZE();
        ret->SetEditor( PG_UNIT_EDITOR::BuildEditorName( aFrame ) );
        break;

    case PROPERTY_DISPLAY::PT_COORD:
        ret = new PGPROPERTY_COORD();
        static_cast<PGPROPERTY_COORD*>( ret )->SetCoordType( aProperty->CoordType() );
        ret->SetEditor( PG_UNIT_EDITOR::BuildEditorName( aFrame ) );
        break;

    case PROPERTY_DISPLAY::PT_DECIDEGREE:
    case PROPERTY_DISPLAY::PT_DEGREE:
    {
        PGPROPERTY_ANGLE* prop = new PGPROPERTY_ANGLE();

        if( display == PROPERTY_DISPLAY::PT_DECIDEGREE )
            prop->SetScale( 10.0 );

        ret = prop;
        ret->SetEditor( PG_UNIT_EDITOR::BuildEditorName( aFrame ) );
        break;
    }

    default:
        wxFAIL;
        KI_FALLTHROUGH;
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
            ret = new PGPROPERTY_BOOL();
        }
        else if( typeId == TYPE_HASH( wxString ) )
        {
            ret = new PGPROPERTY_STRING();
        }
        else if( typeId == TYPE_HASH( COLOR4D ) )
        {
            ret = new PGPROPERTY_COLOR4D();
        }
        else
        {
            wxFAIL_MSG( wxString::Format( wxS( "Property %s not supported by PGPropertyFactory" ),
                                          aProperty->Name() ) );
            ret = new wxPropertyCategory();
            ret->Enable( false );
        }
        break;
    }
    }

    if( ret )
    {
        ret->SetLabel( wxGetTranslation( aProperty->Name() ) );
        ret->SetName( aProperty->Name() );
        ret->SetHelpString( wxGetTranslation( aProperty->Name() ) );
        ret->SetClientData( const_cast<PROPERTY_BASE*>( aProperty ) );
    }

    return ret;
}


PGPROPERTY_DISTANCE::PGPROPERTY_DISTANCE( const wxString& aRegEx,
                                          ORIGIN_TRANSFORMS::COORD_TYPES_T aCoordType ) :
        m_coordType( aCoordType )
{
    m_regExValidator.reset( new REGEX_VALIDATOR( aRegEx ) );
}


PGPROPERTY_DISTANCE::~PGPROPERTY_DISTANCE()
{
}


bool PGPROPERTY_DISTANCE::StringToDistance( wxVariant& aVariant, const wxString& aText,
                                            int aArgFlags ) const
{
    // TODO(JE): Are there actual use cases for this?
    wxCHECK_MSG( false, false, wxS( "PGPROPERTY_DISTANCE::StringToDistance should not be used." ) );
}


wxString PGPROPERTY_DISTANCE::DistanceToString( wxVariant& aVariant, int aArgFlags ) const
{
    wxCHECK( aVariant.GetType() == wxPG_VARIANT_TYPE_LONG, wxEmptyString );

    long distanceIU = aVariant.GetLong();

    ORIGIN_TRANSFORMS* transforms = PROPERTY_MANAGER::Instance().GetTransforms();
    const EDA_IU_SCALE* iuScale   = PROPERTY_MANAGER::Instance().GetIuScale();

    if( transforms )
        distanceIU = transforms->ToDisplay( static_cast<long long int>( distanceIU ), m_coordType );

    switch( PROPERTY_MANAGER::Instance().GetUnits() )
    {
        case EDA_UNITS::INCHES:
            return wxString::Format( wxS( "%g in" ), iuScale->IUToMils( distanceIU ) / 1000.0 );

        case EDA_UNITS::MILS:
            return wxString::Format( wxS( "%d mils" ), iuScale->IUToMils( distanceIU ) );

        case EDA_UNITS::MILLIMETRES:
            return wxString::Format( wxS( "%g mm" ), iuScale->IUTomm( distanceIU ) );

        case EDA_UNITS::UNSCALED:
            return wxString::Format( wxS( "%li" ), distanceIU );

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
    return nullptr;
}


PGPROPERTY_COORD::PGPROPERTY_COORD( const wxString& aLabel, const wxString& aName,
                                    long aValue, ORIGIN_TRANSFORMS::COORD_TYPES_T aCoordType ) :
        wxIntProperty( aLabel, aName, aValue ),
        PGPROPERTY_DISTANCE( REGEX_SIGNED_DISTANCE, aCoordType )
{
}


wxValidator* PGPROPERTY_COORD::DoGetValidator() const
{
    return nullptr;
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
    if( aVariant.GetType() == wxPG_VARIANT_TYPE_DOUBLE )
    {
        // TODO(JE) Is this still needed?
        return wxString::Format( wxS( "%g\u00B0" ), aVariant.GetDouble() / m_scale );
    }
    else if( aVariant.GetType() == wxS( "EDA_ANGLE" ) )
    {
        wxString ret;
        static_cast<EDA_ANGLE_VARIANT_DATA*>( aVariant.GetData() )->Write( ret );
        return ret;
    }
    else
    {
        wxCHECK_MSG( false, wxEmptyString, wxS( "Unexpected variant type in PGPROPERTY_ANGLE" ) );
    }
}


wxValidator* PGPROPERTY_ANGLE::DoGetValidator() const
{
    return nullptr;
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

    wxColour color = GetColor( GetChoices().GetValue( index ) );

    if( color == wxNullColour )
        return;

    aDC.SetPen( *wxTRANSPARENT_PEN );
    aDC.SetBrush( wxBrush( color ) );
    aDC.DrawRectangle( aRect );

    aPaintData.m_drawnWidth = aRect.width;
}


wxString PGPROPERTY_STRING::ValueToString( wxVariant& aValue, int aFlags ) const
{
    if( aValue.GetType() != wxPG_VARIANT_TYPE_STRING )
        return wxEmptyString;

    return UnescapeString( aValue.GetString() );
}


bool PGPROPERTY_STRING::StringToValue( wxVariant& aVariant, const wxString& aString,
                                       int aFlags ) const
{
    aVariant = EscapeString( aString, CTX_QUOTED_STR );
    return true;
}


PGPROPERTY_BOOL::PGPROPERTY_BOOL( const wxString& aLabel, const wxString& aName, bool aValue ) :
            wxBoolProperty( aLabel, aName, aValue )
{
    SetEditor( PG_CHECKBOX_EDITOR::EDITOR_NAME );
}


const wxPGEditor* PGPROPERTY_BOOL::DoGetEditorClass() const
{
    wxCHECK_MSG( m_customEditor, wxPGEditor_CheckBox,
                 wxT( "Make sure to set custom editor for PGPROPERTY_BOOL!" ) );
    return m_customEditor;
}


PGPROPERTY_COLOR4D::PGPROPERTY_COLOR4D( const wxString& aLabel, const wxString& aName,
                                        COLOR4D aValue, COLOR4D aBackgroundColor ) :
        wxStringProperty( aLabel, aName, aValue.ToCSSString() ),
        m_backgroundColor( aBackgroundColor )
{
    SetEditor( PG_COLOR_EDITOR::EDITOR_NAME );
    SetFlag( wxPG_PROP_NOEDITOR );
}


bool PGPROPERTY_COLOR4D::StringToValue( wxVariant& aVariant, const wxString& aString,
                                        int aFlags ) const
{
    aVariant.SetData( new COLOR4D_VARIANT_DATA( aString ) );
    return true;
}


wxString PGPROPERTY_COLOR4D::ValueToString( wxVariant& aValue, int aFlags ) const
{
    wxString ret;

    if( aValue.IsType( wxS( "COLOR4D" ) ) )
        static_cast<COLOR4D_VARIANT_DATA*>( aValue.GetData() )->Write( ret );
    else
        return wxStringProperty::ValueToString( aValue, aFlags );

    return ret;
}
