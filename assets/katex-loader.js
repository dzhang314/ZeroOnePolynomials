(function () {
  const loadScript = (url, callback) => {
    const script = document.createElement("script");
    script.src = url;
    script.onload = callback;
    document.head.appendChild(script);
  };
  loadScript("https://cdn.jsdelivr.net/npm/katex@0.17.0/dist/katex.min.js", () => {
    loadScript("https://cdn.jsdelivr.net/npm/katex@0.17.0/dist/contrib/auto-render.min.js", () => {
      const render = container => {
        container.querySelectorAll(".math.inline, .math.display").forEach(m =>
          katex.render(m.textContent, m, {
            displayMode: m.classList.contains("display"),
            throwOnError: false,
          })
        );
        container.querySelectorAll(".docstring").forEach(d =>
          renderMathInElement(d, {
            delimiters: [
              { left: '$$', right: '$$', display: true },
              { left: '$', right: '$', display: false },
            ],
            throwOnError: false,
          })
        );
      };
      const _tippy = window.tippy;
      if (_tippy) {
        window.tippy = (target, properties) => {
          if (properties) {
            const _onMount = properties.onMount;
            properties.onMount = tooltip => {
              if (_onMount) _onMount(tooltip);
              render(tooltip.popper);
            };
          }
          return _tippy(target, properties);
        };
      }
      if (document.readyState === "loading") {
        document.addEventListener("DOMContentLoaded", () => render(document));
      } else {
        render(document);
      }
    });
  });
})();
