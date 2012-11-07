/*
	David Steinberg
	Operating Systems
	Project 1, Part 2
	October 14, 2012
*/

/////////////////////////////////////
//		DEFINE / INCLUDES
/////////////////////////////////////

// Needed to use yield() on Unix machines
#define _GLIBCXX_USE_SCHED_YIELD
// Needed for sleep_for() on Unix machines
#define _GLIBCXX_USE_NANOSLEEP

#include <thread>
#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <string>
#include <chrono> 
#include <ctime>
#include <cmath>
#include <cstdlib>
#include <cctype>
#include <mutex>
#include <condition_variable>

using namespace std;

/////////////////////////////////////
//		GLOBALS
/////////////////////////////////////

// Constants used for size of the lane.
const double
	laneWidth = 10.0, laneLength = 100.0;

/*
	These are used to track:
		How many of each direction are currently crossing.
		The last ID numbers of each direction's crossers.
		How many cars have crossed in each direction in one crossing period.
		Whether a car is still crossing the road.
		The running total time of the program, for sleeping threads.
*/
int
	pedCount = 0, carWestCount = 0, carEastCount = 0,
	pedLast = 0, carWestLast = 0, carEastLast = 0,
	carWestTotal = 0, carEastTotal = 0,
	timeSinceDirStarted = 0, timeDirCanStillGo = 0,
	totalTime = 0;

// Used to which directions are waiting and if two cars have gone (so pedestrians should).
bool
	pedWaiting = false, carWestWaiting = false, carEastWaiting = false, maxCarsForPedReached = false,
	DEBUGGING = false; // Used to print out when cars block at the road and why.

// Used to know which direction arrived first after another and which went last.
char
	PNotifyNext = 'n', WNotifyNext = 'n', ENotifyNext = 'n', lastToGo = 'n';

// The following are used to lock and release the road upon use and control the directions' trying to go.
mutex
	lane;
condition_variable_any
	pedCV, carWestCV, carEastCV;

/////////////////////////////////////
//		SLEEP FUNCTION
/////////////////////////////////////

// Sleep for the given number of seconds
void sleep (int s) {
	this_thread::sleep_for(chrono::milliseconds(s));
}

/////////////////////////////////////
//		PEDESTRIAN FUNCTION
/////////////////////////////////////

void pedestrian (int timeSinceLastCrosser, int idNum, int speed, int totalTime) {

	// Sleep until it is the actual arrival time
	sleep (totalTime);

	// The seconds after the previous participant arrived are added to the time elapsed from a direction's start.
	timeSinceDirStarted += timeSinceLastCrosser;

	// Calculate how long the pedestrian will take to cross the street. Note: this is using width.
	int timeToCross = ceil(laneWidth/speed);

	// Zone Entering
	lane.lock();
	pedWaiting = true;
	
	/*
		Verify the following are not true to allow the pedestrian to enter:
			It is the first pedestrian remaining.
			There are no cars going in either direction.
			Pedestrian is the next direction to go.
	*/
	while (
		idNum != pedLast + 1
		|| carWestCount > 0
		|| carEastCount > 0
		|| (lastToGo == 'W' && WNotifyNext != 'P')
		|| (lastToGo == 'E' && ENotifyNext != 'P')
	) {
		if (DEBUGGING) { // Will print that a pedestrian has blocked and why.
			cout << '\t' << 'P' << idNum << " blocks : ";
			if (idNum != pedLast + 1)
				cout << "idNum != pedLast + 1" << endl;
			else if (carWestCount > 0)
				cout << "carWestCount (" << carWestCount << ") > 0" << endl;
			else if (carEastCount > 0)
				cout << "carEastCount (" << carEastCount << ") > 0" << endl;
			else if (lastToGo == 'W' && WNotifyNext != 'P')
				cout << "lastToGo == 'W' && WNotifyNext != 'P'" << endl;
			else if (lastToGo == 'E' && ENotifyNext != 'P')
				cout << "lastToGo == 'E' && ENotifyNext != 'P'" << endl;
		}

		// Let both other directions know to let pedestrians go next if they do not have a next already.
		if (WNotifyNext == 'n') WNotifyNext = 'P';
		if (ENotifyNext == 'n') ENotifyNext = 'P';
		
		pedCV.wait(lane);
	}

	lastToGo = 'P';
	maxCarsForPedReached = false;
	carWestTotal = 0;
	carEastTotal = 0;
	pedWaiting = false;

	pedCount++;
	pedLast++;

	// Let both other directions know that pedestrians have gone.
	if (WNotifyNext == 'P') WNotifyNext = 'n';
	if (ENotifyNext == 'P') ENotifyNext = 'n';

	cout << 'P' << idNum << " entering construction" << endl;
	lane.unlock();
	
	// Zone Crossing
	sleep(timeToCross);
		
	// Zone Exiting
	lane.lock();
	cout << 'P' << idNum << " exiting construction" << endl;

	pedCount--;

	// Alert all directions that the pedestrian has crossed.
	pedCV.notify_all();
	carWestCV.notify_all();
	carEastCV.notify_all();

	lane.unlock();

}

/////////////////////////////////////
//		CAR WEST FUNCTION
/////////////////////////////////////

void carWest (int timeSinceLastCrosser, int idNum, int speed, int totalTime) {

	// Sleep until it is the actual arrival time
	sleep (totalTime);

	// The seconds after the previous participant arrived are added to the time elapsed from a direction's start.
	timeSinceDirStarted += timeSinceLastCrosser;

	// Calculate how long the car will take to cross the street. Note: this is using length.
	int timeToCross = ceil(laneLength/speed);

	// Zone Entering
	lane.lock();
	carWestWaiting = true;

	/*
		Verify the following are not true to allow the car to enter:
			It is the first car from the west remaining.
			There are no pedestrians or cars from the east going.
			There are not pedestrians waiting more than two cars.
			There are not cars from the east waiting more than two cars from the west.
			West is the next direction to go.
	*/
	while (
		idNum != carWestLast + 1
		|| pedCount > 0
		|| carEastCount > 0
		|| (pedWaiting && maxCarsForPedReached)
		|| (carEastWaiting && carWestTotal > 1)
		|| (lastToGo == 'P' && PNotifyNext != 'W')
		|| (lastToGo == 'E' && ENotifyNext != 'W')
	) {
		if (DEBUGGING) { // Will print that a west car has blocked and why.
			cout << '\t' << 'W' << idNum << " blocks : ";
			if (idNum != carWestLast + 1)
				cout << "idNum != carWestLast + 1" << endl;
			else if (pedCount > 0)
				cout << "pedCount (" << pedCount << ") > 0" << endl;
			else if (carEastCount > 0)
				cout << "carEastCount (" << carEastCount << ") > 0" << endl;
			else if (pedWaiting && maxCarsForPedReached)
				cout << "pedWaiting && maxCarsForPedReached" << endl;
			else if (carEastWaiting && carWestTotal > 1)
				cout << "carEastWaiting && carWestTotal > 1" << endl;
			else if (lastToGo == 'P' && PNotifyNext != 'W')
				cout << "lastToGo == 'P' && PNotifyNext != 'W'" << endl;
			else if (lastToGo == 'E' && ENotifyNext != 'W')
				cout << "lastToGo == 'E' && ENotifyNext != 'W'" << endl;
		}

		// Let both other directions know to let west cars go next if they do not have a next already.
		if (PNotifyNext == 'n') PNotifyNext = 'W';
		if (ENotifyNext == 'n') ENotifyNext = 'W';

		carWestCV.wait(lane);
	}

	lastToGo = 'W';
	cout << 'W' << idNum << " entering construction" << endl;
	carWestWaiting = false;
	carWestCount++;
	carWestTotal++;
	carWestLast++;
	carEastTotal = 0;
	
	if (carWestCount + carEastCount >= 2)
		maxCarsForPedReached = true;

	// Unless it will take longer to cross, make the car sleep for as long as the car before it.
	int timeToSleep = 0;
	if (carWestCount == 1) {
		timeDirCanStillGo = timeToSleep = timeToCross;
	}
	else {
		if (timeToCross < timeDirCanStillGo)
			timeToSleep = timeDirCanStillGo;
		else
			timeToSleep = timeToCross;
		// This sets the total amount of time west cars have left to enter.
		timeDirCanStillGo -= timeSinceDirStarted += timeToCross;
	}

	// Let both other directions know that west cars have gone.
	if (PNotifyNext == 'W') PNotifyNext = 'n';
	if (ENotifyNext == 'W') ENotifyNext = 'n';

	lane.unlock();
	
	// Zone Crossing
	sleep(timeToSleep);
	
	// Zone Exiting
	lane.lock();
	cout << 'W' << idNum << " exiting construction" << endl;
	carWestCount--;

	// If this was the last car to exit, reset the time variables for the direction.
	if (carWestCount == 0) {
		timeSinceDirStarted = 0;
		timeDirCanStillGo = 0;
	}

	// Alert all directions that the car has crossed.
	carWestCV.notify_all();
	pedCV.notify_all();
	carEastCV.notify_all();
	
	lane.unlock();

}

/////////////////////////////////////
//		CAR EAST FUNCTION
/////////////////////////////////////

void carEast (int timeSinceLastCrosser, int idNum, int speed, int totalTime) {

	// Sleep until it is the actual arrival time
	sleep (totalTime);

	// The seconds after the previous participant arrived are added to the time elapsed from a direction's start.
	timeSinceDirStarted += timeSinceLastCrosser;

	// Calculate how long the east car will take to cross the street. Note: this is using length.
	int timeToCross = ceil(laneLength/speed);

	// Zone Entering
	lane.lock();
	carEastWaiting = true;

	/*
		Verify the following are not true to allow the car to enter:
			It is the first car from the east remaining.
			There are no pedestrians or cars from the west going.
			There are not pedestrians waiting more than two cars.
			There are not cars from the west waiting more than two cars from the east.
			East is the next direction to go.
	*/
	while (
		idNum != carEastLast + 1
		|| pedCount > 0
		|| carWestCount > 0
		|| (pedWaiting && maxCarsForPedReached)
		|| (carWestWaiting && carEastTotal > 1)
		|| (lastToGo == 'P' && PNotifyNext != 'E')
		|| (lastToGo == 'W' && WNotifyNext != 'E')
	) {
		if (DEBUGGING) { // Will print that a west car has blocked and why.
			cout << '\t' << 'E' << idNum << " blocks : ";
			if (idNum != carEastLast + 1)
				cout << "idNum != carEastLast + 1" << endl;
			else if (pedCount > 0)
				cout << "pedCount (" << pedCount << ") > 0" << endl;
			else if (carWestCount > 0)
				cout << "carWestCount (" << carWestCount << ") > 0" << endl;
			else if (pedWaiting && maxCarsForPedReached)
				cout << "pedWaiting && maxCarsForPedReached" << endl;
			else if (carWestWaiting && carEastTotal > 1)
				cout << "carWestWaiting && carEastTotal > 1" << endl;
			else if (lastToGo == 'P' && PNotifyNext != 'E')
				cout << "lastToGo == 'P' && PNotifyNext != 'E'" << endl;
			else if (lastToGo == 'W' && WNotifyNext != 'E')
				cout << "lastToGo == 'W' && WNotifyNext != 'E'" << endl;
		}

		// Let both other directions know to let east cars go next if they do not have a next already.
		if (PNotifyNext == 'n') PNotifyNext = 'E';
		if (WNotifyNext == 'n') WNotifyNext = 'E';

		carEastCV.wait(lane);
	}

	lastToGo = 'E';
	carEastWaiting = false;
	cout << 'E' << idNum << " entering construction" << endl;
	carEastCount++;
	carEastTotal++;
	carEastLast++;
	carWestTotal = 0;

	if (carWestCount + carEastCount >= 2)
		maxCarsForPedReached = true;

	// Unless it will take longer to cross, make the car sleep for as long as the car before it.
	int timeToSleep = 0;
	if (carEastCount == 1) {
		timeDirCanStillGo = timeToSleep = timeToCross;
	}
	else {
		if (timeToCross < timeDirCanStillGo)
			timeToSleep = timeDirCanStillGo;
		else
			timeToSleep = timeToCross;
		// This sets the total amount of time east cars have left to enter.
		timeDirCanStillGo -= timeSinceDirStarted += timeToCross;
	}

	// Let both other directions know that east cars have gone.
	if (PNotifyNext == 'E') PNotifyNext = 'n';
	if (WNotifyNext == 'E') WNotifyNext = 'n';

	lane.unlock();
	
	// Zone Crossing
	sleep(timeToSleep);
	
	// Zone Exiting
	lane.lock();
	cout << 'E' << idNum << " exiting construction" << endl;
	carEastCount--;

	// If this was the last car to exit, reset the time variables for the direction.
	if (carEastCount == 0) {
		timeSinceDirStarted = 0;
		timeDirCanStillGo = 0;
		maxCarsForPedReached = false;
	}

	// Alert all directions that the car has crossed.
	carEastCV.notify_all();
	pedCV.notify_all();
	carWestCV.notify_all();
	
	lane.unlock();

}

/////////////////////////////////////
//		MAIN FUNCTION
/////////////////////////////////////

int main (int argc, char ** argv) {

	// Argument check
	if (argc != 2) {
		cerr << "Usage : " << argv[0] << " <fileName>" << endl;
		exit(1);
	}

	// Three vectors to hold each directions' threads
	vector<thread>
		pedestrians, carWests, carEasts;

	string
		line, id;
	int
		timeSinceLastCrosser, idNum, speed;

	// Open file
	ifstream in;
	in.open(argv[1]); // phrase.txt

	// Read all lines in
	while (!in.eof()) {
		getline(in,line);

		// Catch empty lines at file end
		if (line.length() == 0 || !isdigit(line[0]))
			break;

		// Break line into proper variables
		stringstream lineStream(line);
		lineStream >> timeSinceLastCrosser >> id >> speed;
		
		stringstream idStream(id.substr(1));
		idStream >> idNum;
		
		totalTime += timeSinceLastCrosser;

		// Create a thread for the proper vector with the proper function
		switch (id[0]) {
			case 'P' :
				pedestrians.push_back(thread(pedestrian, timeSinceLastCrosser, idNum, speed, totalTime));
				break;
			case 'W' :
				carWests.push_back(thread(carWest, timeSinceLastCrosser, idNum, speed, totalTime));
				break;
			case 'E' :
				carEasts.push_back(thread(carEast, timeSinceLastCrosser, idNum, speed, totalTime));
				break;
			default :
				cout << "Invalid entry : " << line << endl;
		}
	}

	// Close file
	in.close();

	// Make sure program does not finish before threads do
	for (thread& t : pedestrians) t.join();
	for (thread& t : carWests)    t.join();
	for (thread& t : carEasts)    t.join();

	// Exit normally
	return 0;

}

