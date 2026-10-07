#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cstddef>
#include <cstdint>
#include <unordered_set>
#include <mutex>
#include <type_traits>


// ============================================================
// HBPL RUNTIME
// ============================================================


// ============================================================
// CONTROL DEL RUNTIME
// ============================================================

static bool hbpl_finishing = false;

static std::unordered_set<void*> hbpl_allocations;
static std::mutex hbpl_memory_mutex;


// ============================================================
// ERROR
// ============================================================

[[noreturn]]
static void hbpl_error(const char* msg) {

    std::fputs("HBPL Runtime Error: ", stderr);
    std::fputs(msg, stderr);
    std::fputc('\n', stderr);

    std::exit(1);
}


// ============================================================
// RESERVA GENERICA
// ============================================================
//
// Esta es la base de:
//
//     int
//     double
//     float
//     char
//     bool
//     etc.
//
// NO usar directamente para char* / str.
// ============================================================

template<typename T>
T* hbpl_reservar_impl(const T& value) {

    static_assert(
        !std::is_pointer_v<T>,
        "hbpl_reservar_impl no debe utilizarse con punteros"
    );

    T* ptr = static_cast<T*>(
        std::malloc(sizeof(T))
    );

    if (!ptr) {
        hbpl_error("sin memoria");
    }

    std::memcpy(
        ptr,
        &value,
        sizeof(T)
    );

    {
        std::lock_guard<std::mutex> lock(
            hbpl_memory_mutex
        );

        hbpl_allocations.insert(ptr);
    }

    return ptr;
}


// ============================================================
// RESERVA DE MEMORIA RAW
// ============================================================
//
// Permite a HBPL reservar:
//
//     reserv(address, size)
//
// Esto sirve para tipos futuros.
// ============================================================

extern "C"
void* hbpl_reserv_raw(
    const void* source,
    std::size_t size
) {

    if (size == 0) {
        hbpl_error("no se puede reservar 0 bytes");
    }

    void* ptr = std::malloc(size);

    if (!ptr) {
        hbpl_error("sin memoria");
    }

    if (source) {
        std::memcpy(
            ptr,
            source,
            size
        );
    }

    {
        std::lock_guard<std::mutex> lock(
            hbpl_memory_mutex
        );

        hbpl_allocations.insert(ptr);
    }

    return ptr;
}


// ============================================================
// FREE GENÉRICO
// ============================================================

extern "C"
void hbpl_free(void* ptr) {

    if (!ptr) {
        return;
    }

    {
        std::lock_guard<std::mutex> lock(
            hbpl_memory_mutex
        );

        auto it = hbpl_allocations.find(ptr);

        if (it == hbpl_allocations.end()) {

            hbpl_error(
                "free() sobre memoria no reservada "
                "o ya liberada"
            );
        }

        hbpl_allocations.erase(it);
    }

    std::free(ptr);
}


// ============================================================
// RESERVA GENÉRICA POR TIPO
// ============================================================

extern "C"
char* hbpl_reserv_char(char value) {

    return hbpl_reservar_impl(value);
}


extern "C"
short* hbpl_reserv_short(short value) {

    return hbpl_reservar_impl(value);
}


extern "C"
int* hbpl_reserv_int(int value) {

    return hbpl_reservar_impl(value);
}


extern "C"
long* hbpl_reserv_long(long value) {

    return hbpl_reservar_impl(value);
}


extern "C"
long long* hbpl_reserv_longlong(
    long long value
) {

    return hbpl_reservar_impl(value);
}


extern "C"
unsigned char* hbpl_reserv_uchar(
    unsigned char value
) {

    return hbpl_reservar_impl(value);
}


extern "C"
unsigned short* hbpl_reserv_ushort(
    unsigned short value
) {

    return hbpl_reservar_impl(value);
}


extern "C"
unsigned int* hbpl_reserv_uint(
    unsigned int value
) {

    return hbpl_reservar_impl(value);
}


extern "C"
unsigned long* hbpl_reserv_ulong(
    unsigned long value
) {

    return hbpl_reservar_impl(value);
}


extern "C"
unsigned long long* hbpl_reserv_ulonglong(
    unsigned long long value
) {

    return hbpl_reservar_impl(value);
}


extern "C"
float* hbpl_reserv_float(float value) {

    return hbpl_reservar_impl(value);
}


extern "C"
double* hbpl_reserv_double(double value) {

    return hbpl_reservar_impl(value);
}


extern "C"
bool* hbpl_reserv_bool(bool value) {

    return hbpl_reservar_impl(value);
}


// ============================================================
// STR
// ============================================================
//
// str necesita strlen() porque sizeof(char*) solamente
// devuelve el tamaño del puntero.
// ============================================================

extern "C"
char* hbpl_str_reserv(const char* str) {

    if (!str) {
        hbpl_error("se intentó reservar un str nulo");
    }

    std::size_t size =
        std::strlen(str) + 1;

    char* ptr =
        static_cast<char*>(
            std::malloc(size)
        );

    if (!ptr) {
        hbpl_error("sin memoria");
    }

    std::memcpy(
        ptr,
        str,
        size
    );

    {
        std::lock_guard<std::mutex> lock(
            hbpl_memory_mutex
        );

        hbpl_allocations.insert(ptr);
    }

    return ptr;
}


// ============================================================
// LONGITUD DE STR
// ============================================================

extern "C"
std::size_t hbpl_str_len(const char* str) {

    if (!str) {
        return 0;
    }

    return std::strlen(str);
}


// ============================================================
// COPIAR STR
// ============================================================

extern "C"
char* hbpl_str_copy(const char* str) {

    return hbpl_str_reserv(str);
}


// ============================================================
// CONCATENAR STR
// ============================================================

extern "C"
char* hbpl_str_concat(
    const char* a,
    const char* b
) {

    if (!a) a = "";
    if (!b) b = "";

    std::size_t lenA =
        std::strlen(a);

    std::size_t lenB =
        std::strlen(b);

    char* result =
        static_cast<char*>(
            std::malloc(
                lenA + lenB + 1
            )
        );

    if (!result) {
        hbpl_error("sin memoria");
    }

    std::memcpy(
        result,
        a,
        lenA
    );

    std::memcpy(
        result + lenA,
        b,
        lenB
    );

    result[lenA + lenB] = '\0';

    {
        std::lock_guard<std::mutex> lock(
            hbpl_memory_mutex
        );

        hbpl_allocations.insert(result);
    }

    return result;
}


// ============================================================
// BORROW / PREST
// ============================================================
//
// prest() NO reserva memoria.
// Solamente devuelve la misma dirección.
//
// La validación de ownership real debe hacerla el compilador.
// ============================================================

extern "C"
void* hbpl_prest(void* ptr) {

    return ptr;
}


// ============================================================
// MOVE
// ============================================================
//
// move() tampoco copia.
//
// El compilador debe marcar la variable original como
// "Moved" para impedir:
//
//     x.move()
//     x.free()
//
// desde HBPL.
// ============================================================

extern "C"
void* hbpl_move(void* ptr) {

    return ptr;
}


// ============================================================
// VALIDAR MEMORIA
// ============================================================

extern "C"
bool hbpl_is_allocated(void* ptr) {

    if (!ptr) {
        return false;
    }

    std::lock_guard<std::mutex> lock(
        hbpl_memory_mutex
    );

    return hbpl_allocations.contains(ptr);
}


// ============================================================
// TAMAÑO DE TIPO
// ============================================================

extern "C"
std::size_t hbpl_sizeof(std::size_t size) {

    return size;
}


// ============================================================
// MEMORIA RAW
// ============================================================

extern "C"
void hbpl_memcpy(
    void* destination,
    const void* source,
    std::size_t size
) {

    if (!destination || !source) {
        hbpl_error("memcpy recibió un puntero nulo");
    }

    std::memcpy(
        destination,
        source,
        size
    );
}


extern "C"
void hbpl_memset(
    void* destination,
    int value,
    std::size_t size
) {

    if (!destination) {
        hbpl_error("memset recibió un puntero nulo");
    }

    std::memset(
        destination,
        value,
        size
    );
}


// ============================================================
// CONSOLE
// ============================================================

extern "C"
void hbpl_console_log(const char* msg) {

    if (!msg) {
        std::puts("(null)");
        return;
    }

    std::puts(msg);
}


extern "C"
void hbpl_console_log_char(char value) {

    std::printf(
        "%c\n",
        value
    );
}


extern "C"
void hbpl_console_log_short(short value) {

    std::printf(
        "%hd\n",
        value
    );
}


extern "C"
void hbpl_console_log_int(int value) {

    std::printf(
        "%d\n",
        value
    );
}


extern "C"
void hbpl_console_log_long(long value) {

    std::printf(
        "%ld\n",
        value
    );
}


extern "C"
void hbpl_console_log_longlong(
    long long value
) {

    std::printf(
        "%lld\n",
        value
    );
}


extern "C"
void hbpl_console_log_uchar(
    unsigned char value
) {

    std::printf(
        "%u\n",
        static_cast<unsigned>(value)
    );
}


extern "C"
void hbpl_console_log_ushort(
    unsigned short value
) {

    std::printf(
        "%hu\n",
        value
    );
}


extern "C"
void hbpl_console_log_uint(
    unsigned int value
) {

    std::printf(
        "%u\n",
        value
    );
}


extern "C"
void hbpl_console_log_ulong(
    unsigned long value
) {

    std::printf(
        "%lu\n",
        value
    );
}


extern "C"
void hbpl_console_log_ulonglong(
    unsigned long long value
) {

    std::printf(
        "%llu\n",
        value
    );
}


extern "C"
void hbpl_console_log_float(
    float value
) {

    std::printf(
        "%f\n",
        static_cast<double>(value)
    );
}


extern "C"
void hbpl_console_log_double(
    double value
) {

    std::printf(
        "%f\n",
        value
    );
}


extern "C"
void hbpl_console_log_bool(
    bool value
) {

    std::puts(
        value ? "true" : "false"
    );
}


// ============================================================
// INPUT
// ============================================================

extern "C"
char* hbpl_console_readline() {

    char buffer[4096];

    if (!std::fgets(
        buffer,
        sizeof(buffer),
        stdin
    )) {

        return hbpl_str_reserv("");
    }

    std::size_t len =
        std::strlen(buffer);

    if (
        len > 0 &&
        buffer[len - 1] == '\n'
    ) {
        buffer[len - 1] = '\0';
    }

    return hbpl_str_reserv(buffer);
}


// ============================================================
// EXIT
// ============================================================
//
// hbpl_finish() es generado por el compilador HBPL.
// ============================================================

extern "C"
void hbpl_finish();


extern "C"
void hbpl_exit(int code) {

    if (!hbpl_finishing) {

        hbpl_finishing = true;

        hbpl_finish();
    }

    std::exit(code);
}


// ============================================================
// MAIN DEL RUNTIME
// ============================================================
//
// El compilador genera:
//
//     extern "C" void hbpl_main();
//     extern "C" void hbpl_finish();
// ============================================================

extern "C"
void hbpl_main();


int main() {

    hbpl_main();

    hbpl_exit(0);

    return 0;
}
