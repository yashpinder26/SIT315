#include <mpi.h>
#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <map>
#include <algorithm>

using namespace std;

struct TrafficData {
    string timestamp;
    int light_id;
    int cars_passed;
};

int extractHour(const string& timestamp) {
    if (timestamp.length() < 2) return -1;
    try {
        return stoi(timestamp.substr(0, 2));
    } catch (...) {
        return -1;
    }
}

bool compareByCars(const pair<int, int>& a, const pair<int, int>& b) {
    return a.second > b.second;
}

int main(int argc, char** argv) {
    MPI_Init(&argc, &argv);

    int rank, size;
    const int TOP_N = 2;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    map<int, vector<TrafficData>> hourlyData;

    if (rank == 0) {
        ifstream file("traffic_data.txt");
        string line;
        while (getline(file, line)) {
            if (line.empty()) continue;

            stringstream ss(line);
            TrafficData data;
            ss >> data.timestamp >> data.light_id >> data.cars_passed;

            int hour = extractHour(data.timestamp);
            if (hour >= 0) {
                hourlyData[hour].push_back(data);
            }
        }

        for (int hour = 0; hour < 24; ++hour) {
            vector<TrafficData>& dataVec = hourlyData[hour];
            if (dataVec.empty()) continue;

            int target = (hour % (size - 1)) + 1;

            int count = dataVec.size();
            MPI_Send(&hour, 1, MPI_INT, target, 0, MPI_COMM_WORLD);
            MPI_Send(&count, 1, MPI_INT, target, 0, MPI_COMM_WORLD);

            for (const TrafficData& d : dataVec) {
                int ts_len = d.timestamp.length();
                MPI_Send(&ts_len, 1, MPI_INT, target, 0, MPI_COMM_WORLD);
                MPI_Send(d.timestamp.c_str(), ts_len, MPI_CHAR, target, 0, MPI_COMM_WORLD);
                MPI_Send(&d.light_id, 1, MPI_INT, target, 0, MPI_COMM_WORLD);
                MPI_Send(&d.cars_passed, 1, MPI_INT, target, 0, MPI_COMM_WORLD);
            }
        }

        for (int i = 1; i < size; ++i) {
            int stop_signal = -1;
            MPI_Send(&stop_signal, 1, MPI_INT, i, 0, MPI_COMM_WORLD);
        }

        cout << "\n=== Traffic Analysis Report ===\n";
        cout << "Top " << TOP_N << " busiest traffic lights for each hour:\n";
        cout << "--------------------------------------------\n";
        for (int i = 1; i < size; ++i) {
            while (true) {
                int has_data;
                MPI_Recv(&has_data, 1, MPI_INT, i, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                if (has_data == 0) break;

                int hour, result_count;
                MPI_Recv(&hour, 1, MPI_INT, i, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                MPI_Recv(&result_count, 1, MPI_INT, i, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

                for (int j = 0; j < result_count; ++j) {
                    int light_id, cars;
                    MPI_Recv(&light_id, 1, MPI_INT, i, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                    MPI_Recv(&cars, 1, MPI_INT, i, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

                    printf("At %02d:00 - Traffic Light %d handled %d cars.\n", hour, light_id, cars);
                }
            }
        }

    } else {
        while (true) {
            int hour;
            MPI_Recv(&hour, 1, MPI_INT, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            if (hour == -1) break;

            int count;
            MPI_Recv(&count, 1, MPI_INT, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

            map<int, int> lightCounts;

            for (int i = 0; i < count; ++i) {
                int ts_len;
                MPI_Recv(&ts_len, 1, MPI_INT, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

                char* ts_buf = new char[ts_len + 1];
                MPI_Recv(ts_buf, ts_len, MPI_CHAR, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                ts_buf[ts_len] = '\0';
                string ts(ts_buf);
                delete[] ts_buf;

                int light_id, cars;
                MPI_Recv(&light_id, 1, MPI_INT, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                MPI_Recv(&cars, 1, MPI_INT, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

                lightCounts[light_id] += cars;
            }

            vector<pair<int, int>> sortedLights(lightCounts.begin(), lightCounts.end());
            sort(sortedLights.begin(), sortedLights.end(), compareByCars);

            int send_count = min(TOP_N, (int)sortedLights.size());

            int has_data = 1;
            MPI_Send(&has_data, 1, MPI_INT, 0, 0, MPI_COMM_WORLD);
            MPI_Send(&hour, 1, MPI_INT, 0, 0, MPI_COMM_WORLD);
            MPI_Send(&send_count, 1, MPI_INT, 0, 0, MPI_COMM_WORLD);

            for (int i = 0; i < send_count; ++i) {
                MPI_Send(&sortedLights[i].first, 1, MPI_INT, 0, 0, MPI_COMM_WORLD);
                MPI_Send(&sortedLights[i].second, 1, MPI_INT, 0, 0, MPI_COMM_WORLD);
            }
        }

        int has_data = 0;
        MPI_Send(&has_data, 1, MPI_INT, 0, 0, MPI_COMM_WORLD);
    }

    MPI_Finalize();
    return 0;
}
