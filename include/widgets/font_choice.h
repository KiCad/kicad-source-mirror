/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2021-2022 KiCad Developers, see AUTHORS.txt for contributors.
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

#ifndef FONT_CHOICE_H
#define FONT_CHOICE_H

#include <wx/choice.h>
#include <wx/fontenum.h>
#include <font/font.h>


class FONT_CHOICE : public wxChoice
{
public:
    FONT_CHOICE( wxWindow* aParent, int aId, wxPoint aPosition, wxSize aSize, int nChoices,
                 wxString* aChoices, int aStyle );

    virtual ~FONT_CHOICE();

    void SetFontSelection( KIFONT::FONT* aFont );

    bool HaveFontSelection() const;

    KIFONT::FONT* GetFontSelection( bool aBold, bool aItalic ) const;

private:
    int       m_systemFontCount;
    wxString  m_notFound;
};

#endif // FONT_CHOICE_H
