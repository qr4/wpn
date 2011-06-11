#!/usr/bin/perl -w
#
# Test-CGI zur json-anbindung des webGL-clients
#
# Ruft auf ungeheuer ekelige art und weise das graphische routing-teil da auf.

print "Content-type: text/plain\n\n";

open(GITTER, "nc localhost 8080|") || die "hualp.";

while(<GITTER>) {
	print;
}
