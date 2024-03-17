#include "statistic.h"
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


int loadData(const char *matchesFile, const char *playersFile,
             PlayerStats **players, int *playerCount) {
    FILE *playersFilePtr = fopen(playersFile, "r");
    if (!playersFilePtr) {
        perror("Chyba pri otevirani souboru s hraci");
        return 1;
    }

    *players = malloc(sizeof(PlayerStats) * MAX_PLAYERS);
    memset(*players, 0, sizeof(PlayerStats) * MAX_PLAYERS);
    if (!*players) {
        perror("Chyba pri alokaci pameti pro hrace");
        fclose(playersFilePtr);
        return 1;
    }

    *playerCount = 0;


    while (*playerCount < MAX_PLAYERS &&
           fscanf(playersFilePtr, "%d,%255s\n",
                  &(*players)[*playerCount].playerId,
                  (*players)[*playerCount].nickname) == 2) {
        (*players)[*playerCount].kills = 0;
        (*players)[*playerCount].assists = 0;
        (*players)[*playerCount].deaths = 0;
        (*players)[*playerCount].wins = 0;
        (*players)[*playerCount].losses = 0;
        (*players)[*playerCount].redTeamCount = 0;
        (*players)[*playerCount].blueTeamCount = 0;
        (*players)[*playerCount].matchCount = 0;
        (*playerCount)++;
    }

    fclose(playersFilePtr);

    FILE *matchesFilePtr = fopen(matchesFile, "r");
    if (!matchesFilePtr) {
        perror("Chyba pri otevirani souboru s zapasy");
        return 1;
    }

    char buffer[1000];
    Match currentMatch;

    int matchCount = 0;

    while (fgets(buffer, sizeof(buffer), matchesFilePtr) != NULL) {
        if (strcmp(buffer, "match\n") == 0) {

            if (matchCount >= MAX_MATCHES) {
                fprintf(stderr, "Chyba: Překročen maximální počet zápasů.\n");
                return 1;
            }

            fgets(buffer, sizeof(buffer), matchesFilePtr);
            sscanf(buffer, "%d,%d,%d\n", &currentMatch.redTeam[0],
                   &currentMatch.redTeam[1], &currentMatch.redTeam[2]);

            fgets(buffer, sizeof(buffer), matchesFilePtr);
            sscanf(buffer, "%d,%d,%d\n", &currentMatch.blueTeam[0],
                   &currentMatch.blueTeam[1], &currentMatch.blueTeam[2]);

            fgets(buffer, sizeof(buffer), matchesFilePtr);
            sscanf(buffer, "%s\n", currentMatch.result);
        }
        matchCount++;
    }

    fclose(matchesFilePtr);

    return 0;
}


int processTeam(int *team, const char *teamName, PlayerStats *players,
                int playerCount, FILE *file) {
    char buffer[1024];
    fgets(buffer, sizeof(buffer), file);
    int status = sscanf(buffer, "%d,%d,%d\n", &team[0], &team[1], &team[2]);

    if (status != 3) {
        fprintf(stderr, "No data for %s team.\n", teamName);
        return EXIT_FAILURE;
    }

    if (fgets(buffer, sizeof(buffer), file) == NULL) {
        fprintf(stderr, "Nejsou validni K/D/A data pro %s team\n", teamName);
        return EXIT_FAILURE;
    }

    char *start = buffer;
    char *end = NULL;

    for (int j = 0; j < 3; ++j) {
        int playerId = team[j];
        for (int k = 0; k < 3; ++k) {
            if (k != j && playerId == team[k]) {
                fprintf(stderr,
                        "Chyba: Hráč s ID %d hrál vícekrát v červeném "
                        "týmu v zápase.\n",
                        playerId);
                return EXIT_FAILURE;
            }
        }
    }

    for (int i = 0; i < 3; ++i) {
        int playerId = team[i];
        PlayerStats *player = NULL;

        
        for (int j = 0; j < playerCount; ++j) {
            if (playerId == players[j].playerId) {
                player = &players[j];
                break;
            }
        }

        if (!player) {
            fprintf(stderr, "Player s id %d neni nalezen\n", playerId);
            return EXIT_FAILURE;
        }

        int kills = 0, assists = 0, deaths = 0;

        player->kills += kills;
        player->assists += assists;
        player->deaths += deaths;

        player->matchCount++;

        end = strchr(start, ',');
        if (end)
            *end = '\0';

        status = sscanf(start, "%d;%d;%d", &kills, &assists, &deaths);
        if (status != 3) {
            fprintf(stderr, "Chyba: Format K/D/A - %s\n", start);
            return EXIT_FAILURE;
        }

        player->kills += kills;
        player->assists += assists;
        player->deaths += deaths;

        if (end)
            start = end + 1;
        else
            start = NULL;
    }

    return EXIT_SUCCESS;
}

Match **processMatches(const char *matchesFile, PlayerStats *players,
                       int playerCount) {
    FILE *file = fopen(matchesFile, "r");
    if (!file) {
        perror("Chyba při otevírání souboru s zápasy");
        return NULL;
    }

    char buffer[1000];
    Match **matches = (Match **)malloc(sizeof(Match *) * MAX_MATCHES);
    memset(matches, 0, sizeof(Match *) * MAX_MATCHES);
    int matchIndex = 0;

    while (fgets(buffer, sizeof(buffer), file) != NULL) {
        if (strcmp(buffer, "match\n") != 0) {
            fprintf(stderr, "Chyba: Očekával se řetězec 'match'.\n");
            free(matches);
            return NULL;
        }

        if (strcmp(buffer, "match\n") == 0) {
            Match *currentMatch = (Match *)malloc(sizeof(Match));
            memset(currentMatch, 0, sizeof(Match));
            matches[matchIndex] = currentMatch;

            int status = processTeam(currentMatch->redTeam, "red", players,
                                     playerCount, file);
            if (status == EXIT_FAILURE) {
                for (int i = 0; matches[i] != NULL; ++i) {
                    free(matches[i]);
                }
                free(matches);
                return NULL;
            }
            status = processTeam(currentMatch->blueTeam, "blue", players,
                                 playerCount, file);
            if (status == EXIT_FAILURE) {
                for (int i = 0; matches[i] != NULL; ++i) {
                    free(matches[i]);
                }
                free(matches);
                return NULL;
            }

            int redTeamIds[3];
            int blueTeamIds[3];

            for (int j = 0; j < 3; ++j) {
                redTeamIds[j] = currentMatch->redTeam[j];
                blueTeamIds[j] = currentMatch->blueTeam[j];
            }

            for (int i = 0; i < 3; ++i) {
                for (int j = 0; j < 3; ++j) {
                    if (redTeamIds[i] == blueTeamIds[j]) {
                        fprintf(stderr,
                                "Chyba: Hráč s ID %d hrál za obě komandy v "
                                "zápase.\n",
                                redTeamIds[i]);
                        for (int i = 0; matches[i] != NULL; ++i) {
                            free(matches[i]);
                        }
                        free(matches);
                        return NULL;
                    }
                }
            }

            fgets(buffer, sizeof(buffer), file);
            sscanf(buffer, "%s\n", currentMatch->result);

            if (strcmp(currentMatch->result, "red") != 0 &&
                strcmp(currentMatch->result, "blue") != 0) {
                fprintf(stderr, "Chyba: Neplatný výsledek zápasu. Povolené "
                                "hodnoty: 'red' nebo 'blue'.\n");
                for (int i = 0; matches[i] != NULL; ++i) {
                    free(matches[i]);
                }
                free(matches);
                return NULL;
            }

            for (int i = 0; i < 3; i++) {
                int redPlayerId = currentMatch->redTeam[i];
                int bluePlayerId = currentMatch->blueTeam[i];

                PlayerStats *redPlayer = NULL;
                PlayerStats *bluePlayer = NULL;

                for (int j = 0; j < playerCount; j++) {
                    if (players[j].playerId == redPlayerId) {
                        redPlayer = &players[j];
                    }
                    if (players[j].playerId == bluePlayerId) {
                        bluePlayer = &players[j];
                    }
                }

                if (redPlayer && bluePlayer) {
                    if (strcmp(currentMatch->result, "red") == 0) {
                        redPlayer->wins++;
                        bluePlayer->losses++;
                        redPlayer->redTeamCount++;
                        bluePlayer->blueTeamCount++;
                    } else if (strcmp(currentMatch->result, "blue") == 0) {
                        bluePlayer->wins++;
                        redPlayer->losses++;
                        bluePlayer->blueTeamCount++;
                        redPlayer->redTeamCount++;
                    }
                }
            }
            matchIndex++;
        }
    }

    fclose(file);
    return matches;
}


int calculateAdditionalStats(PlayerStats *players, int playerCount) {
    for (int i = 0; i < playerCount; i++) {
        PlayerStats *player = &players[i];

        if (player->deaths > 0) {
            player->killDeathRatio = (float)player->kills / player->deaths;
        } else {
            player->killDeathRatio = (float)player->kills;
        }
    }

    return 0;
}

int compareKillStats(const void *a, const void *b) {
    const PlayerStats *killA = (const PlayerStats *)a;
    const PlayerStats *killB = (const PlayerStats *)b;
    return (killB->kills - killA->kills);
}

void topNKillsHtml(FILE *file, PlayerStats *players, int playerCount, int n) {
    PlayerStats *killStats = malloc(sizeof(PlayerStats) * playerCount);
    if (!killStats) {
        perror("Chyba pri alokaci pameti");
        return;
    }

    for (int i = 0; i < playerCount; i++) {
        killStats[i].playerId = players[i].playerId;
        killStats[i].kills = players[i].kills;
        strcpy(killStats[i].nickname, players[i].nickname);
    }

    qsort(killStats, playerCount, sizeof(PlayerStats), compareKillStats);

    fprintf(file, "<h2>Top %d hráčů s nejvíce killy:</h2>\n", n);
    fprintf(file, "<ul>\n");
    for (int i = 0; i < n && i < playerCount; i++) {
        fprintf(file, "<li>Nickname hráče: %s, Killy: %d</li>\n",
                killStats[i].nickname, killStats[i].kills);
    }
    fprintf(file, "</ul>\n");

    free(killStats);
}


void mostFrequentTeammateHtml(FILE *file, PlayerStats *players, int playerCount,
                              Match **matches) {
    fprintf(file, "<h2>Nejčastější spoluhráč pro každého hráče:</h2>\n");
    fprintf(file, "<ul>\n");
    int *count = (int *)malloc(sizeof(int) * playerCount);

    
    for (int i = 0; i < playerCount; i++) {
        PlayerStats *player = &players[i];
        int mostFrequentId = -1;
        int maxCount = 0;
        int isRed = 0;

        memset(count, 0, playerCount * sizeof(int));

        for (int j = 0; matches[j] != NULL; ++j) {
            for (int k = 0; k < 3; ++k) {
                if (player->playerId == matches[j]->redTeam[k]) {
                    isRed = 1;
                    break;
                }
            }

            for (int k = 0; k < 3; ++k) {
                if (player->playerId == matches[j]->blueTeam[k]) {
                    isRed = 0;
                    break;
                }
            }

            for (int k = 0; k < 3; ++k) {
                int playerId = 0;

                if (isRed == 1) {
                    playerId = matches[j]->redTeam[k];
                } else {
                    playerId = matches[j]->blueTeam[k];
                }

                if (playerId == player->playerId) {
                    continue;
                }

                count[playerId - 1]++;
            }
        }

        for (int j = 0; j < playerCount; ++j) {
            if (j == i)
                continue;

            if (count[j] > maxCount) {
                maxCount = count[j];
                mostFrequentId = j + 1;
            }
        }

        if (mostFrequentId != -1) {
            char *mostFrequentNickname = NULL;
            for (int j = 0; j < playerCount; ++j) {
                if (players[j].playerId == mostFrequentId) {
                    mostFrequentNickname = players[j].nickname;
                    break;
                }
            }

            fprintf(file,
                    "<li>Pro hráče s nicknamem - '%s' je nejčastějším "
                    "spoluhráčem hráč s "
                    "nicknamem - '%s' "
                    "s počtem společných zápasů: %d</li>\n",
                    player->nickname, mostFrequentNickname, maxCount);
        }
    }
    fprintf(file, "</ul>\n");
    free(count);
}

int printStatsToFile(const char *outputFile, PlayerStats *players,
                     int playerCount, Match **matches) {
    FILE *file = fopen(outputFile, "w");
    if (!file) {
        perror("Chyba pri otevirani vystupniho souboru");
        return 1;
    }

    fprintf(file, "<html>\n");
    fprintf(file, "<head>\n");
    fprintf(file, "<title>Statistiky LoL hracu</title>\n");
    fprintf(file, "</head>\n");
    fprintf(file, "<body>\n");

    fprintf(file, "<h1>Statistiky hracu</h1>\n");

    fprintf(file, "<table border=\"1\">\n");
    fprintf(file,
            "<tr><th>ID</th><th>Nickname</th><th>Kills</th><th>Assists</"
            "th><th>Deaths</th><th>Games</th><th>Wins</th><th>Losses</"
            "th><th>Red Team "
            "Count</th><th>Blue Team Count</th><th>K/D Ratio</th></tr>\n");

    for (int i = 0; i < playerCount; i++) {
        PlayerStats *player = &players[i];
        fprintf(file,
                "<tr><td>%d</td><td>%s</td><td>%d</td><td>%d</td><td>%d</"
                "td><td>%d</"
                "td><td>%d</td><td>%d</td><td>%d</td><td>%d</td><td>%.2f</td></"
                "tr>\n",
                player->playerId, player->nickname, player->kills,
                player->assists, player->deaths, player->matchCount,
                player->wins, player->losses, player->redTeamCount,
                player->blueTeamCount, player->killDeathRatio);
    }

    fprintf(file, "</table>\n");

    topNKillsHtml(file, players, playerCount, 10);
    mostFrequentTeammateHtml(file, players, playerCount, matches);

    fprintf(file, "</body>\n");
    fprintf(file, "</html>\n");

    fclose(file);

    return 0;
}