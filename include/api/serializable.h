/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2024 Jon Evans <jon@craftyjon.com>
 * Copyright (C) 2024 KiCad Developers, see AUTHORS.txt for contributors.
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

#ifndef KICAD_SERIALIZABLE_H
#define KICAD_SERIALIZABLE_H

#include <wx/debug.h>

namespace google {
    namespace protobuf {
        class Any;
    }
}

/**
 * Interface for objects that can be serialized to Protobuf messages
 */
class SERIALIZABLE
{
public:
    /**
     * Serializes this object to the given Any message.
     * The Any message's concrete type will be specific to the object in question.
     * @param aContainer will be filled with a message describing this object
     */
    virtual void Serialize( google::protobuf::Any &aContainer ) const
    {
        wxASSERT_MSG( false, wxS( "Serialize called on an object that doesn't implement it!" ) );
    }

    /**
     * Deserializes the given protobuf message into this object.
     * @param aContainer is an Any which should have a concrete type matching this object
     * @return true if unpacking and deserialization succeeded
     */
    virtual bool Deserialize( const google::protobuf::Any &aContainer )
    {
        wxASSERT_MSG( false, wxS( "Deserialize called on an object that doesn't implement it!" ) );
        return false;
    }
};

#endif //KICAD_SERIALIZABLE_H
