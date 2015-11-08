Rob Allen GitHub ‘example_invaderers’ Project Report
Branch: messing_about
Link to game demo: https://youtu.be/amUCRY0rxjY

In this report I will seek to convey my experience of using C++ for the ‘example_invaderers’ project. The initial game was forked from Octet from where I started to learn about the workings from a beginner level.

Having no previous experience using C++ I started by looking at some basic tutorials learning how the program worked and how it could be used.

//#include "stdafx.h"
//#include<iostream>

int main()
{
std::cout << "Hello world!" << std::endl;
return 0;
}

The first thing I tried in the solution was to change small things and see how they changed the game, then seeking to find out ‘why’ they changed it.

+	  if (live_invaderers == 3) {
+		  invader_velocity *= 2;
+	  }
+	  else if (live_invaderers == 2) {
+		  invader_velocity *= 3;
+	  }
+	  else if (live_invaderers == 1) {
+		  invader_velocity *= 4;

I started by changing amounts for the invader velocity, learning how they corresponded to the speed of the Invaderers in game. Through this process I learnt about ‘if’ statements which are used to decide between executing two or more blocks of code. So above, if the invaderers left alive are equal to 3 the speed will change to 2, if there are 2 invaderers left the speed will go up to 3 etc. 

I then changed simple assets such as pictures (.gifs) and sounds (.wav). Learning that to add an image, to the code, I must change it in the octet/assets/invaderers folder to be able to implement it in the code. The code below reads the .gif from the assets folder and projects it at a certain location within the game.

+	  GLuint ash = resource_dict::get_texture_handle(GL_RGBA, 
+	"assets/invaderers/ash3.gif");	  
+	  sprites[ash_sprite].init(ash, 0, -2.75f, 0.25f, 0.25f);

One of the initial problems I came across was not knowing how to implement or change aspects of the code to cause differences in the game. For this I used the help of the Msc students. Elio De Berardinis explained how to implement the code for reading CSV files, and the process from start to finish.
The first step was to produce a .csv file in excel consisting of a table of 0 and 1. I also created .gif images in the assets folder for which the 0 and 1 would correspond to.

+	  read_csv();
+	  read_csv2();
+	  setup_visual_map();

Next I had to write the code that would tell the program to implement the .csv into the matrix and what was to be read in C++ from the CSV file.

+	static const int map_width = 20;
+	static const int map_height = 20;
+ 	int map [map_height][map_width];
+ 	int map2 [map_height][map_width];
+	dynarray<sprite> map_sprites_grass;
+	dynarray<sprite> map_sprites_wildgrass;
+	dynarray<sprite> object_sprites;
 	
Then the code for reading the actual .csv file and .gif files. 

//called to read a CSV file for the background map
+	void read_csv() {
+	std::ifstream file("backgroundmap1.csv");
+	char buffer[2048];
+	int i = 0;
+	while (!file.eof()) {
+		file.getline(buffer, sizeof(buffer));
+		char *b = buffer;
+		for (int j = 0; ; ++j) {
+			char *e = b;
+			while (*e != 0 && *e != ',') ++e;
+			map[i][j] = std::atoi(b);
+				if (*e != ',') break;
+				b = e + 1;
+			}
+			++i;
+		}
+	}
 
The code for initialising the images.

+	int num_bush = 0;
+	void setup_visual_map() {
+		GLuint bush = resource_dict::get_texture_handle(GL_RGBA, "assets/invaderers/grass1.gif");
+		GLuint dirt = resource_dict::get_texture_handle(GL_RGBA, "assets/invaderers/wildgrass1.gif");
+		for (int i = 0; i < map_height; ++i) {
+			for (int j = 0; j < map_width; ++j) {
+				sprite s;
+				if (map[i][j] == 1) {
+					s.init(bush, -3 + 0.15f + 0.3f*j, 3 - 0.15f - 0.3*i, 0.3f, 0.3f);
+					map_sprites_bush.push_back(s);
+				}
+				else if (map[i][j] == 0) {
+					s.init(dirt, -3 + 0.15f + 0.3f*j, 3 - 0.15f - 0.3*i, 0.3f, 0.3f);
+					map_sprites_dirt.push_back(s);
+				}
+			}
+		}
+	}
+  };

And finally the code for rendering it in the game. 

+	  //draw the map sprites(bush)
+	  for (unsigned int i = 0; i < map_sprites_bush.size(); ++i) {
+	  map_sprites_bush[i].render(texture_shader_, cameraToWorld);
+	  }
+
+	  //draw the map sprites(dirt)
+	  for (unsigned int i = 0; i < map_sprites_dirt.size(); ++i) {
+		  map_sprites_dirt[i].render(texture_shader_, cameraToWorld);
+	  }
+

The way I used this code was to change the background to two different images depending on where the 0 and 1 were in the CSV file.

I used Mircea Catana’s code for the ‘fragment shader’ and tried to change it myself to fit my goals, learning about shaders in the process. 
Shaders are a program and there are two primary types, Fragment and Vertex shaders. Fragment shaders effect the colour of a pixel while Vertex shaders effect the position of a vertex. 

The first step to implementing a simple shader code is to compile the shader:

<GLuint> glCreateShader (<type>)
//Cretaes an ID of a shader
glShaderSource (<id>, <count>, <src code>, <lengths>)
//Binds the source code to the shader
glCompileShader (<id>)
//Makes part of the shader program.

Then to actually use the shader:

<GLuint> glCreateProgram()
//Returns an ID
glAttachShader (<prog ID>, <shader ID>)
//Call the function
glLinkProgram (<prog ID>)
//Makes the shader program
glUseProgram (<prog ID>)
//Uses the shader when you’re about to draw triangles

In conclusion the things I changed within the game code were:

Speed of invaderers
Speed of ship
Invaderer image 
Ship image
Missile image
Bomb image
Whoosh sound
Bang sound
Array of enemies = top layer were stationary
Movement controls to A,S,W,D and included up and down functions within this
Background colour
Background image using CSV
Game over when sprites reach the bottom of the screen
Number of lives
Position of ‘Score’ and ‘Lives’ to top right
Fragment shader

In the end I went for a Pokémon themed game with Pikachu as the invaderer, Ash Ketchum as the ship and scenes from the game as the background.



