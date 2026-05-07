## Enemy
: Entity

*enemy.h*

**Enemy** is a Non Playable Character that is hostile to the player.

**Enemy** has three states:
 - idle: where it checks if the player is in its field of view or if it heard the player
 - alert: the enemy paths towards a player and attacks them if they are in a range
 - dead: enemy has player its dying animation and does nothing

 Animations are handled using **AnimSFM**.