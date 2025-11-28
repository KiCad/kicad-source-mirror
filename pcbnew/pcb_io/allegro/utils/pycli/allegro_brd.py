# This is a generated file! Please edit source .ksy file and use kaitai-struct-compiler to rebuild

import kaitaistruct
from kaitaistruct import KaitaiStruct, KaitaiStream, BytesIO
from enum import Enum
import collections


if getattr(kaitaistruct, 'API_VERSION', (0, 9)) < (0, 9):
    raise Exception("Incompatible Kaitai Struct Python API: 0.9 or later is required, but you have %s" % (kaitaistruct.__version__))

class AllegroBrd(KaitaiStruct):
    """Cadence Allegro printed circuit board (PCB) board file. This is the native
    binary format saved by the Cadence Allegro PCB editor. Board files (.brd)
    and footprint drawing files (.dra) use the same format.
    
    There is no known public documentation for this format.
    """

    class BoardUnits(Enum):
        imperial = 1
        metric = 3

    class Version(Enum):
        a160 = 1245184
        a162 = 1246208
        a164 = 1248256
        a166 = 1250560
        a172 = 1311744
        a174 = 1313024
        a175 = 1316096
    SEQ_FIELDS = ["magic", "unk1", "object_count", "unk2", "ll_x04", "ll_x06", "ll_x0c_2", "ll_x0e_x28", "ll_x14", "ll_x1b", "ll_x1c", "ll_x24_x28", "ll_unused_1", "ll_x2b", "ll_x03_x30", "ll_x0a", "ll_x1d_x1e_x1f", "ll_unused_2", "ll_x38", "ll_x2c", "ll_x0c", "ll_unused_3", "x35_start", "x35_end", "ll_x36", "ll_unused_4", "ll_unused_5", "ll_x0a_2", "_unnamed28", "allegro_version", "_unnamed30", "max_key", "_unnamed32", "board_units", "_unnamed34", "_unnamed35", "_unnamed36", "_unnamed37", "x27_end_offset", "_unnamed39", "strings_count", "_unnamed41", "unit_divisor", "_unnamed43", "layer_map", "_unnamed45", "string_map", "objects"]
    def __init__(self, _io, _parent=None, _root=None):
        self._io = _io
        self._parent = _parent
        self._root = _root if _root else self
        self._debug = collections.defaultdict(dict)
        self._read()

    def _read(self):
        self._debug['magic']['start'] = self._io.pos()
        self.magic = self._io.read_u4le()
        self._debug['magic']['end'] = self._io.pos()
        self._debug['unk1']['start'] = self._io.pos()
        self.unk1 = []
        for i in range(4):
            if not 'arr' in self._debug['unk1']:
                self._debug['unk1']['arr'] = []
            self._debug['unk1']['arr'].append({'start': self._io.pos()})
            self.unk1.append(self._io.read_u4le())
            self._debug['unk1']['arr'][i]['end'] = self._io.pos()

        self._debug['unk1']['end'] = self._io.pos()
        self._debug['object_count']['start'] = self._io.pos()
        self.object_count = self._io.read_u4le()
        self._debug['object_count']['end'] = self._io.pos()
        self._debug['unk2']['start'] = self._io.pos()
        self.unk2 = []
        for i in range(9):
            if not 'arr' in self._debug['unk2']:
                self._debug['unk2']['arr'] = []
            self._debug['unk2']['arr'].append({'start': self._io.pos()})
            self.unk2.append(self._io.read_u4le())
            self._debug['unk2']['arr'][i]['end'] = self._io.pos()

        self._debug['unk2']['end'] = self._io.pos()
        self._debug['ll_x04']['start'] = self._io.pos()
        self.ll_x04 = AllegroBrd.LinkedList(self._io, self, self._root)
        self._debug['ll_x04']['end'] = self._io.pos()
        self._debug['ll_x06']['start'] = self._io.pos()
        self.ll_x06 = AllegroBrd.LinkedList(self._io, self, self._root)
        self._debug['ll_x06']['end'] = self._io.pos()
        self._debug['ll_x0c_2']['start'] = self._io.pos()
        self.ll_x0c_2 = AllegroBrd.LinkedList(self._io, self, self._root)
        self._debug['ll_x0c_2']['end'] = self._io.pos()
        self._debug['ll_x0e_x28']['start'] = self._io.pos()
        self.ll_x0e_x28 = AllegroBrd.LinkedList(self._io, self, self._root)
        self._debug['ll_x0e_x28']['end'] = self._io.pos()
        self._debug['ll_x14']['start'] = self._io.pos()
        self.ll_x14 = AllegroBrd.LinkedList(self._io, self, self._root)
        self._debug['ll_x14']['end'] = self._io.pos()
        self._debug['ll_x1b']['start'] = self._io.pos()
        self.ll_x1b = AllegroBrd.LinkedList(self._io, self, self._root)
        self._debug['ll_x1b']['end'] = self._io.pos()
        self._debug['ll_x1c']['start'] = self._io.pos()
        self.ll_x1c = AllegroBrd.LinkedList(self._io, self, self._root)
        self._debug['ll_x1c']['end'] = self._io.pos()
        self._debug['ll_x24_x28']['start'] = self._io.pos()
        self.ll_x24_x28 = AllegroBrd.LinkedList(self._io, self, self._root)
        self._debug['ll_x24_x28']['end'] = self._io.pos()
        self._debug['ll_unused_1']['start'] = self._io.pos()
        self.ll_unused_1 = AllegroBrd.LinkedList(self._io, self, self._root)
        self._debug['ll_unused_1']['end'] = self._io.pos()
        self._debug['ll_x2b']['start'] = self._io.pos()
        self.ll_x2b = AllegroBrd.LinkedList(self._io, self, self._root)
        self._debug['ll_x2b']['end'] = self._io.pos()
        self._debug['ll_x03_x30']['start'] = self._io.pos()
        self.ll_x03_x30 = AllegroBrd.LinkedList(self._io, self, self._root)
        self._debug['ll_x03_x30']['end'] = self._io.pos()
        self._debug['ll_x0a']['start'] = self._io.pos()
        self.ll_x0a = AllegroBrd.LinkedList(self._io, self, self._root)
        self._debug['ll_x0a']['end'] = self._io.pos()
        self._debug['ll_x1d_x1e_x1f']['start'] = self._io.pos()
        self.ll_x1d_x1e_x1f = AllegroBrd.LinkedList(self._io, self, self._root)
        self._debug['ll_x1d_x1e_x1f']['end'] = self._io.pos()
        self._debug['ll_unused_2']['start'] = self._io.pos()
        self.ll_unused_2 = AllegroBrd.LinkedList(self._io, self, self._root)
        self._debug['ll_unused_2']['end'] = self._io.pos()
        self._debug['ll_x38']['start'] = self._io.pos()
        self.ll_x38 = AllegroBrd.LinkedList(self._io, self, self._root)
        self._debug['ll_x38']['end'] = self._io.pos()
        self._debug['ll_x2c']['start'] = self._io.pos()
        self.ll_x2c = AllegroBrd.LinkedList(self._io, self, self._root)
        self._debug['ll_x2c']['end'] = self._io.pos()
        self._debug['ll_x0c']['start'] = self._io.pos()
        self.ll_x0c = AllegroBrd.LinkedList(self._io, self, self._root)
        self._debug['ll_x0c']['end'] = self._io.pos()
        self._debug['ll_unused_3']['start'] = self._io.pos()
        self.ll_unused_3 = AllegroBrd.LinkedList(self._io, self, self._root)
        self._debug['ll_unused_3']['end'] = self._io.pos()
        self._debug['x35_start']['start'] = self._io.pos()
        self.x35_start = self._io.read_u4le()
        self._debug['x35_start']['end'] = self._io.pos()
        self._debug['x35_end']['start'] = self._io.pos()
        self.x35_end = self._io.read_u4le()
        self._debug['x35_end']['end'] = self._io.pos()
        self._debug['ll_x36']['start'] = self._io.pos()
        self.ll_x36 = AllegroBrd.LinkedList(self._io, self, self._root)
        self._debug['ll_x36']['end'] = self._io.pos()
        self._debug['ll_unused_4']['start'] = self._io.pos()
        self.ll_unused_4 = AllegroBrd.LinkedList(self._io, self, self._root)
        self._debug['ll_unused_4']['end'] = self._io.pos()
        self._debug['ll_unused_5']['start'] = self._io.pos()
        self.ll_unused_5 = AllegroBrd.LinkedList(self._io, self, self._root)
        self._debug['ll_unused_5']['end'] = self._io.pos()
        self._debug['ll_x0a_2']['start'] = self._io.pos()
        self.ll_x0a_2 = AllegroBrd.LinkedList(self._io, self, self._root)
        self._debug['ll_x0a_2']['end'] = self._io.pos()
        self._debug['_unnamed28']['start'] = self._io.pos()
        self._unnamed28 = self._io.read_u4le()
        self._debug['_unnamed28']['end'] = self._io.pos()
        self._debug['allegro_version']['start'] = self._io.pos()
        self.allegro_version = (self._io.read_bytes(60)).decode(u"ASCII")
        self._debug['allegro_version']['end'] = self._io.pos()
        self._debug['_unnamed30']['start'] = self._io.pos()
        self._unnamed30 = self._io.read_u4le()
        self._debug['_unnamed30']['end'] = self._io.pos()
        self._debug['max_key']['start'] = self._io.pos()
        self.max_key = self._io.read_u4le()
        self._debug['max_key']['end'] = self._io.pos()
        self._debug['_unnamed32']['start'] = self._io.pos()
        self._unnamed32 = []
        for i in range(17):
            if not 'arr' in self._debug['_unnamed32']:
                self._debug['_unnamed32']['arr'] = []
            self._debug['_unnamed32']['arr'].append({'start': self._io.pos()})
            self._unnamed32.append(self._io.read_u4le())
            self._debug['_unnamed32']['arr'][i]['end'] = self._io.pos()

        self._debug['_unnamed32']['end'] = self._io.pos()
        self._debug['board_units']['start'] = self._io.pos()
        self.board_units = KaitaiStream.resolve_enum(AllegroBrd.BoardUnits, self._io.read_u1())
        self._debug['board_units']['end'] = self._io.pos()
        self._debug['_unnamed34']['start'] = self._io.pos()
        self._unnamed34 = self._io.read_u1()
        self._debug['_unnamed34']['end'] = self._io.pos()
        self._debug['_unnamed35']['start'] = self._io.pos()
        self._unnamed35 = self._io.read_u2le()
        self._debug['_unnamed35']['end'] = self._io.pos()
        self._debug['_unnamed36']['start'] = self._io.pos()
        self._unnamed36 = self._io.read_u4le()
        self._debug['_unnamed36']['end'] = self._io.pos()
        self._debug['_unnamed37']['start'] = self._io.pos()
        self._unnamed37 = self._io.read_u4le()
        self._debug['_unnamed37']['end'] = self._io.pos()
        self._debug['x27_end_offset']['start'] = self._io.pos()
        self.x27_end_offset = self._io.read_u4le()
        self._debug['x27_end_offset']['end'] = self._io.pos()
        self._debug['_unnamed39']['start'] = self._io.pos()
        self._unnamed39 = self._io.read_u4le()
        self._debug['_unnamed39']['end'] = self._io.pos()
        self._debug['strings_count']['start'] = self._io.pos()
        self.strings_count = self._io.read_u4le()
        self._debug['strings_count']['end'] = self._io.pos()
        self._debug['_unnamed41']['start'] = self._io.pos()
        self._unnamed41 = []
        for i in range(53):
            if not 'arr' in self._debug['_unnamed41']:
                self._debug['_unnamed41']['arr'] = []
            self._debug['_unnamed41']['arr'].append({'start': self._io.pos()})
            self._unnamed41.append(self._io.read_u4le())
            self._debug['_unnamed41']['arr'][i]['end'] = self._io.pos()

        self._debug['_unnamed41']['end'] = self._io.pos()
        self._debug['unit_divisor']['start'] = self._io.pos()
        self.unit_divisor = self._io.read_u4le()
        self._debug['unit_divisor']['end'] = self._io.pos()
        self._debug['_unnamed43']['start'] = self._io.pos()
        self._unnamed43 = []
        for i in range(110):
            if not 'arr' in self._debug['_unnamed43']:
                self._debug['_unnamed43']['arr'] = []
            self._debug['_unnamed43']['arr'].append({'start': self._io.pos()})
            self._unnamed43.append(self._io.read_u4le())
            self._debug['_unnamed43']['arr'][i]['end'] = self._io.pos()

        self._debug['_unnamed43']['end'] = self._io.pos()
        self._debug['layer_map']['start'] = self._io.pos()
        self.layer_map = AllegroBrd.LayerMap(self._io, self, self._root)
        self._debug['layer_map']['end'] = self._io.pos()
        self._debug['_unnamed45']['start'] = self._io.pos()
        self._unnamed45 = []
        i = 0
        while True:
            if not 'arr' in self._debug['_unnamed45']:
                self._debug['_unnamed45']['arr'] = []
            self._debug['_unnamed45']['arr'].append({'start': self._io.pos()})
            _ = self._io.read_u4le()
            self._unnamed45.append(_)
            self._debug['_unnamed45']['arr'][len(self._unnamed45) - 1]['end'] = self._io.pos()
            if self._io.pos() == 4608:
                break
            i += 1
        self._debug['_unnamed45']['end'] = self._io.pos()
        self._debug['string_map']['start'] = self._io.pos()
        self.string_map = AllegroBrd.StringMap(self.strings_count, self._io, self, self._root)
        self._debug['string_map']['end'] = self._io.pos()
        self._debug['objects']['start'] = self._io.pos()
        self.objects = []
        i = 0
        while not self._io.is_eof():
            if not 'arr' in self._debug['objects']:
                self._debug['objects']['arr'] = []
            self._debug['objects']['arr'].append({'start': self._io.pos()})
            self.objects.append(AllegroBrd.BoardObject(self._io, self, self._root))
            self._debug['objects']['arr'][len(self.objects) - 1]['end'] = self._io.pos()
            i += 1

        self._debug['objects']['end'] = self._io.pos()

    class Type1d(KaitaiStruct):
        SEQ_FIELDS = ["_unnamed0", "_unnamed1", "key", "_unnamed3", "size_a", "size_b", "_unnamed6", "_unnamed7", "_unnamed8"]
        def __init__(self, _io, _parent=None, _root=None):
            self._io = _io
            self._parent = _parent
            self._root = _root if _root else self
            self._debug = collections.defaultdict(dict)
            self._read()

        def _read(self):
            self._debug['_unnamed0']['start'] = self._io.pos()
            self._unnamed0 = self._io.read_u1()
            self._debug['_unnamed0']['end'] = self._io.pos()
            self._debug['_unnamed1']['start'] = self._io.pos()
            self._unnamed1 = self._io.read_u2le()
            self._debug['_unnamed1']['end'] = self._io.pos()
            self._debug['key']['start'] = self._io.pos()
            self.key = self._io.read_u4le()
            self._debug['key']['end'] = self._io.pos()
            self._debug['_unnamed3']['start'] = self._io.pos()
            self._unnamed3 = []
            for i in range(3):
                if not 'arr' in self._debug['_unnamed3']:
                    self._debug['_unnamed3']['arr'] = []
                self._debug['_unnamed3']['arr'].append({'start': self._io.pos()})
                self._unnamed3.append(self._io.read_u4le())
                self._debug['_unnamed3']['arr'][i]['end'] = self._io.pos()

            self._debug['_unnamed3']['end'] = self._io.pos()
            self._debug['size_a']['start'] = self._io.pos()
            self.size_a = self._io.read_u2le()
            self._debug['size_a']['end'] = self._io.pos()
            self._debug['size_b']['start'] = self._io.pos()
            self.size_b = self._io.read_u2le()
            self._debug['size_b']['end'] = self._io.pos()
            self._debug['_unnamed6']['start'] = self._io.pos()
            self._unnamed6 = []
            for i in range((self.size_b * 56)):
                if not 'arr' in self._debug['_unnamed6']:
                    self._debug['_unnamed6']['arr'] = []
                self._debug['_unnamed6']['arr'].append({'start': self._io.pos()})
                self._unnamed6.append(self._io.read_u1())
                self._debug['_unnamed6']['arr'][i]['end'] = self._io.pos()

            self._debug['_unnamed6']['end'] = self._io.pos()
            self._debug['_unnamed7']['start'] = self._io.pos()
            self._unnamed7 = []
            for i in range((self.size_a * 256)):
                if not 'arr' in self._debug['_unnamed7']:
                    self._debug['_unnamed7']['arr'] = []
                self._debug['_unnamed7']['arr'].append({'start': self._io.pos()})
                self._unnamed7.append(self._io.read_u1())
                self._debug['_unnamed7']['arr'][i]['end'] = self._io.pos()

            self._debug['_unnamed7']['end'] = self._io.pos()
            if self._root.ver >= 1311744:
                self._debug['_unnamed8']['start'] = self._io.pos()
                self._unnamed8 = self._io.read_u4le()
                self._debug['_unnamed8']['end'] = self._io.pos()



    class Type37(KaitaiStruct):
        """Seems to be the object that collects entries for a group
        (e.g. table or xsection charts have one of these)
        """
        SEQ_FIELDS = ["t", "t2", "key", "group_ptr", "unknown_1", "capacity", "count", "unknown_2", "ptrs", "unknown_3"]
        def __init__(self, _io, _parent=None, _root=None):
            self._io = _io
            self._parent = _parent
            self._root = _root if _root else self
            self._debug = collections.defaultdict(dict)
            self._read()

        def _read(self):
            self._debug['t']['start'] = self._io.pos()
            self.t = self._io.read_u1()
            self._debug['t']['end'] = self._io.pos()
            self._debug['t2']['start'] = self._io.pos()
            self.t2 = self._io.read_u2le()
            self._debug['t2']['end'] = self._io.pos()
            self._debug['key']['start'] = self._io.pos()
            self.key = self._io.read_u4le()
            self._debug['key']['end'] = self._io.pos()
            self._debug['group_ptr']['start'] = self._io.pos()
            self.group_ptr = self._io.read_u4le()
            self._debug['group_ptr']['end'] = self._io.pos()
            self._debug['unknown_1']['start'] = self._io.pos()
            self.unknown_1 = self._io.read_u4le()
            self._debug['unknown_1']['end'] = self._io.pos()
            self._debug['capacity']['start'] = self._io.pos()
            self.capacity = self._io.read_u4le()
            self._debug['capacity']['end'] = self._io.pos()
            self._debug['count']['start'] = self._io.pos()
            self.count = self._io.read_u4le()
            self._debug['count']['end'] = self._io.pos()
            self._debug['unknown_2']['start'] = self._io.pos()
            self.unknown_2 = self._io.read_u4le()
            self._debug['unknown_2']['end'] = self._io.pos()
            self._debug['ptrs']['start'] = self._io.pos()
            self.ptrs = []
            for i in range(self.capacity):
                if not 'arr' in self._debug['ptrs']:
                    self._debug['ptrs']['arr'] = []
                self._debug['ptrs']['arr'].append({'start': self._io.pos()})
                self.ptrs.append(self._io.read_u4le())
                self._debug['ptrs']['arr'][i]['end'] = self._io.pos()

            self._debug['ptrs']['end'] = self._io.pos()
            if self._root.ver >= 1313024:
                self._debug['unknown_3']['start'] = self._io.pos()
                self.unknown_3 = self._io.read_u4le()
                self._debug['unknown_3']['end'] = self._io.pos()



    class Type32PlacedPad(KaitaiStruct):
        SEQ_FIELDS = ["type", "layer", "key", "next", "net_ptr", "flags", "prev", "next_in_fp", "parent_fp", "track", "pad_ptr", "ptr3", "ratline", "ptr5", "fp_inst", "unknown_1", "pad_name", "ptr6", "bbox_0", "bbox_1"]
        def __init__(self, _io, _parent=None, _root=None):
            self._io = _io
            self._parent = _parent
            self._root = _root if _root else self
            self._debug = collections.defaultdict(dict)
            self._read()

        def _read(self):
            self._debug['type']['start'] = self._io.pos()
            self.type = self._io.read_u1()
            self._debug['type']['end'] = self._io.pos()
            self._debug['layer']['start'] = self._io.pos()
            self.layer = AllegroBrd.LayerInfo(self._io, self, self._root)
            self._debug['layer']['end'] = self._io.pos()
            self._debug['key']['start'] = self._io.pos()
            self.key = self._io.read_u4le()
            self._debug['key']['end'] = self._io.pos()
            self._debug['next']['start'] = self._io.pos()
            self.next = self._io.read_u4le()
            self._debug['next']['end'] = self._io.pos()
            self._debug['net_ptr']['start'] = self._io.pos()
            self.net_ptr = self._io.read_u4le()
            self._debug['net_ptr']['end'] = self._io.pos()
            self._debug['flags']['start'] = self._io.pos()
            self.flags = self._io.read_bits_int_be(32)
            self._debug['flags']['end'] = self._io.pos()
            self._io.align_to_byte()
            if self._root.ver >= 1311744:
                self._debug['prev']['start'] = self._io.pos()
                self.prev = self._io.read_u4le()
                self._debug['prev']['end'] = self._io.pos()

            self._debug['next_in_fp']['start'] = self._io.pos()
            self.next_in_fp = self._io.read_u4le()
            self._debug['next_in_fp']['end'] = self._io.pos()
            self._debug['parent_fp']['start'] = self._io.pos()
            self.parent_fp = self._io.read_u4le()
            self._debug['parent_fp']['end'] = self._io.pos()
            self._debug['track']['start'] = self._io.pos()
            self.track = self._io.read_u4le()
            self._debug['track']['end'] = self._io.pos()
            self._debug['pad_ptr']['start'] = self._io.pos()
            self.pad_ptr = self._io.read_u4le()
            self._debug['pad_ptr']['end'] = self._io.pos()
            self._debug['ptr3']['start'] = self._io.pos()
            self.ptr3 = self._io.read_u4le()
            self._debug['ptr3']['end'] = self._io.pos()
            self._debug['ratline']['start'] = self._io.pos()
            self.ratline = self._io.read_u4le()
            self._debug['ratline']['end'] = self._io.pos()
            self._debug['ptr5']['start'] = self._io.pos()
            self.ptr5 = self._io.read_u4le()
            self._debug['ptr5']['end'] = self._io.pos()
            self._debug['fp_inst']['start'] = self._io.pos()
            self.fp_inst = self._io.read_u4le()
            self._debug['fp_inst']['end'] = self._io.pos()
            if self._root.ver >= 1311744:
                self._debug['unknown_1']['start'] = self._io.pos()
                self.unknown_1 = self._io.read_u4le()
                self._debug['unknown_1']['end'] = self._io.pos()

            self._debug['pad_name']['start'] = self._io.pos()
            self.pad_name = self._io.read_u4le()
            self._debug['pad_name']['end'] = self._io.pos()
            self._debug['ptr6']['start'] = self._io.pos()
            self.ptr6 = self._io.read_u4le()
            self._debug['ptr6']['end'] = self._io.pos()
            self._debug['bbox_0']['start'] = self._io.pos()
            self.bbox_0 = AllegroBrd.Coords(self._io, self, self._root)
            self._debug['bbox_0']['end'] = self._io.pos()
            self._debug['bbox_1']['start'] = self._io.pos()
            self.bbox_1 = AllegroBrd.Coords(self._io, self, self._root)
            self._debug['bbox_1']['end'] = self._io.pos()


    class Coords(KaitaiStruct):
        SEQ_FIELDS = ["x", "y"]
        def __init__(self, _io, _parent=None, _root=None):
            self._io = _io
            self._parent = _parent
            self._root = _root if _root else self
            self._debug = collections.defaultdict(dict)
            self._read()

        def _read(self):
            self._debug['x']['start'] = self._io.pos()
            self.x = self._io.read_s4le()
            self._debug['x']['end'] = self._io.pos()
            self._debug['y']['start'] = self._io.pos()
            self.y = self._io.read_s4le()
            self._debug['y']['end'] = self._io.pos()


    class Type08(KaitaiStruct):
        """Seems to be a counterpart to 0x11 - the counts are the same and they have 1:1
        pointers to each other.
        
        They form some kind of chain with interlinks
        
            0x11 -> 0x11 -> 0x11 <-> 0x0F
              ^       ^       ^       ^
              v       v       v       v
            0x08 -> 0x08 -> 0x08 <-> 0x06
        
        From 17.2, there is a backlink too.
        """
        SEQ_FIELDS = ["_unnamed0", "_unnamed1", "key", "prev_ptr", "str_ptr_16x", "next_ptr", "str_ptr", "ptr3", "unknown_1", "ptr4"]
        def __init__(self, _io, _parent=None, _root=None):
            self._io = _io
            self._parent = _parent
            self._root = _root if _root else self
            self._debug = collections.defaultdict(dict)
            self._read()

        def _read(self):
            self._debug['_unnamed0']['start'] = self._io.pos()
            self._unnamed0 = self._io.read_u1()
            self._debug['_unnamed0']['end'] = self._io.pos()
            self._debug['_unnamed1']['start'] = self._io.pos()
            self._unnamed1 = self._io.read_u2le()
            self._debug['_unnamed1']['end'] = self._io.pos()
            self._debug['key']['start'] = self._io.pos()
            self.key = self._io.read_u4le()
            self._debug['key']['end'] = self._io.pos()
            if self._root.ver >= 1311744:
                self._debug['prev_ptr']['start'] = self._io.pos()
                self.prev_ptr = self._io.read_u4le()
                self._debug['prev_ptr']['end'] = self._io.pos()

            if self._root.ver < 1311744:
                self._debug['str_ptr_16x']['start'] = self._io.pos()
                self.str_ptr_16x = self._io.read_u4le()
                self._debug['str_ptr_16x']['end'] = self._io.pos()

            self._debug['next_ptr']['start'] = self._io.pos()
            self.next_ptr = self._io.read_u4le()
            self._debug['next_ptr']['end'] = self._io.pos()
            if self._root.ver >= 1311744:
                self._debug['str_ptr']['start'] = self._io.pos()
                self.str_ptr = self._io.read_u4le()
                self._debug['str_ptr']['end'] = self._io.pos()

            self._debug['ptr3']['start'] = self._io.pos()
            self.ptr3 = self._io.read_u4le()
            self._debug['ptr3']['end'] = self._io.pos()
            if self._root.ver >= 1311744:
                self._debug['unknown_1']['start'] = self._io.pos()
                self.unknown_1 = self._io.read_u4le()
                self._debug['unknown_1']['end'] = self._io.pos()

            self._debug['ptr4']['start'] = self._io.pos()
            self.ptr4 = self._io.read_u4le()
            self._debug['ptr4']['end'] = self._io.pos()


    class Type0e(KaitaiStruct):
        SEQ_FIELDS = ["t", "t2", "key", "next", "fp_ptr", "un2", "un1", "coords", "_unnamed8"]
        def __init__(self, _io, _parent=None, _root=None):
            self._io = _io
            self._parent = _parent
            self._root = _root if _root else self
            self._debug = collections.defaultdict(dict)
            self._read()

        def _read(self):
            self._debug['t']['start'] = self._io.pos()
            self.t = self._io.read_u1()
            self._debug['t']['end'] = self._io.pos()
            self._debug['t2']['start'] = self._io.pos()
            self.t2 = self._io.read_u2le()
            self._debug['t2']['end'] = self._io.pos()
            self._debug['key']['start'] = self._io.pos()
            self.key = self._io.read_u4le()
            self._debug['key']['end'] = self._io.pos()
            self._debug['next']['start'] = self._io.pos()
            self.next = self._io.read_u4le()
            self._debug['next']['end'] = self._io.pos()
            self._debug['fp_ptr']['start'] = self._io.pos()
            self.fp_ptr = self._io.read_u4le()
            self._debug['fp_ptr']['end'] = self._io.pos()
            self._debug['un2']['start'] = self._io.pos()
            self.un2 = []
            for i in range(3):
                if not 'arr' in self._debug['un2']:
                    self._debug['un2']['arr'] = []
                self._debug['un2']['arr'].append({'start': self._io.pos()})
                self.un2.append(self._io.read_u4le())
                self._debug['un2']['arr'][i]['end'] = self._io.pos()

            self._debug['un2']['end'] = self._io.pos()
            if self._root.ver >= 1311744:
                self._debug['un1']['start'] = self._io.pos()
                self.un1 = []
                for i in range(2):
                    if not 'arr' in self._debug['un1']:
                        self._debug['un1']['arr'] = []
                    self._debug['un1']['arr'].append({'start': self._io.pos()})
                    self.un1.append(self._io.read_u4le())
                    self._debug['un1']['arr'][i]['end'] = self._io.pos()

                self._debug['un1']['end'] = self._io.pos()

            self._debug['coords']['start'] = self._io.pos()
            self.coords = []
            for i in range(4):
                if not 'arr' in self._debug['coords']:
                    self._debug['coords']['arr'] = []
                self._debug['coords']['arr'].append({'start': self._io.pos()})
                self.coords.append(self._io.read_s4le())
                self._debug['coords']['arr'][i]['end'] = self._io.pos()

            self._debug['coords']['end'] = self._io.pos()
            self._debug['_unnamed8']['start'] = self._io.pos()
            self._unnamed8 = []
            for i in range(4):
                if not 'arr' in self._debug['_unnamed8']:
                    self._debug['_unnamed8']['arr'] = []
                self._debug['_unnamed8']['arr'].append({'start': self._io.pos()})
                self._unnamed8.append(self._io.read_u4le())
                self._debug['_unnamed8']['arr'][i]['end'] = self._io.pos()

            self._debug['_unnamed8']['end'] = self._io.pos()


    class Type0aDrc(KaitaiStruct):
        SEQ_FIELDS = ["t", "layer", "key", "next", "un1", "un2", "coords", "un4", "un5", "un3"]
        def __init__(self, _io, _parent=None, _root=None):
            self._io = _io
            self._parent = _parent
            self._root = _root if _root else self
            self._debug = collections.defaultdict(dict)
            self._read()

        def _read(self):
            self._debug['t']['start'] = self._io.pos()
            self.t = self._io.read_u1()
            self._debug['t']['end'] = self._io.pos()
            self._debug['layer']['start'] = self._io.pos()
            self.layer = AllegroBrd.LayerInfo(self._io, self, self._root)
            self._debug['layer']['end'] = self._io.pos()
            self._debug['key']['start'] = self._io.pos()
            self.key = self._io.read_u4le()
            self._debug['key']['end'] = self._io.pos()
            self._debug['next']['start'] = self._io.pos()
            self.next = self._io.read_u4le()
            self._debug['next']['end'] = self._io.pos()
            self._debug['un1']['start'] = self._io.pos()
            self.un1 = self._io.read_u4le()
            self._debug['un1']['end'] = self._io.pos()
            if self._root.ver >= 1311744:
                self._debug['un2']['start'] = self._io.pos()
                self.un2 = self._io.read_u4le()
                self._debug['un2']['end'] = self._io.pos()

            self._debug['coords']['start'] = self._io.pos()
            self.coords = []
            for i in range(4):
                if not 'arr' in self._debug['coords']:
                    self._debug['coords']['arr'] = []
                self._debug['coords']['arr'].append({'start': self._io.pos()})
                self.coords.append(self._io.read_s4le())
                self._debug['coords']['arr'][i]['end'] = self._io.pos()

            self._debug['coords']['end'] = self._io.pos()
            self._debug['un4']['start'] = self._io.pos()
            self.un4 = []
            for i in range(4):
                if not 'arr' in self._debug['un4']:
                    self._debug['un4']['arr'] = []
                self._debug['un4']['arr'].append({'start': self._io.pos()})
                self.un4.append(self._io.read_u4le())
                self._debug['un4']['arr'][i]['end'] = self._io.pos()

            self._debug['un4']['end'] = self._io.pos()
            self._debug['un5']['start'] = self._io.pos()
            self.un5 = []
            for i in range(5):
                if not 'arr' in self._debug['un5']:
                    self._debug['un5']['arr'] = []
                self._debug['un5']['arr'].append({'start': self._io.pos()})
                self.un5.append(self._io.read_u4le())
                self._debug['un5']['arr'][i]['end'] = self._io.pos()

            self._debug['un5']['end'] = self._io.pos()
            if self._root.ver >= 1313024:
                self._debug['un3']['start'] = self._io.pos()
                self.un3 = self._io.read_u4le()
                self._debug['un3']['end'] = self._io.pos()



    class Type06(KaitaiStruct):
        SEQ_FIELDS = ["_unnamed0", "_unnamed1", "key", "next", "str", "str_2", "ptr_instance", "ptr_fp", "ptr_x08", "ptr_x03_symbol", "unknown_1"]
        def __init__(self, _io, _parent=None, _root=None):
            self._io = _io
            self._parent = _parent
            self._root = _root if _root else self
            self._debug = collections.defaultdict(dict)
            self._read()

        def _read(self):
            self._debug['_unnamed0']['start'] = self._io.pos()
            self._unnamed0 = self._io.read_u1()
            self._debug['_unnamed0']['end'] = self._io.pos()
            self._debug['_unnamed1']['start'] = self._io.pos()
            self._unnamed1 = self._io.read_u2le()
            self._debug['_unnamed1']['end'] = self._io.pos()
            self._debug['key']['start'] = self._io.pos()
            self.key = self._io.read_u4le()
            self._debug['key']['end'] = self._io.pos()
            self._debug['next']['start'] = self._io.pos()
            self.next = self._io.read_u4le()
            self._debug['next']['end'] = self._io.pos()
            self._debug['str']['start'] = self._io.pos()
            self.str = self._io.read_u4le()
            self._debug['str']['end'] = self._io.pos()
            self._debug['str_2']['start'] = self._io.pos()
            self.str_2 = self._io.read_u4le()
            self._debug['str_2']['end'] = self._io.pos()
            self._debug['ptr_instance']['start'] = self._io.pos()
            self.ptr_instance = self._io.read_u4le()
            self._debug['ptr_instance']['end'] = self._io.pos()
            self._debug['ptr_fp']['start'] = self._io.pos()
            self.ptr_fp = self._io.read_u4le()
            self._debug['ptr_fp']['end'] = self._io.pos()
            self._debug['ptr_x08']['start'] = self._io.pos()
            self.ptr_x08 = self._io.read_u4le()
            self._debug['ptr_x08']['end'] = self._io.pos()
            self._debug['ptr_x03_symbol']['start'] = self._io.pos()
            self.ptr_x03_symbol = self._io.read_u4le()
            self._debug['ptr_x03_symbol']['end'] = self._io.pos()
            if self._root.ver >= 1310720:
                self._debug['unknown_1']['start'] = self._io.pos()
                self.unknown_1 = self._io.read_u4le()
                self._debug['unknown_1']['end'] = self._io.pos()



    class Type2cGroup(KaitaiStruct):
        """Seems to be some kind of group.
        
        All tables have one of thes, but there appear to be others too,
        some to do with dimensions.
        """
        SEQ_FIELDS = ["_unnamed0", "_unnamed1", "key", "next", "unknown_1", "unknown_2", "unknown_3", "subclass_str", "unknown_4", "ptr_1", "ptr_2", "ptr_3", "flags"]
        def __init__(self, _io, _parent=None, _root=None):
            self._io = _io
            self._parent = _parent
            self._root = _root if _root else self
            self._debug = collections.defaultdict(dict)
            self._read()

        def _read(self):
            self._debug['_unnamed0']['start'] = self._io.pos()
            self._unnamed0 = self._io.read_u1()
            self._debug['_unnamed0']['end'] = self._io.pos()
            self._debug['_unnamed1']['start'] = self._io.pos()
            self._unnamed1 = self._io.read_u2le()
            self._debug['_unnamed1']['end'] = self._io.pos()
            self._debug['key']['start'] = self._io.pos()
            self.key = self._io.read_u4le()
            self._debug['key']['end'] = self._io.pos()
            self._debug['next']['start'] = self._io.pos()
            self.next = self._io.read_u4le()
            self._debug['next']['end'] = self._io.pos()
            if self._root.ver >= 1311744:
                self._debug['unknown_1']['start'] = self._io.pos()
                self.unknown_1 = self._io.read_u4le()
                self._debug['unknown_1']['end'] = self._io.pos()

            if self._root.ver >= 1311744:
                self._debug['unknown_2']['start'] = self._io.pos()
                self.unknown_2 = self._io.read_u4le()
                self._debug['unknown_2']['end'] = self._io.pos()

            if self._root.ver >= 1311744:
                self._debug['unknown_3']['start'] = self._io.pos()
                self.unknown_3 = self._io.read_u4le()
                self._debug['unknown_3']['end'] = self._io.pos()

            self._debug['subclass_str']['start'] = self._io.pos()
            self.subclass_str = self._io.read_u4le()
            self._debug['subclass_str']['end'] = self._io.pos()
            if self._root.ver < 1311744:
                self._debug['unknown_4']['start'] = self._io.pos()
                self.unknown_4 = self._io.read_u4le()
                self._debug['unknown_4']['end'] = self._io.pos()

            self._debug['ptr_1']['start'] = self._io.pos()
            self.ptr_1 = self._io.read_u4le()
            self._debug['ptr_1']['end'] = self._io.pos()
            self._debug['ptr_2']['start'] = self._io.pos()
            self.ptr_2 = self._io.read_u4le()
            self._debug['ptr_2']['end'] = self._io.pos()
            self._debug['ptr_3']['start'] = self._io.pos()
            self.ptr_3 = self._io.read_u4le()
            self._debug['ptr_3']['end'] = self._io.pos()
            self._debug['flags']['start'] = self._io.pos()
            self.flags = self._io.read_bits_int_be(32)
            self._debug['flags']['end'] = self._io.pos()


    class CadenceFp(KaitaiStruct):
        SEQ_FIELDS = ["a", "b"]
        def __init__(self, _io, _parent=None, _root=None):
            self._io = _io
            self._parent = _parent
            self._root = _root if _root else self
            self._debug = collections.defaultdict(dict)
            self._read()

        def _read(self):
            self._debug['a']['start'] = self._io.pos()
            self.a = self._io.read_u4le()
            self._debug['a']['end'] = self._io.pos()
            self._debug['b']['start'] = self._io.pos()
            self.b = self._io.read_u4le()
            self._debug['b']['end'] = self._io.pos()


    class Type20(KaitaiStruct):
        SEQ_FIELDS = ["_unnamed0", "_unnamed1", "key", "ptr1", "un", "un1"]
        def __init__(self, _io, _parent=None, _root=None):
            self._io = _io
            self._parent = _parent
            self._root = _root if _root else self
            self._debug = collections.defaultdict(dict)
            self._read()

        def _read(self):
            self._debug['_unnamed0']['start'] = self._io.pos()
            self._unnamed0 = self._io.read_u1()
            self._debug['_unnamed0']['end'] = self._io.pos()
            self._debug['_unnamed1']['start'] = self._io.pos()
            self._unnamed1 = self._io.read_u2le()
            self._debug['_unnamed1']['end'] = self._io.pos()
            self._debug['key']['start'] = self._io.pos()
            self.key = self._io.read_u4le()
            self._debug['key']['end'] = self._io.pos()
            self._debug['ptr1']['start'] = self._io.pos()
            self.ptr1 = self._io.read_u4le()
            self._debug['ptr1']['end'] = self._io.pos()
            self._debug['un']['start'] = self._io.pos()
            self.un = []
            for i in range(7):
                if not 'arr' in self._debug['un']:
                    self._debug['un']['arr'] = []
                self._debug['un']['arr'].append({'start': self._io.pos()})
                self.un.append(self._io.read_u4le())
                self._debug['un']['arr'][i]['end'] = self._io.pos()

            self._debug['un']['end'] = self._io.pos()
            if self._root.ver >= 1313024:
                self._debug['un1']['start'] = self._io.pos()
                self.un1 = []
                for i in range(10):
                    if not 'arr' in self._debug['un1']:
                        self._debug['un1']['arr'] = []
                    self._debug['un1']['arr'].append({'start': self._io.pos()})
                    self.un1.append(self._io.read_u4le())
                    self._debug['un1']['arr'][i]['end'] = self._io.pos()

                self._debug['un1']['end'] = self._io.pos()



    class LinkedList(KaitaiStruct):
        SEQ_FIELDS = ["tail", "head"]
        def __init__(self, _io, _parent=None, _root=None):
            self._io = _io
            self._parent = _parent
            self._root = _root if _root else self
            self._debug = collections.defaultdict(dict)
            self._read()

        def _read(self):
            self._debug['tail']['start'] = self._io.pos()
            self.tail = self._io.read_u4le()
            self._debug['tail']['end'] = self._io.pos()
            self._debug['head']['start'] = self._io.pos()
            self.head = self._io.read_u4le()
            self._debug['head']['end'] = self._io.pos()


    class Type30StrWrapper(KaitaiStruct):

        class TextReversal(Enum):
            straight = 0
            reversed = 1

        class TextAligmnent(Enum):
            left = 1
            right = 2
            center = 3
        SEQ_FIELDS = ["type", "layer", "key", "next", "unknown_1", "unknown_2", "font", "ptr3", "unknown_3", "str_graphic_ptr", "unknown_4", "font_16x", "ptr4", "coords", "unknown_5", "rotation", "ptr3_16x"]
        def __init__(self, _io, _parent=None, _root=None):
            self._io = _io
            self._parent = _parent
            self._root = _root if _root else self
            self._debug = collections.defaultdict(dict)
            self._read()

        def _read(self):
            self._debug['type']['start'] = self._io.pos()
            self.type = self._io.read_u1()
            self._debug['type']['end'] = self._io.pos()
            self._debug['layer']['start'] = self._io.pos()
            self.layer = AllegroBrd.LayerInfo(self._io, self, self._root)
            self._debug['layer']['end'] = self._io.pos()
            self._debug['key']['start'] = self._io.pos()
            self.key = self._io.read_u4le()
            self._debug['key']['end'] = self._io.pos()
            self._debug['next']['start'] = self._io.pos()
            self.next = self._io.read_u4le()
            self._debug['next']['end'] = self._io.pos()
            if self._root.ver >= 1311744:
                self._debug['unknown_1']['start'] = self._io.pos()
                self.unknown_1 = self._io.read_u4le()
                self._debug['unknown_1']['end'] = self._io.pos()

            if self._root.ver >= 1311744:
                self._debug['unknown_2']['start'] = self._io.pos()
                self.unknown_2 = self._io.read_u4le()
                self._debug['unknown_2']['end'] = self._io.pos()

            if self._root.ver >= 1311744:
                self._debug['font']['start'] = self._io.pos()
                self.font = AllegroBrd.Type30StrWrapper.TextProperties(self._io, self, self._root)
                self._debug['font']['end'] = self._io.pos()

            if self._root.ver >= 1311744:
                self._debug['ptr3']['start'] = self._io.pos()
                self.ptr3 = self._io.read_u4le()
                self._debug['ptr3']['end'] = self._io.pos()

            if self._root.ver >= 1313024:
                self._debug['unknown_3']['start'] = self._io.pos()
                self.unknown_3 = self._io.read_u4le()
                self._debug['unknown_3']['end'] = self._io.pos()

            self._debug['str_graphic_ptr']['start'] = self._io.pos()
            self.str_graphic_ptr = self._io.read_u4le()
            self._debug['str_graphic_ptr']['end'] = self._io.pos()
            self._debug['unknown_4']['start'] = self._io.pos()
            self.unknown_4 = self._io.read_u4le()
            self._debug['unknown_4']['end'] = self._io.pos()
            if self._root.ver < 1311744:
                self._debug['font_16x']['start'] = self._io.pos()
                self.font_16x = AllegroBrd.Type30StrWrapper.TextProperties(self._io, self, self._root)
                self._debug['font_16x']['end'] = self._io.pos()

            if self._root.ver >= 1311744:
                self._debug['ptr4']['start'] = self._io.pos()
                self.ptr4 = self._io.read_u4le()
                self._debug['ptr4']['end'] = self._io.pos()

            self._debug['coords']['start'] = self._io.pos()
            self.coords = AllegroBrd.Coords(self._io, self, self._root)
            self._debug['coords']['end'] = self._io.pos()
            self._debug['unknown_5']['start'] = self._io.pos()
            self.unknown_5 = self._io.read_u4le()
            self._debug['unknown_5']['end'] = self._io.pos()
            self._debug['rotation']['start'] = self._io.pos()
            self.rotation = self._io.read_u4le()
            self._debug['rotation']['end'] = self._io.pos()
            if self._root.ver < 1311744:
                self._debug['ptr3_16x']['start'] = self._io.pos()
                self.ptr3_16x = self._io.read_u4le()
                self._debug['ptr3_16x']['end'] = self._io.pos()


        class TextProperties(KaitaiStruct):
            SEQ_FIELDS = ["key", "flags", "alignment", "reversal"]
            def __init__(self, _io, _parent=None, _root=None):
                self._io = _io
                self._parent = _parent
                self._root = _root if _root else self
                self._debug = collections.defaultdict(dict)
                self._read()

            def _read(self):
                self._debug['key']['start'] = self._io.pos()
                self.key = self._io.read_u1()
                self._debug['key']['end'] = self._io.pos()
                self._debug['flags']['start'] = self._io.pos()
                self.flags = self._io.read_bits_int_be(8)
                self._debug['flags']['end'] = self._io.pos()
                self._io.align_to_byte()
                self._debug['alignment']['start'] = self._io.pos()
                self.alignment = KaitaiStream.resolve_enum(AllegroBrd.Type30StrWrapper.TextAligmnent, self._io.read_u1())
                self._debug['alignment']['end'] = self._io.pos()
                self._debug['reversal']['start'] = self._io.pos()
                self.reversal = KaitaiStream.resolve_enum(AllegroBrd.Type30StrWrapper.TextReversal, self._io.read_u1())
                self._debug['reversal']['end'] = self._io.pos()



    class Type1bNet(KaitaiStruct):
        SEQ_FIELDS = ["_unnamed0", "_unnamed1", "key", "next", "net_name", "unknown_1", "unknown_2", "type", "assignments", "ptr2", "path_str_ptr", "ptr4", "model_ptr", "unknown_3", "unknown_4", "ptr6"]
        def __init__(self, _io, _parent=None, _root=None):
            self._io = _io
            self._parent = _parent
            self._root = _root if _root else self
            self._debug = collections.defaultdict(dict)
            self._read()

        def _read(self):
            self._debug['_unnamed0']['start'] = self._io.pos()
            self._unnamed0 = self._io.read_u1()
            self._debug['_unnamed0']['end'] = self._io.pos()
            self._debug['_unnamed1']['start'] = self._io.pos()
            self._unnamed1 = self._io.read_u2le()
            self._debug['_unnamed1']['end'] = self._io.pos()
            self._debug['key']['start'] = self._io.pos()
            self.key = self._io.read_u4le()
            self._debug['key']['end'] = self._io.pos()
            self._debug['next']['start'] = self._io.pos()
            self.next = self._io.read_u4le()
            self._debug['next']['end'] = self._io.pos()
            self._debug['net_name']['start'] = self._io.pos()
            self.net_name = self._io.read_u4le()
            self._debug['net_name']['end'] = self._io.pos()
            self._debug['unknown_1']['start'] = self._io.pos()
            self.unknown_1 = self._io.read_u4le()
            self._debug['unknown_1']['end'] = self._io.pos()
            if self._root.ver >= 1311744:
                self._debug['unknown_2']['start'] = self._io.pos()
                self.unknown_2 = self._io.read_u4le()
                self._debug['unknown_2']['end'] = self._io.pos()

            self._debug['type']['start'] = self._io.pos()
            self.type = self._io.read_u4le()
            self._debug['type']['end'] = self._io.pos()
            self._debug['assignments']['start'] = self._io.pos()
            self.assignments = self._io.read_u4le()
            self._debug['assignments']['end'] = self._io.pos()
            self._debug['ptr2']['start'] = self._io.pos()
            self.ptr2 = self._io.read_u4le()
            self._debug['ptr2']['end'] = self._io.pos()
            self._debug['path_str_ptr']['start'] = self._io.pos()
            self.path_str_ptr = self._io.read_u4le()
            self._debug['path_str_ptr']['end'] = self._io.pos()
            self._debug['ptr4']['start'] = self._io.pos()
            self.ptr4 = self._io.read_u4le()
            self._debug['ptr4']['end'] = self._io.pos()
            self._debug['model_ptr']['start'] = self._io.pos()
            self.model_ptr = self._io.read_u4le()
            self._debug['model_ptr']['end'] = self._io.pos()
            self._debug['unknown_3']['start'] = self._io.pos()
            self.unknown_3 = self._io.read_u4le()
            self._debug['unknown_3']['end'] = self._io.pos()
            self._debug['unknown_4']['start'] = self._io.pos()
            self.unknown_4 = self._io.read_u4le()
            self._debug['unknown_4']['end'] = self._io.pos()
            self._debug['ptr6']['start'] = self._io.pos()
            self.ptr6 = self._io.read_u4le()
            self._debug['ptr6']['end'] = self._io.pos()


    class Type3c(KaitaiStruct):
        """Looks like a binding from a dimension to the dimensioned object.
        """
        SEQ_FIELDS = ["t", "t2", "key", "unknown_1", "num_entries", "entries"]
        def __init__(self, _io, _parent=None, _root=None):
            self._io = _io
            self._parent = _parent
            self._root = _root if _root else self
            self._debug = collections.defaultdict(dict)
            self._read()

        def _read(self):
            self._debug['t']['start'] = self._io.pos()
            self.t = self._io.read_u1()
            self._debug['t']['end'] = self._io.pos()
            self._debug['t2']['start'] = self._io.pos()
            self.t2 = self._io.read_u2le()
            self._debug['t2']['end'] = self._io.pos()
            self._debug['key']['start'] = self._io.pos()
            self.key = self._io.read_u4le()
            self._debug['key']['end'] = self._io.pos()
            if self._root.ver >= 1313024:
                self._debug['unknown_1']['start'] = self._io.pos()
                self.unknown_1 = self._io.read_u4le()
                self._debug['unknown_1']['end'] = self._io.pos()

            self._debug['num_entries']['start'] = self._io.pos()
            self.num_entries = self._io.read_u4le()
            self._debug['num_entries']['end'] = self._io.pos()
            self._debug['entries']['start'] = self._io.pos()
            self.entries = []
            for i in range(self.num_entries):
                if not 'arr' in self._debug['entries']:
                    self._debug['entries']['arr'] = []
                self._debug['entries']['arr'].append({'start': self._io.pos()})
                self.entries.append(self._io.read_u4le())
                self._debug['entries']['arr'][i]['end'] = self._io.pos()

            self._debug['entries']['end'] = self._io.pos()


    class StringMap(KaitaiStruct):
        SEQ_FIELDS = ["entries"]
        def __init__(self, num_entries, _io, _parent=None, _root=None):
            self._io = _io
            self._parent = _parent
            self._root = _root if _root else self
            self.num_entries = num_entries
            self._debug = collections.defaultdict(dict)
            self._read()

        def _read(self):
            self._debug['entries']['start'] = self._io.pos()
            self.entries = []
            for i in range(self.num_entries):
                if not 'arr' in self._debug['entries']:
                    self._debug['entries']['arr'] = []
                self._debug['entries']['arr'].append({'start': self._io.pos()})
                self.entries.append(AllegroBrd.StringMap.Entry(self._io, self, self._root))
                self._debug['entries']['arr'][i]['end'] = self._io.pos()

            self._debug['entries']['end'] = self._io.pos()

        class Entry(KaitaiStruct):
            SEQ_FIELDS = ["string_id", "value", "_unnamed2"]
            def __init__(self, _io, _parent=None, _root=None):
                self._io = _io
                self._parent = _parent
                self._root = _root if _root else self
                self._debug = collections.defaultdict(dict)
                self._read()

            def _read(self):
                self._debug['string_id']['start'] = self._io.pos()
                self.string_id = self._io.read_u4le()
                self._debug['string_id']['end'] = self._io.pos()
                self._debug['value']['start'] = self._io.pos()
                self.value = (self._io.read_bytes_term(0, False, True, True)).decode(u"ASCII")
                self._debug['value']['end'] = self._io.pos()
                if (self._io.pos() % 4) != 0:
                    self._debug['_unnamed2']['start'] = self._io.pos()
                    self._unnamed2 = []
                    i = 0
                    while True:
                        if not 'arr' in self._debug['_unnamed2']:
                            self._debug['_unnamed2']['arr'] = []
                        self._debug['_unnamed2']['arr'].append({'start': self._io.pos()})
                        _ = self._io.read_u1()
                        self._unnamed2.append(_)
                        self._debug['_unnamed2']['arr'][len(self._unnamed2) - 1]['end'] = self._io.pos()
                        if (self._io.pos() % 4) == 0:
                            break
                        i += 1
                    self._debug['_unnamed2']['end'] = self._io.pos()




    class Type2b(KaitaiStruct):
        SEQ_FIELDS = ["_unnamed0", "subtype", "key", "fp_str_ref", "unknown_1", "coords0", "coords1", "next", "first_inst_ptr", "ptr_2", "ptr_3", "ptr_4", "lib_path", "ptr_5", "ptr_6", "ptr_7", "unknown_2", "unknown_3"]
        def __init__(self, _io, _parent=None, _root=None):
            self._io = _io
            self._parent = _parent
            self._root = _root if _root else self
            self._debug = collections.defaultdict(dict)
            self._read()

        def _read(self):
            self._debug['_unnamed0']['start'] = self._io.pos()
            self._unnamed0 = self._io.read_u1()
            self._debug['_unnamed0']['end'] = self._io.pos()
            self._debug['subtype']['start'] = self._io.pos()
            self.subtype = self._io.read_u2le()
            self._debug['subtype']['end'] = self._io.pos()
            self._debug['key']['start'] = self._io.pos()
            self.key = self._io.read_u4le()
            self._debug['key']['end'] = self._io.pos()
            self._debug['fp_str_ref']['start'] = self._io.pos()
            self.fp_str_ref = self._io.read_u4le()
            self._debug['fp_str_ref']['end'] = self._io.pos()
            self._debug['unknown_1']['start'] = self._io.pos()
            self.unknown_1 = self._io.read_u4le()
            self._debug['unknown_1']['end'] = self._io.pos()
            self._debug['coords0']['start'] = self._io.pos()
            self.coords0 = AllegroBrd.Coords(self._io, self, self._root)
            self._debug['coords0']['end'] = self._io.pos()
            self._debug['coords1']['start'] = self._io.pos()
            self.coords1 = AllegroBrd.Coords(self._io, self, self._root)
            self._debug['coords1']['end'] = self._io.pos()
            self._debug['next']['start'] = self._io.pos()
            self.next = self._io.read_u4le()
            self._debug['next']['end'] = self._io.pos()
            self._debug['first_inst_ptr']['start'] = self._io.pos()
            self.first_inst_ptr = self._io.read_u4le()
            self._debug['first_inst_ptr']['end'] = self._io.pos()
            self._debug['ptr_2']['start'] = self._io.pos()
            self.ptr_2 = self._io.read_u4le()
            self._debug['ptr_2']['end'] = self._io.pos()
            self._debug['ptr_3']['start'] = self._io.pos()
            self.ptr_3 = self._io.read_u4le()
            self._debug['ptr_3']['end'] = self._io.pos()
            self._debug['ptr_4']['start'] = self._io.pos()
            self.ptr_4 = self._io.read_u4le()
            self._debug['ptr_4']['end'] = self._io.pos()
            self._debug['lib_path']['start'] = self._io.pos()
            self.lib_path = self._io.read_u4le()
            self._debug['lib_path']['end'] = self._io.pos()
            self._debug['ptr_5']['start'] = self._io.pos()
            self.ptr_5 = self._io.read_u4le()
            self._debug['ptr_5']['end'] = self._io.pos()
            self._debug['ptr_6']['start'] = self._io.pos()
            self.ptr_6 = self._io.read_u4le()
            self._debug['ptr_6']['end'] = self._io.pos()
            self._debug['ptr_7']['start'] = self._io.pos()
            self.ptr_7 = self._io.read_u4le()
            self._debug['ptr_7']['end'] = self._io.pos()
            if self._root.ver >= 1248256:
                self._debug['unknown_2']['start'] = self._io.pos()
                self.unknown_2 = self._io.read_u4le()
                self._debug['unknown_2']['end'] = self._io.pos()

            if self._root.ver >= 1311744:
                self._debug['unknown_3']['start'] = self._io.pos()
                self.unknown_3 = self._io.read_u4le()
                self._debug['unknown_3']['end'] = self._io.pos()



    class Type27(KaitaiStruct):
        """A block that extends to the offset in the file header."""
        SEQ_FIELDS = ["_unnamed0"]
        def __init__(self, _io, _parent=None, _root=None):
            self._io = _io
            self._parent = _parent
            self._root = _root if _root else self
            self._debug = collections.defaultdict(dict)
            self._read()

        def _read(self):
            self._debug['_unnamed0']['start'] = self._io.pos()
            self._unnamed0 = []
            i = 0
            while True:
                if not 'arr' in self._debug['_unnamed0']:
                    self._debug['_unnamed0']['arr'] = []
                self._debug['_unnamed0']['arr'].append({'start': self._io.pos()})
                _ = self._io.read_u1()
                self._unnamed0.append(_)
                self._debug['_unnamed0']['arr'][len(self._unnamed0) - 1]['end'] = self._io.pos()
                if self._io.pos() == (self._root.x27_end_offset - 1):
                    break
                i += 1
            self._debug['_unnamed0']['end'] = self._io.pos()


    class TypeDefault(KaitaiStruct):
        SEQ_FIELDS = ["_unnamed0", "_unnamed1", "key"]
        def __init__(self, _io, _parent=None, _root=None):
            self._io = _io
            self._parent = _parent
            self._root = _root if _root else self
            self._debug = collections.defaultdict(dict)
            self._read()

        def _read(self):
            self._debug['_unnamed0']['start'] = self._io.pos()
            self._unnamed0 = self._io.read_u1()
            self._debug['_unnamed0']['end'] = self._io.pos()
            self._debug['_unnamed1']['start'] = self._io.pos()
            self._unnamed1 = self._io.read_u2le()
            self._debug['_unnamed1']['end'] = self._io.pos()
            self._debug['key']['start'] = self._io.pos()
            self.key = self._io.read_u4le()
            self._debug['key']['end'] = self._io.pos()
            _ = self.key
            if not False:
                raise kaitaistruct.ValidationExprError(self.key, self._io, u"/types/type_default/seq/2")


    class Type33Via(KaitaiStruct):
        SEQ_FIELDS = ["t", "layer", "key", "next", "net_ptr", "unknown_1", "unknown_2", "ptr1", "ptr2", "pos", "connection", "padstack", "ptr3", "ptr4", "unknown_3", "unknown_4", "bb_coords"]
        def __init__(self, _io, _parent=None, _root=None):
            self._io = _io
            self._parent = _parent
            self._root = _root if _root else self
            self._debug = collections.defaultdict(dict)
            self._read()

        def _read(self):
            self._debug['t']['start'] = self._io.pos()
            self.t = self._io.read_u1()
            self._debug['t']['end'] = self._io.pos()
            self._debug['layer']['start'] = self._io.pos()
            self.layer = AllegroBrd.LayerInfo(self._io, self, self._root)
            self._debug['layer']['end'] = self._io.pos()
            self._debug['key']['start'] = self._io.pos()
            self.key = self._io.read_u4le()
            self._debug['key']['end'] = self._io.pos()
            self._debug['next']['start'] = self._io.pos()
            self.next = self._io.read_u4le()
            self._debug['next']['end'] = self._io.pos()
            self._debug['net_ptr']['start'] = self._io.pos()
            self.net_ptr = self._io.read_u4le()
            self._debug['net_ptr']['end'] = self._io.pos()
            self._debug['unknown_1']['start'] = self._io.pos()
            self.unknown_1 = self._io.read_u4le()
            self._debug['unknown_1']['end'] = self._io.pos()
            if self._root.ver >= 1311744:
                self._debug['unknown_2']['start'] = self._io.pos()
                self.unknown_2 = self._io.read_u4le()
                self._debug['unknown_2']['end'] = self._io.pos()

            self._debug['ptr1']['start'] = self._io.pos()
            self.ptr1 = self._io.read_u4le()
            self._debug['ptr1']['end'] = self._io.pos()
            if self._root.ver >= 1311744:
                self._debug['ptr2']['start'] = self._io.pos()
                self.ptr2 = self._io.read_u4le()
                self._debug['ptr2']['end'] = self._io.pos()

            self._debug['pos']['start'] = self._io.pos()
            self.pos = AllegroBrd.Coords(self._io, self, self._root)
            self._debug['pos']['end'] = self._io.pos()
            self._debug['connection']['start'] = self._io.pos()
            self.connection = self._io.read_u4le()
            self._debug['connection']['end'] = self._io.pos()
            self._debug['padstack']['start'] = self._io.pos()
            self.padstack = self._io.read_u4le()
            self._debug['padstack']['end'] = self._io.pos()
            self._debug['ptr3']['start'] = self._io.pos()
            self.ptr3 = self._io.read_u4le()
            self._debug['ptr3']['end'] = self._io.pos()
            self._debug['ptr4']['start'] = self._io.pos()
            self.ptr4 = self._io.read_u4le()
            self._debug['ptr4']['end'] = self._io.pos()
            self._debug['unknown_3']['start'] = self._io.pos()
            self.unknown_3 = self._io.read_u4le()
            self._debug['unknown_3']['end'] = self._io.pos()
            self._debug['unknown_4']['start'] = self._io.pos()
            self.unknown_4 = self._io.read_u4le()
            self._debug['unknown_4']['end'] = self._io.pos()
            self._debug['bb_coords']['start'] = self._io.pos()
            self.bb_coords = []
            for i in range(4):
                if not 'arr' in self._debug['bb_coords']:
                    self._debug['bb_coords']['arr'] = []
                self._debug['bb_coords']['arr'].append({'start': self._io.pos()})
                self.bb_coords.append(self._io.read_s4le())
                self._debug['bb_coords']['arr'][i]['end'] = self._io.pos()

            self._debug['bb_coords']['end'] = self._io.pos()


    class Type14(KaitaiStruct):
        SEQ_FIELDS = ["type", "layer", "key", "next", "parent_ptr", "unknown_1", "un3", "segment_ptr", "ptr_0x03", "ptr_0x26"]
        def __init__(self, _io, _parent=None, _root=None):
            self._io = _io
            self._parent = _parent
            self._root = _root if _root else self
            self._debug = collections.defaultdict(dict)
            self._read()

        def _read(self):
            self._debug['type']['start'] = self._io.pos()
            self.type = self._io.read_u1()
            self._debug['type']['end'] = self._io.pos()
            self._debug['layer']['start'] = self._io.pos()
            self.layer = AllegroBrd.LayerInfo(self._io, self, self._root)
            self._debug['layer']['end'] = self._io.pos()
            self._debug['key']['start'] = self._io.pos()
            self.key = self._io.read_u4le()
            self._debug['key']['end'] = self._io.pos()
            self._debug['next']['start'] = self._io.pos()
            self.next = self._io.read_u4le()
            self._debug['next']['end'] = self._io.pos()
            self._debug['parent_ptr']['start'] = self._io.pos()
            self.parent_ptr = self._io.read_u4le()
            self._debug['parent_ptr']['end'] = self._io.pos()
            self._debug['unknown_1']['start'] = self._io.pos()
            self.unknown_1 = self._io.read_u4le()
            self._debug['unknown_1']['end'] = self._io.pos()
            if self._root.ver >= 1311744:
                self._debug['un3']['start'] = self._io.pos()
                self.un3 = self._io.read_u4le()
                self._debug['un3']['end'] = self._io.pos()

            self._debug['segment_ptr']['start'] = self._io.pos()
            self.segment_ptr = self._io.read_u4le()
            self._debug['segment_ptr']['end'] = self._io.pos()
            self._debug['ptr_0x03']['start'] = self._io.pos()
            self.ptr_0x03 = self._io.read_u4le()
            self._debug['ptr_0x03']['end'] = self._io.pos()
            self._debug['ptr_0x26']['start'] = self._io.pos()
            self.ptr_0x26 = self._io.read_u4le()
            self._debug['ptr_0x26']['end'] = self._io.pos()


    class Type2f(KaitaiStruct):
        SEQ_FIELDS = ["_unnamed0", "_unnamed1", "key", "un"]
        def __init__(self, _io, _parent=None, _root=None):
            self._io = _io
            self._parent = _parent
            self._root = _root if _root else self
            self._debug = collections.defaultdict(dict)
            self._read()

        def _read(self):
            self._debug['_unnamed0']['start'] = self._io.pos()
            self._unnamed0 = self._io.read_u1()
            self._debug['_unnamed0']['end'] = self._io.pos()
            self._debug['_unnamed1']['start'] = self._io.pos()
            self._unnamed1 = self._io.read_u2le()
            self._debug['_unnamed1']['end'] = self._io.pos()
            self._debug['key']['start'] = self._io.pos()
            self.key = self._io.read_u4le()
            self._debug['key']['end'] = self._io.pos()
            self._debug['un']['start'] = self._io.pos()
            self.un = []
            for i in range(6):
                if not 'arr' in self._debug['un']:
                    self._debug['un']['arr'] = []
                self._debug['un']['arr'].append({'start': self._io.pos()})
                self.un.append(self._io.read_u4le())
                self._debug['un']['arr'][i]['end'] = self._io.pos()

            self._debug['un']['end'] = self._io.pos()


    class Type2a(KaitaiStruct):
        SEQ_FIELDS = ["_unnamed0", "num_entries", "unknown_1", "nonrefs", "refs", "key"]
        def __init__(self, _io, _parent=None, _root=None):
            self._io = _io
            self._parent = _parent
            self._root = _root if _root else self
            self._debug = collections.defaultdict(dict)
            self._read()

        def _read(self):
            self._debug['_unnamed0']['start'] = self._io.pos()
            self._unnamed0 = self._io.read_u1()
            self._debug['_unnamed0']['end'] = self._io.pos()
            self._debug['num_entries']['start'] = self._io.pos()
            self.num_entries = self._io.read_u2le()
            self._debug['num_entries']['end'] = self._io.pos()
            if self._root.ver >= 1313024:
                self._debug['unknown_1']['start'] = self._io.pos()
                self.unknown_1 = self._io.read_u4le()
                self._debug['unknown_1']['end'] = self._io.pos()

            if self._root.ver <= 1248256:
                self._debug['nonrefs']['start'] = self._io.pos()
                self.nonrefs = []
                for i in range(self.num_entries):
                    if not 'arr' in self._debug['nonrefs']:
                        self._debug['nonrefs']['arr'] = []
                    self._debug['nonrefs']['arr'].append({'start': self._io.pos()})
                    self.nonrefs.append(AllegroBrd.Type2a.Nonref(self._io, self, self._root))
                    self._debug['nonrefs']['arr'][i]['end'] = self._io.pos()

                self._debug['nonrefs']['end'] = self._io.pos()

            if self._root.ver > 1248256:
                self._debug['refs']['start'] = self._io.pos()
                self.refs = []
                for i in range(self.num_entries):
                    if not 'arr' in self._debug['refs']:
                        self._debug['refs']['arr'] = []
                    self._debug['refs']['arr'].append({'start': self._io.pos()})
                    self.refs.append(AllegroBrd.Type2a.RefEntry(self._io, self, self._root))
                    self._debug['refs']['arr'][i]['end'] = self._io.pos()

                self._debug['refs']['end'] = self._io.pos()

            self._debug['key']['start'] = self._io.pos()
            self.key = self._io.read_u4le()
            self._debug['key']['end'] = self._io.pos()

        class Nonref(KaitaiStruct):
            SEQ_FIELDS = ["name"]
            def __init__(self, _io, _parent=None, _root=None):
                self._io = _io
                self._parent = _parent
                self._root = _root if _root else self
                self._debug = collections.defaultdict(dict)
                self._read()

            def _read(self):
                self._debug['name']['start'] = self._io.pos()
                self.name = AllegroBrd.StringAligned(36, self._io, self, self._root)
                self._debug['name']['end'] = self._io.pos()


        class RefEntry(KaitaiStruct):
            SEQ_FIELDS = ["ptr", "properties", "un1"]
            def __init__(self, _io, _parent=None, _root=None):
                self._io = _io
                self._parent = _parent
                self._root = _root if _root else self
                self._debug = collections.defaultdict(dict)
                self._read()

            def _read(self):
                self._debug['ptr']['start'] = self._io.pos()
                self.ptr = self._io.read_u4le()
                self._debug['ptr']['end'] = self._io.pos()
                self._debug['properties']['start'] = self._io.pos()
                self.properties = self._io.read_u4le()
                self._debug['properties']['end'] = self._io.pos()
                self._debug['un1']['start'] = self._io.pos()
                self.un1 = self._io.read_u4le()
                self._debug['un1']['end'] = self._io.pos()



    class Type3b(KaitaiStruct):
        SEQ_FIELDS = ["t", "subtype", "len", "name", "type", "un1", "un2", "un3", "value"]
        def __init__(self, _io, _parent=None, _root=None):
            self._io = _io
            self._parent = _parent
            self._root = _root if _root else self
            self._debug = collections.defaultdict(dict)
            self._read()

        def _read(self):
            self._debug['t']['start'] = self._io.pos()
            self.t = self._io.read_u1()
            self._debug['t']['end'] = self._io.pos()
            self._debug['subtype']['start'] = self._io.pos()
            self.subtype = self._io.read_u2le()
            self._debug['subtype']['end'] = self._io.pos()
            self._debug['len']['start'] = self._io.pos()
            self.len = self._io.read_u4le()
            self._debug['len']['end'] = self._io.pos()
            self._debug['name']['start'] = self._io.pos()
            self.name = (KaitaiStream.bytes_terminate(self._io.read_bytes(128), 0, False)).decode(u"ASCII")
            self._debug['name']['end'] = self._io.pos()
            self._debug['type']['start'] = self._io.pos()
            self.type = (KaitaiStream.bytes_terminate(self._io.read_bytes(32), 0, False)).decode(u"ASCII")
            self._debug['type']['end'] = self._io.pos()
            self._debug['un1']['start'] = self._io.pos()
            self.un1 = self._io.read_u4le()
            self._debug['un1']['end'] = self._io.pos()
            self._debug['un2']['start'] = self._io.pos()
            self.un2 = self._io.read_u4le()
            self._debug['un2']['end'] = self._io.pos()
            if self._root.ver >= 1311744:
                self._debug['un3']['start'] = self._io.pos()
                self.un3 = self._io.read_u4le()
                self._debug['un3']['end'] = self._io.pos()

            self._debug['value']['start'] = self._io.pos()
            self.value = AllegroBrd.StringAligned(self.len, self._io, self, self._root)
            self._debug['value']['end'] = self._io.pos()


    class LayerInfo(KaitaiStruct):
        """Layer information structure used in many board objects.
        
        Consists of a layer class and subclass, which together identify
        a specific layer in the board.
        
        These seem to be contiguous and in the order listed here: https://www.artwork.com/all2dxf/alleggeo.htm
        """

        class LayerClass(Enum):
            board_geometry = 1
            etch = 6
            manufacturing = 7
            package_geometry = 9
            ref_des = 13

        class EnumPackageGeom(Enum):
            dfa_bounds_top = 239
            display_top = 242
            dimension = 249
            place_bounds_top = 251
            assembly_top = 253

        class EnumBoardGeom(Enum):
            constraint_area = 235
            off_grid_area = 236
            soldermask_bottom = 237
            soldermask_top = 238
            assembly_detail = 239
            silkscreen_bottom = 240
            silkscreen_top = 241
            switch_area_bottom = 242
            switch_area_top = 243
            both_rooms = 244
            bottom_room = 245
            top_room = 246
            place_grid_bottom = 247
            place_grid_top = 248
            dimension = 249
            tooling_corners = 250
            assembly_notes = 251
            plating_bar = 252
            outline = 253

        class EnumManufacturing(Enum):
            unknown = 240
        SEQ_FIELDS = ["lclass", "subclass"]
        def __init__(self, _io, _parent=None, _root=None):
            self._io = _io
            self._parent = _parent
            self._root = _root if _root else self
            self._debug = collections.defaultdict(dict)
            self._read()

        def _read(self):
            self._debug['lclass']['start'] = self._io.pos()
            self.lclass = KaitaiStream.resolve_enum(AllegroBrd.LayerInfo.LayerClass, self._io.read_u1())
            self._debug['lclass']['end'] = self._io.pos()
            self._debug['subclass']['start'] = self._io.pos()
            _on = self.lclass
            if _on == AllegroBrd.LayerInfo.LayerClass.package_geometry:
                self.subclass = AllegroBrd.LayerInfo.SubclassPackageGeom(self._io, self, self._root)
            elif _on == AllegroBrd.LayerInfo.LayerClass.board_geometry:
                self.subclass = AllegroBrd.LayerInfo.SubclassBoardGeom(self._io, self, self._root)
            elif _on == AllegroBrd.LayerInfo.LayerClass.manufacturing:
                self.subclass = AllegroBrd.LayerInfo.SubclassManufacturing(self._io, self, self._root)
            elif _on == AllegroBrd.LayerInfo.LayerClass.etch:
                self.subclass = self._io.read_u1()
            else:
                self.subclass = self._io.read_u1()
            self._debug['subclass']['end'] = self._io.pos()

        class SubclassBoardGeom(KaitaiStruct):
            SEQ_FIELDS = ["sc"]
            def __init__(self, _io, _parent=None, _root=None):
                self._io = _io
                self._parent = _parent
                self._root = _root if _root else self
                self._debug = collections.defaultdict(dict)
                self._read()

            def _read(self):
                self._debug['sc']['start'] = self._io.pos()
                self.sc = KaitaiStream.resolve_enum(AllegroBrd.LayerInfo.EnumBoardGeom, self._io.read_u1())
                self._debug['sc']['end'] = self._io.pos()


        class SubclassManufacturing(KaitaiStruct):
            SEQ_FIELDS = ["sc"]
            def __init__(self, _io, _parent=None, _root=None):
                self._io = _io
                self._parent = _parent
                self._root = _root if _root else self
                self._debug = collections.defaultdict(dict)
                self._read()

            def _read(self):
                self._debug['sc']['start'] = self._io.pos()
                self.sc = KaitaiStream.resolve_enum(AllegroBrd.LayerInfo.EnumManufacturing, self._io.read_u1())
                self._debug['sc']['end'] = self._io.pos()


        class SubclassPackageGeom(KaitaiStruct):
            SEQ_FIELDS = ["sc"]
            def __init__(self, _io, _parent=None, _root=None):
                self._io = _io
                self._parent = _parent
                self._root = _root if _root else self
                self._debug = collections.defaultdict(dict)
                self._read()

            def _read(self):
                self._debug['sc']['start'] = self._io.pos()
                self.sc = KaitaiStream.resolve_enum(AllegroBrd.LayerInfo.EnumPackageGeom, self._io.read_u1())
                self._debug['sc']['end'] = self._io.pos()



    class Type31Sgraphic(KaitaiStruct):

        class StringLayer(Enum):
            bot_text = 61441
            top_text = 61697
            bot_pin = 62985
            top_pin = 63241
            top_pin_label = 63753
            bot_refdes = 64013
            top_refdes = 64269
        SEQ_FIELDS = ["t", "layer", "key", "str_graphic_wrapper_ptr", "coords", "unknown_1", "len", "unknown_2", "value"]
        def __init__(self, _io, _parent=None, _root=None):
            self._io = _io
            self._parent = _parent
            self._root = _root if _root else self
            self._debug = collections.defaultdict(dict)
            self._read()

        def _read(self):
            self._debug['t']['start'] = self._io.pos()
            self.t = self._io.read_u1()
            self._debug['t']['end'] = self._io.pos()
            self._debug['layer']['start'] = self._io.pos()
            self.layer = AllegroBrd.LayerInfo(self._io, self, self._root)
            self._debug['layer']['end'] = self._io.pos()
            self._debug['key']['start'] = self._io.pos()
            self.key = self._io.read_u4le()
            self._debug['key']['end'] = self._io.pos()
            self._debug['str_graphic_wrapper_ptr']['start'] = self._io.pos()
            self.str_graphic_wrapper_ptr = self._io.read_u4le()
            self._debug['str_graphic_wrapper_ptr']['end'] = self._io.pos()
            self._debug['coords']['start'] = self._io.pos()
            self.coords = AllegroBrd.Coords(self._io, self, self._root)
            self._debug['coords']['end'] = self._io.pos()
            self._debug['unknown_1']['start'] = self._io.pos()
            self.unknown_1 = self._io.read_u2le()
            self._debug['unknown_1']['end'] = self._io.pos()
            self._debug['len']['start'] = self._io.pos()
            self.len = self._io.read_u2le()
            self._debug['len']['end'] = self._io.pos()
            if self._root.ver >= 1313024:
                self._debug['unknown_2']['start'] = self._io.pos()
                self.unknown_2 = self._io.read_u4le()
                self._debug['unknown_2']['end'] = self._io.pos()

            self._debug['value']['start'] = self._io.pos()
            self.value = AllegroBrd.StringAligned(self.len, self._io, self, self._root)
            self._debug['value']['end'] = self._io.pos()


    class Type23Ratline(KaitaiStruct):
        SEQ_FIELDS = ["type", "layer", "key", "next", "flags", "ptr1", "ptr2", "ptr3", "unknown_1", "coords_0", "coords_1", "unknown_2", "unknown_3", "unknown_4"]
        def __init__(self, _io, _parent=None, _root=None):
            self._io = _io
            self._parent = _parent
            self._root = _root if _root else self
            self._debug = collections.defaultdict(dict)
            self._read()

        def _read(self):
            self._debug['type']['start'] = self._io.pos()
            self.type = self._io.read_u1()
            self._debug['type']['end'] = self._io.pos()
            self._debug['layer']['start'] = self._io.pos()
            self.layer = AllegroBrd.LayerInfo(self._io, self, self._root)
            self._debug['layer']['end'] = self._io.pos()
            self._debug['key']['start'] = self._io.pos()
            self.key = self._io.read_u4le()
            self._debug['key']['end'] = self._io.pos()
            self._debug['next']['start'] = self._io.pos()
            self.next = self._io.read_u4le()
            self._debug['next']['end'] = self._io.pos()
            self._debug['flags']['start'] = self._io.pos()
            self.flags = []
            for i in range(2):
                if not 'arr' in self._debug['flags']:
                    self._debug['flags']['arr'] = []
                self._debug['flags']['arr'].append({'start': self._io.pos()})
                self.flags.append(self._io.read_bits_int_be(32))
                self._debug['flags']['arr'][i]['end'] = self._io.pos()

            self._debug['flags']['end'] = self._io.pos()
            self._io.align_to_byte()
            self._debug['ptr1']['start'] = self._io.pos()
            self.ptr1 = self._io.read_u4le()
            self._debug['ptr1']['end'] = self._io.pos()
            self._debug['ptr2']['start'] = self._io.pos()
            self.ptr2 = self._io.read_u4le()
            self._debug['ptr2']['end'] = self._io.pos()
            self._debug['ptr3']['start'] = self._io.pos()
            self.ptr3 = self._io.read_u4le()
            self._debug['ptr3']['end'] = self._io.pos()
            self._debug['unknown_1']['start'] = self._io.pos()
            self.unknown_1 = self._io.read_u4le()
            self._debug['unknown_1']['end'] = self._io.pos()
            self._debug['coords_0']['start'] = self._io.pos()
            self.coords_0 = AllegroBrd.Coords(self._io, self, self._root)
            self._debug['coords_0']['end'] = self._io.pos()
            self._debug['coords_1']['start'] = self._io.pos()
            self.coords_1 = AllegroBrd.Coords(self._io, self, self._root)
            self._debug['coords_1']['end'] = self._io.pos()
            self._debug['unknown_2']['start'] = self._io.pos()
            self.unknown_2 = []
            for i in range(4):
                if not 'arr' in self._debug['unknown_2']:
                    self._debug['unknown_2']['arr'] = []
                self._debug['unknown_2']['arr'].append({'start': self._io.pos()})
                self.unknown_2.append(self._io.read_u4le())
                self._debug['unknown_2']['arr'][i]['end'] = self._io.pos()

            self._debug['unknown_2']['end'] = self._io.pos()
            if self._root.ver >= 1248256:
                self._debug['unknown_3']['start'] = self._io.pos()
                self.unknown_3 = []
                for i in range(4):
                    if not 'arr' in self._debug['unknown_3']:
                        self._debug['unknown_3']['arr'] = []
                    self._debug['unknown_3']['arr'].append({'start': self._io.pos()})
                    self.unknown_3.append(self._io.read_u4le())
                    self._debug['unknown_3']['arr'][i]['end'] = self._io.pos()

                self._debug['unknown_3']['end'] = self._io.pos()

            if self._root.ver >= 1313024:
                self._debug['unknown_4']['start'] = self._io.pos()
                self.unknown_4 = self._io.read_u4le()
                self._debug['unknown_4']['end'] = self._io.pos()



    class Type2e(KaitaiStruct):
        SEQ_FIELDS = ["_unnamed0", "_unnamed1", "key", "next", "net_ptr", "unknown_1", "coords", "connection", "unknown_4", "unknown_5"]
        def __init__(self, _io, _parent=None, _root=None):
            self._io = _io
            self._parent = _parent
            self._root = _root if _root else self
            self._debug = collections.defaultdict(dict)
            self._read()

        def _read(self):
            self._debug['_unnamed0']['start'] = self._io.pos()
            self._unnamed0 = self._io.read_u1()
            self._debug['_unnamed0']['end'] = self._io.pos()
            self._debug['_unnamed1']['start'] = self._io.pos()
            self._unnamed1 = self._io.read_u2le()
            self._debug['_unnamed1']['end'] = self._io.pos()
            self._debug['key']['start'] = self._io.pos()
            self.key = self._io.read_u4le()
            self._debug['key']['end'] = self._io.pos()
            self._debug['next']['start'] = self._io.pos()
            self.next = self._io.read_u4le()
            self._debug['next']['end'] = self._io.pos()
            self._debug['net_ptr']['start'] = self._io.pos()
            self.net_ptr = self._io.read_u4le()
            self._debug['net_ptr']['end'] = self._io.pos()
            self._debug['unknown_1']['start'] = self._io.pos()
            self.unknown_1 = self._io.read_u4le()
            self._debug['unknown_1']['end'] = self._io.pos()
            self._debug['coords']['start'] = self._io.pos()
            self.coords = AllegroBrd.Coords(self._io, self, self._root)
            self._debug['coords']['end'] = self._io.pos()
            self._debug['connection']['start'] = self._io.pos()
            self.connection = self._io.read_u4le()
            self._debug['connection']['end'] = self._io.pos()
            self._debug['unknown_4']['start'] = self._io.pos()
            self.unknown_4 = self._io.read_u4le()
            self._debug['unknown_4']['end'] = self._io.pos()
            if self._root.ver >= 1311744:
                self._debug['unknown_5']['start'] = self._io.pos()
                self.unknown_5 = self._io.read_u4le()
                self._debug['unknown_5']['end'] = self._io.pos()



    class Type10(KaitaiStruct):
        SEQ_FIELDS = ["_unnamed0", "_unnamed1", "key", "_unnamed3", "ptr1", "_unnamed5", "ptr2", "un1", "str", "ptr4", "path_str"]
        def __init__(self, _io, _parent=None, _root=None):
            self._io = _io
            self._parent = _parent
            self._root = _root if _root else self
            self._debug = collections.defaultdict(dict)
            self._read()

        def _read(self):
            self._debug['_unnamed0']['start'] = self._io.pos()
            self._unnamed0 = self._io.read_u1()
            self._debug['_unnamed0']['end'] = self._io.pos()
            self._debug['_unnamed1']['start'] = self._io.pos()
            self._unnamed1 = self._io.read_u2le()
            self._debug['_unnamed1']['end'] = self._io.pos()
            self._debug['key']['start'] = self._io.pos()
            self.key = self._io.read_u4le()
            self._debug['key']['end'] = self._io.pos()
            if self._root.ver >= 1311744:
                self._debug['_unnamed3']['start'] = self._io.pos()
                self._unnamed3 = self._io.read_u4le()
                self._debug['_unnamed3']['end'] = self._io.pos()

            self._debug['ptr1']['start'] = self._io.pos()
            self.ptr1 = self._io.read_u4le()
            self._debug['ptr1']['end'] = self._io.pos()
            if self._root.ver >= 1313024:
                self._debug['_unnamed5']['start'] = self._io.pos()
                self._unnamed5 = self._io.read_u4le()
                self._debug['_unnamed5']['end'] = self._io.pos()

            self._debug['ptr2']['start'] = self._io.pos()
            self.ptr2 = self._io.read_u4le()
            self._debug['ptr2']['end'] = self._io.pos()
            self._debug['un1']['start'] = self._io.pos()
            self.un1 = self._io.read_u4le()
            self._debug['un1']['end'] = self._io.pos()
            self._debug['str']['start'] = self._io.pos()
            self.str = self._io.read_u4le()
            self._debug['str']['end'] = self._io.pos()
            self._debug['ptr4']['start'] = self._io.pos()
            self.ptr4 = self._io.read_u4le()
            self._debug['ptr4']['end'] = self._io.pos()
            self._debug['path_str']['start'] = self._io.pos()
            self.path_str = self._io.read_u4le()
            self._debug['path_str']['end'] = self._io.pos()


    class Type0c(KaitaiStruct):
        """Object that seems to represent a FIGURE in Allegro.
        
        For example, drill table marks, origin marks
        """
        SEQ_FIELDS = ["_unnamed0", "layer", "key", "next", "unknown_1", "unknown_2", "shape", "drill_char", "padding", "shape_16x", "drill_chars", "unknown_3", "unknown_4x", "coords", "size", "unknown_5", "unknown_6", "unknown_7", "unknown_8"]
        def __init__(self, _io, _parent=None, _root=None):
            self._io = _io
            self._parent = _parent
            self._root = _root if _root else self
            self._debug = collections.defaultdict(dict)
            self._read()

        def _read(self):
            self._debug['_unnamed0']['start'] = self._io.pos()
            self._unnamed0 = self._io.read_u1()
            self._debug['_unnamed0']['end'] = self._io.pos()
            self._debug['layer']['start'] = self._io.pos()
            self.layer = AllegroBrd.LayerInfo(self._io, self, self._root)
            self._debug['layer']['end'] = self._io.pos()
            self._debug['key']['start'] = self._io.pos()
            self.key = self._io.read_u4le()
            self._debug['key']['end'] = self._io.pos()
            self._debug['next']['start'] = self._io.pos()
            self.next = self._io.read_u4le()
            self._debug['next']['end'] = self._io.pos()
            self._debug['unknown_1']['start'] = self._io.pos()
            self.unknown_1 = self._io.read_u4le()
            self._debug['unknown_1']['end'] = self._io.pos()
            self._debug['unknown_2']['start'] = self._io.pos()
            self.unknown_2 = self._io.read_u4le()
            self._debug['unknown_2']['end'] = self._io.pos()
            if self._root.ver < 1311744:
                self._debug['shape']['start'] = self._io.pos()
                self.shape = self._io.read_u1()
                self._debug['shape']['end'] = self._io.pos()

            if self._root.ver < 1311744:
                self._debug['drill_char']['start'] = self._io.pos()
                self.drill_char = self._io.read_u1()
                self._debug['drill_char']['end'] = self._io.pos()

            if self._root.ver < 1311744:
                self._debug['padding']['start'] = self._io.pos()
                self.padding = self._io.read_u2le()
                self._debug['padding']['end'] = self._io.pos()

            if self._root.ver >= 1311744:
                self._debug['shape_16x']['start'] = self._io.pos()
                self.shape_16x = self._io.read_u4le()
                self._debug['shape_16x']['end'] = self._io.pos()

            if self._root.ver >= 1311744:
                self._debug['drill_chars']['start'] = self._io.pos()
                self.drill_chars = self._io.read_u4le()
                self._debug['drill_chars']['end'] = self._io.pos()

            if self._root.ver >= 1311744:
                self._debug['unknown_3']['start'] = self._io.pos()
                self.unknown_3 = self._io.read_u4le()
                self._debug['unknown_3']['end'] = self._io.pos()

            self._debug['unknown_4x']['start'] = self._io.pos()
            self.unknown_4x = self._io.read_u4le()
            self._debug['unknown_4x']['end'] = self._io.pos()
            self._debug['coords']['start'] = self._io.pos()
            self.coords = AllegroBrd.Coords(self._io, self, self._root)
            self._debug['coords']['end'] = self._io.pos()
            self._debug['size']['start'] = self._io.pos()
            self.size = AllegroBrd.Coords(self._io, self, self._root)
            self._debug['size']['end'] = self._io.pos()
            self._debug['unknown_5']['start'] = self._io.pos()
            self.unknown_5 = self._io.read_u4le()
            self._debug['unknown_5']['end'] = self._io.pos()
            self._debug['unknown_6']['start'] = self._io.pos()
            self.unknown_6 = self._io.read_u4le()
            self._debug['unknown_6']['end'] = self._io.pos()
            self._debug['unknown_7']['start'] = self._io.pos()
            self.unknown_7 = self._io.read_u4le()
            self._debug['unknown_7']['end'] = self._io.pos()
            if self._root.ver >= 1313024:
                self._debug['unknown_8']['start'] = self._io.pos()
                self.unknown_8 = self._io.read_u4le()
                self._debug['unknown_8']['end'] = self._io.pos()



    class LayerMap(KaitaiStruct):
        SEQ_FIELDS = ["entries"]
        def __init__(self, _io, _parent=None, _root=None):
            self._io = _io
            self._parent = _parent
            self._root = _root if _root else self
            self._debug = collections.defaultdict(dict)
            self._read()

        def _read(self):
            self._debug['entries']['start'] = self._io.pos()
            self.entries = []
            for i in range(25):
                if not 'arr' in self._debug['entries']:
                    self._debug['entries']['arr'] = []
                self._debug['entries']['arr'].append({'start': self._io.pos()})
                self.entries.append(AllegroBrd.LayerMap.Entry(self._io, self, self._root))
                self._debug['entries']['arr'][i]['end'] = self._io.pos()

            self._debug['entries']['end'] = self._io.pos()

        class Entry(KaitaiStruct):
            SEQ_FIELDS = ["a", "layer_list"]
            def __init__(self, _io, _parent=None, _root=None):
                self._io = _io
                self._parent = _parent
                self._root = _root if _root else self
                self._debug = collections.defaultdict(dict)
                self._read()

            def _read(self):
                self._debug['a']['start'] = self._io.pos()
                self.a = self._io.read_u4le()
                self._debug['a']['end'] = self._io.pos()
                self._debug['layer_list']['start'] = self._io.pos()
                self.layer_list = self._io.read_u4le()
                self._debug['layer_list']['end'] = self._io.pos()



    class Type01Arc(KaitaiStruct):
        SEQ_FIELDS = ["t1", "t2", "subtype", "key", "next", "parent", "unknown_1", "unknown_2", "width", "coords_0", "coords_1", "x", "y", "r", "bbox"]
        def __init__(self, _io, _parent=None, _root=None):
            self._io = _io
            self._parent = _parent
            self._root = _root if _root else self
            self._debug = collections.defaultdict(dict)
            self._read()

        def _read(self):
            self._debug['t1']['start'] = self._io.pos()
            self.t1 = self._io.read_u1()
            self._debug['t1']['end'] = self._io.pos()
            self._debug['t2']['start'] = self._io.pos()
            self.t2 = self._io.read_u1()
            self._debug['t2']['end'] = self._io.pos()
            self._debug['subtype']['start'] = self._io.pos()
            self.subtype = self._io.read_u1()
            self._debug['subtype']['end'] = self._io.pos()
            self._debug['key']['start'] = self._io.pos()
            self.key = self._io.read_u4le()
            self._debug['key']['end'] = self._io.pos()
            self._debug['next']['start'] = self._io.pos()
            self.next = self._io.read_u4le()
            self._debug['next']['end'] = self._io.pos()
            self._debug['parent']['start'] = self._io.pos()
            self.parent = self._io.read_u4le()
            self._debug['parent']['end'] = self._io.pos()
            self._debug['unknown_1']['start'] = self._io.pos()
            self.unknown_1 = self._io.read_u4le()
            self._debug['unknown_1']['end'] = self._io.pos()
            if self._root.ver >= 1311744:
                self._debug['unknown_2']['start'] = self._io.pos()
                self.unknown_2 = self._io.read_u4le()
                self._debug['unknown_2']['end'] = self._io.pos()

            self._debug['width']['start'] = self._io.pos()
            self.width = self._io.read_u4le()
            self._debug['width']['end'] = self._io.pos()
            self._debug['coords_0']['start'] = self._io.pos()
            self.coords_0 = AllegroBrd.Coords(self._io, self, self._root)
            self._debug['coords_0']['end'] = self._io.pos()
            self._debug['coords_1']['start'] = self._io.pos()
            self.coords_1 = AllegroBrd.Coords(self._io, self, self._root)
            self._debug['coords_1']['end'] = self._io.pos()
            self._debug['x']['start'] = self._io.pos()
            self.x = AllegroBrd.CadenceFp(self._io, self, self._root)
            self._debug['x']['end'] = self._io.pos()
            self._debug['y']['start'] = self._io.pos()
            self.y = AllegroBrd.CadenceFp(self._io, self, self._root)
            self._debug['y']['end'] = self._io.pos()
            self._debug['r']['start'] = self._io.pos()
            self.r = AllegroBrd.CadenceFp(self._io, self, self._root)
            self._debug['r']['end'] = self._io.pos()
            self._debug['bbox']['start'] = self._io.pos()
            self.bbox = []
            for i in range(4):
                if not 'arr' in self._debug['bbox']:
                    self._debug['bbox']['arr'] = []
                self._debug['bbox']['arr'].append({'start': self._io.pos()})
                self.bbox.append(self._io.read_s4le())
                self._debug['bbox']['arr'][i]['end'] = self._io.pos()

            self._debug['bbox']['end'] = self._io.pos()


    class Type26(KaitaiStruct):
        """Could be some kind of table/grouping assignment
        """
        SEQ_FIELDS = ["_unnamed0", "_unnamed1", "key", "member_ptr", "un", "group_ptr", "const_ptr", "un1"]
        def __init__(self, _io, _parent=None, _root=None):
            self._io = _io
            self._parent = _parent
            self._root = _root if _root else self
            self._debug = collections.defaultdict(dict)
            self._read()

        def _read(self):
            self._debug['_unnamed0']['start'] = self._io.pos()
            self._unnamed0 = self._io.read_u1()
            self._debug['_unnamed0']['end'] = self._io.pos()
            self._debug['_unnamed1']['start'] = self._io.pos()
            self._unnamed1 = self._io.read_u2le()
            self._debug['_unnamed1']['end'] = self._io.pos()
            self._debug['key']['start'] = self._io.pos()
            self.key = self._io.read_u4le()
            self._debug['key']['end'] = self._io.pos()
            self._debug['member_ptr']['start'] = self._io.pos()
            self.member_ptr = self._io.read_u4le()
            self._debug['member_ptr']['end'] = self._io.pos()
            if self._root.ver >= 1311744:
                self._debug['un']['start'] = self._io.pos()
                self.un = self._io.read_u4le()
                self._debug['un']['end'] = self._io.pos()

            self._debug['group_ptr']['start'] = self._io.pos()
            self.group_ptr = self._io.read_u4le()
            self._debug['group_ptr']['end'] = self._io.pos()
            self._debug['const_ptr']['start'] = self._io.pos()
            self.const_ptr = self._io.read_u4le()
            self._debug['const_ptr']['end'] = self._io.pos()
            if self._root.ver >= 1313024:
                self._debug['un1']['start'] = self._io.pos()
                self.un1 = self._io.read_u4le()
                self._debug['un1']['end'] = self._io.pos()



    class Type22(KaitaiStruct):
        SEQ_FIELDS = ["_unnamed0", "_unnamed1", "key", "un1", "un"]
        def __init__(self, _io, _parent=None, _root=None):
            self._io = _io
            self._parent = _parent
            self._root = _root if _root else self
            self._debug = collections.defaultdict(dict)
            self._read()

        def _read(self):
            self._debug['_unnamed0']['start'] = self._io.pos()
            self._unnamed0 = self._io.read_u1()
            self._debug['_unnamed0']['end'] = self._io.pos()
            self._debug['_unnamed1']['start'] = self._io.pos()
            self._unnamed1 = self._io.read_u2le()
            self._debug['_unnamed1']['end'] = self._io.pos()
            self._debug['key']['start'] = self._io.pos()
            self.key = self._io.read_u4le()
            self._debug['key']['end'] = self._io.pos()
            if self._root.ver >= 1311744:
                self._debug['un1']['start'] = self._io.pos()
                self.un1 = self._io.read_u4le()
                self._debug['un1']['end'] = self._io.pos()

            self._debug['un']['start'] = self._io.pos()
            self.un = []
            for i in range(8):
                if not 'arr' in self._debug['un']:
                    self._debug['un']['arr'] = []
                self._debug['un']['arr'].append({'start': self._io.pos()})
                self.un.append(self._io.read_u4le())
                self._debug['un']['arr'][i]['end'] = self._io.pos()

            self._debug['un']['end'] = self._io.pos()


    class Type38Film(KaitaiStruct):
        SEQ_FIELDS = ["_unnamed0", "_unnamed1", "key", "next", "layer_list", "film_name", "layer_name_str", "unknown_1", "unknown_2", "unknown_3"]
        def __init__(self, _io, _parent=None, _root=None):
            self._io = _io
            self._parent = _parent
            self._root = _root if _root else self
            self._debug = collections.defaultdict(dict)
            self._read()

        def _read(self):
            self._debug['_unnamed0']['start'] = self._io.pos()
            self._unnamed0 = self._io.read_u1()
            self._debug['_unnamed0']['end'] = self._io.pos()
            self._debug['_unnamed1']['start'] = self._io.pos()
            self._unnamed1 = self._io.read_u2le()
            self._debug['_unnamed1']['end'] = self._io.pos()
            self._debug['key']['start'] = self._io.pos()
            self.key = self._io.read_u4le()
            self._debug['key']['end'] = self._io.pos()
            self._debug['next']['start'] = self._io.pos()
            self.next = self._io.read_u4le()
            self._debug['next']['end'] = self._io.pos()
            self._debug['layer_list']['start'] = self._io.pos()
            self.layer_list = self._io.read_u4le()
            self._debug['layer_list']['end'] = self._io.pos()
            if self._root.ver < 1250560:
                self._debug['film_name']['start'] = self._io.pos()
                self.film_name = (self._io.read_bytes(20)).decode(u"ASCII")
                self._debug['film_name']['end'] = self._io.pos()

            if self._root.ver >= 1250560:
                self._debug['layer_name_str']['start'] = self._io.pos()
                self.layer_name_str = self._io.read_u4le()
                self._debug['layer_name_str']['end'] = self._io.pos()

            if self._root.ver >= 1250560:
                self._debug['unknown_1']['start'] = self._io.pos()
                self.unknown_1 = self._io.read_u4le()
                self._debug['unknown_1']['end'] = self._io.pos()

            self._debug['unknown_2']['start'] = self._io.pos()
            self.unknown_2 = []
            for i in range(7):
                if not 'arr' in self._debug['unknown_2']:
                    self._debug['unknown_2']['arr'] = []
                self._debug['unknown_2']['arr'].append({'start': self._io.pos()})
                self.unknown_2.append(self._io.read_u4le())
                self._debug['unknown_2']['arr'][i]['end'] = self._io.pos()

            self._debug['unknown_2']['end'] = self._io.pos()
            if self._root.ver >= 1313024:
                self._debug['unknown_3']['start'] = self._io.pos()
                self.unknown_3 = self._io.read_u4le()
                self._debug['unknown_3']['end'] = self._io.pos()



    class BoardObject(KaitaiStruct):
        """The basic object types that make up most of a .brd file.
        
        The occur in a long block after the header and string tables, and are
        joined end-to end. There is no length field, and the object sizes
        vary based on content and file version, so you have to parse the objects
        to find the next one. All objects seem to be 4-byte aligned.
        
        Every object starts with a type byte. Most objects have a field, usually
        named 'k' in this spec, and first word in the object after the type,
        that is a four-byte unique identifier for the object.
        This is often used by other objects to refer to this object via
        "pointers" - i.e. the 'k' value of the pointed-to object.
        """
        SEQ_FIELDS = ["type", "data"]
        def __init__(self, _io, _parent=None, _root=None):
            self._io = _io
            self._parent = _parent
            self._root = _root if _root else self
            self._debug = collections.defaultdict(dict)
            self._read()

        def _read(self):
            self._debug['type']['start'] = self._io.pos()
            self.type = self._io.read_u1()
            self._debug['type']['end'] = self._io.pos()
            self._debug['data']['start'] = self._io.pos()
            _on = self.type
            if _on == 14:
                self.data = AllegroBrd.Type0e(self._io, self, self._root)
            elif _on == 10:
                self.data = AllegroBrd.Type0aDrc(self._io, self, self._root)
            elif _on == 17:
                self.data = AllegroBrd.Type11(self._io, self, self._root)
            elif _on == 47:
                self.data = AllegroBrd.Type2f(self._io, self, self._root)
            elif _on == 4:
                self.data = AllegroBrd.Type04NetAssignment(self._io, self, self._root)
            elif _on == 42:
                self.data = AllegroBrd.Type2a(self._io, self, self._root)
            elif _on == 46:
                self.data = AllegroBrd.Type2e(self._io, self, self._root)
            elif _on == 39:
                self.data = AllegroBrd.Type27(self._io, self, self._root)
            elif _on == 60:
                self.data = AllegroBrd.Type3c(self._io, self, self._root)
            elif _on == 35:
                self.data = AllegroBrd.Type23Ratline(self._io, self, self._root)
            elif _on == 6:
                self.data = AllegroBrd.Type06(self._io, self, self._root)
            elif _on == 20:
                self.data = AllegroBrd.Type14(self._io, self, self._root)
            elif _on == 32:
                self.data = AllegroBrd.Type20(self._io, self, self._root)
            elif _on == 7:
                self.data = AllegroBrd.Type07(self._io, self, self._root)
            elif _on == 1:
                self.data = AllegroBrd.Type01Arc(self._io, self, self._root)
            elif _on == 55:
                self.data = AllegroBrd.Type37(self._io, self, self._root)
            elif _on == 27:
                self.data = AllegroBrd.Type1bNet(self._io, self, self._root)
            elif _on == 13:
                self.data = AllegroBrd.Type0dPad(self._io, self, self._root)
            elif _on == 52:
                self.data = AllegroBrd.Type34Keepout(self._io, self, self._root)
            elif _on == 56:
                self.data = AllegroBrd.Type38Film(self._io, self, self._root)
            elif _on == 45:
                self.data = AllegroBrd.Type2d(self._io, self, self._root)
            elif _on == 12:
                self.data = AllegroBrd.Type0c(self._io, self, self._root)
            elif _on == 59:
                self.data = AllegroBrd.Type3b(self._io, self, self._root)
            elif _on == 58:
                self.data = AllegroBrd.Type3aFilmListNode(self._io, self, self._root)
            elif _on == 3:
                self.data = AllegroBrd.Type03(self._io, self, self._root)
            elif _on == 5:
                self.data = AllegroBrd.Type05Track(self._io, self, self._root)
            elif _on == 33:
                self.data = AllegroBrd.Type21Header(self._io, self, self._root)
            elif _on == 51:
                self.data = AllegroBrd.Type33Via(self._io, self, self._root)
            elif _on == 23:
                self.data = AllegroBrd.Type151617Segment(self._io, self, self._root)
            elif _on == 48:
                self.data = AllegroBrd.Type30StrWrapper(self._io, self, self._root)
            elif _on == 53:
                self.data = AllegroBrd.Type35(self._io, self, self._root)
            elif _on == 15:
                self.data = AllegroBrd.Type0f(self._io, self, self._root)
            elif _on == 8:
                self.data = AllegroBrd.Type08(self._io, self, self._root)
            elif _on == 38:
                self.data = AllegroBrd.Type26(self._io, self, self._root)
            elif _on == 40:
                self.data = AllegroBrd.Type28Shape(self._io, self, self._root)
            elif _on == 44:
                self.data = AllegroBrd.Type2cGroup(self._io, self, self._root)
            elif _on == 57:
                self.data = AllegroBrd.Type39FilmLayerList(self._io, self, self._root)
            elif _on == 9:
                self.data = AllegroBrd.Type09(self._io, self, self._root)
            elif _on == 21:
                self.data = AllegroBrd.Type151617Segment(self._io, self, self._root)
            elif _on == 41:
                self.data = AllegroBrd.Type29(self._io, self, self._root)
            elif _on == 36:
                self.data = AllegroBrd.Type24Rect(self._io, self, self._root)
            elif _on == 28:
                self.data = AllegroBrd.Type1cPadStack(self._io, self, self._root)
            elif _on == 16:
                self.data = AllegroBrd.Type10(self._io, self, self._root)
            elif _on == 18:
                self.data = AllegroBrd.Type12(self._io, self, self._root)
            elif _on == 31:
                self.data = AllegroBrd.Type1f(self._io, self, self._root)
            elif _on == 49:
                self.data = AllegroBrd.Type31Sgraphic(self._io, self, self._root)
            elif _on == 34:
                self.data = AllegroBrd.Type22(self._io, self, self._root)
            elif _on == 54:
                self.data = AllegroBrd.Type36(self._io, self, self._root)
            elif _on == 29:
                self.data = AllegroBrd.Type1d(self._io, self, self._root)
            elif _on == 43:
                self.data = AllegroBrd.Type2b(self._io, self, self._root)
            elif _on == 50:
                self.data = AllegroBrd.Type32PlacedPad(self._io, self, self._root)
            elif _on == 22:
                self.data = AllegroBrd.Type151617Segment(self._io, self, self._root)
            elif _on == 30:
                self.data = AllegroBrd.Type1e(self._io, self, self._root)
            else:
                self.data = AllegroBrd.TypeDefault(self._io, self, self._root)
            self._debug['data']['end'] = self._io.pos()


    class Type1cPadStack(KaitaiStruct):

        class PadType(Enum):
            through_via = 0
            via = 1
            smt_pin = 2
            slot = 3
            npth = 8
            smt_pin2 = 10
        SEQ_FIELDS = ["_unnamed0", "n", "_unnamed2", "key", "next", "pad_str", "drill", "unknown_str", "pad_path", "unknown_3", "unknown_4", "unknown_5", "unknown_6", "pad_info", "unknown_7", "unknown_8", "unknown_9", "unknown_10", "layer_count", "unknown_11", "unknown_arr8", "unknown_arr28", "unknown_arr8_2", "components", "unk2", "unk3"]
        def __init__(self, _io, _parent=None, _root=None):
            self._io = _io
            self._parent = _parent
            self._root = _root if _root else self
            self._debug = collections.defaultdict(dict)
            self._read()

        def _read(self):
            self._debug['_unnamed0']['start'] = self._io.pos()
            self._unnamed0 = self._io.read_u1()
            self._debug['_unnamed0']['end'] = self._io.pos()
            self._debug['n']['start'] = self._io.pos()
            self.n = self._io.read_u1()
            self._debug['n']['end'] = self._io.pos()
            self._debug['_unnamed2']['start'] = self._io.pos()
            self._unnamed2 = self._io.read_u1()
            self._debug['_unnamed2']['end'] = self._io.pos()
            self._debug['key']['start'] = self._io.pos()
            self.key = self._io.read_u4le()
            self._debug['key']['end'] = self._io.pos()
            self._debug['next']['start'] = self._io.pos()
            self.next = self._io.read_u4le()
            self._debug['next']['end'] = self._io.pos()
            self._debug['pad_str']['start'] = self._io.pos()
            self.pad_str = self._io.read_u4le()
            self._debug['pad_str']['end'] = self._io.pos()
            self._debug['drill']['start'] = self._io.pos()
            self.drill = self._io.read_u4le()
            self._debug['drill']['end'] = self._io.pos()
            self._debug['unknown_str']['start'] = self._io.pos()
            self.unknown_str = self._io.read_u4le()
            self._debug['unknown_str']['end'] = self._io.pos()
            self._debug['pad_path']['start'] = self._io.pos()
            self.pad_path = self._io.read_u4le()
            self._debug['pad_path']['end'] = self._io.pos()
            if self._root.ver < 1311744:
                self._debug['unknown_3']['start'] = self._io.pos()
                self.unknown_3 = self._io.read_u4le()
                self._debug['unknown_3']['end'] = self._io.pos()

            if self._root.ver < 1311744:
                self._debug['unknown_4']['start'] = self._io.pos()
                self.unknown_4 = self._io.read_u4le()
                self._debug['unknown_4']['end'] = self._io.pos()

            if self._root.ver < 1311744:
                self._debug['unknown_5']['start'] = self._io.pos()
                self.unknown_5 = self._io.read_u4le()
                self._debug['unknown_5']['end'] = self._io.pos()

            if self._root.ver < 1311744:
                self._debug['unknown_6']['start'] = self._io.pos()
                self.unknown_6 = self._io.read_u4le()
                self._debug['unknown_6']['end'] = self._io.pos()

            self._debug['pad_info']['start'] = self._io.pos()
            self.pad_info = AllegroBrd.Type1cPadStack.PadInfo(self._io, self, self._root)
            self._debug['pad_info']['end'] = self._io.pos()
            if self._root.ver >= 1311744:
                self._debug['unknown_7']['start'] = self._io.pos()
                self.unknown_7 = self._io.read_u4le()
                self._debug['unknown_7']['end'] = self._io.pos()

            if self._root.ver >= 1311744:
                self._debug['unknown_8']['start'] = self._io.pos()
                self.unknown_8 = self._io.read_u4le()
                self._debug['unknown_8']['end'] = self._io.pos()

            if self._root.ver >= 1311744:
                self._debug['unknown_9']['start'] = self._io.pos()
                self.unknown_9 = self._io.read_u4le()
                self._debug['unknown_9']['end'] = self._io.pos()

            if self._root.ver < 1311744:
                self._debug['unknown_10']['start'] = self._io.pos()
                self.unknown_10 = self._io.read_u2le()
                self._debug['unknown_10']['end'] = self._io.pos()

            self._debug['layer_count']['start'] = self._io.pos()
            self.layer_count = self._io.read_u2le()
            self._debug['layer_count']['end'] = self._io.pos()
            if self._root.ver >= 1311744:
                self._debug['unknown_11']['start'] = self._io.pos()
                self.unknown_11 = self._io.read_u2le()
                self._debug['unknown_11']['end'] = self._io.pos()

            self._debug['unknown_arr8']['start'] = self._io.pos()
            self.unknown_arr8 = []
            for i in range(8):
                if not 'arr' in self._debug['unknown_arr8']:
                    self._debug['unknown_arr8']['arr'] = []
                self._debug['unknown_arr8']['arr'].append({'start': self._io.pos()})
                self.unknown_arr8.append(self._io.read_u4le())
                self._debug['unknown_arr8']['arr'][i]['end'] = self._io.pos()

            self._debug['unknown_arr8']['end'] = self._io.pos()
            if self._root.ver >= 1311744:
                self._debug['unknown_arr28']['start'] = self._io.pos()
                self.unknown_arr28 = []
                for i in range(28):
                    if not 'arr' in self._debug['unknown_arr28']:
                        self._debug['unknown_arr28']['arr'] = []
                    self._debug['unknown_arr28']['arr'].append({'start': self._io.pos()})
                    self.unknown_arr28.append(self._io.read_u4le())
                    self._debug['unknown_arr28']['arr'][i]['end'] = self._io.pos()

                self._debug['unknown_arr28']['end'] = self._io.pos()

            if (self._root.ver & 16773120) == 1249280:
                self._debug['unknown_arr8_2']['start'] = self._io.pos()
                self.unknown_arr8_2 = []
                for i in range(8):
                    if not 'arr' in self._debug['unknown_arr8_2']:
                        self._debug['unknown_arr8_2']['arr'] = []
                    self._debug['unknown_arr8_2']['arr'].append({'start': self._io.pos()})
                    self.unknown_arr8_2.append(self._io.read_u4le())
                    self._debug['unknown_arr8_2']['arr'][i]['end'] = self._io.pos()

                self._debug['unknown_arr8_2']['end'] = self._io.pos()

            self._debug['components']['start'] = self._io.pos()
            self.components = []
            for i in range(self.num_components):
                if not 'arr' in self._debug['components']:
                    self._debug['components']['arr'] = []
                self._debug['components']['arr'].append({'start': self._io.pos()})
                self.components.append(AllegroBrd.Type1cPadStack.PadstackComponent(self.num_components, i, self._io, self, self._root))
                self._debug['components']['arr'][i]['end'] = self._io.pos()

            self._debug['components']['end'] = self._io.pos()
            if self._root.ver < 1311744:
                self._debug['unk2']['start'] = self._io.pos()
                self.unk2 = []
                for i in range((self.n * 8)):
                    if not 'arr' in self._debug['unk2']:
                        self._debug['unk2']['arr'] = []
                    self._debug['unk2']['arr'].append({'start': self._io.pos()})
                    self.unk2.append(self._io.read_u4le())
                    self._debug['unk2']['arr'][i]['end'] = self._io.pos()

                self._debug['unk2']['end'] = self._io.pos()

            if self._root.ver >= 1311744:
                self._debug['unk3']['start'] = self._io.pos()
                self.unk3 = []
                for i in range((self.n * 10)):
                    if not 'arr' in self._debug['unk3']:
                        self._debug['unk3']['arr'] = []
                    self._debug['unk3']['arr'].append({'start': self._io.pos()})
                    self.unk3.append(self._io.read_u4le())
                    self._debug['unk3']['arr'][i]['end'] = self._io.pos()

                self._debug['unk3']['end'] = self._io.pos()


        class PadInfo(KaitaiStruct):
            SEQ_FIELDS = ["pad_type", "a", "b", "c", "d"]
            def __init__(self, _io, _parent=None, _root=None):
                self._io = _io
                self._parent = _parent
                self._root = _root if _root else self
                self._debug = collections.defaultdict(dict)
                self._read()

            def _read(self):
                self._debug['pad_type']['start'] = self._io.pos()
                self.pad_type = KaitaiStream.resolve_enum(AllegroBrd.Type1cPadStack.PadType, self._io.read_bits_int_be(4))
                self._debug['pad_type']['end'] = self._io.pos()
                self._debug['a']['start'] = self._io.pos()
                self.a = self._io.read_bits_int_be(4)
                self._debug['a']['end'] = self._io.pos()
                self._io.align_to_byte()
                self._debug['b']['start'] = self._io.pos()
                self.b = self._io.read_u1()
                self._debug['b']['end'] = self._io.pos()
                self._debug['c']['start'] = self._io.pos()
                self.c = self._io.read_u1()
                self._debug['c']['end'] = self._io.pos()
                self._debug['d']['start'] = self._io.pos()
                self.d = self._io.read_u1()
                self._debug['d']['end'] = self._io.pos()


        class PadstackComponent(KaitaiStruct):
            SEQ_FIELDS = ["t", "_unnamed1", "_unnamed2", "_unnamed3", "_unnamed4", "w", "h", "z1", "x3", "x4", "z", "str_ptr", "z2"]
            def __init__(self, total_count, index, _io, _parent=None, _root=None):
                self._io = _io
                self._parent = _parent
                self._root = _root if _root else self
                self.total_count = total_count
                self.index = index
                self._debug = collections.defaultdict(dict)
                self._read()

            def _read(self):
                self._debug['t']['start'] = self._io.pos()
                self.t = self._io.read_u1()
                self._debug['t']['end'] = self._io.pos()
                self._debug['_unnamed1']['start'] = self._io.pos()
                self._unnamed1 = self._io.read_u1()
                self._debug['_unnamed1']['end'] = self._io.pos()
                self._debug['_unnamed2']['start'] = self._io.pos()
                self._unnamed2 = self._io.read_u1()
                self._debug['_unnamed2']['end'] = self._io.pos()
                self._debug['_unnamed3']['start'] = self._io.pos()
                self._unnamed3 = self._io.read_u1()
                self._debug['_unnamed3']['end'] = self._io.pos()
                if self._root.ver >= 1311744:
                    self._debug['_unnamed4']['start'] = self._io.pos()
                    self._unnamed4 = self._io.read_u4le()
                    self._debug['_unnamed4']['end'] = self._io.pos()

                self._debug['w']['start'] = self._io.pos()
                self.w = self._io.read_s4le()
                self._debug['w']['end'] = self._io.pos()
                self._debug['h']['start'] = self._io.pos()
                self.h = self._io.read_s4le()
                self._debug['h']['end'] = self._io.pos()
                if self._root.ver >= 1311744:
                    self._debug['z1']['start'] = self._io.pos()
                    self.z1 = self._io.read_u4le()
                    self._debug['z1']['end'] = self._io.pos()

                self._debug['x3']['start'] = self._io.pos()
                self.x3 = self._io.read_s4le()
                self._debug['x3']['end'] = self._io.pos()
                self._debug['x4']['start'] = self._io.pos()
                self.x4 = self._io.read_s4le()
                self._debug['x4']['end'] = self._io.pos()
                if self._root.ver >= 1311744:
                    self._debug['z']['start'] = self._io.pos()
                    self.z = self._io.read_u4le()
                    self._debug['z']['end'] = self._io.pos()

                self._debug['str_ptr']['start'] = self._io.pos()
                self.str_ptr = self._io.read_u4le()
                self._debug['str_ptr']['end'] = self._io.pos()
                if  ((self._root.ver < 1311744) and (not ((self.total_count - 1) == self.index))) :
                    self._debug['z2']['start'] = self._io.pos()
                    self.z2 = self._io.read_u4le()
                    self._debug['z2']['end'] = self._io.pos()



        @property
        def num_components(self):
            if hasattr(self, '_m_num_components'):
                return self._m_num_components

            self._m_num_components = ((10 + (self.layer_count * 3)) if self._root.ver < 1311744 else (21 + (self.layer_count * 4)))
            return getattr(self, '_m_num_components', None)


    class Type11(KaitaiStruct):
        SEQ_FIELDS = ["_unnamed0", "_unnamed1", "key", "name", "ptr1", "ptr2", "unknown_1", "unknown_2"]
        def __init__(self, _io, _parent=None, _root=None):
            self._io = _io
            self._parent = _parent
            self._root = _root if _root else self
            self._debug = collections.defaultdict(dict)
            self._read()

        def _read(self):
            self._debug['_unnamed0']['start'] = self._io.pos()
            self._unnamed0 = self._io.read_u1()
            self._debug['_unnamed0']['end'] = self._io.pos()
            self._debug['_unnamed1']['start'] = self._io.pos()
            self._unnamed1 = self._io.read_u2le()
            self._debug['_unnamed1']['end'] = self._io.pos()
            self._debug['key']['start'] = self._io.pos()
            self.key = self._io.read_u4le()
            self._debug['key']['end'] = self._io.pos()
            self._debug['name']['start'] = self._io.pos()
            self.name = self._io.read_u4le()
            self._debug['name']['end'] = self._io.pos()
            self._debug['ptr1']['start'] = self._io.pos()
            self.ptr1 = self._io.read_u4le()
            self._debug['ptr1']['end'] = self._io.pos()
            self._debug['ptr2']['start'] = self._io.pos()
            self.ptr2 = self._io.read_u4le()
            self._debug['ptr2']['end'] = self._io.pos()
            self._debug['unknown_1']['start'] = self._io.pos()
            self.unknown_1 = self._io.read_u4le()
            self._debug['unknown_1']['end'] = self._io.pos()
            if self._root.ver >= 1313024:
                self._debug['unknown_2']['start'] = self._io.pos()
                self.unknown_2 = self._io.read_u4le()
                self._debug['unknown_2']['end'] = self._io.pos()



    class Type21Header(KaitaiStruct):
        SEQ_FIELDS = ["t", "r", "size", "key", "_unnamed4"]
        def __init__(self, _io, _parent=None, _root=None):
            self._io = _io
            self._parent = _parent
            self._root = _root if _root else self
            self._debug = collections.defaultdict(dict)
            self._read()

        def _read(self):
            self._debug['t']['start'] = self._io.pos()
            self.t = self._io.read_u1()
            self._debug['t']['end'] = self._io.pos()
            self._debug['r']['start'] = self._io.pos()
            self.r = self._io.read_u2le()
            self._debug['r']['end'] = self._io.pos()
            self._debug['size']['start'] = self._io.pos()
            self.size = self._io.read_u4le()
            self._debug['size']['end'] = self._io.pos()
            self._debug['key']['start'] = self._io.pos()
            self.key = self._io.read_u4le()
            self._debug['key']['end'] = self._io.pos()
            self._debug['_unnamed4']['start'] = self._io.pos()
            self._unnamed4 = []
            for i in range((self.size - 12)):
                if not 'arr' in self._debug['_unnamed4']:
                    self._debug['_unnamed4']['arr'] = []
                self._debug['_unnamed4']['arr'].append({'start': self._io.pos()})
                self._unnamed4.append(self._io.read_u1())
                self._debug['_unnamed4']['arr'][i]['end'] = self._io.pos()

            self._debug['_unnamed4']['end'] = self._io.pos()


    class Type03(KaitaiStruct):
        SEQ_FIELDS = ["_unnamed0", "_unnamed1", "key", "next", "unknown_1", "subtype", "unknown_hdr", "size", "unknown_2", "data"]
        def __init__(self, _io, _parent=None, _root=None):
            self._io = _io
            self._parent = _parent
            self._root = _root if _root else self
            self._debug = collections.defaultdict(dict)
            self._read()

        def _read(self):
            self._debug['_unnamed0']['start'] = self._io.pos()
            self._unnamed0 = self._io.read_u1()
            self._debug['_unnamed0']['end'] = self._io.pos()
            self._debug['_unnamed1']['start'] = self._io.pos()
            self._unnamed1 = self._io.read_u2le()
            self._debug['_unnamed1']['end'] = self._io.pos()
            self._debug['key']['start'] = self._io.pos()
            self.key = self._io.read_u4le()
            self._debug['key']['end'] = self._io.pos()
            self._debug['next']['start'] = self._io.pos()
            self.next = self._io.read_u4le()
            self._debug['next']['end'] = self._io.pos()
            if self._root.ver >= 1311744:
                self._debug['unknown_1']['start'] = self._io.pos()
                self.unknown_1 = self._io.read_u4le()
                self._debug['unknown_1']['end'] = self._io.pos()

            self._debug['subtype']['start'] = self._io.pos()
            self.subtype = self._io.read_u1()
            self._debug['subtype']['end'] = self._io.pos()
            self._debug['unknown_hdr']['start'] = self._io.pos()
            self.unknown_hdr = self._io.read_u1()
            self._debug['unknown_hdr']['end'] = self._io.pos()
            self._debug['size']['start'] = self._io.pos()
            self.size = self._io.read_u2le()
            self._debug['size']['end'] = self._io.pos()
            if self._root.ver >= 1311744:
                self._debug['unknown_2']['start'] = self._io.pos()
                self.unknown_2 = self._io.read_u4le()
                self._debug['unknown_2']['end'] = self._io.pos()

            if self.subtype != 101:
                self._debug['data']['start'] = self._io.pos()
                _on = self.subtype
                if _on == 120:
                    self.data = AllegroBrd.StringAligned(self.size, self._io, self, self._root)
                elif _on == 246:
                    self.data = AllegroBrd.Type03.F6Block(self._io, self, self._root)
                elif _on == 105:
                    self.data = AllegroBrd.Type03.EightBytes(self._io, self, self._root)
                elif _on == 112:
                    self.data = AllegroBrd.Type03.X70X74(self._io, self, self._root)
                elif _on == 116:
                    self.data = AllegroBrd.Type03.X70X74(self._io, self, self._root)
                elif _on == 113:
                    self.data = AllegroBrd.StringAligned(self.size, self._io, self, self._root)
                elif _on == 106:
                    self.data = AllegroBrd.Type03.FourBytes(self._io, self, self._root)
                elif _on == 100:
                    self.data = AllegroBrd.Type03.FourBytes(self._io, self, self._root)
                elif _on == 115:
                    self.data = AllegroBrd.StringAligned(self.size, self._io, self, self._root)
                elif _on == 107:
                    self.data = AllegroBrd.StringAligned(self.size, self._io, self, self._root)
                elif _on == 104:
                    self.data = AllegroBrd.StringAligned(self.size, self._io, self, self._root)
                elif _on == 103:
                    self.data = AllegroBrd.Type03.FourBytes(self._io, self, self._root)
                elif _on == 109:
                    self.data = AllegroBrd.StringAligned(self.size, self._io, self, self._root)
                elif _on == 108:
                    self.data = AllegroBrd.Type03.X6c(self._io, self, self._root)
                elif _on == 102:
                    self.data = AllegroBrd.Type03.FourBytes(self._io, self, self._root)
                elif _on == 110:
                    self.data = AllegroBrd.StringAligned(self.size, self._io, self, self._root)
                elif _on == 111:
                    self.data = AllegroBrd.StringAligned(self.size, self._io, self, self._root)
                self._debug['data']['end'] = self._io.pos()


        class EightBytes(KaitaiStruct):
            SEQ_FIELDS = ["val1", "val2"]
            def __init__(self, _io, _parent=None, _root=None):
                self._io = _io
                self._parent = _parent
                self._root = _root if _root else self
                self._debug = collections.defaultdict(dict)
                self._read()

            def _read(self):
                self._debug['val1']['start'] = self._io.pos()
                self.val1 = self._io.read_u4le()
                self._debug['val1']['end'] = self._io.pos()
                self._debug['val2']['start'] = self._io.pos()
                self.val2 = self._io.read_u4le()
                self._debug['val2']['end'] = self._io.pos()


        class X70X74(KaitaiStruct):
            SEQ_FIELDS = ["x0", "x1", "unk"]
            def __init__(self, _io, _parent=None, _root=None):
                self._io = _io
                self._parent = _parent
                self._root = _root if _root else self
                self._debug = collections.defaultdict(dict)
                self._read()

            def _read(self):
                self._debug['x0']['start'] = self._io.pos()
                self.x0 = self._io.read_u2le()
                self._debug['x0']['end'] = self._io.pos()
                self._debug['x1']['start'] = self._io.pos()
                self.x1 = self._io.read_u2le()
                self._debug['x1']['end'] = self._io.pos()
                self._debug['unk']['start'] = self._io.pos()
                self.unk = []
                for i in range((self.x1 + (4 * self.x0))):
                    if not 'arr' in self._debug['unk']:
                        self._debug['unk']['arr'] = []
                    self._debug['unk']['arr'].append({'start': self._io.pos()})
                    self.unk.append(self._io.read_u1())
                    self._debug['unk']['arr'][i]['end'] = self._io.pos()

                self._debug['unk']['end'] = self._io.pos()


        class FourBytes(KaitaiStruct):
            SEQ_FIELDS = ["val"]
            def __init__(self, _io, _parent=None, _root=None):
                self._io = _io
                self._parent = _parent
                self._root = _root if _root else self
                self._debug = collections.defaultdict(dict)
                self._read()

            def _read(self):
                self._debug['val']['start'] = self._io.pos()
                self.val = self._io.read_u4le()
                self._debug['val']['end'] = self._io.pos()


        class F6Block(KaitaiStruct):
            SEQ_FIELDS = ["_unnamed0"]
            def __init__(self, _io, _parent=None, _root=None):
                self._io = _io
                self._parent = _parent
                self._root = _root if _root else self
                self._debug = collections.defaultdict(dict)
                self._read()

            def _read(self):
                self._debug['_unnamed0']['start'] = self._io.pos()
                self._unnamed0 = []
                for i in range(20):
                    if not 'arr' in self._debug['_unnamed0']:
                        self._debug['_unnamed0']['arr'] = []
                    self._debug['_unnamed0']['arr'].append({'start': self._io.pos()})
                    self._unnamed0.append(self._io.read_u4le())
                    self._debug['_unnamed0']['arr'][i]['end'] = self._io.pos()

                self._debug['_unnamed0']['end'] = self._io.pos()


        class X6c(KaitaiStruct):
            SEQ_FIELDS = ["num_entries", "entries"]
            def __init__(self, _io, _parent=None, _root=None):
                self._io = _io
                self._parent = _parent
                self._root = _root if _root else self
                self._debug = collections.defaultdict(dict)
                self._read()

            def _read(self):
                self._debug['num_entries']['start'] = self._io.pos()
                self.num_entries = self._io.read_u4le()
                self._debug['num_entries']['end'] = self._io.pos()
                self._debug['entries']['start'] = self._io.pos()
                self.entries = []
                for i in range(self.num_entries):
                    if not 'arr' in self._debug['entries']:
                        self._debug['entries']['arr'] = []
                    self._debug['entries']['arr'].append({'start': self._io.pos()})
                    self.entries.append(self._io.read_u4le())
                    self._debug['entries']['arr'][i]['end'] = self._io.pos()

                self._debug['entries']['end'] = self._io.pos()



    class Type151617Segment(KaitaiStruct):
        """The difference between 15,16,17 seems to be:
        
        - 15: horizontal
        - 16: oblique
        - 17: vertical
        """
        SEQ_FIELDS = ["_unnamed0", "_unnamed1", "key", "next", "parent_ptr", "flags", "unknown_1", "width", "coords_0", "coords_1"]
        def __init__(self, _io, _parent=None, _root=None):
            self._io = _io
            self._parent = _parent
            self._root = _root if _root else self
            self._debug = collections.defaultdict(dict)
            self._read()

        def _read(self):
            self._debug['_unnamed0']['start'] = self._io.pos()
            self._unnamed0 = self._io.read_u1()
            self._debug['_unnamed0']['end'] = self._io.pos()
            self._debug['_unnamed1']['start'] = self._io.pos()
            self._unnamed1 = self._io.read_u2le()
            self._debug['_unnamed1']['end'] = self._io.pos()
            self._debug['key']['start'] = self._io.pos()
            self.key = self._io.read_u4le()
            self._debug['key']['end'] = self._io.pos()
            self._debug['next']['start'] = self._io.pos()
            self.next = self._io.read_u4le()
            self._debug['next']['end'] = self._io.pos()
            self._debug['parent_ptr']['start'] = self._io.pos()
            self.parent_ptr = self._io.read_u4le()
            self._debug['parent_ptr']['end'] = self._io.pos()
            self._debug['flags']['start'] = self._io.pos()
            self.flags = self._io.read_u4le()
            self._debug['flags']['end'] = self._io.pos()
            if self._root.ver >= 1311744:
                self._debug['unknown_1']['start'] = self._io.pos()
                self.unknown_1 = self._io.read_u4le()
                self._debug['unknown_1']['end'] = self._io.pos()

            self._debug['width']['start'] = self._io.pos()
            self.width = self._io.read_u4le()
            self._debug['width']['end'] = self._io.pos()
            self._debug['coords_0']['start'] = self._io.pos()
            self.coords_0 = AllegroBrd.Coords(self._io, self, self._root)
            self._debug['coords_0']['end'] = self._io.pos()
            self._debug['coords_1']['start'] = self._io.pos()
            self.coords_1 = AllegroBrd.Coords(self._io, self, self._root)
            self._debug['coords_1']['end'] = self._io.pos()


    class Type3aFilmListNode(KaitaiStruct):
        SEQ_FIELDS = ["t", "layer", "key", "next", "unknown_1", "unknown_2"]
        def __init__(self, _io, _parent=None, _root=None):
            self._io = _io
            self._parent = _parent
            self._root = _root if _root else self
            self._debug = collections.defaultdict(dict)
            self._read()

        def _read(self):
            self._debug['t']['start'] = self._io.pos()
            self.t = self._io.read_u1()
            self._debug['t']['end'] = self._io.pos()
            self._debug['layer']['start'] = self._io.pos()
            self.layer = AllegroBrd.LayerInfo(self._io, self, self._root)
            self._debug['layer']['end'] = self._io.pos()
            self._debug['key']['start'] = self._io.pos()
            self.key = self._io.read_u4le()
            self._debug['key']['end'] = self._io.pos()
            self._debug['next']['start'] = self._io.pos()
            self.next = self._io.read_u4le()
            self._debug['next']['end'] = self._io.pos()
            self._debug['unknown_1']['start'] = self._io.pos()
            self.unknown_1 = self._io.read_u4le()
            self._debug['unknown_1']['end'] = self._io.pos()
            if self._root.ver >= 1313024:
                self._debug['unknown_2']['start'] = self._io.pos()
                self.unknown_2 = self._io.read_u4le()
                self._debug['unknown_2']['end'] = self._io.pos()



    class Type36(KaitaiStruct):
        SEQ_FIELDS = ["t", "c", "key", "next", "un1", "num_items", "count", "last_idx", "un3", "un2", "items"]
        def __init__(self, _io, _parent=None, _root=None):
            self._io = _io
            self._parent = _parent
            self._root = _root if _root else self
            self._debug = collections.defaultdict(dict)
            self._read()

        def _read(self):
            self._debug['t']['start'] = self._io.pos()
            self.t = self._io.read_u1()
            self._debug['t']['end'] = self._io.pos()
            self._debug['c']['start'] = self._io.pos()
            self.c = self._io.read_u2le()
            self._debug['c']['end'] = self._io.pos()
            self._debug['key']['start'] = self._io.pos()
            self.key = self._io.read_u4le()
            self._debug['key']['end'] = self._io.pos()
            self._debug['next']['start'] = self._io.pos()
            self.next = self._io.read_u4le()
            self._debug['next']['end'] = self._io.pos()
            if self._root.ver >= 1311744:
                self._debug['un1']['start'] = self._io.pos()
                self.un1 = self._io.read_u4le()
                self._debug['un1']['end'] = self._io.pos()

            self._debug['num_items']['start'] = self._io.pos()
            self.num_items = self._io.read_u4le()
            self._debug['num_items']['end'] = self._io.pos()
            self._debug['count']['start'] = self._io.pos()
            self.count = self._io.read_u4le()
            self._debug['count']['end'] = self._io.pos()
            self._debug['last_idx']['start'] = self._io.pos()
            self.last_idx = self._io.read_u4le()
            self._debug['last_idx']['end'] = self._io.pos()
            self._debug['un3']['start'] = self._io.pos()
            self.un3 = self._io.read_u4le()
            self._debug['un3']['end'] = self._io.pos()
            if self._root.ver >= 1313024:
                self._debug['un2']['start'] = self._io.pos()
                self.un2 = self._io.read_u4le()
                self._debug['un2']['end'] = self._io.pos()

            self._debug['items']['start'] = self._io.pos()
            self.items = []
            for i in range(self.num_items):
                if not 'arr' in self._debug['items']:
                    self._debug['items']['arr'] = []
                self._debug['items']['arr'].append({'start': self._io.pos()})
                _on = self.c
                if _on == 6:
                    if not 'arr' in self._debug['items']:
                        self._debug['items']['arr'] = []
                    self._debug['items']['arr'].append({'start': self._io.pos()})
                    self.items.append(AllegroBrd.Type36.X06(self._io, self, self._root))
                    self._debug['items']['arr'][i]['end'] = self._io.pos()
                elif _on == 13:
                    if not 'arr' in self._debug['items']:
                        self._debug['items']['arr'] = []
                    self._debug['items']['arr'].append({'start': self._io.pos()})
                    self.items.append(AllegroBrd.Type36.X0d(self._io, self, self._root))
                    self._debug['items']['arr'][i]['end'] = self._io.pos()
                elif _on == 11:
                    if not 'arr' in self._debug['items']:
                        self._debug['items']['arr'] = []
                    self._debug['items']['arr'].append({'start': self._io.pos()})
                    self.items.append(AllegroBrd.Type36.X0b(self._io, self, self._root))
                    self._debug['items']['arr'][i]['end'] = self._io.pos()
                elif _on == 12:
                    if not 'arr' in self._debug['items']:
                        self._debug['items']['arr'] = []
                    self._debug['items']['arr'].append({'start': self._io.pos()})
                    self.items.append(AllegroBrd.Type36.X0c(self._io, self, self._root))
                    self._debug['items']['arr'][i]['end'] = self._io.pos()
                elif _on == 3:
                    if not 'arr' in self._debug['items']:
                        self._debug['items']['arr'] = []
                    self._debug['items']['arr'].append({'start': self._io.pos()})
                    self.items.append(AllegroBrd.Type36.X03(self._io, self, self._root))
                    self._debug['items']['arr'][i]['end'] = self._io.pos()
                elif _on == 5:
                    if not 'arr' in self._debug['items']:
                        self._debug['items']['arr'] = []
                    self._debug['items']['arr'].append({'start': self._io.pos()})
                    self.items.append(AllegroBrd.Type36.X05(self._io, self, self._root))
                    self._debug['items']['arr'][i]['end'] = self._io.pos()
                elif _on == 15:
                    if not 'arr' in self._debug['items']:
                        self._debug['items']['arr'] = []
                    self._debug['items']['arr'].append({'start': self._io.pos()})
                    self.items.append(AllegroBrd.Type36.X0f(self._io, self, self._root))
                    self._debug['items']['arr'][i]['end'] = self._io.pos()
                elif _on == 8:
                    if not 'arr' in self._debug['items']:
                        self._debug['items']['arr'] = []
                    self._debug['items']['arr'].append({'start': self._io.pos()})
                    self.items.append(AllegroBrd.Type36.X08(self._io, self, self._root))
                    self._debug['items']['arr'][i]['end'] = self._io.pos()
                elif _on == 16:
                    if not 'arr' in self._debug['items']:
                        self._debug['items']['arr'] = []
                    self._debug['items']['arr'].append({'start': self._io.pos()})
                    self.items.append(AllegroBrd.Type36.X10(self._io, self, self._root))
                    self._debug['items']['arr'][i]['end'] = self._io.pos()
                elif _on == 2:
                    if not 'arr' in self._debug['items']:
                        self._debug['items']['arr'] = []
                    self._debug['items']['arr'].append({'start': self._io.pos()})
                    self.items.append(AllegroBrd.Type36.X02(self._io, self, self._root))
                    self._debug['items']['arr'][i]['end'] = self._io.pos()
                self._debug['items']['arr'][i]['end'] = self._io.pos()

            self._debug['items']['end'] = self._io.pos()

        class X05(KaitaiStruct):
            SEQ_FIELDS = ["unk"]
            def __init__(self, _io, _parent=None, _root=None):
                self._io = _io
                self._parent = _parent
                self._root = _root if _root else self
                self._debug = collections.defaultdict(dict)
                self._read()

            def _read(self):
                self._debug['unk']['start'] = self._io.pos()
                self.unk = []
                for i in range(28):
                    if not 'arr' in self._debug['unk']:
                        self._debug['unk']['arr'] = []
                    self._debug['unk']['arr'].append({'start': self._io.pos()})
                    self.unk.append(self._io.read_u1())
                    self._debug['unk']['arr'][i]['end'] = self._io.pos()

                self._debug['unk']['end'] = self._io.pos()


        class X08(KaitaiStruct):
            SEQ_FIELDS = ["a", "b", "char_height", "char_width", "unknown_1", "xs", "ys"]
            def __init__(self, _io, _parent=None, _root=None):
                self._io = _io
                self._parent = _parent
                self._root = _root if _root else self
                self._debug = collections.defaultdict(dict)
                self._read()

            def _read(self):
                self._debug['a']['start'] = self._io.pos()
                self.a = self._io.read_u4le()
                self._debug['a']['end'] = self._io.pos()
                self._debug['b']['start'] = self._io.pos()
                self.b = self._io.read_u4le()
                self._debug['b']['end'] = self._io.pos()
                self._debug['char_height']['start'] = self._io.pos()
                self.char_height = self._io.read_u4le()
                self._debug['char_height']['end'] = self._io.pos()
                self._debug['char_width']['start'] = self._io.pos()
                self.char_width = self._io.read_u4le()
                self._debug['char_width']['end'] = self._io.pos()
                if self._root.ver >= 1313024:
                    self._debug['unknown_1']['start'] = self._io.pos()
                    self.unknown_1 = self._io.read_u4le()
                    self._debug['unknown_1']['end'] = self._io.pos()

                self._debug['xs']['start'] = self._io.pos()
                self.xs = []
                for i in range(4):
                    if not 'arr' in self._debug['xs']:
                        self._debug['xs']['arr'] = []
                    self._debug['xs']['arr'].append({'start': self._io.pos()})
                    self.xs.append(self._io.read_u4le())
                    self._debug['xs']['arr'][i]['end'] = self._io.pos()

                self._debug['xs']['end'] = self._io.pos()
                if self._root.ver >= 1311744:
                    self._debug['ys']['start'] = self._io.pos()
                    self.ys = []
                    for i in range(8):
                        if not 'arr' in self._debug['ys']:
                            self._debug['ys']['arr'] = []
                        self._debug['ys']['arr'].append({'start': self._io.pos()})
                        self.ys.append(self._io.read_u4le())
                        self._debug['ys']['arr'][i]['end'] = self._io.pos()

                    self._debug['ys']['end'] = self._io.pos()



        class X0d(KaitaiStruct):
            SEQ_FIELDS = ["unk"]
            def __init__(self, _io, _parent=None, _root=None):
                self._io = _io
                self._parent = _parent
                self._root = _root if _root else self
                self._debug = collections.defaultdict(dict)
                self._read()

            def _read(self):
                self._debug['unk']['start'] = self._io.pos()
                self.unk = []
                for i in range(200):
                    if not 'arr' in self._debug['unk']:
                        self._debug['unk']['arr'] = []
                    self._debug['unk']['arr'].append({'start': self._io.pos()})
                    self.unk.append(self._io.read_u1())
                    self._debug['unk']['arr'][i]['end'] = self._io.pos()

                self._debug['unk']['end'] = self._io.pos()


        class X0f(KaitaiStruct):
            SEQ_FIELDS = ["key", "ptrs", "ptr2"]
            def __init__(self, _io, _parent=None, _root=None):
                self._io = _io
                self._parent = _parent
                self._root = _root if _root else self
                self._debug = collections.defaultdict(dict)
                self._read()

            def _read(self):
                self._debug['key']['start'] = self._io.pos()
                self.key = self._io.read_u4le()
                self._debug['key']['end'] = self._io.pos()
                self._debug['ptrs']['start'] = self._io.pos()
                self.ptrs = []
                for i in range(3):
                    if not 'arr' in self._debug['ptrs']:
                        self._debug['ptrs']['arr'] = []
                    self._debug['ptrs']['arr'].append({'start': self._io.pos()})
                    self.ptrs.append(self._io.read_u4le())
                    self._debug['ptrs']['arr'][i]['end'] = self._io.pos()

                self._debug['ptrs']['end'] = self._io.pos()
                self._debug['ptr2']['start'] = self._io.pos()
                self.ptr2 = self._io.read_u4le()
                self._debug['ptr2']['end'] = self._io.pos()


        class X0c(KaitaiStruct):
            SEQ_FIELDS = ["unk"]
            def __init__(self, _io, _parent=None, _root=None):
                self._io = _io
                self._parent = _parent
                self._root = _root if _root else self
                self._debug = collections.defaultdict(dict)
                self._read()

            def _read(self):
                self._debug['unk']['start'] = self._io.pos()
                self.unk = []
                for i in range(232):
                    if not 'arr' in self._debug['unk']:
                        self._debug['unk']['arr'] = []
                    self._debug['unk']['arr'].append({'start': self._io.pos()})
                    self.unk.append(self._io.read_u1())
                    self._debug['unk']['arr'][i]['end'] = self._io.pos()

                self._debug['unk']['end'] = self._io.pos()


        class X06(KaitaiStruct):
            SEQ_FIELDS = ["n", "r", "s", "un1", "un2"]
            def __init__(self, _io, _parent=None, _root=None):
                self._io = _io
                self._parent = _parent
                self._root = _root if _root else self
                self._debug = collections.defaultdict(dict)
                self._read()

            def _read(self):
                self._debug['n']['start'] = self._io.pos()
                self.n = self._io.read_u2le()
                self._debug['n']['end'] = self._io.pos()
                self._debug['r']['start'] = self._io.pos()
                self.r = self._io.read_u1()
                self._debug['r']['end'] = self._io.pos()
                self._debug['s']['start'] = self._io.pos()
                self.s = self._io.read_u1()
                self._debug['s']['end'] = self._io.pos()
                self._debug['un1']['start'] = self._io.pos()
                self.un1 = self._io.read_u4le()
                self._debug['un1']['end'] = self._io.pos()
                if self._root.ver < 1311744:
                    self._debug['un2']['start'] = self._io.pos()
                    self.un2 = []
                    for i in range(50):
                        if not 'arr' in self._debug['un2']:
                            self._debug['un2']['arr'] = []
                        self._debug['un2']['arr'].append({'start': self._io.pos()})
                        self.un2.append(self._io.read_u4le())
                        self._debug['un2']['arr'][i]['end'] = self._io.pos()

                    self._debug['un2']['end'] = self._io.pos()



        class X02(KaitaiStruct):
            SEQ_FIELDS = ["str", "xs", "ys", "zs"]
            def __init__(self, _io, _parent=None, _root=None):
                self._io = _io
                self._parent = _parent
                self._root = _root if _root else self
                self._debug = collections.defaultdict(dict)
                self._read()

            def _read(self):
                self._debug['str']['start'] = self._io.pos()
                self.str = (self._io.read_bytes(32)).decode(u"ASCII")
                self._debug['str']['end'] = self._io.pos()
                self._debug['xs']['start'] = self._io.pos()
                self.xs = []
                for i in range(14):
                    if not 'arr' in self._debug['xs']:
                        self._debug['xs']['arr'] = []
                    self._debug['xs']['arr'].append({'start': self._io.pos()})
                    self.xs.append(self._io.read_u4le())
                    self._debug['xs']['arr'][i]['end'] = self._io.pos()

                self._debug['xs']['end'] = self._io.pos()
                if self._root.ver >= 1248256:
                    self._debug['ys']['start'] = self._io.pos()
                    self.ys = []
                    for i in range(3):
                        if not 'arr' in self._debug['ys']:
                            self._debug['ys']['arr'] = []
                        self._debug['ys']['arr'].append({'start': self._io.pos()})
                        self.ys.append(self._io.read_u4le())
                        self._debug['ys']['arr'][i]['end'] = self._io.pos()

                    self._debug['ys']['end'] = self._io.pos()

                if self._root.ver >= 1311744:
                    self._debug['zs']['start'] = self._io.pos()
                    self.zs = []
                    for i in range(2):
                        if not 'arr' in self._debug['zs']:
                            self._debug['zs']['arr'] = []
                        self._debug['zs']['arr'].append({'start': self._io.pos()})
                        self.zs.append(self._io.read_u4le())
                        self._debug['zs']['arr'][i]['end'] = self._io.pos()

                    self._debug['zs']['end'] = self._io.pos()



        class X10(KaitaiStruct):
            SEQ_FIELDS = ["unk"]
            def __init__(self, _io, _parent=None, _root=None):
                self._io = _io
                self._parent = _parent
                self._root = _root if _root else self
                self._debug = collections.defaultdict(dict)
                self._read()

            def _read(self):
                self._debug['unk']['start'] = self._io.pos()
                self.unk = []
                for i in range(108):
                    if not 'arr' in self._debug['unk']:
                        self._debug['unk']['arr'] = []
                    self._debug['unk']['arr'].append({'start': self._io.pos()})
                    self.unk.append(self._io.read_u1())
                    self._debug['unk']['arr'][i]['end'] = self._io.pos()

                self._debug['unk']['end'] = self._io.pos()


        class X0b(KaitaiStruct):
            SEQ_FIELDS = ["unk"]
            def __init__(self, _io, _parent=None, _root=None):
                self._io = _io
                self._parent = _parent
                self._root = _root if _root else self
                self._debug = collections.defaultdict(dict)
                self._read()

            def _read(self):
                self._debug['unk']['start'] = self._io.pos()
                self.unk = []
                for i in range(1016):
                    if not 'arr' in self._debug['unk']:
                        self._debug['unk']['arr'] = []
                    self._debug['unk']['arr'].append({'start': self._io.pos()})
                    self.unk.append(self._io.read_u1())
                    self._debug['unk']['arr'][i]['end'] = self._io.pos()

                self._debug['unk']['end'] = self._io.pos()


        class X03(KaitaiStruct):
            SEQ_FIELDS = ["str", "str_16x", "un2"]
            def __init__(self, _io, _parent=None, _root=None):
                self._io = _io
                self._parent = _parent
                self._root = _root if _root else self
                self._debug = collections.defaultdict(dict)
                self._read()

            def _read(self):
                if self._root.ver >= 1311744:
                    self._debug['str']['start'] = self._io.pos()
                    self.str = []
                    for i in range(64):
                        if not 'arr' in self._debug['str']:
                            self._debug['str']['arr'] = []
                        self._debug['str']['arr'].append({'start': self._io.pos()})
                        self.str.append(self._io.read_u1())
                        self._debug['str']['arr'][i]['end'] = self._io.pos()

                    self._debug['str']['end'] = self._io.pos()

                if self._root.ver < 1311744:
                    self._debug['str_16x']['start'] = self._io.pos()
                    self.str_16x = []
                    for i in range(32):
                        if not 'arr' in self._debug['str_16x']:
                            self._debug['str_16x']['arr'] = []
                        self._debug['str_16x']['arr'].append({'start': self._io.pos()})
                        self.str_16x.append(self._io.read_u1())
                        self._debug['str_16x']['arr'][i]['end'] = self._io.pos()

                    self._debug['str_16x']['end'] = self._io.pos()

                if self._root.ver >= 1313024:
                    self._debug['un2']['start'] = self._io.pos()
                    self.un2 = self._io.read_u4le()
                    self._debug['un2']['end'] = self._io.pos()




    class Type2d(KaitaiStruct):
        SEQ_FIELDS = ["_unnamed0", "_unnamed1", "key", "next", "unknown_1", "inst_ref_16x", "unknown_2", "unknown_3", "unknown_4", "flags", "rotation", "coords", "inst_ref", "ptr_1", "first_pad_ptr", "ptr_2", "ptr_3", "ptr_4", "ptr_5", "ptr_6"]
        def __init__(self, _io, _parent=None, _root=None):
            self._io = _io
            self._parent = _parent
            self._root = _root if _root else self
            self._debug = collections.defaultdict(dict)
            self._read()

        def _read(self):
            self._debug['_unnamed0']['start'] = self._io.pos()
            self._unnamed0 = self._io.read_u1()
            self._debug['_unnamed0']['end'] = self._io.pos()
            self._debug['_unnamed1']['start'] = self._io.pos()
            self._unnamed1 = self._io.read_u2le()
            self._debug['_unnamed1']['end'] = self._io.pos()
            self._debug['key']['start'] = self._io.pos()
            self.key = self._io.read_u4le()
            self._debug['key']['end'] = self._io.pos()
            self._debug['next']['start'] = self._io.pos()
            self.next = self._io.read_u4le()
            self._debug['next']['end'] = self._io.pos()
            if self._root.ver >= 1311744:
                self._debug['unknown_1']['start'] = self._io.pos()
                self.unknown_1 = self._io.read_u4le()
                self._debug['unknown_1']['end'] = self._io.pos()

            if self._root.ver < 1311744:
                self._debug['inst_ref_16x']['start'] = self._io.pos()
                self.inst_ref_16x = self._io.read_u4le()
                self._debug['inst_ref_16x']['end'] = self._io.pos()

            self._debug['unknown_2']['start'] = self._io.pos()
            self.unknown_2 = self._io.read_u2le()
            self._debug['unknown_2']['end'] = self._io.pos()
            self._debug['unknown_3']['start'] = self._io.pos()
            self.unknown_3 = self._io.read_u2le()
            self._debug['unknown_3']['end'] = self._io.pos()
            if self._root.ver >= 1311744:
                self._debug['unknown_4']['start'] = self._io.pos()
                self.unknown_4 = self._io.read_u4le()
                self._debug['unknown_4']['end'] = self._io.pos()

            self._debug['flags']['start'] = self._io.pos()
            self.flags = self._io.read_bits_int_be(32)
            self._debug['flags']['end'] = self._io.pos()
            self._io.align_to_byte()
            self._debug['rotation']['start'] = self._io.pos()
            self.rotation = self._io.read_u4le()
            self._debug['rotation']['end'] = self._io.pos()
            self._debug['coords']['start'] = self._io.pos()
            self.coords = AllegroBrd.Coords(self._io, self, self._root)
            self._debug['coords']['end'] = self._io.pos()
            if self._root.ver >= 1311744:
                self._debug['inst_ref']['start'] = self._io.pos()
                self.inst_ref = self._io.read_u4le()
                self._debug['inst_ref']['end'] = self._io.pos()

            self._debug['ptr_1']['start'] = self._io.pos()
            self.ptr_1 = self._io.read_u4le()
            self._debug['ptr_1']['end'] = self._io.pos()
            self._debug['first_pad_ptr']['start'] = self._io.pos()
            self.first_pad_ptr = self._io.read_u4le()
            self._debug['first_pad_ptr']['end'] = self._io.pos()
            self._debug['ptr_2']['start'] = self._io.pos()
            self.ptr_2 = self._io.read_u4le()
            self._debug['ptr_2']['end'] = self._io.pos()
            self._debug['ptr_3']['start'] = self._io.pos()
            self.ptr_3 = self._io.read_u4le()
            self._debug['ptr_3']['end'] = self._io.pos()
            self._debug['ptr_4']['start'] = self._io.pos()
            self.ptr_4 = self._io.read_u4le()
            self._debug['ptr_4']['end'] = self._io.pos()
            self._debug['ptr_5']['start'] = self._io.pos()
            self.ptr_5 = self._io.read_u4le()
            self._debug['ptr_5']['end'] = self._io.pos()
            self._debug['ptr_6']['start'] = self._io.pos()
            self.ptr_6 = self._io.read_u4le()
            self._debug['ptr_6']['end'] = self._io.pos()


    class Type1e(KaitaiStruct):
        SEQ_FIELDS = ["_unnamed0", "_unnamed1", "key", "un1", "un2", "un3", "str_ptr", "size", "str", "_unnamed9"]
        def __init__(self, _io, _parent=None, _root=None):
            self._io = _io
            self._parent = _parent
            self._root = _root if _root else self
            self._debug = collections.defaultdict(dict)
            self._read()

        def _read(self):
            self._debug['_unnamed0']['start'] = self._io.pos()
            self._unnamed0 = self._io.read_u1()
            self._debug['_unnamed0']['end'] = self._io.pos()
            self._debug['_unnamed1']['start'] = self._io.pos()
            self._unnamed1 = self._io.read_u2le()
            self._debug['_unnamed1']['end'] = self._io.pos()
            self._debug['key']['start'] = self._io.pos()
            self.key = self._io.read_u4le()
            self._debug['key']['end'] = self._io.pos()
            self._debug['un1']['start'] = self._io.pos()
            self.un1 = self._io.read_u4le()
            self._debug['un1']['end'] = self._io.pos()
            if self._root.ver >= 1248256:
                self._debug['un2']['start'] = self._io.pos()
                self.un2 = self._io.read_u2le()
                self._debug['un2']['end'] = self._io.pos()

            if self._root.ver >= 1248256:
                self._debug['un3']['start'] = self._io.pos()
                self.un3 = self._io.read_u2le()
                self._debug['un3']['end'] = self._io.pos()

            self._debug['str_ptr']['start'] = self._io.pos()
            self.str_ptr = self._io.read_u4le()
            self._debug['str_ptr']['end'] = self._io.pos()
            self._debug['size']['start'] = self._io.pos()
            self.size = self._io.read_u4le()
            self._debug['size']['end'] = self._io.pos()
            self._debug['str']['start'] = self._io.pos()
            self.str = AllegroBrd.StringAligned(self.size, self._io, self, self._root)
            self._debug['str']['end'] = self._io.pos()
            if self._root.ver >= 1311744:
                self._debug['_unnamed9']['start'] = self._io.pos()
                self._unnamed9 = self._io.read_u4le()
                self._debug['_unnamed9']['end'] = self._io.pos()



    class Type04NetAssignment(KaitaiStruct):
        SEQ_FIELDS = ["_unnamed0", "_unnamed1", "key", "next", "net", "conn_item", "unknown_1"]
        def __init__(self, _io, _parent=None, _root=None):
            self._io = _io
            self._parent = _parent
            self._root = _root if _root else self
            self._debug = collections.defaultdict(dict)
            self._read()

        def _read(self):
            self._debug['_unnamed0']['start'] = self._io.pos()
            self._unnamed0 = self._io.read_u1()
            self._debug['_unnamed0']['end'] = self._io.pos()
            self._debug['_unnamed1']['start'] = self._io.pos()
            self._unnamed1 = self._io.read_u2le()
            self._debug['_unnamed1']['end'] = self._io.pos()
            self._debug['key']['start'] = self._io.pos()
            self.key = self._io.read_u4le()
            self._debug['key']['end'] = self._io.pos()
            self._debug['next']['start'] = self._io.pos()
            self.next = self._io.read_u4le()
            self._debug['next']['end'] = self._io.pos()
            self._debug['net']['start'] = self._io.pos()
            self.net = self._io.read_u4le()
            self._debug['net']['end'] = self._io.pos()
            self._debug['conn_item']['start'] = self._io.pos()
            self.conn_item = self._io.read_u4le()
            self._debug['conn_item']['end'] = self._io.pos()
            if self._root.ver >= 1313024:
                self._debug['unknown_1']['start'] = self._io.pos()
                self.unknown_1 = self._io.read_u4le()
                self._debug['unknown_1']['end'] = self._io.pos()



    class Type24Rect(KaitaiStruct):
        SEQ_FIELDS = ["t", "layer", "key", "next", "ptr1", "unknown_1", "unknown_2", "coords_0", "coords_1", "ptr2", "un"]
        def __init__(self, _io, _parent=None, _root=None):
            self._io = _io
            self._parent = _parent
            self._root = _root if _root else self
            self._debug = collections.defaultdict(dict)
            self._read()

        def _read(self):
            self._debug['t']['start'] = self._io.pos()
            self.t = self._io.read_u1()
            self._debug['t']['end'] = self._io.pos()
            self._debug['layer']['start'] = self._io.pos()
            self.layer = AllegroBrd.LayerInfo(self._io, self, self._root)
            self._debug['layer']['end'] = self._io.pos()
            self._debug['key']['start'] = self._io.pos()
            self.key = self._io.read_u4le()
            self._debug['key']['end'] = self._io.pos()
            self._debug['next']['start'] = self._io.pos()
            self.next = self._io.read_u4le()
            self._debug['next']['end'] = self._io.pos()
            self._debug['ptr1']['start'] = self._io.pos()
            self.ptr1 = self._io.read_u4le()
            self._debug['ptr1']['end'] = self._io.pos()
            self._debug['unknown_1']['start'] = self._io.pos()
            self.unknown_1 = self._io.read_u4le()
            self._debug['unknown_1']['end'] = self._io.pos()
            if self._root.ver >= 1311744:
                self._debug['unknown_2']['start'] = self._io.pos()
                self.unknown_2 = self._io.read_u4le()
                self._debug['unknown_2']['end'] = self._io.pos()

            self._debug['coords_0']['start'] = self._io.pos()
            self.coords_0 = AllegroBrd.Coords(self._io, self, self._root)
            self._debug['coords_0']['end'] = self._io.pos()
            self._debug['coords_1']['start'] = self._io.pos()
            self.coords_1 = AllegroBrd.Coords(self._io, self, self._root)
            self._debug['coords_1']['end'] = self._io.pos()
            self._debug['ptr2']['start'] = self._io.pos()
            self.ptr2 = self._io.read_u4le()
            self._debug['ptr2']['end'] = self._io.pos()
            self._debug['un']['start'] = self._io.pos()
            self.un = []
            for i in range(3):
                if not 'arr' in self._debug['un']:
                    self._debug['un']['arr'] = []
                self._debug['un']['arr'].append({'start': self._io.pos()})
                self.un.append(self._io.read_u4le())
                self._debug['un']['arr'][i]['end'] = self._io.pos()

            self._debug['un']['end'] = self._io.pos()


    class Type0dPad(KaitaiStruct):
        SEQ_FIELDS = ["_unnamed0", "_unnamed1", "key", "str_ptr", "next", "unknown_1", "coords", "padstack_ptr", "unknown_2", "unknown_3", "flags", "rotation"]
        def __init__(self, _io, _parent=None, _root=None):
            self._io = _io
            self._parent = _parent
            self._root = _root if _root else self
            self._debug = collections.defaultdict(dict)
            self._read()

        def _read(self):
            self._debug['_unnamed0']['start'] = self._io.pos()
            self._unnamed0 = self._io.read_u1()
            self._debug['_unnamed0']['end'] = self._io.pos()
            self._debug['_unnamed1']['start'] = self._io.pos()
            self._unnamed1 = self._io.read_u2le()
            self._debug['_unnamed1']['end'] = self._io.pos()
            self._debug['key']['start'] = self._io.pos()
            self.key = self._io.read_u4le()
            self._debug['key']['end'] = self._io.pos()
            self._debug['str_ptr']['start'] = self._io.pos()
            self.str_ptr = self._io.read_u4le()
            self._debug['str_ptr']['end'] = self._io.pos()
            self._debug['next']['start'] = self._io.pos()
            self.next = self._io.read_u4le()
            self._debug['next']['end'] = self._io.pos()
            if self._root.ver >= 1313024:
                self._debug['unknown_1']['start'] = self._io.pos()
                self.unknown_1 = self._io.read_u4le()
                self._debug['unknown_1']['end'] = self._io.pos()

            self._debug['coords']['start'] = self._io.pos()
            self.coords = AllegroBrd.Coords(self._io, self, self._root)
            self._debug['coords']['end'] = self._io.pos()
            self._debug['padstack_ptr']['start'] = self._io.pos()
            self.padstack_ptr = self._io.read_u4le()
            self._debug['padstack_ptr']['end'] = self._io.pos()
            self._debug['unknown_2']['start'] = self._io.pos()
            self.unknown_2 = self._io.read_u4le()
            self._debug['unknown_2']['end'] = self._io.pos()
            if self._root.ver >= 1311744:
                self._debug['unknown_3']['start'] = self._io.pos()
                self.unknown_3 = self._io.read_u4le()
                self._debug['unknown_3']['end'] = self._io.pos()

            self._debug['flags']['start'] = self._io.pos()
            self.flags = self._io.read_bits_int_be(32)
            self._debug['flags']['end'] = self._io.pos()
            self._io.align_to_byte()
            self._debug['rotation']['start'] = self._io.pos()
            self.rotation = self._io.read_u4le()
            self._debug['rotation']['end'] = self._io.pos()


    class Type09(KaitaiStruct):
        SEQ_FIELDS = ["_unnamed0", "_unnamed1", "key", "un1", "un3", "ptr1", "ptr2", "un2", "ptr3", "ptr4", "un4"]
        def __init__(self, _io, _parent=None, _root=None):
            self._io = _io
            self._parent = _parent
            self._root = _root if _root else self
            self._debug = collections.defaultdict(dict)
            self._read()

        def _read(self):
            self._debug['_unnamed0']['start'] = self._io.pos()
            self._unnamed0 = self._io.read_u1()
            self._debug['_unnamed0']['end'] = self._io.pos()
            self._debug['_unnamed1']['start'] = self._io.pos()
            self._unnamed1 = self._io.read_u2le()
            self._debug['_unnamed1']['end'] = self._io.pos()
            self._debug['key']['start'] = self._io.pos()
            self.key = self._io.read_u4le()
            self._debug['key']['end'] = self._io.pos()
            self._debug['un1']['start'] = self._io.pos()
            self.un1 = []
            for i in range(4):
                if not 'arr' in self._debug['un1']:
                    self._debug['un1']['arr'] = []
                self._debug['un1']['arr'].append({'start': self._io.pos()})
                self.un1.append(self._io.read_u4le())
                self._debug['un1']['arr'][i]['end'] = self._io.pos()

            self._debug['un1']['end'] = self._io.pos()
            if self._root.ver >= 1311744:
                self._debug['un3']['start'] = self._io.pos()
                self.un3 = self._io.read_u4le()
                self._debug['un3']['end'] = self._io.pos()

            self._debug['ptr1']['start'] = self._io.pos()
            self.ptr1 = self._io.read_u4le()
            self._debug['ptr1']['end'] = self._io.pos()
            self._debug['ptr2']['start'] = self._io.pos()
            self.ptr2 = self._io.read_u4le()
            self._debug['ptr2']['end'] = self._io.pos()
            self._debug['un2']['start'] = self._io.pos()
            self.un2 = self._io.read_u4le()
            self._debug['un2']['end'] = self._io.pos()
            self._debug['ptr3']['start'] = self._io.pos()
            self.ptr3 = self._io.read_u4le()
            self._debug['ptr3']['end'] = self._io.pos()
            self._debug['ptr4']['start'] = self._io.pos()
            self.ptr4 = self._io.read_u4le()
            self._debug['ptr4']['end'] = self._io.pos()
            if self._root.ver >= 1313024:
                self._debug['un4']['start'] = self._io.pos()
                self.un4 = self._io.read_u4le()
                self._debug['un4']['end'] = self._io.pos()



    class Type05Track(KaitaiStruct):
        SEQ_FIELDS = ["_unnamed0", "layer", "key", "next", "net_assignment", "ptr1", "unknown_2", "unknown_3", "ptr2a", "ptr2b", "unknown_4", "ptr3a", "ptr3b", "unknown_5a", "unknown_5b", "first_segment", "ptr5", "unknown_6"]
        def __init__(self, _io, _parent=None, _root=None):
            self._io = _io
            self._parent = _parent
            self._root = _root if _root else self
            self._debug = collections.defaultdict(dict)
            self._read()

        def _read(self):
            self._debug['_unnamed0']['start'] = self._io.pos()
            self._unnamed0 = self._io.read_u1()
            self._debug['_unnamed0']['end'] = self._io.pos()
            self._debug['layer']['start'] = self._io.pos()
            self.layer = AllegroBrd.LayerInfo(self._io, self, self._root)
            self._debug['layer']['end'] = self._io.pos()
            self._debug['key']['start'] = self._io.pos()
            self.key = self._io.read_u4le()
            self._debug['key']['end'] = self._io.pos()
            self._debug['next']['start'] = self._io.pos()
            self.next = self._io.read_u4le()
            self._debug['next']['end'] = self._io.pos()
            self._debug['net_assignment']['start'] = self._io.pos()
            self.net_assignment = self._io.read_u4le()
            self._debug['net_assignment']['end'] = self._io.pos()
            self._debug['ptr1']['start'] = self._io.pos()
            self.ptr1 = self._io.read_u4le()
            self._debug['ptr1']['end'] = self._io.pos()
            self._debug['unknown_2']['start'] = self._io.pos()
            self.unknown_2 = self._io.read_u4le()
            self._debug['unknown_2']['end'] = self._io.pos()
            self._debug['unknown_3']['start'] = self._io.pos()
            self.unknown_3 = self._io.read_u4le()
            self._debug['unknown_3']['end'] = self._io.pos()
            self._debug['ptr2a']['start'] = self._io.pos()
            self.ptr2a = self._io.read_u4le()
            self._debug['ptr2a']['end'] = self._io.pos()
            self._debug['ptr2b']['start'] = self._io.pos()
            self.ptr2b = self._io.read_u4le()
            self._debug['ptr2b']['end'] = self._io.pos()
            self._debug['unknown_4']['start'] = self._io.pos()
            self.unknown_4 = self._io.read_u4le()
            self._debug['unknown_4']['end'] = self._io.pos()
            self._debug['ptr3a']['start'] = self._io.pos()
            self.ptr3a = self._io.read_u4le()
            self._debug['ptr3a']['end'] = self._io.pos()
            self._debug['ptr3b']['start'] = self._io.pos()
            self.ptr3b = self._io.read_u4le()
            self._debug['ptr3b']['end'] = self._io.pos()
            if self._root.ver >= 1311744:
                self._debug['unknown_5a']['start'] = self._io.pos()
                self.unknown_5a = self._io.read_u4le()
                self._debug['unknown_5a']['end'] = self._io.pos()

            if self._root.ver >= 1311744:
                self._debug['unknown_5b']['start'] = self._io.pos()
                self.unknown_5b = self._io.read_u4le()
                self._debug['unknown_5b']['end'] = self._io.pos()

            self._debug['first_segment']['start'] = self._io.pos()
            self.first_segment = self._io.read_u4le()
            self._debug['first_segment']['end'] = self._io.pos()
            self._debug['ptr5']['start'] = self._io.pos()
            self.ptr5 = self._io.read_u4le()
            self._debug['ptr5']['end'] = self._io.pos()
            self._debug['unknown_6']['start'] = self._io.pos()
            self.unknown_6 = self._io.read_u4le()
            self._debug['unknown_6']['end'] = self._io.pos()


    class Type29(KaitaiStruct):
        SEQ_FIELDS = ["_unnamed0", "t", "key", "ptr1", "ptr2", "_unnamed5", "ptr3", "coord1", "coord2", "ptr_padstack", "_unnamed10", "ptr_x30", "_unnamed12", "_unnamed13", "_unnamed14"]
        def __init__(self, _io, _parent=None, _root=None):
            self._io = _io
            self._parent = _parent
            self._root = _root if _root else self
            self._debug = collections.defaultdict(dict)
            self._read()

        def _read(self):
            self._debug['_unnamed0']['start'] = self._io.pos()
            self._unnamed0 = self._io.read_u1()
            self._debug['_unnamed0']['end'] = self._io.pos()
            self._debug['t']['start'] = self._io.pos()
            self.t = self._io.read_u2le()
            self._debug['t']['end'] = self._io.pos()
            self._debug['key']['start'] = self._io.pos()
            self.key = self._io.read_u4le()
            self._debug['key']['end'] = self._io.pos()
            self._debug['ptr1']['start'] = self._io.pos()
            self.ptr1 = self._io.read_u4le()
            self._debug['ptr1']['end'] = self._io.pos()
            self._debug['ptr2']['start'] = self._io.pos()
            self.ptr2 = self._io.read_u4le()
            self._debug['ptr2']['end'] = self._io.pos()
            self._debug['_unnamed5']['start'] = self._io.pos()
            self._unnamed5 = self._io.read_u4le()
            self._debug['_unnamed5']['end'] = self._io.pos()
            self._debug['ptr3']['start'] = self._io.pos()
            self.ptr3 = self._io.read_u4le()
            self._debug['ptr3']['end'] = self._io.pos()
            self._debug['coord1']['start'] = self._io.pos()
            self.coord1 = self._io.read_s4le()
            self._debug['coord1']['end'] = self._io.pos()
            self._debug['coord2']['start'] = self._io.pos()
            self.coord2 = self._io.read_s4le()
            self._debug['coord2']['end'] = self._io.pos()
            self._debug['ptr_padstack']['start'] = self._io.pos()
            self.ptr_padstack = self._io.read_u4le()
            self._debug['ptr_padstack']['end'] = self._io.pos()
            self._debug['_unnamed10']['start'] = self._io.pos()
            self._unnamed10 = self._io.read_u4le()
            self._debug['_unnamed10']['end'] = self._io.pos()
            self._debug['ptr_x30']['start'] = self._io.pos()
            self.ptr_x30 = self._io.read_u4le()
            self._debug['ptr_x30']['end'] = self._io.pos()
            self._debug['_unnamed12']['start'] = self._io.pos()
            self._unnamed12 = self._io.read_u4le()
            self._debug['_unnamed12']['end'] = self._io.pos()
            self._debug['_unnamed13']['start'] = self._io.pos()
            self._unnamed13 = self._io.read_u4le()
            self._debug['_unnamed13']['end'] = self._io.pos()
            self._debug['_unnamed14']['start'] = self._io.pos()
            self._unnamed14 = self._io.read_u4le()
            self._debug['_unnamed14']['end'] = self._io.pos()


    class Type12(KaitaiStruct):
        SEQ_FIELDS = ["_unnamed0", "_unnamed1", "key", "next", "ptr_0x11", "pad_ptr", "unknown_1", "unknown_2", "unknown_3"]
        def __init__(self, _io, _parent=None, _root=None):
            self._io = _io
            self._parent = _parent
            self._root = _root if _root else self
            self._debug = collections.defaultdict(dict)
            self._read()

        def _read(self):
            self._debug['_unnamed0']['start'] = self._io.pos()
            self._unnamed0 = self._io.read_u1()
            self._debug['_unnamed0']['end'] = self._io.pos()
            self._debug['_unnamed1']['start'] = self._io.pos()
            self._unnamed1 = self._io.read_u2le()
            self._debug['_unnamed1']['end'] = self._io.pos()
            self._debug['key']['start'] = self._io.pos()
            self.key = self._io.read_u4le()
            self._debug['key']['end'] = self._io.pos()
            self._debug['next']['start'] = self._io.pos()
            self.next = self._io.read_u4le()
            self._debug['next']['end'] = self._io.pos()
            self._debug['ptr_0x11']['start'] = self._io.pos()
            self.ptr_0x11 = self._io.read_u4le()
            self._debug['ptr_0x11']['end'] = self._io.pos()
            self._debug['pad_ptr']['start'] = self._io.pos()
            self.pad_ptr = self._io.read_u4le()
            self._debug['pad_ptr']['end'] = self._io.pos()
            self._debug['unknown_1']['start'] = self._io.pos()
            self.unknown_1 = self._io.read_u4le()
            self._debug['unknown_1']['end'] = self._io.pos()
            if self._root.ver >= 1249280:
                self._debug['unknown_2']['start'] = self._io.pos()
                self.unknown_2 = self._io.read_u4le()
                self._debug['unknown_2']['end'] = self._io.pos()

            if self._root.ver >= 1313024:
                self._debug['unknown_3']['start'] = self._io.pos()
                self.unknown_3 = self._io.read_u4le()
                self._debug['unknown_3']['end'] = self._io.pos()



    class Type07(KaitaiStruct):
        SEQ_FIELDS = ["_unnamed0", "_unnamed1", "key", "_unnamed3", "ptr_1", "unknown_1", "unknown_2", "ptr_0x2d", "unknown_3", "ref_des_ref", "ptr_2", "ptr_3", "un3", "ptr_4"]
        def __init__(self, _io, _parent=None, _root=None):
            self._io = _io
            self._parent = _parent
            self._root = _root if _root else self
            self._debug = collections.defaultdict(dict)
            self._read()

        def _read(self):
            self._debug['_unnamed0']['start'] = self._io.pos()
            self._unnamed0 = self._io.read_u1()
            self._debug['_unnamed0']['end'] = self._io.pos()
            self._debug['_unnamed1']['start'] = self._io.pos()
            self._unnamed1 = self._io.read_u2le()
            self._debug['_unnamed1']['end'] = self._io.pos()
            self._debug['key']['start'] = self._io.pos()
            self.key = self._io.read_u4le()
            self._debug['key']['end'] = self._io.pos()
            self._debug['_unnamed3']['start'] = self._io.pos()
            self._unnamed3 = self._io.read_u4le()
            self._debug['_unnamed3']['end'] = self._io.pos()
            if self._root.ver >= 1311744:
                self._debug['ptr_1']['start'] = self._io.pos()
                self.ptr_1 = self._io.read_u4le()
                self._debug['ptr_1']['end'] = self._io.pos()

            if self._root.ver >= 1311744:
                self._debug['unknown_1']['start'] = self._io.pos()
                self.unknown_1 = self._io.read_u4le()
                self._debug['unknown_1']['end'] = self._io.pos()

            if self._root.ver >= 1311744:
                self._debug['unknown_2']['start'] = self._io.pos()
                self.unknown_2 = self._io.read_u4le()
                self._debug['unknown_2']['end'] = self._io.pos()

            self._debug['ptr_0x2d']['start'] = self._io.pos()
            self.ptr_0x2d = self._io.read_u4le()
            self._debug['ptr_0x2d']['end'] = self._io.pos()
            if self._root.ver < 1311744:
                self._debug['unknown_3']['start'] = self._io.pos()
                self.unknown_3 = self._io.read_u4le()
                self._debug['unknown_3']['end'] = self._io.pos()

            self._debug['ref_des_ref']['start'] = self._io.pos()
            self.ref_des_ref = self._io.read_u4le()
            self._debug['ref_des_ref']['end'] = self._io.pos()
            self._debug['ptr_2']['start'] = self._io.pos()
            self.ptr_2 = self._io.read_u4le()
            self._debug['ptr_2']['end'] = self._io.pos()
            self._debug['ptr_3']['start'] = self._io.pos()
            self.ptr_3 = self._io.read_u4le()
            self._debug['ptr_3']['end'] = self._io.pos()
            self._debug['un3']['start'] = self._io.pos()
            self.un3 = self._io.read_u4le()
            self._debug['un3']['end'] = self._io.pos()
            self._debug['ptr_4']['start'] = self._io.pos()
            self.ptr_4 = self._io.read_u4le()
            self._debug['ptr_4']['end'] = self._io.pos()


    class Type34Keepout(KaitaiStruct):
        SEQ_FIELDS = ["t", "layer", "key", "next", "ptr1", "un2", "flags", "ptr2", "ptr3", "un"]
        def __init__(self, _io, _parent=None, _root=None):
            self._io = _io
            self._parent = _parent
            self._root = _root if _root else self
            self._debug = collections.defaultdict(dict)
            self._read()

        def _read(self):
            self._debug['t']['start'] = self._io.pos()
            self.t = self._io.read_u1()
            self._debug['t']['end'] = self._io.pos()
            self._debug['layer']['start'] = self._io.pos()
            self.layer = AllegroBrd.LayerInfo(self._io, self, self._root)
            self._debug['layer']['end'] = self._io.pos()
            self._debug['key']['start'] = self._io.pos()
            self.key = self._io.read_u4le()
            self._debug['key']['end'] = self._io.pos()
            self._debug['next']['start'] = self._io.pos()
            self.next = self._io.read_u4le()
            self._debug['next']['end'] = self._io.pos()
            self._debug['ptr1']['start'] = self._io.pos()
            self.ptr1 = self._io.read_u4le()
            self._debug['ptr1']['end'] = self._io.pos()
            if self._root.ver >= 1311744:
                self._debug['un2']['start'] = self._io.pos()
                self.un2 = self._io.read_u4le()
                self._debug['un2']['end'] = self._io.pos()

            self._debug['flags']['start'] = self._io.pos()
            self.flags = self._io.read_bits_int_be(32)
            self._debug['flags']['end'] = self._io.pos()
            self._io.align_to_byte()
            self._debug['ptr2']['start'] = self._io.pos()
            self.ptr2 = self._io.read_u4le()
            self._debug['ptr2']['end'] = self._io.pos()
            self._debug['ptr3']['start'] = self._io.pos()
            self.ptr3 = self._io.read_u4le()
            self._debug['ptr3']['end'] = self._io.pos()
            self._debug['un']['start'] = self._io.pos()
            self.un = self._io.read_u4le()
            self._debug['un']['end'] = self._io.pos()


    class Type0f(KaitaiStruct):
        SEQ_FIELDS = ["_unnamed0", "_unnamed1", "key", "str_unk", "s", "ptr_x06", "ptr_x11", "unknown_1", "unknown_2", "unknown_3"]
        def __init__(self, _io, _parent=None, _root=None):
            self._io = _io
            self._parent = _parent
            self._root = _root if _root else self
            self._debug = collections.defaultdict(dict)
            self._read()

        def _read(self):
            self._debug['_unnamed0']['start'] = self._io.pos()
            self._unnamed0 = self._io.read_u1()
            self._debug['_unnamed0']['end'] = self._io.pos()
            self._debug['_unnamed1']['start'] = self._io.pos()
            self._unnamed1 = self._io.read_u2le()
            self._debug['_unnamed1']['end'] = self._io.pos()
            self._debug['key']['start'] = self._io.pos()
            self.key = self._io.read_u4le()
            self._debug['key']['end'] = self._io.pos()
            self._debug['str_unk']['start'] = self._io.pos()
            self.str_unk = self._io.read_u4le()
            self._debug['str_unk']['end'] = self._io.pos()
            self._debug['s']['start'] = self._io.pos()
            self.s = (self._io.read_bytes(32)).decode(u"ASCII")
            self._debug['s']['end'] = self._io.pos()
            self._debug['ptr_x06']['start'] = self._io.pos()
            self.ptr_x06 = self._io.read_u4le()
            self._debug['ptr_x06']['end'] = self._io.pos()
            self._debug['ptr_x11']['start'] = self._io.pos()
            self.ptr_x11 = self._io.read_u4le()
            self._debug['ptr_x11']['end'] = self._io.pos()
            self._debug['unknown_1']['start'] = self._io.pos()
            self.unknown_1 = self._io.read_u4le()
            self._debug['unknown_1']['end'] = self._io.pos()
            if self._root.ver >= 1311744:
                self._debug['unknown_2']['start'] = self._io.pos()
                self.unknown_2 = self._io.read_u4le()
                self._debug['unknown_2']['end'] = self._io.pos()

            if self._root.ver >= 1313024:
                self._debug['unknown_3']['start'] = self._io.pos()
                self.unknown_3 = self._io.read_u4le()
                self._debug['unknown_3']['end'] = self._io.pos()



    class Type39FilmLayerList(KaitaiStruct):
        SEQ_FIELDS = ["_unnamed0", "_unnamed1", "key", "parent", "head", "x"]
        def __init__(self, _io, _parent=None, _root=None):
            self._io = _io
            self._parent = _parent
            self._root = _root if _root else self
            self._debug = collections.defaultdict(dict)
            self._read()

        def _read(self):
            self._debug['_unnamed0']['start'] = self._io.pos()
            self._unnamed0 = self._io.read_u1()
            self._debug['_unnamed0']['end'] = self._io.pos()
            self._debug['_unnamed1']['start'] = self._io.pos()
            self._unnamed1 = self._io.read_u2le()
            self._debug['_unnamed1']['end'] = self._io.pos()
            self._debug['key']['start'] = self._io.pos()
            self.key = self._io.read_u4le()
            self._debug['key']['end'] = self._io.pos()
            self._debug['parent']['start'] = self._io.pos()
            self.parent = self._io.read_u4le()
            self._debug['parent']['end'] = self._io.pos()
            self._debug['head']['start'] = self._io.pos()
            self.head = self._io.read_u4le()
            self._debug['head']['end'] = self._io.pos()
            self._debug['x']['start'] = self._io.pos()
            self.x = []
            for i in range(22):
                if not 'arr' in self._debug['x']:
                    self._debug['x']['arr'] = []
                self._debug['x']['arr'].append({'start': self._io.pos()})
                self.x.append(self._io.read_u2le())
                self._debug['x']['arr'][i]['end'] = self._io.pos()

            self._debug['x']['end'] = self._io.pos()


    class StringAligned(KaitaiStruct):
        SEQ_FIELDS = ["chars", "_unnamed1"]
        def __init__(self, num_chars, _io, _parent=None, _root=None):
            self._io = _io
            self._parent = _parent
            self._root = _root if _root else self
            self.num_chars = num_chars
            self._debug = collections.defaultdict(dict)
            self._read()

        def _read(self):
            self._debug['chars']['start'] = self._io.pos()
            self.chars = []
            for i in range(self.num_chars):
                if not 'arr' in self._debug['chars']:
                    self._debug['chars']['arr'] = []
                self._debug['chars']['arr'].append({'start': self._io.pos()})
                self.chars.append(self._io.read_u1())
                self._debug['chars']['arr'][i]['end'] = self._io.pos()

            self._debug['chars']['end'] = self._io.pos()
            if (self._io.pos() % 4) != 0:
                self._debug['_unnamed1']['start'] = self._io.pos()
                self._unnamed1 = []
                i = 0
                while True:
                    if not 'arr' in self._debug['_unnamed1']:
                        self._debug['_unnamed1']['arr'] = []
                    self._debug['_unnamed1']['arr'].append({'start': self._io.pos()})
                    _ = self._io.read_u1()
                    self._unnamed1.append(_)
                    self._debug['_unnamed1']['arr'][len(self._unnamed1) - 1]['end'] = self._io.pos()
                    if (self._io.pos() % 4) == 0:
                        break
                    i += 1
                self._debug['_unnamed1']['end'] = self._io.pos()



    class Type1f(KaitaiStruct):
        SEQ_FIELDS = ["_unnamed0", "_unnamed1", "key", "_unnamed3", "un1", "size", "_unnamed6", "_unnamed7", "_unnamed8", "_unnamed9"]
        def __init__(self, _io, _parent=None, _root=None):
            self._io = _io
            self._parent = _parent
            self._root = _root if _root else self
            self._debug = collections.defaultdict(dict)
            self._read()

        def _read(self):
            self._debug['_unnamed0']['start'] = self._io.pos()
            self._unnamed0 = self._io.read_u1()
            self._debug['_unnamed0']['end'] = self._io.pos()
            self._debug['_unnamed1']['start'] = self._io.pos()
            self._unnamed1 = self._io.read_u2le()
            self._debug['_unnamed1']['end'] = self._io.pos()
            self._debug['key']['start'] = self._io.pos()
            self.key = self._io.read_u4le()
            self._debug['key']['end'] = self._io.pos()
            self._debug['_unnamed3']['start'] = self._io.pos()
            self._unnamed3 = []
            for i in range(4):
                if not 'arr' in self._debug['_unnamed3']:
                    self._debug['_unnamed3']['arr'] = []
                self._debug['_unnamed3']['arr'].append({'start': self._io.pos()})
                self._unnamed3.append(self._io.read_u4le())
                self._debug['_unnamed3']['arr'][i]['end'] = self._io.pos()

            self._debug['_unnamed3']['end'] = self._io.pos()
            self._debug['un1']['start'] = self._io.pos()
            self.un1 = self._io.read_u2le()
            self._debug['un1']['end'] = self._io.pos()
            self._debug['size']['start'] = self._io.pos()
            self.size = self._io.read_u2le()
            self._debug['size']['end'] = self._io.pos()
            if self._root.ver >= 1316096:
                self._debug['_unnamed6']['start'] = self._io.pos()
                self._unnamed6 = []
                for i in range(((self.size * 384) + 8)):
                    if not 'arr' in self._debug['_unnamed6']:
                        self._debug['_unnamed6']['arr'] = []
                    self._debug['_unnamed6']['arr'].append({'start': self._io.pos()})
                    self._unnamed6.append(self._io.read_u1())
                    self._debug['_unnamed6']['arr'][i]['end'] = self._io.pos()

                self._debug['_unnamed6']['end'] = self._io.pos()

            if  ((self._root.ver >= 1311744) and (self._root.ver < 1316096)) :
                self._debug['_unnamed7']['start'] = self._io.pos()
                self._unnamed7 = []
                for i in range(((self.size * 280) + 8)):
                    if not 'arr' in self._debug['_unnamed7']:
                        self._debug['_unnamed7']['arr'] = []
                    self._debug['_unnamed7']['arr'].append({'start': self._io.pos()})
                    self._unnamed7.append(self._io.read_u1())
                    self._debug['_unnamed7']['arr'][i]['end'] = self._io.pos()

                self._debug['_unnamed7']['end'] = self._io.pos()

            if  ((self._root.ver >= 1246208) and (self._root.ver < 1311744)) :
                self._debug['_unnamed8']['start'] = self._io.pos()
                self._unnamed8 = []
                for i in range(((self.size * 280) + 4)):
                    if not 'arr' in self._debug['_unnamed8']:
                        self._debug['_unnamed8']['arr'] = []
                    self._debug['_unnamed8']['arr'].append({'start': self._io.pos()})
                    self._unnamed8.append(self._io.read_u1())
                    self._debug['_unnamed8']['arr'][i]['end'] = self._io.pos()

                self._debug['_unnamed8']['end'] = self._io.pos()

            if self._root.ver < 1246208:
                self._debug['_unnamed9']['start'] = self._io.pos()
                self._unnamed9 = []
                for i in range(((self.size * 240) + 4)):
                    if not 'arr' in self._debug['_unnamed9']:
                        self._debug['_unnamed9']['arr'] = []
                    self._debug['_unnamed9']['arr'].append({'start': self._io.pos()})
                    self._unnamed9.append(self._io.read_u1())
                    self._debug['_unnamed9']['arr'][i]['end'] = self._io.pos()

                self._debug['_unnamed9']['end'] = self._io.pos()



    class Type28Shape(KaitaiStruct):
        SEQ_FIELDS = ["type", "layer", "key", "next", "parent", "unknown_1", "unknown_2", "unknown_3", "ptr2", "ptr3", "ptr4", "first_segment_ptr", "unknown_4", "unknown_5", "ptr5", "ptr6", "ptr7_16x", "coords_0", "coords_1"]
        def __init__(self, _io, _parent=None, _root=None):
            self._io = _io
            self._parent = _parent
            self._root = _root if _root else self
            self._debug = collections.defaultdict(dict)
            self._read()

        def _read(self):
            self._debug['type']['start'] = self._io.pos()
            self.type = self._io.read_u1()
            self._debug['type']['end'] = self._io.pos()
            self._debug['layer']['start'] = self._io.pos()
            self.layer = AllegroBrd.LayerInfo(self._io, self, self._root)
            self._debug['layer']['end'] = self._io.pos()
            self._debug['key']['start'] = self._io.pos()
            self.key = self._io.read_u4le()
            self._debug['key']['end'] = self._io.pos()
            self._debug['next']['start'] = self._io.pos()
            self.next = self._io.read_u4le()
            self._debug['next']['end'] = self._io.pos()
            self._debug['parent']['start'] = self._io.pos()
            self.parent = self._io.read_u4le()
            self._debug['parent']['end'] = self._io.pos()
            self._debug['unknown_1']['start'] = self._io.pos()
            self.unknown_1 = self._io.read_u4le()
            self._debug['unknown_1']['end'] = self._io.pos()
            if self._root.ver >= 1311744:
                self._debug['unknown_2']['start'] = self._io.pos()
                self.unknown_2 = self._io.read_u4le()
                self._debug['unknown_2']['end'] = self._io.pos()

            if self._root.ver >= 1311744:
                self._debug['unknown_3']['start'] = self._io.pos()
                self.unknown_3 = self._io.read_u4le()
                self._debug['unknown_3']['end'] = self._io.pos()

            self._debug['ptr2']['start'] = self._io.pos()
            self.ptr2 = self._io.read_u4le()
            self._debug['ptr2']['end'] = self._io.pos()
            self._debug['ptr3']['start'] = self._io.pos()
            self.ptr3 = self._io.read_u4le()
            self._debug['ptr3']['end'] = self._io.pos()
            self._debug['ptr4']['start'] = self._io.pos()
            self.ptr4 = self._io.read_u4le()
            self._debug['ptr4']['end'] = self._io.pos()
            self._debug['first_segment_ptr']['start'] = self._io.pos()
            self.first_segment_ptr = self._io.read_u4le()
            self._debug['first_segment_ptr']['end'] = self._io.pos()
            self._debug['unknown_4']['start'] = self._io.pos()
            self.unknown_4 = self._io.read_u4le()
            self._debug['unknown_4']['end'] = self._io.pos()
            self._debug['unknown_5']['start'] = self._io.pos()
            self.unknown_5 = self._io.read_u4le()
            self._debug['unknown_5']['end'] = self._io.pos()
            if self._root.ver >= 1311744:
                self._debug['ptr5']['start'] = self._io.pos()
                self.ptr5 = self._io.read_u4le()
                self._debug['ptr5']['end'] = self._io.pos()

            self._debug['ptr6']['start'] = self._io.pos()
            self.ptr6 = self._io.read_u4le()
            self._debug['ptr6']['end'] = self._io.pos()
            if self._root.ver < 1311744:
                self._debug['ptr7_16x']['start'] = self._io.pos()
                self.ptr7_16x = self._io.read_u4le()
                self._debug['ptr7_16x']['end'] = self._io.pos()

            self._debug['coords_0']['start'] = self._io.pos()
            self.coords_0 = AllegroBrd.Coords(self._io, self, self._root)
            self._debug['coords_0']['end'] = self._io.pos()
            self._debug['coords_1']['start'] = self._io.pos()
            self.coords_1 = AllegroBrd.Coords(self._io, self, self._root)
            self._debug['coords_1']['end'] = self._io.pos()


    class Type35(KaitaiStruct):
        SEQ_FIELDS = ["t2", "t3", "content"]
        def __init__(self, _io, _parent=None, _root=None):
            self._io = _io
            self._parent = _parent
            self._root = _root if _root else self
            self._debug = collections.defaultdict(dict)
            self._read()

        def _read(self):
            self._debug['t2']['start'] = self._io.pos()
            self.t2 = self._io.read_u1()
            self._debug['t2']['end'] = self._io.pos()
            self._debug['t3']['start'] = self._io.pos()
            self.t3 = self._io.read_u2le()
            self._debug['t3']['end'] = self._io.pos()
            self._debug['content']['start'] = self._io.pos()
            self.content = []
            for i in range(120):
                if not 'arr' in self._debug['content']:
                    self._debug['content']['arr'] = []
                self._debug['content']['arr'].append({'start': self._io.pos()})
                self.content.append(self._io.read_u1())
                self._debug['content']['arr'][i]['end'] = self._io.pos()

            self._debug['content']['end'] = self._io.pos()


    @property
    def ver(self):
        """This provides the version of the file, which is used to determine
        how to parse the file. It seems that magics that vary in the last byte
        are format-compatible.
        """
        if hasattr(self, '_m_ver'):
            return self._m_ver

        self._m_ver = (self.magic & 4294967040)
        return getattr(self, '_m_ver', None)


