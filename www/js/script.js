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

function applyThemeFromCookie() {
	const theme = getCookie('theme');
	const nextTheme = theme === 'dark' ? 'dark' : 'light';

	document.documentElement.dataset.theme = nextTheme;
}

document.addEventListener('DOMContentLoaded', () => {
	applyThemeFromCookie();
});



