#!/bin/bash
echo "Content-Type: text/html; charset=utf-8"
echo ""
cat <<HTML
<!DOCTYPE html>
<html><head><title>Shell Probe</title>
<link rel="stylesheet" href="/assets/css/styles.css"></head>
<body>
<div class="auth-wrap"><div class="auth-card">
  <h1>Shell CGI Probe</h1>
  <p class="sub">Served by <code>/bin/bash</code>.</p>
  <div class="notice">Date: $(date -u +"%Y-%m-%dT%H:%M:%SZ")</div>
  <p class="muted">
    Method: ${REQUEST_METHOD}<br>
    URI:    ${REQUEST_URI}<br>
    UA:     ${HTTP_USER_AGENT}
  </p>
  <a class="btn btn-primary btn-block" href="/dashboard/file-manager.html">Back to dashboard</a>
</div></div>
</body></html>
HTML
