/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2019 Franck Jullien, franck.jullien at gmail.com
 * Copyright (C) 2019 KiCad Developers, see AUTHORS.txt for contributors.
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

#include <sch_text.h>
#include <sch_iref.h>
#include <sch_sheet.h>
#include <tools/sch_editor_control.h>

std::vector<SCH_GLOBALLABEL*> gLabelTable;

SCH_IREF::SCH_IREF(
        const wxPoint& pos, const wxString& text, SCH_GLOBALLABEL* aParent, KICAD_T aType )
        : SCH_TEXT( pos, text, SCH_IREF_T )

{
    m_Layer = LAYER_GLOBLABEL;
    m_parent = aParent;
    SetMultilineAllowed( false );
}

void buildHierarchyTree( SCH_SHEET_PATH* aList )
{
    SCH_ITEM* schitem = aList->LastDrawList();

    while( schitem )
    {
        if( schitem->Type() == SCH_SHEET_T )
        {
            SCH_SHEET* sheet = (SCH_SHEET*) schitem;
            aList->push_back( sheet );
            buildHierarchyTree( aList );
        }

        schitem = schitem->Next();
    }
}

void SCH_IREF::PlaceAtDefaultPosition()
{
    wxPoint offset;

    int labelLen = m_parent->GetBoundingBox().GetSizeMax();

    switch( m_parent->GetLabelSpinStyle() )
    {
    default:
    case 0:
        offset.x -= labelLen;
        break;
    case 1:
        offset.y -= labelLen;
        break;
    case 2:
        offset.x += labelLen;
        break;
    case 3:
        offset.y += labelLen;
        break;
    }

    SetPosition( m_parent->GetPosition() + offset );
}

int SCH_EDITOR_CONTROL::IntersheetsRefs( const TOOL_EVENT& aEvent )
{
    SCH_IREF*        iref;
    SCH_GLOBALLABEL* gLabel;
    SCH_SHEET_PATH   list;
    SCH_SHEET*       sheet;
    wxString         labelsWithoutIref;

    list.push_back( g_RootSheet );

    buildHierarchyTree( &list );

    gLabelTable.clear();

    for( size_t i = 0; i < list.size(); i++ )
    {
        sheet = list.GetSheet( i );

        for( SCH_ITEM* item = sheet->GetScreen()->GetDrawItems(); item; item = item->Next() )
        {
            if( item->Type() == SCH_GLOBAL_LABEL_T )
            {
                gLabel = static_cast<SCH_GLOBALLABEL*>( item );
                gLabelTable.push_back( gLabel );
                if( gLabel->GetIref() == nullptr )
                {
                    iref = new SCH_IREF();
                    gLabel->SetIref( iref );

                    iref->SetParent( gLabel );
                    iref->SetOwnPageNumber( sheet->GetScreen()->m_ScreenNumber );
                    iref->GetRefTable()->clear();
                    iref->SetFlags( IS_NEW );
                    iref->SetScreen( sheet->GetScreen() );
                    iref->SetLabelSpinStyle( gLabel->GetLabelSpinStyle() );
                    if( gLabel->GetIrefSavedPosition() != wxDefaultPosition )
                        iref->SetPosition( gLabel->GetIrefSavedPosition() );
                    else
                        iref->PlaceAtDefaultPosition();
                    iref->CopyParentStyle();
                }
                else
                {
                    iref = gLabel->GetIref();
                    iref->GetRefTable()->clear();
                }
            }
        }
    }

    /* Fill intersheets references for each global label */
    for( SCH_GLOBALLABEL* item : gLabelTable )
    {
        for( SCH_GLOBALLABEL* iter : gLabelTable )
        {
            if( iter->GetText().IsSameAs( item->GetText() ) && ( iter != item ) )
                iter->GetIref()->GetRefTable()->push_back( item->GetIref()->GetOwnPageNumber() );
        }
    }

    /* Refresh all global labels */
    for( SCH_GLOBALLABEL* item : gLabelTable )
    {
        wxString text, tmp;
        bool     hasIref = false;

        iref = item->GetIref();

        sort( iref->GetRefTable()->begin(), iref->GetRefTable()->end() );
        iref->GetRefTable()->erase(
                unique( iref->GetRefTable()->begin(), iref->GetRefTable()->end() ),
                iref->GetRefTable()->end() );


        text.Printf( "[" );

        for( int ref : *( iref->GetRefTable() ) )
        {
            tmp.Printf( "%d,", ref );
            text.Append( tmp );
            hasIref = true;
        }

        if( text.Last() == ',' )
            text.RemoveLast();

        text.Append( "]" );

        if( hasIref == false )
            labelsWithoutIref.Append(
                    wxString::Format( _( "Global Label \"%s\" doesn't have an iref\n" ),
                            iref->GetParent()->GetText() ) );


        iref->SetText( text );

        SCH_SCREEN* screen = iref->GetScreen();

        if( !screen->CheckIfOnDrawList( iref ) )
            m_frame->AddToScreen( iref, screen );

        iref->ClearFlags( IS_NEW );

        screen->SetModify();
        m_frame->RefreshItem( iref );

        iref->ClearEditFlags();
        m_frame->GetCanvas()->Refresh();
    }

    if( !labelsWithoutIref.IsEmpty() )
        wxMessageBox( labelsWithoutIref, "Intersheets references", wxOK | wxICON_INFORMATION );

    return 0;
}

EDA_ITEM* SCH_IREF::Clone() const
{
    return new SCH_IREF( *this );
}

void SCH_IREF::SetLabelSpinStyle( int aSpinStyle )
{
    wxPoint pt = GetTextPos();
    double  angle;

    switch( aSpinStyle )
    {
    case 0:
        switch( m_spin_style )
        {
        case 1:  angle = 900;  break;
        case 2:  angle = 1800; break;
        case 3:  angle = -900; break;
        default: angle = 0;    break;
        }
        break;
    case 1:
        switch( m_spin_style )
        {
        case 2:  angle = 900;  break;
        case 3:  angle = 1800; break;
        case 0:  angle = -900; break;
        default: angle = 0;    break;
        }
        break;
    case 2:
        switch( m_spin_style )
        {
        case 3:  angle = 900;  break;
        case 0:  angle = 1800; break;
        case 1:  angle = -900; break;
        default: angle = 0;    break;
        }
        break;
    case 3:
        switch( m_spin_style )
        {
        case 0:  angle = 900;  break;
        case 1:  angle = 1800; break;
        case 2:  angle = -900; break;
        default: angle = 0;    break;
        }
        break;
    default:
        angle = 0;
    }

    RotatePoint( &pt, GetParent()->GetPosition(), angle );
    SetTextPos( pt );

    m_spin_style = aSpinStyle;

    switch( aSpinStyle )
    {
    default:
        wxASSERT_MSG( 1, "Bad spin style" );
    case 0: // Horiz Normal Orientation
        //
        m_spin_style = 0; // Handle the error spin style by resetting
        SetTextAngle( TEXT_ANGLE_HORIZ );
        SetHorizJustify( GR_TEXT_HJUSTIFY_RIGHT );
        SetVertJustify( GR_TEXT_VJUSTIFY_CENTER );
        break;

    case 1: // Vert Orientation UP
        SetTextAngle( TEXT_ANGLE_VERT );
        SetHorizJustify( GR_TEXT_HJUSTIFY_LEFT );
        SetVertJustify( GR_TEXT_VJUSTIFY_CENTER );
        break;

    case 2: // Horiz Orientation - Left justified
        SetTextAngle( TEXT_ANGLE_HORIZ );
        SetHorizJustify( GR_TEXT_HJUSTIFY_LEFT );
        SetVertJustify( GR_TEXT_VJUSTIFY_CENTER );
        break;

    case 3: //  Vert Orientation BOTTOM
        SetTextAngle( TEXT_ANGLE_VERT );
        SetHorizJustify( GR_TEXT_HJUSTIFY_RIGHT );
        SetVertJustify( GR_TEXT_VJUSTIFY_CENTER );
        break;
    }
}

void SCH_IREF::CopyParentStyle()
{
    SetTextSize( m_parent->GetTextSize() );
    SetItalic( m_parent->IsItalic() );
    SetBold( m_parent->IsBold() );
    SetThickness( m_parent->GetThickness() );
    SetLabelSpinStyle( m_parent->GetLabelSpinStyle() );
}
