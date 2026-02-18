================================================================================
NOTES ON SUPER FAST BINARY DATA SERIALIZATION

Upsides:
. Would help us achieve the rewind time ability
. Would be extra fast for saving/loading = no stutters, player does not even notice
. Serialization of entities would work out of the box - no need to manually
  serialize each data member

Downsides:
. Requires a significant refactor (described bellow)
. Its a binary serialization and therefore would be unreadable and would cause
  a lot of incompatibilities between different versions of the game
. Requires treating stuff like data and not like objects - can't use pointers,
  virtual functions or containers like vectors

How would it work:
All serialized data would have to be stored together in a one long memory pool,
which would allow us to (de)serialize it using only one quick memcpy. No
move constructors or initialization, just a memcpy. This way, the only limiting
factor performance wise would be memory speed.

Reworks required if we would like to implement this:

. Entity types - they would all have to be POD and stored in memory consecutively
  . Either make Appearance a POD, or remove it from entities
    . Making it POD would require creating static string class
  . ActorFSM has pointers, store only some index
  . Remove pointer to the entity register from entity base
    . We need to get the handle to registry from elsewhere
  . Paths on enemies can't be stored in a vector, but in a static array

. Entity registry
  . Store entities in memory consecutively
  . Load them from binary

. Entity mapping
  . Rebuild mapping after loading all new entities

. Attachment manager
  . Store mapping in binary, load on startup
    . Maybe remake it to be linear in memory so it can be scanned into binary?

. Map system
  . Remember states of the sectors

. Map dynamics
  . Store dynamic trigger data in map system, or serialize it into a binary

================================================================================
NOTES ON A COMMON BASE CLASS FOR ENEMY AND PLAYER

There are several elements that player and enemy have in common.. For example:
. facing direction
. velocity
. handling of the movement
. moving through portals
. health/attack
. update of anim state machine
. and other..
================================================================================
