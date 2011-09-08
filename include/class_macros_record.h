/****************************************/
/* Macros: used to record and play      */
/* sequence hotkeys and their positions */
/****************************************/

#ifndef _CLASS_MACROS_RECORD_H
#define _CLASS_MACROS_RECORD_H

#include <list>
 
class MACROS_RECORD
{
public:
    int                         m_HotkeyCode;
    int                         m_Idcommand;
    wxPoint                     m_Position;
};

class MACROS_RECORDED
{
public:
    wxPoint                     m_StartPosition;
    std::list<MACROS_RECORD>    m_Record;
};

#endif      //  _CLASS_MACROS_RECORD_H
