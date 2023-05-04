#include <iostream>
#include <fstream>
#include <vector>
#include <sstream>
#include <thread>
#include <string>
#include <mutex>
#include <atomic>
#include <Windows.h>
#include <iomanip>
#include <lmcons.h>

using namespace std;

string message;

atomic<bool> updated(false);

mutex messageMutex;

string fileName = "C:\\application\\sharedDrive\\chat_data.csv";

void clearConsole() {
        system("cls");
}

string getUserName() {
    wchar_t buffer[UNLEN + 1];
    DWORD size = UNLEN + 1;
    GetUserNameW(buffer, &size);
    wstring wideUsername(buffer);
    string username(wideUsername.begin(), wideUsername.end());
    return username;
}

string getCurrentTime() {
    auto now = chrono::system_clock::now();
    time_t timestamp = chrono::system_clock::to_time_t(now);
    ostringstream stream;
    stream << put_time(gmtime(&timestamp), "%Y-%m-%dT%H:%M:%SZ");
    return stream.str();
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
            
            ifstream inputFile(filename);
            inputFile.seekg(fileSize);
            string newEntry;
            while (getline(inputFile, newEntry)) {
                newEntries.push_back(newEntry);
            }
            inputFile.close();

            
            fileSize = newFileSize;

            updated.store(true);

            for (const auto& entry : newEntries) {
                std::cout << entry;
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
        bool inQuotes = false;
        stringstream cellStream;
        char c;
        
        while (ss.get(c)) {
            if (c == '\"') {
                inQuotes = !inQuotes;
            } else if (c == ',' && !inQuotes) {
                row.push_back(cellStream.str());
                cellStream.str("");
                cellStream.clear();
            } else {
                cellStream << c;
            }
        }

        row.push_back(cellStream.str());
        data.push_back(row);
    }
    inFile.close();
    return data;
}

void printMessagesAndUserInput(const string& user_input) {
    auto chatData = readCsvFile(fileName);
    clearConsole();

    const int usernameWidth = 10;
    const int messageWidth = 60;
    const int timestampWidth = 20;

    for (const auto& row : chatData) {

        std::cout << std::setw(usernameWidth) << std::left << row[0] << " ";

       
        std::cout << std::setw(messageWidth) << std::left << row[1] << " ";

        
        std::cout << std::setw(timestampWidth) << std::right << row[2] << std::endl;
    }

    std::cout << std::endl << user_input;
    std::cout.flush();
}

void printMessagesAndUserInput() {
    auto chatData = readCsvFile(fileName);
    clearConsole();

    const int usernameWidth = 10;
    const int messageWidth = 60;
    const int timestampWidth = 20;

    for (const auto& row : chatData) {
        
        std::cout << std::setw(usernameWidth) << std::left << row[0] << " ";

       
        std::cout << std::setw(messageWidth) << std::left << row[1] << " ";

        
        std::cout << std::setw(timestampWidth) << std::right << row[2] << std::endl;
    }

    std::cout << std::endl;
    std::cout.flush();
}

void appendMessageToCsv(const string& fileName, const string& message, const string& timestamp, const string& username) {
    ofstream outFile(fileName, ios_base::app);
    if (!outFile.is_open()) {
        cerr << "Failed to open the file: " << fileName << endl;
        exit(1);
    }

    string quote = "\"" + message + "\"";

    outFile << username << "," << quote << "," << timestamp << '\n';
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
            appendMessageToCsv(fileName, input, getCurrentTime(), getUserName());
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
    printMessagesAndUserInput();

    thread userInputThread(getUserInput, ref(message));
    thread timerThread(timerFunction);
    thread checkCSVThread(checkCSVFile, fileName);

    timerThread.join();
    userInputThread.join();
    checkCSVThread.join();
    

    return 0;
}