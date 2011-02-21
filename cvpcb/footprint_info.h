/*
 * @file footprint_info.h
 */

#ifndef _FOOTPRINT_INFO_H_
#define _FOOTPRINT_INFO_H_

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

typedef boost::ptr_vector< FOOTPRINT_INFO > FOOTPRINT_LIST;

/* FOOTPRINT object list sort function. */
inline bool operator<( const FOOTPRINT_INFO& item1, const FOOTPRINT_INFO& item2 )
{
    return ( StrNumICmp( item1.m_Module.GetData(),
                         item2.m_Module.GetData() ) < 0 );
}

#endif  // _FOOTPRINT_INFO_H_
