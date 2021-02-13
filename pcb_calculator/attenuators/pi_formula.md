### PI Attenuator
__Zin__ desired input impedance in &#x2126;<br>
__Zout__ desired output impedance<br>
__Zin = Zout__<br><br>

* __a__ attenuation in dB
* __L = 10<sup>a/20</sup>__ (the loss)
* __A = (L + 1) / (L - 1)__<br><br>
* ___R2 = (L- 1) / 2 \* &radic; ( (Zin \* Zout) / L)___
* ___R1 = 1 / (A / Zin - 1 / R2)___
* ___R3 = 1 / (A / Zout - 1 / R2)___