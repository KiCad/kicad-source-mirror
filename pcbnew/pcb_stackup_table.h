/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef PCB_STACKUP_TABLE_H
#define PCB_STACKUP_TABLE_H

#include <board_tables/stackup_table_model.h>
#include <pcb_generated_table.h>


/**
 * A board stackup table placed on the board, kept in step with the stackup.
 *
 * The stackup can change behind the table's back, through Import Settings or the API, so
 * staleness is the default content comparison rather than a generation counter.
 */
class PCB_STACKUP_TABLE : public PCB_GENERATED_TABLE
{
public:
    explicit PCB_STACKUP_TABLE( BOARD_ITEM* aParent );

    static inline bool ClassOf( const EDA_ITEM* aItem )
    {
        return aItem && PCB_STACKUP_TABLE_T == aItem->Type();
    }

    wxString GetClass() const override { return wxT( "PCB_STACKUP_TABLE" ); }

    EDA_ITEM* Clone() const override { return new PCB_STACKUP_TABLE( *this ); }

    const GENERATED_TABLE_SCHEMA& Schema() const override { return StackupTableSchema(); }

    wxString GetItemDescription( UNITS_PROVIDER* aUnitsProvider, bool aFull ) const override;

    bool operator==( const BOARD_ITEM& aOther ) const override;

    void Serialize( google::protobuf::Any& aContainer ) const override;
    bool Deserialize( const google::protobuf::Any& aContainer ) override;

protected:
    std::unique_ptr<GENERATED_TABLE_CONTENT> generate( const BOARD&             aBoard,
                                                       GENERATED_TABLE_REFRESH* aRefresh ) const override;

    /**
     * The text style of the one-shot stackup table this replaces, so a placed table looks the same.
     */
    void styleNewCell( PCB_TABLECELL* aCell, bool aIsHeading ) override;
};

#endif // PCB_STACKUP_TABLE_H
