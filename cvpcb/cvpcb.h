/*********/
/* CVPCB */
/*********/

#ifndef __CVPCB_H__
#define __CVPCB_H__

// config for footprints doc file access
#define DEFAULT_FOOTPRINTS_LIST_FILENAME wxT( "footprints_doc/footprints.pdf" )

// Define print format to display a schematic component line
#define CMP_FORMAT wxT( "%3d %8s - %16s : %s" )

#define FILTERFOOTPRINTKEY "FilterFootprint"

#define LISTB_STYLE     ( wxSUNKEN_BORDER | wxLC_NO_HEADER | wxLC_REPORT | wxLC_VIRTUAL | \
                          wxLC_SINGLE_SEL | wxVSCROLL | wxHSCROLL )

extern const wxString FootprintAliasFileExtension;
extern const wxString RetroFileExtension;

extern const wxString FootprintAliasFileWildcard;


#endif /* __CVPCB_H__ */
