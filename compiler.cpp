// HBPL Compiler v0.8 - bootstrap compiler
// Compiles HBPL -> C++17 -> native executable.
//
// Build:
//   g++ -std=c++17 compiler.cpp -o compiler
//
// Use:
//   compiler archivo.hbpl -o programa.exe
//   compiler archivo.hbpl --emit-cpp archivo.cpp
//
// Novedades v0.8:
//   - decoradores:            @app.get("/")  (y @manual dentro de funciones)
//   - variables globales:     const server app = http.init()
//   - import ... using ...:   import Network using http, http.types
//   - tipos server/request/response (puentes a core/http)
//   - modulo path (puente a core/path)
//   - los strings ahora siempre se emiten como std::string("...")
//   - el compilador enlaza automaticamente core/http/http.cpp y core/path/path.cpp
//
// Supported:
//   - end blocks, // comments, primitives + const, expressions/operators
//   - functions + return, if / else if / else, while, simplified for, foreach
//   - arrays and list<T>, class, visibility, me, onCreated, new
//   - Console.Log/Write/ReadLine/Clear/ReadKey
//   - basic try/catch

#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <map>
#include <memory>
#include <optional>
#include <sstream>
#include <stdexcept>
#include <string>
#include <system_error>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

using namespace std;

// ------------------------------------------------------------
// Diagnostics
// ------------------------------------------------------------

struct SourcePos {
    int line = 1;
    int col = 1;
};

[[noreturn]] static void fail(const SourcePos& p, const string& msg) {
    throw runtime_error("HBPL Error: linea " + to_string(p.line) +
                        ", columna " + to_string(p.col) + ": " + msg);
}

// ------------------------------------------------------------
// Lexer
// ------------------------------------------------------------

enum class TK {
    End, Ident, Number, String,

    KwFn, KwMain, KwFinish, KwEnd, KwReturn,
    KwIf, KwElse, KwWhile, KwFor, KwForeach, KwIn,
    KwClass, KwType, KwEnum, KwConst, KwNew,
    KwOverride, KwTry, KwCatch, KwImport, KwCodeSpace,
    KwBreak, KwContinue,
    KwTrue, KwFalse, KwNull,
    KwPrivate, KwPublic, KwProtected,
    KwOnCreated,

    TypeStr, TypeChar, TypeShort, TypeInt, TypeLong, TypeLongLong,
    TypeUChar, TypeUShort, TypeUInt, TypeULong, TypeULongLong,
    TypeFloat, TypeDouble, TypeBool,

    LParen, RParen, LBracket, RBracket, LBrace, RBrace,
    Comma, Dot, Colon, Semicolon, Arrow, GreaterBlock,
    Plus, Minus, Star, Slash, Percent,
    Equal, EqualEqual, Bang, BangEqual,
    Less, LessEqual, Greater, GreaterEqual,
    AndAnd, OrOr,
    At
};

struct Token {
    TK kind;
    string text;
    SourcePos pos;
};

static bool isTypeName(TK k) {
    switch (k) {
        case TK::TypeStr: case TK::TypeChar: case TK::TypeShort:
        case TK::TypeInt: case TK::TypeLong: case TK::TypeLongLong:
        case TK::TypeUChar: case TK::TypeUShort: case TK::TypeUInt:
        case TK::TypeULong: case TK::TypeULongLong:
        case TK::TypeFloat: case TK::TypeDouble: case TK::TypeBool:
            return true;
        default: return false;
    }
}

class Lexer {
    string s;
    size_t i = 0;
    SourcePos p;

    char peek(size_t n = 0) const {
        return i + n < s.size() ? s[i + n] : '\0';
    }

    char get() {
        char c = peek();
        if (c) {
            ++i;
            if (c == '\n') { ++p.line; p.col = 1; }
            else ++p.col;
        }
        return c;
    }

    void skipSpace() {
        while (true) {
            while (isspace((unsigned char)peek())) get();
            if (peek() == '/' && peek(1) == '/') {
                while (peek() && peek() != '\n') get();
                continue;
            }
            break;
        }
    }

public:
    explicit Lexer(string src) : s(move(src)) {}

    vector<Token> run() {
        vector<Token> out;
        while (true) {
            skipSpace();
            SourcePos at = p;
            char c = peek();

            if (!c) {
                out.push_back({TK::End, "", at});
                break;
            }

            if (isalpha((unsigned char)c) || c == '_') {
                string x;
                while (isalnum((unsigned char)peek()) || peek() == '_') x += get();

                static const unordered_map<string, TK> kw = {
                    {"fn",TK::KwFn},{"main",TK::KwMain},{"finish",TK::KwFinish},{"end",TK::KwEnd},
                    {"return",TK::KwReturn},{"if",TK::KwIf},{"else",TK::KwElse},
                    {"while",TK::KwWhile},{"for",TK::KwFor},{"foreach",TK::KwForeach},
                    {"in",TK::KwIn},{"class",TK::KwClass},{"type",TK::KwType},
                    {"enum",TK::KwEnum},{"const",TK::KwConst},{"new",TK::KwNew},
                    {"override",TK::KwOverride},{"try",TK::KwTry},{"catch",TK::KwCatch},
                    {"import",TK::KwImport},{"codeSpace",TK::KwCodeSpace},
                    {"break",TK::KwBreak},{"continue",TK::KwContinue},
                    {"true",TK::KwTrue},{"false",TK::KwFalse},{"null",TK::KwNull},
                    {"private",TK::KwPrivate},{"public",TK::KwPublic},
                    {"protected",TK::KwProtected},{"onCreated",TK::KwOnCreated},
                    {"str",TK::TypeStr},{"char",TK::TypeChar},{"short",TK::TypeShort},
                    {"int",TK::TypeInt},{"long",TK::TypeLong},{"longlong",TK::TypeLongLong},
                    {"uchar",TK::TypeUChar},{"ushort",TK::TypeUShort},{"uint",TK::TypeUInt},
                    {"ulong",TK::TypeULong},{"ulonglong",TK::TypeULongLong},
                    {"float",TK::TypeFloat},{"double",TK::TypeDouble},{"bool",TK::TypeBool}
                };

                auto it = kw.find(x);
                out.push_back({it == kw.end() ? TK::Ident : it->second, x, at});
                continue;
            }

            if (isdigit((unsigned char)c)) {
                string x;
                while (isdigit((unsigned char)peek()) || peek() == '.') x += get();
                out.push_back({TK::Number, x, at});
                continue;
            }

            if (c == '"') {
                get();
                string x;
                while (peek() && peek() != '"') {
                    char q = get();
                    if (q == '\\' && peek()) {
                        char n = get();
                        switch (n) {
                            case 'n': x += '\n'; break;
                            case 't': x += '\t'; break;
                            case 'r': x += '\r'; break;
                            case '\\': x += '\\'; break;
                            case '"': x += '"'; break;
                            default: x += n; break;
                        }
                    } else x += q;
                }
                if (peek() != '"') fail(at, "cadena sin cerrar");
                get();
                out.push_back({TK::String, x, at});
                continue;
            }

            auto two = string() + c + peek(1);
            if (two == "->") { get(); get(); out.push_back({TK::Arrow,two,at}); continue; }
            if (two == "==") { get(); get(); out.push_back({TK::EqualEqual,two,at}); continue; }
            if (two == "!=") { get(); get(); out.push_back({TK::BangEqual,two,at}); continue; }
            if (two == "<=") { get(); get(); out.push_back({TK::LessEqual,two,at}); continue; }
            if (two == ">=") { get(); get(); out.push_back({TK::GreaterEqual,two,at}); continue; }
            if (two == "&&") { get(); get(); out.push_back({TK::AndAnd,two,at}); continue; }
            if (two == "||") { get(); get(); out.push_back({TK::OrOr,two,at}); continue; }

            get();
            switch (c) {
                case '(': out.push_back({TK::LParen,"(",at}); break;
                case ')': out.push_back({TK::RParen,")",at}); break;
                case '[': out.push_back({TK::LBracket,"[",at}); break;
                case ']': out.push_back({TK::RBracket,"]",at}); break;
                case '{': out.push_back({TK::LBrace,"{",at}); break;
                case '}': out.push_back({TK::RBrace,"}",at}); break;
                case ',': out.push_back({TK::Comma,",",at}); break;
                case '.': out.push_back({TK::Dot,".",at}); break;
                case ':': out.push_back({TK::Colon,":",at}); break;
                case ';': out.push_back({TK::Semicolon,";",at}); break;
                case '>': out.push_back({TK::GreaterBlock,">",at}); break;
                case '+': out.push_back({TK::Plus,"+",at}); break;
                case '-': out.push_back({TK::Minus,"-",at}); break;
                case '*': out.push_back({TK::Star,"*",at}); break;
                case '/': out.push_back({TK::Slash,"/",at}); break;
                case '%': out.push_back({TK::Percent,"%",at}); break;
                case '=': out.push_back({TK::Equal,"=",at}); break;
                case '!': out.push_back({TK::Bang,"!",at}); break;
                case '<': out.push_back({TK::Less,"<",at}); break;
                case '@': out.push_back({TK::At,"@",at}); break;
                default: fail(at, string("caracter inesperado '") + c + "'");
            }
        }
        return out;
    }
};

// ------------------------------------------------------------
// AST
// ------------------------------------------------------------

struct Expr {
    enum Kind { Literal, Variable, Binary, Unary, Call, Member, NewObject, Array } kind;
    SourcePos pos;
    string value;
    bool isString = false;
    vector<shared_ptr<Expr>> args;
    shared_ptr<Expr> left, right;
};

struct Stmt {
    enum Kind {
        Block, VarDecl, Assign, ExprStmt, Return,
        If, While, For, Foreach, TryCatch, Console,
        Break, Continue
    } kind;
    SourcePos pos;
    string type, name, op, catchName;
    bool isConst = false;
    vector<shared_ptr<Stmt>> body, elseBody, catchBody;
    vector<shared_ptr<Expr>> args;
    shared_ptr<Expr> expr, iterable;
};

struct Param { string type, name; };

struct Function {
    string name;
    vector<Param> params;
    string returnType = "int";
    vector<shared_ptr<Stmt>> body;
    bool isMethod = false;
    string owner;
    bool isOverride = false;
    vector<shared_ptr<Expr>> decorators;   // @app.get("/")
};

struct Field {
    string type, name;
    string visibility = "private";
};

struct ClassDef {
    string name;
    string base;
    vector<Field> fields;
    vector<Function> methods;
};

struct Program {
    vector<string> imports;
    vector<Function> functions;
    vector<ClassDef> classes;
    vector<shared_ptr<Stmt>> globals;      // variables globales
    string codeSpace;
};

static bool hasImport(const Program& p, const string& needle) {
    for (const auto& i : p.imports)
        if (i.find(needle) != string::npos) return true;
    return false;
}

// ------------------------------------------------------------
// Parser
// ------------------------------------------------------------

class Parser {
    vector<Token> t;
    size_t i = 0;

    const Token& cur() const { return t[i]; }
    bool is(TK k) const { return cur().kind == k; }

    Token take() {
        Token x = cur();
        if (x.kind != TK::End) ++i;
        return x;
    }

    bool eat(TK k) {
        if (!is(k)) return false;
        ++i;
        return true;
    }

    Token expect(TK k, const string& msg) {
        if (!is(k)) fail(cur().pos, msg);
        return take();
    }

    string parseType() {
        if (isTypeName(cur().kind)) {
            string x = take().text;
            if (eat(TK::LBracket)) {
                expect(TK::RBracket, "se esperaba ']'");
                x += "[]";
            }
            return x;
        }
        if (is(TK::Ident)) {
            string x = take().text;
            // list<T>
            if (x == "list" && eat(TK::Less)) {
                string inner = parseType();
                expect(TK::GreaterBlock, "se esperaba '>' en list<T>");
                return "list<" + inner + ">";
            }
            return x;
        }
        fail(cur().pos, "se esperaba un tipo");
    }

    shared_ptr<Expr> make(Expr::Kind k, SourcePos p, string v = "") {
        auto e = make_shared<Expr>();
        e->kind = k; e->pos = p; e->value = move(v);
        return e;
    }

    shared_ptr<Expr> primary() {
        Token x = cur();

        if (eat(TK::LParen)) {
            auto e = expression();
            expect(TK::RParen, "se esperaba ')'");
            return e;
        }

        if (is(TK::String) || is(TK::Number)) {
            take();
            auto e = make(Expr::Literal, x.pos, x.text);
            e->isString = (x.kind == TK::String);
            return e;
        }

        if (is(TK::KwTrue) || is(TK::KwFalse) || is(TK::KwNull)) {
            take();
            return make(Expr::Literal, x.pos, x.text);
        }

        if (eat(TK::KwNew)) {
            Token name = expect(TK::Ident, "se esperaba nombre después de new");
            auto e = make(Expr::NewObject, x.pos, name.text);
            expect(TK::LParen, "se esperaba '(' en new");
            if (!is(TK::RParen)) {
                do { e->args.push_back(expression()); } while (eat(TK::Comma));
            }
            expect(TK::RParen, "se esperaba ')'");
            return e;
        }

        if (is(TK::Ident) || is(TK::KwOnCreated) || isTypeName(cur().kind)) {
            take();
            auto e = make(Expr::Variable, x.pos, x.text);

            while (true) {
                if (eat(TK::Dot)) {
                    // tras '.', cualquier palabra vale (res.type, req.in, ...)
                    Token m = take();
                    if (m.text.empty() || !(isalpha((unsigned char)m.text[0]) || m.text[0] == '_'))
                        fail(m.pos, "se esperaba nombre después de '.'");
                    auto n = make(Expr::Member, m.pos, m.text);
                    n->left = e;
                    e = n;
                } else if (eat(TK::LParen)) {
                    auto n = make(Expr::Call, x.pos);
                    n->left = e;
                    if (!is(TK::RParen)) {
                        do { n->args.push_back(expression()); } while (eat(TK::Comma));
                    }
                    expect(TK::RParen, "se esperaba ')'");
                    e = n;
                } else if (eat(TK::LBracket)) {
                    auto n = make(Expr::Member, x.pos, "[]");
                    n->left = e;
                    n->args.push_back(expression());
                    expect(TK::RBracket, "se esperaba ']'");
                    e = n;
                } else break;
            }
            return e;
        }

        fail(x.pos, "se esperaba una expresión");
    }

    shared_ptr<Expr> unary() {
        if (is(TK::Bang) || is(TK::Minus) || is(TK::Plus)) {
            Token x = take();
            auto e = make(Expr::Unary, x.pos, x.text);
            e->right = unary();
            return e;
        }
        return primary();
    }

    shared_ptr<Expr> binaryPrec(int minPrec) {
        auto lhs = unary();

        auto prec = [](TK k) {
            switch (k) {
                case TK::OrOr: return 1;
                case TK::AndAnd: return 2;
                case TK::EqualEqual: case TK::BangEqual: return 3;
                case TK::Less: case TK::LessEqual: case TK::Greater: case TK::GreaterEqual: return 4;
                case TK::Plus: case TK::Minus: return 5;
                case TK::Star: case TK::Slash: case TK::Percent: return 6;
                default: return -1;
            }
        };

        while (true) {
            int p = prec(cur().kind);
            if (p < minPrec) break;
            Token op = take();
            auto rhs = binaryPrec(p + 1);
            auto e = make(Expr::Binary, op.pos, op.text);
            e->left = lhs; e->right = rhs; lhs = e;
        }
        return lhs;
    }

    shared_ptr<Expr> expression() { return binaryPrec(1); }

    bool startsType() const {
        return isTypeName(cur().kind) ||
               (is(TK::Ident) && (i + 1 < t.size()) &&
                (t[i + 1].kind == TK::Ident || t[i + 1].kind == TK::LBracket));
    }

    shared_ptr<Stmt> statement();

    vector<shared_ptr<Stmt>> block();

    shared_ptr<Stmt> variableDecl(bool allowConst = true) {
        auto s = make_shared<Stmt>();
        s->kind = Stmt::VarDecl; s->pos = cur().pos;
        if (allowConst && eat(TK::KwConst)) s->isConst = true;
        s->type = parseType();
        Token n = expect(TK::Ident, "se esperaba nombre de variable");
        s->name = n.text;
        if (eat(TK::Equal)) s->expr = expression();
        else s->expr = nullptr;
        return s;
    }

    Function parseFunction(string owner = "", bool overrideFlag = false) {
        Function f;
        f.returnType = "";
        f.owner = owner;
        f.isMethod = !owner.empty();
        f.isOverride = overrideFlag;
        if (!overrideFlag) expect(TK::KwFn, "se esperaba fn");
        Token name = take();
        if (name.kind != TK::Ident && name.kind != TK::KwMain && name.kind != TK::KwFinish &&
            name.kind != TK::KwOnCreated)
            fail(name.pos, "se esperaba nombre de función");
        f.name = name.text;

        expect(TK::LParen, "se esperaba '('");
        if (!is(TK::RParen)) {
            do {
                // me is an implicit parameter
                if (is(TK::Ident) && cur().text == "me") {
                    take();
                    f.params.push_back({"me","me"});
                } else if (isTypeName(cur().kind) || (is(TK::Ident) && i + 1 < t.size() && t[i+1].kind == TK::Ident)) {
                    string ty = parseType();
                    Token pn = expect(TK::Ident, "se esperaba nombre del parámetro");
                    f.params.push_back({ty,pn.text});
                } else {
                    Token pn = expect(TK::Ident, "se esperaba nombre del parámetro");
                    f.params.push_back({"auto",pn.text});
                }
            } while (eat(TK::Comma));
        }
        expect(TK::RParen, "se esperaba ')'");
        if (eat(TK::Arrow)) f.returnType = parseType();
        expect(TK::GreaterBlock, "se esperaba '>' después de la firma");

        f.body = block();
        expect(TK::KwEnd, "se esperaba end");
        return f;
    }

    ClassDef parseClass() {
        expect(TK::KwClass, "se esperaba class");
        Token n = expect(TK::Ident, "se esperaba nombre de clase");
        ClassDef c; c.name = n.text;
        expect(TK::GreaterBlock, "se esperaba '>'");

        string visibility = "private";

        while (!is(TK::KwEnd) && !is(TK::End)) {
            if (is(TK::LBracket)) {
                take();
                Token v = take();
                if (v.kind != TK::Ident &&
                    v.kind != TK::KwPrivate &&
                    v.kind != TK::KwPublic &&
                    v.kind != TK::KwProtected) {
                    fail(v.pos, "se esperaba private/public/protected");
                }
                if (v.text != "private" && v.text != "public" && v.text != "protected")
                    fail(v.pos, "visibilidad inválida");
                visibility = v.text;
                expect(TK::RBracket, "se esperaba ']'");
                continue;
            }

            if (is(TK::KwFn) || is(TK::KwOnCreated)) {
                c.methods.push_back(parseFunction(c.name, false));
                continue;
            }

            if (is(TK::KwOverride)) {
                take();
                c.methods.push_back(parseFunction(c.name, true));
                continue;
            }

            if (is(TK::Ident) || isTypeName(cur().kind)) {
                string ty = parseType();
                Token fn = expect(TK::Ident, "se esperaba nombre de campo");
                c.fields.push_back({ty, fn.text, visibility});
                continue;
            }

            fail(cur().pos, "elemento inválido dentro de class");
        }

        expect(TK::KwEnd, "se esperaba end de class");
        return c;
    }

public:
    explicit Parser(vector<Token> toks) : t(move(toks)) {}

    Program parse() {
        Program p;
        vector<shared_ptr<Expr>> pending;   // decoradores pendientes

        while (!is(TK::End)) {

            // import a.b
            // import Network using http, http.types
            if (eat(TK::KwImport)) {
                Token n = take();
                if (n.kind != TK::Ident) fail(n.pos, "se esperaba nombre de librería");
                string name = n.text;
                while (eat(TK::Dot)) {
                    name += "." + expect(TK::Ident, "se esperaba parte del import").text;
                }

                if (is(TK::Ident) && cur().text == "using") {
                    take();
                    do {
                        string sub = expect(TK::Ident, "se esperaba nombre después de using").text;
                        while (eat(TK::Dot))
                            sub += "." + expect(TK::Ident, "se esperaba parte del import").text;
                        p.imports.push_back(name + "::" + sub);
                    } while (eat(TK::Comma));
                } else {
                    p.imports.push_back(name);
                }
                continue;
            }

            if (eat(TK::KwCodeSpace)) {
                Token n = expect(TK::Ident, "se esperaba nombre de codeSpace");
                p.codeSpace = n.text;
                expect(TK::GreaterBlock, "se esperaba '>'");
                continue;
            }

            // @decorador  /  @app.get("/")
            if (eat(TK::At)) {
                pending.push_back(expression());
                continue;
            }

            if (is(TK::KwClass)) {
                p.classes.push_back(parseClass());
                continue;
            }

            if (is(TK::KwFn) || is(TK::KwOnCreated)) {
                Function f = parseFunction();
                f.decorators = move(pending);
                pending.clear();
                p.functions.push_back(move(f));
                continue;
            }

            // variable global
            if (is(TK::KwConst) || isTypeName(cur().kind) ||
                (is(TK::Ident) && i + 1 < t.size() && t[i+1].kind == TK::Ident)) {
                p.globals.push_back(variableDecl());
                continue;
            }

            if (is(TK::KwOverride)) {
                fail(cur().pos, "override solo puede aparecer dentro de class");
            }

            if (is(TK::KwType) || is(TK::KwEnum)) {
                fail(cur().pos, "type/enum aún requieren el backend de tipos nominales");
            }

            fail(cur().pos, "declaración global no soportada");
        }

        if (!pending.empty())
            fail(cur().pos, "decorador sin función a continuación");

        return p;
    }
};

shared_ptr<Stmt> Parser::statement() {
        Token x = cur();

        // @manual (marcador dentro de funciones; sin efecto en el backend C++)
        if (eat(TK::At)) {
            expect(TK::Ident, "se esperaba nombre después de '@'");
            auto s = make_shared<Stmt>(); s->kind = Stmt::Block; s->pos = x.pos;
            return s;
        }

        if (is(TK::KwConst) || isTypeName(cur().kind) ||
            (is(TK::Ident) && i + 1 < t.size() && t[i+1].kind == TK::Ident)) {
            return variableDecl();
        }

        if (eat(TK::KwReturn)) {
            auto s = make_shared<Stmt>(); s->kind = Stmt::Return; s->pos = x.pos;
            s->expr = expression();
            return s;
        }
        if (eat(TK::KwBreak)) {
            auto s = make_shared<Stmt>(); s->kind = Stmt::Break; s->pos = x.pos;
            return s;
        }

        if (eat(TK::KwContinue)) {
            auto s = make_shared<Stmt>(); s->kind = Stmt::Continue; s->pos = x.pos;
            return s;
        }

        if (eat(TK::KwIf)) {
            auto s = make_shared<Stmt>(); s->kind = Stmt::If; s->pos = x.pos;
            s->expr = expression();
            expect(TK::GreaterBlock, "se esperaba '>'");
            s->body = block();
            if (eat(TK::KwElse)) {
                if (is(TK::KwIf)) {
                    // El if anidado consume el 'end' compartido.
                    s->elseBody.push_back(statement());
                    return s;
                }
                expect(TK::GreaterBlock, "se esperaba '>' después de else");
                s->elseBody = block();
                expect(TK::KwEnd, "se esperaba end");
                return s;
            }
            expect(TK::KwEnd, "se esperaba end");
            return s;
        }

        if (eat(TK::KwWhile)) {
            auto s = make_shared<Stmt>(); s->kind = Stmt::While; s->pos = x.pos;
            s->expr = expression();
            expect(TK::GreaterBlock, "se esperaba '>'");
            s->body = block();
            expect(TK::KwEnd, "se esperaba end");
            return s;
        }

        if (eat(TK::KwFor)) {
            auto s = make_shared<Stmt>(); s->kind = Stmt::For; s->pos = x.pos;
            // for int i = 0; i < 10>
            if (startsType()) {
                s->body.push_back(variableDecl());
                expect(TK::Semicolon, "el for usa ';' entre inicialización y condición");
            }
            s->expr = expression();
            expect(TK::GreaterBlock, "se esperaba '>'");
            s->elseBody.clear();
            auto b = block();
            s->body.insert(s->body.end(), b.begin(), b.end());
            expect(TK::KwEnd, "se esperaba end");
            return s;
        }

        if (eat(TK::KwForeach)) {
            auto s = make_shared<Stmt>(); s->kind = Stmt::Foreach; s->pos = x.pos;
            Token n = expect(TK::Ident, "se esperaba variable de foreach");
            s->name = n.text;
            expect(TK::KwIn, "se esperaba in");
            s->iterable = expression();
            expect(TK::GreaterBlock, "se esperaba '>'");
            s->body = block();
            expect(TK::KwEnd, "se esperaba end");
            return s;
        }

        if (eat(TK::KwTry)) {
            auto s = make_shared<Stmt>(); s->kind = Stmt::TryCatch; s->pos = x.pos;
            expect(TK::GreaterBlock, "se esperaba '>'");
            s->body = block();
            expect(TK::KwCatch, "se esperaba catch");
            if (is(TK::Ident)) s->catchName = take().text;
            expect(TK::GreaterBlock, "se esperaba '>' después de catch");
            s->catchBody = block();
            expect(TK::KwEnd, "se esperaba end");
            return s;
        }

        // Assignment / expression statement
        auto e = expression();
        if (eat(TK::Equal)) {
            auto s = make_shared<Stmt>(); s->kind = Stmt::Assign; s->pos = x.pos;
            s->expr = e;
            s->args.push_back(expression());
            return s;
        }
        auto s = make_shared<Stmt>(); s->kind = Stmt::ExprStmt; s->pos = x.pos;
        s->expr = e;
        return s;
    }

vector<shared_ptr<Stmt>> Parser::block() {
    vector<shared_ptr<Stmt>> b;
    while (!is(TK::KwEnd) && !is(TK::KwElse) && !is(TK::KwCatch)) {
        b.push_back(statement());
    }
    return b;
}

// ------------------------------------------------------------
// Semantic analyzer
// ------------------------------------------------------------

struct Symbol {
    string type;
    bool isConst = false;
    bool initialized = false;
};

class Analyzer {
    Program& p;
    unordered_map<string, Function*> funcs;
    unordered_map<string, ClassDef*> classes;
    int loopDepth = 0;

    static bool numeric(const string& t) {
        static const unordered_set<string> n = {
            "char","short","int","long","longlong",
            "uchar","ushort","uint","ulong","ulonglong",
            "float","double","bool"
        };
        return n.count(t) != 0;
    }

    string exprType(shared_ptr<Expr> e, unordered_map<string,Symbol>& env) {
        if (!e) return "int";
        switch (e->kind) {
            case Expr::Literal:
                if (e->isString) return "str";
                if (e->value == "true" || e->value == "false") return "bool";
                if (e->value == "null") return "null";
                if (e->value.find('.') != string::npos) return "double";
                return "int";
            case Expr::Variable: {
                auto it = env.find(e->value);
                if (it != env.end()) return it->second.type;
                if (e->value == "me") return "me";
                // function names are valid only when called
                return "unknown";
            }
            case Expr::Unary: return exprType(e->right, env);
            case Expr::Binary: {
                string a = exprType(e->left, env), b = exprType(e->right, env);
                if (e->value == "==" || e->value == "!=" ||
                    e->value == "<" || e->value == "<=" ||
                    e->value == ">" || e->value == ">=" ||
                    e->value == "&&" || e->value == "||") return "bool";
                if (a == "unknown" || b == "unknown") return "unknown";
                if (a == "str" || b == "str") {
                    if (e->value == "+") return "str";
                    fail(e->pos, "operación inválida entre str");
                }
                if (!numeric(a) || !numeric(b))
                    fail(e->pos, "operación aritmética requiere tipos numéricos");
                return (a == "double" || b == "double") ? "double" : "int";
            }
            case Expr::Call: {
                if (e->left && e->left->kind == Expr::Member) {
                    auto m = e->left;
                    if (m->left && m->left->kind == Expr::Variable &&
                        m->left->value == "Console") {
                        if (m->value == "ReadLine") return "str";
                        return "int";
                    }
                }
                if (e->left && e->left->kind == Expr::Variable) {
                    auto it = funcs.find(e->left->value);
                    if (it != funcs.end()) return it->second->returnType;
                }
                return "unknown";
            }
            case Expr::Member: return "unknown";
            case Expr::NewObject: return e->value;
            case Expr::Array: return "array";
        }
        return "unknown";
    }

    void stmt(shared_ptr<Stmt> s, unordered_map<string,Symbol>& env) {
        switch (s->kind) {
            case Stmt::VarDecl: {
                if (env.count(s->name)) fail(s->pos, "variable redeclarada: " + s->name);
                env[s->name] = {s->type, s->isConst, s->expr != nullptr};
                if (s->expr) exprType(s->expr, env);
                break;
            }
            case Stmt::Assign: {
                if (!s->expr || s->expr->kind != Expr::Variable) {
                    // member/array assignments are accepted by backend
                } else {
                    auto it = env.find(s->expr->value);
                    if (it != env.end() && it->second.isConst)
                        fail(s->pos, "no se puede modificar const " + s->expr->value);
                }
                if (!s->args.empty()) exprType(s->args[0], env);
                break;
            }
            case Stmt::ExprStmt:
                exprType(s->expr, env); break;
            case Stmt::Return:
                exprType(s->expr, env); break;
            case Stmt::If:
                exprType(s->expr, env);
                for (auto& x : s->body) stmt(x, env);
                for (auto& x : s->elseBody) stmt(x, env);
                break;
            case Stmt::While:
                exprType(s->expr, env);
                ++loopDepth;
                for (auto& x : s->body) stmt(x, env);
                --loopDepth;
                break;
            case Stmt::For:
                ++loopDepth;
                for (auto& x : s->body) stmt(x, env);
                --loopDepth;
                break;
            case Stmt::Foreach:
                ++loopDepth;
                for (auto& x : s->body) stmt(x, env);
                --loopDepth;
                break;
            case Stmt::Break:
                if (loopDepth == 0) fail(s->pos, "break fuera de un bucle");
                break;
            case Stmt::Continue:
                if (loopDepth == 0) fail(s->pos, "continue fuera de un bucle");
                break;
            case Stmt::TryCatch:
                for (auto& x : s->body) stmt(x, env);
                for (auto& x : s->catchBody) stmt(x, env);
                break;
            default: break;
        }
    }

public:
    explicit Analyzer(Program& x) : p(x) {}

    void run() {
        for (auto& c : p.classes) classes[c.name] = &c;
        for (auto& f : p.functions) {
            if (funcs.count(f.name)) fail({1,1}, "función duplicada: " + f.name);
            funcs[f.name] = &f;
        }

        if (!funcs.count("main")) fail({1,1}, "falta fn main()");

        // variables globales
        unordered_map<string,Symbol> globalEnv;
        for (auto& g : p.globals) {
            if (globalEnv.count(g->name)) fail(g->pos, "variable global redeclarada: " + g->name);
            globalEnv[g->name] = {g->type, g->isConst, true};
            if (g->expr) exprType(g->expr, globalEnv);
        }

        for (auto& f : p.functions) {
            unordered_map<string,Symbol> env = globalEnv;
            for (auto& a : f.params) {
                // un parametro puede ocultar a una global
                env[a.name] = {a.type, false, true};
            }
            for (auto& s : f.body) stmt(s, env);
        }
    }
};

// ------------------------------------------------------------
// C++ backend
// ------------------------------------------------------------

class CppBackend {
    Program& p;
    ostream& o;
    int indent = 0;
    unordered_map<string, string> listElementTypes;
    unordered_map<string, string> localTypes;
    bool useHttp = false;
    bool usePath = false;

    string ind() const { return string(indent * 4, ' '); }

    bool isClassType(const string& t) const {
        for (const auto& c : p.classes)
            if (c.name == t) return true;
        return false;
    }

    bool hasFinish() const {
        for (const auto& f : p.functions)
            if (f.name == "finish") return true;
        return false;
    }

    string cppType(string t) {
        if (t == "void") return "void";
        if (t == "str") return "std::string";
        if (t == "char") return "char";
        if (t == "short") return "short";
        if (t == "int") return "int";
        if (t == "long") return "long";
        if (t == "longlong") return "long long";
        if (t == "uchar") return "unsigned char";
        if (t == "ushort") return "unsigned short";
        if (t == "uint") return "unsigned int";
        if (t == "ulong") return "unsigned long";
        if (t == "ulonglong") return "unsigned long long";
        if (t == "float") return "float";
        if (t == "double") return "double";
        if (t == "bool") return "bool";
        if (t == "null") return "std::nullptr_t";
        // tipos de red: handles opacos del runtime
        if (t == "server" || t == "request" || t == "response") return "void*";
        if (t.size() > 2 && t.substr(t.size()-2) == "[]")
            return "std::vector<" + cppType(t.substr(0,t.size()-2)) + ">";
        if (t == "list") return "std::vector<std::shared_ptr<void>>";
        if (t.rfind("list<",0)==0) {
            string inner = t.substr(5, t.size()-6);
            if (isClassType(inner)) return "std::vector<std::shared_ptr<" + inner + ">>";
            return "std::vector<" + cppType(inner) + ">";
        }
        if (isClassType(t)) return "std::shared_ptr<" + t + ">";
        return t;
    }

    string declaredType(const string& t, const string& name = "") {
        if (t == "list" && !name.empty()) {
            auto it = listElementTypes.find(name);
            if (it != listElementTypes.end())
                return "std::vector<std::shared_ptr<" + it->second + ">>";
        }
        return cppType(t);
    }

    string listElementFromExpr(const shared_ptr<Expr>& e) {
        if (!e) return "";
        if (e->kind == Expr::Variable) {
            auto it = localTypes.find(e->value);
            if (it != localTypes.end()) return it->second;
            return "";
        }
        if (e->kind == Expr::NewObject) return e->value == "list" ? "" : e->value;
        return "";
    }

    void collectListTypes(const vector<shared_ptr<Stmt>>& body) {
        for (auto& s : body) {
            if (!s) continue;
            if (s->kind == Stmt::VarDecl) {
                localTypes[s->name] = s->type;
            }
            if (s->kind == Stmt::VarDecl && s->type == "list") {
                // Start unknown; push() statements below will determine the element type.
                listElementTypes.emplace(s->name, "");
            }
            if (s->kind == Stmt::ExprStmt && s->expr &&
                s->expr->kind == Expr::Call && s->expr->left &&
                s->expr->left->kind == Expr::Member &&
                s->expr->left->value == "push" &&
                s->expr->left->left && s->expr->left->left->kind == Expr::Variable) {
                string listName = s->expr->left->left->value;
                for (auto& a : s->expr->args) {
                    string ty = listElementFromExpr(a);
                    if (!ty.empty() && isClassType(ty)) {
                        listElementTypes[listName] = ty;
                        break;
                    }
                }
            }
            if (s->kind == Stmt::If || s->kind == Stmt::While || s->kind == Stmt::For ||
                s->kind == Stmt::Foreach || s->kind == Stmt::TryCatch) {
                collectListTypes(s->body);
                collectListTypes(s->elseBody);
                collectListTypes(s->catchBody);
            }
        }
    }

    // Prepara los mapas de tipos para emitir el cuerpo de f:
    // globales + parametros + locales.
    void beginScope(const Function& f) {
        listElementTypes.clear();
        localTypes.clear();
        for (auto& g : p.globals) localTypes[g->name] = g->type;
        for (auto& a : f.params) localTypes[a.name] = a.type;
        collectListTypes(f.body);
    }

    static string escapeString(const string& v) {
        string z = "std::string(\"";
        for (char c : v) {
            switch (c) {
                case '\\': z += "\\\\"; break;
                case '"':  z += "\\\""; break;
                case '\n': z += "\\n"; break;
                case '\t': z += "\\t"; break;
                case '\r': z += "\\r"; break;
                default: z += c;
            }
        }
        return z + "\")";
    }

    string typeOfVar(const string& name) const {
        auto it = localTypes.find(name);
        return it == localTypes.end() ? "" : it->second;
    }

    // Llamadas a la libreria estandar de HBPL (http, path, server, response).
    // Devuelve "" si la llamada no es de libreria.
    string libraryCall(const shared_ptr<Expr>& e) {
        if (!e->left || e->left->kind != Expr::Member) return "";
        auto m = e->left;
        if (!m->left || m->left->kind != Expr::Variable) return "";

        const string& recv = m->left->value;
        const string& name = m->value;
        string rt = typeOfVar(recv);

        // modulos estaticos (solo si no hay una variable con ese nombre)
        if (rt.empty()) {
            if (recv == "http" && name == "init") {
                if (!useHttp) return "";
                return "hbpl_http_create()";
            }
            if (recv == "path" && usePath) {
                static const unordered_map<string,string> fns = {
                    {"join","path_join"},{"currentDir","path_currentDir"},
                    {"exists","path_exists"},{"isFile","path_isFile"},
                    {"isDir","path_isDir"},{"filename","path_filename"},
                    {"extension","path_extension"},{"parent","path_parent"}
                };
                auto it = fns.find(name);
                if (it != fns.end())
                    return "hbpl::" + it->second + "(" + joinArgs(e->args) + ")";
            }
            return "";
        }

        if (rt == "server" && useHttp) {
            if (name == "listen" && e->args.size() == 1)
                return "hbpl::listen(" + recv + ", " + expr(e->args[0]) + ")";
            if (name == "start") return "hbpl_http_start(" + recv + ")";
            if (name == "stop")  return "hbpl_http_stop(" + recv + ")";
            if ((name == "get" || name == "post") && e->args.size() == 2)
                return "hbpl_http_" + name + "(" + recv + ", " + expr(e->args[0]) +
                       ".c_str(), " + expr(e->args[1]) + ")";
        }

        if (rt == "request" && useHttp && e->args.empty()) {
            if (name == "body" || name == "path" || name == "method" || name == "query")
                return "hbpl::req_" + name + "(" + recv + ")";
        }

        if (rt == "response" && useHttp) {
            if (name == "type" && e->args.size() == 1)
                return "hbpl_http_response_type(" + recv + ", " + expr(e->args[0]) + ".c_str())";
            if (name == "sendfile" && e->args.size() == 1)
                return "hbpl_http_response_sendfile(" + recv + ", " + expr(e->args[0]) + ".c_str())";
            if (name == "send" && e->args.size() == 1)
                return "hbpl_http_response_send(" + recv + ", " + expr(e->args[0]) + ".c_str())";
            if (name == "status" && e->args.size() == 1)
                return "hbpl_http_response_status(" + recv + ", " + expr(e->args[0]) + ")";
        }

        return "";
    }

    string expr(shared_ptr<Expr> e) {
        if (!e) return "0";
        switch (e->kind) {
            case Expr::Literal:
                if (e->isString) return escapeString(e->value);
                if (e->value == "true") return "true";
                if (e->value == "false") return "false";
                if (e->value == "null") return "nullptr";
                return e->value;

            case Expr::Variable:
                if (e->value == "me") return "this";
                return e->value;

            case Expr::Unary:
                return "(" + e->value + expr(e->right) + ")";

            case Expr::Binary:
                return "(" + expr(e->left) + " " + e->value + " " + expr(e->right) + ")";

            case Expr::Member: {
                string base = expr(e->left);
                if (e->value == "[]") return base + "[" + expr(e->args[0]) + "]";
                if (e->value == "len") return "static_cast<int>(" + base + ".size())";
                if (e->value == "push") return base + ".push_back";
                if (e->value == "pop") return base + ".pop_back";
                if (e->value == "clear") return base + ".clear";
                if (base == "this") return "this->" + e->value;
                // propiedades de request: req.body / req.path / req.method / req.query
                if (useHttp && e->left && e->left->kind == Expr::Variable &&
                    typeOfVar(e->left->value) == "request" &&
                    (e->value == "body" || e->value == "path" ||
                     e->value == "method" || e->value == "query"))
                    return "hbpl::req_" + e->value + "(" + base + ")";
                // Class variables are emitted as shared_ptr<T>.
                if (e->left && e->left->kind == Expr::Variable) {
                    const string& n = e->left->value;
                    if (n != "Console") return base + "->" + e->value;
                }
                return base + "." + e->value;
            }

            case Expr::Call: {
                // Console
                if (e->left && e->left->kind == Expr::Member &&
                    e->left->left && e->left->left->kind == Expr::Variable &&
                    e->left->left->value == "Console") {
                    string m = e->left->value;
                    if (m == "Log") {
                        string a = e->args.empty() ? "std::string()" : expr(e->args[0]);
                        return "hbpl::log(" + a + ")";
                    }
                    if (m == "Write") {
                        string a = e->args.empty() ? "std::string()" : expr(e->args[0]);
                        return "hbpl::write(" + a + ")";
                    }
                    if (m == "ReadLine") return "hbpl::readLine()";
                    if (m == "Clear") return "hbpl::clear()";
                    if (m == "ReadKey") return "hbpl::readKey()";
                }

                // http / path / server / response
                {
                    string lib = libraryCall(e);
                    if (!lib.empty()) return lib;
                }

                // str(x) conversion
                if (e->left && e->left->kind == Expr::Variable && e->left->value == "str" && e->args.size() == 1)
                    return "hbpl::toStr(" + expr(e->args[0]) + ")";

                // list.push(a,b) becomes several push_back calls.
                if (e->left && e->left->kind == Expr::Member && e->left->value == "push") {
                    string z = "[&](){";
                    for (auto& a : e->args) z += expr(e->left->left) + ".push_back(" + expr(a) + ");";
                    z += "}()";
                    return z;
                }

                // Generic call / method call
                return expr(e->left) + "(" + joinArgs(e->args) + ")";
            }

            case Expr::NewObject:
                if (e->value == "list") return "std::vector<std::shared_ptr<void>>{}";
                return "std::make_shared<" + e->value + ">( " + joinArgs(e->args) + " )";

            case Expr::Array: {
                string z = "{";
                for (size_t i=0;i<e->args.size();++i) {
                    if (i) z += ", ";
                    z += expr(e->args[i]);
                }
                return z + "}";
            }
        }
        return "0";
    }

    string joinArgs(const vector<shared_ptr<Expr>>& a) {
        string z;
        for (size_t i=0;i<a.size();++i) {
            if (i) z += ", ";
            z += expr(a[i]);
        }
        return z;
    }

    void stmt(shared_ptr<Stmt> s) {
        switch (s->kind) {
            case Stmt::VarDecl: {
                string ty = declaredType(s->type, s->name);
                bool ptr = !ty.empty() && ty.back() == '*';
                o << ind();
                if (s->isConst && !ptr) o << "const ";
                o << ty;
                if (s->isConst && ptr) o << " const";   // puntero constante, no puntero a const
                o << " " << s->name;
                if (s->expr) {
                    if (s->type == "list" && s->expr->kind == Expr::NewObject && s->expr->value == "list")
                        o << "{}";
                    else
                        o << " = " << expr(s->expr);
                }
                o << ";\n";
                break;
            }

            case Stmt::Assign:
                o << ind() << expr(s->expr) << " = " << expr(s->args[0]) << ";\n";
                break;

            case Stmt::ExprStmt:
                o << ind() << expr(s->expr) << ";\n";
                break;

            case Stmt::Return:
                o << ind() << "return " << expr(s->expr) << ";\n";
                break;

            case Stmt::If:
                o << ind() << "if (" << expr(s->expr) << ") {\n";
                ++indent;
                for (auto& x : s->body) stmt(x);
                --indent;
                o << ind() << "}";
                if (!s->elseBody.empty()) {
                    o << " else ";
                    if (s->elseBody.size() == 1 && s->elseBody[0]->kind == Stmt::If) {
                        o << "{\n"; ++indent; stmt(s->elseBody[0]); --indent; o << ind() << "}";
                    } else {
                        o << "{\n"; ++indent;
                        for (auto& x : s->elseBody) stmt(x);
                        --indent; o << ind() << "}";
                    }
                }
                o << "\n";
                break;

            case Stmt::While:
                o << ind() << "while (" << expr(s->expr) << ") {\n";
                ++indent; for (auto& x : s->body) stmt(x); --indent;
                o << ind() << "}\n";
                break;

            case Stmt::For:
                // The HBPL simplified for has no explicit increment.
                // Infer ++ for the first declaration when possible.
                if (!s->body.empty() && s->body[0]->kind == Stmt::VarDecl) {
                    auto init = s->body[0];
                    o << ind() << "for (" << (init->isConst ? "const " : "")
                      << cppType(init->type) << " " << init->name;
                    if (init->expr) o << " = " << expr(init->expr);
                    o << "; " << expr(s->expr) << "; ++" << init->name << ") {\n";
                    ++indent;
                    for (size_t i=1;i<s->body.size();++i) stmt(s->body[i]);
                    --indent;
                    o << ind() << "}\n";
                } else {
                    o << ind() << "while (" << expr(s->expr) << ") {\n";
                    ++indent; for (auto& x : s->body) stmt(x); --indent;
                    o << ind() << "}\n";
                }
                break;

            case Stmt::Foreach:
                o << ind() << "for (auto& " << s->name << " : " << expr(s->iterable) << ") {\n";
                ++indent; for (auto& x : s->body) stmt(x); --indent;
                o << ind() << "}\n";
                break;

            case Stmt::TryCatch:
                o << ind() << "try {\n";
                ++indent; for (auto& x : s->body) stmt(x); --indent;
                o << ind() << "} catch (const std::exception& " << (s->catchName.empty() ? "err" : s->catchName) << ") {\n";
                ++indent; for (auto& x : s->catchBody) stmt(x); --indent;
                o << ind() << "}\n";
                break;

            case Stmt::Break:
                o << ind() << "break;\n";
                break;

            case Stmt::Continue:
                o << ind() << "continue;\n";
                break;

            default: break;   // Block (@manual) no emite nada
        }
    }

    string inferredParamType(const Function& f, const string& owner, const Param& param) const {
        if (param.type != "auto") return param.type;
        // Infer constructor parameters from assignments such as:
        // me.nombre = nombre
        if (f.name == "onCreated") {
            for (const auto& st : f.body) {
                if (st->kind != Stmt::Assign || !st->expr || st->expr->kind != Expr::Member) continue;
                if (!st->expr->left || st->expr->left->kind != Expr::Variable || st->expr->left->value != "me") continue;
                if (st->args.empty() || st->args[0]->kind != Expr::Variable || st->args[0]->value != param.name) continue;
                for (const auto& c : p.classes) if (c.name == owner)
                    for (const auto& field : c.fields) if (field.name == st->expr->value) return field.type;
            }
        }
        return "std::string";
    }

    void method(const Function& f, const string& owner) {
        beginScope(f);
        if (f.name == "onCreated") o << owner << "::" << owner << "(";
        else o << cppType(f.returnType.empty() ? "void" : f.returnType) << " " << owner << "::" << f.name << "(";
        bool first = true;
        for (auto& a : f.params) {
            if (a.name == "me") continue;
            if (!first) o << ", ";
            first = false;
            o << cppType(inferredParamType(f, owner, a)) << " " << a.name;
        }
        o << ") {\n";
        ++indent;
        for (auto& s : f.body) stmt(s);
        --indent;
        o << "}\n\n";
    }

    string functionSignature(const Function& f) {
        string fn = (f.name == "main") ? "hbpl_main" : f.name;
        string ret = cppType((f.returnType.empty() && f.name != "main") ? "void"
                             : (f.returnType.empty() ? "int" : f.returnType));
        string z = ret + " " + fn + "(";
        for (size_t i=0;i<f.params.size();++i) {
            if (i) z += ", ";
            z += cppType(f.params[i].type) + " " + f.params[i].name;
        }
        return z + ")";
    }

    // Registra las rutas declaradas con decoradores:
    //   @app.get("/")  ->  hbpl_http_get(app, "/", home);
    void emitRoutes() {
        for (auto& f : p.functions) {
            for (auto& d : f.decorators) {
                if (!d || d->kind != Expr::Call || !d->left || d->left->kind != Expr::Member ||
                    !d->left->left || d->left->left->kind != Expr::Variable)
                    continue;

                const string& target = d->left->left->value;
                const string& verb = d->left->value;

                if (typeOfVar(target) != "server")
                    fail(d->pos, "el decorador debe aplicarse sobre una variable de tipo server: " + target);
                if ((verb != "get" && verb != "post") || d->args.size() != 1)
                    fail(d->pos, "decorador no soportado: @" + target + "." + verb + " (use get/post con una ruta)");

                o << ind() << "hbpl_http_" << verb << "(" << target << ", "
                  << expr(d->args[0]) << ".c_str(), " << f.name << ");\n";
            }
        }
    }

public:
    CppBackend(Program& x, ostream& out) : p(x), o(out) {
        useHttp = hasImport(p, "http");
        usePath = hasImport(p, "path");
    }

    void emit() {
        o << "#include <iostream>\n"
             "#include <string>\n"
             "#include <vector>\n"
             "#include <memory>\n"
             "#include <stdexcept>\n"
             "#include <cstdlib>\n"
             "#include <cmath>\n"
             "#include <limits>\n\n";

        // ---- puentes al core de HBPL ----
        if (useHttp) {
            o << "extern \"C\" {\n"
                 "void* hbpl_http_create();\n"
                 "bool hbpl_http_listen(void* server, int port);\n"
                 "void hbpl_http_start(void* server);\n"
                 "void hbpl_http_stop(void* server);\n"
                 "void hbpl_http_get(void* server, const char* path, void (*handler)(void*, void*));\n"
                 "void hbpl_http_post(void* server, const char* path, void (*handler)(void*, void*));\n"
                 "char* hbpl_http_request_method(void* request);\n"
                 "char* hbpl_http_request_path(void* request);\n"
                 "char* hbpl_http_request_query(void* request);\n"
                 "char* hbpl_http_request_body(void* request);\n"
                 "void hbpl_http_response_type(void* response, const char* type);\n"
                 "void hbpl_http_response_status(void* response, int status);\n"
                 "void hbpl_http_response_send(void* response, const char* data);\n"
                 "void hbpl_http_response_sendfile(void* response, const char* path);\n"
                 "}\n\n";
        }
        if (usePath) {
            o << "extern \"C\" {\n"
                 "char* hbpl_path_current_dir();\n"
                 "char* hbpl_path_join(const char* a, const char* b);\n"
                 "bool hbpl_path_exists(const char* path);\n"
                 "bool hbpl_path_is_file(const char* path);\n"
                 "bool hbpl_path_is_dir(const char* path);\n"
                 "char* hbpl_path_filename(const char* path);\n"
                 "char* hbpl_path_extension(const char* path);\n"
                 "char* hbpl_path_parent(const char* path);\n"
                 "}\n\n";
        }

        o << "namespace hbpl {\n"
             "template<class T> void log(const T& x){ std::cout << x << '\\n'; }\n"
             "inline void log(const std::string& x){ std::cout << x << '\\n'; }\n"
             "template<class T> void write(const T& x){ std::cout << x; std::cout.flush(); }\n"
             "inline std::string readLine(){ std::string x; std::getline(std::cin,x); return x; }\n"
             "inline void clear(){\n"
             "#ifdef _WIN32\n"
             "std::system(\"cls\");\n"
             "#else\n"
             "std::system(\"clear\");\n"
             "#endif\n"
             "}\n"
             "inline char readKey(){ char c=0; std::cin.get(c); return c; }\n"
             "inline std::string toStr(const std::string& x){ return x; }\n"
             "inline std::string toStr(const char* x){ return std::string(x); }\n"
             "template<class T> std::string toStr(const T& x){ return std::to_string(x); }\n";

        if (usePath || useHttp)
            o << "inline std::string takeStr(char* s){ std::string r = s ? s : \"\"; std::free(s); return r; }\n";
        if (useHttp) {
            o << "inline std::string req_method(void* r){ return takeStr(hbpl_http_request_method(r)); }\n"
                 "inline std::string req_path(void* r){ return takeStr(hbpl_http_request_path(r)); }\n"
                 "inline std::string req_query(void* r){ return takeStr(hbpl_http_request_query(r)); }\n"
                 "inline std::string req_body(void* r){ return takeStr(hbpl_http_request_body(r)); }\n";
        }
        if (usePath) {
            o << "inline std::string path_currentDir(){ return takeStr(hbpl_path_current_dir()); }\n"
                 // quita los '/' iniciales de b para que join no descarte a
                 "inline std::string path_join(const std::string& a, const std::string& b){\n"
                 "    size_t k = 0;\n"
                 "    while (k < b.size() && (b[k] == '/' || b[k] == '\\\\')) ++k;\n"
                 "    return takeStr(hbpl_path_join(a.c_str(), b.substr(k).c_str()));\n"
                 "}\n"
                 "inline bool path_exists(const std::string& a){ return hbpl_path_exists(a.c_str()); }\n"
                 "inline bool path_isFile(const std::string& a){ return hbpl_path_is_file(a.c_str()); }\n"
                 "inline bool path_isDir(const std::string& a){ return hbpl_path_is_dir(a.c_str()); }\n"
                 "inline std::string path_filename(const std::string& a){ return takeStr(hbpl_path_filename(a.c_str())); }\n"
                 "inline std::string path_extension(const std::string& a){ return takeStr(hbpl_path_extension(a.c_str())); }\n"
                 "inline std::string path_parent(const std::string& a){ return takeStr(hbpl_path_parent(a.c_str())); }\n";
        }
        if (useHttp) {
            o << "inline void listen(void* s, int port){\n"
                 "    if (!hbpl_http_listen(s, port)) {\n"
                 "        std::cerr << \"HBPL Error: no se pudo escuchar en el puerto \" << port << \"\\n\";\n"
                 "        std::exit(1);\n"
                 "    }\n"
                 "    std::cout << \"HBPL: servidor escuchando en http://localhost:\" << port << \"\\n\";\n"
                 "}\n";
        }
        o << "}\n\n";

        // ---- clases ----
        for (auto& c : p.classes) {
            o << "struct " << c.name;
            if (!c.base.empty()) o << " : public " << c.base;
            o << " {\n";
            ++indent;
            for (auto& f : c.fields) {
                o << ind() << cppType(f.type) << " " << f.name << ";\n";
            }
            for (auto& f : c.methods) {
                if (f.name == "onCreated") {
                    o << ind() << c.name << "(";
                } else {
                    o << ind() << cppType(f.returnType.empty() ? "void" : f.returnType) << " " << f.name << "(";
                }
                bool first=true;
                for (auto& a : f.params) {
                    if (a.name=="me") continue;
                    if (!first) o << ", ";
                    first=false;
                    o << cppType(inferredParamType(f, c.name, a)) << " " << a.name;
                }
                o << ");\n";
            }
            --indent;
            o << "};\n\n";
        }

        // ---- prototipos de funciones globales ----
        for (auto& f : p.functions)
            o << functionSignature(f) << ";\n";
        if (!hasFinish()) o << "inline void finish() {}\n";
        o << "\n";

        // ---- variables globales ----
        if (!p.globals.empty()) {
            Function none;
            beginScope(none);
            for (auto& g : p.globals) stmt(g);
            o << "\n";
        }

        // ---- metodos ----
        for (auto& c : p.classes)
            for (auto& f : c.methods)
                method(f, c.name);

        // ---- funciones globales ----
        for (auto& f : p.functions) {
            beginScope(f);
            o << functionSignature(f) << " {\n";
            ++indent;
            if (f.name == "main") emitRoutes();
            for (auto& s : f.body) stmt(s);
            if (f.name == "main") o << ind() << "return 0;\n";
            --indent;
            o << "}\n\n";
        }

        o << "int main() { int rc = hbpl_main(); finish(); return rc; }\n";
    }
};

// ------------------------------------------------------------
// Driver
// ------------------------------------------------------------

static string readFile(const string& path) {
    ifstream f(path, ios::binary);
    if (!f) throw runtime_error("no se pudo abrir: " + path);
    stringstream ss; ss << f.rdbuf();
    return ss.str();
}

static void usage() {
    cout << "HBPL Compiler v0.8\n"
         << "Uso:\n"
         << "  compiler archivo.hbpl -o programa.exe\n"
         << "  compiler archivo.hbpl --emit-cpp archivo.cpp\n";
}

static void ver() {
    cout << "v0.8\n";
}

// Busca la carpeta core/ (cwd, junto al compilador, o un nivel arriba).
static filesystem::path findCore(const char* argv0) {
    error_code ec;
    vector<filesystem::path> candidates;
    candidates.push_back(filesystem::current_path(ec) / "core");
    auto exeDir = filesystem::weakly_canonical(argv0, ec).parent_path();
    candidates.push_back(exeDir / "core");
    candidates.push_back(exeDir / ".." / "core");
    for (auto& c : candidates)
        if (filesystem::is_directory(c, ec)) return c;
    return {};
}

int main(int argc, char** argv) {
    try {
        if (argc < 2) { usage(); return 1; }

        if (string(argv[1]) == "--ver") { ver(); return 0; }
        if (string(argv[1]) == "--help") { usage(); return 0; }

        string input = argv[1];
        string output = "./.bin/main.exe";
        string emitCpp;

        for (int i=2;i<argc;++i) {
            string a = argv[i];
            if (a == "-o" && i+1 < argc) output = argv[++i];
            else if (a == "--ver") { ver(); return 0; }
            else if (a == "--emit-cpp" && i+1 < argc) emitCpp = argv[++i];
            else if (a == "--help") { usage(); return 0; }
            else throw runtime_error("opción desconocida: " + a);
        }

        string source = readFile(input);
        Lexer lexer(source);
        auto tokens = lexer.run();

        Parser parser(tokens);
        Program program = parser.parse();

        Analyzer analyzer(program);
        analyzer.run();

        string cppPath = emitCpp.empty()
            ? (filesystem::temp_directory_path() / "hbpl_generated.cpp").string()
            : emitCpp;

        {
            ofstream cpp(cppPath);
            if (!cpp) throw runtime_error("no se pudo crear: " + cppPath);
            CppBackend backend(program, cpp);
            backend.emit();
        }

        bool needHttp = hasImport(program, "http");
        bool needPath = hasImport(program, "path");

        if (!emitCpp.empty()) {
            cout << "HBPL: C++ generado en " << cppPath << "\n";
            if (needHttp || needPath)
                cout << "HBPL: recuerda enlazar los archivos de core/ al compilar ese .cpp\n";
            return 0;
        }

        // fuentes del core necesarias
        vector<string> extra;
        if (needHttp || needPath) {
            auto core = findCore(argv[0]);
            if (core.empty())
                throw runtime_error("no se encontró la carpeta core/ (se busca junto al compilador y en el directorio actual)");
            if (needHttp) extra.push_back((core / "http" / "http.cpp").string());
            if (needPath) extra.push_back((core / "path" / "path.cpp").string());
        }

        // crear carpeta de salida
        error_code ec;
        auto outParent = filesystem::path(output).parent_path();
        if (!outParent.empty()) filesystem::create_directories(outParent, ec);

        string cmd = "g++ -std=c++17 \"" + cppPath + "\"";
        for (auto& s : extra) cmd += " \"" + s + "\"";
        cmd += " -o \"" + output + "\"";
        if (needHttp) {
#ifdef _WIN32
            cmd += " -lws2_32";
#else
            cmd += " -pthread";
#endif
        }

        cout << "HBPL: compilando...\n";
        int rc = std::system(cmd.c_str());
        filesystem::remove(cppPath, ec);

        if (rc != 0) {
            cerr << "HBPL Error: fallo el backend C++/linker.\n";
            return rc;
        }

        cout << "HBPL: generado " << output << "\n";
        return 0;
    } catch (const exception& e) {
        cerr << e.what() << "\n";
        return 1;
    }
}
