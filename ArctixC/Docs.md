# ArctixC - Docs

**The full SDK manual lives in two interchangeable files (same content, same 121 functions):**

- **`ArctixC/Documentation.html`** - open it in any browser (double-click the file; no server, no internet needed). Black-and-white liquid-glass dashboard, searchable and clickable function list, syntax-highlighted code. This is the recommended reading experience.
- **`ArctixC/Documentation.md`** - the same manual as Markdown: dashboard, all categories, and every function with signature, arguments, return value, example call, and explanation. For readers, diff tools, and offline grep.

The `.html` file is the single source of truth. After editing it, regenerate the `.md` from the `Arctix/` root:

```powershell
node ArctixC/generate-docs-md.js
```

Build: `powershell -ExecutionPolicy Bypass -File ArctixC\build.ps1` (from the `Arctix/` root). Output: single `arctixc.dll` + `arctixc.lib` in `ArctixC/build/`.
