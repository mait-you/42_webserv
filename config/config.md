# Configuration Guide

This document explain how use the configuration file.

## Global directives (inside server)

- `listen <host>:<port>;` Bind address and port.
- `server_name <name>;` Name used for routing.
- `root <path>;` Base directory for this server.
- `index <filename>;` Default file for directory requests.
- `client_max_body_size <bytes>;` Max request body size.
- `error_page <code> <path>;` Map error code to a file path under root.

## Location directives (inside location <path>)

- `allow_methods GET POST DELETE;` Allowed methods for this location.
- `autoindex on|off;` Enable or disable directory listing.
- `index <filename>;` Default file for directory requests in this location.
- `root <path>;` Override server root for this location.
- `error_page <code> <path>;` Override error page mapping in this location.
- `upload_path <path>;` Filesystem path where uploads are stored.
- `cgi_pass <.ext> <interpreter>;` Run CGI for files with the given extension.
- `return <301|302> <url>;` Redirect requests to the given URL.
