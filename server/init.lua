
-- Type Constants for entities
CLUSTER   = 1;
PLANET    = 2;
ASTEROID  = 4;
BASE      = 8;
SHIP      = 16;


-- A default, out of the box autopilot arrival handler.
function on_autopilot_arrived() 
	print "I arrived somewhere! Yay I'm awesome!"
	print "I will now proceed to do nothing."
end


-- A debugging handler for dockedness.
function	on_docking_complete()
		print("I detect: DOCKING COMPLETE!")

		print("Undocking...");
		undock();
end

function on_undocking_complete()
		print("Undocked again. Yay")

		set_timer(3)
end
-- And here is some verbose initialization code for testing:

print "Hi, I'm a ship"

s = entity_to_string(self)
print("Here is some information about myself:\n".. s)
closest_planet = find_closest(600, PLANET)
if(closest_planet) then
	print("I'm very close to:\n".. entity_to_string(closest_planet))
end

-- print("And that guy belongs to player number "..get_player(find_closest(600,PLANET)));
-- x,y = get_position(find_closest(600,PLANET));
-- print("Also, his position is ".. x .. " " .. y);

closest_ship = find_closest(600,SHIP);
if(closest_ship) then
	docked = dock(closest_ship);
	print("Docking returned "..docked)

else
	print("No ship in 600 blocks radius.")
end

-- For testing the execution time limit: an endless loop
--while 1 do
--	print("derping")
--end

set_timer(3)
function on_timer_expired()
	print "My Alarm rang!"

	-- set new timer
	--set_timer(3)
	set_autopilot_to(500, 500)
	set_timer(1)
	on_timer_expired = function()
		x,y = get_position()
		print("I'm now at "..x..", "..y)
		set_timer(1)
	end
	
end


-- Override global functions which should not be available within spaceships
--print = nil   -- Ships are not supposed to be able to write to stdout
--pcall = nil   -- Lua errors can't be caught, but lead to distruption of the ship
