#include <iostream>
#include <algorithm> 

using namespace std;

int main() {
	
	float litros, km, abastecimento, media, probabilidade, diferenca, probabilidadetwo;
	string resposta;

	cout << "Bem vindo" << endl;
	cout << "Vamos calcular a media de combustivel do seu carro." << endl;
	
	while (true) {

		cout << "Quantos litros vai no tanque do seu carro? " << endl;
		cin >> litros;
		cout << "Depois de abastecer voce andou quantos km? " << endl;
		cin >> km;
		cout << "Quando voce abasteceu novamente quantos litros foi no tanque? " << endl;
		cin >> abastecimento;

		media = km / abastecimento;
		probabilidade = litros - abastecimento;
		diferenca = km * litros;
		probabilidadetwo = diferenca / abastecimento;

		cout << "Sua media atua e de " << media << " km por litros." << endl;
		cout << "Com o tanque cheio voce pode chegar a rodar " << probabilidadetwo << " km por litros." << endl;

		
		cout << "Deseja calcular uma nova media? " << endl;
		cin >> resposta;

		// converte para minúsculas
		std::transform(resposta.begin(), resposta.end(), resposta.begin(),
			[](unsigned char c) { return std::tolower(c); });

		if (resposta == "nao") {
			break;
		}
	}
	return 0;
}