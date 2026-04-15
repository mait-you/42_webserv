#!/usr/bin/env perl
use strict;
use warnings;

my $method = $ENV{'REQUEST_METHOD'} || '';
my $query  = $ENV{'QUERY_STRING'} || '';
my $body   = '';

if ($method eq 'POST') {
    my $length = $ENV{'CONTENT_LENGTH'} || '0';
    if ($length =~ /^\d+$/ && $length > 0) {
        read(STDIN, $body, $length);
    }
}

print "Content-Type: text/html\n\n";
print "<html><body>\n";
print "<h1>CGI Test Page (Perl)</h1>\n";
print "<p>METHOD: $method</p>\n";
print "<p>QUERY_STRING: $query</p>\n" if $query ne '';
print "<p>BODY: $body</p>\n" if $body ne '';
print "</body></html>\n";