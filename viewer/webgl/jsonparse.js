
var ajaxobject;
var old_pos;
var players = [];

/* Has the bounding box been set already? */
var bounding_box_was_set = false;

function createXMLHttpRequest() {
	if (window.XMLHttpRequest) {
		return new XMLHttpRequest();
	} else if (window.ActiveXObject) {
		return new ActiveXObject('Microsoft.XMLHTTP')
	} else {
		_error("Could not create XMLHttpRequest on this browser");
		return null;
	}
}

function init_json_listener() {

	// Starte einen AJAX-Request auf dieses objekt
	var cachebuster = Math.floor(Math.random()*10000001);
	old_pos=0;
	ajaxobject=createXMLHttpRequest();
	ajaxobject.onprogress = new_json_data;
	//ajaxobject.setRequestHeader('Cache-Control', 'no-cache');
	ajaxobject.open("GET", "/cgi/test.cgi?x=" + cachebuster);
	//ajaxobject.open("GET", "test.json?x=" + cachebuster);
	//ajaxobject.open("GET", "visualizer-example-world.json?x=" + cachebuster);
	ajaxobject.send(null);
}

/* Handler fuer neu-reinkommende json-daten */
function new_json_data(event) {
	old_pos += parse_json(ajaxobject.responseText.substr(old_pos));

	if(old_pos > 1024*1024*2) {
		var cachebuster = Math.floor(Math.random()*10000001);
		ajaxobject.abort();
		ajaxobject.responseText = "";
		ajaxobject.open("GET", "/cgi/test.cgi?x=" + cachebuster);
		ajaxobject.send(null);
		old_pos=0;
	}
}

/* Versuche den übergebenen text als (potentiell mehrere, leerzeilen-
 * getrennte) json-datensätze zu parsen.
 * Rückgabewert ist die anzahl der erfolgreich gelesenen zeichen
 */
function parse_json(text) {

	var jsonstrings = text.split("\n\n");
	
	var successful_parse=0;

	for(var i=0; i<jsonstrings.length; i++) {
		try {
			var obj = JSON.parse(jsonstrings[i]);

			if(obj.world) {
				parse_world(obj.world);
			}
			if(obj.update) {
				parse_update(obj.update);
			}
		} catch (error) {
			/* Offenbar liess sich dieser teil nicht parsen.
			 * wir brechen also ab, und versuchen's beim nächsten mal
			 * erneut */
			return successful_parse + i*2;
		}
	
		/* Dieser chunk liess sich offenbar problemlos parsen */
		successful_parse += jsonstrings[i].length;
	}

	/* Anzahl der erfolgreich geparsten bytes zurückgeben
   * (plus zwei bytes trenner pro jsonklotz) */
	return successful_parse + (jsonstrings.length-1) * 2;
}

function parse_world(world) {

	// Planeten werden einfach direkt dem parser übergeben
	if(world.planets) {

		// Mit neuem, leerem array anfangen.
		planets = [];
		for(var i=0; i<world.planets.length; i++) {
			parse_planet(world.planets[i]);
		}

		// Die Graphikkarte sollte davon auch wissen.
		update_planet_vbo();
	}
	// Das gleiche für Schiffe
	if(world.ships) {
		//ships = [];
		for(var i=0; i<world.ships.length; i++) {
			parse_ship(world.ships[i],false);
		}
	}
	if(world.bases) {
		//TODO: Das ist noch nicht so richtig schön.
		//Vielleicht doch nicht basen & Schiffe in die selbe
		//Struktur packen?
		for(var i=0; i<world.bases.length; i++) {
			parse_ship(world.bases[i],true);
		}
	}
	// Ein world-bounding-box-update führt zum zoomen auf
	// die gesamtwelt
	// TODO: das ist natürlich nicht toll, wenn das immer
	// automatisch passiert
	if(world["bounding-box"]) {
		var xmin=world["bounding-box"].xmin;
		var xmax=world["bounding-box"].xmax;
		var ymin=world["bounding-box"].ymin;
		var ymax=world["bounding-box"].ymax;

		var scale=Math.min(1./(xmax-xmin), 1./(ymax-ymin));

		if(!bounding_box_was_set) {
			mat4.identity(view_matrix);
			mat4.translate(view_matrix, [0.,0.,-2.]);
			mat4.scale(view_matrix, [scale,scale,1.]);
			mat4.translate(view_matrix, [-(xmin+xmax)/2., -(ymin+ymax)/2., 0.]);
		}

		bounding_box_was_set = true;

		resize_nebula(xmin,xmax,ymin,ymax);
		render_planet_labels();
	}

	if(world.players) {
		for(i = 0; i < world.players.length; i++) {
			this_id = world.players[i].id;
			players[this_id] = world.players[i].name;
		}
	}
}

function parse_update(update) {
	if(update.planets) {
		// eventuelle planeten-veränderungen einpflegen
		for(var i=0; i<update.planets.length; i++) {
			parse_planet(update.planets[i]);
		}
		// Die Graphikkarte sollte davon auch wissen.
		update_planet_vbo();
	}
	if(update.ships) {//...und für Schiffe
		for(var i=0; i<update.ships.length; i++) {
			parse_ship(update.ships[i]);
		}
	}
}
