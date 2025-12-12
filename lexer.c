/**
 * LEXICAL ANALYZER FOR PYTHON AND TYPESCRIPT
 * 
 * Features:
 * 1. Comment Detection - Extracts single-line and multi-line comments
 * 2. Tokenization - Breaks code into tokens (keywords, identifiers, etc.)
 * 3. Error Detection:
 *    - Misspelled keywords (using Levenshtein distance)
 *    - Type mismatches (int x = 3.14)
 *    - Undeclared identifiers
 *    - Invalid operators (=< instead of <=)
 * 
 * Output: Displays results in terminal
 * Usage: ./lexer <source_file.py|source_file.ts>
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

/*===========================================================================
 * ANSI COLOR CODES FOR TERMINAL OUTPUT
 *===========================================================================*/
#define COLOR_RESET       "\033[0m"
#define COLOR_BOLD        "\033[1m"

// Comment colors
#define COLOR_SINGLE_LINE_COMMENT   "\033[32m"  // Green
#define COLOR_MULTI_LINE_COMMENT    "\033[36m"  // Cyan

// Token attribute colors
#define COLOR_KEYWORD         "\033[35m"  // Magenta
#define COLOR_IDENTIFIER      "\033[33m"  // Yellow
#define COLOR_LITERAL         "\033[34m"  // Blue
#define COLOR_OPERATOR        "\033[31m"  // Red
#define COLOR_DELIMITER       "\033[37m"  // White

// Error type colors
#define COLOR_ERROR_MISSPELL  "\033[93m"  // Bright Yellow
#define COLOR_ERROR_TYPE      "\033[91m"  // Bright Red
#define COLOR_ERROR_UNDECL    "\033[95m"  // Bright Magenta
#define COLOR_ERROR_OPERATOR  "\033[96m"  // Bright Cyan

// UI elements
#define COLOR_HEADER          "\033[1;36m" // Bold Cyan
#define COLOR_LINE_NUMBER     "\033[90m"   // Gray

/*===========================================================================
 * SECTION 1: CONSTANTS
 *===========================================================================*/

#define MAX_TOKENS   1000
#define MAX_COMMENTS 100
#define MAX_ERRORS   100
#define MAX_SYMBOLS  500
#define MAX_LENGTH   1024

/* Python Keywords */
const char *PYTHON_KEYWORDS[] = {
    "False", "None", "True", "and", "as", "assert", "async", "await",
    "break", "class", "continue", "def", "del", "elif", "else", "except",
    "finally", "for", "from", "global", "if", "import", "in", "is",
    "lambda", "nonlocal", "not", "or", "pass", "raise", "return", "try",
    "while", "with", "yield", "int", "float", "str", "bool", "list", "dict"
};
#define PYTHON_KEYWORD_COUNT 41

/* TypeScript Keywords */
const char *TYPESCRIPT_KEYWORDS[] = {
    "break", "case", "catch", "class", "const", "continue", "debugger",
    "default", "delete", "do", "else", "enum", "export", "extends", "false",
    "finally", "for", "function", "if", "import", "in", "instanceof",
    "interface", "let", "new", "null", "return", "super", "switch", "this",
    "throw", "true", "try", "typeof", "var", "void", "while", "with",
    "number", "string", "boolean", "any", "never", "unknown", "async", "await"
};
#define TYPESCRIPT_KEYWORD_COUNT 46

typedef enum { LANG_PYTHON, LANG_TYPESCRIPT } Language;

/*===========================================================================
 * SECTION 2: DATA STRUCTURES
 *===========================================================================*/

/* Token: smallest meaningful unit (e.g., "print", "123", "+") */
typedef struct {
    char value[256];    // The text of the token
    char type[32];      // KEYWORD, IDENTIFIER, OPERATOR, etc.
    int line;           // Line number
} Token;

/* Comment: stores extracted comment information */
typedef struct {
    char content[MAX_LENGTH];
    int start_line;
    int end_line;
    int is_multiline;
} Comment;

/* Error: stores detected error information */
typedef enum {
    ERROR_TYPE_MISSPELLED_KEYWORD,
    ERROR_TYPE_TYPE_MISMATCH,
    ERROR_TYPE_UNDECLARED_IDENTIFIER,
    ERROR_TYPE_INVALID_OPERATOR
} ErrorType;

typedef struct {
    char message[MAX_LENGTH];
    int line_number;
    ErrorType type;
} Error;

/* Symbol: for tracking declared variables */
typedef struct {
    char name[256];
    int line;
} Symbol;

/*===========================================================================
 * SECTION 3: UTILITY FUNCTIONS
 *===========================================================================*/

/* Returns minimum of three integers */
int min_of_three(int a, int b, int c) {
    int min = a;
    if (b < min) min = b;
    if (c < min) min = c;
    return min;
}

/**
 * Levenshtein Distance Algorithm
 * Calculates edit distance between two strings (insertions, deletions, substitutions)
 * Used to detect misspelled keywords (e.g., "pritn" vs "print" = distance 2)
 */
int levenshtein_distance(const char *str1, const char *str2) {
    int len1 = strlen(str1);
    int len2 = strlen(str2);
    int matrix[len1 + 1][len2 + 1];

    // Initialize base cases
    for (int i = 0; i <= len1; i++) matrix[i][0] = i;
    for (int j = 0; j <= len2; j++) matrix[0][j] = j;

    // Fill matrix using dynamic programming
    for (int i = 1; i <= len1; i++) {
        for (int j = 1; j <= len2; j++) {
            int cost = (tolower(str1[i-1]) == tolower(str2[j-1])) ? 0 : 1;
            matrix[i][j] = min_of_three(
                matrix[i-1][j] + 1,      // deletion
                matrix[i][j-1] + 1,      // insertion
                matrix[i-1][j-1] + cost  // substitution
            );
        }
    }
    return matrix[len1][len2];
}

/* Check if word is a Python keyword */
int is_python_keyword(const char *word) {
    for (int i = 0; i < PYTHON_KEYWORD_COUNT; i++) {
        if (strcmp(word, PYTHON_KEYWORDS[i]) == 0) return 1;
    }
    return 0;
}

/* Check if word is a TypeScript keyword */
int is_typescript_keyword(const char *word) {
    for (int i = 0; i < TYPESCRIPT_KEYWORD_COUNT; i++) {
        if (strcmp(word, TYPESCRIPT_KEYWORDS[i]) == 0) return 1;
    }
    return 0;
}

/* Check if character is part of an operator */
int is_operator_char(char c) {
    return strchr("+-*/%=<>!&|^~", c) != NULL;
}

/* Check if character is a delimiter */
int is_delimiter_char(char c) {
    return strchr("()[]{},:;.", c) != NULL;
}

/* Read entire file into a string */
char *read_file(const char *filename) {
    FILE *file = fopen(filename, "r");
    if (!file) {
        printf("Error: Cannot open file '%s'\n", filename);
        return NULL;
    }
    fseek(file, 0, SEEK_END);
    long size = ftell(file);
    fseek(file, 0, SEEK_SET);
    
    char *content = malloc(size + 1);
    fread(content, 1, size, file);
    content[size] = '\0';
    fclose(file);
    return content;
}

/*===========================================================================
 * SECTION 4: COMMENT EXTRACTION
 * Extracts comments and returns code without comments (clean_code)
 *===========================================================================*/

/**
 * Extract Python comments
 * - Single-line: # comment
 * - Multi-line: ''' or """ (docstrings)
 */
void extract_comments_python(const char *source_code, Comment *comments, int *comment_count, char *code_without_comments) {
    *comment_count = 0;
    int source_index = 0, clean_index = 0, current_line = 1;
    int source_length = strlen(source_code);

    while (source_index < source_length) {
        // Single-line comment: #
        if (source_code[source_index] == '#') {
            comments[*comment_count].start_line = current_line;
            comments[*comment_count].end_line = current_line;
            comments[*comment_count].is_multiline = 0;
            
            int content_index = 0;
            while (source_index < source_length && source_code[source_index] != '\n') {
                comments[*comment_count].content[content_index++] = source_code[source_index++];
            }
            comments[*comment_count].content[content_index] = '\0';
            (*comment_count)++;
        }
        // Multi-line: ''' or """
        else if (source_index + 2 < source_length &&
                 ((source_code[source_index] == '\'' && source_code[source_index+1] == '\'' && source_code[source_index+2] == '\'') ||
                  (source_code[source_index] == '"' && source_code[source_index+1] == '"' && source_code[source_index+2] == '"'))) {
            
            char quote_char = source_code[source_index];
            comments[*comment_count].start_line = current_line;
            comments[*comment_count].is_multiline = 1;
            
            int content_index = 0;
            // Copy opening quotes
            for (int q = 0; q < 3; q++) comments[*comment_count].content[content_index++] = source_code[source_index++];
            
            // Copy until closing quotes
            while (source_index + 2 < source_length) {
                if (source_code[source_index] == '\n') current_line++;
                if (source_code[source_index] == quote_char && source_code[source_index+1] == quote_char && source_code[source_index+2] == quote_char) {
                    for (int q = 0; q < 3; q++) comments[*comment_count].content[content_index++] = source_code[source_index++];
                    break;
                }
                comments[*comment_count].content[content_index++] = source_code[source_index++];
            }
            comments[*comment_count].content[content_index] = '\0';
            comments[*comment_count].end_line = current_line;
            (*comment_count)++;
        }
        // Regular code
        else {
            if (source_code[source_index] == '\n') current_line++;
            code_without_comments[clean_index++] = source_code[source_index++];
        }
    }
    code_without_comments[clean_index] = '\0';
}

/**
 * Extract TypeScript comments
 * - Single-line: //
 * - Multi-line: starts with slash-star, ends with star-slash
 */
void extract_comments_typescript(const char *source_code, Comment *comments, int *comment_count, char *code_without_comments) {
    *comment_count = 0;
    int source_index = 0, clean_index = 0, current_line = 1;
    int source_length = strlen(source_code);

    while (source_index < source_length) {
        // Single-line: //
        if (source_index + 1 < source_length && source_code[source_index] == '/' && source_code[source_index+1] == '/') {
            comments[*comment_count].start_line = current_line;
            comments[*comment_count].end_line = current_line;
            comments[*comment_count].is_multiline = 0;
            
            int content_index = 0;
            while (source_index < source_length && source_code[source_index] != '\n') {
                comments[*comment_count].content[content_index++] = source_code[source_index++];
            }
            comments[*comment_count].content[content_index] = '\0';
            (*comment_count)++;
        }
        // Multi-line: /* */
        else if (source_index + 1 < source_length && source_code[source_index] == '/' && source_code[source_index+1] == '*') {
            comments[*comment_count].start_line = current_line;
            comments[*comment_count].is_multiline = 1;
            
            int content_index = 0;
            comments[*comment_count].content[content_index++] = source_code[source_index++];
            comments[*comment_count].content[content_index++] = source_code[source_index++];
            
            while (source_index + 1 < source_length) {
                if (source_code[source_index] == '\n') current_line++;
                if (source_code[source_index] == '*' && source_code[source_index+1] == '/') {
                    comments[*comment_count].content[content_index++] = source_code[source_index++];
                    comments[*comment_count].content[content_index++] = source_code[source_index++];
                    break;
                }
                comments[*comment_count].content[content_index++] = source_code[source_index++];
            }
            comments[*comment_count].content[content_index] = '\0';
            comments[*comment_count].end_line = current_line;
            (*comment_count)++;
        }
        // Regular code
        else {
            if (source_code[source_index] == '\n') current_line++;
            code_without_comments[clean_index++] = source_code[source_index++];
        }
    }
    code_without_comments[clean_index] = '\0';
}

/*===========================================================================
 * SECTION 5: TOKENIZER
 * Breaks source code into tokens
 * Token types: KEYWORD, IDENTIFIER, INT_LITERAL, FLOAT_LITERAL, 
 *              STRING_LITERAL, OPERATOR, DELIMITER
 *===========================================================================*/

/* Tokenize Python source code */
void tokenize_python(const char *source_code, Token *tokens, int *token_count) {
    *token_count = 0;
    int code_index = 0, current_line = 1;
    int code_length = strlen(source_code);

    while (code_index < code_length && *token_count < MAX_TOKENS) {
        // Skip whitespace
        while (code_index < code_length && isspace(source_code[code_index])) {
            if (source_code[code_index] == '\n') current_line++;
            code_index++;
        }
        if (code_index >= code_length) break;

        // Identifier or Keyword
        if (isalpha(source_code[code_index]) || source_code[code_index] == '_') {
            int value_index = 0;
            while (code_index < code_length && (isalnum(source_code[code_index]) || source_code[code_index] == '_')) {
                tokens[*token_count].value[value_index++] = source_code[code_index++];
            }
            tokens[*token_count].value[value_index] = '\0';
            tokens[*token_count].line = current_line;
            strcpy(tokens[*token_count].type, is_python_keyword(tokens[*token_count].value) ? "KEYWORD" : "IDENTIFIER");
            (*token_count)++;
        }
        // Number (integer or float)
        else if (isdigit(source_code[code_index])) {
            int value_index = 0, has_decimal_point = 0;
            while (code_index < code_length && (isdigit(source_code[code_index]) || source_code[code_index] == '.')) {
                if (source_code[code_index] == '.') has_decimal_point = 1;
                tokens[*token_count].value[value_index++] = source_code[code_index++];
            }
            tokens[*token_count].value[value_index] = '\0';
            tokens[*token_count].line = current_line;
            strcpy(tokens[*token_count].type, has_decimal_point ? "FLOAT_LITERAL" : "INT_LITERAL");
            (*token_count)++;
        }
        // String literal
        else if (source_code[code_index] == '"' || source_code[code_index] == '\'') {
            char quote_char = source_code[code_index];
            int value_index = 0;
            tokens[*token_count].value[value_index++] = source_code[code_index++];
            while (code_index < code_length && source_code[code_index] != quote_char) {
                if (source_code[code_index] == '\\' && code_index + 1 < code_length) tokens[*token_count].value[value_index++] = source_code[code_index++];
                tokens[*token_count].value[value_index++] = source_code[code_index++];
            }
            if (code_index < code_length) tokens[*token_count].value[value_index++] = source_code[code_index++];
            tokens[*token_count].value[value_index] = '\0';
            tokens[*token_count].line = current_line;
            strcpy(tokens[*token_count].type, "STRING_LITERAL");
            (*token_count)++;
        }
        // Operator
        else if (is_operator_char(source_code[code_index])) {
            int value_index = 0;
            while (code_index < code_length && is_operator_char(source_code[code_index]) && value_index < 3) {
                tokens[*token_count].value[value_index++] = source_code[code_index++];
            }
            tokens[*token_count].value[value_index] = '\0';
            tokens[*token_count].line = current_line;
            strcpy(tokens[*token_count].type, "OPERATOR");
            (*token_count)++;
        }
        // Delimiter
        else if (is_delimiter_char(source_code[code_index])) {
            tokens[*token_count].value[0] = source_code[code_index++];
            tokens[*token_count].value[1] = '\0';
            tokens[*token_count].line = current_line;
            strcpy(tokens[*token_count].type, "DELIMITER");
            (*token_count)++;
        }
        else {
            code_index++; // Skip unknown characters
        }
    }
}

/* Tokenize TypeScript source code */
void tokenize_typescript(const char *source_code, Token *tokens, int *token_count) {
    *token_count = 0;
    int code_index = 0, current_line = 1;
    int code_length = strlen(source_code);

    while (code_index < code_length && *token_count < MAX_TOKENS) {
        // Skip whitespace
        while (code_index < code_length && isspace(source_code[code_index])) {
            if (source_code[code_index] == '\n') current_line++;
            code_index++;
        }
        if (code_index >= code_length) break;

        // Identifier or Keyword (TypeScript allows $)
        if (isalpha(source_code[code_index]) || source_code[code_index] == '_' || source_code[code_index] == '$') {
            int value_index = 0;
            while (code_index < code_length && (isalnum(source_code[code_index]) || source_code[code_index] == '_' || source_code[code_index] == '$')) {
                tokens[*token_count].value[value_index++] = source_code[code_index++];
            }
            tokens[*token_count].value[value_index] = '\0';
            tokens[*token_count].line = current_line;
            strcpy(tokens[*token_count].type, is_typescript_keyword(tokens[*token_count].value) ? "KEYWORD" : "IDENTIFIER");
            (*token_count)++;
        }
        // Number
        else if (isdigit(source_code[code_index])) {
            int value_index = 0, has_decimal_point = 0;
            while (code_index < code_length && (isdigit(source_code[code_index]) || source_code[code_index] == '.')) {
                if (source_code[code_index] == '.') has_decimal_point = 1;
                tokens[*token_count].value[value_index++] = source_code[code_index++];
            }
            tokens[*token_count].value[value_index] = '\0';
            tokens[*token_count].line = current_line;
            strcpy(tokens[*token_count].type, has_decimal_point ? "FLOAT_LITERAL" : "INT_LITERAL");
            (*token_count)++;
        }
        // String literal (includes template strings with backtick)
        else if (source_code[code_index] == '"' || source_code[code_index] == '\'' || source_code[code_index] == '`') {
            char quote_char = source_code[code_index];
            int value_index = 0;
            tokens[*token_count].value[value_index++] = source_code[code_index++];
            while (code_index < code_length && source_code[code_index] != quote_char) {
                if (source_code[code_index] == '\\' && code_index + 1 < code_length) tokens[*token_count].value[value_index++] = source_code[code_index++];
                if (source_code[code_index] == '\n') current_line++;
                tokens[*token_count].value[value_index++] = source_code[code_index++];
            }
            if (code_index < code_length) tokens[*token_count].value[value_index++] = source_code[code_index++];
            tokens[*token_count].value[value_index] = '\0';
            tokens[*token_count].line = current_line;
            strcpy(tokens[*token_count].type, "STRING_LITERAL");
            (*token_count)++;
        }
        // Operator
        else if (is_operator_char(source_code[code_index])) {
            int value_index = 0;
            while (code_index < code_length && is_operator_char(source_code[code_index]) && value_index < 3) {
                tokens[*token_count].value[value_index++] = source_code[code_index++];
            }
            tokens[*token_count].value[value_index] = '\0';
            tokens[*token_count].line = current_line;
            strcpy(tokens[*token_count].type, "OPERATOR");
            (*token_count)++;
        }
        // Delimiter
        else if (is_delimiter_char(source_code[code_index])) {
            tokens[*token_count].value[0] = source_code[code_index++];
            tokens[*token_count].value[1] = '\0';
            tokens[*token_count].line = current_line;
            strcpy(tokens[*token_count].type, "DELIMITER");
            (*token_count)++;
        }
        else {
            code_index++;
        }
    }
}

/*===========================================================================
 * SECTION 6: ERROR DETECTION
 * Detects 4 types of errors for each language
 *===========================================================================*/

/**
 * ERROR 1: Misspelled Keywords
 * Uses Levenshtein distance to find identifiers similar to keywords
 */
void check_misspelled_keyword_python(Token *tokens, int count, Error *errors, int *err_count) {
    for (int i = 0; i < count; i++) {
        if (strcmp(tokens[i].type, "IDENTIFIER") != 0 || strlen(tokens[i].value) <= 2) continue;
        
        for (int j = 0; j < PYTHON_KEYWORD_COUNT; j++) {
            int edit_distance = levenshtein_distance(tokens[i].value, PYTHON_KEYWORDS[j]);
            if (edit_distance > 0 && edit_distance <= 2) {
                snprintf(errors[*err_count].message, MAX_LENGTH,
                    "Misspelled keyword - '%s' (did you mean '%s'?)",
                    tokens[i].value, PYTHON_KEYWORDS[j]);
                errors[*err_count].line_number = tokens[i].line;
                errors[*err_count].type = ERROR_TYPE_MISSPELLED_KEYWORD;
                (*err_count)++;
                break;
            }
        }
    }
}

void check_misspelled_keyword_typescript(Token *tokens, int count, Error *errors, int *err_count) {
    for (int i = 0; i < count; i++) {
        if (strcmp(tokens[i].type, "IDENTIFIER") != 0 || strlen(tokens[i].value) <= 2) continue;
        
        for (int j = 0; j < TYPESCRIPT_KEYWORD_COUNT; j++) {
            int edit_distance = levenshtein_distance(tokens[i].value, TYPESCRIPT_KEYWORDS[j]);
            if (edit_distance > 0 && edit_distance <= 2) {
                snprintf(errors[*err_count].message, MAX_LENGTH,
                    "Misspelled keyword - '%s' (did you mean '%s'?)",
                    tokens[i].value, TYPESCRIPT_KEYWORDS[j]);
                errors[*err_count].line_number = tokens[i].line;
                errors[*err_count].type = ERROR_TYPE_MISSPELLED_KEYWORD;
                (*err_count)++;
                break;
            }
        }
    }
}

/**
 * ERROR 2: Type Mismatch
 * Detects when declared type doesn't match assigned value
 * Python: x: int = 3.14 (int declared, float assigned)
 * TypeScript: let x: number = "hello"
 */
void check_type_mismatch_python(Token *tokens, int count, Error *errors, int *err_count) {
    // Pattern: identifier : type = value
    for (int i = 0; i < count - 4; i++) {
        if (strcmp(tokens[i].type, "IDENTIFIER") != 0) continue;
        if (strcmp(tokens[i+1].value, ":") != 0) continue;
        if (strcmp(tokens[i+2].type, "KEYWORD") != 0) continue;
        if (strcmp(tokens[i+3].value, "=") != 0) continue;

        char *declared_type = tokens[i+2].value;
        char *value_type = tokens[i+4].type;

        if (strcmp(declared_type, "int") == 0 && strcmp(value_type, "FLOAT_LITERAL") == 0) {
            snprintf(errors[*err_count].message, MAX_LENGTH,
                "Type mismatch - '%s' declared as int but assigned float value %s",
                tokens[i].value, tokens[i+4].value);
            errors[*err_count].line_number = tokens[i].line;
            errors[*err_count].type = ERROR_TYPE_TYPE_MISMATCH;
            (*err_count)++;
        }
        else if ((strcmp(declared_type, "int") == 0 || strcmp(declared_type, "float") == 0) &&
                  strcmp(value_type, "STRING_LITERAL") == 0) {
            snprintf(errors[*err_count].message, MAX_LENGTH,
                "Type mismatch - '%s' declared as %s but assigned string value",
                tokens[i].value, declared_type);
            errors[*err_count].line_number = tokens[i].line;
            errors[*err_count].type = ERROR_TYPE_TYPE_MISMATCH;
            (*err_count)++;
        }
        else if (strcmp(declared_type, "str") == 0 &&
                (strcmp(value_type, "INT_LITERAL") == 0 || strcmp(value_type, "FLOAT_LITERAL") == 0)) {
            snprintf(errors[*err_count].message, MAX_LENGTH,
                "Type mismatch - '%s' declared as str but assigned numeric value %s",
                tokens[i].value, tokens[i+4].value);
            errors[*err_count].line_number = tokens[i].line;
            errors[*err_count].type = ERROR_TYPE_TYPE_MISMATCH;
            (*err_count)++;
        }
    }
}

void check_type_mismatch_typescript(Token *tokens, int count, Error *errors, int *err_count) {
    // Pattern: let/const/var identifier : type = value
    for (int i = 0; i < count - 5; i++) {
        int is_declaration = strcmp(tokens[i].value, "let") == 0 ||
                            strcmp(tokens[i].value, "const") == 0 ||
                            strcmp(tokens[i].value, "var") == 0;
        if (!is_declaration) continue;
        if (strcmp(tokens[i+1].type, "IDENTIFIER") != 0) continue;
        if (strcmp(tokens[i+2].value, ":") != 0) continue;
        if (strcmp(tokens[i+4].value, "=") != 0) continue;

        char *declared_type = tokens[i+3].value;
        char *value_type = tokens[i+5].type;

        if (strcmp(declared_type, "number") == 0 && strcmp(value_type, "STRING_LITERAL") == 0) {
            snprintf(errors[*err_count].message, MAX_LENGTH,
                "Type mismatch - '%s' declared as number but assigned string value",
                tokens[i+1].value);
            errors[*err_count].line_number = tokens[i].line;
            errors[*err_count].type = ERROR_TYPE_TYPE_MISMATCH;
            (*err_count)++;
        }
        else if (strcmp(declared_type, "string") == 0 &&
                (strcmp(value_type, "INT_LITERAL") == 0 || strcmp(value_type, "FLOAT_LITERAL") == 0)) {
            snprintf(errors[*err_count].message, MAX_LENGTH,
                "Type mismatch - '%s' declared as string but assigned numeric value %s",
                tokens[i+1].value, tokens[i+5].value);
            errors[*err_count].line_number = tokens[i].line;
            errors[*err_count].type = ERROR_TYPE_TYPE_MISMATCH;
            (*err_count)++;
        }
        else if (strcmp(declared_type, "boolean") == 0 &&
                strcmp(tokens[i+5].value, "true") != 0 &&
                strcmp(tokens[i+5].value, "false") != 0) {
            snprintf(errors[*err_count].message, MAX_LENGTH,
                "Type mismatch - '%s' declared as boolean but assigned non-boolean value",
                tokens[i+1].value);
            errors[*err_count].line_number = tokens[i].line;
            errors[*err_count].type = ERROR_TYPE_TYPE_MISMATCH;
            (*err_count)++;
        }
    }
}

/**
 * ERROR 3: Undeclared Identifiers
 * Builds symbol table of declared variables, then checks for undeclared usage
 */
void check_undeclared_identifier_python(Token *tokens, int count, Error *errors, int *err_count) {
    Symbol symbol_table[MAX_SYMBOLS];
    int symbol_count = 0;

    // Pass 1: Collect declared variables (identifier = value)
    for (int i = 0; i < count - 1; i++) {
        if (strcmp(tokens[i].type, "IDENTIFIER") == 0 &&
            strcmp(tokens[i+1].value, "=") == 0 &&
            strcmp(tokens[i+1].value, "==") != 0) {
            
            int already_exists = 0;
            for (int j = 0; j < symbol_count; j++) {
                if (strcmp(symbol_table[j].name, tokens[i].value) == 0) { already_exists = 1; break; }
            }
            if (!already_exists && symbol_count < MAX_SYMBOLS) {
                strcpy(symbol_table[symbol_count++].name, tokens[i].value);
            }
        }
        // Add function params and for loop vars
        if (strcmp(tokens[i].value, "def") == 0 || strcmp(tokens[i].value, "for") == 0) {
            for (int j = i + 1; j < count && strcmp(tokens[j].value, ":") != 0; j++) {
                if (strcmp(tokens[j].type, "IDENTIFIER") == 0) {
                    int already_exists = 0;
                    for (int k = 0; k < symbol_count; k++) {
                        if (strcmp(symbol_table[k].name, tokens[j].value) == 0) { already_exists = 1; break; }
                    }
                    if (!already_exists && symbol_count < MAX_SYMBOLS) {
                        strcpy(symbol_table[symbol_count++].name, tokens[j].value);
                    }
                }
            }
        }
    }

    // Pass 2: Check for undeclared usage
    for (int i = 0; i < count; i++) {
        if (strcmp(tokens[i].type, "IDENTIFIER") != 0 || is_python_keyword(tokens[i].value)) continue;
        if (i + 1 < count && strcmp(tokens[i+1].value, "=") == 0) continue; // Skip declarations
        
        // Skip built-in functions
        if (strcmp(tokens[i].value, "print") == 0 || strcmp(tokens[i].value, "len") == 0 ||
            strcmp(tokens[i].value, "range") == 0 || strcmp(tokens[i].value, "input") == 0 ||
            strcmp(tokens[i].value, "open") == 0 || strcmp(tokens[i].value, "type") == 0) continue;

        int is_declared = 0;
        for (int j = 0; j < symbol_count; j++) {
            if (strcmp(symbol_table[j].name, tokens[i].value) == 0) { is_declared = 1; break; }
        }
        if (!is_declared) {
            snprintf(errors[*err_count].message, MAX_LENGTH,
                "Undeclared identifier - '%s' used but never declared", tokens[i].value);
            errors[*err_count].line_number = tokens[i].line;
            errors[*err_count].type = ERROR_TYPE_UNDECLARED_IDENTIFIER;
            (*err_count)++;
        }
    }
}

void check_undeclared_identifier_typescript(Token *tokens, int count, Error *errors, int *err_count) {
    Symbol symbol_table[MAX_SYMBOLS];
    int symbol_count = 0;

    // Pass 1: Collect declarations (let/const/var identifier)
    for (int i = 0; i < count - 1; i++) {
        if ((strcmp(tokens[i].value, "let") == 0 || strcmp(tokens[i].value, "const") == 0 ||
             strcmp(tokens[i].value, "var") == 0) && strcmp(tokens[i+1].type, "IDENTIFIER") == 0) {
            
            int already_exists = 0;
            for (int j = 0; j < symbol_count; j++) {
                if (strcmp(symbol_table[j].name, tokens[i+1].value) == 0) { already_exists = 1; break; }
            }
            if (!already_exists && symbol_count < MAX_SYMBOLS) {
                strcpy(symbol_table[symbol_count++].name, tokens[i+1].value);
            }
        }
        // Add function parameters
        if (strcmp(tokens[i].value, "function") == 0) {
            for (int j = i + 1; j < count && strcmp(tokens[j].value, ")") != 0; j++) {
                if (strcmp(tokens[j].type, "IDENTIFIER") == 0 &&
                    (j == i + 1 || strcmp(tokens[j-1].value, "(") == 0 || strcmp(tokens[j-1].value, ",") == 0)) {
                    int already_exists = 0;
                    for (int k = 0; k < symbol_count; k++) {
                        if (strcmp(symbol_table[k].name, tokens[j].value) == 0) { already_exists = 1; break; }
                    }
                    if (!already_exists && symbol_count < MAX_SYMBOLS) {
                        strcpy(symbol_table[symbol_count++].name, tokens[j].value);
                    }
                }
            }
        }
    }

    // Pass 2: Check usage
    for (int i = 0; i < count; i++) {
        if (strcmp(tokens[i].type, "IDENTIFIER") != 0 || is_typescript_keyword(tokens[i].value)) continue;
        
        // Skip declarations
        if (i > 0 && (strcmp(tokens[i-1].value, "let") == 0 || strcmp(tokens[i-1].value, "const") == 0 ||
                      strcmp(tokens[i-1].value, "var") == 0 || strcmp(tokens[i-1].value, "function") == 0)) continue;
        
        // Skip common globals
        if (strcmp(tokens[i].value, "console") == 0 || strcmp(tokens[i].value, "log") == 0 ||
            strcmp(tokens[i].value, "document") == 0 || strcmp(tokens[i].value, "window") == 0 ||
            strcmp(tokens[i].value, "Math") == 0 || strcmp(tokens[i].value, "Array") == 0) continue;

        int is_declared = 0;
        for (int j = 0; j < symbol_count; j++) {
            if (strcmp(symbol_table[j].name, tokens[i].value) == 0) { is_declared = 1; break; }
        }
        if (!is_declared) {
            snprintf(errors[*err_count].message, MAX_LENGTH,
                "Undeclared identifier - '%s' used but never declared", tokens[i].value);
            errors[*err_count].line_number = tokens[i].line;
            errors[*err_count].type = ERROR_TYPE_UNDECLARED_IDENTIFIER;
            (*err_count)++;
        }
    }
}

/**
 * ERROR 4: Invalid Operators
 * Detects malformed or wrong operators (=< instead of <=, === in Python)
 */
void check_invalid_operator_python(Token *tokens, int count, Error *errors, int *err_count) {
    for (int i = 0; i < count; i++) {
        if (strcmp(tokens[i].type, "OPERATOR") != 0) continue;

        if (strcmp(tokens[i].value, "===") == 0) {
            snprintf(errors[*err_count].message, MAX_LENGTH,
                "Invalid operator - '===' is not valid in Python, use '==' instead");
            errors[*err_count].line_number = tokens[i].line;
            errors[*err_count].type = ERROR_TYPE_INVALID_OPERATOR;
            (*err_count)++;
        }
        else if (strcmp(tokens[i].value, "!==") == 0) {
            snprintf(errors[*err_count].message, MAX_LENGTH,
                "Invalid operator - '!==' is not valid in Python, use '!=' instead");
            errors[*err_count].line_number = tokens[i].line;
            errors[*err_count].type = ERROR_TYPE_INVALID_OPERATOR;
            (*err_count)++;
        }
        else if (strcmp(tokens[i].value, "=<") == 0) {
            snprintf(errors[*err_count].message, MAX_LENGTH,
                "Invalid operator - '=<' should be '<='");
            errors[*err_count].line_number = tokens[i].line;
            errors[*err_count].type = ERROR_TYPE_INVALID_OPERATOR;
            (*err_count)++;
        }
        else if (strcmp(tokens[i].value, "=>") == 0) {
            snprintf(errors[*err_count].message, MAX_LENGTH,
                "Invalid operator - '=>' is not valid in Python, use '>=' for comparison");
            errors[*err_count].line_number = tokens[i].line;
            errors[*err_count].type = ERROR_TYPE_INVALID_OPERATOR;
            (*err_count)++;
        }
    }
}

void check_invalid_operator_typescript(Token *tokens, int count, Error *errors, int *err_count) {
    for (int i = 0; i < count; i++) {
        if (strcmp(tokens[i].type, "OPERATOR") != 0) continue;

        if (strcmp(tokens[i].value, "=<") == 0) {
            snprintf(errors[*err_count].message, MAX_LENGTH,
                "Invalid operator - '=<' should be '<='");
            errors[*err_count].line_number = tokens[i].line;
            errors[*err_count].type = ERROR_TYPE_INVALID_OPERATOR;
            (*err_count)++;
        }
    }
}

/*===========================================================================
 * SECTION 7: OUTPUT FUNCTIONS
 *===========================================================================*/

/* Helper function to get color for token attribute type */
const char* get_token_attribute_color(const char* attribute_type) {
    if (strcmp(attribute_type, "KEYWORD") == 0) {
        return COLOR_KEYWORD;
    } else if (strcmp(attribute_type, "IDENTIFIER") == 0) {
        return COLOR_IDENTIFIER;
    } else if (strstr(attribute_type, "LITERAL") != NULL) {
        return COLOR_LITERAL;
    } else if (strcmp(attribute_type, "OPERATOR") == 0) {
        return COLOR_OPERATOR;
    } else if (strcmp(attribute_type, "DELIMITER") == 0) {
        return COLOR_DELIMITER;
    }
    return COLOR_RESET;
}

/* Helper function to get color for error type */
const char* get_error_type_color(ErrorType error_type) {
    switch(error_type) {
        case ERROR_TYPE_MISSPELLED_KEYWORD:
            return COLOR_ERROR_MISSPELL;
        case ERROR_TYPE_TYPE_MISMATCH:
            return COLOR_ERROR_TYPE;
        case ERROR_TYPE_UNDECLARED_IDENTIFIER:
            return COLOR_ERROR_UNDECL;
        case ERROR_TYPE_INVALID_OPERATOR:
            return COLOR_ERROR_OPERATOR;
        default:
            return COLOR_RESET;
    }
}

/* Helper function to get error type name */
const char* get_error_type_name(ErrorType error_type) {
    switch(error_type) {
        case ERROR_TYPE_MISSPELLED_KEYWORD:
            return "MISSPELLED KEYWORD";
        case ERROR_TYPE_TYPE_MISMATCH:
            return "TYPE MISMATCH";
        case ERROR_TYPE_UNDECLARED_IDENTIFIER:
            return "UNDECLARED IDENTIFIER";
        case ERROR_TYPE_INVALID_OPERATOR:
            return "INVALID OPERATOR";
        default:
            return "UNKNOWN ERROR";
    }
}

/* Print all results to screen */
void print_results(Token *tokens, int token_count, Comment *comments, int comment_count, Error *errors, int error_count) {
    // Print tokens table with colors
    printf("\n");
    printf("%s╔══════════════════════════════════════════════════════════════════════╗%s\n", COLOR_HEADER, COLOR_RESET);
    printf("%s║                         TOKENIZATION TABLE                           ║%s\n", COLOR_HEADER, COLOR_RESET);
    printf("%s╚══════════════════════════════════════════════════════════════════════╝%s\n", COLOR_HEADER, COLOR_RESET);
    printf("%s┌──────────────────────────────────┬───────────────────────────────────┐%s\n", COLOR_BOLD, COLOR_RESET);
    printf("%s│%-34s│%-35s│%s\n", COLOR_BOLD, "            TOKEN", "           ATTRIBUTE", COLOR_RESET);
    printf("%s├──────────────────────────────────┼───────────────────────────────────┤%s\n", COLOR_BOLD, COLOR_RESET);
    
    for (int i = 0; i < token_count; i++) {
        const char* attribute_color = get_token_attribute_color(tokens[i].type);
        printf("│ %-32s │ %s%-33s%s │\n", 
               tokens[i].value, 
               attribute_color, 
               tokens[i].type, 
               COLOR_RESET);
    }
    printf("%s└──────────────────────────────────┴───────────────────────────────────┘%s\n", COLOR_BOLD, COLOR_RESET);

    // Print comments with colors
    printf("\n");
    printf("%s╔══════════════════════════════════════════════════════════════════════╗%s\n", COLOR_HEADER, COLOR_RESET);
    printf("%s║                         COMMENTS DETECTED                            ║%s\n", COLOR_HEADER, COLOR_RESET);
    printf("%s╚══════════════════════════════════════════════════════════════════════╝%s\n", COLOR_HEADER, COLOR_RESET);
    printf("\n");
    
    if (comment_count == 0) {
        printf("  %s✓ No comments found in the source code.%s\n", COLOR_LINE_NUMBER, COLOR_RESET);
    } else {
        for (int i = 0; i < comment_count; i++) {
            if (comments[i].is_multiline) {
                printf("%s[Lines %d-%d]%s %sMULTI-LINE%s\n%s%s%s\n", 
                       COLOR_LINE_NUMBER,
                       comments[i].start_line, 
                       comments[i].end_line,
                       COLOR_RESET,
                       COLOR_BOLD,
                       COLOR_RESET,
                       COLOR_MULTI_LINE_COMMENT,
                       comments[i].content,
                       COLOR_RESET);
            } else {
                printf("%s[Line %d]%s %sSINGLE-LINE%s: %s%s%s\n", 
                       COLOR_LINE_NUMBER,
                       comments[i].start_line,
                       COLOR_RESET,
                       COLOR_BOLD,
                       COLOR_RESET,
                       COLOR_SINGLE_LINE_COMMENT,
                       comments[i].content,
                       COLOR_RESET);
            }
        }
    }

    // Print errors with colors and categorization
    printf("\n");
    printf("%s╔══════════════════════════════════════════════════════════════════════╗%s\n", COLOR_HEADER, COLOR_RESET);
    printf("%s║                         ERROR DETECTION                              ║%s\n", COLOR_HEADER, COLOR_RESET);
    printf("%s╚══════════════════════════════════════════════════════════════════════╝%s\n", COLOR_HEADER, COLOR_RESET);
    printf("\n");
    
    if (error_count == 0) {
        printf("  %s✓ No errors detected! Code is clean.%s\n", COLOR_SINGLE_LINE_COMMENT, COLOR_RESET);
    } else {
        for (int i = 0; i < error_count; i++) {
            const char* error_color = get_error_type_color(errors[i].type);
            const char* error_type_name = get_error_type_name(errors[i].type);
            
            printf("  %s[Line %d]%s %s[%s]%s\n", 
                   COLOR_LINE_NUMBER,
                   errors[i].line_number,
                   COLOR_RESET,
                   error_color,
                   error_type_name,
                   COLOR_RESET);
            printf("    %s↳ %s%s\n\n", 
                   COLOR_LINE_NUMBER,
                   errors[i].message,
                   COLOR_RESET);
        }
    }
}

/*===========================================================================
 * SECTION 8: MAIN FUNCTION
 *===========================================================================*/

/* Validate and detect language from file extension */
int validate_and_detect_language(const char *filename, Language *lang) {
    const char *ext = strrchr(filename, '.');
    
    if (!ext || ext == filename) {
        printf("%sError:%s File has no extension.\n", COLOR_BOLD, COLOR_RESET);
        printf("Please provide a Python (.py) or TypeScript (.ts, .js) file.\n");
        return 0;
    }
    
    if (strcmp(ext, ".py") == 0) {
        *lang = LANG_PYTHON;
        return 1;
    } else if (strcmp(ext, ".ts") == 0 || strcmp(ext, ".js") == 0) {
        *lang = LANG_TYPESCRIPT;
        return 1;
    } else {
        printf("%sError:%s Unsupported file extension '%s'.\n", COLOR_BOLD, COLOR_RESET, ext);
        printf("Please provide a Python (.py) or TypeScript (.ts, .js) file.\n");
        return 0;
    }
}

int main(int argc, char *argv[]) {
    // Validate command line arguments
    if (argc < 2) {
        printf("\n%sError:%s No input file provided.\n", COLOR_BOLD, COLOR_RESET);
        printf("%sUsage:%s %s <source_file.py|source_file.ts>\n\n", COLOR_BOLD, COLOR_RESET, argv[0]);
        printf("Examples:\n");
        printf("  %s script.py    %s# Analyze Python file\n", argv[0], COLOR_LINE_NUMBER);
        printf("  %s script.ts    %s# Analyze TypeScript file\n", argv[0], COLOR_LINE_NUMBER);
        printf("  %s script.js    %s# Analyze JavaScript file\n", argv[0], COLOR_LINE_NUMBER);
        printf("%s\n", COLOR_RESET);
        return 1;
    }

    if (argc > 2) {
        printf("\n%sError:%s Too many arguments provided.\n", COLOR_BOLD, COLOR_RESET);
        printf("Please provide only one source file at a time.\n");
        printf("%sUsage:%s %s <source_file.py|source_file.ts>\n\n", COLOR_BOLD, COLOR_RESET, argv[0]);
        return 1;
    }

    // Validate file extension and detect language
    Language detected_language;
    if (!validate_and_detect_language(argv[1], &detected_language)) {
        return 1;
    }

    // Read source file
    char *source_code = read_file(argv[1]);
    if (!source_code) return 1;

    const char* language_name = (detected_language == LANG_PYTHON) ? "Python" : "TypeScript";
    
    printf("\n%s╔══════════════════════════════════════════════════════════════════════╗%s\n", COLOR_HEADER, COLOR_RESET);
    printf("%s              LEXICAL ANALYZER - %s MODE                           %s\n", 
           COLOR_HEADER, 
           detected_language == LANG_PYTHON ? "PYTHON    " : "TYPESCRIPT", 
           COLOR_RESET);
    printf("%s╚══════════════════════════════════════════════════════════════════════╝%s\n", COLOR_HEADER, COLOR_RESET);
    printf("\n%sAnalyzing file:%s %s\n", COLOR_BOLD, COLOR_RESET, argv[1]);
    printf("%sLanguage detected:%s %s\n", COLOR_BOLD, COLOR_RESET, language_name);

    // Allocate memory for analysis
    Token *token_array = malloc(sizeof(Token) * MAX_TOKENS);
    Comment *comment_array = malloc(sizeof(Comment) * MAX_COMMENTS);
    Error *error_array = malloc(sizeof(Error) * MAX_ERRORS);
    char *code_without_comments = malloc(strlen(source_code) + 1);
    
    int total_tokens = 0, total_comments = 0, total_errors = 0;

    // Extract comments
    if (detected_language == LANG_PYTHON) {
        extract_comments_python(source_code, comment_array, &total_comments, code_without_comments);
    } else {
        extract_comments_typescript(source_code, comment_array, &total_comments, code_without_comments);
    }

    // Tokenize
    if (detected_language == LANG_PYTHON) {
        tokenize_python(code_without_comments, token_array, &total_tokens);
    } else {
        tokenize_typescript(code_without_comments, token_array, &total_tokens);
    }

    // Perform error detection
    if (detected_language == LANG_PYTHON) {
        check_misspelled_keyword_python(token_array, total_tokens, error_array, &total_errors);
        check_type_mismatch_python(token_array, total_tokens, error_array, &total_errors);
        check_undeclared_identifier_python(token_array, total_tokens, error_array, &total_errors);
        check_invalid_operator_python(token_array, total_tokens, error_array, &total_errors);
    } else {
        check_misspelled_keyword_typescript(token_array, total_tokens, error_array, &total_errors);
        check_type_mismatch_typescript(token_array, total_tokens, error_array, &total_errors);
        check_undeclared_identifier_typescript(token_array, total_tokens, error_array, &total_errors);
        check_invalid_operator_typescript(token_array, total_tokens, error_array, &total_errors);
    }

    // Display formatted results
    print_results(token_array, total_tokens, comment_array, total_comments, error_array, total_errors);

    // Cleanup memory
    free(source_code);
    free(token_array);
    free(comment_array);
    free(error_array);
    free(code_without_comments);

    return 0;
}


