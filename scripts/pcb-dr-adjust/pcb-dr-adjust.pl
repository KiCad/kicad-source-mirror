#!/usr/bin/perl -w
# version:0.01
# Script is written by Vladimir Kalyaev. You can copy and modify it, but
# this header should remain unchanged if any part of code is taken. Use 
# it gently AS-IS without any warranties.
# Dedicated to my daughter Jacy.

open FH, "<pcb-dr-adjust.cfg" || die "can't load configuration file pcb-dr-adjust.cfg\n";
$totalBins=0;
print "Type\tLocked\tBin\tXmin\tXmax\tXnew\tYmin\tYmax\tYnew\n";
#$prev=0;
#@bins=();

$inp{"Dr"}[0]  = qr!^(Dr\s+)(\d+)(\s+)(\d+)(\s+.*)$!;
$inp{"ShC"}[0] = qr!^(Sh\s+\"\d+\"\s+C\s+)(\d+)(\s+)(\d+)(\s+.*)$!;
$inp{"ShR"}[0] = qr!^(Sh\s+\"\d+\"\s+R\s+)(\d+)(\s+)(\d+)(\s+.*)$!;
$inp{"ShO"}[0] = qr!^(Sh\s+\"\d+\"\s+O\s+)(\d+)(\s+)(\d+)(\s+.*)$!;
$inp{"Vias"}[0] = qr!^(Po\s+\d+\s+\d+\s+\d+\s+\d+\s+\d+\s+)(\d+)(\s+)(\d+)(.*)$!;
#my ($pref,$curX,$space,$curY,$rest)=($1,$2,$3,$4,$5);

#< Po 3 72000 42500 72000 42500 450 450
#---
#> Po 3 72000 42500 72000 42500 620 450

while (<FH>)
{ 
 s!\s+$!!;
 $line=$_;
 if ($line=~s!.*(Dr)::\s*!!)
 {
  $totalBins=3; $type=$1; $locked="u"; # Dr cant be "locked"!
  while ($line=~m!(\d+),(\d+),(\d+)!g)
  {
   $inp{$type}[$totalBins]=[$1, $2, $3, 0, 0, 0];
   $inp{$type}[1]=$locked;
   print "$type\t$inp{$type}[1]\t$totalBins\t$inp{$type}[$totalBins]->[0]\t$inp{$type}[$totalBins]->[1]\t$inp{$type}[$totalBins]->[2]\n";
   $totalBins++;
  }
   $inp{$type}[2]=$totalBins-1; $inp{$type}[1]=$locked;
 }
 if ($line =~s!.*(Sh.|Vias):(.?):\s*!!)
 {
  $totalBins=3; $type=$1;
  if (defined $2) {$locked = $2;};
  if ($locked =~ m!^\s*$!) {$locked="u";};

  while ($line=~m!(\d+),(\d+),(\d+),(\d+),(\d+),(\d+)!g)
  {
   $inp{$type}[1]=$locked;
   $inp{$type}[$totalBins]=[$1, $2, $3, $4, $5, $6];
   print "$type\t$inp{$type}[1]\t$totalBins\t$inp{$type}[$totalBins]->[0]\t$inp{$type}[$totalBins]->[1]\t$inp{$type}[$totalBins]->[2]\t$inp{$type}[$totalBins]->[3]\t$inp{$type}[$totalBins]->[4]\t$inp{$type}[$totalBins]->[5]\n";
   $totalBins++;
  }
  $inp{$type}[2]=$totalBins-1; $inp{$type}[1]=$locked;
 }
 if ($line =~ s!infile:\s*(.+?)\s*$!!)
 {
  $infile=$1;
 }
 if ($line =~ s!outfile:\s*(.+?)\s*$!!)
 {
  $outfile=$1;
 }
}
#Sh "1" C 1600 1600 0 0 0
#Sh "1" R 550 550 0 0 900
#R/C/O
#Dr 1200 0 0
if (!defined $infile)  { $infile  = "<example.brd";}     else {$infile="<$infile";};
if (!defined $outfile) { $outfile = ">processed.brd";} else {$outfile=">$outfile";};

open FH, $infile || die "can't read inputfile ==$infile==\n";
open FHout, $outfile || die "can't open outfile ==$outfile==\n";
while (<FH>)
{
#print "before:$_|\n";
 s!\s+$!!;
 #chomp();# chop();
 $curline=$_;
#print "after :$_|\n";
 foreach $key (sort keys %inp)
 {
#   print "proc key:$key\n";
  if ($_ =~ $inp{$key}[0])
  {
#   print "proc key:$key\n";
#Sh "6" C 550 550 0 0 900
#Sh "6" C ,550, ,550, 0 0 900
#Dr 320 0 0
#Dr ,320, ,0, 0
#$inp{"ShC"}[0] = qr!^(Sh\s+\"\d+\"\s+C\s+)(\d+)(\s+)(\d+)(\s+.*)$!;
   my ($pref,$curX,$space,$curY,$rest)=($1,$2,$3,$4,$5);
#   print "inp{$key}=$inp{$key}[0]\n";
#   print "$curline\n";
#   print "$1,$2,$3,$4,$5\n";
#--------------------------------

#0 - regexp
#1 - locked
#2 - totalBins
#3=>[1,2,3,4,5,6] - bins  
#4=>....
#5=>....

   $index=3;
   $flagX=0; $flagY=0;
   $newX =0; $newY =0;
#   $curline = "bef:$pref,$curX,$space,$curY,$rest. locked=$inp{$key}[1], totalBins=$inp{$key}[2]\n";
#   print $curline;

   while ( ($index < $inp{$key}[2]+1) )
   {
    if (($flagX == 0) && ($curX > $inp{$key}[$index]->[0])  && ($curX <= $inp{$key}[$index]->[1]) )
    {
     $flagX=1; $newX=$inp{$key}[$index]->[2];
     if ($inp{$key}[1] =~ m!l!i) { } else {$curX=$newX;};
    }

    if (($flagY == 0) && ($curY > $inp{$key}[$index]->[3])  && ($curY <= $inp{$key}[$index]->[4]) )
    {
     $flagY=1; $newY=$inp{$key}[$index]->[5];
     if ($inp{$key}[1] =~ m!l!i) { } else {$curY=$newY;};
    }

    if (($inp{$key}[1] =~ m!l!i) && ($flagX =~ m!1!) && ($flagY =~ m!1!))
    {
     # This section means LOCKED = change together only
     $curX=$newX;
     $curY=$newY;
    }
    $index ++;
   }  

#--------------------------------
#   $curline = "aft:$pref,$curX,$space,$curY,$rest.\n";
#   print $curline;
   $curline = "$pref$curX$space$curY$rest";
  }
 }
 print FHout "$curline\n";
}
#$local = "TR|PL|EX|TQ|TA|SN|BA|BS|DT|BH|GL";
#$pcode = qr/^\s*($local)\d\w?\s+\d[A-Z]{2}\s*$/;
#@vcheck = ("SN12 6QL","G3 7XR","GLZ 7PX"," OX11 0EY","NW1 1AD");
#foreach $tp (@vcheck) {
#        $tp =~ $pcode and print "$1\n";
#        }