# HBPL 1.0

**HBPL** es un lenguaje de programación compilado diseñado para crear aplicaciones de propósito general, incluyendo programas de consola, aplicaciones de escritorio, servidores, herramientas del sistema, juegos y software de bajo nivel.

HBPL busca combinar una sintaxis sencilla con control de tipos, programación orientada a objetos y administración explícita de memoria.

> **Versión:** 1.0
> **Estado:** Stable
> **Paquetes externos:** No incluidos en 1.0

---

# 1. Introducción

HBPL utiliza una sintaxis basada en bloques terminados mediante `end`.

Ejemplo:

```hbpl
fn main()>
    Console.Log("Hola, mundo")
end
```

Los bloques de funciones, clases y estructuras de control utilizan `>` para comenzar el bloque y `end` para finalizarlo.

---

# 2. Comentarios

Los comentarios de una línea comienzan con `//`.

```hbpl
// Este es un comentario

fn main()>
    Console.Log("Hola")
end
```

Todo lo que aparece después de `//` hasta el final de la línea es ignorado por el compilador.

---

# 3. Tipos de datos

HBPL proporciona los siguientes tipos primitivos:

```text
str
char

short
int
long
longlong

uchar
ushort
uint
ulong
ulonglong

float
double

bool
```

## 3.1 Strings

```hbpl
str nombre = "Juan"

Console.Log(nombre)
```

Los strings pueden concatenarse mediante `+`.

```hbpl
str nombre = "Juan"
str mensaje = "Hola " + nombre

Console.Log(mensaje)
```

También pueden convertirse otros valores a `str`:

```hbpl
int edad = 15

Console.Log("Edad: " + str(edad))
```

---

# 4. Booleanos

Los valores booleanos son:

```hbpl
true
false
```

Ejemplo:

```hbpl
bool activo = true

if activo>
    Console.Log("Activo")
end
```

---

# 5. Null

HBPL dispone del valor:

```hbpl
null
```

Los tipos pueden representar un valor nulo.

```hbpl
str nombre = null
```

El compilador debe comprobar los usos inválidos de valores nulos cuando sea necesario.

---

# 6. Variables

Las variables se declaran indicando su tipo y nombre.

```hbpl
int edad = 15
str nombre = "Juan"
double precio = 10.5
bool activo = true
```

Las variables son mutables por defecto.

```hbpl
int numero = 10
numero = 20
```

---

# 7. Constantes

Las constantes se declaran utilizando `const`.

```hbpl
const int MAX = 100
```

Una constante no puede modificarse después de su inicialización.

```hbpl
const int MAX = 100

MAX = 200 // Error
```

HBPL no utiliza `mut`.

---

# 8. Conversiones

Las conversiones utilizan la sintaxis:

```text
tipo(valor)
```

Ejemplos:

```hbpl
int numero = int("15")

str texto = str(20)

float decimal = float(10)

double valor = double(15)
```

---

# 9. Operadores

## 9.1 Aritméticos

```text
+
-
*
/
%
```

Ejemplo:

```hbpl
int resultado = 10 + 5 * 2
```

Los operadores respetan precedencia matemática.

---

## 9.2 Comparación

HBPL permite realizar comparaciones entre valores.

```text
==
!=
<
>
<=
>=
```

Ejemplo:

```hbpl
if edad >= 18>
    Console.Log("Mayor")
end
```

---

## 9.3 Operadores lógicos

HBPL proporciona operaciones lógicas para trabajar con condiciones booleanas.

```text
and
or
not
```

Ejemplo:

```hbpl
if activo and edad > 10>
    Console.Log("Correcto")
end
```

---

# 10. Funciones

Las funciones se declaran mediante `fn`.

```hbpl
fn saludar()>
    Console.Log("Hola")
end
```

La función principal del programa es:

```hbpl
fn main()>
    Console.Log("Hola")
end
```

---

# 11. Parámetros

Los parámetros pueden declararse con su tipo.

```hbpl
fn sumar(int a, int b) -> int>
    return a + b
end
```

Uso:

```hbpl
int resultado = sumar(10, 20)
```

HBPL también permite parámetros sin tipo explícito en determinados contextos soportados por el compilador.

---

# 12. Return

Una función puede devolver un valor mediante `return`.

```hbpl
fn multiplicar(int a, int b) -> int>
    return a * b
end
```

El valor devuelto debe ser compatible con el tipo de retorno declarado.

---

# 13. Control de flujo

## 13.1 If

```hbpl
if edad >= 18>
    Console.Log("Adulto")
end
```

---

## 13.2 Else

```hbpl
if edad >= 18>
    Console.Log("Adulto")
else>
    Console.Log("Menor")
end
```

---

## 13.3 Else if

```hbpl
if edad >= 18>
    Console.Log("Adulto")
else if edad >= 13>
    Console.Log("Adolescente")
else>
    Console.Log("Niño")
end
```

---

# 14. While

```hbpl
int numero = 0

while numero < 10>
    Console.Log(numero)
    numero = numero + 1
end
```

El bloque se ejecuta mientras la condición sea verdadera.

---

# 15. For

HBPL dispone de bucles `for` para realizar iteraciones automáticas.

```hbpl
for ...
    // código
end
```

El incremento del ciclo es administrado automáticamente por el lenguaje según la forma de `for` utilizada.

---
# 15.1 break

HBPL para parar un bucle se usa `break`

```hbpl
for ...
  if i == 1>
    break
  end
  //codigo
end
```
___
# 15.2 continue

HBPL para parar un bucle se usa `continue`

```hbpl
for ...
  if i == 1>
    continue
  end
  //codigo
end
```
___
# 16. Foreach

`foreach` permite recorrer elementos de una colección.

```hbpl
foreach estudiante in estudiantes>
    estudiante.say()
end
```

La variable del `foreach` representa el elemento actual de la colección.

---

# 17. Arrays

HBPL permite utilizar arrays indexados desde cero.

```text
0  1  2  3  4
```

El acceso se realiza mediante índices.

```hbpl
numeros[0]
```

El compilador debe detectar accesos que puedan demostrarse inválidos durante compilación y el programa debe mantener comprobaciones de límites cuando sean necesarias durante ejecución.

---

# 18. Listas

Las listas pueden declararse especificando su tipo:

```hbpl
list<int> numeros
```

También existe la posibilidad de declarar una lista sin especificar explícitamente su elemento cuando el compilador puede inferirlo.

```hbpl
list estudiantes = new list()
```

---

## 18.1 Push

Añade elementos:

```hbpl
estudiantes.push(juan)
```

También pueden añadirse varios elementos:

```hbpl
estudiantes.push(juan, karla, josh)
```

---

## 18.2 Pop

Elimina/extrae un elemento de la lista:

```hbpl
estudiantes.pop()
```

---

## 18.3 Len

Obtiene la cantidad de elementos:

```hbpl
estudiantes.len()
```

---

## 18.4 Clear

Elimina los elementos de la lista:

```hbpl
estudiantes.clear()
```

---

# 19. Clases

HBPL soporta programación orientada a objetos mediante `class`.

```hbpl
class Persona>
    str nombre
    int edad
end
```

---

# 20. Visibilidad

Los miembros de una clase pueden utilizar:

```text
[private]
[public]
[protected]
```

Ejemplo:

```hbpl
class Persona>
    [private]
    str nombre

    [private]
    int edad

    [public]
    fn say(me)>
        Console.Log(me.nombre)
    end
end
```

---

# 21. `me`

`me` representa la instancia actual de una clase.

```hbpl
class Persona>
    str nombre

    fn say(me)>
        Console.Log(me.nombre)
    end
end
```

Cuando se utiliza un miembro de la instancia:

```hbpl
me.nombre
```

---

# 22. Constructores

El constructor de una clase se define mediante `onCreated`.

```hbpl
class Persona>
    [private]
    str nombre

    [private]
    int edad

    fn onCreated(me, str nombre, int edad)>
        me.nombre = nombre
        me.edad = edad
    end
end
```

`onCreated` se ejecuta cuando se crea una nueva instancia.

---

# 23. Crear objetos

Los objetos se crean utilizando `new`.

```hbpl
Persona juan = new Persona("Juan", 15)
```

El constructor correspondiente se ejecutará automáticamente.

---

# 24. Métodos

Las clases pueden contener funciones.

```hbpl
class Persona>
    str nombre

    fn say(me)>
        Console.Log("Hola " + me.nombre)
    end
end
```

Uso:

```hbpl
juan.say()
```

---

# 25. Herencia

HBPL permite reutilizar comportamiento de una clase base.

La llamada a la implementación de la clase base se realiza mediante `Ext()`.

```hbpl
fn onCreated(me)>
    Ext(persona)
end
```

HBPL no utiliza la palabra clave `extends`.

---

# 26. Override

Los métodos heredados pueden reemplazarse utilizando `override`.

```hbpl
override say(me)>
    Console.Log("Nuevo comportamiento")
end
```

`override` no utiliza `fn`.

---

# 27. Types

HBPL permite declarar tipos de valor mediante `type`.

```hbpl
type Punto>
    int x
    int y
end
```

Uso:

```hbpl
Punto punto
```

Los `type` representan datos estructurados sin utilizar la palabra `struct`.

---

# 28. Enums

Los enumerados se declaran mediante `enum`.

```hbpl
enum Color>
    RED
    GREEN
    BLUE
end
```

Un enum representa un conjunto definido de valores.

---

# 29. Espacios de código

HBPL utiliza `codeSpace` para organizar código en espacios de nombres.

```hbpl
codeSpace MiPrograma>
    // código
end
```

Los `codeSpace` permiten organizar y separar componentes del programa.

---

# 30. Memoria

HBPL proporciona operaciones explícitas para trabajar con memoria.

Las operaciones principales son:

```text
reserv
free
prest
move
```

---

## 30.1 Reservar

`reserv` representa la reserva de memoria.

```hbpl
reserv ...
```

Una reserva puede encontrarse en diferentes estados durante su ciclo de vida.

---

## 30.2 Liberar

La memoria puede liberarse mediante:

```hbpl
free ...
```

El compilador debe detectar situaciones inválidas como liberar dos veces la misma reserva.

---

## 30.3 Prest

`prest` permite prestar una referencia/recurso sin transferir definitivamente su propiedad.

---

## 30.4 Move

`move` transfiere la propiedad de un recurso.

Después de un movimiento, el valor original puede quedar en estado `Moved` y no debe utilizarse como si siguiera siendo propietario.

---

# 31. Estados de memoria

HBPL define conceptualmente los estados:

```text
SinReservar
Reservada
Liberada
Moved
```

El analizador del compilador debe utilizar estos estados para detectar usos incorrectos de memoria.

Ejemplos de errores que HBPL busca detectar:

```text
double free
use after move
uso de memoria liberada
liberar memoria no propia
uso inválido de préstamos
```

---

# 32. Manual Memory

HBPL mantiene soporte para operaciones manuales mediante:

```text
@manual
```

Estas operaciones permiten al programador tener un mayor control sobre determinados recursos.

---

# 33. Manejo de errores

HBPL utiliza `try` y `catch` para manejar errores.

Conceptualmente:

```hbpl
try>
    // código que puede producir un error
catch>
    // manejo del error
end
```

El sistema de errores de HBPL no utiliza `Result` ni `Option` como mecanismo principal.

---

# 34. Consola

La biblioteca de consola proporciona operaciones básicas.

## Console.Log

```hbpl
Console.Log("Hola")
```

Muestra información en la consola.

---

## Console.Write

```hbpl
Console.Write("Hola")
```

Escribe información sin utilizar necesariamente el mismo comportamiento de línea de `Log`.

---

## Console.ReadLine

```hbpl
str nombre = Console.ReadLine()
```

Lee una línea introducida por el usuario.

---

## Console.ReadKey

```hbpl
char tecla = Console.ReadKey()
```

Lee una tecla.

---

## Console.Clear

```hbpl
Console.Clear()
```

Limpia la consola.

---

# 35. Programa mínimo

Un programa HBPL mínimo:

```hbpl
fn main()>
    Console.Log("Hola, mundo")
end
```

---

# 36. Programa completo de ejemplo

```hbpl
class estudiante>
    [private]
    int grado

    [private]
    str nombre

    fn onCreated(me, str nombre, int grado)>
        me.nombre = nombre
        me.grado = grado
    end

    fn say(me)>
        Console.Log(me.nombre + str(me.grado))
    end
end

fn main()>
    estudiante juan = new estudiante("juan", 6)
    estudiante karla = new estudiante("Karla", 4)
    estudiante josh = new estudiante("josh", 9)

    list estudiantes = new list()

    estudiantes.push(juan, karla, josh)

    foreach estudiante in estudiantes>
        estudiante.say()
    end
end

fn finish()>
    Console.Log("Finishing")
end
```

Este programa demuestra:

* clases
* campos privados
* constructor
* `me`
* objetos
* `new`
* listas
* `push`
* `foreach`
* métodos
* conversión `str()`
* `main`
* `finish`

---

# 37. `main`

Todo programa ejecutable debe tener una función:

```hbpl
fn main()>
    // programa
end
```

`main` es el punto de entrada de la aplicación.

---

# 38. `finish`

HBPL permite definir:

```hbpl
fn finish()>
    // limpieza final
end
```

`finish` se utiliza para ejecutar tareas de finalización del programa.

---

# 39. Sobrecarga

HBPL permite definir varias funciones con el mismo nombre cuando sus parámetros permiten diferenciarlas.

Conceptualmente:

```hbpl
fn mostrar(int numero)>
    ...
end

fn mostrar(str texto)>
    ...
end
```

El compilador determina qué función utilizar basándose en los argumentos.

---

# 40. Sistema de tipos

El compilador debe comprobar:

* tipos incompatibles
* variables inexistentes
* variables utilizadas antes de inicializarse
* funciones inexistentes
* métodos inexistentes
* cantidad incorrecta de argumentos
* tipos incorrectos de argumentos
* acceso inválido a miembros
* modificación de constantes
* operaciones inválidas
* errores de memoria conocidos durante compilación

Ejemplo:

```hbpl
int numero = "Hola"
```

Debe producir un error de compilación.

---

# 41. Scopes

Las variables pertenecen al ámbito donde fueron declaradas.

```hbpl
fn main()>
    int x = 10

    if true>
        int y = 20
    end

    Console.Log(x)
end
```

Una variable local de un bloque no debe estar disponible fuera de su ámbito.

---

# 42. Compilador

La arquitectura conceptual de HBPL es:

```text
Código HBPL
     │
     ▼
   Lexer
     │
     ▼
   Parser
     │
     ▼
     AST
     │
     ▼
 Analyzer
     │
     ▼
     IR
     │
     ▼
 Backend
     │
     ▼
 Código nativo
     │
     ▼
 Ejecutable
```

El backend utilizado durante el desarrollo de HBPL 1.0 puede utilizar C++ como etapa de generación mientras el lenguaje y su semántica se estabilizan.

El objetivo final del compilador es producir código nativo directamente mediante un backend propio.

---

# 43. Filosofía de HBPL

HBPL está diseñado alrededor de varias ideas:

### Simplicidad

La sintaxis debe ser fácil de leer.

### Tipado

El compilador debe detectar errores antes de ejecutar el programa.

### Control

El programador puede controlar explícitamente recursos y memoria.

### Seguridad

El compilador debe detectar tantos errores de memoria y tipos como sea posible.

### Rendimiento

HBPL está diseñado como lenguaje compilado, no como lenguaje interpretado tradicional.

### Generalidad

El lenguaje pretende servir para:

```text
Consola
Aplicaciones
GUI
Servidores
Networking
Juegos
Herramientas
Archivos
Software de sistema
```

---

# 44. Características fuera de HBPL 1.0

HBPL 1.0 no depende de un sistema de paquetes.

Las siguientes características pueden formar parte de versiones posteriores:

* gestor de paquetes
* paquetes externos
* generics
* macros
* async/await
* metaprogramación
* reflection
* operator overloading
* backend ASM definitivo
* librerías adicionales
* frameworks de web
* frameworks GUI

Estas características no son requisitos para considerar estable la versión 1.0.

---

# 45. Palabras reservadas principales

Entre las palabras reservadas y elementos especiales del lenguaje se encuentran:

```text
fn
class
type
enum

if
else
while
for
foreach
in

return

new
const
null
true
false

try
catch

override
onCreated
Ext

codeSpace

main
finish

reserv
free
prest
move

and
or
not
```

Los modificadores de visibilidad son:

```text
private
public
protected
```

y se utilizan mediante:

```hbpl
[private]
[public]
[protected]
```

---

# 46. Objetivo de HBPL 1.0

HBPL 1.0 representa la primera versión estable del lenguaje.

El objetivo de esta versión no es proporcionar todas las características posibles, sino establecer una base sólida:

```text
Sintaxis
   +
Tipos
   +
Funciones
   +
Clases
   +
Colecciones
   +
Control de flujo
   +
Memoria
   +
Análisis
   +
Compilación
```

A partir de esta base podrán desarrollarse las futuras versiones de HBPL.

---

# HBPL

**HBPL 1.0 — Stable**

> Un lenguaje diseñado para crear, controlar y compilar software.
