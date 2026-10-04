#include <any>
#include <cctype>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

using namespace std;


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
        chara = 0;
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
    VarArgToken,             // 7
    colonToken,              // 8
    indentToken,             // 9
    spaceToken,              // 10
    StringToken,             // 11
    DotToken                 // 12
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
// LEXER
// ============================================================

class lexer {

private:

    ifstream _lines;


public:

    lexer(string file)
        : _lines(file) {
    }


    vector<token> parse() {

        vector<token> tokens;

        string line;

        // Posición actual
        pos currentPos{1, 1};

        // Indica que la siguiente línea necesita indentación
        bool defFN = false;


        // ====================================================
        // LEER ARCHIVO
        // ====================================================

        while (getline(_lines, line)) {

            string buf = "";

            bool inFN = false;
            bool inArg = false;
            bool inString = false;

            string stringBuf = "";

            int spaceAcum = 0;


            // =================================================
            // COMPROBAR INDENTACIÓN
            // =================================================

            if (defFN) {

                int spaces = 0;

                while (
                    spaces < static_cast<int>(line.size()) &&
                    line[spaces] == ' '
                ) {
                    spaces++;
                }


                // Necesitamos mínimo 4 espacios
                if (spaces < 4) {

                    cerr << "Err--indent\n";

                    exit(-1);
                }

                defFN = false;
            }


            // =================================================
            // POSICIÓN DEL BUFFER
            // =================================================

            pos bufferPos;

            bool hasBuffer = false;


            // =================================================
            // RECORRER LINEA
            // =================================================

            for (char c : line) {


                // =================================================
                // STRING
                // =================================================

                if (c == '"') {

                    // ---------------------------------------------
                    // ABRIR STRING
                    // ---------------------------------------------

                    if (!inString) {

                        inString = true;

                        stringBuf = "";

                        currentPos.NextChara();

                        continue;
                    }


                    // ---------------------------------------------
                    // CERRAR STRING
                    // ---------------------------------------------

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

                        currentPos.NextChara();

                        continue;
                    }
                }


                // =================================================
                // DENTRO DE STRING
                // =================================================

                if (inString) {

                    stringBuf.push_back(c);

                    currentPos.NextChara();

                    continue;
                }


                // =================================================
                // SEPARADORES
                // =================================================

                if (
                    c == ' ' ||
                    c == '(' ||
                    c == ')' ||
                    c == '>' ||
                    c == ',' ||
                    c == '.'
                ) {


                    // =================================================
                    // PROCESAR BUFFER
                    // =================================================

                    if (!buf.empty()) {

                        // -----------------------------------------
                        // FN
                        // -----------------------------------------

                        if (buf == "fn") {

                            tokens.push_back(
                                token(
                                    TokenTypes::FnToken,
                                    bufferPos,
                                    buf
                                )
                            );

                            inFN = true;
                        }


                        // -----------------------------------------
                        // CONSOLE
                        // -----------------------------------------

                        else if (buf == "Console") {

                            tokens.push_back(
                                token(
                                    TokenTypes::ConsoleToken,
                                    bufferPos,
                                    buf
                                )
                            );
                        }


                        // -----------------------------------------
                        // LOG
                        // -----------------------------------------

                        else if (buf == "Log") {

                            tokens.push_back(
                                token(
                                    TokenTypes::LogToken,
                                    bufferPos,
                                    buf
                                )
                            );
                        }


                        // -----------------------------------------
                        // NOMBRE DE FUNCIÓN
                        // -----------------------------------------

                        else if (inFN) {

                            tokens.push_back(
                                token(
                                    TokenTypes::FnameToken,
                                    bufferPos,
                                    buf
                                )
                            );

                            inFN = false;
                        }


                        // -----------------------------------------
                        // ARGUMENTO
                        // -----------------------------------------

                        else if (inArg) {

                            tokens.push_back(
                                token(
                                    TokenTypes::VarArgToken,
                                    bufferPos,
                                    buf
                                )
                            );
                        }


                        buf = "";

                        hasBuffer = false;
                    }


                    // =================================================
                    // ESPACIO
                    // =================================================

                    if (c == ' ') {

                        // Guardamos la posición del PRIMER espacio
                        pos spacePos = currentPos;


                        tokens.push_back(
                            token(
                                TokenTypes::spaceToken,
                                currentPos,
                                " "
                            )
                        );

                        spaceAcum++;


                        // =================================================
                        // 4 ESPACIOS = INDENT
                        // =================================================

                        if (spaceAcum == 4) {

                            // Eliminamos exactamente los 4 spaces
                            for (int i = 0; i < 4; i++) {

                                tokens.pop_back();
                            }


                            // Los reemplazamos por indent
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


                    // =================================================
                    // (
                    // =================================================

                    if (c == '(') {

                        tokens.push_back(
                            token(
                                TokenTypes::OpenparenthesisToken,
                                currentPos,
                                "("
                            )
                        );

                        inArg = true;
                    }


                    // =================================================
                    // )
                    // =================================================

                    if (c == ')') {

                        tokens.push_back(
                            token(
                                TokenTypes::CloseparenthesisToken,
                                currentPos,
                                ")"
                            )
                        );

                        inArg = false;
                    }


                    // =================================================
                    // >
                    // =================================================

                    if (c == '>') {

                        tokens.push_back(
                            token(
                                TokenTypes::BiggerthanToken,
                                currentPos,
                                ">"
                            )
                        );

                        // La siguiente línea debe tener indentación
                        defFN = true;
                    }


                    // =================================================
                    // ,
                    // =================================================

                    if (c == ',') {

                        tokens.push_back(
                            token(
                                TokenTypes::colonToken,
                                currentPos,
                                ","
                            )
                        );
                    }


                    // =================================================
                    // .
                    // =================================================

                    if (c == '.') {

                        tokens.push_back(
                            token(
                                TokenTypes::DotToken,
                                currentPos,
                                "."
                            )
                        );
                    }


                    currentPos.NextChara();

                    continue;
                }


                // =================================================
                // CARÁCTER NORMAL
                // =================================================

                if (buf.empty()) {

                    // Guardamos dónde comenzó el buffer
                    bufferPos = currentPos;

                    hasBuffer = true;
                }

                buf.push_back(c);

                currentPos.NextChara();
            }


            // =================================================
            // PROCESAR BUFFER AL FINAL
            // =================================================

            if (!buf.empty()) {

                // -----------------------------------------
                // FN
                // -----------------------------------------

                if (buf == "fn") {

                    tokens.push_back(
                        token(
                            TokenTypes::FnToken,
                            bufferPos,
                            buf
                        )
                    );
                }


                // -----------------------------------------
                // CONSOLE
                // -----------------------------------------

                else if (buf == "Console") {

                    tokens.push_back(
                        token(
                            TokenTypes::ConsoleToken,
                            bufferPos,
                            buf
                        )
                    );
                }


                // -----------------------------------------
                // LOG
                // -----------------------------------------

                else if (buf == "Log") {

                    tokens.push_back(
                        token(
                            TokenTypes::LogToken,
                            bufferPos,
                            buf
                        )
                    );
                }


                // -----------------------------------------
                // NOMBRE DE FUNCIÓN
                // -----------------------------------------

                else if (inFN) {

                    tokens.push_back(
                        token(
                            TokenTypes::FnameToken,
                            bufferPos,
                            buf
                        )
                    );

                    inFN = false;
                }


                // -----------------------------------------
                // ARGUMENTO
                // -----------------------------------------

                else if (inArg) {

                    tokens.push_back(
                        token(
                            TokenTypes::VarArgToken,
                            bufferPos,
                            buf
                        )
                    );
                }
            }


            // =================================================
            // SIGUIENTE LÍNEA
            // =================================================

            currentPos.NextLine();
        }


        return tokens;
    }
};


// ============================================================
// MAIN
// ============================================================

int main(int argc, char* argv[]) {

    if (argc < 2) {

        cout << "Missing Arguments\n";

        return 1;
    }


    lexer lex(argv[1]);


    vector<token> tokens = lex.parse();


    for (token invtoken : tokens) {

        cout << invtoken.GetAll();

        cout << "\n\n";
    }


    return 0;
}
