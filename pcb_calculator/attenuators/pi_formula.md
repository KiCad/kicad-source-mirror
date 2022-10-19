### Pi Attenuator
___a___ is attenuation in dB<br>
___Z<sub>in</sub>___ is desired input impedance in &#x2126;<br>
___Z<sub>out</sub>___ is desired output impedance in &#x2126;<br>

___K = V<sub>I</sub>/V<sub>O</sub> = 10<sup>a/20</sup>___<br>
___L = K<sup>2</sup> = 10<sup>a/10</sup>___<br>
___A = (L+1) / (L&minus;1)___<br><br>
___R2 = (L&minus;1) / 2&middot;&radic;(Z<sub>in</sub> &middot; Z<sub>out</sub> / L)___<br>
___R1 = 1 / (A/Z<sub>in</sub> &minus; 1/R2)___<br>
___R3 = 1 / (A/Z<sub>out</sub> &minus; 1/R2)___
