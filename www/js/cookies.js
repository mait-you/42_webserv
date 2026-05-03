/* cookies.js — theme cookie is set by the server, read here */

function getCookie(name) {
    var cookies = document.cookie ? document.cookie.split('; ') : [];

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
}

function toggleTheme() {
    var current = getCookie('theme');
    var next    = current === 'dark' ? 'light' : 'dark';

    fetch('/?theme=' + next, { method: 'GET' })
        .then(function() {
            applyThemeFromCookie();
        });
}

document.addEventListener('DOMContentLoaded', applyThemeFromCookie);
