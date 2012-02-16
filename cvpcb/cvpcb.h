/*********/
/* CVPCB */
/*********/

#ifndef __CVPCB_H__
#define __CVPCB_H__

#include <pcbcommon.h>

#include <boost/ptr_container/ptr_vector.hpp>
#include <boost/foreach.hpp>


// config for footprints doc file access
#define DEFAULT_FOOTPRINTS_LIST_FILENAME wxT( "footprints_doc/footprints.pdf" )

// Define print format to display a schematic component line
#define CMP_FORMAT wxT( "%3d %8s - %16s : %-.32s" )

#define FILTERFOOTPRINTKEY "FilterFootprint"

#include <netlist_reader.h>

typedef boost::ptr_vector< COMPONENT_INFO > COMPONENT_LIST;

extern const wxString FootprintAliasFileExtension;
extern const wxString RetroFileExtension;

extern const wxString RetroFileWildcard;
extern const wxString FootprintAliasFileWildcard;

extern const wxString titleLibLoadError;

#endif /* __CVPCB_H__ */
