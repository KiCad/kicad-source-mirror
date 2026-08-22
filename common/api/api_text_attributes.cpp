/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <api/api_utils.h>
#include <api/api_enums.h>
#include <api/common/types/base_types.pb.h>

#include <font/font.h>
#include <font/text_attributes.h>


namespace kiapi::common
{

void PackTextAttributes( types::TextAttributes& aOutput, const TEXT_ATTRIBUTES& aInput )
{
    if( aInput.m_Font )
        aOutput.set_font_name( aInput.m_Font->GetName().ToStdString() );

    aOutput.set_horizontal_alignment( ToProtoEnum<GR_TEXT_H_ALIGN_T, types::HorizontalAlignment>( aInput.m_Halign ) );
    aOutput.set_vertical_alignment( ToProtoEnum<GR_TEXT_V_ALIGN_T, types::VerticalAlignment>( aInput.m_Valign ) );
    aOutput.mutable_angle()->set_value_degrees( aInput.m_Angle.AsDegrees() );
    aOutput.set_line_spacing( aInput.m_LineSpacing );
    aOutput.mutable_stroke_width()->set_value_nm( aInput.m_StrokeWidth );
    aOutput.set_italic( aInput.m_Italic );
    aOutput.set_bold( aInput.m_Bold );
    aOutput.set_underlined( aInput.m_Underlined );
    aOutput.set_mirrored( aInput.m_Mirrored );
    aOutput.set_multiline( aInput.m_Multiline );
    aOutput.set_keep_upright( aInput.m_KeepUpright );

    PackVector2( *aOutput.mutable_size(), aInput.m_Size );
}


void UnpackTextAttributes( TEXT_ATTRIBUTES& aOutput, const types::TextAttributes& aInput )
{
    aOutput.m_Bold = aInput.bold();
    aOutput.m_Italic = aInput.italic();
    aOutput.m_Underlined = aInput.underlined();
    aOutput.m_Mirrored = aInput.mirrored();
    aOutput.m_Multiline = aInput.multiline();
    aOutput.m_KeepUpright = aInput.keep_upright();
    aOutput.m_Size = UnpackVector2( aInput.size() );

    if( !aInput.font_name().empty() )
    {
        aOutput.m_Font = KIFONT::FONT::GetFont( wxString::FromUTF8( aInput.font_name().c_str() ), aOutput.m_Bold,
                                                aOutput.m_Italic );
    }

    aOutput.m_Angle = EDA_ANGLE( aInput.angle().value_degrees(), DEGREES_T );
    aOutput.m_LineSpacing = aInput.line_spacing();
    aOutput.m_StrokeWidth = aInput.stroke_width().value_nm();
    aOutput.m_Halign = FromProtoEnum<GR_TEXT_H_ALIGN_T, types::HorizontalAlignment>( aInput.horizontal_alignment() );
    aOutput.m_Valign = FromProtoEnum<GR_TEXT_V_ALIGN_T, types::VerticalAlignment>( aInput.vertical_alignment() );
}

} // namespace kiapi::common
