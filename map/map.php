<?php
$map_data = file_get_contents("blubb.txt");
$map = json_decode($map_data, TRUE);
if (!$map) {
	die ("Could not read map!\n");
}
$planets = $map['world']['planets'];
/*foreach ($planets as $planet_data) {
	foreach ($planet_data as $key => $value) {
		echo $key."\t".$value."\t";
	}
	echo "\n";
}*/
/*echo "Size of image is: ".$map['world']['bounding-box']['xmax']." x ".$map['world']['bounding-box']['ymax']."\n";*/
$img_size_x = $map['world']['bounding-box']['xmax'];
$img_size_y = $map['world']['bounding-box']['ymax'];
$gd = imagecreatetruecolor($img_size_x, $img_size_y);
$img_color = imagecolorallocate($gd, 154, 255, 242);
foreach ($planets as $planet_data) {
	$x = round($planet_data['x'], 0);
	$y = round($planet_data['y'], 0);
	echo "Setting pixel: ".$x.",".$y."\n";
	imagefilledellipse($gd, $x, $y, 5, 5, $img_color);
}
imagepng($gd, "map.png");
?>