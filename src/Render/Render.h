#pragma once

#include	"stdafx.h"


class Render {

	Texture waitb;
	Texture wait;
	Texture texb;
	Texture texp;
	Texture texz;
	Texture texbl;
	Texture texf;
	Texture texsm;
	Texture texl;

	Sprite waitingb;
	Sprite waiting;

	Transform wtransform;

	Font font;
	
	Sprite background;

public:

	Render() {}

	Texture& GetTexture(std::string name) {
		if (name == "player") return texp;
		else if (name == "zombie") return texz;
		else if (name == "blood") return texbl;
		else if (name == "fire") return texf;
		else if (name == "smoke") return texsm;
		else if (name == "light") return texl;
	}

	Font GetFont() { return font; }

	void RenderStatic() {
		window.clear();
		window.draw(background);
		
	}

	void LoadTextures() {
		if (!waitb.loadFromFile("Textures\\waiting.jpg")) printf("ERROR: unexpected texture");
		if (!wait.loadFromFile("Textures\\wait.png")) printf("ERROR: unexpected texture");
		if (!texb.loadFromFile("Textures\\backgroundnew.jpg")) printf("ERROR: unexpected texture");
		if (!texp.loadFromFile("Textures\\player.png")) printf("ERROR: unexpected texture");
		if (!texz.loadFromFile("Textures\\zombie.png")) printf("ERROR: unexpected texture");
		if (!texbl.loadFromFile("Textures\\blood.png")) printf("ERROR: unexpected texture");
		if (!texf.loadFromFile("Textures\\fire.png")) printf("ERROR: unexpected texture");
		if (!texsm.loadFromFile("Textures\\smoke.png")) printf("ERROR: unexpected texture");
		if (!texl.loadFromFile("Textures\\light.png")) printf("ERROR: unexpected texture");
		font.loadFromFile("Fonts/PixellettersFull.ttf");
		waitingb.setTexture(waitb);
		waiting.setTexture(wait);
		background.setTexture(texb);
	}

	void UIWait() {
		waiting.setPosition(W_WIDTH / 2 - 150, W_HEIGHT / 2 - 150);
		window.clear();
		window.draw(waitingb);
		wtransform.rotate(2, Vector2f(W_WIDTH / 2, W_HEIGHT / 2));
		window.draw(waiting, wtransform);
		window.display();
	}

};