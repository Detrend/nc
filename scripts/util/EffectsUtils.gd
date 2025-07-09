extends Object
class_name EffectsUtils

static func do_particles_one_shot(particles: GPUParticles2D)->void:
	if !particles: return;
	particles.set_one_shot(true);
	particles.restart();
