(function() {
    const s = document.createElement("script");
    s.src = "https://cdn.jsdelivr.net/npm/katex@0.17.0/dist/katex.min.js";
    const render = () => document.querySelectorAll(".math.inline, .math.display").forEach(m => 
        katex.render(m.textContent, m, { displayMode: m.classList.contains("display"), throwOnError: false })
    );
    s.onload = () => document.readyState === "loading" ? document.addEventListener("DOMContentLoaded", render) : render();
    document.head.appendChild(s);
})();
