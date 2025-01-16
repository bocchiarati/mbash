#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <ctype.h>
#include <readline.h>
#include <history.h>

/* Variables globales */
char prompt[1024] = "mbash> ";

#define STATE_ESPACE     0
#define STATE_ARG        1
#define STATE_QUOTE      2
#define STATE_FINI       3

// Automate
int parse_command(char *command, char *argv[], int max_args) {
    int state = STATE_ESPACE;
    char *start = NULL;
    int argc = 0;

    for (char *p = command; *p != '\0'; p++) {
        switch (state) {
            case STATE_ESPACE:
                if (!isspace((unsigned char)*p)) {
                    if (*p == '"') {
                        state = STATE_QUOTE;
                        start = p + 1; // Début après le guillemet
                    } else {
                        state = STATE_ARG;
                        start = p; // Début du mot
                    }
                }
                break;

            case STATE_ARG:
                if (isspace((unsigned char)*p)) {
                    *p = '\0'; // Fin du mot
                    argv[argc++] = start;
                    if (argc >= max_args - 1) return argc; // Limite atteinte
                    state = STATE_ESPACE;
                }
                break;

            case STATE_QUOTE:
                if (*p == '"') {
                    *p = '\0'; // Fin de la chaîne entre guillemets
                    argv[argc++] = start;
                    if (argc >= max_args - 1) return argc; // Limite atteinte
                    state = STATE_ESPACE;
                }
                break;
        }
    }

    // Ajouter le dernier mot si nécessaire
    if (state == STATE_ARG || state == STATE_QUOTE) {
        argv[argc++] = start;
    }

    argv[argc] = NULL; // Terminer la liste des arguments
    return argc;
}

void color(int color_index) {
    printf("\033[0;%dm", color_index);
}

void color_help() {
    printf("Pour changer la couleur du texte dans le terminal, vous pouvez utiliser les codes suivants :\n");
    printf(" - 1 : Rouge\n");
    printf(" - 2 : Vert\n");
    printf(" - 3 : Jaune\n");
    printf(" - 4 : Bleu\n");
    printf(" - 5 : Magenta\n");
    printf(" - 6 : Cyan\n");
    printf(" - 7 : Blanc\n");
    printf(" - 0 : Normal\n");
}

// Fonction pour exécuter une commande avec execvp
void execute_command(char *argv[], int argc) {
    pid_t pid;
    int status;

    if (argc == 0) {
        return; // Si aucune commande n'est donnée, ne rien faire
    }

    pid = fork();
    if (pid == 0) {
        if (execvp(argv[0], argv) == -1) {
            perror("execvp");
        }
        exit(EXIT_FAILURE);
    } else if (pid < 0) {
        perror("fork");
    } else {
        if (strcmp(argv[argc - 1], "&") != 0) {
            waitpid(pid, &status, 0);
        } else {
            printf("[%d]\n", pid);
        }
    }
}

// Fonction pour gérer la commande "cd"
void change_directory(char *path) {
    if (chdir(path) != 0) {
        perror("cd");
    } else {
        if (getcwd(prompt, sizeof(prompt)) != NULL) {
            strcat(prompt, "> ");
        } else {
            perror("getcwd");
            strcpy(prompt, "mbash> ");
        }
    }
}

int main() {
    char *command;

    if (getcwd(prompt, sizeof(prompt)) != NULL) {
        strcat(prompt, "> ");
    }

    while (1) {
        command = readline(prompt);

        if (command == NULL) {
            printf("\n");
            break;
        }

        // Ajouter à l'historique si non vide
        if (*command) {
            add_history(command);
        }

        // Quitter si la commande est "exit"
        if (strcmp(command, "exit") == 0) {
            free(command);
            break;
        }

        char *argv[128];
        int argc = parse_command(command, argv, 128);

        // Gérer la commande "cd"
        if (strcmp(argv[0], "cd") == 0) {
            change_directory(argv[1]);
        } else if (strcmp(argv[0], "history") == 0) {
            HIST_ENTRY **the_history_list = history_list();
            if (the_history_list) {
                for (int i = 0; the_history_list[i]; i++) {
                    printf("%d %s\n", i + history_base, the_history_list[i]->line);
                }
            }
        } else if (strncmp(command, "color", 5) == 0) {
            if (strcmp(command + 6, "1") == 0) {
                color(31);
            } else if (strcmp(command + 6, "2") == 0) {
                color(32);
            } else if (strcmp(command + 6, "3") == 0) {
                color(33);
            } else if (strcmp(command + 6, "4") == 0) {
                color(34);
            } else if (strcmp(command + 6, "5") == 0) {
                color(35);
            } else if (strcmp(command + 6, "6") == 0) {
                color(36);
            } else if (strcmp(command + 6, "7") == 0) {
                color(37);
            } else {
                color_help();
            }
        } else {
            execute_command(argv, argc);
        }

        free(command);
    }

    return 0;
}
