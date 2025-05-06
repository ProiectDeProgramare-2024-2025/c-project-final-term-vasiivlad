#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <ctype.h>
#include <unistd.h>
#include <limits.h>
#include <mach-o/dyld.h>
#include <libgen.h>

#define MAX_WORD_LENGTH 50
#define MAX_WORDS 100
#define MAX_INCORRECT 5
#define MAX_SCORES 1000
#define COLOR_RESET   "\x1b[0m"
#define COLOR_RED     "\x1b[31m"
#define COLOR_GREEN   "\x1b[32m"
#define COLOR_YELLOW  "\x1b[33m"
#define COLOR_CYAN    "\x1b[36m"
#define COLOR_MAGENTA "\x1b[35m"

struct Dictionary {
    char words[MAX_WORDS][MAX_WORD_LENGTH];
    int count;
};

struct PlayerScore {
    char name[50];
    int score;
};

void fix_working_directory() {
    char exePath[PATH_MAX];
    uint32_t size = PATH_MAX;
    if (_NSGetExecutablePath(exePath, &size) == 0) {
        char *dir = dirname(exePath);
        chdir(dir);
    }
}

void clear_screen() {
    system("clear");
}

int get_menu_choice() {
    char buf[10];
    int choice;
    while (1) {
        printf(COLOR_YELLOW "Enter your choice (1-5): " COLOR_RESET);
        if (!fgets(buf, sizeof(buf), stdin)) continue;
        if (sscanf(buf, "%d", &choice) != 1) continue;
        if (choice >= 1 && choice <= 5) return choice;
    }
}

void initialize_dictionary_file() {
    FILE *fp = fopen("words.txt", "r");
    if (!fp) {
        fp = fopen("words.txt", "w");
        if (fp) {
            fprintf(fp, "banana\ncomputer\nprogramming\nhangman\nexample\n");
            fclose(fp);
        }
    } else {
        fclose(fp);
    }
}

void load_dictionary(struct Dictionary *dict) {
    initialize_dictionary_file();
    FILE *fp = fopen("words.txt", "r");
    dict->count = 0;
    while (dict->count < MAX_WORDS && fgets(dict->words[dict->count], MAX_WORD_LENGTH, fp)) {
        dict->words[dict->count][strcspn(dict->words[dict->count], "\n")] = '\0';
        dict->count++;
    }
    fclose(fp);
}

void add_word_to_dictionary(struct Dictionary *dict) {
    clear_screen();
    printf(COLOR_CYAN "===== ADD A NEW WORD =====\n" COLOR_RESET);
    if (dict->count >= MAX_WORDS) {
        printf(COLOR_RED "Dictionary is full, cannot add more words.\n" COLOR_RESET);
        printf("Press Enter to return to menu...");
        getchar();
        return;
    }
    char buf[MAX_WORD_LENGTH];
    while (1) {
        printf("Enter the new word (only letters): ");
        if (!fgets(buf, sizeof(buf), stdin)) continue;
        buf[strcspn(buf, "\n")] = '\0';
        int valid = buf[0] != '\0';
        for (int i = 0; buf[i] && valid; i++) {
            if (!isalpha((unsigned char)buf[i])) valid = 0;
        }
        if (valid) break;
        printf(COLOR_RED "Invalid word, please use only letters.\n" COLOR_RESET);
    }
    for (int i = 0; buf[i]; i++) {
        buf[i] = (char)tolower((unsigned char)buf[i]);
    }
    FILE *fp = fopen("words.txt", "a");
    fprintf(fp, "%s\n", buf);
    fclose(fp);
    strncpy(dict->words[dict->count], buf, MAX_WORD_LENGTH - 1);
    dict->words[dict->count][MAX_WORD_LENGTH - 1] = '\0';
    dict->count++;
    printf(COLOR_GREEN "Word '%s' added successfully!\n" COLOR_RESET, buf);
    printf("Press Enter to return to menu...");
    getchar();
}

const char *select_random_word(const struct Dictionary *dict) {
    return dict->words[rand() % dict->count];
}

int play_round(const struct Dictionary *dict) {
    const char *word = select_random_word(dict);
    int len = (int)strlen(word);
    char display[MAX_WORD_LENGTH];
    int incorrect = 0;
    char guessed[26] = {0};
    int gcount = 0;
    for (int i = 0; i < len; i++) {
        display[i] = (i == 0 || i == len - 1) ? word[i] : '_';
    }
    display[len] = '\0';
    while (incorrect < MAX_INCORRECT) {
        if (!strchr(display, '_')) {
            printf(COLOR_GREEN "\nYou guessed the word: %s\n" COLOR_RESET, word);
            return len;
        }
        printf("\nWord: ");
        for (int i = 0; i < len; i++) {
            if (display[i] == '_') printf("_ ");
            else printf(COLOR_MAGENTA "%c " COLOR_RESET, display[i]);
        }
        printf("\nEnter a letter (a-z): ");
        char buf[10]; char letter;
        if (!fgets(buf, sizeof(buf), stdin)) continue;
        if (sscanf(buf, " %c", &letter) != 1) continue;
        letter = (char)tolower((unsigned char)letter);
        if (!isalpha((unsigned char)letter)) {
            printf(COLOR_RED "Invalid input, please enter a letter.\n" COLOR_RESET);
            continue;
        }
        int already = 0;
        for (int i = 0; i < gcount; i++) if (guessed[i] == letter) { already = 1; break; }
        if (already) {
            printf(COLOR_YELLOW "Letter already guessed.\n" COLOR_RESET);
            continue;
        }
        guessed[gcount++] = letter;
        int found = 0;
        for (int i = 0; i < len; i++) {
            if (word[i] == letter) { display[i] = letter; found = 1; }
        }
        if (!found) {
            incorrect++;
            printf(COLOR_RED "Incorrect guess! Total incorrect: %d\n" COLOR_RESET, incorrect);
        }
    }
    printf(COLOR_RED "\nGame over! The word was: %s\n" COLOR_RESET, word);
    return 0;
}

int compare_scores(const void *a, const void *b) {
    const struct PlayerScore *pa = a;
    const struct PlayerScore *pb = b;
    return pb->score - pa->score;
}

void load_leaderboard(struct PlayerScore arr[], int *count) {
    *count = 0;
    FILE *fp = fopen("leaderboard.txt", "r");
    if (!fp) return;
    while (*count < MAX_SCORES && fscanf(fp, "%49s %d", arr[*count].name, &arr[*count].score) == 2) {
        (*count)++;
    }
    fclose(fp);
}

void save_leaderboard(struct PlayerScore arr[], int count) {
    FILE *fp = fopen("leaderboard.txt", "w");
    for (int i = 0; i < count; i++) {
        fprintf(fp, "%s %d\n", arr[i].name, arr[i].score);
    }
    fclose(fp);
}

void update_leaderboard(const struct PlayerScore *new_ps) {
    struct PlayerScore arr[MAX_SCORES];
    int count;
    load_leaderboard(arr, &count);
    int found;
    for (int i = 0; i < count; i++) {
        if (strcmp(arr[i].name, new_ps->name) == 0) {
            if (new_ps->score > arr[i].score) arr[i].score = new_ps->score;
            return save_leaderboard(arr, count);
        }
    }
    if (count < MAX_SCORES) arr[count++] = *new_ps;
    qsort(arr, count, sizeof(struct PlayerScore), compare_scores);
    save_leaderboard(arr, count);
}

void play_game(const struct Dictionary *dict) {
    int total = 0;
    while (1) {
        clear_screen();
        printf(COLOR_CYAN "===== PLAY HANGMAN =====\n" COLOR_RESET);
        int score = play_round(dict);
        if (score == 0) break;
        total += score;
        printf("Round score: %d, Total score: %d\n", score, total);
        printf("Play next word? (y/n): ");
        char c = getchar(); getchar();
        if (c != 'y' && c != 'Y') break;
    }
    printf("\nFinal score: %d\n", total);
    struct PlayerScore ps;
    while (1) {
        printf("Enter your name: ");
        fgets(ps.name, sizeof(ps.name), stdin);
        ps.name[strcspn(ps.name, "\n")] = '\0';
        if (ps.name[0]) break;
        printf(COLOR_RED "Name cannot be empty.\n" COLOR_RESET);
    }
    ps.score = total;
    update_leaderboard(&ps);
    printf("Your best score has been recorded; lower scores for your name are discarded.\n");
    printf("Press Enter to return to menu...");
    getchar();
}

void show_leaderboard() {
    clear_screen();
    printf(COLOR_CYAN "===== LEADERBOARD =====\n" COLOR_RESET);
    struct PlayerScore arr[MAX_SCORES];
    int count;
    load_leaderboard(arr, &count);
    qsort(arr, count, sizeof(struct PlayerScore), compare_scores);
    if (count == 0) {
        printf("No data available.\n");
    } else {
        for (int i = 0; i < count; i++) {
            printf("%s %d\n", arr[i].name, arr[i].score);
        }
    }
    printf("Press Enter to return to menu...");
    getchar();
}

void show_history() {
    clear_screen();
    printf(COLOR_CYAN "===== VIEW HISTORY =====\n" COLOR_RESET);
    char name[50];
    printf("Enter name: ");
    fgets(name, sizeof(name), stdin);
    name[strcspn(name, "\n")] = '\0';
    FILE *fp = fopen("leaderboard.txt", "r");
    int found = 0;
    if (fp) {
        char record[100];
        while (fgets(record, sizeof(record), fp)) {
            char rname[50]; int score;
            if (sscanf(record, "%s %d", rname, &score) == 2 && strcmp(rname, name) == 0) {
                printf("%s", record);
                found = 1;
            }
        }
        fclose(fp);
    }
    if (!found) printf("No history for '%s'.\n", name);
    printf("Press Enter to return to menu...");
    getchar();
}

void show_menu() {
    clear_screen();
    printf(COLOR_CYAN "===== MAIN MENU =====\n" COLOR_RESET);
    printf("1. Play Hangman\n");
    printf("2. View Leaderboard\n");
    printf("3. View History\n");
    printf("4. Exit\n");
    printf("5. Add a Word to the Dictionary\n");
}

int main() {
    fix_working_directory();
    srand((unsigned int)time(NULL));
    struct Dictionary dict;
    load_dictionary(&dict);
    while (1) {
        show_menu();
        int choice = get_menu_choice();
        if (choice == 1) play_game(&dict);
        else if (choice == 2) show_leaderboard();
        else if (choice == 3) show_history();
        else if (choice == 4) { clear_screen(); printf("Goodbye!\n"); break; }
        else if (choice == 5) add_word_to_dictionary(&dict);
    }
    return 0;
}
