cmpp.exe : cmpp.obj cmppa.obj
	xset WILD=nullmain.obj
	lcc cmpp.obj cmppa.obj

cmppa.obj : cmppa.asm
	tasm cmppa.asm,,nul

cmpp.obj : cmpp.c
	lcc -c cmpp.c

