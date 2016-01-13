/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2009 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 1992-2015 KiCad Developers, see AUTHORS.txt for contributors.
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
#include <class_drawpanel.h>
#include <drawtxt.h>
#include <trigo.h>
#include <richio.h>
#include <schframe.h>
#include <plot_common.h>
#include <kicad_string.h>
#include <msgpanel.h>

#include <class_library.h>
#include <sch_sheet.h>
#include <sch_sheet_path.h>
#include <sch_component.h>
#include <class_netlist_object.h>
#include <sch_reference_list.h>


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
//    wxLogDebug( wxT( "Destroying sheet " ) + m_name );

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


bool SCH_SHEET::Save( FILE* aFile ) const
{
    if( fprintf( aFile, "$Sheet\n" ) == EOF
        || fprintf( aFile, "S %-4d %-4d %-4d %-4d\n",
                    m_pos.x, m_pos.y, m_size.x, m_size.y ) == EOF )
        return false;

    //save the unique timestamp, like other schematic parts.
    if( fprintf( aFile, "U %8.8lX\n", m_TimeStamp ) == EOF )
        return false;

    /* Save schematic sheetname and filename. */
    if( !m_name.IsEmpty() )
    {
        if( fprintf( aFile, "F0 %s %d\n", EscapedUTF8( m_name ).c_str(),
                     m_sheetNameSize ) == EOF )
            return false;
    }

    if( !m_fileName.IsEmpty() )
    {
        if( fprintf( aFile, "F1 %s %d\n", EscapedUTF8( m_fileName ).c_str(),
                     m_fileNameSize ) == EOF )
            return false;
    }

    /* Save the list of labels in the sheet. */

    BOOST_FOREACH( const SCH_SHEET_PIN& label, m_pins )
    {
        if( !label.Save( aFile ) )
            return false;
    }

    if( fprintf( aFile, "$EndSheet\n" ) == EOF )
        return false;

    return true;
}


bool SCH_SHEET::Load( LINE_READER& aLine, wxString& aErrorMsg )
{
    int              fieldNdx, size;
    SCH_SHEET_PIN*   sheetPin;
    char*            ptcar;

    if( IsRootSheet() )
        m_number = 1;
    else
        m_number = GetRootSheet()->CountSheets();

    SetTimeStamp( GetNewTimeStamp() );

    // sheets are added to the GetDrawItems() like other schematic components.
    // however, in order to preserve the hierarchy (through m_Parent pointers),
    // a duplicate of the sheet is added to m_SubSheet array.
    // must be a duplicate, references just work for a two-layer structure.
    // this is accomplished through the Sync() function.

    if( ((char*)aLine)[0] == '$' )   // line should be "$Sheet"
    {
        if( !aLine.ReadLine() )
        {
            aErrorMsg.Printf( wxT( "Read File Error" ) );
            return false;
        }
    }

    /* Next line: must be "S xx yy nn mm" with xx, yy = sheet position
     *  ( upper left corner  ) et nn,mm = sheet size */
    if( ( sscanf( &((char*)aLine)[1], "%d %d %d %d",
                  &m_pos.x, &m_pos.y, &m_size.x, &m_size.y ) != 4 )
        || ( ((char*)aLine)[0] != 'S' ) )
    {
        aErrorMsg.Printf( wxT( " ** Eeschema file sheet error at line %d, aborted\n" ),
                          aLine.LineNumber() );

        aErrorMsg << FROM_UTF8( ((char*)aLine) );
        return false;
    }

    /* Read fields */
    for( ; ; ) /* Analysis of lines "Fn" text. */
    {
        if( !aLine.ReadLine() )
            return false;

        if( ((char*)aLine)[0] == 'U' )
        {
            sscanf( ((char*)aLine) + 1, "%lX", &m_TimeStamp );

            if( m_TimeStamp == 0 )  // zero is not unique!
                SetTimeStamp( GetNewTimeStamp() );

            continue;
        }

        if( ((char*)aLine)[0] != 'F' )
            break;

        sscanf( ((char*)aLine) + 1, "%d", &fieldNdx );

        /* Read the field:
         * If fieldNdx> = 2: Fn "text" t s posx posy
         * If F0 "text" for SheetName
         * F1 and "text" for filename
         */
        ptcar = ((char*)aLine);

        while( *ptcar && ( *ptcar != '"' ) )
            ptcar++;

        if( *ptcar != '"' )
        {
            aErrorMsg.Printf( wxT( "Eeschema file sheet label F%d at line %d, aborted\n" ),
                              fieldNdx, aLine.LineNumber() );
            aErrorMsg << FROM_UTF8( (char*) aLine );
            return false;
        }

        wxString sheetName;
        ptcar += ReadDelimitedText( &sheetName, ptcar );

        if( *ptcar == 0 )
        {
            aErrorMsg.Printf( wxT( "Eeschema file sheet field F at line %d, aborted\n" ),
                              aLine.LineNumber() );
            aErrorMsg << FROM_UTF8( (char*) aLine );
            return false;
        }

        if( ( fieldNdx == 0 ) || ( fieldNdx == 1 ) )
        {
            if( sscanf( ptcar, "%d", &size ) != 1 )
            {
                aErrorMsg.Printf( wxT( "Eeschema file sheet Label error line %d, aborted\n" ),
                                  aLine.LineNumber() );

                aErrorMsg << FROM_UTF8( (char*) aLine );
            }

            if( size == 0 )
                size = GetDefaultTextSize();

            if( fieldNdx == 0 )
            {
                m_name     = sheetName;
                m_sheetNameSize = size;
            }
            else
            {
                SetFileName( sheetName );
                m_fileNameSize = size;
            }
        }

        if( fieldNdx > 1 )
        {
            sheetPin = new SCH_SHEET_PIN( this );

            if( !sheetPin->Load( aLine, aErrorMsg ) )
            {
                delete sheetPin;
                sheetPin = NULL;
                return false;
            }

            AddPin( sheetPin );
        }
    }

    if( strnicmp( "$End", ((char*)aLine), 4 ) != 0 )
    {
        aErrorMsg.Printf( wxT( "**Eeschema file end_sheet error at line %d, aborted\n" ),
                          aLine.LineNumber() );
        aErrorMsg << FROM_UTF8( ((char*)aLine) );
        return false;
    }

    return true;
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
    BOOST_FOREACH( SCH_SHEET_PIN& sheetPin, m_pins )
    {
        sheetPin.SetParent( this );
    }

    BOOST_FOREACH( SCH_SHEET_PIN& sheetPin, sheet->m_pins )
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
    BOOST_FOREACH( SCH_SHEET_PIN pin, m_pins )
    {
        if( pin.GetText().CmpNoCase( aName ) == 0 )
            return true;
    }

    return false;
}


bool SCH_SHEET::IsVerticalOrientation() const
{
    BOOST_FOREACH( SCH_SHEET_PIN pin, m_pins )
    {
        if( pin.GetEdge() > 1 )
            return true;
    }
    return false;
}


bool SCH_SHEET::HasUndefinedPins()
{
    BOOST_FOREACH( SCH_SHEET_PIN pin, m_pins )
    {
        /* Search the schematic for a hierarchical label corresponding to this sheet label. */
        EDA_ITEM* DrawStruct  = m_screen->GetDrawItems();
        const SCH_HIERLABEL* HLabel = NULL;

        for( ; DrawStruct != NULL; DrawStruct = DrawStruct->Next() )
        {
            if( DrawStruct->Type() != SCH_HIERARCHICAL_LABEL_T )
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

        // Make sure pin is on right or left side of sheet.
        if( edge >= 2 )
            continue;

        EDA_RECT rect = m_pins[i].GetBoundingBox();

        if( width < rect.GetWidth() )
            width = rect.GetWidth();

        for( size_t j = 0; j < m_pins.size(); j++ )
        {
            if( (i == j) || (m_pins[i].GetPosition().y != m_pins[j].GetPosition().y) )
                continue;

            if( width < rect.GetWidth() + m_pins[j].GetBoundingBox().GetWidth() )
            {
                width = rect.GetWidth() + m_pins[j].GetBoundingBox().GetWidth();
                break;
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
        int pinY = m_pins[i].GetPosition().y - m_pos.y;

        if( pinY > height )
            height = pinY;
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
            if( DrawStruct->Type() != SCH_HIERARCHICAL_LABEL_T )
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
    BOOST_FOREACH( SCH_SHEET_PIN& pin, m_pins )
    {
        if( pin.HitTest( aPosition, 0 ) )
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

    if( IsVerticalOrientation() )
    {
        pos.x -= 8;
        pos.y += m_size.y;
    }
    else
    {
        pos.y -= 8;
    }

    return pos;
}


wxPoint SCH_SHEET::GetFileNamePosition()
{
    wxPoint  pos = m_pos;
    int      margin = GetPenSize() + 4;

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


void SCH_SHEET::Draw( EDA_DRAW_PANEL* aPanel, wxDC* aDC,
                      const wxPoint& aOffset, GR_DRAWMODE aDrawMode, EDA_COLOR_T aColor )
{
    EDA_COLOR_T txtcolor;
    wxString Text;
    EDA_COLOR_T color;
    int      name_orientation;
    wxPoint  pos_sheetname,pos_filename;
    wxPoint  pos = m_pos + aOffset;
    int      lineWidth = GetPenSize();
    EDA_RECT* clipbox  = aPanel? aPanel->GetClipBox() : NULL;

    if( aColor >= 0 )
        color = aColor;
    else
        color = GetLayerColor( m_Layer );

    GRSetDrawMode( aDC, aDrawMode );

    GRRect( clipbox, aDC, pos.x, pos.y,
            pos.x + m_size.x, pos.y + m_size.y, lineWidth, color );

    pos_sheetname = GetSheetNamePosition() + aOffset;
    pos_filename = GetFileNamePosition() + aOffset;

    if( IsVerticalOrientation() )
        name_orientation = TEXT_ORIENT_VERT;
    else
        name_orientation = TEXT_ORIENT_HORIZ;

    /* Draw text : SheetName */
    if( aColor > 0 )
        txtcolor = aColor;
    else
        txtcolor = GetLayerColor( LAYER_SHEETNAME );

    Text = wxT( "Sheet: " ) + m_name;
    DrawGraphicText( clipbox, aDC, pos_sheetname,
                     txtcolor, Text, name_orientation,
                     wxSize( m_sheetNameSize, m_sheetNameSize ),
                     GR_TEXT_HJUSTIFY_LEFT, GR_TEXT_VJUSTIFY_BOTTOM, lineWidth,
                     false, false );

    /* Draw text : FileName */
    if( aColor >= 0 )
        txtcolor = aColor;
    else
        txtcolor = GetLayerColor( LAYER_SHEETFILENAME );

    Text = wxT( "File: " ) + m_fileName;
    DrawGraphicText( clipbox, aDC, pos_filename,
                     txtcolor, Text, name_orientation,
                     wxSize( m_fileNameSize, m_fileNameSize ),
                     GR_TEXT_HJUSTIFY_LEFT, GR_TEXT_VJUSTIFY_TOP, lineWidth,
                     false, false );


    /* Draw text : SheetLabel */
    BOOST_FOREACH( SCH_SHEET_PIN& sheetPin, m_pins )
    {
        if( !sheetPin.IsMoving() )
            sheetPin.Draw( aPanel, aDC, aOffset, aDrawMode, aColor );
    }
}


const EDA_RECT SCH_SHEET::GetBoundingBox() const
{
    wxPoint end;
    EDA_RECT box( m_pos, m_size );
    int      lineWidth = GetPenSize();

    // Determine length of texts
    wxString text    = wxT( "Sheet: " ) + m_name;
    int      textlen  = GraphicTextWidth( text, m_sheetNameSize, false, lineWidth );
    text = wxT( "File: " ) + m_fileName;
    int      textlen2 = GraphicTextWidth( text, m_fileNameSize, false, lineWidth );

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
                SCH_SHEET* sheet = (SCH_SHEET*) item;

                if( sheet->m_screen
                    && sheet->m_screen->GetFileName().CmpNoCase( aFilename ) == 0 )
                {
                    *aScreen = sheet->m_screen;
                    return true;
                }

                if( sheet->SearchHierarchy( aFilename, aScreen ) )
                    return true;
            }

            item = item->Next();
        }
    }

    return false;
}


bool SCH_SHEET::Load( SCH_EDIT_FRAME* aFrame )
{
    bool success = true;

    SCH_SCREEN* screen = NULL;

    if( !m_screen )
    {
        GetRootSheet()->SearchHierarchy( m_fileName, &screen );

        if( screen )
        {
            SetScreen( screen );

            //do not need to load the sub-sheets - this has already been done.
        }
        else
        {
            SetScreen( new SCH_SCREEN( &aFrame->Kiway() ) );

            success = aFrame->LoadOneEEFile( m_screen, m_fileName );

            if( success )
            {
                EDA_ITEM* bs = m_screen->GetDrawItems();

                while( bs )
                {
                    if( bs->Type() == SCH_SHEET_T )
                    {
                        SCH_SHEET* sheetstruct = (SCH_SHEET*) bs;

                        // Set the parent to this sheet.  This effectively creates the
                        // schematic sheet hierarchy eliminating the need to keep a
                        // copy of the root sheet in order to generate the hierarchy.
                        sheetstruct->SetParent( this );

                        if( !sheetstruct->Load( aFrame ) )
                            success = false;
                    }

                    bs = bs->Next();
                }
            }
        }
    }

    return success;
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


void SCH_SHEET::GetMsgPanelInfo( MSG_PANEL_ITEMS& aList )
{
    aList.push_back( MSG_PANEL_ITEM( _( "Sheet Name" ), m_name, CYAN ) );
    aList.push_back( MSG_PANEL_ITEM( _( "File Name" ), m_fileName, BROWN ) );
    aList.push_back( MSG_PANEL_ITEM( _( "Path" ), GetHumanReadablePath(), DARKMAGENTA ) );

#if 1   // Set to 1 to display the sheet time stamp (mainly for test)
    aList.push_back( MSG_PANEL_ITEM( _( "Time Stamp" ), GetPath(), BLUE ) );
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

    BOOST_FOREACH( SCH_SHEET_PIN& sheetPin, m_pins )
    {
        sheetPin.Rotate( aPosition );
    }
}


void SCH_SHEET::MirrorX( int aXaxis_position )
{
    MIRROR( m_pos.y, aXaxis_position );
    m_pos.y -= m_size.y;

    BOOST_FOREACH( SCH_SHEET_PIN& sheetPin, m_pins )
    {
        sheetPin.MirrorX( aXaxis_position );
    }
}


void SCH_SHEET::MirrorY( int aYaxis_position )
{
    MIRROR( m_pos.x, aYaxis_position );
    m_pos.x -= m_size.x;

    BOOST_FOREACH( SCH_SHEET_PIN& label, m_pins )
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
    BOOST_FOREACH( SCH_SHEET_PIN& label, m_pins )
    {
        label.ConstrainOnEdge( label.GetPosition() );
    }
}


bool SCH_SHEET::Matches( wxFindReplaceData& aSearchData, void* aAuxData, wxPoint* aFindLocation )
{
    wxLogTrace( traceFindItem, wxT( "  item " ) + GetSelectMenuText() );

    // Ignore the sheet file name if searching to replace.
    if( !(aSearchData.GetFlags() & FR_SEARCH_REPLACE)
        && SCH_ITEM::Matches( m_fileName, aSearchData ) )
    {
        if( aFindLocation )
            *aFindLocation = GetFileNamePosition();

        return true;
    }

    if( SCH_ITEM::Matches( m_name, aSearchData ) )
    {
        if( aFindLocation )
            *aFindLocation = GetSheetNamePosition();

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

    BOOST_FOREACH( SCH_SHEET_PIN& pin, m_pins )
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


bool SCH_SHEET::IsDanglingStateChanged( std::vector< DANGLING_END_ITEM >& aItemList )
{
    bool currentState = IsDangling();

    BOOST_FOREACH( SCH_SHEET_PIN& pinsheet, GetPins() )
    {
        pinsheet.IsDanglingStateChanged( aItemList );
    }

    return currentState != IsDangling();
}


bool SCH_SHEET::IsDangling() const
{
    // If any hierarchical label in the sheet is dangling, then the sheet is dangling.
    for( size_t i = 0; i < GetPins().size(); i++ )
    {
        if( GetPins()[i].IsDangling() )
            return true;
    }

    return false;
}


bool SCH_SHEET::IsSelectStateChanged( const wxRect& aRect )
{
    bool previousState = IsSelected();

    EDA_RECT boundingBox = GetBoundingBox();

    if( aRect.Intersects( boundingBox ) )
        SetFlags( SELECTED );
    else
        ClearFlags( SELECTED );

    return previousState != IsSelected();
}


void SCH_SHEET::GetConnectionPoints( std::vector< wxPoint >& aPoints ) const
{
    for( size_t i = 0; i < GetPins().size(); i++ )
        aPoints.push_back( GetPins()[i].GetPosition() );
}


SEARCH_RESULT SCH_SHEET::Visit( INSPECTOR* aInspector, const void* aTestData,
                                const KICAD_T aFilterTypes[] )
{
    KICAD_T stype;

    for( const KICAD_T* p = aFilterTypes;  (stype = *p) != EOT;   ++p )
    {
        // If caller wants to inspect my type
        if( stype == Type() )
        {
            if( SEARCH_QUIT == aInspector->Inspect( this, NULL ) )
                return SEARCH_QUIT;
        }
        else if( stype == SCH_SHEET_PIN_T )
        {
            // Test the sheet labels.
            for( size_t i = 0;  i < m_pins.size();  i++ )
            {
                if( SEARCH_QUIT == aInspector->Inspect( &m_pins[ i ], (void*) this ) )
                    return SEARCH_QUIT;
            }
        }
    }

    return SEARCH_CONTINUE;
}


wxString SCH_SHEET::GetSelectMenuText() const
{
    wxString tmp;
    tmp.Printf( _( "Hierarchical Sheet %s" ), GetChars( m_name ) );
    return tmp;
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
    sheetPath.Push( this );

    for( size_t i = 0;  i < m_pins.size();  i++ )
    {
        NETLIST_OBJECT* item = new NETLIST_OBJECT();
        item->m_SheetPathInclude = sheetPath;
        item->m_SheetPath = *aSheetPath;
        item->m_Comp = &m_pins[i];
        item->m_Link = this;
        item->m_Type = NET_SHEETLABEL;
        item->m_ElectricalType = m_pins[i].GetShape();
        item->m_Label = m_pins[i].GetText();
        item->m_Start = item->m_End = m_pins[i].GetPosition();
        aNetListItems.push_back( item );

        if( IsBusLabel( m_pins[i].GetText() ) )
            item->ConvertBusToNetListItems( aNetListItems );
    }
}


void SCH_SHEET::Plot( PLOTTER* aPlotter )
{
    EDA_COLOR_T txtcolor = UNSPECIFIED_COLOR;
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
        name_orientation = TEXT_ORIENT_VERT;
    }
    else
    {
        pos_sheetname    = wxPoint( m_pos.x, m_pos.y - 4 );
        pos_filename     = wxPoint( m_pos.x, m_pos.y + m_size.y + 4 );
        name_orientation = TEXT_ORIENT_HORIZ;
    }

    /* Draw texts: SheetName */
    Text = m_name;
    size = wxSize( m_sheetNameSize, m_sheetNameSize );

    thickness = GetDefaultLineThickness();
    thickness = Clamp_Text_PenSize( thickness, size, false );

    aPlotter->SetColor( GetLayerColor( LAYER_SHEETNAME ) );

    bool italic = false;
    aPlotter->Text( pos_sheetname, txtcolor, Text, name_orientation, size,
                    GR_TEXT_HJUSTIFY_LEFT, GR_TEXT_VJUSTIFY_BOTTOM,
                    thickness, italic, false );

    /*Draw texts : FileName */
    Text = GetFileName();
    size = wxSize( m_fileNameSize, m_fileNameSize );
    thickness = GetDefaultLineThickness();
    thickness = Clamp_Text_PenSize( thickness, size, false );

    aPlotter->SetColor( GetLayerColor( LAYER_SHEETFILENAME ) );

    aPlotter->Text( pos_filename, txtcolor, Text, name_orientation, size,
                    GR_TEXT_HJUSTIFY_LEFT, GR_TEXT_VJUSTIFY_TOP,
                    thickness, italic, false );

    aPlotter->SetColor( GetLayerColor( GetLayer() ) );

    /* Draw texts : SheetLabel */
    for( size_t i = 0; i < m_pins.size(); i++ )
    {
        m_pins[i].Plot( aPlotter );
    }
}


SCH_SHEET* SCH_SHEET::GetRootSheet()
{
    EDA_ITEM* parent = GetParent();
    SCH_SHEET* rootSheet = this;

    while( parent )
    {
        // The parent of a SCH_SHEET object can only be another SCH_SHEET object or NULL.
        wxASSERT_MSG( parent->Type() == SCH_SHEET_T, "SCH_SHEET parent is not a SCH_SHEET" );
        rootSheet = static_cast<SCH_SHEET*>( parent );
        parent = parent->GetParent();
    }

    return rootSheet;
}


void SCH_SHEET::GetPath( SCH_CONST_SHEETS& aSheetPath ) const
{
    aSheetPath.insert( aSheetPath.begin(),  const_cast<SCH_SHEET*>( this ) );

    if( GetParent() )
        static_cast<SCH_SHEET*>( GetParent() )->GetPath( aSheetPath );
}


wxString SCH_SHEET::GetPath() const
{
    wxString         tmp;
    wxString         path = "/";
    const SCH_SHEET* sheet = this;

    while( sheet->GetParent() )
    {
        tmp.Printf( "/%8.8lX", (long unsigned) sheet->GetTimeStamp() );

        // We are walking up the parent stack so prepend each time stamp.
        path = tmp + path;
        sheet = static_cast<SCH_SHEET*>( sheet->GetParent() );
    }

    return path;
}


wxString SCH_SHEET::GetHumanReadablePath() const
{
    wxString         path = "/";
    const SCH_SHEET* sheet = this;

    while( sheet->GetParent() )
    {
        // We are walking up the parent stack so prepend each sheet name.
        path = "/" + sheet->GetName() + path;
        sheet = static_cast<SCH_SHEET*>( sheet->GetParent() );
    }

    return path;
}


void SCH_SHEET::ClearAnnotation( bool aIncludeSubSheets )
{
    m_screen->ClearAnnotation( this );

    if( aIncludeSubSheets )
    {
        SCH_ITEM* item = m_screen->GetDrawItems();

        while( item )
        {
            if( item->Type() == SCH_SHEET_T )
                static_cast<SCH_SHEET*>( item )->ClearAnnotation( aIncludeSubSheets );

            item = item->Next();
        }
    }
}


bool SCH_SHEET::IsModified() const
{
    if( m_screen->IsModify() )
        return true;

    bool      retv = false;
    SCH_ITEM* item = m_screen->GetDrawItems();

    while( item && !retv )
    {
        if( item->Type() == SCH_SHEET_T )
            retv = static_cast<SCH_SHEET*>( item )->IsModified();

        item = item->Next();
    }

    return retv;
}


bool SCH_SHEET::IsAutoSaveRequired()
{
    if( m_screen->IsModify() )
        return true;

    bool      retv = false;
    SCH_ITEM* item = m_screen->GetDrawItems();

    while( item && !retv )
    {
        if( item->Type() == SCH_SHEET_T )
        {
            SCH_SHEET* sheet = static_cast<SCH_SHEET*>( item );

            if( sheet->m_screen )
                retv = sheet->m_screen->IsSave();
        }

        item = item->Next();
    }

    return retv;
}


void SCH_SHEET::ClearModifyStatus()
{
    m_screen->ClrModify();

    SCH_ITEM* item = m_screen->GetDrawItems();

    while( item )
    {
        if( item->Type() == SCH_SHEET_T )
            static_cast<SCH_SHEET*>( item )->m_screen->ClrModify();

        item = item->Next();
    }
}


void SCH_SHEET::AnnotatePowerSymbols( PART_LIBS* aLibs, int* aReference )
{
    int ref = 1;

    if( aReference )
        ref = *aReference;

    for( EDA_ITEM* item = m_screen->GetDrawItems();  item;  item = item->Next() )
    {
        if( item->Type() != SCH_COMPONENT_T )
            continue;

        SCH_COMPONENT*  component = (SCH_COMPONENT*) item;
        LIB_PART*       part = aLibs->FindLibPart( component->GetPartName() );

        if( !part || !part->IsPower() )
            continue;

        wxString refstr = component->GetPrefix();

        //str will be "C?" or so after the ClearAnnotation call.
        while( refstr.Last() == '?' )
            refstr.RemoveLast();

        if( !refstr.StartsWith( wxT( "#" ) ) )
            refstr = wxT( "#" ) + refstr;

        refstr << wxT( "0" ) << ref;
        component->SetRef( this, refstr );
        ref++;
    }

    if( aReference )
        *aReference = ref;
}


void SCH_SHEET::UpdateAllScreenReferences()
{
    EDA_ITEM* t = m_screen->GetDrawItems();

    while( t )
    {
        if( t->Type() == SCH_COMPONENT_T )
        {
            SCH_COMPONENT* component = (SCH_COMPONENT*) t;
            component->GetField( REFERENCE )->SetText( component->GetRef( this ) );
            component->UpdateUnit( component->GetUnitSelection( this ) );
        }

        t = t->Next();
    }
}


void SCH_SHEET::GetComponents( PART_LIBS* aLibs, SCH_REFERENCE_LIST& aReferences,
                               bool aIncludePowerSymbols, bool aIncludeSubSheets )
{
    for( SCH_ITEM* item = m_screen->GetDrawItems(); item; item = item->Next() )
    {
        if( item->Type() == SCH_SHEET_T && aIncludeSubSheets )
        {
            ((SCH_SHEET*)item)->GetComponents( aLibs, aReferences, aIncludePowerSymbols,
                                               aIncludeSubSheets );
        }

        if( item->Type() == SCH_COMPONENT_T )
        {
            SCH_COMPONENT* component = (SCH_COMPONENT*) item;

            // Skip pseudo components, which have a reference starting with #.  This mainly
            // affects power symbols.
            if( !aIncludePowerSymbols && component->GetRef( this )[0] == wxT( '#' ) )
                continue;

            LIB_PART* part = aLibs->FindLibPart( component->GetPartName() );

            if( part )
            {
                SCH_REFERENCE reference = SCH_REFERENCE( component, part, this );
                reference.SetSheetNumber( m_number );
                aReferences.AddItem( reference );
            }
        }
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
        BOOST_FOREACH( SCH_SHEET_PIN& sheetPin, m_pins )
        {
            sheetPin.SetParent( this );
        }
    }

    return *this;
}


bool SCH_SHEET::operator<( const SCH_SHEET& aRhs ) const
{
    if( (*this - aRhs) < 0 )
        return true;

    return false;
}


int SCH_SHEET::operator-( const SCH_SHEET& aRhs ) const
{
    // Don't waste time against comparing the same objects..
    if( this == &aRhs )
        return 0;

    SCH_CONST_SHEETS lhsPath, rhsPath;

    GetPath( lhsPath );
    aRhs.GetPath( rhsPath );

    // Shorter paths are less than longer paths.
    int retv = lhsPath.size() - rhsPath.size();

    if( retv == 0 )
    {
        // Compare time stamps when path lengths are the same.
        for( unsigned i = 0;  i < lhsPath.size();  i++ )
        {
            retv = lhsPath[i]->GetTimeStamp() - rhsPath[i]->GetTimeStamp();

            if( retv != 0 )
                break;
        }
    }

    return retv;
}


#if defined(DEBUG)

void SCH_SHEET::Show( int nestLevel, std::ostream& os ) const
{
    // XML output:
    wxString s = GetClass();

    NestedSpace( nestLevel, os ) << '<' << s.Lower().mb_str() << ">" << " sheet_name=\""
                                 << TO_UTF8( m_name ) << '"' << ">\n";

    // show all the pins, and check the linked list integrity
    BOOST_FOREACH( const SCH_SHEET_PIN& label, m_pins )
    {
        label.Show( nestLevel + 1, os );
    }

    NestedSpace( nestLevel, os ) << "</" << s.Lower().mb_str() << ">\n" << std::flush;
}

#endif
