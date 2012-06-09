var starfield_tex;
var starfield_shader;

function init_starfield() {
	
	starfield_tex = load_texture("sprites/starfield.png");
	if(!starfield_tex) {
		alert("Wuhargh!");
	}
	starfield_shader = create_shader("starfield_vs", "texture_shader");
}

function render_starfield(depth) {

	// Hintergrund malen
	gl.useProgram(starfield_shader);
	gl.uniformMatrix4fv(gl.getUniformLocation(starfield_shader, 'modelmatrix'), false, model_matrix);
	gl.uniformMatrix4fv(gl.getUniformLocation(starfield_shader, 'viewmatrix'), false, view_matrix);
	gl.uniformMatrix4fv(gl.getUniformLocation(starfield_shader, 'projectionmatrix'), false, projection_matrix);
	gl.uniform1i(gl.getUniformLocation(starfield_shader, 'pos'), 0);
	gl.uniform1i(gl.getUniformLocation(starfield_shader, 'texcoord'), 1);
	gl.uniform1i(gl.getUniformLocation(starfield_shader, 'tex'), 0);
	gl.uniform1f(gl.getUniformLocation(starfield_shader, 'depth'), depth);

	gl.activeTexture(gl.TEXTURE0);
	gl.bindTexture(gl.TEXTURE_2D, starfield_tex);
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
