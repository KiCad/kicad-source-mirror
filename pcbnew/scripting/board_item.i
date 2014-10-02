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
 * @brief board_item helpers, mainly for casting down to all child classes
 */


/* Cast downs from EDA_ITEM/BOARD_ITEM to childs */

%inline
{
    BOARD_ITEM*   Cast_to_BOARD_ITEM(EDA_ITEM* base)    {  return dynamic_cast<BOARD_ITEM*>(base);    }
}

%extend BOARD_ITEM
{
    TEXTE_PCB*        Cast_to_TEXTE_PCB()         {  return dynamic_cast<TEXTE_PCB*>(self);     }
    DIMENSION*        Cast_to_DIMENSION()         {  return dynamic_cast<DIMENSION*>(self);     }
    MODULE*           Cast_to_MODULE()            {  return dynamic_cast<MODULE*>(self);        }
    TEXTE_MODULE*     Cast_to_TEXTE_MODULE()      {  return dynamic_cast<TEXTE_MODULE*>(self);  }
    DRAWSEGMENT*      Cast_to_DRAWSEGMENT()       {  return dynamic_cast<DRAWSEGMENT*>(self);   }
    MARKER_PCB*       Cast_to_MARKER_PCB()        {  return dynamic_cast<MARKER_PCB*>(self);    }
    BOARD*            Cast_to_BOARD()             {  return dynamic_cast<BOARD*>(self);         }
    EDGE_MODULE*      Cast_to_EDGE_MODULE()       {  return dynamic_cast<EDGE_MODULE*>(self);   }
    D_PAD*            Cast_to_D_PAD()             {  return dynamic_cast<D_PAD*>(self);         }
    TRACK*            Cast_to_TRACK()             {  return dynamic_cast<TRACK*>(self);         }
    ZONE_CONTAINER*   Cast_to_ZONE_CONTAINER()    {  return dynamic_cast<ZONE_CONTAINER*>(self);}
    VIA*              Cast_to_VIA()               {  return dynamic_cast<VIA*>(self);           }


    %pythoncode
    {
    def Cast(self):

        ct = self.GetClass()

        if ct=="PTEXT":
            return self.Cast_to_TEXTE_PCB()
        elif ct=="BOARD":
            return self.Cast_to_BOARD()
        elif ct=="DIMENSION":
            return self.Cast_to_DIMENSION()
        elif ct=="DRAWSEGMENT":
            return self.Cast_to_DRAWSEGMENT()
        elif ct=="MGRAPHIC":
            return self.Cast_to_EDGE_MODULE()
        elif ct=="MODULE":
            return self.Cast_to_MODULE()
        elif ct=="PAD":
            return self.Cast_to_D_PAD()
        elif ct=="MTEXT":
            return self.Cast_to_TEXTE_MODULE()
        elif ct=="VIA":
            return self.Cast_to_VIA()
        elif ct=="TRACK":
            return self.Cast_to_TRACK()
        elif ct=="ZONE_CONTAINER":
            return self.Cast_to_ZONE_CONTAINER()
        else:
            return None


    def Duplicate(self):

        ct = self.GetClass()

        if ct=="BOARD":
            return None
        else:
            return Cast_to_BOARD_ITEM(self.Clone()).Cast()
    }
}
