#ifndef STATISTIC_H
#define STATISTIC_H
#define MAX_PLAYERS 100
#define MAX_MATCHES 100

#include <stdio.h>

typedef struct {
    int redTeam[3];
    int blueTeam[3];
    char result[5];
} Match;

typedef struct {
    int playerId;
    char nickname[256];
    int kills;
    int assists;
    int deaths;
    int wins;
    int losses;
    int redTeamCount;
    int blueTeamCount;
    float killDeathRatio;
    int matchCount;
} PlayerStats;

int loadData(const char *matchesFile, const char *playersFile,
             PlayerStats **players, int *playerCount);
Match **processMatches(const char *matchesFile, PlayerStats *players,
                       int playerCount);
int calculateAdditionalStats(PlayerStats *players, int playerCount);
int compareKillStats(const void *a, const void *b);
void topNKillsHtml(FILE *file, PlayerStats *players, int playerCount, int n);
void mostFrequentTeammateHtml(FILE *file, PlayerStats *players, int playerCount,
                              Match **matches);
int printStatsToFile(const char *outputFile, PlayerStats *players,
                     int playerCount, Match **matches);

int processTeam(int *team, const char *teamName, PlayerStats *players,
                int playerCount, FILE *file);
#endif
