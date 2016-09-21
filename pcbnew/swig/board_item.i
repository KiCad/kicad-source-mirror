/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2012 Miguel Angel Ajo <miguelangel@nbee.es>
 * Copyright (C) 1992-2012 KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, you may find one here:
 * http://www.gnu.org/licenses/old-licenses/gpl-2.0.html
 * or you may search the http://www.gnu.org website for the version 2 license,
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

/**
 * @file board_item.i
 * @brief board_item helpers, mainly for casting down to all derived classes
 */




%include class_board_item.h         // generate code for this interface

// this is to help python with the * accessor of DLIST templates
%rename(Get) operator       BOARD_ITEM*;
%template(BOARD_ITEM_List)  DLIST<BOARD_ITEM>;


%{
#include <class_board_item.h>
%}


%inline
{
    /// Cast down from EDA_ITEM/BOARD_ITEM to child
    BOARD_ITEM*   Cast_to_BOARD_ITEM(EDA_ITEM* base)    {  return dynamic_cast<BOARD_ITEM*>(base);    }
}


%{
class TEXTE_PCB;
class DIMENSION;
class MODULE;
class TEXTE_MODULE;
class DRAWSEGMENT;
class MARKER_PCB;
class BOARD;
class EDGE_MODULE;
class D_PAD;
class TRACK;
class VIA;
class ZONE_CONTAINER;
class PCB_TARGET;

// Anthing targeted to the %wrapper section is extern "C" whereas code targeted
// to %header section is C++.
#ifdef __cplusplus
extern "C" {
#endif

static TEXTE_PCB*        Cast_to_TEXTE_PCB( BOARD_ITEM* );
static DIMENSION*        Cast_to_DIMENSION( BOARD_ITEM* );
static MODULE*           Cast_to_MODULE( BOARD_ITEM* );
static TEXTE_MODULE*     Cast_to_TEXTE_MODULE( BOARD_ITEM* );
static DRAWSEGMENT*      Cast_to_DRAWSEGMENT( BOARD_ITEM* );
static MARKER_PCB*       Cast_to_MARKER_PCB( BOARD_ITEM* );
static BOARD*            Cast_to_BOARD( BOARD_ITEM* );
static EDGE_MODULE*      Cast_to_EDGE_MODULE( BOARD_ITEM* );
static D_PAD*            Cast_to_D_PAD( BOARD_ITEM* );
static TRACK*            Cast_to_TRACK( BOARD_ITEM* );
static VIA*              Cast_to_VIA( BOARD_ITEM* );
static ZONE_CONTAINER*   Cast_to_ZONE_CONTAINER( BOARD_ITEM* );
static PCB_TARGET*       Cast_to_PCB_TARGET( BOARD_ITEM* );

#ifdef __cplusplus
}   // extern "C"
#endif
%}


static TEXTE_PCB*        Cast_to_TEXTE_PCB( BOARD_ITEM* );
static DIMENSION*        Cast_to_DIMENSION( BOARD_ITEM* );
static MODULE*           Cast_to_MODULE( BOARD_ITEM* );
static TEXTE_MODULE*     Cast_to_TEXTE_MODULE( BOARD_ITEM* );
static DRAWSEGMENT*      Cast_to_DRAWSEGMENT( BOARD_ITEM* );
static MARKER_PCB*       Cast_to_MARKER_PCB( BOARD_ITEM* );
static BOARD*            Cast_to_BOARD( BOARD_ITEM* );
static EDGE_MODULE*      Cast_to_EDGE_MODULE( BOARD_ITEM* );
static D_PAD*            Cast_to_D_PAD( BOARD_ITEM* );
static TRACK*            Cast_to_TRACK( BOARD_ITEM* );
static VIA*              Cast_to_VIA( BOARD_ITEM* );
static ZONE_CONTAINER*   Cast_to_ZONE_CONTAINER( BOARD_ITEM* );
static PCB_TARGET*       Cast_to_PCB_TARGET( BOARD_ITEM* );


%extend BOARD_ITEM
{
    %pythoncode
    %{
    def Cast(self):

        ct = self.GetClass()

        if ct=="PTEXT":
            return Cast_to_TEXTE_PCB(self)
        elif ct=="BOARD":
            return Cast_to_BOARD(self)
        elif ct=="DIMENSION":
            return Cast_to_DIMENSION(self)
        elif ct=="DRAWSEGMENT":
            return Cast_to_DRAWSEGMENT(self)
        elif ct=="MGRAPHIC":
            return Cast_to_EDGE_MODULE(self)
        elif ct=="MODULE":
            return Cast_to_MODULE(self)
        elif ct=="PAD":
            return Cast_to_D_PAD(self)
        elif ct=="MTEXT":
            return Cast_to_TEXTE_MODULE(self)
        elif ct=="VIA":
            return Cast_to_VIA(self)
        elif ct=="TRACK":
            return Cast_to_TRACK(self)
        elif ct=="PCB_TARGET":
            return Cast_to_PCB_TARGET(self)
        elif ct=="ZONE_CONTAINER":
            return Cast_to_ZONE_CONTAINER(self)
        else:
            return None

    def Duplicate(self):
        ct = self.GetClass()
        if ct=="BOARD":
            return None
        else:
            return Cast_to_BOARD_ITEM(self.Clone()).Cast()

    def SetPos(self,p):
        self.SetPosition(p)
        self.SetPos0(p)

    def SetStartEnd(self,start,end):
        self.SetStart(start)
        self.SetStart0(start)
        self.SetEnd(end)
        self.SetEnd0(end)
    %}
}



// Use %wrapper code generation section so that this block of C++ comes after all referenced
// classes and therefore will C++ compile due to the respective headers which will go into
// the %header section.  See section 5.6.2 of SWIG 3.0 documentation.
%wrapper %{
static TEXTE_PCB*        Cast_to_TEXTE_PCB( BOARD_ITEM* self )       {  return dynamic_cast<TEXTE_PCB*>(self);       }
static DIMENSION*        Cast_to_DIMENSION( BOARD_ITEM* self )       {  return dynamic_cast<DIMENSION*>(self);       }
static MODULE*           Cast_to_MODULE( BOARD_ITEM* self )          {  return dynamic_cast<MODULE*>(self);          }
static TEXTE_MODULE*     Cast_to_TEXTE_MODULE( BOARD_ITEM* self )    {  return dynamic_cast<TEXTE_MODULE*>(self);    }
static DRAWSEGMENT*      Cast_to_DRAWSEGMENT( BOARD_ITEM* self )     {  return dynamic_cast<DRAWSEGMENT*>(self);     }
static MARKER_PCB*       Cast_to_MARKER_PCB( BOARD_ITEM* self )      {  return dynamic_cast<MARKER_PCB*>(self);      }
static BOARD*            Cast_to_BOARD( BOARD_ITEM* self )           {  return dynamic_cast<BOARD*>(self);           }
static EDGE_MODULE*      Cast_to_EDGE_MODULE( BOARD_ITEM* self )     {  return dynamic_cast<EDGE_MODULE*>(self);     }
static D_PAD*            Cast_to_D_PAD( BOARD_ITEM* self )           {  return dynamic_cast<D_PAD*>(self);           }
static TRACK*            Cast_to_TRACK( BOARD_ITEM* self )           {  return dynamic_cast<TRACK*>(self);           }
static VIA*              Cast_to_VIA( BOARD_ITEM* self )             {  return dynamic_cast<VIA*>(self);             }
static ZONE_CONTAINER*   Cast_to_ZONE_CONTAINER( BOARD_ITEM* self )  {  return dynamic_cast<ZONE_CONTAINER*>(self);  }
static PCB_TARGET*       Cast_to_PCB_TARGET( BOARD_ITEM* self )      {  return dynamic_cast<PCB_TARGET*>(self);      }
%}
