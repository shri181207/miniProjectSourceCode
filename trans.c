#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

// Optimized struct layout to reduce padding (Refactoring for Memory)
#pragma pack(push, 1)
struct clientData {
    unsigned int acctNum;
    char lastName[15];
    char firstName[10];
    double balance;
    unsigned int pin;       // Security PIN
};
#pragma pack(pop)

// Functional Decomposition: Prototypes for specific modular tasks
unsigned int enterChoice(void);
void textFile(FILE *readPtr);
void updateRecord(FILE *fPtr);
void newRecord(FILE *fPtr);
void deleteRecord(FILE *fPtr);
void transferFunds(FILE *fPtr);
void listAllAccounts(FILE *fPtr);

// Helper / Refactored functions
void initializeFile(const char* filename);
int validateAccountBounds(int acctNum); // Fixes logical bug in original code
void logTransaction(unsigned int acct, const char* type, double amount, double balance);
int authenticateAndLoadUser(FILE *fPtr, int acctNum, struct clientData *client);

int main(int argc, char *argv[]) {
    FILE *cfPtr;
    unsigned int choice;

    // Auto-create/initialize if missing
    if ((cfPtr = fopen("credit.dat", "rb+")) == NULL) {
        printf("credit.dat not found. Initializing new bank database...\n");
        initializeFile("credit.dat");
        if ((cfPtr = fopen("credit.dat", "rb+")) == NULL) {
            printf("Error creating file.\n");
            exit(-1);
        }
    }

    while ((choice = enterChoice()) != 7) {
        switch (choice) {
            case 1: textFile(cfPtr); break;
            case 2: updateRecord(cfPtr); break;
            case 3: newRecord(cfPtr); break;
            case 4: deleteRecord(cfPtr); break;
            case 5: transferFunds(cfPtr); break;
            case 6: listAllAccounts(cfPtr); break;
            default: puts("Incorrect choice"); break;
        }
    }
    
    printf("\nThank you for using the Global Minibank System. Goodbye!\n");
    fclose(cfPtr);
    return 0;
}

// -------------------------------------------------------------
// HELPER FUNCTIONS (Functional Decomposition)
// -------------------------------------------------------------

// Initializes the database to avoid unpredictable blank spots
void initializeFile(const char* filename) {
    FILE *fPtr = fopen(filename, "wb");
    if (!fPtr) return;
    struct clientData blankClient = {0, "", "", 0.0, 0};
    for (int i = 0; i < 100; i++) {
        fwrite(&blankClient, sizeof(struct clientData), 1, fPtr);
    }
    fclose(fPtr);
}

// LOGICAL ERROR FIX: Checks bounds so we don't access out-of-file memory offsets
int validateAccountBounds(int acctNum) {
    if (acctNum < 1 || acctNum > 100) {
        printf("=> ERROR: Invalid account! Must be between 1 and 100.\n");
        return 0; // false
    }
    return 1; // true
}

// Memory optimization: passing pointers instead of struct copies
int authenticateAndLoadUser(FILE *fPtr, int acctNum, struct clientData *client) {
    unsigned int pinInput;
    
    fseek(fPtr, (acctNum - 1) * sizeof(struct clientData), SEEK_SET);
    fread(client, sizeof(struct clientData), 1, fPtr);

    if (client->acctNum == 0) {
        printf("=> ERROR: Account #%d does not exist.\n", acctNum);
        return 0;
    }

    printf("Enter 4-digit PIN for Account %d: ", acctNum);
    scanf("%u", &pinInput);
    
    if (pinInput != client->pin) {
        printf("=> ERROR: Incorrect PIN! Access denied.\n");
        return 0;
    }
    return 1;
}

// Innovation Feature: Log Transactions (Security/Audit)
void logTransaction(unsigned int acct, const char* type, double amount, double balance) {
    FILE *logPtr = fopen("transactions_log.txt", "a");
    if (logPtr != NULL) {
        time_t t = time(NULL);
        struct tm tm = *localtime(&t);
        fprintf(logPtr, "[%04d-%02d-%02d %02d:%02d:%02d] Account: %-4d | Type: %-15s | Amount: %10.2f | New Balance: %10.2f\n",
                tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec,
                acct, type, amount, balance);
        fclose(logPtr);
    }
}

// -------------------------------------------------------------
// CORE FUNCTIONS 
// -------------------------------------------------------------

// Export to text file
void textFile(FILE *readPtr) {
    FILE *writePtr;
    struct clientData client = {0, "", "", 0.0, 0};

    if ((writePtr = fopen("accounts.txt", "w")) == NULL) {
        puts("File could not be opened.");
    } else {
        rewind(readPtr);
        fprintf(writePtr, "%-6s%-16s%-11s%10s\n", "Acct", "Last Name", "First Name", "Balance");
        while (!feof(readPtr)) {
            fread(&client, sizeof(struct clientData), 1, readPtr);
            if (client.acctNum != 0 && !feof(readPtr)) {
                fprintf(writePtr, "%-6d%-16s%-11s%10.2f\n", client.acctNum, client.lastName, client.firstName, client.balance);
            }
        }
        fclose(writePtr);
        printf("\n=> Successfully exported active accounts to 'accounts.txt'\n");
    }
}

// Print to screen
void listAllAccounts(FILE *readPtr) {
    struct clientData client = {0, "", "", 0.0, 0};
    rewind(readPtr);
    
    printf("\n--- ALL ACTIVE ACCOUNTS ---\n");
    printf("%-6s%-16s%-11s%10s\n", "Acct", "Last Name", "First Name", "Balance");
    printf("---------------------------------------------\n");
    
    // Performance optimization: read whole file block vs line by line stream checking
    while (fread(&client, sizeof(struct clientData), 1, readPtr)) {
        if (client.acctNum != 0) {
            printf("%-6d%-16s%-11s%10.2f\n", client.acctNum, client.lastName, client.firstName, client.balance);
        }
    }
    printf("---------------------------------------------\n");
    
    // Clear eof state so next file ops work seamlessly
    clearerr(readPtr); 
}

// Deposit or withdraw
void updateRecord(FILE *fPtr) {
    int account;
    double transaction;
    struct clientData client;

    printf("Enter account to update (1 - 100): ");
    scanf("%d", &account);
    
    if (!validateAccountBounds(account)) return;
    if (!authenticateAndLoadUser(fPtr, account, &client)) return;

    printf("\nName: %s %s | Current Balance: %.2f\n", client.firstName, client.lastName, client.balance);
    printf("Enter charge (-) to withdraw, or payment (+) to deposit: ");
    scanf("%lf", &transaction);
    
    // Simple Error Handling: Overdraft bounds
    if (transaction < 0 && (client.balance + transaction < 0)) {
        printf("=> ERROR: Insufficient funds. Balance cannot drop below 0.\n");
    } else {
        client.balance += transaction;
        printf("=> SUCCESS: New Balance: %.2f\n", client.balance);
        
        const char* tType = (transaction >= 0) ? "DEPOSIT" : "WITHDRAWAL";
        logTransaction(client.acctNum, tType, transaction, client.balance);

        fseek(fPtr, -(long)sizeof(struct clientData), SEEK_CUR);
        fwrite(&client, sizeof(struct clientData), 1, fPtr);
    }
}

// Transfer funds feature
void transferFunds(FILE *fPtr) {
    int acctFrom, acctTo;
    double amount;
    struct clientData clientFrom, clientTo;

    printf("Enter SENDER account number (1 - 100): ");
    scanf("%d", &acctFrom);
    if (!validateAccountBounds(acctFrom)) return;
    if (!authenticateAndLoadUser(fPtr, acctFrom, &clientFrom)) return;

    printf("Enter RECEIVER account number (1 - 100): ");
    scanf("%d", &acctTo);
    if (!validateAccountBounds(acctTo)) return;

    fseek(fPtr, (acctTo - 1) * sizeof(struct clientData), SEEK_SET);
    fread(&clientTo, sizeof(struct clientData), 1, fPtr);
    if (clientTo.acctNum == 0) {
        printf("=> ERROR: Receiver account #%d does not exist.\n", acctTo);
        return;
    }

    printf("Enter transfer amount: ");
    scanf("%lf", &amount);

    if (amount <= 0) {
        printf("=> ERROR: Transfer amount must be positive.\n");
        return;
    }
    if (clientFrom.balance - amount < 0) {
        printf("=> ERROR: Insufficient funds in Sender account.\n");
        return;
    }

    clientFrom.balance -= amount;
    clientTo.balance += amount;

    // Log & Save
    fseek(fPtr, (acctFrom - 1) * sizeof(struct clientData), SEEK_SET);
    fwrite(&clientFrom, sizeof(struct clientData), 1, fPtr);
    logTransaction(acctFrom, "TRANSFER OUT", -amount, clientFrom.balance);

    fseek(fPtr, (acctTo - 1) * sizeof(struct clientData), SEEK_SET);
    fwrite(&clientTo, sizeof(struct clientData), 1, fPtr);
    logTransaction(acctTo, "TRANSFER IN", amount, clientTo.balance);

    printf("=> SUCCESS: Transferred %.2f from Acct %d to Acct %d.\n", amount, acctFrom, acctTo);
}

// Delete logically
void deleteRecord(FILE *fPtr) {
    int accountNum;
    struct clientData client;
    struct clientData blankClient = {0, "", "", 0.0, 0};

    printf("Enter account number to delete (1 - 100): ");
    scanf("%d", &accountNum);
    if (!validateAccountBounds(accountNum)) return;
    if (!authenticateAndLoadUser(fPtr, accountNum, &client)) return;

    // Use absolute positioning to erase record (Refactoring for Memory/Speed)
    fseek(fPtr, (accountNum - 1) * sizeof(struct clientData), SEEK_SET);
    if (fwrite(&blankClient, sizeof(struct clientData), 1, fPtr) == 1) {
        printf("=> SUCCESS: Account %d successfully closed and deleted.\n", accountNum);
        logTransaction(accountNum, "ACCOUNT CLOSED", 0.0, 0.0);
    }
}

// Create new 
void newRecord(FILE *fPtr) {
    struct clientData client;
    int accountNum;

    printf("Enter new account number (1 - 100): ");
    scanf("%d", &accountNum);
    if (!validateAccountBounds(accountNum)) return;
    
    fseek(fPtr, (accountNum - 1) * sizeof(struct clientData), SEEK_SET);
    fread(&client, sizeof(struct clientData), 1, fPtr);

    if (client.acctNum != 0) {
        printf("=> ERROR: Account #%d already contains information.\n", client.acctNum);
    } else {
        printf("Enter lastname, firstname, initial balance:\n? ");
        scanf("%14s%9s%lf", client.lastName, client.firstName, &client.balance);
        
        printf("Create a 4-digit PIN for this account:\n? ");
        scanf("%u", &client.pin);

        client.acctNum = accountNum;
        
        fseek(fPtr, (client.acctNum - 1) * sizeof(struct clientData), SEEK_SET);
        fwrite(&client, sizeof(struct clientData), 1, fPtr);
        
        printf("\n--- Account Created Successfully ---\n");
        printf("Acct: %d\nName: %s %s\nBalance: %.2f\nPIN: %u (Keep this secret!)\n", 
               client.acctNum, client.firstName, client.lastName, client.balance, client.pin);
               
        logTransaction(client.acctNum, "ACCOUNT OPEN", client.balance, client.balance);
    }
}

unsigned int enterChoice(void) {
    unsigned int menuChoice;
    printf("\n=== GLOBAL MINIBANK SYSTEM ===\n");
    printf("1 - Export all accounts to \"accounts.txt\"\n");
    printf("2 - Update an account (Deposit / Withdraw)\n");
    printf("3 - Open a new account\n");
    printf("4 - Close / Delete an account\n");
    printf("5 - Transfer funds between accounts\n");
    printf("6 - View all active accounts on screen\n");
    printf("7 - End program\n");
    printf("Choice: ");
    
    if (scanf("%u", &menuChoice) != 1) { 
        // Handle junk input 
        while (getchar() != '\n'); 
        return 0; 
    }
    return menuChoice;
}