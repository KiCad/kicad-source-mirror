E-series are defined in IEC 60063.

Available values are approximately equally spaced in a logarithmic scale.

	E24(5%):  1.0 1.1 1.2 1.3 1.5 1.6 1.8 2.0 2.2 2.4 2.7 3.0 3.3 3.6 3.9 4.3 4.7 5.1 5.6 6.2 6.8 7.5 8.2 9.1
	E12(10%): 1.0 1.2 1.5 1.8 2.2 2.7 3.3 3.9 4.7 5.6 6.8 8.2
	E6(20%):  1.0  -  1.5  -  2.2  -  3.3  -  4.7  -  6.8  -
	E3(50%):  1.0  -   -   -  2.2  -   -   -  4.7  -   -   -
	E1     :  1.0  -   -   -   -   -   -   -   -   -   -   -

- This calculator finds combinations of standard E-series (between 10Ω and 1MΩ) to create arbitrary values.
- You can enter the required resistance from 0.0025 to 4000 kΩ.
- Solutions using up to 4 components are given.

By default, the request value is always excluded from the solution set.  It is also possible to specify
up to two additional values to exclude from the solution if these component values are not available

Solutions are given in the following formats:

	R1 + R2 +...+ Rn	resistors in series
	R1 | R2 |...| Rn	resistors in parallel
	R1 + (R2|R3)...		any combination of the above
