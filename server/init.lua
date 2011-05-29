
print "Hi, I'm a ship"

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
