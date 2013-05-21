/***************/
/* worksheet.h */
/***************/

// Values are in 1/1000 inch

#ifndef WORKSHEET_H_
#define WORKSHEET_H_

struct Ki_WorkSheetData
{
public:
    int               m_Type;
    Ki_WorkSheetData* Pnext;
    int               m_Posx, m_Posy;
    int               m_Endx, m_Endy;
    const wxChar*     m_Legende;
    const wxChar*     m_Text;
};

#endif // WORKSHEET_H_
