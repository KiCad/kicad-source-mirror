/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2016 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 1992-2023 Kicad Developers, see AUTHORS.txt for contributors.
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

#include <cstdlib>

#include <bitmaps.h>
#include <core/mirror.h>
#include <core/kicad_algo.h>
#include <sch_draw_panel.h>
#include <trigo.h>
#include <sch_edit_frame.h>
#include <plotters/plotter.h>
#include <string_utils.h>
#include <widgets/msgpanel.h>
#include <math/util.h>      // for KiROUND
#include <sch_sheet.h>
#include <sch_sheet_path.h>
#include <sch_sheet_pin.h>
#include <sch_symbol.h>
#include <sch_painter.h>
#include <schematic.h>
#include <settings/color_settings.h>
#include <trace_helpers.h>
#include <pgm_base.h>
#include <wx/log.h>

// N.B. Do not change these values without transitioning the file format
#define SHEET_NAME_CANONICAL "Sheetname"
#define SHEET_FILE_CANONICAL "Sheetfile"
#define USER_FIELD_CANONICAL "Field%d"


const wxString SCH_SHEET::GetDefaultFieldName( int aFieldNdx, bool aTranslated )
{
    if( !aTranslated )
    {
        switch( aFieldNdx )
        {
        case  SHEETNAME:     return SHEET_NAME_CANONICAL;
        case  SHEETFILENAME: return SHEET_FILE_CANONICAL;
        default:             return wxString::Format( USER_FIELD_CANONICAL, aFieldNdx );
        }
    }

    // Fixed values for the mandatory fields
    switch( aFieldNdx )
    {
    case  SHEETNAME:     return _( SHEET_NAME_CANONICAL );
    case  SHEETFILENAME: return _( SHEET_FILE_CANONICAL );
    default:             return wxString::Format( _( USER_FIELD_CANONICAL ), aFieldNdx );
    }
}


SCH_SHEET::SCH_SHEET( EDA_ITEM* aParent, const VECTOR2I& aPos, VECTOR2I aSize,
                      FIELDS_AUTOPLACED aAutoplaceFields ) :
        SCH_ITEM( aParent, SCH_SHEET_T )
{
    m_layer = LAYER_SHEET;
    m_pos = aPos;
    m_size = aSize;
    m_screen = nullptr;

    for( int i = 0; i < SHEET_MANDATORY_FIELDS; ++i )
    {
        m_fields.emplace_back( aPos, i, this, GetDefaultFieldName( i ) );
        m_fields.back().SetVisible( true );

        if( i == SHEETNAME )
            m_fields.back().SetLayer( LAYER_SHEETNAME );
        else if( i == SHEETFILENAME )
            m_fields.back().SetLayer( LAYER_SHEETFILENAME );
        else
            m_fields.back().SetLayer( LAYER_SHEETFIELDS );
    }

    m_fieldsAutoplaced = aAutoplaceFields;
    AutoAutoplaceFields( nullptr );

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

    for( SCH_FIELD& field : m_fields )
        field.SetParent( this );

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

    if( m_screen != nullptr )
    {
        m_screen->DecRefCount();

        if( m_screen->GetRefCount() == 0 )
        {
            delete m_screen;
            m_screen = nullptr;
        }
    }

    m_screen = aScreen;

    if( m_screen )
        m_screen->IncRefCount();
}


int SCH_SHEET::GetScreenCount() const
{
    if( m_screen == nullptr )
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
    auto add =
            [&]( const wxString& aVar )
            {
                if( !alg::contains( *aVars, aVar ) )
                    aVars->push_back( aVar );
            };

    for( int i = 0; i < SHEET_MANDATORY_FIELDS; ++i )
        add( m_fields[i].GetCanonicalName().Upper() );

    for( size_t i = SHEET_MANDATORY_FIELDS; i < m_fields.size(); ++i )
        add( m_fields[i].GetName() );

    SCH_SHEET_PATH sheetPath = findSelf();

    if( sheetPath.size() >= 2 )
    {
        sheetPath.pop_back();
        sheetPath.Last()->GetContextualTextVars( aVars );
    }
    else if( Schematic() )
    {
        Schematic()->GetContextualTextVars( aVars );
    }

    add( wxT( "#" ) );
    add( wxT( "##" ) );
    add( wxT( "SHEETPATH" ) );

    m_screen->GetTitleBlock().GetContextualTextVars( aVars );
}


bool SCH_SHEET::ResolveTextVar( const SCH_SHEET_PATH* aPath, wxString* token, int aDepth ) const
{
    if( !Schematic() )
        return false;

    if( token->Contains( ':' ) )
    {
        if( Schematic()->ResolveCrossReference( token, aDepth + 1 ) )
            return true;
    }

    for( int i = 0; i < SHEET_MANDATORY_FIELDS; ++i )
    {
        if( token->IsSameAs( m_fields[i].GetCanonicalName().Upper() ) )
        {
            *token = m_fields[i].GetShownText( nullptr, false, aDepth + 1 );
            return true;
        }
    }

    for( size_t i = SHEET_MANDATORY_FIELDS; i < m_fields.size(); ++i )
    {
        if( token->IsSameAs( m_fields[i].GetName() ) )
        {
            *token = m_fields[i].GetShownText( nullptr, false, aDepth + 1 );
            return true;
        }
    }

    PROJECT *project = &Schematic()->Prj();

    // We cannot resolve text variables initially on load as we need to first load the screen and
    // then parse the hierarchy.  So skip the resolution if the screen isn't set yet
    if( m_screen && m_screen->GetTitleBlock().TextVarResolver( token, project ) )
    {
        return true;
    }

    if( token->IsSameAs( wxT( "#" ) ) )
    {
        *token = wxString::Format( "%s", findSelf().GetPageNumber() );
        return true;
    }
    else if( token->IsSameAs( wxT( "##" ) ) )
    {
        SCH_SHEET_LIST sheetList = Schematic()->GetSheets();
        *token = wxString::Format( wxT( "%d" ), (int) sheetList.size() );
        return true;
    }
    else if( token->IsSameAs( wxT( "SHEETPATH" ) ) )
    {
        *token = findSelf().PathHumanReadable();
        return true;
    }

    // See if parent can resolve it (these will recurse to ancestors)

    SCH_SHEET_PATH sheetPath = aPath ? *aPath : findSelf();

    if( sheetPath.size() >= 2 )
    {
        sheetPath.pop_back();

        if( sheetPath.Last()->ResolveTextVar( &sheetPath, token, aDepth + 1 ) )
            return true;
    }
    else
    {
        if( Schematic()->ResolveTextVar( token, aDepth + 1 ) )
            return true;
    }

    return false;
}


void SCH_SHEET::SwapData( SCH_ITEM* aItem )
{
    SCH_ITEM::SwapFlags( aItem );

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

    for( SCH_FIELD& field : m_fields )
        field.SetParent( this );

    for( SCH_FIELD& field : sheet->m_fields )
        field.SetParent( sheet );

    std::swap( m_borderWidth, sheet->m_borderWidth );
    std::swap( m_borderColor, sheet->m_borderColor );
    std::swap( m_backgroundColor, sheet->m_backgroundColor );
    std::swap( m_instances, sheet->m_instances );
}


void SCH_SHEET::SetFields( const std::vector<SCH_FIELD>& aFields )
{
    m_fields = aFields;
    int next_id = SHEET_MANDATORY_FIELDS;

    for( int ii = 0; ii < int( m_fields.size() ); )
    {
        if( m_fields[ii].GetId() < 0 || m_fields[ii].GetId() >= ssize_t( m_fields.size() ) )
            m_fields[ii].SetId( next_id++ );

        if( m_fields[ii].GetId() != ii )
            std::swap( m_fields[ii], m_fields[m_fields[ii].GetId()]);

        if( m_fields[ii].GetId() == ii )
            ++ii;
    }

    // Make sure that we get the UNIX variant of the file path
    SetFileName( m_fields[SHEETFILENAME].GetText() );
}


void SCH_SHEET::AddPin( SCH_SHEET_PIN* aSheetPin )
{
    wxASSERT( aSheetPin != nullptr );
    wxASSERT( aSheetPin->Type() == SCH_SHEET_PIN_T );

    aSheetPin->SetParent( this );
    m_pins.push_back( aSheetPin );
    renumberPins();
}


void SCH_SHEET::RemovePin( const SCH_SHEET_PIN* aSheetPin )
{
    wxASSERT( aSheetPin != nullptr );
    wxASSERT( aSheetPin->Type() == SCH_SHEET_PIN_T );

    for( auto i = m_pins.begin(); i < m_pins.end(); ++i )
    {
        if( *i == aSheetPin )
        {
            m_pins.erase( i );
            renumberPins();
            return;
        }
    }
}


bool SCH_SHEET::HasPin( const wxString& aName ) const
{
    for( SCH_SHEET_PIN* pin : m_pins )
    {
        if( pin->GetText().Cmp( aName ) == 0 )
            return true;
    }

    return false;
}


bool SCH_SHEET::doIsConnected( const VECTOR2I& aPosition ) const
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
        switch( pin->GetSide() )
        {
        case SHEET_SIDE::LEFT:   leftRight++; break;
        case SHEET_SIDE::RIGHT:  leftRight++; break;
        case SHEET_SIDE::TOP:    topBottom++; break;
        case SHEET_SIDE::BOTTOM: topBottom++; break;
        default:                              break;
        }
    }

    return topBottom > 0 && leftRight == 0;
}


bool SCH_SHEET::HasUndefinedPins() const
{
    for( SCH_SHEET_PIN* pin : m_pins )
    {
        /* Search the schematic for a hierarchical label corresponding to this sheet label. */
        const SCH_HIERLABEL* HLabel = nullptr;

        for( SCH_ITEM* aItem : m_screen->Items().OfType( SCH_HIER_LABEL_T ) )
        {
            if( !pin->GetText().Cmp( static_cast<SCH_HIERLABEL*>( aItem )->GetText() ) )
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


int bumpToNextGrid( const int aVal, const int aDirection )
{
    constexpr int gridSize = schIUScale.MilsToIU( 50 );

    int base = aVal / gridSize;
    int excess = abs( aVal % gridSize );

    if( aDirection > 0 )
    {
        return ( base + 1 ) * gridSize;
    }
    else if( excess > 0 )
    {
        return ( base ) * gridSize;
    }
    else
    {
        return ( base - 1 ) * gridSize;
    }
}


int SCH_SHEET::GetMinWidth( bool aFromLeft ) const
{
    int pinsLeft = m_pos.x + m_size.x;
    int pinsRight = m_pos.x;

    for( size_t i = 0; i < m_pins.size();  i++ )
    {
        SHEET_SIDE edge = m_pins[i]->GetSide();

        if( edge == SHEET_SIDE::TOP || edge == SHEET_SIDE::BOTTOM )
        {
            BOX2I pinRect = m_pins[i]->GetBoundingBox();

            pinsLeft = std::min( pinsLeft, pinRect.GetLeft() );
            pinsRight = std::max( pinsRight, pinRect.GetRight() );
        }
    }

    pinsLeft = bumpToNextGrid( pinsLeft, -1 );
    pinsRight = bumpToNextGrid( pinsRight, 1 );

    int pinMinWidth;

    if( pinsLeft >= pinsRight )
        pinMinWidth = 0;
    else if( aFromLeft )
        pinMinWidth = pinsRight - m_pos.x;
    else
        pinMinWidth = m_pos.x + m_size.x - pinsLeft;

    return std::max( pinMinWidth, schIUScale.MilsToIU( MIN_SHEET_WIDTH ) );
}


int SCH_SHEET::GetMinHeight( bool aFromTop ) const
{
    int pinsTop = m_pos.y + m_size.y;
    int pinsBottom = m_pos.y;

    for( size_t i = 0; i < m_pins.size();  i++ )
    {
        SHEET_SIDE edge = m_pins[i]->GetSide();

        if( edge == SHEET_SIDE::RIGHT || edge == SHEET_SIDE::LEFT )
        {
            BOX2I pinRect = m_pins[i]->GetBoundingBox();

            pinsTop = std::min( pinsTop, pinRect.GetTop() );
            pinsBottom = std::max( pinsBottom, pinRect.GetBottom() );
        }
    }

    pinsTop = bumpToNextGrid( pinsTop, -1 );
    pinsBottom = bumpToNextGrid( pinsBottom, 1 );

    int pinMinHeight;

    if( pinsTop >= pinsBottom )
        pinMinHeight = 0;
    else if( aFromTop )
        pinMinHeight = pinsBottom - m_pos.y;
    else
        pinMinHeight = m_pos.y + m_size.y - pinsTop;

    return std::max( pinMinHeight, schIUScale.MilsToIU( MIN_SHEET_HEIGHT ) );
}


void SCH_SHEET::CleanupSheet()
{
    std::vector<SCH_SHEET_PIN*> pins = m_pins;

    m_pins.clear();

    for( SCH_SHEET_PIN* pin : pins )
    {
        /* Search the schematic for a hierarchical label corresponding to this sheet label. */
        const SCH_HIERLABEL* HLabel = nullptr;

        for( SCH_ITEM* aItem : m_screen->Items().OfType( SCH_HIER_LABEL_T ) )
        {
            if( pin->GetText().CmpNoCase( static_cast<SCH_HIERLABEL*>( aItem )->GetText() ) == 0 )
            {
                HLabel = static_cast<SCH_HIERLABEL*>( aItem );
                break;
            }
        }

        if( HLabel )
            m_pins.push_back( pin );
    }
}


SCH_SHEET_PIN* SCH_SHEET::GetPin( const VECTOR2I& aPosition )
{
    for( SCH_SHEET_PIN* pin : m_pins )
    {
        if( pin->HitTest( aPosition ) )
            return pin;
    }

    return nullptr;
}


int SCH_SHEET::GetPenWidth() const
{
    if( GetBorderWidth() > 0 )
        return GetBorderWidth();

    if( Schematic() )
        return Schematic()->Settings().m_DefaultLineWidth;

    return schIUScale.MilsToIU( DEFAULT_LINE_WIDTH_MILS );
}


void SCH_SHEET::AutoplaceFields( SCH_SCREEN* aScreen, bool /* aManual */ )
{
    VECTOR2I textSize = m_fields[SHEETNAME].GetTextSize();
    int      borderMargin = KiROUND( GetPenWidth() / 2.0 ) + 4;
    int      margin = borderMargin + KiROUND( std::max( textSize.x, textSize.y ) * 0.5 );

    if( IsVerticalOrientation() )
    {
        m_fields[SHEETNAME].SetTextPos( m_pos + VECTOR2I( -margin, m_size.y ) );
        m_fields[ SHEETNAME ].SetHorizJustify( GR_TEXT_H_ALIGN_LEFT );
        m_fields[ SHEETNAME ].SetVertJustify( GR_TEXT_V_ALIGN_BOTTOM );
        m_fields[ SHEETNAME ].SetTextAngle( ANGLE_VERTICAL );
    }
    else
    {
        m_fields[SHEETNAME].SetTextPos( m_pos + VECTOR2I( 0, -margin ) );
        m_fields[ SHEETNAME ].SetHorizJustify( GR_TEXT_H_ALIGN_LEFT );
        m_fields[ SHEETNAME ].SetVertJustify( GR_TEXT_V_ALIGN_BOTTOM );
        m_fields[ SHEETNAME ].SetTextAngle( ANGLE_HORIZONTAL );
    }

    textSize = m_fields[ SHEETFILENAME ].GetTextSize();
    margin = borderMargin + KiROUND( std::max( textSize.x, textSize.y ) * 0.4 );

    if( IsVerticalOrientation() )
    {
        m_fields[SHEETFILENAME].SetTextPos( m_pos + VECTOR2I( m_size.x + margin, m_size.y ) );
        m_fields[ SHEETFILENAME ].SetHorizJustify( GR_TEXT_H_ALIGN_LEFT );
        m_fields[ SHEETFILENAME ].SetVertJustify( GR_TEXT_V_ALIGN_TOP );
        m_fields[ SHEETFILENAME ].SetTextAngle( ANGLE_VERTICAL );
    }
    else
    {
        m_fields[SHEETFILENAME].SetTextPos( m_pos + VECTOR2I( 0, m_size.y + margin ) );
        m_fields[ SHEETFILENAME ].SetHorizJustify( GR_TEXT_H_ALIGN_LEFT );
        m_fields[ SHEETFILENAME ].SetVertJustify( GR_TEXT_V_ALIGN_TOP );
        m_fields[ SHEETFILENAME ].SetTextAngle( ANGLE_HORIZONTAL );
    }

    m_fieldsAutoplaced = FIELDS_AUTOPLACED_AUTO;
}


void SCH_SHEET::ViewGetLayers( int aLayers[], int& aCount ) const
{
    aCount     = 8;
    aLayers[0] = LAYER_DANGLING;     // Sheet pins are drawn by their parent sheet, so the
                                     //   parent needs to draw to LAYER_DANGLING
    aLayers[1] = LAYER_HIERLABEL;
    aLayers[2] = LAYER_SHEETNAME;
    aLayers[3] = LAYER_SHEETFILENAME;
    aLayers[4] = LAYER_SHEETFIELDS;
    aLayers[5] = LAYER_SHEET;
    aLayers[6] = LAYER_SHEET_BACKGROUND;
    aLayers[7] = LAYER_SELECTION_SHADOWS;
}


const BOX2I SCH_SHEET::GetBodyBoundingBox() const
{
    VECTOR2I end;
    BOX2I    box( m_pos, m_size );
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


const BOX2I SCH_SHEET::GetBoundingBox() const
{
    BOX2I bbox = GetBodyBoundingBox();

    for( const SCH_FIELD& field : m_fields )
        bbox.Merge( field.GetBoundingBox() );

    return bbox;
}


VECTOR2I SCH_SHEET::GetRotationCenter() const
{
    BOX2I box( m_pos, m_size );
    return box.GetCenter();
}


int SCH_SHEET::SymbolCount() const
{
    int n = 0;

    if( m_screen )
    {
        for( SCH_ITEM* aItem : m_screen->Items().OfType( SCH_SYMBOL_T ) )
        {
            SCH_SYMBOL* symbol = (SCH_SYMBOL*) aItem;

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

        for( SCH_ITEM* aItem : m_screen->Items().OfType( SCH_SHEET_T ) )
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

        for( EDA_ITEM* item : m_screen->Items().OfType( SCH_SHEET_T ) )
        {
            SCH_SHEET* sheet = static_cast<SCH_SHEET*>( item );

            if( sheet->LocatePathOfScreen( aScreen, aList ) )
                return true;
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
        for( SCH_ITEM* aItem : m_screen->Items().OfType( SCH_SHEET_T ) )
            count += static_cast<SCH_SHEET*>( aItem )->CountSheets();
    }

    return count;
}


void SCH_SHEET::GetMsgPanelInfo( EDA_DRAW_FRAME* aFrame, std::vector<MSG_PANEL_ITEM>& aList )
{
    // Don't use GetShownText(); we want to see the variable references here
    aList.emplace_back( _( "Sheet Name" ),
                        KIUI::EllipsizeStatusText( aFrame, m_fields[ SHEETNAME ].GetText() ) );

    if( SCH_EDIT_FRAME* schframe = dynamic_cast<SCH_EDIT_FRAME*>( aFrame ) )
    {
        SCH_SHEET_PATH path = schframe->GetCurrentSheet();
        path.push_back( this );

        aList.emplace_back( _( "Hierarchical Path" ), path.PathHumanReadable( false, true ) );
    }

    // Don't use GetShownText(); we want to see the variable references here
    aList.emplace_back( _( "File Name" ),
                        KIUI::EllipsizeStatusText( aFrame, m_fields[ SHEETFILENAME ].GetText() ) );
}


void SCH_SHEET::SetPositionIgnoringPins( const VECTOR2I& aPosition )
{
    VECTOR2I delta = aPosition - m_pos;

    m_pos = aPosition;

    for( SCH_FIELD& field : m_fields )
        field.Move( delta );
}


void SCH_SHEET::Move( const VECTOR2I& aMoveVector )
{
    m_pos += aMoveVector;

    for( SCH_SHEET_PIN* pin : m_pins )
        pin->Move( aMoveVector );

    for( SCH_FIELD& field : m_fields )
        field.Move( aMoveVector );
}


void SCH_SHEET::Rotate( const VECTOR2I& aCenter )
{
    VECTOR2I prev = m_pos;

    RotatePoint( m_pos, aCenter, ANGLE_90 );
    RotatePoint( &m_size.x, &m_size.y, ANGLE_90 );

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
        sheetPin->Rotate( aCenter );

    if( m_fieldsAutoplaced == FIELDS_AUTOPLACED_AUTO )
    {
        AutoplaceFields( /* aScreen */ nullptr, /* aManual */ false );
    }
    else
    {
        // Move the fields to the new position because the parent itself has moved.
        for( SCH_FIELD& field : m_fields )
        {
            VECTOR2I pos = field.GetTextPos();
            pos.x -= prev.x - m_pos.x;
            pos.y -= prev.y - m_pos.y;
            field.SetTextPos( pos );
        }
    }
}


void SCH_SHEET::MirrorVertically( int aCenter )
{
    int dy = m_pos.y;

    MIRROR( m_pos.y, aCenter );
    m_pos.y -= m_size.y;
    dy -= m_pos.y;     // 0,dy is the move vector for this transform

    for( SCH_SHEET_PIN* sheetPin : m_pins )
        sheetPin->MirrorVertically( aCenter );

    for( SCH_FIELD& field : m_fields )
    {
        VECTOR2I pos = field.GetTextPos();
        pos.y -= dy;
        field.SetTextPos( pos );
    }
}


void SCH_SHEET::MirrorHorizontally( int aCenter )
{
    int dx = m_pos.x;

    MIRROR( m_pos.x, aCenter );
    m_pos.x -= m_size.x;
    dx -= m_pos.x;     // dx,0 is the move vector for this transform

    for( SCH_SHEET_PIN* sheetPin : m_pins )
        sheetPin->MirrorHorizontally( aCenter );

    for( SCH_FIELD& field : m_fields )
    {
        VECTOR2I pos = field.GetTextPos();
        pos.x -= dx;
        field.SetTextPos( pos );
    }
}


void SCH_SHEET::SetPosition( const VECTOR2I& aPosition )
{
    // Remember the sheet and all pin sheet positions must be
    // modified. So use Move function to do that.
    Move( aPosition - m_pos );
}


void SCH_SHEET::Resize( const VECTOR2I& aSize )
{
    if( aSize == m_size )
        return;

    m_size = aSize;

    // Move the fields if we're in autoplace mode
    if( m_fieldsAutoplaced == FIELDS_AUTOPLACED_AUTO )
        AutoplaceFields( /* aScreen */ nullptr, /* aManual */ false );

    // Move the sheet labels according to the new sheet size.
    for( SCH_SHEET_PIN* sheetPin : m_pins )
        sheetPin->ConstrainOnEdge( sheetPin->GetPosition(), false );
}


bool SCH_SHEET::Matches( const EDA_SEARCH_DATA& aSearchData, void* aAuxData ) const
{
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


SCH_SHEET_PATH SCH_SHEET::findSelf() const
{
    wxCHECK_MSG( Schematic(), SCH_SHEET_PATH(), "Can't call findSelf without a schematic" );

    SCH_SHEET_PATH sheetPath = Schematic()->CurrentSheet();

    while( !sheetPath.empty() && sheetPath.Last() != this )
        sheetPath.pop_back();

    if( sheetPath.empty() )
    {
        // If we weren't in the hierarchy, then we must be a child of the current sheet.
        sheetPath = Schematic()->CurrentSheet();
        sheetPath.push_back( const_cast<SCH_SHEET*>( this ) );
    }

    return sheetPath;
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


std::vector<VECTOR2I> SCH_SHEET::GetConnectionPoints() const
{
    std::vector<VECTOR2I> retval;

    for( SCH_SHEET_PIN* sheetPin : m_pins )
        retval.push_back( sheetPin->GetPosition() );

    return retval;
}


INSPECT_RESULT SCH_SHEET::Visit( INSPECTOR aInspector, void* testData,
                                 const std::vector<KICAD_T>& aScanTypes )
{
    for( KICAD_T scanType : aScanTypes )
    {
        // If caller wants to inspect my type
        if( scanType == SCH_LOCATE_ANY_T || scanType == Type() )
        {
            if( INSPECT_RESULT::QUIT == aInspector( this, nullptr ) )
                return INSPECT_RESULT::QUIT;
        }

        if( scanType == SCH_LOCATE_ANY_T || scanType == SCH_FIELD_T )
        {
            // Test the sheet fields.
            for( SCH_FIELD& field : m_fields )
            {
                if( INSPECT_RESULT::QUIT == aInspector( &field, this ) )
                    return INSPECT_RESULT::QUIT;
            }
        }

        if( scanType == SCH_LOCATE_ANY_T || scanType == SCH_SHEET_PIN_T )
        {
            // Test the sheet labels.
            for( SCH_SHEET_PIN* sheetPin : m_pins )
            {
                if( INSPECT_RESULT::QUIT == aInspector( sheetPin, this ) )
                    return INSPECT_RESULT::QUIT;
            }
        }
    }

    return INSPECT_RESULT::CONTINUE;
}


void SCH_SHEET::RunOnChildren( const std::function<void( SCH_ITEM* )>& aFunction )
{
    for( SCH_FIELD& field : m_fields )
        aFunction( &field );

    for( SCH_SHEET_PIN* pin : m_pins )
        aFunction( pin );
}


wxString SCH_SHEET::GetItemDescription( UNITS_PROVIDER* aUnitsProvider ) const
{
    return wxString::Format( _( "Hierarchical Sheet %s" ),
                             KIUI::EllipsizeMenuText( m_fields[ SHEETNAME ].GetText() ) );
}


BITMAPS SCH_SHEET::GetMenuImage() const
{
    return BITMAPS::add_hierarchical_subsheet;
}


bool SCH_SHEET::HitTest( const VECTOR2I& aPosition, int aAccuracy ) const
{
    BOX2I rect = GetBodyBoundingBox();

    rect.Inflate( aAccuracy );

    return rect.Contains( aPosition );
}


bool SCH_SHEET::HitTest( const BOX2I& aRect, bool aContained, int aAccuracy ) const
{
    BOX2I rect = aRect;

    rect.Inflate( aAccuracy );

    if( aContained )
        return rect.Contains( GetBodyBoundingBox() );

    return rect.Intersects( GetBodyBoundingBox() );
}


void SCH_SHEET::Plot( PLOTTER* aPlotter, bool aBackground ) const
{
    if( aBackground && !aPlotter->GetColorMode() )
        return;

    auto*    settings = dynamic_cast<KIGFX::SCH_RENDER_SETTINGS*>( aPlotter->RenderSettings() );
    bool     override = settings ? settings->m_OverrideItemColors : false;
    COLOR4D  borderColor = GetBorderColor();
    COLOR4D  backgroundColor = GetBackgroundColor();

    if( override || borderColor == COLOR4D::UNSPECIFIED )
        borderColor = aPlotter->RenderSettings()->GetLayerColor( LAYER_SHEET );

    if( override || backgroundColor == COLOR4D::UNSPECIFIED )
        backgroundColor = aPlotter->RenderSettings()->GetLayerColor( LAYER_SHEET_BACKGROUND );

    if( aBackground && backgroundColor.a > 0.0 )
    {
        aPlotter->SetColor( backgroundColor );
        aPlotter->Rect( m_pos, m_pos + m_size, FILL_T::FILLED_SHAPE, 1 );
    }
    else
    {
        aPlotter->SetColor( borderColor );

        int penWidth = std::max( GetPenWidth(), aPlotter->RenderSettings()->GetMinPenWidth() );
        aPlotter->Rect( m_pos, m_pos + m_size, FILL_T::NO_FILL, penWidth );
    }

    // Make the sheet object a clickable hyperlink (e.g. for PDF plotter)
    std::vector<wxString> properties;

    properties.emplace_back( EDA_TEXT::GotoPageHref( findSelf().GetPageNumber() ) );

    for( const SCH_FIELD& field : GetFields() )
    {
        properties.emplace_back( wxString::Format( wxT( "!%s = %s" ),
                                                   field.GetName(),
                                                   field.GetShownText( false ) ) );
    }

    aPlotter->HyperlinkMenu( GetBoundingBox(), properties );

    // Plot sheet pins
    for( SCH_SHEET_PIN* sheetPin : m_pins )
        sheetPin->Plot( aPlotter, aBackground );

    // Plot the fields
    for( const SCH_FIELD& field : m_fields )
        field.Plot( aPlotter, aBackground );
}


void SCH_SHEET::Print( const RENDER_SETTINGS* aSettings, const VECTOR2I& aOffset )
{
    wxDC*       DC = aSettings->GetPrintDC();
    VECTOR2I    pos = m_pos + aOffset;
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

    if( background.a > 0.0 )
        GRFilledRect( DC, pos, pos + m_size, 0, background, background );

    GRRect( DC, pos, pos + m_size, lineWidth, border );

    for( SCH_FIELD& field : m_fields )
        field.Print( aSettings, aOffset );

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


bool SCH_SHEET::addInstance( const SCH_SHEET_PATH& aSheetPath )
{
    wxCHECK( aSheetPath.IsFullPath(), false );
    wxCHECK( !aSheetPath.Last() || ( aSheetPath.Last()->m_Uuid != m_Uuid ), false );

    for( const SCH_SHEET_INSTANCE& instance : m_instances )
    {
        // if aSheetPath is found, nothing to do:
        if( instance.m_Path == aSheetPath.Path() )
            return false;
    }

    wxLogTrace( traceSchSheetPaths, wxT( "Adding instance `%s` to sheet `%s`." ),
                aSheetPath.Path().AsString(),
                ( GetName().IsEmpty() ) ? wxString( wxT( "root" ) ) : GetName() );

    SCH_SHEET_INSTANCE instance;

    instance.m_Path = aSheetPath.Path();

    // This entry does not exist: add it with an empty page number.
    m_instances.emplace_back( instance );
    return true;
}


bool SCH_SHEET::getInstance( SCH_SHEET_INSTANCE& aInstance, const KIID_PATH& aSheetPath,
                             bool aTestFromEnd ) const
{
    for( const SCH_SHEET_INSTANCE& instance : m_instances )
    {
        if( !aTestFromEnd )
        {
            if( instance.m_Path == aSheetPath )
            {
                aInstance = instance;
                return true;
            }
        }
        else if( instance.m_Path.EndsWith( aSheetPath ) )
        {
            aInstance = instance;
            return true;
        }
    }

    return false;
}


bool SCH_SHEET::HasRootInstance() const
{
    for( const SCH_SHEET_INSTANCE& instance : m_instances )
    {
        if( instance.m_Path.size() == 0 )
            return true;
    }

    return false;
}


const SCH_SHEET_INSTANCE& SCH_SHEET::GetRootInstance() const
{
    for( const SCH_SHEET_INSTANCE& instance : m_instances )
    {
        if( instance.m_Path.size() == 0 )
            return instance;
    }

    wxFAIL;

    static SCH_SHEET_INSTANCE dummy;

    return dummy;
}


wxString SCH_SHEET::getPageNumber( const SCH_SHEET_PATH& aSheetPath ) const
{
    wxCHECK( aSheetPath.IsFullPath(), wxEmptyString );
    wxCHECK( !aSheetPath.Last() || ( aSheetPath.Last()->m_Uuid != m_Uuid ), wxEmptyString );

    wxString pageNumber;
    KIID_PATH path = aSheetPath.Path();

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


void SCH_SHEET::setPageNumber( const SCH_SHEET_PATH& aSheetPath, const wxString& aPageNumber )
{
    wxCHECK( aSheetPath.IsFullPath(), /* void */ );
    wxCHECK( !aSheetPath.Last() || ( aSheetPath.Last()->m_Uuid != m_Uuid ), /* void */ );

    KIID_PATH path = aSheetPath.Path();

    for( SCH_SHEET_INSTANCE& instance : m_instances )
    {
        if( instance.m_Path == path )
        {
            instance.m_PageNumber = aPageNumber;
            break;
        }
    }
}


int SCH_SHEET::ComparePageNum( const wxString& aPageNumberA, const wxString& aPageNumberB )
{
    if( aPageNumberA == aPageNumberB )
        return 0; // A == B

    // First sort numerically if the page numbers are integers
    long pageA, pageB;
    bool isIntegerPageA = aPageNumberA.ToLong( &pageA );
    bool isIntegerPageB = aPageNumberB.ToLong( &pageB );

    if( isIntegerPageA && isIntegerPageB )
    {
        if( pageA < pageB )
            return -1; //A < B
        else
            return 1; // A > B
    }

    // Numerical page numbers always before strings
    if( isIntegerPageA )
        return -1; //A < B
    else if( isIntegerPageB )
        return 1; // A > B

    // If not numeric, then sort as strings using natural sort
    int result = StrNumCmp( aPageNumberA, aPageNumberB );

    // Divide by zero bad.
    wxCHECK( result != 0, 0 );

    result = result / std::abs( result );

    return result;
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

static struct SCH_SHEET_DESC
{
    SCH_SHEET_DESC()
    {
        PROPERTY_MANAGER& propMgr = PROPERTY_MANAGER::Instance();
        REGISTER_TYPE( SCH_SHEET );
        propMgr.InheritsAfter( TYPE_HASH( SCH_SHEET ), TYPE_HASH( SCH_ITEM ) );

        propMgr.AddProperty( new PROPERTY<SCH_SHEET, wxString>( _HKI( "Sheet Name" ),
                             &SCH_SHEET::SetName, &SCH_SHEET::GetName ) );

        propMgr.AddProperty( new PROPERTY<SCH_SHEET, int>( _HKI( "Border Width" ),
                             &SCH_SHEET::SetBorderWidth, &SCH_SHEET::GetBorderWidth,
                             PROPERTY_DISPLAY::PT_SIZE ) );

        propMgr.AddProperty( new PROPERTY<SCH_SHEET, COLOR4D>( _HKI( "Border Color" ),
                             &SCH_SHEET::SetBorderColor, &SCH_SHEET::GetBorderColor ) );

        propMgr.AddProperty( new PROPERTY<SCH_SHEET, COLOR4D>( _HKI( "Background Color" ),
                             &SCH_SHEET::SetBackgroundColor, &SCH_SHEET::GetBackgroundColor ) );
    }
} _SCH_SHEET_DESC;
