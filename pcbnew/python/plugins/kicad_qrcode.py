#
# QR Code Generator for Python
#
# Copyright (c) 2012 Kazuhiko Arase
#
# URL: http://www.d-project.com/
#
# Licensed under the MIT license:
#    http://www.opensource.org/licenses/mit-license.php
#
# The word 'QR Code' is registered trademark of
# DENSO WAVE INCORPORATED
#    http://www.denso-wave.com/qrcode/faqpatent-e.html
#

"""QR Code Generator for Python

    from qrcode import QRCode, ErrorCorrectLevel

    # generate with explicit type number
    qr = QRCode()
    qr.setTypeNumber(4)
    qr.setErrorCorrectLevel(ErrorCorrectLevel.M)
    qr.addData('here comes qr!')
    qr.make()

    # generate with auto type number
    # qr = QRCode.getMinimumQRCode('here comes qr!', ErrorCorrectLevel.M)

    # create an image
    for r in range(qr.getModuleCount() ):
        for c in range(qr.getModuleCount() ):
            color = black if qr.isDark(r, c) else white
            # set pixel ...

"""

class QRCode:

    PAD0 = 0xEC
    PAD1 = 0x11

    def __init__(self):
        self.typeNumber = 1
        self.errorCorrectLevel = ErrorCorrectLevel.H
        self.qrDataList = []
        self.modules = []
        self.moduleCount = 0

    def getTypeNumber(self):
        return self.typeNumber

    def setTypeNumber(self, typeNumber):
        self.typeNumber = typeNumber

    def getErrorCorrectLevel(self):
        return self.errorCorrectLevel

    def setErrorCorrectLevel(self, errorCorrectLevel):
        self.errorCorrectLevel = errorCorrectLevel

    def clearData(self):
        self.qrDataList = []

    def addData(self, data):
        self.qrDataList.append(QR8BitByte(data) )

    def getDataCount(self):
        return len(self.qrDataList)

    def getData(self, index):
        return self.qrDataList[index]

    def isDark(self, row, col):
        return (self.modules[row][col] if self.modules[row][col] != None
            else False)

    def getModuleCount(self):
        return self.moduleCount

    def make(self):
        self._make(False, self._getBestMaskPattern() )

    def _getBestMaskPattern(self):
        minLostPoint = 0
        pattern = 0
        for i in range(8):
            self._make(True, i)
            lostPoint = QRUtil.getLostPoint(self)
            if i == 0 or minLostPoint > lostPoint:
                minLostPoint = lostPoint
                pattern = i
        return pattern

    def _make(self, test, maskPattern):

        self.moduleCount = self.typeNumber * 4 + 17
        self.modules = [[None] * self.moduleCount
            for i in range(self.moduleCount)]

        self._setupPositionProbePattern(0, 0)
        self._setupPositionProbePattern(self.moduleCount - 7, 0)
        self._setupPositionProbePattern(0, self.moduleCount - 7)

        self._setupPositionAdjustPattern()
        self._setupTimingPattern()

        self._setupTypeInfo(test, maskPattern)

        if self.typeNumber >= 7:
            self._setupTypeNumber(test)

        data = QRCode._createData(
            self.typeNumber,
            self.errorCorrectLevel,
            self.qrDataList)

        self._mapData(data, maskPattern)

    def _mapData(self, data, maskPattern):

        rows = list(range(self.moduleCount) )
        cols = [col - 1 if col <= 6 else col
            for col in range(self.moduleCount - 1, 0, -2)]
        maskFunc = QRUtil.getMaskFunction(maskPattern)

        byteIndex = 0
        bitIndex = 7

        for col in cols:
            rows.reverse()
            for row in rows:
                for c in range(2):
                    if self.modules[row][col - c] is None:

                        dark = False
                        if byteIndex < len(data):
                            dark = ( (data[byteIndex] >> bitIndex) & 1) == 1
                        if maskFunc(row, col - c):
                            dark = not dark
                        self.modules[row][col - c] = dark

                        bitIndex -= 1
                        if bitIndex == -1:
                            byteIndex += 1
                            bitIndex = 7

    def _setupPositionAdjustPattern(self):
        pos = QRUtil.getPatternPosition(self.typeNumber)
        for row in pos:
            for col in pos:
                if self.modules[row][col] != None:
                    continue
                for r in range(-2, 3):
                    for c in range(-2, 3):
                        self.modules[row + r][col + c] = (
                            r == -2 or r == 2 or c == -2 or c == 2
                            or (r == 0 and c == 0) )

    def _setupPositionProbePattern(self, row, col):
        for r in range(-1, 8):
            for c in range(-1, 8):
                if (row + r <= -1 or self.moduleCount <= row + r
                        or col + c <= -1 or self.moduleCount <= col + c):
                    continue
                self.modules[row + r][col + c] = (
                    (0 <= r and r <= 6 and (c == 0 or c == 6) )
                    or (0 <= c and c <= 6 and (r == 0 or r == 6) )
                    or (2 <= r and r <= 4 and 2 <= c and c <= 4) )

    def _setupTimingPattern(self):
        for r in range(8, self.moduleCount - 8):
            if self.modules[r][6] != None:
                continue
            self.modules[r][6] = r % 2 == 0
        for c in range(8, self.moduleCount - 8):
            if self.modules[6][c] != None:
                continue
            self.modules[6][c] = c % 2 == 0

    def _setupTypeNumber(self, test):
        bits = QRUtil.getBCHTypeNumber(self.typeNumber)
        for i in range(18):
            self.modules[i // 3][i % 3 + self.moduleCount - 8 - 3] = (
                not test and ( (bits >> i) & 1) == 1)
        for i in range(18):
            self.modules[i % 3 + self.moduleCount - 8 - 3][i // 3] = (
                not test and ( (bits >> i) & 1) == 1)

    def _setupTypeInfo(self, test, maskPattern):

        data = (self.errorCorrectLevel << 3) | maskPattern
        bits = QRUtil.getBCHTypeInfo(data)

        # vertical
        for i in range(15):
            mod = not test and ( (bits >> i) & 1) == 1
            if i < 6:
                self.modules[i][8] = mod
            elif i < 8:
                self.modules[i + 1][8] = mod
            else:
                self.modules[self.moduleCount - 15 + i][8] = mod

        # horizontal
        for i in range(15):
            mod = not test and ( (bits >> i) & 1) == 1
            if i < 8:
                self.modules[8][self.moduleCount - i - 1] = mod
            elif i < 9:
                self.modules[8][15 - i - 1 + 1] = mod
            else:
                self.modules[8][15 - i - 1] = mod

        # fixed
        self.modules[self.moduleCount - 8][8] = not test

    @staticmethod
    def _createData(typeNumber, errorCorrectLevel, dataArray):

        rsBlocks = RSBlock.getRSBlocks(typeNumber, errorCorrectLevel)

        buffer = BitBuffer()

        for data in dataArray:
            buffer.put(data.getMode(), 4)
            buffer.put(data.getLength(), data.getLengthInBits(typeNumber) )
            data.write(buffer)

        totalDataCount = sum(rsBlock.getDataCount()
                              for rsBlock in rsBlocks)

        if buffer.getLengthInBits() > totalDataCount * 8:
            raise Exception('code length overflow. (%s > %s)' %
                    (buffer.getLengthInBits(), totalDataCount * 8) )

        # end code
        if buffer.getLengthInBits() + 4 <= totalDataCount * 8:
            buffer.put(0, 4)

        # padding
        while buffer.getLengthInBits() % 8 != 0:
            buffer.put(False)

        # padding
        while True:
            if buffer.getLengthInBits() >= totalDataCount * 8:
                break
            buffer.put(QRCode.PAD0, 8)
            if buffer.getLengthInBits() >= totalDataCount * 8:
                break
            buffer.put(QRCode.PAD1, 8)

        return QRCode._createBytes(buffer, rsBlocks)

    @staticmethod
    def _createBytes(buffer, rsBlocks):

        offset = 0

        maxDcCount = 0
        maxEcCount = 0

        dcdata = [None] * len(rsBlocks)
        ecdata = [None] * len(rsBlocks)

        for r in range(len(rsBlocks) ):

            dcCount = rsBlocks[r].getDataCount()
            ecCount = rsBlocks[r].getTotalCount() - dcCount

            maxDcCount = max(maxDcCount, dcCount)
            maxEcCount = max(maxEcCount, ecCount)

            dcdata[r] = [0] * dcCount
            for i in range(len(dcdata[r] ) ):
                dcdata[r][i] = 0xff & buffer.getBuffer()[i + offset]
            offset += dcCount

            rsPoly = QRUtil.getErrorCorrectPolynomial(ecCount)
            rawPoly = Polynomial(dcdata[r], rsPoly.getLength() - 1)

            modPoly = rawPoly.mod(rsPoly)
            ecdata[r] = [0] * (rsPoly.getLength() - 1)
            for i in range(len(ecdata[r]) ):
                modIndex = i + modPoly.getLength() - len(ecdata[r])
                ecdata[r][i] = modPoly.get(modIndex) if modIndex >= 0 else 0

        totalCodeCount = sum(rsBlock.getTotalCount()
                              for rsBlock in rsBlocks)

        data = [0] * totalCodeCount

        index = 0

        for i in range(maxDcCount):
            for r in range(len(rsBlocks) ):
                if i < len(dcdata[r] ):
                    data[index] = dcdata[r][i]
                    index += 1

        for i in range(maxEcCount):
            for r in range(len(rsBlocks) ):
                if i < len(ecdata[r] ):
                    data[index] = ecdata[r][i]
                    index += 1

        return data

    @staticmethod
    def getMinimumQRCode(data, errorCorrectLevel):
        mode = Mode.MODE_8BIT_BYTE # fixed to 8bit byte
        qr = QRCode()
        qr.setErrorCorrectLevel(errorCorrectLevel)
        qr.addData(data)
        length = qr.getData(0).getLength()
        for typeNumber in range(1, 11):
            if length <= QRUtil.getMaxLength(
                    typeNumber, mode, errorCorrectLevel):
                qr.setTypeNumber(typeNumber)
                break
        qr.make()
        return qr

class Mode:
    MODE_NUMBER    = 1 << 0
    MODE_ALPHA_NUM = 1 << 1
    MODE_8BIT_BYTE = 1 << 2
    MODE_KANJI     = 1 << 3

class ErrorCorrectLevel:
    L = 1 # 7%
    M = 0 # 15%
    Q = 3 # 25%
    H = 2 # 30%

class MaskPattern:
    PATTERN000 = 0
    PATTERN001 = 1
    PATTERN010 = 2
    PATTERN011 = 3
    PATTERN100 = 4
    PATTERN101 = 5
    PATTERN110 = 6
    PATTERN111 = 7

class QRUtil:

    @staticmethod
    def getPatternPosition(typeNumber):
        return QRUtil.PATTERN_POSITION_TABLE[typeNumber - 1]

    PATTERN_POSITION_TABLE = [
        [],
        [6, 18],
        [6, 22],
        [6, 26],
        [6, 30],
        [6, 34],
        [6, 22, 38],
        [6, 24, 42],
        [6, 26, 46],
        [6, 28, 50],
        [6, 30, 54],
        [6, 32, 58],
        [6, 34, 62],
        [6, 26, 46, 66],
        [6, 26, 48, 70],
        [6, 26, 50, 74],
        [6, 30, 54, 78],
        [6, 30, 56, 82],
        [6, 30, 58, 86],
        [6, 34, 62, 90],
        [6, 28, 50, 72, 94],
        [6, 26, 50, 74, 98],
        [6, 30, 54, 78, 102],
        [6, 28, 54, 80, 106],
        [6, 32, 58, 84, 110],
        [6, 30, 58, 86, 114],
        [6, 34, 62, 90, 118],
        [6, 26, 50, 74, 98, 122],
        [6, 30, 54, 78, 102, 126],
        [6, 26, 52, 78, 104, 130],
        [6, 30, 56, 82, 108, 134],
        [6, 34, 60, 86, 112, 138],
        [6, 30, 58, 86, 114, 142],
        [6, 34, 62, 90, 118, 146],
        [6, 30, 54, 78, 102, 126, 150],
        [6, 24, 50, 76, 102, 128, 154],
        [6, 28, 54, 80, 106, 132, 158],
        [6, 32, 58, 84, 110, 136, 162],
        [6, 26, 54, 82, 110, 138, 166],
        [6, 30, 58, 86, 114, 142, 170]
        ]

    MAX_LENGTH = [
        [ [41,  25,  17,  10],  [34,  20,  14,  8],   [27,  16,  11,  7],  [17,  10,  7,   4] ],
        [ [77,  47,  32,  20],  [63,  38,  26,  16],  [48,  29,  20,  12], [34,  20,  14,  8] ],
        [ [127, 77,  53,  32],  [101, 61,  42,  26],  [77,  47,  32,  20], [58,  35,  24,  15] ],
        [ [187, 114, 78,  48],  [149, 90,  62,  38],  [111, 67,  46,  28], [82,  50,  34,  21] ],
        [ [255, 154, 106, 65],  [202, 122, 84,  52],  [144, 87,  60,  37], [106, 64,  44,  27] ],
        [ [322, 195, 134, 82],  [255, 154, 106, 65],  [178, 108, 74,  45], [139, 84,  58,  36] ],
        [ [370, 224, 154, 95],  [293, 178, 122, 75],  [207, 125, 86,  53], [154, 93,  64,  39] ],
        [ [461, 279, 192, 118], [365, 221, 152, 93],  [259, 157, 108, 66], [202, 122, 84,  52] ],
        [ [552, 335, 230, 141], [432, 262, 180, 111], [312, 189, 130, 80], [235, 143, 98,  60] ],
        [ [652, 395, 271, 167], [513, 311, 213, 131], [364, 221, 151, 93], [288, 174, 119, 74] ]
        ]

    @staticmethod
    def getMaxLength(typeNumber, mode, errorCorrectLevel):
        t = typeNumber - 1
        e = {
            ErrorCorrectLevel.L: 0,
            ErrorCorrectLevel.M: 1,
            ErrorCorrectLevel.Q: 2,
            ErrorCorrectLevel.H: 3
            }[errorCorrectLevel]
        m = {
            Mode.MODE_NUMBER: 0,
            Mode.MODE_ALPHA_NUM: 1,
            Mode.MODE_8BIT_BYTE: 2,
            Mode.MODE_KANJI: 3
            }[mode]
        return QRUtil.MAX_LENGTH[t][e][m]

    @staticmethod
    def getErrorCorrectPolynomial(errorCorrectLength):
        a = Polynomial([1])
        for i in range(errorCorrectLength):
            a = a.multiply(Polynomial([1, QRMath.gexp(i)]) )
        return a

    @staticmethod
    def getMaskFunction(maskPattern):
        return {
            MaskPattern.PATTERN000:
                lambda i, j: (i + j) % 2 == 0,
            MaskPattern.PATTERN001:
                lambda i, j: i % 2 == 0,
            MaskPattern.PATTERN010:
                lambda i, j: j % 3 == 0,
            MaskPattern.PATTERN011:
                lambda i, j: (i + j) % 3 == 0,
            MaskPattern.PATTERN100:
                lambda i, j: (i // 2 + j // 3) % 2 == 0,
            MaskPattern.PATTERN101:
                lambda i, j: (i * j) % 2 + (i * j) % 3 == 0,
            MaskPattern.PATTERN110:
                lambda i, j: ( (i * j) % 2 + (i * j) % 3) % 2 == 0,
            MaskPattern.PATTERN111:
                lambda i, j: ( (i * j) % 3 + (i + j) % 2) % 2 == 0
            }[maskPattern]

    @staticmethod
    def getLostPoint(qrcode):

        moduleCount = qrcode.getModuleCount()
        lostPoint = 0

        # LEVEL1
        for row in range(moduleCount):
            for col in range(moduleCount):
                sameCount = 0
                dark = qrcode.isDark(row, col)
                for r in range(-1, 2):
                    if row + r < 0 or moduleCount <= row + r:
                        continue
                    for c in range(-1, 2):
                        if col + c < 0 or moduleCount <= col + c:
                            continue
                        if r == 0 and c == 0:
                            continue
                        if dark == qrcode.isDark(row + r, col + c):
                            sameCount += 1
                if sameCount > 5:
                    lostPoint += (3 + sameCount - 5)

        # LEVEL2
        for row in range(moduleCount - 1):
            for col in range(moduleCount - 1):
                count = 0
                if qrcode.isDark(row, col):
                    count += 1
                if qrcode.isDark(row + 1, col):
                    count += 1
                if qrcode.isDark(row, col + 1):
                    count += 1
                if qrcode.isDark(row + 1, col + 1):
                    count += 1
                if count == 0 or count == 4:
                    lostPoint += 3

        # LEVEL3
        for row in range(moduleCount):
            for col in range(moduleCount - 6):
                if (qrcode.isDark(row, col)
                        and not qrcode.isDark(row, col + 1)
                        and     qrcode.isDark(row, col + 2)
                        and     qrcode.isDark(row, col + 3)
                        and     qrcode.isDark(row, col + 4)
                        and not qrcode.isDark(row, col + 5)
                        and     qrcode.isDark(row, col + 6) ):
                    lostPoint += 40

        for col in range(moduleCount):
            for row in range(moduleCount - 6):
                if (qrcode.isDark(row, col)
                        and not qrcode.isDark(row + 1, col)
                        and     qrcode.isDark(row + 2, col)
                        and     qrcode.isDark(row + 3, col)
                        and     qrcode.isDark(row + 4, col)
                        and not qrcode.isDark(row + 5, col)
                        and     qrcode.isDark(row + 6, col) ):
                    lostPoint += 40

        # LEVEL4
        darkCount = 0
        for col in range(moduleCount):
            for row in range(moduleCount):
                if qrcode.isDark(row, col):
                    darkCount += 1

        ratio = abs(100 * darkCount // moduleCount // moduleCount - 50) // 5
        lostPoint += ratio * 10

        return lostPoint

    G15 = ( (1 << 10) | (1 << 8) | (1 << 5) | (1 << 4) |
            (1 << 2) | (1 << 1) | (1 << 0) )
    G18 = ( (1 << 12) | (1 << 11) | (1 << 10) | (1 << 9) |
            (1 << 8) | (1 << 5) | (1 << 2) | (1 << 0) )
    G15_MASK = (1 << 14) | (1 << 12) | (1 << 10) | (1 << 4) | (1 << 1)

    @staticmethod
    def getBCHTypeInfo(data):
        d = data << 10
        while QRUtil.getBCHDigit(d) - QRUtil.getBCHDigit(QRUtil.G15) >= 0:
            d ^= (QRUtil.G15 << (QRUtil.getBCHDigit(d) -
                                 QRUtil.getBCHDigit(QRUtil.G15) ) )
        return ( (data << 10) | d) ^ QRUtil.G15_MASK

    @staticmethod
    def getBCHTypeNumber(data):
        d = data << 12
        while QRUtil.getBCHDigit(d) - QRUtil.getBCHDigit(QRUtil.G18) >= 0:
            d ^= (QRUtil.G18 << (QRUtil.getBCHDigit(d) -
                                 QRUtil.getBCHDigit(QRUtil.G18) ) )
        return (data << 12) | d

    @staticmethod
    def getBCHDigit(data):
        digit = 0
        while data != 0:
            digit += 1
            data >>= 1
        return digit

    @staticmethod
    def stringToBytes(s):
        return [ord(c) & 0xff for c in s]

class QR8BitByte:

    def __init__(self, data):
        self.mode = Mode.MODE_8BIT_BYTE
        self.data = data

    def getMode(self):
        return self.mode

    def getData(self):
        return self.data

    '''
    def write(self, buffer): raise Exception('not implemented.')
    def getLength(self): raise Exception('not implemented.')
    '''

    def write(self, buffer):
        data = QRUtil.stringToBytes(self.getData() )
        for d in data:
            buffer.put(d, 8)

    def getLength(self):
        return len(QRUtil.stringToBytes(self.getData() ) )

    def getLengthInBits(self, type):
        if 1 <= type and type < 10: # 1 - 9
            return {
                Mode.MODE_NUMBER:    10,
                Mode.MODE_ALPHA_NUM: 9,
                Mode.MODE_8BIT_BYTE: 8,
                Mode.MODE_KANJI:     8
                }[self.mode]

        elif type < 27: # 10 - 26
            return {
                Mode.MODE_NUMBER:    12,
                Mode.MODE_ALPHA_NUM: 11,
                Mode.MODE_8BIT_BYTE: 16,
                Mode.MODE_KANJI:     10
                }[self.mode]

        elif type < 41: # 27 - 40
            return {
                Mode.MODE_NUMBER:    14,
                Mode.MODE_ALPHA_NUM: 13,
                Mode.MODE_8BIT_BYTE: 16,
                Mode.MODE_KANJI:     12
                }[self.mode]

        else:
            raise Exception('type:%s' % type)

class QRMath:

    EXP_TABLE = None
    LOG_TABLE = None

    @staticmethod
    def _init():

        QRMath.EXP_TABLE = [0] * 256
        for i in range(256):
            QRMath.EXP_TABLE[i] = (1 << i if i < 8 else
                     QRMath.EXP_TABLE[i - 4] ^ QRMath.EXP_TABLE[i - 5] ^
                     QRMath.EXP_TABLE[i - 6] ^ QRMath.EXP_TABLE[i - 8])

        QRMath.LOG_TABLE = [0] * 256
        for i in range(255):
            QRMath.LOG_TABLE[QRMath.EXP_TABLE[i] ] = i

    @staticmethod
    def glog(n):
        if n < 1:
            raise Exception('log(%s)' % n)
        return QRMath.LOG_TABLE[n]

    @staticmethod
    def gexp(n):
        while n < 0:
            n += 255
        while n >= 256:
            n -= 255
        return QRMath.EXP_TABLE[n]

# initialize statics
QRMath._init()

class Polynomial:

    def __init__(self, num, shift=0):
        offset = 0
        length = len(num)
        while offset < length and num[offset] == 0:
            offset += 1
        self.num = num[offset:] + [0] * shift

    def get(self, index):
        return self.num[index]

    def getLength(self):
        return len(self.num)

    def __repr__(self):
        return ','.join( [str(self.get(i) )
            for i in range(self.getLength() ) ] )

    def toLogString(self):
        return ','.join( [str(QRMath.glog(self.get(i) ) )
            for i in range(self.getLength() ) ] )

    def multiply(self, e):
        num = [0] * (self.getLength() + e.getLength() - 1)
        for i in range(self.getLength() ):
            for j in range(e.getLength() ):
                num[i + j] ^= QRMath.gexp(QRMath.glog(self.get(i) ) +
                              QRMath.glog(e.get(j) ) )
        return Polynomial(num)

    def mod(self, e):
        if self.getLength() - e.getLength() < 0:
            return self
        ratio = QRMath.glog(self.get(0) ) - QRMath.glog(e.get(0) )
        num = self.num[:]
        for i in range(e.getLength() ):
            num[i] ^= QRMath.gexp(QRMath.glog(e.get(i) ) + ratio)
        return Polynomial(num).mod(e)

class RSBlock:

    RS_BLOCK_TABLE = [

        # L
        # M
        # Q
        # H

        # 1
        [1, 26, 19],
        [1, 26, 16],
        [1, 26, 13],
        [1, 26, 9],

        # 2
        [1, 44, 34],
        [1, 44, 28],
        [1, 44, 22],
        [1, 44, 16],

        # 3
        [1, 70, 55],
        [1, 70, 44],
        [2, 35, 17],
        [2, 35, 13],

        # 4
        [1, 100, 80],
        [2, 50, 32],
        [2, 50, 24],
        [4, 25, 9],

        # 5
        [1, 134, 108],
        [2, 67, 43],
        [2, 33, 15, 2, 34, 16],
        [2, 33, 11, 2, 34, 12],

        # 6
        [2, 86, 68],
        [4, 43, 27],
        [4, 43, 19],
        [4, 43, 15],

        # 7
        [2, 98, 78],
        [4, 49, 31],
        [2, 32, 14, 4, 33, 15],
        [4, 39, 13, 1, 40, 14],

        # 8
        [2, 121, 97],
        [2, 60, 38, 2, 61, 39],
        [4, 40, 18, 2, 41, 19],
        [4, 40, 14, 2, 41, 15],

        # 9
        [2, 146, 116],
        [3, 58, 36, 2, 59, 37],
        [4, 36, 16, 4, 37, 17],
        [4, 36, 12, 4, 37, 13],

        # 10
        [2, 86, 68, 2, 87, 69],
        [4, 69, 43, 1, 70, 44],
        [6, 43, 19, 2, 44, 20],
        [6, 43, 15, 2, 44, 16]
        ]

    def __init__(self, totalCount, dataCount):
        self.totalCount = totalCount
        self.dataCount  = dataCount

    def getDataCount(self):
        return self.dataCount

    def getTotalCount(self):
        return self.totalCount

    def __repr__(self):
        return ('(total=%s,data=%s)' % (self.totalCount, self.dataCount) )

    @staticmethod
    def getRSBlocks(typeNumber, errorCorrectLevel):
        rsBlock = RSBlock.getRsBlockTable(typeNumber, errorCorrectLevel)
        length = len(rsBlock) // 3
        list = []
        for i in range(length):
            count      = rsBlock[i * 3 + 0]
            totalCount = rsBlock[i * 3 + 1]
            dataCount  = rsBlock[i * 3 + 2]
            list += [RSBlock(totalCount, dataCount)] * count
        return list

    @staticmethod
    def getRsBlockTable(typeNumber, errorCorrectLevel):
        return {
            ErrorCorrectLevel.L:
                RSBlock.RS_BLOCK_TABLE[ (typeNumber - 1) * 4 + 0],
            ErrorCorrectLevel.M:
                RSBlock.RS_BLOCK_TABLE[ (typeNumber - 1) * 4 + 1],
            ErrorCorrectLevel.Q:
                RSBlock.RS_BLOCK_TABLE[ (typeNumber - 1) * 4 + 2],
            ErrorCorrectLevel.H:
                RSBlock.RS_BLOCK_TABLE[ (typeNumber - 1) * 4 + 3]
            }[errorCorrectLevel]

class BitBuffer:

    def __init__(self, inclements=32):
        self.inclements = inclements
        self.buffer = [0] * self.inclements
        self.length = 0

    def getBuffer(self):
        return self.buffer

    def getLengthInBits(self):
        return self.length

    def get(self, index):
        return ( (self.buffer[index // 8] >> (7 - index % 8) ) & 1) == 1

    def putBit(self, bit):
        if self.length == len(self.buffer) * 8:
            self.buffer += [0] * self.inclements
        if bit:
            self.buffer[self.length // 8] |= (0x80 >> (self.length % 8) )
        self.length += 1

    def put(self, num, length):
        for i in range(length):
            self.putBit( ( (num >> (length - i - 1) ) & 1) == 1)

    def __repr__(self):
        return ''.join('1' if self.get(i) else '0'
            for i in range(self.getLengthInBits() ) )
