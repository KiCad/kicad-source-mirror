### Expression functions

All function parameters support simple wildcards (`*` and `?`).
<br><br>

    A.enclosedByArea('<zone_name>')
True if all of `A` lies within the given zone's outline.

NB: this is potentially a more expensive call than `intersectsArea()`.  Use `intersectsArea()`
where possible.
<br><br>

    A.existsOnLayer('<layer_name>')
True if `A` exists on the given layer.  The layer name can be
either the name assigned in Board Setup > Board Editor Layers or
the canonical name (ie: `F.Cu`).

NB: this returns true if `A` is on the given layer, independently
of whether or not the rule is being evaluated for that layer.
For the latter use a `(layer "layer_name")` clause in the rule.
<br><br>

    A.fromTo('x', 'y')
True if the object exists on the copper path between the given 
pads. `x` and `y` are the full names of pads in the design, such as 
`R1-Pad1`.
<br><br>

    A.getField('<field_name>')
The value of the given field. Only footprints have fields, so a field is only returned if
`A` is a footprint.
<br><br>

    A.hasComponentClass('<component_class_name>')
True if the set of component classes assigned to `A` contains the named 
component class.
<br><br>

    A.hasNetclass('<netclass_name>')
True if `A` has had the given netclass assigned to it, either by an explicit netclass label
or through a pattern match assignment.
<br><br>

    A.inDiffPair('<net_name>')
True if `A` has a net that is part of the specified differential pair.
`<net_name>` is the base name of the differential pair.  For example, `inDiffPair('/CLK')`
matches items in the `/CLK_P` and `/CLK_N` nets.
<br><br>

    A.intersectsArea('<zone_name>')
True if any part of `A` lies within the given zone's outline.
<br><br>

    A.intersectsCourtyard('<footprint_identifier>')
True if any part of `A` lies within the given footprint's principal courtyard.
<br><br>

    A.intersectsFrontCourtyard('<footprint_identifier>')
True if any part of `A` lies within the given footprint's front courtyard.
<br><br>

    A.intersectsBackCourtyard('<footprint_identifier>')
True if any part of `A` lies within the given footprint's back courtyard.
<br><br>

The `footprint_identifier` listed above can be one of the following:

1. A reference designator, possibly containing wildcards `*` and `?`
2. A footprint library identifier such as `LibName:FootprintName`. In this case,
   the library identifier must contain the `:` character to separate the library
   name from the footprint name, and either name may contain wildcards.
3. A component class, in the form `${Class:ClassName}`.  The keyword `Class` is not
   case-sensitive, but component class names are case-sensitive.

<br>

    A.isBlindBuriedVia()
True if `A` is a blind/buried via.
<br><br>

    AB.isCoupledDiffPair()
True if `A` and `B` are members of the same diff pair.
<br><br>

    A.isMicroVia()
True if `A` is a microvia.
<br><br>

    A.isPlated()
True if `A` has a hole which is plated.
<br><br>

    A.memberOfGroup('<group_name>')
True if `A` is a member of the given group. The name can contain wildcards.
Includes nested membership.
<br><br>

    A.memberOfFootprint('<footprint_identifier>')
True if `A` is a member of a given footprint (for example, a pad or graphic shape defined
inside that footprint).  The various ways of specifying `footprint_identifier` are described above.
<br><br>

    A.memberOfSheet('<sheet_path>')
True if `A` is a member of the given schematic sheet. The sheet path can contain wildcards.
<br><br>

    A.memberOfSheetOrChildren('<sheet_path>')
True if `A` is a member of the given schematic sheet, or any of its child hierarchical sheets. The sheet path can 
contain wildcards.
<br><br>

