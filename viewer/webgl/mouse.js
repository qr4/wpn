var mouse_pressed = false;

var lastMouseX, lastMouseY;

function mousedown_handler(event) {
  mouse_pressed = true;
  lastMouseX = event.clientX;
  lastMouseY = event.clientY;

}

function mouseup_handler(event) {
  mouse_pressed = false;
}

function mousemove_handler(event) {
  if (!mouse_pressed) {
    return;
  }
  var newX = event.clientX;
  var newY = event.clientY;

  var deltaX = newX - lastMouseX;
  var deltaY = newY - lastMouseY;

	var scale = Math.sqrt(mat4.determinant(view_matrix));
	deltaX/=scale; deltaY/=scale;

	mat4.translate(view_matrix, [2*deltaX/canvas.width, deltaY/canvas.height, 0.]);

  lastMouseX = newX;
  lastMouseY = newY;
	render_planet_labels();
}

function mousewheel_handler(event) {
	var amount = 0;

	if (!event) event = window.event;
	if (event.wheelDelta) {
		amount = event.wheelDelta/120; 
		if (window.opera) amount = -amount;
	} else if (event.detail) {
		amount = -event.detail/3;
	}
	if(amount != 0) {
		amount = 1. + 0.3 * amount;

		/* Zoom von links an die view-matrix dranmultiplizieren */
		var zoom_matrix = mat4.create();
		mat4.identity(zoom_matrix);
		mat4.scale(zoom_matrix, [amount,amount,1.]);
		mat4.multiply(zoom_matrix, view_matrix);
		view_matrix = zoom_matrix;
	}

	render_planet_labels();
}
