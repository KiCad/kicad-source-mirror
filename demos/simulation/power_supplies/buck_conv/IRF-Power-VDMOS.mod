* VDMOS models

* no. 1
.model IRF9540_1 vdmos pchan VTO=-3.192 RS=0.05098 KP=13.966 RD=0.0985 RG=21.486 mfg=International_Rectifier Vds=-100 CGDMAX=2.00n CGDMIN=2.00E-11 Cjo=5.13E-10 IS=2.39e-27 Rb=0.0447 TT=1.465e-07 Cgs=1.27E-09 Ksubthres=0.1

* no. 2
.model IRF9540_2 vdmos (pchan VTO=-3.192 RS=0.05098 KP=13.966 RD=0.0985 RG=21.486 mfg=International_Rectifier Vds=-100 CGDMAX=2.00n CGDMIN=2.00E-11 Cjo=5.13E-10 IS=2.39e-27 Rb=0.0447 TT=1.465e-07 Cgs=1.27E-09 Ksubthres=0.1)

* no. 3
.model IRF9540_3 vdmos ( pchan VTO=-3.192 RS=0.05098 KP=13.966 RD=0.0985 RG=21.486 mfg=International_Rectifier Vds=-100 CGDMAX=2.00n CGDMIN=2.00E-11 Cjo=5.13E-10 IS=2.39e-27 Rb=0.0447 TT=1.465e-07 Cgs=1.27E-09 Ksubthres=0.1 )

* no. 4
.model IRF540 vdmos VTO=3.542 RS=0.03646 KP=35.149 RD=0.0291 RG=6 mfg=International_Rectifier Vds=100 CGDMAX=2.70n CGDMIN=4.00E-11 Cjo=4.76E-10 IS=1.32p Rb=0.01 TT=2.305e-07 Cgs=1.54E-09 Ksubthres=0.1

* no. 5
.model IRF540N vdmos nchan VTO=3.708 RS=0.03657 KP=83.934 RD=0.0031 RG=15.08 mfg=International_Rectifier Vds=100 CGDMAX=4.00n CGDMIN=8.00E-11 Cjo=1.87E-10 IS=2.84e-15 Rb=0.0014 TT=2.179e-07 Cgs=1.21E-09 Ksubthres=0.1
