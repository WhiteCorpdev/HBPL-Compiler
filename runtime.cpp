#include <cstdio>
#include <cstdlib>
#include <cstring>

// Las genera el compilador a partir del .hbpl
extern "C" void hbpl_main();
extern "C" void hbpl_finish();

static bool finishing = false;


// Console.Log("texto")
extern "C" void hbpl_console_log(const char* msg) {
    std::puts(msg);
}


// variable.reserv(): copia el texto al heap y devuelve el puntero nuevo
extern "C" char* hbpl_str_reserv(const char* s) {

    size_t n = std::strlen(s) + 1;

    char* p = static_cast<char*>(std::malloc(n));

    if (!p) {
        std::fputs("Err: sin memoria\n", stderr);
        std::exit(1);
    }

    std::memcpy(p, s, n);

    return p;
}


// variable.free(): libera la memoria reservada con reserv()
extern "C" void hbpl_free(void* p) {
    std::free(p);
}


// exit(codigo): ejecuta finish y luego termina
extern "C" void hbpl_exit(int code) {

    if (!finishing) {          // evita bucle si finish() llama a exit()
        finishing = true;
        hbpl_finish();
    }

    std::exit(code);
}


// Punto de entrada real del .exe
int main() {

    hbpl_main();
    hbpl_exit(0);              // fin normal: finish() y salir con 0
}
