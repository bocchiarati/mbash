#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <ctype.h>

/* Variables globales */
char prompt[1024] = "mbash> ";

#define STATE_ESPACE     0
#define STATE_ARG        1
#define STATE_QUOTE      2

#define STATE_FINI       3

//Automate
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

// Fonction pour exécuter une commande avec execvp
void execute_command(char *argv[], int argc) {
    pid_t pid;
    int status;

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
        // Processus parent
      if(strcmp(argv[argc - 1], "&") != 0){ //si le dernier arg n'est pas &
        waitpid(pid, &status, 0);
      }else{
	printf("[%d]\n",pid);
      }
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

    if (getcwd(prompt, sizeof(prompt)) != NULL) {
        strcat(prompt, "> "); // Met à jour le prompt
    }

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

        // Quitter si la commande est "exit"
        if (strcmp(command, "exit") == 0) {
            break;
        }

	char *argv[128];

        // Parse la commande avec l'automate
        int argc = parse_command(command, argv, 128);

        // Gérer la commande "cd"
        if (strcmp(argv[0], "cd") == 0) {
	    change_directory(argv[1]);
        } else {
	  execute_command(argv,argc);
        }
    }

    return 0;
}
