#include "PlayMode.hpp"

#include "LitColorTextureProgram.hpp"

#include "DrawLines.hpp"
#include "Mesh.hpp"
#include "Load.hpp"
#include "gl_errors.hpp"
#include "data_path.hpp"

#include <glm/gtc/type_ptr.hpp>

#include <random>

GLuint hexapod_meshes_for_lit_color_texture_program = 0;
Load< MeshBuffer > hexapod_meshes(LoadTagDefault, []() -> MeshBuffer const * {
	MeshBuffer const *ret = new MeshBuffer(data_path("shabby.pnct"));
	hexapod_meshes_for_lit_color_texture_program = ret->make_vao_for_program(lit_color_texture_program->program);
	return ret;
});

Load< Scene > hexapod_scene(LoadTagDefault, []() -> Scene const * {
	return new Scene(data_path("shabby.scene"), [&](Scene &scene, Scene::Transform *transform, std::string const &mesh_name){
		Mesh const &mesh = hexapod_meshes->lookup(mesh_name);

		scene.drawables.emplace_back(transform);
		Scene::Drawable &drawable = scene.drawables.back();

		drawable.pipeline = lit_color_texture_program_pipeline;

		drawable.pipeline.vao = hexapod_meshes_for_lit_color_texture_program;
		drawable.pipeline.type = mesh.type;
		drawable.pipeline.start = mesh.start;
		drawable.pipeline.count = mesh.count;

	});
});

PlayMode::PlayMode() : scene(*hexapod_scene) {
	if (SDL_GetRelativeMouseMode() == SDL_FALSE) {
		SDL_SetRelativeMouseMode(SDL_TRUE);
	}
	collectibles.resize(8);
	for (auto &transform : scene.transforms) {
		if (transform.name == "Cube") cube = &transform;
		else if (transform.name == "Knot") knot = &transform;
		else if (transform.name == "Collectible1") collectibles[0] = transform.position;
		else if (transform.name == "Collectible2") collectibles[1] = transform.position;
		else if (transform.name == "Collectible3") collectibles[2] = transform.position;
		else if (transform.name == "Collectible4") collectibles[3] = transform.position;
		else if (transform.name == "Collectible5") collectibles[4] = transform.position;
		else if (transform.name == "Collectible6") collectibles[5] = transform.position;
		else if (transform.name == "Collectible7") collectibles[6] = transform.position;
		else if (transform.name == "Collectible8") collectibles[7] = transform.position;
		else if (transform.name == "Needle") needle_pos = transform.position;
	}
	if (cube == nullptr) throw std::runtime_error("Cube not found.");
	if (knot == nullptr) throw std::runtime_error("Knot not found.");

	cube_base_rotation = cube->rotation;
	knot_base_rotation = knot->rotation;
	cube_base_position = cube->position;
	knot_base_position = knot->position;
	cube->scale = glm::vec3(0.7f, 0.7f, 0.7f);

	//get pointer to camera for convenience:
	if (scene.cameras.size() != 1) throw std::runtime_error("Expecting scene to have exactly one camera, but it has " + std::to_string(scene.cameras.size()));
	camera = &scene.cameras.front();
	camera_base_position = camera->transform->position;
}

PlayMode::~PlayMode() {
}

bool PlayMode::handle_event(SDL_Event const &evt, glm::uvec2 const &window_size) {

	if (evt.type == SDL_KEYDOWN) {
		if (evt.key.keysym.sym == SDLK_a) {
			left.downs += 1;
			left.pressed = true;
			return true;
		} else if (evt.key.keysym.sym == SDLK_d) {
			right.downs += 1;
			right.pressed = true;
			return true;
		} else if (evt.key.keysym.sym == SDLK_w) {
			up.downs += 1;
			up.pressed = true;
			return true;
		} else if (evt.key.keysym.sym == SDLK_s) {
			down.downs += 1;
			down.pressed = true;
			return true;
		} else if (evt.key.keysym.sym == SDLK_SPACE) {
			reset = true;
			return true;
		}
	} else if (evt.type == SDL_KEYUP) {
		if (evt.key.keysym.sym == SDLK_a) {
			left.pressed = false;
			return true;
		} else if (evt.key.keysym.sym == SDLK_d) {
			right.pressed = false;
			return true;
		} else if (evt.key.keysym.sym == SDLK_w) {
			up.pressed = false;
			return true;
		} else if (evt.key.keysym.sym == SDLK_s) {
			down.pressed = false;
			return true;
		}
	} else if (evt.type == SDL_MOUSEMOTION) {
		if (SDL_GetRelativeMouseMode() == SDL_TRUE) {
			glm::vec2 motion = glm::vec2(
				evt.motion.xrel / float(window_size.y),
				-evt.motion.yrel / float(window_size.y)
			);
			camera->transform->rotation = glm::normalize(
				camera->transform->rotation
				* glm::angleAxis(-motion.x * camera->fovy, glm::vec3(0.0f, 1.0f, 0.0f))
				* glm::angleAxis(motion.y * camera->fovy, glm::vec3(1.0f, 0.0f, 0.0f))
			);
			return true;
		}
	}

	return false;
}

void PlayMode::update(float elapsed) {
	if (reset && balloon_blown_up) {
		reset = false;
		balloon_blown_up = false;
		cube->position = cube_base_position;
		knot->position = knot_base_position;
		cube->scale = glm::vec3(0.7f, 0.7f, 0.7f);
		knot->scale = glm::vec3(1.0f, 1.0f, 1.0f);
		camera->transform->position = camera_base_position;
		cur_collectible = 0;
		swollen = false;
		swollen_timer = 2.0f;
		times_played++;
	}

	//slowly rotates through [0,1):
	swing += elapsed / 5.0f;
	swing -= std::floor(swing);

	cube->rotation = cube_base_rotation * glm::angleAxis(
		glm::radians(5.0f * std::sin(swing * 2.0f * float(M_PI))),
		glm::vec3(1.0f, 0.0f, 0.0f)
	);

	float knot_swing = swing;

	// reposition cube
	glm::vec3 move = glm::vec3(0);

	float speed = fmax(0.005f * (7.0f - ((float) cur_collectible)), 0.025f);
	if (left.pressed && !right.pressed) move.y = -speed;
	if (!left.pressed && right.pressed) move.y = speed;
	if (down.pressed && !up.pressed) move.x = speed;
	if (!down.pressed && up.pressed) move.x = -speed;

	cube->position += move;
	knot->position += move;
	camera->transform->position += move;

	if (swollen) {
		swollen_timer -= elapsed;
		if (swollen_timer >= 0.0f) {
			cube->scale += glm::vec3(0.0007f, 0.0007f, 0.0007f);
			knot->scale += glm::vec3(0.0007f, 0.0007f, 0.0007f);
			cube->position.z += 0.006f;
			knot->position.z += 0.006f;
			camera->transform->position.z += 0.005f;
			knot_swing = swing * 5.0f;
		} else {
			cur_collectible++;
			swollen = false;
		}
	} else {
		if (cur_collectible < 8) {
			float dx = cube->position.x - collectibles[cur_collectible].x;
			float dy = cube->position.y - collectibles[cur_collectible].y;
			float dz = cube->position.z - collectibles[cur_collectible].z;
			if (dx * dx + dy * dy + dz * dz <= 1.0f) {
				swollen = true;
				swollen_timer = 2.0f;
			}
		} else {
			float dx = cube->position.x - needle_pos.x;
			float dy = cube->position.y - needle_pos.y;
			float dz = cube->position.z - needle_pos.z;
			if (dx * dx + dy * dy + dz * dz <= 4.0f) {
				balloon_blown_up = true;
				cube->scale += glm::vec3(100.0f, 100.0f, 100.0f);
				knot->scale += glm::vec3(100.0f, 100.0f, 100.0f);
			}
		}
	}

	knot->rotation = knot_base_rotation * glm::angleAxis(
		glm::radians(25.0f * std::sin(knot_swing * 2.0f * float(M_PI))),
		glm::vec3(0.0f, 0.0f, 1.0f)
	);

	//reset button press counters:
	left.downs = 0;
	right.downs = 0;
	up.downs = 0;
	down.downs = 0;
}

void PlayMode::draw(glm::uvec2 const &drawable_size) {
	//update camera aspect ratio for drawable:
	camera->aspect = float(drawable_size.x) / float(drawable_size.y);

	//set up light type and position for lit_color_texture_program:
	// TODO: consider using the Light(s) in the scene to do this
	glUseProgram(lit_color_texture_program->program);
	glUniform1i(lit_color_texture_program->LIGHT_TYPE_int, 1);
	GL_ERRORS();
	glUniform3fv(lit_color_texture_program->LIGHT_DIRECTION_vec3, 1, glm::value_ptr(glm::vec3(0.0f, 0.0f,-1.0f)));
	GL_ERRORS();
	glUniform3fv(lit_color_texture_program->LIGHT_ENERGY_vec3, 1, glm::value_ptr(glm::vec3(1.0f, 1.0f, 0.95f)));
	GL_ERRORS();
	glUseProgram(0);

	glClearColor(0.5f, 0.5f, 0.5f, 1.0f);
	glClearDepth(1.0f); //1.0 is actually the default value to clear the depth buffer to, but FYI you can change it.
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LESS); //this is the default depth comparison function, but FYI you can change it.

	scene.draw(*camera);

	{ //use DrawLines to overlay some text:
		glDisable(GL_DEPTH_TEST);
		float aspect = float(drawable_size.x) / float(drawable_size.y);
		DrawLines lines(glm::mat4(
			1.0f / aspect, 0.0f, 0.0f, 0.0f,
			0.0f, 1.0f, 0.0f, 0.0f,
			0.0f, 0.0f, 1.0f, 0.0f,
			0.0f, 0.0f, 0.0f, 1.0f
		));

		constexpr float H = 0.09f;
		if (balloon_blown_up) {
			switch (times_played) {
				case 1:
					lines.draw_text("Our life is just like this balloon", glm::vec3(-aspect + 8.0f * H, -1.0 + 10.0f * H, 0.0f),
						glm::vec3(H * 2.0f, 0.0f, 0.0f), glm::vec3(0.0f, H * 2.0f, 0.0f),
						glm::u8vec4(0x00, 0x00, 0x00, 0x00));
					lines.draw_text("Press Space to Restart to Get Next Line", glm::vec3(-aspect + 10.0f * H, -1.0 + 8.0f * H, 0.0f),
						glm::vec3(H, 0.0f, 0.0f), glm::vec3(0.0f, H, 0.0f),
						glm::u8vec4(0xff, 0xff, 0xff, 0x00));
					break;
				case 2:
					lines.draw_text("We rise up to get screwed by...", glm::vec3(-aspect + 10.0f * H, -1.0 + 10.0f * H, 0.0f),
						glm::vec3(H * 2.0f, 0.0f, 0.0f), glm::vec3(0.0f, H * 2.0f, 0.0f),
						glm::u8vec4(0x00, 0x00, 0x00, 0x00));
					lines.draw_text("Press Space", glm::vec3(-aspect + 18.0f * H, -1.0 + 8.0f * H, 0.0f),
						glm::vec3(H, 0.0f, 0.0f), glm::vec3(0.0f, H, 0.0f),
						glm::u8vec4(0xff, 0xff, 0xff, 0x00));
					break;
				case 3:
					lines.draw_text("Something so subtle like this needle, yet...", glm::vec3(-aspect + 8.0f * H, -1.0 + 10.0f * H, 0.0f),
						glm::vec3(H * 2.0f, 0.0f, 0.0f), glm::vec3(0.0f, H * 2.0f, 0.0f),
						glm::u8vec4(0x00, 0x00, 0x00, 0x00));
					break;
				case 4:
					lines.draw_text("Yet we have no other choice but to rise again, and...", glm::vec3(-aspect + 6.0f * H, -1.0 + 10.0f * H, 0.0f),
						glm::vec3(H * 1.5f, 0.0f, 0.0f), glm::vec3(0.0f, H * 1.5f, 0.0f),
						glm::u8vec4(0x00, 0x00, 0x00, 0x00));
					break;
				case 5:
					lines.draw_text("And there is no way out as long as we live.", glm::vec3(-aspect + 6.0f * H, -1.0 + 10.0f * H, 0.0f),
						glm::vec3(H * 2.0f, 0.0f, 0.0f), glm::vec3(0.0f, H * 2.0f, 0.0f),
						glm::u8vec4(0x00, 0x00, 0x00, 0x00));
					break;
				default:
					lines.draw_text("BOOOOOOOOOOOM!!!!!!!!!", glm::vec3(-aspect + 10.0f * H, -1.0 + 10.0f * H, 0.0f),
						glm::vec3(H * 3.0f, 0.0f, 0.0f), glm::vec3(0.0f, H * 3.0f, 0.0f),
						glm::u8vec4(0x00, 0x00, 0x00, 0x00));
					lines.draw_text("Press Space to Restart", glm::vec3(-aspect + 16.0f * H, -1.0 + 8.0f * H, 0.0f),
						glm::vec3(H, 0.0f, 0.0f), glm::vec3(0.0f, H, 0.0f),
						glm::u8vec4(0xff, 0xff, 0xff, 0x00));
			}
		} else {
			lines.draw_text("Use WASD to navigate the balloon; adjust camera using the mouse",
				glm::vec3(-aspect + 0.1f * H, -1.0 + 0.1f * H, 0.0),
				glm::vec3(H, 0.0f, 0.0f), glm::vec3(0.0f, H, 0.0f),
				glm::u8vec4(0x00, 0x00, 0x00, 0x00));
		}
	}
}
