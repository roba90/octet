////////////////////////////////////////////////////////////////////////////////
//
// (C) Andy Thomason 2012-2014
//
// Modular Framework for OpenGLES2 rendering on multiple platforms.
//
// invaderer example: simple game with sprites and sounds
//
// Level: 1
//
// Demonstrates:
//   Basic framework app
//   Shaders
//   Basic Matrices
//   Simple game mechanics
//   Texture loaded from GIF file
//   Audio
//

namespace octet {
  class sprite {
    // where is our sprite (overkill for a 2D game!)
    mat4t modelToWorld;

    // half the width of the sprite
    float halfWidth;

    // half the height of the sprite
    float halfHeight;

    // what texture is on our sprite
    int texture;

    // true if this sprite is enabled.
    bool enabled;
  public:
    sprite() {
      texture = 0;
      enabled = true;
    }

    void init(int _texture, float x, float y, float w, float h) {
      modelToWorld.loadIdentity();
      modelToWorld.translate(x, y, 0);
      halfWidth = w * 0.5f;
      halfHeight = h * 0.5f;
      texture = _texture;
      enabled = true;
    }

    void render(texture_shader &shader, mat4t &cameraToWorld) {
      // invisible sprite... used for gameplay.
      if (!texture) return;

      // build a projection matrix: model -> world -> camera -> projection
      // the projection space is the cube -1 <= x/w, y/w, z/w <= 1
      mat4t modelToProjection = mat4t::build_projection_matrix(modelToWorld, cameraToWorld);

      // set up opengl to draw textured triangles using sampler 0 (GL_TEXTURE0)
      glActiveTexture(GL_TEXTURE0);
      glBindTexture(GL_TEXTURE_2D, texture);

      // use "old skool" rendering
      //glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_NEAREST);
      //glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
      shader.render(modelToProjection, 0);

      // this is an array of the positions of the corners of the sprite in 3D
      // a straight "float" here means this array is being generated here at runtime.
      float vertices[] = {
        -halfWidth, -halfHeight, 0,
         halfWidth, -halfHeight, 0,
         halfWidth,  halfHeight, 0,
        -halfWidth,  halfHeight, 0,
      };

      // attribute_pos (=0) is position of each corner
      // each corner has 3 floats (x, y, z)
      // there is no gap between the 3 floats and hence the stride is 3*sizeof(float)
      glVertexAttribPointer(attribute_pos, 3, GL_FLOAT, GL_FALSE, 3*sizeof(float), (void*)vertices );
      glEnableVertexAttribArray(attribute_pos);
    
      // this is an array of the positions of the corners of the texture in 2D
      static const float uvs[] = {
         0,  0,
         1,  0,
         1,  1,
         0,  1,
      };

	  // aargh!

      // attribute_uv is position in the texture of each corner
      // each corner (vertex) has 2 floats (x, y)
      // there is no gap between the 2 floats and hence the stride is 2*sizeof(float)
      glVertexAttribPointer(attribute_uv, 2, GL_FLOAT, GL_FALSE, 2*sizeof(float), (void*)uvs );
      glEnableVertexAttribArray(attribute_uv);
    
      // finally, draw the sprite (4 vertices)
      glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
    }

	void render(rob_shader &shader, mat4t &cameraToWorld, int v_width, int v_height) {
		
			mat4t modelToProjection = mat4t::build_projection_matrix(modelToWorld, cameraToWorld);
		
			shader.render(modelToProjection, vec2(v_width, v_height));
		
			float vertices[] = {
			-halfWidth, -halfHeight, 0,
			halfWidth, -halfHeight, 0,
			halfWidth,  halfHeight, 0,
			-halfWidth,  halfHeight, 0,
			};
		
			glVertexAttribPointer(attribute_pos, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)vertices);
		glEnableVertexAttribArray(attribute_pos);
		
			static const float uvs[] = {
			0,  0,
			1,  0,
			1,  1,
			0,  1,
			};
		
			glVertexAttribPointer(attribute_uv, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)uvs);
		glEnableVertexAttribArray(attribute_uv);
		
			glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
		
	}


    // move the object
    void translate(float x, float y) {
      modelToWorld.translate(x, y, 0);
    }

    // position the object relative to another.
    void set_relative(sprite &rhs, float x, float y) {
      modelToWorld = rhs.modelToWorld;
      modelToWorld.translate(x, y, 0);
    }

    // return true if this sprite collides with another.
    // note the "const"s which say we do not modify either sprite
    bool collides_with(const sprite &rhs) const {
      float dx = rhs.modelToWorld[3][0] - modelToWorld[3][0];
      float dy = rhs.modelToWorld[3][1] - modelToWorld[3][1];

      // both distances have to be under the sum of the halfwidths
      // for a collision
      return
        (fabsf(dx) < halfWidth + rhs.halfWidth) &&
        (fabsf(dy) < halfHeight + rhs.halfHeight)
      ;
    }

    bool is_above(const sprite &rhs, float margin) const {
      float dx = rhs.modelToWorld[3][0] - modelToWorld[3][0];

      return
        (fabsf(dx) < halfWidth + margin)
      ;
    }

    bool &is_enabled() {
      return enabled;
    }
  };

  class invaderers_app : public octet::app {
    // Matrix to transform points in our camera space to the world.
    // This lets us move our camera
    mat4t cameraToWorld;

    // shader to draw a textured triangle
    texture_shader texture_shader_;
	rob_shader rob_shader_;

    enum {
      num_sound_sources = 8,
      num_rows = 5,
      num_cols = 10,
      num_missiles = 2,
      num_bombs = 2,
      num_borders = 4,
      num_invaderers = num_rows * num_cols,

      // sprite definitions
      ash_sprite = 0,
      game_over_sprite,

      first_invaderer_sprite,
      last_invaderer_sprite = first_invaderer_sprite + num_invaderers - 1,

      first_missile_sprite,
      last_missile_sprite = first_missile_sprite + num_missiles - 1,

      first_bomb_sprite,
      last_bomb_sprite = first_bomb_sprite + num_bombs - 1,

      first_border_sprite,
      last_border_sprite = first_border_sprite + num_borders - 1,

      num_sprites,

    };

    // timers for missiles and bombs
    int missiles_disabled;
    int bombs_disabled;

    // accounting for bad guys
    int live_invaderers;
    int num_lives;

    // game state
    bool game_over;
    int score;

    // speed of enemy
    float invader_velocity;

    // sounds
    ALuint whoosh;
    ALuint bang;
    unsigned cur_source;
    ALuint sources[num_sound_sources];

    // big array of sprites
    sprite sprites[num_sprites];
	sprite background_sprite;

    // random number generator
    class random randomizer;

    // a texture for our text
    GLuint font_texture;

    // information for our text
    bitmap_font font;

	enum direction {
		LEFT,
		RIGHT,
		UP,
		DOWN
	};

	direction myDirection;

	static const int map_width = 20;
	static const int map_height = 20;
	int map [map_height][map_width];
	int map2 [map_height][map_width];
	dynarray<sprite> map_sprites_grass;
	dynarray<sprite> map_sprites_wildgrass;
	dynarray<sprite> object_sprites;

    ALuint get_sound_source() { return sources[cur_source++ % num_sound_sources]; }

    // called when we hit an enemy
    void on_hit_invaderer() {
      ALuint source = get_sound_source();
      alSourcei(source, AL_BUFFER, bang);
      alSourcePlay(source);

      live_invaderers--;
      score++;
	  if (live_invaderers == 3) {
		  invader_velocity *= 2;
	  }
	  else if (live_invaderers == 2) {
		  invader_velocity *= 3;
	  }
	  else if (live_invaderers == 1) {
		  invader_velocity *= 4;
	  
      } else if (live_invaderers == 0) {
        game_over = true;
        sprites[game_over_sprite].translate(-20, 0);
      }
    }

    // called when we are hit
    void on_hit_ash() {
      ALuint source = get_sound_source();
      alSourcei(source, AL_BUFFER, bang);
      alSourcePlay(source);

      if (--num_lives == 0) {
        game_over = true;
        sprites[game_over_sprite].translate(-20, 0);
      }
    }

    // use the keyboard to move Ash
	void move_ash() {
		const float ash_speed = 0.2f;
		// left and right arrows
		if (is_key_down((char)'A')) {
			sprites[ash_sprite].translate(-ash_speed, 0);
			if (sprites[ash_sprite].collides_with(sprites[first_border_sprite + 2])) {
				sprites[ash_sprite].translate(+ash_speed, 0);
			}
		}
		if (is_key_down((char)'D')) {
			sprites[ash_sprite].translate(+ash_speed, 0);
			if (sprites[ash_sprite].collides_with(sprites[first_border_sprite + 3])) {
				sprites[ash_sprite].translate(-ash_speed, 0);
			}
		}	
		if (is_key_down((char)'W')) {
			sprites[ash_sprite].translate(0, +ash_speed);
			if (sprites[ash_sprite].collides_with(sprites[first_border_sprite + 1])) {
				sprites[ash_sprite].translate(0, -ash_speed);
			}
		}
		if (is_key_down((char)'S')) {
			sprites[ash_sprite].translate(0, -ash_speed);
			if (sprites[ash_sprite].collides_with(sprites[first_border_sprite + 0])) {
				sprites[ash_sprite].translate(0, +ash_speed);
			}
		}
	}

    // fire button (space)
    void fire_missiles() {
      if (missiles_disabled) {
        --missiles_disabled;
      } else if (is_key_going_down(' ')) {
        // find a missile
        for (int i = 0; i != num_missiles; ++i) {
          if (!sprites[first_missile_sprite+i].is_enabled()) {
            sprites[first_missile_sprite+i].set_relative(sprites[ash_sprite], 0, 0.5f);
            sprites[first_missile_sprite+i].is_enabled() = true;
            missiles_disabled = 5;
            ALuint source = get_sound_source();
            alSourcei(source, AL_BUFFER, whoosh);
            alSourcePlay(source);
            break;
          }
        }
      }
    }

    // pick and invader and fire a bomb
    void fire_bombs() {
      if (bombs_disabled) {
        --bombs_disabled;
      } else {
        // find an invaderer
        sprite &ash = sprites[ash_sprite];
        for (int j = randomizer.get(0, num_invaderers); j < num_invaderers; ++j) {
          sprite &invaderer = sprites[first_invaderer_sprite+j];
          if (invaderer.is_enabled() && invaderer.is_above(ash, 0.3f)) {
            // find a bomb
            for (int i = 0; i != num_bombs; ++i) {
              if (!sprites[first_bomb_sprite+i].is_enabled()) {
                sprites[first_bomb_sprite+i].set_relative(invaderer, 0, -0.25f);
                sprites[first_bomb_sprite+i].is_enabled() = true;
                bombs_disabled = 30;
                ALuint source = get_sound_source();
                alSourcei(source, AL_BUFFER, whoosh);
                alSourcePlay(source);
                return;
              }
            }
            return;
          }
        }
      }
    }

    // animate the missiles
    void move_missiles() {
      const float missile_speed = 0.3f;
      for (int i = 0; i != num_missiles; ++i) {
        sprite &missile = sprites[first_missile_sprite+i];
        if (missile.is_enabled()) {
          missile.translate(0, missile_speed);
          for (int j = 0; j != num_invaderers; ++j) {
            sprite &invaderer = sprites[first_invaderer_sprite+j];
            if (invaderer.is_enabled() && missile.collides_with(invaderer)) {
              invaderer.is_enabled() = false;
              invaderer.translate(20, 0);
              missile.is_enabled() = false;
              missile.translate(20, 0);
              on_hit_invaderer();

              goto next_missile;
            }
          }
          if (missile.collides_with(sprites[first_border_sprite+1])) {
            missile.is_enabled() = false;
            missile.translate(20, 0);
          }
        }
      next_missile:;
      }
    }

    // animate the bombs
    void move_bombs() {
      const float bomb_speed = 0.2f;
      for (int i = 0; i != num_bombs; ++i) {
        sprite &bomb = sprites[first_bomb_sprite+i];
        if (bomb.is_enabled()) {
          bomb.translate(0, -bomb_speed);
          if (bomb.collides_with(sprites[ash_sprite])) {
            bomb.is_enabled() = false;
            bomb.translate(20, 0);
            bombs_disabled = 50;
            on_hit_ash();
            goto next_bomb;
          }
          if (bomb.collides_with(sprites[first_border_sprite+0])) {
            bomb.is_enabled() = false;
            bomb.translate(20, 0);
          }
        }
      next_bomb:;
      }
    }

    // move the array of enemies
    void move_invaders(float dx, float dy) {
      for (int j = 0; j != num_invaderers; ++j) {
        sprite &invaderer = sprites[first_invaderer_sprite+j];
        if (invaderer.is_enabled()) {
          invaderer.translate(dx, dy);
		  if (sprites[first_invaderer_sprite + j].collides_with(sprites[first_border_sprite + 0])) {
			  num_lives = 0;
			 // draw_text(texture_shader_, 1.95f, 2, 1.0f / 256, "GameOver");
			  sprites[game_over_sprite].translate(-20, 0);
			  game_over = true;
			  //sprites reach bottom of screen = game_over
		  }
        }
      }
    }

    // check if any invaders hit the sides.
    bool invaders_collide(sprite &border) {
      for (int j = 0; j != num_invaderers; ++j) {
        sprite &invaderer = sprites[first_invaderer_sprite+j];
        if (invaderer.is_enabled() && invaderer.collides_with(border)) {
          return true;
        }
      }
      return false;
    }


    void draw_text(texture_shader &shader, float x, float y, float scale, const char *text) {
      mat4t modelToWorld;
      modelToWorld.loadIdentity();
      modelToWorld.translate(x, y, 0);
      modelToWorld.scale(scale, scale, 1);
      mat4t modelToProjection = mat4t::build_projection_matrix(modelToWorld, cameraToWorld);

      /*mat4t tmp;
      glLoadIdentity();
      glTranslatef(x, y, 0);
      glGetFloatv(GL_MODELVIEW_MATRIX, (float*)&tmp);
      glScalef(scale, scale, 1);
      glGetFloatv(GL_MODELVIEW_MATRIX, (float*)&tmp);*/

      enum { max_quads = 32 };
      bitmap_font::vertex vertices[max_quads*4];
      uint32_t indices[max_quads*6];
      aabb bb(vec3(0, 0, 0), vec3(256, 256, 0));

      unsigned num_quads = font.build_mesh(bb, vertices, indices, max_quads, text, 0);
      glActiveTexture(GL_TEXTURE0);
      glBindTexture(GL_TEXTURE_2D, font_texture);

      shader.render(modelToProjection, 0);

      glVertexAttribPointer(attribute_pos, 3, GL_FLOAT, GL_FALSE, sizeof(bitmap_font::vertex), (void*)&vertices[0].x );
      glEnableVertexAttribArray(attribute_pos);
      glVertexAttribPointer(attribute_uv, 3, GL_FLOAT, GL_FALSE, sizeof(bitmap_font::vertex), (void*)&vertices[0].u );
      glEnableVertexAttribArray(attribute_uv);

      glDrawElements(GL_TRIANGLES, num_quads * 6, GL_UNSIGNED_INT, indices);
    }

  public:

    // this is called when we construct the class
    invaderers_app(int argc, char **argv) : app(argc, argv), font(512, 256, "assets/big.fnt") {
    }

    // this is called once OpenGL is initialized
    void app_init() {
      // set up the shader
      texture_shader_.init();
	  rob_shader_.init();

	  read_csv();
	  read_csv2();
	  setup_visual_map();
	  

	  myDirection = RIGHT;


      // set up the matrices with a camera 5 units from the origin
      cameraToWorld.loadIdentity();
      cameraToWorld.translate(0, 0, 3);

      font_texture = resource_dict::get_texture_handle(GL_RGBA, "assets/big_0.gif");

      GLuint ash = resource_dict::get_texture_handle(GL_RGBA, "assets/invaderers/ash3.gif");
      sprites[ash_sprite].init(ash, 0, -2.75f, 0.25f, 0.25f);

      GLuint GameOver = resource_dict::get_texture_handle(GL_RGBA, "assets/invaderers/GameOver.gif");
      sprites[game_over_sprite].init(GameOver, 20, 0, 3, 1.5f);

      GLuint invaderer = resource_dict::get_texture_handle(GL_RGBA, "assets/invaderers/pikachu2.gif");
      for (int j = 0; j != num_rows; ++j) {
        for (int i = 0; i != num_cols; ++i) {
          assert(first_invaderer_sprite + i + j*num_cols <= last_invaderer_sprite);
          sprites[first_invaderer_sprite + i + j*num_cols].init(
            invaderer, ((float)i - num_cols * 0.5f) * 0.5f, 2.50f - ((float)j * 0.5f), 0.25f, 0.25f
          );
        }
      }

      // set the border to white for clarity
      GLuint white = resource_dict::get_texture_handle(GL_RGB, "#ffffff");
      sprites[first_border_sprite+0].init(white, 0, -3, 6, 0.2f);
      sprites[first_border_sprite+1].init(white, 0,  3, 6, 0.2f);
      sprites[first_border_sprite+2].init(white, -3, 0, 0.2f, 6);
      sprites[first_border_sprite+3].init(white, 3,  0, 0.2f, 6);
	  background_sprite.init(white, 0, 0, 6, 6);

      // use the missile texture
      GLuint missile = resource_dict::get_texture_handle(GL_RGBA, "assets/invaderers/ball1.gif");
      for (int i = 0; i != num_missiles; ++i) {
        // create missiles off-screen
        sprites[first_missile_sprite+i].init(missile, 20, 0, 0.0625f, 0.25f);
        sprites[first_missile_sprite+i].is_enabled() = false;
      }

      // use the bomb texture
      GLuint bomb = resource_dict::get_texture_handle(GL_RGBA, "assets/invaderers/bolt3.gif");
      for (int i = 0; i != num_bombs; ++i) {
        // create bombs off-screen
        sprites[first_bomb_sprite+i].init(bomb, 20, 0, 0.0625f, 0.25f);
        sprites[first_bomb_sprite+i].is_enabled() = false;
      }

      // sounds
      whoosh = resource_dict::get_sound_handle(AL_FORMAT_MONO16, "assets/invaderers/bolt.wav");
      bang = resource_dict::get_sound_handle(AL_FORMAT_MONO16, "assets/invaderers/bang.wav");
      cur_source = 0;
      alGenSources(num_sound_sources, sources);

      // sundry counters and game state.
      missiles_disabled = 0;
      bombs_disabled = 50;
      invader_velocity = 0.01f;
      live_invaderers = num_invaderers;
      num_lives = 3;
      game_over = false;
      score = 0;
    }

    // called every frame to move things
    void simulate() {
      if (game_over) {

        return;
      }

      move_ash();

      fire_missiles();

      fire_bombs();

      move_missiles();

      move_bombs();

      move_invaders(invader_velocity, 0);

      sprite &border = sprites[first_border_sprite+(invader_velocity < 0 ? 2 : 3)];
      if (invaders_collide(border)) {
        invader_velocity = -invader_velocity;
        move_invaders(invader_velocity, -0.5f);
      }
    }

    // this is called to draw the world
    void draw_world(int x, int y, int w, int h) {
      simulate();

      // set a viewport - includes whole window area
      glViewport(x, y, w, h);

      // clear the background to black
      glClearColor(0, 0, 0, 1);
      glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

      // don't allow Z buffer depth testing (closer objects are always drawn in front of far ones)
      glDisable(GL_DEPTH_TEST);

	  // draw background
	  background_sprite.render(rob_shader_, cameraToWorld, w, h);

      // allow alpha blend (transparency when alpha channel is 0)
      glEnable(GL_BLEND);
      glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	  //draw the map sprites(grass)
	  for (unsigned int i = 0; i < map_sprites_grass.size(); ++i) {
	  map_sprites_grass[i].render(texture_shader_, cameraToWorld);
	  }

	  //draw the map sprites(wildgrass)
	  for (unsigned int i = 0; i < map_sprites_wildgrass.size(); ++i) {
		  map_sprites_wildgrass[i].render(texture_shader_, cameraToWorld);
	  }

      // draw all the sprites
      for (int i = 0; i != num_sprites; ++i) {
        sprites[i].render(texture_shader_, cameraToWorld);
      }

      char score_text[32];
      sprintf(score_text, "score: %d   lives: %d\n", score, num_lives);
      draw_text(texture_shader_, 1.95f, 2, 1.0f/256, score_text);

      // move the listener with the camera
      vec4 &cpos = cameraToWorld.w();
      alListener3f(AL_POSITION, cpos.x(), cpos.y(), cpos.z());
    }
	//called to read a CSV file for the background map
	void read_csv() {

		std::ifstream file("backgroundmap1.csv");

		char buffer[2048];
		int i = 0;

		while (!file.eof()) {
			file.getline(buffer, sizeof(buffer));

			char *b = buffer;
			for (int j = 0; ; ++j) {
				char *e = b;
				while (*e != 0 && *e != ',') ++e;

				map[i][j] = std::atoi(b);

				if (*e != ',') break;
				b = e + 1;
			}
			++i;
		}
	}

	// read CSV for object map
	void read_csv2() {

		std::ifstream file("backgroundmap1.csv");

		char buffer[2048];
		int i = 0;

		while (!file.eof()) {
			file.getline(buffer, sizeof(buffer));

			char *b = buffer;
			for (int j = 0;; ++j) {
				char *e = b;
				while (*e != 0 && *e != ',') ++e;

				map2[i][j] = std::atoi(b);

				if (*e != ',') break;
				b = e + 1;
			}
			++i;
		}
	}

	int num_grass = 0;

	void setup_visual_map() {

		GLuint grass = resource_dict::get_texture_handle(GL_RGBA, "assets/invaderers/grass1.gif");
		GLuint wildgrass = resource_dict::get_texture_handle(GL_RGBA, "assets/invaderers/wildgrass1.gif");

		for (int i = 0; i < map_height; ++i) {
			for (int j = 0; j < map_width; ++j) {

				sprite s;


				if (map[i][j] == 1) {

					s.init(grass, -3 + 0.15f + 0.3f*j, 3 - 0.15f - 0.3*i, 0.3f, 0.3f);
					map_sprites_grass.push_back(s);
						



				}
				else if (map[i][j] == 0) {
					s.init(wildgrass, -3 + 0.15f + 0.3f*j, 3 - 0.15f - 0.3*i, 0.3f, 0.3f);
					map_sprites_wildgrass.push_back(s);
				}
			}
		}
	}



  };



}
