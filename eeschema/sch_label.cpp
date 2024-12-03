/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2016 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 2015 Wayne Stambaugh <stambaughw@gmail.com>
 * Copyright (C) 1992-2024 KiCad Developers, see AUTHORS.txt for contributors.
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

#include <base_units.h>
#include <pgm_base.h>
#include <sch_edit_frame.h>
#include <sch_plotter.h>
#include <widgets/msgpanel.h>
#include <bitmaps.h>
#include <string_utils.h>
#include <schematic.h>
#include <settings/color_settings.h>
#include <sch_painter.h>
#include <default_values.h>
#include <wx/debug.h>
#include <wx/log.h>
#include <dialogs/html_message_box.h>
#include <project/project_file.h>
#include <project/net_settings.h>
#include <core/kicad_algo.h>
#include <core/mirror.h>
#include <trigo.h>
#include <sch_label.h>

using KIGFX::SCH_RENDER_SETTINGS;


bool IncrementLabelMember( wxString& name, int aIncrement )
{
    if( name.IsEmpty() )
        return true;

    wxString suffix;
    wxString digits;
    wxString outputFormat;
    wxString outputNumber;
    int      ii     = name.Len() - 1;
    int      dCount = 0;

    while( ii >= 0 && !wxIsdigit( name.GetChar( ii ) ) )
    {
        suffix = name.GetChar( ii ) + suffix;
        ii--;
    }

    while( ii >= 0 && wxIsdigit( name.GetChar( ii ) ) )
    {
        digits = name.GetChar( ii ) + digits;
        ii--;
        dCount++;
    }

    if( digits.IsEmpty() )
        return true;

    long number = 0;

    if( digits.ToLong( &number ) )
    {
        number += aIncrement;

        // Don't let result go below zero

        if( number > -1 )
        {
            name.Remove( ii + 1 );
            //write out a format string with correct number of leading zeroes
            outputFormat.Printf( wxS( "%%0%dld" ), dCount );
            //write out the number using the format string
            outputNumber.Printf( outputFormat, number );
            name << outputNumber << suffix;
            return true;
        }
    }

    return false;
}


/* Coding polygons for global symbol graphic shapes.
 *  the first parml is the number of corners
 *  others are the corners coordinates in reduced units
 *  the real coordinate is the reduced coordinate * text half size
 */
static int  TemplateIN_HN[] = { 6, 0, 0, -1, -1, -2, -1, -2, 1, -1, 1, 0, 0 };
static int  TemplateIN_HI[] = { 6, 0, 0, 1, 1, 2, 1, 2, -1, 1, -1, 0, 0 };
static int  TemplateIN_UP[] = { 6, 0, 0, 1, -1, 1, -2, -1, -2, -1, -1, 0, 0 };
static int  TemplateIN_BOTTOM[] = { 6, 0, 0, 1, 1, 1, 2, -1, 2, -1, 1, 0, 0 };

static int  TemplateOUT_HN[] = { 6, -2, 0, -1, 1, 0, 1, 0, -1, -1, -1, -2, 0 };
static int  TemplateOUT_HI[] = { 6, 2, 0, 1, -1, 0, -1, 0, 1, 1, 1, 2, 0 };
static int  TemplateOUT_UP[] = { 6, 0, -2, 1, -1, 1, 0, -1, 0, -1, -1, 0, -2 };
static int  TemplateOUT_BOTTOM[] = { 6, 0, 2, 1, 1, 1, 0, -1, 0, -1, 1, 0, 2 };

static int  TemplateUNSPC_HN[] = { 5, 0, -1, -2, -1, -2, 1, 0, 1, 0, -1 };
static int  TemplateUNSPC_HI[] = { 5, 0, -1, 2, -1, 2, 1, 0, 1, 0, -1 };
static int  TemplateUNSPC_UP[] = { 5, 1, 0, 1, -2, -1, -2, -1, 0, 1, 0 };
static int  TemplateUNSPC_BOTTOM[] = { 5, 1, 0, 1, 2, -1, 2, -1, 0, 1, 0 };

static int  TemplateBIDI_HN[] = { 5, 0, 0, -1, -1, -2, 0, -1, 1, 0, 0 };
static int  TemplateBIDI_HI[] = { 5, 0, 0, 1, -1, 2, 0, 1, 1, 0, 0 };
static int  TemplateBIDI_UP[] = { 5, 0, 0, -1, -1, 0, -2, 1, -1, 0, 0 };
static int  TemplateBIDI_BOTTOM[] = { 5, 0, 0, -1, 1, 0, 2, 1, 1, 0, 0 };

static int  Template3STATE_HN[] = { 5, 0, 0, -1, -1, -2, 0, -1, 1, 0, 0 };
static int  Template3STATE_HI[] = { 5, 0, 0, 1, -1, 2, 0, 1, 1, 0, 0 };
static int  Template3STATE_UP[] = { 5, 0, 0, -1, -1, 0, -2, 1, -1, 0, 0 };
static int  Template3STATE_BOTTOM[] = { 5, 0, 0, -1, 1, 0, 2, 1, 1, 0, 0 };

static int* TemplateShape[5][4] =
{
    { TemplateIN_HN,     TemplateIN_UP,     TemplateIN_HI,     TemplateIN_BOTTOM     },
    { TemplateOUT_HN,    TemplateOUT_UP,    TemplateOUT_HI,    TemplateOUT_BOTTOM    },
    { TemplateBIDI_HN,   TemplateBIDI_UP,   TemplateBIDI_HI,   TemplateBIDI_BOTTOM   },
    { Template3STATE_HN, Template3STATE_UP, Template3STATE_HI, Template3STATE_BOTTOM },
    { TemplateUNSPC_HN,  TemplateUNSPC_UP,  TemplateUNSPC_HI,  TemplateUNSPC_BOTTOM  }
};


wxString getElectricalTypeLabel( LABEL_FLAG_SHAPE aType )
{
    switch( aType )
    {
    case LABEL_FLAG_SHAPE::L_INPUT:       return _( "Input" );
    case LABEL_FLAG_SHAPE::L_OUTPUT:      return _( "Output" );
    case LABEL_FLAG_SHAPE::L_BIDI:        return _( "Bidirectional" );
    case LABEL_FLAG_SHAPE::L_TRISTATE:    return _( "Tri-State" );
    case LABEL_FLAG_SHAPE::L_UNSPECIFIED: return _( "Passive" );
    default:                              return wxT( "???" );
    }
}


SPIN_STYLE SPIN_STYLE::RotateCCW()
{
    SPIN newSpin = m_spin;

    switch( m_spin )
    {
    case SPIN_STYLE::LEFT:   newSpin = SPIN_STYLE::BOTTOM; break;
    case SPIN_STYLE::BOTTOM: newSpin = SPIN_STYLE::RIGHT;  break;
    case SPIN_STYLE::RIGHT:  newSpin = SPIN_STYLE::UP;     break;
    case SPIN_STYLE::UP:     newSpin = SPIN_STYLE::LEFT;   break;
    }

    return SPIN_STYLE( newSpin );
}


SPIN_STYLE SPIN_STYLE::MirrorX()
{
    SPIN newSpin = m_spin;

    switch( m_spin )
    {
    case SPIN_STYLE::UP:     newSpin = SPIN_STYLE::BOTTOM; break;
    case SPIN_STYLE::BOTTOM: newSpin = SPIN_STYLE::UP;     break;
    case SPIN_STYLE::LEFT:                                      break;
    case SPIN_STYLE::RIGHT:                                     break;
    }

    return SPIN_STYLE( newSpin );
}


SPIN_STYLE SPIN_STYLE::MirrorY()
{
    SPIN newSpin = m_spin;

    switch( m_spin )
    {
    case SPIN_STYLE::LEFT:   newSpin = SPIN_STYLE::RIGHT; break;
    case SPIN_STYLE::RIGHT:  newSpin = SPIN_STYLE::LEFT;  break;
    case SPIN_STYLE::UP:                                       break;
    case SPIN_STYLE::BOTTOM:                                   break;
    }

    return SPIN_STYLE( newSpin );
}


SCH_LABEL_BASE::SCH_LABEL_BASE( const VECTOR2I& aPos, const wxString& aText, KICAD_T aType ) :
        SCH_TEXT( aPos, aText, aType ),
        m_shape( L_UNSPECIFIED ),
        m_connectionType( CONNECTION_TYPE::NONE ),
        m_isDangling( true ),
        m_lastResolvedColor( COLOR4D::UNSPECIFIED )
{
    SetMultilineAllowed( false );
    ClearFieldsAutoplaced();    // fields are not yet autoplaced.

    if( !HasTextVars() )
        m_cached_driver_name = EscapeString( EDA_TEXT::GetShownText( true, 0 ), CTX_NETNAME );
}


SCH_LABEL_BASE::SCH_LABEL_BASE( const SCH_LABEL_BASE& aLabel ) :
        SCH_TEXT( aLabel ),
        m_shape( aLabel.m_shape ),
        m_connectionType( aLabel.m_connectionType ),
        m_isDangling( aLabel.m_isDangling ),
        m_lastResolvedColor( aLabel.m_lastResolvedColor ),
        m_cached_driver_name( aLabel.m_cached_driver_name )
{
    SetMultilineAllowed( false );

    m_fields = aLabel.m_fields;

    for( SCH_FIELD& field : m_fields )
        field.SetParent( this );
}


SCH_LABEL_BASE& SCH_LABEL_BASE::operator=( const SCH_LABEL_BASE& aLabel )
{
    SCH_TEXT::operator=( aLabel );

    m_cached_driver_name = aLabel.m_cached_driver_name;

    return *this;
}


const wxString SCH_LABEL_BASE::GetDefaultFieldName( const wxString& aName, bool aUseDefaultName )
{
    if( aName == wxT( "Intersheetrefs" ) )
        return _( "Sheet References" );
    else if( aName == wxT( "Netclass" ) )
        return _( "Net Class" );
    else if( aName.IsEmpty() && aUseDefaultName )
        return _( "Field" );
    else
        return aName;
}


bool SCH_LABEL_BASE::IsType( const std::vector<KICAD_T>& aScanTypes ) const
{
    if( SCH_TEXT::IsType( aScanTypes ) )
        return true;

    for( KICAD_T scanType : aScanTypes )
    {
        if( scanType == SCH_LABEL_LOCATE_ANY_T )
            return true;
    }

    wxCHECK_MSG( Schematic(), false, wxT( "No parent SCHEMATIC set for SCH_LABEL!" ) );

    // Ensure m_connected_items for Schematic()->CurrentSheet() exists.
    // Can be not the case when "this" is living in clipboard
    if( m_connected_items.find( Schematic()->CurrentSheet() ) == m_connected_items.end() )
        return false;

    const SCH_ITEM_VEC& item_set = m_connected_items.at( Schematic()->CurrentSheet() );

    for( KICAD_T scanType : aScanTypes )
    {
        if( scanType == SCH_LABEL_LOCATE_WIRE_T )
        {
            for( SCH_ITEM* connection : item_set )
            {
                if( connection->IsType( { SCH_ITEM_LOCATE_WIRE_T, SCH_PIN_T } ) )
                    return true;
            }
        }

        if ( scanType == SCH_LABEL_LOCATE_BUS_T )
        {
            for( SCH_ITEM* connection : item_set )
            {
                if( connection->IsType( { SCH_ITEM_LOCATE_BUS_T } ) )
                    return true;
            }
        }
    }

    return false;
}


void SCH_LABEL_BASE::SwapData( SCH_ITEM* aItem )
{
    SCH_TEXT::SwapData( aItem );

    SCH_LABEL_BASE* label = static_cast<SCH_LABEL_BASE*>( aItem );

    m_fields.swap( label->m_fields );
    std::swap( m_fieldsAutoplaced, label->m_fieldsAutoplaced );

    for( SCH_FIELD& field : m_fields )
        field.SetParent( this );

    for( SCH_FIELD& field : label->m_fields )
        field.SetParent( label );

    std::swap( m_shape, label->m_shape );
    std::swap( m_connectionType, label->m_connectionType );
    std::swap( m_isDangling, label->m_isDangling );
    std::swap( m_lastResolvedColor, label->m_lastResolvedColor );
}


COLOR4D SCH_LABEL_BASE::GetLabelColor() const
{
    if( GetTextColor() != COLOR4D::UNSPECIFIED )
        m_lastResolvedColor = GetTextColor();
    else if( !IsConnectivityDirty() )
        m_lastResolvedColor = GetEffectiveNetClass()->GetSchematicColor();

    return m_lastResolvedColor;
}


void SCH_LABEL_BASE::SetSpinStyle( SPIN_STYLE aSpinStyle )
{
    // Assume "Right" and Left" mean which side of the anchor the text will be on
    // Thus we want to left justify text up against the anchor if we are on the right
    switch( aSpinStyle )
    {
    default:
        wxFAIL_MSG( "Bad spin style" );
        KI_FALLTHROUGH;

    case SPIN_STYLE::RIGHT:            // Horiz Normal Orientation
        SetTextAngle( ANGLE_HORIZONTAL );
        SetHorizJustify( GR_TEXT_H_ALIGN_LEFT );
        break;

    case SPIN_STYLE::UP:               // Vert Orientation UP
        SetTextAngle( ANGLE_VERTICAL );
        SetHorizJustify( GR_TEXT_H_ALIGN_LEFT );
        break;

    case SPIN_STYLE::LEFT:             // Horiz Orientation - Right justified
        SetTextAngle( ANGLE_HORIZONTAL );
        SetHorizJustify( GR_TEXT_H_ALIGN_RIGHT );
        break;

    case SPIN_STYLE::BOTTOM:           //  Vert Orientation BOTTOM
        SetTextAngle( ANGLE_VERTICAL );
        SetHorizJustify( GR_TEXT_H_ALIGN_RIGHT );
        break;
    }

    SetVertJustify( GR_TEXT_V_ALIGN_BOTTOM );
}


SPIN_STYLE SCH_LABEL_BASE::GetSpinStyle() const
{
    if( GetTextAngle() == ANGLE_VERTICAL )
    {
        if( GetHorizJustify() == GR_TEXT_H_ALIGN_RIGHT )
            return SPIN_STYLE::BOTTOM;
        else
            return SPIN_STYLE::UP;
    }
    else
    {
        if( GetHorizJustify() == GR_TEXT_H_ALIGN_RIGHT )
            return SPIN_STYLE::LEFT;
        else
            return SPIN_STYLE::RIGHT;
    }
}


VECTOR2I SCH_LABEL_BASE::GetSchematicTextOffset( const RENDER_SETTINGS* aSettings ) const
{
    VECTOR2I text_offset;

    // add an offset to x (or y) position to aid readability of text on a wire or line
    int dist = GetTextOffset( aSettings ) + GetPenWidth();

    switch( GetSpinStyle() )
    {
    case SPIN_STYLE::UP:
    case SPIN_STYLE::BOTTOM: text_offset.x = -dist;  break; // Vert Orientation
    default:
    case SPIN_STYLE::LEFT:
    case SPIN_STYLE::RIGHT: text_offset.y = -dist;  break; // Horiz Orientation
    }

    return text_offset;
}


void SCH_LABEL_BASE::SetPosition( const VECTOR2I& aPosition )
{
    VECTOR2I offset = aPosition - GetTextPos();
    Move( offset );
}


void SCH_LABEL_BASE::Move( const VECTOR2I& aMoveVector )
{
    SCH_TEXT::Move( aMoveVector );

    for( SCH_FIELD& field : m_fields )
        field.Offset( aMoveVector );
}


void SCH_LABEL_BASE::Rotate( const VECTOR2I& aCenter )
{
    VECTOR2I pt = GetTextPos();
    RotatePoint( pt, aCenter, ANGLE_90 );
    VECTOR2I offset = pt - GetTextPos();

    Rotate90( false );

    SetTextPos( GetTextPos() + offset );

    for( SCH_FIELD& field : m_fields )
        field.SetTextPos( field.GetTextPos() + offset );
}


void SCH_LABEL_BASE::Rotate90( bool aClockwise )
{
    SCH_TEXT::Rotate90( aClockwise );

    if( m_fieldsAutoplaced == FIELDS_AUTOPLACED_AUTO )
    {
        AutoplaceFields( /* aScreen */ nullptr, /* aManual */ false );
    }
    else
    {
        for( SCH_FIELD& field : m_fields )
        {
            if( field.GetTextAngle().IsVertical()
                    && field.GetHorizJustify() == GR_TEXT_H_ALIGN_LEFT )
            {
                if( !aClockwise )
                    field.SetHorizJustify( GR_TEXT_H_ALIGN_RIGHT );

                field.SetTextAngle( ANGLE_HORIZONTAL );
            }
            else if( field.GetTextAngle().IsVertical()
                        && field.GetHorizJustify() == GR_TEXT_H_ALIGN_RIGHT )
            {
                if( !aClockwise )
                    field.SetHorizJustify( GR_TEXT_H_ALIGN_LEFT );

                field.SetTextAngle( ANGLE_HORIZONTAL );
            }
            else if( field.GetTextAngle().IsHorizontal()
                        && field.GetHorizJustify() == GR_TEXT_H_ALIGN_LEFT )
            {
                if( aClockwise )
                    field.SetHorizJustify( GR_TEXT_H_ALIGN_LEFT );

                field.SetTextAngle( ANGLE_VERTICAL );
            }
            else if( field.GetTextAngle().IsHorizontal()
                        && field.GetHorizJustify() == GR_TEXT_H_ALIGN_RIGHT )
            {
                if( aClockwise )
                    field.SetHorizJustify( GR_TEXT_H_ALIGN_LEFT );

                field.SetTextAngle( ANGLE_VERTICAL );
            }

            VECTOR2I pos = field.GetTextPos();
            RotatePoint( pos, GetPosition(), aClockwise ? -ANGLE_90 : ANGLE_90 );
            field.SetTextPos( pos );
        }
    }
}


void SCH_LABEL_BASE::MirrorSpinStyle( bool aLeftRight )
{
    SCH_TEXT::MirrorSpinStyle( aLeftRight );

    for( SCH_FIELD& field : m_fields )
    {
        if( ( aLeftRight && field.GetTextAngle().IsHorizontal() )
                || ( !aLeftRight && field.GetTextAngle().IsVertical() ) )
        {
            if( field.GetHorizJustify() == GR_TEXT_H_ALIGN_LEFT )
                field.SetHorizJustify( GR_TEXT_H_ALIGN_RIGHT );
            else
                field.SetHorizJustify( GR_TEXT_H_ALIGN_LEFT );
        }

        VECTOR2I pos = field.GetTextPos();
        VECTOR2I delta = (VECTOR2I)GetPosition() - pos;

        if( aLeftRight )
            pos.x = GetPosition().x + delta.x;
        else
            pos.y = GetPosition().y + delta.y;

        field.SetTextPos( pos );
    }
}


void SCH_LABEL_BASE::MirrorHorizontally( int aCenter )
{
    VECTOR2I old_pos = GetPosition();
    SCH_TEXT::MirrorHorizontally( aCenter );

    for( SCH_FIELD& field : m_fields )
    {
        if( field.GetTextAngle() == ANGLE_HORIZONTAL )
            field.FlipHJustify();

        VECTOR2I pos = field.GetTextPos();
        VECTOR2I delta = old_pos - pos;
        pos.x = GetPosition().x + delta.x;

        field.SetPosition( pos );
    }
}


void SCH_LABEL_BASE::MirrorVertically( int aCenter )
{
    VECTOR2I old_pos = GetPosition();
    SCH_TEXT::MirrorVertically( aCenter );

    for( SCH_FIELD& field : m_fields )
    {
        if( field.GetTextAngle() == ANGLE_VERTICAL )
            field.FlipHJustify();

        VECTOR2I pos = field.GetTextPos();
        VECTOR2I delta = old_pos - pos;
        pos.y = GetPosition().y + delta.y;

        field.SetPosition( pos );
    }
}


bool SCH_LABEL_BASE::IncrementLabel( int aIncrement )
{
    wxString text = GetText();

    if( IncrementLabelMember( text, aIncrement ) )
    {
        SetText( text );
        return true;
    }

    return false;
}


bool SCH_LABEL_BASE::operator==( const SCH_ITEM& aOther ) const
{
    const SCH_LABEL_BASE* other = dynamic_cast<const SCH_LABEL_BASE*>( &aOther );

    if( !other )
        return false;

    if( m_shape != other->m_shape )
        return false;

    if( m_connectionType != other->m_connectionType )
        return false;

    if( m_fields.size() != other->m_fields.size() )
        return false;

    for( size_t ii = 0; ii < m_fields.size(); ++ii )
    {
        if( !( m_fields[ii] == other->m_fields[ii] ) )
            return false;
    }

    return SCH_TEXT::operator==( aOther );
}


double SCH_LABEL_BASE::Similarity( const SCH_ITEM& aOther ) const
{
    const SCH_LABEL_BASE* other = dynamic_cast<const SCH_LABEL_BASE*>( &aOther );

    if( !other )
        return 0.0;

    if( m_Uuid == other->m_Uuid )
        return 1.0;

    double similarity = SCH_TEXT::Similarity( aOther );

    if( typeid( *this ) != typeid( aOther ) )
        similarity *= 0.9;

    if( m_shape == other->m_shape )
        similarity *= 0.9;

    if( m_connectionType == other->m_connectionType )
        similarity *= 0.9;

    for( size_t ii = 0; ii < m_fields.size(); ++ii )
    {
        if( ii >= other->m_fields.size() )
            break;

        similarity *= m_fields[ii].Similarity( other->m_fields[ii] );
    }

    int diff = std::abs( int( m_fields.size() ) - int( other->m_fields.size() ) );

    similarity *= std::pow( 0.9, diff );

    return similarity;
}


void SCH_LABEL_BASE::AutoplaceFields( SCH_SCREEN* aScreen, bool aManual )
{
    int margin = GetTextOffset() * 2;
    int labelLen = GetBodyBoundingBox().GetSizeMax();
    int accumulated = GetTextHeight() / 2;

    if( Type() == SCH_GLOBAL_LABEL_T )
        accumulated += margin + GetPenWidth() + margin;

    for( SCH_FIELD& field : m_fields )
    {
        VECTOR2I offset( 0, 0 );

        switch( GetSpinStyle() )
        {
        default:
        case SPIN_STYLE::LEFT:
            field.SetTextAngle( ANGLE_HORIZONTAL );
            field.SetHorizJustify( GR_TEXT_H_ALIGN_RIGHT );

            if( field.GetCanonicalName() == wxT( "Intersheetrefs" ) )
                offset.x = - ( labelLen + margin );
            else
                offset.y = accumulated + field.GetTextHeight() / 2;

            break;

        case SPIN_STYLE::UP:
            field.SetTextAngle( ANGLE_VERTICAL );
            field.SetHorizJustify( GR_TEXT_H_ALIGN_LEFT );

            if( field.GetCanonicalName() == wxT( "Intersheetrefs" ) )
                offset.y = - ( labelLen + margin );
            else
                offset.x = accumulated + field.GetTextHeight() / 2;

            break;

        case SPIN_STYLE::RIGHT:
            field.SetTextAngle( ANGLE_HORIZONTAL );
            field.SetHorizJustify( GR_TEXT_H_ALIGN_LEFT );

            if( field.GetCanonicalName() == wxT( "Intersheetrefs" ) )
                offset.x = labelLen + margin;
            else
                offset.y = accumulated + field.GetTextHeight() / 2;

            break;

        case SPIN_STYLE::BOTTOM:
            field.SetTextAngle( ANGLE_VERTICAL );
            field.SetHorizJustify( GR_TEXT_H_ALIGN_RIGHT );

            if( field.GetCanonicalName() == wxT( "Intersheetrefs" ) )
                offset.y = labelLen + margin;
            else
                offset.x = accumulated + field.GetTextHeight() / 2;

            break;
        }

        field.SetTextPos( GetTextPos() + offset );

        if( field.GetCanonicalName() != wxT( "Intersheetrefs" ) )
            accumulated += field.GetTextHeight() + margin;
    }

    m_fieldsAutoplaced = FIELDS_AUTOPLACED_AUTO;
}


void SCH_LABEL_BASE::GetIntersheetRefs( std::vector<std::pair<wxString, wxString>>* pages )
{
    wxCHECK( pages, /* void */ );

    if( Schematic() )
    {
        auto it = Schematic()->GetPageRefsMap().find( GetText() );

        if( it != Schematic()->GetPageRefsMap().end() )
        {
            std::vector<int> pageListCopy;

            pageListCopy.insert( pageListCopy.end(), it->second.begin(), it->second.end() );

            if( !Schematic()->Settings().m_IntersheetRefsListOwnPage )
            {
                int currentPage = Schematic()->CurrentSheet().GetVirtualPageNumber();
                alg::delete_matching( pageListCopy, currentPage );

                if( pageListCopy.empty() )
                    return;
            }

            std::sort( pageListCopy.begin(), pageListCopy.end() );

            std::map<int, wxString> sheetPages = Schematic()->GetVirtualPageToSheetPagesMap();
            std::map<int, wxString> sheetNames = Schematic()->GetVirtualPageToSheetNamesMap();

            for( int pageNum : pageListCopy )
                pages->push_back( { sheetPages[ pageNum ], sheetNames[ pageNum ] } );
        }
    }
}


void SCH_LABEL_BASE::GetContextualTextVars( wxArrayString* aVars ) const
{
    for( const SCH_FIELD& field : m_fields )
        aVars->push_back( field.GetCanonicalName().Upper() );

    aVars->push_back( wxT( "OP" ) );
    aVars->push_back( wxT( "CONNECTION_TYPE" ) );
    aVars->push_back( wxT( "SHORT_NET_NAME" ) );
    aVars->push_back( wxT( "NET_NAME" ) );
    aVars->push_back( wxT( "NET_CLASS" ) );
}


bool SCH_LABEL_BASE::ResolveTextVar( const SCH_SHEET_PATH* aPath, wxString* token,
                                     int aDepth ) const
{
    static wxRegEx operatingPoint( wxT( "^"
                                        "OP"
                                        "(.([0-9])?([a-zA-Z]*))?"
                                        "$" ) );

    wxCHECK( aPath, false );

    SCHEMATIC* schematic = Schematic();

    if( !schematic )
        return false;

    if( operatingPoint.Matches( *token ) )
    {
        int      precision = 3;
        wxString precisionStr( operatingPoint.GetMatch( *token, 2 ) );
        wxString range( operatingPoint.GetMatch( *token, 3 ) );

        if( !precisionStr.IsEmpty() )
            precision = precisionStr[0] - '0';

        if( range.IsEmpty() )
            range = wxS( "~V" );

        const SCH_CONNECTION* connection = Connection();
        *token = wxS( "?" );

        if( connection )
            *token = schematic->GetOperatingPoint( connection->Name( false ), precision, range );

        return true;
    }

    if( token->Contains( ':' ) )
    {
        if( schematic->ResolveCrossReference( token, aDepth + 1 ) )
            return true;
    }

    if( ( Type() == SCH_GLOBAL_LABEL_T || Type() == SCH_HIER_LABEL_T || Type() == SCH_SHEET_PIN_T )
         && token->IsSameAs( wxT( "CONNECTION_TYPE" ) ) )
    {
        const SCH_LABEL_BASE* label = static_cast<const SCH_LABEL_BASE*>( this );
        *token = getElectricalTypeLabel( label->GetShape() );
        return true;
    }
    else if( token->IsSameAs( wxT( "SHORT_NET_NAME" ) ) )
    {
        const SCH_CONNECTION* connection = Connection();
        *token = wxEmptyString;

        if( connection )
            *token = connection->LocalName();

        return true;
    }
    else if( token->IsSameAs( wxT( "NET_NAME" ) ) )
    {
        const SCH_CONNECTION* connection = Connection();
        *token = wxEmptyString;

        if( connection )
            *token = connection->Name();

        return true;
    }
    else if( token->IsSameAs( wxT( "NET_CLASS" ) ) )
    {
        const SCH_CONNECTION* connection = Connection();
        *token = wxEmptyString;

        if( connection )
            *token = GetEffectiveNetClass()->GetName();

        return true;
    }

    for( const SCH_FIELD& field : m_fields)
    {
        if( token->IsSameAs( field.GetName() ) )
        {
            *token = field.GetShownText( false, aDepth + 1 );
            return true;
        }
    }

    // See if parent can resolve it (these will recurse to ancestors)

    if( Type() == SCH_SHEET_PIN_T && m_parent )
    {
        SCH_SHEET* sheet = static_cast<SCH_SHEET*>( m_parent );

        SCH_SHEET_PATH path = *aPath;
        path.push_back( sheet );

        if( sheet->ResolveTextVar( &path, token, aDepth + 1 ) )
            return true;
    }
    else
    {
        if( aPath->Last()->ResolveTextVar( aPath, token, aDepth + 1 ) )
            return true;
    }

    return false;
}


bool SCH_LABEL_BASE::HasCachedDriverName() const
{
    return !HasTextVars();
}


const wxString& SCH_LABEL_BASE::GetCachedDriverName() const
{
    return m_cached_driver_name;
}


void SCH_LABEL_BASE::cacheShownText()
{
    EDA_TEXT::cacheShownText();

    if( !HasTextVars() )
        m_cached_driver_name = EscapeString( EDA_TEXT::GetShownText( true, 0 ), CTX_NETNAME );
}


wxString SCH_LABEL_BASE::GetShownText( const SCH_SHEET_PATH* aPath, bool aAllowExtraText,
                                       int aDepth ) const
{
    std::function<bool( wxString* )> textResolver =
            [&]( wxString* token ) -> bool
            {
                return ResolveTextVar( aPath, token, aDepth + 1 );
            };

    wxString text = EDA_TEXT::GetShownText( aAllowExtraText, aDepth );

    if( text == wxS( "~" ) ) // Legacy placeholder for empty string
    {
        text = wxS( "" );
    }
    else if( HasTextVars() )
    {
        if( aDepth < 10 )
            text = ExpandTextVars( text, &textResolver );
    }

    return text;
}


void SCH_LABEL_BASE::RunOnChildren( const std::function<void( SCH_ITEM* )>& aFunction )
{
    for( SCH_FIELD& field : m_fields )
        aFunction( &field );
}


bool SCH_LABEL_BASE::Matches( const EDA_SEARCH_DATA& aSearchData, void* aAuxData ) const
{
    return SCH_ITEM::Matches( UnescapeString( GetText() ), aSearchData );
}


bool SCH_LABEL_BASE::Replace( const EDA_SEARCH_DATA& aSearchData, void* aAuxData )
{
    EDA_SEARCH_DATA localSearchData( aSearchData );
    localSearchData.findString = EscapeString( aSearchData.findString, CTX_NETNAME );
    localSearchData.replaceString = EscapeString( aSearchData.replaceString, CTX_NETNAME );

    return EDA_TEXT::Replace( localSearchData );
}


INSPECT_RESULT SCH_LABEL_BASE::Visit( INSPECTOR aInspector, void* testData,
                                      const std::vector<KICAD_T>& aScanTypes )
{
    if( IsType( aScanTypes ) )
    {
        if( INSPECT_RESULT::QUIT == aInspector( this, nullptr ) )
            return INSPECT_RESULT::QUIT;
    }

    for( KICAD_T scanType : aScanTypes )
    {
        if( scanType == SCH_LOCATE_ANY_T || scanType == SCH_FIELD_T )
        {
            for( SCH_FIELD& field : m_fields )
            {
                if( INSPECT_RESULT::QUIT == aInspector( &field, this ) )
                    return INSPECT_RESULT::QUIT;
            }
        }
    }

    return INSPECT_RESULT::CONTINUE;
}


void SCH_LABEL_BASE::GetEndPoints( std::vector<DANGLING_END_ITEM>& aItemList )
{
    DANGLING_END_ITEM item( LABEL_END, this, GetTextPos() );
    aItemList.push_back( item );
}


std::vector<VECTOR2I> SCH_LABEL_BASE::GetConnectionPoints() const
{
    return { GetTextPos() };
}


void SCH_LABEL_BASE::ViewGetLayers( int aLayers[], int& aCount ) const
{
    aCount     = 5;
    aLayers[0] = LAYER_DANGLING;
    aLayers[1] = LAYER_DEVICE;
    aLayers[2] = LAYER_NETCLASS_REFS;
    aLayers[3] = LAYER_FIELDS;
    aLayers[4] = LAYER_SELECTION_SHADOWS;
}


int SCH_LABEL_BASE::GetLabelBoxExpansion( const RENDER_SETTINGS* aSettings ) const
{
    double ratio;

    if( aSettings )
        ratio = static_cast<const SCH_RENDER_SETTINGS*>( aSettings )->m_LabelSizeRatio;
    else if( Schematic() )
        ratio = Schematic()->Settings().m_LabelSizeRatio;
    else
        ratio = DEFAULT_LABEL_SIZE_RATIO; // For previews (such as in Preferences), etc.

    return KiROUND( ratio * GetTextSize().y );
}


const BOX2I SCH_LABEL_BASE::GetBodyBoundingBox() const
{
    // build the bounding box of the label only, without taking into account its fields

    BOX2I                 box;
    std::vector<VECTOR2I> pts;

    CreateGraphicShape( nullptr, pts, GetTextPos() );

    for( const VECTOR2I& pt : pts )
        box.Merge( pt );

    box.Inflate( GetEffectiveTextPenWidth() / 2 );
    box.Normalize();
    return box;
}


const BOX2I SCH_LABEL_BASE::GetBoundingBox() const
{
    // build the bounding box of the entire label, including its fields

    BOX2I box = GetBodyBoundingBox();

    for( const SCH_FIELD& field : m_fields )
    {
        if( field.IsVisible() )
        {
            BOX2I fieldBBox = field.GetBoundingBox();

            if( Type() == SCH_LABEL_T || Type() == SCH_GLOBAL_LABEL_T )
                fieldBBox.Offset( GetSchematicTextOffset( nullptr ) );

            box.Merge( fieldBBox );
        }
    }

    box.Normalize();

    return box;
}


bool SCH_LABEL_BASE::HitTest( const VECTOR2I& aPosition, int aAccuracy ) const
{
    BOX2I bbox = GetBodyBoundingBox();
    bbox.Inflate( aAccuracy );

    if( bbox.Contains( aPosition ) )
        return true;

    for( const SCH_FIELD& field : m_fields )
    {
        if( field.IsVisible() )
        {
            BOX2I fieldBBox = field.GetBoundingBox();
            fieldBBox.Inflate( aAccuracy );

            if( Type() == SCH_LABEL_T || Type() == SCH_GLOBAL_LABEL_T )
                fieldBBox.Offset( GetSchematicTextOffset( nullptr ) );

            if( fieldBBox.Contains( aPosition ) )
                return true;
        }
    }

    return false;
}


bool SCH_LABEL_BASE::HitTest( const BOX2I& aRect, bool aContained, int aAccuracy ) const
{
    BOX2I rect = aRect;

    rect.Inflate( aAccuracy );

    if( aContained )
    {
        return rect.Contains( GetBoundingBox() );
    }
    else
    {
        if( rect.Intersects( GetBodyBoundingBox() ) )
            return true;

        for( const SCH_FIELD& field : m_fields )
        {
            if( field.IsVisible() )
            {
                BOX2I fieldBBox = field.GetBoundingBox();

                if( Type() == SCH_LABEL_T || Type() == SCH_GLOBAL_LABEL_T )
                    fieldBBox.Offset( GetSchematicTextOffset( nullptr ) );

                if( rect.Intersects( fieldBBox ) )
                    return true;
            }
        }

        return false;
    }
}


bool SCH_LABEL_BASE::UpdateDanglingState( std::vector<DANGLING_END_ITEM>& aItemListByType,
                                          std::vector<DANGLING_END_ITEM>& aItemListByPos,
                                          const SCH_SHEET_PATH*           aPath )
{
    bool     previousState = m_isDangling;
    VECTOR2I text_pos = GetTextPos();
    m_isDangling = true;
    m_connectionType   = CONNECTION_TYPE::NONE;

    for( auto it = DANGLING_END_ITEM_HELPER::get_lower_pos( aItemListByPos, text_pos );
         it < aItemListByPos.end() && it->GetPosition() == text_pos; it++ )
    {
        DANGLING_END_ITEM& item = *it;

        if( item.GetItem() == this )
            continue;

        switch( item.GetType() )
        {
        case PIN_END:
        case LABEL_END:
        case SHEET_LABEL_END:
        case NO_CONNECT_END:
            if( text_pos == item.GetPosition() )
            {
                m_isDangling = false;

                if( aPath && item.GetType() != PIN_END )
                    AddConnectionTo( *aPath, static_cast<SCH_ITEM*>( item.GetItem() ) );
            }
            break;

        default: break;
        }

        if( !m_isDangling )
            break;
    }

    if( m_isDangling )
    {
        for( auto it = DANGLING_END_ITEM_HELPER::get_lower_type( aItemListByType, BUS_END );
             it < aItemListByType.end() && it->GetType() == BUS_END; it++ )
        {
            DANGLING_END_ITEM& item = *it;
            DANGLING_END_ITEM& nextItem = *( ++it );

            int accuracy = 1; // We have rounding issues with an accuracy of 0

            m_isDangling = !TestSegmentHit( text_pos, item.GetPosition(), nextItem.GetPosition(),
                                            accuracy );

            if( m_isDangling )
                continue;

            m_connectionType = CONNECTION_TYPE::BUS;

            // Add the line to the connected items, since it won't be picked
            // up by a search of intersecting connection points
            if( aPath )
            {
                auto sch_item = static_cast<SCH_ITEM*>( item.GetItem() );
                AddConnectionTo( *aPath, sch_item );
                sch_item->AddConnectionTo( *aPath, this );
            }
            break;
        }

        if( m_isDangling )
        {
            for( auto it = DANGLING_END_ITEM_HELPER::get_lower_type( aItemListByType, WIRE_END );
                 it < aItemListByType.end() && it->GetType() == WIRE_END; it++ )
            {
                DANGLING_END_ITEM& item = *it;
                DANGLING_END_ITEM& nextItem = *( ++it );

                int accuracy = 1; // We have rounding issues with an accuracy of 0

                m_isDangling = !TestSegmentHit( text_pos, item.GetPosition(),
                                                nextItem.GetPosition(), accuracy );

                if( m_isDangling )
                    continue;

                m_connectionType = CONNECTION_TYPE::NET;

                // Add the line to the connected items, since it won't be picked
                // up by a search of intersecting connection points
                if( aPath )
                {
                    auto sch_item = static_cast<SCH_ITEM*>( item.GetItem() );
                    AddConnectionTo( *aPath, sch_item );
                    sch_item->AddConnectionTo( *aPath, this );
                }
                break;
            }
        }
    }

    if( m_isDangling )
        m_connectionType = CONNECTION_TYPE::NONE;

    return previousState != m_isDangling;
}


bool SCH_LABEL_BASE::HasConnectivityChanges( const SCH_ITEM* aItem,
                                             const SCH_SHEET_PATH* aInstance ) const
{
    // Do not compare to ourself.
    if( aItem == this || !IsConnectable() )
        return false;

    const SCH_LABEL_BASE* label = dynamic_cast<const SCH_LABEL_BASE*>( aItem );

    // Don't compare against a different SCH_ITEM.
    wxCHECK( label, false );

    if( GetPosition() != label->GetPosition() )
        return true;

    if( GetShownText( aInstance ) != label->GetShownText( aInstance ) )
        return true;

    std::vector<wxString> netclasses;
    std::vector<wxString> otherNetclasses;

    for( const SCH_FIELD& field : m_fields )
    {
        if( field.GetCanonicalName() == wxT( "Netclass" ) )
            netclasses.push_back( field.GetText() );
    }

    for( const SCH_FIELD& field : label->m_fields )
    {
        if( field.GetCanonicalName() == wxT( "Netclass" ) )
            otherNetclasses.push_back( field.GetText() );
    }

    return netclasses != otherNetclasses;
}


void SCH_LABEL_BASE::GetMsgPanelInfo( EDA_DRAW_FRAME* aFrame, std::vector<MSG_PANEL_ITEM>& aList )
{
    wxString msg;

    switch( Type() )
    {
    case SCH_LABEL_T:           msg = _( "Label" );                  break;
    case SCH_DIRECTIVE_LABEL_T: msg = _( "Directive Label" );        break;
    case SCH_GLOBAL_LABEL_T:    msg = _( "Global Label" );           break;
    case SCH_HIER_LABEL_T:      msg = _( "Hierarchical Label" );     break;
    case SCH_SHEET_PIN_T:       msg = _( "Hierarchical Sheet Pin" ); break;
    default: return;
    }

    // Don't use GetShownText() here; we want to show the user the variable references
    aList.emplace_back( msg, UnescapeString( GetText() ) );

    // Display electrical type if it is relevant
    if( Type() == SCH_GLOBAL_LABEL_T || Type() == SCH_HIER_LABEL_T || Type() == SCH_SHEET_PIN_T )
        aList.emplace_back( _( "Type" ), getElectricalTypeLabel( GetShape() ) );

    aList.emplace_back( _( "Font" ), GetFont() ? GetFont()->GetName() : _( "Default" ) );

    wxString textStyle[] = { _( "Normal" ), _( "Italic" ), _( "Bold" ), _( "Bold Italic" ) };
    int style = IsBold() && IsItalic() ? 3 : IsBold() ? 2 : IsItalic() ? 1 : 0;
    aList.emplace_back( _( "Style" ), textStyle[style] );

    aList.emplace_back( _( "Text Size" ), aFrame->MessageTextFromValue( GetTextWidth() ) );

    switch( GetSpinStyle() )
    {
    case SPIN_STYLE::LEFT:   msg = _( "Align right" );   break;
    case SPIN_STYLE::UP:     msg = _( "Align bottom" );  break;
    case SPIN_STYLE::RIGHT:  msg = _( "Align left" );    break;
    case SPIN_STYLE::BOTTOM: msg = _( "Align top" );     break;
    default:                 msg = wxT( "???" );         break;
    }

    aList.emplace_back( _( "Justification" ), msg );

    SCH_CONNECTION* conn = nullptr;

    if( !IsConnectivityDirty() && dynamic_cast<SCH_EDIT_FRAME*>( aFrame ) )
        conn = Connection();

    if( conn )
    {
        conn->AppendInfoToMsgPanel( aList );

        if( !conn->IsBus() )
        {
            aList.emplace_back( _( "Resolved Netclass" ),
                                UnescapeString( GetEffectiveNetClass()->GetName() ) );
        }
    }
}


void SCH_LABEL_BASE::Plot( PLOTTER* aPlotter, bool aBackground,
                           const SCH_PLOT_SETTINGS& aPlotSettings ) const
{
    static std::vector<VECTOR2I> s_poly;

    RENDER_SETTINGS* settings = aPlotter->RenderSettings();
    SCH_CONNECTION*  connection = Connection();
    int              layer = ( connection && connection->IsBus() ) ? LAYER_BUS : m_layer;
    COLOR4D          color = settings->GetLayerColor( layer );
    int              penWidth = GetEffectiveTextPenWidth( settings->GetDefaultPenWidth() );
    COLOR4D          labelColor = GetLabelColor();

    if( aPlotter->GetColorMode() && labelColor != COLOR4D::UNSPECIFIED )
        color = labelColor;

    penWidth = std::max( penWidth, settings->GetMinPenWidth() );
    aPlotter->SetCurrentLineWidth( penWidth );

    KIFONT::FONT* font = GetFont();

    if( !font )
        font = KIFONT::FONT::GetFont( settings->GetDefaultFont(), IsBold(), IsItalic() );

    VECTOR2I textpos = GetTextPos() + GetSchematicTextOffset( aPlotter->RenderSettings() );
    CreateGraphicShape( aPlotter->RenderSettings(), s_poly, GetTextPos() );

    TEXT_ATTRIBUTES attrs = GetAttributes();
    attrs.m_StrokeWidth = penWidth;
    attrs.m_Multiline = false;

    if( aBackground )
    {
        // No filled shapes (yet)
    }
    else
    {
        aPlotter->PlotText( textpos, color, GetShownText( true ), attrs, font, GetFontMetrics() );

        if( GetShape() == LABEL_FLAG_SHAPE::F_DOT )
        {
            aPlotter->MoveTo( s_poly[0] );
            aPlotter->LineTo( s_poly[1] );
            aPlotter->PenFinish();

            int diameter = ( s_poly[2] - s_poly[1] ).EuclideanNorm() * 2;
            aPlotter->FilledCircle( s_poly[2], diameter , FILLED, nullptr );
        }
        else if( GetShape() == LABEL_FLAG_SHAPE::F_ROUND )
        {
            aPlotter->MoveTo( s_poly[0] );
            aPlotter->LineTo( s_poly[1] );
            aPlotter->PenFinish();

            int diameter = ( s_poly[2] - s_poly[1] ).EuclideanNorm() * 2;
            aPlotter->ThickCircle( s_poly[2], diameter, penWidth, FILLED, nullptr );
        }
        else
        {
            if( !s_poly.empty() )
                aPlotter->PlotPoly( s_poly, FILL_T::NO_FILL, penWidth );
        }

        // Plot attributes to a hypertext menu
        if( aPlotSettings.m_PDFPropertyPopups )
        {
            std::vector<wxString> properties;

            if( connection )
            {
                properties.emplace_back( wxString::Format( wxT( "!%s = %s" ), _( "Net" ),
                                                           connection->Name() ) );

                properties.emplace_back( wxString::Format( wxT( "!%s = %s" ),
                                                           _( "Resolved netclass" ),
                                                           GetEffectiveNetClass()->GetName() ) );
            }

            for( const SCH_FIELD& field : GetFields() )
            {
                properties.emplace_back( wxString::Format( wxT( "!%s = %s" ), field.GetName(),
                                                           field.GetShownText( false ) ) );
            }

            if( !properties.empty() )
                aPlotter->HyperlinkMenu( GetBodyBoundingBox(), properties );
        }

        if( Type() == SCH_HIER_LABEL_T )
        {
            aPlotter->Bookmark( GetBodyBoundingBox(), GetShownText( false ),
                                _( "Hierarchical Labels" ) );
        }
    }

    for( const SCH_FIELD& field : m_fields )
        field.Plot( aPlotter, aBackground, aPlotSettings );
}


void SCH_LABEL_BASE::Print( const RENDER_SETTINGS* aSettings, const VECTOR2I& aOffset )
{
    static std::vector<VECTOR2I> s_poly;

    SCH_CONNECTION* connection = Connection();
    int             layer = ( connection && connection->IsBus() ) ? LAYER_BUS : m_layer;
    wxDC*           DC = aSettings->GetPrintDC();
    COLOR4D         color = aSettings->GetLayerColor( layer );
    bool            blackAndWhiteMode = GetGRForceBlackPenState();
    int             penWidth = std::max( GetPenWidth(), aSettings->GetDefaultPenWidth() );
    VECTOR2I        text_offset = aOffset + GetSchematicTextOffset( aSettings );
    COLOR4D          labelColor = GetLabelColor();

    if( !blackAndWhiteMode && labelColor != COLOR4D::UNSPECIFIED )
        color = labelColor;

    EDA_TEXT::Print( aSettings, text_offset, color );

    CreateGraphicShape( aSettings, s_poly, GetTextPos() + aOffset );

    if( GetShape() == LABEL_FLAG_SHAPE::F_DOT )
    {
        GRLine( DC, s_poly[0], s_poly[1], penWidth, color );

        int radius = ( s_poly[2] - s_poly[1] ).EuclideanNorm();
        GRFilledCircle( DC, s_poly[2], radius, penWidth, color, color );
    }
    else if( GetShape() == LABEL_FLAG_SHAPE::F_ROUND )
    {
        GRLine( DC, s_poly[0], s_poly[1], penWidth, color );

        int radius = ( s_poly[2] - s_poly[1] ).EuclideanNorm();
        GRCircle( DC, s_poly[2], radius, penWidth, color );
    }
    else
    {
        if( !s_poly.empty() )
            GRPoly( DC, s_poly.size(), &s_poly[0], false, penWidth, color, color );
    }

    for( SCH_FIELD& field : m_fields )
        field.Print( aSettings, aOffset );
}


bool SCH_LABEL_BASE::AutoRotateOnPlacement() const
{
    return m_autoRotateOnPlacement;
}


void SCH_LABEL_BASE::SetAutoRotateOnPlacement( bool autoRotate )
{
    m_autoRotateOnPlacement = autoRotate;
}


SCH_LABEL::SCH_LABEL( const VECTOR2I& pos, const wxString& text ) :
        SCH_LABEL_BASE( pos, text, SCH_LABEL_T )
{
    m_layer      = LAYER_LOCLABEL;
    m_shape      = LABEL_FLAG_SHAPE::L_INPUT;
    m_isDangling = true;
}


const BOX2I SCH_LABEL::GetBodyBoundingBox() const
{
    BOX2I rect = GetTextBox();

    rect.Offset( 0, -GetTextOffset() );
    rect.Inflate( GetEffectiveTextPenWidth() );

    if( !GetTextAngle().IsZero() )
    {
        // Rotate rect
        VECTOR2I pos = rect.GetOrigin();
        VECTOR2I end = rect.GetEnd();

        RotatePoint( pos, GetTextPos(), GetTextAngle() );
        RotatePoint( end, GetTextPos(), GetTextAngle() );

        rect.SetOrigin( pos );
        rect.SetEnd( end );

        rect.Normalize();
    }

    // Labels have a position point that is outside of the TextBox
    rect.Merge( GetPosition() );

    return rect;
}


wxString SCH_LABEL::GetItemDescription( UNITS_PROVIDER* aUnitsProvider ) const
{
    return wxString::Format( _( "Label '%s'" ),
                             KIUI::EllipsizeMenuText( GetShownText( false ) ) );
}


BITMAPS SCH_LABEL::GetMenuImage() const
{
    return BITMAPS::add_line_label;
}


SCH_DIRECTIVE_LABEL::SCH_DIRECTIVE_LABEL( const VECTOR2I& pos ) :
        SCH_LABEL_BASE( pos, wxEmptyString, SCH_DIRECTIVE_LABEL_T )
{
    m_layer      = LAYER_NETCLASS_REFS;
    m_shape      = LABEL_FLAG_SHAPE::F_ROUND;
    m_pinLength  = schIUScale.MilsToIU( 100 );
    m_symbolSize = schIUScale.MilsToIU( 20 );
    m_isDangling = true;
}


void SCH_DIRECTIVE_LABEL::SwapData( SCH_ITEM* aItem )
{
    SCH_LABEL_BASE::SwapData( aItem );

    SCH_DIRECTIVE_LABEL* label = static_cast<SCH_DIRECTIVE_LABEL*>( aItem );

    std::swap( m_pinLength, label->m_pinLength );
    std::swap( m_symbolSize, label->m_symbolSize );
}


SCH_DIRECTIVE_LABEL::SCH_DIRECTIVE_LABEL( const SCH_DIRECTIVE_LABEL& aClassLabel ) :
        SCH_LABEL_BASE( aClassLabel )
{
    m_pinLength = aClassLabel.m_pinLength;
    m_symbolSize = aClassLabel.m_symbolSize;
}


int SCH_DIRECTIVE_LABEL::GetPenWidth() const
{
    int pen = 0;

    if( Schematic() )
        pen = Schematic()->Settings().m_DefaultLineWidth;

    return GetEffectiveTextPenWidth( pen );
}


void SCH_DIRECTIVE_LABEL::MirrorSpinStyle( bool aLeftRight )
{
    // The "text" is in fact a graphic shape. For a horizontal "text", it looks like a
    // vertical shape (like a text reduced to only "I" letter).
    // So the mirroring is not exactly similar to a SCH_TEXT item
    SCH_TEXT::MirrorSpinStyle( !aLeftRight );

    for( SCH_FIELD& field : m_fields )
    {
        if( ( aLeftRight && field.GetTextAngle().IsHorizontal() )
                || ( !aLeftRight && field.GetTextAngle().IsVertical() ) )
        {
            if( field.GetHorizJustify() == GR_TEXT_H_ALIGN_LEFT )
                field.SetHorizJustify( GR_TEXT_H_ALIGN_RIGHT );
            else
                field.SetHorizJustify( GR_TEXT_H_ALIGN_LEFT );
        }

        VECTOR2I pos = field.GetTextPos();
        VECTOR2I delta = (VECTOR2I)GetPosition() - pos;

        if( aLeftRight )
            pos.x = GetPosition().x + delta.x;
        else
            pos.y = GetPosition().y + delta.y;

        field.SetTextPos( pos );
    }
}


void SCH_DIRECTIVE_LABEL::MirrorHorizontally( int aCenter )
{
    VECTOR2I old_pos = GetPosition();
    // The "text" is in fact a graphic shape. For a horizontal "text", it looks like a
    // vertical shape (like a text reduced to only "I" letter).
    // So the mirroring is not exactly similar to a SCH_TEXT item
    // Text is NOT really mirrored; it is moved to a suitable horizontal position
    SetSpinStyle( GetSpinStyle().MirrorX() );

    SetTextX( MIRRORVAL( GetTextPos().x, aCenter ) );

    for( SCH_FIELD& field : m_fields )
    {
        switch( field.GetHorizJustify() )
        {
        case GR_TEXT_H_ALIGN_LEFT:
            field.SetHorizJustify( GR_TEXT_H_ALIGN_RIGHT );
            break;

        case GR_TEXT_H_ALIGN_CENTER:
            break;

        case GR_TEXT_H_ALIGN_RIGHT:
            field.SetHorizJustify( GR_TEXT_H_ALIGN_LEFT );
            break;
        }

        VECTOR2I pos = field.GetTextPos();
        VECTOR2I delta = old_pos - pos;
        pos.x = GetPosition().x + delta.x;

        field.SetPosition( pos );
    }
}


void SCH_DIRECTIVE_LABEL::MirrorVertically( int aCenter )
{
    VECTOR2I old_pos = GetPosition();
    // The "text" is in fact a graphic shape. For a horizontal "text", it looks like a
    // vertical shape (like a text reduced to only "I" letter).
    // So the mirroring is not exactly similar to a SCH_TEXT item
    // Text is NOT really mirrored; it is moved to a suitable vertical position
    SetSpinStyle( GetSpinStyle().MirrorY() );

    SetTextY( MIRRORVAL( GetTextPos().y, aCenter ) );

    for( SCH_FIELD& field : m_fields )
    {
        VECTOR2I pos = field.GetTextPos();
        VECTOR2I delta = old_pos - pos;
        pos.y = GetPosition().y + delta.y;

        field.SetPosition( pos );
    }
}


void SCH_DIRECTIVE_LABEL::CreateGraphicShape( const RENDER_SETTINGS* aRenderSettings,
                                              std::vector<VECTOR2I>& aPoints,
                                              const VECTOR2I&        aPos ) const
{
    int symbolSize = m_symbolSize;

    aPoints.clear();

    switch( m_shape )
    {
    case LABEL_FLAG_SHAPE::F_DOT:
        symbolSize = KiROUND( symbolSize * 0.7 );
        KI_FALLTHROUGH;

    case LABEL_FLAG_SHAPE::F_ROUND:
        // First 3 points are used for generating shape
        aPoints.emplace_back( VECTOR2I(             0, 0                        ) );
        aPoints.emplace_back( VECTOR2I(             0, m_pinLength - symbolSize ) );
        aPoints.emplace_back( VECTOR2I(             0, m_pinLength              ) );

        // These points are just used to bulk out the bounding box
        aPoints.emplace_back( VECTOR2I( -m_symbolSize, m_pinLength              ) );
        aPoints.emplace_back( VECTOR2I(             0, m_pinLength              ) );
        aPoints.emplace_back( VECTOR2I(  m_symbolSize, m_pinLength + symbolSize ) );
        break;

    case LABEL_FLAG_SHAPE::F_DIAMOND:
        aPoints.emplace_back( VECTOR2I(                 0, 0                        ) );
        aPoints.emplace_back( VECTOR2I(                 0, m_pinLength - symbolSize ) );
        aPoints.emplace_back( VECTOR2I( -2 * m_symbolSize, m_pinLength              ) );
        aPoints.emplace_back( VECTOR2I(                 0, m_pinLength + symbolSize ) );
        aPoints.emplace_back( VECTOR2I(  2 * m_symbolSize, m_pinLength              ) );
        aPoints.emplace_back( VECTOR2I(                 0, m_pinLength - symbolSize ) );
        aPoints.emplace_back( VECTOR2I(                 0, 0                        ) );
        break;

    case LABEL_FLAG_SHAPE::F_RECTANGLE:
        symbolSize = KiROUND( symbolSize * 0.8 );

        aPoints.emplace_back( VECTOR2I(               0, 0                        ) );
        aPoints.emplace_back( VECTOR2I(               0, m_pinLength - symbolSize ) );
        aPoints.emplace_back( VECTOR2I( -2 * symbolSize, m_pinLength - symbolSize ) );
        aPoints.emplace_back( VECTOR2I( -2 * symbolSize, m_pinLength + symbolSize ) );
        aPoints.emplace_back( VECTOR2I(  2 * symbolSize, m_pinLength + symbolSize ) );
        aPoints.emplace_back( VECTOR2I(  2 * symbolSize, m_pinLength - symbolSize ) );
        aPoints.emplace_back( VECTOR2I(               0, m_pinLength - symbolSize ) );
        aPoints.emplace_back( VECTOR2I(               0, 0                        ) );
        break;

    default:
        break;
    }

    // Rotate outlines and move corners to real position
    for( VECTOR2I& aPoint : aPoints )
    {
        switch( GetSpinStyle() )
        {
        default:
        case SPIN_STYLE::LEFT:                                     break;
        case SPIN_STYLE::UP:     RotatePoint( aPoint, -ANGLE_90 ); break;
        case SPIN_STYLE::RIGHT:  RotatePoint( aPoint, ANGLE_180 ); break;
        case SPIN_STYLE::BOTTOM: RotatePoint( aPoint, ANGLE_90 );  break;
        }

        aPoint += aPos;
    }
}


void SCH_DIRECTIVE_LABEL::AutoplaceFields( SCH_SCREEN* aScreen, bool aManual )
{
    int margin = GetTextOffset();
    int symbolWidth = m_symbolSize;
    int origin = m_pinLength;

    if( m_shape == LABEL_FLAG_SHAPE::F_DIAMOND || m_shape == LABEL_FLAG_SHAPE::F_RECTANGLE )
        symbolWidth *= 2;

    if( IsItalic() )
        margin = KiROUND( margin * 1.5 );

    VECTOR2I offset;

    for( SCH_FIELD& field : m_fields )
    {
        switch( GetSpinStyle() )
        {
        default:
        case SPIN_STYLE::LEFT:
            field.SetTextAngle( ANGLE_HORIZONTAL );
            offset = { symbolWidth + margin, origin };
            break;

        case SPIN_STYLE::UP:
            field.SetTextAngle( ANGLE_VERTICAL );
            offset = { -origin, -( symbolWidth + margin ) };
            break;

        case SPIN_STYLE::RIGHT:
            field.SetTextAngle( ANGLE_HORIZONTAL );
            offset = { symbolWidth + margin, -origin };
            break;

        case SPIN_STYLE::BOTTOM:
            field.SetTextAngle( ANGLE_VERTICAL );
            offset = { origin, -( symbolWidth + margin ) };
            break;
        }

        field.SetHorizJustify( GR_TEXT_H_ALIGN_LEFT );
        field.SetTextPos( GetPosition() + offset );

        origin -= field.GetTextHeight() + margin;
    }

    m_fieldsAutoplaced = FIELDS_AUTOPLACED_AUTO;
}


wxString SCH_DIRECTIVE_LABEL::GetItemDescription( UNITS_PROVIDER* aUnitsProvider ) const
{
    if( m_fields.empty() )
    {
        return _( "Directive Label" );
    }
    else
    {
        return wxString::Format( _( "Directive Label [%s %s]" ),
                                 UnescapeString( m_fields[0].GetName() ),
                                 KIUI::EllipsizeMenuText( m_fields[0].GetShownText( false ) ) );
    }
}


SCH_GLOBALLABEL::SCH_GLOBALLABEL( const VECTOR2I& pos, const wxString& text ) :
        SCH_LABEL_BASE( pos, text, SCH_GLOBAL_LABEL_T )
{
    m_layer      = LAYER_GLOBLABEL;
    m_shape      = LABEL_FLAG_SHAPE::L_BIDI;
    m_isDangling = true;

    SetVertJustify( GR_TEXT_V_ALIGN_CENTER );

    m_fields.emplace_back( SCH_FIELD( pos, 0, this, wxT( "Sheet References" ) ) );
    m_fields[0].SetText( wxT( "${INTERSHEET_REFS}" ) );
    m_fields[0].SetVisible( false );
    m_fields[0].SetLayer( LAYER_INTERSHEET_REFS );
    m_fields[0].SetVertJustify( GR_TEXT_V_ALIGN_CENTER );
}


SCH_GLOBALLABEL::SCH_GLOBALLABEL( const SCH_GLOBALLABEL& aGlobalLabel ) :
        SCH_LABEL_BASE( aGlobalLabel )
{
}


VECTOR2I SCH_GLOBALLABEL::GetSchematicTextOffset( const RENDER_SETTINGS* aSettings ) const
{
    int horiz = GetLabelBoxExpansion( aSettings );

    // Center the text on the center line of "E" instead of "R" to make room for an overbar
    int vert = GetTextHeight() * 0.0715;

    switch( m_shape )
    {
    case LABEL_FLAG_SHAPE::L_INPUT:
    case LABEL_FLAG_SHAPE::L_BIDI:
    case LABEL_FLAG_SHAPE::L_TRISTATE:
        horiz += GetTextHeight() * 3 / 4;  // Use three-quarters-height as proxy for triangle size
        break;

    case LABEL_FLAG_SHAPE::L_OUTPUT:
    case LABEL_FLAG_SHAPE::L_UNSPECIFIED:
    default:
        break;
    }

    switch( GetSpinStyle() )
    {
    default:
    case SPIN_STYLE::LEFT:   return VECTOR2I( -horiz, vert );
    case SPIN_STYLE::UP:     return VECTOR2I( vert, -horiz );
    case SPIN_STYLE::RIGHT:  return VECTOR2I( horiz, vert );
    case SPIN_STYLE::BOTTOM: return VECTOR2I( vert, horiz );
    }
}


void SCH_GLOBALLABEL::SetSpinStyle( SPIN_STYLE aSpinStyle )
{
    SCH_LABEL_BASE::SetSpinStyle( aSpinStyle );
    SetVertJustify( GR_TEXT_V_ALIGN_CENTER );
}


bool SCH_GLOBALLABEL::ResolveTextVar( const SCH_SHEET_PATH* aPath, wxString* token,
                                      int aDepth ) const
{
    wxCHECK( aPath, false );

    SCHEMATIC* schematic = Schematic();

    if( !schematic )
        return false;

    if( token->IsSameAs( wxT( "INTERSHEET_REFS" ) ) )
    {
        SCHEMATIC_SETTINGS& settings = schematic->Settings();
        wxString            ref;
        auto                it = schematic->GetPageRefsMap().find( GetText() );

        if( it == schematic->GetPageRefsMap().end() )
        {
            ref = "?";
        }
        else
        {
            std::vector<int> pageListCopy;

            pageListCopy.insert( pageListCopy.end(), it->second.begin(), it->second.end() );
            std::sort( pageListCopy.begin(), pageListCopy.end() );

            if( !settings.m_IntersheetRefsListOwnPage )
            {
                int currentPage = schematic->CurrentSheet().GetVirtualPageNumber();
                alg::delete_matching( pageListCopy, currentPage );
            }

            std::map<int, wxString> sheetPages = schematic->GetVirtualPageToSheetPagesMap();

            if( ( settings.m_IntersheetRefsFormatShort ) && ( pageListCopy.size() > 2 ) )
            {
                ref.Append( wxString::Format( wxT( "%s..%s" ),
                                              sheetPages[pageListCopy.front()],
                                              sheetPages[pageListCopy.back()] ) );
            }
            else
            {
                for( const int& pageNo : pageListCopy )
                    ref.Append( wxString::Format( wxT( "%s," ), sheetPages[pageNo] ) );

                if( !ref.IsEmpty() && ref.Last() == ',' )
                    ref.RemoveLast();
            }
        }

        *token = settings.m_IntersheetRefsPrefix + ref + settings.m_IntersheetRefsSuffix;
        return true;
    }

    return SCH_LABEL_BASE::ResolveTextVar( aPath, token, aDepth );
}


void SCH_GLOBALLABEL::ViewGetLayers( int aLayers[], int& aCount ) const
{
    aCount     = 6;
    aLayers[0] = LAYER_DANGLING;
    aLayers[1] = LAYER_DEVICE;
    aLayers[2] = LAYER_INTERSHEET_REFS;
    aLayers[3] = LAYER_NETCLASS_REFS;
    aLayers[4] = LAYER_FIELDS;
    aLayers[5] = LAYER_SELECTION_SHADOWS;
}


void SCH_GLOBALLABEL::CreateGraphicShape( const RENDER_SETTINGS* aRenderSettings,
                                          std::vector<VECTOR2I>& aPoints,
                                          const VECTOR2I&        aPos ) const
{
    int margin    = GetLabelBoxExpansion( aRenderSettings );
    int halfSize  = ( GetTextHeight() / 2 ) + margin;
    int linewidth = GetPenWidth();
    int symb_len  = GetTextBox().GetWidth() + 2 * margin;

    int x = symb_len + linewidth + 3;
    int y = halfSize + linewidth + 3;

    aPoints.clear();

    // Create outline shape : 6 points
    aPoints.emplace_back( VECTOR2I( 0, 0 ) );
    aPoints.emplace_back( VECTOR2I( 0, -y ) );    // Up
    aPoints.emplace_back( VECTOR2I( -x, -y ) );   // left
    aPoints.emplace_back( VECTOR2I( -x, 0 ) );    // Up left
    aPoints.emplace_back( VECTOR2I( -x, y ) );    // left down
    aPoints.emplace_back( VECTOR2I( 0, y ) );     // down

    int x_offset = 0;

    switch( m_shape )
    {
    case LABEL_FLAG_SHAPE::L_INPUT:
        x_offset = -halfSize;
        aPoints[0].x += halfSize;
        break;

    case LABEL_FLAG_SHAPE::L_OUTPUT:
        aPoints[3].x -= halfSize;
        break;

    case LABEL_FLAG_SHAPE::L_BIDI:
    case LABEL_FLAG_SHAPE::L_TRISTATE:
        x_offset = -halfSize;
        aPoints[0].x += halfSize;
        aPoints[3].x -= halfSize;
        break;

    case LABEL_FLAG_SHAPE::L_UNSPECIFIED:
    default:
        break;
    }

    // Rotate outlines and move corners in real position
    for( VECTOR2I& aPoint : aPoints )
    {
        aPoint.x += x_offset;

        switch( GetSpinStyle() )
        {
        default:
        case SPIN_STYLE::LEFT:                                     break;
        case SPIN_STYLE::UP:     RotatePoint( aPoint, -ANGLE_90 ); break;
        case SPIN_STYLE::RIGHT:  RotatePoint( aPoint, ANGLE_180 ); break;
        case SPIN_STYLE::BOTTOM: RotatePoint( aPoint, ANGLE_90 );  break;
        }

        aPoint += aPos;
    }

    aPoints.push_back( aPoints[0] ); // closing
}


wxString SCH_GLOBALLABEL::GetItemDescription( UNITS_PROVIDER* aUnitsProvider ) const
{
    return wxString::Format( _( "Global Label '%s'" ),
                             KIUI::EllipsizeMenuText( GetShownText( false ) ) );
}


BITMAPS SCH_GLOBALLABEL::GetMenuImage() const
{
    return BITMAPS::add_glabel;
}


SCH_HIERLABEL::SCH_HIERLABEL( const VECTOR2I& pos, const wxString& text, KICAD_T aType ) :
        SCH_LABEL_BASE( pos, text, aType )
{
    m_layer      = LAYER_HIERLABEL;
    m_shape      = LABEL_FLAG_SHAPE::L_INPUT;
    m_isDangling = true;
}


void SCH_HIERLABEL::SetSpinStyle( SPIN_STYLE aSpinStyle )
{
    SCH_LABEL_BASE::SetSpinStyle( aSpinStyle );
    SetVertJustify( GR_TEXT_V_ALIGN_CENTER );
}


void SCH_HIERLABEL::CreateGraphicShape( const RENDER_SETTINGS* aSettings,
                                        std::vector<VECTOR2I>& aPoints, const VECTOR2I& aPos ) const
{
    CreateGraphicShape( aSettings, aPoints, aPos, m_shape );
}


void SCH_HIERLABEL::CreateGraphicShape( const RENDER_SETTINGS* aSettings,
                                        std::vector<VECTOR2I>& aPoints, const VECTOR2I& aPos,
                                        LABEL_FLAG_SHAPE aShape ) const
{
    int* Template = TemplateShape[static_cast<int>( aShape )][static_cast<int>( GetSpinStyle() )];
    int  halfSize = GetTextHeight() / 2;
    int  imax = *Template;
    Template++;

    aPoints.clear();

    for( int ii = 0; ii < imax; ii++ )
    {
        VECTOR2I corner;
        corner.x = ( halfSize * (*Template) ) + aPos.x;
        Template++;

        corner.y = ( halfSize * (*Template) ) + aPos.y;
        Template++;

        aPoints.push_back( corner );
    }
}


const BOX2I SCH_HIERLABEL::GetBodyBoundingBox() const
{
    int penWidth = GetEffectiveTextPenWidth();
    int margin = GetTextOffset();

    int x  = GetTextPos().x;
    int y  = GetTextPos().y;

    int height = GetTextHeight() + penWidth + margin;
    int length = GetTextBox().GetWidth();

    length += height;       // add height for triangular shapes

    int dx, dy;

    switch( GetSpinStyle() )
    {
    default:
    case SPIN_STYLE::LEFT:
        dx = -length;
        dy = height;
        x += schIUScale.MilsToIU( DANGLING_SYMBOL_SIZE );
        y -= height / 2;
        break;

    case SPIN_STYLE::UP:
        dx = height;
        dy = -length;
        x -= height / 2;
        y += schIUScale.MilsToIU( DANGLING_SYMBOL_SIZE );
        break;

    case SPIN_STYLE::RIGHT:
        dx = length;
        dy = height;
        x -= schIUScale.MilsToIU( DANGLING_SYMBOL_SIZE );
        y -= height / 2;
        break;

    case SPIN_STYLE::BOTTOM:
        dx = height;
        dy = length;
        x -= height / 2;
        y -= schIUScale.MilsToIU( DANGLING_SYMBOL_SIZE );
        break;
    }

    BOX2I box( VECTOR2I( x, y ), VECTOR2I( dx, dy ) );
    box.Normalize();
    return box;
}


VECTOR2I SCH_HIERLABEL::GetSchematicTextOffset( const RENDER_SETTINGS* aSettings ) const
{
    VECTOR2I text_offset;
    int     dist = GetTextOffset( aSettings );

    dist += GetTextWidth();

    switch( GetSpinStyle() )
    {
    default:
    case SPIN_STYLE::LEFT:   text_offset.x = -dist; break; // Orientation horiz normale
    case SPIN_STYLE::UP:     text_offset.y = -dist; break; // Orientation vert UP
    case SPIN_STYLE::RIGHT:  text_offset.x = dist;  break; // Orientation horiz inverse
    case SPIN_STYLE::BOTTOM: text_offset.y = dist;  break; // Orientation vert BOTTOM
    }

    return text_offset;
}


wxString SCH_HIERLABEL::GetItemDescription( UNITS_PROVIDER* aUnitsProvider ) const
{
    return wxString::Format( _( "Hierarchical Label '%s'" ),
                             KIUI::EllipsizeMenuText( GetShownText( false ) ) );
}


BITMAPS SCH_HIERLABEL::GetMenuImage() const
{
    return BITMAPS::add_hierarchical_label;
}


HTML_MESSAGE_BOX* SCH_TEXT::ShowSyntaxHelp( wxWindow* aParentWindow )
{
    wxString msg =
#include "sch_text_help_md.h"
     ;

    HTML_MESSAGE_BOX* dlg = new HTML_MESSAGE_BOX( nullptr, _( "Syntax Help" ) );
    wxSize            sz( 320, 320 );

    dlg->SetMinSize( dlg->ConvertDialogToPixels( sz ) );
    dlg->SetDialogSizeInDU( sz.x, sz.y );

    wxString html_txt;
    ConvertMarkdown2Html( wxGetTranslation( msg ), html_txt );
    dlg->AddHTML_Text( html_txt );
    dlg->ShowModeless();

    return dlg;
}


static struct SCH_LABEL_DESC
{
    SCH_LABEL_DESC()
    {
        auto& labelShapeEnum = ENUM_MAP<LABEL_SHAPE>::Instance();

        if( labelShapeEnum.Choices().GetCount() == 0 )
        {
            labelShapeEnum.Map( LABEL_SHAPE::LABEL_INPUT, _HKI( "Input" ) )
                          .Map( LABEL_SHAPE::LABEL_OUTPUT, _HKI( "Output" ) )
                          .Map( LABEL_SHAPE::LABEL_BIDI, _HKI( "Bidirectional" ) )
                          .Map( LABEL_SHAPE::LABEL_TRISTATE, _HKI( "Tri-state" ) )
                          .Map( LABEL_SHAPE::LABEL_PASSIVE, _HKI( "Passive" ) );
        }

        PROPERTY_MANAGER& propMgr = PROPERTY_MANAGER::Instance();
        REGISTER_TYPE( SCH_LABEL_BASE );
        REGISTER_TYPE( SCH_LABEL );
        REGISTER_TYPE( SCH_HIERLABEL );

        propMgr.AddTypeCast( new TYPE_CAST<SCH_LABEL, SCH_LABEL_BASE> );
        propMgr.AddTypeCast( new TYPE_CAST<SCH_HIERLABEL, SCH_LABEL_BASE> );
        propMgr.AddTypeCast( new TYPE_CAST<SCH_GLOBALLABEL, SCH_LABEL_BASE> );

        propMgr.AddTypeCast( new TYPE_CAST<SCH_LABEL, SCH_TEXT> );
        propMgr.AddTypeCast( new TYPE_CAST<SCH_HIERLABEL, SCH_TEXT> );
        propMgr.AddTypeCast( new TYPE_CAST<SCH_GLOBALLABEL, SCH_TEXT> );

        propMgr.AddTypeCast( new TYPE_CAST<SCH_LABEL, EDA_TEXT> );
        propMgr.AddTypeCast( new TYPE_CAST<SCH_HIERLABEL, EDA_TEXT> );
        propMgr.AddTypeCast( new TYPE_CAST<SCH_GLOBALLABEL, EDA_TEXT> );

        propMgr.InheritsAfter( TYPE_HASH( SCH_LABEL_BASE ), TYPE_HASH( SCH_TEXT ) );
        propMgr.InheritsAfter( TYPE_HASH( SCH_LABEL ), TYPE_HASH( SCH_LABEL_BASE ) );
        propMgr.InheritsAfter( TYPE_HASH( SCH_HIERLABEL ), TYPE_HASH( SCH_LABEL_BASE ) );
        propMgr.InheritsAfter( TYPE_HASH( SCH_GLOBALLABEL ), TYPE_HASH( SCH_LABEL_BASE ) );

        auto hasLabelShape =
                []( INSPECTABLE* aItem ) -> bool
                {
                    if( SCH_LABEL_BASE* label = dynamic_cast<SCH_LABEL_BASE*>( aItem ) )
                        return label->IsType( { SCH_GLOBAL_LABEL_T, SCH_HIER_LABEL_T } );

                    return false;
                };

        propMgr.AddProperty( new PROPERTY_ENUM<SCH_LABEL_BASE, LABEL_SHAPE>( _HKI( "Shape" ),
                             &SCH_LABEL_BASE::SetLabelShape, &SCH_LABEL_BASE::GetLabelShape ) )
                .SetAvailableFunc( hasLabelShape );

        propMgr.Mask( TYPE_HASH( SCH_LABEL_BASE ), TYPE_HASH( EDA_TEXT ), _HKI( "Hyperlink" ) );
    }
} _SCH_LABEL_DESC;


static struct SCH_DIRECTIVE_LABEL_DESC
{
    SCH_DIRECTIVE_LABEL_DESC()
    {
        auto& flagShapeEnum = ENUM_MAP<FLAG_SHAPE>::Instance();

        if( flagShapeEnum.Choices().GetCount() == 0 )
        {
            flagShapeEnum.Map( FLAG_SHAPE::FLAG_DOT, _HKI( "Dot" ) )
                         .Map( FLAG_SHAPE::FLAG_CIRCLE, _HKI( "Circle" ) )
                         .Map( FLAG_SHAPE::FLAG_DIAMOND, _HKI( "Diamond" ) )
                         .Map( FLAG_SHAPE::FLAG_RECTANGLE, _HKI( "Rectangle" ) );
        }

        PROPERTY_MANAGER& propMgr = PROPERTY_MANAGER::Instance();
        REGISTER_TYPE( SCH_DIRECTIVE_LABEL );
        propMgr.AddTypeCast( new TYPE_CAST<SCH_DIRECTIVE_LABEL, SCH_LABEL_BASE> );
        propMgr.AddTypeCast( new TYPE_CAST<SCH_DIRECTIVE_LABEL, SCH_TEXT> );
        propMgr.AddTypeCast( new TYPE_CAST<SCH_DIRECTIVE_LABEL, EDA_TEXT> );

        propMgr.InheritsAfter( TYPE_HASH( SCH_DIRECTIVE_LABEL ), TYPE_HASH( SCH_LABEL_BASE ) );

        propMgr.AddProperty( new PROPERTY_ENUM<SCH_DIRECTIVE_LABEL, FLAG_SHAPE>( _HKI( "Shape" ),
                             &SCH_DIRECTIVE_LABEL::SetFlagShape, &SCH_DIRECTIVE_LABEL::GetFlagShape ) );

        propMgr.AddProperty( new PROPERTY<SCH_DIRECTIVE_LABEL, int>( _HKI( "Pin length" ),
                             &SCH_DIRECTIVE_LABEL::SetPinLength, &SCH_DIRECTIVE_LABEL::GetPinLength,
                             PROPERTY_DISPLAY::PT_SIZE ) );

        propMgr.Mask( TYPE_HASH( SCH_DIRECTIVE_LABEL ), TYPE_HASH( EDA_TEXT ), _HKI( "Text" ) );
        propMgr.Mask( TYPE_HASH( SCH_DIRECTIVE_LABEL ), TYPE_HASH( EDA_TEXT ), _HKI( "Thickness" ) );
        propMgr.Mask( TYPE_HASH( SCH_DIRECTIVE_LABEL ), TYPE_HASH( EDA_TEXT ), _HKI( "Italic" ) );
        propMgr.Mask( TYPE_HASH( SCH_DIRECTIVE_LABEL ), TYPE_HASH( EDA_TEXT ), _HKI( "Bold" ) );
        propMgr.Mask( TYPE_HASH( SCH_DIRECTIVE_LABEL ), TYPE_HASH( EDA_TEXT ),
                      _HKI( "Horizontal Justification" ) );
        propMgr.Mask( TYPE_HASH( SCH_DIRECTIVE_LABEL ), TYPE_HASH( EDA_TEXT ),
                      _HKI( "Vertical Justification" ) );
    }
} _SCH_DIRECTIVE_LABEL_DESC;


ENUM_TO_WXANY( LABEL_SHAPE )
ENUM_TO_WXANY( FLAG_SHAPE )
