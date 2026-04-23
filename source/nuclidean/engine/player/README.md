## Player
: Entity

*Player.h*

**Player** is an entity controled by the player. It uses **PlayerSpecificInputs** to determine actions of this entity.

Inputs determine:
 - Movement (WASD + space)
 - Looking (mouse)
 - Weapon switching and using (weapon numbers + mouse)
 - With world interaction (use - E)

Player class also includes **Camera** instance for player POV and **AnimFSM** instance for weapon animations.