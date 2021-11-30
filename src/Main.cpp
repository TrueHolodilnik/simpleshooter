
#define _CRT_SECURE_NO_WARNINGS
#define _WIN32_WINNT 0x0500
#define W_WIDTH 1920
#define W_HEIGHT 1080

#include	"stdafx.h"

#include	<vector>
#include	<Windows.h>
#include	<winsock.h>
#include	<mutex>

#pragma comment(lib, "WSOCK32.lib")
RenderWindow window(VideoMode(W_WIDTH, W_HEIGHT), "Shooter: Main Menu", Style::Default);

//
#include	"Render/Render.h"
#include	"Entities/Player.h"
#include	"Entities/Bullet.h"
#include	"Entities/Enemy.h"
#include	"Entities/Particle.h"
#include	"UI/TextField.h"
//

std::mutex mtx;
int role = 5;
bool ready = 1, loaded = 0;

struct gameData {
	std::vector<Enemy> enemies;
	std::vector<Player> players;
	int score;
} data;

struct clientData {
	Player player;
} cldata;

struct hostTaskData {
	SOCKET s;
	int role;
};

struct clientTaskData {
	char addr[16];
	char nickname[16];
};

DWORD WINAPI clientTask(void* args) {
	SOCKET sckt;
	struct sockaddr_in sa;
	struct clientTaskData* cd = (struct clientTaskData*)args;
	WSADATA wsas;
	WORD wersja;
	wersja = MAKEWORD(2, 0);
	WSAStartup(wersja, &wsas);
	sckt = socket(AF_INET, SOCK_STREAM, 0);
	memset((void*)(&sa), 0, sizeof(sa));
	sa.sin_family = AF_INET;
	sa.sin_port = htons(10101);
	sa.sin_addr.s_addr = inet_addr(cd->addr);

	int result, size, size2, score;
	float rot, x, y;
	bool is_dead = false, click;
	float health;
	char n[16];

	Enemy enemy;
	Player player(25.0f, Vector2f(0, 0), "P");
	
	while (connect(sckt, (struct sockaddr FAR*) & sa, sizeof(sa)) == SOCKET_ERROR);
	
	recv(sckt, (char*)&result, sizeof(int), 0);
	role = result + 1;

	recv(sckt, (char*)&x, sizeof(float), 0);
	recv(sckt, (char*)&y, sizeof(float), 0);
	player.setPosition(Vector2f(x, y));

	for (int i = 0; i < strlen(cd->nickname); i++)
		n[i] = cd->nickname[i];
	n[strlen(cd->nickname)] = '\0';
	player.setNickname(n);

	cldata.player = player;

	send(sckt, n, 16, 0);

	std::vector<Enemy> enemies;
	std::vector<Player> players;
	std::string nn;

	while (1) {
		recv(sckt, (char*)&score, sizeof(int), 0);

		recv(sckt, (char*)&size, sizeof(int), 0);
		if (enemies.size() > size)
			for (int i = enemies.size() - size; i > 0; i--)
				enemies.erase(enemies.begin() + i - 1);
		else if (size > enemies.size())
			for (int i = size - enemies.size(); i > 0; i--)
				enemies.push_back(enemy);

		for (int i = 0; i < size; i++) {
			recv(sckt, (char*)&x, sizeof(float), 0);
			recv(sckt, (char*)&y, sizeof(float), 0);
			enemies[i].setPosition(sf::Vector2f(x, y));
		}

		recv(sckt, (char*)&size, sizeof(int), 0);

		if (players.size() > size)
			for (int i = players.size() - size; i > 0; i--)
				players.erase(players.begin() + i - 1);
		else if (size > players.size())
			for (int i = size - players.size(); i > 0; i--)
				players.push_back(player);

		for (int i = 0; i < size; i++) {
			recv(sckt, (char*)&x, sizeof(float), 0);
			recv(sckt, (char*)&y, sizeof(float), 0);
			recv(sckt, (char*)&result, sizeof(int), 0);
			recv(sckt, (char*)&rot, sizeof(float), 0);
			recv(sckt, n, 16, 0);
			recv(sckt, (char*)&is_dead, sizeof(bool), 0);
			recv(sckt, (char*)&click, sizeof(bool), 0);
			recv(sckt, (char*)&health, sizeof(float), 0);
			
			players[i].setPosition(Vector2f(x, y));
			players[i].setDir(result);
			players[i].setRotation(rot);
			players[i].setNickname(n);
			players[i].setClick(click);
			players[i].setDead(is_dead);
			players[i].setHealth(health);
			recv(sckt, (char*)&x, sizeof(float), 0);
			recv(sckt, (char*)&y, sizeof(float), 0);
			players[i].setAim(Vector2f(x, y));
		}

		mtx.lock();
		data.players = players;
		data.enemies = enemies;
		data.score = score;
		mtx.unlock();

		loaded = 1;
		ready = 1;
		while (ready);

		mtx.lock();
		player = cldata.player;
		mtx.unlock();

		result = player.getDir();
		rot = player.getRotation();
		is_dead = player.isDead();
		click = player.getClick();
		health = player.getHealth();
		x = player.getAim().x;
		y = player.getAim().y;

		send(sckt, (char*)&result, sizeof(int), 0);
		send(sckt, (char*)&rot, sizeof(float), 0);
		send(sckt, (char*)&x, sizeof(float), 0);
		send(sckt, (char*)&y, sizeof(float), 0);
		send(sckt, (char*)&is_dead, sizeof(bool), 0);
		send(sckt, (char*)&click, sizeof(bool), 0);
		send(sckt, (char*)&health, sizeof(float), 0);
	}
	return 0;
}
DWORD WINAPI hostTask(void* args) {
	struct hostTaskData* st = (struct hostTaskData*) args;
	int size, size2, dir, score;
	float rot, x, y, health;
	bool is_dead, click;
	std::string nn;
	char n[16];
	
	std::vector<Enemy> tenemies;
	std::vector<Player> tplayers;

	while (1) {
		ready = 1;
		while (ready);

		mtx.lock();
		tenemies = data.enemies;
		tplayers = data.players;
		score = data.score;
		mtx.unlock();

		send(st->s, (char*)&score, sizeof(float), 0);

		size = tenemies.size();
		send(st->s, (char*)&size, sizeof(int), 0);
		for (int i = 0; i < size; i++) {
			x = tenemies[i].getPosition().x;
			y = tenemies[i].getPosition().y;
			send(st->s, (char*)&x, sizeof(float), 0);
			send(st->s, (char*)&y, sizeof(float), 0);
		}

		size = tplayers.size();
		send(st->s, (char*)&size, sizeof(int), 0);
		for (int i = 0; i < size; i++) {
			x = tplayers[i].getPosition().x;
			y = tplayers[i].getPosition().y;
			dir = tplayers[i].getDir();
			rot = tplayers[i].getRotation();
			nn = tplayers[i].getNickname();
			std::copy(nn.begin(), nn.end(), n);
			n[nn.end() - nn.begin()] = '\0';
			is_dead = tplayers[i].isDead();
			click = tplayers[i].getClick();
			health = tplayers[i].getHealth();

			send(st->s, (char*)&x, sizeof(float), 0);
			send(st->s, (char*)&y, sizeof(float), 0);
			send(st->s, (char*)&dir, sizeof(int), 0);
			send(st->s, (char*)&rot, sizeof(float), 0);
			send(st->s, n, 16, 0);
			send(st->s, (char*)&is_dead, sizeof(bool), 0);
			send(st->s, (char*)&click, sizeof(bool), 0);
			send(st->s, (char*)&health, sizeof(float), 0);
			x = tplayers[i].getAim().x;
			y = tplayers[i].getAim().y;
			send(st->s, (char*)&x, sizeof(float), 0);
			send(st->s, (char*)&y, sizeof(float), 0);
		}

		ready = 1;

		recv(st->s, (char*)&dir, sizeof(int), 0);
		recv(st->s, (char*)&rot, sizeof(float), 0);
		recv(st->s, (char*)&x, sizeof(float), 0);
		recv(st->s, (char*)&y, sizeof(float), 0);
		recv(st->s, (char*)&is_dead, sizeof(bool), 0);
		recv(st->s, (char*)&click, sizeof(bool), 0);
		recv(st->s, (char*)&health, sizeof(float), 0);

		mtx.lock();
		data.players[st->role].setDir(dir);
		data.players[st->role].setAim(Vector2f(x, y));
		data.players[st->role].setRotation(rot);
		data.players[st->role].setClick(click);
		data.players[st->role].setNickname("Player");
		mtx.unlock();

		loaded = 1;
	}
	return 0;
}
DWORD WINAPI host(void* args) {
	int result, soc = 0;
	float x, y;
	char n[16];

	SOCKET  s, so[3];
	WSADATA wsas;
	WORD wersja;
	wersja = MAKEWORD(1, 1);
	result = WSAStartup(wersja, &wsas);
	s = socket(AF_INET, SOCK_STREAM, 0);
	struct  sockaddr_in  sa, sc;
	memset((void*)(&sa), 0, sizeof(sa));
	sa.sin_family = AF_INET;
	sa.sin_port = htons(10101);
	sa.sin_addr.s_addr = htonl(INADDR_ANY);
	result = bind(s, (struct sockaddr FAR*) & sa, sizeof(sa));
	result = listen(s, 5);
	int  lenc = sizeof(sc);

	struct hostTaskData hd[3];

	for (soc = 0; soc < 3; soc++) {
		so[soc] = accept(s, (struct  sockaddr  FAR*) & sc, &lenc);

		send(so[soc], (char*)&soc, sizeof(int), 0);
		x = 800 + (soc + 1) * 100;
		y = 450;
		send(so[soc], (char*)&x, sizeof(float), 0);
		send(so[soc], (char*)&y, sizeof(float), 0);
		recv(so[soc], n, 16, 0);

		mtx.lock();
		data.players.push_back(Player(25.0f, Vector2f(x, y), n));
		mtx.unlock();
		loaded = 1;
		hd[soc].s = so[soc];
		hd[soc].role = soc + 1;
		CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)hostTask, &hd[soc], 0, NULL);
	}
	return 0;
}
void OpenMenu(RenderWindow& window) {
	bool isMenu = 1;
	int menu;
	char nickname[16] = "";
	float scale = 0.7;

	Texture host1tx, host2tx, cl1tx, cl2tx, menuBackground, frametx;
	host1tx.loadFromFile("Textures/HOST1.png");
	host2tx.loadFromFile("Textures/HOST2.png");
	cl1tx.loadFromFile("Textures/CLIENT1.png");
	cl2tx.loadFromFile("Textures/CLIENT2.png");
	menuBackground.loadFromFile("Textures/backgroundnew.jpg");
	frametx.loadFromFile("Textures/Frame.png");

	Sprite hostsp(host1tx), clsp(cl1tx), menuBg(menuBackground), framesp(frametx);

	Font font;
	font.loadFromFile("Fonts/PixellettersFull.ttf");
	
	SoundBuffer buffer;
	buffer.loadFromFile("Sounds/menu.ogg");
	Sound sound;
	sound.setBuffer(buffer);
	sound.setVolume(50);
	sound.setLoop(1);
	sound.play();

	framesp.setScale(scale, scale);
	hostsp.setScale(scale, scale);
	clsp.setScale(scale, scale);
	framesp.setPosition(window.getSize().x / 2 - frametx.getSize().x * scale / 2, window.getSize().y / 2 - frametx.getSize().y * scale / 2);
	hostsp.setPosition(framesp.getPosition().x + 30, framesp.getPosition().y + 450);
	clsp.setPosition(framesp.getPosition().x + 30, framesp.getPosition().y + 380);
	menuBg.setPosition(0, 0);

	Event event;

	std::string IP = "";

	Text ipt;
	ipt.setPosition(framesp.getPosition().x + 70, framesp.getPosition().y + 300);
	ipt.setFillColor(Color::Red);
	ipt.setCharacterSize(45);
	ipt.setString("Enter host IP:");
	ipt.setFont(font);

	Text ink;
	ink.setPosition(framesp.getPosition().x + 70, framesp.getPosition().y + 200);
	ink.setFillColor(Color::Red);
	ink.setCharacterSize(45);
	ink.setString("Nickname");
	ink.setFont(font);

	TextField tf = TextField();
	tf.setFont(font);
	tf.setPosition(Vector2f(ipt.getPosition().x + 250, ipt.getPosition().y + 12));
	//tf.open();
	
	TextField tfn = TextField();
	tfn.setFont(font);
	tfn.setPosition(Vector2f(ink.getPosition().x + 250, ink.getPosition().y + 12));
	//tfn.open();

	struct clientTaskData cd;

	while (isMenu)
	{
		menu = 0;

		hostsp.setTexture(host1tx);
		clsp.setTexture(cl1tx);
		if (IntRect(hostsp.getPosition().x, hostsp.getPosition().y, host1tx.getSize().x * scale, host1tx.getSize().y * scale).contains(Mouse::getPosition(window))) { hostsp.setTexture(host2tx); menu = 1; }
		if (IntRect(clsp.getPosition().x, clsp.getPosition().y, cl1tx.getSize().x * scale, cl1tx.getSize().y * scale).contains(Mouse::getPosition(window))) { clsp.setTexture(cl2tx); menu = 2; }

		if (Mouse::isButtonPressed(Mouse::Left))
		{
			tf.setActive(false);
			tfn.setActive(false);
			if (IntRect(ipt.getPosition().x, ipt.getPosition().y, 600, 50).contains(Mouse::getPosition(window))) tf.open();
			if (IntRect(ink.getPosition().x, ink.getPosition().y, 600, 50).contains(Mouse::getPosition(window))) tfn.open();
			
			if (menu == 1) {
				role = 0;

				std::string temp = tfn.getText();
				if (temp.size() == 0 || temp.size() == 1) temp = "Player";
				std::copy(temp.begin(), temp.end(), nickname);
				nickname[temp.end() - temp.begin()] = '\0';

				data.players.push_back(Player(25.0f, Vector2f(800, 450), nickname));

				CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)host, NULL, 0, NULL);

				window.setTitle("Shooter Host");
				sound.stop();
				isMenu = false;

			}
			else if (menu == 2) {
				std::string temp = tf.getText();

				char addr[16];
				std::copy(temp.begin(), temp.end(), cd.addr);
				cd.addr[temp.end() - temp.begin()] = '\0';

				temp = tfn.getText();
				std::copy(temp.begin(), temp.end(), cd.nickname);
				cd.nickname[temp.end() - temp.begin()] = '\0';

				CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)clientTask, &cd, 0, NULL);
			   
				window.setTitle("Shooter Client");
				sound.stop();
				isMenu = false;
			}
		}

		window.pollEvent(event);
		if (event.type == sf::Event::KeyPressed) {
			window.pollEvent(event);
			tf.input(event);
			tfn.input(event);
			if (event.type == Event::Closed)
				window.close();
		}
		
		window.draw(menuBg);
		window.draw(framesp);
		window.draw(hostsp);
		window.draw(clsp);
		window.draw(ipt);
		window.draw(ink);
		tf.render(window);
		tfn.render(window);

		window.display();
	}

}

int main() {
	srand(time(NULL));

	window.setFramerateLimit(60);
	Render render;
	render.LoadTextures();

	data = gameData();
	data.score = 0;

	//Bullets
	Bullet b1;

	//Enemy
	Enemy enemy;

	int b_cntr = 0, spawn = 50;
	int spawnCounter = 10;
	int pdir = 0, temp;
	double dtemp, dtemp2;

	//Vectors
	Vector2f playerCenter;
	Vector2f mousePosWindow;
	Vector2f aimDir;
	Vector2f aimDirNorm;

	std::string nn;

	OpenMenu(window);

	std::vector<SoundBuffer> buffer;

	for (int i = 0; i < 30; i++) {
		SoundBuffer t;
		buffer.push_back(t);
	}

	buffer[0].loadFromFile("Sounds/battle.ogg");
	buffer[1].loadFromFile("Sounds/hurt1.ogg");
	buffer[2].loadFromFile("Sounds/hurt2.ogg");
	buffer[3].loadFromFile("Sounds/hurt3.ogg");
	buffer[4].loadFromFile("Sounds/hurt4.ogg");
	buffer[5].loadFromFile("Sounds/hurt5.ogg");
	buffer[6].loadFromFile("Sounds/hurt6.ogg");
	buffer[7].loadFromFile("Sounds/step1.ogg");
	buffer[8].loadFromFile("Sounds/step2.ogg");
	buffer[9].loadFromFile("Sounds/step3.ogg");
	buffer[10].loadFromFile("Sounds/step4.ogg");
	buffer[11].loadFromFile("Sounds/shoot.ogg");
	buffer[12].loadFromFile("Sounds/death.ogg");
	buffer[13].loadFromFile("Sounds/z_death1.ogg");
	buffer[14].loadFromFile("Sounds/z_death2.ogg");
	buffer[15].loadFromFile("Sounds/z_death3.ogg");
	buffer[16].loadFromFile("Sounds/z_death4.ogg");

	bool play_dead = false;

	Sound shoot;
	shoot.setVolume(30);
	Sound death;
	death.setVolume(50);
	Sound hurt;
	hurt.setVolume(60);
	Sound step;
	step.setVolume(30);
	Sound z_death;
	z_death.setVolume(40);

	int step_cntr = 0;
	float player_speed = 2.0f;
	double zombie_speed = 1.0;
	bool is_pause = false;

	if (role){
		SoundBuffer buff;
		buff.loadFromFile("Sounds/wait.ogg");
		Sound wait;
		wait.setBuffer(buff);
		wait.setVolume(50);
		wait.setLoop(1);
		wait.play();
		while (role > 4 || data.players.size() < 2) {
			render.UIWait();
		}
		wait.stop();
	}

	std::vector<Enemy> enemies = data.enemies;
	std::vector<Player> players = data.players;
	std::vector<std::vector<Bullet>> bullets;
	for (int i = 0; i < 4; i++)
		bullets.push_back(std::vector<Bullet>());
	std::vector<Particle> particles;

	Clock clock;
	float lastTime = 0;
	float fps = 60;

	int score = 0;
	int framecounter = 0;

	Sound battle_theme;
	battle_theme.setBuffer(buffer[0]);
	battle_theme.setLoop(1);
	battle_theme.setVolume(50);
	battle_theme.play();

	Clock mClock;
	int mFrame = 0;

	while (window.isOpen()) {
		if (loaded) {
			mtx.lock();
			enemies = data.enemies;
			players = data.players;
			score = data.score;
			mtx.unlock();
			loaded = 0;
		}

		Event event;
		while (window.pollEvent(event))
		{
			if (event.type == Event::Closed)
				window.close();
		}

		if (players[role].isDead() && !play_dead) {
			play_dead = true;
			death.setBuffer(buffer[12]);
			death.play();
		}

		//Update
		//Vectors
		playerCenter = players[role].getPosition();
		mousePosWindow = Vector2f(Mouse::getPosition(window));
		aimDir = mousePosWindow - playerCenter;
		aimDirNorm = aimDir / sqrt(pow(aimDir.x, 2) + pow(aimDir.y, 2));
		players[role].setAim(aimDirNorm);

		if (!players[role].isDead()) {
			float PI = 3.14159265f;
			float deg = atan2(aimDirNorm.y, aimDirNorm.x) * 180 / PI;
			players[role].setRotation(deg + 90);
		}

		pdir = 0;
		if (!players[role].isDead()) {

			//Player
			if (Keyboard::isKeyPressed(Keyboard::A)) {
				if (step_cntr % 20 == 0) {
					step.setBuffer(buffer[7 + (rand() % 3)]);
					step.play();
				}
				pdir += 1;
			}

			if (Keyboard::isKeyPressed(Keyboard::D)) {
				if (step_cntr % 20 == 0) {
					step.setBuffer(buffer[7 + (rand() % 3)]);
					step.play();
				}
				pdir += 2;
			}

			if (Keyboard::isKeyPressed(Keyboard::W)) {
				if (step_cntr % 20 == 0) {
					step.setBuffer(buffer[7 + (rand() % 3)]);
					step.play();
				}
				pdir += 4;
			}

			if (Keyboard::isKeyPressed(Keyboard::S)) {
				if (step_cntr % 20 == 0) {
					step.setBuffer(buffer[7 + (rand() % 3)]);
					step.play();
				}
				pdir += 8;
			}
			
			if (step_cntr % 20 == 0) step_cntr = 0;
			step_cntr++;

		}
		players[role].setDir(pdir);

		if (Mouse::isButtonPressed(Mouse::Left) && !players[role].isDead()) {
			players[role].setClick(true);
		}
		else
			players[role].setClick(false);

		for (int i = 0; i < players.size(); i++) {
			if (players[i].getClick() && b_cntr % 10 == 0) {
				b1.shape.setPosition(players[i].getPosition());
				aimDir = mousePosWindow - players[i].getPosition();
				aimDirNorm = aimDir / sqrt(pow(aimDir.x, 2) + pow(aimDir.y, 2));
				b1.currVelocity = players[i].getAim() * b1.maxSpeed;
				b1.shape.setRotation(players[i].getRotation());
				bullets[i].push_back(b1);
				shoot.setBuffer(buffer[11]);
				shoot.play();

				b_cntr = 0;

				Particle smoke(&render.GetTexture("smoke"), Color(90, 90, 90, 255), 80, 30, 1);
				smoke.setPosition(players[i].getPosition() - Vector2f(12.5, 12.5));
				particles.push_back(smoke);

				Particle light(&render.GetTexture("light"), Color(255, 255, 0, 0), 5, 30, 2);
				light.setPosition(players[i].getPosition() - Vector2f(12.5,12.5));
				particles.push_back(light);
			}

			temp = players[i].getDir();
			switch (temp) {
			case 1:
				if(players[i].getPosition().x > 13)
					players[i].move(-player_speed, 0.f);
				break;
			case 2:
				if (players[i].getPosition().x < window.getSize().x - 13)
					players[i].move(player_speed, 0.f);
				break;
			case 4:
				if (players[i].getPosition().y > 13)
					players[i].move(0.f, -player_speed);
				break;
			case 5:
				if (players[i].getPosition().x > 13 && players[i].getPosition().y > 13)
					players[i].move(-player_speed / 1.414, -player_speed / 1.414);
				else if(players[i].getPosition().x > 13)
					players[i].move(-player_speed, 0.f);
				else if(players[i].getPosition().y > 13)
					players[i].move(0.f, -player_speed);
				break;
			case 6:
				if (players[i].getPosition().x < window.getSize().x - 13 && players[i].getPosition().y > 13)
					players[i].move(player_speed / 1.414, -player_speed / 1.414);
				else if(players[i].getPosition().x < window.getSize().x - 13)
					players[i].move(player_speed, 0.f);
				else if(players[i].getPosition().y > 13)
					players[i].move(0.f, -player_speed);
				break;
			case 8:
				if (players[i].getPosition().y < window.getSize().y - 13)
					players[i].move(0.f, player_speed);
				break;
			case 9:
				if (players[i].getPosition().x > 13 && players[i].getPosition().y < window.getSize().y - 13)
					players[i].move(-player_speed / 1.414, player_speed / 1.414);
				else if (players[i].getPosition().x > 13)
					players[i].move(-player_speed, 0.f);
				else if (players[i].getPosition().y < window.getSize().y - 13)
					players[i].move(0.f, player_speed);
				break;
			case 10:
				if (players[i].getPosition().x < window.getSize().x - 13 && players[i].getPosition().y < window.getSize().y - 13)
					players[i].move(player_speed / 1.414, player_speed / 1.414);
				else if(players[i].getPosition().x < window.getSize().x - 13)
					players[i].move(player_speed, 0.f);
				else if(players[i].getPosition().y < window.getSize().y - 13)
					players[i].move(0.f, player_speed);
				break;
			}
		}
		//Enemies
		if (role == 0) {
			int p = rand()%1000;

			if (p < spawn) {
				int i = rand() % 4;
				p = rand();
				switch (i) {
				case 0:
					enemy.setPosition(Vector2f(rand() % (window.getSize().x + 100), -50));
					break;
				case 1:
					enemy.setPosition(Vector2f(rand() % (window.getSize().x + 100), window.getSize().y + 50));
					break;
				case 2:
					enemy.setPosition(Vector2f(-50, rand() % (window.getSize().y + 100)));
					break;
				case 3:
					enemy.setPosition(Vector2f(window.getSize().x + 50, rand() % (window.getSize().y + 100)));
					break;
				}

				enemies.push_back(enemy);
			}

			spawnCounter = 0;
			if (framecounter % 30 == 0) {
				spawn++;
				std::cout << spawn << std::endl;
			}
				
		}
		b_cntr++;
		//Shooting

		

		for (size_t k = 0; k < enemies.size(); k++) {

			Vector2f epos;
			dtemp = 100000000;
			temp = 0;
			for (int i = 0; i < players.size(); i++) {
				if (!players[i].isDead()) {
					dtemp2 = pow(players[i].getPosition().x - enemies[k].getPosition().x, 2) + pow(players[i].getPosition().y - enemies[k].getPosition().y, 2);
					if (dtemp2 < dtemp) {
						temp = i;
						dtemp = dtemp2;
					}
				}
			}
			dtemp = sqrt(dtemp);
			epos.x = zombie_speed / dtemp * (players[temp].getPosition().x - enemies[k].getPosition().x);
			epos.y = zombie_speed / dtemp * (players[temp].getPosition().y - enemies[k].getPosition().y);

			enemies.at(k).setPosition(enemies[k].getPosition() + epos);

			Vector2f zaimDir = (enemies[k].getPosition() + epos - players[temp].getPosition());
			Vector2f zaimDirNorm = zaimDir / sqrt(pow(zaimDir.x, 2) + pow(zaimDir.y, 2));

			float PI = 3.14159265f;
			float deg = atan2(zaimDirNorm.y, zaimDirNorm.x) * 180 / PI;
			enemies.at(k).setRotation(deg + 90);


			for (int i = 0; i < players.size(); i++) {
				if (players[i].getVisual()->getGlobalBounds().intersects(enemies[k].getGlobalBounds()) && b_cntr % 10 == 0 && !players[i].isDead()) {
					if (i == role) {
						hurt.setBuffer(buffer[1 + (rand() % 6)]);
						hurt.play();
					}
					players[i].setHealth(players[i].getHealth() - 1);
				}
			}
		}
		for (int j = 0; j < bullets.size(); j++) {
			size_t bsize = bullets[j].size();
			for (size_t i = 0; i < bsize; i++)
			{
				bullets[j][i].shape.move(bullets[j][i].currVelocity);

				//Out of bounds
				if (bullets[j][i].shape.getPosition().x < 0 || bullets[j][i].shape.getPosition().x > window.getSize().x
					|| bullets[j][i].shape.getPosition().y < 0 || bullets[j][i].shape.getPosition().y > window.getSize().y)
				{
					bullets[j].erase(bullets[j].begin() + i);

				}
				else
				{
					//Enemy collision
					for (size_t k = 0; k < enemies.size(); k++)
					{
						if (bullets[j][i].shape.getGlobalBounds().intersects(enemies[k].getGlobalBounds()))
						{
							Particle blood(&render.GetTexture("blood"), Color::Red, 250, 50, 0);
							blood.setPosition(enemies.at(k).getPosition());
							blood.setRotation(enemies.at(k).getRotation());
							particles.push_back(blood);

							bullets[j].erase(bullets[j].begin() + i);
							enemies.erase(enemies.begin() + k);
							z_death.setBuffer(buffer[13 + (rand() % 3)]);
							z_death.play();
							//players[i].addScore();
							if (role == 0) {
								score++;
							}

							break;
						}
					}
				}
				bsize = bullets[j].size();
			}
		}

		for (size_t i = 0; i < particles.size(); i++) {
			if (!particles.at(i).Update()) particles.erase(particles.begin() + i);
		}

		if (ready) {
			mtx.lock();
			if (role != 0 ) {
				cldata.player = players[role];
			}
			else {
				data.enemies = enemies;
				data.players = players;
				data.score = score;
			}
			mtx.unlock();
			ready = 0;
		}
		else if (!role) {
			data.enemies = enemies;
			for (int i=0; i<players.size(); i++)
				data.players[i] = players[i];
			data.score = score;
		}

		//Draw
		render.RenderStatic();

		for (size_t i = 0; i < particles.size(); i++) {
			if (particles[i].getType() != 1) window.draw(particles[i].getVisual());
			else if (particles[i].getType() == 1 || particles[i].getType() == 2) window.draw(particles[i].getVisual(), particles[i].getTransf());
		}

		for (size_t i = 0; i < enemies.size(); i++) {
			enemies[i].setTexture(&render.GetTexture("zombie"));
			window.draw(enemies[i]);
		}

		for (int i = 0; i < players.size(); i++) {
			players[i].Update();
			Text nick;
			Font font = render.GetFont();
			Vector2f ppos = players[i].getPosition();
			ppos.y = ppos.y + 30;
			nick.setPosition(ppos);
			nick.setFillColor(Color::White);
			nick.setCharacterSize(24);
			nick.setString(players[i].getNickname());
			nick.setFont(font);

			Text health;
			ppos.x = ppos.x - 45;
			health.setPosition(ppos);
			health.setFillColor(Color::Red);
			health.setCharacterSize(24);

			std::string h = "";
			int n = (int)players[i].getHealth() / 10;

			for (int j = 0; j < n; j++) h.append("|");

			if (players[i].getHealth() < 1) h = "DEAD";

			health.setString(h);
			health.setFont(font);



			//Text tping;
			//tping.setPosition(Vector2f(10, 50));
			//tping.setFillColor(Color::Green);
			//tping.setCharacterSize(32);
			//tping.setString("Ping: " + std::to_string((int)ping));
			//tping.setFont(font);

			//window.draw(tping);
			window.draw(nick);
			window.draw(health);
			players[i].getVisual()->setTexture(&render.GetTexture("player"));
			window.draw(*players[i].getVisual());
			
		}

		{
			Font font = render.GetFont();

			Text tscore;
			tscore.setPosition(Vector2f(10, 10));
			tscore.setFillColor(Color::Green);
			tscore.setCharacterSize(32);
			tscore.setString("Score: " + std::to_string(score));
			tscore.setFont(font);

			Text fpst;
			fpst.setPosition(Vector2f(10, 30));
			fpst.setFillColor(Color::Green);
			fpst.setCharacterSize(32);
			if (mClock.getElapsedTime().asSeconds() >= 1.f)
			{
				fps = mFrame;
				mFrame = 0;
				mClock.restart();
			}

			++mFrame;
			fpst.setString(" ");
			fpst.setString("FPS: " + std::to_string((int)fps));
			fpst.setFont(font);

			window.draw(fpst);
			window.draw(tscore);
		}

		for (size_t i = 0; i < bullets.size(); i++)
			for (size_t j = 0; j < bullets[i].size(); j++)
				window.draw(bullets[i][j].shape);

		window.display();
		framecounter++;
	}

	return 0;
}