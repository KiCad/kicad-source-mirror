#!/usr/bin/env python2


# Author: Dick Hollenbeck
# Report any length problems pertaining to a SDRAM DDR3 T topology using
# 4 memory chips: a T into 2 Ts routing strategy from the CPU.

# Designed to be run from the command line in a process separate from pcbnew.
# It can monitor writes to disk which will trigger updates to its output, or
# it can be run with --once option.


from __future__ import print_function

import pcbnew
import os.path
import sys
import time


CPU_REF = 'U7'      # CPU reference designator

# four SDRAM memory chips:
DDR_LF  = 'U15'     # left front DRAM
DDR_RF  = 'U17'     # right front DRAM
DDR_LB  = 'U16'     # left back DRAM
DDR_RB  = 'U18'     # right back DRAM


# Length of SDRAM clock, it sets the maximum or equal needed for other traces
CLOCK_LEN = pcbnew.FromMils( 2.25 * 1000 )

def addr_line_netname(line_no):
    """From an address line number, return the netname"""
    netname = '/DDR3/DRAM_A' + str(line_no)
    return netname

# Establish GOALS which are LENs, TOLERANCEs and NETs for each group of nets.

# Net Group: ADDR_AND_CMD
ADDR_AND_CMD_LEN = pcbnew.FromMils( 2.22 * 1000 )
ADDR_AND_CMD_TOLERANCE = pcbnew.FromMils( 25 ) / 2
ADDR_AND_CMD_NETS = [addr_line_netname(a) for a in range(0,16)]
ADDR_AND_CMD_NETS += [
    '/DDR3/DRAM_SDBA0',
    '/DDR3/DRAM_SDBA1',
    '/DDR3/DRAM_SDBA2',
    '/DDR3/DRAM_RAS_B',
    '/DDR3/DRAM_CAS_B',
    '/DDR3/DRAM_WE_B'
    ]


# Net Group: CONTROL
CONTROL_LEN = pcbnew.FromMils( 2.10 * 1000 )
CONTROL_TOLERANCE = pcbnew.FromMils( 50 ) / 2
CONTROL_NETS = [
    '/DDR3/DRAM_SDODT0',
    #'/DDR3/DRAM_SDODT1',
    '/DDR3/DRAM_CS0_B',
    #'/DDR3/DRAM_CS1_B',
    '/DDR3/DRAM_SDCKE0',
    #'/DDR3/DRAM_SDCKE1',
    ]



BRIGHTGREEN = '\033[92;1m'
GREEN = '\033[92m'
BRIGHTRED = '\033[91;1m'
RED = '\033[91m'
ENDC = '\033[0m'


pcb = None
nets = None

dbg_conn = False        # when true prints out reason for track discontinuity

def print_color(color, s):
    print(color + s + ENDC)


def addr_line_netname(line_no):
    netname = '/DDR3/DRAM_A' + str(line_no)
    return netname


def pad_name(pad):
    return str( pad.GetParent().Reference().GetShownText() ) + '/' + pad.GetPadName()


def pad_pos(pad):
    return str(pad.GetPosition())


def pads_in_net(netname):
    byname = {}
    pads = nets[netname].Pads()
    for pad in pads:
        byname[pad_name(pad)] = pad
    return byname


def track_ends(track):
    """return a string showing both ends of a track"""
    return str(track.GetStart()) + ' ' + str(track.GetEnd())


def print_tracks(net_name,tracks):
    print('net:', net_name)
    for track in tracks:
        print(' track:', track_ends(track))


def sum_track_lengths(point1,point2,netcode):
    tracks = pcb.TracksInNetBetweenPoints(point1, point2, netcode)
    sum = 0
    for t in tracks:
        sum += t.GetLength()
    return sum


def tracks_in_net(netname):
    nc = pcb.GetNetcodeFromNetname(netname)
    tracks_and_vias = pcb.TracksInNet(nc)
    # remove vias while making new non-owning list
    tracks = [t for t in tracks_and_vias if not t.Type() == pcbnew.PCB_VIA_T]
    return tracks


def print_pad(pad):
    print( " pad name:'%s' pos:%s" % ( pad_name(pad), pad_pos(pad) ) )


def print_pads(prompt,pads):
    print(prompt)
    for pad in pads:
        print_pad(pad)


def is_connected(start_pad, end_pad):
    """
    Return True if the two pads are copper connected only with vias and tracks
    directly and with no intervening pads, else False.
    """
    netcode = start_pad.GetNet().GetNet()
    try:
        tracks = pcb.TracksInNetBetweenPoints(start_pad.GetPosition(), end_pad.GetPosition(), netcode)
    except IOError as ioe:
        if dbg_conn:        # can be True when wanting details on discontinuity
            print(ioe)
        return False
    return True


def find_connected_pad(start_pad, pads):
    for p in pads:
        if p == start_pad:
            continue
        if is_connected(start_pad,p):
            return p
    raise IOError( 'no connection to pad %s' % pad_name(start_pad) )


def find_cpu_pad(pads):
    for p in pads:
        if CPU_REF in pad_name(p):
            return p
    raise IOError( 'no cpu pad' )


def report_teed_lengths(groupname, netname, target_length, tolerance):
    global dbg_conn

    print(groupname, netname)
    nc = pcb.GetNetcodeFromNetname(netname)
    #print("nc", nc)

    pads = nets[netname].Pads()

    # convert from std::vector<> to python list
    pads = list(pads)
    #print_pads(netname, pads )

    cpu_pad = find_cpu_pad(pads)
    pads.remove(cpu_pad)

    # a trap for a troublesome net that appears to be disconnected or has stray segments.
    if netname == None:
    #if netname == '/DDR3/DRAM_SDCKE0':
        dbg_conn = True

    # find the first T
    #print_pads(netname + ' without cpu pad', pads )
    t1 = find_connected_pad(cpu_pad, pads)
    pads.remove(t1)

    # find 2 second tier T pads
    t2_1 = find_connected_pad(t1, pads)
    pads.remove(t2_1)

    t2_2 = find_connected_pad(t1, pads)
    pads.remove(t2_2)

    cpad = [0] * 4

    # find 4 memory pads off of each 2nd tier T
    cpad[0] = find_connected_pad(t2_1, pads)
    pads.remove(cpad[0])

    cpad[1] = find_connected_pad(t2_1, pads)
    pads.remove(cpad[1])

    cpad[2] = find_connected_pad(t2_2, pads)
    pads.remove(cpad[2])

    cpad[3] = find_connected_pad(t2_2, pads)
    pads.remove(cpad[3])

    len_t1   = sum_track_lengths(cpu_pad.GetPosition(),t1.GetPosition(),nc)
    #print("len_t1 %.0f" % len_t1)

    len_t2_1 = sum_track_lengths(t1.GetPosition(),t2_1.GetPosition(),nc)
    len_t2_2 = sum_track_lengths(t1.GetPosition(),t2_2.GetPosition(),nc)
    #print("len_t2_1 %.0f" % len_t2_1)
    #print("len_t2_2 %.0f" % len_t2_2)

    lens = [0] * 4

    lens[0] = sum_track_lengths(t2_1.GetPosition(),cpad[0].GetPosition(),nc)
    lens[1] = sum_track_lengths(t2_1.GetPosition(),cpad[1].GetPosition(),nc)
    lens[2] = sum_track_lengths(t2_2.GetPosition(),cpad[2].GetPosition(),nc)
    lens[3] = sum_track_lengths(t2_2.GetPosition(),cpad[3].GetPosition(),nc)

    """
    for index, total_len in enumerate(lens):
        print( "%s: %.0f" % (pad_name(cpad[index]), lens[index]))
    """

    # Each net goes from CPU to four memory chips, these are the 4 lengths from
    # CPU to each of the for memory chip balls/pads, some of these journies are
    # common with one another but branch off at each T.
    lens[0] += len_t1 + len_t2_1
    lens[1] += len_t1 + len_t2_1
    lens[2] += len_t1 + len_t2_2
    lens[3] += len_t1 + len_t2_2

    for index, total_len in enumerate(lens):
        delta = total_len - target_length
        if delta > tolerance:
            print_color( BRIGHTRED, "%s %s len:%.0f long by %.0f mils" %
                  (netname, pad_name(cpad[index]), pcbnew.ToMils(total_len), pcbnew.ToMils(delta - tolerance)  ))
        elif delta < -tolerance:
            print_color( BRIGHTRED, "%s %s len:%.0f short by %.0f mils" %
                  (netname, pad_name(cpad[index]), pcbnew.ToMils(total_len), pcbnew.ToMils(tolerance - delta) ))



def load_board_and_report_lengths(filename):

    global pcb
    pcb = pcbnew.LoadBoard(filename)
    pcb.BuildListOfNets()               # required so 'pcb' contains valid netclass data

    global nets
    nets = pcb.GetNetsByName()

    for netname in ADDR_AND_CMD_NETS:
        report_teed_lengths("addr_and_cmd", netname, ADDR_AND_CMD_LEN, ADDR_AND_CMD_TOLERANCE)

    for netname in CONTROL_NETS:
        report_teed_lengths("control", netname, CONTROL_LEN, CONTROL_TOLERANCE)



if __name__ == "__main__":
    try:
        boardfile = sys.argv[1]
    except IndexError:
        print("Usage: %s <boardname.kicad_pcb> [--once]" % sys.argv[0])
        sys.exit(1)

    first = True

    while True:

        # wait for the file contents to change
        lastmtime = os.path.getmtime(boardfile)
        mtime = lastmtime
        while mtime == lastmtime and not first:
            try:
                mtime = os.path.getmtime(boardfile)
            except OSError:
                pass # kicad save process seems to momentarily delete file, so there's a race here with "No such file.."
            time.sleep(0.5)

        # The "Debug" build of pcbnew writes to disk slowy, new file takes time to get to disk.
        time.sleep(1)

        first = False

        print( '\033[2J' )  # clear screen, maybe

        load_board_and_report_lengths(boardfile)

        if "--once" in sys.argv:
            sys.exit(0)
