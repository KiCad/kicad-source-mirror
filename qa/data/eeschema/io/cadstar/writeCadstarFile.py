
from math import factorial
from itertools import permutations,islice

for fileindex, filename in enumerate(["dummycadstarlib.lib", "dummycadstarlibwithheader.lib"]):
    with open(filename, 'w', newline='\r\n') as f:
        f.write('# FORMAT 32\n')

        if fileindex > 0:
            f.write('+N0 \'root\'\n')
            f.write('+N1 N0 ')
            
            for i in range(100):
                f.write(f'\'PartName{i}\'&\n')

        for i in range(100):
            f.write('\n')
            f.write(f'.PartName{i} ({i*5}) :2 ;Part {i} Description\n')
            f.write(f'FOOTPRINT{i} (variant{i*5})\n')

            currentPartData=[]

            currentPartData.append(f'*VALUE {i} uH\n')
            currentPartData.append(f'*DFN PartName{i}\n')

            if i%10 == 1:
                currentPartData.append(f'*NGS\n')

            if i%5 == 1:
                currentPartData.append(f'*NPV\n')


            currentPartData.append('*STM L\n')
            currentPartData.append(f'*MXP {i+10}\n')
            currentPartData.append(f'*SPI (PartName{i}) {i}uH\n')
            currentPartData.append(f'*PAC (PartName{i}) Acceptance{i}\n')
            currentPartData.append(f'*UserFieldpartNo {i*5}\n')
            currentPartData.append(f'*"UserFieldpartNoCreated by" Person{i}\n')
            currentPartData.append(f'$"SCH val1" (val{i})\n')
            currentPartData.append(f'$!SCH val2 (readOnly{i})\n')
            currentPartData.append(f'%"PCB val1" (val{i})\n')
            currentPartData.append(f'%!PCB val2 (readOnly{i})\n')
            currentPartData.append(f'~"Part val1" (val{i})\n')
            currentPartData.append(f'~!Part val2 (readOnly{i})\n')
            currentPartData.append(f'@SCH and PCB val1 (val{i})\n')
            currentPartData.append(f'@!SCH and PCB val2 (readOnly{i})\n')

            # Change ordering to test parser works
            nth=(i*101)%factorial(len(currentPartData)) # make a permutation that exists
            permutatedData=next(islice(permutations(currentPartData), nth, None))

            for data in permutatedData:
                f.write(data)

            # Symbol should always be at the end in fileformat anyway
            f.write(f'Symbol{i}\n')
            f.write('1.0 2.0\n')
            f.write('/GND 3.0\n')


        f.write('\n.END\n')