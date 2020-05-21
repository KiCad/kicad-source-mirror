/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2016 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 2015 Wayne Stambaugh <stambaughw@verizon.net>
 * Copyright (C) 1992-2020 KiCad Developers, see AUTHORS.txt for contributors.
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
 * @file sch_text.cpp
 * @brief Code for handling schematic texts (texts, labels, hlabels and global labels).
 */

#include <fctsys.h>
#include <gr_basic.h>
#include <macros.h>
#include <trigo.h>
#include <sch_component.h>
#include <sch_edit_frame.h>
#include <plotter.h>
#include <msgpanel.h>
#include <gal/stroke_font.h>
#include <bitmaps.h>
#include <math/util.h>      // for KiROUND
#include <kiway.h>
#include <sch_text.h>
#include <schematic.h>
#include <netlist_object.h>
#include <settings/color_settings.h>
#include <sch_painter.h>
#include <default_values.h>
#include <wx/debug.h>
#include <html_messagebox.h>

using KIGFX::SCH_RENDER_SETTINGS;


extern void IncrementLabelMember( wxString& name, int aIncrement );

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


SCH_TEXT::SCH_TEXT( const wxPoint& pos, const wxString& text, KICAD_T aType ) :
        SCH_ITEM( NULL, aType ),
        EDA_TEXT( text ),
        m_shape( PINSHEETLABEL_SHAPE::PS_INPUT ),
        m_isDangling( false ),
        m_connectionType( CONNECTION_TYPE::NONE ),
        m_spin_style( LABEL_SPIN_STYLE::LEFT )
{
    m_Layer = LAYER_NOTES;

    SetTextPos( pos );
    SetMultilineAllowed( true );
}


SCH_TEXT::SCH_TEXT( const SCH_TEXT& aText ) :
        SCH_ITEM( aText ),
        EDA_TEXT( aText ),
        m_shape( aText.m_shape ),
        m_isDangling( aText.m_isDangling ),
        m_connectionType( aText.m_connectionType ),
        m_spin_style( aText.m_spin_style )
{ }


EDA_ITEM* SCH_TEXT::Clone() const
{
    return new SCH_TEXT( *this );
}


void SCH_TEXT::IncrementLabel( int aIncrement )
{
    wxString text = GetText();
    IncrementLabelMember( text, aIncrement );
    SetText(text );
}


wxPoint SCH_TEXT::GetSchematicTextOffset( RENDER_SETTINGS* aSettings ) const
{
    wxPoint text_offset;

    // add an offset to x (or y) position to aid readability of text on a wire or line
    int dist = GetTextOffset( aSettings ) + GetPenWidth();

    switch( GetLabelSpinStyle() )
    {
    case LABEL_SPIN_STYLE::UP:
    case LABEL_SPIN_STYLE::BOTTOM:
        text_offset.x = -dist;
        break; // Vert Orientation
    default:
    case LABEL_SPIN_STYLE::LEFT:
    case LABEL_SPIN_STYLE::RIGHT:
        text_offset.y = -dist;
        break; // Horiz Orientation
    }

    return text_offset;
}


void SCH_TEXT::MirrorY( int aYaxis_position )
{
    // Text is NOT really mirrored; it is moved to a suitable horizontal position
    SetLabelSpinStyle( GetLabelSpinStyle().MirrorY() );

    SetTextX( Mirror( GetTextPos().x, aYaxis_position ) );
}


void SCH_TEXT::MirrorX( int aXaxis_position )
{
    // Text is NOT really mirrored; it is moved to a suitable vertical position
    SetLabelSpinStyle( GetLabelSpinStyle().MirrorX() );

    SetTextY( Mirror( GetTextPos().y, aXaxis_position ) );
}


void SCH_TEXT::Rotate( wxPoint aPosition )
{
    wxPoint pt = GetTextPos();
    RotatePoint( &pt, aPosition, 900 );
    SetTextPos( pt );

    SetLabelSpinStyle( GetLabelSpinStyle().RotateCW() );

    if( this->Type() == SCH_TEXT_T )
    {
        int dy = 0;

        switch( GetLabelSpinStyle() )
        {
        case LABEL_SPIN_STYLE::LEFT:
        case LABEL_SPIN_STYLE::RIGHT:
            dy = GetTextHeight();
            break;
        case LABEL_SPIN_STYLE::UP:
        case LABEL_SPIN_STYLE::BOTTOM:
        default:
            dy = 0;
            break;
        }

        SetTextY( GetTextPos().y + dy );
    }
}


void SCH_TEXT::SetLabelSpinStyle( LABEL_SPIN_STYLE aSpinStyle )
{
    m_spin_style = aSpinStyle;

    // Assume "Right" and Left" mean which side of the anchor the text will be on
    // Thus we want to left justify text up agaisnt the anchor if we are on the right
    switch( aSpinStyle )
    {
    default:
        wxASSERT_MSG( 1, "Bad spin style" );
        break;

    case LABEL_SPIN_STYLE::RIGHT: // Horiz Normal Orientation
        //
        m_spin_style = LABEL_SPIN_STYLE::RIGHT; // Handle the error spin style by resetting
        SetTextAngle( TEXT_ANGLE_HORIZ );
        SetHorizJustify( GR_TEXT_HJUSTIFY_LEFT );
        SetVertJustify( GR_TEXT_VJUSTIFY_BOTTOM );
        break;

    case LABEL_SPIN_STYLE::UP: // Vert Orientation UP
        SetTextAngle( TEXT_ANGLE_VERT );
        SetHorizJustify( GR_TEXT_HJUSTIFY_LEFT );
        SetVertJustify( GR_TEXT_VJUSTIFY_BOTTOM );
        break;

    case LABEL_SPIN_STYLE::LEFT: // Horiz Orientation - Right justified
        SetTextAngle( TEXT_ANGLE_HORIZ );
        SetHorizJustify( GR_TEXT_HJUSTIFY_RIGHT );
        SetVertJustify( GR_TEXT_VJUSTIFY_BOTTOM );
        break;

    case LABEL_SPIN_STYLE::BOTTOM: //  Vert Orientation BOTTOM
        SetTextAngle( TEXT_ANGLE_VERT );
        SetHorizJustify( GR_TEXT_HJUSTIFY_RIGHT );
        SetVertJustify( GR_TEXT_VJUSTIFY_BOTTOM );
        break;
    }
}


void SCH_TEXT::SwapData( SCH_ITEM* aItem )
{
    SCH_TEXT* item = (SCH_TEXT*) aItem;

    std::swap( m_Layer, item->m_Layer );

    std::swap( m_shape, item->m_shape );
    std::swap( m_isDangling, item->m_isDangling );
    std::swap( m_spin_style, item->m_spin_style );

    SwapText( *item );
    SwapEffects( *item );
}


bool SCH_TEXT::operator<( const SCH_ITEM& aItem ) const
{
    if( Type() != aItem.Type() )
        return Type() < aItem.Type();

    auto other = static_cast<const SCH_TEXT*>( &aItem );

    if( GetLayer() != other->GetLayer() )
            return GetLayer() < other->GetLayer();

    if( GetPosition().x != other->GetPosition().x )
        return GetPosition().x < other->GetPosition().x;

    if( GetPosition().y != other->GetPosition().y )
        return GetPosition().y < other->GetPosition().y;

    return GetText() < other->GetText();
}


int SCH_TEXT::GetTextOffset( RENDER_SETTINGS* aSettings ) const
{
    SCH_RENDER_SETTINGS* renderSettings = static_cast<SCH_RENDER_SETTINGS*>( aSettings );

    if( renderSettings )
        return KiROUND( renderSettings->m_TextOffsetRatio * GetTextSize().y );

    return 0;
}


int SCH_TEXT::GetPenWidth() const
{
    return GetEffectiveTextPenWidth();
}


void SCH_TEXT::Print( RENDER_SETTINGS* aSettings, const wxPoint& aOffset )
{
    COLOR4D color = aSettings->GetLayerColor( m_Layer );
    wxPoint text_offset = aOffset + GetSchematicTextOffset( aSettings );

    EDA_TEXT::Print( aSettings, text_offset, color );
}


void SCH_TEXT::GetEndPoints( std::vector <DANGLING_END_ITEM>& aItemList )
{
    // Normal text labels cannot be tested for dangling ends.
    if( Type() == SCH_TEXT_T )
        return;

    DANGLING_END_ITEM item( LABEL_END, this, GetTextPos() );
    aItemList.push_back( item );
}


bool SCH_TEXT::UpdateDanglingState( std::vector<DANGLING_END_ITEM>& aItemList,
                                    const SCH_SHEET_PATH* aPath )
{
    // Normal text labels cannot be tested for dangling ends.
    if( Type() == SCH_TEXT_T )
        return false;

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
                    m_connected_items[ *aPath ].insert( static_cast<SCH_ITEM*>( item.GetItem() ) );
            }

            break;


        case BUS_START_END:
            m_connectionType = CONNECTION_TYPE::BUS;
            KI_FALLTHROUGH;

        case WIRE_START_END:
        {
            // These schematic items have created 2 DANGLING_END_ITEM one per end.  But being
            // a paranoid programmer, I'll check just in case.
            ii++;

            wxCHECK_MSG( ii < aItemList.size(), previousState != m_isDangling,
                         wxT( "Dangling end type list overflow.  Bad programmer!" ) );

            int accuracy = 1;   // We have rounding issues with an accuracy of 0

            DANGLING_END_ITEM & nextItem = aItemList[ii];
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


void SCH_TEXT::GetConnectionPoints( std::vector< wxPoint >& aPoints ) const
{
    // Normal text labels do not have connection points.  All others do.
    if( Type() == SCH_TEXT_T )
        return;

    aPoints.push_back( GetTextPos() );
}


const EDA_RECT SCH_TEXT::GetBoundingBox() const
{
    EDA_RECT rect = GetTextBox();

    if( GetTextAngle() != 0 )      // Rotate rect
    {
        wxPoint pos = rect.GetOrigin();
        wxPoint end = rect.GetEnd();

        RotatePoint( &pos, GetTextPos(), GetTextAngle() );
        RotatePoint( &end, GetTextPos(), GetTextAngle() );

        rect.SetOrigin( pos );
        rect.SetEnd( end );
    }

    rect.Normalize();
    return rect;
}


wxString getElectricalTypeLabel( PINSHEETLABEL_SHAPE aType )
{
    switch( aType )
    {
    case PINSHEETLABEL_SHAPE::PS_INPUT:       return _( "Input" );
    case PINSHEETLABEL_SHAPE::PS_OUTPUT:      return _( "Output" );
    case PINSHEETLABEL_SHAPE::PS_BIDI:        return _( "Bidirectional" );
    case PINSHEETLABEL_SHAPE::PS_TRISTATE:    return _( "Tri-State" );
    case PINSHEETLABEL_SHAPE::PS_UNSPECIFIED: return _( "Passive" );
    default:                                  return wxT( "???" );
    }
}


wxString SCH_TEXT::GetShownText( int aDepth ) const
{
    std::function<bool( wxString* )> textResolver =
            [&]( wxString* token ) -> bool
            {
                if( ( Type() == SCH_GLOBAL_LABEL_T
                        || Type() == SCH_HIER_LABEL_T
                        || Type() == SCH_SHEET_PIN_T )
                     && token->IsSameAs( wxT( "CONNECTION_TYPE" ) ) )
                {
                    *token = getElectricalTypeLabel( GetShape() );
                    return true;
                }

                if( Type() == SCH_SHEET_PIN_T && m_Parent )
                {
                    SCH_SHEET* sheet = static_cast<SCH_SHEET*>( m_Parent );

                    if( sheet->ResolveTextVar( token, aDepth ) )
                        return true;
                }

                if( Type() == SCH_TEXT_T )
                {
                    if( token->Contains( ':' ) )
                    {
                        wxCHECK_MSG( Schematic(), wxEmptyString,
                                     "No parent SCHEMATIC set for SCH_TEXT!" );

                        SCH_SHEET_LIST sheetList = Schematic()->GetSheets();
                        wxString       remainder;
                        wxString       ref = token->BeforeFirst( ':', &remainder );
                        SCH_SHEET_PATH dummy;
                        SCH_ITEM*      refItem = sheetList.GetItem( KIID( ref ), &dummy );

                        if( refItem && refItem->Type() == SCH_COMPONENT_T )
                        {
                            SCH_COMPONENT* refComponent = static_cast<SCH_COMPONENT*>( refItem );

                            if( refComponent->ResolveTextVar( &remainder, aDepth + 1 ) )
                            {
                                *token = remainder;
                                return true;
                            }
                        }
                        else if( refItem && refItem->Type() == SCH_SHEET_T )
                        {
                            SCH_SHEET* refSheet = static_cast<SCH_SHEET*>( refItem );

                            if( refSheet->ResolveTextVar( &remainder, aDepth + 1 ) )
                            {
                                *token = remainder;
                                return true;
                            }
                        }
                    }
                }

                return false;
            };

    bool     processTextVars = false;
    wxString text = EDA_TEXT::GetShownText( &processTextVars );

    if( processTextVars )
    {
        wxCHECK_MSG( Schematic(), wxEmptyString,
                     "No parent SCHEMATIC set for SCH_TEXT!" );

        PROJECT* project = nullptr;

        if( Schematic() )
            project = &Schematic()->Prj();

        if( aDepth < 10 )
            text = ExpandTextVars( text, &textResolver, project );
    }

    return text;
}


wxString SCH_TEXT::GetSelectMenuText( EDA_UNITS aUnits ) const
{
    return wxString::Format( _( "Graphic Text \"%s\"" ), GetChars( ShortenedShownText() ) );
}


BITMAP_DEF SCH_TEXT::GetMenuImage() const
{
    return text_xpm;
}


void SCH_TEXT::GetNetListItem( NETLIST_OBJECT_LIST& aNetListItems,
                               SCH_SHEET_PATH*      aSheetPath )
{
    if( GetLayer() == LAYER_NOTES || GetLayer() == LAYER_SHEETLABEL )
        return;

    NETLIST_OBJECT* item = new NETLIST_OBJECT();
    item->m_SheetPath        = *aSheetPath;
    item->m_SheetPathInclude = *aSheetPath;
    item->m_Comp             = (SCH_ITEM*) this;
    item->m_Type             = NETLIST_ITEM::LABEL;

    if( GetLayer() == LAYER_GLOBLABEL )
        item->m_Type = NETLIST_ITEM::GLOBLABEL;
    else if( GetLayer() == LAYER_HIERLABEL )
        item->m_Type = NETLIST_ITEM::HIERLABEL;

    item->m_Label = GetText();
    item->m_Start = item->m_End = GetTextPos();

    aNetListItems.push_back( item );

    // If a bus connects to label
    if( SCH_CONNECTION::IsBusLabel( GetText() ) )
        item->ConvertBusToNetListItems( aNetListItems );
}


bool SCH_TEXT::HitTest( const wxPoint& aPosition, int aAccuracy ) const
{
    EDA_RECT bBox = GetBoundingBox();
    bBox.Inflate( aAccuracy );
    return bBox.Contains( aPosition );
}


bool SCH_TEXT::HitTest( const EDA_RECT& aRect, bool aContained, int aAccuracy ) const
{
    EDA_RECT bBox = GetBoundingBox();
    bBox.Inflate( aAccuracy );

    if( aContained )
        return aRect.Contains( bBox );

    return aRect.Intersects( bBox );
}


void SCH_TEXT::Plot( PLOTTER* aPlotter )
{
    static std::vector<wxPoint> Poly;
    COLOR4D color = aPlotter->RenderSettings()->GetLayerColor( GetLayer() );
    int penWidth = GetEffectiveTextPenWidth( aPlotter->RenderSettings()->GetDefaultPenWidth() );

    aPlotter->SetCurrentLineWidth( penWidth );

    if( IsMultilineAllowed() )
    {
        std::vector<wxPoint> positions;
        wxArrayString strings_list;
        wxStringSplit( GetShownText(), strings_list, '\n' );
        positions.reserve( strings_list.Count() );

        GetLinePositions( positions, (int) strings_list.Count() );

        for( unsigned ii = 0; ii < strings_list.Count(); ii++ )
        {
            wxPoint textpos = positions[ii] + GetSchematicTextOffset( aPlotter->RenderSettings() );
            wxString& txt = strings_list.Item( ii );
            aPlotter->Text( textpos, color, txt, GetTextAngle(), GetTextSize(), GetHorizJustify(),
                            GetVertJustify(), penWidth, IsItalic(), IsBold() );
        }
    }
    else
    {
        wxPoint textpos = GetTextPos() + GetSchematicTextOffset( aPlotter->RenderSettings() );

        aPlotter->Text( textpos, color, GetShownText(), GetTextAngle(), GetTextSize(),
                        GetHorizJustify(), GetVertJustify(), penWidth, IsItalic(), IsBold() );
    }

    // Draw graphic symbol for global or hierarchical labels
    CreateGraphicShape( aPlotter->RenderSettings(), Poly, GetTextPos() );

    if( Poly.size() )
        aPlotter->PlotPoly( Poly, NO_FILL, penWidth );
}


void SCH_TEXT::GetMsgPanelInfo( EDA_DRAW_FRAME* aFrame, MSG_PANEL_ITEMS& aList )
{
    wxString msg;

    switch( Type() )
    {
    case SCH_TEXT_T:          msg = _( "Graphic Text" );           break;
    case SCH_LABEL_T:         msg = _( "Label" );                  break;
    case SCH_GLOBAL_LABEL_T:  msg = _( "Global Label" );           break;
    case SCH_HIER_LABEL_T:    msg = _( "Hierarchical Label" );     break;
    case SCH_SHEET_PIN_T:     msg = _( "Hierarchical Sheet Pin" ); break;
    default: return;
    }

    aList.push_back( MSG_PANEL_ITEM( msg, GetShownText(), DARKCYAN ) );

    switch( GetLabelSpinStyle() )
    {
    case LABEL_SPIN_STYLE::LEFT:   msg = _( "Horizontal left" );  break;
    case LABEL_SPIN_STYLE::UP:     msg = _( "Vertical up" );      break;
    case LABEL_SPIN_STYLE::RIGHT:  msg = _( "Horizontal right" ); break;
    case LABEL_SPIN_STYLE::BOTTOM: msg = _( "Vertical down" );    break;
    default:                       msg = wxT( "???" );            break;
    }

    aList.push_back( MSG_PANEL_ITEM( _( "Orientation" ), msg, BROWN ) );

    wxString textStyle[] = { _( "Normal" ), _( "Italic" ), _( "Bold" ), _( "Bold Italic" ) };
    int style = 0;

    if( IsItalic() )
        style = 1;

    if( IsBold() )
        style += 2;

    aList.push_back( MSG_PANEL_ITEM( _( "Style" ), textStyle[style], BROWN ) );

    // Display electrical type if it is relevant
    if( Type() == SCH_GLOBAL_LABEL_T || Type() == SCH_HIER_LABEL_T || Type() == SCH_SHEET_PIN_T )
    {
        msg = getElectricalTypeLabel( GetShape() );
        aList.push_back( MSG_PANEL_ITEM( _( "Type" ), msg, BLUE ) );
    }

    // Display text size (X or Y value, with are the same value in Eeschema)
    msg = MessageTextFromValue( aFrame->GetUserUnits(), GetTextWidth(), true );
    aList.push_back( MSG_PANEL_ITEM( _( "Size" ), msg, RED ) );

#if defined(DEBUG)
    SCH_EDIT_FRAME* schframe = dynamic_cast<SCH_EDIT_FRAME*>( aFrame );

    if( schframe )
    {
        if( auto conn = Connection( schframe->GetCurrentSheet() ) )
            conn->AppendDebugInfoToMsgPanel( aList );
    }

    msg.Printf( "%p", this );
    aList.push_back( MSG_PANEL_ITEM( "Object Address", msg, RED ) );

#endif
}

#if defined(DEBUG)

void SCH_TEXT::Show( int nestLevel, std::ostream& os ) const
{
    // XML output:
    wxString s = GetClass();

    NestedSpace( nestLevel, os ) << '<' << s.Lower().mb_str()
                                 << " layer=\"" << m_Layer << '"'
                                 << " shape=\"" << static_cast<int>( m_shape ) << '"'
                                 << " dangling=\"" << m_isDangling << '"'
                                 << '>'
                                 << TO_UTF8( GetText() )
                                 << "</" << s.Lower().mb_str() << ">\n";
}

#endif


SCH_LABEL::SCH_LABEL( const wxPoint& pos, const wxString& text )
        : SCH_TEXT( pos, text, SCH_LABEL_T )
{
    m_Layer      = LAYER_LOCLABEL;
    m_shape      = PINSHEETLABEL_SHAPE::PS_INPUT;
    m_isDangling = true;
    SetMultilineAllowed( false );
}


EDA_ITEM* SCH_LABEL::Clone() const
{
    return new SCH_LABEL( *this );
}


bool SCH_LABEL::IsType( const KICAD_T aScanTypes[] ) const
{
    static KICAD_T wireTypes[] = { SCH_LINE_LOCATE_WIRE_T, EOT };
    static KICAD_T busTypes[] = { SCH_LINE_LOCATE_BUS_T, EOT };

    if( SCH_ITEM::IsType( aScanTypes ) )
        return true;

    wxCHECK_MSG( Schematic(), false, "No parent SCHEMATIC set for SCH_LABEL!" );

    SCH_SHEET_PATH current = Schematic()->CurrentSheet();

    for( const KICAD_T* p = aScanTypes; *p != EOT; ++p )
    {
        if( *p == SCH_LABEL_LOCATE_WIRE_T )
        {
            wxASSERT( m_connected_items.count( current ) );

            for( SCH_ITEM* connection : m_connected_items.at( current ) )
            {
                if( connection->IsType( wireTypes ) )
                    return true;
            }
        }
        else if ( *p == SCH_LABEL_LOCATE_BUS_T )
        {
            wxASSERT( m_connected_items.count( current ) );

            for( SCH_ITEM* connection : m_connected_items.at( current ) )
            {
                if( connection->IsType( busTypes ) )
                    return true;
            }
        }
    }

    return false;
}


const EDA_RECT SCH_LABEL::GetBoundingBox() const
{
    EDA_RECT rect = GetTextBox();

    // In practice this is controlled by the current TextOffsetRatio, but the default is
    // close enough for hit-testing, etc.
    int margin = Mils2iu( TXT_MARGIN );

    rect.Inflate( margin );

    if( GetTextAngle() != 0.0 )
    {
        // Rotate rect
        wxPoint pos = rect.GetOrigin();
        wxPoint end = rect.GetEnd();

        RotatePoint( &pos, GetTextPos(), GetTextAngle() );
        RotatePoint( &end, GetTextPos(), GetTextAngle() );

        rect.SetOrigin( pos );
        rect.SetEnd( end );

        rect.Normalize();
    }

    return rect;
}


wxString SCH_LABEL::GetSelectMenuText( EDA_UNITS aUnits ) const
{
    return wxString::Format( _( "Label %s" ), ShortenedShownText() );
}


BITMAP_DEF SCH_LABEL::GetMenuImage() const
{
    return add_line_label_xpm;
}


SCH_GLOBALLABEL::SCH_GLOBALLABEL( const wxPoint& pos, const wxString& text )
        : SCH_TEXT( pos, text, SCH_GLOBAL_LABEL_T )
{
    m_Layer      = LAYER_GLOBLABEL;
    m_shape      = PINSHEETLABEL_SHAPE::PS_BIDI;
    m_isDangling = true;
    SetMultilineAllowed( false );
}


EDA_ITEM* SCH_GLOBALLABEL::Clone() const
{
    return new SCH_GLOBALLABEL( *this );
}


wxPoint SCH_GLOBALLABEL::GetSchematicTextOffset( RENDER_SETTINGS* aSettings ) const
{
    wxPoint text_offset;
    int     dist = GetTextOffset( aSettings );

    switch( m_shape )
    {
    case PINSHEETLABEL_SHAPE::PS_INPUT:
    case PINSHEETLABEL_SHAPE::PS_BIDI:
    case PINSHEETLABEL_SHAPE::PS_TRISTATE:
        dist += GetTextHeight() * 3 / 4;  // Use three-quarters-height as proxy for triangle size
        break;

    case PINSHEETLABEL_SHAPE::PS_OUTPUT:
    case PINSHEETLABEL_SHAPE::PS_UNSPECIFIED:
    default:
        break;
    }

    switch( GetLabelSpinStyle() )
    {
    default:
    case LABEL_SPIN_STYLE::LEFT:   text_offset.x -= dist; break;
    case LABEL_SPIN_STYLE::UP:     text_offset.y -= dist; break;
    case LABEL_SPIN_STYLE::RIGHT:  text_offset.x += dist; break;
    case LABEL_SPIN_STYLE::BOTTOM: text_offset.y += dist; break;
    }

    return text_offset;
}


void SCH_GLOBALLABEL::SetLabelSpinStyle( LABEL_SPIN_STYLE aSpinStyle )
{
    m_spin_style = aSpinStyle;

    switch( aSpinStyle )
    {
    default:
        wxASSERT_MSG( 1, "Bad spin style" );
        break;

    case LABEL_SPIN_STYLE::RIGHT: // Horiz Normal Orientation
        //
        m_spin_style = LABEL_SPIN_STYLE::RIGHT; // Handle the error spin style by resetting
        SetTextAngle( TEXT_ANGLE_HORIZ );
        SetHorizJustify( GR_TEXT_HJUSTIFY_LEFT );
        SetVertJustify( GR_TEXT_VJUSTIFY_CENTER );
        break;

    case LABEL_SPIN_STYLE::UP: // Vert Orientation UP
        SetTextAngle( TEXT_ANGLE_VERT );
        SetHorizJustify( GR_TEXT_HJUSTIFY_LEFT );
        SetVertJustify( GR_TEXT_VJUSTIFY_CENTER );
        break;

    case LABEL_SPIN_STYLE::LEFT: // Horiz Orientation
        SetTextAngle( TEXT_ANGLE_HORIZ );
        SetHorizJustify( GR_TEXT_HJUSTIFY_RIGHT );
        SetVertJustify( GR_TEXT_VJUSTIFY_CENTER );
        break;

    case LABEL_SPIN_STYLE::BOTTOM: //  Vert Orientation BOTTOM
        SetTextAngle( TEXT_ANGLE_VERT );
        SetHorizJustify( GR_TEXT_HJUSTIFY_RIGHT );
        SetVertJustify( GR_TEXT_VJUSTIFY_CENTER );
        break;
    }
}


void SCH_GLOBALLABEL::Print( RENDER_SETTINGS* aSettings, const wxPoint& aOffset )
{
    static std::vector <wxPoint> Poly;

    wxDC*   DC = aSettings->GetPrintDC();
    COLOR4D color = aSettings->GetLayerColor( m_Layer );
    int     penWidth = std::max( GetPenWidth(), aSettings->GetDefaultPenWidth() );
    wxPoint text_offset = aOffset + GetSchematicTextOffset( aSettings );

    EDA_TEXT::Print( aSettings, text_offset, color );

    CreateGraphicShape( aSettings, Poly, GetTextPos() + aOffset );
    GRPoly( nullptr, DC, Poly.size(), &Poly[0], false, penWidth, color, color );
}


void SCH_GLOBALLABEL::CreateGraphicShape( RENDER_SETTINGS* aRenderSettings,
                                          std::vector<wxPoint>& aPoints, const wxPoint& Pos )
{
    int margin    = GetTextOffset( aRenderSettings );
    int halfSize  = ( GetTextHeight() / 2 ) + margin;
    int linewidth = GetPenWidth();
    int symb_len  = LenSize( GetShownText(), linewidth ) + 2 * margin;

    int x = symb_len + linewidth + 3;
    int y = halfSize + linewidth + 3;

    aPoints.clear();

    // Create outline shape : 6 points
    aPoints.emplace_back( wxPoint( 0, 0 ) );
    aPoints.emplace_back( wxPoint( 0, -y ) );     // Up
    aPoints.emplace_back( wxPoint( -x, -y ) );    // left
    aPoints.emplace_back( wxPoint( -x, 0 ) );     // Up left
    aPoints.emplace_back( wxPoint( -x, y ) );     // left down
    aPoints.emplace_back( wxPoint( 0, y ) );      // down

    int x_offset = 0;

    switch( m_shape )
    {
    case PINSHEETLABEL_SHAPE::PS_INPUT:
        x_offset = -halfSize;
        aPoints[0].x += halfSize;
        break;

    case PINSHEETLABEL_SHAPE::PS_OUTPUT:
        aPoints[3].x -= halfSize;
        break;

    case PINSHEETLABEL_SHAPE::PS_BIDI:
    case PINSHEETLABEL_SHAPE::PS_TRISTATE:
        x_offset = -halfSize;
        aPoints[0].x += halfSize;
        aPoints[3].x -= halfSize;
        break;

    case PINSHEETLABEL_SHAPE::PS_UNSPECIFIED:
    default:
        break;
    }

    int angle = 0;

    switch( GetLabelSpinStyle() )
    {
    default:
    case LABEL_SPIN_STYLE::LEFT:                 break;
    case LABEL_SPIN_STYLE::UP:     angle = -900; break;
    case LABEL_SPIN_STYLE::RIGHT:  angle = 1800; break;
    case LABEL_SPIN_STYLE::BOTTOM: angle = 900;  break;
    }

    // Rotate outlines and move corners in real position
    for( wxPoint& aPoint : aPoints )
    {
        aPoint.x += x_offset;

        if( angle )
            RotatePoint( &aPoint, angle );

        aPoint += Pos;
    }

    aPoints.push_back( aPoints[0] ); // closing
}


const EDA_RECT SCH_GLOBALLABEL::GetBoundingBox() const
{
    int x  = GetTextPos().x;
    int y  = GetTextPos().y;
    int penWidth = GetEffectiveTextPenWidth();

    // In practice this is controlled by the current TextOffsetRatio, but the default is
    // close enough for hit-testing, etc.
    int margin = Mils2iu( TXT_MARGIN );

    int height = ( (GetTextHeight() * 15) / 10 ) + penWidth + 2 * margin;
    int length = LenSize( GetShownText(), penWidth )
                 + height                 // add height for triangular shapes
                 + 2 * margin;

    int dx, dy;

    switch( GetLabelSpinStyle() )    // respect orientation
    {
    default:
    case LABEL_SPIN_STYLE::LEFT:
        dx = -length;
        dy = height;
        x += margin;
        y -= height / 2;
        break;

    case LABEL_SPIN_STYLE::UP:
        dx = height;
        dy = -length;
        x -= height / 2;
        y += margin;
        break;

    case LABEL_SPIN_STYLE::RIGHT:
        dx = length;
        dy = height;
        x -= margin;
        y -= height / 2;
        break;

    case LABEL_SPIN_STYLE::BOTTOM:
        dx = height;
        dy = length;
        x -= height / 2;
        y -= margin;
        break;
    }

    EDA_RECT box( wxPoint( x, y ), wxSize( dx, dy ) );
    box.Normalize();
    return box;
}


wxString SCH_GLOBALLABEL::GetSelectMenuText( EDA_UNITS aUnits ) const
{
    return wxString::Format( _( "Global Label %s" ), ShortenedShownText() );
}


BITMAP_DEF SCH_GLOBALLABEL::GetMenuImage() const
{
    return add_glabel_xpm;
}


SCH_HIERLABEL::SCH_HIERLABEL( const wxPoint& pos, const wxString& text, KICAD_T aType )
        : SCH_TEXT( pos, text, aType )
{
    m_Layer      = LAYER_HIERLABEL;
    m_shape      = PINSHEETLABEL_SHAPE::PS_INPUT;
    m_isDangling = true;
    SetMultilineAllowed( false );
}


EDA_ITEM* SCH_HIERLABEL::Clone() const
{
    return new SCH_HIERLABEL( *this );
}


void SCH_HIERLABEL::SetLabelSpinStyle( LABEL_SPIN_STYLE aSpinStyle )
{
    m_spin_style = aSpinStyle;

    // Assume "Right" and Left" mean which side of the port symbol the text will be on
    // If we are left of the symbol, we want to right justify to line up with the symbol
    switch( aSpinStyle )
    {
    default:
        wxLogWarning( "SetLabelSpinStyle bad spin style" );
        break;

    case LABEL_SPIN_STYLE::LEFT:
        //
        m_spin_style = LABEL_SPIN_STYLE::LEFT; // Handle the error spin style by resetting
        SetTextAngle( TEXT_ANGLE_HORIZ );
        SetHorizJustify( GR_TEXT_HJUSTIFY_RIGHT );
        SetVertJustify( GR_TEXT_VJUSTIFY_CENTER );
        break;

    case LABEL_SPIN_STYLE::UP:
        SetTextAngle( TEXT_ANGLE_VERT );
        SetHorizJustify( GR_TEXT_HJUSTIFY_LEFT );
        SetVertJustify( GR_TEXT_VJUSTIFY_CENTER );
        break;

    case LABEL_SPIN_STYLE::RIGHT:
        SetTextAngle( TEXT_ANGLE_HORIZ );
        SetHorizJustify( GR_TEXT_HJUSTIFY_LEFT );
        SetVertJustify( GR_TEXT_VJUSTIFY_CENTER );
        break;

    case LABEL_SPIN_STYLE::BOTTOM:
        SetTextAngle( TEXT_ANGLE_VERT );
        SetHorizJustify( GR_TEXT_HJUSTIFY_RIGHT );
        SetVertJustify( GR_TEXT_VJUSTIFY_CENTER );
        break;
    }
}


void SCH_HIERLABEL::Print( RENDER_SETTINGS* aSettings, const wxPoint& offset )
{
    wxCHECK_RET( Schematic(), "No parent SCHEMATIC set for SCH_LABEL!" );

    static std::vector <wxPoint> Poly;

    wxDC*           DC = aSettings->GetPrintDC();
    SCH_CONNECTION* conn = Connection( Schematic()->CurrentSheet() );
    bool            isBus = conn && conn->IsBus();
    COLOR4D         color = aSettings->GetLayerColor( isBus ? LAYER_BUS : m_Layer );
    int             penWidth = std::max( GetPenWidth(), aSettings->GetDefaultPenWidth() );
    wxPoint         textOffset = offset + GetSchematicTextOffset( aSettings );

    EDA_TEXT::Print( aSettings, textOffset, color );

    CreateGraphicShape( aSettings, Poly, GetTextPos() + offset );
    GRPoly( nullptr, DC, Poly.size(), &Poly[0], false, penWidth, color, color );
}


void SCH_HIERLABEL::CreateGraphicShape( RENDER_SETTINGS* aRenderSettings,
                                        std::vector<wxPoint>& aPoints, const wxPoint& Pos )
{
    int* Template = TemplateShape[static_cast<int>( m_shape )][static_cast<int>( m_spin_style )];
    int  halfSize = GetTextHeight() / 2;
    int  imax = *Template;
    Template++;

    aPoints.clear();

    for( int ii = 0; ii < imax; ii++ )
    {
        wxPoint corner;
        corner.x = ( halfSize * (*Template) ) + Pos.x;
        Template++;

        corner.y = ( halfSize * (*Template) ) + Pos.y;
        Template++;

        aPoints.push_back( corner );
    }
}


const EDA_RECT SCH_HIERLABEL::GetBoundingBox() const
{
    int penWidth = GetEffectiveTextPenWidth();

    // In practice this is controlled by the current TextOffsetRatio, but the default is
    // close enough for hit-testing, etc.
    int margin = Mils2iu( TXT_MARGIN );

    int x  = GetTextPos().x;
    int y  = GetTextPos().y;

    int height = GetTextHeight() + penWidth + 2 * margin;
    int length = LenSize( GetShownText(), penWidth )
                 + height                 // add height for triangular shapes
                 + 2 * margin;

    int dx, dy;

    switch( GetLabelSpinStyle() )
    {
    default:
    case LABEL_SPIN_STYLE::LEFT:
        dx = -length;
        dy = height;
        x += Mils2iu( DANGLING_SYMBOL_SIZE );
        y -= height / 2;
        break;

    case LABEL_SPIN_STYLE::UP:
        dx = height;
        dy = -length;
        x -= height / 2;
        y += Mils2iu( DANGLING_SYMBOL_SIZE );
        break;

    case LABEL_SPIN_STYLE::RIGHT:
        dx = length;
        dy = height;
        x -= Mils2iu( DANGLING_SYMBOL_SIZE );
        y -= height / 2;
        break;

    case LABEL_SPIN_STYLE::BOTTOM:
        dx = height;
        dy = length;
        x -= height / 2;
        y -= Mils2iu( DANGLING_SYMBOL_SIZE );
        break;
    }

    EDA_RECT box( wxPoint( x, y ), wxSize( dx, dy ) );
    box.Normalize();
    return box;
}


wxPoint SCH_HIERLABEL::GetSchematicTextOffset( RENDER_SETTINGS* aSettings ) const
{
    wxPoint text_offset;
    int     dist = GetTextOffset( aSettings );

    dist += GetTextWidth();

    switch( GetLabelSpinStyle() )
    {
    default:
    case LABEL_SPIN_STYLE::LEFT:   text_offset.x = -dist; break; // Orientation horiz normale
    case LABEL_SPIN_STYLE::UP:     text_offset.y = -dist; break; // Orientation vert UP
    case LABEL_SPIN_STYLE::RIGHT:  text_offset.x = dist;  break; // Orientation horiz inverse
    case LABEL_SPIN_STYLE::BOTTOM: text_offset.y = dist;  break; // Orientation vert BOTTOM
    }

    return text_offset;
}


wxString SCH_HIERLABEL::GetSelectMenuText( EDA_UNITS aUnits ) const
{
    return wxString::Format( _( "Hierarchical Label %s" ), ShortenedShownText() );
}


BITMAP_DEF SCH_HIERLABEL::GetMenuImage() const
{
    return add_hierarchical_label_xpm;
}


void SCH_TEXT::ShowSyntaxHelp( wxWindow* aParentWindow )
{
    wxString msg = _(
            "<table>"
            "   <tr>"
            "      <th>Markup</th>"
            "      <th></th>"
            "      <th>Result</th>"
            "   </tr>"
            "   <tr>"
            "      <td>&nbsp;<br><samp>^{superscript}</samp></td>"
            "      <td></td>"
            "      <td>&nbsp;<br><samp><sup>superscript</sup>&nbsp;</samp></td>"
            "   </tr>"
            "   <tr>"
            "      <td>&nbsp;<br><samp>Driver Board^{Rev A}</samp></td>"
            "      <td></td>"
            "      <td>&nbsp;<br><samp>Driver Board<sup>Rev A</sup></samp></td>"
            "   </tr>"
            "   <tr><td><br></td></tr>"
            "   <tr>"
            "      <td>&nbsp;<br><samp>_{subscript}</samp></td>"
            "      <td></td>"
            "      <td>&nbsp;<br><samp><sub>subscript</sub>&nbsp;</samp></td>"
            "   </tr>"
            "   <tr>"
            "      <td>&nbsp;<br><samp>D_{0} - D_{15}</samp></td>"
            "      <td></td>"
            "      <td>&nbsp;<br><samp>D<sub>0</sub> - D<sub>31</sub></samp></td>"
            "   </tr>"
            "   <tr><td></td></tr>"
            "   <tr>"
            "      <td>"
            "         &nbsp;<br><samp>~overbar</samp><br>"
            "         &nbsp;<br><samp>~CLK</samp>"
            "      </td>"
            "      <td></td>"
            "      <td>"
            "         <samp><u>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;</u></samp><br>"
            "         <samp>overbar</samp><br>"
            "         <samp><u>&nbsp;&nbsp;&nbsp;</u></samp><br>"
            "         <samp>CLK</samp>"
            "      </td>"
            "   </tr>"
            "   <tr><td><br></td></tr>"
            "   <tr>"
            "      <td>&nbsp;<br><samp>${variable}</samp></td>"
            "      <td></td>"
            "      <td>&nbsp;<br><samp><i>variable_value</i></samp></td>"
            "   </tr>"
            "   <tr>"
            "      <td>&nbsp;<br><samp>${REVISION}</samp></td>"
            "      <td></td>"
            "      <td>&nbsp;<br><samp>2020.1</samp></td>"
            "   </tr>"
            "   <tr><td><br></td></tr>"
            "   <tr>"
            "      <td>&nbsp;<br><samp>${refdes:field}</samp></td>"
            "      <td></td>"
            "      <td>&nbsp;<br><samp><i>field_value</i> of symbol <i>refdes</i></samp></td>"
            "   </tr>"
            "   <tr>"
            "      <td>&nbsp;<br><samp>${R3:VALUE}</samp></td>"
            "      <td></td>"
            "      <td>&nbsp;<br><samp>150K</samp></td>"
            "   </tr>"
            "   <tr><td><br></td></tr>"
            "   <tr><td><br></td></tr>"
            "   <tr>"
            "      <th>Bus Definition</th>"
            "      <th>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;</th>"
            "      <th>Resultant Nets</th>"
            "   </tr>"
            "   <tr>"
            "      <td>&nbsp;<br><samp>prefix[m..n]</samp></td>"
            "      <td></td>"
            "      <td>&nbsp;<br><samp>prefixm to prefixn</samp></td>"
            "   </tr>"
            "   <tr>"
            "      <td>&nbsp;<br><samp>D[0..7]</samp></td>"
            "      <td></td>"
            "      <td>&nbsp;<br><samp>D0, D1, D2, D3, D4, D5, D6, D7</samp></td>"
            "   </tr>"
            "   <tr><td><br></td></tr>"
            "   <tr><samp>"
            "      <td>&nbsp;<br><samp>{net1 net2 ...}</samp></td>"
            "      <td></td>"
            "      <td>&nbsp;<br><samp>net1, net2, ...</samp></td>"
            "   </tr>"
            "   <tr>"
            "      <td>&nbsp;<br><samp>{SCL SDA}</samp></td>"
            "      <td></td>"
            "      <td>&nbsp;<br><samp>SCL, SDA</samp></td>"
            "   </tr></samp>"
            "   <tr><td><br></td></tr>"
            "   <tr>"
            "      <td>&nbsp;<br><samp>prefix{net1 net2 ...}</samp></td>"
            "      <td></td>"
            "      <td>&nbsp;<br><samp>prefix.net1, prefix.net2, ...</samp></td>"
            "   </tr>"
            "   <tr>"
            "      <td>&nbsp;<br><samp>USB1{DP DM}</samp></td>"
            "      <td></td>"
            "      <td>&nbsp;<br><samp>USB1.DP, USB1.DM</samp></td>"
            "   </tr>"
            "   <tr><td><br></td></tr>"
            "   <tr>"
            "      <td>&nbsp;<br><samp>MEM{D[1..2] LATCH}</samp></td>"
            "      <td></td>"
            "      <td>&nbsp;<br>"
            "         <samp>MEM.D1, MEM.D2, MEM.LATCH</samp>"
            "      </td>"
            "   </tr>"
            "   <tr>"
            "      <td>&nbsp;<br><samp>MEM{D_{[1..2]} ~LATCH}</samp></td>"
            "      <td></td>"
            "      <td>"
            "         <samp>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;"
            "               &nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;"
            "               <u>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;</u></samp><br>"
            "         <samp>MEM.D<sub>1</sub>, MEM.D<sub>2</sub>, MEM.LATCH</samp>"
            "      </td>"
            "   </tr>"
            "</table>" );

    HTML_MESSAGE_BOX dlg( aParentWindow, _( "Syntax Help" ) );
    dlg.SetDialogSizeInDU( 280, 280 );

    dlg.AddHTML_Text( msg );
    dlg.ShowModal();
}
