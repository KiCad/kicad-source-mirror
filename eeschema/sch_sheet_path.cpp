/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2009 Jean-Pierre Charras, jaen-pierre.charras@gipsa-lab.inpg.com
 * Copyright (C) 2011-2016 Wayne Stambaugh <stambaughw@verizon.net>
 * Copyright (C) 1992-2016 KiCad Developers, see AUTHORS.txt for contributors.
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

/**
 * @file sch_sheet_path.cpp
 * @brief SCH_SHEET_PATH class implementation.
 */

#include <fctsys.h>

#include <general.h>
#include <dlist.h>
#include <class_sch_screen.h>
#include <sch_item_struct.h>

#include <sch_reference_list.h>
#include <class_library.h>
#include <sch_sheet_path.h>
#include <sch_component.h>
#include <template_fieldnames.h>

#include <dialogs/dialog_schematic_find.h>

#include <wx/filename.h>


SCH_SHEET_PATH::SCH_SHEET_PATH()
{
    m_pageNumber = 0;
}


int SCH_SHEET_PATH::Cmp( const SCH_SHEET_PATH& aSheetPathToTest ) const
{
    if( size() > aSheetPathToTest.size() )
        return 1;

    if( size() < aSheetPathToTest.size() )
        return -1;

    //otherwise, same number of sheets.
    for( unsigned i = 0; i < size(); i++ )
    {
        if( at( i )->GetTimeStamp() > aSheetPathToTest.at( i )->GetTimeStamp() )
            return 1;

        if( at( i )->GetTimeStamp() < aSheetPathToTest.at( i )->GetTimeStamp() )
            return -1;
    }

    return 0;
}


SCH_SHEET* SCH_SHEET_PATH::Last() const
{
    if( !empty() )
        return at( size() - 1 );

    return NULL;
}


SCH_SCREEN* SCH_SHEET_PATH::LastScreen() const
{
    SCH_SHEET* lastSheet = Last();

    if( lastSheet )
        return lastSheet->GetScreen();

    return NULL;
}


SCH_ITEM* SCH_SHEET_PATH::LastDrawList() const
{
    SCH_SHEET* lastSheet = Last();

    if( lastSheet && lastSheet->GetScreen() )
        return lastSheet->GetScreen()->GetDrawItems();

    return NULL;
}


SCH_ITEM* SCH_SHEET_PATH::FirstDrawList() const
{
    SCH_ITEM* item = NULL;

    if( !empty() && at( 0 )->GetScreen() )
        item = at( 0 )->GetScreen()->GetDrawItems();

    /* @fixme - These lists really should be one of the boost pointer containers.  This
     *          is a brain dead hack to allow reverse iteration of EDA_ITEM linked
     *          list.
     */
    SCH_ITEM* lastItem = NULL;

    while( item )
    {
        lastItem = item;
        item = item->Next();
    }

    return lastItem;
}


wxString SCH_SHEET_PATH::Path() const
{
    wxString s, t;

    s = wxT( "/" );     // This is the root path

    // start at 1 to avoid the root sheet,
    // which does not need to be added to the path
    // it's timestamp changes anyway.
    for( unsigned i = 1; i < size(); i++ )
    {
        t.Printf( _( "%8.8lX/" ), (long unsigned) at( i )->GetTimeStamp() );
        s = s + t;
    }

    return s;
}


wxString SCH_SHEET_PATH::PathHumanReadable() const
{
    wxString s;

    s = wxT( "/" );

    // start at 1 to avoid the root sheet, as above.
    for( unsigned i = 1; i < size(); i++ )
    {
        s = s + at( i )->GetName() + wxT( "/" );
    }

    return s;
}


void SCH_SHEET_PATH::UpdateAllScreenReferences()
{
    EDA_ITEM* t = LastDrawList();

    while( t )
    {
        if( t->Type() == SCH_COMPONENT_T )
        {
            SCH_COMPONENT* component = (SCH_COMPONENT*) t;
            component->GetField( REFERENCE )->SetText( component->GetRef( this ) );
            component->UpdateUnit( component->GetUnitSelection( this ) );
        }

        t = t->Next();
    }
}


void SCH_SHEET_PATH::AnnotatePowerSymbols( PART_LIBS* aLibs, int* aReference )
{
    int ref = 1;

    if( aReference )
        ref = *aReference;

    for( EDA_ITEM* item = LastDrawList();  item;  item = item->Next() )
    {
        if( item->Type() != SCH_COMPONENT_T )
            continue;

        SCH_COMPONENT*  component = (SCH_COMPONENT*) item;
        LIB_PART*       part = aLibs->FindLibPart( component->GetLibId() );

        if( !part || !part->IsPower() )
            continue;

        wxString refstr = component->GetPrefix();

        //str will be "C?" or so after the ClearAnnotation call.
        while( refstr.Last() == '?' )
            refstr.RemoveLast();

        if( !refstr.StartsWith( wxT( "#" ) ) )
            refstr.insert( refstr.begin(), wxChar( '#' ) );

        refstr << wxT( "0" ) << ref;
        component->SetRef( this, refstr );
        ref++;
    }

    if( aReference )
        *aReference = ref;
}


void SCH_SHEET_PATH::GetComponents( PART_LIBS* aLibs, SCH_REFERENCE_LIST& aReferences,
                                    bool aIncludePowerSymbols )
{
    for( SCH_ITEM* item = LastDrawList(); item; item = item->Next() )
    {
        if( item->Type() == SCH_COMPONENT_T )
        {
            SCH_COMPONENT* component = (SCH_COMPONENT*) item;

            // Skip pseudo components, which have a reference starting with #.  This mainly
            // affects power symbols.
            if( !aIncludePowerSymbols && component->GetRef( this )[0] == wxT( '#' ) )
                continue;

            LIB_PART* part = aLibs->FindLibPart( component->GetLibId() );

            if( part )
            {
                SCH_REFERENCE reference( component, part, *this );

                reference.SetSheetNumber( m_pageNumber );
                aReferences.AddItem( reference );
            }
        }
    }
}


void SCH_SHEET_PATH::GetMultiUnitComponents( PART_LIBS* aLibs,
                                             SCH_MULTI_UNIT_REFERENCE_MAP& aRefList,
                                             bool aIncludePowerSymbols )
{

    for( SCH_ITEM* item = LastDrawList(); item; item = item->Next() )
    {
        if( item->Type() != SCH_COMPONENT_T )
            continue;

        SCH_COMPONENT* component = (SCH_COMPONENT*) item;

        // Skip pseudo components, which have a reference starting with #.  This mainly
        // affects power symbols.
        if( !aIncludePowerSymbols && component->GetRef( this )[0] == wxT( '#' ) )
            continue;

        LIB_PART* part = aLibs->FindLibPart( component->GetLibId() );

        if( part && part->GetUnitCount() > 1 )
        {
            SCH_REFERENCE reference = SCH_REFERENCE( component, part, *this );
            reference.SetSheetNumber( m_pageNumber );
            wxString reference_str = reference.GetRef();

            // Never lock unassigned references
            if( reference_str[reference_str.Len() - 1] == '?' )
                continue;

            aRefList[reference_str].AddItem( reference );
        }
    }
}


SCH_ITEM* SCH_SHEET_PATH::FindNextItem( KICAD_T aType, SCH_ITEM* aLastItem, bool aWrap ) const
{
    bool hasWrapped = false;
    bool firstItemFound = false;
    SCH_ITEM* drawItem = LastDrawList();

    while( drawItem )
    {
        if( drawItem->Type() == aType )
        {
            if( !aLastItem || firstItemFound )
            {
                return drawItem;
            }
            else if( !firstItemFound && drawItem == aLastItem )
            {
                firstItemFound = true;
            }
        }

        drawItem = drawItem->Next();

        if( !drawItem && aLastItem && aWrap && !hasWrapped )
        {
            hasWrapped = true;
            drawItem = LastDrawList();
        }
    }

    return NULL;
}


SCH_ITEM* SCH_SHEET_PATH::FindPreviousItem( KICAD_T aType, SCH_ITEM* aLastItem, bool aWrap ) const
{
    bool hasWrapped = false;
    bool firstItemFound = false;
    SCH_ITEM* drawItem = FirstDrawList();

    while( drawItem )
    {
        if( drawItem->Type() == aType )
        {
            if( aLastItem == NULL || firstItemFound )
            {
                return drawItem;
            }
            else if( !firstItemFound && drawItem == aLastItem )
            {
                firstItemFound = true;
            }
        }

        drawItem = drawItem->Back();

        if( drawItem == NULL && aLastItem && aWrap && !hasWrapped )
        {
            hasWrapped = true;
            drawItem = FirstDrawList();
        }
    }

    return NULL;
}


bool SCH_SHEET_PATH::SetComponentFootprint( const wxString& aReference, const wxString& aFootPrint,
                                            bool aSetVisible )
{
    SCH_SCREEN* screen = LastScreen();

    if( screen == NULL )
        return false;

    return screen->SetComponentFootprint( this, aReference, aFootPrint, aSetVisible );
}


bool SCH_SHEET_PATH::operator==( const SCH_SHEET_PATH& d1 ) const
{
    if( size() != d1.size() )
        return false;

    for( unsigned i = 0; i < size(); i++ )
    {
        if( at( i ) != d1[i] )
            return false;
    }

    return true;
}


bool SCH_SHEET_PATH::TestForRecursion( const wxString& aSrcFileName,
                                       const wxString& aDestFileName ) const
{
    wxFileName rootFn = g_RootSheet->GetFileName();
    wxFileName srcFn = aSrcFileName;
    wxFileName destFn = aDestFileName;

    if( srcFn.IsRelative() )
        srcFn.MakeAbsolute( rootFn.GetPath() );

    if( destFn.IsRelative() )
        destFn.MakeAbsolute( rootFn.GetPath() );


    // The source and destination sheet file names cannot be the same.
    if( srcFn == destFn )
        return true;

    /// @todo Store sheet file names with full path, either relative to project path
    ///       or absolute path.  The current design always assumes subsheet files are
    ///       located in the project folder which may or may not be desirable.
    unsigned i = 0;

    while( i < size() )
    {
        wxFileName cmpFn = at( i )->GetFileName();

        if( cmpFn.IsRelative() )
            cmpFn.MakeAbsolute( rootFn.GetPath() );

        // Test if the file name of the destination sheet is in anywhere in this sheet path.
        if( cmpFn == destFn )
            break;

        i++;
    }

    // The destination sheet file name was not found in the sheet path or the destination
    // sheet file name is the root sheet so no recursion is possible.
    if( i >= size() || i == 0 )
        return false;

    // Walk back up to the root sheet to see if the source file name is already a parent in
    // the sheet path.  If so, recursion will occur.
    do
    {
        i -= 1;

        wxFileName cmpFn = at( i )->GetFileName();

        if( cmpFn.IsRelative() )
            cmpFn.MakeAbsolute( rootFn.GetPath() );

        if( cmpFn == srcFn )
            return true;

    } while( i != 0 );

    // The source sheet file name is not a parent of the destination sheet file name.
    return false;
}


int SCH_SHEET_PATH::FindSheet( const wxString& aFileName ) const
{
    for( unsigned i = 0; i < size(); i++ )
    {
        if( at( i )->GetFileName().CmpNoCase( aFileName ) == 0 )
            return (int)i;
    }

    return SHEET_NOT_FOUND;
}


SCH_SHEET* SCH_SHEET_PATH::FindSheetByName( const wxString& aSheetName )
{
    for( unsigned i = 0; i < size(); i++ )
    {
        if( at( i )->GetName().CmpNoCase( aSheetName ) == 0 )
            return at( i );
    }

    return NULL;
}


/********************************************************************/
/* Class SCH_SHEET_LIST to handle the list of Sheets in a hierarchy */
/********************************************************************/
SCH_SHEET_LIST::SCH_SHEET_LIST( SCH_SHEET* aSheet )
{
    m_isRootSheet = false;

    if( aSheet != NULL )
        BuildSheetList( aSheet );
}


SCH_SHEET_PATH* SCH_SHEET_LIST::GetSheetByPath( const wxString aPath, bool aHumanReadable )
{
    wxString sheetPath;

    for( unsigned i = 0; i < size(); i++ )
    {
        sheetPath = ( aHumanReadable ) ? at( i ).PathHumanReadable() : at( i ).Path();

        if( sheetPath == aPath )
            return &at( i );
    }

    return NULL;
}


void SCH_SHEET_LIST::BuildSheetList( SCH_SHEET* aSheet )
{
    wxCHECK_RET( aSheet != NULL, wxT( "Cannot build sheet list from undefined sheet." ) );

    if( aSheet == g_RootSheet )
        m_isRootSheet = true;

    m_currentSheetPath.push_back( aSheet );

    /**
     * @todo:  Schematic page number is currently a left over relic and is generated as
     *         SCH_SHEET_PATH object is pushed to the list.  This only has meaning when
     *         entire hierarchy is created from the root sheet down.
     */
    m_currentSheetPath.SetPageNumber( size() + 1 );
    push_back( m_currentSheetPath );

    if( aSheet->GetScreen() )
    {
        EDA_ITEM* item = m_currentSheetPath.LastDrawList();

        while( item )
        {
            if( item->Type() == SCH_SHEET_T )
            {
                SCH_SHEET* sheet = (SCH_SHEET*) item;
                BuildSheetList( sheet );
            }

            item = item->Next();
        }
    }

    m_currentSheetPath.pop_back();
}


bool SCH_SHEET_LIST::IsModified()
{
    for( SCH_SHEET_PATHS_ITER it = begin(); it != end(); ++it )
    {
        if( (*it).LastScreen() && (*it).LastScreen()->IsModify() )
            return true;
    }

    return false;
}


bool SCH_SHEET_LIST::IsAutoSaveRequired()
{
    for( SCH_SHEET_PATHS_ITER it = begin(); it != end(); ++it )
    {
        if( (*it).LastScreen() && (*it).LastScreen()->IsSave() )
            return true;
    }

    return false;
}


void SCH_SHEET_LIST::ClearModifyStatus()
{
    for( SCH_SHEET_PATHS_ITER it = begin(); it != end(); ++it )
    {
        if( (*it).LastScreen() )
            (*it).LastScreen()->ClrModify();
    }
}


void SCH_SHEET_LIST::AnnotatePowerSymbols( PART_LIBS* aLibs )
{
    int ref = 1;

    for( SCH_SHEET_PATHS_ITER it = begin(); it != end(); ++it )
        (*it).AnnotatePowerSymbols( aLibs, &ref );
}


void SCH_SHEET_LIST::GetComponents( PART_LIBS* aLibs, SCH_REFERENCE_LIST& aReferences,
                                    bool aIncludePowerSymbols )
{
    for( SCH_SHEET_PATHS_ITER it = begin(); it != end(); ++it )
        (*it).GetComponents( aLibs, aReferences, aIncludePowerSymbols );
}

void SCH_SHEET_LIST::GetMultiUnitComponents( PART_LIBS* aLibs,
                                             SCH_MULTI_UNIT_REFERENCE_MAP &aRefList,
                                             bool aIncludePowerSymbols )
{
    for( SCH_SHEET_PATHS_ITER it = begin(); it != end(); ++it )
    {
        SCH_MULTI_UNIT_REFERENCE_MAP tempMap;
        (*it).GetMultiUnitComponents( aLibs, tempMap );

        for( SCH_MULTI_UNIT_REFERENCE_MAP::value_type& pair : tempMap )
        {
            // Merge this list into the main one
            unsigned n_refs = pair.second.GetCount();

            for( unsigned thisRef = 0; thisRef < n_refs; ++thisRef )
            {
                aRefList[pair.first].AddItem( pair.second[thisRef] );
            }
        }
    }
}


SCH_ITEM* SCH_SHEET_LIST::FindNextItem( KICAD_T aType, SCH_SHEET_PATH** aSheetFoundIn,
                                        SCH_ITEM* aLastItem, bool aWrap )
{
    bool hasWrapped = false;
    bool firstItemFound = false;

    SCH_ITEM*       drawItem = NULL;
    SCH_SHEET_PATHS_ITER it = begin();

    while( it != end() )
    {
        drawItem = (*it).LastDrawList();

        while( drawItem )
        {
            if( drawItem->Type() == aType )
            {
                if( aLastItem == NULL || firstItemFound )
                {
                    if( aSheetFoundIn )
                        *aSheetFoundIn = &(*it);

                    return drawItem;
                }
                else if( !firstItemFound && drawItem == aLastItem )
                {
                    firstItemFound = true;
                }
            }

            drawItem = drawItem->Next();
        }

        ++it;

        if( it == end() && aLastItem && aWrap && !hasWrapped )
        {
            hasWrapped = true;
            it = begin();
        }
    }

    return NULL;
}


SCH_ITEM* SCH_SHEET_LIST::FindPreviousItem( KICAD_T aType, SCH_SHEET_PATH** aSheetFoundIn,
                                            SCH_ITEM* aLastItem, bool aWrap )
{
    bool hasWrapped = false;
    bool firstItemFound = false;
    SCH_ITEM* drawItem = NULL;
    SCH_SHEET_PATHS_RITER it = rbegin();

    while( it != rend() )
    {
        drawItem = (*it).FirstDrawList();

        while( drawItem )
        {
            if( drawItem->Type() == aType )
            {
                if( aLastItem == NULL || firstItemFound )
                {
                    if( aSheetFoundIn )
                        *aSheetFoundIn = &(*it);

                    return drawItem;
                }
                else if( !firstItemFound && drawItem == aLastItem )
                {
                    firstItemFound = true;
                }
            }

            drawItem = drawItem->Back();
        }

        ++it;

        if( it == rend() && aLastItem && aWrap && !hasWrapped )
        {
            hasWrapped = true;
            it = rbegin();
        }
    }

    return NULL;
}


bool SCH_SHEET_LIST::SetComponentFootprint( const wxString& aReference,
                                            const wxString& aFootPrint, bool aSetVisible )
{
    bool found = false;

    for( SCH_SHEET_PATHS_ITER it = begin(); it != end(); ++it )
        found = (*it).SetComponentFootprint( aReference, aFootPrint, aSetVisible );

    return found;
}


bool SCH_SHEET_LIST::IsComplexHierarchy() const
{
    wxString fileName;

    for( unsigned i = 0;  i < size();  i++ )
    {
        fileName = at( i ).Last()->GetFileName();

        for( unsigned j = 0;  j < size();  j++ )
        {
            if( i == j )
                continue;

            if( fileName == at( j ).Last()->GetFileName() )
                return true;
        }
    }

    return false;
}


bool SCH_SHEET_LIST::TestForRecursion( const SCH_SHEET_LIST& aSrcSheetHierarchy,
                                       const wxString& aDestFileName ) const
{
    wxFileName rootFn = g_RootSheet->GetFileName();
    wxFileName destFn = aDestFileName;

    if( destFn.IsRelative() )
        destFn.MakeAbsolute( rootFn.GetPath() );

    // Test each SCH_SHEET_PATH in this SCH_SHEET_LIST for potential recursion.
    for( unsigned i = 0; i < size(); i++ )
    {
        // Test each SCH_SHEET_PATH in the source sheet.
        for( unsigned j = 0; j < aSrcSheetHierarchy.size(); j++ )
        {
            const SCH_SHEET_PATH* sheetPath = &aSrcSheetHierarchy[j];

            for( unsigned k = 0; k < sheetPath->size(); k++ )
            {
                if( at( i ).TestForRecursion( sheetPath->GetSheet( k )->GetFileName(),
                                              aDestFileName ) )
                    return true;
            }
        }
    }

    // The source sheet file can safely be added to the destination sheet file.
    return false;
}


SCH_SHEET* SCH_SHEET_LIST::FindSheetByName( const wxString& aSheetName )
{
    for( unsigned i = 0; i < size(); i++ )
    {
        SCH_SHEET* sheet = at( i ).FindSheetByName( aSheetName );

        if( sheet )
            return sheet;
    }

    return NULL;
}
