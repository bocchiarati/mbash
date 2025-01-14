#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

/* Etats */
#define S_DEPART		0
#define S_APRES_SIGNE		1
#define S_DANS_NOMBRE 		2
#define S_APRES_NOMBRE 		3
 
#define S_FINI	 		4
#define S_ERREUR 	       	5

/* Variables globales */

char prompt[1024] = "mbash> ";

// Fonction pour exécuter une commande (base avec system())
void execute_command(char *command) {
    if (system(command) == -1) {
        perror("system");
    }
}

// Fonction pour gérer la commande "cd"
void change_directory(char *path) {
    if (chdir(path) != 0) {
        perror("cd");
    }else{
        strcpy(prompt, path);  
        strcat(prompt, "> "); //Met à jour le prompt
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
