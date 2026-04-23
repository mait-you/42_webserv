/* ============================================================
   webserv File Manager — frontend logic
   Talks to the C++ webserv via plain HTTP:
     GET    /uploads/             -> autoindex listing (parsed)
     POST   /uploads              -> multipart upload
     DELETE /uploads/<name>       -> delete file
   ============================================================ */

(function () {
    const UPLOAD_URL = "/uploads";
    const LIST_URL   = "/uploads/";

    const $  = (sel, el = document) => el.querySelector(sel);
    const $$ = (sel, el = document) => Array.from(el.querySelectorAll(sel));

    /* ---------- formatters ---------- */
    function humanSize(n) {
        if (n === undefined || n === null || isNaN(n)) return "—";
        const u = ["B","KB","MB","GB","TB"];
        let i = 0; let v = Number(n);
        while (v >= 1024 && i < u.length - 1) { v /= 1024; i++; }
        return v.toFixed(v >= 10 || i === 0 ? 0 : 1) + " " + u[i];
    }

    function extOf(name) {
        const i = name.lastIndexOf(".");
        return i < 0 ? "" : name.slice(i + 1).toLowerCase();
    }

    function iconFor(name, isDir) {
        if (isDir) return { cls: "ftype-folder", txt: "DIR" };
        const e = extOf(name);
        if (["png","jpg","jpeg","gif","svg","webp"].includes(e)) return { cls: "ftype-img", txt: "IMG" };
        if (["zip","tar","gz","rar","7z"].includes(e))           return { cls: "ftype-zip", txt: "ZIP" };
        if (["pdf","doc","docx","txt","md"].includes(e))         return { cls: "ftype-doc", txt: "DOC" };
        return { cls: "", txt: e ? e.toUpperCase().slice(0,3) : "FIL" };
    }

    /* ---------- listing ----------
       webserv autoindex returns a simple HTML page with <a href="...">
       links. We parse those to build our own table.
    */
    async function fetchListing() {
        const res = await fetch(LIST_URL, { credentials: "include" });
        if (!res.ok) throw new Error("HTTP " + res.status);
        const html = await res.text();
        const doc = new DOMParser().parseFromString(html, "text/html");
        const items = [];
        // Try to read size from each <a>'s parent text. webserv autoindex
        // typically prints:  <a href="x">x</a>   12-Apr-2024 10:00   1234
        doc.querySelectorAll("a").forEach(a => {
            const href = a.getAttribute("href") || "";
            const name = decodeURIComponent(a.textContent || href).trim();
            if (!name || name === "../" || name === "..") return;
            const isDir = href.endsWith("/");

            let size = null;
            // Look at the trailing text after the link in its line.
            const parent = a.parentNode;
            if (parent) {
                const tail = (parent.textContent || "").split(name).pop() || "";
                const m = tail.match(/(\d{2,})\s*$/);
                if (m && !isDir) size = parseInt(m[1], 10);
            }

            items.push({
                name: name.replace(/\/$/, ""),
                href: href,
                isDir: isDir,
                size: size
            });
        });
        return items;
    }

    function renderTable(items, filter) {
        const tbody = $("#file-rows");
        const empty = $("#empty");
        tbody.innerHTML = "";

        const f = (filter || "").toLowerCase();
        const list = items.filter(it => !f || it.name.toLowerCase().includes(f));

        if (!list.length) {
            empty.style.display = "block";
            $("#file-table-wrap").style.display = "none";
            return;
        }
        empty.style.display = "none";
        $("#file-table-wrap").style.display = "block";

        for (const it of list) {
            const tr = document.createElement("tr");

            const ico = iconFor(it.name, it.isDir);
            const linkHref = LIST_URL + encodeURIComponent(it.name) + (it.isDir ? "/" : "");

            tr.innerHTML = `
                <td>
                    <div class="fname">
                        <div class="ftype-ico ${ico.cls}">${ico.txt}</div>
                        <div>
                            <div><a href="${linkHref}" target="_blank">${escapeHtml(it.name)}</a></div>
                            <div class="muted" style="font-size:12px;">
                                ${it.isDir ? "Directory" : (extOf(it.name).toUpperCase() || "File")}
                            </div>
                        </div>
                    </div>
                </td>
                <td>${it.isDir ? "—" : (it.size != null ? humanSize(it.size) : "—")}</td>
                <td class="muted">${it.isDir ? "Folder" : "Stored on disk"}</td>
                <td>
                    <div class="row-actions">
                        ${it.isDir ? "" : `<a class="btn btn-sm btn-ghost" href="${linkHref}" download>Download</a>`}
                        ${it.isDir ? "" : `<button class="btn btn-sm btn-danger" data-del="${escapeHtml(it.name)}">Delete</button>`}
                    </div>
                </td>
            `;
            tbody.appendChild(tr);
        }

        tbody.querySelectorAll("[data-del]").forEach(btn => {
            btn.addEventListener("click", () => {
                const name = btn.getAttribute("data-del");
                deleteFile(name);
            });
        });

        $("#stat-files").textContent = items.filter(i => !i.isDir).length;
        $("#stat-dirs").textContent  = items.filter(i =>  i.isDir).length;
    }

    function escapeHtml(s) {
        return String(s).replace(/[&<>"']/g, c => ({
            "&":"&amp;","<":"&lt;",">":"&gt;","\"":"&quot;","'":"&#39;"
        }[c]));
    }

    /* ---------- upload ---------- */
    function uploadFiles(files) {
        if (!files || !files.length) return;
        const form = new FormData();
        for (const f of files) form.append("file", f, f.name);

        const xhr = new XMLHttpRequest();
        xhr.open("POST", UPLOAD_URL, true);
        xhr.withCredentials = true;

        const bar     = $("#progress-bar");
        const wrap    = $("#progress");
        const status  = $("#upload-status");
        wrap.classList.add("show");
        bar.style.width = "0%";
        status.textContent = "Uploading " + files.length + " file(s)…";

        xhr.upload.addEventListener("progress", e => {
            if (e.lengthComputable) {
                const pct = Math.round((e.loaded / e.total) * 100);
                bar.style.width = pct + "%";
                status.textContent = "Uploading… " + pct + "%";
            }
        });
        xhr.onload = () => {
            if (xhr.status >= 200 && xhr.status < 300) {
                bar.style.width = "100%";
                status.textContent = "Upload complete.";
                setTimeout(() => { wrap.classList.remove("show"); status.textContent = ""; }, 1500);
                refresh();
            } else {
                status.textContent = "Upload failed (HTTP " + xhr.status + ").";
            }
        };
        xhr.onerror = () => { status.textContent = "Network error during upload."; };
        xhr.send(form);
    }

    /* ---------- delete ---------- */
    async function deleteFile(name) {
        if (!confirm("Delete \"" + name + "\"? This cannot be undone.")) return;
        try {
            const res = await fetch(LIST_URL + encodeURIComponent(name), {
                method: "DELETE",
                credentials: "include"
            });
            if (!res.ok && res.status !== 204) {
                alert("Delete failed: HTTP " + res.status);
                return;
            }
            refresh();
        } catch (e) {
            alert("Delete error: " + e.message);
        }
    }

    /* ---------- session cookie demo ---------- */
    function ensureSession() {
        if (!document.cookie.split(";").some(c => c.trim().startsWith("sid="))) {
            const sid = Math.random().toString(36).slice(2) + Date.now().toString(36);
            document.cookie = "sid=" + sid + "; path=/; max-age=86400; SameSite=Lax";
        }
    }

    /* ---------- wiring ---------- */
    let allItems = [];
    async function refresh() {
        try {
            allItems = await fetchListing();
            renderTable(allItems, $("#search").value);
        } catch (e) {
            $("#empty").style.display = "block";
            $("#empty h3").textContent = "Couldn't load /uploads";
            $("#empty p").textContent  = e.message;
        }
    }

    document.addEventListener("DOMContentLoaded", () => {
        ensureSession();

        $("#file-input").addEventListener("change", e => uploadFiles(e.target.files));
        $("#refresh-btn").addEventListener("click", refresh);
        $("#search").addEventListener("input", e => renderTable(allItems, e.target.value));

        const drop = $("#upload-card");
        ["dragenter","dragover"].forEach(ev =>
            drop.addEventListener(ev, e => { e.preventDefault(); drop.classList.add("drag"); })
        );
        ["dragleave","drop"].forEach(ev =>
            drop.addEventListener(ev, e => { e.preventDefault(); drop.classList.remove("drag"); })
        );
        drop.addEventListener("drop", e => uploadFiles(e.dataTransfer.files));

        refresh();
    });
})();
