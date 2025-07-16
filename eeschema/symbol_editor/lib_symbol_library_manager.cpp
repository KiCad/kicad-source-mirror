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

#include <lib_logger.h>
#include <lib_symbol_library_manager.h>
#include <project_sch.h>
#include <symbol_edit_frame.h>
#include <template_fieldnames.h>
#include <lib_symbol.h>
#include <sch_field.h>
#include <libraries/symbol_library_adapter.h>


// TODO(JE) library tables -- this class doesn't need to exist anymore
LIB_SYMBOL_LIBRARY_MANAGER::LIB_SYMBOL_LIBRARY_MANAGER( SYMBOL_EDIT_FRAME& aFrame ) :
        SYMBOL_LIBRARY_MANAGER( aFrame ),
        m_syncHash( 0 )
{
    m_adapter = SYMBOL_TREE_SYNCHRONIZING_ADAPTER::Create( &aFrame, this );
    m_adapter->ShowUnits( false );
}


void LIB_SYMBOL_LIBRARY_MANAGER::Sync( const wxString& aForceRefresh,
                                       std::function<void( int, int,
                                                           const wxString& )> aProgressCallback )
{
    m_logger->Activate();
    {
        getAdapter()->Sync( aForceRefresh, aProgressCallback );
        SYMBOL_LIBRARY_ADAPTER* adapter = PROJECT_SCH::SymbolLibAdapter( &m_frame.Prj() );
        m_syncHash = adapter->GetModifyHash();
    }
    m_logger->Deactivate();
}


std::unique_ptr<LIB_SYMBOL> LIB_SYMBOL_LIBRARY_MANAGER::CreateSymbol( const NEW_SYMBOL_PROPERTIES& aProps,
                                                                      LIB_SYMBOL* aParent )
{
    std::unique_ptr<LIB_SYMBOL> new_symbol = std::make_unique<LIB_SYMBOL>( aProps.name );

    if( !aParent )
    {
        new_symbol->GetReferenceField().SetText( aProps.reference );
        new_symbol->SetUnitCount( aProps.unitCount, true );

        if( aProps.pinNameInside )
        {
            new_symbol->SetPinNameOffset( aProps.pinTextPosition );

            if( new_symbol->GetPinNameOffset() == 0 )
                new_symbol->SetPinNameOffset( 1 );
        }
        else
        {
            new_symbol->SetPinNameOffset( 0 );
        }

        ( aProps.powerSymbol ) ? new_symbol->SetGlobalPower() : new_symbol->SetNormal();
        new_symbol->SetShowPinNumbers( aProps.showPinNumber );
        new_symbol->SetShowPinNames( aProps.showPinName );
        new_symbol->LockUnits( !aProps.unitsInterchangeable );
        new_symbol->SetExcludedFromBOM( !aProps.includeInBom );
        new_symbol->SetExcludedFromBoard( !aProps.includeOnBoard );

        if( aProps.unitCount < 2 )
            new_symbol->LockUnits( false );

        if( aProps.alternateBodyStyle )
            new_symbol->SetBodyStyleCount( 2, false, true );
    }
    else
    {
        new_symbol->SetParent( aParent );

        for( FIELD_T fieldId : MANDATORY_FIELDS )
        {
            SCH_FIELD* field = new_symbol->GetField( fieldId );
            SCH_FIELD* parentField = aParent->GetField( fieldId );

            *field = *parentField;

            switch( fieldId )
            {
            case FIELD_T::REFERENCE:
                break;

            case FIELD_T::VALUE:
                if( aParent->IsPower() )
                    field->SetText( aProps.name );
                break;

            case FIELD_T::FOOTPRINT:
                if( !aProps.keepFootprint )
                    field->SetText( wxEmptyString );
                break;

            case FIELD_T::DATASHEET:
                if( !aProps.keepDatasheet )
                    field->SetText( wxEmptyString );
                break;

            default:
                break;
            }

            field->SetParent( new_symbol.get() );
        }

        if( aProps.transferUserFields )
        {
            std::vector<SCH_FIELD*> listFields;
            aParent->GetFields( listFields );

            for( SCH_FIELD* field : listFields )
            {
                if( field->GetId() == FIELD_T::USER )
                {
                    SCH_FIELD* new_field = new SCH_FIELD( *field );

                    if( !aProps.keepContentUserFields )
                        new_field->SetText( wxEmptyString );

                    new_field->SetParent( new_symbol.get() );
                    new_symbol->AddField( new_field );
                }
            }
        }
    }

    return new_symbol;
}


bool LIB_SYMBOL_LIBRARY_MANAGER::CreateNewSymbol( const wxString& aLibrary,
                                                  const NEW_SYMBOL_PROPERTIES& aProps )
{
    LIB_SYMBOL* parent = nullptr;

    if( !aProps.parentSymbolName.IsEmpty() )
        parent = GetSymbol( aProps.parentSymbolName, aLibrary );

    std::unique_ptr<LIB_SYMBOL> new_symbol = CreateSymbol( aProps, parent );

    if( !UpdateSymbol( new_symbol.get(), aLibrary ) )
        return false;

    return true;
}


void LIB_SYMBOL_LIBRARY_MANAGER::OnDataChanged() const
{
    static_cast<SYMBOL_EDIT_FRAME&>( m_frame ).SyncLibraries( false );
}
