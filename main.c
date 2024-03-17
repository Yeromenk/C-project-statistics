#include "statistic.h"
#include <stdio.h>
#include <stdlib.h>

int main(int argc, char *argv[]) {
    if (argc != 4) {
        fprintf(stderr, "Chyba: Program musi obdrzet tri argumenty.\n");
        return 1;
    } 

    char *matchesFile = argv[1];
    char *playersFile = argv[2];
    char *outputFile = argv[3];

    PlayerStats *players = NULL;
    int playerCount = 0;

    if (loadData(matchesFile, playersFile, &players, &playerCount) != 0) {
        fprintf(stderr, "Chyba pri nacitani hracu.\n");
        return 1;
    }

    Match **matches = processMatches(matchesFile, players, playerCount);
    if (!matches) {
        fprintf(stderr, "Chyba pri zpracovani zapasu.\n");
        free(players);
        return 1;
    }

    if (calculateAdditionalStats(players, playerCount) != 0) {
        fprintf(stderr, "Chyba pri vypoctu dodatecnych statistik.\n");
        free(players);
        return 1;
    }

    if (printStatsToFile(outputFile, players, playerCount, matches) != 0) {
        fprintf(stderr, "Chyba pri zapisu statistik do vystupniho souboru.\n");
        free(players);
        return 1;
    }

    printf("Statistiky byly uspesne vytvoreny a ulozeny do %s.\n", outputFile);

    free(players);
    for (int i = 0; matches[i] != NULL; ++i) {
        free(matches[i]);
    }
    free(matches);
    return 0;
}
