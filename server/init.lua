
-- Type Constants for entities
CLUSTER   = 1;
PLANET    = 2;
ASTEROID  = 4;
BASE      = 8;
SHIP      = 16;

-- Type Constants for slot contents (returned as a list by get_slots)
EMPTY     = 0;
WEAPON    = 1;
DRIVE     = 2;
ORE       = 3;
_slot_strings = {[0] = "EMPTY", [1]="WEAPON", [2]="DRIVE", [3]="ORE"}

-- Type Constants for possible return values of is_busy
IDLE = false
RELOADING = 4
DOCKING = 6
UNDOCKING = 7
TRANSFER = 9
BUILD = 10
TIMER = 11
MINING = 12
MANUFACTURE = 13
COLONIZE = 14
UPGRADE = 15

-- The default behaviour when recieving data from a docked partner is to
-- execute it right away. (This allows initial programming of a new ship)
function on_incoming_data(d)
	local f = loadstring(d);
	f();
end


-- A handler that screams if your homebase is destroyed
function on_homebase_killed()
	print("OH NOES! MY HOMEBASE HAS BEEN NUKED! AAAAAAH!")
end


-- Override global functions which should not be available within spaceships
--print = nil   -- Ships are not supposed to be able to write to stdout
pcall = nil   -- Lua errors can't be caught, but lead to distruption of the ship



-- Useful function library.

-- For friendly interactive base operation.
function print_slots()
  local s = get_slots()
  print("Slot contents:")
  for i=1,#s do
    print("  ".._slot_strings[s[i]])
  end
  print("end.")
end

-- For knowing when to build a ship.
function count_ore()
  local s = get_slots()
  local num = 0;

  for i=1,#s do
    if(s[i]==ORE) then
      num = num+1
    end
  end

  return num
end

----  ********* Everything below here is debugging code *********
--
---- A default, out of the box autopilot arrival handler.
--function on_autopilot_arrived()
--	print "I arrived somewhere! Yay I'm awesome!"
--	--print "I will now proceed to do nothing."
--	xmin, xmax, ymin, ymax = get_world_size()
--	x = xmin+ math.random(xmax-xmin)
--	y = ymin+ math.random(ymax-ymin)
--	print("Going to "..x..", "..y)
--	set_autopilot_to(x,y)
--	--moveto(x, y)
--end
--
--
---- A debugging handler for dockedness.
--function	on_docking_complete()
--		print("I detect: DOCKING COMPLETE!")
--
--		print("I am sending some code.");
--
--		-- Send a function which rewrites the timer-handler on the other side.
--		send_data(function()
--			print "Code transferred successfully!";
--			on_timer_expired = function()
--				x,y = get_position();
--				set_autopilot_to(4500,4500);
--				print("Flying at " .. x .. ", " .. y)
--				set_timer(3);
--			end
--		end)
--
--		print("Undocking...");
--		undock();
--end
--
--function on_undocking_complete()
--		print("Undocked again. Yay")
--
--		set_timer(3)
--end
--
--
---- And here is some verbose initialization code for testing:
--
---- print "Hi, I'm a ship"
--
---- s = entity_to_string(self)
---- print("Here is some information about myself:\n".. s)
---- closest_planet = find_closest(600, PLANET)
---- if(closest_planet) then
----	print("I'm very close to:\n".. entity_to_string(closest_planet))
---- end
--
---- print("And that guy belongs to player number "..get_player(find_closest(600,PLANET)));
---- x,y = get_position(find_closest(600,PLANET));
---- print("Also, his position is ".. x .. " " .. y);
--
---- closest_ship = find_closest(600,SHIP);
---- if(closest_ship) then
---- 	docked = dock(closest_ship);
---- 	print("Docking returned "..docked)
---- else
---- 	print("No ship in 600 blocks radius.")
---- end
--
---- For testing the execution time limit: an endless loop
----while 1 do
----	print("derping")
----end
--
--
---- Random rumfliegerei
--set_timer(3)
--function on_timer_expired()
--	-- set new timer
--	--set_timer(3)
--	xmin, xmax, ymin, ymax = get_world_size()
--	set_autopilot_to(xmin+math.random(xmax-xmin), ymin+math.random(ymax-ymin))
--	set_timer(1)
--	on_timer_expired = function()
--		x,y = get_position()
--		-- print("I'm now at "..x..", "..y)
--		set_timer(1)
--	end
--end
