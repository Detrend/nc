# ui-04: remaining GL cleanup bugs and destructor leaks across the UI layer

**Source:** code_review_findings_ui.txt, findings 3 and 4 (remainder)
**Files:** source/nuclidean/engine/ui/ui_hud_display.cpp,
           source/nuclidean/engine/ui/ui_screen_effect.h/.cpp,
           source/nuclidean/engine/ui/user_interface_system.cpp,
           source/nuclidean/engine/ui/ui_menu_page.cpp (OptionsPage),
           source/nuclidean/engine/ui/ui_button.h

## ui_hud_display.cpp: same VAO-delete + duplicate-upload pattern as MenuManager (fixed separately as `ui-03`)
`glDeleteBuffers(1, &VAO)` -> `glDeleteVertexArrays(1, &VAO)`, and removed
the redundant pre-VAO `glBufferData` call in `init()`.

## UiScreenEffect: no destructor at all (GL resource leak) + duplicate upload
`UiScreenEffect` created a VAO+VBO in `init()` and had no destructor.
Added `~UiScreenEffect()` (declared in the header, defined in the .cpp)
that deletes both, using `glDeleteVertexArrays` for the VAO. Also removed
the same redundant pre-VAO `glBufferData` duplicate-upload found in the
other two GL-owning UI classes.

## UserInterfaceSystem: screen_effect never deleted
`~UserInterfaceSystem()` deleted `hud_display` and `menu` but not
`screen_effect`. Added `delete screen_effect;`.

## OptionsPage destructor: 4 more missing deletes
`~OptionsPage()` deleted most of its buttons/text but not
`crosshair_text`, `shadow_text`, `shadow_on_button`, or
`shadow_off_button`. Added all four.

## UiButton: no virtual destructor + uninitialized texture_name
- `UiButton` has virtual `on_click()`/`draw()` but no virtual destructor.
  `UiLoadGameButton` (which owns a `std::string`) is currently only ever
  deleted through its exact type (`std::vector<UiLoadGameButton*>`), so
  this was latent -- but the first place that deletes a derived button
  through a `UiButton*` gets UB and a leaked `std::string`. Added
  `virtual ~UiButton() = default;`.
- The default `UiButton()` constructor (empty body, no member init list)
  left `texture_name` (`const char*`) uninitialized; a default-constructed
  button that gets drawn would pass garbage to
  `TextureManager::operator[]`. Added a default member initializer
  (`= nullptr`) in the header.

All of these are one-shot leaks/UB that exist only because ownership here
is manual `new`/`delete` rather than e.g. `std::unique_ptr` -- fixed each
call site directly rather than restructuring ownership, to keep the change
minimal and match the existing pattern in each file.
