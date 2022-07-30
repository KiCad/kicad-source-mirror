/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2016 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 2015 Wayne Stambaugh <stambaughw@gmail.com>
 * Copyright (C) 1992-2022 KiCad Developers, see AUTHORS.txt for contributors.
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

#include <pgm_base.h>
#include <sch_edit_frame.h>
#include <plotters/plotter.h>
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
            outputFormat.Printf( "%%0%dd", dCount );
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


SCH_LABEL_BASE::SCH_LABEL_BASE( const VECTOR2I& aPos, const wxString& aText, KICAD_T aType ) :
        SCH_TEXT( aPos, aText, aType ),
        m_shape( L_UNSPECIFIED ),
        m_connectionType( CONNECTION_TYPE::NONE ),
        m_isDangling( true )
{
    SetMultilineAllowed( false );
    ClearFieldsAutoplaced();    // fiels are not yet autoplaced.
}


SCH_LABEL_BASE::SCH_LABEL_BASE( const SCH_LABEL_BASE& aLabel ) :
        SCH_TEXT( aLabel ),
        m_shape( aLabel.m_shape ),
        m_connectionType( aLabel.m_connectionType ),
        m_isDangling( aLabel.m_isDangling )
{
    SetMultilineAllowed( false );

    m_fields = aLabel.m_fields;

    for( SCH_FIELD& field : m_fields )
        field.SetParent( this );
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


bool SCH_LABEL_BASE::IsType( const KICAD_T aScanTypes[] ) const
{
    static KICAD_T wireTypes[] = { SCH_ITEM_LOCATE_WIRE_T, SCH_PIN_T, EOT };
    static KICAD_T busTypes[] = { SCH_ITEM_LOCATE_BUS_T, EOT };

    if( SCH_TEXT::IsType( aScanTypes ) )
        return true;

    for( const KICAD_T* p = aScanTypes; *p != EOT; ++p )
    {
        if( *p == SCH_LABEL_LOCATE_ANY_T )
            return true;
    }

    wxCHECK_MSG( Schematic(), false, wxT( "No parent SCHEMATIC set for SCH_LABEL!" ) );

    // Ensure m_connected_items for Schematic()->CurrentSheet() exists.
    // Can be not the case when "this" is living in clipboard
    if( m_connected_items.find( Schematic()->CurrentSheet() ) == m_connected_items.end() )
        return false;

    const SCH_ITEM_SET& item_set = m_connected_items.at( Schematic()->CurrentSheet() );

    for( const KICAD_T* p = aScanTypes; *p != EOT; ++p )
    {
        if( *p == SCH_LABEL_LOCATE_WIRE_T )
        {
            for( SCH_ITEM* connection : item_set )
            {
                if( connection->IsType( wireTypes ) )
                    return true;
            }
        }

        if ( *p == SCH_LABEL_LOCATE_BUS_T )
        {
            for( SCH_ITEM* connection : item_set )
            {
                if( connection->IsType( busTypes ) )
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
}


VECTOR2I SCH_LABEL_BASE::GetSchematicTextOffset( const RENDER_SETTINGS* aSettings ) const
{
    VECTOR2I text_offset;

    // add an offset to x (or y) position to aid readability of text on a wire or line
    int dist = GetTextOffset( aSettings ) + GetPenWidth();

    switch( GetTextSpinStyle() )
    {
    case TEXT_SPIN_STYLE::UP:
    case TEXT_SPIN_STYLE::BOTTOM: text_offset.x = -dist;  break; // Vert Orientation
    default:
    case TEXT_SPIN_STYLE::LEFT:
    case TEXT_SPIN_STYLE::RIGHT: text_offset.y = -dist;  break; // Horiz Orientation
    }

    return text_offset;
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


bool SCH_LABEL_BASE::IncrementLabel( int aIncrement )
{
    wxString text = GetText();
    bool ReturnVal = IncrementLabelMember( text, aIncrement );

    if( ReturnVal )
        SetText( text );

    return ReturnVal;
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

        switch( GetTextSpinStyle() )
        {
        default:
        case TEXT_SPIN_STYLE::LEFT:
            field.SetTextAngle( ANGLE_HORIZONTAL );
            field.SetHorizJustify( GR_TEXT_H_ALIGN_RIGHT );

            if( Type() == SCH_GLOBAL_LABEL_T && field.GetId() == 0 )
                offset.x = - ( labelLen + margin );
            else
                offset.y = accumulated + field.GetTextHeight() / 2;

            break;

        case TEXT_SPIN_STYLE::UP:
            field.SetTextAngle( ANGLE_VERTICAL );
            field.SetHorizJustify( GR_TEXT_H_ALIGN_LEFT );

            if( Type() == SCH_GLOBAL_LABEL_T && field.GetId() == 0 )
                offset.y = - ( labelLen + margin );
            else
                offset.x = accumulated + field.GetTextHeight() / 2;

            break;

        case TEXT_SPIN_STYLE::RIGHT:
            field.SetTextAngle( ANGLE_HORIZONTAL );
            field.SetHorizJustify( GR_TEXT_H_ALIGN_LEFT );

            if( Type() == SCH_GLOBAL_LABEL_T && field.GetId() == 0 )
                offset.x = labelLen + margin;
            else
                offset.y = accumulated + field.GetTextHeight() / 2;

            break;

        case TEXT_SPIN_STYLE::BOTTOM:
            field.SetTextAngle( ANGLE_VERTICAL );
            field.SetHorizJustify( GR_TEXT_H_ALIGN_RIGHT );

            if( Type() == SCH_GLOBAL_LABEL_T && field.GetId() == 0 )
                offset.y = labelLen + margin;
            else
                offset.x = accumulated + field.GetTextHeight() / 2;

            break;
        }

        field.SetTextPos( GetTextPos() + offset );

        if( Type() != SCH_GLOBAL_LABEL_T || field.GetId() > 0 )
            accumulated += field.GetTextHeight() + margin;
    }

    m_fieldsAutoplaced = FIELDS_AUTOPLACED_AUTO;
}


bool SCH_LABEL_BASE::ResolveTextVar( wxString* token, int aDepth ) const
{
    if( token->Contains( ':' ) )
    {
        if( Schematic()->ResolveCrossReference( token, aDepth ) )
            return true;
    }

    if( ( Type() == SCH_GLOBAL_LABEL_T || Type() == SCH_HIER_LABEL_T || Type() == SCH_SHEET_PIN_T )
         && token->IsSameAs( wxT( "CONNECTION_TYPE" ) ) )
    {
        const SCH_LABEL_BASE* label = static_cast<const SCH_LABEL_BASE*>( this );
        *token = getElectricalTypeLabel( label->GetShape() );
        return true;
    }

    for( size_t i = 0; i < m_fields.size(); ++i )
    {
        if( token->IsSameAs( m_fields[i].GetName() ) )
        {
            *token = m_fields[i].GetShownText( aDepth + 1 );
            return true;
        }
    }

    if( Type() == SCH_SHEET_PIN_T && m_parent )
    {
        SCH_SHEET* sheet = static_cast<SCH_SHEET*>( m_parent );

        if( sheet->ResolveTextVar( token, aDepth ) )
            return true;
    }

    return false;
}


wxString SCH_LABEL_BASE::GetShownText( int aDepth ) const
{
    std::function<bool( wxString* )> textResolver =
            [&]( wxString* token ) -> bool
            {
                return ResolveTextVar( token, aDepth );
            };

    std::function<bool( wxString* )> schematicTextResolver =
            [&]( wxString* token ) -> bool
            {
                return Schematic()->ResolveTextVar( token, aDepth + 1 );
            };

    wxString text = EDA_TEXT::GetShownText();

    if( text == "~" )   // Legacy placeholder for empty string
    {
        text = "";
    }
    else if( HasTextVars() )
    {
        PROJECT* project = nullptr;

        if( Schematic() )
            project = &Schematic()->Prj();

        if( aDepth < 10 )
            text = ExpandTextVars( text, &textResolver, &schematicTextResolver, project );
    }

    return text;
}


void SCH_LABEL_BASE::RunOnChildren( const std::function<void( SCH_ITEM* )>& aFunction )
{
    for( SCH_FIELD& field : m_fields )
        aFunction( &field );
}


INSPECT_RESULT SCH_LABEL_BASE::Visit( INSPECTOR aInspector, void* testData,
                                     const KICAD_T aFilterTypes[] )
{
    KICAD_T stype;

    if( IsType( aFilterTypes ) )
    {
        if( INSPECT_RESULT::QUIT == aInspector( this, nullptr ) )
            return INSPECT_RESULT::QUIT;
    }

    for( const KICAD_T* p = aFilterTypes; (stype = *p) != EOT; ++p )
    {
        if( stype == SCH_LOCATE_ANY_T || stype == SCH_FIELD_T )
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


const EDA_RECT SCH_LABEL_BASE::GetBodyBoundingBox() const
{
    // build the bounding box of the label only, without taking into account its fields

    EDA_RECT             box;
    std::vector<VECTOR2I> pts;

    CreateGraphicShape( nullptr, pts, GetTextPos() );

    for( const VECTOR2I& pt : pts )
        box.Merge( pt );

    box.Inflate( GetEffectiveTextPenWidth() / 2 );
    box.Normalize();
    return box;
}


const EDA_RECT SCH_LABEL_BASE::GetBoundingBox() const
{
    // build the bounding box of the entire label, including its fields

    EDA_RECT box( GetBodyBoundingBox() );

    for( const SCH_FIELD& field : m_fields )
    {
        if( field.IsVisible() )
        {
            EDA_RECT fieldBBox = field.GetBoundingBox();

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
    EDA_RECT bbox = GetBodyBoundingBox();
    bbox.Inflate( aAccuracy );

    if( bbox.Contains( aPosition ) )
        return true;

    for( const SCH_FIELD& field : m_fields )
    {
        if( field.IsVisible() )
        {
            EDA_RECT fieldBBox = field.GetBoundingBox();
            fieldBBox.Inflate( aAccuracy );

            if( Type() == SCH_LABEL_T || Type() == SCH_GLOBAL_LABEL_T )
                fieldBBox.Offset( GetSchematicTextOffset( nullptr ) );

            if( fieldBBox.Contains( aPosition ) )
                return true;
        }
    }

    return false;
}


bool SCH_LABEL_BASE::HitTest( const EDA_RECT& aRect, bool aContained, int aAccuracy ) const
{
    EDA_RECT rect = aRect;

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
                EDA_RECT fieldBBox = field.GetBoundingBox();

                if( Type() == SCH_LABEL_T || Type() == SCH_GLOBAL_LABEL_T )
                    fieldBBox.Offset( GetSchematicTextOffset( nullptr ) );

                if( rect.Intersects( fieldBBox ) )
                    return true;
            }
        }

        return false;
    }
}


bool SCH_LABEL_BASE::UpdateDanglingState( std::vector<DANGLING_END_ITEM>& aItemList,
                                          const SCH_SHEET_PATH* aPath )
{
    bool previousState = m_isDangling;
    m_isDangling       = true;
    m_connectionType   = CONNECTION_TYPE::NONE;

    for( unsigned ii = 0; ii < aItemList.size(); ii++ )
    {
        DANGLING_END_ITEM& item = aItemList[ii];

        if( item.GetItem() == this )
            continue;

        switch( item.GetType() )
        {
        case PIN_END:
        case LABEL_END:
        case SHEET_LABEL_END:
        case NO_CONNECT_END:
            if( GetTextPos() == item.GetPosition() )
            {
                m_isDangling = false;

                if( aPath && item.GetType() != PIN_END )
                    AddConnectionTo( *aPath, static_cast<SCH_ITEM*>( item.GetItem() ) );
            }

            break;

        case BUS_END:
            m_connectionType = CONNECTION_TYPE::BUS;
            KI_FALLTHROUGH;

        case WIRE_END:
        {
            DANGLING_END_ITEM& nextItem = aItemList[++ii];

            int accuracy = 1;   // We have rounding issues with an accuracy of 0

            m_isDangling = !TestSegmentHit( GetTextPos(), item.GetPosition(),
                                            nextItem.GetPosition(), accuracy );

            if( !m_isDangling )
            {
                if( m_connectionType != CONNECTION_TYPE::BUS )
                    m_connectionType = CONNECTION_TYPE::NET;

                // Add the line to the connected items, since it won't be picked
                // up by a search of intersecting connection points
                if( aPath )
                {
                    auto sch_item = static_cast<SCH_ITEM*>( item.GetItem() );
                    AddConnectionTo( *aPath, sch_item );
                    sch_item->AddConnectionTo( *aPath, this );
                }
            }
        }
            break;

        default:
            break;
        }

        if( !m_isDangling )
            break;
    }

    if( m_isDangling )
        m_connectionType = CONNECTION_TYPE::NONE;

    return previousState != m_isDangling;
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

    aList.emplace_back( _( "Font" ), GetDrawFont()->GetName() );

    wxString textStyle[] = { _( "Normal" ), _( "Italic" ), _( "Bold" ), _( "Bold Italic" ) };
    int style = IsBold() && IsItalic() ? 3 : IsBold() ? 2 : IsItalic() ? 1 : 0;
    aList.emplace_back( _( "Style" ), textStyle[style] );

    aList.emplace_back( _( "Text Size" ), MessageTextFromValue( aFrame->GetUserUnits(),
                                                                GetTextWidth() ) );

    switch( GetTextSpinStyle() )
    {
    case TEXT_SPIN_STYLE::LEFT:   msg = _( "Align right" );   break;
    case TEXT_SPIN_STYLE::UP:     msg = _( "Align bottom" );  break;
    case TEXT_SPIN_STYLE::RIGHT:  msg = _( "Align left" );    break;
    case TEXT_SPIN_STYLE::BOTTOM: msg = _( "Align top" );     break;
    default:                      msg = wxT( "???" );         break;
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
            NET_SETTINGS& netSettings = Schematic()->Prj().GetProjectFile().NetSettings();
            const wxString& netname = conn->Name( true );

            if( netSettings.m_NetClassAssignments.count( netname ) )
            {
                const wxString& netclassName = netSettings.m_NetClassAssignments[ netname ];
                aList.emplace_back( _( "Assigned Netclass" ), netclassName );
            }
        }
    }
}


void SCH_LABEL_BASE::Plot( PLOTTER* aPlotter, bool aBackground ) const
{
    static std::vector<VECTOR2I> s_poly;

    RENDER_SETTINGS* settings = aPlotter->RenderSettings();
    SCH_CONNECTION*  connection = Connection();
    int              layer = ( connection && connection->IsBus() ) ? LAYER_BUS : m_layer;
    COLOR4D          color = settings->GetLayerColor( layer );
    int              penWidth = GetEffectiveTextPenWidth( settings->GetDefaultPenWidth() );

    if( aPlotter->GetColorMode() && GetTextColor() != COLOR4D::UNSPECIFIED )
        color = GetTextColor();

    penWidth = std::max( penWidth, settings->GetMinPenWidth() );
    aPlotter->SetCurrentLineWidth( penWidth );

    VECTOR2I textpos = GetTextPos() + GetSchematicTextOffset( aPlotter->RenderSettings() );
    CreateGraphicShape( aPlotter->RenderSettings(), s_poly, GetTextPos() );

    if( aBackground )
    {
        // No filled shapes (yet)
    }
    else
    {
        aPlotter->Text( textpos, color, GetShownText(), GetTextAngle(), GetTextSize(),
                        GetHorizJustify(), GetVertJustify(), penWidth, IsItalic(), IsBold(),
                        false, GetDrawFont() );

        if( s_poly.size() )
            aPlotter->PlotPoly( s_poly, FILL_T::NO_FILL, penWidth );
    }

    for( const SCH_FIELD& field : m_fields )
        field.Plot( aPlotter, aBackground );
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

    if( !blackAndWhiteMode && GetTextColor() != COLOR4D::UNSPECIFIED )
        color = GetTextColor();

    EDA_TEXT::Print( aSettings, text_offset, color );

    CreateGraphicShape( aSettings, s_poly, GetTextPos() + aOffset );

    if( !s_poly.empty() )
        GRPoly( DC, s_poly.size(), &s_poly[0], false, penWidth, color, color );

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


const EDA_RECT SCH_LABEL::GetBodyBoundingBox() const
{
    EDA_RECT rect = GetTextBox();

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


wxString SCH_LABEL::GetSelectMenuText( EDA_UNITS aUnits ) const
{
    return wxString::Format( _( "Label '%s'" ), ShortenedShownText() );
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
    m_pinLength  = Mils2iu( 100 );
    m_symbolSize = Mils2iu( 20 );
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
        switch( GetTextSpinStyle() )
        {
        default:
        case TEXT_SPIN_STYLE::LEFT:                                     break;
        case TEXT_SPIN_STYLE::UP:     RotatePoint( aPoint, -ANGLE_90 ); break;
        case TEXT_SPIN_STYLE::RIGHT:  RotatePoint( aPoint, ANGLE_180 ); break;
        case TEXT_SPIN_STYLE::BOTTOM: RotatePoint( aPoint, ANGLE_90 );  break;
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
        switch( GetTextSpinStyle() )
        {
        default:
        case TEXT_SPIN_STYLE::LEFT:
            field.SetTextAngle( ANGLE_HORIZONTAL );
            offset = { symbolWidth + margin, origin };
            break;

        case TEXT_SPIN_STYLE::UP:
            field.SetTextAngle( ANGLE_VERTICAL );
            offset = { -origin, -( symbolWidth + margin ) };
            break;

        case TEXT_SPIN_STYLE::RIGHT:
            field.SetTextAngle( ANGLE_HORIZONTAL );
            offset = { symbolWidth + margin, -origin };
            break;

        case TEXT_SPIN_STYLE::BOTTOM:
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


wxString SCH_DIRECTIVE_LABEL::GetSelectMenuText( EDA_UNITS aUnits ) const
{
    return wxString::Format( _( "Directive Label" ), ShortenedShownText() );
}


SCH_GLOBALLABEL::SCH_GLOBALLABEL( const VECTOR2I& pos, const wxString& text ) :
        SCH_LABEL_BASE( pos, text, SCH_GLOBAL_LABEL_T )
{
    m_layer      = LAYER_GLOBLABEL;
    m_shape      = LABEL_FLAG_SHAPE::L_BIDI;
    m_isDangling = true;

    SetVertJustify( GR_TEXT_V_ALIGN_CENTER );

    m_fields.emplace_back( SCH_FIELD( { 0, 0 }, 0, this, _( "Sheet References" ) ) );
    m_fields[0].SetText( wxT( "${INTERSHEET_REFS}" ) );
    m_fields[0].SetVisible( true );
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

    switch( GetTextSpinStyle() )
    {
    default:
    case TEXT_SPIN_STYLE::LEFT:   return VECTOR2I( -horiz, vert );
    case TEXT_SPIN_STYLE::UP:     return VECTOR2I( vert, -horiz );
    case TEXT_SPIN_STYLE::RIGHT:  return VECTOR2I( horiz, vert );
    case TEXT_SPIN_STYLE::BOTTOM: return VECTOR2I( vert, horiz );
    }
}


void SCH_GLOBALLABEL::SetTextSpinStyle( TEXT_SPIN_STYLE aSpinStyle )
{
    SCH_TEXT::SetTextSpinStyle( aSpinStyle );
    SetVertJustify( GR_TEXT_V_ALIGN_CENTER );
}


void SCH_GLOBALLABEL::MirrorSpinStyle( bool aLeftRight )
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


void SCH_GLOBALLABEL::MirrorHorizontally( int aCenter )
{
    VECTOR2I old_pos = GetPosition();
    SCH_TEXT::MirrorHorizontally( aCenter );

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


void SCH_GLOBALLABEL::MirrorVertically( int aCenter )
{
    VECTOR2I old_pos = GetPosition();
    SCH_TEXT::MirrorVertically( aCenter );

    for( SCH_FIELD& field : m_fields )
    {
        VECTOR2I pos = field.GetTextPos();
        VECTOR2I delta = old_pos - pos;
        pos.y = GetPosition().y + delta.y;

        field.SetPosition( pos );
    }
}


bool SCH_GLOBALLABEL::ResolveTextVar( wxString* token, int aDepth ) const
{
    if( token->IsSameAs( wxT( "INTERSHEET_REFS" ) ) && Schematic() )
    {
        SCHEMATIC_SETTINGS& settings = Schematic()->Settings();
        wxString            ref;
        auto                it = Schematic()->GetPageRefsMap().find( GetText() );

        if( it == Schematic()->GetPageRefsMap().end() )
        {
            ref = "?";
        }
        else
        {
            std::vector<wxString> pageListCopy;

            pageListCopy.insert( pageListCopy.end(), it->second.begin(), it->second.end() );
            std::sort( pageListCopy.begin(), pageListCopy.end(),
                       []( const wxString& a, const wxString& b ) -> bool
                       {
                           return StrNumCmp( a, b, true ) < 0;
                       } );

            if( !settings.m_IntersheetRefsListOwnPage )
            {
                wxString currentPage = Schematic()->CurrentSheet().GetPageNumber();
                alg::delete_matching( pageListCopy, currentPage );
            }

            if( ( settings.m_IntersheetRefsFormatShort ) && ( pageListCopy.size() > 2 ) )
            {
                ref.Append( wxString::Format( wxT( "%s..%s" ),
                                              pageListCopy.front(),
                                              pageListCopy.back() ) );
            }
            else
            {
                for( const wxString& pageNo : pageListCopy )
                    ref.Append( wxString::Format( wxT( "%s," ), pageNo ) );

                if( !ref.IsEmpty() && ref.Last() == ',' )
                    ref.RemoveLast();
            }
        }

        *token = settings.m_IntersheetRefsPrefix + ref + settings.m_IntersheetRefsSuffix;
        return true;
    }

    return SCH_LABEL_BASE::ResolveTextVar( token, aDepth );
}


void SCH_GLOBALLABEL::ViewGetLayers( int aLayers[], int& aCount ) const
{
    aCount     = 5;
    aLayers[0] = LAYER_DEVICE;
    aLayers[1] = LAYER_INTERSHEET_REFS;
    aLayers[2] = LAYER_NETCLASS_REFS;
    aLayers[3] = LAYER_FIELDS;
    aLayers[4] = LAYER_SELECTION_SHADOWS;
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

        switch( GetTextSpinStyle() )
        {
        default:
        case TEXT_SPIN_STYLE::LEFT:                                     break;
        case TEXT_SPIN_STYLE::UP:     RotatePoint( aPoint, -ANGLE_90 ); break;
        case TEXT_SPIN_STYLE::RIGHT:  RotatePoint( aPoint, ANGLE_180 ); break;
        case TEXT_SPIN_STYLE::BOTTOM: RotatePoint( aPoint, ANGLE_90 );  break;
        }

        aPoint += aPos;
    }

    aPoints.push_back( aPoints[0] ); // closing
}


wxString SCH_GLOBALLABEL::GetSelectMenuText( EDA_UNITS aUnits ) const
{
    return wxString::Format( _( "Global Label '%s'" ), ShortenedShownText() );
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


void SCH_HIERLABEL::SetTextSpinStyle( TEXT_SPIN_STYLE aSpinStyle )
{
    SCH_TEXT::SetTextSpinStyle( aSpinStyle );
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
    int* Template = TemplateShape[static_cast<int>( aShape )][static_cast<int>( m_spin_style )];
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


const EDA_RECT SCH_HIERLABEL::GetBodyBoundingBox() const
{
    int penWidth = GetEffectiveTextPenWidth();
    int margin = GetTextOffset();

    int x  = GetTextPos().x;
    int y  = GetTextPos().y;

    int height = GetTextHeight() + penWidth + margin;
    int length = GetTextBox().GetWidth();

    length += height;       // add height for triangular shapes

    int dx, dy;

    switch( GetTextSpinStyle() )
    {
    default:
    case TEXT_SPIN_STYLE::LEFT:
        dx = -length;
        dy = height;
        x += Mils2iu( DANGLING_SYMBOL_SIZE );
        y -= height / 2;
        break;

    case TEXT_SPIN_STYLE::UP:
        dx = height;
        dy = -length;
        x -= height / 2;
        y += Mils2iu( DANGLING_SYMBOL_SIZE );
        break;

    case TEXT_SPIN_STYLE::RIGHT:
        dx = length;
        dy = height;
        x -= Mils2iu( DANGLING_SYMBOL_SIZE );
        y -= height / 2;
        break;

    case TEXT_SPIN_STYLE::BOTTOM:
        dx = height;
        dy = length;
        x -= height / 2;
        y -= Mils2iu( DANGLING_SYMBOL_SIZE );
        break;
    }

    EDA_RECT box( VECTOR2I( x, y ), VECTOR2I( dx, dy ) );
    box.Normalize();
    return box;
}


VECTOR2I SCH_HIERLABEL::GetSchematicTextOffset( const RENDER_SETTINGS* aSettings ) const
{
    VECTOR2I text_offset;
    int     dist = GetTextOffset( aSettings );

    dist += GetTextWidth();

    switch( GetTextSpinStyle() )
    {
    default:
    case TEXT_SPIN_STYLE::LEFT:   text_offset.x = -dist; break; // Orientation horiz normale
    case TEXT_SPIN_STYLE::UP:     text_offset.y = -dist; break; // Orientation vert UP
    case TEXT_SPIN_STYLE::RIGHT:  text_offset.x = dist;  break; // Orientation horiz inverse
    case TEXT_SPIN_STYLE::BOTTOM: text_offset.y = dist;  break; // Orientation vert BOTTOM
    }

    return text_offset;
}


wxString SCH_HIERLABEL::GetSelectMenuText( EDA_UNITS aUnits ) const
{
    return wxString::Format( _( "Hierarchical Label '%s'" ), ShortenedShownText() );
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
