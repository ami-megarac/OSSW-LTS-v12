#!/usr/bin/perl

use strict;
use warnings;

use Test::More;

use IO::Socket::IP;
use Socket qw( AI_NUMERICHOST );

my $server = IO::Socket::IP->new(
   Listen    => 1,
   LocalHost => "127.0.0.1",
   LocalPort => 0,
   GetAddrInfoFlags => AI_NUMERICHOST,
) or die "Cannot listen on PF_INET - $!";

my $client = IO::Socket::IP->new(
   PeerHost => $server->sockhost,
   PeerPort => $server->sockport,
   GetAddrInfoFlags => AI_NUMERICHOST,
) or die "Cannot connect on PF_INET - $!";

my $accepted = $server->accept( 'MySubclass' )
   or die "Cannot accept - $!";

isa_ok( $accepted, 'MySubclass' );

done_testing;

package MySubclass;
use base qw( IO::Socket::IP );
