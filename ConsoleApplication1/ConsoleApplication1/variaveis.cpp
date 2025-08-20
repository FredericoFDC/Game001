#include <iostream>
#include <string>
#include <algorithm>
#include "funcoes001.h"


using namespace std;

int main() {
    int level = 0;
    int pontos = 100;
    int vida = 3;

    cout << "Bem vindo ao jogo!" << endl;

    // Perguntas
    if (!fazerPergunta("Quanto e 2 + 2 ?", "4", vida, level, pontos)) return 0;
    if (!fazerPergunta("Qual a capital do Brasil ?", "brasilia", vida, level, pontos)) return 0;
    if (!fazerPergunta("Quanto e 5 x 5 ?", "25", vida, level, pontos)) return 0;

    // Resultado final
    cout << "============================" << endl;
    cout << "Voce chegou no level: " << level << endl;
    cout << "Seus pontos foram: " << pontos << endl;
    cout << "============================" << endl;

    return 0;
}