/********************************************/
/* Definitions for the EESchema program:	*/
/********************************************/

#ifndef PROGRAM_H
#define PROGRAM_H

#ifndef eda_global
#define eda_global extern
#endif

#include "macros.h"
#include "base_struct.h"

#include "component_class.h"
#include "class_screen.h"
#include "class_drawsheet.h"
#include "class_text-label.h"

#define DRAWJUNCTION_SIZE  16       /* Rayon du symbole connexion */
#define DRAWMARKER_SIZE    16       /* Rayon du symbole marqueur */
#define DRAWNOCONNECT_SIZE 48       /* Rayon du symbole No Connexion */

#define HIGHLIGHT_COLOR WHITE


#define TEXT_NO_VISIBLE 1

/* flags pour BUS ENTRY (bus to bus ou wire to bus */
#define WIRE_TO_BUS 0
#define BUS_TO_BUS  1


typedef enum {      /* Type des Marqueurs */
    MARQ_UNSPEC,
    MARQ_ERC,
    MARQ_PCB,
    MARQ_SIMUL,
    MARQ_NMAX        /* Derniere valeur: fin de tableau */
} TypeMarker;

/* Messages correspondants aux types des marqueurs */
#ifdef MAIN
const wxChar*        NameMarqueurType[] =
{
    wxT( "" ),
    wxT( "ERC" ),
    wxT( "PCB" ),
    wxT( "SIMUL" ),
    wxT( "?????" )
};
#else
extern const wxChar* NameMarqueurType[];
#endif


/* Forward declarations */
class DrawSheetStruct;


/**
 * Class EDA_DrawLineStruct
 * is a segment decription base class to describe items which have 2 end
 * points (track, wire, draw line ...)
 */
class EDA_DrawLineStruct : public EDA_BaseStruct
{
public:
    int     m_Layer;            // Layer number
    int     m_Width;            // 0 = line, > 0 = tracks, bus ...
    wxPoint m_Start;            // Line start point
    wxPoint m_End;              // Line end point

    bool    m_StartIsDangling;
    bool    m_EndIsDangling;       // TRUE si Start ou End not connected  (wires, tracks...)

public:
    EDA_DrawLineStruct( const wxPoint& pos, int layer );
    ~EDA_DrawLineStruct() { }

    virtual wxString GetClass() const
    {
        return wxT( "EDA_DrawLine" );
    }


    bool                IsOneEndPointAt( const wxPoint& pos );
    EDA_DrawLineStruct* GenCopy();

    bool IsNull()
    {
        return m_Start == m_End;
    }

    EDA_Rect GetBoundingBox();

    virtual void Draw( WinEDA_DrawPanel* panel, wxDC* DC, const wxPoint& offset, int draw_mode,
                       int Color = -1 );

#if defined(DEBUG)
    /**
     * Function Show
     * is used to output the object tree, currently for debugging only.
     * @param nestLevel An aid to prettier tree indenting, and is the level
     *          of nesting of this object within the overall tree.
     * @param os The ostream& to output to.
     */
    void Show( int nestLevel, std::ostream& os );
#endif
};


class DrawMarkerStruct    : public EDA_BaseStruct   /* marqueurs */
{
public:
    wxPoint    m_Pos;               /* XY coordinates of marker. */
    TypeMarker m_Type;
    int        m_MarkFlags;         // complements d'information
    wxString   m_Comment;           /* Texte (commentaireassocie eventuel */

public:
    DrawMarkerStruct( const wxPoint& pos, const wxString& text );
    ~DrawMarkerStruct();
    virtual wxString GetClass() const
    {
        return wxT( "DrawMarker" );
    }


    DrawMarkerStruct*   GenCopy();
    wxString            GetComment();
    virtual void        Draw( WinEDA_DrawPanel* panel, wxDC* DC, const wxPoint& offset,
                              int draw_mode, int Color = -1 );
#if defined(DEBUG)
    /**
     * Function Show
     * is used to output the object tree, currently for debugging only.
     * @param nestLevel An aid to prettier tree indenting, and is the level
     *          of nesting of this object within the overall tree.
     * @param os The ostream& to output to.
     */
    void Show( int nestLevel, std::ostream& os );
#endif
};


class DrawNoConnectStruct : public EDA_BaseStruct    /* Symboles de non connexion */
{
public:
    wxPoint m_Pos;                      /* XY coordinates of NoConnect. */

public:
    DrawNoConnectStruct( const wxPoint& pos );
    ~DrawNoConnectStruct() { }
    virtual wxString GetClass() const
    {
        return wxT( "DrawNoConnect" );
    }


    DrawNoConnectStruct*    GenCopy();
    virtual void            Draw( WinEDA_DrawPanel* panel, wxDC* DC, const wxPoint& offset,
                                  int draw_mode, int Color = -1 );
    EDA_Rect            GetBoundingBox();
};


/**
 * Class DrawBusEntryStruct
 * Struct de descr 1 raccord a 45 degres de BUS ou WIRE
 */
class DrawBusEntryStruct  : public EDA_BaseStruct
{
public:
    int     m_Layer;
    int     m_Width;
    wxPoint m_Pos;
    wxSize  m_Size;

public:
    DrawBusEntryStruct( const wxPoint& pos, int shape, int id );
    ~DrawBusEntryStruct() { }

    virtual wxString GetClass() const
    {
        return wxT( "DrawBusEntry" );
    }


    DrawBusEntryStruct* GenCopy();
    wxPoint             m_End(); // retourne la coord de fin du raccord
    virtual void        Draw( WinEDA_DrawPanel* panel, wxDC* DC, const wxPoint& offset,
                              int draw_mode, int Color = -1 );
    EDA_Rect            GetBoundingBox();
};

class DrawPolylineStruct  : public EDA_BaseStruct /* Polyligne (serie de segments) */
{
public:
    int  m_Layer;
    int  m_Width;
    int  m_NumOfPoints;             /* Number of XY pairs in Points array. */
    int* m_Points;                  /* XY pairs that forms the polyline. */

public:
    DrawPolylineStruct( int layer );
    ~DrawPolylineStruct();

    virtual wxString GetClass() const
    {
        return wxT( "DrawPolyline" );
    }


    DrawPolylineStruct* GenCopy();
    virtual void        Draw( WinEDA_DrawPanel* panel, wxDC* DC, const wxPoint& offset,
                              int draw_mode, int Color = -1 );
};

class DrawJunctionStruct  : public EDA_BaseStruct
{
public:
    int     m_Layer;
    wxPoint m_Pos;                  /* XY coordinates of connection. */

public:
    DrawJunctionStruct( const wxPoint& pos );
    ~DrawJunctionStruct() { }

    virtual wxString GetClass() const
    {
        return wxT( "DrawJunction" );
    }

    EDA_Rect GetBoundingBox();

    DrawJunctionStruct* GenCopy();
    virtual void        Draw( WinEDA_DrawPanel* panel, wxDC* DC, const wxPoint& offset,
                              int draw_mode, int Color = -1 );
};



#define MAX_LAYERS 44
class LayerStruct
{
public:
    char LayerNames[MAX_LAYERS + 1][8];
    int  LayerColor[MAX_LAYERS + 1];
    char LayerStatus[MAX_LAYERS + 1];
    int  NumberOfLayers;
    int  CurrentLayer;
    int  CurrentWidth;
    int  CommonColor;
    int  Flags;
};


#endif /* PROGRAM_H */
