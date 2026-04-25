## Pickup
: Entity

[*item.h*](item.h)

**Pickup** is an entity that **Player** can walk over and pick up and their inventory can be affected by this **Pickup**.

**PickupType** determines what the **Pickup** is (what it should do when it is picked up and its sprite). Different pickup types are defined in [`item_resources.h`](item_resources.h).

## Projectile
: Entity

[*projectile.h*](projectile.h)

**Projectile** is an entity type that travels in the starting direction and collides with potential enemies or player. It can optionally bounce of the walls for a fixed number of times and can have a sprite and emit light. The collision detection is continuous.

The projectile also has an author entity, which is the entity that created it. Can be used to determine if a enemy was hit by a projectile of his ally or by player.

## Particle
: Entity

[*particle.h*](particle.h)
**Particle** is an entity type with a limited lifetime that optionally emits light or has a animated sprite. After a fixed amount of time passes, the particle destroys itself. Used for blood particles.

## Projectiles
[*projectiles.h*](projectiles.h)

The projectile types are defined in a full data-driven way. There are several projectile types and each one has several properties like radius, speed, lifetime or sprite.

For now, all projectile types are hardcoded in the file [`projectiles.cpp`](projectiles.cpp). This can be potentially replaced by loading from files in the future.

## Weapons
[*weapons.h*](weapons.h)

Defines a stats for each possible weapon the player can have. Data driven. For now, all weapon types are hardcoded in [`weapons.cpp`](weapons.cpp), but can be loaded from file in the future.

## Enemies
[*enemies.h*](enemies.h)

Defines set of stats for each enemy type like speed, health points or damage type. For now, all enemy types are hardcoded in [`enemies.cpp`](enemies.cpp), but in the future they can be loaded from a file.