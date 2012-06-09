#!/bin/bash
#Die Default-Auflösung sind 300DPI.
#Das entspricht 30x30 pixel für einen Raumschiff-Slot.
#10px (die Inkscape-Einheit, die das Format unten
#ausspuckt) entsprechen 1/9inch, d.h. bei 300DPI
#entsprechen 30px im .svg 100 Pixeln im .png
if [ -n "$1" ]; then
	if [ "$1" -gt 0 ]; then
		echo "Resolution $1 DPI selected"
		RESOLUTION=$1
	else
		echo "Standard Resolution selected (100DPI)"
		RESOLUTION=100
	fi
else
	echo "Standard Resolution selected (100DPI)"
	RESOLUTION=100
fi
#Die Namen von Schiffen/Basen bestehen aus 3 Teilen:
#s_ar_6
#Typ(s Schiff b Basis)_Serie_Anzahl an Slots
#svg-Dateien mit den Schiffen/Basen drin
SVGFILES="ships.svg bases.svg"
if [ -e "slots.js" ]; then
	rm slots.js
fi
echo "var block_positions_str=new Array();" >> slots.js
echo "var ship_tex_sizes_str= new Array();" >> slots.js
echo "var ship_tex_pos_str= new Array();" >> slots.js
for SVGFILE in $SVGFILES; do
	echo "Exporting from $SVGFILE"
	PNGFILE=`echo $SVGFILE | sed 's/svg/png/'`
	cat tmp_objects | cut -d "," -f 1 | grep '[bs]_.*_[0-9]*$' > tmp_ships 
	#Extrahiere die Positionen und Größen aller (relevanten) objekte
	inkscape -S $SVGFILE | grep '.*_.*_.*' | sort > tmp_objects
	if [ $SVGFILE == "ships.svg" ]; then
		SVGNAME="s_all"
	else
		SVGNAME="b_all"
	fi
	#exportiere Schiffe/Basen in ein großes png
	inkscape --export-png="$PNGFILE" --export-width="2048" --export-id="$SVGNAME" --export-id-only "$SVGFILE"
	#Berechne die relativen Positionen der slots in pixeln
	#und schreibe das Ergebnis in die Datei 'slots.js'
	./get_textures.py $RESOLUTION
done
mv slots.js ..
