#include <iostream>    // For input-output
#include <fstream>     // For reading from a file
#include <queue>       // For using a queue
#include <string>      // For handling strings
#include <thread>      // For creating threads
#include <mutex>       // To avoid thread interference
#include <condition_variable> // To manage synchronization between producer and consumer
#include <map>         // For storing traffic light data
#include <algorithm>   // For sorting

using namespace std;

const int MAX_QUEUE_SIZE = 10;   // Set a small queue size to make it simple
const int DELAY = 1000; // 1-second delay to simulate real-time traffic data

// Struct to hold traffic data
struct TrafficData {
    string timestamp;
    string traffic_light_id;
    int cars_passed;
};

class TrafficQueue {
public: 
    queue<TrafficData> traffic_queue; // queue to hold traffic data
    mutex mtx;                        // To lock operations
    condition_variable cv;            // To coordinate producer and consumer
    int max_size;                     // Maximum size of the queue

    TrafficQueue(int size) : max_size(size) {}

    // Add traffic data to the queue
    void addTrafficData(const TrafficData& data) {
        unique_lock<mutex> lock(mtx);  // Lock the queue
        cv.wait(lock, [this]() { return traffic_queue.size() < max_size; }); // Wait if full
        traffic_queue.push(data);      // Add data to the queue
        cv.notify_one();              // Notify consumer that data is available
    }

    // Remove traffic data from the queue
    TrafficData getTrafficData() {
        unique_lock<mutex> lock(mtx);  // Lock the queue
        cv.wait(lock, [this]() { return !traffic_queue.empty(); }); // Wait if empty
        TrafficData data = traffic_queue.front(); // Get the front data
        traffic_queue.pop();           // Remove the data from the queue
        return data;
    }
};

// Producer function: Reads from file and adds traffic data to the queue
void producer(TrafficQueue& queue, const string& filename) {
    ifstream file(filename);  // Open the file
    string line;              // Store each line from the file

    // Read traffic data line by line
    while (getline(file, line)) {
        TrafficData data;
        size_t comma1 = line.find(", "); 
        size_t comma2 = line.find(", ", comma1 + 2); 
        data.timestamp = line.substr(0, comma1);
        data.traffic_light_id = line.substr(comma1 + 2, comma2 - comma1 - 2);
        data.cars_passed = stoi(line.substr(comma2 + 2));

        queue.addTrafficData(data);  // Add traffic data to the queue
        this_thread::sleep_for(chrono::milliseconds(DELAY)); // 1-second delay
    }

    file.close();
}

// Consumer function: Processes traffic data and keeps track of congestion
void consumer(TrafficQueue& queue) {
    map<string, int> traffic_count;  // To track cars passing each traffic light

    while (true) {
        TrafficData data = queue.getTrafficData();  // Get traffic data from queue
        traffic_count[data.traffic_light_id] += data.cars_passed; // Update car count

        // Create a vector from the map to sort by congestion
        vector<pair<string, int>> traffic_vector(traffic_count.begin(), traffic_count.end());

        // Sort by number of cars passed in descending order
        sort(traffic_vector.begin(), traffic_vector.end(), 
             [](const pair<string, int>& a, const pair<string, int>& b) {
                 return a.second > b.second; 
             });

        // Display the top 5 traffic lights
        cout << "\nTop 5 traffic lights:\n";
        for (size_t i = 0; i < traffic_vector.size() && i < 5; ++i) {
            cout << traffic_vector[i].first << ":- " << traffic_vector[i].second << " cars\n";
        }
        cout << "________________________________________________________________\n";
    }
}

int main() {
    TrafficQueue queue(MAX_QUEUE_SIZE); // Create queue with max size
    string filename = "traffic_data.txt"; // Input file name

    // Start producer and consumer threads
    thread producer_thread(producer, ref(queue), filename);
    thread consumer_thread(consumer, ref(queue));

    // Wait for both threads to finish
    producer_thread.join();
    consumer_thread.join();

    return 0;
}
