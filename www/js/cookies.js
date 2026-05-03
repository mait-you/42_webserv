/* cookies.js — theme from SERVER only */

function getCookie(name) {
    const cookies = document.cookie ? document.cookie.split('; ') : [];

    for (var i = 0; i < cookies.length; i++) {
        var separatorIndex = cookies[i].indexOf('=');
        var key = separatorIndex === -1 ? cookies[i] : cookies[i].slice(0, separatorIndex);

        if (key === name) {
            return decodeURIComponent(cookies[i].slice(separatorIndex + 1));
        }
    }
    return '';
}

function applyThemeFromCookie() {
    var theme   = getCookie('theme');
    var applied = theme === 'dark' ? 'dark' : 'light';

    document.documentElement.dataset.theme = applied;

    var icon  = document.getElementById('theme-icon');
    var label = document.getElementById('theme-label');

    if (icon)  icon.textContent  = applied === 'light' ? '☾' : '☀';
    if (label) label.textContent = applied === 'light' ? 'Dark' : 'Light';
    if (bar)   bar.textContent   = applied;
}

function toggleTheme() {
    var current = getCookie('theme');
    var next    = current === 'dark' ? 'light' : 'dark';

    /* Send to server — server sets the cookie */
    fetch('/?theme=' + next, { method: 'GET' })
        .then(function() {
            applyThemeFromCookie(); /* now read cookie server gave us */
        });
}

document.addEventListener('DOMContentLoaded', applyThemeFromCookie);
