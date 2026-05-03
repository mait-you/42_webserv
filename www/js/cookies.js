/* cookies.js — cookie helpers + theme management only */

function getCookie(name) {
	const cookies = document.cookie ? document.cookie.split('; ') : [];

	for (const cookie of cookies) {
		const separatorIndex = cookie.indexOf('=');
		const key = separatorIndex === -1 ? cookie : cookie.slice(0, separatorIndex);

		if (key === name) {
			return decodeURIComponent(cookie.slice(separatorIndex + 1));
		}
	}

	return '';
}

function setCookie(name, value, maxAge) {
	document.cookie = name + '=' + encodeURIComponent(value) +
		'; path=/; max-age=' + maxAge;
}

function applyThemeFromCookie() {
	const theme    = getCookie('theme');
	const applied  = theme === 'dark' ? 'dark' : 'light';

	document.documentElement.dataset.theme = applied;

	const icon  = document.getElementById('theme-icon');
	const label = document.getElementById('theme-label');
	const bar   = document.getElementById('ctheme');

	if (icon)  icon.textContent  = applied === 'light' ? '☾' : '☀';
	if (label) label.textContent = applied === 'light' ? 'Dark' : 'Light';
	if (bar)   bar.textContent   = applied;
}

function toggleTheme() {
	const cur  = getCookie('theme');
	const next = cur === 'dark' ? 'light' : 'dark';
	setCookie('theme', next, 31536000);
	applyThemeFromCookie();
}

document.addEventListener('DOMContentLoaded', applyThemeFromCookie);
