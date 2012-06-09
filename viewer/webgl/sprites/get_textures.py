#!/usr/bin/python
import sys
from math import ceil
resolution=100/90.0#default: 300 DPI
if len(sys.argv)>1:
	resolution=int(sys.argv[1])/90.0
f=open('tmp_objects','r')
objects=f.readlines()
f.close()
f=open('slots.js','aw')
#Arrays definieren
#f.write('var block_positions_str=new Array();\n');
#f.write('var ship_tex_sizes_str= new Array();\n');
#f.write('var ship_tex_pos_str= new Array();\n');
objects=map(lambda x: x.strip().split(','),objects)#In Spalten aufteilen
i=0;
slot_x=list()
slot_y=list()
while (i<len(objects)):
	if (objects[i][0].count('_')==2):#Zwei _: Eintrag fuer ein Schiff
		ship_x=float(objects[i][1])
		ship_y=float(objects[i][2])
		ship_w=ceil(float(objects[i][3]))
		num_slots=int(objects[i][0].split('_')[-1])#letzter Eintrag: Anzahl an slots
		s_x=list()
		s_y=list()
		s='block_positions_str[\''+(objects[i][0])+'\']=['
		for j in range(num_slots):
			x=(float(objects[i+j+1][1])-ship_x)*resolution
			y=(float(objects[i+j+1][2])-ship_y)*resolution
			s_x=x
			s_y=y
			if (j>0):
				s+=','
			s+=('%f,%f' % (x,y))
		s+=('];\n')
		f.write(s)
		slot_x.append(s_x)
		slot_y.append(s_y)
		i+=num_slots
		f.write('ship_tex_sizes_str[\'%s\']=%3d;\n'%(objects[i][0],ship_w))
		f.write('ship_tex_pos_str[\'%s\']=[%3d,%3d];\n'%(objects[i][0],ceil(ship_x),ceil(ship_y)))
	i+=1
f.close()
