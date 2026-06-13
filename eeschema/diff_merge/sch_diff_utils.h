/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
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
 */

#ifndef SCH_DIFF_UTILS_H
#define SCH_DIFF_UTILS_H

#include <schematic.h>
#include <sch_item.h>
#include <sch_screen.h>
#include <sch_sheet_path.h>


namespace KICAD_DIFF
{

/**
 * RAII guard that temporarily swaps SCHEMATIC::CurrentSheet to a given path for
 * the duration of a scope, restoring it on exit.
 *
 * SCH_SYMBOL / SCH_FIELD property and bounding-box getters look up the active
 * instance through SCHEMATIC::CurrentSheet(); the differ and the geometry
 * extractor both need that to point at the item's own sheet path while they
 * read it. SCHEMATIC::CurrentSheet() returns a non-const reference even off a
 * const SCHEMATIC*, so no const_cast on the schematic itself is needed.
 *
 * A null @p aPath leaves CurrentSheet untouched (only the original value is
 * saved and restored), matching the differ's nullable-path call sites.
 */
class SHEET_SCOPE
{
public:
    SHEET_SCOPE( const SCHEMATIC* aSch, const SCH_SHEET_PATH* aPath ) :
            m_sch( aSch ),
            m_saved( aSch ? aSch->CurrentSheet() : SCH_SHEET_PATH{} )
    {
        if( m_sch && aPath )
            m_sch->CurrentSheet() = *aPath;
    }

    ~SHEET_SCOPE()
    {
        if( m_sch )
            m_sch->CurrentSheet() = m_saved;
    }

    SHEET_SCOPE( const SHEET_SCOPE& )            = delete;
    SHEET_SCOPE& operator=( const SHEET_SCOPE& ) = delete;

private:
    const SCHEMATIC* m_sch;
    SCH_SHEET_PATH   m_saved;
};


/**
 * Visit every SCH_ITEM in every sheet of @p aSchematic in page-number order.
 *
 * Sheets are enumerated via BuildSheetListSortedByPageNumbers(); for each
 * sheet the item visitor is invoked once per item on that sheet's screen as
 * `aItemVisitor( item, sheetPath, kiidPath )` where:
 *   - item is the SCH_ITEM* (never null),
 *   - sheetPath is the SCH_SHEET_PATH for the current sheet,
 *   - kiidPath is sheetPath.Path() with the item's m_Uuid pushed on.
 *
 * @p aSheetVisitor is invoked once for every sheet in the list, before its
 * items, as `aSheetVisitor( sheetPath )` regardless of whether the sheet has a
 * screen or any items. Callers that need to own a SCH_SHEET_PATH per sheet
 * (the differ and the applier keep raw pointers into a storage vector) use it
 * to materialize the copy exactly once per sheet.
 *
 * The sheet path passed to either visitor is a reference to a local that is
 * only valid for the duration of the call; callers that retain it must copy.
 *
 * This intentionally visits every sheet INSTANCE with no screen-level dedup, so
 * a screen shared by several instances is walked once per instance and yields a
 * distinct per-instance KIID_PATH (the differ and applier key on that path).
 * Contrast sch_geometry_extractor's ExtractSchematicGeometry, which dedups by
 * screen so a shared screen renders once. Don't unify the two enumerations.
 */
template <typename SHEET_VISITOR, typename ITEM_VISITOR>
void WalkSchematic( const SCHEMATIC* aSchematic, SHEET_VISITOR&& aSheetVisitor, ITEM_VISITOR&& aItemVisitor,
                    const KIID_PATH& aScope = {} )
{
    if( !aSchematic )
        return;

    SCH_SHEET_LIST sheets = aSchematic->BuildSheetListSortedByPageNumbers();

    for( const SCH_SHEET_PATH& path : sheets )
    {
        const KIID_PATH sheetPath = path.Path();

        if( !aScope.empty() && sheetPath != aScope )
            continue;

        aSheetVisitor( path );

        SCH_SCREEN* screen = path.LastScreen();

        if( !screen )
            continue;

        for( SCH_ITEM* item : screen->Items() )
        {
            if( !item )
                continue;

            // Scoped mode emits just the item KIID so items align across
            // schematics with different hierarchies.
            KIID_PATH kiidPath;

            if( aScope.empty() )
                kiidPath = sheetPath;

            kiidPath.push_back( item->m_Uuid );

            aItemVisitor( item, path, kiidPath );
        }
    }
}

} // namespace KICAD_DIFF

#endif // SCH_DIFF_UTILS_H
