var ships = [];

/* Und ihre graphikkarten-representation */
var ship_vbo=[];
var ship_shader;
var texCoordsOffset;//TODO: Das Aufräumen
//Abkürzungen für Block-Inhalte im JSON
var block_indices=[];block_indices['E']=0;block_indices['R']=1;block_indices['L']=2;block_indices['T']=3;
block_indices[' ']=0;

function init_ships() {
	ship_shader = create_shader("ship_vs", "ship_fs"); 
	for (var s=0;s<ships.length;s++) {
		ship_vbo[s] = gl.createBuffer();
		update_ship_vbo(s);
	}
}

function update_ship_vbo(index) {
	//Für Schiffe:
	//Das Vertex Buffer Object enthält:
	//Vertexkoordinaten (in erster Linie wo die Slots hingemalt werden
	//Texturkoordinaten (in erster Linie was in die Slots hineingemalt wird)
	//Index des Schiffs in den Arrays für Textures und Blockpositionen
	//TODO: Das ordentlich machen!
	if (ships[index].size==3) ssel=0;
	else if (ships[index].size==6) ssel=1;
	else if (ships[index].size==12) ssel=2;
	else ssel=3;
	//Die Texturen/Slotkoordinaten für Basen kommen nach denen
	//für Schiffe
	if (ships[index].isbase) ssel+=num_ship_styles*num_ship_sizes;
	ssel+=(ships[index].owner%num_ship_styles)*num_ship_sizes;
	//Position der slots am Schiff
	var block_pos=new Float32Array(ship_block_positions[ssel]);
	//TODO: Das aus der tatsächlichen Textur bestimmen
	//Relative Größe des Sprites für dieses Schiff
	var tw=ship_tex_sizes[ssel]/512;
	//Welche Art blöcke befinden sich in diesen slots?
	var blocks=new Int32Array(ships[index].blocks);
	//Ein einfaches quad
	var vertices_quad = new Float32Array([
	     tw*ship_scale,  tw*ship_scale, 0.0,
	    -tw*ship_scale,  tw*ship_scale, 0.0,
	    -tw*ship_scale, -tw*ship_scale, 0.0,
	     tw*ship_scale,  tw*ship_scale, 0.0,
	    -tw*ship_scale, -tw*ship_scale, 0.0,
	     tw*ship_scale, -tw*ship_scale, 0.0]);
	var vertices= new Float32Array(vertices_quad.length+vertices_quad.length*block_pos.length);
	vertices.set(vertices_quad);
	for (j=0;j<block_pos.length;j++) {
		var vertices_add= new Float32Array(vertices_quad);
		for (i=0;i<6;i++) {
			vertices_add[i*3]	=((vertices_add[i*3]*0.163/tw*0.25+block_pos[j*2]*2*tw*ship_scale)-tw*ship_scale);
			vertices_add[i*3+1]	=((vertices_add[i*3+1]*0.163/tw*0.25-block_pos[j*2+1]*2*tw*ship_scale)+tw*ship_scale);
		}
		vertices.set(vertices_add,vertices_quad.length*(j+1));
	}
	//Texture-Koordinaten dafür
	var texCoords_quad = new Float32Array([
	    1.0, 1.0,
	    0.0, 1.0,
	    0.0, 0.0,
	    1.0, 1.0,
	    0.0, 0.0,
	    1.0, 0.0]);
	var texCoords= new Float32Array(texCoords_quad.length+texCoords_quad.length*block_pos.length);
	texCoords.set(texCoords_quad);
	for (j=0;j<block_pos.length;j++) {
		var texCoords_add= new Float32Array(texCoords_quad);
		for (i=0;i<6;i++) {
			texCoords_add[i*2]=texCoords_add[i*2]*0.25+0.25*blocks[j];
			texCoords_add[i*2+1]=texCoords_add[i*2+1]*0.25+0.75;
		}
		texCoords.set(texCoords_add,texCoords_quad.length*(j+1));
	}

	gl.bindBuffer(gl.ARRAY_BUFFER, ship_vbo[index]);
	texCoordsOffset=vertices.byteLength;
	gl.bufferData(gl.ARRAY_BUFFER, texCoordsOffset + texCoords.byteLength, gl.STATIC_DRAW);
	gl.bufferSubData(gl.ARRAY_BUFFER, 0, vertices);
	gl.bufferSubData(gl.ARRAY_BUFFER, texCoordsOffset, texCoords);
}
function parse_ship(ship,isbase) {
	ship.blocks=[];
	ship.isbase=isbase;
	for(var i=0;i<ship.size;i++) ship.blocks.push(block_indices[ship.contents[i]]);
	for(var i=0;i<ships.length; i++) {
		if(ships[i].id == ship.id) {
			//vbo nur updaten, wenn sich der Inhalt auch geändert hat
			var changed=false;
			if (ship.size==ships[i].size) {
				for (b=0;b<ship.size;b++) if (ship.blocks[b]!=ships[i].blocks[b]) changed=true;
			} else {
				changed=true;
			}
			var dx=ship.x-ships[i].x;
			var dy=ship.y-ships[i].y;
			if (!ship.isbase && Math.abs(dx)>0 >0.0001 && Math.abs(dy)>0.0001) {
				if (dy==0) dy=0.00001;//Division durch 0, wissenschon.
				ship.direction=Math.atan(-dx/dy)+Math.PI*(1+(dy>0));
			} else {
				ship.direction=ships[i].direction;
			}
			//ship.direction=(ship.id*17)%(2*Math.PI);
			ships[i] = ship;
			//Dieses vbo existiert schon: Updaten
			if (changed) update_ship_vbo(i);
			return;
		}
	}	
	ships.push(ship);
	//Ein neues vbo erzeugen
	ship_vbo[ships.length-1] = gl.createBuffer();
	update_ship_vbo(ships.length-1);
}
/*Male Schiffe*/
function render_ships() {
	gl.useProgram(ship_shader);

	gl.enable(gl.BLEND);
	gl.blendFunc(gl.SRC_ALPHA, gl.ONE_MINUS_SRC_ALPHA);
	gl.enableVertexAttribArray(0);
	gl.enableVertexAttribArray(1);

	for (var s=0;s<ships.length;s++) {
		//Matrix auf Ursprung zurücksetzen
		mat4.identity(model_matrix);
		//Auf Position des Schiffs verschieben
		mat4.translate(model_matrix,[ships[s].x,ships[s].y,0.]);
		//Schiffe passend drehen
		if (!ships[s].isbase) {
			mat4.rotate(model_matrix,ships[s].direction,[0.0,0.0,1.0]);
		} else {//TODO Basen, die sich langsam drehen. Nur ein Test!
			mat4.rotate(model_matrix,(-0.0001+(ships[s].id%20)*0.00001)*time+(ships[s].id%3.14),[0.0,0.0,1.0]);
		}
		/* Darstellungstransformation an den Shader übergeben */
		gl.uniformMatrix4fv(gl.getUniformLocation(ship_shader, 'modelmatrix'), false, model_matrix);
		gl.uniformMatrix4fv(gl.getUniformLocation(ship_shader, 'viewmatrix'), false, view_matrix);
		gl.uniformMatrix4fv(gl.getUniformLocation(ship_shader, 'projectionmatrix'), false, projection_matrix);

		gl.activeTexture(gl.TEXTURE0);//Textur des Schiffs
		//TODO: Das ordentlich machen!
		if (ships[s].size==3) ssel=0;
		else if (ships[s].size==6) ssel=1;
		else if (ships[s].size==12) ssel=2;
		else ssel=3;
		if (ships[s].isbase) ssel+=num_ship_styles*num_ship_sizes;
		ssel+=(ships[s].owner%num_ship_styles)*num_ship_sizes;
		//Die Textur nach Größe des Schiffs und Spieler auswählen
		gl.bindTexture(gl.TEXTURE_2D, tex_ship_sprites[ssel]);
	
		gl.uniform1i(gl.getUniformLocation(ship_shader, 'spriteMap'), 0);
	
		gl.bindAttribLocation(ship_shader,0,'vertexCoord');
		gl.bindAttribLocation(ship_shader,1,'texCoord');
	
		gl.bindBuffer(gl.ARRAY_BUFFER, ship_vbo[s]);
		gl.vertexAttribPointer(0, 3, gl.FLOAT, false, 0, 0); // <-- Vertex-Koordinaten
		gl.vertexAttribPointer(1, 2, gl.FLOAT, false, 0, 4*3*6*(ships[s].size*2+1)); // <-- Textur-Koordinaten
	
	
		gl.drawArrays(gl.TRIANGLES, 0, 6);
	
		gl.activeTexture(gl.TEXTURE0);//Texture mit den blöcken drin
		gl.bindTexture(gl.TEXTURE_2D, g_block_texture);
	
		gl.drawArrays(gl.TRIANGLES, 6, 6*ships[s].size);
	
	}
	mat4.identity(model_matrix);
	gl.disable(gl.BLEND);
	gl.disableVertexAttribArray(0);
	gl.disableVertexAttribArray(1);
}
