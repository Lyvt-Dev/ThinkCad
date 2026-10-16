# UI catalogs

`catalog.json` maps English source strings to `[Russian, Ukrainian, Spanish,
French]`. German/English source pairs remain in `language.cpp`. Preserve `%1`
and other Qt placeholders, technical identifiers, and leading/trailing spaces.
The catalogs are compiled into the desktop library; no network is required.
`Language::sourceTexts()` supplies the complete base catalog for coverage tests.

The extended Markdown reference guides are German/English. The in-app tutorial
and normal UI catalog support all six languages. Compiler and external backend
output retains the language used by that tool.
