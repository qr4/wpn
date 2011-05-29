#ifndef TUNEABLES_H
#define TUNEABLES_H

#define ASTEROID_RADIUS_TO_SLOTS_RATIO 1
#define PLANET_SIZE 50

#define dt 0.5
#define vmax 25

// Leergewicht eines kleinen Schiffs
#define m0_small 1
// Leergewicht eines mittleren Schiffs
#define m0_medium 2
// Gewicht eines Klotzes (thruster, recourse, laser)
#define m0_klotz 1
// Schub eines thrusters
#define F_thruster 20

// Missmatches im Kurs durch Rundungsfehler ab denen die Physiksengine sich beklagen soll
#define epsilon 1e-10

#endif
