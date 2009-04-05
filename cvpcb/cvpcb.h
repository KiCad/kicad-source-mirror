/**********************************************/
/* CVPCB : declaration des variables globales */
/**********************************************/

#ifndef __CVPCB_H__
#define __CVPCB_H__

#include "pcbcommon.h"

// config for footprints doc file acces
#define DOC_FOOTPRINTS_LIST_KEY wxT("footprints_doc_file")
#define DEFAULT_FOOTPRINTS_LIST_FILENAME wxT("footprints_doc/footprints.pdf")

// Define print format to display a schematic component line
#define CMP_FORMAT wxT( "%3d %8s - %16s : %-.32s" )

#define FILTERFOOTPRINTKEY "FilterFootprint"

/* Types de netliste: */
#define TYPE_NON_SPECIFIE  0
#define TYPE_ORCADPCB2     1
#define TYPE_PCAD          2
#define TYPE_VIEWLOGIC_WIR 3
#define TYPE_VIEWLOGIC_NET 4


enum TypeOfStruct {
    STRUCT_NOT_INIT,
    STRUCT_COMPONENT,
    STRUCT_PIN,
    STRUCT_MODULE,
    STRUCT_PSEUDOMODULE
};

class STOREPIN
{
public:
    int       m_Type;           /* Type de la structure */
    STOREPIN* Pnext;            /* Chainage avant */
    int       m_Index;          /* variable utilisee selon types de netlistes */
    int       m_PinType;        /* code type electrique ( Entree Sortie Passive..) */
    wxString  m_PinNet;         /* Pointeur sur le texte nom de net */
    wxString  m_PinNum;
    wxString  m_PinName;
    wxString  m_Repere;     /* utilise selon formats de netliste */

    STOREPIN();
};

class STORECMP
{
public:
    int           m_Type;       /* Type de la structure */
    STORECMP*     Pnext;        /* Chainage avant */
    STORECMP*     Pback;        /* Chainage arriere */
    int           m_Num;        /* Numero d'ordre */
    int           m_Multi;      /* Nombre d' unites par boitier */
    STOREPIN*     m_Pins;       /* pointeur sur la liste des Pins */
    wxString      m_Reference;  /* U3, R5  ... */
    wxString      m_Valeur;     /* 7400, 47K ... */
    wxString      m_TimeStamp;  /* Signature temporelle ("00000000" si absente) */
    wxString      m_Module;     /* Nom du module (Package) corresp */
    wxString      m_Repere;     /* utilise selon formats de netliste */
    wxArrayString m_FootprintFilter;    /* List of allowed footprints (wildcart allowed
                                          * if void: no filtering */

    STORECMP();
    ~STORECMP();
};

class STOREMOD
{
public:
    int       m_Type;       /* Type de la structure */
    STOREMOD* Pnext;        /* Chainage avant */
    STOREMOD* Pback;        /* Chainage arriere */
    wxString  m_Module;     /* Nom du module */
    wxString  m_LibName;    /* Nom de la librairie contenant ce module */
    int       m_Num;        /* Numero d'ordre pour affichage sur la liste */
    wxString  m_Doc;        /* Doc associee */
    wxString  m_KeyWord;    /* Mots cles associes */

    STOREMOD();
};


/* Gestion des noms des librairies */
extern const wxString EquivFileExtension;
extern const wxString RetroFileExtension;
extern const wxString ComponentFileExtension;

extern const wxString RetroFileWildcard;
extern const wxString EquivFileWildcard;

extern const wxString titleLibLoadError;

/* CvPcb global variable definition references. */
extern STOREMOD* g_BaseListePkg;
extern STORECMP* g_BaseListeCmp;

extern wxString   g_NetlistFileExtension;
extern wxString   g_UserNetDirBuffer;

extern wxArrayString g_ListName_Equ;        // list of .equ files to load

extern int    g_FlagEESchema;
extern int    Rjustify; /* flag pout troncature des noms de Net:
                         * = 0: debut de chaine conservee (->ORCADPCB2)
                         * = 1: fin de chaine conservee (->VIEWLOGIC) */

extern int    modified; /* Flag != 0 si modif attribution des module. */

extern int    nbcomp;                    /* nombre de composants trouves */
extern int    nblib;                     /* nombre d'empreintes trouvees */
extern int    composants_non_affectes;   /* nbre de composants non affectes */

void Plume( int state );

#endif /* __CVPCB_H__ */
