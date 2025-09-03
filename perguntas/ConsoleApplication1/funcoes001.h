#pragma once
#include <iostream>
#include <string>
#include <algorithm>

using namespace std;

bool fazerPergunta(string enunciado, string respostaCerta, int& vida, int& level, int& pontos) { string resposta;

    while (true) {
        cout << enunciado << endl;
        cin >> resposta;

        // coloca em minúsculas para comparar
        transform(resposta.begin(), resposta.end(), resposta.begin(), ::tolower);

        if (resposta != respostaCerta) {
            cout << "Voce errou, tente novamente." << endl;
            vida -= 1;
            if (vida < 1) {
                cout << "GAME OVER" << endl;
                return false; // perdeu
            }
            cout << "Voce ainda tem " << vida << " tentativas." << endl;
        }
        else {
            cout << "Parabens, voce acertou!" << endl;
            level += 1;
            pontos += 100;
            return true; // acertou
        }
    }
}
