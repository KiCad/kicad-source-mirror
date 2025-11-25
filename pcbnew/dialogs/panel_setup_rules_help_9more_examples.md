### More Examples

    (rule "copper keepout"
       (constraint disallow track via zone)
       (condition "A.intersectsArea('zone3')"))


    (rule "BGA neckdown"
       (constraint track_width (min 0.2mm) (opt 0.25mm))
       (constraint clearance (min 0.05mm) (opt 0.08mm))
       (condition "A.intersectsCourtyard('U3')"))


    # prevent silk over tented vias
    (rule silk_over_via
       (constraint silk_clearance (min 0.2mm))
       (condition "A.Type == '*Text' && B.Type == 'Via'"))


    (rule "Distance between Vias of Different Nets"
        (constraint hole_to_hole (min 0.254mm))
        (condition "A.Type == 'Via' && B.Type == 'Via' && A.Net != B.Net"))

    (rule "Clearance between Pads of Different Nets"
        (constraint clearance (min 3.0mm))
        (condition "A.Type == 'Pad' && B.Type == 'Pad' && A.Net != B.Net"))


    (rule "Via Hole to Track Clearance"
        (constraint hole_clearance (min 0.254mm))
        (condition "A.Type == 'Via' && B.Type == 'Track'"))

    (rule "Pad to Track Clearance"
        (constraint clearance (min 0.2mm))
        (condition "A.Type == 'Pad' && B.Type == 'Track'"))


    (rule "clearance-to-1mm-cutout"
        (constraint edge_clearance (min 0.8mm))
        (condition "A.Layer == 'Edge.Cuts' && A.Line_Width == 1.0mm"))


    (rule "Max Drill Hole Size Mechanical"
        (constraint hole_size (max 6.3mm))
        (condition "A.Pad_Type == 'NPTH, mechanical'"))

    (rule "Max Drill Hole Size PTH"
        (constraint hole_size (max 6.35mm))
        (condition "A.Pad_Type == 'Through-hole'"))


    # Specify an optimal gap for a particular diff-pair
    (rule "dp clock gap"
        (constraint diff_pair_gap (opt "0.8mm"))
        (condition "A.inDiffPair('/CLK')"))

    # Specify a larger clearance around any diff-pair
    (rule "dp clearance"
        (constraint clearance (min "1.5mm"))
        (condition "A.inDiffPair('*') && !AB.isCoupledDiffPair()"))


    # Don't use thermal reliefs on heatsink pads
    (rule heat_sink_pad
        (constraint zone_connection solid)
        (condition "A.Fabrication_Property == 'Heatsink pad'"))

    # Require all four thermal relief spokes to connect to parent zone
    (rule fully_spoked_pads
        (constraint min_resolved_spokes 4))

    # Set thermal relief gap & spoke width for all zones
    (rule defined_relief
        (constraint thermal_relief_gap (min 10mil))
        (constraint thermal_spoke_width (min 12mil)))

    # Override thermal relief gap & spoke width for GND and PWR zones
    (rule defined_relief_pwr
        (constraint thermal_relief_gap (min 10mil))
        (constraint thermal_spoke_width (min 12mil))
        (condition "A.Name == 'zone_GND' || A.Name == 'zone_PWR'"))

    # Prevent copper fills under the courtyards of capacitors
    (rule no_copper_under_caps
        (constraint physical_clearance (min 0mm))
        (condition "A.Type == 'Zone' && B.Reference == 'C*'"))


    # Prevent solder wicking from SMD pads
    (rule holes_in_pads
        (constraint physical_hole_clearance (min 0.2mm))
        (condition "B.Pad_Type == 'SMD'"))

    # Disallow solder mask margin overrides
    (rule "disallow solder mask margin overrides"
        (constraint assertion "A.Soldermask_Margin_Override == null")
        (condition "A.Type == 'Pad'"))


    # Enforce a mechanical clearance between components and board edge
    (rule front_mechanical_board_edge_clearance
        (layer "F.Courtyard")
        (constraint physical_clearance (min 3mm))
        (condition "B.Layer == 'Edge.Cuts'"))


    # Allow silk intersection with board edge for connectors
    (rule silk_board_edge_clearance
        (constraint silk_clearance)
        (severity ignore)
        (condition "A.memberOfFootprint('J*') && B.Layer=='Edge.Cuts'"))


    # Check current-carrying capacity
    (rule high-current
        (constraint track_width (min 1.0mm))
        (constraint connection_width (min 0.8mm))
        (condition "A.hasNetclass('Power')"))


    # Separate drill bit and milling cutter size constraints
    (rule "Plated through-hole size"
        (constraint hole_size (min 0.2mm) (max 6.35mm))
        (condition "A.isPlated() && A.Hole_Size_X == A.Hole_Size_Y"))
    (rule "Plated slot size"
        (constraint hole_size (min 0.5mm))
        (condition "A.isPlated() && A.Hole_Size_X != A.Hole_Size_Y"))


    # Allow blind/buried to micro-via hole-to-hole violations when it is known that
    # the fab will mechanically drill blind/buried via holes -before- laser drilling 
    # micro-vias.
    (rule hole_to_hole_uvia_exclusion
        (condition "A.Via_Type == 'Blind/buried' && B.Via_Type == 'Micro'")
        (constraint hole_to_hole)
        (severity ignore))


    # No solder mask expansion for vias.
    (rule "no mask expansion on vias"
        (constraint solder_mask_expansion (opt 0mm))
        (condition "A.type == via"))


    # Remove solder paste from DNP footprints.
    (rule remove_solder_paste_from_DNP
        (constraint solder_paste_abs_margin (opt -50mm))
        (condition "A.Do_not_Populate"))


    # Allow solder mask bridging under guard ring mask apertures
    (rule guard_ring_bridging
        (constraint bridged_mask)
        (condition "A.intersectsArea('guard_ring')")
        (severity ignore))
