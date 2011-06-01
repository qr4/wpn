
CLUSTER   = 1;
PLANET    = 2;
ASTEROID  = 4;
BASE      = 8;
SHIP      = 16;

print "Hi, I'm a ship"

s = entity_to_string(self)
print("Here is some information about myself:\n".. s)
closest = entity_to_string(find_closest(600, PLANET))
print("I'm very close to:\n".. closest)

function on_autopilot_arrived() 
	print "I arrived somewhere! Uhm, where should I fly?"
	x = 0.1;
	y = 0.1;
	print("Flying to " .. x .. ", " .. y);
	moveto(x, y);
	print("On my way");
end



-- For testing the execution time limit: an endless loop
--while 1 do
--	print("derping")
--end
