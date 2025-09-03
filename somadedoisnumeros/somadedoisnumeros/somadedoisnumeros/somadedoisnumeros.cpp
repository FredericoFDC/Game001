// somadedoisnumeros.cpp : Este arquivo contém a função 'main'. A execução do programa começa e termina ali.
//

#include <iostream>

using namespace std;

int main()
{   
    int valor1, valor2, soma;

    cout << "Vamos criar uma soma entre dois numeros inteiros.\n";
    cout << "Digite o primeiro numero.\n";
    cin >> valor1;
 
    cout << "Digite o segundo numero.\n";
    cin >> valor2;

    soma = valor1 + valor2;

    cout << "A soma dos dois valores e igual a " << soma << ".\n";
    return 0;
}
