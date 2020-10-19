#!/usr/bin/env python
# -*- coding: utf-8 -*-
#####################################
#
# This program source code file is part of KiCad, a free EDA CAD application.
#
# Copyright (C) 2016 Nick Østergaard <oe.nick at gmail dot com>
# Copyright (C) 2016 KiCad Developers
#
# License GNU GPL Version 3 or any later version.
#
#####################################

import matplotlib.pyplot as plt
import csv
import numpy as np
import time

# Initialize data structure variable
data = []

# Read CSV file
with open('i18n_status.csv', 'r') as csvfile:
    spamreader = csv.reader(csvfile, delimiter=';')
    for row in spamreader:
        data.append(row)

# Replace empyt values with zero and convert numbers to int
for index,value in np.ndenumerate( data ):
    if value=='':
        data[index[0]][index[1]] = 0
    try:
        data[index[0]][index[1]] = int(data[index[0]][index[1]])
    except:
        pass

# Sort data after mostly translated
data[1:] = sorted(data[1:], key=lambda x: int(x[1]), reverse=True)

# Prepare some number for formatting the plot
N = len(data[1:])          # number of languages
width = 0.35               # the width of the bars
ind = np.arange(N)*width*5 # the x locations for the groups

# Plot the bars
fig, ax = plt.subplots()
rects1 = ax.bar(ind, list(zip(*data))[1][1:], width, color='b')
rects2 = ax.bar(ind+width, list(zip(*data))[2][1:], width, color='r')
rects3 = ax.bar(ind+2*width, list(zip(*data))[3][1:], width, color='y')

# Plot ceiling
max_nof_strings = sum(map(int, data[1][1:4]))
plt.plot([0,max(ind)+3*width],[max_nof_strings,max_nof_strings], color='k', linewidth='2')
ax.set_xlim([0,max(ind)+3*width])

# Add some text for labels, title and axes ticks
ax.set_ylabel('Number of strings')
ax.set_title('Translation status')
ax.set_xticks(ind+width*1.5)
ax.set_xticklabels(list(zip(*data))[0][1:])
ax.yaxis.grid(True, which='both') # horizontal lines
ax.legend((rects1[0], rects2[0], rects3[0]), ('TRANSLATED', 'FUZZY', 'UNTRANSLATED'), loc='upper center', bbox_to_anchor=(0.5, -0.05), fancybox=True, ncol=3)
plt.figtext(0.99, 0.96, time.strftime("%d/%m %Y"), horizontalalignment='right')
plt.subplots_adjust(left=0.07, right=0.99, top=0.95, bottom=0.12)

fig.set_size_inches(12, 8)

fig.savefig('i18n_status.svg')

# Show the magic to the user
plt.show()
