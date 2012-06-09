text_elements = [];

function cleanup_texts() {
	for(i=0; i<text_elements.length; i++) {
		document.body.removeChild(text_elements[i]);
	}
	text_elements=[];
}

function world_coords_to_html_coords(x,y) {
  i = [x, y, 0, 1.];
	mat4.multiplyVec4(view_matrix, i);
	mat4.multiplyVec4(projection_matrix, i);

	return i;
}

function place_text_element(x,y,content) {
	v = world_coords_to_html_coords(x,y);
	x = v[0];
	y = v[1]; 

	text = document.createElement('div');
	document.body.appendChild(text);
	text.style.position = "absolute";
	text.style.top = ((2.-y)*canvas.height/4.);
	text.style.left = ((x+2.)*canvas.width/4.);
	text.style.color = "#ffffff";
	text.style["font-family"] = "Sans-serif";
	text.style["font-size"] = "12px";
	text.innerHTML=content;

	text_elements.push(text);
}


