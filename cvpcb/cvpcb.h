/**********************************************/
/* CVPCB : declaration des variables globales */
/**********************************************/


#ifndef eda_global
#define eda_global extern
#endif

#include "wxstruct.h"
#include "pcbnew.h"
#include "cvstruct.h"

#include "gr_basic.h"
#include "colors.h"

// Define print format d to display a schematic component line
#define CMP_FORMAT wxT( "%3d %8s - %16s : %-.32s" )

#define FILTERFOOTPRINTKEY "FilterFootprint"

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


eda_global STOREMOD* g_BaseListePkg;
eda_global STORECMP* g_BaseListeCmp;

eda_global FILE*     source;
eda_global FILE*     dest;
eda_global FILE*     libcmp;
eda_global FILE*     lib_module;

/* nom des fichiers a traiter */
eda_global wxString  FFileName;
eda_global wxString  NetNameBuffer;

/* Types de netliste: */
#define TYPE_NON_SPECIFIE  0
#define TYPE_ORCADPCB2     1
#define TYPE_PCAD          2
#define TYPE_VIEWLOGIC_WIR 3
#define TYPE_VIEWLOGIC_NET 4

/* Gestion des noms des librairies */
eda_global wxString g_EquivExtBuffer
#ifdef MAIN
( wxT( ".equ" ) )
#endif
;
eda_global wxString g_ExtCmpBuffer
#ifdef MAIN
( wxT( ".cmp" ) )
#endif
;

eda_global wxString      g_UserNetDirBuffer;    // Netlist path (void = current working directory)

eda_global wxArrayString g_ListName_Equ;        // list of .equ files to load

eda_global int           g_FlagEESchema;
eda_global int           Rjustify; /* flag pout troncature des noms de Net:
                                  * = 0: debut de chaine conservee (->ORCADPCB2)
                                  * = 1: fin de chaine conservee (->VIEWLOGIC) */
eda_global int           selection_type;    /* 0 pour sel par U??, 1 pour sel par ref ORCADPCB */

eda_global int           modified;          /* Flag != 0 si modif attribution des modules */
eda_global int           ListModIsModified; /* Flag != 0 si modif liste des lib modules */

eda_global char          alim[1024];

eda_global int           nbcomp;                    /* nombre de composants trouves */
eda_global int           nblib;                     /* nombre d'empreintes trouvees */
eda_global int           composants_non_affectes;   /* nbre de composants non affectes */

eda_global wxString      NameBuffer;
eda_global wxString      NetInNameBuffer;
eda_global wxString      NetInExtBuffer;
eda_global wxString      PkgInExtBuffer;
eda_global wxString      NetDirBuffer;

eda_global wxString      ExtRetroBuffer
#ifdef MAIN
( wxT( ".stf" ) )
#endif
;


// Variables generales */
// Unused, for pcbnew compatibility:
eda_global Ki_PageDescr* SheetList[]
#ifdef MAIN
= { NULL }
#endif
;

// Unused, for pcbnew compatibility:
void Plume( int state );
