(function () {
    var LIST_URL = "/uploads/";

    function fetchListing(callback) {
        var xhr = new XMLHttpRequest();
        xhr.open("GET", LIST_URL, true);
        xhr.onload = function () {
            if (xhr.status !== 200) return callback([]);
            var doc = new DOMParser().parseFromString(xhr.responseText, "text/html");
            var items = [];
            var links = doc.querySelectorAll("a");
            for (var i = 0; i < links.length; i++) {
                var a    = links[i];
                var href = a.getAttribute("href") || "";
                var name = decodeURIComponent(a.textContent.trim());
                if (!name || name === ".." || name === "../") continue;
                items.push({ name: name.replace(/\/$/, ""), isDir: href.slice(-1) === "/" });
            }
            callback(items);
        };
        xhr.onerror = function () { callback([]); };
        xhr.send();
    }

    function refresh() {
        fetchListing(function (items) {
            var tbody = document.getElementById("file-rows");
            tbody.innerHTML = "";
            for (var i = 0; i < items.length; i++) {
                var it  = items[i];
                var url = LIST_URL + encodeURIComponent(it.name) + (it.isDir ? "/" : "");
                var tr  = document.createElement("tr");
                tr.innerHTML =
                    "<td><a href='" + url + "' target='_blank'>" + it.name + "</a></td>" +
                    "<td>" +
                        (it.isDir ? "" :
                            "<a class='btn btn-sm btn-ghost' href='" + url + "' download>Download</a> " +
                            "<button class='btn btn-sm btn-danger' data-del='" + it.name + "'>Delete</button>"
                        ) +
                    "</td>";
                tbody.appendChild(tr);
            }
            var btns = tbody.querySelectorAll("[data-del]");
            for (var j = 0; j < btns.length; j++) {
                (function (btn) {
                    btn.addEventListener("click", function () {
                        if (!confirm('Delete "' + btn.getAttribute("data-del") + '"?')) return;
                        var xhr = new XMLHttpRequest();
                        xhr.open("DELETE", LIST_URL + encodeURIComponent(btn.getAttribute("data-del")), true);
                        xhr.onload = function () { refresh(); };
                        xhr.send();
                    });
                })(btns[j]);
            }
        });
    }

    document.addEventListener("DOMContentLoaded", function () {
        document.getElementById("file-input").addEventListener("change", function (e) {
            var form = new FormData();
            for (var i = 0; i < e.target.files.length; i++)
                form.append("file", e.target.files[i], e.target.files[i].name);
            var xhr = new XMLHttpRequest();
            xhr.open("POST", "/uploads", true);
            xhr.onload = function () { refresh(); };
            xhr.send(form);
            e.target.value = "";
        });
        refresh();
    });
})();

