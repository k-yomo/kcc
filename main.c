#include <ctype.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

char *user_input;

typedef enum {
    TK_RESERVED, // symbol
    TK_NUM,      // number
    TK_EOF,      // end of input
} TokenKind;

typedef struct Token Token;

struct Token {
    TokenKind kind; // kind of token
    Token *next;    // next token
    int val;        // num if kind is TK_NUM
    char *str;      // token string
    int len;        // length of token
};

Token *token;

// initialize new token and set it as the next token of current token
Token *new_token(TokenKind kind, Token *cur, char *str, int len) {
    Token *tok = calloc(1, sizeof(Token));
    tok->kind = kind;
    tok->str = str;
    cur->len = len;
    cur->next = tok;
    return tok;
}

typedef enum {
    ND_ADD, // +
    ND_SUB, // -
    ND_MUL, // *
    ND_DIV, // /
    ND_EQ,  // ==
    ND_NE,  // !=
    ND_LT,  // <
    ND_LE,  // <=
    ND_NUM, // int
} NodeKind;

typedef struct Node Node;

// ast node
struct Node {
    NodeKind kind;
    Node *lhs; // left-hand side
    Node *rhs; // right-hand side
    int val; // used only if kind is 'ND_NUM'
};

Node *new_node(NodeKind kind, Node *lhs, Node *rhs) {
    Node *node = calloc(1, sizeof(Node));
    node->kind = kind;
    node->lhs = lhs;
    node->rhs = rhs;
    return node;
}

Node *new_num_node(int val) {
    Node *node = calloc(1, sizeof(Node));
    node->kind = ND_NUM;
    node->val = val;
    return node;
}

// output error with the position where the errors occurred at
void error_at(char *loc, char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);

    int pos = loc - user_input;
    fprintf(stderr, "%s\n", user_input);
    fprintf(stderr, "%*s", pos, ""); // output whitespaces to the position
    fprintf(stderr, "^ ");
    vfprintf(stderr, fmt, ap);
    fprintf(stderr, "\n");
    exit(1);
}

// check if token is expected and move to next
bool consume(char *op) {
    if (token->kind != TK_RESERVED || strlen(op) != token->len || memcmp(token->str, op, token->len))
        return false;
    token = token->next;
    return true;
}

// move to next and return true when token is expected symbol, return false if not
void expect(char *op) {
    if (token->kind != TK_RESERVED || strlen(op) != token->len || memcmp(token->str, op, token->len)) 
        error_at(token->str,"'%c' is expected, but got '%c'", op, token->str);
    token = token->next;
}

// move to next and return the number
int expect_number() {
    if (token->kind != TK_NUM)
        error_at(token->str,"'%c' is not a number", token->kind);
    int val = token->val;
    token = token->next;
    return val;
}

bool at_eof() {
    return token->kind == TK_EOF;
}

bool startswith(char *p, char *q) {
    return memcmp(p, q, strlen(q)) == 0;
}

// tokenize given string
Token *tokenize(char *p) {
    Token head;
    head.next = NULL;
    Token *cur = &head;

    while (*p) {
        if (isspace(*p)) {
            p++;
            continue;
        }

        if (startswith(p, "==") || startswith(p, "!=") || startswith(p, "<=") || startswith(p, ">=")) {
            cur = new_token(TK_RESERVED, cur, p, 2);
            p += 2;
            continue;
        }
        if (strchr("+-*/()<>", *p)) {
            cur = new_token(TK_RESERVED, cur, p++, 1);
            continue;
        }
        if (isdigit(*p)) {
            cur = new_token(TK_NUM, cur, p, 0);
            char *q = p;
            cur->val = strtol(p, &p, 10);
            cur->len  = p - q;
            continue;
        }
        error_at(token->str, "'%c' is not tokenizable", *p);
    }

    new_token(TK_EOF, cur, p, 0);
    return head.next;
}

static Node *expr();
static Node *equality();
static Node *relational();
static Node *add();
static Node *mul();
static Node *primary();
static Node *unary();
static Node *primary();


// expr = equality
Node *expr() {
    return equality();
}

Node *equality() {
    Node *node = relational();

    for (;;) {
        if (consume("==")) 
            node = new_node(ND_EQ, node, relational());
        else if (consume("!=")) 
            node = new_node(ND_NE, node, relational());
        else
            return node;
    }
}

// relational = add ("<" add | "<=" add | ">" add | ">=" add)*
Node *relational() {
    Node *node = add();

    for (;;) {
        if (consume("<"))
            node = new_node(ND_LT, node, add());
        else if (consume("<="))
            node = new_node(ND_LE, node, add());
        else if (consume(">"))
            node = new_node(ND_LT, add(), node);
        else if (consume(">="))
            node = new_node(ND_LE, add(), node);
        else
            return node;
    }
    
}

// add = mul ("+" mul | "-" mul)*
Node *add() {
    Node *node = mul();

    for (;;) {
        if (consume("+")) 
            node = new_node(ND_ADD, node, mul());
        else if (consume("-")) 
           node = new_node(ND_SUB, node, mul());
        else
            return node;
    }
}

// mul = unary ("*" unary | "/" unary)*
Node *mul() {
    Node *node = unary();

    for (;;) {
        if (consume("*")) 
            node = new_node(ND_MUL, node, unary());
        else if (consume("/"))
            node = new_node(ND_DIV, node, unary());
        else
            return node;
    }
}

// unary = ("+" | "-")? unary
Node *unary() {
    if (consume("+")) 
        return unary();
    if (consume("-")) 
        return new_node(ND_SUB, new_num_node(0), unary());
    return primary();
}

Node *primary() {
    if (consume("(")) {
        Node *node = expr();
        expect(")");
        return node;
    }
    return new_num_node(expect_number());
}

void gen(Node *node) {
    if (node->kind == ND_NUM) {
        printf("  push %d\n", node->val);
        return;
    }

    gen(node->lhs);
    gen(node->rhs);

    printf("  pop rdi\n");
    printf("  pop rax\n");

    switch (node->kind) {
        case ND_ADD:
            printf("  add rax, rdi\n");
            break;
        case ND_SUB:
            printf("  sub rax, rdi\n");
            break;
        case ND_MUL:
            printf("  imul rax, rdi\n");
            break;
        case ND_DIV:
            printf("  cqo\n");
            printf("  idiv rdi\n");
            break;
        case ND_EQ:
            printf("  cmp rax, rdi\n");
            printf("  sete al\n");
            printf("  movzb rax, al\n");
            break;
        case ND_NE:
            printf("  cmp rax, rdi\n");
            printf("  setne al\n");
            printf("  movzb rax, al\n");
            break;
        case ND_LT:
            printf("  cmp rax, rdi\n");
            printf("  setl al\n");
            printf("  movzb rax, al\n");
            break;
        case ND_LE:
            printf("  cmp rax, rdi\n");
            printf("  setle al\n");
            printf("  movzb rax, al\n");
            break;
    }
    printf("  push rax\n");
}

int main(int argc, char **argv) {
    if (argc != 2) {
        fprintf(stderr, "invalid number of arguments \n");
        return 1;
    }

    user_input = argv[1];
    token = tokenize(user_input);
    Node *node = expr();

    printf(".intel_syntax noprefix\n");
    printf(".global main\n");
    printf("main:\n");

    gen(node);

    printf("  pop rax\n");
    printf("  ret\n");
    return 0;
}