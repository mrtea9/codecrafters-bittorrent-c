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
    value** list;
} value;

void value_print(value* value);

bool is_digit(char c) {
    return c >= '0' && c <= '9';
}

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
    value* val = malloc(sizeof(value));
    val->type = VAL_LIST;
    val->count = 0;
    val->list = NULL;
    return val;
}

value* value_add(value* val1, value* val2) {
    val1->count++;
    val1->list = realloc(val1->list, sizeof(value*) * val1->count);
    val1->list[val1->count - 1] = val2;
    return val1;
}

void value_print_list(value* value) {
    putchar('[');
    for (int i = 0; i < value->count; i++) {
        value_print(value->list[i]);

        putchar(',');
    }
    printf("]\n");
}

void value_print(value* value) {
    switch (value->type) {
        case VAL_NUMBER:
            printf("%i\n", value->number);
            break;
        case VAL_STRING:
            printf("\"%s\"\n", value->string);
            break;
        case VAL_LIST:
            value_print_list(value);
            break;
    }
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

value* decode_integer(char* bencoded_value) {
    int length = strlen(bencoded_value) - 2;
    char* colon_index = strchr(bencoded_value, 'i');

    if (colon_index != NULL) {
        char* start = colon_index + 1;
        char* decoded_str = (char*)malloc(length + 1);
        int result;

        strncpy(decoded_str, start, length);
        decoded_str[length] = '\0';
        result = atoi(decoded_str);

        free(decoded_str);
        return value_number(result);
    }
    else {
        fprintf(stderr, "Invalid encoded value: %s\n", bencoded_value);
        exit(1);
    }
}

value* value_take(char** string, int start) {
    value* result;
    printf("string begin = %s\n", *string);

    if (is_digit(*string[0])) {
        result = decode_string(*string);
        *string = *string + strlen(result->string) + 2;
    }

    if (*string[0] == 'i') {
        result = decode_integer(*string);
        //*string = *string + strlen((char)result->number) + 2;
    }

    printf("string end = %s\n", *string);

    return result;
}

value* decode_list(char* bencoded_value) {
    int length = strlen(bencoded_value) - 2;
    char* encoded = bencoded_value + 1;
    value* result;

    encoded[length] = '\0';

    if (length == 0) {
        return value_list();
    }

    for (int i = 0; i < length; i++) {

        if (is_digit(encoded[i])) {
            printf("encoded = %s\n", encoded);
            value_print(value_take(&encoded, 1));
            printf("encoded = %s\n", encoded);

            exit(1);
        }
        printf("%c ", encoded[i]);
    }

    printf("%s\n%i\n", encoded, length);

    return result;
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
        value_print(result);
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
