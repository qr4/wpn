
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

-- And here is some verbose initialization code for testing:

print "Hi, I'm a ship"

s = entity_to_string(self)
print("Here is some information about myself:\n".. s)
closest = entity_to_string(find_closest(600, PLANET))
print("I'm very close to:\n".. closest)

-- For testing the execution time limit: an endless loop
--while 1 do
--	print("derping")
--end

set_timer(3)
function on_timer_expired()
	print "My Alarm rang!"

	-- set new timer
	set_timer(3)
end

-- Override global functions which should not be available within spaceships
--print = nil   -- Ships are not supposed to be able to write to stdout
--pcall = nil   -- Lua errors can't be caught, but lead to distruption of the ship
