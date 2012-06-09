var ships = [];

/* Und ihre graphikkarten-representation */
/* Ein grosser vbo, der für jedes schiff enthält:
 * {x, y, tu, tv} x 4 */
var ship_vbo;
var ship_shader;
var texCoordsOffset;//TODO: Das Aufräumen
//Abkürzungen für Block-Inhalte im JSON
var block_indices=[];block_indices['E']=0;block_indices['R']=1;block_indices['L']=2;block_indices['T']=3;
block_indices[' ']=0;

function init_ships() {
	ship_shader = create_shader("ship_vs", "ship_fs"); 
	ship_vbo = gl.createBuffer();
	update_ship_vbo();
}

function update_ship_vbo() {
	var new_ship_vbo = new Float32Array(6*4*ships.length);

	for(index =0; index < ships.length; index++) {
		
		/* Transformationsmatrix für dieses Schiff bauen */
		var ship_matrix = mat4.create();
		mat4.identity(ship_matrix);
		mat4.translate(ship_matrix,[ships[index].x,ships[index].y,0.]);
		mat4.rotate(ship_matrix,ships[index].direction,[0.0,0.0,1.0]);
		
		/* TODO: Basen drehen lassen:
		 * 			mat4.rotate(model_matrix,(-0.0001+(ships[s].id%20)*0.00001)*time+(ships[s].id%3.14),[0.0,0.0,1.0]);
		 */

		/* Schiffgrösse bestimmen */
		if (ships[index].size==3) ssel=0;
		else if (ships[index].size==6) ssel=1;
		else if (ships[index].size==12) ssel=2;
		else ssel=3;

		/* Schiffs-style aussuchen */
		ssel+=(ships[index].owner%num_ship_styles)*num_ship_sizes;
		var tw=ship_tex_sizes[ssel]/2048.;
		var tex_start = ship_tex_pos[ssel];
		tx0 = tex_start[0]/2048.;
		tx1 = tx0 + tw;
		ty1 = 1. - tex_start[1]/2048.;
		ty0 = ty1 - tw;

		/* Vertex-Koordinaten transformieren */
		var vert1=[tw*ship_scale,tw*ship_scale,0.,1.];
		var vert2=[tw*ship_scale,-tw*ship_scale,0.,1.];
		var vert3=[-tw*ship_scale,-tw*ship_scale,0.,1.];
		var vert4=[-tw*ship_scale,tw*ship_scale,0.,1.];
		
		mat4.multiplyVec4(ship_matrix, vert1);
		mat4.multiplyVec4(ship_matrix, vert2);
		mat4.multiplyVec4(ship_matrix, vert3);
		mat4.multiplyVec4(ship_matrix, vert4);

		new_ship_vbo[6*4*index]=vert1[0];
		new_ship_vbo[6*4*index+1]=vert1[1];
		new_ship_vbo[6*4*index+2]=tx1;
		new_ship_vbo[6*4*index+3]=ty1;
		new_ship_vbo[6*4*index+4]=vert2[0];
		new_ship_vbo[6*4*index+5]=vert2[1];
		new_ship_vbo[6*4*index+6]=tx1;
		new_ship_vbo[6*4*index+7]=ty0;
		new_ship_vbo[6*4*index+8]=vert3[0];
		new_ship_vbo[6*4*index+9]=vert3[1];
		new_ship_vbo[6*4*index+10]=tx0;
		new_ship_vbo[6*4*index+11]=ty0;
		new_ship_vbo[6*4*index+12]=vert3[0];
		new_ship_vbo[6*4*index+13]=vert3[1];
		new_ship_vbo[6*4*index+14]=tx0;
		new_ship_vbo[6*4*index+15]=ty0;
		new_ship_vbo[6*4*index+16]=vert4[0];
		new_ship_vbo[6*4*index+17]=vert4[1];
		new_ship_vbo[6*4*index+18]=tx0;
		new_ship_vbo[6*4*index+19]=ty1;
		new_ship_vbo[6*4*index+20]=vert1[0];
		new_ship_vbo[6*4*index+21]=vert1[1];
		new_ship_vbo[6*4*index+22]=tx1;
		new_ship_vbo[6*4*index+23]=ty1;

		/* Hier als nächstes: VBO für die Slots bauen */
	}

	/* Diese Daten in die Graphikkarte hochladen */
	gl.bindBuffer(gl.ARRAY_BUFFER, ship_vbo);
	gl.bufferData(gl.ARRAY_BUFFER, new_ship_vbo, gl.STATIC_DRAW);

	//Position der slots am Schiff
	//var block_pos=new Float32Array(ship_block_positions[ssel]);
	//Welche Art blöcke befinden sich in diesen slots?
	//var blocks=new Int32Array(ships[index].blocks);
	//	for (j=0;j<block_pos.length;j++) {
	//	var vertices_add= new Float32Array(vertices_quad);
	//	for (i=0;i<6;i++) {
	//		vertices_add[i*3]	=((vertices_add[i*3]*0.163/tw*0.25+block_pos[j*2]*2*tw*ship_scale)-tw*ship_scale);
	//		vertices_add[i*3+1]	=((vertices_add[i*3+1]*0.163/tw*0.25-block_pos[j*2+1]*2*tw*ship_scale)+tw*ship_scale);
	//	}
	//	vertices.set(vertices_add,vertices_quad.length*(j+1));
	//}
	//var texCoords= new Float32Array(texCoords_quad.length+texCoords_quad.length*block_pos.length);
	//texCoords.set(texCoords_quad);
	//for (j=0;j<block_pos.length;j++) {
	//	var texCoords_add= new Float32Array(texCoords_quad);
	//	for (i=0;i<6;i++) {
	//		texCoords_add[i*2]=texCoords_add[i*2]*0.25+0.25*blocks[j];
	//		texCoords_add[i*2+1]=texCoords_add[i*2+1]*0.25+0.75;
	//	}
	//	texCoords.set(texCoords_add,texCoords_quad.length*(j+1));
	//}
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
	update_ship_vbo();
}

/*Male Schiffe*/
function render_ships() {
	gl.useProgram(ship_shader);

	gl.enable(gl.BLEND);
	gl.blendFunc(gl.SRC_ALPHA, gl.ONE_MINUS_SRC_ALPHA);
	gl.enableVertexAttribArray(0);
	gl.enableVertexAttribArray(1);

	/* Darstellungstransformation an den Shader übergeben */
	// Wir brauchen die modelmatrix nicht, da wir diese schon per-ship
	// reintransformiert haben
	//gl.uniformMatrix4fv(gl.getUniformLocation(ship_shader, 'modelmatrix'), false, model_matrix);
	gl.uniformMatrix4fv(gl.getUniformLocation(ship_shader, 'viewmatrix'), false, view_matrix);
	gl.uniformMatrix4fv(gl.getUniformLocation(ship_shader, 'projectionmatrix'), false, projection_matrix);

	/* Schiffstextur auswählen */
	gl.activeTexture(gl.TEXTURE0); 
	gl.bindTexture(gl.TEXTURE_2D, tex_ships);
	
	gl.uniform1i(gl.getUniformLocation(ship_shader, 'spriteMap'), 0);
	
	gl.bindAttribLocation(ship_shader,0,'vertexCoord');
	gl.bindAttribLocation(ship_shader,1,'texCoord');
	
	gl.bindBuffer(gl.ARRAY_BUFFER, ship_vbo);
	gl.vertexAttribPointer(0, 2, gl.FLOAT, false,  4*4, 0);
	gl.vertexAttribPointer(1, 2, gl.FLOAT, false,  4*4, 2*4);
	gl.drawArrays( gl.TRIANGLES, 0, 6*ships.length );
	gl.disableVertexAttribArray(0);
	gl.disableVertexAttribArray(1);
	

	/* TODO: */
	//	gl.activeTexture(gl.TEXTURE0);//Texture mit den blöcken drin
	//	gl.bindTexture(gl.TEXTURE_2D, g_block_texture);
	//
	//	gl.drawArrays(gl.TRIANGLES, 6, 6*ships[s].size);
	//
	//}
	//gl.disable(gl.BLEND);
	gl.disableVertexAttribArray(0);
	gl.disableVertexAttribArray(1);
}
