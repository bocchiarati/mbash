#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <ctype.h>

/* Variables globales */
char prompt[1024] = "mbash> ";

// Fonction pour nettoyer les espaces superflus
void trim_and_normalize(char *str) {
    char *src = str, *dst = str;
    int in_space = 0;

    // Ignorer les espaces initiaux
    while (isspace((unsigned char)*src)) src++;

    // Parcourir le reste de la chaîne
    while (*src) {
        if (isspace((unsigned char)*src)) {
            if (!in_space) {
                *dst++ = ' ';
                in_space = 1;
            }
        } else {
            *dst++ = *src;
            in_space = 0;
        }
        src++;
    }

    // Supprimer les espaces en fin de chaîne
    if (dst > str && isspace((unsigned char)*(dst - 1))) {
        dst--;
    }

    *dst = '\0'; // Terminer la chaîne
}

#define STATE_DEPART     0
#define STATE_ESPACE     1
#define STATE_ARG        2
#define STATE_QUOTE      3

#define STATE_FINI       4

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

// Fonction pour exécuter une commande avec execve
void execute_command(char *command) {
    pid_t pid;
    int status;

    // Tableau d'arguments pour execve
    char *argv[128];

    // Parse la commande avec l'automate
    int argc = parse_command(command, argv, 128);

    if (argc == 0) {
        return; // Si aucune commande n'est donnée, ne rien faire
    }

    // Créer un processus enfant
    pid = fork();
    if (pid == 0) {
        // Processus enfant
        if (execvp(argv[0], argv) == -1) {
            perror("execvp");
        }
        exit(EXIT_FAILURE); // Quitter si exec échoue
    } else if (pid < 0) {
        // Erreur lors de la création du processus
        perror("fork");
    } else {
        // Processus parent, attendre la fin du processus enfant
        waitpid(pid, &status, 0);
    }
}

// Fonction pour gérer la commande "cd"
void change_directory(char *path) {
    if (chdir(path) != 0) {
        perror("cd");
    } else {
        if (getcwd(prompt, sizeof(prompt)) != NULL) {
            strcat(prompt, "> "); // Met à jour le prompt
        } else {
            perror("getcwd");
            strcpy(prompt, "mbash> ");
        }
    }
}

int main() {
    char command[1024];

    while (1) {
        // Afficher le prompt
        printf("%s", prompt);
        fflush(stdout);

        // Lire la commande
        if (fgets(command, sizeof(command), stdin) == NULL) {
            printf("\n");
            break; // Quitter sur EOF (Ctrl+D)
        }

        // Supprimer le saut de ligne
        command[strcspn(command, "\n")] = '\0';

        // Normaliser la commande
        trim_and_normalize(command);

        // Quitter si la commande est "exit"
        if (strcmp(command, "exit") == 0) {
            break;
        }

        // Gérer la commande "cd"
        if (strncmp(command, "cd ", 3) == 0) {
            change_directory(command + 3);
        } else {
            execute_command(command);
        }
    }

    return 0;
}
