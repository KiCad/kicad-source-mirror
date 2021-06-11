/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2012 Miguel Angel Ajo <miguelangel@nbee.es>
 * Copyright (C) 1992-2021 KiCad Developers, see AUTHORS.txt for contributors.
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



%include board_item.h         // generate code for this interface

/* Only for compatibility with old python scripts: */
const int S_SEGMENT = (const int)PCB_SHAPE_TYPE::SEGMENT;
const int S_RECT = (const int)PCB_SHAPE_TYPE::RECT;
const int S_ARC = (const int)PCB_SHAPE_TYPE::ARC;
const int S_CIRCLE = (const int)PCB_SHAPE_TYPE::CIRCLE;
const int S_POLYGON = (const int)PCB_SHAPE_TYPE::POLYGON;
const int S_CURVE = (const int)PCB_SHAPE_TYPE::CURVE;

%rename(Get) operator       BOARD_ITEM*;

%{
#include <board_item.h>
/* for compatibility with old python scripts: */
const int S_SEGMENT = (const int)PCB_SHAPE_TYPE::SEGMENT;
const int S_RECT = (const int)PCB_SHAPE_TYPE::RECT;
const int S_ARC = (const int)PCB_SHAPE_TYPE::ARC;
const int S_CIRCLE = (const int)PCB_SHAPE_TYPE::CIRCLE;
const int S_POLYGON = (const int)PCB_SHAPE_TYPE::POLYGON;
const int S_CURVE = (const int)PCB_SHAPE_TYPE::CURVE;
%}


%inline
{
    /// Cast down from EDA_ITEM/BOARD_ITEM to child
    BOARD_ITEM*   Cast_to_BOARD_ITEM(EDA_ITEM* base)    {  return dynamic_cast<BOARD_ITEM*>(base);    }
}


%{
class PCB_TEXT;
class PCB_DIM_ALIGNED;
class PCB_DIM_ORTHOGONAL;
class PCB_DIM_LEADER;
class PCB_DIM_CENTER;
class FOOTPRINT;
class PCB_GROUP;
class FP_TEXT;
class PCB_SHAPE;
class MARKER_PCB;
class BOARD;
class FP_SHAPE;
class PAD;
class PCB_TRACK;
class PCB_VIA;
class PCB_ARC;
class ZONE;
class FP_ZONE;
class PCB_TARGET;

// Anything targeted to the %wrapper section is extern "C" whereas code targeted
// to %header section is C++.
#ifdef __cplusplus
extern "C" {
#endif

static PCB_TEXT*             Cast_to_PCB_TEXT( BOARD_ITEM* );
static PCB_DIM_ALIGNED*      Cast_to_PCB_DIM_ALIGNED( BOARD_ITEM* );
static PCB_DIM_ORTHOGONAL*   Cast_to_PCB_DIM_ORTHOGONAL( BOARD_ITEM* );
static PCB_DIM_LEADER*       Cast_to_PCB_DIM_LEADER( BOARD_ITEM* );
static PCB_DIM_CENTER*       Cast_to_PCB_DIM_CENTER( BOARD_ITEM* );
static FOOTPRINT*            Cast_to_FOOTPRINT( BOARD_ITEM* );
static PCB_GROUP*            Cast_to_PCB_GROUP( BOARD_ITEM* );
static FP_TEXT*              Cast_to_FP_TEXT( BOARD_ITEM* );
static PCB_SHAPE*            Cast_to_PCB_SHAPE( BOARD_ITEM* );
static PCB_MARKER*           Cast_to_PCB_MARKER( BOARD_ITEM* );
static BOARD*                Cast_to_BOARD( BOARD_ITEM* );
static FP_SHAPE*             Cast_to_FP_SHAPE( BOARD_ITEM* );
static PAD*                  Cast_to_PAD( BOARD_ITEM* );
static PCB_TRACK*            Cast_to_PCB_TRACK( BOARD_ITEM* );
static PCB_VIA*              Cast_to_PCB_VIA( BOARD_ITEM* );
static PCB_ARC*              Cast_to_PCB_ARC( BOARD_ITEM* );
static ZONE*                 Cast_to_ZONE( BOARD_ITEM* );
static FP_ZONE*              Cast_to_FP_ZONE( BOARD_ITEM* );
static PCB_TARGET*           Cast_to_PCB_TARGET( BOARD_ITEM* );

#ifdef __cplusplus
}   // extern "C"
#endif
%}


static PCB_TEXT*             Cast_to_PCB_TEXT( BOARD_ITEM* );
static PCB_DIM_ALIGNED*      Cast_to_PCB_DIM_ALIGNED( BOARD_ITEM* );
static PCB_DIM_ORTHOGONAL*   Cast_to_PCB_DIM_ORTHOGONAL( BOARD_ITEM* );
static PCB_DIM_LEADER*       Cast_to_PCB_DIM_LEADER( BOARD_ITEM* );
static PCB_DIM_CENTER*       Cast_to_PCB_DIM_CENTER( BOARD_ITEM* );
static FOOTPRINT*            Cast_to_FOOTPRINT( BOARD_ITEM* );
static PCB_GROUP*            Cast_to_PCB_GROUP( BOARD_ITEM* );
static FP_TEXT*              Cast_to_FP_TEXT( BOARD_ITEM* );
static PCB_SHAPE*            Cast_to_PCB_SHAPE( BOARD_ITEM* );
static PCB_MARKER*           Cast_to_PCB_MARKER( BOARD_ITEM* );
static BOARD*                Cast_to_BOARD( BOARD_ITEM* );
static FP_SHAPE*             Cast_to_FP_SHAPE( BOARD_ITEM* );
static PAD*                  Cast_to_PAD( BOARD_ITEM* );
static PCB_TRACK*            Cast_to_PCB_TRACK( BOARD_ITEM* );
static PCB_VIA*              Cast_to_PCB_VIA( BOARD_ITEM* );
static PCB_ARC*              Cast_to_PCB_ARC( BOARD_ITEM* );
static ZONE*                 Cast_to_ZONE( BOARD_ITEM* );
static FP_ZONE*              Cast_to_FP_ZONE( BOARD_ITEM* );
static PCB_TARGET*           Cast_to_PCB_TARGET( BOARD_ITEM* );


%extend BOARD_ITEM
{
    %pythoncode
    %{
    def Cast(self):

        ct = self.GetClass()

        if ct=="PTEXT":
            return Cast_to_PCB_TEXT(self)
        elif ct=="BOARD":
            return Cast_to_BOARD(self)
        elif ct=="ALIGNED_DIMENSION":
            return Cast_to_PCB_DIM_ALIGNED(self)
        elif ct=="LEADER":
            return Cast_to_PCB_DIM_LEADER(self)
        elif ct=="CENTER_DIMENSION":
            return Cast_to_PCB_DIM_CENTER(self)
        elif ct=="ORTHOGONAL_DIMENSION":
            return Cast_to_PCB_DIM_ORTHOGONAL(self)
        elif ct=="PCB_SHAPE":
            return Cast_to_PCB_SHAPE(self)
        elif ct=="MGRAPHIC":
            return Cast_to_FP_SHAPE(self)
        elif ct=="MODULE":
            return Cast_to_FOOTPRINT(self)
        elif ct=="PCB_GROUP":
            return Cast_to_PCB_GROUP(self)
        elif ct=="PAD":
            return Cast_to_PAD(self)
        elif ct=="MTEXT":
            return Cast_to_FP_TEXT(self)
        elif ct=="VIA":
            return Cast_to_PCB_VIA(self)
        elif ct=="TRACK":
            return Cast_to_PCB_TRACK(self)
        elif ct=="ARC":
            return Cast_to_PCB_ARC(self)
        elif ct=="PCB_TARGET":
            return Cast_to_PCB_TARGET(self)
        elif ct=="ZONE":
            return Cast_to_ZONE(self)
        else:
            raise TypeError("Unsupported drawing class: %s" % ct)

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
static PCB_TEXT*             Cast_to_PCB_TEXT( BOARD_ITEM* self )             { return dynamic_cast<PCB_TEXT*>(self);             }
static PCB_DIM_ALIGNED*      Cast_to_PCB_DIM_ALIGNED( BOARD_ITEM* self )      { return dynamic_cast<PCB_DIM_ALIGNED *>(self);     }
static PCB_DIM_ORTHOGONAL*   Cast_to_PCB_DIM_ORTHOGONAL( BOARD_ITEM* self )   { return dynamic_cast<PCB_DIM_ORTHOGONAL *>(self);  }
static PCB_DIM_LEADER*       Cast_to_PCB_DIM_LEADER( BOARD_ITEM* self )       { return dynamic_cast<PCB_DIM_LEADER *>(self);      }
static PCB_DIM_CENTER*       Cast_to_PCB_DIM_CENTER( BOARD_ITEM* self )       { return dynamic_cast<PCB_DIM_CENTER *>(self);      }
static FOOTPRINT*            Cast_to_FOOTPRINT( BOARD_ITEM* self )            { return dynamic_cast<FOOTPRINT*>(self);            }
static PCB_GROUP*            Cast_to_PCB_GROUP( BOARD_ITEM* self )            { return dynamic_cast<PCB_GROUP*>(self);            }
static FP_TEXT*              Cast_to_FP_TEXT( BOARD_ITEM* self )              { return dynamic_cast<FP_TEXT*>(self);              }
static PCB_SHAPE*            Cast_to_PCB_SHAPE( BOARD_ITEM* self )            { return dynamic_cast<PCB_SHAPE*>(self);            }
static PCB_MARKER*           Cast_to_PCB_MARKER( BOARD_ITEM* self )           { return dynamic_cast<PCB_MARKER*>(self);           }
static BOARD*                Cast_to_BOARD( BOARD_ITEM* self )                { return dynamic_cast<BOARD*>(self);                }
static FP_SHAPE*             Cast_to_FP_SHAPE( BOARD_ITEM* self )             { return dynamic_cast<FP_SHAPE*>(self);             }
static PAD*                  Cast_to_PAD( BOARD_ITEM* self )                  { return dynamic_cast<PAD*>(self);                  }
static PCB_TRACK*            Cast_to_PCB_TRACK( BOARD_ITEM* self )            { return dynamic_cast<PCB_TRACK *>(self);           }
static PCB_VIA*              Cast_to_PCB_VIA( BOARD_ITEM* self )              { return dynamic_cast<PCB_VIA *>(self);             }
static PCB_ARC*              Cast_to_PCB_ARC( BOARD_ITEM* self )              { return dynamic_cast<PCB_ARC *>(self);             }
static ZONE*                 Cast_to_ZONE( BOARD_ITEM* self )                 { return dynamic_cast<ZONE*>(self);                 }
static FP_ZONE*              Cast_to_FP_ZONE( BOARD_ITEM* self )              { return dynamic_cast<FP_ZONE*>(self);              }
static PCB_TARGET*           Cast_to_PCB_TARGET( BOARD_ITEM* self )           { return dynamic_cast<PCB_TARGET*>(self);           }
%}
