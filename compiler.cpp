// HBPL Compiler v0.2 - bootstrap compiler
// Compiles HBPL -> C++17 -> native executable.
// This is the first working backend; the frontend is designed to be replaced
// by a native ASM backend later without changing the language frontend.
//
// Build:
//   g++ -std=c++17 compiler.cpp -o compiler
//
// Use:
//   compiler archivo.hbpl -o programa.exe
//
// Supported in this version:
//   - end blocks
//   - // comments
//   - primitive declarations + const
//   - expressions/operators
//   - functions + return
//   - if / else if / else
//   - while
//   - simplified for
//   - foreach
//   - arrays and list<T>
//   - class, visibility, me, onCreated, new
//   - override
//   - Ext(Base)
//   - Console.Log/Write/ReadLine/Clear/ReadKey
//   - imports (validated/recorded; standard library names are accepted)
//   - basic try/catch
//
// NOTE: generated C++ is compiled to native machine code by g++.
// The next backend can replace emitCpp() with a direct ASM/IR backend.

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
    AndAnd, OrOr
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
    string codeSpace;
};

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
            return make(Expr::Literal, x.pos, x.text);
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
                    Token m = expect(TK::Ident, "se esperaba nombre después de '.'");
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
        while (!is(TK::End)) {
            if (eat(TK::KwImport)) {
                Token n = take();
                if (n.kind != TK::Ident) fail(n.pos, "se esperaba nombre de librería");
                string name = n.text;
                while (eat(TK::Dot)) {
                    name += "." + expect(TK::Ident, "se esperaba parte del import").text;
                }
                p.imports.push_back(name);
                continue;
            }

            if (eat(TK::KwCodeSpace)) {
                Token n = expect(TK::Ident, "se esperaba nombre de codeSpace");
                p.codeSpace = n.text;
                expect(TK::GreaterBlock, "se esperaba '>'");
                continue;
            }

            if (is(TK::KwClass)) {
                p.classes.push_back(parseClass());
                continue;
            }

            if (is(TK::KwFn) || is(TK::KwOnCreated)) {
                p.functions.push_back(parseFunction());
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
        return p;
    }
};

shared_ptr<Stmt> Parser::statement() {
        Token x = cur();

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
                    // No consumir el 'if': statement() lo hace.
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
            return s;}

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

        for (auto& f : p.functions) {
            unordered_map<string,Symbol> env;
            for (auto& a : f.params) {
                if (env.count(a.name)) fail({1,1}, "parámetro duplicado: " + a.name);
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

    string ind() const { return string(indent * 4, ' '); }

    bool isClassType(const string& t) const {
        for (const auto& c : p.classes)
            if (c.name == t) return true;
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

    string expr(shared_ptr<Expr> e) {
        if (!e) return "0";
        switch (e->kind) {
            case Expr::Literal:
                if (e->value == "true") return "true";
                if (e->value == "false") return "false";
                if (e->value == "null") return "nullptr";
                if (e->value.find('\n') != string::npos) {
                    string q = e->value;
                    string z = "\"";
                    for (char c : q) {
                        if (c == '"') z += "\\\"";
                        else if (c == '\n') z += "\\n";
                        else if (c == '\t') z += "\\t";
                        else z += c;
                    }
                    return z + "\"";
                }
                // String literals are represented directly with quotes.
                // Numeric literals are returned as-is.
                if (!e->value.empty() && !isdigit((unsigned char)e->value[0]) &&
                    e->value.find('.') == string::npos) {
                    return "\"" + e->value + "\"";
                }
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
                // Class variables are emitted as shared_ptr<T>.
                // Known STL/string members keep '.', other object members use '->'.
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
                        string a = e->args.empty() ? "\"\"" : expr(e->args[0]);
                        return "hbpl::log(" + a + ")";
                    }
                    if (m == "Write") {
                        string a = e->args.empty() ? "\"\"" : expr(e->args[0]);
                        return "hbpl::write(" + a + ")";
                    }
                    if (m == "ReadLine") return "hbpl::readLine()";
                    if (m == "Clear") return "hbpl::clear()";
                    if (m == "ReadKey") return "hbpl::readKey()";
                }

                // str(x) conversion
                if (e->left && e->left->kind == Expr::Variable && e->left->value == "str" && e->args.size() == 1)
                    return "std::to_string(" + expr(e->args[0]) + ")";

                // list.push(a,b) becomes two push_back calls.
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
            case Stmt::VarDecl:
                o << ind() << (s->isConst ? "const " : "")
                  << declaredType(s->type, s->name) << " " << s->name;
                if (s->expr) {
                    if (s->type == "list" && s->expr->kind == Expr::NewObject && s->expr->value == "list")
                        o << "{}";
                    else
                        o << " = " << expr(s->expr);
                }
                o << ";\n";
                break;

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
                        // Simplified nested output.
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

            default: break;
        }
    }

    string inferredParamType(const Function& f, const string& owner, const Param& param) const {
        if (param.type != "auto") return param.type;
        // Infer constructor parameters from assignments such as:
        // me.nombre = nombre
        // me.grado = grado
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

public:
    CppBackend(Program& x, ostream& out) : p(x), o(out) {}

    void emit() {
        o << "#include <iostream>\n"
             "#include <string>\n"
             "#include <vector>\n"
             "#include <memory>\n"
             "#include <stdexcept>\n"
             "#include <cstdlib>\n"
             "#include <cmath>\n"
             "#include <limits>\n\n";

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
             "}\n\n";

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

        // Global functions need declarations first.
        for (auto& f : p.functions) {
            listElementTypes.clear();
            localTypes.clear();
            collectListTypes(f.body);
            string fn = (f.name == "main") ? "hbpl_main" : f.name;
            o << cppType((f.returnType.empty() && f.name != "main") ? "void" : (f.returnType.empty() ? "int" : f.returnType)) << " " << fn << "(";
            for (size_t i=0;i<f.params.size();++i) {
                if (i) o << ", ";
                o << cppType(f.params[i].type) << " " << f.params[i].name;
            }
            o << ");\n";
        }
        o << "\n";

        for (auto& c : p.classes)
            for (auto& f : c.methods)
                method(f, c.name);

        for (auto& f : p.functions) {
            listElementTypes.clear();
            localTypes.clear();
            collectListTypes(f.body);
            string fn = (f.name == "main") ? "hbpl_main" : f.name;
            o << cppType((f.returnType.empty() && f.name != "main") ? "void" : (f.returnType.empty() ? "int" : f.returnType)) << " " << fn << "(";
            for (size_t i=0;i<f.params.size();++i) {
                if (i) o << ", ";
                o << cppType(f.params[i].type) << " " << f.params[i].name;
            }
            o << ") {\n";
            ++indent;
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
    cout << "HBPL Compiler v0.7\n"
         << "Uso:\n"
         << "  compiler archivo.hbpl -o programa.exe\n"
         << "  compiler archivo.hbpl --emit-cpp archivo.cpp\n";
}
static void ver()
{
    cout << "v0.7";

}
int main(int argc, char** argv) {
    try {
        if (argc < 2) { usage(); return 1; }

        string input = argv[1];
        string output = "./.bin/main.exe";
        string emitCpp;

        for (int i=2;i<argc;++i) {
            string a = argv[i];
            if (a == "-o" && i+1 < argc) output = argv[++i];
            else if (a == "--ver") {ver();return 0;}
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

        ofstream cpp(cppPath);
        if (!cpp) throw runtime_error("no se pudo crear: " + cppPath);

        CppBackend backend(program, cpp);
        backend.emit();
        cpp.close();

        if (!emitCpp.empty()) {
            cout << "HBPL: C++ generado en " << cppPath << "\n";
            return 0;
        }

        string cmd = "g++ -std=c++17 \"" + cppPath + "\" -o \"" + output + "\"";
        cout << "HBPL: compilando...\n";
        int rc = std::system(cmd.c_str());

        if (!emitCpp.empty()) return rc;
        filesystem::remove(cppPath);

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
