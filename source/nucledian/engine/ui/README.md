# UserInterfaceSystem
: IEngineModule

*user_interface_module.h*

**UserInterfaceSystem** servers as a wrapper for all UI elements, these being **MenuManager**, **Heads-Up Display** (HUD) and **ScreenEffect**.

Being an IEngineModule, it inherits on_event() method, however draw() method has to be publicly accessible as it has to be called from GraphicsSystem to be rendered, as to be called after GLClearBuffer() and before GLSwapBuffers().

## MenuManager
*ui_button.h*

**MenuManager** handles rendering and interactions of main menu which appears at the start of the game and when player presses ESC during gameplay. **MenuManager** also handles rendering of cursor and mouse input.

### Pages
The menu is separated into pages, each page being a whole that can be shown one at a time. These pages then contain graphics and buttons that can be interracted with.

 - **MainMenuPage**: main hub between other pages
 - **NewGamePage**: page to choose a level, interacts with *GameSystem*
 - **OptionsPage**: page to modify settings, because of this, the page interacts with multiple systems:
    - *SoundSystem*: sound and music volumes
    - *InputSystem*: mouse sensitivity
    - *HUD*: crosshair
 - **LoadGamePage**: page where player can pick a previously saved game, interracts with *GameSystem*
 - **QuitGamePage**: page that serves as a confirmation for exiting the game, may request engine to quit

 Then there is a special page that is rendered in between levels.
  - **NextLavelPage**: intermission between levels, includes button that requests engine to transition to next level or to transition to menu if it was the last level

### UiButton
UiButtons are interactive elements in menus.

**UiButton** has these properties:
 - *position*: position of the center of button
 - *scale*: size of button
 - *texture_name*: name of texture to be rendered
 - *func*: function to be called
 - *isHover*: whether the cursor is above the button, determines if the button should be render darker in the shader.

All buttons are rendered using a single *VAO*. This *VAO* and button properties are then used in a *ui_button* shader to render the button.

 ### UiLoadGameButton
: UiButton

Button specialized for loading of game and rendering text that represents the save file. Because of this, the button does not include *texture_name* or *func* and is rendered using *ui_text* shader.

## Heads-Up Display (HUD)
*ui_hud_display.h*
The *Heads-Up Display* conveys gameplay information to the player, the information beeing health amount and ammo count for currently held weapon. These values are read from current *Player* instance.

Rendering is done using *ui_button* shader for graphics and *ui_text* shader for values.

## ScreenEffect
*ui_screen_effect.h*

This class handless screen effents (flash near edges of screen) and when should the effect appear (when the player is damaged or picks up an item), fade out and the final rendering of screen effect.

**ScreenEffect** thus interacts with current *Player* instance.

## Shaders
There are two shaders used to draw the UI.

 ### ui_button
 Renders a graphic (an image) with a specific scale on a specific position. The graphic may be darker if a *HOVER* variable is true.

 ### ui_text
  Given a font graphic (image with a grid) and coordinates, the shader then picks out a character from the font and renders it in a given position with a given scale.

  In order to work properly, the shader needs to know how many characters are in a line and in a collumn.