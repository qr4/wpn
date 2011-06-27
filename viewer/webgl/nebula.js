
var bg_shader;
var bg_vbo;
var bg_noise_scale = 1.;
var bg_offset = -.5;
var bg_overhang = 0.2;

/* Resize the background nebula to fit the universe comfortably */
function resize_nebula(xmin, xmax, ymin, ymax) {
	
	/* Add a bit of overhang */
	var deltax = xmax-xmin;
	var deltay = ymax-ymin;

	xmin -= bg_overhang * deltax;
	ymin -= bg_overhang * deltay;
	xmax += bg_overhang * deltax;
	ymax += bg_overhang * deltay;

	gl.bindBuffer(gl.ARRAY_BUFFER, bg_vbo);
	gl.bufferData(gl.ARRAY_BUFFER, new Float32Array(
			[ xmin, ymin, bg_offset, 	0., 0.,
				 xmax, ymin, bg_offset,		bg_noise_scale, 0.,
				xmin,  ymax, bg_offset,		0., bg_noise_scale,
				 xmax, ymin, bg_offset,   bg_noise_scale, 0.,
				 xmax,  ymax, bg_offset,   bg_noise_scale, bg_noise_scale,
				xmin,  ymax, bg_offset,		0., bg_noise_scale] ), gl.STATIC_DRAW);
}

function init_nebula() {
	bg_shader = create_shader("worldpos_vs", "space_shader");

	bg_vbo = gl.createBuffer();
	resize_nebula(0,10240,0,10240);

}

function render_nebula() {

	// Hintergrund malen
	gl.useProgram(bg_shader);
	gl.uniformMatrix4fv(gl.getUniformLocation(bg_shader, 'modelmatrix'), false, model_matrix);
	gl.uniformMatrix4fv(gl.getUniformLocation(bg_shader, 'viewmatrix'), false, view_matrix);
	gl.uniformMatrix4fv(gl.getUniformLocation(bg_shader, 'projectionmatrix'), false, projection_matrix);
	gl.uniform1i(gl.getUniformLocation(bg_shader, 'pos'), 0);
	gl.uniform1i(gl.getUniformLocation(bg_shader, 'texcoord'), 1);
	gl.uniform1f(gl.getUniformLocation(bg_shader, 'time'), time);
	//gl.uniform1i(gl.getUniformLocation(space_shader, 'tex'), 0);

	//gl.activeTexture(gl.TEXTURE0);
	//gl.bindTexture(gl.TEXTURE_2D, bg_tex);
	gl.enable(gl.BLEND);
	gl.blendFunc(gl.SRC_ALPHA, gl.ONE);
	gl.bindBuffer(gl.ARRAY_BUFFER, bg_vbo);
	gl.vertexAttribPointer(0, 3, gl.FLOAT, false, 5*4, 0); // <-- ccords
	gl.vertexAttribPointer(1, 2, gl.FLOAT, false, 5*4, 3*4); // <-- texcoords
	gl.enableVertexAttribArray(0);
	gl.enableVertexAttribArray(1);
	gl.drawArrays( gl.TRIANGLES, 0, 6 );
	gl.disableVertexAttribArray(0);
	gl.disableVertexAttribArray(1);
	gl.disable(gl.BLEND);

}
