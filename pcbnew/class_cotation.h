/************************************/
/* fonctions de la classe COTATION */
/************************************/
#ifndef COTATION_H
#define COTATION_H

#include "base_struct.h"

class COTATION : public BOARD_ITEM
{
public:
    int        m_Width;
    wxPoint    m_Pos;
    int        m_Shape;
    int        m_Unit;  /* 0 = inches, 1 = mm */
    int        m_Value; /* valeur en unites PCB de la cote */

    TEXTE_PCB* m_Text;  /* pour affichage du texte */
    int        Barre_ox, Barre_oy, Barre_fx, Barre_fy;
    int        TraitG_ox, TraitG_oy, TraitG_fx, TraitG_fy;
    int        TraitD_ox, TraitD_oy, TraitD_fx, TraitD_fy;
    int        FlecheD1_ox, FlecheD1_oy, FlecheD1_fx, FlecheD1_fy;
    int        FlecheD2_ox, FlecheD2_oy, FlecheD2_fx, FlecheD2_fy;
    int        FlecheG1_ox, FlecheG1_oy, FlecheG1_fx, FlecheG1_fy;
    int        FlecheG2_ox, FlecheG2_oy, FlecheG2_fx, FlecheG2_fy;

public:
    COTATION( BOARD_ITEM* StructFather );
    ~COTATION();

    bool    ReadCotationDescr( FILE* File, int* LineNum );
    bool    WriteCotationDescr( FILE* File );

    /* supprime du chainage la structure Struct */
    void    UnLink();

    /* Modification du texte de la cotation */
    void    SetText( const wxString& NewText );

    void    Copy( COTATION* source );

    void    Draw( WinEDA_DrawPanel* panel, wxDC* DC, const wxPoint& offset, int mode_color );

    
    /**
     * Function Display_Infos
     * has knowledge about the frame and how and where to put status information
     * about this object into the frame's message panel.
     * Is virtual from EDA_BaseStruct.
     * @param frame A WinEDA_DrawFrame in which to print status information.
     */ 
    void    Display_Infos( WinEDA_DrawFrame* frame );
    
    /**
     * Function HitTest
     * tests if the given wxPoint is within the bounds of this object.
     * @param ref_pos A wxPoint to test
     * @return bool - true if a hit, else false
     */
    bool    HitTest( const wxPoint& ref_pos );
    
    
    /**
     * Function GetClass
     * returns the class name.
     * @return wxString
     */
    wxString GetClass() const
    {
        return wxT( "DIMENSION" );
    }
};

#endif  // #define COTATION_H
