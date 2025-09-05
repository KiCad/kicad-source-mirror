### Examples

    (version 1)

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

    # Total routed length across every net in signal SIG_A
    (rule sig_group_length
       (constraint signal_length (min 40mm) (max 60mm))
       (condition "A.Net.Signal == 'SIG_A'") )

    # Total propagation delay across every net in signal CLK_GRP
    (rule sig_group_delay
       (constraint signal_length (min 180ps) (max 220ps) (time_domain yes))
       (condition "A.Net.Signal == 'CLK_GRP'") )
<br><br>


