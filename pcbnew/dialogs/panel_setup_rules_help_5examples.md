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
<br><br>

