## Things

This module contains different types of entities or objects that can be placed in the world.   
All things contained here derive from the `Thing` class.  

- `Thing` contains the basic logic for determining its placement in the game world.   
- `Entity` is an agent that can move through the level and interact with it and with other agents.   
- `PickUp` is an object (e.g. ammo, medkit) that can be picked by the player.   
- `Prop` is a cosmetic-only bilboard that gets displayed in the level and it's job is to look cool
- `PointLight`, `AmbientLight`, `DirectionalLight` are light sources   

To place a `Thing` in the level, DO NOT assign these scripts directly to blank Nodes. 
Just use an already existing prefab from the `prefabs/Entities|Lights|PickUps|Props` folder
