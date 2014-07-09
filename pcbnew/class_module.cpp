/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2012 Jean-Pierre Charras, jean-pierre.charras@ujf-grenoble.fr
 * Copyright (C) 2012 SoftPLC Corporation, Dick Hollenbeck <dick@softplc.com>
 * Copyright (C) 2012 Wayne Stambaugh <stambaughw@verizon.net>
 * Copyright (C) 1992-2012 KiCad Developers, see AUTHORS.txt for contributors.
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
 * @file class_module.cpp
 * @brief MODULE class implementation.
 */

#include <fctsys.h>
#include <gr_basic.h>
#include <wxstruct.h>
#include <plot_common.h>
#include <class_drawpanel.h>
#include <trigo.h>
#include <confirm.h>
#include <kicad_string.h>
#include <pcbcommon.h>
#include <pcbnew.h>
#include <colors_selection.h>
#include <richio.h>
#include <filter_reader.h>
#include <macros.h>
#include <3d_struct.h>
#include <msgpanel.h>

#include <drag.h>
#include <class_board.h>
#include <class_edge_mod.h>
#include <class_module.h>


MODULE::MODULE( BOARD* parent ) :
    BOARD_ITEM( (BOARD_ITEM*) parent, PCB_MODULE_T ),
    m_initial_comments( 0 )
{
    m_Attributs    = MOD_DEFAULT;
    m_Layer        = F_Cu;
    m_Orient       = 0;
    m_ModuleStatus = 0;
    flag = 0;
    m_CntRot90 = m_CntRot180 = 0;
    m_Surface  = 0.0;
    m_Link     = 0;
    m_LastEditTime  = time( NULL );
    m_LocalClearance = 0;
    m_LocalSolderMaskMargin  = 0;
    m_LocalSolderPasteMargin = 0;
    m_LocalSolderPasteMarginRatio = 0.0;
    m_ZoneConnection = UNDEFINED_CONNECTION; // Use zone setting by default
    m_ThermalWidth = 0; // Use zone setting by default
    m_ThermalGap = 0; // Use zone setting by default

    m_Reference = new TEXTE_MODULE( this, TEXTE_MODULE::TEXT_is_REFERENCE );

    m_Value = new TEXTE_MODULE( this, TEXTE_MODULE::TEXT_is_VALUE );

    // Reserve one void 3D entry, to avoid problems with void list
    m_3D_Drawings.PushBack( new S3D_MASTER( this ) );
}


MODULE::MODULE( const MODULE& aModule ) :
    BOARD_ITEM( aModule )
{
    m_Pos = aModule.m_Pos;
    m_fpid = aModule.m_fpid;
    m_Layer  = aModule.m_Layer;
    m_Attributs = aModule.m_Attributs;
    m_ModuleStatus = aModule.m_ModuleStatus;
    m_Orient = aModule.m_Orient;
    m_BoundaryBox = aModule.m_BoundaryBox;
    m_CntRot90 = aModule.m_CntRot90;
    m_CntRot180 = aModule.m_CntRot180;
    m_LastEditTime = aModule.m_LastEditTime;
    m_Link = aModule.m_Link;
    m_Path = aModule.m_Path;              //is this correct behavior?

    m_LocalClearance = aModule.m_LocalClearance;
    m_LocalSolderMaskMargin = aModule.m_LocalSolderMaskMargin;
    m_LocalSolderPasteMargin = aModule.m_LocalSolderPasteMargin;
    m_LocalSolderPasteMarginRatio = aModule.m_LocalSolderPasteMarginRatio;
    m_ZoneConnection = aModule.m_ZoneConnection;
    m_ThermalWidth = aModule.m_ThermalWidth;
    m_ThermalGap = aModule.m_ThermalGap;

    // Copy reference and value.
    m_Reference = new TEXTE_MODULE( *aModule.m_Reference );
    m_Reference->SetParent( this );

    m_Value = new TEXTE_MODULE( *aModule.m_Value );
    m_Value->SetParent( this );

    // Copy auxiliary data: Pads
    // m_Pads.DeleteAll();

    for( D_PAD* pad = aModule.m_Pads;  pad;  pad = pad->Next() )
    {
        D_PAD* newpad = new D_PAD( *pad );
        newpad->SetParent( this );
        m_Pads.PushBack( newpad );
    }

    // Copy auxiliary data: Drawings
    for( BOARD_ITEM* item = aModule.m_Drawings;  item;  item = item->Next() )
    {
        BOARD_ITEM* newItem;

        switch( item->Type() )
        {
        case PCB_MODULE_TEXT_T:
        case PCB_MODULE_EDGE_T:
            newItem = (BOARD_ITEM*)item->Clone();
            newItem->SetParent( this );
            m_Drawings.PushBack( newItem );
            break;

        default:
            wxLogMessage( wxT( "MODULE::Copy() Internal Err:  unknown type" ) );
            break;
        }
    }

    // Copy auxiliary data: 3D_Drawings info
    for( S3D_MASTER* item = aModule.m_3D_Drawings;  item;  item = item->Next() )
    {
        if( item->GetShape3DName().IsEmpty() )           // do not copy empty shapes.
            continue;

        S3D_MASTER* t3d = m_3D_Drawings;

        t3d = new S3D_MASTER( this );
        t3d->Copy( item );
        m_3D_Drawings.PushBack( t3d );
    }

    // Ensure there is at least one item in m_3D_Drawings.
    if( m_3D_Drawings.GetCount() == 0 )
        m_3D_Drawings.PushBack( new S3D_MASTER( this ) ); // push a void item

    m_Doc     = aModule.m_Doc;
    m_KeyWord = aModule.m_KeyWord;

    // Ensure auxiliary data is up to date
    CalculateBoundingBox();

    m_initial_comments = aModule.m_initial_comments ?
                            new wxArrayString( *aModule.m_initial_comments ) : 0;
}


MODULE::~MODULE()
{
    delete m_Reference;
    delete m_Value;
    delete m_initial_comments;
}


/* Draw the anchor cross (vertical)
 * Must be done after the pads, because drawing the hole will erase overwrite
 * every thing already drawn.
 */
void MODULE::DrawAncre( EDA_DRAW_PANEL* panel, wxDC* DC, const wxPoint& offset,
                        int dim_ancre, GR_DRAWMODE draw_mode )
{
    GRSetDrawMode( DC, draw_mode );

    if( GetBoard()->IsElementVisible( ANCHOR_VISIBLE ) )
    {
        GRDrawAnchor( panel->GetClipBox(), DC, m_Pos.x, m_Pos.y,
                      dim_ancre,
                      g_ColorsSettings.GetItemColor( ANCHOR_VISIBLE ) );
    }
}


void MODULE::Copy( MODULE* aModule )
{
    m_Pos           = aModule->m_Pos;
    m_Layer         = aModule->m_Layer;
    m_fpid          = aModule->m_fpid;
    m_Attributs     = aModule->m_Attributs;
    m_ModuleStatus  = aModule->m_ModuleStatus;
    m_Orient        = aModule->m_Orient;
    m_BoundaryBox   = aModule->m_BoundaryBox;
    m_CntRot90      = aModule->m_CntRot90;
    m_CntRot180     = aModule->m_CntRot180;
    m_LastEditTime  = aModule->m_LastEditTime;
    m_Link          = aModule->m_Link;
    m_Path          = aModule->m_Path; //is this correct behavior?
    SetTimeStamp( GetNewTimeStamp() );

    m_LocalClearance                = aModule->m_LocalClearance;
    m_LocalSolderMaskMargin         = aModule->m_LocalSolderMaskMargin;
    m_LocalSolderPasteMargin        = aModule->m_LocalSolderPasteMargin;
    m_LocalSolderPasteMarginRatio   = aModule->m_LocalSolderPasteMarginRatio;
    m_ZoneConnection                = aModule->m_ZoneConnection;
    m_ThermalWidth                  = aModule->m_ThermalWidth;
    m_ThermalGap                    = aModule->m_ThermalGap;

    // Copy reference and value.
    m_Reference->Copy( aModule->m_Reference );
    m_Value->Copy( aModule->m_Value );

    // Copy auxiliary data: Pads
    m_Pads.DeleteAll();

    for( D_PAD* pad = aModule->m_Pads;  pad;  pad = pad->Next() )
    {
        D_PAD* newpad = new D_PAD( this );
        newpad->Copy( pad );
        m_Pads.PushBack( newpad );
    }

    // Copy auxiliary data: Drawings
    m_Drawings.DeleteAll();

    for( BOARD_ITEM* item = aModule->m_Drawings;  item;  item = item->Next() )
    {
        switch( item->Type() )
        {
        case PCB_MODULE_TEXT_T:
            TEXTE_MODULE * textm;
            textm = new TEXTE_MODULE( this );
            textm->Copy( (TEXTE_MODULE*) item );
            m_Drawings.PushBack( textm );
            break;

        case PCB_MODULE_EDGE_T:
            EDGE_MODULE * edge;
            edge = new EDGE_MODULE( this );
            edge->Copy( (EDGE_MODULE*) item );
            m_Drawings.PushBack( edge );
            break;

        default:
            wxLogMessage( wxT( "MODULE::Copy() Internal Err:  unknown type" ) );
            break;
        }
    }

    // Copy auxiliary data: 3D_Drawings info
    m_3D_Drawings.DeleteAll();

    // Ensure there is one (or more) item in m_3D_Drawings
    m_3D_Drawings.PushBack( new S3D_MASTER( this ) ); // push a void item

    for( S3D_MASTER* item = aModule->m_3D_Drawings;  item;  item = item->Next() )
    {
        if( item->GetShape3DName().IsEmpty() )           // do not copy empty shapes.
            continue;

        S3D_MASTER* t3d = m_3D_Drawings;

        if( t3d && t3d->GetShape3DName().IsEmpty() )    // The first entry can
        {                                               // exist, but is empty : use it.
            t3d->Copy( item );
        }
        else
        {
            t3d = new S3D_MASTER( this );
            t3d->Copy( item );
            m_3D_Drawings.PushBack( t3d );
        }
    }

    m_Doc     = aModule->m_Doc;
    m_KeyWord = aModule->m_KeyWord;

    // Ensure auxiliary data is up to date
    CalculateBoundingBox();
}


void MODULE::CopyNetlistSettings( MODULE* aModule )
{
    // Don't do anything foolish like trying to copy to yourself.
    wxCHECK_RET( aModule != NULL && aModule != this, wxT( "Cannot copy to NULL or yourself." ) );

    // Not sure what to do with the value field.  Use netlist for now.
    aModule->SetPosition( GetPosition() );

    if( aModule->GetLayer() != GetLayer() )
        aModule->Flip( aModule->GetPosition() );

    if( aModule->GetOrientation() != GetOrientation() )
        aModule->Rotate( aModule->GetPosition(), GetOrientation() );

    aModule->SetLocalSolderMaskMargin( GetLocalSolderMaskMargin() );
    aModule->SetLocalClearance( GetLocalClearance() );
    aModule->SetLocalSolderPasteMargin( GetLocalSolderPasteMargin() );
    aModule->SetLocalSolderPasteMarginRatio( GetLocalSolderPasteMarginRatio() );
    aModule->SetZoneConnection( GetZoneConnection() );
    aModule->SetThermalWidth( GetThermalWidth() );
    aModule->SetThermalGap( GetThermalGap() );

    for( D_PAD* pad = Pads();  pad;  pad = pad->Next() )
    {
        D_PAD* newPad = aModule->FindPadByName( pad->GetPadName() );

        if( newPad )
            pad->CopyNetlistSettings( newPad );
    }

    // Not sure about copying description, keywords, 3D models or any other
    // local user changes to footprint.  Stick with the new footprint settings
    // called out in the footprint loaded in the netlist.
    aModule->CalculateBoundingBox();
}


void MODULE::Draw( EDA_DRAW_PANEL* aPanel, wxDC* aDC, GR_DRAWMODE aDrawMode,
                   const wxPoint& aOffset )
{
    if( (m_Flags & DO_NOT_DRAW) || (IsMoving()) )
        return;

    for( D_PAD* pad = m_Pads;  pad;  pad = pad->Next() )
    {
        if( pad->IsMoving() )
            continue;

        pad->Draw( aPanel, aDC, aDrawMode, aOffset );
    }

    BOARD* brd = GetBoard();

    // Draws footprint anchor
    DrawAncre( aPanel, aDC, aOffset, DIM_ANCRE_MODULE, aDrawMode );

    // Draw graphic items
    if( brd->IsElementVisible( MOD_REFERENCES_VISIBLE ) )
    {
        if( !(m_Reference->IsMoving()) )
            m_Reference->Draw( aPanel, aDC, aDrawMode, aOffset );
    }

    if( brd->IsElementVisible( MOD_VALUES_VISIBLE ) )
    {
        if( !(m_Value->IsMoving()) )
            m_Value->Draw( aPanel, aDC, aDrawMode, aOffset );
    }

    for( BOARD_ITEM* item = m_Drawings;  item;  item = item->Next() )
    {
        if( item->IsMoving() )
            continue;

        switch( item->Type() )
        {
        case PCB_MODULE_TEXT_T:
        case PCB_MODULE_EDGE_T:
            item->Draw( aPanel, aDC, aDrawMode, aOffset );
            break;

        default:
            break;
        }
    }

    // Enable these line to draw m_BoundaryBox (debug tests purposes only)
#if 0
    GRRect( aPanel->GetClipBox(), aDC, m_BoundaryBox, 0, BROWN );
#endif

}


void MODULE::DrawEdgesOnly( EDA_DRAW_PANEL* panel, wxDC* DC, const wxPoint& offset,
                            GR_DRAWMODE draw_mode )
{
    for( BOARD_ITEM* item = m_Drawings;  item;  item = item->Next() )
    {
        switch( item->Type() )
        {
        case PCB_MODULE_EDGE_T:
            item->Draw( panel, DC, draw_mode, offset );
            break;

        default:
            break;
        }
    }
}


void MODULE::CalculateBoundingBox()
{
    m_BoundaryBox = GetFootprintRect();
    m_Surface = std::abs( (double) m_BoundaryBox.GetWidth() * m_BoundaryBox.GetHeight() );
}


EDA_RECT MODULE::GetFootprintRect() const
{
    EDA_RECT area;

    area.SetOrigin( m_Pos );
    area.SetEnd( m_Pos );
    area.Inflate( Millimeter2iu( 0.25 ) );   // Give a min size to the area

    for( const BOARD_ITEM* item = m_Drawings.GetFirst(); item; item = item->Next() )
    {
        const EDGE_MODULE* edge = dyn_cast<const EDGE_MODULE*>( item );

        if( edge )
            area.Merge( edge->GetBoundingBox() );
    }

    for( D_PAD* pad = m_Pads;  pad;  pad = pad->Next() )
        area.Merge( pad->GetBoundingBox() );

    return area;
}


const EDA_RECT MODULE::GetBoundingBox() const
{
    EDA_RECT area = GetFootprintRect();

    // Calculate extended area including text fields
    area.Merge( m_Reference->GetBoundingBox() );
    area.Merge( m_Value->GetBoundingBox() );

    // Add the Clearance shape size: (shape around the pads when the
    // clearance is shown.  Not optimized, but the draw cost is small
    // (perhaps smaller than optimization).
    BOARD* board = GetBoard();
    if( board )
    {
        int biggest_clearance = board->GetDesignSettings().GetBiggestClearanceValue();
        area.Inflate( biggest_clearance );
    }

    return area;
}


/* Virtual function, from EDA_ITEM.
 * display module info on MsgPanel
 */
void MODULE::GetMsgPanelInfo( std::vector< MSG_PANEL_ITEM >& aList )
{
    int      nbpad;
    char     bufcar[512], Line[512];
    wxString msg;

    aList.push_back( MSG_PANEL_ITEM( m_Reference->GetText(), m_Value->GetText(), DARKCYAN ) );

    // Display last date the component was edited (useful in Module Editor).
    time_t edit_time = m_LastEditTime;
    strcpy( Line, ctime( &edit_time ) );
    strtok( Line, " \n\r" );
    strcpy( bufcar, strtok( NULL, " \n\r" ) ); strcat( bufcar, " " );
    strcat( bufcar, strtok( NULL, " \n\r" ) ); strcat( bufcar, ", " );
    strtok( NULL, " \n\r" );
    strcat( bufcar, strtok( NULL, " \n\r" ) );
    msg = FROM_UTF8( bufcar );
    aList.push_back( MSG_PANEL_ITEM( _( "Last Change" ), msg, BROWN ) );

    // display schematic path
    aList.push_back( MSG_PANEL_ITEM( _( "Netlist path" ), m_Path, BROWN ) );

    aList.push_back( MSG_PANEL_ITEM( _( "Layer" ), GetLayerName(), RED ) );

    EDA_ITEM* PtStruct = m_Pads;
    nbpad = 0;

    while( PtStruct )
    {
        nbpad++;
        PtStruct = PtStruct->Next();
    }

    msg.Printf( wxT( "%d" ), nbpad );
    aList.push_back( MSG_PANEL_ITEM( _( "Pads" ), msg, BLUE ) );

    msg = wxT( ".." );

    if( IsLocked() )
        msg[0] = 'L';

    if( m_ModuleStatus & MODULE_is_PLACED )
        msg[1] = 'P';

    aList.push_back( MSG_PANEL_ITEM( _( "Stat" ), msg, MAGENTA ) );

    msg.Printf( wxT( "%.1f" ), m_Orient / 10.0 );
    aList.push_back( MSG_PANEL_ITEM( _( "Orient" ), msg, BROWN ) );

    // Controls on right side of the dialog
    switch( m_Attributs & 255 )
    {
    case 0:
        msg = _("Normal");
        break;

    case MOD_CMS:
        msg = _("Insert");
        break;

    case MOD_VIRTUAL:
        msg = _("Virtual");
        break;

    default:
        msg = wxT("???");
        break;
    }

    aList.push_back( MSG_PANEL_ITEM( _( "Attrib" ), msg, BROWN ) );
    aList.push_back( MSG_PANEL_ITEM( _( "Module" ), FROM_UTF8( m_fpid.Format().c_str() ), BLUE ) );

    msg = _( "No 3D shape" );
    // Search the first active 3D shape in list
    for( S3D_MASTER* struct3D = m_3D_Drawings; struct3D; struct3D = struct3D->Next() )
    {
        if( !struct3D->GetShape3DName().IsEmpty() )
        {
            msg = struct3D->GetShape3DName();
            break;
        }
    }

    aList.push_back( MSG_PANEL_ITEM( _( "3D-Shape" ), msg, RED ) );

    wxString doc, keyword;
    doc.Printf( _( "Doc: %s" ), GetChars( m_Doc ) );
    keyword.Printf( _( "KeyW: %s" ), GetChars( m_KeyWord ) );
    aList.push_back( MSG_PANEL_ITEM( doc, keyword, BLACK ) );
}


bool MODULE::HitTest( const wxPoint& aPosition ) const
{
    if( m_BoundaryBox.Contains( aPosition ) )
        return true;

    return false;
}


bool MODULE::HitTest( const EDA_RECT& aRect, bool aContained, int aAccuracy ) const
{
    EDA_RECT arect = aRect;
    arect.Inflate( aAccuracy );

    if( aContained )
        return arect.Contains( m_BoundaryBox );
    else
        return m_BoundaryBox.Intersects( arect );
}


D_PAD* MODULE::FindPadByName( const wxString& aPadName ) const
{
    wxString buf;

    for( D_PAD* pad = m_Pads;  pad;  pad = pad->Next() )
    {
        pad->StringPadName( buf );
#if 1
        if( buf.CmpNoCase( aPadName ) == 0 )    // why case insensitive?
#else
        if( buf == aPadName )
#endif
            return pad;
    }

    return NULL;
}


D_PAD* MODULE::GetPad( const wxPoint& aPosition, LSET aLayerMask )
{
    for( D_PAD* pad = m_Pads;  pad;  pad = pad->Next() )
    {
        // ... and on the correct layer.
        if( !( pad->GetLayerSet() & aLayerMask ).any() )
            continue;

        if( pad->HitTest( aPosition ) )
            return pad;
    }

    return NULL;
}


unsigned MODULE::GetPadCount( INCLUDE_NPTH_T aIncludeNPTH ) const
{
    if( aIncludeNPTH )
        return m_Pads.GetCount();

    unsigned cnt = 0;

    for( D_PAD* pad = m_Pads; pad; pad = pad->Next() )
    {
        if( pad->GetAttribute() == PAD_HOLE_NOT_PLATED )
            continue;

        cnt++;
    }

    return cnt;
}


void MODULE::Add3DModel( S3D_MASTER* a3DModel )
{
    a3DModel->SetParent( this );
    m_3D_Drawings.PushBack( a3DModel );
}


void MODULE::AddPad( D_PAD* aPad )
{
    aPad->SetParent( this );
    m_Pads.PushBack( aPad );
}


// see class_module.h
SEARCH_RESULT MODULE::Visit( INSPECTOR* inspector, const void* testData,
                             const KICAD_T scanTypes[] )
{
    KICAD_T        stype;
    SEARCH_RESULT  result = SEARCH_CONTINUE;
    const KICAD_T* p    = scanTypes;
    bool           done = false;

#if 0 && defined(DEBUG)
    std::cout << GetClass().mb_str() << ' ';
#endif

    while( !done )
    {
        stype = *p;

        switch( stype )
        {
        case PCB_MODULE_T:
            result = inspector->Inspect( this, testData );  // inspect me
            ++p;
            break;

        case PCB_PAD_T:
            result = IterateForward( m_Pads, inspector, testData, p );
            ++p;
            break;

        case PCB_MODULE_TEXT_T:
            result = inspector->Inspect( m_Reference, testData );

            if( result == SEARCH_QUIT )
                break;

            result = inspector->Inspect( m_Value, testData );

            if( result == SEARCH_QUIT )
                break;

        // m_Drawings can hold TYPETEXTMODULE also, so fall thru

        case PCB_MODULE_EDGE_T:
            result = IterateForward( m_Drawings, inspector, testData, p );

            // skip over any types handled in the above call.
            for( ; ; )
            {
                switch( stype = *++p )
                {
                case PCB_MODULE_TEXT_T:
                case PCB_MODULE_EDGE_T:
                    continue;

                default:
                    ;
                }

                break;
            }

            break;

        default:
            done = true;
            break;
        }

        if( result == SEARCH_QUIT )
            break;
    }

    return result;
}


wxString MODULE::GetSelectMenuText() const
{
    wxString text;
    text.Printf( _( "Footprint %s on %s" ),
                 GetChars ( GetReference() ),
                 GetChars ( GetLayerName() ) );

    return text;
}


EDA_ITEM* MODULE::Clone() const
{
    return new MODULE( *this );
}


void MODULE::RunOnChildren( boost::function<void (BOARD_ITEM*)> aFunction )
{
    for( D_PAD* pad = m_Pads.GetFirst(); pad; pad = pad->Next() )
        aFunction( static_cast<BOARD_ITEM*>( pad ) );

    for( BOARD_ITEM* drawing = m_Drawings.GetFirst(); drawing; drawing = drawing->Next() )
        aFunction( drawing );

    aFunction( static_cast<BOARD_ITEM*>( m_Reference ) );
    aFunction( static_cast<BOARD_ITEM*>( m_Value ) );
}


void MODULE::ViewUpdate( int aUpdateFlags )
{
    if( !m_view )
        return;

    // Update the module itself
    VIEW_ITEM::ViewUpdate( aUpdateFlags );

    // Update pads
    for( D_PAD* pad = m_Pads.GetFirst(); pad; pad = pad->Next() )
        pad->ViewUpdate( aUpdateFlags );

    // Update module's drawing (mostly silkscreen)
    for( BOARD_ITEM* drawing = m_Drawings.GetFirst(); drawing; drawing = drawing->Next() )
        drawing->ViewUpdate( aUpdateFlags );

    // Update module's texts
    m_Reference->ViewUpdate( aUpdateFlags );
    m_Value->ViewUpdate( aUpdateFlags );
}


/* Test for validity of the name in a library of the footprint
 * ( no spaces, dir separators ... )
 * return true if the given name is valid
 * static function
 */
bool MODULE::IsLibNameValid( const wxString & aName )
{
    const wxChar * invalids = StringLibNameInvalidChars( false );

    if( aName.find_first_of( invalids ) != std::string::npos )
        return false;

    return true;
}


/* Test for validity of the name of a footprint to be used in a footprint library
 * ( no spaces, dir separators ... )
 * param bool aUserReadable = false to get the list of invalid chars
 *        true to get a readable form (i.e ' ' = 'space' '\t'= 'tab')
 * return a constant string giving the list of invalid chars in lib name
 * static function
 */
const wxChar* MODULE::StringLibNameInvalidChars( bool aUserReadable )
{
    static const wxChar invalidChars[] = wxT("%$\t \"\\/");
    static const wxChar invalidCharsReadable[] = wxT("% $ 'tab' 'space' \\ \" /");

    if( aUserReadable )
        return invalidCharsReadable;
    else
        return invalidChars;
}


void MODULE::Move( const wxPoint& aMoveVector )
{
    wxPoint newpos = m_Pos + aMoveVector;
    SetPosition( newpos );
}


void MODULE::Rotate( const wxPoint& aRotCentre, double aAngle )
{
    wxPoint newpos = m_Pos;
    RotatePoint( &newpos, aRotCentre, aAngle );
    SetPosition( newpos );
    SetOrientation( GetOrientation() + aAngle );
}


void MODULE::Flip( const wxPoint& aCentre )
{
    TEXTE_MODULE* text;

    // Move module to its final position:
    wxPoint finalPos = m_Pos;

    finalPos.y  = aCentre.y - ( finalPos.y - aCentre.y );     /// Mirror the Y position

    SetPosition( finalPos );

    // Flip layer
    SetLayer( FlipLayer( GetLayer() ) );

    // Reverse mirror orientation.
    NEGATE( m_Orient );
    NORMALIZE_ANGLE_POS( m_Orient );

    // Mirror pads to other side of board about the x axis, i.e. vertically.
    for( D_PAD* pad = m_Pads; pad; pad = pad->Next() )
        pad->Flip( m_Pos );

    // Mirror reference.
    text = m_Reference;
    text->m_Pos.y -= m_Pos.y;
    NEGATE( text->m_Pos.y );
    text->m_Pos.y += m_Pos.y;
    NEGATE(text->m_Pos0.y);
    NEGATE_AND_NORMALIZE_ANGLE_POS( text->m_Orient );
    text->SetLayer( FlipLayer( text->GetLayer() ) );
    text->m_Mirror = IsBackLayer( GetLayer() );

    // Mirror value.
    text = m_Value;
    text->m_Pos.y -= m_Pos.y;
    NEGATE( text->m_Pos.y );
    text->m_Pos.y += m_Pos.y;
    NEGATE( text->m_Pos0.y );
    NEGATE_AND_NORMALIZE_ANGLE_POS( text->m_Orient );
    text->SetLayer( FlipLayer( text->GetLayer() ) );
    text->m_Mirror = IsBackLayer( GetLayer() );

    // Reverse mirror module graphics and texts.
    for( EDA_ITEM* item = m_Drawings; item; item = item->Next() )
    {
        switch( item->Type() )
        {
        case PCB_MODULE_EDGE_T:
            {
                EDGE_MODULE* em = (EDGE_MODULE*) item;

                wxPoint s = em->GetStart();
                s.y -= m_Pos.y;
                s.y  = -s.y;
                s.y += m_Pos.y;
                em->SetStart( s );

                wxPoint e = em->GetEnd();
                e.y -= m_Pos.y;
                e.y  = -e.y;
                e.y += m_Pos.y;
                em->SetEnd( e );

                NEGATE( em->m_Start0.y );
                NEGATE( em->m_End0.y );

                if( em->GetShape() == S_ARC )
                {
                    em->SetAngle( -em->GetAngle() );
                }

                em->SetLayer( FlipLayer( em->GetLayer() ) );
            }
            break;

        case PCB_MODULE_TEXT_T:
            text = (TEXTE_MODULE*) item;
            text->m_Pos.y -= m_Pos.y;
            NEGATE( text->m_Pos.y );
            text->m_Pos.y += m_Pos.y;
            NEGATE( text->m_Pos0.y );
            NEGATE_AND_NORMALIZE_ANGLE_POS( text->m_Orient );
            text->SetLayer( FlipLayer( text->GetLayer() ) );
            text->m_Mirror = IsBackLayer( GetLayer() );
            break;

        default:
            wxMessageBox( wxT( "MODULE::Flip() error: Unknown Draw Type" ) );
            break;
        }
    }

    CalculateBoundingBox();
}


void MODULE::SetPosition( const wxPoint& newpos )
{
    wxPoint delta = newpos - m_Pos;

    m_Pos += delta;
    m_Reference->SetTextPosition( m_Reference->GetTextPosition() + delta );
    m_Value->SetTextPosition( m_Value->GetTextPosition() + delta );

    for( D_PAD* pad = m_Pads;  pad;  pad = pad->Next() )
    {
        pad->SetPosition( pad->GetPosition() + delta );
    }

    for( EDA_ITEM* item = m_Drawings;  item;  item = item->Next() )
    {
        switch( item->Type() )
        {
        case PCB_MODULE_EDGE_T:
        {
            EDGE_MODULE* pt_edgmod = (EDGE_MODULE*) item;
            pt_edgmod->SetDrawCoord();
            break;
        }

        case PCB_MODULE_TEXT_T:
        {
            TEXTE_MODULE* text = (TEXTE_MODULE*) item;
            text->m_Pos += delta;
            break;
        }

        default:
            wxMessageBox( wxT( "Draw type undefined." ) );
            break;
        }
    }

    CalculateBoundingBox();
}


void MODULE::MoveAnchorPosition( const wxPoint& aMoveVector )
{
    /* Move the reference point of the footprint
     * the footprints elements (pads, outlines, edges .. ) are moved
     * but:
     * - the footprint position is not modified.
     * - the relative (local) coordinates of these items are modified
     */

    wxPoint footprintPos = GetPosition();

    /* Update the relative coordinates:
     * The coordinates are relative to the anchor point.
     * Calculate deltaX and deltaY from the anchor. */
    wxPoint moveVector = aMoveVector;
    RotatePoint( &moveVector, -GetOrientation() );

    // Update of the reference and value.
    m_Reference->SetPos0( m_Reference->GetPos0() + moveVector );
    m_Reference->SetDrawCoord();
    m_Value->SetPos0( m_Value->GetPos0() + moveVector );
    m_Value->SetDrawCoord();

    // Update the pad local coordinates.
    for( D_PAD* pad = Pads(); pad; pad = pad->Next() )
    {
        pad->SetPos0( pad->GetPos0() + moveVector );
        pad->SetPosition( pad->GetPos0() + footprintPos );
    }

    // Update the draw element coordinates.
    for( EDA_ITEM* item = GraphicalItems(); item; item = item->Next() )
    {
        switch( item->Type() )
        {
        case PCB_MODULE_EDGE_T:
            #undef STRUCT
            #define STRUCT ( (EDGE_MODULE*) item )
            STRUCT->m_Start0 += moveVector;
            STRUCT->m_End0   += moveVector;
            STRUCT->SetDrawCoord();
            break;

        case PCB_MODULE_TEXT_T:
            #undef STRUCT
            #define STRUCT ( (TEXTE_MODULE*) item )
            STRUCT->SetPos0( STRUCT->GetPos0() + moveVector );
            STRUCT->SetDrawCoord();
            break;

        default:
            break;
        }
    }

    CalculateBoundingBox();
}


void MODULE::SetOrientation( double newangle )
{
    double  angleChange = newangle - m_Orient;  // change in rotation
    wxPoint pt;

    NORMALIZE_ANGLE_POS( newangle );

    m_Orient = newangle;

    for( D_PAD* pad = m_Pads;  pad;  pad = pad->Next() )
    {
        pt = pad->GetPos0();

        pad->SetOrientation( pad->GetOrientation() + angleChange );

        RotatePoint( &pt, m_Orient );

        pad->SetPosition( GetPosition() + pt );
    }

    // Update of the reference and value.
    m_Reference->SetDrawCoord();
    m_Value->SetDrawCoord();

    // Displace contours and text of the footprint.
    for( BOARD_ITEM* item = m_Drawings;  item;  item = item->Next() )
    {
        if( item->Type() == PCB_MODULE_EDGE_T )
        {
            EDGE_MODULE* edge = (EDGE_MODULE*) item;
            edge->SetDrawCoord();
        }

        else if( item->Type() == PCB_MODULE_TEXT_T )
        {
            TEXTE_MODULE* text = (TEXTE_MODULE*) item;
            text->SetDrawCoord();
        }
    }

    CalculateBoundingBox();
}

