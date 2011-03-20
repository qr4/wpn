var canvas, gl;
var model_matrix, view_matrix, projection_matrix;

var fullscreen_vbo;

// WebGL initialisieren
function glInit() {
	
	// Finde das canvas-objekt, in das reingerendert werden soll
	canvas = document.getElementById( 'GLarea' );

	// Suche den opengl-context dafuer
	try {
		gl = canvas.getContext( 'experimental-webgl' );
	} catch(error) { }
	if(!gl) {
		try {
			gl = canvas.getContext( 'webgl' );
		} catch(error) { }
	}

	if(!gl) {
		alert("Your browser does not support WebGL.");
		throw "WebGL not supported";
	}

	// Matrizen initialisieren
	model_matrix = mat4.create();
	view_matrix = mat4.create();
	mat4.identity(model_matrix);
	mat4.identity(view_matrix);
	mat4.translate(view_matrix, [0.,0.,-2.]);
	projection_matrix = mat4.create();

	// VBO für postprocessing einrichten
	fullscreen_vbo = gl.createBuffer();
	gl.bindBuffer(gl.ARRAY_BUFFER, fullscreen_vbo);
	gl.bufferData(gl.ARRAY_BUFFER, new Float32Array( 
			[ -1024., -1024., 0., 	0., 0.,
				 1024., -1024., 0.,		1., 0.,
				-1024.,  1024., 0.,		0., 1.,
				 1024., -1024., 0.,   1., 0.,
				 1024.,  1024., 0.,   1., 1.,
				-1024.,  1024., 0.,		0., 1.] ), gl.STATIC_DRAW);

	resize_handler();
	window.addEventListener( 'resize', resize_handler, false);
}



// Groesse des Opengl-zeugs mit dem Fenster mitskalieren.
function resize_handler() {
	canvas.width = window.innerWidth;
	canvas.height = window.innerHeight;

	// gl-viewport mit anpassen
	gl.viewport( 0, 0, canvas.width, canvas.height );

	// Projektonsmatrix anpassen
	mat4.perspective(45, canvas.width/canvas.height, 1, 100, projection_matrix);
}


// Einen shader aus fragment- und vertexshaderquelltext bauen.
// (die argumente sind DOM-elemente, also <script>-tags
function create_shader(vs_text, fs_text) {
	
	var retval = gl.createProgram();
	var vs = gl.createShader(gl.VERTEX_SHADER);
	var fs = gl.createShader(gl.FRAGMENT_SHADER);

	gl.shaderSource(vs, document.getElementById(vs_text).textContent);
	gl.compileShader(vs);
	if(!gl.getShaderParameter(vs, gl.COMPILE_STATUS)) {
		alert("vs error:\n" + gl.getShaderInfoLog(vs));
		return null;
	}
	gl.shaderSource(fs, '#ifdef GL_ES\nprecision highp float;\n#endif\n\n' + document.getElementById(fs_text).textContent);
	gl.compileShader(fs);
	if(!gl.getShaderParameter(fs, gl.COMPILE_STATUS)) {
		alert("fs error:\n" + gl.getShaderInfoLog(fs));
		return null;
	}

	gl.attachShader(retval, vs);
	gl.attachShader(retval, fs);

	gl.linkProgram(retval);
	if(!gl.getProgramParameter(retval, gl.LINK_STATUS)) {
		alert("link error:\n" + gl.getError());
		return null;
	}

	return retval;
}

// Textur laden
// TODO: Mipmapping
// TODO: racecondition?
function load_texture(url) {
	var texture = gl.createTexture();
	texture.image = new Image();
	texture.image.onload = function() {
		gl.bindTexture(gl.TEXTURE_2D, texture);
		gl.pixelStorei(gl.UNPACK_FLIP_Y_WEBGL, true);
		gl.texImage2D(gl.TEXTURE_2D, 0, gl.RGBA, gl.RGBA, gl.UNSIGNED_BYTE, texture.image);
		gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_MAG_FILTER, gl.LINEAR);
		gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_MIN_FILTER, gl.LINEAR);
		gl.bindTexture(gl.TEXTURE_2D, null);
	}

	texture.image.src = url;
	return texture;
}

// Zum regelmässigen aufrufen der redraw-funktion (browser-unabhängig)
window.requestAnimFrame = (function(){
		return  window.requestAnimationFrame       || 
		window.webkitRequestAnimationFrame || 
		window.mozRequestAnimationFrame    || 
		window.oRequestAnimationFrame      || 
		window.msRequestAnimationFrame     || 
			function(/* function */ callback, /* DOMElement */ element){
			window.setTimeout(callback, 1000 / 60);
		};
})();

