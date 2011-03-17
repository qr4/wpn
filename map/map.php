<?php
// Read JSON data
$map_data = file_get_contents("map.json");
if (!$map_data) {
  die("Could not read file!\n");
}
$map = json_decode($map_data, TRUE);
if (!$map) {
	die("Could not read map correctly!\n");
}

// Get Plantes
$planets = $map['world']['planets'];

// Get World size
$img_size_x = $map['world']['bounding-box']['xmax'] - $map['world']['bounding-box']['xmin'];
$img_size_y = $map['world']['bounding-box']['ymax'] - $map['world']['bounding-box']['ymin'];

// Create an image with GDlib
$gd = imagecreatetruecolor($img_size_x, $img_size_y);
if (!$gd) {
  die("Can't create image via GDlib!\n");
}

// Color in RGB
$img_color = imagecolorallocate($gd, 154, 255, 242);

// Render Map
foreach ($planets as $planet_data) {
	$x = round($planet_data['x'], 0);
	$y = round($planet_data['y'], 0);
	echo "Setting pixel: ".$x.",".$y."\n";
  // Place a filed circle on every planet position
	imagefilledellipse($gd, $x, $y, 5, 5, $img_color);
}
// Return Image as PNG
if (!imagepng($gd, "map.png")) {
  die("Can't create image map.png!\n");
}
?>
