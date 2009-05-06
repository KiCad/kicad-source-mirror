/**********************************************/
/* CVPCB : declaration des variables globales */
/**********************************************/

#ifndef __CVPCB_H__
#define __CVPCB_H__

#include "pcbcommon.h"

#include <boost/ptr_container/ptr_vector.hpp>
#include <boost/foreach.hpp>


// config for footprints doc file acces
#define DEFAULT_FOOTPRINTS_LIST_FILENAME wxT( "footprints_doc/footprints.pdf" )

// Define print format to display a schematic component line
#define CMP_FORMAT wxT( "%3d %8s - %16s : %-.32s" )

#define FILTERFOOTPRINTKEY "FilterFootprint"

/* Types de netliste: */
#define TYPE_NON_SPECIFIE  0
#define TYPE_ORCADPCB2     1
#define TYPE_PCAD          2
#define TYPE_VIEWLOGIC_WIR 3
#define TYPE_VIEWLOGIC_NET 4


class PIN
{
public:
    int       m_Index;     /* variable utilisee selon types de netlistes */
    int       m_Type;      /* code type electrique ( Entree Sortie Passive..) */
    wxString  m_Net;       /* Pointeur sur le texte nom de net */
    wxString  m_Number;
    wxString  m_Name;
    wxString  m_Repere;    /* utilise selon formats de netliste */

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
    int           m_Num;       /* Numero d'ordre */
    int           m_Multi;     /* Nombre d' unites par boitier */
    PIN_LIST      m_Pins;      /* pointeur sur la liste des Pins */
    wxString      m_Reference; /* U3, R5  ... */
    wxString      m_Value;     /* 7400, 47K ... */
    wxString      m_TimeStamp; /* Signature temporelle ("00000000" si absente) */
    wxString      m_Module;    /* Nom du module (Package) corresp */
    wxString      m_Repere;    /* utilise selon formats de netliste */
    wxArrayString m_FootprintFilter;  /* List of allowed footprints (wildcards
                                       * allowed ). If empty: no filtering */

    COMPONENT();
    ~COMPONENT();
};

typedef boost::ptr_vector< COMPONENT > COMPONENT_LIST;

/* COMPONENT object list sort function. */
extern bool operator<( const COMPONENT& item1, const COMPONENT& item2 );


class FOOTPRINT
{
public:
    wxString  m_Module;     /* Nom du module */
    wxString  m_LibName;    /* Nom de la librairie contenant ce module */
    int       m_Num;        /* Numero d'ordre pour affichage sur la liste */
    wxString  m_Doc;        /* Doc associee */
    wxString  m_KeyWord;    /* Mots cles associes */

    FOOTPRINT();
};

typedef boost::ptr_vector< FOOTPRINT > FOOTPRINT_LIST;

/* FOOTPRINT object list sort function. */
extern bool operator<( const FOOTPRINT& item1, const FOOTPRINT& item2 );

/* Gestion des noms des librairies */
extern const wxString FootprintAliasFileExtension;
extern const wxString RetroFileExtension;
extern const wxString ComponentFileExtension;

extern const wxString RetroFileWildcard;
extern const wxString FootprintAliasFileWildcard;

extern const wxString titleLibLoadError;

void Plume( int state );

#endif /* __CVPCB_H__ */
