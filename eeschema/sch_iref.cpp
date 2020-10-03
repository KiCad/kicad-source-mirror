/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2019-2020 Franck Jullien, franck.jullien at gmail.com
 * Copyright (C) 2019-2020 KiCad Developers, see AUTHORS.txt for contributors.
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
#include <schematic.h>
#include <tool/tool_manager.h>
#include <tools/sch_editor_control.h>
#include <tools/sch_navigate_tool.h>

SCH_IREF::SCH_IREF( const wxPoint& pos, const wxString& text, SCH_GLOBALLABEL* aParent,
                    KICAD_T aType ) :
        SCH_TEXT( pos, text, SCH_IREF_T )
{
    m_Layer  = LAYER_GLOBLABEL;
    m_parentLabel = aParent;
    SetMultilineAllowed( false );
    m_screen = nullptr;
}


void SCH_IREF::PlaceAtDefaultPosition()
{
    wxPoint offset;

    int labelLen = m_parentLabel->GetBoundingBox().GetSizeMax();

    switch( m_parentLabel->GetLabelSpinStyle() )
    {
    default:
    case LABEL_SPIN_STYLE::LEFT:   offset.x -= labelLen; break;
    case LABEL_SPIN_STYLE::UP:     offset.y -= labelLen; break;
    case LABEL_SPIN_STYLE::RIGHT:  offset.x += labelLen; break;
    case LABEL_SPIN_STYLE::BOTTOM: offset.y += labelLen; break;
    }

    SetTextPos( m_parentLabel->GetPosition() + offset );
}

wxPoint SCH_IREF::GetSchematicTextOffset( RENDER_SETTINGS* aSettings ) const
{
    return m_parentLabel->GetSchematicTextOffset( aSettings );
}


EDA_ITEM* SCH_IREF::Clone() const
{
    return new SCH_IREF( *this );
}


void SCH_IREF::SetIrefOrientation( LABEL_SPIN_STYLE aSpinStyle )
{
    wxPoint pt     = GetTextPos() - GetParent()->GetPosition();
    int     offset = std::max( abs( pt.x ), abs( pt.y ) );

    switch( aSpinStyle )
    {
    case LABEL_SPIN_STYLE::RIGHT:
        SetTextAngle( TEXT_ANGLE_HORIZ );
        SetHorizJustify( GR_TEXT_HJUSTIFY_LEFT );
        SetVertJustify( GR_TEXT_VJUSTIFY_CENTER );
        pt.y = 0;
        pt.x = offset;
        break;
    case LABEL_SPIN_STYLE::UP:
        SetTextAngle( TEXT_ANGLE_VERT );
        SetHorizJustify( GR_TEXT_HJUSTIFY_LEFT );
        SetVertJustify( GR_TEXT_VJUSTIFY_CENTER );
        pt.y = -offset;
        pt.x = 0;
        break;
    case LABEL_SPIN_STYLE::LEFT:
        SetTextAngle( TEXT_ANGLE_HORIZ );
        SetHorizJustify( GR_TEXT_HJUSTIFY_RIGHT );
        SetVertJustify( GR_TEXT_VJUSTIFY_CENTER );
        pt.y = 0;
        pt.x = -offset;
        break;
    case LABEL_SPIN_STYLE::BOTTOM:
        SetTextAngle( TEXT_ANGLE_VERT );
        SetHorizJustify( GR_TEXT_HJUSTIFY_RIGHT );
        SetVertJustify( GR_TEXT_VJUSTIFY_CENTER );
        pt.y = offset;
        pt.x = 0;
        break;
    }

    SetPosition( GetParent()->GetPosition() + pt );
}


void SCH_IREF::CopyParentStyle()
{
    SetTextSize( m_parentLabel->GetTextSize() );
    SetItalic( m_parentLabel->IsItalic() );
    SetBold( m_parentLabel->IsBold() );
    SetTextThickness( m_parentLabel->GetTextThickness() );
    SetIrefOrientation( m_parentLabel->GetLabelSpinStyle() );
}


void SCH_IREF::BuildHypertextMenu( wxMenu* aMenu )
{
    std::map<int, wxString> sheetNames;

    for( const SCH_SHEET_PATH& sheet : Schematic()->GetSheets() )
        sheetNames[ sheet.GetPageNumber() ] = sheet.Last()->GetName();

    for( int i : m_refTable )
    {
        aMenu->Append( i, wxString::Format( _( "Go to Page %d (%s)" ),
                                            i,
                                            i == 1 ? _( "Root" ) : sheetNames[ i ] ) );
    }

    aMenu->AppendSeparator();
    aMenu->Append( ID_HYPERTEXT_BACK, _( "Back" ) );
}