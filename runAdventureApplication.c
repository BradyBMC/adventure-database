#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include "libpq-fe.h"

/* These constants would normally be in a header file */
/* Maximum length of string used to submit a connection */
#define MAXCONNECTIONSTRINGSIZE 501
/* Maximum length of string used to submit a SQL statement */
#define MAXSQLSTATEMENTSTRINGSIZE 2001
/* Maximum length of string version of integer; you don't have to use a value this big */
#define  MAXNUMBERSTRINGSIZE        20


/* Exit with success after closing connection to the server
 *  *  and freeing memory that was used by the PGconn object.
 *   */
static void good_exit(PGconn *conn)
{
    PQfinish(conn);
    exit(EXIT_SUCCESS);
}

/* Exit with failure after closing connection to the server
 *  *  and freeing memory that was used by the PGconn object.
 *   */
static void bad_exit(PGconn *conn)
{
    PQfinish(conn);
    exit(EXIT_FAILURE); }

/* The three C functions that for Lab4 should appear below.
 *  * Write those functions, as described in Lab4 Section 4 (and Section 5,
 *   * which describes the Stored Function used by the third C function).
 *    *
 *     * Write the tests of those function in main, as described in Section 6
 *      * of Lab4.
 *       */

/* Function: printNumberOfThingsInRoom:
 *  * -------------------------------------
 *   * Parameters:  connection, and theRoomID, which should be the roomID of a room.
 *    * Prints theRoomID, and number of things in that room.
 *     * Return 0 if normal execution, -1 if no such room.
 *      * bad_exit if SQL statement execution fails.
 *       */


int printNumberOfThingsInRoom(PGconn *conn, int theRoomID)
{
    PGresult *res = NULL;
    char buf[MAXNUMBERSTRINGSIZE];
    snprintf(buf, 20, "%d", theRoomID);

    res = PQexec(conn, "BEGIN TRANSACTION ISOLATION LEVEL SERIALIZABLE");
    if (PQresultStatus(res) != PGRES_COMMAND_OK) {
      PQclear(res);
      res = PQexec(conn, "COMMIT TRANSACTION");
      PQclear(res);
      printf("SQL AT BEGIN ERROR\n");
      bad_exit(conn);
      return -1;
    }
    PQclear(res);

    // Query count of valid roomID
    char select[MAXSQLSTATEMENTSTRINGSIZE] = "SELECT COUNT(*) FROM ROOMS WHERE roomID=";
    strcat(select, buf);
    strcat(select, ";");
    res = PQexec(conn, select);

    if (PQresultStatus(res) != PGRES_TUPLES_OK) {
      PQclear(res);
      printf("1st SQL ERROR\n");
      bad_exit(conn);
      return -1;
    }

    if (atoi(PQgetvalue(res, 0, 0)) == 0) {
       PQclear(res);
       res = PQexec(conn, "COMMIT TRANSACTION");
       PQclear(res);
       return -1;
    }
    PQclear(res);
    strcpy(select, "");

    int count = 0;
    // Count number of characters with items in the room
    strcat(select, "SELECT COUNT(*) FROM Characters c, Things t WHERE c.memberID = t.ownerMemberID AND c.role = t.ownerRole AND c.roomID = ");
    strcat(select, buf);
    strcat(select, ";");
    res = PQexec(conn, select);
    
    if (PQresultStatus(res) != PGRES_TUPLES_OK) {
      printf("2nd SQL ERROR\n");
      bad_exit(conn);
      return -1;
    }

    count += atoi(PQgetvalue(res, 0, 0));
    PQclear(res);

    strcpy(select, "");
    // Count number of items with no owner in the room
    strcat(select, "SELECT COUNT(*) FROM Things WHERE ownerMemberID IS NULL AND ownerRole is NULL AND initialRoomID =");
    strcat(select, buf);
    strcat(select, ";");
    res = PQexec(conn, select);

    if (PQresultStatus(res) != PGRES_TUPLES_OK) {
      printf("3rd SQL ERROR\n");
      bad_exit(conn);
      return -1;
    }

    count += atoi(PQgetvalue(res, 0, 0));
    PQclear(res);

    strcpy(select, "");
    // Get room description
    strcat(select, "SELECT roomDescription FROM Rooms WHERE RoomID =");
    strcat(select, buf);
    strcat(select, ";");
    res = PQexec(conn, select);

    if (PQresultStatus(res) != PGRES_TUPLES_OK) {
      printf("4th SQL ERROR\n");
      bad_exit(conn);
      return -1;
    }

    printf("Room %d, %s, has %d in it.\n", theRoomID, PQgetvalue(res, 0, 0),count);
    PQclear(res);

    res = PQexec(conn, "COMMIT TRANSACTION");
    if (PQresultStatus(res) != PGRES_COMMAND_OK) {
      printf("SQL ERROR\n");
      bad_exit(conn);
      return -1;
    }
    PQclear(res);

    return 0;
}

/* Function: updateQueryCharacters:
 *  * ----------------------------
 *   * Parameters:  connection, result, and if it won
 *    * Checks if character or monster was defeated
 *        */
int updateQueryCharacter(PGconn *conn, PGresult *res, char* win) {
    int count = 0;
    char update[MAXSQLSTATEMENTSTRINGSIZE];
    PGresult *res2 = NULL;
    for (int i = 0;i < PQntuples(res);i++) {
        strcpy(update, "UPDATE Characters SET wasDefeated = ");
        strcat(update, win);
        strcat(update, " WHERE memberID = ");
        strcat(update, PQgetvalue(res, i, 0));
        strcat(update, " AND role = '");
        strcat(update, PQgetvalue(res, i, 1));
        strcat(update, "' AND wasDefeated != ");
        strcat(update, win);
        strcat(update, ";");
        res2 = PQexec(conn, update);
        if (PQresultStatus(res2) != PGRES_COMMAND_OK) {
            printf("error\n");
            bad_exit(conn);
            return -1;
        }
        count += atoi(PQcmdTuples(res2));
        PQclear(res2);
        strcpy(update, "");
    }
    res2 = NULL;
    return count;
}

/* Function: updateQueryMonster
 *  * ----------------------------
 *   * Parameters:  connection, result ptr, and if it won
 *    * Does same as above but with monster
 *        */
int updateQueryMonster(PGconn *conn, PGresult *res, char* win) {
    int count = 0;
    char update[MAXSQLSTATEMENTSTRINGSIZE];
    PGresult *res2 = NULL;
    for (int i = 0;i < PQntuples(res);i++) {
        strcpy(update, "UPDATE Monsters SET wasDefeated = ");
        strcat(update, win);
        strcat(update, " WHERE monsterID = ");
        strcat(update, PQgetvalue(res, i, 0));
        strcat(update, " AND wasDefeated != ");
        strcat(update, win);
        strcat(update, ";");
        res2 = PQexec(conn, update);
        if (PQresultStatus(res2) != PGRES_COMMAND_OK) {
            printf("error\n");
            bad_exit(conn);
            return -1;
        }
        count += atoi(PQcmdTuples(res2));
        PQclear(res2);
        strcpy(update, "");
    }
    res2 = NULL;
    return count;
}

/* Function: updateWasDefeated:
 *  * ----------------------------
 *   * Parameters:  connection, and a string, doCharactersOrMonsters, which should be 'C' or 'M'.
 *    * Updates the wasDefeated value of either characters or monsters (depending on value of
 *     * doCharactersOrMonsters) if that value is not correct.
 *      * Returns number of characters or monsters whose wasDefeated value was updated,
 *       * or -1 if doCharactersOrMonsters value is invalid.
 *        */
int updateWasDefeated(PGconn *conn, char *doCharactersOrMonsters)
{
    if (doCharactersOrMonsters[0] != 'C' && doCharactersOrMonsters[0] != 'M') {
        return -1;
    }
    PGresult *res = NULL;

    res = PQexec(conn, "BEGIN TRANSACTION ISOLATION LEVEL SERIALIZABLE");
    if (PQresultStatus(res) != PGRES_COMMAND_OK) {
      PQclear(res);
      res = PQexec(conn, "COMMIT TRANSACTION");
      PQclear(res);
      printf("SQL AT BEGIN ERROR\n");
      bad_exit(conn);
      return -1;
    }
    PQclear(res);

    char select[MAXSQLSTATEMENTSTRINGSIZE];
    int count = 0;
    if (doCharactersOrMonsters[0] == 'C') {
        // Finds character loser
        strcpy(select, "SELECT DISTINCT characterMemberID, characterRole FROM Battles WHERE characterBattlePoints < monsterBattlePoints;");
        res = PQexec(conn, select);

        if (PQresultStatus(res) != PGRES_TUPLES_OK) {
            printf("ha error\n");
            bad_exit(conn);
            return -1;
        }
        count += updateQueryCharacter(conn, res, "TRUE");
        PQclear(res);

        // Finds no battle loss
        strcpy(select, "");
        strcpy(select, "SELECT memberID, role FROM Characters EXCEPT SELECT DISTINCT characterMemberID, characterRole FROM Battles WHERE characterBattlePoints < monsterBattlePoints;");
        res = PQexec(conn, select);

        if (PQresultStatus(res) != PGRES_TUPLES_OK) {
            printf("error\n");
            bad_exit(conn);
            return -1;
        }

        count += updateQueryCharacter(conn, res, "FALSE");
        PQclear(res);
    } else {
        // Find monster loser
        strcpy(select, "SELECT DISTINCT monsterID FROM Battles WHERE characterBattlePoints > monsterBattlePoints;");
        res = PQexec(conn, select);

        if (PQresultStatus(res) != PGRES_TUPLES_OK) {
            bad_exit(conn);
            printf("error\n");
            return -1;
        }
        count += updateQueryMonster(conn, res, "TRUE");
        PQclear(res);

        // Find no battle loss
        strcpy(select, "");
        strcpy(select, "SELECT monsterID FROM Monsters EXCEPT SELECT DISTINCT monsterID FROM Battles WHERE characterBattlePoints > monsterBattlePoints;");
        res = PQexec(conn, select);

        if (PQresultStatus(res) != PGRES_TUPLES_OK) {
            bad_exit(conn);
            printf("error\n");
            return -1;
        }
        count += updateQueryMonster(conn, res, "FALSE");
        PQclear(res);
    }

    res = PQexec(conn, "COMMIT TRANSACTION");
    if (PQresultStatus(res) != PGRES_COMMAND_OK) {
      bad_exit(conn);
      printf("SQL ERROR\n");
      return -1;
    }
    PQclear(res);

    return count;
}

/* Function: increaseSomeThingCosts:
 *  * -------------------------------
 *   * Parameters:  connection, and an integer maxTotalIncrease, the maximum total increase that
 *    * it should make in the cost of some things.
 *     * Executes by invoking a Stored Function, increaseSomeThingCostsFunction, which
 *      * returns a negative value if there is an error, and otherwise returns the total
 *       * cost increase that was performed by the Stored Function.
 *        */

int increaseSomeThingCosts(PGconn *conn, int maxTotalIncrease)
{
    PGresult *res = NULL;
    char buf[MAXNUMBERSTRINGSIZE];
    snprintf(buf, 20, "%d", maxTotalIncrease);
    char select[MAXSQLSTATEMENTSTRINGSIZE] = "SELECT increaseSomeThingCostsFunction(";

    strcat(select, buf);
    strcat(select, ");");
    res = PQexec(conn, select);

    if (PQresultStatus(res) != PGRES_TUPLES_OK) {
        bad_exit(conn);
        printf("SQL ERROR\n");
        return -1;
    }
    int count = atoi(PQgetvalue(res, 0, 0));
    PQclear(res);
    res = NULL;
    return count;
}

/* Function: updateCheck:
 *  * -------------------------------
 *   * Parameters:  connection, result of update, and what letter being tested
 *    * Handles checks for valid result
 *        */

void updateCheck(PGconn *conn, int result, char *letter) {
    if (result >= 0) {
        printf("%d wasDefeated values were fixed for %s\n", result, letter);
    } else if (result == -1) {
        printf("Illegal value for doCharactersOrMonsters %s\n", letter);
    } else {
        printf("ERROR: %d bad value returned\n", result);
        bad_exit(conn);
    }
}

/* Function: increaseCheck:
 *  * -------------------------------
 *   * Parameters:  connection, maxTotalIncrease, and result of the increase
 *    * Handles checks for valid increase
 *        */
void increaseCheck(PGconn *conn, int maxTotalIncrease, int result) {
    if (result >= 0) {
        printf("Total increase for maxTotalIncrease %d is %d\n", maxTotalIncrease, result);
    } else {
        printf("ERROR: %d bad value returned\n", result);
        bad_exit(conn);
    }
}

int main(int argc, char **argv)
{
    PGconn *conn;
    int theResult;

    if (argc != 3)
    {
        fprintf(stderr, "Usage: ./runAdventureApplication <username> <password>\n");
        exit(EXIT_FAILURE);
    }

    char *userID = argv[1];
    char *pwd = argv[2];

    char conninfo[MAXCONNECTIONSTRINGSIZE] = "host=cse180-db.lt.ucsc.edu user=";
    strcat(conninfo, userID);
    strcat(conninfo, " password=");
    strcat(conninfo, pwd);

    /* Make a connection to the database */
    conn = PQconnectdb(conninfo);

    /* Check to see if the database connection was successfully made. */
    if (PQstatus(conn) != CONNECTION_OK)
    {
        fprintf(stderr, "Connection to database failed: %s\n",
                PQerrorMessage(conn));
        bad_exit(conn);
    }

    int result;
    
    /* Perform the calls to printNumberOfThingsInRoom listed in Section 6 of Lab4,
 *      * printing error message if there's an error.
 *           */
    
    /* Extra newline for readability */

    for (int i = 1; i <= 3;i++) {
        result = printNumberOfThingsInRoom(conn, i);
        if (result == -1) {
            printf("No room exists whose id is %d\n", i);
        } else if (result != 0) {
            printf("Error printNumberOfThingsinRoom bad return %d\n", result);
            bad_exit(conn);
        }
    }
    result = printNumberOfThingsInRoom(conn, 7);
    if (result == -1) {
        printf("No room exists whose id is 7\n");
    } else if (result != 0) {
        printf("Error printNumberOfThingsinRoom bad return %d\n", result);
        bad_exit(conn);
    }
    printf("\n");
    
    /* Perform the calls to updateWasDefeated listed in Section 6 of Lab4,
 *      * and print messages as described.
 *           */
    
    /* Extra newline for readability */

    char letter[2] = "C";
    result = updateWasDefeated(conn, letter);
    updateCheck(conn, result, letter);

    strcpy(letter, "M");
    result = updateWasDefeated(conn, letter);
    updateCheck(conn, result, letter);

    strcpy(letter, "X");
    result = updateWasDefeated(conn, letter);
    updateCheck(conn, result, letter);

    printf("\n");

    
    /* Perform the calls to increaseSomeThingCosts listed in Section 6 of Lab4,
 *      * and print messages as described.
 *           */
    int mti = 12;
    result = increaseSomeThingCosts(conn, mti);
    increaseCheck(conn, mti, result);

    mti = 500;
    result = increaseSomeThingCosts(conn, mti);
    increaseCheck(conn, mti, result);

    mti = 39;
    result = increaseSomeThingCosts(conn, mti);
    increaseCheck(conn, mti, result);

    mti = 1;
    result = increaseSomeThingCosts(conn, mti);
    increaseCheck(conn, mti, result);

    good_exit(conn);
    return 0;
}

