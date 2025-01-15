## Overview
The package consists of a set of functions which take as argument a subtype of `SimObject`.
These subtypes can be arbitrarily defined, but in practice contain a mixture of predefined
types like `MechanicalPoint` and user defined types (not subtypes of `SimObject`).  
For the predefined types, overloads for methods like `get_mass`, `get_inertia_matrix`,
`get_mechanical_reaction` etc. already exist (if there exists a reasonable meaning for that type).
The user defined types should be equipped with appropriate overloads, depending on the desired
interactions.  
The functions acting on subtypes of `SimObject` often expect certain interactions to be possible.
For example `get_center_of_gravity(obj::ObjT) where {ObjT <: SimObject}` assumes that all masses
and their positions are contained in types which have overloads for `get_mass` and `get_position`.

## On generic methods for SimObjects
A set of overloads for a specific object should all refer to the same *thing*. Although this
sounds obvious there are some unexpected constraints arising from that requirement.
For example, if `get_pos` returns the exact position of the object (whatever that means) then
`get_mass` should also return the mass *of the thing at that position* and not the total mass
contained in the object, when some of that mass is at a different position. Instead the function
`get_total_mass` should be used. Note how `get_total_pos` has no obvious meaning.
