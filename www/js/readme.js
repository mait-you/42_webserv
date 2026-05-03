document.addEventListener('DOMContentLoaded', function() {
    fetch('/srcs/README.md')
        .then(function(res) { return res.text(); })
        .then(function(text) {
            var body = document.getElementById('readme-body');
            if (!body) return;
            if (typeof marked === 'undefined') {
                body.textContent = 'Error: marked.js not loaded';
                return;
            }
            body.innerHTML = marked.parse(text);
        })
        .catch(function(err) {
            var body = document.getElementById('readme-body');
            if (body) body.textContent = 'Failed to load README.md: ' + err.message;
        });
});
