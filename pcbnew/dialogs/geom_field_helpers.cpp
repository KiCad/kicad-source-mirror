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
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include <dialogs/geom_field_helpers.h>

#include <widgets/unit_binder.h>

#include <wx/gbsizer.h>
#include <wx/stattext.h>
#include <wx/textctrl.h>


void AddXYPointToSizer( EDA_DRAW_FRAME& aFrame, wxGridBagSizer& aSizer, int row, int col,
                        const wxString& aName, bool aRelative,
                        std::vector<BOUND_CONTROL>& aBoundCtrls )
{
    //    Name
    // X [Ctrl] mm
    // Y [Ctrl] mm
    wxWindow* parent = aSizer.GetContainingWindow();

    wxStaticText* titleLabel = new wxStaticText( parent, wxID_ANY, aName );
    aSizer.Add( titleLabel, wxGBPosition( row, col ), wxGBSpan( 1, 3 ),
                wxALIGN_CENTER_VERTICAL | wxALIGN_CENTER_HORIZONTAL | wxALL | wxEXPAND );
    row++;

    for( size_t coord = 0; coord < 2; ++coord )
    {
        wxStaticText* label = new wxStaticText( parent, wxID_ANY, coord == 0 ? _( "X:" ) : _( "Y:" ) );
        aSizer.Add( label, wxGBPosition( row, col ), wxDefaultSpan,
                    wxALIGN_LEFT | wxALIGN_CENTER_VERTICAL | wxLEFT, col > 0 ? 20 : 5 );

        wxTextCtrl* ctrl = new wxTextCtrl( parent, wxID_ANY, "" );
        aSizer.Add( ctrl, wxGBPosition( row, col + 1 ), wxDefaultSpan,
                    wxEXPAND | wxALIGN_CENTER_VERTICAL, 5 );

        wxStaticText* units = new wxStaticText( parent, wxID_ANY, _( "mm" ) );
        aSizer.Add( units, wxGBPosition( row, col + 2 ), wxDefaultSpan,
                    wxALIGN_LEFT | wxALIGN_CENTER_VERTICAL | wxRIGHT, 5 );

        auto binder = std::make_unique<UNIT_BINDER>( &aFrame, label, ctrl, units );

        if( aRelative )
            binder->SetCoordType( coord == 0 ? ORIGIN_TRANSFORMS::REL_X_COORD : ORIGIN_TRANSFORMS::REL_Y_COORD );
        else
            binder->SetCoordType( coord == 0 ? ORIGIN_TRANSFORMS::ABS_X_COORD : ORIGIN_TRANSFORMS::ABS_Y_COORD );

        aBoundCtrls.push_back( BOUND_CONTROL{ std::move( binder ), ctrl } );
        row++;
    }

    if( !aSizer.IsColGrowable( col + 1 ) )
        aSizer.AddGrowableCol( col + 1 );
}


void AddFieldToSizer( EDA_DRAW_FRAME& aFrame, wxGridBagSizer& aSizer, int row, int col,
                      const wxString& aName, ORIGIN_TRANSFORMS::COORD_TYPES_T aCoordType,
                      bool aIsAngle, std::vector<BOUND_CONTROL>& aBoundCtrls )
{
    // Name: [Ctrl] mm
    wxWindow* parent = aSizer.GetContainingWindow();

    wxStaticText* label = new wxStaticText( parent, wxID_ANY, aName + wxS( ":" ) );
    aSizer.Add( label, wxGBPosition( row, col ), wxDefaultSpan,
                wxALIGN_LEFT | wxALIGN_CENTER_VERTICAL | wxLEFT, col > 0 ? 20 : 5 );

    wxTextCtrl* ctrl = new wxTextCtrl( parent, wxID_ANY );
    aSizer.Add( ctrl, wxGBPosition( row, col + 1 ), wxDefaultSpan,
                wxEXPAND | wxALIGN_CENTER_VERTICAL, 5 );

    wxStaticText* units = new wxStaticText( parent, wxID_ANY, _( "mm" ) );
    aSizer.Add( units, wxGBPosition( row, col + 2 ), wxDefaultSpan,
                wxALIGN_LEFT | wxALIGN_CENTER_VERTICAL | wxRIGHT, 5 );

    auto binder = std::make_unique<UNIT_BINDER>( &aFrame, label, ctrl, units );
    binder->SetCoordType( aCoordType );

    if( aIsAngle )
    {
        binder->SetPrecision( 4 );
        binder->SetUnits( EDA_UNITS::DEGREES );
    }

    aBoundCtrls.push_back( BOUND_CONTROL{ std::move( binder ), ctrl } );

    if( !aSizer.IsColGrowable( col + 1 ) )
        aSizer.AddGrowableCol( col + 1 );
}
