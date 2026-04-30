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

print "Content-Type: text/html\r\n\r\n";
print "<!DOCTYPE html>\n<html>\n<head>\n";
print "<title>Simple Perl CGI</title>\n";
print "</head>\n<body style='font-family: Arial, sans-serif; margin: 40px; color: #333;'>\n";

print "<h2>Perl CGI Test</h2>\n";
print "<div style='background: #f9f9f9; padding: 20px; border-radius: 5px; border: 1px solid #ddd;'>\n";
print "  <p><strong>METHOD:</strong> $method</p>\n";

if ($query ne '') {
    print "  <p><strong>QUERY_STRING:</strong> $query</p>\n";
}

if ($body ne '') {
    print "  <p><strong>BODY:</strong> $body</p>\n";
}

print "</div>\n";

print "<hr style='margin-top: 30px; border: 0; border-top: 1px solid #eee;'>\n";
print "<h3>Test POST Request</h3>\n";
print "<form method='POST' action='/cgi-bin/test.pl'>\n";
print "  <input type='text' name='data' placeholder='Enter some data...' style='padding: 8px; border: 1px solid #ccc; border-radius: 4px;'>\n";
print "  <input type='submit' value='Submit POST' style='padding: 8px 15px; background: #0066cc; color: white; border: none; border-radius: 4px; cursor: pointer;'>\n";
print "</form>\n";

print "<br><br><a href='/' style='color: #0066cc; text-decoration: none;'>&larr; Back to Home</a>\n";
print "</body>\n</html>\n";