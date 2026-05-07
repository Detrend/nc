## Input System

The input system, defined in [`input_system.h`](input_system.h), handles player input that is later requested from game system. It also handles cursor visibility *(for example, in menu the cursor should be visible, but not during a gameplay)*.

Stores both previous and current player inputs, so that changes in the input can be detected. The input data is defined in [`game_input.h`](game_input.h) and currently, these exact inputs are tracked:
- `forward`
- `backward`
- `left`
- `right`
- `jump`
- `use` - opening doors, activating buttons
- `primary weapon` - shooting
- `secondary weapon` - unused
- `weapon 0` - melee
- `weapon 1` - shotgun
- `weapon 2` - plasma gun
- `weapon 3` - nail gun *(not implemented in the game)*