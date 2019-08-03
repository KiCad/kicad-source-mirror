/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2016 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 1992-2019 Kicad Developers, see AUTHORS.txt for contributors.
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
 * @file sch_sheet.cpp
 * @brief Implementation of SCH_SHEET class.
 */

#include <fctsys.h>
#include <sch_draw_panel.h>
#include <gr_text.h>
#include <trigo.h>
#include <richio.h>
#include <sch_edit_frame.h>
#include <plotter.h>
#include <kicad_string.h>
#include <msgpanel.h>

#include <sch_sheet.h>
#include <sch_sheet_path.h>
#include <sch_component.h>
#include <netlist_object.h>
#include <trace_helpers.h>


SCH_SHEET::SCH_SHEET( const wxPoint& pos ) :
    SCH_ITEM( NULL, SCH_SHEET_T )
{
    m_Layer = LAYER_SHEET;
    m_pos = pos;
    m_size = wxSize( MIN_SHEET_WIDTH, MIN_SHEET_HEIGHT );
    SetTimeStamp( GetNewTimeStamp() );
    m_sheetNameSize = GetDefaultTextSize();
    m_fileNameSize = GetDefaultTextSize();
    m_screen = NULL;
    m_name.Printf( wxT( "Sheet%8.8lX" ), (long) m_TimeStamp );
    m_fileName.Printf( wxT( "file%8.8lX.sch" ), (long) m_TimeStamp );
}


SCH_SHEET::SCH_SHEET( const SCH_SHEET& aSheet ) :
    SCH_ITEM( aSheet )
{
    m_pos = aSheet.m_pos;
    m_size = aSheet.m_size;
    m_Layer = aSheet.m_Layer;
    SetTimeStamp( aSheet.m_TimeStamp );
    m_sheetNameSize = aSheet.m_sheetNameSize;
    m_fileNameSize = aSheet.m_fileNameSize;
    m_screen = aSheet.m_screen;
    m_name = aSheet.m_name;
    m_fileName = aSheet.m_fileName;
    m_pins = aSheet.m_pins;

    for( size_t i = 0;  i < m_pins.size();  i++ )
        m_pins[i].SetParent( this );

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


SCH_SHEET* SCH_SHEET::GetRootSheet()
{
    SCH_SHEET* sheet = dynamic_cast< SCH_SHEET* >( GetParent() );

    if( sheet == NULL )
        return this;

    // Recurse until a sheet is found with no parent which is the root sheet.
    return sheet->GetRootSheet();
}


void SCH_SHEET::SwapData( SCH_ITEM* aItem )
{
    wxCHECK_RET( aItem->Type() == SCH_SHEET_T,
                 wxString::Format( wxT( "SCH_SHEET object cannot swap data with %s object." ),
                                   GetChars( aItem->GetClass() ) ) );

    SCH_SHEET* sheet = ( SCH_SHEET* ) aItem;

    std::swap( m_pos, sheet->m_pos );
    std::swap( m_size, sheet->m_size );
    std::swap( m_name, sheet->m_name );
    std::swap( m_sheetNameSize, sheet->m_sheetNameSize );
    std::swap( m_fileNameSize, sheet->m_fileNameSize );
    m_pins.swap( sheet->m_pins );

    // Ensure sheet labels have their .m_Parent member pointing really on their
    // parent, after swapping.
    for( SCH_SHEET_PIN& sheetPin : m_pins )
    {
        sheetPin.SetParent( this );
    }

    for( SCH_SHEET_PIN& sheetPin : sheet->m_pins )
    {
        sheetPin.SetParent( sheet );
    }
}


void SCH_SHEET::AddPin( SCH_SHEET_PIN* aSheetPin )
{
    wxASSERT( aSheetPin != NULL );
    wxASSERT( aSheetPin->Type() == SCH_SHEET_PIN_T );

    m_pins.push_back( aSheetPin );
    renumberPins();
}


void SCH_SHEET::RemovePin( SCH_SHEET_PIN* aSheetPin )
{
    wxASSERT( aSheetPin != NULL );
    wxASSERT( aSheetPin->Type() == SCH_SHEET_PIN_T );

    SCH_SHEET_PINS::iterator i;

    for( i = m_pins.begin();  i < m_pins.end();  ++i )
    {
        if( *i == aSheetPin )
        {
            m_pins.erase( i );
            renumberPins();
            return;
        }
    }

    wxLogDebug( wxT( "Fix me: attempt to remove label %s which is not in sheet %s." ),
                GetChars( aSheetPin->GetShownText() ), GetChars( m_name ) );
}


bool SCH_SHEET::HasPin( const wxString& aName )
{
    for( const SCH_SHEET_PIN& pin : m_pins )
    {
        if( pin.GetText().CmpNoCase( aName ) == 0 )
            return true;
    }

    return false;
}


bool SCH_SHEET::IsVerticalOrientation() const
{
    for( const SCH_SHEET_PIN& pin : m_pins )
    {
        if( pin.GetEdge() > 1 )
            return true;
    }
    return false;
}


bool SCH_SHEET::HasUndefinedPins()
{
    for( const SCH_SHEET_PIN& pin : m_pins )
    {
        /* Search the schematic for a hierarchical label corresponding to this sheet label. */
        EDA_ITEM* DrawStruct  = m_screen->GetDrawItems();
        const SCH_HIERLABEL* HLabel = NULL;

        for( ; DrawStruct != NULL; DrawStruct = DrawStruct->Next() )
        {
            if( DrawStruct->Type() != SCH_HIER_LABEL_T )
                continue;

            HLabel = static_cast<SCH_HIERLABEL*>( DrawStruct );

            if( pin.GetText().CmpNoCase( HLabel->GetText() ) == 0 )
                break;  // Found!

            HLabel = NULL;
        }

        if( HLabel == NULL )   // Corresponding hierarchical label not found.
            return true;
    }

    return false;
}


int SCH_SHEET::GetMinWidth() const
{
    int width = MIN_SHEET_WIDTH;

    for( size_t i = 0; i < m_pins.size();  i++ )
    {
        int edge = m_pins[i].GetEdge();
        EDA_RECT pinRect = m_pins[i].GetBoundingBox();

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
                if( (i == j) || (m_pins[i].GetPosition().y != m_pins[j].GetPosition().y) )
                    continue;

                if( width < pinRect.GetWidth() + m_pins[j].GetBoundingBox().GetWidth() )
                {
                    width = pinRect.GetWidth() + m_pins[j].GetBoundingBox().GetWidth();
                    break;
                }
            }
        }
    }

    return width;
}


int SCH_SHEET::GetMinHeight() const
{
    int height = MIN_SHEET_HEIGHT;

    for( size_t i = 0; i < m_pins.size();  i++ )
    {
        int edge = m_pins[i].GetEdge();
        EDA_RECT pinRect = m_pins[i].GetBoundingBox();

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
                if( (i == j) || (m_pins[i].GetPosition().x != m_pins[j].GetPosition().x) )
                    continue;

                if( height < pinRect.GetHeight() + m_pins[j].GetBoundingBox().GetHeight() )
                {
                    height = pinRect.GetHeight() + m_pins[j].GetBoundingBox().GetHeight();
                    break;
                }
            }
        }
    }

    return height;
}


void SCH_SHEET::CleanupSheet()
{
    SCH_SHEET_PINS::iterator i = m_pins.begin();

    while( i != m_pins.end() )
    {
        /* Search the schematic for a hierarchical label corresponding to this sheet label. */
        EDA_ITEM* DrawStruct = m_screen->GetDrawItems();
        const SCH_HIERLABEL* HLabel = NULL;

        for( ; DrawStruct != NULL; DrawStruct = DrawStruct->Next() )
        {
            if( DrawStruct->Type() != SCH_HIER_LABEL_T )
                continue;

            HLabel = static_cast<SCH_HIERLABEL*>( DrawStruct );

            if( i->GetText().CmpNoCase( HLabel->GetText() ) == 0 )
                break;  // Found!

            HLabel = NULL;
        }

        if( HLabel == NULL )   // Hlabel not found: delete sheet label.
            i = m_pins.erase( i );
        else
            ++i;
    }
}


SCH_SHEET_PIN* SCH_SHEET::GetPin( const wxPoint& aPosition )
{
    for( SCH_SHEET_PIN& pin : m_pins )
    {
        if( pin.HitTest( aPosition ) )
            return &pin;
    }

    return NULL;
}


int SCH_SHEET::GetPenSize() const
{
    return GetDefaultLineThickness();
}


wxPoint SCH_SHEET::GetSheetNamePosition()
{
    wxPoint pos = m_pos;
    int     margin = KiROUND( GetPenSize() / 2.0 + 4 + m_sheetNameSize * 0.3 );

    if( IsVerticalOrientation() )
    {
        pos.x -= margin;
        pos.y += m_size.y;
    }
    else
    {
        pos.y -= margin;
    }

    return pos;
}


wxPoint SCH_SHEET::GetFileNamePosition()
{
    wxPoint  pos = m_pos;
    int      margin = KiROUND( GetPenSize() / 2.0 + 4 + m_fileNameSize * 0.4 );

    if( IsVerticalOrientation() )
    {
        pos.x += m_size.x + margin;
        pos.y += m_size.y;
    }
    else
    {
        pos.y += m_size.y + margin;
    }

    return pos;
}


void SCH_SHEET::ViewGetLayers( int aLayers[], int& aCount ) const
{
    aCount      = 4;
    aLayers[0]  = LAYER_HIERLABEL;
    aLayers[1]  = LAYER_SHEET;
    aLayers[2]  = LAYER_SHEET_BACKGROUND;
    aLayers[3]  = LAYER_SELECTION_SHADOWS;
}


void SCH_SHEET::Print( wxDC* aDC, const wxPoint& aOffset )
{
    wxString  Text;
    int       name_orientation;
    wxPoint   pos_sheetname,pos_filename;
    wxPoint   pos = m_pos + aOffset;
    int       lineWidth = GetPenSize();
    int       textWidth;
    wxSize    textSize;
    COLOR4D   color = GetLayerColor( m_Layer );

    GRRect( nullptr, aDC, pos.x, pos.y, pos.x + m_size.x, pos.y + m_size.y, lineWidth, color );

    pos_sheetname = GetSheetNamePosition() + aOffset;
    pos_filename = GetFileNamePosition() + aOffset;

    if( IsVerticalOrientation() )
        name_orientation = TEXT_ANGLE_VERT;
    else
        name_orientation = TEXT_ANGLE_HORIZ;

    /* Draw text : SheetName */
    Text = wxT( "Sheet: " ) + m_name;
    textSize = wxSize( m_sheetNameSize, m_sheetNameSize );
    textWidth = Clamp_Text_PenSize( lineWidth, textSize, false );
    GRText( aDC, pos_sheetname, GetLayerColor( LAYER_SHEETNAME ), Text, name_orientation,
            textSize, GR_TEXT_HJUSTIFY_LEFT, GR_TEXT_VJUSTIFY_BOTTOM, textWidth, false, false );

    /* Draw text : FileName */
    Text = wxT( "File: " ) + m_fileName;
    textSize = wxSize( m_fileNameSize, m_fileNameSize );
    textWidth = Clamp_Text_PenSize( lineWidth, textSize, false );
    GRText( aDC, pos_filename, GetLayerColor( LAYER_SHEETFILENAME ), Text, name_orientation,
            textSize, GR_TEXT_HJUSTIFY_LEFT, GR_TEXT_VJUSTIFY_TOP, textWidth, false, false );

    /* Draw text : SheetLabel */
    for( SCH_SHEET_PIN& sheetPin : m_pins )
        sheetPin.Print( aDC, aOffset );
}


const EDA_RECT SCH_SHEET::GetBoundingBox() const
{
    wxPoint end;
    EDA_RECT box( m_pos, m_size );
    int      lineWidth = GetPenSize();

    // Determine length of texts
    wxString text    = wxT( "Sheet: " ) + m_name;
    int      textlen  = GraphicTextWidth( text, wxSize( m_sheetNameSize, m_sheetNameSize ),
                                          false, false );

    text = wxT( "File: " ) + m_fileName;
    int      textlen2 = GraphicTextWidth( text, wxSize( m_fileNameSize, m_fileNameSize ),
                                          false, false );

    // Calculate bounding box X size:
    textlen = std::max( textlen, textlen2 );
    end.x = std::max( m_size.x, textlen );

    // Calculate bounding box pos:
    end.y = m_size.y;
    end += m_pos;

    // Move upper and lower limits to include texts:
    box.SetY( box.GetY() - ( KiROUND( m_sheetNameSize * 1.3 ) + 8 ) );
    end.y += KiROUND( m_fileNameSize * 1.3 ) + 8;

    box.SetEnd( end );
    box.Inflate( lineWidth / 2 );

    return box;
}


int SCH_SHEET::ComponentCount()
{
    int n = 0;

    if( m_screen )
    {
        EDA_ITEM* bs;

        for( bs = m_screen->GetDrawItems(); bs != NULL; bs = bs->Next() )
        {
            if( bs->Type() == SCH_COMPONENT_T )
            {
                SCH_COMPONENT* Cmp = (SCH_COMPONENT*) bs;

                if( Cmp->GetField( VALUE )->GetText().GetChar( 0 ) != '#' )
                    n++;
            }

            if( bs->Type() == SCH_SHEET_T )
            {
                SCH_SHEET* sheet = (SCH_SHEET*) bs;
                n += sheet->ComponentCount();
            }
        }
    }

    return n;
}


bool SCH_SHEET::SearchHierarchy( const wxString& aFilename, SCH_SCREEN** aScreen )
{
    if( m_screen )
    {
        EDA_ITEM* item = m_screen->GetDrawItems();

        while( item )
        {
            if( item->Type() == SCH_SHEET_T )
            {
                // Must use the screen's path (which is always absolute) rather than the
                // sheet's (which could be relative).

                SCH_SHEET*  sheet = (SCH_SHEET*) item;
                SCH_SCREEN* screen = sheet->m_screen;

                if( screen && screen->GetFileName().CmpNoCase( aFilename ) == 0 )
                {
                    *aScreen = screen;
                    return true;
                }
                else if( sheet->SearchHierarchy( aFilename, aScreen ) )
                {
                    return true;
                }
            }

            item = item->Next();
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

        EDA_ITEM* strct = m_screen->GetDrawItems();

        while( strct )
        {
            if( strct->Type() == SCH_SHEET_T )
            {
                SCH_SHEET* ss = (SCH_SHEET*) strct;

                if( ss->LocatePathOfScreen( aScreen, aList ) )
                    return true;
            }

            strct = strct->Next();
        }

        aList->pop_back();
    }

    return false;
}


int SCH_SHEET::CountSheets()
{
    int count = 1; //1 = this!!

    if( m_screen )
    {
        EDA_ITEM* strct = m_screen->GetDrawItems();

        for( ; strct; strct = strct->Next() )
        {
            if( strct->Type() == SCH_SHEET_T )
            {
                SCH_SHEET* subsheet = (SCH_SHEET*) strct;
                count += subsheet->CountSheets();
            }
        }
    }
    return count;
}


wxString SCH_SHEET::GetFileName( void ) const
{
    return m_fileName;
}


void SCH_SHEET::GetMsgPanelInfo( EDA_UNITS_T aUnits, MSG_PANEL_ITEMS& aList )
{
    aList.push_back( MSG_PANEL_ITEM( _( "Sheet Name" ), m_name, CYAN ) );
    aList.push_back( MSG_PANEL_ITEM( _( "File Name" ), m_fileName, BROWN ) );

#if 0   // Set to 1 to display the sheet time stamp (mainly for test)
    wxString msg;
    msg.Printf( wxT( "%.8X" ), m_TimeStamp );
    aList.push_back( MSG_PANEL_ITEM( _( "Time Stamp" ), msg, BLUE ) );
#endif
}


void SCH_SHEET::Rotate(wxPoint aPosition)
{
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

    for( SCH_SHEET_PIN& sheetPin : m_pins )
    {
        sheetPin.Rotate( aPosition );
    }
}


void SCH_SHEET::MirrorX( int aXaxis_position )
{
    MIRROR( m_pos.y, aXaxis_position );
    m_pos.y -= m_size.y;

    for( SCH_SHEET_PIN& sheetPin : m_pins )
    {
        sheetPin.MirrorX( aXaxis_position );
    }
}


void SCH_SHEET::MirrorY( int aYaxis_position )
{
    MIRROR( m_pos.x, aYaxis_position );
    m_pos.x -= m_size.x;

    for( SCH_SHEET_PIN& label : m_pins )
    {
        label.MirrorY( aYaxis_position );
    }
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

    /* Move the sheet labels according to the new sheet size. */
    for( SCH_SHEET_PIN& label : m_pins )
    {
        label.ConstrainOnEdge( label.GetPosition() );
    }
}


bool SCH_SHEET::Matches( wxFindReplaceData& aSearchData, void* aAuxData )
{
    wxLogTrace( traceFindItem, wxT( "  item " ) + GetSelectMenuText( MILLIMETRES ) );

    // Ignore the sheet file name if searching to replace.
    if( !(aSearchData.GetFlags() & FR_SEARCH_REPLACE)
        && SCH_ITEM::Matches( m_fileName, aSearchData ) )
    {
        return true;
    }

    if( SCH_ITEM::Matches( m_name, aSearchData ) )
    {
        return true;
    }

    return false;
}


bool SCH_SHEET::Replace( wxFindReplaceData& aSearchData, void* aAuxData )
{
    return EDA_ITEM::Replace( aSearchData, m_name );
}


void SCH_SHEET::renumberPins()
{
    int id = 2;

    for( SCH_SHEET_PIN& pin : m_pins )
    {
        pin.SetNumber( id );
        id++;
    }
}


void SCH_SHEET::GetEndPoints( std::vector <DANGLING_END_ITEM>& aItemList )
{
    for( unsigned ii = 0; ii < GetPins().size(); ii++ )
    {
        SCH_SHEET_PIN &pinsheet = GetPins()[ii];

        wxCHECK2_MSG( pinsheet.Type() == SCH_SHEET_PIN_T, continue,
                      wxT( "Invalid item in schematic sheet pin list.  Bad programmer!" ) );

        pinsheet.GetEndPoints( aItemList );
    }
}


bool SCH_SHEET::UpdateDanglingState( std::vector<DANGLING_END_ITEM>& aItemList )
{
    bool changed = false;

    for( SCH_SHEET_PIN& pinsheet : GetPins() )
        changed |= pinsheet.UpdateDanglingState( aItemList );

    return changed;
}


void SCH_SHEET::GetConnectionPoints( std::vector< wxPoint >& aPoints ) const
{
    for( size_t i = 0; i < GetPins().size(); i++ )
        aPoints.push_back( GetPins()[i].GetPosition() );
}


SEARCH_RESULT SCH_SHEET::Visit( INSPECTOR aInspector, void* testData, const KICAD_T aFilterTypes[] )
{
    KICAD_T stype;

    for( const KICAD_T* p = aFilterTypes;  (stype = *p) != EOT;   ++p )
    {
        // If caller wants to inspect my type
        if( stype == SCH_LOCATE_ANY_T || stype == Type() )
        {
            if( SEARCH_QUIT == aInspector( this, NULL ) )
                return SEARCH_QUIT;
        }

        if( stype == SCH_LOCATE_ANY_T || stype == SCH_SHEET_PIN_T )
        {
            // Test the sheet labels.
            for( size_t i = 0;  i < m_pins.size();  i++ )
            {
                if( SEARCH_QUIT == aInspector( &m_pins[ i ], this ) )
                    return SEARCH_QUIT;
            }
        }
    }

    return SEARCH_CONTINUE;
}


wxString SCH_SHEET::GetSelectMenuText( EDA_UNITS_T aUnits ) const
{
    return wxString::Format( _( "Hierarchical Sheet %s" ), m_name );
}


BITMAP_DEF SCH_SHEET::GetMenuImage() const
{
    return add_hierarchical_subsheet_xpm;
}


bool SCH_SHEET::HitTest( const wxPoint& aPosition, int aAccuracy ) const
{
    EDA_RECT rect = GetBoundingBox();

    rect.Inflate( aAccuracy );

    return rect.Contains( aPosition );
}


bool SCH_SHEET::HitTest( const EDA_RECT& aRect, bool aContained, int aAccuracy ) const
{
    EDA_RECT rect = aRect;

    rect.Inflate( aAccuracy );

    if( aContained )
        return rect.Contains( GetBoundingBox() );

    return rect.Intersects( GetBoundingBox() );
}


wxPoint SCH_SHEET::GetResizePosition() const
{
    return wxPoint( m_pos.x + m_size.GetWidth(), m_pos.y + m_size.GetHeight() );
}


void SCH_SHEET::GetNetListItem( NETLIST_OBJECT_LIST& aNetListItems,
                                SCH_SHEET_PATH*      aSheetPath )
{
    SCH_SHEET_PATH sheetPath = *aSheetPath;
    sheetPath.push_back( this );

    for( size_t i = 0;  i < m_pins.size();  i++ )
    {
        NETLIST_OBJECT* item = new NETLIST_OBJECT();
        item->m_SheetPathInclude = sheetPath;
        item->m_SheetPath = *aSheetPath;
        item->m_Comp = &m_pins[i];
        item->m_Link = this;
        item->m_Type = NET_SHEETLABEL;
        item->m_Label = m_pins[i].GetText();
        item->m_Start = item->m_End = m_pins[i].GetPosition();
        aNetListItems.push_back( item );

        SCH_CONNECTION conn;
        if( conn.IsBusLabel( m_pins[i].GetText() ) )
            item->ConvertBusToNetListItems( aNetListItems );
    }
}


void SCH_SHEET::Plot( PLOTTER* aPlotter )
{
    COLOR4D    txtcolor = COLOR4D::UNSPECIFIED;
    wxSize      size;
    wxString    Text;
    int         name_orientation;
    wxPoint     pos_sheetname, pos_filename;
    wxPoint     pos;

    aPlotter->SetColor( GetLayerColor( GetLayer() ) );

    int thickness = GetPenSize();
    aPlotter->SetCurrentLineWidth( thickness );

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

    if( IsVerticalOrientation() )
    {
        pos_sheetname    = wxPoint( m_pos.x - 8, m_pos.y + m_size.y );
        pos_filename     = wxPoint( m_pos.x + m_size.x + 4, m_pos.y + m_size.y );
        name_orientation = TEXT_ANGLE_VERT;
    }
    else
    {
        pos_sheetname    = wxPoint( m_pos.x, m_pos.y - 4 );
        pos_filename     = wxPoint( m_pos.x, m_pos.y + m_size.y + 4 );
        name_orientation = TEXT_ANGLE_HORIZ;
    }

    /* Draw texts: SheetName */
    Text = m_name;
    size = wxSize( m_sheetNameSize, m_sheetNameSize );

    //pos  = m_pos; pos.y -= 4;
    thickness = GetDefaultLineThickness();
    thickness = Clamp_Text_PenSize( thickness, size, false );

    aPlotter->SetColor( GetLayerColor( LAYER_SHEETNAME ) );

    bool italic = false;
    aPlotter->Text( pos_sheetname, txtcolor, wxT( "Sheet: " ) + Text, name_orientation, size,
                    GR_TEXT_HJUSTIFY_LEFT, GR_TEXT_VJUSTIFY_BOTTOM,
                    thickness, italic, false );

    /*Draw texts : FileName */
    Text = GetFileName();
    size = wxSize( m_fileNameSize, m_fileNameSize );
    thickness = GetDefaultLineThickness();
    thickness = Clamp_Text_PenSize( thickness, size, false );

    aPlotter->SetColor( GetLayerColor( LAYER_SHEETFILENAME ) );

    aPlotter->Text( pos_filename, txtcolor, wxT( "File: " ) + Text, name_orientation, size,
                    GR_TEXT_HJUSTIFY_LEFT, GR_TEXT_VJUSTIFY_TOP,
                    thickness, italic, false );

    aPlotter->SetColor( GetLayerColor( GetLayer() ) );

    /* Draw texts : SheetLabel */
    for( size_t i = 0; i < m_pins.size(); i++ )
    {
        m_pins[i].Plot( aPlotter );
    }
}


SCH_ITEM& SCH_SHEET::operator=( const SCH_ITEM& aItem )
{
    wxLogDebug( wxT( "Sheet assignment operator." ) );

    wxCHECK_MSG( Type() == aItem.Type(), *this,
                 wxT( "Cannot assign object type " ) + aItem.GetClass() + wxT( " to type " ) +
                 GetClass() );

    if( &aItem != this )
    {
        SCH_ITEM::operator=( aItem );

        SCH_SHEET* sheet = (SCH_SHEET*) &aItem;

        m_pos = sheet->m_pos;
        m_size = sheet->m_size;
        m_name = sheet->m_name;
        m_sheetNameSize = sheet->m_sheetNameSize;
        m_fileNameSize = sheet->m_fileNameSize;
        m_pins = sheet->m_pins;

        // Ensure sheet labels have their #m_Parent member pointing really on their
        // parent, after assigning.
        for( SCH_SHEET_PIN& sheetPin : m_pins )
        {
            sheetPin.SetParent( this );
        }
    }

    return *this;
}


#if defined(DEBUG)

void SCH_SHEET::Show( int nestLevel, std::ostream& os ) const
{
    // XML output:
    wxString s = GetClass();

    NestedSpace( nestLevel, os ) << '<' << s.Lower().mb_str() << ">" << " sheet_name=\""
                                 << TO_UTF8( m_name ) << '"' << ">\n";

    // show all the pins, and check the linked list integrity
    for( const SCH_SHEET_PIN& label : m_pins )
    {
        label.Show( nestLevel + 1, os );
    }

    NestedSpace( nestLevel, os ) << "</" << s.Lower().mb_str() << ">\n" << std::flush;
}

#endif
