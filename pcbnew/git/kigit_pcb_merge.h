/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.TXT for contributors.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 3
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, you may find one here:
 * http://www.gnu.org/licenses/gpl-3.0.html
 * or you may search the http://www.gnu.org website for the version 3 license,
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

#ifndef KIGIT_PCB_MERGE_H
#define KIGIT_PCB_MERGE_H
#include <git2.h>
#include <git2/sys/merge.h>

#include <memory>
#include <wx/string.h>

#include <board.h>

typedef struct KIGIT_PCB_MERGE_DIFFERENCES
{
    std::vector<BOARD_ITEM*> m_added;
    std::vector<BOARD_ITEM*> m_removed;
    std::vector<BOARD_ITEM*> m_changed;
} KIGIT_PCB_MERGE_DIFFERENCES;

class KIGIT_PCB_MERGE
{
    public:
        KIGIT_PCB_MERGE( git_merge_driver_source* aSource, git_buf* aBuf ) : m_mergeDriver( aSource ), m_result( aBuf )
        {}

        virtual ~KIGIT_PCB_MERGE();

        int Merge();

        std::set<BOARD_ITEM*>& GetWeModifiedTheyDeleted()
        {
            return we_modified_they_deleted;
        }

        std::set<BOARD_ITEM*>& GetTheyModifiedWeDeleted()
        {
            return they_modified_we_deleted;
        }

        std::set<BOARD_ITEM*>& GetBothModified()
        {
            return both_modified;
        }

    protected:
        std::unique_ptr<BOARD> readBoard( wxString& aFilename );
        KIGIT_PCB_MERGE_DIFFERENCES compareBoards( BOARD* aAncestor, BOARD* aOther );
        void findSetDifferences( const BOARD_ITEM_SET& aAncestorSet, const BOARD_ITEM_SET& aOtherSet,
                                 std::vector<BOARD_ITEM*>& aAdded, std::vector<BOARD_ITEM*>& aRemoved,
                                 std::vector<BOARD_ITEM*>& aChanged );

    private:

        git_merge_driver_source* m_mergeDriver;
        git_buf* m_result;

    std::set<BOARD_ITEM*> we_modified_they_deleted;
    std::set<BOARD_ITEM*> they_modified_we_deleted;
    std::set<BOARD_ITEM*> both_modified;
};



#endif // KIGIT_PCB_MERGE_H