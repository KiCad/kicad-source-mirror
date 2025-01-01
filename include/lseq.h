/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef LSEQ_H
#define LSEQ_H

#include <vector>
#include <layer_ids.h>

/// A sequence of layers, a sequence provides a certain order.
typedef std::vector<PCB_LAYER_ID>   BASE_SEQ;


/**
 * LSEQ is a sequence (and therefore also a set) of PCB_LAYER_IDs.  A sequence provides
 * a certain order.
 * <p>
 * It can also be used as an iterator:
 * <code>
 *
 *      for( LSEQ cu_stack = aSet.CuStack();  cu_stack;  ++cu_stack )
 *      {
 *          layer_id = *cu_stack;
 *          :
 *          things to do with layer_id;
 *      }
 *
 * </code>
 */
class KICOMMON_API LSEQ : public BASE_SEQ
{
public:

    LSEQ()
    {}

    template <class InputIterator>
    LSEQ( InputIterator aStart, InputIterator aEnd ) :
        BASE_SEQ( aStart, aEnd )
    {}

    LSEQ( std::initializer_list<PCB_LAYER_ID> aLayers ) :
        BASE_SEQ( aLayers )
    {}

    int TestLayers( PCB_LAYER_ID aRhs, PCB_LAYER_ID aLhs ) const;
};


#endif // LSEQ_H
