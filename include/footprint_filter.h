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

#ifndef FOOTPRINT_FILTER_H
#define FOOTPRINT_FILTER_H

#include <boost/iterator/iterator_facade.hpp>
#include <eda_pattern_match.h>
#include <footprint_info.h>
#include <wx/arrstr.h>


/**
 * Footprint display filter. Takes a list of footprints and filtering settings,
 * and provides an iterable view of the filtered data.
 */
class FOOTPRINT_FILTER
{
public:
    /**
     * Construct a filter.
     *
     * @param aList         - unfiltered list of footprints
     */
    FOOTPRINT_FILTER( FOOTPRINT_LIST& aList );

    /**
     * Construct a filter without assigning a footprint list. The filter MUST NOT
     * be iterated over until SetList() is called.
     */
    FOOTPRINT_FILTER();

    /**
     * Set the list to filter.
     */
    void SetList( FOOTPRINT_LIST& aList );

    /**
     * Clear all filter criteria.
     */
    void ClearFilters();

    /**
     * Add library name to filter criteria.
     */
    void FilterByLibrary( const wxString& aLibName );

    /**
     * Set a pin count to filter by.
     */
    void FilterByPinCount( int aPinCount );

    /**
     * Set a list of footprint filters to filter by.
     */
    void FilterByFootprintFilters( const wxArrayString& aFilters );

    /**
     * Add a pattern to filter by name, including wildcards and optionally a colon-delimited
     * library name.
     */
    void FilterByTextPattern( const wxString& aPattern );

    /**
     * Inner iterator class returned by begin() and end().
     */
    class ITERATOR
            : public boost::iterator_facade<ITERATOR, FOOTPRINT_INFO, boost::forward_traversal_tag>
    {
    public:
        ITERATOR();
        ITERATOR( const ITERATOR& aOther );
        ITERATOR( FOOTPRINT_FILTER& aFilter );

    private:
        friend class boost::iterator_core_access;
        friend class FOOTPRINT_FILTER;

        void increment();
        bool equal( const ITERATOR& aOther ) const;
        FOOTPRINT_INFO& dereference() const;

        size_t            m_pos;
        FOOTPRINT_FILTER* m_filter;

        /**
         * Check if the stored component matches an item by footprint filter.
         */
        bool FootprintFilterMatch( FOOTPRINT_INFO& aItem );

        /**
         * Check if the stored component matches an item by pin count.
         */
        bool PinCountMatch( FOOTPRINT_INFO& aItem );
    };

    /**
     * Get an iterator to the beginning of the filtered view.
     */
    ITERATOR begin();

    /**
     * Get an iterator to the end of the filtered view. The end iterator is
     * invalid and may not be dereferenced, only compared against.
     */
    ITERATOR end();

private:
    /**
     * Filter setting constants. The filter type is a bitwise OR of these flags,
     * and only footprints matching all selected filter types are shown.
     */
    enum FP_FILTER_T : int
    {
        UNFILTERED_FP_LIST               = 0,
        FILTERING_BY_COMPONENT_FP_FILTER = 0x0001,
        FILTERING_BY_PIN_COUNT           = 0x0002,
        FILTERING_BY_LIBRARY             = 0x0004,
        FILTERING_BY_TEXT_PATTERN        = 0x0008
    };

    FOOTPRINT_LIST* m_list;

    wxString        m_lib_name;
    wxString        m_filter_pattern;
    int             m_pin_count;
    int             m_filter_type;

    std::vector<std::unique_ptr<EDA_COMBINED_MATCHER>> m_pattern_filters;
    std::vector<std::unique_ptr<EDA_PATTERN_MATCH>>    m_footprint_filters;
};

#endif // FOOTPRINT_FILTER_H
