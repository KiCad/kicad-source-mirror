/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2016 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 1992-2020 Kicad Developers, see AUTHORS.txt for contributors.
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

#include <core/mirror.h>
#include <sch_draw_panel.h>
#include <gr_text.h>
#include <trigo.h>
#include <sch_edit_frame.h>
#include <plotter.h>
#include <kicad_string.h>
#include <widgets/msgpanel.h>
#include <math/util.h>      // for KiROUND
#include <sch_sheet.h>
#include <sch_sheet_path.h>
#include <sch_component.h>
#include <sch_painter.h>
#include <schematic.h>
#include <settings/color_settings.h>
#include <trace_helpers.h>
#include <pgm_base.h>


const wxString SCH_SHEET::GetDefaultFieldName( int aFieldNdx )
{
    static void* locale = nullptr;
    static wxString sheetnameDefault;
    static wxString sheetfilenameDefault;
    static wxString fieldDefault;

    // Fetching translations can take a surprising amount of time when loading libraries,
    // so only do it when necessary.
    if( Pgm().GetLocale() != locale )
    {
        sheetnameDefault     = _( "Sheet name" );
        sheetfilenameDefault = _( "Sheet file" );
        fieldDefault         = _( "Field%d" );
        locale = Pgm().GetLocale();
    }

    // Fixed values for the mandatory fields
    switch( aFieldNdx )
    {
    case  SHEETNAME:     return sheetnameDefault;
    case  SHEETFILENAME: return sheetfilenameDefault;
    default:             return wxString::Format( fieldDefault, aFieldNdx );
    }
}


SCH_SHEET::SCH_SHEET( EDA_ITEM* aParent, const wxPoint& pos ) :
    SCH_ITEM( aParent, SCH_SHEET_T )
{
    m_layer = LAYER_SHEET;
    m_pos = pos;
    m_size = wxSize( Mils2iu( MIN_SHEET_WIDTH ), Mils2iu( MIN_SHEET_HEIGHT ) );
    m_screen = NULL;

    for( int i = 0; i < SHEET_MANDATORY_FIELDS; ++i )
    {
        m_fields.emplace_back( pos, i, this, GetDefaultFieldName( i ) );
        m_fields.back().SetVisible( true );

        if( i == SHEETNAME )
            m_fields.back().SetLayer( LAYER_SHEETNAME );
        else if( i == SHEETFILENAME )
            m_fields.back().SetLayer( LAYER_SHEETFILENAME );
        else
            m_fields.back().SetLayer( LAYER_SHEETFIELDS );
    }

    m_fieldsAutoplaced = FIELDS_AUTOPLACED_AUTO;

    m_borderWidth = 0;
    m_borderColor = COLOR4D::UNSPECIFIED;
    m_backgroundColor = COLOR4D::UNSPECIFIED;
}


SCH_SHEET::SCH_SHEET( const SCH_SHEET& aSheet ) :
    SCH_ITEM( aSheet )
{
    m_pos = aSheet.m_pos;
    m_size = aSheet.m_size;
    m_layer = aSheet.m_layer;
    const_cast<KIID&>( m_Uuid ) = aSheet.m_Uuid;
    m_fields = aSheet.m_fields;
    m_fieldsAutoplaced = aSheet.m_fieldsAutoplaced;
    m_screen = aSheet.m_screen;

    for( SCH_SHEET_PIN* pin : aSheet.m_pins )
    {
        m_pins.emplace_back( new SCH_SHEET_PIN( *pin ) );
        m_pins.back()->SetParent( this );
    }

    m_borderWidth = aSheet.m_borderWidth;
    m_borderColor = aSheet.m_borderColor;
    m_backgroundColor = aSheet.m_backgroundColor;
    m_instances = aSheet.m_instances;

    if( m_screen )
        m_screen->IncRefCount();
}


SCH_SHEET::~SCH_SHEET()
{
    // also, look at the associated sheet & its reference count
    // perhaps it should be deleted also.
    if( m_screen )
    {
        m_screen->DecRefCount();

        if( m_screen->GetRefCount() == 0 )
            delete m_screen;
    }

    // We own our pins; delete them
    for( SCH_SHEET_PIN* pin : m_pins )
        delete pin;
}


EDA_ITEM* SCH_SHEET::Clone() const
{
    return new SCH_SHEET( *this );
}


void SCH_SHEET::SetScreen( SCH_SCREEN* aScreen )
{
    if( aScreen == m_screen )
        return;

    if( m_screen != NULL )
    {
        m_screen->DecRefCount();

        if( m_screen->GetRefCount() == 0 )
        {
            delete m_screen;
            m_screen = NULL;
        }
    }

    m_screen = aScreen;

    if( m_screen )
        m_screen->IncRefCount();
}


int SCH_SHEET::GetScreenCount() const
{
    if( m_screen == NULL )
        return 0;

    return m_screen->GetRefCount();
}


bool SCH_SHEET::IsRootSheet() const
{
    wxCHECK_MSG( Schematic(), false, "Can't call IsRootSheet without setting a schematic" );

    return &Schematic()->Root() == this;
}


void SCH_SHEET::GetContextualTextVars( wxArrayString* aVars ) const
{
    for( int i = 0; i < SHEET_MANDATORY_FIELDS; ++i )
        aVars->push_back( m_fields[i].GetCanonicalName().Upper() );

    for( size_t i = SHEET_MANDATORY_FIELDS; i < m_fields.size(); ++i )
        aVars->push_back( m_fields[i].GetName() );

    aVars->push_back( wxT( "#" ) );
    aVars->push_back( wxT( "##" ) );
}


bool SCH_SHEET::ResolveTextVar( wxString* token, int aDepth ) const
{
    for( int i = 0; i < SHEET_MANDATORY_FIELDS; ++i )
    {
        if( token->IsSameAs( m_fields[i].GetCanonicalName().Upper() ) )
        {
            *token = m_fields[i].GetShownText( aDepth + 1 );
            return true;
        }
    }

    for( size_t i = SHEET_MANDATORY_FIELDS; i < m_fields.size(); ++i )
    {
        if( token->IsSameAs( m_fields[i].GetName() ) )
        {
            *token = m_fields[i].GetShownText( aDepth + 1 );
            return true;
        }
    }

    if( token->IsSameAs( wxT( "#" ) ) )
    {
        for( const SCH_SHEET_PATH& sheet : Schematic()->GetSheets() )
        {
            if( sheet.Last() == this )   // Current sheet path found
            {
                *token = wxString::Format( "%s", sheet.GetPageNumber() );
                return true;
            }
        }
    }
    else if( token->IsSameAs( wxT( "##" ) ) )
    {
        SCH_SHEET_LIST sheetList = Schematic()->GetSheets();
        *token = wxString::Format( wxT( "%d" ), (int) sheetList.size() );
        return true;
    }

    return false;
}


bool SCH_SHEET::UsesDefaultStroke() const
{
    return m_borderWidth == 0 && m_borderColor == COLOR4D::UNSPECIFIED;
}


void SCH_SHEET::SwapData( SCH_ITEM* aItem )
{
    wxCHECK_RET( aItem->Type() == SCH_SHEET_T,
            wxString::Format( wxT( "SCH_SHEET object cannot swap data with %s object." ),
                    aItem->GetClass() ) );

    SCH_SHEET* sheet = ( SCH_SHEET* ) aItem;

    std::swap( m_pos, sheet->m_pos );
    std::swap( m_size, sheet->m_size );
    m_fields.swap( sheet->m_fields );
    std::swap( m_fieldsAutoplaced, sheet->m_fieldsAutoplaced );
    m_pins.swap( sheet->m_pins );

    // Update parent pointers after swapping.
    for( SCH_SHEET_PIN* sheetPin : m_pins )
        sheetPin->SetParent( this );

    for( SCH_SHEET_PIN* sheetPin : sheet->m_pins )
        sheetPin->SetParent( sheet );

    std::swap( m_borderWidth, sheet->m_borderWidth );
    std::swap( m_borderColor, sheet->m_borderColor );
    std::swap( m_backgroundColor, sheet->m_backgroundColor );
    std::swap( m_instances, sheet->m_instances );
}


void SCH_SHEET::AddPin( SCH_SHEET_PIN* aSheetPin )
{
    wxASSERT( aSheetPin != NULL );
    wxASSERT( aSheetPin->Type() == SCH_SHEET_PIN_T );

    aSheetPin->SetParent( this );
    m_pins.push_back( aSheetPin );
    renumberPins();
}


void SCH_SHEET::RemovePin( SCH_SHEET_PIN* aSheetPin )
{
    wxASSERT( aSheetPin != NULL );
    wxASSERT( aSheetPin->Type() == SCH_SHEET_PIN_T );

    for( auto i = m_pins.begin();  i < m_pins.end();  ++i )
    {
        if( *i == aSheetPin )
        {
            m_pins.erase( i );
            renumberPins();
            return;
        }
    }
}


bool SCH_SHEET::HasPin( const wxString& aName )
{
    for( SCH_SHEET_PIN* pin : m_pins )
    {
        if( pin->GetText().CmpNoCase( aName ) == 0 )
            return true;
    }

    return false;
}


bool SCH_SHEET::doIsConnected( const wxPoint& aPosition ) const
{
    for( SCH_SHEET_PIN* sheetPin : m_pins )
    {
        if( sheetPin->GetPosition() == aPosition )
            return true;
    }

    return false;
}


bool SCH_SHEET::IsVerticalOrientation() const
{
    int leftRight = 0;
    int topBottom = 0;

    for( SCH_SHEET_PIN* pin : m_pins )
    {
        switch( pin->GetEdge() )
        {
        case SHEET_LEFT_SIDE:   leftRight++; break;
        case SHEET_RIGHT_SIDE:  leftRight++; break;
        case SHEET_TOP_SIDE:    topBottom++; break;
        case SHEET_BOTTOM_SIDE: topBottom++; break;
        default:                             break;
        }
    }

    return topBottom > 0 && leftRight == 0;
}


bool SCH_SHEET::HasUndefinedPins()
{
    for( SCH_SHEET_PIN* pin : m_pins )
    {
        /* Search the schematic for a hierarchical label corresponding to this sheet label. */
        const SCH_HIERLABEL* HLabel = nullptr;
        for( auto aItem : m_screen->Items().OfType( SCH_HIER_LABEL_T ) )
        {
            if( !pin->GetText().CmpNoCase( static_cast<SCH_HIERLABEL*>( aItem )->GetText() ) )
            {
                HLabel = static_cast<SCH_HIERLABEL*>( aItem );
                break;
            }
        }

        if( HLabel == nullptr ) // Corresponding hierarchical label not found.
            return true;
    }

    return false;
}


int SCH_SHEET::GetMinWidth() const
{
    int width = Mils2iu( MIN_SHEET_WIDTH );

    for( size_t i = 0; i < m_pins.size();  i++ )
    {
        int edge = m_pins[i]->GetEdge();
        EDA_RECT pinRect = m_pins[i]->GetBoundingBox();

        wxASSERT( edge != SHEET_UNDEFINED_SIDE );

        if( edge == SHEET_TOP_SIDE || edge == SHEET_BOTTOM_SIDE )
        {
            if( width < pinRect.GetRight() - m_pos.x )
                width = pinRect.GetRight() - m_pos.x;
        }
        else
        {
            if( width < pinRect.GetWidth() )
                width = pinRect.GetWidth();

            for( size_t j = 0; j < m_pins.size(); j++ )
            {
                // Check for pin directly across from the current pin.
                if( (i == j) || (m_pins[i]->GetPosition().y != m_pins[j]->GetPosition().y) )
                    continue;

                if( width < pinRect.GetWidth() + m_pins[j]->GetBoundingBox().GetWidth() )
                {
                    width = pinRect.GetWidth() + m_pins[j]->GetBoundingBox().GetWidth();
                    break;
                }
            }
        }
    }

    return width;
}


int SCH_SHEET::GetMinHeight() const
{
    int height = Mils2iu( MIN_SHEET_HEIGHT );

    for( size_t i = 0; i < m_pins.size();  i++ )
    {
        int edge = m_pins[i]->GetEdge();
        EDA_RECT pinRect = m_pins[i]->GetBoundingBox();

        // Make sure pin is on top or bottom side of sheet.
        if( edge == SHEET_RIGHT_SIDE || edge == SHEET_LEFT_SIDE )
        {
            if( height < pinRect.GetBottom() - m_pos.y )
                height = pinRect.GetBottom() - m_pos.y;
        }
        else
        {
            if( height < pinRect.GetHeight() )
                height = pinRect.GetHeight();

            for( size_t j = 0; j < m_pins.size(); j++ )
            {
                // Check for pin directly above or below the current pin.
                if( (i == j) || (m_pins[i]->GetPosition().x != m_pins[j]->GetPosition().x) )
                    continue;

                if( height < pinRect.GetHeight() + m_pins[j]->GetBoundingBox().GetHeight() )
                {
                    height = pinRect.GetHeight() + m_pins[j]->GetBoundingBox().GetHeight();
                    break;
                }
            }
        }
    }

    return height;
}


void SCH_SHEET::CleanupSheet()
{
    auto i = m_pins.begin();

    while( i != m_pins.end() )
    {
        /* Search the schematic for a hierarchical label corresponding to this sheet label. */
        const SCH_HIERLABEL* HLabel = NULL;

        for( SCH_ITEM* aItem : m_screen->Items().OfType( SCH_HIER_LABEL_T ) )
        {
            if( (*i)->GetText().CmpNoCase( static_cast<SCH_HIERLABEL*>( aItem )->GetText() ) == 0 )
            {
                HLabel = static_cast<SCH_HIERLABEL*>( aItem );
                break;
            }
        }

        if( HLabel == NULL )   // Hlabel not found: delete sheet label.
            i = m_pins.erase( i );
        else
            ++i;
    }
}


SCH_SHEET_PIN* SCH_SHEET::GetPin( const wxPoint& aPosition )
{
    for( SCH_SHEET_PIN* pin : m_pins )
    {
        if( pin->HitTest( aPosition ) )
            return pin;
    }

    return NULL;
}


int SCH_SHEET::GetPenWidth() const
{
    return std::max( GetBorderWidth(), 1 );
}


void SCH_SHEET::AutoplaceFields( SCH_SCREEN* aScreen, bool aManual )
{
    wxSize textSize = m_fields[ SHEETNAME ].GetTextSize();
    int    borderMargin = KiROUND( GetPenWidth() / 2.0 ) + 4;
    int    margin = borderMargin + KiROUND( std::max( textSize.x, textSize.y ) * 0.5 );

    if( IsVerticalOrientation() )
    {
        m_fields[ SHEETNAME ].SetTextPos( m_pos + wxPoint( -margin, m_size.y ) );
        m_fields[ SHEETNAME ].SetHorizJustify( GR_TEXT_HJUSTIFY_LEFT );
        m_fields[ SHEETNAME ].SetVertJustify(GR_TEXT_VJUSTIFY_BOTTOM );
        m_fields[ SHEETNAME ].SetTextAngle( TEXT_ANGLE_VERT );
    }
    else
    {
        m_fields[ SHEETNAME ].SetTextPos( m_pos + wxPoint( 0, -margin ) );
        m_fields[ SHEETNAME ].SetHorizJustify( GR_TEXT_HJUSTIFY_LEFT );
        m_fields[ SHEETNAME ].SetVertJustify(GR_TEXT_VJUSTIFY_BOTTOM );
        m_fields[ SHEETNAME ].SetTextAngle( TEXT_ANGLE_HORIZ );
    }

    textSize = m_fields[ SHEETFILENAME ].GetTextSize();
    margin = borderMargin + KiROUND( std::max( textSize.x, textSize.y ) * 0.4 );

    if( IsVerticalOrientation() )
    {
        m_fields[ SHEETFILENAME ].SetTextPos( m_pos + wxPoint( m_size.x + margin, m_size.y ) );
        m_fields[ SHEETFILENAME ].SetHorizJustify( GR_TEXT_HJUSTIFY_LEFT );
        m_fields[ SHEETFILENAME ].SetVertJustify(GR_TEXT_VJUSTIFY_TOP );
        m_fields[ SHEETFILENAME ].SetTextAngle( TEXT_ANGLE_VERT );
    }
    else
    {
        m_fields[ SHEETFILENAME ].SetTextPos( m_pos + wxPoint( 0, m_size.y + margin ) );
        m_fields[ SHEETFILENAME ].SetHorizJustify( GR_TEXT_HJUSTIFY_LEFT );
        m_fields[ SHEETFILENAME ].SetVertJustify(GR_TEXT_VJUSTIFY_TOP );
        m_fields[ SHEETFILENAME ].SetTextAngle( TEXT_ANGLE_HORIZ );
    }

    m_fieldsAutoplaced = FIELDS_AUTOPLACED_AUTO;
}


void SCH_SHEET::ViewGetLayers( int aLayers[], int& aCount ) const
{
    aCount      = 4;
    aLayers[0]  = LAYER_HIERLABEL;
    aLayers[1]  = LAYER_SHEET;
    aLayers[2]  = LAYER_SHEET_BACKGROUND;
    aLayers[3]  = LAYER_SELECTION_SHADOWS;
}


const EDA_RECT SCH_SHEET::GetBodyBoundingBox() const
{
    wxPoint  end;
    EDA_RECT box( m_pos, m_size );
    int      lineWidth = GetPenWidth();
    int      textLength = 0;

    // Calculate bounding box X size:
    end.x = std::max( m_size.x, textLength );

    // Calculate bounding box pos:
    end.y = m_size.y;
    end += m_pos;

    box.SetEnd( end );
    box.Inflate( lineWidth / 2 );

    return box;
}


const EDA_RECT SCH_SHEET::GetBoundingBox() const
{
    EDA_RECT box = GetBodyBoundingBox();

    for( const SCH_FIELD& field : m_fields )
        box.Merge( field.GetBoundingBox() );

    return box;
}


wxPoint SCH_SHEET::GetRotationCenter() const
{
    EDA_RECT box( m_pos, m_size );
    return box.GetCenter();
}


int SCH_SHEET::SymbolCount() const
{
    int n = 0;

    if( m_screen )
    {
        for( SCH_ITEM* aItem : m_screen->Items().OfType( SCH_COMPONENT_T ) )
        {
            SCH_COMPONENT* symbol = (SCH_COMPONENT*) aItem;

            if( symbol->GetField( VALUE_FIELD )->GetText().GetChar( 0 ) != '#' )
                n++;
        }

        for( SCH_ITEM* aItem : m_screen->Items().OfType( SCH_SHEET_T ) )
            n += static_cast<const SCH_SHEET*>( aItem )->SymbolCount();
    }

    return n;
}


bool SCH_SHEET::SearchHierarchy( const wxString& aFilename, SCH_SCREEN** aScreen )
{
    if( m_screen )
    {
        // Only check the root sheet once and don't recurse.
        if( !GetParent() )
        {
            if( m_screen && m_screen->GetFileName().Cmp( aFilename ) == 0 )
            {
                *aScreen = m_screen;
                return true;
            }
        }

        for( auto aItem : m_screen->Items().OfType( SCH_SHEET_T ) )
        {
            SCH_SHEET*  sheet  = static_cast<SCH_SHEET*>( aItem );
            SCH_SCREEN* screen = sheet->m_screen;

            // Must use the screen's path (which is always absolute) rather than the
            // sheet's (which could be relative).
            if( screen && screen->GetFileName().Cmp( aFilename ) == 0 )
            {
                *aScreen = screen;
                return true;
            }

            if( sheet->SearchHierarchy( aFilename, aScreen ) )
                return true;
        }
    }

    return false;
}


bool SCH_SHEET::LocatePathOfScreen( SCH_SCREEN* aScreen, SCH_SHEET_PATH* aList )
{
    if( m_screen )
    {
        aList->push_back( this );

        if( m_screen == aScreen )
            return true;

        for( auto item : m_screen->Items().OfType( SCH_SHEET_T ) )
        {
            SCH_SHEET* sheet = static_cast<SCH_SHEET*>( item );

            if( sheet->LocatePathOfScreen( aScreen, aList ) )
            {
                return true;
            }
        }

        aList->pop_back();
    }

    return false;
}


int SCH_SHEET::CountSheets() const
{
    int count = 1; //1 = this!!

    if( m_screen )
    {
        for( auto aItem : m_screen->Items().OfType( SCH_SHEET_T ) )
            count += static_cast<SCH_SHEET*>( aItem )->CountSheets();
    }

    return count;
}


void SCH_SHEET::GetMsgPanelInfo( EDA_DRAW_FRAME* aFrame, MSG_PANEL_ITEMS& aList )
{
    aList.emplace_back( _( "Sheet Name" ), m_fields[ SHEETNAME ].GetText() );

    if( SCH_EDIT_FRAME* schframe = dynamic_cast<SCH_EDIT_FRAME*>( aFrame ) )
    {
        SCH_SHEET_PATH path = schframe->GetCurrentSheet();
        path.push_back( this );

        aList.emplace_back( _( "Hierarchical Path" ), path.PathHumanReadable( false ) );
    }

    aList.emplace_back( _( "File Name" ), m_fields[ SHEETFILENAME ].GetText() );
}


void SCH_SHEET::Rotate( wxPoint aPosition )
{
    wxPoint prev = m_pos;

    RotatePoint( &m_pos, aPosition, 900 );
    RotatePoint( &m_size.x, &m_size.y, 900 );

    if( m_size.x < 0 )
    {
        m_pos.x += m_size.x;
        m_size.x = -m_size.x;
    }

    if( m_size.y < 0 )
    {
        m_pos.y += m_size.y;
        m_size.y = -m_size.y;
    }

    // Pins must be rotated first as that's how we determine vertical vs horizontal
    // orientation for auto-placement
    for( SCH_SHEET_PIN* sheetPin : m_pins )
        sheetPin->Rotate( aPosition );

    if( m_fieldsAutoplaced == FIELDS_AUTOPLACED_AUTO )
    {
        AutoplaceFields( /* aScreen */ NULL, /* aManual */ false );
    }
    else
    {
        // Move the fields to the new position because the component itself has moved.
        for( SCH_FIELD& field : m_fields )
        {
            wxPoint pos = field.GetTextPos();
            pos.x -= prev.x - m_pos.x;
            pos.y -= prev.y - m_pos.y;
            field.SetTextPos( pos );
        }
    }
}


void SCH_SHEET::MirrorX( int aXaxis_position )
{
    MIRROR( m_pos.y, aXaxis_position );
    m_pos.y -= m_size.y;

    for( SCH_SHEET_PIN* sheetPin : m_pins )
        sheetPin->MirrorX( aXaxis_position );
}


void SCH_SHEET::MirrorY( int aYaxis_position )
{
    MIRROR( m_pos.x, aYaxis_position );
    m_pos.x -= m_size.x;

    for( SCH_SHEET_PIN* sheetPin : m_pins )
        sheetPin->MirrorY( aYaxis_position );
}


void SCH_SHEET::SetPosition( const wxPoint& aPosition )
{
    // Remember the sheet and all pin sheet positions must be
    // modified. So use Move function to do that.
    Move( aPosition - m_pos );
}


void SCH_SHEET::Resize( const wxSize& aSize )
{
    if( aSize == m_size )
        return;

    m_size = aSize;

    // Move the fields if we're in autoplace mode
    if( m_fieldsAutoplaced == FIELDS_AUTOPLACED_AUTO )
        AutoplaceFields( /* aScreen */ NULL, /* aManual */ false );

    // Move the sheet labels according to the new sheet size.
    for( SCH_SHEET_PIN* sheetPin : m_pins )
        sheetPin->ConstrainOnEdge( sheetPin->GetPosition() );
}


bool SCH_SHEET::Matches( wxFindReplaceData& aSearchData, void* aAuxData )
{
    wxLogTrace( traceFindItem, wxT( "  item " ) + GetSelectMenuText( EDA_UNITS::MILLIMETRES ) );

    // Sheets are searchable via the child field and pin item text.
    return false;
}


void SCH_SHEET::renumberPins()
{
    int id = 2;

    for( SCH_SHEET_PIN* pin : m_pins )
    {
        pin->SetNumber( id );
        id++;
    }
}


void SCH_SHEET::GetEndPoints( std::vector <DANGLING_END_ITEM>& aItemList )
{
    for( SCH_SHEET_PIN* sheetPin : m_pins )
    {
        wxCHECK2_MSG( sheetPin->Type() == SCH_SHEET_PIN_T, continue,
                      wxT( "Invalid item in schematic sheet pin list.  Bad programmer!" ) );

        sheetPin->GetEndPoints( aItemList );
    }
}


bool SCH_SHEET::UpdateDanglingState( std::vector<DANGLING_END_ITEM>& aItemList,
                                     const SCH_SHEET_PATH* aPath )
{
    bool changed = false;

    for( SCH_SHEET_PIN* sheetPin : m_pins )
        changed |= sheetPin->UpdateDanglingState( aItemList );

    return changed;
}


std::vector<wxPoint> SCH_SHEET::GetConnectionPoints() const
{
    std::vector<wxPoint> retval;

    for( SCH_SHEET_PIN* sheetPin : m_pins )
        retval.push_back( sheetPin->GetPosition() );

    return retval;
}


SEARCH_RESULT SCH_SHEET::Visit( INSPECTOR aInspector, void* testData, const KICAD_T aFilterTypes[] )
{
    KICAD_T stype;

    for( const KICAD_T* p = aFilterTypes;  (stype = *p) != EOT;   ++p )
    {
        // If caller wants to inspect my type
        if( stype == SCH_LOCATE_ANY_T || stype == Type() )
        {
            if( SEARCH_RESULT::QUIT == aInspector( this, NULL ) )
                return SEARCH_RESULT::QUIT;
        }

        if( stype == SCH_LOCATE_ANY_T || stype == SCH_FIELD_T )
        {
            // Test the sheet fields.
            for( SCH_FIELD& field : m_fields )
            {
                if( SEARCH_RESULT::QUIT == aInspector( &field, this ) )
                    return SEARCH_RESULT::QUIT;
            }
        }

        if( stype == SCH_LOCATE_ANY_T || stype == SCH_SHEET_PIN_T )
        {
            // Test the sheet labels.
            for( SCH_SHEET_PIN* sheetPin : m_pins )
            {
                if( SEARCH_RESULT::QUIT == aInspector( sheetPin, this ) )
                    return SEARCH_RESULT::QUIT;
            }
        }
    }

    return SEARCH_RESULT::CONTINUE;
}


void SCH_SHEET::RunOnChildren( const std::function<void( SCH_ITEM* )>& aFunction )
{
    for( SCH_FIELD& field : m_fields )
        aFunction( &field );

    for( SCH_SHEET_PIN* pin : m_pins )
        aFunction( pin );
}


wxString SCH_SHEET::GetSelectMenuText( EDA_UNITS aUnits ) const
{
    return wxString::Format( _( "Hierarchical Sheet %s" ),
                             m_fields[ SHEETNAME ].GetText() );
}


BITMAP_DEF SCH_SHEET::GetMenuImage() const
{
    return add_hierarchical_subsheet_xpm;
}


bool SCH_SHEET::HitTest( const wxPoint& aPosition, int aAccuracy ) const
{
    EDA_RECT rect = GetBodyBoundingBox();

    rect.Inflate( aAccuracy );

    return rect.Contains( aPosition );
}


bool SCH_SHEET::HitTest( const EDA_RECT& aRect, bool aContained, int aAccuracy ) const
{
    EDA_RECT rect = aRect;

    rect.Inflate( aAccuracy );

    if( aContained )
        return rect.Contains( GetBodyBoundingBox() );

    return rect.Intersects( GetBodyBoundingBox() );
}


void SCH_SHEET::Plot( PLOTTER* aPlotter )
{
    wxString msg;
    wxPoint  pos;
    auto*    settings = dynamic_cast<KIGFX::SCH_RENDER_SETTINGS*>( aPlotter->RenderSettings() );
    bool     override = settings ? settings->m_OverrideItemColors : false;
    COLOR4D  borderColor = GetBorderColor();
    COLOR4D  backgroundColor = GetBackgroundColor();

    if( override || borderColor == COLOR4D::UNSPECIFIED )
        borderColor = aPlotter->RenderSettings()->GetLayerColor( LAYER_SHEET );

    if( override || backgroundColor == COLOR4D::UNSPECIFIED )
        backgroundColor = aPlotter->RenderSettings()->GetLayerColor( LAYER_SHEET_BACKGROUND );

    aPlotter->SetColor( backgroundColor );
    // Do not fill shape in B&W mode, otherwise texts are unreadable
    bool fill = aPlotter->GetColorMode();

    aPlotter->Rect( m_pos, m_pos + m_size, fill ? FILL_TYPE::FILLED_SHAPE : FILL_TYPE::NO_FILL,
                    1.0 );

    aPlotter->SetColor( borderColor );

    int penWidth = std::max( GetPenWidth(), aPlotter->RenderSettings()->GetMinPenWidth() );
    aPlotter->SetCurrentLineWidth( penWidth );

    aPlotter->MoveTo( m_pos );
    pos = m_pos;
    pos.x += m_size.x;

    aPlotter->LineTo( pos );
    pos.y += m_size.y;

    aPlotter->LineTo( pos );
    pos = m_pos;
    pos.y += m_size.y;

    aPlotter->LineTo( pos );
    aPlotter->FinishTo( m_pos );

    for( SCH_FIELD field : m_fields )
        field.Plot( aPlotter );

    /* Draw texts : SheetLabel */
    for( SCH_SHEET_PIN* sheetPin : m_pins )
        sheetPin->Plot( aPlotter );
}


void SCH_SHEET::Print( const RENDER_SETTINGS* aSettings, const wxPoint& aOffset )
{
    wxDC*       DC = aSettings->GetPrintDC();
    wxPoint     pos = m_pos + aOffset;
    int         lineWidth = std::max( GetPenWidth(), aSettings->GetDefaultPenWidth() );
    const auto* settings = dynamic_cast<const KIGFX::SCH_RENDER_SETTINGS*>( aSettings );
    bool        override = settings && settings->m_OverrideItemColors;
    COLOR4D     border = GetBorderColor();
    COLOR4D     background = GetBackgroundColor();

    if( override || border == COLOR4D::UNSPECIFIED )
        border = aSettings->GetLayerColor( LAYER_SHEET );

    if( override || background == COLOR4D::UNSPECIFIED )
        background = aSettings->GetLayerColor( LAYER_SHEET_BACKGROUND );

    if( GetGRForceBlackPenState() )     // printing in black & white
        background = COLOR4D::UNSPECIFIED;

    if( background != COLOR4D::UNSPECIFIED )
    {
        GRFilledRect( nullptr, DC, pos.x, pos.y, pos.x + m_size.x, pos.y + m_size.y, 0,
                      background, background );
    }

    GRRect( nullptr, DC, pos.x, pos.y, pos.x + m_size.x, pos.y + m_size.y, lineWidth, border );

    for( SCH_FIELD& field : m_fields )
        field.Print( aSettings, aOffset );

    /* Draw text : SheetLabel */
    for( SCH_SHEET_PIN* sheetPin : m_pins )
        sheetPin->Print( aSettings, aOffset );
}


SCH_SHEET& SCH_SHEET::operator=( const SCH_ITEM& aItem )
{
    wxCHECK_MSG( Type() == aItem.Type(), *this,
                 wxT( "Cannot assign object type " ) + aItem.GetClass() + wxT( " to type " ) +
                 GetClass() );

    if( &aItem != this )
    {
        SCH_ITEM::operator=( aItem );

        SCH_SHEET* sheet = (SCH_SHEET*) &aItem;

        m_pos = sheet->m_pos;
        m_size = sheet->m_size;
        m_fields = sheet->m_fields;

        for( SCH_SHEET_PIN* pin : sheet->m_pins )
        {
            m_pins.emplace_back( new SCH_SHEET_PIN( *pin ) );
            m_pins.back()->SetParent( this );
        }

        for( const SCH_SHEET_INSTANCE& instance : sheet->m_instances )
            m_instances.emplace_back( instance );
    }

    return *this;
}


bool SCH_SHEET::operator <( const SCH_ITEM& aItem ) const
{
    if( Type() != aItem.Type() )
        return Type() < aItem.Type();

    auto sheet = static_cast<const SCH_SHEET*>( &aItem );

    if (m_fields[ SHEETNAME ].GetText() != sheet->m_fields[ SHEETNAME ].GetText() )
        return m_fields[ SHEETNAME ].GetText() < sheet->m_fields[ SHEETNAME ].GetText();

    if (m_fields[ SHEETFILENAME ].GetText() != sheet->m_fields[ SHEETFILENAME ].GetText() )
        return m_fields[ SHEETFILENAME ].GetText() < sheet->m_fields[ SHEETFILENAME ].GetText();

    return false;
}


bool SCH_SHEET::AddInstance( const KIID_PATH& aSheetPath )
{
    // a empty sheet path is illegal:
    wxCHECK( aSheetPath.size() > 0, false );

    wxString path;

    for( const SCH_SHEET_INSTANCE& instance : m_instances )
    {
        // if aSheetPath is found, nothing to do:
        if( instance.m_Path == aSheetPath )
            return false;
    }

    SCH_SHEET_INSTANCE instance;

    instance.m_Path = aSheetPath;

    // This entry does not exist: add it with an empty page number.
    m_instances.emplace_back( instance );
    return true;
}


wxString SCH_SHEET::GetPageNumber( const SCH_SHEET_PATH& aInstance ) const
{
    wxString pageNumber;
    KIID_PATH path = aInstance.Path();

    for( const SCH_SHEET_INSTANCE& instance : m_instances )
    {
        if( instance.m_Path == path )
        {
            pageNumber = instance.m_PageNumber;
            break;
        }
    }

    return pageNumber;
}


void SCH_SHEET::SetPageNumber( const SCH_SHEET_PATH& aInstance, const wxString& aPageNumber )
{
    KIID_PATH path = aInstance.Path();

    for( SCH_SHEET_INSTANCE& instance : m_instances )
    {
        if( instance.m_Path == path )
        {
            instance.m_PageNumber = aPageNumber;
            break;
        }
    }
}


int SCH_SHEET::ComparePageNum( const wxString& aPageNumberA, const wxString aPageNumberB )
{
    if( aPageNumberA == aPageNumberB )
        return 1;

    // First sort numerically if the page numbers are integers
    long pageA, pageB;
    bool isIntegerPageA = aPageNumberA.ToLong( &pageA );
    bool isIntegerPageB = aPageNumberB.ToLong( &pageB );

    if( isIntegerPageA && isIntegerPageB )
    {
        if( pageA > pageB )
            return 1;
        else if( pageA == pageB )
            return 0;
        else
            return -1;
    }

    // Numerical page numbers always before strings
    if( isIntegerPageA )
        return -1;
    else if( isIntegerPageB )
        return 1;

    // If not numeric, then sort as strings
    int result = aPageNumberA.Cmp( aPageNumberB );

    if( result == 0 )
        return 0;
    else if( result > 0 )
        return 1;

    return -1;
}


#if defined(DEBUG)

void SCH_SHEET::Show( int nestLevel, std::ostream& os ) const
{
    // XML output:
    wxString s = GetClass();

    NestedSpace( nestLevel, os ) << '<' << s.Lower().mb_str() << ">" << " sheet_name=\""
                                 << TO_UTF8( m_fields[ SHEETNAME ].GetText() ) << '"' << ">\n";

    // show all the pins, and check the linked list integrity
    for( SCH_SHEET_PIN* sheetPin : m_pins )
        sheetPin->Show( nestLevel + 1, os );

    NestedSpace( nestLevel, os ) << "</" << s.Lower().mb_str() << ">\n" << std::flush;
}

#endif
