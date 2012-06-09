/*organisiertes Laden von Textures für Schiffe & Basen*/
//Große Textures für Schiffe/Basen
var tex_ships;
var tex_bases;
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
//Positionen der slots an den Schiffen/Basen
ship_block_positions=[];
//Texturgrößen (wichtig für die Skalierung)
ship_tex_sizes=[];
//Texturpositionen (für Verwendung großer Texturen)
ship_tex_pos=[];

/*Schiff-Textures*/
function load_ship_textures() {
	for(t=0;t<num_ship_sizes*num_ship_styles*2;t++) {
		if (t<num_ship_sizes*num_ship_styles) {
			prefix="s_";//Schiffe
		} else {
			prefix="b_";//Basen
		}
		name=prefix+str_style_names[((t-t%num_ship_sizes)/num_ship_sizes)%num_ship_styles]+"_"+ship_sizes[t%num_ship_sizes];
		var new_texture=load_texture(str_sprite_dir+name+".png");
		tex_ship_sprites.push(new_texture);
		ship_block_positions.push(block_positions_str[name]);
		ship_tex_sizes.push(ship_tex_sizes_str[name]);
		ship_tex_pos.push(ship_tex_pos_str[name]);
	}
	tex_ships=load_texture(str_sprite_dir+"ships.png");
	tex_bases=load_texture(str_sprite_dir+"bases.png");
}
