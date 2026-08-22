# ui-03: MenuManager VAO deleted via glDeleteBuffers, quit_game_page leaked, vertex data uploaded twice

**Source:** code_review_findings_ui.txt, findings 3 and 4 (MenuManager part)
**File:** source/nuclidean/engine/ui/ui_menu_manager.cpp

## Issue 1: VAO deleted with glDeleteBuffers instead of glDeleteVertexArrays
```
glDeleteBuffers(1, &VAO);
```
A VAO must be deleted with `glDeleteVertexArrays`. VAO and buffer names
live in separate namespaces, so this leaked the VAO and -- worse --
deleted whatever unrelated buffer object happened to have the same
numeric name.

### Fix
Changed to `glDeleteVertexArrays(1, &VAO);`.

## Issue 2: quit_game_page never deleted
`~MenuManager()` deleted every other page (`main_menu_page`,
`options_page`, `load_game_page`, `new_game_page`, `next_level_page`) but
not `quit_game_page` -- a one-shot leak on shutdown, existing only because
ownership here is manual `new`/`delete` rather than e.g.
`std::unique_ptr`.

### Fix
Added `delete quit_game_page;` alongside the others.

## Issue 3 (related, same constructor): vertex data uploaded twice
The constructor called `glBufferData` once before the VAO existed, then
called it again (identical data, same VBO) right after binding the VAO --
the first upload was entirely redundant.

### Fix
Removed the first (pre-VAO) `glBufferData` call; the second one (correctly
performed while the VAO is bound, so the vertex attrib state gets
recorded) is the one that matters and was left as-is.
