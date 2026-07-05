If you specify the maximum current, then the track widths will be calculated to suit.

If you specify one of the track widths, the maximum current it can handle will be calculated. The width for the other track to also handle this current will then be calculated.

The controlling value is shown in bold.

The calculations use the closed-form equations fit by Douglas Brooks and Johannes Adam to the IPC-2152 data, which superseded the older IPC-2221 charts in 2009.

The formula is
<center>___&Delta;T = K &middot; I<sup>a</sup> &middot; W<sup>b</sup> &middot; Th<sup>c</sup>___</center>
where:<br>
___I___ is maximum current in A<br>
___&Delta;T___ is temperature rise above ambient in &deg;C<br>
___W___ is width in mils<br>
___Th___ is thickness (height) in mils<br>
and the coefficients ___K___, ___a___, ___b___ and ___c___ are<br>
___K___ = 215.3, ___a___ = 2, ___b___ = -1.15, ___c___ = -1.0 for external tracks (all copper weights)<br>
and for internal tracks they depend on copper weight, selected from the nearest of:<br>
0.5 oz: ___K___ = 120, ___a___ = 2, ___b___ = -1.10, ___c___ = -1.52<br>
1 oz: ___K___ = 200, ___a___ = 1.9, ___b___ = -1.10, ___c___ = -1.52<br>
2 oz: ___K___ = 300, ___a___ = 2, ___b___ = -1.15, ___c___ = -1.52<br>
3 oz: ___K___ = 262.5, ___a___ = 1.9, ___b___ = -1.15, ___c___ = -1.52

Unlike the IPC-2221 charts, IPC-2152 found that internal traces cool about as well as external ones, so internal tracks are no longer derated by a fixed factor.
