/**
 * DZML Runtime — resolves arbitrary dz-* classes to inline styles.
 * Include this script in your DZML file for classes not pre-generated in dzml.css.
 * 
 * <script src="dzml-runtime.js"></script>
 */
(function() {
  'use strict';

  function resolveClass(el, cls) {
    if (!cls.startsWith('dz-')) return;
    const body = cls.substring(3);

    // Position: xy-{x}-{y}
    if (body.startsWith('xy-')) {
      const val = body.substring(3);
      const parts = splitPair(val);
      if (parts) {
        el.style.left = toCSS(parts[0]);
        el.style.top = toCSS(parts[1]);
      }
      return;
    }

    // Size: wh-{w}-{h}
    if (body === 'wh-full') {
      el.style.width = '100%';
      el.style.height = '100%';
      return;
    }
    if (body.startsWith('wh-')) {
      const val = body.substring(3);
      const parts = splitPair(val);
      if (parts) {
        el.style.width = toCSS(parts[0]);
        el.style.height = toCSS(parts[1]);
      }
      return;
    }

    // Color: c-{hex}
    if (body.startsWith('c-')) {
      const hex = body.substring(2);
      el.style.backgroundColor = hexToCSS(hex);
      return;
    }

    // Text color: tc-{hex}
    if (body.startsWith('tc-')) {
      const hex = body.substring(3);
      el.style.color = hexToCSS(hex);
      return;
    }
  }

  function splitPair(str) {
    // Split on dash, handling px suffix
    const dashes = [];
    for (let i = 0; i < str.length; i++) {
      if (str[i] === '-') dashes.push(i);
    }
    for (const idx of dashes) {
      const left = str.substring(0, idx);
      const right = str.substring(idx + 1);
      if (left.length > 0 && right.length > 0) return [left, right];
    }
    return null;
  }

  function toCSS(val) {
    if (val.endsWith('px')) return val;
    const n = parseInt(val);
    if (isNaN(n)) return val;
    return n + '%';
  }

  function hexToCSS(hex) {
    if (hex.length === 2) {
      const v = parseInt(hex, 16);
      return 'rgb(' + v + ',' + v + ',' + v + ')';
    }
    if (hex.length <= 6) return '#' + hex.padEnd(6, hex);
    if (hex.length === 8) {
      const r = parseInt(hex.substring(0, 2), 16);
      const g = parseInt(hex.substring(2, 4), 16);
      const b = parseInt(hex.substring(4, 6), 16);
      const a = (parseInt(hex.substring(6, 8), 16) / 255).toFixed(3);
      return 'rgba(' + r + ',' + g + ',' + b + ',' + a + ')';
    }
    return '#' + hex;
  }

  // ── Template import system ──
  // Fetches child .dzml fragments referenced by <template data-template-id data-src>
  // and clones them into [data-template] containers for preview.

  var templateCache = {};

  function fetchTemplate(src) {
    if (templateCache[src]) return templateCache[src];
    templateCache[src] = fetch(src + '?v=' + Date.now())
      .then(function(res) {
        if (!res.ok) throw new Error('Failed to fetch ' + src + ': ' + res.status);
        return res.text();
      })
      .then(function(html) {
        // Parse the fragment — may contain one or more root elements
        var doc = new DOMParser().parseFromString(html, 'text/html');
        // Return all child elements from body as a DocumentFragment
        var frag = document.createDocumentFragment();
        while (doc.body.firstChild) {
          frag.appendChild(doc.body.firstChild);
        }
        return frag;
      });
    return templateCache[src];
  }

  function loadTemplates() {
    // Collect all <template data-template-id data-src> declarations
    var declarations = document.querySelectorAll('template[data-template-id][data-src]');
    var templateMap = {};
    declarations.forEach(function(tmpl) {
      templateMap[tmpl.getAttribute('data-template-id')] = tmpl.getAttribute('data-src');
    });

    // Find all containers with data-template
    var containers = document.querySelectorAll('[data-template]');
    var promises = [];

    containers.forEach(function(container) {
      var templateId = container.getAttribute('data-template');
      var src = templateMap[templateId];
      if (!src) return;

      // Determine how many preview items to show
      var cols = 1;
      var classStr = container.getAttribute('class') || '';
      var colMatch = classStr.match(/dz-cols-(\d+)/);
      if (colMatch) cols = parseInt(colMatch[1]);
      // Show enough items to fill 2 rows (or at least 3 items)
      var previewCount = Math.max(3, cols * 2);

      var p = fetchTemplate(src).then(function(frag) {
        for (var i = 0; i < previewCount; i++) {
          var clone = frag.cloneNode(true);
          // Resolve dz-* classes on all elements in the cloned fragment
          var els = clone.querySelectorAll('[data-dz]');
          els.forEach(function(el) {
            var classes = (el.getAttribute('class') || '').split(/\s+/);
            classes.forEach(function(cls) { resolveClass(el, cls); });
          });
          container.appendChild(clone);
        }
      }).catch(function(err) {
        console.warn('Template "' + templateId + '" load error:', err);
      });
      promises.push(p);
    });

    return Promise.all(promises);
  }

  function resolveAll(root) {
    var els = root.querySelectorAll ? root.querySelectorAll('[data-dz]') : [];
    // Also resolve root if it has data-dz
    if (root.getAttribute && root.getAttribute('data-dz')) {
      var rootClasses = (root.getAttribute('class') || '').split(/\s+/);
      rootClasses.forEach(function(cls) { resolveClass(root, cls); });
    }
    els.forEach(function(el) {
      var classes = (el.getAttribute('class') || '').split(/\s+/);
      classes.forEach(function(cls) { resolveClass(el, cls); });
    });
  }

  // ── Navigation system ──
  // Simulates NavButtonBinding + ComponentWidgetBinding in the browser.
  // data-nav-target="selector" → loads the matching sub-page HTML
  // data-component="{{ expr }}" → container that receives sub-pages

  // Map of selector → HTML file path (convention: kebab-case → PascalCase file)
  var pageCache = {};

  function selectorToFilename(selector) {
    // banking-account-tab → AccountTabView.html
    // Remove project prefix (first segment)
    var parts = selector.split('-');
    if (parts.length >= 3) parts.shift(); // remove prefix like "banking"
    // PascalCase the rest
    var name = parts.map(function(p) {
      return p.charAt(0).toUpperCase() + p.slice(1);
    }).join('');
    return name + 'View.html';
  }

  function loadPage(container, selector) {
    var filename = selectorToFilename(selector);

    // Ensure container is a positioning context for absolute children
    container.style.position = 'relative';

    if (pageCache[filename]) {
      renderPage(container, pageCache[filename]);
      return;
    }

    fetch(filename + '?v=' + Date.now())
      .then(function(res) {
        if (!res.ok) throw new Error(res.status);
        return res.text();
      })
      .then(function(html) {
        // Extract only direct [data-dz] children of <body> (root widgets)
        var doc = new DOMParser().parseFromString(html, 'text/html');
        var frag = document.createDocumentFragment();
        Array.from(doc.body.children).forEach(function(el) {
          if (el.getAttribute && el.getAttribute('data-dz')) {
            frag.appendChild(el.cloneNode(true));
          }
        });
        pageCache[filename] = frag;
        renderPage(container, frag);
      })
      .catch(function(err) {
        console.warn('Page load failed: ' + filename, err);
        container.innerHTML = '<div style="color:#666;padding:20px;text-align:center;">Page not found: ' + filename + '</div>';
      });
  }

  function renderPage(container, frag) {
    container.innerHTML = '';
    var clone = frag.cloneNode(true);
    container.appendChild(clone);
    // Resolve dz-* classes on all loaded content
    var els = container.querySelectorAll('[data-dz]');
    els.forEach(function(el) {
      var classes = (el.getAttribute('class') || '').split(/\s+/);
      classes.forEach(function(cls) { resolveClass(el, cls); });
    });
  }

  function setupNavigation() {
    // Find all component containers
    var containers = document.querySelectorAll('[data-component]');

    containers.forEach(function(container) {
      // Load default page (first nav button's target)
      var firstNav = document.querySelector('[data-nav-target]');
      if (firstNav) {
        var defaultSelector = firstNav.getAttribute('data-nav-target');
        loadPage(container, defaultSelector);
        firstNav.classList.add('dz-active');
      }
    });

    // Bind nav buttons
    var navButtons = document.querySelectorAll('[data-nav-target]');
    navButtons.forEach(function(btn) {
      btn.addEventListener('click', function() {
        var selector = btn.getAttribute('data-nav-target');
        var container = document.querySelector('[data-component]');
        if (container) {
          loadPage(container, selector);
        }
        // Update active state on all nav buttons
        navButtons.forEach(function(b) { b.classList.remove('dz-active'); });
        btn.classList.add('dz-active');
      });
    });

    // Bind click handlers (data-on-click) for preview feedback
    var clickables = document.querySelectorAll('[data-on-click]');
    clickables.forEach(function(el) {
      el.addEventListener('click', function() {
        var handler = el.getAttribute('data-on-click');
        console.log('[DZML Preview] ' + handler + '()');
        // Visual feedback
        el.style.opacity = '0.7';
        setTimeout(function() { el.style.opacity = ''; }, 150);
      });
    });
  }

  // Run on DOM ready
  function init() {
    var els = document.querySelectorAll('[data-dz]');
    els.forEach(function(el) {
      var classes = (el.getAttribute('class') || '').split(/\s+/);
      classes.forEach(function(cls) {
        resolveClass(el, cls);
      });
    });

    // Load templates then setup navigation
    loadTemplates().then(function() {
      setupNavigation();
    });
  }

  if (document.readyState === 'loading') {
    document.addEventListener('DOMContentLoaded', init);
  } else {
    init();
  }
})();
