

-- Maximum amount of cycles a ship may use per lua function call
lua_max_cycles = 10000


-- Name of the initial lua code file that will be executed before any player-
-- specific code, in a new ship computer.
ship_init_code_file = "init.lua"

-- Estimated maximum amount of ships and bases. These do not have to be really realistic,
-- and are just a hint for memory allocation
max_ship_estimation = 8192
max_base_estimation = 32

-- Range Limits:
weapon_range = 20
docking_range = 1
scanner_range = 50

-- Durations for a number of actions (measured in ticks)
docking_duration = 3
undocking_duration = 3

-- Tunables of the physics engine
dt = 0.5
vmax = 25
m0_small   =  1 -- Leergewicht eines kleinen Schiffs (3 slots)
m0_medium  =  2 -- Leergewicht eines mittleren Schiffs (6 slots)
m0_large   =  4 -- Leergewicht eines grossen Schiffs (12 slots)
m0_huge    =  8 -- Leergewicht eines riesigen Schiffs (24 slots)
m0_klotz   =  1 -- Gewicht eines Klotzes (thruster, resource, laser)
F_thruster = 20 -- Schub eines thrusters
epsilon    = 1e-10 -- // Missmatches im Kurs durch Rundungsfehler ab denen die Physiksengine sich beklagen soll

