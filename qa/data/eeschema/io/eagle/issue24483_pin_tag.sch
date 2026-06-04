<?xml version="1.0" encoding="utf-8"?>
<!DOCTYPE eagle SYSTEM "eagle.dtd">
<eagle version="8.2.1">
<drawing>
<settings>
<setting alwaysvectorfont="no"/>
<setting verticaltext="up"/>
</settings>
<grid distance="0.1" unitdist="inch" unit="inch" style="lines" multiple="1" display="no" altdistance="0.01" altunitdist="inch" altunit="inch"/>
<layers>
<layer number="91" name="Nets" color="2" fill="1" visible="yes" active="yes"/>
<layer number="93" name="Pins" color="2" fill="1" visible="no" active="yes"/>
<layer number="94" name="Symbols" color="4" fill="1" visible="yes" active="yes"/>
<layer number="95" name="Names" color="7" fill="1" visible="yes" active="yes"/>
<layer number="96" name="Values" color="7" fill="1" visible="yes" active="yes"/>
</layers>
<schematic xreflabel="%F%N/%S.%C%R" xrefpart="/%S.%C%R">
<libraries>
<library name="tagtest">
<packages>
<package name="DIP4">
<pad name="1" x="-2.54" y="-1.27" drill="0.8"/>
<pad name="2" x="-2.54" y="1.27" drill="0.8"/>
<pad name="3" x="2.54" y="1.27" drill="0.8"/>
<pad name="4" x="2.54" y="-1.27" drill="0.8"/>
</package>
</packages>
<symbols>
<symbol name="MYSYM">
<wire x1="-5.08" y1="5.08" x2="5.08" y2="5.08" width="0.254" layer="94"/>
<wire x1="5.08" y1="5.08" x2="5.08" y2="-7.62" width="0.254" layer="94"/>
<wire x1="5.08" y1="-7.62" x2="-5.08" y2="-7.62" width="0.254" layer="94"/>
<wire x1="-5.08" y1="-7.62" x2="-5.08" y2="5.08" width="0.254" layer="94"/>
<pin name="IN@1" x="-7.62" y="2.54" visible="pin" length="short" direction="in"/>
<pin name="IN@2" x="-7.62" y="0" visible="pin" length="short" direction="in"/>
<pin name="GND" x="-7.62" y="-2.54" visible="pin" length="short" direction="pwr"/>
<pin name="NC@3" x="7.62" y="2.54" visible="pin" length="short" direction="nc" rot="R180"/>
</symbol>
</symbols>
<devicesets>
<deviceset name="MYDEV" prefix="U">
<gates>
<gate name="A" symbol="MYSYM" x="0" y="0"/>
</gates>
<devices>
<device name="" package="DIP4">
<connects>
<connect gate="A" pin="IN@1" pad="1"/>
<connect gate="A" pin="IN@2" pad="2"/>
<connect gate="A" pin="GND" pad="3"/>
<connect gate="A" pin="NC@3" pad="4"/>
</connects>
<technologies>
<technology name=""/>
</technologies>
</device>
</devices>
</deviceset>
<deviceset name="MYDEV_NC" prefix="U">
<gates>
<gate name="A" symbol="MYSYM" x="0" y="0"/>
</gates>
<devices>
<device name="">
<technologies>
<technology name=""/>
</technologies>
</device>
</devices>
</deviceset>
</devicesets>
</library>
</libraries>
<attributes>
</attributes>
<variantdefs>
</variantdefs>
<classes>
<class number="0" name="default" width="0" drill="0">
</class>
</classes>
<parts>
<part name="U1" library="tagtest" deviceset="MYDEV" device=""/>
<part name="U2" library="tagtest" deviceset="MYDEV_NC" device=""/>
</parts>
<sheets>
<sheet>
<plain>
</plain>
<instances>
<instance part="U1" gate="A" x="50.8" y="50.8"/>
<instance part="U2" gate="A" x="101.6" y="50.8"/>
</instances>
<busses>
</busses>
<nets>
</nets>
</sheet>
</sheets>
</schematic>
</drawing>
</eagle>
