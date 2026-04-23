<?php
header("Content-Type: text/html; charset=utf-8");
?>
<!DOCTYPE html>
<html><head><title>PHP Probe</title>
<link rel="stylesheet" href="/assets/css/styles.css"></head>
<body>
<div class="auth-wrap"><div class="auth-card">
  <h1>PHP CGI Probe</h1>
  <p class="sub">Served by <code>php-cgi</code> via <code>cgi_pass</code>.</p>
  <div class="notice">PHP version: <code><?= phpversion() ?></code></div>
  <p class="muted">
    Method: <?= htmlspecialchars($_SERVER['REQUEST_METHOD'] ?? '') ?><br>
    URI:    <?= htmlspecialchars($_SERVER['REQUEST_URI']    ?? '') ?>
  </p>
  <a class="btn btn-primary btn-block" href="/dashboard/file-manager.html">Back to dashboard</a>
</div></div>
</body></html>
