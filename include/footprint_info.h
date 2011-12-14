/*
 * @file footprint_info.h
 */

#ifndef _FOOTPRINT_INFO_H_
#define _FOOTPRINT_INFO_H_

#include <boost/ptr_container/ptr_vector.hpp>
#include <boost/foreach.hpp>

#include "kicad_string.h"

/*
 * class FOOTPRINT_INFO is a helper class to handle the list of footprints
 * available in libraries
 * It stores footprint names, doc and keywords
 */
class FOOTPRINT_INFO
{
public:
    wxString  m_Module;     /* Module name. */
    wxString  m_LibName;    /* Name of the library containing this module. */
    int       m_Num;        /* Order number in the display list. */
    wxString  m_Doc;        /* Footprint description. */
    wxString  m_KeyWord;    /* Footprint key words. */

    FOOTPRINT_INFO()
    {
        m_Num = 0;
    }
};

class FOOTPRINT_LIST
{
public:
    boost::ptr_vector< FOOTPRINT_INFO > m_List;
    wxString m_filesNotFound;
    wxString m_filesInvalid;

public:

    /**
     * Function GetCount
     * @return the number of items stored in list
     */
    unsigned GetCount() const { return m_List.size(); }

    /**
     * Function GetModuleInfo
     * @return the item stored in list if found
     * @param aFootprintName = the name of item
     */
    FOOTPRINT_INFO * GetModuleInfo( const wxString & aFootprintName )
    {
        BOOST_FOREACH( FOOTPRINT_INFO& footprint, m_List )
        {
            if( aFootprintName.CmpNoCase( footprint.m_Module ) == 0 )
                return &footprint;
        }
        return NULL;
    }

    /**
     * Function GetItem
     * @return the aIdx item in list
     * @param aIdx = index of the given item
     */
    FOOTPRINT_INFO & GetItem( unsigned aIdx )
    {
        return m_List[aIdx];
    }

    /**
     * Function AddItem
     * add aItem in list
     * @param aItem = item to add
     */
    void AddItem( FOOTPRINT_INFO* aItem )
    {
        m_List.push_back( aItem);
    }

    /**
     * Function ReadFootprintFiles
     * Read the list of libraries (*.mod files) and populates m_List ( list of availaible
     * modules in libs ).
     * for each module, are stored
     *      the module name
     *      documentation string
     *      associated keywords
     *      library name
     * Module description format:
     *   $MODULE c64acmd                    First line of module description
     *   Li c64acmd DIN connector           Library reference
     *   Cd Europe 96 AC male vertical      documentation string
     *   Kw PAD_CONN DIN                    associated keywords
     *   ...... other data (pads, outlines ..)
     *   $Endmodule
     *
     * @param aFootprintsLibNames = an array string giving the list of libraries to load
     */
    bool ReadFootprintFiles( wxArrayString & aFootprintsLibNames );
};

/* FOOTPRINT object list sort function. */
inline bool operator<( const FOOTPRINT_INFO& item1, const FOOTPRINT_INFO& item2 )
{
    return StrNumCmp( item1.m_Module, item2.m_Module, INT_MAX, true ) < 0;
}

#endif  // _FOOTPRINT_INFO_H_
