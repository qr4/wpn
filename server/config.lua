

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
