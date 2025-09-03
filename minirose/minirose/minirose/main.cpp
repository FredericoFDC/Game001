// MiniROSE - Parte 1 (C++98)
// Objetivo: esqueleto de MMORPG estilo Rose no console (C++98), sem recursos modernos.
// Recursos: Player, Monster, combate, EXP/Level, posições simples, drop simulada.
// Compilação (GCC/MinGW): g++ -std=c++98 -O2 -Wall -Wextra -o minirose minirose.cpp
// Compilação (MSVC): cl /EHsc /W4 minirose.cpp
#define _CRT_SECURE_NO_WARNINGS

#include <iostream>
#include <string>
#include <vector>
#include <ctime>
#include <cstdlib>
#include <cstdio>
#include <cstring>

// --- Utilidades ------------------------------------------------------------
struct Vec3 {
    float x; float y; float z;
    Vec3() : x(0), y(0), z(0) {}
    Vec3(float _x, float _y, float _z) : x(_x), y(_y), z(_z) {}
};

static int irand(int a, int b) { // inteiro aleatório [a,b]
    if (b < a) { int t = a; a = b; b = t; }
    return a + (std::rand() % (b - a + 1));
}

// --- Entidades base --------------------------------------------------------
class Entity {
protected:
    char name[32];
    int level;
    int hp, hpMax;
    int attackMin, attackMax;
    Vec3 pos;
public:
    Entity() : level(1), hp(1), hpMax(1), attackMin(1), attackMax(1), pos() {
        std::strcpy(name, "Entity");
    }
    virtual ~Entity() {}

    const char* getName() const { return name; }
    void setName(const char* n) {
        std::strncpy(name, n, sizeof(name));
        name[sizeof(name) - 1] = '\0';
    }

    int getLevel() const { return level; }
    int getHP() const { return hp; }
    int getHPMax() const { return hpMax; }
    bool isAlive() const { return hp > 0; }

    virtual int rollDamage() const {
        if (attackMax < attackMin) return attackMin;
        return irand(attackMin, attackMax);
    }

    virtual void receiveDamage(int dmg) {
        if (dmg < 0) dmg = 0;
        hp -= dmg;
        if (hp < 0) hp = 0;
    }

    virtual void healToFull() { hp = hpMax; }
    Vec3 getPos() const { return pos; }
    void setPos(const Vec3& p) { pos = p; }
};

// --- Player ----------------------------------------------------------------
class Player : public Entity {
    int exp;
    int statSTR;
    int zuly; // moeda
public:
    Player() : Entity(), exp(0), statSTR(10), zuly(0) {
        setName("Novato");
        level = 1; hpMax = 120; hp = hpMax; attackMin = 10; attackMax = 18;
        pos = Vec3(5200.f, 5200.f, 0.f);
    }

    int getExp() const { return exp; }
    int getZuly() const { return zuly; }

    void addZuly(int v) { if (v > 0) zuly += v; }

    // Fórmula simples de exp para upar: expNec = 100 + (level-1)*50
    int expToNext() const { return 100 + (level - 1) * 50; }

    void addExp(int amount) {
        if (amount <= 0) return;
        exp += amount;
        while (exp >= expToNext()) {
            exp -= expToNext();
            levelUp();
        }
    }

    void levelUp() {
        level += 1;
        // buffs básicos ao upar
        statSTR += 2;
        hpMax += 20; hp = hpMax;
        attackMin += 2; attackMax += 3;
        std::cout << "\n>> Parabéns! Você atingiu o nível " << level << "! HP curado e atributos aumentados.\n";
    }

    int getSTR() const { return statSTR; }

    virtual int rollDamage() const {
        int base = Entity::rollDamage();
        // bônus simples de STR
        base += (statSTR / 5);
        return base;
    }
};

// --- Monster ---------------------------------------------------------------
class Monster : public Entity {
    int expReward;
    int zulyMin, zulyMax;
public:
    Monster(const char* n, int lvl, int hpBase, int atkMin, int atkMax, int expRw, int zMin, int zMax) : Entity() {
        setName(n);
        level = lvl;
        hpMax = hpBase; hp = hpMax;
        attackMin = atkMin; attackMax = atkMax;
        expReward = expRw;
        zulyMin = zMin; zulyMax = zMax;
    }

    int getExpReward() const { return expReward; }
    int rollZuly() const { return irand(zulyMin, zulyMax); }
};

// --- Mundo simples ---------------------------------------------------------
class World {
    Player* player;
    std::vector<Monster*> monsters; // vida curta; geridos manualmente
public:
    World(Player* p) : player(p) {}
    ~World() { clearMonsters(); }

    void clearMonsters() {
        for (size_t i = 0; i < monsters.size(); ++i) delete monsters[i];
        monsters.clear();
    }

    void spawnSlime() {
        Monster* m = new Monster("Jellybean", player->getLevel(), 60 + player->getLevel() * 5, 6, 11, 30, 5, 20);
        monsters.push_back(m);
        std::cout << "Um Jellybean apareceu! (HP " << m->getHP() << ")\n";
    }

    void spawnGolem() {
        Monster* m = new Monster("Stone Golem", player->getLevel() + 2, 150 + player->getLevel() * 10, 12, 20, 80, 20, 60);
        monsters.push_back(m);
        std::cout << "Um Stone Golem apareceu! (HP " << m->getHP() << ")\n";
    }

    bool hasMonsters() const { return !monsters.empty(); }

    Monster* frontMonster() {
        if (monsters.empty()) return 0;
        return monsters[0];
    }

    void removeFrontIfDead() {
        if (!monsters.empty() && !monsters[0]->isAlive()) {
            delete monsters[0];
            monsters.erase(monsters.begin());
        }
    }

    void showStatus() const {
        std::cout << "\n=== STATUS DO PLAYER ===\n";
        std::cout << "Nome: " << player->getName() << "  Lv: " << player->getLevel()
            << "  HP: " << player->getHP() << "/" << player->getHPMax()
            << "  STR: " << player->getSTR()
            << "  EXP: " << player->getExp() << "/" << player->expToNext()
            << "  Zuly: " << player->getZuly()
            << "\n";
        if (monsters.empty()) {
            std::cout << "Sem monstros no mapa. Use 'spawn' para chamar um." << "\n";
        }
        else {
            std::cout << "Monstros no mapa: " << monsters.size() << " (atacará o da frente).\n";
        }
    }

    // Combate simples estilo turno
    void playerAttack() {
        Monster* m = frontMonster();
        if (!m) { std::cout << "Não há monstros para atacar.\n"; return; }
        int dmg = player->rollDamage();
        m->receiveDamage(dmg);
        std::cout << "Você causou " << dmg << " de dano ao " << m->getName() << ". (HP restante: " << m->getHP() << ")\n";
        if (!m->isAlive()) {
            int expGain = m->getExpReward();
            int cash = m->rollZuly();
            std::cout << "\n>> Você derrotou " << m->getName() << "! Ganhou " << expGain << " EXP e " << cash << " zuly.\n";
            player->addExp(expGain);
            player->addZuly(cash);
            removeFrontIfDead();
        }
    }

    void monsterTurn() {
        Monster* m = frontMonster();
        if (!m) return;
        if (!m->isAlive()) return;
        int dmg = m->rollDamage();
        player->receiveDamage(dmg);
        std::cout << m->getName() << " te atacou causando " << dmg << " de dano! Seu HP: " << player->getHP() << "\n";
    }
};

// --- Interface de texto ----------------------------------------------------
static void printHelp() {
    std::cout << "\nComandos:\n"
        << "  help          - mostra este menu\n"
        << "  status        - mostra status do player\n"
        << "  spawn slime   - invoca um Jellybean\n"
        << "  spawn golem   - invoca um Stone Golem\n"
        << "  atk           - ataca o monstro da frente\n"
        << "  heal          - cura HP do player (descanso)\n"
        << "  quit          - sair\n";
}

int main() {
    std::srand((unsigned int)std::time(0));

    Player player;
    player.setName("Aventureiro");
    World world(&player);

    std::cout << "Bem-vindo ao MiniROSE (console, C++98)!\n";
    printHelp();

    std::string cmd;
    while (true) {
        if (!player.isAlive()) {
            std::cout << "\n>> Você foi derrotado. Cura completa aplicada e -10 zuly (taxa do médico).\n";
            player.healToFull();
            player.addZuly(-10); // pode ficar negativo
        }

        std::cout << "\n> ";
        if (!std::getline(std::cin, cmd)) break;

        if (cmd == "help") {
            printHelp();
        }
        else if (cmd == "status") {
            world.showStatus();
        }
        else if (cmd == "spawn slime") {
            world.spawnSlime();
        }
        else if (cmd == "spawn golem") {
            world.spawnGolem();
        }
        else if (cmd == "atk") {
            world.playerAttack();
            world.monsterTurn();
        }
        else if (cmd == "heal") {
            player.healToFull();
            std::cout << "HP restaurado!\n";
        }
        else if (cmd == "quit") {
            std::cout << "Saindo...\n";
            break;
        }
        else if (cmd.size() == 0) {
            continue;
        }
        else {
            std::cout << "Comando desconhecido. Digite 'help'.\n";
        }
    }

    return 0;
}
