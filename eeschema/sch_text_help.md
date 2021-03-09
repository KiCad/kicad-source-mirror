<table>
    <tr>
        <th>Markup</th>
        <th></th>
        <th>Result</th>
    </tr>
    <tr>
        <td>&nbsp;<br><samp>^{superscript}</samp></td>
        <td></td>
        <td>&nbsp;<br><samp><sup>superscript</sup>&nbsp;</samp></td>
    </tr>
    <tr>
        <td>&nbsp;<br><samp>Driver Board^{Rev A}</samp></td>
        <td></td>
        <td>&nbsp;<br><samp>Driver Board<sup>Rev A</sup></samp></td>
    </tr>
    <tr>
        <td><br></td>
    </tr>
    <tr>
        <td>&nbsp;<br><samp>_{subscript}</samp></td>
        <td></td>
        <td>&nbsp;<br><samp><sub>subscript</sub>&nbsp;</samp></td>
    </tr>
    <tr>
        <td>&nbsp;<br><samp>D_{0} - D_{15}</samp></td>
        <td></td>
        <td>&nbsp;<br><samp>D<sub>0</sub> - D<sub>15</sub></samp></td>
    </tr>
    <tr>
        <td></td>
    </tr>
    <tr>
        <td> &nbsp;<br><samp>~overbar</samp><br> &nbsp;<br><samp>~CLK</samp><br> &nbsp;<br><samp>~~</samp> </td>
        <td></td>
        <td> <samp><u>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;</u></samp><br> <samp>overbar</samp><br> <samp><u>&nbsp;&nbsp;&nbsp;</u></samp><br> <samp>CLK</samp><br> <samp>&nbsp;</samp><br> <samp>~</samp> </td>
    </tr>
    <tr>
        <td><br></td>
    </tr>
    <tr>
        <td>&nbsp;<br><samp>${variable}</samp></td>
        <td></td>
        <td>&nbsp;<br><samp><i>variable_value</i></samp></td>
    </tr>
    <tr>
        <td>&nbsp;<br><samp>${REVISION}</samp></td>
        <td></td>
        <td>&nbsp;<br><samp>2020.1</samp></td>
    </tr>
    <tr>
        <td><br></td>
    </tr>
    <tr>
        <td>&nbsp;<br><samp>${refdes:field}</samp></td>
        <td></td>
        <td>&nbsp;<br><samp><i>field_value</i> of symbol <i>refdes</i></samp></td>
    </tr>
    <tr>
        <td>&nbsp;<br><samp>${R3:VALUE}</samp></td>
        <td></td>
        <td>&nbsp;<br><samp>150K</samp></td>
    </tr>
    <tr>
        <td><br></td>
    </tr>
    <tr>
        <td><br></td>
    </tr>
    <tr>
        <th>Bus Definition</th>
        <th>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;</th>
        <th>Resultant Nets</th>
    </tr>
    <tr>
        <td>&nbsp;<br><samp>prefix[m..n]</samp></td>
        <td></td>
        <td>&nbsp;<br><samp>prefixm to prefixn</samp></td>
    </tr>
    <tr>
        <td>&nbsp;<br><samp>D[0..7]</samp></td>
        <td></td>
        <td>&nbsp;<br><samp>D0, D1, D2, D3, D4, D5, D6, D7</samp></td>
    </tr>
    <tr>
        <td><br></td>
    </tr>
    <tr>
		<td>&nbsp;<br><samp>{net1 net2 ...}</samp></td>
		<td></td>
		<td>&nbsp;<br><samp>net1, net2, ...</samp></td>
    </tr>
    <tr>
        <td>&nbsp;<br><samp>{SCL SDA}</samp></td>
        <td></td>
        <td>&nbsp;<br><samp>SCL, SDA</samp></td>
    </tr>
    <tr>
        <td><br></td>
    </tr>
    <tr>
        <td>&nbsp;<br><samp>prefix{net1 net2 ...}</samp></td>
        <td></td>
        <td>&nbsp;<br><samp>prefix.net1, prefix.net2, ...</samp></td>
    </tr>
    <tr>
        <td>&nbsp;<br><samp>USB1{D+ D-}</samp></td>
        <td></td>
        <td>&nbsp;<br><samp>USB1.D+, USB1.D-</samp></td>
    </tr>
    <tr>
        <td><br></td>
    </tr>
    <tr>
        <td>&nbsp;<br><samp>MEM{D[1..2] LATCH}</samp></td>
        <td></td>
        <td>&nbsp;<br> <samp>MEM.D1, MEM.D2, MEM.LATCH</samp> </td>
    </tr>
    <tr>
        <td>&nbsp;<br><samp>MEM{D_{[1..2]} ~LATCH}</samp></td>
        <td></td>
        <td> <samp>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<sub>&nbsp;</sub>&nbsp; &nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<sub>&nbsp;</sub>&nbsp; &nbsp;&nbsp;&nbsp;&nbsp;<u>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;</u></samp><br> <samp>MEM.D<sub>1</sub>, MEM.D<sub>2</sub>, MEM.LATCH</samp> </td>
    </tr>
</table>
