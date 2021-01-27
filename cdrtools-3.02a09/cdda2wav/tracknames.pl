#!/usr/local/bin/perl 
# A quick perl hack to get rename files pulled in with cdda2wav.
# by billo@billo.com
#
use Socket;
use IO::Handle;
use Env qw(USER);
use strict;
no strict 'subs'; # can't get it to stop complaining about SOCK

my $state = "header";

my $global_album = "Artist / Title";
my $global_title = "Title";
my $global_artist = "Artist";
my @global_tracks = ("") x 100;
my $global_ntracks = 0;

my @track_offsets = ();
my $disc_id = 0;
my $disc_time = 0;


if ($#ARGV != 1)
{
    print "usage: cddbhack.pl CDDBHOST PORT < audio.cddb\n";
    exit 0;
}

while (<STDIN>)
{
    if ($state eq "header")
    {
	if (/#\s[\s]*(\d[\d]*)$/)
	{
	    push @track_offsets, $1;
	} elsif (/#\s[\s]*Disc length:\s(\d[\d]*)/) {
   	    $disc_time = $1;
	    $state = "discid";
        }
    } elsif ($state eq "discid")
    {
	if (/DISCID=(\w[\w]*)/)
	{
	    $disc_id = $1;
	    last;
	} 
    }
}

my $query_string = "cddb query $disc_id " . ($#track_offsets + 1);
foreach my $offset (@track_offsets)
{
    $query_string .= " $offset";
}
$query_string .= " $disc_time";

print "$query_string\n";

my $host = $ARGV[0];
my $port = $ARGV[1];

my $iaddr = inet_aton($host);
my $paddr = sockaddr_in($port, $iaddr);


socket(SOCK, AF_INET, SOCK_STREAM, getprotobyname('tcp')) or die "socket: $!";

connect(SOCK, $paddr) or die "connect: $!";

autoflush SOCK 1;

print "Connected.\n";

my ($status, $result) = &resp(\*SOCK);

if (int($status) != 201)
{
    print "Unexpected status.\n";
    close(\*SOCK);
    exit 0;
}

$host = `hostname`;

$host =~ s/\n//g;

&cmd(\*SOCK, "cddb hello $USER $host billo-scan 0.1");
($status, $result) = &resp(\*SOCK);
if (int($status) != 200)
{
    print "Unexpected status.\n";
    close(\*SOCK);
    exit 0;
}

&cmd(\*SOCK, "$query_string");
($status, $result) = &resp(\*SOCK);
if (int($status) != 200)
{
    print "Unexpected status.\n";
    close(\*SOCK);
    exit 0;
}

my ($ignore, $cat, $id, @rest) = split (" ", $result);



my $read_string = "cddb read $cat $id";

&cmd(\*SOCK, $read_string);
&resp(\*SOCK);
while (<SOCK>)
{
    if (/^\./)
    {
	# print $_;
	# print "last line\n";
	last;
    } else {
	&process($_);
	# print $_;
    }
}

&cmd(\*SOCK, "quit");
&resp(\*SOCK);

close(\*SOCK);

&rename;

exit 0;

sub cmd
{
    my ($S, $cmd) = @_;

    print "$cmd\n";
    print $S "$cmd\r\n";
}

sub resp
{
    my ($S) = @_;
    my ($code, $message);
    while (<$S>)
    {
	if (/^(\d[\d]*)\s.*/)
	{
	    # print "\n$1\n";
	    print "$_\n";
	    $code = $1;
	    $message = $_;
	    last;
	}
	sleep(1);
    }
    my @return_array = ($code, $message);
    return @return_array;
}

sub process 
{
    my ($line) = @_;

    $_ = $line;
    if (/^DTITLE=(.*)$/)
    {
	$global_album = $1;
	$_ = $global_album;
	if (m/([^\/][^\/]*)\s\/\s([^\/][^\/\n\r]*)/)
	{
	    $global_artist = $1;
	    $global_title = $2;
	}
	print "$global_album\n";
	print "$global_title\n";
	print "$global_artist\n";
	return;
    }
    if (/^TTITLE(\d[\d]*)=(.*)$/)
    {
	my $track = $1 + 1;
	if ($track > $global_ntracks)
	{
	    $global_ntracks = $track;
	}
	$global_tracks[$track] = sprintf ("%s-%02d-%s", $global_title, 
					  $track, $2);
	$global_tracks[$track] =~ s/\s$//g;
	$global_tracks[$track] =~ s/'//g;
	$global_tracks[$track] =~ s/\s/_/g;
	$global_tracks[$track] =~ s/:/_/g;
	$global_tracks[$track] =~ s/\?//g;
	$global_tracks[$track] =~ s/\*//g;
	$global_tracks[$track] =~ s/\\/_/g;
	$global_tracks[$track] =~ s/\s/_/g;
	$global_tracks[$track] =~ s/\//_/g;
	print "Track match " . $global_tracks[$track] . "\n";
    }
}

sub rename
{
    my $i = 1;
    
    for ($i = 1; $i <= $global_ntracks; $i++)
    {
	my $track_name = $global_tracks[$i];
	if ($track_name ne "")
	{
	    my $file_name = sprintf("audio_%02d.wav", $i);
	    my $new_file_name = sprintf("$track_name.wav", $i);
	    
	    my $mv_cmd = "mv '" . $file_name . "' '" 
		. $new_file_name . "'";
	    print "$mv_cmd\n";
	    `echo $mv_cmd >> rename.sh`;
	}
    }
}

sub unrename
{
    my $i = 1;
    
    for ($i = 1; $i <= $global_ntracks; $i++)
    {
	my $track_name = $global_tracks[$i];
	if ($track_name ne "")
	{
	    my $file_name = sprintf("$track_name.wav", $i);
	    my $new_file_name = sprintf("audio_%02d.wav", $i);
	    
	    my $mv_cmd = "mv '" . $file_name . "' '" 
		. $new_file_name . "'";
	    print "$mv_cmd\n";
	    `echo $mv_cmd >> unrename.sh`;
	}
    }
}


