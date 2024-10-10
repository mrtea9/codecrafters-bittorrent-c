#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

typedef struct value value;

enum {
    VAL_NUMBER,
    VAL_STRING,
    VAL_LIST
};

typedef struct value {
    int type;

    int number;
    char* string;

    int count;
    value* list;
} value;

value* value_number(int number) {
    value* val = malloc(sizeof(value));
    val->type = VAL_NUMBER;
    val->number = number;
    return val;
}

value* value_string(char* string) {
    value* val = malloc(sizeof(value));
    val->type = VAL_STRING;
    val->string = malloc(strlen(string) + 1);
    strcpy(val->string, string);
    return val;
}

value* value_list(void) {
    value* val = malloc(sizoef(value));
    val->type = VAL_LIST;
    val->count = 0;
    val->list = NULL;
    return val;
}

void value_print_list(value* value) {
    putchar("[");
    for (int i = 0; i < value->count; i++) {
        value_print(value->list);

        putchar(",");
    }
    putchar("]");
}

void value_print(value* value) {
    switch (value->type) {
        case VAL_NUMBER:
            printf("%s\n", value->number);
            break;
        case VAL_STRING:
            printf("\"%s\"\n", value->string);
            break;
        case VAL_LIST:
            values_list_print(value);
            break;
    }
}

void value_println(value* value) {
    value_print(value);
    putchar('\n');
}

bool is_digit(char c) {
    return c >= '0' && c <= '9';
}

value* decode_string(char* bencoded_value) {
    int length = atoi(bencoded_value);
    char* colon_index = strchr(bencoded_value, ':');

    if (colon_index != NULL) {
        char* start = colon_index + 1;
        char* decoded_str = (char*)malloc(length + 1);

        strncpy(decoded_str, start, length);
        decoded_str[length] = '\0';

        return value_string(decoded_str);
    }
    else {
        fprintf(stderr, "Invalid encoded value: %s\n", bencoded_value);
        exit(1);
    }
}

char* decode_integer(char* bencoded_value) {
    int length = strlen(bencoded_value) - 2;
    char* colon_index = strchr(bencoded_value, 'i');

    if (colon_index != NULL) {
        char* start = colon_index + 1;
        char* decoded_str = (char*)malloc(length + 1);

        strncpy(decoded_str, start, length);
        decoded_str[length] = '\0';

        return value_number(decoded_str);
    }
    else {
        fprintf(stderr, "Invalid encoded value: %s\n", bencoded_value);
        exit(1);
    }
}

void decode_list(char* bencoded_value) {
    int length = strlen(bencoded_value) - 2;
    char* encoded = bencoded_value + 1;
    char* result;

    encoded[length] = '\0';

    if (length == 0) {
        printf("[]\n");
        exit(0);
    }

    for (int i = 0; i < length; i++) {

        if (is_digit(encoded[i])) {
            int length = atoi(encoded);

            result = decode_string(encoded);

            printf("%s\n", result);

            exit(1);
        }

        printf("%c ", encoded[i]);

    }

    printf("%s\n%i\n", encoded, length);

    return value_list();
}

value* decode_bencode(char* bencoded_value) {
    int len = strlen(bencoded_value) - 1;

    if (is_digit(bencoded_value[0])) return decode_string(bencoded_value);
    if (bencoded_value[0] == 'i' && bencoded_value[len] == 'e') return decode_integer(bencoded_value);
    if (bencoded_value[0] == 'l' && bencoded_value[len] == 'e') return decode_list(bencoded_value);

    fprintf(stderr, "Only strings and integer are supported at the moment\n");
    exit(1);
    }

int process_command(char* command, char* encoded_str) {

    if (strcmp(command, "decode") == 0) {
        value* result = decode_bencode(encoded_str);
        value_println(result);
    }
    else {
        fprintf(stderr, "Unknown command: %s\n", command);
        return 1;
    }

    return 0;
}

int main(int argc, char* argv[]) {
	// Disable output buffering
	setbuf(stdout, NULL);
 	setbuf(stderr, NULL);

    if (argc < 3) {
        fprintf(stderr, "Usage: your_bittorrent.sh <command> <args>\n");
        return 1;
    }

    char* command = argv[1];
    char* encoded_str = argv[2];

    process_command(command, encoded_str);

    return 0;
}
