#include <iostream>
#include <fstream>
#include <sstream>
#include <unordered_map>
#include <vector>
#include <cmath>
#include <set>

using namespace std;

// --- Parameters ---
const int HZ = 5;        // Minimum trades per insider
const double HM = 2;   // Similarity threshold

// --- Date difference helper (naive but safe for YYYY-MM-DD) ---
int days_between(const string& date1, const string& date2) {
    int y1 = stoi(date1.substr(0, 4)), y2 = stoi(date2.substr(0, 4));
    int m1 = stoi(date1.substr(5, 2)), m2 = stoi(date2.substr(5, 2));
    int d1 = stoi(date1.substr(8, 2)), d2 = stoi(date2.substr(8, 2));
    return abs((y1 - y2) * 365 + (m1 - m2) * 30 + (d1 - d2));
}

// --- Linear decay weight ---
double time_weight(int d) {
    return (d <= 7) ? 1.0 - d / 7.0 : 0.0;
}

// --- Similarity function for same-direction trades ---
double compute_similarity(const vector<string>& buy1, const vector<string>& buy2,
                          const vector<string>& sell1, const vector<string>& sell2) {
    double bb = 0.0, ss = 0.0;

    for (const string& d1 : buy1)
        for (const string& d2 : buy2)
            bb += time_weight(days_between(d1, d2));

    for (const string& d1 : sell1)
        for (const string& d2 : sell2)
            ss += time_weight(days_between(d1, d2));

    double numerator = pow(bb, 2) + pow(ss, 2);
    double denominator = (buy1.size() * buy2.size()) + (sell1.size() * sell2.size());

    return (denominator > 0) ? numerator / denominator : 0.0;
}

int main() {
    string line, filename = "trades_by_day.csv";

    // Structure: company → insider → trade dates
    unordered_map<string, unordered_map<string, vector<string>>> buy_dates;
    unordered_map<string, unordered_map<string, vector<string>>> sell_dates;

    ifstream file(filename);
    getline(file, line); // skip header

    // --- Load and group trades ---
    while (getline(file, line)) {
        stringstream ss(line);
        string symbol, insider, action, date;

        getline(ss, symbol, ',');    // ISSUERTRADINGSYMBOL
        getline(ss, insider, ',');   // RPTOWNERNAME_lower
        getline(ss, action, ',');    // TRANS_ACQUIRED_DISP_CD
        getline(ss, date, ',');      // TRANS_DATE

        if (action == "A")
            buy_dates[symbol][insider].push_back(date);
        else if (action == "D")
            sell_dates[symbol][insider].push_back(date);
    }

    // --- Prepare output ---
    ofstream out("edges.csv");
    out << "source,target,company,similarity\n";

    set<string> all_nodes;
    int edge_count = 0;

    // --- Loop over companies ---
    for (auto& [symbol, buy_map] : buy_dates) {
        set<string> insiders;

        for (const auto& [name, _] : buy_map) insiders.insert(name);
        for (const auto& [name, _] : sell_dates[symbol]) insiders.insert(name);

        vector<string> insider_list(insiders.begin(), insiders.end());

        for (size_t i = 0; i < insider_list.size(); ++i) {
            for (size_t j = i + 1; j < insider_list.size(); ++j) {
                const string& i1 = insider_list[i];
                const string& i2 = insider_list[j];

                auto buy1 = buy_map[i1];
                auto buy2 = buy_map[i2];
                auto sell1 = sell_dates[symbol][i1];
                auto sell2 = sell_dates[symbol][i2];

                if (buy1.size() + sell1.size() < HZ || buy2.size() + sell2.size() < HZ)
                    continue;

                double sim = compute_similarity(buy1, buy2, sell1, sell2);
                if (sim >= HM) {
                    out << i1 << "," << i2 << "," << symbol << "," << sim << "\n";
                    all_nodes.insert(i1);
                    all_nodes.insert(i2);
                    edge_count++;
                }
            }
        }
    }

    // --- Final reporting ---
    cout << "✅ Network construction complete!\n";
    cout << "Similarity threshold (h_m): " << HM << "\n";
    cout << "Minimum trades per insider (h_z): " << HZ << "\n";
    cout << "Number of nodes: " << all_nodes.size() << "\n";
    cout << "Number of edges: " << edge_count << "\n";
    cout << "Edges written to: edges.csv\n";

    return 0;
}