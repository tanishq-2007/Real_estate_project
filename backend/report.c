#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define LINE_SIZE 1024
#define FIELD_COUNT 7

static int parse_nonnegative(const char *text, double *value)
{
    char *end;

    errno = 0;
    *value = strtod(text, &end);
    return end != text && *end == '\0' && errno != ERANGE && *value >= 0;
}

int main(int argc, char **argv)
{
    const char *path = argc > 1 ? argv[1] : "properties.db";
    FILE *file;
    char line[LINE_SIZE];
    size_t total = 0;
    size_t available = 0;
    size_t booked = 0;
    double total_value = 0;
    double total_area = 0;
    size_t line_number = 0;

    if (argc > 2) {
        fprintf(stderr, "Usage: %s [properties.db]\n", argv[0]);
        return 2;
    }

    file = fopen(path, "r");
    if (file == NULL) {
        perror(path);
        return 1;
    }

    while (fgets(line, sizeof(line), file) != NULL) {
        char *fields[FIELD_COUNT];
        char *cursor;
        size_t field_count = 1;
        double price;
        double area;
        size_t length;

        line_number++;
        length = strlen(line);
        if (length > 0 && line[length - 1] == '\n') {
            line[--length] = '\0';
        } else if (!feof(file)) {
            fprintf(stderr, "Skipping overlong record on line %zu.\n", line_number);
            while (fgets(line, sizeof(line), file) != NULL &&
                   strchr(line, '\n') == NULL) {
            }
            continue;
        }
        if (length > 0 && line[length - 1] == '\r') {
            line[--length] = '\0';
        }

        fields[0] = line;
        for (cursor = line; *cursor != '\0'; cursor++) {
            if (*cursor == '|') {
                if (field_count == FIELD_COUNT) {
                    field_count++;
                    break;
                }
                *cursor = '\0';
                fields[field_count++] = cursor + 1;
            }
        }

        if (field_count != FIELD_COUNT ||
            !parse_nonnegative(fields[4], &price) ||
            !parse_nonnegative(fields[5], &area) ||
            (strcmp(fields[6], "0") != 0 && strcmp(fields[6], "1") != 0)) {
            fprintf(stderr, "Skipping invalid property record on line %zu.\n", line_number);
            continue;
        }

        total++;
        total_value += price;
        total_area += area;
        if (fields[6][0] == '1') {
            available++;
        } else {
            booked++;
        }
    }

    if (ferror(file)) {
        perror("Error reading property database");
        fclose(file);
        return 1;
    }
    fclose(file);

    printf("Real Estate Portfolio Report\n");
    printf("Database: %s\n", path);
    printf("Properties: %zu\n", total);
    printf("Available: %zu\n", available);
    printf("Booked: %zu\n", booked);
    printf("Total listed value: INR %.2f\n", total_value);
    printf("Average asking price: INR %.2f\n",
           total == 0 ? 0 : total_value / total);
    printf("Total area: %.2f sq.ft\n", total_area);
    return 0;
}
