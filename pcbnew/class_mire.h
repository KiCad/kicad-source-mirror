/****************************************************/
/* fonctions de la classe MIRE (targets for photos) */
/****************************************************/
#ifndef MIRE_H
#define MIRE_H

#include "base_struct.h"


class MIREPCB : public EDA_BaseStruct
{
public:
    int     m_Width;
    wxPoint m_Pos;
    int     m_Shape;            // bit 0 : 0 = forme +, 1 = forme X
    int     m_Size;

public:
    MIREPCB( EDA_BaseStruct* StructFather );
    ~MIREPCB( void );

    bool    WriteMirePcbDescr( FILE* File );
    bool    ReadMirePcbDescr( FILE* File, int* LineNum );

    /* supprime du chainage la structure Struct */
    void    UnLink( void );

    void    Copy( MIREPCB* source );

    void    Draw( WinEDA_DrawPanel* panel, wxDC* DC, const wxPoint& offset, int mode_color );

    
    /**
     * Function HitTest
     * tests if the given wxPoint is within the bounds of this object.
     * @param refPos A wxPoint to test
     * @return bool - true if a hit, else false
     */
    bool    HitTest( const wxPoint& refPos );
};


#endif  // #define MIRE_H
