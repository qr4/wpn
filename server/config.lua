-- Weltraumprogrammiernacht Config file, for dynamically controllable parameters of the
-- simulation.

-- Maximum amount of cycles a ship may use per lua function call
lua_max_cycles = 10000

-- If we didn't have an complete map in $n cycles send one
map_interval = 500

-- Minimum time of one "tick", in microseconds
-- (this is 1000000/framerate)
frametime = 100000

-- Name of the initial lua code file that will be executed before any player-
-- specific code, in a new ship computer.
ship_init_code_file = "init.lua"

-- Estimated maximum amount of ships and bases. These do not have to be really realistic,
-- and are just a hint for memory allocation
max_ship_size = 1 -- Do not change, or everything will be blown into pieces (this is the radius btw)
max_ship_estimation = 8192
max_base_estimation = 32

-- Map configuration
maximum_cluster_size = 500;

minimum_planet_size = 30;
maximum_planet_size = 50;
planets = 20;

asteroids = 150;
minimum_asteroids = 5;
maximum_asteroids = 5;

average_grid_size = 500;

map_size_x = 150000.;
map_size_y = 76250.;

-- Range Limits:
weapon_range = 500
docking_range = 100
colonize_range = 100
scanner_range = 5000

-- Durations for a number of actions (measured in ticks)
docking_duration = 3
undocking_duration = 3
transfer_duration = 3
mining_duration = 20
manufacture_duration = 20
build_ship_duration = 50
colonize_duration = 50
upgrade_base_duration = 50
-- This better be a multiple of 24, so a large ship can fire every x timesteps
laser_recharge_duration = 24

-- Hit probabilities when shooting at stuff
ship_hit_probability = 0.3
base_hit_probability = 0.3
asteroid_hit_probability = 0.3

-- Tunables of the physics engine
dt = 0.5
vmax = 250
m0_small   =  1 -- Leergewicht eines kleinen Schiffs (3 slots)
m0_medium  =  2 -- Leergewicht eines mittleren Schiffs (6 slots)
m0_large   =  4 -- Leergewicht eines grossen Schiffs (12 slots)
m0_huge    =  8 -- Leergewicht eines riesigen Schiffs (24 slots)
m0_klotz   =  1 -- Gewicht eines Klotzes (thruster, resource, laser)
F_thruster = 20 -- Schub eines thrusters
epsilon    = 1e-10 -- // Missmatches im Kurs durch Rundungsfehler ab denen die Physiksengine sich beklagen soll
asteroid_radius_to_slots_ratio = 1
planet_size = 50

-- Stuff in astroids upon creation in percent
-- First drives are filled, then weapons, then ore.
-- Once the astroid is full we stop putting stuff in.
-- Remaining slots are filled with "empty".
initial_asteroid_drive = 2
initial_asteroid_weapon = 2
initial_asteroid_ore = 86

-- When building a new ship, offset it from the building bases' position by this
-- amount. (This should always be larger than the collision distance)
build_offset_x = -50
build_offset_y = 50

-- A new player starts with a base of this size
initial_base_size = 12
