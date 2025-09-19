/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2009 Wayne Stambaugh <stambaughw@gmail.com>
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
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

#pragma once

#include <widgets/unit_binder.h>
#include <string_utils.h>
#include <dialog_lib_new_symbol_base.h>

class EDA_DRAW_FRAME;
class wxArrayString;

class DIALOG_LIB_NEW_SYMBOL : public DIALOG_LIB_NEW_SYMBOL_BASE
{
public:
    DIALOG_LIB_NEW_SYMBOL( EDA_DRAW_FRAME* aParent, const wxArrayString& aSymbolNames,
                           const wxString& aInheritFromSymbolName,
                           std::function<bool( const wxString& newName )> aValidator );

    ~DIALOG_LIB_NEW_SYMBOL();

    void SetName( const wxString& name ) override
    {
        m_textName->SetValue( UnescapeString( name ) );
    }

    wxString GetName() const override
    {
        wxString name = EscapeString( m_textName->GetValue(), CTX_LIBID );

        // Currently, symbol names cannot include a space, that breaks libraries:
        name.Replace( " ", "_" );

        return name;
    }

    wxString GetParentSymbolName() const
    {
        return EscapeString( m_comboInheritanceSelect->GetValue(), CTX_LIBID );
    }

    void SetReference( const wxString& reference ) { m_textReference->SetValue( reference ); }
    wxString GetReference() { return m_textReference->GetValue(); }

    void SetPartCount( int count ) { m_spinPartCount->SetValue( count ); }
    int GetUnitCount() { return m_spinPartCount->GetValue(); }

    void SetAlternateBodyStyle( bool enable ) { m_checkHasAlternateBodyStyle->SetValue( enable ); }
    bool GetAlternateBodyStyle()  { return m_checkHasAlternateBodyStyle->GetValue(); }

    void SetPowerSymbol( bool enable ) { m_checkIsPowerSymbol->SetValue( enable ); }
    bool GetPowerSymbol() { return m_checkIsPowerSymbol->GetValue(); }

    void SetUnitsInterchangeable( bool enable ) { m_checkUnitsInterchangeable->SetValue( enable ); }
    bool GetUnitsInterchangeable() { return m_checkUnitsInterchangeable->GetValue(); }

    void SetIncludeInBom( bool aInclude ) { m_excludeFromBomCheckBox->SetValue( !aInclude ); }
    bool GetIncludeInBom() const { return !m_excludeFromBomCheckBox->GetValue(); }

    void SetIncludeOnBoard( bool aInclude ) { m_excludeFromBoardCheckBox->SetValue( !aInclude ); }
    bool GetIncludeOnBoard() const { return !m_excludeFromBoardCheckBox->GetValue(); }

    void SetPinTextPosition( int position ) { m_pinTextPosition.SetValue( position ); }
    int GetPinTextPosition() { return m_pinTextPosition.GetValue(); }

    void SetKeepDatasheet( bool keep ) { m_checkKeepDatasheet->SetValue( keep ); }
    bool GetKeepDatasheet() { return m_checkKeepDatasheet->GetValue(); }

    void SetKeepFootprint( bool keep ) { m_checkKeepFootprint->SetValue( keep ); }
    bool GetKeepFootprint() { return m_checkKeepFootprint->GetValue(); }

    void SetTransferUserFields( bool keep ) { m_checkTransferUserFields->SetValue( keep ); }
    bool GetTransferUserFields() { return m_checkTransferUserFields->GetValue(); }

    void SetKeepContentUserFields( bool keep ) { m_checkKeepContentUserFields->SetValue( keep ); }
    bool GetKeepContentUserFields() { return m_checkKeepContentUserFields->GetValue(); }

    void SetShowPinNumber( bool show ) { m_checkShowPinNumber->SetValue( show ); }
    bool GetShowPinNumber() { return m_checkShowPinNumber->GetValue(); }

    void SetShowPinName( bool show ) { m_checkShowPinName->SetValue( show ); }
    bool GetShowPinName() { return m_checkShowPinName->GetValue(); }

    void SetPinNameInside( bool show ) { m_checkShowPinNameInside->SetValue( show ); }
    bool GetPinNameInside() { return m_checkShowPinNameInside->GetValue(); }

protected:
    bool TransferDataToWindow() override;
    bool TransferDataFromWindow() override;

    virtual void onPowerCheckBox( wxCommandEvent& aEvent ) override;
    virtual void onCheckTransferUserFields( wxCommandEvent& aEvent ) override;

private:
    void onParentSymbolSelect( wxCommandEvent& aEvent );

    void syncControls( bool aIsDerivedPart );

private:
    UNIT_BINDER                                    m_pinTextPosition;
    std::function<bool( const wxString& newName )> m_validator;
    wxString                                       m_inheritFromSymbolName;
    bool                                           m_nameIsDefaulted;
};
