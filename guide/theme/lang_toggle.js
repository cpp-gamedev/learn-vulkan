'use strict';

(function languages() {
  const langToggleButton = document.getElementById('lang-toggle');
  const langPopup = document.getElementById('lang-list');

  if (!langToggleButton || !langPopup) {
      return; // Safety check: if they're missing from the page, do nothing
  }

  function showLangs() {
      langPopup.style.display = 'block';
      langToggleButton.setAttribute('aria-expanded', 'true');
  }

  function hideLangs() {
      langPopup.style.display = 'none';
      langToggleButton.setAttribute('aria-expanded', 'false');
      langToggleButton.focus();
  }

  langToggleButton.addEventListener('click', function() {
      if (langPopup.style.display === 'block') {
          hideLangs();
      } else {
          showLangs();
      }
  });

  document.addEventListener('click', function (e) {
    if (e.target && e.target.matches('button[data-lang]')) {
      const chosenLang = e.target.getAttribute('data-lang');
      const supportedLangs = ['en', 'zh-TW', 'ko-KR']; // Add translated languages here
  
      let currentPath = window.location.pathname;
  
      // Find "book/" in the path
      const bookAnchor = 'book/';
      const idx = currentPath.indexOf(bookAnchor);
  
      if (idx === -1) {
        // Fallback: If "book/" isn’t in the path
        // Just go to the top-level file in the chosen language.
        if (chosenLang === 'en') 
          window.location.href = 'index.html'; 
        else 
          window.location.href = chosenLang + '/index.html';

        return;
      }
  
      // Everything up to (and including) "book/"
      // e.g. "/C:/Users/.../guide/book/" 
      const base = currentPath.substring(0, idx + bookAnchor.length);
  
      // The rest "after book/" part
      // e.g. "index.html" or "zh-TW/getting_started/foo.html"
      let after = currentPath.substring(idx + bookAnchor.length);
  
      // Split into segments and remove any leading lang if it’s known
      // e.g. ["index.html"] or ["en", "getting_started", "foo.html"]
      let segments = after.split('/').filter(s => s.length > 0);
  
      if (segments.length > 0 && supportedLangs.includes(segments[0])) {
        // remove the first segment if it’s a supported lang
        // e.g. now ["getting_started", "foo.html"]
        segments.shift(); 
      }
  
      // Insert the chosen language as the first path segment
      // Also, English has no prefix
      let newPath;
      if (chosenLang === 'en') {
        // e.g. /C:/Users/.../guide/book/getting_started/foo.html
        newPath = base + segments.join('/');
      } else {
        // e.g. /C:/Users/.../guide/book/zh-TW/getting_started/foo.html
        newPath = base + chosenLang + '/' + segments.join('/');
      }
  
      window.location.href = newPath;
    }
  
    if (
      langPopup.style.display === 'block' &&
      !langToggleButton.contains(e.target) &&
      !langPopup.contains(e.target)
    ) {
      hideLangs();
    }
  });

  // Also hide if focus goes elsewhere
  langPopup.addEventListener('focusout', function(e) {
      // e.relatedTarget can be null in some browsers
      if (!!e.relatedTarget &&
          !langToggleButton.contains(e.relatedTarget) &&
          !langPopup.contains(e.relatedTarget)) {
          hideLangs();
      }
  });

  // Optional: Add keyboard navigation (like theme popup)
  document.addEventListener('keydown', function(e) {
      if (langPopup.style.display !== 'block') return;
      if (e.altKey || e.ctrlKey || e.metaKey || e.shiftKey) return;

      let li;
      switch (e.key) {
          case 'Escape':
              e.preventDefault();
              hideLangs();
              break;
          case 'ArrowUp':
              e.preventDefault();
              li = document.activeElement.parentElement;
              if (li && li.previousElementSibling) {
                  li.previousElementSibling.querySelector('a, button').focus();
              }
              break;
          case 'ArrowDown':
              e.preventDefault();
              li = document.activeElement.parentElement;
              if (li && li.nextElementSibling) {
                  li.nextElementSibling.querySelector('a, button').focus();
              }
              break;
          case 'Home':
              e.preventDefault();
              langPopup.querySelector('li:first-child a, li:first-child button').focus();
              break;
          case 'End':
              e.preventDefault();
              langPopup.querySelector('li:last-child a, li:last-child button').focus();
              break;
      }
  });
})();
