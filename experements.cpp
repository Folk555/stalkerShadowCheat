#include <iostream>

// Примеры с разными соглашениями
int __cdecl add_cdecl(int a, int b) {
    return a + b;
}

int __stdcall add_stdcall(int a, int b) {
    return a + b;
}

int __fastcall add_fastcall(int a, int b) {
    return a + b;
}

// Стартовая функция для теста
void TestCallingConventions() {
    int a = 10, b = 20;
    int r1 = add_cdecl(a, b);
    int r2 = add_stdcall(a, b);
    int r3 = add_fastcall(a, b);

    std::cout << "cdecl: " << r1 << "\n";
    std::cout << "stdcall: " << r2 << "\n";
    std::cout << "fastcall: " << r3 << "\n";
}
