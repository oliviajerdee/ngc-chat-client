#include <iostream>
#include <fstream>
#include <vector>
#include <sstream>
#include <thread>
#include <string>
#include <mutex>
#include <atomic>

using namespace std;

string message;

atomic<bool> updated(false);
string timestamp = "2023-05-03T12:00:00Z";
string username = "Vincent Candelario";

mutex messageMutex;

string fileName = "/home/jollibeeworker/coding/projects/ngc-chat-client/application/sharedDrive/chat_data.csv";

void clearConsole() {
        system("clear");
        //cout << "\033[2J\033[1;1H";
}

void checkCSVFile(const string& filename) {
    vector<string> newEntries;
    ifstream inputFile(filename);
    inputFile.seekg(0, ios::end);
    size_t fileSize = inputFile.tellg();
    inputFile.close();

    while (true) {
        
        ifstream inputFile(filename);
        inputFile.seekg(0, ios::end);
        size_t newFileSize = inputFile.tellg();
        inputFile.close();

        if (newFileSize > fileSize) {
            // Read the new entries from the file
            ifstream inputFile(filename);
            inputFile.seekg(fileSize);
            string newEntry;
            while (getline(inputFile, newEntry)) {
                newEntries.push_back(newEntry);
            }
            inputFile.close();

            // Update the file size
            fileSize = newFileSize;

            updated.store(true);

            for (const auto& entry : newEntries) {
                cout << entry;
            }

            newEntries.clear();

        }

        
    
    }

}

vector<vector<string>> readCsvFile(const string& fileName) {
    ifstream inFile(fileName);
    if (!inFile.is_open()) {
        cerr << "Failed to open the file: " << fileName << endl;
        exit(1);
    }

    vector<vector<string>> data;
    string line;
    while (getline(inFile, line)) {
        stringstream ss(line);
        vector<string> row;
        string cell;
        while (getline(ss, cell, ',')) {
            row.push_back(cell);
        }
        data.push_back(row);
    }
    inFile.close();
    return data;
}

void printMessagesAndUserInput(const string& user_input) {
    auto chatData = readCsvFile(fileName);
    clearConsole();
    for (const auto& row : chatData) {
        for (const auto& cell : row) {
            cout << cell << " ";
        }
        cout << endl;
    }
    cout << endl;
    cout << user_input; // Print the current user input
    cout.flush();
}

void appendMessageToCsv(const string& fileName, const string& message, const string& timestamp, const string& username) {
    ofstream outFile(fileName, ios_base::app);
    if (!outFile.is_open()) {
        cerr << "Failed to open the file: " << fileName << endl;
        exit(1);
    }

    outFile << message << "," << timestamp << "," << username << '\n';
    outFile.close();
}

void writeCsvFile(const string& fileName, const vector<vector<string>>& data) {
    ofstream outFile(fileName);
    if (!outFile.is_open()) {
        cerr << "Failed to open the file: " << fileName << endl;
        exit(1);
    }

    for (const auto& row : data) {
        for (size_t i = 0; i < row.size(); ++i) {
            outFile << row[i];
            if (i + 1 != row.size()) {
                outFile << ',';
            }
        }
        outFile << '\n';
    }
    outFile.close();
}

void timerFunction() {
    string current_user_input;
    while (true) {
        {
            lock_guard<mutex> lock(messageMutex);
            current_user_input = message;
        }

        if (updated.load()) {
            printMessagesAndUserInput(current_user_input);
            updated.store(false);
        }

        
        this_thread::sleep_for(chrono::milliseconds(100));
    }
}

void getUserInput(string& message) {
    string input;
    char ch;

    while (true) {
        ch = getchar();
        if (ch == '\n') {
            {
                lock_guard<mutex> lock(messageMutex);
                message = input;
            }
            appendMessageToCsv(fileName, input, timestamp, username);
            input.clear();
            {
                lock_guard<mutex> lock(messageMutex);
                message = "";
            }
        } else {
            input += ch;
        }
    }
}

int main() {
    clearConsole();

    thread userInputThread(getUserInput, ref(message));
    thread timerThread(timerFunction);
    thread checkCSVThread(checkCSVFile, fileName);

    timerThread.join();
    userInputThread.join();
    checkCSVThread.join();
    

    return 0;
}