/*organisiertes Laden von Textures für Schiffe & Basen*/
//Textures für Schiffe
tex_ship_sprites=[];
//Anzahl an Schiffsgrößen, ist auch der Abstand an Textures
//zwischen Schiffsdesigns der gleichen Größe
num_ship_sizes=4;
num_ship_styles=5;
ship_sizes=[3,6,12,24];
//Bezeichnungen der Design-Stile
str_style_names=["tk","ar","av","kr","de"];
str_sprite_dir="sprites/";

ship_block_positions=[];
ship_tex_sizes=[];

/*Schiff-Textures*/
function load_ship_textures() {
	for(t=0;t<num_ship_sizes*num_ship_styles;t++) {
		name="s_"+str_style_names[(t-t%num_ship_sizes)/num_ship_sizes]+"_"+ship_sizes[t%num_ship_sizes];
		var new_texture=load_texture(str_sprite_dir+name+".png");
		tex_ship_sprites.push(new_texture);
		ship_block_positions.push(block_positions_str[name]);
		ship_tex_sizes.push(ship_tex_sizes_str[name]);
	}
}

