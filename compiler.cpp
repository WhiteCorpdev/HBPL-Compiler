#include <any>
#include <cctype>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <map>
#include <sstream>
#include <string>
#include <vector>

using namespace std;


// ============================================================
// ERRORES
// ============================================================

[[noreturn]] void fallo(const string& where, const string& msg) {

    cerr << "Err: " << msg << " (" << where << ")\n";

    exit(1);
}


// ============================================================
// POSICION
// ============================================================

struct pos {

    int chara;
    int line;

    string Print() {
        return "line: " +
               to_string(line) +
               ", character: " +
               to_string(chara);
    }

    void NextLine() {
        chara = 1;
        line++;
    }

    void NextChara() {
        chara++;
    }
};


// ============================================================
// TOKEN TYPES
// ============================================================

enum TokenTypes {

    FnToken,                 // 0
    FnameToken,              // 1
    OpenparenthesisToken,    // 2
    CloseparenthesisToken,   // 3
    BiggerthanToken,         // 4
    ConsoleToken,            // 5
    LogToken,                // 6
    VarArgToken,             // 7  (ya no se usa)
    colonToken,              // 8
    indentToken,             // 9
    spaceToken,              // 10
    StringToken,             // 11
    DotToken,                // 12
    ExitToken,               // 13
    StrToken,                // 14  palabra clave: str
    ManualToken,             // 15  palabra clave: manual
    IdentToken,              // 16  cualquier otro nombre
    AtToken,                 // 17  @
    EqualToken               // 18  =
};


// ============================================================
// TOKEN
// ============================================================

class token {

private:

    TokenTypes _tokentype;
    pos _pos;
    string _text;
    any _value;


public:

    token(
        TokenTypes type,
        pos position,
        string text,
        any value = any()
    ) {

        _tokentype = type;
        _pos = position;
        _text = text;
        _value = value;
    }


    TokenTypes GetTokentype() {
        return _tokentype;
    }


    string GetText() {
        return _text;
    }


    string GetPosition() {
        return _pos.Print();
    }


    any GetValue() {
        return _value;
    }


    string GetAll() {

        return
            "Type: " +
            to_string(static_cast<int>(_tokentype)) +

            "\nPosition: " +
            _pos.Print() +

            "\nText: " +
            _text;
    }
};


// ============================================================
// LEXER  (texto -> tokens)
// ============================================================

// Quita el comentario // de una linea (ignora // dentro de un string)
static string quitarComentario(const string& line) {

    bool inStr = false;

    for (size_t i = 0; i < line.size(); i++) {

        if (line[i] == '"') {
            inStr = !inStr;
        }

        else if (
            !inStr &&
            line[i] == '/' &&
            i + 1 < line.size() &&
            line[i + 1] == '/'
        ) {
            return line.substr(0, i);
        }
    }

    return line;
}


class lexer {

private:

    ifstream _lines;


public:

    lexer(string file)
        : _lines(file) {

        if (!_lines) {
            cerr << "Err: no se pudo abrir el archivo: " << file << "\n";
            exit(1);
        }
    }


    vector<token> parse() {

        vector<token> tokens;

        string line;

        // Posicion actual
        pos currentPos{1, 1};

        // Indica que la siguiente linea necesita indentacion
        bool defFN = false;


        while (getline(_lines, line)) {

            // Quitar el \r de los archivos de Windows
            if (!line.empty() && line.back() == '\r') {
                line.pop_back();
            }

            line = quitarComentario(line);


            // Lineas vacias: se saltan
            if (line.find_first_not_of(" \t") == string::npos) {

                currentPos.NextLine();

                continue;
            }


            string buf = "";

            bool inFN = false;
            bool inString = false;

            string stringBuf = "";

            int spaceAcum = 0;

            pos bufferPos{1, 1};


            // =================================================
            // COMPROBAR INDENTACION
            // =================================================

            if (defFN) {

                int spaces = 0;

                while (
                    spaces < static_cast<int>(line.size()) &&
                    line[spaces] == ' '
                ) {
                    spaces++;
                }

                // Necesitamos minimo 4 espacios
                if (spaces < 4) {

                    fallo(
                        currentPos.Print(),
                        "se esperaba indentacion de 4 espacios"
                    );
                }

                defFN = false;
            }


            // =================================================
            // PROCESAR BUFFER (palabra acumulada)
            // =================================================

            auto processBuffer = [&]() {

                if (buf.empty()) {
                    return;
                }

                TokenTypes type;

                if (buf == "fn") {
                    type = FnToken;
                    inFN = true;
                }

                else if (buf == "Console") type = ConsoleToken;
                else if (buf == "Log")     type = LogToken;
                else if (buf == "exit")    type = ExitToken;
                else if (buf == "str")     type = StrToken;
                else if (buf == "manual")  type = ManualToken;

                else if (inFN) {
                    type = FnameToken;
                    inFN = false;
                }

                else {
                    type = IdentToken;
                }

                tokens.push_back(token(type, bufferPos, buf));

                buf = "";
            };


            // =================================================
            // RECORRER LINEA
            // =================================================

            for (char c : line) {

                // Cualquier caracter que no sea espacio corta la racha
                if (c != ' ') {
                    spaceAcum = 0;
                }


                // -------------------------------------------------
                // STRING
                // -------------------------------------------------

                if (c == '"') {

                    if (!inString) {

                        inString = true;

                        stringBuf = "";
                    }

                    else {

                        inString = false;

                        tokens.push_back(
                            token(
                                TokenTypes::StringToken,
                                currentPos,
                                stringBuf
                            )
                        );

                        stringBuf = "";
                    }

                    currentPos.NextChara();

                    continue;
                }


                if (inString) {

                    stringBuf.push_back(c);

                    currentPos.NextChara();

                    continue;
                }


                // -------------------------------------------------
                // SEPARADORES
                // -------------------------------------------------

                if (
                    c == ' ' ||
                    c == '(' ||
                    c == ')' ||
                    c == '>' ||
                    c == ',' ||
                    c == '.' ||
                    c == '=' ||
                    c == '@' ||
                    c == ';'
                ) {

                    processBuffer();


                    if (c == ' ') {

                        // Guardamos la posicion del primer espacio de la racha
                        pos spacePos = currentPos;

                        tokens.push_back(
                            token(
                                TokenTypes::spaceToken,
                                currentPos,
                                " "
                            )
                        );

                        spaceAcum++;

                        // 4 espacios seguidos = INDENT
                        if (spaceAcum == 4) {

                            for (int i = 0; i < 4; i++) {
                                tokens.pop_back();
                            }

                            tokens.push_back(
                                token(
                                    TokenTypes::indentToken,
                                    spacePos,
                                    "indent"
                                )
                            );

                            spaceAcum = 0;
                        }
                    }

                    if (c == '(') {

                        tokens.push_back(
                            token(
                                TokenTypes::OpenparenthesisToken,
                                currentPos,
                                "("
                            )
                        );
                    }

                    if (c == ')') {

                        tokens.push_back(
                            token(
                                TokenTypes::CloseparenthesisToken,
                                currentPos,
                                ")"
                            )
                        );
                    }

                    if (c == '>') {

                        tokens.push_back(
                            token(
                                TokenTypes::BiggerthanToken,
                                currentPos,
                                ">"
                            )
                        );

                        // La siguiente linea debe tener indentacion
                        defFN = true;
                    }

                    if (c == ',') {

                        tokens.push_back(
                            token(
                                TokenTypes::colonToken,
                                currentPos,
                                ","
                            )
                        );
                    }

                    if (c == '.') {

                        tokens.push_back(
                            token(
                                TokenTypes::DotToken,
                                currentPos,
                                "."
                            )
                        );
                    }

                    if (c == '=') {

                        tokens.push_back(
                            token(
                                TokenTypes::EqualToken,
                                currentPos,
                                "="
                            )
                        );
                    }

                    if (c == '@') {

                        tokens.push_back(
                            token(
                                TokenTypes::AtToken,
                                currentPos,
                                "@"
                            )
                        );
                    }

                    // ';' se ignora: no genera token

                    currentPos.NextChara();

                    continue;
                }


                // -------------------------------------------------
                // CARACTER NORMAL
                // -------------------------------------------------

                if (buf.empty()) {

                    // Guardamos donde comenzo el buffer
                    bufferPos = currentPos;
                }

                buf.push_back(c);

                currentPos.NextChara();
            }


            // Procesar lo que quede al final de la linea
            processBuffer();

            currentPos.NextLine();
        }

        return tokens;
    }
};


// ============================================================
// AST  (la estructura del programa)
// ============================================================

// Un valor: "texto", mens, o mens.prest()
struct Expr {

    enum Kind { Str, Var, Borrow } kind = Str;

    string text;   // el texto literal o el nombre de la variable
};


struct Stmt {

    enum Kind { Log, Exit, VarDecl, Reserv, Free, Call } kind = Log;

    string name;            // variable (VarDecl/Reserv/Free) o funcion (Call)
    bool manual = false;    // VarDecl: tiene @manual
    int code = 0;           // Exit
    vector<Expr> args;      // Log: 1 valor / VarDecl: valor inicial / Call: argumentos
    string where;           // posicion, para los mensajes de error
};


struct Param {

    string type;
    string name;
};


struct Function {

    string name;
    vector<Param> params;
    vector<Stmt> body;
};


struct Program {

    vector<Function> functions;
};


// ============================================================
// PARSER  (tokens -> Program)
// ============================================================

class parser {

private:

    vector<token> _t;
    size_t _i = 0;


public:

    parser(const vector<token>& tokens) {

        // El parser no necesita espacios ni indentacion
        for (token tk : tokens) {

            TokenTypes ty = tk.GetTokentype();

            if (ty == spaceToken || ty == indentToken) {
                continue;
            }

            _t.push_back(tk);
        }
    }


    bool atEnd() {
        return _i >= _t.size();
    }


    TokenTypes peek() {
        return _t[_i].GetTokentype();
    }


    // Exige un token concreto o da error
    token expect(TokenTypes type, const string& what) {

        if (atEnd()) {
            fallo("fin del archivo", "se esperaba " + what);
        }

        if (_t[_i].GetTokentype() != type) {
            fallo(_t[_i].GetPosition(), "se esperaba " + what);
        }

        return _t[_i++];
    }


    // "texto"  |  nombre  |  nombre.prest()
    Expr parseExpr() {

        Expr e;

        if (atEnd()) {
            fallo("fin del archivo", "se esperaba un valor");
        }

        if (peek() == StringToken) {

            e.kind = Expr::Str;
            e.text = _t[_i++].GetText();

            return e;
        }

        if (peek() == IdentToken) {

            e.kind = Expr::Var;
            e.text = _t[_i++].GetText();

            if (!atEnd() && peek() == DotToken) {

                _i++;

                token m = expect(IdentToken, "'prest'");

                if (m.GetText() != "prest") {
                    fallo(m.GetPosition(),
                          "metodo desconocido en un valor: " + m.GetText());
                }

                expect(OpenparenthesisToken, "'('");
                expect(CloseparenthesisToken, "')'");

                e.kind = Expr::Borrow;
            }

            return e;
        }

        fallo(_t[_i].GetPosition(), "se esperaba un valor (texto o variable)");
    }


    // Console.Log(valor)
    Stmt parseLog() {

        Stmt s;
        s.kind = Stmt::Log;
        s.where = _t[_i].GetPosition();

        expect(ConsoleToken, "'Console'");
        expect(DotToken, "'.'");
        expect(LogToken, "'Log'");
        expect(OpenparenthesisToken, "'('");

        s.args.push_back(parseExpr());

        expect(CloseparenthesisToken, "')'");

        return s;
    }


    // exit(Complete)  |  exit(3)  |  exit()
    Stmt parseExit() {

        Stmt s;
        s.kind = Stmt::Exit;
        s.where = _t[_i].GetPosition();

        expect(ExitToken, "'exit'");
        expect(OpenparenthesisToken, "'('");

        if (!atEnd() && peek() == IdentToken) {

            string arg = _t[_i++].GetText();

            if (arg != "Complete") {

                char* fin;
                long v = strtol(arg.c_str(), &fin, 10);

                if (*fin != '\0') {
                    fallo(s.where, "codigo de salida invalido: " + arg);
                }

                s.code = static_cast<int>(v);
            }
        }

        expect(CloseparenthesisToken, "')'");

        return s;
    }


    // [@manual] str nombre = valor
    Stmt parseDecl() {

        Stmt s;
        s.kind = Stmt::VarDecl;
        s.where = _t[_i].GetPosition();

        if (peek() == AtToken) {

            _i++;

            expect(ManualToken, "'manual' despues de '@'");

            s.manual = true;
        }

        expect(StrToken, "el tipo 'str'");

        s.name = expect(IdentToken, "nombre de variable").GetText();

        expect(EqualToken, "'='");

        s.args.push_back(parseExpr());

        return s;
    }


    // nombre.reserv()  |  nombre.free()  |  nombre(args)
    Stmt parseIdentStmt() {

        Stmt s;
        s.where = _t[_i].GetPosition();

        string name = expect(IdentToken, "un nombre").GetText();

        // ---- metodo: variable.algo() ----
        if (!atEnd() && peek() == DotToken) {

            _i++;

            token m = expect(IdentToken, "nombre de metodo");

            expect(OpenparenthesisToken, "'('");
            expect(CloseparenthesisToken, "')'");

            s.name = name;

            if (m.GetText() == "reserv") {
                s.kind = Stmt::Reserv;
            }

            else if (m.GetText() == "free") {
                s.kind = Stmt::Free;
            }

            else {
                fallo(m.GetPosition(),
                      "metodo desconocido: " + m.GetText() +
                      " (los validos son reserv, free y prest)");
            }

            return s;
        }

        // ---- llamada: funcion(args) ----
        if (!atEnd() && peek() == OpenparenthesisToken) {

            _i++;

            s.kind = Stmt::Call;
            s.name = name;

            while (!atEnd() && peek() != CloseparenthesisToken) {

                s.args.push_back(parseExpr());

                if (!atEnd() && peek() == colonToken) {
                    _i++;
                }
            }

            expect(CloseparenthesisToken, "')'");

            return s;
        }

        fallo(s.where, "instruccion no valida despues de '" + name + "'");
    }


    Stmt parseStmt() {

        TokenTypes t = peek();

        if (t == ConsoleToken)                  return parseLog();
        if (t == ExitToken)                     return parseExit();
        if (t == AtToken || t == StrToken)      return parseDecl();
        if (t == IdentToken)                    return parseIdentStmt();

        fallo(_t[_i].GetPosition(), "instruccion inesperada");
    }


    // fn nombre(str a, str b)>  instrucciones...
    Function parseFunction() {

        Function f;

        expect(FnToken, "'fn'");

        f.name = expect(FnameToken, "nombre de funcion").GetText();

        expect(OpenparenthesisToken, "'('");

        while (!atEnd() && peek() != CloseparenthesisToken) {

            Param pr;

            expect(StrToken, "un tipo (por ahora solo 'str')");

            pr.type = "str";
            pr.name = expect(IdentToken, "nombre del parametro").GetText();

            if (!atEnd() && peek() == EqualToken) {
                fallo(_t[_i].GetPosition(),
                      "los valores por defecto todavia no estan soportados");
            }

            f.params.push_back(pr);

            if (!atEnd() && peek() == colonToken) {
                _i++;
            }
        }

        expect(CloseparenthesisToken, "')'");
        expect(BiggerthanToken, "'>'");

        // El cuerpo dura hasta el siguiente 'fn' o el final del archivo
        while (!atEnd() && peek() != FnToken) {
            f.body.push_back(parseStmt());
        }

        return f;
    }


    Program parseProgram() {

        Program p;

        while (!atEnd()) {
            p.functions.push_back(parseFunction());
        }

        return p;
    }
};


// ============================================================
// ANALISIS  (reglas de @manual y comprobaciones)
// ============================================================

enum class Estado { SinReservar, Reservada, Liberada };

struct VarInfo {

    bool manual = false;
    Estado estado = Estado::SinReservar;
};


void analizar(const Program& p) {

    // Tabla de funciones
    map<string, const Function*> fns;

    for (const Function& f : p.functions) {

        if (fns.count(f.name)) {
            fallo("fn " + f.name, "la funcion '" + f.name + "' esta repetida");
        }

        if (f.params.size() > 4) {
            fallo("fn " + f.name, "por ahora una funcion admite maximo 4 parametros");
        }

        fns[f.name] = &f;
    }

    if (!fns.count("main")) {
        fallo("programa", "el programa no tiene 'fn main'");
    }


    for (const Function& f : p.functions) {

        map<string, VarInfo> vars;

        // Los parametros son variables NO manuales: la funcion solo las "toma prestadas"
        for (const Param& pr : f.params) {

            if (vars.count(pr.name)) {
                fallo("fn " + f.name, "parametro repetido: " + pr.name);
            }

            vars[pr.name] = VarInfo();
        }


        // Comprueba que un valor se pueda usar
        auto usar = [&](const Expr& e, const string& where, bool esArgumento) {

            if (e.kind == Expr::Str) {
                return;
            }

            auto it = vars.find(e.text);

            if (it == vars.end()) {
                fallo(where, "la variable '" + e.text + "' no existe");
            }

            VarInfo& v = it->second;

            if (v.manual && v.estado == Estado::Liberada) {
                fallo(where, "'" + e.text + "' ya fue liberada (use after free)");
            }

            if (e.kind == Expr::Borrow) {

                if (!v.manual) {
                    fallo(where, "prest() solo se usa en variables @manual");
                }

                if (v.estado != Estado::Reservada) {
                    fallo(where, "no se puede prestar '" + e.text +
                                 "': primero usa " + e.text + ".reserv()");
                }
            }

            if (e.kind == Expr::Var && esArgumento && v.manual) {
                fallo(where, "'" + e.text + "' es @manual: pasala con " +
                             e.text + ".prest()");
            }
        };


        for (const Stmt& s : f.body) {

            switch (s.kind) {

            case Stmt::Log:

                usar(s.args[0], s.where, false);

                break;


            case Stmt::Exit:

                break;


            case Stmt::VarDecl: {

                if (vars.count(s.name)) {
                    fallo(s.where, "la variable '" + s.name + "' ya existe");
                }

                if (s.args[0].kind == Expr::Borrow) {
                    fallo(s.where, "prest() solo se usa al llamar a una funcion");
                }

                usar(s.args[0], s.where, true);

                VarInfo v;
                v.manual = s.manual;

                vars[s.name] = v;

                break;
            }


            case Stmt::Reserv: {

                auto it = vars.find(s.name);

                if (it == vars.end()) {
                    fallo(s.where, "la variable '" + s.name + "' no existe");
                }

                if (!it->second.manual) {
                    fallo(s.where, "'" + s.name + "' no es @manual, no puede usar reserv()");
                }

                if (it->second.estado == Estado::Reservada) {
                    fallo(s.where, "'" + s.name + "' ya esta reservada");
                }

                it->second.estado = Estado::Reservada;

                break;
            }


            case Stmt::Free: {

                auto it = vars.find(s.name);

                if (it == vars.end()) {
                    fallo(s.where, "la variable '" + s.name + "' no existe");
                }

                if (!it->second.manual) {
                    fallo(s.where, "'" + s.name + "' no es @manual, no puede usar free()");
                }

                if (it->second.estado == Estado::SinReservar) {
                    fallo(s.where, "'" + s.name + "' nunca se reservo con reserv()");
                }

                if (it->second.estado == Estado::Liberada) {
                    fallo(s.where, "'" + s.name + "' ya fue liberada (double free)");
                }

                it->second.estado = Estado::Liberada;

                break;
            }


            case Stmt::Call: {

                auto it = fns.find(s.name);

                if (it == fns.end()) {
                    fallo(s.where, "la funcion '" + s.name + "' no existe");
                }

                if (s.args.size() != it->second->params.size()) {
                    fallo(s.where, "'" + s.name + "' espera " +
                                   to_string(it->second->params.size()) +
                                   " argumento(s) y recibio " +
                                   to_string(s.args.size()));
                }

                for (const Expr& a : s.args) {
                    usar(a, s.where, true);
                }

                break;
            }
            }
        }


        // Avisos: memoria reservada que nunca se libero
        for (const auto& kv : vars) {

            if (kv.second.manual && kv.second.estado == Estado::Reservada) {

                cerr << "Aviso: '" << kv.first << "' en fn " << f.name
                     << " se reservo pero nunca se libero (fuga de memoria)\n";
            }
        }
    }
}


// ============================================================
// GENERADOR DE ASM  (Program -> out.s)
// ============================================================

// main -> hbpl_main, finish -> hbpl_finish, el resto -> hbpl_fn_nombre
string asmName(const string& n) {

    if (n == "main")   return "hbpl_main";
    if (n == "finish") return "hbpl_finish";

    return "hbpl_fn_" + n;
}


// Los strings del .hbpl pueden traer " \ o saltos de linea,
// y eso romperia el .asciz si no se escapa
string escaparAsm(const string& s) {

    string r;

    for (unsigned char c : s) {

        if      (c == '"')  r += "\\\"";
        else if (c == '\\') r += "\\\\";
        else if (c == '\n') r += "\\n";
        else if (c == '\t') r += "\\t";

        else if (c < 32) {

            char b[8];
            snprintf(b, sizeof b, "\\%03o", c);
            r += b;
        }

        else r += static_cast<char>(c);
    }

    return r;
}


void generar(const Program& p, const string& archivo) {

    vector<string> strings;   // textos que van a .rdata
    ostringstream code;

    bool hayFinish = false;

    // Registros de los 4 primeros argumentos en Windows x64
    const char* regs[4] = {"%rcx", "%rdx", "%r8", "%r9"};


    for (const Function& f : p.functions) {

        if (f.name == "finish") hayFinish = true;


        // Cada variable (parametro o local) ocupa 8 bytes en la pila,
        // por encima de los 32 bytes de shadow space
        map<string, int> slot;

        for (const Param& pr : f.params) {
            int n = static_cast<int>(slot.size());
            slot[pr.name] = n;
        }

        for (const Stmt& s : f.body) {
            if (s.kind == Stmt::VarDecl) {
                int n = static_cast<int>(slot.size());
                slot[s.name] = n;
            }
        }

        // Al entrar a la funcion rsp = 8 (mod 16): el marco debe ser 8 (mod 16)
        int frame = 32 + 8 * static_cast<int>(slot.size());

        if (frame % 16 == 0) frame += 8;


        auto offset = [&](const string& v) {
            return 32 + 8 * slot[v];
        };

        // Carga un valor en un registro
        auto cargar = [&](const Expr& e, const string& reg) {

            if (e.kind == Expr::Str) {

                code << "    leaq .LC" << strings.size()
                     << "(%rip), " << reg << "\n";

                strings.push_back(e.text);
            }

            else {   // Var o Borrow: ambos pasan el puntero

                code << "    movq " << offset(e.text)
                     << "(%rsp), " << reg << "\n";
            }
        };


        string name = asmName(f.name);

        code << "    .globl " << name << "\n";
        code << name << ":\n";
        code << "    subq $" << frame << ", %rsp\n";

        // Guardar los parametros recibidos en sus casillas de la pila
        for (size_t i = 0; i < f.params.size(); i++) {

            code << "    movq " << regs[i] << ", "
                 << offset(f.params[i].name) << "(%rsp)\n";
        }


        for (const Stmt& s : f.body) {

            switch (s.kind) {

            case Stmt::Log:

                cargar(s.args[0], "%rcx");
                code << "    call hbpl_console_log\n";

                break;


            case Stmt::Exit:

                code << "    movl $" << s.code << ", %ecx\n";
                code << "    call hbpl_exit\n";

                break;


            case Stmt::VarDecl:

                cargar(s.args[0], "%rax");
                code << "    movq %rax, " << offset(s.name) << "(%rsp)\n";

                break;


            case Stmt::Reserv:

                code << "    movq " << offset(s.name) << "(%rsp), %rcx\n";
                code << "    call hbpl_str_reserv\n";
                code << "    movq %rax, " << offset(s.name) << "(%rsp)\n";

                break;


            case Stmt::Free:

                code << "    movq " << offset(s.name) << "(%rsp), %rcx\n";
                code << "    call hbpl_free\n";

                break;


            case Stmt::Call:

                for (size_t i = 0; i < s.args.size(); i++) {
                    cargar(s.args[i], regs[i]);
                }

                code << "    call " << asmName(s.name) << "\n";

                break;
            }
        }

        code << "    addq $" << frame << ", %rsp\n";
        code << "    ret\n\n";
    }


    // El runtime siempre llama a hbpl_finish, asi que si el usuario
    // no escribio 'fn finish', generamos uno vacio
    if (!hayFinish) {

        code << "    .globl hbpl_finish\n";
        code << "hbpl_finish:\n";
        code << "    ret\n";
    }


    ofstream out(archivo);

    if (!out) {
        fallo("archivo", "no se pudo crear " + archivo);
    }

    out << "    .section .rdata\n";

    for (size_t i = 0; i < strings.size(); i++) {

        out << ".LC" << i << ":\n"
            << "    .asciz \"" << escaparAsm(strings[i]) << "\"\n";
    }

    out << "\n    .text\n";
    out << code.str();
}


// ============================================================
// MAIN
// ============================================================

int main(int argc, char* argv[]) {

    if (argc < 2) {

        cout << "Uso: compiler archivo.hbpl\n";

        return 1;
    }


    // texto -> tokens
    lexer lex(argv[1]);
    vector<token> tokens = lex.parse();


    // tokens -> Program
    parser p(tokens);
    Program prog = p.parseProgram();


    // comprobar reglas (@manual, variables, funciones)
    analizar(prog);


    // Program -> asm
    filesystem::create_directories(".bin");

    generar(prog, ".bin/out.s");


    // asm + runtime -> exe
    if (system("g++ -c runtime.cpp -o .bin/runtime.o") != 0) {

        cerr << "Err: fallo al compilar runtime.cpp\n";

        return 1;
    }

    if (system("g++ .bin/out.s .bin/runtime.o -o .bin/out.exe") != 0) {

        cerr << "Err: fallo al ensamblar out.s\n";

        return 1;
    }
    if (system("rm .bin/out.s") != 0){
        cerr << "Err: fallo al eliminar archivo temporal";
        return 1;
    }



    return 0;
}
