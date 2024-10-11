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

struct value {
    int type;

    long number;
    char* string;

    int count;
    value** list;
};

void value_print(value* val);
value* decode_list(char** bencoded_value);

bool is_digit(char c) {
    return c >= '0' && c <= '9';
}

int num_of_digits(int number) {
    int count = 0;
    do {
        number /= 10;
        ++count;
    } while (number != 0);
    return count;
}

value* value_number(long number) {
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

void value_print_list(value* val) {
    putchar('[');
    for (int i = 0; i < val->count; i++) {
        value_print(val->list[i]);
        if (i != (val->count - 1)) putchar(',');
    }
    putchar(']');
}

void value_print(value* val) {
    switch (val->type) {
    case VAL_NUMBER:
        printf("%ld", val->number);
        break;
    case VAL_STRING:
        printf("\"%s\"", val->string);
        break;
    case VAL_LIST:
        value_print_list(val);
        break;
    }
}

void value_println(value* val) {
    value_print(val);
    putchar('\n');
}

value* decode_string(char** bencoded_value) {
    int length = atoi(*bencoded_value);
    char* colon_index = strchr(*bencoded_value, ':');

    if (colon_index != NULL) {
        char* start = colon_index + 1;
        char* decoded_str = (char*)malloc(length + 1);

        strncpy(decoded_str, start, length);
        decoded_str[length] = '\0';

        *bencoded_value = start + length; // Move the pointer past the string
        value* result = value_string(decoded_str);
        free(decoded_str);
        return result;
    }
    else {
        fprintf(stderr, "Invalid encoded value: %s\n", *bencoded_value);
        exit(1);
    }
}

value* decode_integer(char** bencoded_value) {
    (*bencoded_value)++; // Skip the 'i'
    long result = atol(*bencoded_value);

    while (**bencoded_value != 'e') {
        (*bencoded_value)++;
    }
    (*bencoded_value)++; // Skip the 'e'
    printf("%ld\n", result);
    return value_number(result);
}

value* value_take(char** string) {
    if (is_digit(**string)) {
        return decode_string(string);
    }

    if (**string == 'i') {
        return decode_integer(string);
    }

    if (**string == 'l') {
        return decode_list(string);
    }

    return NULL;
}

value* decode_list(char** bencoded_value) {
    (*bencoded_value)++; // Skip the 'l'
    value* result = value_list();

    while (**bencoded_value != 'e') {
        value* element = value_take(bencoded_value);
        result = value_add(result, element);
    }
    (*bencoded_value)++; // Skip the 'e'

    return result;
}

value* decode_bencode(char* bencoded_value) {
    if (is_digit(bencoded_value[0])) return decode_string(&bencoded_value);
    if (bencoded_value[0] == 'i') return decode_integer(&bencoded_value);
    if (bencoded_value[0] == 'l') return decode_list(&bencoded_value);

    fprintf(stderr, "Invalid bencoded value: %s\n", bencoded_value);
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
