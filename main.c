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
    TokenKind kind;
    Token *next;
    int val;  // num if kind is TK_NUM
    char *str;
};

Token *token;

// initialize new token and set it as the next token of current token
Token *new_token(TokenKind kind, Token *cur, char *str) {
    Token *tok = calloc(1, sizeof(Token));
    tok->kind = kind;
    tok->str = str;
    cur->next = tok;
    return tok;
}

typedef enum {
    ND_ADD, // +
    ND_SUB, // -
    ND_MUL, // *
    ND_DIV, // /
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
bool consume(char op) {
    if (token->kind != TK_RESERVED || token->str[0] != op)
        return false;
    token = token->next;
    return true;
}

// move to next and return true when token is expected symbol, return false if not
void expect(char op) {
    if (token->kind != TK_RESERVED || token->str[0] != op) {
        error_at(token->str,"'%c' is not '%c'", token->str[0], op);
    }
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
        if (strchr("+-*/()", *p)) {
            cur = new_token(TK_RESERVED, cur, p++);
            continue;
        }
        if (isdigit(*p)) {
            cur = new_token(TK_NUM, cur, p);
            cur->val = strtol(p, &p, 10);
            continue;
        }
        error_at(token->str, "'%c' is not tokenizable", *p);
    }

    new_token(TK_EOF, cur, p);
    return head.next;
}

static Node *expr();
static Node *mul();
static Node *primary();
static Node *unary();
static Node *primary();

Node *expr() {
    Node *node = mul();

    for (;;) {
        if (consume('+')) {
            node = new_node(ND_ADD, node, mul());
        } else if (consume('-')) {
            node = new_node(ND_SUB, node, mul());
        } else {
            return node;
        }
    }
}

Node *mul() {
    Node *node = unary();
    for (;;) {
        if (consume('*')) {
            node = new_node(ND_MUL, node, primary());
        } else if (consume('/')) {
           node = new_node(ND_DIV, node, primary());
        }
        return node;
    }
}

Node *unary() {
    if (consume('-')) 
        return new_node(ND_SUB, new_num_node(0), primary());
    return primary();
}

Node *primary() {
    if (consume('(')) {
        Node *node = expr();
        expect(')');
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