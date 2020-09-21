#include "Mode.hpp"

#include "Scene.hpp"

#include <glm/glm.hpp>

#include <vector>
#include <deque>

struct PlayMode : Mode {
	PlayMode();
	virtual ~PlayMode();

	//functions called by main loop:
	virtual bool handle_event(SDL_Event const &, glm::uvec2 const &window_size) override;
	virtual void update(float elapsed) override;
	virtual void draw(glm::uvec2 const &drawable_size) override;

	//----- game state -----

	//input tracking:
	struct Button {
		uint8_t downs = 0;
		uint8_t pressed = 0;
	} left, right, down, up;

	//local copy of the game scene (so code can change it during gameplay):
	Scene scene;

	Scene::Transform *cube = nullptr;
	Scene::Transform *knot = nullptr;
	std::vector<glm::vec3> collectibles;
	glm::vec3 needle_pos;
	int cur_collectible = 0;
	glm::quat cube_base_rotation;
	glm::quat knot_base_rotation;
	glm::vec3 cube_base_position;
	glm::vec3 knot_base_position;
	glm::vec3 camera_base_position;
	float swing = 0.0f;
	float swollen_timer = 2.0f;
	bool swollen = false;
	bool balloon_blown_up = false;
	bool reset = false;
	int times_played = 1;
	
	//camera:
	Scene::Camera *camera = nullptr;

};
