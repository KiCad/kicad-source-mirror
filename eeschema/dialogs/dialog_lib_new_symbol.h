/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2009 Wayne Stambaugh <stambaughw@gmail.com>
 * Copyright (C) 2015-2021 KiCad Developers, see AUTHORS.txt for contributors.
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

#ifndef __dialog_lib_new_symbol__
#define __dialog_lib_new_symbol__

#include <widgets/unit_binder.h>
#include <kicad_string.h>
#include <dialog_lib_new_symbol_base.h>

class EDA_DRAW_FRAME;
class wxArrayString;

class DIALOG_LIB_NEW_SYMBOL : public DIALOG_LIB_NEW_SYMBOL_BASE
{
public:
    DIALOG_LIB_NEW_SYMBOL( EDA_DRAW_FRAME* parent,
                           const wxArrayString* aRootSymbolNames = nullptr );

    void SetName( const wxString& name ) override
    {
        m_textName->SetValue( UnescapeString( name ) );
    }

    wxString GetName( void ) const override
    {
        return EscapeString( m_textName->GetValue(), CTX_LIBID );
    }

    wxString GetParentSymbolName() const
    {
        return EscapeString( m_comboInheritanceSelect->GetValue(), CTX_LIBID );
    }

    void SetReference( const wxString& reference ) { m_textReference->SetValue( reference ); }
    wxString GetReference( void ) { return m_textReference->GetValue(); }

    void SetPartCount( int count ) { m_spinPartCount->SetValue( count ); }
    int GetUnitCount( void ) { return m_spinPartCount->GetValue(); }

    void SetAlternateBodyStyle( bool enable ) { m_checkHasConversion->SetValue( enable ); }
    bool GetAlternateBodyStyle( void )  { return m_checkHasConversion->GetValue(); }

    void SetPowerSymbol( bool enable ) { m_checkIsPowerSymbol->SetValue( enable ); }
    bool GetPowerSymbol( void ) { return m_checkIsPowerSymbol->GetValue(); }

    void SetLockItems( bool enable ) { m_checkLockItems->SetValue( enable ); }
    bool GetLockItems( void ) { return m_checkLockItems->GetValue(); }

    void SetIncludeInBom( bool aInclude ) { m_excludeFromBomCheckBox->SetValue( !aInclude ); }
    bool GetIncludeInBom() const { return !m_excludeFromBomCheckBox->GetValue(); }

    void SetIncludeOnBoard( bool aInclude ) { m_excludeFromBoardCheckBox->SetValue( !aInclude ); }
    bool GetIncludeOnBoard() const { return !m_excludeFromBoardCheckBox->GetValue(); }

    void SetPinTextPosition( int position ) { m_pinTextPosition.SetValue( position ); }
    int GetPinTextPosition( void ) { return m_pinTextPosition.GetValue(); }

    void SetShowPinNumber( bool show ) { m_checkShowPinNumber->SetValue( show ); }
    bool GetShowPinNumber( void ) { return m_checkShowPinNumber->GetValue(); }

    void SetShowPinName( bool show ) { m_checkShowPinName->SetValue( show ); }
    bool GetShowPinName( void ) { return m_checkShowPinName->GetValue(); }

    void SetPinNameInside( bool show ) { m_checkShowPinNameInside->SetValue( show ); }
    bool GetPinNameInside( void ) { return m_checkShowPinNameInside->GetValue(); }

protected:
    virtual void OnParentSymbolSelect( wxCommandEvent& aEvent ) override;
    virtual void onPowerCheckBox( wxCommandEvent& aEvent ) override;

private:
    void syncControls( bool aIsDerivedPart );

    UNIT_BINDER     m_pinTextPosition;
};

#endif // __dialog_lib_new_symbol__
