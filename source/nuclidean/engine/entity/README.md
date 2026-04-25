## Entity
**entity.h**

Entity serves as a base class to objects that appear in a level. These objects have:
 - position
 - scale *(radius and height)*
 - appearance
 
and can be affected by physics.

The radius of entities is used both for their physical shape and also for visibility calculation and distribution into sectors. To see how are the entities mapped onto sectors check [sector mapping](sector_mapping.h).

Each entity type has to be derived from the [Entity](entity.h) base class. Even though we use inheritance to define the entity types, we avoid virtual tables and dynamic cast. To perform dynamic casting from base entity pointer to a derived one we first do a check if the conversion is valid by comparing the entity type.

These are the currently implemented entity types:
- [**Player**](../player/player.h) - Implements player entity that receives inputs and reacts to them.
- [**Enemy**](../enemies/enemy.h) - Enemy entity with AI, physics and animations.
- [**Pickup**](../../game/item.h) - An object like ammo box, medkit or weapon pickable by player.
- [**Projectile**](../../game/projectile.h) - Shootable projectile that can collide with enemies or player.
- [**Ambient light**](../graphics/entities/lights.h) - A non-directional light affecting the whole level.
- [**Directional light**](../graphics/entities/lights.h) - A directional light affecting the whole level.
- [**Point light**](../graphics/entities/lights.h) - A point light with radius, intensity and color affecting nearby objects and casting shadows.
- [**Prop**](../graphics/entities/prop.h) - A 2D billboard in the world that does nothing. Can be used for prop sprites of the environment or dead enemies.
- [**Sky box**](../graphics/entities/sky_box.h) - A skybox entity. The position and radius are irrelevant, this is only a hack how to get a certain skybox into a level.
- [**Particle**](../../game/particle.h) - A 2D billboard animated particle that can have a light attached to it. Used for blood sprites of flashes of light.
- [**Sound emitter**](../sound/sound_emitter.h) - Represents a 3D movable sound in the level. Changes the volume with distance to the camera.

### Entity System
Defined in [`entity_system.h`](entity_system.h).

This is a system that owns all existing entities within a level. Handles creation and destruction of entities and their tracking.

Each entity has an unique integral `EntityID` which identifies it. A pointer to the entity can be queried from the entity system from this ID. Compared to pointers, integral IDs have multiple advantages:
- IDs, unlike pointers, are valid between game runs. An ID can be stored in a save file and will be valid after deserialization.
- IDs can have less than 64bits. This allows us to store them in framebuffer to identity entities while rendering.
- Nothing breaks after an entity gets destroyed. We simply try to query it with the ID and nullptr is returned. If we used pointers then accessing a memory of destroyed entity would result in undefined behavior.

Even after an entity gets destroyed, its ID does not get reused *for a long time*.

#### Entity System Listener
Systems that want to react to changes of entities *(destruction, creation or movement)* can do so by implementing [`IEntityListener`](entity_system_listener.h).

Examples of such systems are [sector mapping](sector_mapping.h) and [entity attachment manager](../../game/entity_attachment_manager.h).

#### Entity Attachment Manager
Implemented in [entity_attachment_manager.h](../../game/entity_attachment_manager.h).

Allows attaching entities onto each other. The attached entity inherits position changes of its parent entity. Optionally, it can also get destroyed if the parent entity dies.

#### Entity Components
Although our entity system is not a Entity Component System or a Unity-like component system, we still have some traces of components for systems where they are necessary. This allows us to add similar features to more entity types at once.

Used components:
- [`Appearance`](../appearance.h) - Component that determines properties of the sprite the entity has. This includes exact sprite name, its size or how does it rotate with camera.
- `Snap type` - How does entity react to moving floors/ceilings within its sector. For example, dead bodies of enemies should stick to the floor.
