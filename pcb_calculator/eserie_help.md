E-series defined in IEC 60063 are a widely accepted system of preferred
numbers for electronic components.  Available values are approximately
equally spaced in a logarithmic scale.  Although E-series are used for
Zener diodes, inductors and other components, this calculator is mainly
intended for resistors.

	E12: 1,0 1,2 1,5 1,8 2,2 2,7 3,3 3,9 4,7 5,6 6,8 8,2
	E6:  1,0  -  1,5  -  2,2  -  3,3  -  4,7  -  6,8  -
	E3:  1,0  -   -   -  2,2  -   -   -  4,7  -   -   -
	E1:  1,0  -   -   -   -   -   -   -   -   -   -   -
If your design requires any resistor value which is not readily available,
this calculator will find a combination of standard E-series components to
create it.  You can enter the required resistance from 0,0025 to 4000 KOhm. 
Solutions using 3 or 4 resistors are given if a better match can be found. 
The 4R checkbox option will take longer to process is considered for the E12
series only.  Optionally it is possible to exclude up to two additional
values from the solution for the reason of being not available.  If a
E-series value is entered to the required input field, it is always excluded
from any solution as it is assumed that this value is unavailable.

Solutions are given in the following formats:

	R1 + R2 +...+ Rn	resistors in series
	R1 | R2 |...| Rn	resistors in parallel
	R1 + (R2|R3)...		any combination of the above
__Example:__ Voltage dividers, commonly used for 1:10 range selection
require resistor ratio values 1:9.  Unfortunately the "9" is a value, what
is not even in the E192 series available.  Deviation of 1% and more is yet
unacceptable for 8 bit accuracy.  For a required resistor value of 9 KOhm,
the calculator suggests the E6 values 2k2 + 6k8 in series as a possible
exact solution.
