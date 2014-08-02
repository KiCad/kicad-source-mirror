#!/bin/bash

pandoc -f markdown_mmd -t html interactive_router.mmd > interactive_router.html
pandoc -f markdown_mmd -t latex interactive_router.mmd > tmp.tex
pdflatex interactive_router.tex