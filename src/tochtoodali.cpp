
#define _CRT_SECURE_NO_WARNINGS
#define _WIN32_WINNT 0x0500
#define W_WIDTH 1600
#define W_HEIGHT 900

#include	"stdafx.h"

#include	<vector>
#include	<Windows.h>
#include	<winsock.h>
#include	<mutex>

#pragma comment(lib, "WSOCK32.lib")
RenderWindow window(VideoMode(W_WIDTH, W_HEIGHT), "Shooter: Main Menu", Style::Default);

//
#include	"Render.h"
#include	"Player.h"
#include	"Bullet.h"
#include	"Enemy.h"
#include	"Particle.h"
#include	"TextField.h"
//

std::mutex mtx;
int role = 5;
bool ready = 1;
int ping = 0;

struct gamedata {
	std::vector<std::vector<Bullet>> bullets;
	std::vector<Enemy> enemies;
	std::vector<Player> players;
	gamedata() {
		for (int i = 0; i < 4; i++)
			bullets.push_back(std::vector<Bullet>());
	}
	int score;
} data;

struct clientdata {
	std::vector<std::vector<Bullet>> bullets;
	Player player;
	clientdata() {
		for (int i = 0; i < 4; i++)
			bullets.push_back(std::vector<Bullet>());
	}
} cldata;

struct sendtaskdata {
	SOCKET s;
	int role;
};
struct clienttaskdata {
	char addr[16];
	char nickname[16];
};
DWORD WINAPI recvtask(void* args) {
	struct sendtaskdata* st = (struct sendtaskdata*) args;
	int size, size2, dir;
	float rot, x, y, health;
	char n[16];
	bool is_dead = false, click;
	Bullet bullet;
	DWORD timeout = 3000;
	setsockopt(st->s, SOL_SOCKET, SO_RCVTIMEO, (char*)&timeout, sizeof(timeout));
	Player player(25.0f, Vector2f(0, 0), "Player");
	std::vector<Bullet> tbullets;
	Clock mClock;
	while (1) {

		recv(st->s, (char*)&dir, sizeof(int), 0);
		recv(st->s, (char*)&rot, sizeof(float), 0);
		recv(st->s, (char*)&x, sizeof(float), 0);
		recv(st->s, (char*)&y, sizeof(float), 0);
		recv(st->s, (char*)&is_dead, sizeof(bool), 0);
		recv(st->s, (char*)&click, sizeof(bool), 0);
		recv(st->s, (char*)&health, sizeof(float), 0);
		mtx.lock();
		data.players[st->role].setDir(dir);
		data.players[st->role].setAim(Vector2f(x,y));
		data.players[st->role].setRotation(rot);
		data.players[st->role].setClick(click);
		mtx.unlock();

		ready = 1;
		Sleep(30);
		ping = mClock.getElapsedTime().asMilliseconds();
		mClock.restart();
	}
	return 0;
}

DWORD WINAPI client(void* args) {
	SOCKET sckt;
	struct sockaddr_in sa;
	WSADATA wsas;
	WORD wersja;
	wersja = MAKEWORD(2, 0);
	int result, size, size2, score;
	float rot, x, y;
	char n[16];
	Bullet bullet;
	Enemy enemy;
	Player player(25.0f, Vector2f(0, 0), "P");
	struct clienttaskdata* cd = (struct clienttaskdata*)args;
	for (int i = 0; i < strlen(cd->nickname); i++)
		n[i] = cd->nickname[i];
	n[strlen(cd->nickname)] = '\0';
	WSAStartup(wersja, &wsas);
	sckt = socket(AF_INET, SOCK_STREAM, 0);
	DWORD timeout = 3000;
	setsockopt(sckt, SOL_SOCKET, SO_RCVTIMEO, (char*)&timeout, sizeof(timeout));
	memset((void*)(&sa), 0, sizeof(sa));
	sa.sin_family = AF_INET;
	sa.sin_port = htons(10101);
	sa.sin_addr.s_addr = inet_addr("25.56.163.165"); //cd->addr
	while (connect(sckt, (struct sockaddr FAR*) & sa, sizeof(sa)) == SOCKET_ERROR);
	recv(sckt, (char*)&result, sizeof(int), 0);
	role = result + 1;
	send(sckt, n, 16, 0);
	recv(sckt, (char*)&x, sizeof(float), 0);
	recv(sckt, (char*)&y, sizeof(float), 0);
	player.setPosition(Vector2f(x, y));
	player.setNickname(cd->nickname);
	cldata.player = player;
	std::vector<Enemy> tenemies;
	std::vector<Player> tplayers;
	std::vector<std::vector<Bullet>> tbullets;
	std::vector<Bullet> sb;
	std::string nn;
	bool is_dead = false, click;
	float health;
	for (int i = 0; i < 4; i++) {
		std::vector<Bullet> b;
		tbullets.push_back(b);
	}
	Clock mClock;
	while (1) {
		recv(sckt, (char*)&score, sizeof(int), 0);
		recv(sckt, (char*)&size, sizeof(int), 0);
		tenemies.clear();
		for (int i = 0; i < size; i++) {
			recv(sckt, (char*)&x, sizeof(float), 0);
			recv(sckt, (char*)&y, sizeof(float), 0);
			enemy.setPosition(sf::Vector2f(x, y));
			tenemies.push_back(enemy);
		}
		mtx.lock();
		data.enemies = tenemies;
		data.score = score;
		mtx.unlock();

		tplayers.clear();
		recv(sckt, (char*)&size, sizeof(int), 0);
		for (int i = 0; i < size; i++) {
			recv(sckt, (char*)&x, sizeof(float), 0);
			recv(sckt, (char*)&y, sizeof(float), 0);
			recv(sckt, (char*)&result, sizeof(int), 0);
			recv(sckt, (char*)&rot, sizeof(float), 0);
			recv(sckt, n, 16, 0);
			recv(sckt, (char*)&is_dead, sizeof(bool), 0);
			recv(sckt, (char*)&click, sizeof(bool), 0);
			recv(sckt, (char*)&health, sizeof(float), 0);
			player.setPosition(sf::Vector2f(x, y));
			player.setDir(result);
			player.setRotation(rot);
			player.setNickname(n);
			player.setClick(click);
			player.setDead(is_dead);
			player.setHealth(health);
			recv(sckt, (char*)&x, sizeof(float), 0);
			recv(sckt, (char*)&y, sizeof(float), 0);
			player.setAim(Vector2f(x,y));
			tplayers.push_back(player);
		}
		mtx.lock();
		data.players = tplayers;
		mtx.unlock();


		ready = 1;
		mtx.lock();
		result = cldata.player.getDir();
		rot = cldata.player.getRotation();
		is_dead = cldata.player.isDead();
		click = cldata.player.getClick();
		health = cldata.player.getHealth();
		x = cldata.player.getAim().x;
		y = cldata.player.getAim().y;
		mtx.unlock();
		send(sckt, (char*)&result, sizeof(int), 0);
		send(sckt, (char*)&rot, sizeof(float), 0);
		send(sckt, (char*)&x, sizeof(float), 0);
		send(sckt, (char*)&y, sizeof(float), 0);
		send(sckt, (char*)&is_dead, sizeof(bool), 0);
		send(sckt, (char*)&click, sizeof(bool), 0);
		send(sckt, (char*)&health, sizeof(float), 0);
		
		ping = mClock.getElapsedTime().asMilliseconds();
		mClock.restart();
		Sleep(30);
	}
	return 0;
}
DWORD WINAPI sendtask(void* args) {
	std::vector<SOCKET>* so = (std::vector<SOCKET>*) args;
	float rot, x, y, health;
	bool is_dead, click;
	std::string nn;
	DWORD timeout = 3000;
	for (int s = 0; s < so->size(); s++) setsockopt((*so)[s], SOL_SOCKET, SO_RCVTIMEO, (char*)&timeout, sizeof(timeout));
	char n[16];
	int size, size2, dir, score;
	std::vector<Enemy> tenemies;
	std::vector<Player> tplayers;
	std::vector<std::vector<Bullet>> tbullets;
	while (1) {
		mtx.lock();
		tenemies = data.enemies;
		tplayers = data.players;
		score = data.score;
		mtx.unlock();

		for (int s = 0; s < so->size(); s++) {
		
			send((*so)[s], (char*)&score, sizeof(float), 0);
			
			size = tenemies.size();
			send((*so)[s], (char*)&size, sizeof(int), 0);
			for (int i = 0; i < size; i++) {
				x = tenemies[i].getPosition().x;
				y = tenemies[i].getPosition().y;
				send((*so)[s], (char*)&x, sizeof(float), 0);
				send((*so)[s], (char*)&y, sizeof(float), 0);
			}
			
			size = tplayers.size();
			send((*so)[s], (char*)&size, sizeof(int), 0);
			for (int i = 0; i < size; i++) {

				x = tplayers[i].getPosition().x;
				y = tplayers[i].getPosition().y;
				dir = tplayers[i].getDir();
				rot = tplayers[i].getRotation();
				is_dead = tplayers[i].isDead();
				health = tplayers[i].getHealth();
				nn = tplayers[i].getNickname();
				click = tplayers[i].getClick();
				std::copy(nn.begin(), nn.end(), n);
				n[nn.end() - nn.begin()] = '\0';
				rot = tplayers[i].getRotation();
				send((*so)[s], (char*)&x, sizeof(float), 0);
				send((*so)[s], (char*)&y, sizeof(float), 0);
				send((*so)[s], (char*)&dir, sizeof(int), 0);
				send((*so)[s], (char*)&rot, sizeof(float), 0);
				send((*so)[s], n, 16, 0);
				send((*so)[s], (char*)&is_dead, sizeof(bool), 0);
				send((*so)[s], (char*)&click, sizeof(bool), 0);
				send((*so)[s], (char*)&health, sizeof(float), 0);
				x = tplayers[i].getAim().x;
				y = tplayers[i].getAim().y;
				send((*so)[s], (char*)&x, sizeof(float), 0);
				send((*so)[s], (char*)&y, sizeof(float), 0);
			}
		}

		Sleep(30);
	}
	return 0;
}
DWORD WINAPI host(void* args) {
	Texture texp;
	WSADATA wsas;
	int result, soc = 0;
	float x, y;
	char n[16];
	WORD wersja;
	wersja = MAKEWORD(1, 1);
	result = WSAStartup(wersja, &wsas);

	SOCKET s;
	std::vector<SOCKET> so;
	CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)sendtask, &so, 0, NULL);
	s = socket(AF_INET, SOCK_STREAM, 0);
	struct  sockaddr_in  sa, sc;
	memset((void*)(&sa), 0, sizeof(sa));
	sa.sin_family = AF_INET;
	sa.sin_port = htons(10101);
	sa.sin_addr.s_addr = htonl(INADDR_ANY);
	result = bind(s, (struct sockaddr FAR*) & sa, sizeof(sa));

	result = listen(s, 5);
	int  lenc = sizeof(sc);
	struct sendtaskdata st[3];
	while (soc < 3) {
		so.push_back(accept(s, (struct  sockaddr  FAR*) & sc, &lenc));
		send(so[soc], (char*)&soc, sizeof(int), 0);
		mtx.lock();
		recv(so[soc], n, 16, 0);
		data.players.push_back(Player(25.0f, Vector2f(800 + (soc + 1) * 100, 450), n));
		x = 800 + (soc + 1) * 100;
		y = 450;
		send(so[soc], (char*)&x, sizeof(float), 0);
		send(so[soc], (char*)&y, sizeof(float), 0);
		mtx.unlock();
		st[soc].s = so[soc];
		st[soc].role = soc + 1;
		CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)recvtask, &st[soc], 0, NULL);
		soc++;
	}
	return 0;
}
std::string OpenMenu(RenderWindow& window, Texture* texp) {

	Texture host1tx, host2tx, cl1tx, cl2tx, menuBackground, frametx;
	host1tx.loadFromFile("Textures/HOST1.png");
	host2tx.loadFromFile("Textures/HOST2.png");
	cl1tx.loadFromFile("Textures/CLIENT1.png");
	cl2tx.loadFromFile("Textures/CLIENT2.png");
	menuBackground.loadFromFile("Textures/backgroundnew.jpg");
	frametx.loadFromFile("Textures/Frame.png");
	Font font;
	font.loadFromFile("Fonts/PixellettersFull.ttf");
	Sprite hostsp(host1tx), clsp(cl1tx), menuBg(menuBackground), framesp(frametx);
	bool isMenu = 1;

	SoundBuffer buffer;
	buffer.loadFromFile("Sounds/menu.ogg");
	Sound sound;
	sound.setBuffer(buffer);
	sound.setVolume(50);
	sound.setLoop(1);
	sound.play();

	framesp.setScale(0.7, 0.7);
	hostsp.setScale(0.7, 0.7);
	clsp.setScale(0.7, 0.7);
	framesp.setPosition(window.getSize().x / 2 - frametx.getSize().x * 0.7 / 2, window.getSize().y / 2 - frametx.getSize().y * 0.7 / 2);
	hostsp.setPosition(framesp.getPosition().x + 30, framesp.getPosition().y + 450);
	clsp.setPosition(framesp.getPosition().x + 30, framesp.getPosition().y + 380);
	menuBg.setPosition(0, 0);

	Event event;
	std::string IP = "";
	char nickname[16] = "";
	Text ipt;
	Text ink;
	ipt.setPosition(framesp.getPosition().x + 70, framesp.getPosition().y + 300);
	ipt.setFillColor(Color::Red);
	ipt.setCharacterSize(45);
	ipt.setString("Enter host IP:");
	ipt.setFont(font);
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
	int menu;
	struct clienttaskdata cd;
	while (isMenu)
	{
		menu = 0;
		hostsp.setTexture(host1tx);
		clsp.setTexture(cl1tx);
		if (IntRect(hostsp.getPosition().x, hostsp.getPosition().y, host1tx.getSize().x * 0.7, host1tx.getSize().y * 0.7).contains(Mouse::getPosition(window))) { hostsp.setTexture(host2tx); menu = 1; }
		if (IntRect(clsp.getPosition().x, clsp.getPosition().y, cl1tx.getSize().x * 0.7, cl1tx.getSize().y * 0.7).contains(Mouse::getPosition(window))) { clsp.setTexture(cl2tx); menu = 2; }

		if (Mouse::isButtonPressed(Mouse::Left))
		{
			if (menu == 1) {
				role = 0;

				isMenu = false;
				std::string temp = tfn.getText();
				if (temp.size() == 0 || temp.size() == 1) temp = "Player";
				std::copy(temp.begin(), temp.end(), nickname);
				nickname[temp.end() - temp.begin()] = '\0';
				sound.stop();
				data.players.push_back(Player(25.0f, Vector2f(800, 450), nickname));
				CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)host, NULL, 0, NULL);
				window.setTitle("Shooter Host");
				return std::string(nickname);

			}
			else if (menu == 2) {
				std::string temp = tf.getText();

				char addr[16];
				std::copy(temp.begin(), temp.end(), cd.addr);
				cd.addr[temp.end() - temp.begin()] = '\0';

				temp = tfn.getText();
				if (temp.size() == 0 || temp.size() == 1) temp = "Player";
				std::copy(temp.begin(), temp.end(), cd.nickname);
				cd.nickname[temp.end() - temp.begin()] = '\0';

				CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)client, &cd, 0, NULL);
				isMenu = false;

				window.setTitle("Shooter Client");
				sound.stop();
				return std::string(nickname);
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

		tf.setActive(false);
		if (IntRect(ipt.getPosition().x, ipt.getPosition().y, 600, 50).contains(Mouse::getPosition(window))) tf.open();
		tfn.setActive(false);
		if (IntRect(ink.getPosition().x, ink.getPosition().y, 600, 50).contains(Mouse::getPosition(window))) tfn.open();

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

	data = gamedata();
	data.score = 0;
	//Bullets
	Bullet b1;

	//Enemy
	Enemy enemy;

	int b_cntr = 0;
	int spawnCounter = 10;
	int pdir = 0, temp;
	double dtemp, dtemp2;

	//Vectors
	Vector2f playerCenter;
	Vector2f mousePosWindow;
	Vector2f aimDir;
	Vector2f aimDirNorm;

	std::string nn;

	nn = OpenMenu(window, &render.GetTexture("player"));

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
	double zombie_speed = 0.5;
	bool is_pause = false;

	{
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

		if (role == 0) {
			mtx.lock();

			mtx.unlock();
		}
		wait.stop();
	}

	std::vector<Enemy> enemies;
	std::vector<Player> players;
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

	while (window.isOpen())
	{
	start:

		if (ready == 1) {
			mtx.lock();
			enemies = data.enemies;
			players = data.players;
			score = data.score;
			mtx.unlock();
		}
		if (players.size() == 0) {
			printf("\n \n ERROR: player size is 0 \n \n");
			goto start;
		}
		if (players.size() == 1) {
			printf("\n \n ERROR: player size is 1 \n \n");
			goto start;
		}

		if (Keyboard::isKeyPressed(Keyboard::P)) {
			if (!is_pause) is_pause = true;
		}

		if (is_pause) {
			Sleep(500);
			while (is_pause) {

				if (Keyboard::isKeyPressed(Keyboard::P)) {
					is_pause = false;
					Sleep(500);
				}
			}
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
			players[role].setDir(pdir);
			if (step_cntr % 20 == 0) step_cntr = 0;
			step_cntr++;

		}

		if (Mouse::isButtonPressed(Mouse::Left)  && !players[role].isDead()) {
			players[role].setClick(true);
		}
		else
			players[role].setClick(false);

		for (int i = 0; i < players.size(); i++) {
			if (players[i].getClick() && b_cntr % 10 == 0) {
				Vector2f tpos = Vector2f(players[i].getPosition().x * 1.03, players[i].getPosition().y * 1.03);
				b1.shape.setPosition(tpos);
				aimDir = mousePosWindow - players[i].getPosition();
				aimDirNorm = aimDir / sqrt(pow(aimDir.x, 2) + pow(aimDir.y, 2));
				b1.currVelocity = players[i].getAim()*b1.maxSpeed;
				b1.shape.setRotation(players[i].getRotation());
				bullets[i].push_back(b1);
				shoot.setBuffer(buffer[11]);
				shoot.play();

				b_cntr = 0;

				Particle smoke(&render.GetTexture("smoke"), Color(90, 90, 90, 255), 80, 30, 1);
				smoke.setPosition(tpos);
				smoke.setRotation(players[i].getRotation());
				particles.push_back(smoke);

				tpos = Vector2f(players[i].getPosition().x, players[i].getPosition().y);
				Particle light(&render.GetTexture("light"), Color(255, 255, 0, 0), 5, 30, 2);
				light.setPosition(tpos);
				light.setRotation(players[i].getRotation());
				particles.push_back(light);
			}

			temp = players[i].getDir();
			switch (temp) {
			case 1:
				players[i].move(-player_speed, 0.f);
				break;
			case 2:
				players[i].move(player_speed, 0.f);
				break;
			case 4:
				players[i].move(0.f, -player_speed);
				break;
			case 5:
				players[i].move(-player_speed / 1.414, -player_speed / 1.414);
				break;
			case 6:
				players[i].move(player_speed / 1.414, -player_speed / 1.414);
				break;
			case 8:
				players[i].move(0.f, player_speed);
				break;
			case 9:
				players[i].move(-player_speed / 1.414, player_speed / 1.414);
				break;
			case 10:
				players[i].move(player_speed / 1.414, player_speed / 1.414);
				break;
			}
		}
		//Enemies
		if (role == 0) {
			if (spawnCounter < 10)
				spawnCounter++;

			if (spawnCounter >= 10 && enemies.size() < 70)
			{
				enemy.setPosition(Vector2f(rand() % window.getSize().x, rand() % (int)((window.getSize().y - window.getSize().y * 0.9))));
				enemies.push_back(enemy);

				spawnCounter = 0;
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
								players[0].setScore(score);
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

		if (ready == 1) {
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
		else {
			if (!role) {
				data.enemies = enemies;
				data.players = players;
				data.score = score;
			}
		}

		//Draw
		render.RenderStatic();

		for (size_t i = 0; i < particles.size(); i++)
		{
			if (particles[i].getType() != 1) window.draw(particles[i].getVisual());
			else if (particles[i].getType() == 1 || particles[i].getType() == 2) window.draw(particles[i].getVisual(), particles[i].getTransf());
		}

		for (size_t i = 0; i < enemies.size(); i++)
		{
			enemies[i].setTexture(&render.GetTexture("zombie"));
			window.draw(enemies[i]);
		}

		for (int i = 0; i < players.size(); i++)
		{
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