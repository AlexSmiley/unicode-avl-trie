const $ = (s) => document.querySelector(s);
let wasm;
let toastTimer;
function toast(message) {
  const el = $("#toast");
  el.textContent = message;
  el.classList.add("show");
  clearTimeout(toastTimer);
  toastTimer = setTimeout(() => el.classList.remove("show"), 2200);
}
function withString(value, fn) {
  const ptr = wasm.allocateUTF8(value);
  try { return fn(ptr); } finally { wasm._free(ptr); }
}
function search(word) {
  if (!wasm || !word.trim()) return;
  const translation = withString(word, (ptr) => {
    const result = wasm._wasm_search(ptr);
    return result ? wasm.UTF8ToString(result) : null;
  });
  const box = $("#search-result");
  box.replaceChildren();
  box.className = "result " + (translation ? "found" : "missing");
  const text = document.createElement("span");
  if (translation) {
    text.append(document.createTextNode(word + "  →  "));
    const value = document.createElement("strong");
    value.className = "translation";
    value.textContent = translation;
    text.append(value);
    const remove = document.createElement("button");
    remove.type = "button";
    remove.className = "remove-button";
    remove.textContent = "Remove";
    remove.addEventListener("click", () => {
      const removed = withString(word, (ptr) => wasm._wasm_remove(ptr));
      if (removed) {
        box.className = "result missing";
        box.textContent = "Entry removed.";
        toast("Entry removed from the dictionary.");
      }
    });
    box.append(text, remove);
  } else {
    text.textContent = "“" + word + "” is not in this dictionary yet. Add it below.";
    box.append(text);
  }
}
$("#search-form").addEventListener("submit", (event) => {
  event.preventDefault();
  search($("#search-word").value);
});
$("#entry-form").addEventListener("submit", (event) => {
  event.preventDefault();
  if (!wasm) return;
  const word = $("#entry-word").value.trim();
  const translation = $("#entry-translation").value.trim();
  if (!word || !translation) return;
  const ok = withString(word, (w) => withString(translation, (t) => wasm._wasm_add(w, t)));
  if (!ok) { toast("Invalid UTF-8 or translation exceeds 511 bytes."); return; }
  $("#search-word").value = word;
  search(word);
  $("#entry-word").value = "";
  $("#entry-translation").value = "";
  toast("Entry added or updated in the dictionary.");
});
$("#clear-button").addEventListener("click", () => {
  if (!wasm) return;
  wasm._wasm_clear();
  $("#search-result").className = "result";
  $("#search-result").textContent = "Dictionary cleared. Add a word to get started.";
  toast("Dictionary cleared.");
});
document.querySelectorAll("[data-word]").forEach((button) => {
  button.addEventListener("click", () => {
    $("#search-word").value = button.dataset.word;
    search(button.dataset.word);
  });
});
try {
  const { default: createDictionaryModule } = await import("./dictionary.js");
  wasm = await createDictionaryModule();
  wasm._wasm_init();
  const examples = [["hello","a greeting"],["world","the earth; all people and places"],["мачка","cat"],["Македонија","North Macedonia"],["карта","map"],["Map42","A map with numeric suffix"]];
  for (const pair of examples) {
    withString(pair[0], (w) => withString(pair[1], (t) => wasm._wasm_add(w, t)));
  }
  $("#engine-dot").classList.add("ready");
  $("#engine-label").textContent = "C11 / WebAssembly engine ready";
} catch (error) {
  console.error(error);
  $("#engine-dot").classList.add("error");
  $("#engine-label").textContent = "Engine unavailable · run web/build.sh";
  $("#search-result").textContent = "Build the WebAssembly module with Emscripten, then reload this page.";
}
