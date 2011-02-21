/*********/
/* CVPCB */
/*********/

#ifndef __CVPCB_H__
#define __CVPCB_H__

#include "pcbcommon.h"

#include <boost/ptr_container/ptr_vector.hpp>
#include <boost/foreach.hpp>


// config for footprints doc file access
#define DEFAULT_FOOTPRINTS_LIST_FILENAME wxT( "footprints_doc/footprints.pdf" )

// Define print format to display a schematic component line
#define CMP_FORMAT wxT( "%3d %8s - %16s : %-.32s" )

#define FILTERFOOTPRINTKEY "FilterFootprint"

class PIN
{
public:
    int       m_Index;     /* Type of net list. */
    int       m_Type;      /* Electrical connection type. */
    wxString  m_Net;       /* Name of net. */
    wxString  m_Number;
    wxString  m_Name;
    wxString  m_Repere;    /* Formats used by net lister. */

    PIN();
};

typedef boost::ptr_vector< PIN > PIN_LIST;

/* PIN object list sort function. */
extern bool operator<( const PIN& item1, const PIN& item2 );

/* PIN uniqueness test function. */
extern bool operator==( const PIN& item1, const PIN& item2 );

extern bool same_pin_number( const PIN* item1, const PIN* item2 );
extern bool same_pin_net( const PIN* item1, const PIN* item2 );


class COMPONENT
{
public:
    int           m_Num;       /* Component number. */
    int           m_Multi;     /* Part if component has multiple parts. */
    PIN_LIST      m_Pins;      /* List of component pins. */
    wxString      m_Reference; /* Reference designator: U3, R5  ... */
    wxString      m_Value;     /* Value: 7400, 47K ... */
    wxString      m_TimeStamp; /* Time stamp ("00000000" if absent) */
    wxString      m_Module;    /* Footprint (module) name. */
    wxString      m_Repere;    /* Net list format */
    wxArrayString m_FootprintFilter;  /* List of allowed footprints (wildcards
                                       * allowed ). If empty: no filtering */

    COMPONENT();
    ~COMPONENT();
};

typedef boost::ptr_vector< COMPONENT > COMPONENT_LIST;

/* COMPONENT object list sort function. */
extern bool operator<( const COMPONENT& item1, const COMPONENT& item2 );


extern const wxString FootprintAliasFileExtension;
extern const wxString RetroFileExtension;
extern const wxString ComponentFileExtension;

extern const wxString RetroFileWildcard;
extern const wxString FootprintAliasFileWildcard;

extern const wxString titleLibLoadError;

void Plume( int state );

#endif /* __CVPCB_H__ */
