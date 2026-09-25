#pragma once


char* message[] = {
	(char*)"Welcome to system anti-virus",
	(char*)"You must shoot viruses to remove them",
	(char*)"Is viruses win your pc will break!"
};

const float pixel_scale = 2;
Texture2D player_texture;
Texture2D enemy_cancer_texture;
Texture2D enemy_koronavirus_texture;
Texture2D enemy_bad_texture;

Texture2D cursor_texture;

Rectangle player_rectangle;
Rectangle enemy_cancer_rectangle;
Rectangle enemy_koronavirus_rectangle;

Rectangle cursor_rectangle;