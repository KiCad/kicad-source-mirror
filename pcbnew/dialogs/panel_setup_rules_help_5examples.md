### Examples

    (version 2)

    (rule HV
       (constraint clearance (min 1.5mm))
       (condition "A.hasNetclass('HV')"))


    (rule HV
       (layer outer)
       (constraint clearance (min 1.5mm))
       (condition "A.hasNetclass('HV')"))


    (rule HV_HV
       # wider clearance between HV tracks
       (constraint clearance (min "1.5mm + 2.0mm"))
       (condition "A.hasNetclass('HV') && B.hasNetclass('HV')"))


    (rule HV_unshielded
       (constraint clearance (min 2mm))
       (condition "A.hasNetclass('HV') && !A.enclosedByArea('Shield*')"))


    (rule heavy_thermals
       (constraint thermal_spoke_width (min 0.5mm))
       (condition "A.hasNetclass('HV')"))

    # Total routed length across every net in chain SIG_A
    (rule sig_group_length
       (constraint net_chain_length (min 40mm) (max 60mm))
       (condition "A.Net.NetChain == 'SIG_A'") )

    # Total propagation delay across every net in chain CLK_GRP
    (rule sig_group_delay
       (constraint net_chain_length (min 180ps) (max 220ps) (time_domain yes))
       (condition "A.Net.NetChain == 'CLK_GRP'") )

    # --- Net chain rules: multi-net signals through series passives ---

    # Same effect as sig_group_length, scoped via inNetChain() instead.
    (rule "DDR_DQ chain length"
       (constraint net_chain_length (min 40mm) (max 60mm))
       (condition "A.inNetChain('DDR_DQ*')") )

    # Stub branches off the trunk of any DDR chain may not exceed 2 mm.
    (rule "DDR stub length"
       (constraint stub_length (max 2mm))
       (condition "A.inNetChain('DDR_*')") )

    # Every chain in the HighSpeed class needs an unbroken B.Cu return.
    (rule "HS return path"
       (constraint return_path (layer "B.Cu"))
       (condition "A.inNetChainClass('HighSpeed')") )

    # Tighter clearance for any net that belongs to a chain.
    (rule "chain members tighter clearance"
       (constraint clearance (min 0.2mm))
       (condition "A.hasNetChain()") )
<br><br>


