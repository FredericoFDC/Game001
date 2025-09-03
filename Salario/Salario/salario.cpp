#include <iostream>

using namespace std;

int main() {

	float salario, media;
	int diaMes = 21;
	int diaCompleto = 10;
	int ValorHora;

	cout << "Seja Bem vindo ao ContSalario." << endl;
	cout << "Digite quanto seu salario bruto mensal: " << endl;
	cin >> salario;

	cout << "Legal!!!! Vamos calcular quanto voce ganha por dia." << endl;
	cout << "......................." << endl;

	media = salario / diaMes; //Valor do dia trabalhado.
	ValorHora = media / diaCompleto; // Valor da hora trabalhada.

	cout << "Voce ganha por dia R$ " << media << " reais." << endl;
	cout << "Voce ganha por hora R$ " << ValorHora << " reais." << endl;

	


	return 0;
}