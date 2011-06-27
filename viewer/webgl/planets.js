
/* json-abgeleitete planeten */
var planets = [];

/* Und ihre graphikkarten-representation */
var planet_vbo;
var planet_shader;
var planet_glow_shader;

function init_planets() {

	/* VBO und shader und so einrichten */
	planet_vbo = gl.createBuffer();
	planet_shader = create_shader("planet_vs", "planet_fs"); 
	planet_glow_shader = create_shader("planet_glow_vs", "planet_glow_fs");
	update_planet_vbo();
}

// Veränderte planeten-info in der graphikkarte updaten
function update_planet_vbo() {
	
	/* Die planeten werden durch 4 fliesskommazahlen (x,y,id,player) gespeichert */
	var planet_buffer = new Float32Array(planets.length * 4);
	for(var i=0; i<planets.length; i++) {
		planet_buffer[4*i] = planets[i].x;
		planet_buffer[4*i+1] = planets[i].y;
		planet_buffer[4*i+2] = planets[i].id % 16348;
		planet_buffer[4*i+3] = planets[i].owner;
	}
	
	/* Diese daten in die graphikkarte hochladen */
	gl.bindBuffer(gl.ARRAY_BUFFER, planet_vbo);
	gl.bufferData(gl.ARRAY_BUFFER, planet_buffer, gl.STATIC_DRAW);
}

/* Parse einen einzelnen planeten aus der json-repräsentation */
function parse_planet(planet) {
	
	// Suche, ob wir den planeten schon haben. Wenn ja: ersetzen.
	for(var i=0; i<planets.length; i++) {
		if(planets[i].id == planet.id) {
			planets[i] = planet;
			return;
		}
	}	
	
	// Ansonsten: Anfügen.
	planets.push(planet);
}

/* Male alle aktuell bekannten planeten, in einem schub */
function render_planets() {
	
	/* Zuerst den Glow aller Planeten malen */
	gl.useProgram(planet_glow_shader);

	/* Darstellungstransformation an den Shader übergeben */
	gl.uniformMatrix4fv(gl.getUniformLocation(planet_glow_shader, 'modelmatrix'), false, model_matrix);
	gl.uniformMatrix4fv(gl.getUniformLocation(planet_glow_shader, 'viewmatrix'), false, view_matrix);
	gl.uniformMatrix4fv(gl.getUniformLocation(planet_glow_shader, 'projectionmatrix'), false, projection_matrix);
	gl.uniform1f(gl.getUniformLocation(planet_glow_shader, 'scaling'),  planet_glow_size * Math.sqrt(mat4.determinant(view_matrix)*canvas.width*canvas.height));
	gl.uniform1f(gl.getUniformLocation(planet_glow_shader, 'time'), time/5000.);

	/* Planeten-VBO angeben */
	gl.uniform1i(gl.getUniformLocation(planet_glow_shader, 'planet_data'), false, 0);
	gl.bindBuffer(gl.ARRAY_BUFFER, planet_vbo);
	gl.vertexAttribPointer(0, 4, gl.FLOAT, false, 0, 0); // <-- 4 floats pro eintrag
	gl.enableVertexAttribArray(0);

	/* Blending an */
	gl.enable(gl.BLEND);
	gl.blendFunc(gl.SRC_ALPHA, gl.ONE_MINUS_SRC_ALPHA);


	/* Und ab gehts */
	gl.drawArrays(gl.POINTS, 0, planets.length);

	gl.useProgram(planet_shader);


	/* Danach: die planeten selbst */

	/* Darstellungstransformation an den Shader übergeben */
	gl.uniformMatrix4fv(gl.getUniformLocation(planet_shader, 'modelmatrix'), false, model_matrix);
	gl.uniformMatrix4fv(gl.getUniformLocation(planet_shader, 'viewmatrix'), false, view_matrix);
	gl.uniformMatrix4fv(gl.getUniformLocation(planet_shader, 'projectionmatrix'), false, projection_matrix);
	gl.uniform1f(gl.getUniformLocation(planet_shader, 'scaling'), planet_overbloat * Math.sqrt(mat4.determinant(view_matrix)*canvas.width*canvas.height));
	gl.uniform1f(gl.getUniformLocation(planet_shader, 'time'), time/5000.);

	/* Planeten-VBO angeben */
	gl.uniform1i(gl.getUniformLocation(planet_shader, 'planet_data'), false, 0);


	/* Und ab gehts */
	gl.drawArrays(gl.POINTS, 0, planets.length);

	gl.disableVertexAttribArray(0);
	gl.disable(gl.BLEND);
}
