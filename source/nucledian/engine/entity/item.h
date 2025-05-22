#include <engine/entity/entity.h>
#include <engine/player/player.h>
#include <engine/appearance.h>

#include <types.h>       // f32
#include <math/vector.h> // vec3

namespace nc
{
  class PickUp : public Entity
  {
  public:
    PickUp(vec3 position);

    static EntityType get_type_static();

    virtual void on_pickup(Player player);

    const Appearance& get_appearance() const;
  private:
    Appearance appear;
  };
}